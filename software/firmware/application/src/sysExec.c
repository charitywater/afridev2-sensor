/** 
 * @file sysExec.c
 * \n Source File
 * \n Afridev-V2 MSP430 Firmware
 * 
 * \brief main system exec that runs the top level loop.
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
#ifdef WATER_DEBUG
#include "debugUart.h"
#endif
#include "outpour.h"
#include "waterDetect.h"
#include "appAlgo.h"
#include "waterSense.h"

/***************************
 * Module Data Definitions
 **************************/

/**
 * \def START_UP_MSG_TX_DELAY_IN_SECONDS
 * \brief Two messages are transmitted after the system starts. 
 *        This definition specifies the delay in seconds after
 *        startup until the first startup message is sent to the
 *        modem for transmit. After the first startup message
 *        has completed transmitting, the same delay is used to
 *        determine when to transmit the second startup message.
 */
#define START_UP_MSG_TX_DELAY_IN_SECONDS ((uint8_t)10)

/**
 * \def REBOOT_DELAY_IN_SECONDS
 * \brief This define is used in conjunction with the OTA reset
 *        device message. It specifies how long to wait after
 *        the message was received to perform the MSP430 reset.
 */
#define REBOOT_DELAY_IN_SECONDS ((uint8_t)20*TIME_SCALER)


/****************************
 * Module Data Declarations
 ***************************/

sysExecData_t sysExecData;
static uint16_t xAppErrorBits = 0;

/*************************
 * Module Prototypes
 ************************/
static void startUpMessageCheck(void);
static bool startUpSendTestCheck(void);
static void sendModemTestMsg(void);
static void sendStartUpMsg1(void);
static void sendStartUpMsg2(void);
#ifndef WATER_DEBUG
static void sysExec_doReboot(void);
#ifdef SEND_DEBUG_TIME_DATA
static void sendTimeStampMsg(void);
#endif
#endif
#ifdef SEND_DEBUG_INFO_TO_UART
static void sysExec_sendDebugDataToUart(void);
#endif
#ifdef SIMULATE
uint16_t simulateWaterAnalysis(uint8_t num_samples);
#endif

/***************************
 * Module Public Functions
 **************************/

