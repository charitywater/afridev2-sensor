/** 
 * @file msgOtaUpgrade.c
 * \n Source File
 * \n Cascade MSP430 Firmware
 * 
 * \brief Manage the details of processing OTA firmware upgrade 
 * message. Runs the upgrade OTA state machine to retrieve new
 * firmware and burn it into the second image flash location.
 */
#include "outpour.h"
#include "linkAddr.h"

/***************************
 * Module Data Definitions
 **************************/

/**
 * \def OTA_MODEM_ERROR_RETRY_MAX
 * \brief Specify the max number of times to retry sending a 
 *        modem command if an error occurs.
 */
#define OTA_MODEM_ERROR_RETRY_MAX ((uint8_t)3)

/**
 * \def OTA_UPDATE_MSG_HEADER_SIZE
 * \brief Specify information about the upgrade message.  The 
 *        header portion of the message contains 8 bytes.
 */
#define OTA_UPDATE_MSG_HEADER_SIZE ((uint8_t)8)
/**
 * \def OTA_UPDATE_SECTION_HEADER_SIZE
 * \brief Specify information about the upgrade message. After 
 *        the header comes a section description header.  This
 *        contains information on a continuous section of code
 *        that needs to be programmed into flash.
 */
#define OTA_UPDATE_SECTION_HEADER_SIZE ((uint8_t)8)
/**
 * \def FLASH_UPGRADE_SECTION_START
 * \brief Specify information about the upgrade message.  The 
 *        section description begins with an 0xA5.
 */
#define FLASH_UPGRADE_SECTION_START ((uint8_t)0xA5)

/**
 * \typedef modemBatchCmdType_t
 * \brief Specify the modem batch command to prepare.  Currently 
 *        there is only one command type.
 */
typedef enum modemBatchCmdType_e {
    MODEM_BATCH_CMD_GET_OTA_PARTIAL,
} modemBatchCmdType_t;

/**
 * \typedef otaUpState_t
 * \brief Specify the states for retrieving an OTA msg from the 
 *        modem.
 */
typedef enum otaUpState_e {
    OTA_UP_STATE_IDLE,
    OTA_UP_STATE_SEND_OTA_CMD_PHASE1,
    OTA_UP_STATE_OTA_CMD_PHASE1_WAIT,
    OTA_UP_STATE_PROCESS_OTA_CMD_PHASE1,
    OTA_UP_STATE_DONE,
} otaUpState_t;

/**
 * \typedef otaFlashState_t
 * \brief Specify the states for writing and erasing flash with 
 *        new firmware.
 */
typedef enum otaFlashState_e {
    OTA_FLASH_STATE_GET_SECTION_INFO,
    OTA_FLASH_STATE_ERASE_SECTION_DATA,
    OTA_FLASH_STATE_WRITE_SECTION_DATA,
    OTA_FLASH_STATE_VERIFY_SECTION_DATA,
} otaFlashState_t;

/**
 * \typedef fwUpdateErrNum_t
 * \brief Specify the error codes that identify what step the fw
 *        upgrade failed at.
 */
typedef enum fwUpgradeErrorCode_e {
    FW_UP_ERR_NONE = 0,
    FW_UP_ERR_MODEM = -1,
    FW_UP_ERR_SECTION_HEADER = -2,
    FW_UP_ERR_PARAMETER = -3,
    FW_UP_ERR_CRC = -4,
    FW_UP_ERR_TIMEOUT = -5,
} fwUpdateErrNum_t;

/**
 * \typedef otaUpData_t
 * \brief Define a container to store data specific to 
 *        retrieving OTA messages from the modem.
 */
