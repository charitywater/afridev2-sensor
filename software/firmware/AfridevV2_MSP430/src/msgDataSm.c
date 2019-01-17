/** 
 * @file msgDataSm.c
 * \n Source File
 * \n Outpour MSP430 Firmware
 * 
 * \brief Manage the details of sending a data command message. 
 *        Used when sending all modem messages of type 0x40
 *        (M_COMMAND_SEND_DATA).  Sequences through different
 *        states to manage the process of bringing up the modem
 *        and sending the data.
 * \brief The state machine sequences through the following
 *        states:
 * \li    DMSG_STATE_GRAB:  Allocate modem and start power up 
 *        sequence
 * \li    DMSG_STATE_WAIT_FOR_MODEM_UP: Wait for power up sequence 
 *        to complete
 * \li    DMSG_STATE_SEND_MSG: Send the data command
 * \li    DMSG_STATE_SEND_MSG_WAIT: Wait for data command send 
 *        sequence to complete.
 * \li    DMSG_STATE_WAIT_FOR_LINK:  Wait for the modem to get 
 *        connected to the Network.  
 * \li    DMSG_STATE_PROCESS_OTA:  Start processing OTA messages 
 *        (if any are present).
 * \li    DMSG_STATE_PROCESS_OTA_WAIT:  wait for OTA message 
 *        processing to complete.
 * \li    DMSG_STATE_RELEASE: Release the modem. Sends power 
 *        down command to modem
 * \li    DMSG_STATE_RELEASE_WAIT: wait for modem power down cmd 
 *        to complete. Then turn off the modem.
 */

#include "outpour.h"

/***************************
 * Module Data Definitions
 **************************/
/**
 * \def WAIT_FOR_LINK_UP_TIME_IN_SECONDS
 * \brief Specify how long to wait for the modem to connect to 
 *        the network.
 */
#define WAIT_FOR_LINK_UP_TIME_IN_SECONDS ((uint16_t)60*10*(TIME_SCALER))

/**
 * \def MAX_MODEM_POWER_CYCLES 
 * \brief Specify max times to power cycle the modem when an 
 *        error occurs during communication.
 */
#define MAX_MODEM_POWER_CYCLES ((uint8_t)1)

/***************************
 * Module Data Declarations
 **************************/

/*************************
 * Module Prototypes
 ************************/

/***************************
 * Module Public Functions
 **************************/

/**
 * \brief Initialize the module.  Should be called once on system
 *        start up.
* \ingroup PUBLIC_API
 */
void dataMsgSm_init(void)
{
    // Currently does nothing
}

/**
 *
 * \brief Initialize a data message object.  Used by clients to
 *        initialize their data message object in preparation of
 *        sending out a new data command message.
 * \ingroup PUBLIC_API
 * 
 * @param dataMsgP Data message object pointer.  The object is 
 *                 owned by the client (i.e. water Msg, final
 *                 Assembly Msg, Monthly Msg).  Each client has
 *                 their own dataMsg object.
 */
void dataMsgSm_initForNewSession(dataMsgSm_t *dataMsgP)
{
    // Initialize the data msg object.
    memset(dataMsgP, 0, sizeof(dataMsgSm_t));
    dataMsgP->modemResetCount = 0;
    dataMsgP->dataMsgState = DMSG_STATE_GRAB;
}

/**
* \brief The client can send multiple messages within one modem 
*        session.  The client uses this function to reset the
*        data message state machine to the correct state for
*        sending another message.
* \ingroup PUBLIC_API
*/
void dataMsgSm_sendAnotherDataMsg(dataMsgSm_t *dataMsgP)
{
    dataMsgP->dataMsgState = DMSG_STATE_SEND_MSG;
    dataMsgP->sendCmdDone = false;
}

