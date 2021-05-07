/** 
 * @file modemCmd.c
 * \n Source File
 * \n Outpour MSP430 Firmware
 * 
 * \brief Handle sending commands to and getting responses from 
 *        the modem.  Handle the details of modem message
 *        protocol and format details.
 *
 * \par   Copyright Notice
 *        Copyright 2021 charity: water
 *
 *        Licensed under the Apache License, Version 2.0 (the "License");
 *        you may not use this file except in compliance with the License.
 *        You may obtain a copy of the License at
 *
 *            http://www.apache.org/licenses/LICENSE-2.0
 *
 *        Unless required by applicable law or agreed to in writing, software
 *        distributed under the License is distributed on an "AS IS" BASIS,
 *        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *        See the License for the specific language governing permissions and
 *        limitations under the License.
 *
 */

#include "outpour.h"

/******************************************************************************
 
 **** Modem Message Summary Used by the Software ****

-Ping Cmd (cmd = 0x00)
txHeaderLength = 1 byte
txMsgContainsAPayload = false
expectedResponseLength = 5 bytes          
rx byte template: <start-byte>,cmd,crc[2],<end-byte>

-Power Off Cmd (cmd = 0xe0)
txHeaderLength = 1 byte
txMsgContainsAPayload = false
expectedResponseLength = 5 bytes 
rx byte template: <start-byte>,cmd,crc[2],<end-byte>

-Send Data Cmd (cmd = 0x40)
txHeaderLength = 5 bytes
txMsgContainsAPayload = true
txMsgPayloadLength = size (which is dependent on data sent)
expectedResponseLength = 5 bytes 
rx byte template: <start-byte>,cmd,crc[2],<end-byte>

-Modem Status Cmd (cmd = 0x2)
txMsgContainsAPayload = false
expectedResponseLength = 15 bytes
rx byte template: <start-byte>,cmd,modem-status[10],crc[2],<end-byte>

-Msg Status Cmd  (cmd = 0x3)
txMsgContainsAPayload = false
expectedResponseLength = 23 bytes 
rx byte template: <start-byte>,cmd,msg-status[18],crc[2],<end-byte>

-Incomming Partial Cmd (cmd = 0x42)
txHeaderLength = 9 bytes
txMsgContainsAPayload = false
expectedResponseLength = 13 bytes + payload data          
rx byte template: <start-byte>,cmd,len[4],remaining[4],payload[sizeInBytes],crc[2],<end-byte>

-Delete Incoming Cmd (cmd = 0x43)
txHeaderLength = 1 bytes
txMsgContainsAPayload = false
expectedResponseLength = 5 bytes                       
rx byte template: <start-byte>,cmd,crc[2],<end-byte>

******************************************************************************/

/***************************
 * Module Data Definitions
 **************************/

/**
 * \def ISR_RX_BUF_SIZE 
 * \brief Define the size of the UART receive buffer.  Most 
 *        OTA messages are relatively small. The only exception
 *        is the firmware upgrade message. A get partial data
 *        response (M_COMMAND_GET_INCOMING_PARTIAL) from the
 *        modem will include 13 bytes of modem overhead data
 *        plus the amount of data requested.
 */
#define ISR_RX_BUF_SIZE ((uint16_t)OTA_PAYLOAD_MAX_RX_READ_LENGTH+16)

/**
 * \def ISR_TX_BUF_SIZE 
 * \brief Define the size of the UART transmit ISR buffer. The
 *        transmit buffer is used only to send the header
 *        portion of the data to the modem. The buffer
 *        associated with the payload portion and the size is
 *        set by the upper layers based on the data they are
 *        sending.
 */
#define ISR_TX_BUF_SIZE ((uint16_t)16)

/**
 * \def TX_HEADER_BUF
 * Pointer to the buffer for holding the header portion of the 
 * cmd buffer that is sent to the modem. 
 */
#define TX_HEADER_BUF (isrTxBuf)

/**
 * \def RX_BUF
 * Pointer to the buffer where the Rx ISR puts data
 */