typedef struct otaUpData_s {
    bool active;                                           /**< Flag to indicate upgrade state machine is running */
    otaUpState_t otaUpState;                               /**< current state */
    modemCmdWriteData_t cmdWrite;                          /**< A modem write cmd object */
    uint16_t modemRequestLength;                           /**< Specify how much data to get from the modem */
    uint16_t modemRequestOffset;                           /**< Specify how much data to get from the modem */
    otaFlashState_t otaFlashState;                         /**< current state of flash state machine */
    uint16_t sectionStartAddrP;                            /**< Current upgrade section start address */
    uint16_t sectionDataLength;                            /**< Current upgrade section total data length */
    uint16_t sectionCrc16;                                 /**< Current section CRC16 value */
    uint16_t sectionDataRemaining;                         /**< Count in bytes of section data remaining to retrieve from modem */
    uint8_t *sectionWriteAddrP;                            /**< Where in flash to write the data */
    fwUpdateResult_t fwUpdateResult;                       /**< result for last flash state machine processing */
    fwUpdateErrNum_t fwUpdateErrNum;                       /**< If upgrade failure, identify what step it failed at */
    uint8_t modemRetryCount;                               /**< If modem error, how many retries we have performed */
    bool exitModemProcessing;                              /**< Flag to indicate we need to exit state machine processing */
    uint16_t lastCalcCrc16;                                /**< Save last calculated CRC16 value */
} otaUpData_t;

/****************************
 * Module Data Declarations
 ***************************/

/**
* \var otaUpData
* \brief Declare a data object to "house" data for this module.
*/
otaUpData_t otaUpData;

/*************************
 * Module Prototypes
 ************************/
static void otaUpgrade_modemStateMachine(void);
static void otaUpgrade_flashStateMachine(void);
static bool otaUpgrade_processSectionInfo(void);
static bool otaUpgrade_eraseSection(void);
static bool otaUpgrade_writeSectionData(void);
static bool otaUpgrade_verifySection(void);

/***************************
 * Module Public Functions
 **************************/

/**
* \brief Run the otaUpgrade state machine processing for
*        retrieving the firmware upgrade message from the modem
*        and writing the firmware into the second image location
*        in flash.  In the future, the Bootloader will be used
*        to move the firmware from the second image location
*        into the main location.  The steps include:
*
* \li Retrieve and check firmware upgrade message header
* \li Erase flash 
* \li Retrieve data from the modem in chunks and burn into flash
* \li Verify flash against CRC16 received in message
*
* \note This function will run the otaUpgrade state machine
*  until the firmware is completely collected from the modem.
*  This model is very different than standard interaction with
*  the modem elsewhere in the code in that we are not exiting
*  this loop until all the data has been retrieved from the
*  modem and burned into flash or an error occurs.  This will
*  hog the system while in operation. But otherwise it takes too
*  long to retrieve the firmware if using the standard state
*  machine model where it only runs once every second.
*
* @return fwUpdateResult_t Returns the status result of the
*         firmware upgrade message processing.
*
* \ingroup PUBLIC_API
*
*/
fwUpdateResult_t otaUpgrade_processOtaUpgradeMessage(void)
{

    // In case the state machine gets lost, set a limit to how
    // long it can run.
    sys_tick_t timestamp = GET_SYSTEM_TICK();
    sys_tick_t elapsedSeconds = 0;

    // Clear the module data structure
    memset(&otaUpData, 0, sizeof(otaUpData_t));

    // Init variables needed for upgrade session
    otaUpData.active = true;
    otaUpData.otaUpState = OTA_UP_STATE_SEND_OTA_CMD_PHASE1;
    otaUpData.fwUpdateResult = RESULT_NO_FWUPGRADE_PERFORMED;

    // Init variables so we grab the right amount of data as we
    // head into retrieving the message from the modem.  Setup
    // so that we read the section info portion of the upgrade message next.
    otaUpData.modemRequestLength = OTA_UPDATE_MSG_HEADER_SIZE;
    otaUpData.modemRequestOffset = OTA_UPDATE_SECTION_HEADER_SIZE;
    otaUpData.otaFlashState = OTA_FLASH_STATE_GET_SECTION_INFO;

    // Run the otaUpgrade state machine until the firmware is completely
    // collected from the modem.
    while (otaUpData.active)
    {

        // Run state machine
        otaUpgrade_modemStateMachine();
        WATCHDOG_TICKLE();

        // Run other lower-level state machines in the system needed to retrieve
        // data from the modem.
        modemCmd_exec();
        modemMgr_exec();
        modemCmd_exec();
        WATCHDOG_TICKLE();

        // Only allow the upgrade loop to run for 10 minutes.
        elapsedSeconds = GET_ELAPSED_TIME_IN_SEC(timestamp);
        if (elapsedSeconds > (sys_tick_t)(10 * 60))
        {
            modemMgr_stopModemCmdBatch();
            otaUpData.active = false;
            otaUpData.otaUpState = OTA_UP_STATE_IDLE;
            otaUpData.fwUpdateResult = RESULT_DONE_ERROR;
            otaUpData.fwUpdateErrNum = FW_UP_ERR_TIMEOUT;
            break;
        }
    }

    return (otaUpData.fwUpdateResult);
}

