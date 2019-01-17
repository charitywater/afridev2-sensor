/** 
 * @file msgOta.c
 * \n Source File
 * \n AfridevV2 MSP430 Bootloader Firmware
 * 
 * \brief Manage the details of processing OTA messages.  Runs 
 * the ota state machine to retrieve OTA messages from the modem
 * and process them.  This module has been customized around the 
 * firmware upgrade OTA processing. 
 *  
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
 * \def WAIT_FOR_LINK_UP_TIME_IN_SECONDS
 * \brief Specify how long to wait for the modem to connect to 
 *        the network.
 */
#define WAIT_FOR_LINK_UP_TIME_IN_SECONDS ((uint16_t)60*10*(SYS_TICKS_PER_SECOND))

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
 * \brief Specify the modem batch command to prepare.
 */
typedef enum modemBatchCmdType_e {
    MODEM_BATCH_CMD_STATUS_ONLY,
    MODEM_BATCH_CMD_SOS,
    MODEM_BATCH_CMD_FW_UPGRADE_RESPONSE,
    MODEM_BATCH_CMD_GET_OTA_PARTIAL,
    MODEM_BATCH_CMD_DELETE_MESSAGE
} modemBatchCmdType_t;

/**
 * \typedef otaState_t
 * \brief Specify the states for retrieving an Firmware Update 
 *        OTA msg from the modem. The state machine will
 *        sequence through these steps.
 */
typedef enum otaState_e {
    OTA_STATE_IDLE,                                        /**< Not active */
    OTA_STATE_GRAB,                                        /**< Start up the modem */
    OTA_STATE_WAIT_FOR_MODEM_UP,                           /**< Wait for modem to power up */
    OTA_STATE_SEND_OTA_CMD_PHASE0,                         /**< Send cmd to modem to check for messages */
    OTA_STATE_OTA_CMD_PHASE0_WAIT,                         /**< Wait for modem cmd to complete */
    OTA_STATE_WAIT_FOR_LINK,                               /**< Wait for the modem to get connected */
    OTA_STATE_PROCESS_OTA_CMD_PHASE0,                      /**< See what type of message is available (if any) */
    OTA_STATE_SEND_OTA_CMD_PHASE1,                         /**< FwUpdate msg available - start requesting partial data from modem */
    OTA_STATE_OTA_CMD_PHASE1_WAIT,                         /**< Wait to get fwUpdate partial data from modem */
    OTA_STATE_PROCESS_OTA_CMD_PHASE1,                      /**< Process fwUpdate partial data from modem */
    OTA_STATE_SEND_OTA_RESPONSE,                           /**< If done with fwUpdate, send response */
    OTA_STATE_SEND_OTA_RESPONSE_WAIT,                      /**< What for response to transmit */
    OTA_STATE_SEND_DELETE_OTA_CMD,                         /**< Delete messages */
    OTA_STATE_DELETE_OTA_CMD_WAIT,                         /**< Wait for delete to complete */
    OTA_STATE_RELEASE,                                     /**< Release the modem */
    OTA_STATE_RELEASE_WAIT,                                /**< Wait for modem release complete */
} otaState_t;

/**
 * \typedef otaFlashState_t
 * \brief Specify the states for writing and erasing flash with 
 *        new firmware.
 */
typedef enum otaFlashState_e {
    OTA_FLASH_STATE_START,
    OTA_FLASH_STATE_GET_MSG_INFO,
    OTA_FLASH_STATE_GET_SECTION_INFO,
    OTA_FLASH_STATE_WRITE_SECTION_DATA,
} otaFlashState_t;

/**
 * \typedef fwUpdateErrNum_t
 * \brief Specify the error codes that identify what step the fw
 *        upgrade failed at.
 */
typedef enum fwUpgradeErrorCode_e {
    OTA_FW_UP_ERR_NONE = 0,
    OTA_FW_UP_ERR_MODEM = -1,
    OTA_FW_UP_ERR_SECTION_HEADER = -2,
    OTA_FW_UP_ERR_PARAMETER = -3,
    OTA_FW_UP_ERR_CRC = -4,
    OTA_FW_UP_ERR_TIMEOUT = -5,
} fwUpdateErrNum_t;

/**
 * \typedef otaData_t
 * \brief Define a container to store data specific to 
 *        retrieving OTA messages from the modem.
 */