#define RX_BUF (isrRxBuf)

/**
 * \def MODEM_CMD_START_BYTE 
 * \brief Modem Command start byte 
 */
#define	MODEM_CMD_START_BYTE	((uint8_t)0x3c)

/**
 * \def MODEM_RESP_START_BYTE 
 * \brief Modem Response start byte 
 */
#define	MODEM_RESP_START_BYTE	((uint8_t)0x3e)

/**
 * \def MODEM_CMD_END_BYTE 
 * \brief Modem Command/Response end byte 
 */
#define	MODEM_CMD_END_BYTE		((uint8_t)0x3b)

/**
 * \def MODEM_TX_RX_TIMEOUT_IN_SEC
 * \brief define how long to wait before declaring an error for 
 *        a modem tx/rx session.
 */
#define MODEM_TX_RX_TIMEOUT_IN_SEC ((uint32_t)5*(TIME_SCALER))

/**
 * \typedef txIsrState_t
 * \brief Define the states that the tx ISR will transition 
 *        through to send a message to the modem.
 */
typedef enum txIsrState_e {
    TX_ISR_STATE_SEND_START_BYTE,
    TX_ISR_STATE_HEADER,
    TX_ISR_STATE_PAYLOAD,
    TX_ISR_STATE_CRC_BYTE_0,
    TX_ISR_STATE_CRC_BYTE_1,
    TX_ISR_STATE_SEND_STOP_BYTE,
    TX_ISR_STATE_DISABLE,
} txIsrState_t;

/**
 * \def MODEM_CMD_MAX_RETRIES
 * \brief Define how many times to resend a command and wait for 
 *        response after an unsuccessful attempt.
 */
#define MODEM_CMD_MAX_RETRIES ((uint8_t)3)

/**
 * \typedef modemCmdData_t 
 * \brief Contains module data 
 */
typedef struct modemCmdData_s {

    bool busy;                                             /**< signals we are busy sending a modem msg */
    modem_command_t modemCmdId;                            /**< the cmd we are sending to the modem */
    sys_tick_t sendTimestamp;                              /**< time we enabled the tx isr */

    uint8_t retryCount;                                    /**< how many tries to tx/rx the msg */
    bool msgTxRxFailed;                                    /**< the message failed to tx or rx properly */
    bool responseReady;                                    /**< flag to indicate we have response data ready for pickup */
    uint16_t crc;                                          /**< crc16 calculated value on tx or rx msg */

    uint8_t txHeaderLength;                                /**< length of msg tx cmd header */
    bool txMsgContainsAPayload;                            /**< modem tx msg has data associated with it */
    uint16_t txMsgPayloadLength;                           /**< modem tx msg data payload length (if any) */
    uint16_t expectedResponseLength;                       /**< expected msg response length */

    txIsrState_t txIsrState;                               /**< holds the tx isr state */
    bool txIsrMsgComplete;                                 /**< A complete msg has been transmitted */
    uint8_t *txPayloadP;                                   /**< pointer to tx data payload */
    uint16_t txIsrDataIndex;                               /**< Tx Isr Data Index into buffer */

    bool rxIsrMsgComplete;                                 /**< A complete msg has been received */
    uint16_t rxIsrDataIndex;                               /**< Rx ISR Data Index into buffer */

} modemCmdData_t;

/****************************
 * Module Data Declarations
 ***************************/

/**
 * \var isrRxBuf
 * \brief This buffer location is specified in the linker
 *        command file to live right below the stack space in
 *        RAM.
 */
#pragma SET_DATA_SECTION(".commbufs")
uint8_t isrRxBuf[ISR_RX_BUF_SIZE];                         /**< buffer used to tx and rx data to/from */
uint8_t isrTxBuf[ISR_TX_BUF_SIZE];                         /**< buffer used to tx and rx data to/from */
#pragma SET_DATA_SECTION()

/**
 * \var mcData
 * \brief Declare the object that contains the module data.
 */
// static
modemCmdData_t mcData;

/**
 * \note This is defined in CTS_HAL.c.
 */
