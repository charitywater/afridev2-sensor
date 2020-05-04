/** 
 * @file msgOta.c
 * \n Source File
 * \n AfridevV2 MSP430 Firmware
 * 
 * \brief Manage the details of processing OTA messages.  Runs 
 * the OTA state machine to retrieve OTA messages from the 
 * modem, process them and send OTA responses. 
 *  
 */
#include "outpour.h"

/***************************
 * Module Data Definitions
 **************************/

/**
 * \typedef otaState_t
 * \brief The OTA state machine is what drives receiving and 
 *        sending OTA responses.  Specify the different states
 *        for retrieving OTA messages from the modem and for
 *        sending OTA responses to the modem.
 */
typedef enum otaState_e {
    OTA_STATE_IDLE,
    OTA_STATE_SEND_OTA_CMD_PHASE0,
    OTA_STATE_OTA_CMD_PHASE0_WAIT,
    OTA_STATE_PROCESS_OTA_CMD_PHASE0,
    OTA_STATE_SEND_OTA_CMD_PHASE1,
    OTA_STATE_OTA_CMD_PHASE1_WAIT,
    OTA_STATE_PROCESS_OTA_CMD_PHASE1,
    OTA_STATE_SEND_DELETE_OTA_CMD,
    OTA_STATE_DELETE_OTA_CMD_WAIT,
    OTA_STATE_SEND_OTA_RESPONSE,
    OTA_STATE_SEND_OTA_RESPONSE_WAIT,
    OTA_STATE_CHECK_FOR_MORE_MESSAGES,
    OTA_STATE_POST_PROCESS,
    OTA_STATE_DONE,
} otaState_t;

/**
 * \def RESPONSE_DATA_OFFSET 
 * \brief Offset into response buffer to put response data. 
 *        This goes after the response header, msg opcode (1
 *        byte) and msg id (2 bytes).
 */
#define RESPONSE_DATA_OFFSET (OTA_RESPONSE_HEADER_LENGTH + 3)

/**
 * \def MAX_OTA_MEMORY_READ_BYTES
 * \brief For the OTA memory read debug command, specify the max
 *        number of bytes that can fit into the OTA data
 *        response portion of the response after the return
 *        parameters.
 * \li byte 0   = msg op code
 * \li byte 1,2 = msg id
 * \li byte 3   = status
 * \li byte 4,5 = address
 * \li byte 6   = readLength
 * \li byte 7   = readWidth
 * \li byte 8   = start of return data
 */
// #define MAX_OTA_MEMORY_READ_BYTES (OTA_PAYLOAD_BUF_LENGTH - 8)
#define MAX_OTA_MEMORY_READ_BYTES (255)

/**
 * \typedef otaData_t
 *  \brief Define a container (i.e. structure) to hold data for
 *         the OTA message module.
 */
typedef struct otaData_s {
    bool active;                                           /**< Identifies that OTA message processing is in progress  */
    otaState_t otaState;                                   /**< Current state */
    modemCmdWriteData_t cmdWrite;                          /**< Modem write cmd object for lower-level functions */
    uint8_t *responseBufP;                                 /**< Buffer to assemble an OTA response that will be transmitted */
    uint8_t totalMsgsProcessed;                            /**< Count of messages processed per session */
    uint8_t totalPostMessagesSent;                         /**< Count of total unsolicited messages sent */
    OtaOpcode_t lastMsgOpcode;                             /**< Save the last OTA opcode */
    uint16_t lastMsgId;                                    /**< Save the last OTA msgId */
    bool deleteOtaMessage;                                 /**< Flag to indicate current OTA should be deleted */
    bool sendOtaResponse;                                  /**< Flag to indicate current OTA response ready */
    uint8_t activateReboot;                                /**< If reboot required by msg, must be set correctly to reboot */
    uint8_t activateFwUpgrade;                             /**< Identify that the special fw upgrade message was received */
    bool gmtTimeHasBeenUpdated;                            /**< Flag to indicate GMT time has been updated */
    bool gmtTimeUpdateCandidate;                           /**< Flag to indicate there is a GMT update candidate */
    uint16_t gmtCandidateMsgId;                            /**< MsgId from gmt candidate */
    uint8_t gmtBinSecondsOffset;                           /**< Value from gmt update msg candidate */
    uint8_t gmtBinMinutesOffset;                           /**< Value from gmt update msg candidate */
    uint8_t gmtBinHoursOffset;                             /**< Value from gmt update msg candidate */
    uint16_t gmtBinDaysOffset;                             /**< Value from gmt update msg candidate */
} otaData_t;

/****************************
 * Module Data Declarations
 ***************************/

/**
* \var otaData
* \brief Declare the OTA message manager object.
*/
// static
otaData_t otaData;

/*************************
 * Module Prototypes
 ************************/
static void otaMsgMgr_stateMachine(void);
static void otaMsgMgr_processOtaMsg(void);
static uint8_t otaMsgMgr_getOtaLength(void);
static bool otaMsgMgr_processGmtClocksetPart1(otaResponse_t *otaRespP);
static bool otaMsgMgr_processGmtClocksetPart2(void);
static bool otaMsgMgr_processLocalOffset(otaResponse_t *otaRespP);
static bool otaMsgMgr_processResetData(otaResponse_t *otaRespP);
static bool otaMsgMgr_processResetRedFlag(otaResponse_t *otaRespP);
static bool otaMsgMgr_processActivateDevice(otaResponse_t *otaRespP);
static bool otaMsgMgr_processSilenceDevice(otaResponse_t *otaRespP);
static bool otaMsgMgr_processFirmwareUpgrade(otaResponse_t *otaRespP);
static bool otaMsgMgr_processResetDevice(otaResponse_t *otaRespP);
static bool otaMsgMgr_processSetTransmissionRate(otaResponse_t *otaRespP);
static bool otaMsgMgr_processClockRequest(otaResponse_t *otaRespP);
static bool otaMsgMgr_processGpsRequest(otaResponse_t *otaRespP);
static bool otaMsgMgr_setGpsMeasCriteria(otaResponse_t *otaRespP);
static bool otaMsgMgr_processUnknownRequest(otaResponse_t *otaRespP);
static bool otaMsgMgr_processMemoryRead(otaResponse_t *otaRespP);
static void sendPhase0_OtaCommand(void);
static void sendPhase1_OtaCommand(uint8_t lengthInBytes);
static void sendDelete_OtaCommand(void);
static void postOtaMessageProcessing(void);
static void prepareOtaResponse(uint8_t opcode, uint8_t msgId0, uint8_t msgId1,
                               uint8_t *dataResponseP, uint8_t dataResponseLen);

/***************************
 * Module Public Functions
 **************************/

// For debug support - trace the state transitions.  Not used by default.
//#define STATE_TRACE
#ifdef STATE_TRACE
#define MAX_STATE_TRACE_SIZE 64
typedef struct state_trace_s {
    otaState_t state;
    OtaOpcode_t arg;
} state_trace_t;
state_trace_t stateTrace[MAX_STATE_TRACE_SIZE];
uint8_t stateTraceIndex = 0;
void addStateTracePoint(otaState_t state, OtaOpcode_t b1)
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
*        Will only run the state machine if activated.
*        Activation is performed by calling the
*        otaMsgMgr_getAndProcessOtaMsgs function.
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
    otaData.otaState = OTA_STATE_IDLE;
    otaData.responseBufP = modemMgr_getSharedBuffer();
    addStateTracePoint(otaData.otaState, (OtaOpcode_t)0);
}