typedef struct otaData_s {
    bool active;                                           /**< marks the water msg manager as busy */
    bool sos;                                              /**< specifies that we are in SOS mode */
    otaState_t otaState;                                   /**< current state */
    modemCmdWriteData_t cmdWrite;                          /**< A pointer to a modem write cmd object */
    uint16_t modemRequestLength;                           /**< Specify how much data to get from the modem */
    uint16_t modemRequestOffset;                           /**< Specify how much data to get from the modem */
    otaFlashState_t otaFlashState;                         /**< current state of flash state machine */
    uint8_t totalSections;                                 /**< Total number of code sections in the fw update message */
    uint8_t nextSectionNumber;                             /**< The next code section to process of the fw update message */
    uint16_t sectionStartAddrP;                            /**< Current upgrade section start address */
    uint16_t sectionDataLength;                            /**< Current upgrade section total data length */
    uint16_t sectionCrc16;                                 /**< Current section CRC16 value */
    uint16_t sectionDataRemaining;                         /**< Count in bytes of section data remaining to retrieve from modem */
    uint8_t *sectionWriteAddrP;                            /**< Where in flash to write the data */
    fwUpdateResult_t fwUpdateResult;                       /**< result for last flash state machine processing */
    fwUpdateErrNum_t fwUpdateErrNum;                       /**< If upgrade failure, identify what step it failed at */
    bool doneProcessingFwUpdateMsg;                        /**< Flag indicating the update msg processing is complete */
    uint8_t modemRetryCount;                               /**< If modem error, how many retries we have performed */
    uint16_t lastMsgId;                                    /**< Save the last OTA msgId */
    uint16_t lastCalcCrc16;                                /**< Save last calculated CRC16 value */
} otaData_t;

/****************************
 * Module Data Declarations
 ***************************/

/**
* \var otaData
* \brief Declare a data object to "house" data for this module.
*/
// static
otaData_t otaData;

/*************************
 * Module Prototypes
 ************************/
static void otaMsgMgr_prepareFwUpResponseMsg(modemCmdWriteData_t *cmdWriteP);
static void otaMsgMgr_prepareSosMsg(modemCmdWriteData_t *cmdWriteP);
static void otaMsgMgr_sendModemBatchCmd(modemBatchCmdType_t cmdType);
static void otaMsgMgr_stateMachine(void);
static void otaMsgMgr_processFwUpdateMsg(void);
static bool otaMsgMgr_checkForOta(void);
static void fwUpdateMsg_start(void);
static void fwUpdateMsg_getMsgInfo(void);
static void fwUpdateMsg_getSectionInfo(void);
static void fwUpdateMsg_eraseSection(void);
static void fwUpdateMsg_writeSectionData(void);
static void fwUpdateMsg_verifySection(void);

/***************************
 * Module Public Functions
 **************************/

// #define STATE_TRACE
#ifdef STATE_TRACE
#define MAX_STATE_TRACE_SIZE 64
typedef struct state_trace_s {
    otaState_t state;
    uint16_t arg;
} state_trace_t;
state_trace_t stateTrace[MAX_STATE_TRACE_SIZE];
uint8_t stateTraceIndex = 0;
void addStateTracePoint(otaState_t state, uint16_t b1)
{
    stateTrace[stateTraceIndex].state = state;
    stateTrace[stateTraceIndex].arg = b1;
    stateTraceIndex++;
    stateTraceIndex &= (MAX_STATE_TRACE_SIZE - 1);
}
#else
#define addStateTracePoint(a,b)
#endif

/**
* \brief Exec routine should be called as part of the main 
*        processing loop. Drives the ota message state machine.
*        Will only run the state machine if activated to do so
*        via the otaMsgMgr_getAndProcessOtaMsgs function.
* \ingroup EXEC_ROUTINE
*/
void otaMsgMgr_exec(void)
{
    if (otaData.active)
    {
        otaMsgMgr_stateMachine();
    }
}

/**
* \brief Call once as system startup to initialize the ota 
*        message manager object.
* \ingroup PUBLIC_API
*/
void otaMsgMgr_init(void)
{
    memset(&otaData, 0, sizeof(otaData_t));
    addStateTracePoint(otaData.otaState, (uint16_t)0);
}

/**
* \brief Kick-off the ota state machine processing for 
*        retrieving OTA messages from the modem. The state
*        machine will retrieve messages one-by-one from the
*        modem and process them.  Customized to process the
*        Firmware upgrade message.  All other messages will be
*        deleted.
* \ingroup PUBLIC_API
*
* @param sos true/false flag to specify whether an SOS message
*            should be sent as part of the OTA processing.
*/
void otaMsgMgr_getAndProcessOtaMsgs(bool sos)
{
    // Init variables needed for upgrade session
    otaData.active = true;
    otaData.otaState = OTA_STATE_GRAB;
    otaData.sos = sos;
    otaData.fwUpdateResult = RESULT_NO_FWUPGRADE_PERFORMED;
    // Run the OTA state machine
    otaMsgMgr_stateMachine();
}