/**
* \brief High level state machine for sending a data command 
*        message to the modem. The state machine sequences
*        through the following states:
* \li    DMSG_STATE_GRAB:  Allocate modem and start power up 
*        sequence
* \li    DMSG_STATE_WAIT_FOR_MODEM_UP: Wait for power up sequence 
*        to complete
* \li    DMSG_STATE_SEND_MSG: Send the data command
* \li    DMSG_STATE_SEND_MSG_WAIT: Wait for data command send 
*        sequence to complete.
* \li    DMSG_STATE_WAIT_FOR_LINK:  Wait for the modem to get 
*        connected to the Network.  
* \li    DMSG_STATE_PROCESS_OTA:  Start processing OTA messages 
*        (if any are present).
* \li    DMSG_STATE_PROCESS_OTA_WAIT:  wait for OTA message 
*        processing to complete.
* \li    DMSG_STATE_RELEASE: Release the modem. Sends power down 
*        command to modem
* \li    DMSG_STATE_RELEASE_WAIT: wait for modem power down cmd to 
*        complete. Then turn off the modem.
* \ingroup PUBLIC_API
* 
* 
* @param dataMsgP Data message object pointer.  The object is 
*                 owned by the client (i.e. water Msg, final
*                 Assembly Msg, Monthly Msg).  Each client has
*                 their own dataMsg object.
*/
void dataMsgSm_stateMachine(dataMsgSm_t *dataMsgP)
{
    bool continue_processing = false;

    do
    {
        continue_processing = false;
        switch (dataMsgP->dataMsgState)
        {
            case DMSG_STATE_IDLE:
                // This is the state when nothing is being sent.
                break;
            case DMSG_STATE_GRAB:
                if (modemMgr_grab())
                {
                    dataMsgP->dataMsgState = DMSG_STATE_WAIT_FOR_MODEM_UP;
                    continue_processing = true;
                }
                break;
            case DMSG_STATE_WAIT_FOR_MODEM_UP:
                if (modemMgr_isModemUp())
                {
                    dataMsgP->dataMsgState = DMSG_STATE_SEND_MSG;
                    continue_processing = true;
                }
                break;
            case DMSG_STATE_SEND_MSG:
                dataMsgP->cmdWrite.statusOnly = false;
                modemMgr_sendModemCmdBatch(&dataMsgP->cmdWrite);
                dataMsgP->dataMsgState = DMSG_STATE_SEND_MSG_WAIT;
                break;
            case DMSG_STATE_SEND_MSG_WAIT:
                if (modemMgr_isModemCmdError())
                {
                    // A modem communication error has occured from the UART point of view.
                    dataMsgP->modemResetCount++;
                    // We attempt a modem power-off/power-on sequence to fix the problem
                    // unless max number of tries is hit.
                    if (dataMsgP->modemResetCount > MAX_MODEM_POWER_CYCLES)
                    {
                        // BAD - we already power cycled the modem and it failed.
                        // Set commError and move to release state.
                        dataMsgP->commError = true;
                        dataMsgP->dataMsgState = DMSG_STATE_RELEASE;
                    }
                    else
                    {
                        // We will try to power cycle the modem.
                        // Stop the current modem batch job
                        modemMgr_stopModemCmdBatch();
                        // Power cycle the modem
                        modemMgr_restartModem();
                        // Go back to wait for the modem to power up
                        dataMsgP->dataMsgState = DMSG_STATE_WAIT_FOR_MODEM_UP;
                    }
                }
                else if (modemMgr_isModemCmdComplete())
                {
                    // Set the sendCmdDone flag in case the client wants to send
                    // another message while the modem is up.
                    dataMsgP->sendCmdDone = true;
                    // Standard healthy path
                    // Check if the modem is connected to the network
                    if (modemMgr_isLinkUp())
                    {
                        // We are done - message sent correctly.
                        // Move to retrieving any OTA Messages.
                        dataMsgP->dataMsgState = DMSG_STATE_PROCESS_OTA;
                    }
                    else
                    {
                        // modem not connected to network yet.
                        dataMsgP->dataMsgState = DMSG_STATE_WAIT_FOR_LINK;
                    }
                }
                break;
            case DMSG_STATE_WAIT_FOR_LINK:
                // Waiting for network link to come up
                if (modemMgr_isLinkUp())
                {
                    // Success - all done
                    // Move to retrieving any OTA Messages.
                    dataMsgP->dataMsgState = DMSG_STATE_PROCESS_OTA;
                }
                else if (modemMgr_isLinkUpError() ||
                         (modemPower_getModemOnTimeInSecs() > WAIT_FOR_LINK_UP_TIME_IN_SECONDS))
                {
                    // Timeout has occurred.
                    dataMsgP->connectTimeout = true;

                    // We are done with the modem
                    // Move to retrieving any OTA Messages.
                    dataMsgP->dataMsgState = DMSG_STATE_PROCESS_OTA;
                }
                else
                {
                    // While waiting for the modem link to come up,
                    // resend the command to retrieve status only.
                    dataMsgP->cmdWrite.statusOnly = true;
                    modemMgr_sendModemCmdBatch(&dataMsgP->cmdWrite);
                    dataMsgP->dataMsgState = DMSG_STATE_SEND_MSG_WAIT;
                }
                continue_processing = true;
                break;
            case DMSG_STATE_PROCESS_OTA:
                if (modemMgr_getNumOtaMsgsPending())
                {
                    otaMsgMgr_getAndProcessOtaMsgs();
                    dataMsgP->dataMsgState = DMSG_STATE_PROCESS_OTA_WAIT;
                }
                else
                {
                    dataMsgP->dataMsgState = DMSG_STATE_RELEASE;
                }
                break;
            case DMSG_STATE_PROCESS_OTA_WAIT:
                if (otaMsgMgr_isOtaProcessingDone())
                {
                    dataMsgP->dataMsgState = DMSG_STATE_RELEASE;
                }
                break;
            case DMSG_STATE_RELEASE:
                modemMgr_release();
                dataMsgP->dataMsgState = DMSG_STATE_RELEASE_WAIT;
                break;
            case DMSG_STATE_RELEASE_WAIT:
                if (modemMgr_isReleaseComplete())
                {
                    dataMsgP->sendCmdDone = true;
                    dataMsgP->allDone = true;
                }
                break;
        }
    } while (continue_processing);
}