/**
* \brief This function contains the main software loop
*        for the MSP430 firmware.  After calling the
*        initialization routines, it drops into an infinite
*        while loop.  In the while loop, it goes into a low
*        power mode (i.e. sleep) waiting for the timer interrupt
*        to wake it up. The timer interrupt occurs once every .5
*        seconds. Once awake, it performs a water capacitance
*        measurement. Every 4th time through the main loop, it
*        performs the water detection algorithm and system tasks
*        by calling the exec routines of the different
*        sub-modules.
*
* \ingroup EXEC_ROUTINE
*/
void sysExec_exec(void)
{
    uint8_t exec_main_loop_counter = 0;

    // Restart the one-second watchdog timeout
    WATCHDOG_TICKLE();

    memset(&sysExecData, 0, sizeof(sysExecData_t));

    // Set how long to wait until first startup message should be transmitted
    sysExecData.secondsTillStartUpMsgTx = START_UP_MSG_TX_DELAY_IN_SECONDS;

    // Initialize the date for Jan 1, 2018
    // h, m, s, am/pm (must be in BCD)
    setTime(0x00, 0x00, 0x00, 0x00);
    // y, m, d
    setDate(2018, 1, 1);


    //set error bit indicating we dont have a valid time - this gets cleared
    //when we get the GMT message
    sysExec_indicateError(NO_RTC_TIME);

#ifndef WATER_DEBUG
    // Call the module init routines
    modemPower_init();
    modemCmd_init();
    modemMgr_init();
    dataMsgSm_init();
    dataMsgMgr_init();
    otaMsgMgr_init();
#else
    dbg_uart_init();
#endif
    waterSense_init();
    storageMgr_init();

    gpsMsg_init();
    gpsPower_init();
    gps_init();
    msgSched_init();

    APP_ALGO_init();

    // Start the timer interrupt
    timerA0_init();

    // Enable the global interrupt
    enableGlobalInterrupt();

#ifndef WATER_DEBUG
    // Restart the one-second watchdog timeout
    WATCHDOG_TICKLE();
#endif

#if 0
    LED_GREEN_DISABLE();
    LED_RED_DISABLE();
    WATCHDOG_STOP();
    timerA0_20sec_sleep();
    hal_low_power_enter();
    WATCHDOG_TICKLE();
#endif

    // the following line makes it possible to load new firmware without waiting 24 hours
    // just pump water (or take the well apart to reload)
#ifdef DEBUG_SEND_SENSOR_DATA_NOW
    sysExecData.sendSensorDataNow = 1;  
#endif
    // Start the infinite exec main loop
    while (1)
    {
        // Go into low power mode.
        // Wake on the exit of the TimerA0 interrupt.
        // Wakes every .1 seconds to run the main loop & take cap sense sample
       __bis_SR_register(LPM3_bits);

        // Restart the one-second watchdog timeout
        WATCHDOG_TICKLE();

        // Don't take a measurement if the modem or GPS is in use
#ifndef WATER_DEBUG
        if (!modemMgr_isAllocated() && !gps_isActive())
#else
        if (!gps_isActive())
#endif
        {
            //take cap sense reading
            waterSense_takeReading();

            //Run algorithm nest
            APP_ALGO_runNest();
        }

        // Increment main loop counter
        exec_main_loop_counter++;

        // Perform system tasks every SECONDS_PER_TREND (i.e. every 2 seconds)
        // which is every 20th time that the exec main loop runs.
        if (exec_main_loop_counter >= TICKS_PER_TREND )
        {
            exec_main_loop_counter = 0;

            // Record the water stats and initiate periodic communication if it is time to do so
            storageMgr_exec();

#ifndef WATER_DEBUG
            // Perform communication support - these run the state machines
            // that perform the software modem interaction.  They do not
            // initiate modem communication, but once communication is started
            // they handle all aspects of the modem interfacing.
            modemCmd_exec();                               /* perform Low-level modem interface processing */
            dataMsgMgr_exec();                             /* perform High-level send data message */
            otaMsgMgr_exec();                              /* perform High-level OTA Message Processing */
            modemMgr_exec();                               /* perform Low-level message processing */
            modemCmd_exec();                               /* perform Low-level modem interface processing (again) */
            modemPower_exec();                             /* Handle powering on and off the modem */
#else
            manufRecord_manuf_test_result();
#endif
            gpsMsg_exec();                                 /* Handle GPS message processing */
            gpsPower_exec();                               /* Handle power on and off the GPS device */
            gps_exec();                                    /* Manage GPS processing */
            msgSched_exec();                               /* Transmit modem messages if scheduled */

#ifndef WATER_DEBUG
            // A system reboot sequence is started when a firmware upgrade message
            // or system restart message is received. If a reboot sequence has started,
            // then decrement counter and check for a timeout.  When timeout occurs,
            // perform a reboot.
            if (sysExecData.rebootCountdownIsActive == ACTIVATE_REBOOT_KEY)
            {
                if (sysExecData.secondsTillReboot >= 0)
                {
                    sysExecData.secondsTillReboot -= SECONDS_PER_TREND;
                }
                if (sysExecData.secondsTillReboot <= 0)
                {
                    sysExec_doReboot();
                }
            }
#endif
            // don't try to send while GPS is going
            if (!gps_isActive())
            {
                if (startUpSendTestCheck())
                {
                    // Two messages are transmitted shortly after the system starts:
                    // The final assembly message and a monthly check-in message.
                    if (!sysExecData.FAMsgWasSent || !sysExecData.mCheckInMsgWasSent || !sysExecData.appRecordWasSet)
                    {
                        startUpMessageCheck();
                    }
                }
            }

#ifdef SEND_DEBUG_INFO_TO_UART
            // Only send debug data if the modem is not in use.
            if (!modemMgr_isAllocated())
            {
                sysExec_sendDebugDataToUart();
            }
#endif
        } // every 20 clock ticks ( 2 seconds)

    } // end while 1
}

void sysExec_indicateError(uint16_t errorBit)
{
    //set bit
    xAppErrorBits|= errorBit;
}

void sysExec_indicateErrorResolved(uint16_t errorBit)
{
    //clear the bit
    xAppErrorBits&= ~errorBit;
}

uint16_t sysExec_getErrorBits(void)
{
    return xAppErrorBits;
}

/***************************
 * Module Private Functions
 **************************/

/**
 * \brief This function coordinates the one-time-only operation of sending the SEND_TEST 
 *        message to the Modem.  This is a first step in the process of provisioning the Modem.
 * 
 * @return bool Returns true if the Modem reports that the SEND_TEST message was seen and responded
 *         to by the Network.
 */
static bool startUpSendTestCheck(void)
{

    // this is run only once per startup
    if (!sysExecData.sendTestRespWasSeen)
    {
        // check if the Modem SEND_TEST message must be sent
        if (!sysExecData.sendTestMsgWasSent)
        {
            // send the SEND_TEST message
            // this can take minutes to finish
            sendModemTestMsg();
            sysExecData.sendTestMsgWasSent = true;
        }
        else
        {
            // Restart the one-second watchdog timeout
            WATCHDOG_TICKLE();

            // this is set in the Modem Code
#ifndef WATER_DEBUG
            if (sysExecData.send_test_result > 0)
#else
            if (1)                                         // modem code is not running in debug
#endif
                {
                    sysExecData.sendTestRespWasSeen = true;
                    // give modem a chance to shutdown for this test
                    sysExecData.secondsTillStartUpMsgTx = START_UP_MSG_TX_DELAY_IN_SECONDS;

                }                                          // if send test result was seen (or timed out)
        }                                                  // else send_test was sent
    }                                                      // if !sysExecData.sendTestRespWasSeen
    return (sysExecData.sendTestRespWasSeen);
}