extern uint8_t CAPSENSE_ACTIVE;

/*************************
 * Module Prototypes
 ************************/

static bool modemCmdProcessRxMsg(void);
static void modemCmdIsrRestart(void);
static void modemCmdCleanup(void);
static void initForPingCmd(modemCmdWriteData_t *writeCmdP);
static void initForPowerOffCmd(modemCmdWriteData_t *writeCmdP);
static void initForSendDataCmd(modemCmdWriteData_t *writeCmdP);
static void initForSendDebugDataCmd(modemCmdWriteData_t *writeCmdP);
static void initForModemStatusCmd(modemCmdWriteData_t *writeCmdP);
static void initForMsgStatusCmd(modemCmdWriteData_t *writeCmdP);
static void initForIncommingPartialCmd(modemCmdWriteData_t *writeCmdP);
static void initForDeleteIncomingCmd(modemCmdWriteData_t *writeCmdP);
static void initForSendTestCmd(modemCmdWriteData_t *writeCmdP);
static inline void enable_UART_tx(void);
static inline void enable_UART_rx(void);
static inline void disable_UART_tx(void);
static inline void disable_UART_rx(void);

/***************************
 * Module Public Functions
 **************************/

/**
* \brief Modem Cmd executive.  Called every 2 seconds to manage 
*        a single modem transmit/receive transaction. If no
*        transaction is in progress, just returns. Detects when
*        a transaction is timeout or completed successfully.
* \ingroup EXEC_ROUTINE
*/
void modemCmd_exec(void)
{

    bool done = false;
    bool retryNeeded = false;
    bool msgOk = false;

    // If we are NOT currently busy with a modem transaction - just return.
    if (!mcData.busy)
    {
        return;
    }

    // Check if modem tx/rx transaction is complete
    if (mcData.txIsrMsgComplete && mcData.rxIsrMsgComplete)
    {
        msgOk = modemCmdProcessRxMsg();
        if (msgOk)
        {
            // Success! Message transaction complete
            mcData.responseReady = true;
            done = true;
        }
        else
        {
            retryNeeded = true;
        }
    }
    // Check for a timeout
    else if (GET_ELAPSED_TIME_IN_SEC(mcData.sendTimestamp) > MODEM_TX_RX_TIMEOUT_IN_SEC)
    {
        retryNeeded = true;
    }

    if (retryNeeded)
    {
        // Check for a max retry error for current tx/rx transaction
        if (mcData.retryCount < MODEM_CMD_MAX_RETRIES)
        {
            mcData.retryCount++;
            // Resend the command
            modemCmdIsrRestart();
        }
        else
        {
            mcData.msgTxRxFailed = true;
            done = true;
        }
    }

    if (done)
    {
        modemCmdCleanup();
    }
}

/**
* \brief One time initialization for module.  Call one time 
*        after system starts or (optionally) wakes.
* \ingroup PUBLIC_API
*/
void modemCmd_init(void)
{
    memset(&mcData, 0, sizeof(modemCmdData_t));
}

typedef void (*modemCmdFuncP_t)(modemCmdWriteData_t *writeCmdP);

static const modemCmdFuncP_t modemCmdTable[] = {
    initForPingCmd,                                        // OUTPOUR_M_COMMAND_PING = 0x00,
    NULL,                                                  // OUTPOUR_M_COMMAND_MODEM_INFO = 0x1,
    initForModemStatusCmd,                                 // OUTPOUR_M_COMMAND_MODEM_STATUS = 0x2,
    initForMsgStatusCmd,                                   // OUTPOUR_M_COMMAND_MESSAGE_STATUS = 0x3,
    initForSendTestCmd,                                    // OUTPOUR_M_COMMAND_SEND_TEST = 0x4,
    initForSendDataCmd,                                    // OUTPOUR_M_COMMAND_SEND_DATA = 0x5,
    initForIncommingPartialCmd,                            // OUTPOUR_M_COMMAND_GET_INCOMING_PARTIAL = 0x6,
    initForDeleteIncomingCmd,                              // OUTPOUR_M_COMMAND_DELETE_INCOMING = 0x7,
    initForSendDebugDataCmd,                               // OUTPOUR_M_COMMAND_SEND_DEBUG_DATA = 0x8,
    initForPowerOffCmd                                     // OUTPOUR_M_COMMAND_POWER_OFF = 0x9,
};