/**
* \brief Return the status of the last fw update message check 
*        and fw update attempt.
* \ingroup PUBLIC_API
* 
* @return fwUpdateResult_t  The status of the last fw update 
*         check for a modem update message, and the fw update
*         result if a fw update message was available from the
*         modem.
*/
fwUpdateResult_t otaUpgrade_getFwUpdateResult(void)
{
    return (otaUpData.fwUpdateResult);
}

/**
* \brief Return the CRC of the last firmware update processing 
*        as it was reported in the firmware upgrade message.
* 
* @return uint16_t Reported crc16 of the firmware stored in the 
*         second image location based on last firmware update
*         message.
* \ingroup PUBLIC_API
*/
uint16_t otaUpgrade_getFwMessageCrc(void)
{
    return (otaUpData.sectionCrc16);
}

/**
* \brief Return the calculated CRC of the last firmware update 
*        processing.
* 
* @return uint16_t Calculated crc16 of the firmware stored in 
*         the second image location based on last firmware
*         update message processing.
* \ingroup PUBLIC_API
*/
uint16_t otaUpgrade_getFwCalculatedCrc(void)
{
    return (otaUpData.sectionCrc16);
}

/**
* \brief Return the firmware image data length of the last 
*        firmware update processing.
* 
* @return uint16_t Length of the firmware stored in the second 
*         image location based on last firmware update message
*         processing.
* \ingroup PUBLIC_API
*/
uint16_t otaUpgrade_getFwLength(void)
{
    return (otaUpData.sectionDataLength);
}

/**
* \brief Return the error code identifying where an upgrade 
*        failed.
* 
* @return uint8_t Error Code
* \ingroup PUBLIC_API
*/
uint8_t otaUpgrade_getErrorCode(void)
{
    return (otaUpData.fwUpdateErrNum);
}

/*************************
 * Module Private Functions
 ************************/

/**
* \brief Helper function to setup the cmdWrite object that 
*        contains the info to tell the lower layer functions
*        what modem operation to perform.
* 
* @param cmdType Currently only supports the 
*                MODEM_BATCH_CMD_GET_OTA_PARTIAL command.
*/
static void sendModemBatchCmd(modemBatchCmdType_t cmdType)
{
    if (cmdType == MODEM_BATCH_CMD_GET_OTA_PARTIAL)
    {
        memset(&otaUpData.cmdWrite, 0, sizeof(modemCmdWriteData_t));
        otaUpData.cmdWrite.cmd = OUTPOUR_M_COMMAND_GET_INCOMING_PARTIAL;
        otaUpData.cmdWrite.payloadLength = otaUpData.modemRequestLength;
        otaUpData.cmdWrite.payloadOffset = otaUpData.modemRequestOffset;
        modemMgr_sendModemCmdBatch(&otaUpData.cmdWrite);
    }
}