/**
 * \brief There are two startup messages that must be sent.
 *        Check the flags and delays to determine if the first
 *        one should be sent. Once the first startup message is
 *        sent, start the countdown to send the next one - but only
 *        after the first one has completed transmitting.
 *        there is a seperate 10 second pause for writing the APP record to ensure
 *        the modem is not active while the flash write occurs (we don't want a
 *        WATCHDOG reset)
 *
 * \note The application record is updated after the first
 *       startup message completes. The record is written by the
 *       Application and used by the bootloader to help identify
 *       that the Application started successfully. The
 *       application record lives in one of the flash info
 *       sections.
 * 
 */
static void startUpMessageCheck(void)
{
#ifndef WATER_DEBUG
    if (!sysExecData.FAMsgWasSent && !dataMsgMgr_isSendMsgActive() && !mwBatchData.batchWriteActive)
    {
#else
    if (!sysExecData.FAMsgWasSent)
    {
#endif
        if (sysExecData.secondsTillStartUpMsgTx > 0)
        {
            sysExecData.secondsTillStartUpMsgTx -= SECONDS_PER_TREND;
        }
        if (sysExecData.secondsTillStartUpMsgTx <= 0)
        {
            sendStartUpMsg1();
            sysExecData.FAMsgWasSent = true;
            sysExecData.secondsTillStartUpMsgTx = START_UP_MSG_TX_DELAY_IN_SECONDS;
        }
    }
#ifndef WATER_DEBUG
    else if (!sysExecData.appRecordWasSet && !dataMsgMgr_isSendMsgActive())
    {
#else
    else if (!sysExecData.appRecordWasSet)
    {
#endif
        if (sysExecData.secondsTillStartUpMsgTx > 0)
        {
            sysExecData.secondsTillStartUpMsgTx -= SECONDS_PER_TREND;
        }
        if (sysExecData.secondsTillStartUpMsgTx <= 0)
        {

            // Update the Application Record after sending the first FA packet.
            // The record is written by the Application and used by the
            // bootloader to help identify that the Application started successfully.
            // We wait until after the first FA packet is sent to write the
            // Application Record in an attempt to help verify that the
            // application code does not have a catastrophic issue.
            // If the bootloader does not see an Application
            // Record when it boots, then it will go into recovery mode.
            //
            // Don't write a new Application Record if one already exists.  It is
            // erased by the bootloader after a new firmware upgrade has been
            // performed before jumping to the new Application code.
            // The Application Record is located in the flash INFO C section.
            if (!appRecord_checkForValidAppRecord())
            {
                // If the record is not found, write one.
#ifndef WATER_DEBUG
                appRecord_initAppRecord();
#else
                //if (appRecord_initAppRecord())
                {
                    debug_message("***App Record Set***");
                    __delay_cycles(1000);
                }
#endif
            }
            sysExecData.appRecordWasSet = true;
            sysExecData.secondsTillStartUpMsgTx = START_UP_MSG_TX_DELAY_IN_SECONDS;
        }
    }
#ifndef WATER_DEBUG
    else if (!sysExecData.mCheckInMsgWasSent && !dataMsgMgr_isSendMsgActive() && !mwBatchData.batchWriteActive)
    {
#else
    else if (!sysExecData.mCheckInMsgWasSent)
    {
#endif
        if (sysExecData.secondsTillStartUpMsgTx > 0)
        {
            sysExecData.secondsTillStartUpMsgTx -= SECONDS_PER_TREND;
        }
        if (sysExecData.secondsTillStartUpMsgTx <= 0)
        {
            sendStartUpMsg2();
            sysExecData.mCheckInMsgWasSent = true;
            sysExecData.secondsTillStartUpMsgTx = START_UP_MSG_TX_DELAY_IN_SECONDS;
        }
    }
}

/**
* \brief Initiate sending the first startup message which is a 
*        Final Assembly Message Type
*/
static void sendModemTestMsg(void)
{
    // Prepare the SEND_TEST message, the monthly check in message is formatted, the body trace server ignores the payload
#ifndef WATER_DEBUG
    // Get the shared buffer (we borrow the ota buffer)
    uint8_t *payloadP = modemMgr_getSharedBuffer();
    // Fill in the buffer with the standard message header
    uint8_t payloadSize = storageMgr_prepareMsgHeader(payloadP, MSG_TYPE_MODEM_SEND_TEST);

    // Initiate sending the test message
    dataMsgMgr_sendTestMsg(MSG_TYPE_MODEM_SEND_TEST, payloadP, payloadSize);
#else
    debug_message("***Modem Send Test***");
    __delay_cycles(1000);
#endif
}


/**
* \brief Initiate sending the first startup message which is a
*        Final Assembly Message Type
*/
static void sendStartUpMsg1(void)
{

#ifndef WATER_DEBUG
    // Prepare the final assembly message
    // Get the shared buffer (we borrow the ota buffer)
    uint8_t *payloadP = modemMgr_getSharedBuffer();
    // Fill in the buffer with the standard message header
    uint8_t payloadSize = storageMgr_prepareMsgHeader(payloadP, MSG_TYPE_FINAL_ASSEMBLY);

    // Initiate sending the final assembly message
    dataMsgMgr_sendDataMsg(MSG_TYPE_FINAL_ASSEMBLY, payloadP, payloadSize);
#else
    debug_message("***Modem FA Message ***");
    __delay_cycles(1000);
#endif
}

/**
* \brief Initiate sending the second startup message which is a 
*        Monthly Check-In Message Type
*/
static void sendStartUpMsg2(void)
{

#ifndef WATER_DEBUG
    // Prepare the monthly check-in message
    // Get the shared buffer (we borrow the ota buffer)
    uint8_t *payloadP = modemMgr_getSharedBuffer();
    // Fill in the buffer with the standard message header
    uint8_t payloadSize = storageMgr_prepareMsgHeader(payloadP, MSG_TYPE_CHECKIN);

    // Initiate sending the monthly check-in message
    dataMsgMgr_sendDataMsg(MSG_TYPE_CHECKIN, payloadP, payloadSize);
#else
    debug_message("***Modem Monthly Check-In Message ***");
    __delay_cycles(1000);
#endif
}

/**
* \brief Initiate sending the Sensor Data Message
*/
#ifdef SEND_DEBUG_TIME_DATA
#ifndef WATER_DEBUG
static void sendTimeStampMsg(void)
{
    uint8_t *payloadP;
    uint8_t payloadSize = storageMgr_getTimestampMessage(&payloadP);

    dataMsgMgr_sendDataMsg(MSG_TYPE_TIMESTAMP, payloadP, payloadSize);
}
#endif
#endif

/**
* \brief Support utility for the OTA message that resets the 
*        unit.  Checks to make sure the message keys are
*        correct, and if so, then starts a countdown counter for
*        rebooting the unit.
* 
* @return bool  Returns true if keys were correct.
*/
bool sysExec_startRebootCountdown(uint8_t activateReboot)
{
    bool status = false;

    if (activateReboot == ACTIVATE_REBOOT_KEY)
    {
        sysExecData.secondsTillReboot = REBOOT_DELAY_IN_SECONDS;
        sysExecData.rebootCountdownIsActive = activateReboot;
        status = true;
    }
    return (status);
}

/**
* \brief Set the local variable that holds the result of the SEND_TEST message transmission.
* 
* @param result The result of the transmission
*
*/
void sysExec_set_send_test_result(uint8_t result)
{
    sysExecData.send_test_result = result;
}

#ifndef WATER_DEBUG
/**
* \brief Support routine that is called to perform a system 
*        reboot.  Called as a result of receiving the OTA reset
*        unit message.
*/
static void sysExec_doReboot(void)
{
    if (sysExecData.rebootCountdownIsActive == ACTIVATE_REBOOT_KEY)
    {
        // Disable the global interrupt
        disableGlobalInterrupt();
        // Modem should already be off, but just for safety, turn it off
        modemPower_powerDownModem();
        while (1)
        {
            // Force watchdog reset
            WDTCTL = 0xDEAD;
            while (1);
        }
    }
    else
    {
        sysExecData.rebootCountdownIsActive = 0;
    }
}


#ifdef SEND_DEBUG_INFO_TO_UART
/**
* \brief Send debug information to the uart.  
*/
static void sysExec_sendDebugDataToUart(void)
{
    // Get the shared buffer (we borrow the ota buffer)
    uint8_t *payloadP = modemMgr_getSharedBuffer();
    uint8_t payloadSize = storageMgr_prepareMsgHeader(payloadP, MSG_TYPE_DEBUG_TIME_INFO);

    dbgMsgMgr_sendDebugMsg(MSG_TYPE_DEBUG_TIME_INFO, payloadP, payloadSize);
    _delay_cycles(10000);
    storageMgr_sendDebugDataToUart();
    _delay_cycles(10000);
    // waterSense_sendDebugDataToUart();
    // _delay_cycles(10000);
}
#endif
#endif