/**
* \brief Start a new data tx/rx transaction with the modem.
* \ingroup PUBLIC_API
* 
* @param writeCmdP Pointer to a modemCmdWriteData_t object 
*                  initialized with the cmd data to transmit.
* 
* @return bool Returns true if the modem is not busy and a 
*         message transaction session was started.
*/
bool modemCmd_write(modemCmdWriteData_t *writeCmdP)
{

    modemCmdFuncP_t modemCmdFuncP = NULL;

    // Bad....
    if (writeCmdP == NULL)
    {
        sysError();
    }

    // If not busy, mark as busy.  Otherwise return false if already busy.
    if (!mcData.busy)
    {
        // For safety, disable UART interrupts
        disable_UART_tx();
        disable_UART_rx();
        mcData.busy = true;
    }
    else
    {
        return (false);
    }

    // Using the cmd number that is being sent,
    // look up the message init function.
    if (writeCmdP->cmd <= OUTPOUR_M_COMMAND_POWER_OFF)
    {
        modemCmdFuncP = modemCmdTable[writeCmdP->cmd];
        if (!modemCmdFuncP)
        {
            return (false);
        }
        // Call the message init function to prepare
        // the command to send to the modem.
        modemCmdFuncP(writeCmdP);
    }
    else
    {
        return (false);
    }

    // Init ISR parameters and enable ISR's to start the modem transaction.
    mcData.retryCount = 0;
    mcData.msgTxRxFailed = false;
    mcData.responseReady = false;
    modemCmdIsrRestart();

    return (true);
}

/**
* \brief Read the last response from the modem.  This represents 
*        the data received as a result of the last completed
*        tx/rx transaction.
* \ingroup PUBLIC_API
*/
void modemCmd_read(modemCmdReadData_t *readDataP)
{
    readDataP->dataP = RX_BUF;
    readDataP->lengthInBytes = mcData.rxIsrDataIndex;
    readDataP->valid = !mcData.msgTxRxFailed;
    readDataP->modemCmdId = mcData.modemCmdId;
}

/**
* \brief Return True if a modem response is available to read. 
*        This means that a complete tx/rx transaction completed
*        and the result is ready to read via the modemCmdRead
*        API.
* \ingroup PUBLIC_API
* 
* @return bool True if ready to read.
*/
bool modemCmd_isResponseReady(void)
{
    return (mcData.responseReady);
}

/**
* \brief Return true if a modem msg tx/rx transaction is in 
*        process.
* \ingroup PUBLIC_API
* 
* @return bool True if busy.
*/
bool modemCmd_isBusy(void)
{
    return (mcData.busy);
}

/**
* \brief Return the status of the last modem msg rx/tx attempt.
* \ingroup PUBLIC_API
* \note clear on read operation.
* 
* @return bool True if error occurred
*/
bool modemCmd_isError(void)
{
    return (mcData.msgTxRxFailed);
}

/*************************
 * Module Private Functions
 ************************/

/**
* \brief Helper function to setup parameters and enable UART 
*        hardware interrupts to start a tx/rx transaction.
*/
static void modemCmdIsrRestart(void)
{
    uint8_t __attribute__((unused)) garbage;

    disable_UART_rx();
    disable_UART_tx();
    mcData.sendTimestamp = GET_SYSTEM_TICK();
    mcData.txIsrMsgComplete = false;
    mcData.txIsrDataIndex = 0;
    mcData.txIsrState = TX_ISR_STATE_SEND_START_BYTE;
    mcData.rxIsrDataIndex = 0;
    mcData.rxIsrMsgComplete = false;
    // For all non-debug cases, there is an expected response from the
    // modem.  Only if we are sending out debug data is there no response
    // expected.
    if (mcData.expectedResponseLength == 0)
    {
        mcData.rxIsrMsgComplete = true;
    }

    // Clear out the UART receive buffer
    garbage = UCA0RXBUF;

    // Enable interrupts
    enable_UART_rx();
    enable_UART_tx();
}