/**
* \brief Modem data retrieval state machine. This is the high
*        level state machine to retrieve the flash upgrade
*        message data from the modem.  After a packet of data is
*        retrieved, it calls the flash state machine.
* \note This state machine assumes the modem is already up. 
*/
static void otaUpgrade_modemStateMachine(void)
{
    switch (otaUpData.otaUpState)
    {
        case OTA_UP_STATE_IDLE:
            break;

            /**
             * PHASE 1 - send an incoming partial cmd to the modem to 
             * retrieve data. 
             */
        case OTA_UP_STATE_SEND_OTA_CMD_PHASE1:
            sendModemBatchCmd(MODEM_BATCH_CMD_GET_OTA_PARTIAL);
            otaUpData.otaUpState = OTA_UP_STATE_OTA_CMD_PHASE1_WAIT;
            break;

        case OTA_UP_STATE_OTA_CMD_PHASE1_WAIT:
            if (modemMgr_isModemCmdError())
            {
                otaUpData.modemRetryCount++;
                // Retry if errors occur during data retrieval.
                if (otaUpData.modemRetryCount < OTA_MODEM_ERROR_RETRY_MAX)
                {
                    // Reset state to resend modem command.
                    otaUpData.otaUpState = OTA_UP_STATE_SEND_OTA_CMD_PHASE1;
                }
                else
                {
                    otaUpData.modemRetryCount = 0;
                    // If no more retries, we abort.
                    otaUpData.fwUpdateResult = RESULT_DONE_ERROR;
                    otaUpData.fwUpdateErrNum = FW_UP_ERR_MODEM;
                    otaUpData.otaUpState = OTA_UP_STATE_DONE;
                    otaUpData.exitModemProcessing = true;
                }
            }
            else if (modemMgr_isModemCmdComplete())
            {
                otaUpData.modemRetryCount = 0;
                // Track how many bytes of the message we have requested.
                otaUpData.modemRequestOffset += otaUpData.modemRequestLength;
                // Move to processing the data retrieved from the modem.
                otaUpData.otaUpState = OTA_UP_STATE_PROCESS_OTA_CMD_PHASE1;
            }
            break;

        case OTA_UP_STATE_PROCESS_OTA_CMD_PHASE1:
        {
            // Process the data retrieved from the modem.
            otaUpgrade_flashStateMachine();
            if (!otaUpData.exitModemProcessing)
            {
                // Retrieve more data from the modem.
                otaUpData.otaUpState = OTA_UP_STATE_SEND_OTA_CMD_PHASE1;
            }
            else
            {
                otaUpData.otaUpState = OTA_UP_STATE_DONE;
            }
        }
            break;

        case OTA_UP_STATE_DONE:
            otaUpData.active = false;
            otaUpData.otaUpState = OTA_UP_STATE_IDLE;
            break;
    }
}

/**
* \brief Flash State Machine. Call the appropriate flash state 
*        machine function function based on the current
*        otaFlashState.
*/
static void otaUpgrade_flashStateMachine(void)
{
    bool continue_processing = false;

    do
    {
        // By default, run only one loop iteration of this function.  Certain
        // states will set continue_processing to true if more than one loop
        // iteration should be performed before exiting this state machine.
        continue_processing = false;

        switch (otaUpData.otaFlashState)
        {
            case OTA_FLASH_STATE_GET_SECTION_INFO:
                continue_processing = otaUpgrade_processSectionInfo();
                break;
            case OTA_FLASH_STATE_ERASE_SECTION_DATA:
                continue_processing = otaUpgrade_eraseSection();
                break;
            case OTA_FLASH_STATE_WRITE_SECTION_DATA:
                continue_processing = otaUpgrade_writeSectionData();
                break;
            case OTA_FLASH_STATE_VERIFY_SECTION_DATA:
                continue_processing = otaUpgrade_verifySection();
                break;
        }
    } while (continue_processing);
}

