/** 
 * @file msgData.c
 * \n Source File
 * \n Outpour MSP430 Firmware
 * 
 * \brief Manage the high level details of sending data messages
 *        to the modem.
 *  
 * \note There are two ways to send a message to the modem: 
 *       immediate and scheduled. To send a message immediately,
 *       usd the dataMsgMgr_sendDataMsg function. To schedule a
 *       message to be transmitted at 1:00AM, use the scheduler
 *       API's (in msgScheduler.c). The scheduler kicks off the
 *       message transmission session at 1:00AM if there are
 *       messages scheduled. The scheduler calls the
 *       dataMsgMgr_startSendingScheduled function to kick off
 *       the transmission session.
 */

#include "outpour.h"

/***************************
 * Module Data Definitions
 **************************/

/**
 * \def DATA_MSG_MAX_RETRIES
 * \brief Specify how many retries to attempt to send the data 
 *        msg if the network was not able to connect.
 */
#define DATA_MSG_MAX_RETRIES ((uint8_t)1)

/**
 * \def DATA_MSG_DELAY_IN_SECONDS_TILL_RETRY
 * \brief Specify how long to wait to retransmit as a result of 
 *        the modem failing to connect to the network.
 * \note Max hours in a uint16_t representing seconds is 18.2 
 *       hours.  Currently set at 12 hours.
 */
#define DATA_MSG_DELAY_IN_SECONDS_TILL_RETRY ((uint16_t)12*60*60)

/**
 * \typedef msgData_t
 * \brief Define a container to store data and state information
 *        to support sending a data message to the modem.
 */
typedef struct msgData_s {
    bool sendDataMsgActive;                                /**< flag to mark a data message is in progress */
    bool sendDataMsgRetryScheduled;                        /**< flag to mark a data message is scheduled */
    uint8_t retryCount;                                    /**< number of retries attempted */
    uint16_t secsTillTransmit;                             /**< time in seconds until transmit: max is 18.2 hours as 16 bit value */
    dataMsgSm_t dataMsgSm;                                 /**< Data message state machine object */
} msgData_t;

/****************************
 * Module Data Declarations
 ***************************/

/**
* \var msgData
* \brief Declare the data msg object.
*/
// static
msgData_t msgData;

/*************************
 * Module Prototypes
 ************************/

/***************************
 * Module Public Functions
 **************************/

/**
* \brief Call once as system startup to initialize the data 
*        message module.
* \ingroup PUBLIC_API
*/
void dataMsgMgr_init(void)
{
    memset(&msgData, 0, sizeof(msgData_t));
    msgData.sendDataMsgActive = 0;                         // so compiler doesn't think it is not initialized
}

/**
* \brief Return status on whether the data message manager is 
*        busy sending a message or not.
* 
* @return bool Returns true is actively sending.
*/
bool dataMsgMgr_isSendMsgActive(void)
{
    return (msgData.sendDataMsgActive);
}

/**
* \brief Exec routine should be called once every time from the 
*        main processing loop.  Drives the data message state
*        machine if there is a data message transmit currently
*        in progress.
* \ingroup EXEC_ROUTINE
*/
void dataMsgMgr_exec(void)
{

    if (msgData.sendDataMsgActive)
    {

        dataMsgSm_t *dataMsgSmP = &msgData.dataMsgSm;

        // Handle sending multiple messages.
        // Check if the data message state machine is done sending the current
        // message. If done, then check if there is another message to send.
        // If there is, start sending it.
        if (dataMsgSmP->sendCmdDone)
        {
            modemCmdWriteData_t *cmdWriteP = &dataMsgSmP->cmdWrite;

            // Check if there is another message to send.
            // If so, scheduler returns a non-zero value representing the length.
            msgSched_getNextMessageToTransmit(cmdWriteP);
            if (cmdWriteP->payloadLength)
            {
                // Update the data message state machine so that it will be
                // in the correct state to send the next message.
                dataMsgSm_sendAnotherDataMsg(dataMsgSmP);
            }
        }

        // Call the data message state machine to perform work
        dataMsgSm_stateMachine(dataMsgSmP);

        // Check if the data message state machine is done with the
        // complete session
        if (dataMsgSmP->allDone)
        {
            msgData.sendDataMsgActive = false;
            // Check if a modem network connect timeout occurred
            if (dataMsgSmP->connectTimeout)
            {
                // Error case
                // Check if this already was a retry
                // If not, then schedule a retry.  If it was, then abort.
                if (msgData.retryCount < DATA_MSG_MAX_RETRIES)
                {
                    msgData.retryCount++;
                    msgData.sendDataMsgRetryScheduled = true;
                    msgData.secsTillTransmit = DATA_MSG_DELAY_IN_SECONDS_TILL_RETRY;
                }
            }
        }
    }
    else if (msgData.sendDataMsgRetryScheduled)
    {
        if (msgData.secsTillTransmit > 0)
        {
            msgData.secsTillTransmit--;
        }
        if (msgData.secsTillTransmit == 0)
        {
            // The sendWaterMsg function will clear the retryCount.
            // We need to save the current value so we can restore it.
            uint8_t retryCount = msgData.retryCount;

            // Use the standard data msg API to initiate the retry.
            // For retries, we assume the data is already stored in the modem
            // from the original try. We only have to "kick" the modem with any
            // type of M_COMMAND_SEND_DATA msg to get it to send out what it has
            // stored in its FIFOs (once its connected to the network).
            dataMsgMgr_sendDataMsg(MSG_TYPE_RETRYBYTE, NULL, 0);
            // Restore retryCount value.
            msgData.retryCount = retryCount;
        }
    }
}