/**
* \brief Helper function to put the UART hardware and module in 
*        a quiescent state.
*/
static void modemCmdCleanup(void)
{
    disable_UART_rx();
    disable_UART_tx();
    mcData.busy = false;
}

/**
 * \brief Helper function to check for valid values returned from
 *         the modem.  Checks for start, end response bytes and
 *         verifies CRC.
 * 
 * \return bool Returns true if rx processing was successful,
 *         false otherwise.
 */
static bool modemCmdProcessRxMsg(void)
{
    bool errorOccured = false;

    // The length of the response is the rx isr buffer index
    uint16_t totalRxBytes = mcData.rxIsrDataIndex;

    // If no data back was expected, just return.
    if (mcData.expectedResponseLength == 0)
    {
        return (true);
    }

    // Verify start byte
    if (RX_BUF[0] != MODEM_RESP_START_BYTE)
    {
        errorOccured = true;
    }

    // Verify end byte
    if (RX_BUF[(totalRxBytes - 1)] != MODEM_CMD_END_BYTE)
    {
        errorOccured = true;
    }

    // Verify response command type matches command sent
    if (RX_BUF[1] != mcData.modemCmdId)
    {
        errorOccured = true;
    }

    // Verify length
    if (mcData.expectedResponseLength != totalRxBytes)
    {
        errorOccured = true;
    }

    if (!errorOccured)
    {
        // Perform CRC check on the message.
        uint16_t calculatedCrc = 0;
        uint16_t rxCrc = ~0;

        // The CRC of the response does not include the start byte, the crc[2] and the end byte
        uint16_t rxCrcNumBytes = totalRxBytes - 4;

        // Grab the CRC in the response
        rxCrc = RX_BUF[totalRxBytes - 3];                  // get MSB
        rxCrc <<= 8;
        rxCrc |= RX_BUF[totalRxBytes - 2];                 // get LSB

        // Calculate CRC of message
        calculatedCrc = gen_crc16(&RX_BUF[1], rxCrcNumBytes);
        // compare calculated against received
        if (rxCrc != calculatedCrc)
        {
            errorOccured = true;
        }
    }

    return (!errorOccured);
}

/**
* \brief Fill header with a ping modem message.
* \brief Help function to initialize the buffer with a stop 
*        modem command and enable interrupts.
*/
static void initForPingCmd(modemCmdWriteData_t *writeCmdP)
{
    mcData.modemCmdId = M_COMMAND_PING;
    TX_HEADER_BUF[0] = M_COMMAND_PING;                     // command Byte
    mcData.crc = ((uint16_t)0x0);                          // precalculated crc
    mcData.txHeaderLength = 1;
    mcData.txMsgContainsAPayload = false;
    mcData.expectedResponseLength = 5;                     // start,cmd,crc[2],end
}

/**
* \brief Fill header with a power off modem message.
* \brief Help function to initialize the buffer with a stop 
*        modem command and enable interrupts.
*/
static void initForPowerOffCmd(modemCmdWriteData_t *writeCmdP)
{
    mcData.modemCmdId = M_COMMAND_POWER_OFF;
    TX_HEADER_BUF[0] = M_COMMAND_POWER_OFF;                // command Byte
    mcData.crc = ((uint16_t)0x8801);                       // precalculated crc
    mcData.txHeaderLength = 1;
    mcData.txMsgContainsAPayload = false;
    mcData.expectedResponseLength = 5;                     // start,cmd,crc[2],end
}