/**
* \brief Firmware Upgrade Flash State Machine Function
*/
static bool otaUpgrade_processSectionInfo(void)
{
    bool continue_processing = false;

    // Setup addresses
    uint16_t backupImageFlashLength = getAppImageLength();
    uint16_t backupImageStartAddr = getBackupImageStartAddr();
    uint16_t backupImageEndAddr = getBackupImageEndAddr();

    // Get the buffer that contains the OTA message data
    otaResponse_t *otaRespP = modemMgr_getLastOtaResponse();

    // Retreive section info from the message
    volatile uint8_t *bufP = &otaRespP->buf[0];
    uint8_t sectionStartId = *bufP++;
    uint8_t sectionNumber = *bufP++;

    // Check start section byte and section number in message
    if ((sectionStartId == FLASH_UPGRADE_SECTION_START) && (sectionNumber == 0))
    {
        // Retrieve Start Address from section info in message
        otaUpData.sectionStartAddrP = (*bufP++ << 8);
        otaUpData.sectionStartAddrP |= *bufP++;
        // This is a bit of a hack.
        // The section info assumes it is bound for the main image location.
        // But we are storing it in the second image location.
        // We need to shift address from main image to the second image location
        otaUpData.sectionStartAddrP -= (getAppImageStartAddr() - getBackupImageStartAddr());
        otaUpData.sectionWriteAddrP = (uint8_t *)otaUpData.sectionStartAddrP;
        // Retrieve Length from section info in message
        otaUpData.sectionDataLength = (*bufP++ << 8);
        otaUpData.sectionDataLength |= *bufP++;
        otaUpData.sectionDataRemaining = otaUpData.sectionDataLength;
        // Retrieve CRC from section info in message
        otaUpData.sectionCrc16 = (*bufP++ << 8);
        otaUpData.sectionCrc16 |= *bufP++;

        // Verify the section parameters
        // 1. Check that the modem message has enough data to fill the section.
        // 2. Check that the section start and end address is located in app flash area
        // Otherwise we consider this a catastrophic error.

        uint16_t startBurnAddr = otaUpData.sectionStartAddrP;
        uint16_t endBurnAddr = otaUpData.sectionStartAddrP + otaUpData.sectionDataLength - 1;

        if ((otaRespP->remainingInBytes >= otaUpData.sectionDataLength) &&
            (otaUpData.sectionDataLength <= backupImageFlashLength) &&
            (startBurnAddr >= backupImageStartAddr) &&
            (startBurnAddr < backupImageEndAddr) &&
            (endBurnAddr > backupImageStartAddr) &&
            (endBurnAddr <= backupImageEndAddr))
        {
            // Setup state for the erase flash sequence.
            otaUpData.otaFlashState = OTA_FLASH_STATE_ERASE_SECTION_DATA;
            // Don't exit flash state machine.  Move to next state immediately to
            // erase data.
            continue_processing = true;
        }
        else
        {
            // Set error flags
            otaUpData.fwUpdateResult = RESULT_DONE_ERROR;
            otaUpData.fwUpdateErrNum = FW_UP_ERR_PARAMETER;
            otaUpData.exitModemProcessing = true;
        }
    }
    else
    {
        // Set error flags
        otaUpData.fwUpdateResult = RESULT_DONE_ERROR;
        otaUpData.fwUpdateErrNum = FW_UP_ERR_SECTION_HEADER;
        otaUpData.exitModemProcessing = true;
    }

    return (continue_processing);
}

/**
* \brief Firmware Upgrade Flash State Machine Function
*/
static bool otaUpgrade_eraseSection(void)
{

    uint8_t i;
    uint16_t numSectors = getNumSectorsInImage();
    uint8_t *flashSegmentAddrP = (uint8_t *)otaUpData.sectionStartAddrP;
    uint8_t *backupImageEndAddrP = (uint8_t *)getBackupImageEndAddr();

    // Update the APP record with new firmware info. If there was good firmware
    // in there, it's getting erased now.
    appRecord_updateFwInfo(false, 0);

    // Erase the flash segments
    for (i = 0; i < numSectors; i++, flashSegmentAddrP += 0x200)
    {
        // Check that the address falls below the bootloader flash area
        if (flashSegmentAddrP < backupImageEndAddrP)
        {
            // Tickle the watchdog before erasing
            WATCHDOG_TICKLE();
            msp430Flash_erase_segment(flashSegmentAddrP);
        }
    }

    // Set up for starting the write data to flash.
    // Initialize request size from modem.
    // The maximum data we can request from the modem at one time is
    // OTA_PAYLOAD_MAX_RX_READ_LENGTH
    if (otaUpData.sectionDataRemaining > OTA_PAYLOAD_MAX_RX_READ_LENGTH)
    {
        otaUpData.modemRequestLength = OTA_PAYLOAD_MAX_RX_READ_LENGTH;
    }
    else
    {
        otaUpData.modemRequestLength = otaUpData.sectionDataRemaining;
    }

    // Setup state for the write to flash sequence.
    otaUpData.otaFlashState = OTA_FLASH_STATE_WRITE_SECTION_DATA;

    // Exit the flash state machine upon return in order to
    // retrieve more more data from the modem.
    return (false);
}