/**
* \brief Poll to identify if the ota message processing is 
*        complete.
* \ingroup PUBLIC_API
* 
* @return bool True is ota message processing is not active.
*/
bool otaMsgMgr_isOtaProcessingDone(void)
{
    return (!otaData.active);
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
fwUpdateResult_t otaMsgMgr_getFwUpdateResult(void)
{
    return (otaData.fwUpdateResult);
}

/*************************
 * Module Private Functions
 ************************/

static void otaMsgMgr_stateMachine(void)
{
    switch (otaData.otaState)
    {
        case OTA_STATE_IDLE:
            break;

        case OTA_STATE_GRAB:
            if (modemMgr_grab())
            {
                otaData.otaState = OTA_STATE_WAIT_FOR_MODEM_UP;
            }
            break;

        case OTA_STATE_WAIT_FOR_MODEM_UP:
            if (modemMgr_isModemUp())
            {
                otaData.otaState = OTA_STATE_SEND_OTA_CMD_PHASE0;
                addStateTracePoint(otaData.otaState, (uint16_t)0);
            }
            break;

            /** 
             * PHASE 0 - This phase is used to determine if the modem has a 
             * message available to retrieve.  For this case, we send only a
             * status request batch command.  But if the SOS flag is set, we
             * also send an SOS message to the cloud as well as retrieve 
             * status information.
             */
        case OTA_STATE_SEND_OTA_CMD_PHASE0:
            if (otaData.sos)
            {
                otaMsgMgr_sendModemBatchCmd(MODEM_BATCH_CMD_SOS);
            }
            else
            {
                otaMsgMgr_sendModemBatchCmd(MODEM_BATCH_CMD_STATUS_ONLY);
            }
            otaData.otaState = OTA_STATE_OTA_CMD_PHASE0_WAIT;
            addStateTracePoint(otaData.otaState, (uint16_t)0);
            break;

        case OTA_STATE_OTA_CMD_PHASE0_WAIT:
            if (modemMgr_isModemCmdError())
            {
                // Retry if error occurred during data retrieval.
                if (otaData.modemRetryCount < OTA_MODEM_ERROR_RETRY_MAX)
                {
                    otaData.modemRetryCount++;
                    // Reset state to resend modem command.
                    otaData.otaState = OTA_STATE_SEND_OTA_CMD_PHASE0;
                    addStateTracePoint(otaData.otaState, (uint16_t)0);
                }
                else
                {
                    otaData.modemRetryCount = 0;
                    // If no more retries, we abort.
                    otaData.fwUpdateResult = RESULT_DONE_ERROR;
                    otaData.fwUpdateErrNum = OTA_FW_UP_ERR_MODEM;
                    // Try to delete the message as we can't let messages build up in
                    // the modem or else that is a certain brick situation.
                    otaData.otaState = OTA_STATE_SEND_DELETE_OTA_CMD;
                    addStateTracePoint(otaData.otaState, (uint16_t)0);
                }
            }
            else if (modemMgr_isModemCmdComplete())
            {
                otaData.modemRetryCount = 0;
                // If we sent an SOS message, we want to wait until the modem is
                // connected to the network (i.e. the link is up).  Otherwise we
                // don't care about connecting to the network, as we only wanted to
                // check if the modem has a message stored internally for us to
                // retrieve.
                if (otaData.sos)
                {
                    otaData.otaState = OTA_STATE_WAIT_FOR_LINK;
                    addStateTracePoint(otaData.otaState, (uint16_t)0);
                }
                else
                {
                    otaData.otaState = OTA_STATE_PROCESS_OTA_CMD_PHASE0;
                    addStateTracePoint(otaData.otaState, (uint16_t)0);
                }
            }
            break;

        case OTA_STATE_WAIT_FOR_LINK:
            // Waiting for network link to come up
            if (modemMgr_isLinkUp() ||
                modemMgr_isLinkUpError() ||
                (modemPower_getModemUpTimeInSysTicks() > WAIT_FOR_LINK_UP_TIME_IN_SECONDS))
            {
                // Done waiting.  Move to retrieving any OTA Messages.
                otaData.otaState = OTA_STATE_PROCESS_OTA_CMD_PHASE0;
                addStateTracePoint(otaData.otaState, (uint16_t)0);
            }
            else
            {
                // While waiting for the modem link to come up,
                // retrieve status only from modem.
                otaMsgMgr_sendModemBatchCmd(MODEM_BATCH_CMD_STATUS_ONLY);
                otaData.otaState = OTA_STATE_OTA_CMD_PHASE0_WAIT;
                addStateTracePoint(otaData.otaState, (uint16_t)0);
            }
            break;

        case OTA_STATE_PROCESS_OTA_CMD_PHASE0:
            // Check if there is an OTA message available from the modem.
            if (otaMsgMgr_checkForOta())
            {
                otaData.otaFlashState = OTA_FLASH_STATE_START;
                // Init variables so we grab the right amount of data as we
                // head into retrieving a message from the modem.

                // Only request one byte from modem - the message type.
                otaData.modemRequestLength = 1;
                // Get the first byte that modem has. Don't index into message.
                otaData.modemRequestOffset = 0;
                otaData.otaState = OTA_STATE_SEND_OTA_CMD_PHASE1;
                addStateTracePoint(otaData.otaState, (uint16_t)0);
                otaData.doneProcessingFwUpdateMsg = false;
            }
            else
            {
                // No messages available from the modem.  We are done.
                otaData.otaState = OTA_STATE_RELEASE;
                addStateTracePoint(otaData.otaState, (uint16_t)0);
            }
            break;

            /**
             * PHASE 1 - send an incoming partial cmd to the modem to retrieve
             * the first bytes of the OTA message.  We are only interested 
             * in retrieving the header portion of the message - enough to 
             * identify the type of message that it is. 
             */
        case OTA_STATE_SEND_OTA_CMD_PHASE1:
            otaMsgMgr_sendModemBatchCmd(MODEM_BATCH_CMD_GET_OTA_PARTIAL);
            otaData.otaState = OTA_STATE_OTA_CMD_PHASE1_WAIT;
            addStateTracePoint(otaData.otaState, (uint16_t)0);
            break;

        case OTA_STATE_OTA_CMD_PHASE1_WAIT:
            if (modemMgr_isModemCmdError())
            {
                // Retry if error occurred during data retrieval.
                if (otaData.modemRetryCount < OTA_MODEM_ERROR_RETRY_MAX)
                {
                    otaData.modemRetryCount++;
                    // Reset state to resend modem command.
                    otaData.otaState = OTA_STATE_SEND_OTA_CMD_PHASE1;
                    addStateTracePoint(otaData.otaState, (uint16_t)0);
                }
                else
                {
                    otaData.modemRetryCount = 0;
                    // If no more retries, we abort.
                    otaData.fwUpdateResult = RESULT_DONE_ERROR;
                    otaData.fwUpdateErrNum = OTA_FW_UP_ERR_MODEM;
                    // Try to delete the message as we can't let messages build up in
                    // the modem or else that is a certain brick situation.
                    otaData.otaState = OTA_STATE_SEND_DELETE_OTA_CMD;
                    addStateTracePoint(otaData.otaState, (uint16_t)0);
                }
            }
            else if (modemMgr_isModemCmdComplete())
            {
                otaData.modemRetryCount = 0;
                // Move to processing the data retrieved from the modem.
                otaData.otaState = OTA_STATE_PROCESS_OTA_CMD_PHASE1;
                addStateTracePoint(otaData.otaState, (uint16_t)0);
            }
            break;

        case OTA_STATE_PROCESS_OTA_CMD_PHASE1:
        {
            // Track how many bytes of the message we have requested.
            otaData.modemRequestOffset += otaData.modemRequestLength;
            // Process the data retrieved from the modem.
            otaMsgMgr_processFwUpdateMsg();
            if (!otaData.doneProcessingFwUpdateMsg)
            {
                // Retrieve more data from the modem.
                otaData.otaState = OTA_STATE_SEND_OTA_CMD_PHASE1;
                addStateTracePoint(otaData.otaState, (uint16_t)0);
            }
            else
            {
                // Determine next action based on the results of the processing.
                if ((otaData.fwUpdateResult == RESULT_DONE_SUCCESS) | (otaData.fwUpdateResult == RESULT_DONE_ERROR))
                {
                    // We are done.  Send response next.
                    otaData.otaState = OTA_STATE_SEND_OTA_RESPONSE;
                    addStateTracePoint(otaData.otaState, (uint16_t)0);
                }
                else if (otaData.fwUpdateResult == RESULT_NO_FWUPGRADE_PERFORMED)
                {
                    // There was no upgrade message available.
                    // If we are in an SOS condition, we need to clear out any messages
                    // stored in the modem other than upgrade messages.
                    if (otaData.sos)
                    {
                        otaData.otaState = OTA_STATE_SEND_DELETE_OTA_CMD;
                        addStateTracePoint(otaData.otaState, (uint16_t)0);
                    }
                    else
                    {
                        otaData.otaState = OTA_STATE_RELEASE;
                        addStateTracePoint(otaData.otaState, (uint16_t)0);
                    }
                }
            }
        }
            break;

            /**
             * Send the FwUpgrade OTA Reply
             */
        case OTA_STATE_SEND_OTA_RESPONSE:
            otaMsgMgr_sendModemBatchCmd(MODEM_BATCH_CMD_FW_UPGRADE_RESPONSE);
            otaData.otaState = OTA_STATE_SEND_OTA_RESPONSE_WAIT;
            addStateTracePoint(otaData.otaState, (uint16_t)0);
            break;
        case OTA_STATE_SEND_OTA_RESPONSE_WAIT:
            // Wait until modem manager is done sending the OTA message
            // or an error occurred.
            if (modemMgr_isModemCmdError())
            {
                // Retry if error occurred during data retrieval.
                if (otaData.modemRetryCount < OTA_MODEM_ERROR_RETRY_MAX)
                {
                    otaData.modemRetryCount++;
                    // Reset state to resend modem command.
                    otaData.otaState = OTA_STATE_SEND_OTA_RESPONSE;
                    addStateTracePoint(otaData.otaState, (uint16_t)0);
                }
                else
                {
                    otaData.modemRetryCount = 0;
                    // Try to delete the message as we can't let messages build up in
                    // the modem or else that is a certain brick situation.
                    otaData.otaState = OTA_STATE_SEND_DELETE_OTA_CMD;
                    addStateTracePoint(otaData.otaState, (uint16_t)0);
                }
            }
            else if (modemMgr_isModemCmdComplete())
            {
                otaData.modemRetryCount = 0;
                // Waiting for response to get transmitted or error.  LinkUp looks
                // for when the modem returns MODEM_STATE_CONNECTED (=4).  This state
                // also indicates that the message was done transmitting because the
                // modem will set the modem state (modem_state_t) to MODEM_STATE_XFER
                // (=5) while transmitting the message.  Hence isLinkUp can also be
                // used to determine when the modem has finished transmitting.
                if (modemMgr_isLinkUp() ||
                    modemMgr_isLinkUpError() ||
                    (modemPower_getModemUpTimeInSysTicks() > WAIT_FOR_LINK_UP_TIME_IN_SECONDS))
                {
                    // Done waiting.  Move to delete message.
                    otaData.otaState = OTA_STATE_SEND_DELETE_OTA_CMD;
                    addStateTracePoint(otaData.otaState, (uint16_t)0);
                }
                else
                {
                    // While waiting for the modem to transmit,
                    // retrieve status only from modem.
                    otaMsgMgr_sendModemBatchCmd(MODEM_BATCH_CMD_STATUS_ONLY);
                }
            }
            break;

            /**
             * Delete the OTA Command
             */
        case OTA_STATE_SEND_DELETE_OTA_CMD:
            otaMsgMgr_sendModemBatchCmd(MODEM_BATCH_CMD_DELETE_MESSAGE);
            otaData.otaState = OTA_STATE_DELETE_OTA_CMD_WAIT;
            addStateTracePoint(otaData.otaState, (uint16_t)0);
            break;

        case OTA_STATE_DELETE_OTA_CMD_WAIT:
            if (modemMgr_isModemCmdError())
            {
                // Retry if error occurred.
                if (otaData.modemRetryCount < OTA_MODEM_ERROR_RETRY_MAX)
                {
                    otaData.modemRetryCount++;
                    // Reset state to resend modem command.
                    otaData.otaState = OTA_STATE_SEND_DELETE_OTA_CMD;
                    addStateTracePoint(otaData.otaState, (uint16_t)0);
                }
                else
                {
                    otaData.modemRetryCount = 0;
                    // If no more retries, we abort.
                    // Note that we don't consider not being able to delete
                    // the message a failure with respect to the fw upgrade.
                    otaData.otaState = OTA_STATE_RELEASE;
                    addStateTracePoint(otaData.otaState, (uint16_t)0);
                }
            }
            else if (modemMgr_isModemCmdComplete())
            {
                otaData.modemRetryCount = 0;
                // If we are in an SOS condition we need to
                // handle all messages in the modem (i.e. delete them).
                // Otherwise we can get into a stuck situation and never recover.
                if (otaData.sos && (otaData.fwUpdateResult != RESULT_DONE_SUCCESS))
                {
                    otaData.otaState = OTA_STATE_PROCESS_OTA_CMD_PHASE0;
                    addStateTracePoint(otaData.otaState, (uint16_t)0);
                }
                else
                {
                    otaData.otaState = OTA_STATE_RELEASE;
                    addStateTracePoint(otaData.otaState, (uint16_t)0);
                }
            }
            break;

        case OTA_STATE_RELEASE:
            modemMgr_release();
            otaData.otaState = OTA_STATE_RELEASE_WAIT;
            break;

        case OTA_STATE_RELEASE_WAIT:
            if (modemMgr_isReleaseComplete())
            {
                otaData.active = false;
                otaData.otaState = OTA_STATE_IDLE;
                addStateTracePoint(otaData.otaState, (uint16_t)0);
            }
            break;
    }
}

/**
* \brief Prepare the SOS message for the lower layers. Prepare 
*        the message header and message data portions.  Fill in
*        the cmdWriteData structure.
* 
* @param cmdWriteP Pointer to the cmdWriteData structure to fill 
*                  in.
*/
static void otaMsgMgr_prepareSosMsg(modemCmdWriteData_t *cmdWriteP)
{
    // prepare OTA response message
    // Get the shared buffer (we borrow the ota buffer)
    uint8_t *bufP = modemMgr_getSharedBuffer();

    // Fill the response message header with the SOS markers (0x55).
    memset(&bufP[0], 0x55, OTA_RESPONSE_HEADER_LENGTH);
    // Fill in individual values of the SOS header
    bufP[0] = 0x01;                                        // Payload start byte
    bufP[1] = MSG_TYPE_SOS;                                // Payload message type
    bufP[2] = AFRIDEV2_PRODUCT_ID;                         // AfridevV2 Product ID
    bufP[9] = FW_VERSION_MAJOR;                            // FW MAJOR
    bufP[10] = FW_VERSION_MINOR;                           // FW MINOR
    bufP[15] = 0xA5;                                       // End of message header delimeter

    // Zero the data portion of the SOS message
    memset(&bufP[OTA_RESPONSE_HEADER_LENGTH], 0, OTA_RESPONSE_DATA_LENGTH);
    // Fill in the data portion of the SOS message
    main_copyBootInfo(&bufP[OTA_RESPONSE_HEADER_LENGTH]);

    cmdWriteP->cmd = M_COMMAND_SEND_DATA;
    cmdWriteP->payloadMsgId = MSG_TYPE_SOS;
    cmdWriteP->payloadP = bufP;
    cmdWriteP->payloadLength = OTA_RESPONSE_LENGTH;
}

/**
* \brief Prepare the FwUpgrade response message for the lower 
*        layers. Prepare the message header and message data
*        portions. Fill in the cmdWriteData structure.
* 
* @param cmdWriteP Pointer to the cmdWriteData structure to fill 
*                  in.
*/
static void otaMsgMgr_prepareFwUpResponseMsg(modemCmdWriteData_t *cmdWriteP)
{
    uint8_t offset = 0;
    // prepare OTA response message
    // Get the shared buffer (we borrow the ota buffer)
    uint8_t *bufP = modemMgr_getSharedBuffer();

    // Fill the response message header with the SOS markers (0x55).
    memset(&bufP[0], 0x55, OTA_RESPONSE_HEADER_LENGTH);
    // Fill in individual values of the SOS header
    bufP[0] = 0x01;                                        // Payload start byte
    bufP[1] = MSG_TYPE_OTAREPLY;                           // Payload message type
    bufP[2] = AFRIDEV2_PRODUCT_ID;                         // AfridevV2 Product ID
    bufP[9] = FW_VERSION_MAJOR;                            // FW MAJOR
    bufP[10] = FW_VERSION_MINOR;                           // FW MINOR

    // Zero the response message data portion.
    // The data portion goes after the 16 byte header
    memset(&bufP[OTA_RESPONSE_HEADER_LENGTH], 0, OTA_RESPONSE_DATA_LENGTH);
    // Fill in data portion for the fw update response message
    offset = OTA_RESPONSE_HEADER_LENGTH;
    bufP[offset++] = OTA_OPCODE_FIRMWARE_UPGRADE;          // opcode
    bufP[offset++] = otaData.lastMsgId >> 8;               // msg num hi
    bufP[offset++] = otaData.lastMsgId & 0xFF;             // msg num lo
    bufP[offset++] = otaData.fwUpdateResult;               // fw update result
    bufP[offset++] = otaData.fwUpdateErrNum;               // fw update err num
    bufP[offset++] = otaData.sectionCrc16 >> 8;            // crc hi from msg
    bufP[offset++] = otaData.sectionCrc16 & 0xFF;          // crc lo from msg
    bufP[offset++] = otaData.lastCalcCrc16 >> 8;           // crc hi calculated
    bufP[offset++] = otaData.lastCalcCrc16 & 0xFF;         // crc lo calculated

    cmdWriteP->cmd = M_COMMAND_SEND_DATA;
    cmdWriteP->payloadMsgId = MSG_TYPE_OTAREPLY;
    cmdWriteP->payloadP = bufP;
    cmdWriteP->payloadLength = OTA_RESPONSE_LENGTH;
}

/**
* \brief Utility function to send a command to the modem. Based 
*        on the message type, do the appropriate initialization.
* 
* @param cmdType Identifies the command type to send to the 
*                modem.
*/
static void otaMsgMgr_sendModemBatchCmd(modemBatchCmdType_t cmdType)
{
    memset(&otaData.cmdWrite, 0, sizeof(modemCmdWriteData_t));
    switch (cmdType)
    {
        case MODEM_BATCH_CMD_STATUS_ONLY:
            otaData.cmdWrite.statusOnly = true;
            break;
        case MODEM_BATCH_CMD_SOS:
            otaMsgMgr_prepareSosMsg(&otaData.cmdWrite);
            break;
        case MODEM_BATCH_CMD_FW_UPGRADE_RESPONSE:
            otaMsgMgr_prepareFwUpResponseMsg(&otaData.cmdWrite);
            break;
        case MODEM_BATCH_CMD_GET_OTA_PARTIAL:
            otaData.cmdWrite.cmd = M_COMMAND_GET_INCOMING_PARTIAL;
            otaData.cmdWrite.payloadLength = otaData.modemRequestLength;
            otaData.cmdWrite.payloadOffset = otaData.modemRequestOffset;
            break;
        case MODEM_BATCH_CMD_DELETE_MESSAGE:
            otaData.cmdWrite.cmd = M_COMMAND_DELETE_INCOMING;
            break;
    }
    modemMgr_sendModemCmdBatch(&otaData.cmdWrite);
}

/**
* \brief Check for the presence of an OTA message. 
*
* @return bool true if an OTA message is waiting and the message
*         size is greater than OTA_UPDATE_MSG_HEADER_SIZE;
*/
static bool otaMsgMgr_checkForOta(void)
{
    bool status = false;
    uint8_t numOtaMessages = modemMgr_getNumOtaMsgsPending();
    uint16_t sizeOfOtaMessages = modemMgr_getSizeOfOtaMsgsPending();

    sizeOfOtaMessages = modemMgr_getSizeOfOtaMsgsPending();
    if (numOtaMessages && (sizeOfOtaMessages > 0))
    {
        status = true;
    }
    return (status);
}

/**************************************************** 
  * Functions to process a firmware update message
  ***************************************************/

/**
* \brief Process a Fw Update OTA message.  
*/
static void otaMsgMgr_processFwUpdateMsg(void)
{
    switch (otaData.otaFlashState)
    {
        case OTA_FLASH_STATE_START:
            fwUpdateMsg_start();
            break;
        case OTA_FLASH_STATE_GET_MSG_INFO:
            fwUpdateMsg_getMsgInfo();
            break;
        case OTA_FLASH_STATE_GET_SECTION_INFO:
            fwUpdateMsg_getSectionInfo();
            break;
        case OTA_FLASH_STATE_WRITE_SECTION_DATA:
            fwUpdateMsg_writeSectionData();
            break;
    }
}

/**
* \brief Firmware Update Processing Function
*/
static void fwUpdateMsg_start(void)
{
    // Get the buffer that contains the OTA message data
    otaResponse_t *otaRespP = modemMgr_getLastOtaResponse();
    // First byte is the OTA opcode
    OtaOpcode_t opcode = (OtaOpcode_t)otaRespP->buf[0];

    if (opcode != OTA_OPCODE_FIRMWARE_UPGRADE)
    {
        otaData.fwUpdateResult = RESULT_NO_FWUPGRADE_PERFORMED;
        otaData.doneProcessingFwUpdateMsg = true;
    }
    else
    {
        otaData.modemRequestLength = OTA_UPDATE_MSG_HEADER_SIZE;
        otaData.otaFlashState = OTA_FLASH_STATE_GET_MSG_INFO;
        otaData.modemRequestOffset = 0;
    }
}

/**
* \brief Firmware Update Processing Function
*/
static void fwUpdateMsg_getMsgInfo(void)
{
    // Get the buffer that contains the OTA message data
    otaResponse_t *otaRespP = modemMgr_getLastOtaResponse();
    // First byte is the OTA opcode
    OtaOpcode_t opcode = (OtaOpcode_t)otaRespP->buf[0];

    if (opcode != OTA_OPCODE_FIRMWARE_UPGRADE)
    {
        otaData.fwUpdateResult = RESULT_NO_FWUPGRADE_PERFORMED;
        otaData.doneProcessingFwUpdateMsg = true;
    }
    else
    {
        // Save the message ID
        otaData.lastMsgId = (otaRespP->buf[1] << 8) | otaRespP->buf[2];
        uint8_t *bufP = &otaRespP->buf[0];

        // Verify Keys of the firmware upgrade message
        if ((bufP[3] == FLASH_UPGRADE_KEY1) &&
            (bufP[4] == FLASH_UPGRADE_KEY2) &&
            (bufP[5] == FLASH_UPGRADE_KEY3) &&
            (bufP[6] == FLASH_UPGRADE_KEY4))
        {
            otaData.totalSections = bufP[7];
            otaData.nextSectionNumber = 0;
            otaData.modemRequestLength = OTA_UPDATE_SECTION_HEADER_SIZE;
            otaData.otaFlashState = OTA_FLASH_STATE_GET_SECTION_INFO;
        }
        else
        {
            otaData.fwUpdateResult = RESULT_DONE_ERROR;
            otaData.doneProcessingFwUpdateMsg = true;
        }
    }
}

/**
* \brief Firmware Update Processing Function
*/
static void fwUpdateMsg_getSectionInfo(void)
{

    // Setup addresses
    uint16_t bootFlashStartAddr = getBootImageStartAddr();
    uint16_t appFlashStartAddr = getAppImageStartAddr();
    uint16_t appFlashLength = getAppImageLength();

    // Get the buffer that contains the OTA message data
    otaResponse_t *otaRespP = modemMgr_getLastOtaResponse();

    // Get the buffer that contains the OTA message data
    volatile uint8_t *bufP = &otaRespP->buf[0];
    uint8_t sectionStartId = *bufP++;
    uint8_t sectionNumber = *bufP++;

    // Check start section byte and section number in message
    if ((sectionStartId == FLASH_UPGRADE_SECTION_START) &&
        (sectionNumber == otaData.nextSectionNumber++))
    {
        // Retrieve Start Address from section info in message
        otaData.sectionStartAddrP = (*bufP++ << 8);
        otaData.sectionStartAddrP |= *bufP++;
        otaData.sectionWriteAddrP = (uint8_t *)otaData.sectionStartAddrP;

        // Retrieve Length from section info in message
        otaData.sectionDataLength = (*bufP++ << 8);
        otaData.sectionDataLength |= *bufP++;
        otaData.sectionDataRemaining = otaData.sectionDataLength;
        // Retrieve CRC from section info in message
        otaData.sectionCrc16 = (*bufP++ << 8);
        otaData.sectionCrc16 |= *bufP++;

        // Verify the section parameters
        // 1. Check that the modem message has enough data to fill the section.
        // 2. Check that the section start and end address is located in app flash area
        // Otherwise we consider this a catastrophic error.

        uint16_t startBurnAddr = otaData.sectionStartAddrP;
        uint16_t endBurnAddr = otaData.sectionStartAddrP + otaData.sectionDataLength;

        if ((otaRespP->remainingInBytes >= otaData.sectionDataLength) &&
            (otaData.sectionDataLength <= appFlashLength) &&
            (startBurnAddr >= appFlashStartAddr) &&
            (startBurnAddr < bootFlashStartAddr) &&
            (endBurnAddr > appFlashStartAddr) &&
            (endBurnAddr <= bootFlashStartAddr))
        {
            // We have verified the section information.  Now erase the flash.
            fwUpdateMsg_eraseSection();
        }
        else
        {
            // Set error flags
            otaData.fwUpdateResult = RESULT_DONE_ERROR;
            otaData.fwUpdateErrNum = OTA_FW_UP_ERR_PARAMETER;
            otaData.doneProcessingFwUpdateMsg = true;
        }
    }
    else
    {
        // Set error flags
        otaData.fwUpdateResult = RESULT_DONE_ERROR;
        otaData.fwUpdateErrNum = OTA_FW_UP_ERR_SECTION_HEADER;
        otaData.doneProcessingFwUpdateMsg = true;
    }
}

/**
* \brief Firmware Update Processing Function
*/
static void fwUpdateMsg_eraseSection(void)
{
    uint8_t i;
    uint16_t numSectors = getNumSectorsInImage();
    uint8_t *flashSegmentAddrP = (uint8_t *)otaData.sectionStartAddrP;
    uint8_t *bootFlashStartAddrP = (uint8_t *)getBootImageStartAddr();

    // Before we start wiping out the application, zero the app reset vector
    // so that we can never go back unless the programming completes successfully.
    // The app reset vector is always checked for a valid range before jumping to it.
    msp430Flash_zeroAppResetVector();

    // Erase the flash segments
    for (i = 0; i < numSectors; i++, flashSegmentAddrP += 0x200)
    {
        // Check that the address falls below the bootloader flash area
        if (flashSegmentAddrP < bootFlashStartAddrP)
        {
            // Tickle the watchdog before erasing
            WATCHDOG_TICKLE();
            msp430Flash_erase_segment(flashSegmentAddrP);
        }
    }

    // Set up for starting the write data to flash.
    // Initialize request size from modem.
    // The maximum data we can request from the modem at one time is
    // OTA_PAYLOAD_BUF_LENGTH
    if (otaData.sectionDataRemaining > OTA_PAYLOAD_BUF_LENGTH)
    {
        otaData.modemRequestLength = OTA_PAYLOAD_BUF_LENGTH;
    }
    else
    {
        otaData.modemRequestLength = otaData.sectionDataRemaining;
    }

    // Setup state for the write to flash sequence.
    otaData.otaFlashState = OTA_FLASH_STATE_WRITE_SECTION_DATA;
}

/**
* \brief Firmware Update Processing Function
*/
static void fwUpdateMsg_writeSectionData(void)
{
    // Get the buffer that contains the OTA message data
    otaResponse_t *otaRespP = modemMgr_getLastOtaResponse();
    uint8_t *bufP = &otaRespP->buf[0];

    // Make sure we got data back from the modem, else its an error condition.
    if (otaRespP->lengthInBytes > 0)
    {

        uint16_t writeDataSize = otaRespP->lengthInBytes;
        uint8_t *bootFlashStartAddrP = (uint8_t *)getBootImageStartAddr();

        // This should not happen but just in case, make sure
        // we did not get too much data back from the modem.
        if (otaData.sectionDataRemaining < writeDataSize)
        {
            writeDataSize = otaData.sectionDataRemaining;
        }

        // Write the data to flash.
        // Check that we are not going to write into the bootloader flash area.
        if ((otaData.sectionWriteAddrP + writeDataSize) <= bootFlashStartAddrP)
        {
            // Tickle the watchdog before erasing
            WATCHDOG_TICKLE();
            msp430Flash_write_bytes(otaData.sectionWriteAddrP, &bufP[0], writeDataSize);
        }

        // Update counters and flash pointer
        otaData.sectionDataRemaining -= writeDataSize;
        otaData.sectionWriteAddrP += writeDataSize;

        // Check if we have all the data for the section.
        // If so, call the flash verify function.
        // Otherwise, set request length for next buffer of data.
        if (otaData.sectionDataRemaining == 0x0)
        {
            fwUpdateMsg_verifySection();
        }
        else
        {
            // We need more data.
            if (otaData.sectionDataRemaining > OTA_PAYLOAD_BUF_LENGTH)
            {
                otaData.modemRequestLength = OTA_PAYLOAD_BUF_LENGTH;
            }
            else
            {
                otaData.modemRequestLength = otaData.sectionDataRemaining;
            }
        }
    }
    else
    {
        // We did not get any data back from the modem.
        // Error condition.
        otaData.fwUpdateResult = RESULT_DONE_ERROR;
        otaData.fwUpdateErrNum = OTA_FW_UP_ERR_MODEM;
        otaData.doneProcessingFwUpdateMsg = true;
        // Zero the app reset vector so that this app image will not be used.
        msp430Flash_zeroAppResetVector();
    }
}

/**
* \brief Firmware Update Processing Function
*/
static void fwUpdateMsg_verifySection(void)
{
    uint16_t calcCrc16 = gen_crc16((const unsigned char *)otaData.sectionStartAddrP, otaData.sectionDataLength);

    otaData.lastCalcCrc16 = calcCrc16;
    if (calcCrc16 == otaData.sectionCrc16)
    {
        otaData.fwUpdateResult = RESULT_DONE_SUCCESS;
    }
    else
    {
        // The CRC failed.
        // Error condition.
        otaData.fwUpdateResult = RESULT_DONE_ERROR;
        otaData.fwUpdateErrNum = OTA_FW_UP_ERR_CRC;
        // Zero the app reset vector so that this app image will not be used.
        msp430Flash_zeroAppResetVector();
    }
    otaData.doneProcessingFwUpdateMsg = true;
}