/**
* \brief Fill header with a data modem message.
* \brief Helper function to initialize the buffer with a modem
*        command and enable uart tx interrupt. 
* 
* @param payloadP Data Pointer to data buffer to send to modem
* @param payloadSize Size of data in bytes to send
* @param payloadMsgId The Outpour message ID
*/
static void initForSendDataCmd(modemCmdWriteData_t *writeCmdP)
{
    mcData.modemCmdId = M_COMMAND_SEND_DATA;
    uint16_t size = writeCmdP->payloadLength;

    // TODO NEED TO CHECK FOR MAX PAYLOAD SIZE (DavidL)
    TX_HEADER_BUF[0] = M_COMMAND_SEND_DATA;                // command byte
    TX_HEADER_BUF[1] = 0;                                  // Payload Size: bits 24-31
    TX_HEADER_BUF[2] = 0;                                  // Payload Size: bits 16-23
    TX_HEADER_BUF[3] = (size >> 8) & 0xFF;                 // Payload Size: bits 8-15
    TX_HEADER_BUF[4] = size & 0xFF;                        // Payload Size: bits 0-7
    mcData.txHeaderLength = 5;
    if (size > 0)
    {
        mcData.txMsgContainsAPayload = true;
        mcData.txMsgPayloadLength = size;
        mcData.txPayloadP = writeCmdP->payloadP;
    }
    mcData.crc = gen_crc16_2buf((uint8_t *)&(TX_HEADER_BUF[0]),
                                mcData.txHeaderLength,
                                writeCmdP->payloadP,
                                size);
    mcData.expectedResponseLength = 5;                     // start,cmd,crc[2],end
}

/**
* \brief Fill header with a test modem message.
* \brief Helper function to initialize the buffer with a modem
*        command and enable uart tx interrupt. 
* 
* @param payloadP Data Pointer to data buffer to send to modem
* @param payloadSize Size of data in bytes to send
* @param payloadMsgId The Cascade message ID
*/
static void initForSendTestCmd(modemCmdWriteData_t *writeCmdP)
{
    mcData.modemCmdId = M_COMMAND_SEND_TEST;
    uint16_t size = writeCmdP->payloadLength;

    TX_HEADER_BUF[0] = M_COMMAND_SEND_TEST;                // command byte
    TX_HEADER_BUF[1] = 0;                                  // Payload Size: bits 24-31
    TX_HEADER_BUF[2] = 0;                                  // Payload Size: bits 16-23
    TX_HEADER_BUF[3] = (size >> 8) & 0xFF;                 // Payload Size: bits 8-15
    TX_HEADER_BUF[4] = size & 0xFF;                        // Payload Size: bits 0-7
    mcData.txHeaderLength = 5;
    if (size > 0)
    {
        mcData.txMsgContainsAPayload = true;
        mcData.txMsgPayloadLength = size;
        mcData.txPayloadP = writeCmdP->payloadP;
    }
    mcData.crc = gen_crc16_2buf((uint8_t *)&(TX_HEADER_BUF[0]),
                                mcData.txHeaderLength,
                                writeCmdP->payloadP,
                                size);
    mcData.expectedResponseLength = 5;                     // start,cmd,crc[2],end
}

/**
* \brief Fill header with a debug data modem message.
* \brief Helper function to initialize the buffer with a modem
*        command and enable uart tx interrupt. 
* 
* @param payloadP Data Pointer to data buffer to send to modem
* @param payloadSize Size of data in bytes to send
* @param payloadMsgId The Outpour message ID
*/
static void initForSendDebugDataCmd(modemCmdWriteData_t *writeCmdP)
{
    mcData.modemCmdId = M_COMMAND_SEND_DEBUG_DATA;
    uint16_t size = writeCmdP->payloadLength;

    TX_HEADER_BUF[0] = M_COMMAND_SEND_DEBUG_DATA;          // command byte
    TX_HEADER_BUF[1] = 0;                                  // Payload Size: bits 24-31
    TX_HEADER_BUF[2] = 0;                                  // Payload Size: bits 16-23
    TX_HEADER_BUF[3] = (size >> 8) & 0xFF;                 // Payload Size: bits 8-15
    TX_HEADER_BUF[4] = size & 0xFF;                        // Payload Size: bits 0-7
    mcData.txHeaderLength = 5;
    if (size > 0)
    {
        mcData.txMsgContainsAPayload = true;
        mcData.txMsgPayloadLength = size;
        mcData.txPayloadP = writeCmdP->payloadP;
    }
    // No response expected for debug data
    mcData.expectedResponseLength = 0;
}