/**
* \brief Firmware Upgrade Flash State Machine Function
*/
static bool otaUpgrade_writeSectionData(void)
{

    // The default mode is to exit the flash state machine upon return
    // in order to retrieve more data from the modem before the flash state
    // machine processing continues.
    bool continue_processing = false;

    // Get the buffer that contains the OTA message data
    otaResponse_t *otaRespP = modemMgr_getLastOtaResponse();
    uint8_t *bufP = &otaRespP->buf[0];

    // Make sure we got data back from the modem, else its an error condition.
    if (otaRespP->lengthInBytes > 0)
    {

        uint16_t writeDataSize = otaRespP->lengthInBytes;
        uint8_t *backupImageEndAddrP = (uint8_t *)getBackupImageEndAddr();

        // This should not happen but just in case, make sure
        // we did not get too much data back from the modem.
        if (otaUpData.sectionDataRemaining < writeDataSize)
        {
            writeDataSize = otaUpData.sectionDataRemaining;
        }

        // Write the data to flash.
        // Check that we are not going to write into the bootloader flash area.
        if ((otaUpData.sectionWriteAddrP + writeDataSize) <= (backupImageEndAddrP + 1))
        {
            // Tickle the watchdog before erasing
            WATCHDOG_TICKLE();
            msp430Flash_write_bytes(otaUpData.sectionWriteAddrP, &bufP[0], writeDataSize);
        }

        // Update counters and flash pointer
        otaUpData.sectionDataRemaining -= writeDataSize;
        otaUpData.sectionWriteAddrP += writeDataSize;

        // Check if we have all the data for the section.
        // If so, call the flash verify function.
        // Otherwise, set request length for next buffer of data.
        if (otaUpData.sectionDataRemaining == 0x0)
        {
            // Setup state for the verify flash sequence.
            otaUpData.otaFlashState = OTA_FLASH_STATE_VERIFY_SECTION_DATA;
            // Don't exit flash state machine.  Move to next state immediately to
            // verify flash.
            continue_processing = true;
        }
        else
        {
            // We need more data.
            if (otaUpData.sectionDataRemaining > OTA_PAYLOAD_MAX_RX_READ_LENGTH)
            {
                otaUpData.modemRequestLength = OTA_PAYLOAD_MAX_RX_READ_LENGTH;
            }
            else
            {
                otaUpData.modemRequestLength = otaUpData.sectionDataRemaining;
            }
        }
    }
    else
    {
        // We did not get any data back from the modem.
        // Error condition.
        otaUpData.fwUpdateResult = RESULT_DONE_ERROR;
        otaUpData.fwUpdateErrNum = FW_UP_ERR_MODEM;
        otaUpData.exitModemProcessing = true;
    }

    return (continue_processing);
}

/**
* \brief Firmware Upgrade Flash State Machine Function
*/
static bool otaUpgrade_verifySection(void)
{
    uint16_t calcCrc16 = gen_crc16((const unsigned char *)otaUpData.sectionStartAddrP, otaUpData.sectionDataLength);

    otaUpData.lastCalcCrc16 = calcCrc16;
    if (calcCrc16 == otaUpData.sectionCrc16)
    {
        otaUpData.fwUpdateResult = RESULT_DONE_SUCCESS;
    }
    else
    {
        // The CRC failed.
        // Error condition.
        otaUpData.fwUpdateResult = RESULT_DONE_ERROR;
        otaUpData.fwUpdateErrNum = FW_UP_ERR_CRC;
    }
    otaUpData.exitModemProcessing = true;

    // Exit the flash state machine upon return.
    return (false);
}