/**
* \brief Kick-off the ota state machine processing for 
*        retrieving OTA messages from the modem and sending
*        responses. The state machine will retrieve messages
*        one-by-one from the modem and process them.  Once
*        started here, the state machine is executed repeatedly
*        from the exec routine until all OTA messages have been
*        processed and responses sent.
* \ingroup PUBLIC_API
*/
void otaMsgMgr_getAndProcessOtaMsgs(void)
{
    otaData.active = true;
    otaData.otaState = OTA_STATE_SEND_OTA_CMD_PHASE0;
    addStateTracePoint(otaData.otaState, (OtaOpcode_t)0);
    otaData.totalMsgsProcessed = 0;
    otaData.gmtCandidateMsgId = 0;
    otaData.gmtTimeUpdateCandidate = false;
    otaData.gmtTimeHasBeenUpdated = false;
    otaData.totalPostMessagesSent = 0;
    otaData.sendOtaResponse = false;
    otaData.deleteOtaMessage = false;
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

/*************************
 * Module Private Functions
 ************************/

/**
* \brief Helper function to send a "reconnaissance" get incoming
*        partial cmd to the modem.  The payloadLength is set to
*        zero, as we are just interested in finding out how big
*        the message is before requesting the full message.  By
*        setting the payloadLength to zero, the modem does not
*        return any of the data, only how much message data is
*        stored in the modem.
*/
static void sendPhase0_OtaCommand(void)
{
    otaData.totalMsgsProcessed++;
    memset(&otaData.cmdWrite, 0, sizeof(modemCmdWriteData_t));
    otaData.cmdWrite.cmd = OUTPOUR_M_COMMAND_GET_INCOMING_PARTIAL;
    otaData.cmdWrite.payloadLength = 0;
    otaData.cmdWrite.payloadOffset = 0;
    modemMgr_sendModemCmdBatch(&otaData.cmdWrite);
}

/**
* \brief Helper function to send a get incoming partial cmd to
*        the modem to retrieve the OTA Message.
* 
* @param lengthInBytes Pre-determined length in bytes of the OTA 
*                      message as found from the reconnaissance
*                      get incoming partial message previously sent.
*/
static void sendPhase1_OtaCommand(uint8_t lengthInBytes)
{
    memset(&otaData.cmdWrite, 0, sizeof(modemCmdWriteData_t));
    otaData.cmdWrite.cmd = OUTPOUR_M_COMMAND_GET_INCOMING_PARTIAL;
    otaData.cmdWrite.payloadLength = lengthInBytes;
    otaData.cmdWrite.payloadOffset = 0;
    modemMgr_sendModemCmdBatch(&otaData.cmdWrite);
}

/**
* \brief Helper function to send a delete partial command to the 
*        modem.
* 
*/
static void sendDelete_OtaCommand(void)
{
    memset(&otaData.cmdWrite, 0, sizeof(modemCmdWriteData_t));
    otaData.cmdWrite.cmd = OUTPOUR_M_COMMAND_DELETE_INCOMING;
    modemMgr_sendModemCmdBatch(&otaData.cmdWrite);
}

/**
* \brief This is the OTA state machine.  It sequences through 
*        the steps to retrieve messages from the modem and for
*        sending responses to the modem.  It is designed so that
*        it can be called repeatedly from the exec function
*        (which runs every 1 second via the main processing
*        loop). For some states, it will only run one iteration,
*        and then exit until the next exec call. For other
*        states it will run multiple iterations. This is
*        controlled by the variable continue_processing.
*/
static void otaMsgMgr_stateMachine(void)
{
    uint8_t otaMsgByteLength = 0;
    bool continue_processing = false;

    do
    {
        // By default, run only one loop iteration of this function.  Certain
        // states will set continue_processing to true if more than one loop
        // iteration should be performed.
        continue_processing = false;

        // Switch on the current state.
        switch (otaData.otaState)
        {

            // Non-active, IDLE state
            case OTA_STATE_IDLE:
                break;

                /** 
                 * PHASE 0 - send a "reconnaissance" get incoming partial cmd to the modem.
                 * The payloadLength is set to zero, as we are just interested in finding out
                 *  how big the message is before requesting the full message.
                 */
            case OTA_STATE_SEND_OTA_CMD_PHASE0:
                sendPhase0_OtaCommand();
                otaData.otaState = OTA_STATE_OTA_CMD_PHASE0_WAIT;
                addStateTracePoint(otaData.otaState, (OtaOpcode_t)0);
                break;
            case OTA_STATE_OTA_CMD_PHASE0_WAIT:
                if (modemMgr_isModemCmdError())
                {
                    otaData.otaState = OTA_STATE_SEND_DELETE_OTA_CMD;
                    addStateTracePoint(otaData.otaState, (OtaOpcode_t)0);
                    continue_processing = true;
                }
                else if (modemMgr_isModemCmdComplete())
                {
                    otaData.otaState = OTA_STATE_PROCESS_OTA_CMD_PHASE0;
                    addStateTracePoint(otaData.otaState, (OtaOpcode_t)0);
                    continue_processing = true;
                }
                break;
            case OTA_STATE_PROCESS_OTA_CMD_PHASE0:
                otaMsgByteLength = otaMsgMgr_getOtaLength();
                otaData.otaState = otaMsgByteLength ?
                                   OTA_STATE_SEND_OTA_CMD_PHASE1 : OTA_STATE_SEND_DELETE_OTA_CMD;
                addStateTracePoint(otaData.otaState, (OtaOpcode_t)0);
                continue_processing = true;
                break;

                /**
                 * PHASE 1 - send an incoming partial cmd to the modem to retrieve
                 * the OTA message.  The length was previously determined in phase 0.
                 */
            case OTA_STATE_SEND_OTA_CMD_PHASE1:
                sendPhase1_OtaCommand(otaMsgByteLength);
                otaData.otaState = OTA_STATE_OTA_CMD_PHASE1_WAIT;
                addStateTracePoint(otaData.otaState, (OtaOpcode_t)0);
                break;
            case OTA_STATE_OTA_CMD_PHASE1_WAIT:
                if (modemMgr_isModemCmdError())
                {
                    otaData.otaState = OTA_STATE_SEND_DELETE_OTA_CMD;
                    addStateTracePoint(otaData.otaState, (OtaOpcode_t)0);
                    continue_processing = true;
                }
                else if (modemMgr_isModemCmdComplete())
                {
                    otaData.otaState = OTA_STATE_PROCESS_OTA_CMD_PHASE1;
                    addStateTracePoint(otaData.otaState, (OtaOpcode_t)0);
                    continue_processing = true;
                }
                break;
            case OTA_STATE_PROCESS_OTA_CMD_PHASE1:
            {
                // Process the data retrieved from the modem
                otaMsgMgr_processOtaMsg();

                // Check status flags to determine next state
                if (otaData.sendOtaResponse)
                {
                    otaData.otaState = OTA_STATE_SEND_OTA_RESPONSE;
                    addStateTracePoint(otaData.otaState, otaData.lastMsgOpcode);
                }
                else if (otaData.deleteOtaMessage)
                {
                    otaData.otaState = OTA_STATE_SEND_DELETE_OTA_CMD;
                    addStateTracePoint(otaData.otaState, otaData.lastMsgOpcode);
                }

                continue_processing = true;
            }
                break;

                /**
                 * Send the OTA Reply
                 */
            case OTA_STATE_SEND_OTA_RESPONSE:
                modemMgr_sendModemCmdBatch(&otaData.cmdWrite);
                otaData.sendOtaResponse = false;
                otaData.otaState = OTA_STATE_SEND_OTA_RESPONSE_WAIT;
                addStateTracePoint(otaData.otaState, (OtaOpcode_t)otaData.cmdWrite.payloadP[14]);
                break;
            case OTA_STATE_SEND_OTA_RESPONSE_WAIT:
                // Wait until modem manager is done sending the OTA message
                // or an error occurred.
                if (modemMgr_isModemCmdComplete() || modemMgr_isModemCmdError())
                {

                    // If a firmware upgrade message was received, directly go to post processing
                    // as we don't want to delete that message or perform any additional OTA processing.
                    // We need to jump into the bootloader, so a system reboot sequence will be started.
                    // Otherwise delete the OTA message just processed, or check for more OTA messages.
                    if (otaData.activateFwUpgrade == ACTIVATE_FWUPGRADE_KEY)
                    {
                        otaData.otaState = OTA_STATE_POST_PROCESS;
                    }
                    else if (otaData.deleteOtaMessage)
                    {
                        otaData.otaState = OTA_STATE_SEND_DELETE_OTA_CMD;
                        addStateTracePoint(otaData.otaState, (OtaOpcode_t)0);
                    }
                    else
                    {
                        otaData.otaState = OTA_STATE_CHECK_FOR_MORE_MESSAGES;
                        addStateTracePoint(otaData.otaState, (OtaOpcode_t)0);
                    }
                    continue_processing = true;
                }
                break;

                /**
                 * Delete the OTA Command
                 */
            case OTA_STATE_SEND_DELETE_OTA_CMD:
                sendDelete_OtaCommand();
                otaData.deleteOtaMessage = false;
                otaData.otaState = OTA_STATE_DELETE_OTA_CMD_WAIT;
                addStateTracePoint(otaData.otaState, otaData.lastMsgOpcode);
                break;
            case OTA_STATE_DELETE_OTA_CMD_WAIT:
                if (modemMgr_isModemCmdError() || modemMgr_isModemCmdComplete())
                {
                    // If there is a reboot pending, don't process any more messages.
                    // Go directly to post-processing where a reboot sequence will
                    // be started (if there are more messages, the strategy is to
                    // process those after the reboot).
                    if (otaData.activateReboot == ACTIVATE_REBOOT_KEY)
                    {
                        otaData.otaState = OTA_STATE_POST_PROCESS;
                    }
                    else
                    {
                        otaData.otaState = OTA_STATE_CHECK_FOR_MORE_MESSAGES;
                        addStateTracePoint(otaData.otaState, (OtaOpcode_t)0);
                    }
                    continue_processing = true;
                }
                break;

            case OTA_STATE_CHECK_FOR_MORE_MESSAGES:
                if (modemMgr_getNumOtaMsgsPending() && (otaData.totalMsgsProcessed < 50))
                {
                    // Modem has more OTA messages.
                    // Prepare state and status flags to process another OTA message.
                    otaData.sendOtaResponse = false;
                    otaData.deleteOtaMessage = false;
                    otaData.otaState = OTA_STATE_SEND_OTA_CMD_PHASE0;
                    addStateTracePoint(otaData.otaState, (OtaOpcode_t)0);
                }
                else
                {
                    otaData.otaState = OTA_STATE_POST_PROCESS;
                    addStateTracePoint(otaData.otaState, (OtaOpcode_t)0);
                }
                break;

            case OTA_STATE_POST_PROCESS:
                // Some messages may need post processing
                postOtaMessageProcessing();
                // Post processing may need to send a response
                if (otaData.sendOtaResponse && (otaData.totalPostMessagesSent < 50))
                {
                    // Post processing needs to send an OTA response.
                    otaData.otaState = OTA_STATE_SEND_OTA_RESPONSE;
                    addStateTracePoint(otaData.otaState, (OtaOpcode_t)otaData.cmdWrite.payloadP[14]);
                    otaData.totalPostMessagesSent++;
                }
                else
                {
                    otaData.otaState = OTA_STATE_DONE;
                    addStateTracePoint(otaData.otaState, (OtaOpcode_t)0);
                }
                break;

            case OTA_STATE_DONE:
                // We are done.  Update state and status flags.
                otaData.otaState = OTA_STATE_IDLE;
                addStateTracePoint(otaData.otaState, (OtaOpcode_t)0);
                otaData.active = false;
                break;
        }
    } while (continue_processing);
}

/**
* \brief Perform post OTA processing.  This function is called 
*        after all messages have been processed from the modem.
*        As part of the post processing, there may a message to
*        send to the cloud. Currently there are three OTA
*        messages that need post processing performed: GMT
*        update, firmware update and reboot request.
*  
* \note Based on the post processing performed, it may be  
*        required to send an OTA response. In that case, the
*        otaData.sendOtaResponse flag must be set to true so
*        that the OTA state machine performs the OTA message
*        transmit.  It is the responsibility of the post
*        processing function to setup the response by calling
*        the prepareOtaResponse function.
*  
*/
static void postOtaMessageProcessing(void)
{
    otaData.sendOtaResponse = false;
    // There are two commands that require rebooting after OTA message
    // processing.  This includes receiving a firmware upgrade message and
    // the reboot OTA message. After all OTA processing is done, check if
    // we need to start a reboot sequence.
    if (otaData.activateReboot == ACTIVATE_REBOOT_KEY)
    {
        sysExec_startRebootCountdown(otaData.activateReboot);
        otaData.activateReboot = 0;
    }

    // If there is a GMT update candidate, apply it now.
    else if (otaData.gmtTimeUpdateCandidate)
    {
        // Perform GMT update post processing.   If there is a GMT update
        // performed based on the post processing, an OTA response will be needed.
        otaData.sendOtaResponse = otaMsgMgr_processGmtClocksetPart2();
        // There is no OTA message to delete based on GMT OTA post processing.
        // It has already been deleted.
        otaData.deleteOtaMessage = false;
    }
}

/**
* \brief (msgId=0x1) Check and save information from a GMT Clock
*        Set message.  We only allow one GMT update to occur in
*        the system. If the GMT clock has already been updated,
*        ignore message. If GMT clock has not been updated yet,
*        then store the information to apply later. It is legal
*        to receive multiple GMT time updates messages during
*        one OTA processing session. After all OTA messages for
*        this OTA processing session has been processed, then
*        the newest GMT time update will be applied. We
*        determine newest by looking at the msgId. The higher
*        msgId takes precedence over lower msgId numbers.
*
* \brief Input OTA parameters
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
* \li gmtBinSecondsOffset (1 byte)
* \li gmtBinMinutesOffset (1 byte)
* \li gmtBinHoursOffset   (1 byte)
* \li gmtBinDaysOffset    (2 bytes)
*  
* \brief Output OTA response when candidiate is NOT accepted:
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
* \li status     (1 byte): 1 = success, 0xFF = failure
* \li reject     (1 byte): 1 = accept, 0xFF = reject
* \li gmtBinSecondsOffset (1 byte)
* \li gmtBinMinutesOffset (1 byte)
* \li gmtBinHoursOffset   (1 byte)
* \li gmtBinDaysOffset    (2 bytes)
*
* @param otaRespP Pointer to the response data and other info
*                 received from the modem.
*
* @return bool Set to true if a OTA response should be sent
*/
static bool otaMsgMgr_processGmtClocksetPart1(otaResponse_t *otaRespP)
{
    // Flag to indicate the GMT response OTA should be delayed.
    bool delayResponse = false;
    // Get the 16 bit msgId from the message.
    uint16_t msgId = (otaRespP->buf[1] << 8) | otaRespP->buf[2];
    // variables to store the previous candidates msgID and offset values (if there was one)
    uint16_t prevCandidateMsgId = 0;
    uint8_t prevSecondsOffset = otaData.gmtBinSecondsOffset;
    uint8_t prevMinutesOffset = otaData.gmtBinMinutesOffset;
    uint8_t prevHoursOffset = otaData.gmtBinHoursOffset;
    uint16_t prevDaysOffset = otaData.gmtBinDaysOffset;

    // Ignore message if a GMT update has already been applied to the system.
    if (!otaData.gmtTimeHasBeenUpdated)
    {
        // Check if this msgId is newer or equal to current candidate
        if (!otaData.gmtTimeUpdateCandidate || (msgId >= otaData.gmtCandidateMsgId))
        {
            // We have a new GMT update candidate.  Save the values to
            // apply later.
            // Note - the times are in binary/hex values
            otaData.gmtBinSecondsOffset = otaRespP->buf[3];
            otaData.gmtBinMinutesOffset = otaRespP->buf[4];
            otaData.gmtBinHoursOffset = otaRespP->buf[5];
            otaData.gmtBinDaysOffset = (otaRespP->buf[6] << 8) + otaRespP->buf[7];

            // If we don't have a previous candidate, then don't send a
            // response back yet.  If we do have a previous candidate, then
            // we will send the msgId associated with that previous Candidate
            // since we are done with it (we have a new Candidate).
            if (!otaData.gmtTimeUpdateCandidate)
            {
                delayResponse = true;
            }

            // Save the previous candidate's
            prevCandidateMsgId = otaData.gmtCandidateMsgId;
            // Update with new Candidate's msgId
            otaData.gmtCandidateMsgId = msgId;
            // Mark that we have a Candidate
            otaData.gmtTimeUpdateCandidate = true;
            // Overwrite the current msgId with the msgId of the old Candidate.
            // We will send a response back for that message.
            msgId = prevCandidateMsgId;
        }
    }

    // Prepare the OTA response if we are done with an OTA candidate.
    if (!delayResponse)
    {
        prepareOtaResponse(otaRespP->buf[0], msgId >> 8, msgId & 0xFF, NULL, 0);
        uint8_t *responseDataP = &otaData.responseBufP[RESPONSE_DATA_OFFSET];

        *responseDataP++ = 1;                              // success status
        *responseDataP++ = 0xff;                           // reject old GMT candidate
        *responseDataP++ = prevSecondsOffset;              // old candidate's seconds offset
        *responseDataP++ = prevMinutesOffset;              // old candidate's minutes offset
        *responseDataP++ = prevHoursOffset;                // old candidate's hours offset
        *responseDataP++ = prevDaysOffset >> 8;            // old candidate's days MSB offset
        *responseDataP++ = prevDaysOffset & 0xFF;          // old candidate's days LSB offset
    }
    else
    {
        // Tell state machine not to send an OTA response.  But it should
        // go ahead and delete the OTA message.
        otaData.sendOtaResponse = false;
    }

    return (!delayResponse);
}

/**
* \brief Apply a GMT Clock update.  Sets the TI Real Time Clock
*        by incrementing the time by the values received in the
*        message.  Only applied if a new candidate is available.
* \note  We need to send a OTA response for the msgId associated
*        with the GMT Update Candidate message.  It was never
*        sent as part of the Part1 GMT clock set message
*        processing.
*
* \brief Output OTA response if candidate is accepted:
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
* \li status     (1 byte): 1 = success, 0xFF = failure
* \li accept     (1 byte): 1 = accept, 0xFF = reject
* \li gmtBinSecondsOffset (1 byte)
* \li gmtBinMinutesOffset (1 byte)
* \li gmtBinHoursOffset   (1 byte)
* \li gmtBinDaysOffset    (2 bytes)
* 
*/
static bool otaMsgMgr_processGmtClocksetPart2(void)
{
    bool sendGmtResponse = false;
    timePacket_t tp;
    uint8_t *responseDataP = &otaData.responseBufP[RESPONSE_DATA_OFFSET];

    // Only apply the GMT update if a new candidate has been received and a
    // GMT update was not ever previously applied.
    if (!sendGmtResponse &&
        !otaData.gmtTimeHasBeenUpdated &&
        otaData.gmtTimeUpdateCandidate)
    {

        // Note - the times are in binary/hex values
        uint16_t i;
        uint16_t mask;

        // Disable the timer interrupt while we are updating the time.
        mask = getAndDisableSysTimerInterrupt();
        for (i = 0; i < otaData.gmtBinSecondsOffset; i++)
        {
            incrementSeconds();
        }
        for (i = 0; i < otaData.gmtBinMinutesOffset; i++)
        {
            incrementMinutes();
        }
        for (i = 0; i < otaData.gmtBinHoursOffset; i++)
        {
            incrementHours();
        }
        for (i = 0; i < otaData.gmtBinDaysOffset; i++)
        {
            incrementDays();
        }
        // Restore timer interrupt
        restoreSysTimerInterrupt(mask);

        // get rtc
        getBinTime(&tp);
        
        // When the GMT time is set, set the storage clock to the same time
        storageMgr_setStorageTime(tp.second, tp.hour24, tp.minute); 

        otaData.gmtTimeHasBeenUpdated = true;
        otaData.gmtTimeUpdateCandidate = false;

        // Prepare the OTA response for the Update Candidate message
        prepareOtaResponse(OTA_OPCODE_GMT_CLOCKSET,
                           otaData.gmtCandidateMsgId >> 8,
                           otaData.gmtCandidateMsgId & 0xFF,
                           NULL, 0);
        // Add OTA response data
        *responseDataP++ = 1;                              // status
        *responseDataP++ = 1;                              // accept GMT candidate
        *responseDataP++ = otaData.gmtBinSecondsOffset;
        *responseDataP++ = otaData.gmtBinMinutesOffset;
        *responseDataP++ = otaData.gmtBinHoursOffset;
        *responseDataP++ = otaData.gmtBinDaysOffset >> 8;
        *responseDataP++ = otaData.gmtBinDaysOffset & 0xFF;

        sendGmtResponse = true;
    }
    return (sendGmtResponse);
}

/**
* \brief (msgId=0x2) Process the Local Time Offset command. The
*        values contained in this message represent the GMT time
*        zone offset for the current time zone. For example, in
*        Ethiopia the GMT time offset is 3 hours. This means
*        that when the GMT time is 9:00PM, Ethiopian local time
*        will be midnight.
*  
* \brief Input OTA parameters
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
* \li secondsOffset (1 byte)
* \li minutesOffset (1 byte)
* \li hours24Offset (1 byte)
*  
* \brief Output OTA response
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
* \li status     (1 byte): 1 = success, 0xFF = failure
* \li secondsOffset (1 byte)
* \li minutesOffset (1 byte) 
*
* \note This function assumes positive GMT offset values (i.e.
*       GMT + X).  Targeted for Africa regions.
* 
* @param otaRespP Pointer to the response data and other info
*                 received from the modem.
*
* @return bool Set to true if a OTA response should be sent
*/
static bool otaMsgMgr_processLocalOffset(otaResponse_t *otaRespP)
{
    uint32_t temp32;
    uint8_t *responseDataP = &otaData.responseBufP[RESPONSE_DATA_OFFSET];
    uint8_t seconds = 0;
    uint8_t minutes = 0;
    uint8_t hours24 = 0;

    // Retrieve the GMT time zone offset for hours, minute and seconds
    uint8_t secondsOffset = otaRespP->buf[3];              // GMT timezone offset seconds
    uint8_t minutesOffset = otaRespP->buf[4];              // GMT timezone offset minutes
    uint8_t hours24Offset = otaRespP->buf[5];              // GMT timezone offset hours

    // Prepare the OTA response
    prepareOtaResponse(otaRespP->buf[0], otaRespP->buf[1], otaRespP->buf[2], NULL, 0);

    // Check that the numbers are reasonable
    if ((hours24Offset > 23) || (minutesOffset > 59) || (secondsOffset > 59))
    {
        // Add OTA response data - error status only
        *responseDataP++ = 0xFF;                           // error status
        *responseDataP++ = secondsOffset;                  // sent seconds offset
        *responseDataP++ = minutesOffset;                  // sent minute offset
        *responseDataP++ = hours24Offset;                  // sent hours offset
    }
    else
    {
        timePacket_t tp;  
        
        // get rtc
        getBinTime(&tp);
        seconds = tp.second;
        minutes = tp.minute;
        hours24 = tp.hour24 + hours24Offset;
        
        // handle wrap around of hours
        if (hours24 > 23)
        {
            hours24 -= 24;
        }

        // Setup the storage system to the specified GMT time.
        storageMgr_adjustStorageTime(hours24Offset);

        // Add OTA response data
        *responseDataP++ = 1;                              // success status
        *responseDataP++ = secondsOffset;                  // sent seconds offset
        *responseDataP++ = minutesOffset;                  // sent minute offset
        *responseDataP++ = hours24Offset;                  // sent hours offset
        *responseDataP++ = seconds;                        // new storage seconds
        *responseDataP++ = minutes;                        // new storage minutes
        *responseDataP++ = hours24;                        // new storage hours
    }

    return (true);
}

/**
* \brief (msgId=0x3) Process the Reset Data OTA command.  
* 
* \brief Input OTA parameters
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
*  
* \brief Output OTA response
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
* \li status     (1 byte): 1 = success, 0xFF = failure
*
* @param otaRespP Pointer to the response data and other info
*                 received from the modem.
*
* @return bool Set to true if a OTA response should be sent
*/
static bool otaMsgMgr_processResetData(otaResponse_t *otaRespP)
{
    uint8_t *responseDataP = &otaData.responseBufP[RESPONSE_DATA_OFFSET];

    storageMgr_overrideUnitActivation(false);
    storageMgr_resetRedFlagAndMap();
    storageMgr_resetWeeklyLogs();
    // Prepare OTA response.  It will be sent after this function exits.
    prepareOtaResponse(otaRespP->buf[0], otaRespP->buf[1], otaRespP->buf[2], NULL, 0);
    // Add response data
    *responseDataP = 1;                                    // success status
    return (true);
}

/**
* \brief (msgId=0x4) Process the Reset Red Flag Alarm OTA
*        command.
* 
* \brief Input OTA parameters
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
*  
* \brief Output OTA response
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
* \li status     (1 byte): 1 = success, 0xFF = failure
*
* @param otaRespP Pointer to the response data and other info
*                 received from the modem.
*
* @return bool Set to true if a OTA response should be sent
*/
static bool otaMsgMgr_processResetRedFlag(otaResponse_t *otaRespP)
{
    uint8_t *responseDataP = &otaData.responseBufP[RESPONSE_DATA_OFFSET];

    storageMgr_resetRedFlagAndMap();
    // Prepare OTA response.  It will be sent after this function exits.
    prepareOtaResponse(otaRespP->buf[0], otaRespP->buf[1], otaRespP->buf[2], NULL, 0);
    // Add response data
    *responseDataP = 1;                                    // success status
    return (true);
}

/**
* \brief (msgId=0x5) Process Activate Device OTA command.  
* 
* \brief Input OTA parameters
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
*  
* \brief Output OTA response
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
* \li status     (1 byte): 1 = success, 0xFF = failure
*
* @param otaRespP Pointer to the response data and other info
*                 received from the modem.
*
* @return bool Set to true if a OTA response should be sent
*/
static bool otaMsgMgr_processActivateDevice(otaResponse_t *otaRespP)
{
    uint8_t *responseDataP = &otaData.responseBufP[RESPONSE_DATA_OFFSET];

    storageMgr_overrideUnitActivation(true);
    // Prepare OTA response.  It will be sent after this function exits.
    prepareOtaResponse(otaRespP->buf[0], otaRespP->buf[1], otaRespP->buf[2], NULL, 0);
    // Add response data
    *responseDataP = 1;                                    // success status
    return (true);
}

/**
* \brief (msgId=0x6) Process De-Activate Device OTA command.  
* 
* \brief Input OTA parameters
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
*  
* \brief Output OTA response
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
* \li status     (1 byte): 1 = success, 0xFF = failure
*
* @param otaRespP Pointer to the response data and other info
*                 received from the modem.
*
* @return bool Set to true if a OTA response should be sent
*/
static bool otaMsgMgr_processSilenceDevice(otaResponse_t *otaRespP)
{
    uint8_t *responseDataP = &otaData.responseBufP[RESPONSE_DATA_OFFSET];

    storageMgr_overrideUnitActivation(false);
    // Prepare OTA response.  It will be sent after this function exits.
    prepareOtaResponse(otaRespP->buf[0], otaRespP->buf[1], otaRespP->buf[2], NULL, 0);
    // Add response data
    *responseDataP = 1;                                    // success status
    return (true);
}

/**
* \brief (msgId=0x7) Process Set Transmission Rate OTA command. 
* 
* @param otaRespP Pointer to the response data and other info
*                 received from the modem.
*  
* \brief Input OTA parameters
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
* \li transmission rate in days (1 byte)
*  
* \brief Output OTA response
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
* \li status     (1 byte): 1 = success, 0xFF = failure
* \li transmission rate in days (1 byte)
*
* @return bool Set to true if a OTA response should be sent
*/
static bool otaMsgMgr_processSetTransmissionRate(otaResponse_t *otaRespP)
{
    uint8_t *responseDataP = &otaData.responseBufP[RESPONSE_DATA_OFFSET];
    uint8_t transmissionRateInDays = 1;
    uint8_t *bufP = otaRespP->buf;

    transmissionRateInDays = bufP[3];

    // Prepare OTA response.  It will be sent after this function exits.
    prepareOtaResponse(otaRespP->buf[0], otaRespP->buf[1], otaRespP->buf[2], NULL, 0);

    if (transmissionRateInDays == 0 || transmissionRateInDays > (4 * 7))
    {
        // Add OTA response data
        *responseDataP++ = 0xff;                           // error status
        *responseDataP++ = transmissionRateInDays;
    }
    else
    {
        storageMgr_setTransmissionRate(transmissionRateInDays);
        // Add OTA response data
        *responseDataP++ = 1;                              // success status
        *responseDataP++ = transmissionRateInDays;
    }

    return (true);
}

/**
* \brief (msgId=0x8) Process Reset Device OTA command.  
* 
* @param otaRespP Pointer to the response data and other info
*                 received from the modem.
*  
* \brief Input OTA parameters
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
* \li key0       (1 byte)
* \li key1       (1 byte)
* \li key2       (1 byte)
* \li key3       (1 byte)
*  
* \brief Output OTA response
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
* \li status     (1 byte): 1 = success, 0xFF = failure
*
* @return bool Set to true if a OTA response should be sent
*/
static bool otaMsgMgr_processResetDevice(otaResponse_t *otaRespP)
{
    uint8_t *responseDataP = &otaData.responseBufP[RESPONSE_DATA_OFFSET];

    if ((otaRespP->buf[3] == REBOOT_KEY1) &&
        (otaRespP->buf[4] == REBOOT_KEY2) &&
        (otaRespP->buf[5] == REBOOT_KEY3) &&
        (otaRespP->buf[6] == REBOOT_KEY4))
    {
        otaData.activateReboot = ACTIVATE_REBOOT_KEY;
    }

    // Prepare OTA response.  It will be sent after this function exits.
    prepareOtaResponse(otaRespP->buf[0], otaRespP->buf[1], otaRespP->buf[2], NULL, 0);

    if (otaData.activateReboot == ACTIVATE_REBOOT_KEY)
    {
        *responseDataP++ = 1;                              // success status
    }
    else
    {
        *responseDataP++ = 0xFF;                           // error status
    }

    return (true);
}

/**
* \brief (msgId=0x0C) Return storage clock information
*  
* \brief Input OTA parameters
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
*
* \brief Output OTA response 
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
* \li status     (1 byte): 1 = success, 0xFF = failure
* \li storageTime_seconds   (1 byte)
* \li storageTime_minutes   (1 byte)
* \li storageTime_hours     (1 byte)
* \li storageTime_dayOfWeek (1 byte)
* \li storageTime_week      (1 byte)
* \li alignStorageFlag      (1 byte)
* \li alignSecond           (1 byte)
* \li alignMinute           (1 byte)
* \li alignHour24           (1 byte)
* 
* @param otaRespP Pointer to the response data and other info
*                 received from the modem.
* 
* @return bool Set to true if a OTA response should be sent
*/
static bool otaMsgMgr_processClockRequest(otaResponse_t *otaRespP)
{
    uint8_t *responseDataP = &otaData.responseBufP[RESPONSE_DATA_OFFSET];

    // Prepare OTA response.  It will be sent after this function exits.
    prepareOtaResponse(otaRespP->buf[0], otaRespP->buf[1], otaRespP->buf[2], NULL, 0);
    *responseDataP++ = 1;                                  // success status
    storageMgr_getStorageClockInfo(responseDataP);
    return (true);
}

/**
* 
* \brief (msgId=0x0D) Request GPS data. If request type is 0, 
*        return GPS data from last measurement. If request type
*        is 1, schedule a new GPS measurement.
* 
* \brief Input OTA parameters
* \li msg opcode  (1 byte)
* \li msg id      (2 bytes)
* \li requestType (1 byte) 0=return existing,1=schedule new GPS
*     measurement
* 
* \brief Output OTA response
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
* \li status     (1 byte): 1 = success, 0xFF = failure
* For request type = 0:
* \li hours      (1 byte)
* \li minutes    (1 byte)
* \li latitude   (4 bytes)
* \li longitude  (4 bytes)
* \li fixQuality (1 byte)
* \li numOfSats  (1 byte)
* \li hdop       (1 byte)
* \li fixTimeInSecs (2 bytes) 
* \li reserved   (1 byte) 
*  
* \brief If requestType = 0, send stored GPS data as part of OTA 
*        response. Data will be placed after status byte.
* 
* @param otaRespP Pointer to the response data and other info
*                 received from the modem.
* 
* @return bool Set to true if a OTA response should be sent
*/
static bool otaMsgMgr_processGpsRequest(otaResponse_t *otaRespP)
{
    uint8_t *responseDataP = &otaData.responseBufP[RESPONSE_DATA_OFFSET];
    // Create pointer to where status byte lives
    uint8_t *statusP = responseDataP++;
    // Grab the requestType from the input buffer
    uint8_t requestType = otaRespP->buf[3];

    // Prepare OTA response.  It will be sent after this function exits.
    prepareOtaResponse(otaRespP->buf[0], otaRespP->buf[1], otaRespP->buf[2], NULL, 0);
    // Set status to success for now
    *statusP = 1;
    if (requestType == 0)
    {
        // Return existing GPS data. Data will be copied to location after status byte
        gps_getGpsData(responseDataP);
    }
    else if (requestType == 1)
    {
        // Schedule a new GPS measurement and message
        msgSched_scheduleGpsMeasurement();
    }
    else
    {
        // Invalid request type
        *statusP = 0xFF;                                   // failure status
    }
    return (true);
}

/**
* 
* \brief (msgId=0x0E) Set GPS measurement criteria.
* 
* \brief Input OTA parameters
* \li msg opcode  (1 byte)
* \li msg id      (2 bytes)
* \li number of satelites        (1 byte)
* \li hdop value (x 10)          (1 byte)
* \li mininum on time in seconds (2 bytes)
* 
* \brief Output OTA response
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
* \li status     (1 byte): 1 = success, 0xFF = failure
* \li echo number of satelites        (1 byte)
* \li echo hdop value (x 10)          (1 byte)
* \li echo mininum on time in seconds (2 bytes)
*  
* @param otaRespP Pointer to the response data and other info
*                 received from the modem.
* 
* @return bool Set to true if a OTA response should be sent
*/
static bool otaMsgMgr_setGpsMeasCriteria(otaResponse_t *otaRespP)
{
    uint8_t *responseDataP = &otaData.responseBufP[RESPONSE_DATA_OFFSET];
    bool error = false;

    // Create pointer to where status byte lives
    uint8_t *statusP = responseDataP++;

    // Grab the data from the input buffer
    uint8_t numSats = otaRespP->buf[3];
    uint8_t hdop = otaRespP->buf[4];
    uint16_t minOnTimeInSeconds = (otaRespP->buf[5] << 8) | otaRespP->buf[6];

    // Prepare OTA response.  It will be sent after this function exits.
    prepareOtaResponse(otaRespP->buf[0], otaRespP->buf[1], otaRespP->buf[2], NULL, 0);

    // Set status to success for now
    *statusP = 1;

    // Echo back data sent in response buffer
    *responseDataP++ = numSats;
    *responseDataP++ = hdop;
    *responseDataP++ = (minOnTimeInSeconds >> 8);
    *responseDataP++ = (minOnTimeInSeconds & 0xFF);

    // Check data for valid range
    if (numSats > 16)
    {
        error = true;
    }
    if (hdop > 100)
    {
        error = true;
    }
    if (minOnTimeInSeconds > (15 * 60))
    {
        error = true;
    }
    if (error)
    {
        // Invalid request type
        *statusP = 0xFF;                                   // failure status
    }
    else
    {
        // Set the GPS measurement criteria parameters in the gps module
        gpsMsg_setMeasCriteria(numSats, hdop, minOnTimeInSeconds);
    }
    return (true);
}

/**
* \brief (msgId=0x10) Process a Firmware Upgrade OTA command. 
*        This message is handled differently than other
*        messages.  A separate, special state machine is called
*        to handle retrieving the firmware upgrade data from the
*        modem and burn it into the second image location.  The
*        processing steps include:
* \li Call special state machine to retrieve data and burn to
*     flash.
* \li If retrieval and flash burn was successful, prepare the
*     successful OTA response.
* \li If retrieval and flash burn was successful, update the
*     appRecord to store new image information.  This is used by
*     the boot loader to copy the second image into the main
*     image location.
* \li If retrieval and flash burn was successful, start the
*     reboot timer.
* \li If retrieval and flash burn was unsuccessful, prepare the
*     unsuccessful OTA response.
* \li Set up state machine control parameters
* 
* \brief Input OTA parameters
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
* \li key0       (1 byte)
* \li key1       (1 byte)
* \li key2       (1 byte)
* \li key3       (1 byte)
* \li total sections (1 byte)
* \li section start byte (1 byte)
* \li section number (1 byte)
* \li firmware address in flash (2 bytes, MSB first)
* \li firmware length in bytes  (2 bytes, MSB first)
* \li crc16 of firmware         (2 bytes)
* \li 11264 bytes of firmware   (11K, 0x2C00 bytes)
*  
* \brief Output OTA response
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
* \li status     (1 byte): 1 = success, 0xFF = failure
* \li sent crc16 of firmware (2 bytes)
*  
* @param otaRespP Pointer to the response data and other info
*                 received from the modem.
*
* @return bool Set to true if a OTA response should be sent
*/
static bool otaMsgMgr_processFirmwareUpgrade(otaResponse_t *otaRespP)
{
    uint8_t *bufP = otaRespP->buf;
    uint8_t retryCount = 0;
    fwUpdateResult_t fwUpdateResult = RESULT_NO_FWUPGRADE_PERFORMED;
    uint16_t crc;
    uint8_t *responseDataP = &otaData.responseBufP[RESPONSE_DATA_OFFSET];

    // Verify the firmware upgrade keys in the message.
    if ((bufP[3] == FLASH_UPGRADE_KEY1) &&
        (bufP[4] == FLASH_UPGRADE_KEY2) &&
        (bufP[5] == FLASH_UPGRADE_KEY3) &&
        (bufP[6] == FLASH_UPGRADE_KEY4))
    {

        // Retry up to four times to retrieve the upgrade data and burn into flash.
        do
        {
            // Run the special firmware upgrade state machine.
            fwUpdateResult = otaUpgrade_processOtaUpgradeMessage();
        } while ((fwUpdateResult != RESULT_DONE_SUCCESS) && (retryCount++ < 4));
    }


    // Prepare OTA response.  It will be sent after this function exits.
    prepareOtaResponse(OTA_OPCODE_FIRMWARE_UPGRADE,
                       otaData.lastMsgId >> 8,
                       otaData.lastMsgId & 0xFF,
                       NULL, 0);
    // Prepare OTA response data
    *responseDataP++ = (fwUpdateResult == RESULT_DONE_SUCCESS) ? 0x1 : 0xFF;
    *responseDataP++ = (fwUpdateResult == RESULT_DONE_SUCCESS) ? 0x0 : otaUpgrade_getErrorCode();
    // Return the received CRC and the calculated CRC
    crc = otaUpgrade_getFwMessageCrc();
    *responseDataP++ = crc >> 8;
    *responseDataP++ = crc & 0xFF;
    crc = otaUpgrade_getFwCalculatedCrc();
    *responseDataP++ = crc >> 8;
    *responseDataP++ = crc & 0xFF;

    if (fwUpdateResult == RESULT_DONE_SUCCESS)
    {
        // Update the APP record with new firmware info.  The bootloader uses
        // information to copy the new image from the second image location to the
        // main image location.
        appRecord_updateFwInfo(true, otaUpgrade_getFwMessageCrc());

        // Set the reboot flag to allow a reboot to start after
        // all other OTA message processing is complete.
        otaData.activateReboot = ACTIVATE_REBOOT_KEY;
    }
    return (true);
}

/**
* \brief Process an unknown OTA message type
* 
* @param otaRespP Pointer to the response data and other info
*                 received from the modem.
* 
* @return bool Set to true if a OTA response should be sent
*/
static bool otaMsgMgr_processUnknownRequest(otaResponse_t *otaRespP)
{
    uint8_t *responseDataP = &otaData.responseBufP[RESPONSE_DATA_OFFSET];

    // Prepare OTA response.  It will be sent after this function exits.
    prepareOtaResponse(otaRespP->buf[0], otaRespP->buf[1], otaRespP->buf[2], NULL, 0);
    *responseDataP = 0xfe;                                 // unknown message status
    return (true);
}

/**
*
* \brief (msgId=0x0F) Request Sensor data.
*
* \brief Input OTA parameters
* \li msg opcode  (1 byte)
* \li msg id      (2 bytes)
* \li requestType (1 byte) 0=request sensor data,1=overwrite factory data,2=reset water detect,3=set unknown limit
*     measurement
* \li requestData (2 bytes) optional, for request 3: unknown limit value
*
* \brief Output OTA response
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
* \li status     (1 byte): 1 = success, 0xFF = failure
* \li echo request type (1 byte)
* \li echo request data (2 bytes)
*
* \brief get Sensor Data
*
* @param otaRespP Pointer to the response data and other info
*                 received from the modem.
*
* @return bool Set to true if a OTA response should be sent
*/
static bool otaMsgMgr_getSensorData(otaResponse_t *otaRespP)
{
    uint8_t *responseDataP = &otaData.responseBufP[RESPONSE_DATA_OFFSET];
    // Create pointer to where status byte lives
    uint8_t *statusP = responseDataP++;
    // Grab the requestType from the input buffer
    uint8_t requestType = otaRespP->buf[3];
    // request 3 only needs a byte, but the word is there in case
    // we add another request
    uint16_t requestData = otaRespP->buf[5];
    timePacket_t NowTime;
    bool error = false;

    requestData = (requestData << 8) + otaRespP->buf[4];

    // sanity check
    if (requestType == SENSOR_DOWNSPOUT_RATE)
    {
        // "fix" bad data but report no error
        if (requestData < SENSOR_MIN_DOWNSPOUT)
            requestData = SENSOR_MIN_DOWNSPOUT;
        else if (requestData > SENSOR_MAX_DOWNSPOUT)
            requestData = SENSOR_MAX_DOWNSPOUT;
    }
    else if (requestType == SENSOR_SET_UNKNOWN_LIMIT)
    {
        //"reject command" when out of spec
        if (requestData > 100)
            error = true;
    }
    else if (requestType == SENSOR_NOP_RESPONSE)
    {
        getBinTime(&NowTime);
        requestData = time_util_RTC_hms(&NowTime);
    }

    // Prepare OTA response.  It will be sent after this function exits.
    prepareOtaResponse(otaRespP->buf[0], otaRespP->buf[1], otaRespP->buf[2], NULL, 0);
    // Set status to success for now
    *statusP = 1;

    *responseDataP++ = requestType;                        // echo request type
    *responseDataP++ = (uint8_t)(requestData & 0xFF);
    *responseDataP++ = (uint8_t)(requestData >> 8);        // echo request data

    switch (requestType)
    {
        case SENSOR_REQ_SENSOR_DATA:
            //this tells sysexec to queue up SENSOR_DATA when ready
            //we just need to be sure that this OTA response goes out first
            sysExecData.sendSensorDataMessage = true;
            break;
        case SENSOR_OVERWRITE_FACTORY:
            waterDetect_record_pads_baseline();
            break;
        case SENSOR_RESET_WATER_DETECT:
            waterDetect_init();
            sysExecData.waterDetectResets++;
            break; 
        case SENSOR_SET_UNKNOWN_LIMIT:
            if (requestData <= 100)
                padStats.unknown_limit = requestData;
            break;
        case SENSOR_REPORT_NOW:
            // set feature to report water totals immediately (for debugging)
            sysExecData.sendSensorDataNow = requestData > 0 ? true : false;
            break;
        case SENSOR_DOWNSPOUT_RATE:
            // update the downspout rate
            sysExecData.downspout_rate = requestData;
            break;
        case SENSOR_SET_WATER_LIMIT:
            // set the water limit
            padStats.water_limit = requestData;
            break;
        case SENSOR_SET_WAKE_TIME:
            // set time that unit has no water before sleeping
            sysExecData.dry_wake_time = requestData;
            break;
        case SENSOR_NOP_RESPONSE:
            // respond with NOP so the clock can be adjusted without a reset command
            break;
        default:
            // Invalid request type
            error = true;
    }

    if (error)
        *statusP = 0xFF;                                   // failure status

    return (true);
}

/**
* \brief Debug command to return data read from MSP430 memory.  
*  
* \brief (msgId = 0x1F) Input OTA parameters
* \li address - The address to read (2 bytes)
* \li readLength - The number of units to read (1 byte)
* \li units - Unit width (8= 8 bits,16=16 bits)
*  
* \brief Output response
* \li msg opcode (1 byte)
* \li msg id     (2 bytes)
* \li status     (1 byte): 1 = success, 0xFF = failure
* \li address    (2 bytes)
* \li readLenth  (1 byte)
* \li readWidth  (1 byte)
* \li data read from memory. If unit width is bytes, there will
*     be readLength bytes returned. If unit width is 16, there
*     will be readLength x 2 bytes returned.
* 
* @param otaRespP Pointer to the response data and other info
*                 received from the modem.
* 
* @return bool Set to true if a OTA response should be sent
*/
static bool otaMsgMgr_processMemoryRead(otaResponse_t *otaRespP)
{
    uint8_t i;
    uint8_t *responseDataP = &otaData.responseBufP[RESPONSE_DATA_OFFSET];
    uint16_t address;

    // retrieve number of units to read
    uint8_t readLength = otaRespP->buf[5];
    // unit type, 8 or 16
    uint8_t readWidth = otaRespP->buf[6];

    // Get address
    address = otaRespP->buf[3];
    address <<= 8;
    address += otaRespP->buf[4];

    // Prepare OTA response.  It will be sent after this function exits.
    prepareOtaResponse(otaRespP->buf[0], otaRespP->buf[1], otaRespP->buf[2], NULL, 0);
    // Add return parameters of status, address, readLength and readWidth
    *responseDataP++ = 1;                                  // Status holding place
    *responseDataP++ = address >> 8;
    *responseDataP++ = address & 0xFF;
    *responseDataP++ = readLength;
    *responseDataP++ = readWidth;

    if (readWidth == 8)
    {
        // retrieve 16 bit address pointer from message
        uint8_t *src8P = (uint8_t *)address;
        // Set destination pointer where to store data in the response buffer
        // Offset by one because first byte will be for status byte.
        uint8_t *dst8P = (uint8_t *)(responseDataP);
        // Protect max size to read
        uint8_t numBytes = readLength;

        if (numBytes > MAX_OTA_MEMORY_READ_BYTES)
        {
            readLength = MAX_OTA_MEMORY_READ_BYTES;
        }
        // Read data from memory
        for (i = 0; i < readLength; i++)
        {
            *dst8P++ = *src8P++;
        }
    }
    else if (readWidth == 16)
    {
        // retrieve 16 bit address pointer from message
        uint16_t *src16P = (uint16_t *)address;
        // Set destination pointer where to store data in the response buffer
        // Offset by one because first byte will be for status byte.
        uint16_t *dst16P = (uint16_t *)(responseDataP);
        // Protect max size to read
        uint8_t numBytes = readLength * 2;

        if (numBytes > MAX_OTA_MEMORY_READ_BYTES)
        {
            readLength = MAX_OTA_MEMORY_READ_BYTES >> 1;
        }
        // Read data from memory
        for (i = 0; i < readLength; i++)
        {
            *dst16P++ = *src16P++;
        }
    }
    else
    {
        readLength = 0;
        // Update status with error
        otaData.responseBufP[RESPONSE_DATA_OFFSET] = 0xFF;
    }

    if (readLength)
    {
        // Hack to over-ride OTA_RESPONSE_DATA_LENGTH byte max response data return
        otaData.cmdWrite.payloadLength = OTA_RESPONSE_HEADER_LENGTH + 8 + readLength;
    }

    return (true);
}

/**
* \brief Get the length of an OTA message. 
*
* @return uint8_t length in bytes.
*/
static uint8_t otaMsgMgr_getOtaLength(void)
{
    // Get the object that contains the OTA message data and info
    otaResponse_t *otaRespP = modemMgr_getLastOtaResponse();
    uint8_t bytes = otaRespP->remainingInBytes;

    // The only message that is longer than the OTA max read size
    // is the upgrade message.  For that, we only need to identify
    // that we received it.  It will be processed in the bootloader.
    if (bytes > OTA_PAYLOAD_MAX_RX_READ_LENGTH)
    {
        bytes = 16;
    }
    return (bytes);
}

/**
* \brief Process OTA commands.  
*/
static void otaMsgMgr_processOtaMsg(void)
{
    bool sendOtaResponse = false;
    // Get the buffer that contains the OTA message data
    otaResponse_t *otaRespP = modemMgr_getLastOtaResponse();
    // First byte is the OTA opcode
    volatile OtaOpcode_t opcode = (OtaOpcode_t)otaRespP->buf[0];

    // Set default values to the variables that control the state
    // machine behavior after processing of the OTA message.  For most cases,
    // we always send back a response, delete the message and move to process the
    // next OTA message.  There are a few exceptions.  Currently this is only the
    // GMT update message and the firmware upgrade message.  They are handled slightly
    // differently.
    otaData.sendOtaResponse = true;
    otaData.deleteOtaMessage = true;

    // Save the opcode and msgId for this OTA
    otaData.lastMsgOpcode = (OtaOpcode_t)otaRespP->buf[0];
    otaData.lastMsgId = (otaRespP->buf[1] << 8) | otaRespP->buf[2];

    // Process OTA based on the opcode number
    switch (opcode)
    {
        case OTA_OPCODE_GMT_CLOCKSET:
            sendOtaResponse = otaMsgMgr_processGmtClocksetPart1(otaRespP);
            break;
        case OTA_OPCODE_LOCAL_OFFSET:
            sendOtaResponse = otaMsgMgr_processLocalOffset(otaRespP);
            break;
        case OTA_OPCODE_RESET_DATA:
            sendOtaResponse = otaMsgMgr_processResetData(otaRespP);
            break;
        case OTA_OPCODE_RESET_RED_FLAG:
            sendOtaResponse = otaMsgMgr_processResetRedFlag(otaRespP);
            break;
        case OTA_OPCODE_ACTIVATE_DEVICE:
            sendOtaResponse = otaMsgMgr_processActivateDevice(otaRespP);
            break;
        case OTA_OPCODE_SILENCE_DEVICE:
            sendOtaResponse = otaMsgMgr_processSilenceDevice(otaRespP);
            break;
        case OTA_OPCODE_FIRMWARE_UPGRADE:
            sendOtaResponse = otaMsgMgr_processFirmwareUpgrade(otaRespP);
            break;
        case OTA_OPCODE_RESET_DEVICE:
            sendOtaResponse = otaMsgMgr_processResetDevice(otaRespP);
            break;
        case OTA_OPCODE_SET_TRANSMISSION_RATE:
            sendOtaResponse = otaMsgMgr_processSetTransmissionRate(otaRespP);
            break;
        case OTA_OPCODE_CLOCK_REQUEST:
            sendOtaResponse = otaMsgMgr_processClockRequest(otaRespP);
            break;
        case OTA_OPCODE_GPS_REQUEST:
            sendOtaResponse = otaMsgMgr_processGpsRequest(otaRespP);
            break;
        case OTA_OPCODE_SET_GPS_MEAS_PARAMS:
            sendOtaResponse = otaMsgMgr_setGpsMeasCriteria(otaRespP);
            break;
        case OTA_OPCODE_SENSOR_DATA:
            sendOtaResponse = otaMsgMgr_getSensorData(otaRespP);
            break;
        case OTA_OPCODE_MEMORY_READ:
            // Fall through for now...
            sendOtaResponse = otaMsgMgr_processMemoryRead(otaRespP);
            break;
        default:
            sendOtaResponse = otaMsgMgr_processUnknownRequest(otaRespP);
            break;
    }

    // Save whether an OTA response should be sent back immediately.
    otaData.sendOtaResponse = sendOtaResponse;
}

/**
* \brief Prepare an OTA response.  ( 
* \li Fill in the values to the response buffer that will be 
*     sent to the Modem.
* \li Fill in the parameters into the modem write cmd object
*     (modemCmdWriteData_t) which instructs lower level
*     processing what to send to Modem.
* 
* @param opcode OTA Opcode we are responding/replying to.
* @param msgId0 OTA Msg ID fist byte of OTA message we are 
*               responding/replying to.
* @param msgId0 OTA Msg ID second byte of OTA message we are 
*               responding/replying to.
* @param responseDataP A pointer to additional data that should 
*                      be returned in the response message
* @param responseDataLen Length of the additional data 
*/
static void prepareOtaResponse(uint8_t opcode,
                               uint8_t msgId0,
                               uint8_t msgId1,
                               uint8_t *responseDataP,
                               uint8_t responseDataLen)
{

    // prepare OTA response message
    uint8_t *bufP = otaData.responseBufP;

    // Zero the OTA response buffer
    memset(bufP, 0, OTA_RESPONSE_LENGTH);

    // Fill in the buffer with the standard message header
    uint8_t index = storageMgr_prepareMsgHeader(bufP, MSG_TYPE_OTAREPLY);

    // Always send OTA_RESPONSE_DATA_LENGTH bytes back beyond the header - even if some not used.
    // Allows for future additions to return other data
    uint8_t returnLength = index + OTA_RESPONSE_DATA_LENGTH;

    // Copy the opcode we received into the buffer
    bufP[index++] = opcode;
    // Copy the message ID we received into the buffer
    bufP[index++] = msgId0;
    bufP[index++] = msgId1;

    // There may be additional response data to return.  Max data
    // length of response is OTA_RESPONSE_DATA_LENGTH minus opcode, msgId0 and msgI1.
    if (responseDataP && responseDataLen && (responseDataLen <= (OTA_RESPONSE_DATA_LENGTH - 3)))
    {
        memcpy(&bufP[index], responseDataP, responseDataLen);
    }

    // Prepare the command for the batch write operation
    memset(&otaData.cmdWrite, 0, sizeof(modemCmdWriteData_t));
    otaData.cmdWrite.cmd = OUTPOUR_M_COMMAND_SEND_DATA;
    otaData.cmdWrite.payloadMsgId = MSG_TYPE_OTAREPLY;
    otaData.cmdWrite.payloadP = bufP;
    otaData.cmdWrite.payloadLength = returnLength;
}