/**
* \brief Fill header with a check status modem message.
* \brief Helper function to initialize tx msg header with a
*        check status modem command.
*/
static void initForModemStatusCmd(modemCmdWriteData_t *writeCmdP)
{
    mcData.modemCmdId = M_COMMAND_MODEM_STATUS;
    TX_HEADER_BUF[0] = M_COMMAND_MODEM_STATUS;             // command byte
    mcData.crc = ((uint16_t)0xc181);                       // precalculated crc
    mcData.txHeaderLength = 1;
    mcData.txMsgContainsAPayload = false;
    mcData.expectedResponseLength = 15;                    // start,cmd,status[10],crc[2],end
}

/**
* \brief Fill header with a check messages modem message.
* \brief Helper function to initialize the tx msg header with a 
*        check messages modem command.
*/
static void initForMsgStatusCmd(modemCmdWriteData_t *writeCmdP)
{
    mcData.modemCmdId = M_COMMAND_MESSAGE_STATUS;
    TX_HEADER_BUF[0] = M_COMMAND_MESSAGE_STATUS;           // command Byte
    mcData.crc = ((uint16_t)0x0140);                       // pre-calculated crc
    mcData.txHeaderLength = 1;
    mcData.txMsgContainsAPayload = false;
    mcData.expectedResponseLength = 23;                    // start,cmd,status[18],crc[2],end
}

/**
* \brief Fill header with a read partial data modem message.
* \brief Helper function to initialize the tx msg header with a
*        read partial data command.
*
* @param offset Offset in bytes
* @param size  Size in bytes
*/
static void initForIncommingPartialCmd(modemCmdWriteData_t *writeCmdP)
{

    uint16_t offsetInBytes = writeCmdP->payloadOffset;
    uint16_t sizeInBytes = writeCmdP->payloadLength;

    mcData.modemCmdId = M_COMMAND_GET_INCOMING_PARTIAL;
    TX_HEADER_BUF[0] = M_COMMAND_GET_INCOMING_PARTIAL;
    TX_HEADER_BUF[1] = 0x00;                               // offset bits 24-31
    TX_HEADER_BUF[2] = 0x00;                               // offset bits 16-23
    TX_HEADER_BUF[3] = (offsetInBytes >> 8) & 0xff;        // offset bits 8-15
    TX_HEADER_BUF[4] = offsetInBytes & 0xff;               // offset bits 0-7
    TX_HEADER_BUF[5] = 0x00;                               // size bits 24-31
    TX_HEADER_BUF[6] = 0x00;                               // size bits 16-23
    TX_HEADER_BUF[7] = (sizeInBytes >> 8) & 0xff;          // size bits 8-15
    TX_HEADER_BUF[8] = sizeInBytes & 0xff;                 // size bits 0-7
    mcData.txHeaderLength = 9;
    mcData.txMsgContainsAPayload = false;
    mcData.crc = gen_crc16(&(TX_HEADER_BUF[0]), mcData.txHeaderLength);
    mcData.expectedResponseLength = 13 + sizeInBytes;      // start,cmd,len[4],remaining[4],payload[sizeInBytes],crc[2],end
}

/**
* \brief Fill header with a delete modem message.
* \brief Helper function to initialize the tx msg header with a
*        delete incoming message command.
*/
static void initForDeleteIncomingCmd(modemCmdWriteData_t *writeCmdP)
{
    mcData.modemCmdId = M_COMMAND_DELETE_INCOMING;
    TX_HEADER_BUF[0] = M_COMMAND_DELETE_INCOMING;          // command byte
    mcData.crc = 0xf141;                                   // precalculated crc
    mcData.txHeaderLength = 1;
    mcData.txMsgContainsAPayload = false;
    mcData.expectedResponseLength = 5;                     // start,cmd,crc[2],end
}