/*******************************************************************************/
/*******************************************************************************/

/**
* \brief Used by upper layers to start off a multi-module/multi
*        state machine sequence for sending all scheduled data
*        commands to the modem.
*
* \ingroup PUBLIC_API
*  
* \note If the modem does not connect to the network within a
*       specified time frame (WAIT_FOR_LINK_UP_TIME_IN_SECONDS),
*       then one retry will be scheduled for the future.
*/
bool dataMsgMgr_startSendingScheduled(void)
{
    dataMsgSm_t *dataMsgSmP = &msgData.dataMsgSm;

    // If already busy, just return.
    // Should never happen, but just in case.
    if (msgData.sendDataMsgActive)
    {
        return (false);
    }

    // Note - a new data transmission will cancel any scheduled re-transmission

    msgData.sendDataMsgActive = true;
    msgData.sendDataMsgRetryScheduled = false;
    msgData.retryCount = 0;
    msgData.secsTillTransmit = 0;

    // Initialize the data msg object.
    dataMsgSm_initForNewSession(dataMsgSmP);

    return (true);
}

/**
* \brief Used by upper layers to send a data msg to the modem.
*        This kicks off a multi-module/multi state machine
*        sequence for sending the data command to the modem.
*        Note that this does not schedule a message for
*        transmission in the future. The function will
*        immediately start the transmission process.
*
* \ingroup PUBLIC_API
*  
* \note If the modem does not connect to the network within a
*       specified time frame (WAIT_FOR_LINK_UP_TIME_IN_SECONDS),
*       then one retry will be scheduled for the future.
* 
* @param msgId The outpour message identifier
* @param dataP Pointer to the data to send
* @param lengthInBytes The length of the data to send.
*/
bool dataMsgMgr_sendDataMsg(MessageType_t msgId, uint8_t *dataP, uint16_t lengthInBytes)
{
    dataMsgSm_t *dataMsgSmP = &msgData.dataMsgSm;

    // If already busy, just return.
    // Should never happen, but just in case.
    if (msgData.sendDataMsgActive)
    {
        return (false);
    }

    // Note - a new data transmission will cancel any scheduled re-transmission

    msgData.sendDataMsgActive = true;
    msgData.sendDataMsgRetryScheduled = false;
    msgData.retryCount = 0;
    msgData.secsTillTransmit = 0;

    // Initialize the data msg object.
    dataMsgSm_initForNewSession(dataMsgSmP);

    // Initialize the data command object in the data object
    dataMsgSmP->cmdWrite.cmd = OUTPOUR_M_COMMAND_SEND_DATA;
    dataMsgSmP->cmdWrite.payloadMsgId = msgId;             /* the payload type */
    dataMsgSmP->cmdWrite.payloadP = dataP;                 /* the payload pointer */
    dataMsgSmP->cmdWrite.payloadLength = lengthInBytes;    /* size of the payload in bytes */

    // Call the state machine
    dataMsgSm_stateMachine(dataMsgSmP);

    return (true);
}

/**
* \brief Used by upper layers to send a cascade test msg to the
*        modem. This kicks off a multi-module/multi state
*        machine sequence for sending the data command to the
*        modem.
*
* \ingroup PUBLIC_API
*  
* \note If the modem does not connect to the network within a
*       specified time frame (WAIT_FOR_LINK_UP_TIME_IN_SECONDS),
*       then one retry will be scheduled for the future.
* 
* @param msgId The cascade message identifier
* @param dataP Pointer to the data to send
* @param lengthInBytes The length of the data to send.
*/
bool dataMsgMgr_sendTestMsg(MessageType_t msgId, uint8_t *dataP, uint16_t lengthInBytes)
{
    dataMsgSm_t *dataMsgSmP = &msgData.dataMsgSm;

    // If already busy, just return.
    // Should never happen, but just in case.
    if (msgData.sendDataMsgActive)
    {
        return (false);
    }

    // Note - a new data transmission will cancel any scheduled re-transmission

    msgData.sendDataMsgActive = true;
    msgData.sendDataMsgRetryScheduled = false;
    msgData.retryCount = 0;
    msgData.secsTillTransmit = 0;

    // Initialize the data msg object.
    dataMsgSm_initForNewSession(dataMsgSmP);

    // Initialize the data command object in the data object
    dataMsgSmP->cmdWrite.cmd = OUTPOUR_M_COMMAND_SEND_TEST;
    dataMsgSmP->cmdWrite.payloadMsgId = msgId;             /* the payload type */
    dataMsgSmP->cmdWrite.payloadP = dataP;                 /* the payload pointer */
    dataMsgSmP->cmdWrite.payloadLength = lengthInBytes;    /* size of the payload in bytes */

    // Call the state machine
    dataMsgSm_stateMachine(dataMsgSmP);

    return (true);
}