/*=============================================================================*/

static inline void enable_UART_tx(void)
{
    UC0IE |= UCA0TXIE;
}

static inline void enable_UART_rx(void)
{
    UC0IE |= UCA0RXIE;
}

static inline void disable_UART_tx(void)
{
    UC0IE &= ~UCA0TXIE;
}

static inline void disable_UART_rx(void)
{
    UC0IE &= ~UCA0RXIE;
}

/*=============================================================================*/

/*****************************
 * UART Interrupt Functions
 ****************************/

/**
* \brief Uart Transmit Interrupt Service Routine.
* \ingroup ISR
*/
#ifndef FOR_USE_WITH_BOOTLOADER
#pragma vector=USCIAB0TX_VECTOR
#endif
__interrupt void USCI0TX_ISR(void)
{

    switch (mcData.txIsrState)
    {

        case TX_ISR_STATE_SEND_START_BYTE:
            UCA0TXBUF = MODEM_CMD_START_BYTE;
            mcData.txIsrState = TX_ISR_STATE_HEADER;
            break;

        case TX_ISR_STATE_HEADER:
            UCA0TXBUF = TX_HEADER_BUF[mcData.txIsrDataIndex++];
            if (mcData.txIsrDataIndex == mcData.txHeaderLength)
            {
                mcData.txIsrDataIndex = 0;
                if (mcData.txMsgContainsAPayload && mcData.txMsgPayloadLength)
                {
                    mcData.txIsrState = TX_ISR_STATE_PAYLOAD;
                }
                else
                {
                    mcData.txIsrState = TX_ISR_STATE_CRC_BYTE_0;
                }
            }
            break;

        case TX_ISR_STATE_PAYLOAD:
            UCA0TXBUF = mcData.txPayloadP[mcData.txIsrDataIndex++];
            if (mcData.txIsrDataIndex >= mcData.txMsgPayloadLength)
            {
                mcData.txIsrState = TX_ISR_STATE_CRC_BYTE_0;
            }
            break;

        case TX_ISR_STATE_CRC_BYTE_0:
            UCA0TXBUF = ((mcData.crc >> 8) & 0xff);
            mcData.txIsrState = TX_ISR_STATE_CRC_BYTE_1;
            break;

        case TX_ISR_STATE_CRC_BYTE_1:
            UCA0TXBUF = mcData.crc & 0xff;
            mcData.txIsrState = TX_ISR_STATE_SEND_STOP_BYTE;
            break;

        case TX_ISR_STATE_SEND_STOP_BYTE:
            UCA0TXBUF = MODEM_CMD_END_BYTE;
            mcData.txIsrState = TX_ISR_STATE_DISABLE;
            break;

        default:
        case TX_ISR_STATE_DISABLE:
            disable_UART_tx();
            mcData.txIsrMsgComplete = true;
            break;
    }
}

/**
* \brief Uart Receive Interrupt Service Routine.
* \note see uartIsr.c for the top level UART RX interrupt
*       routine.
*
* \ingroup ISR
*/
void modemCmd_isr(void)
{

    bool done = false;
    uint8_t rxByte = UCA0RXBUF;

    if ((mcData.rxIsrDataIndex == 0) && rxByte != MODEM_RESP_START_BYTE)
    {
        // Just return if we are expecting a response start byte and its not one.
        return;
    }
    else if (mcData.rxIsrDataIndex < ISR_RX_BUF_SIZE)
    {
        // Read the data as long as there is room in the rx buffer
        RX_BUF[mcData.rxIsrDataIndex++] = rxByte;
    }
    else
    {
        // Trouble - we went beyond the buffer length
        done = true;
    }

    if (mcData.rxIsrDataIndex == mcData.expectedResponseLength)
    {
        // If at the expected response length, we are done
        done = true;
    }

    if (done)
    {
        disable_UART_rx();
        mcData.rxIsrMsgComplete = true;
    }
}

