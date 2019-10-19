/** 
 * @file sysExec.c
 * \n Source File
 * \n Afridev-V2 MSP430 Firmware
 * 
 * \brief main system exec that runs the top level loop.
 */
#ifdef WATER_DEBUG
#include "debugUart.h"
#endif
#include "outpour.h"
#include "waterDetect.h"
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

/**
 * \def NO_WATER_HF_TO_LF_TIME_IN_SECONDS
 * \brief  Specify how long to wait before switching to
 *         the low-frequency (LF) water measurement interval in
 *         order to save power. By default, the system is in the
 *         high-frequency (HF) water measurement interval where
 *         water measurements are take every main loop
 *         iterataion. If water is detected while in the LF
 *         mode, the system immediately switches back to HF
 *         mode.
 */
#define NO_WATER_HF_TO_LF_TIME_IN_SECONDS ((uint16_t)TIME_60_SECONDS*5)

/**
 * \def LOW_FREQUENCY_MEAS_TIME_IN_SECONDS
 * \brief Specify how often to take the batch water measurements
 *        when in the low-frequency water measurement mode. The
 *        system transitions to the low-frequency interval if no
 *        water has been detected over a
 *        NO_WATER_HF_TO_LF_TIME_IN_SECONDS window of time.
 */
#define LOW_FREQUENCY_MEAS_TIME_IN_SECONDS ((uint8_t)TIME_20_SECONDS)

/**
 * \def WATER_LF_MEAS_BATCH_COUNT
 * This controls the number of water measurements taken per 
 * LOW_FREQUENCY_MEAS_TIME_IN_SECONDS while in the low-frequency 
 * water measurement mode.
 */
#define WATER_LF_MEAS_BATCH_COUNT 4

/****************************
 * Module Data Declarations
 ***************************/

sysExecData_t sysExecData;

/*************************
 * Module Prototypes
 ************************/

static uint16_t analyzeWaterMeasurementData(uint8_t num_samples);

static void startUpMessageCheck(void);
static bool startUpSendTestCheck(void);
static void sendModemTestMsg(void);
static void sendStartUpMsg1(void);
static void sendStartUpMsg2(void);
#ifndef WATER_DEBUG
static void sendSensorDataMsg(void);
static void sysExec_doReboot(void);
#ifdef SEND_DEBUG_TIME_DATA
static void sendTimeStampMsg(void);
#endif
#endif
uint8_t lowPowerMode_check(uint16_t currentFlowRateInMLPerSec);
#ifdef SEND_DEBUG_INFO_TO_UART
static void sysExec_sendDebugDataToUart(void);
#endif

/***************************
 * Module Public Functions
 **************************/

uint16_t processWaterAnalysis(num_samples)
{
    uint16_t currentFlowRateInMLPerSec = 0;

    // Don't perform water data analysis if modem is in use.
    #ifndef WATER_DEBUG
    if (!modemMgr_isAllocated() && !gps_isActive())
    {
    #else
    if (!gps_isActive())
    {
    #endif
        currentFlowRateInMLPerSec = analyzeWaterMeasurementData(num_samples);

        // make sure there is no water on the pads when saving the baseline
        if (sysExecData.saveCapSensorBaselineData && currentFlowRateInMLPerSec == 0)
        {
    #ifdef WATER_DEBUG
            bool air_targets_set;

            // the baseline cap data was recorded at a certain temperature.  Upon system reset
            // this data needs to be loaded as the default air target but temperature adjusted
            air_targets_set = manufRecord_setBaselineAirTargets();

            if (!air_targets_set)
            {
                debug_message("***AIR Targets Not SET***");
                __delay_cycles(1000);
            }
    #else
            manufRecord_setBaselineAirTargets();
    #endif
            sysExecData.saveCapSensorBaselineData = 0;
        }
    }
    return (currentFlowRateInMLPerSec);
}

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
    uint8_t temperature_loop_counter = 0;
    uint16_t currentFlowRateInMLPerSec = 0;

    // Restart the one-second watchdog timeout
    WATCHDOG_TICKLE();

    memset(&sysExecData, 0, sizeof(sysExecData_t));

    // Set how long to wait until first startup message should be transmitted
    sysExecData.secondsTillStartUpMsgTx = START_UP_MSG_TX_DELAY_IN_SECONDS;
    sysExecData.dry_wake_time = SYSEXEC_NO_WATER_SLEEP_DELAY;

    // Initialize the date for Jan 1, 2018
    // h, m, s, am/pm (must be in BCD)
    setTime(0x00, 0x00, 0x00, 0x00);
    // y, m, d
    setDate(2018, 1, 1);

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

    waterSenseReadInternalTemp();
    // setup manufacturing testing and restore baseline water data from flash measured during
    // these tests
    sysExecData.saveCapSensorBaselineData = manufRecord_manuf_test_init();

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
        // Wakes every .5 seconds to run the main loop
        if (!sysExecData.last_sleep_time)
           __bis_SR_register(LPM3_bits);

        // Restart the one-second watchdog timeout
        WATCHDOG_TICKLE();

        // Take a water measurement
        if (!sysExecData.last_sleep_time)
        {
            // Don't take a measurement if the modem or GPS is in use
#ifndef WATER_DEBUG
            if (!modemMgr_isAllocated() && !gps_isActive())
#else
            if (!gps_isActive())
#endif
            {
                waterSense_takeReading();
            }
            // Increment main loop counter
            exec_main_loop_counter++;
        }

        // Perform system tasks every SECONDS_PER_TREND (i.e. every 2 seconds)
        // which is every fourth time that the exec main loop runs.
        // time elapsed is set when the unit slept for 20 seconds
        if (exec_main_loop_counter >= TICKS_PER_TREND || sysExecData.last_sleep_time)
        {
            // up to 2 seconds elapsed since last action (sleep or awake) add to sleep time and
            // clear the loop counter
            sysExecData.last_sleep_time += exec_main_loop_counter/2;
            exec_main_loop_counter = 0;

            // only do water measurements when not sleeping
            if (sysExecData.last_sleep_time <= 2)
            {
                temperature_loop_counter++;

                // Take a temperature measurement every 20 seconds while awake
                if (temperature_loop_counter > 10)
                {
                    temperature_loop_counter = 0;
                    waterSenseReadInternalTemp();
                }
                currentFlowRateInMLPerSec =  processWaterAnalysis(SAMPLE_COUNT);
                waterDetect_start();  // prepare for next session
            }
            else
            {
                // we were sleeping 20 seconds, read the temperature
                temperature_loop_counter = 0;
                waterDetect_start();
                waterSenseReadInternalTemp();
#ifdef READ_WATER_BETWEEN_SLEEPS
#ifndef WATER_DEBUG
                if (!modemMgr_isAllocated() && !gps_isActive())
#else
                if (!gps_isActive())
#endif
                {

                    // there is a perceived problem with the unit sleeping and not tracking water consumption while
                	// sleeping.   This will update the water data between sleep attempts
                    waterSense_takeReading();
                    currentFlowRateInMLPerSec = processWaterAnalysis(1);
#ifdef WATER_DEBUG
                    uint32_t sys_time = getSecondsSinceBoot();
                    debug_pour_total(sys_time, currentFlowRateInMLPerSec);
#endif
                }
                else
                    currentFlowRateInMLPerSec = 0;
#else
                currentFlowRateInMLPerSec = 0;
#endif
                waterDetect_start();  // prepare for next session
            }

            // Record the water stats and initiate periodic communication
            storageMgr_exec(currentFlowRateInMLPerSec, sysExecData.last_sleep_time);
            // prepare for next iteration of this loop to be sure water measurements are made
            sysExecData.last_sleep_time = 0;

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
#ifndef WATER_DEBUG
                    // wait until system startup sequences have finished and measurement data was seen at least once
                    // and baseline data recorded to queue up transmission
                    else if (sysExecData.sendSensorDataMessage || sysExecData.faultWaterDetect)
                    {
                        // let the OTA reply for SENSOR_DATA get out before queuing up the reply message
                        // also be sure NO OTHER MESSAGE is being transmitted
                        if (mTestBaselineDone() && !dataMsgMgr_isSendMsgActive() && !mwBatchData.batchWriteActive)
                        {
                            // Tell the modem code to send the data
                            // if the water detection got stuck reporting water or unknowns endlessly, we want to see data for this
                            if (sysExecData.sendSensorDataNow || sysExecData.faultWaterDetect)
                            {
                                sendSensorDataMsg();
                                sysExecData.total_flow = 0;
                            }
                            else
                                msgSched_scheduleSensorDataMessage();

                            // clear out the data and start again
                            if (sysExecData.faultWaterDetect)
                            {
                            	waterDetect_init();
                            	sysExecData.faultWaterDetect = false;
                            }

                            sysExecData.sendSensorDataMessage = false;
                        }
                    }
#ifdef SEND_DEBUG_TIME_DATA
                    else if (sysExecData.sendTimeStamp)
                    {
                        if (mTestBaselineDone() && !dataMsgMgr_isSendMsgActive() && !mwBatchData.batchWriteActive)
                        {
                            sendTimeStampMsg();
                            sysExecData.sendTimeStamp = false;
                        }
                    }
#endif
#endif
                }
            }

            // update feedback LEDs to show when Modem initialization happens
            // and the result of the test
            manufRecord_update_LEDs();

#ifdef SEND_DEBUG_INFO_TO_UART
            // Only send debug data if the modem is not in use.
            if (!modemMgr_isAllocated())
            {
                sysExec_sendDebugDataToUart();
            }
#endif
        } // every 4 clock ticks

        sysExecData.last_sleep_time = lowPowerMode_check(currentFlowRateInMLPerSec);  //every clock tick
    } // end while 1
}

/***************************
 * Module Private Functions
 **************************/

/**
 * \brief Call the water data analysis routine.
 * 
 * @return uint16_t Returns the current flow rate measured in 
 *         milliliters per second.
 */
static uint16_t analyzeWaterMeasurementData(uint8_t num_samples)
{
    uint16_t currentFlowRateInMLPerSec = 0;

    // Perform algorithm to analyze water data samples.
    waterSense_analyzeData(num_samples);

    // Read the flow rate in milliliters per second
    currentFlowRateInMLPerSec = waterSense_getLastMeasFlowRateInML();

    return (currentFlowRateInMLPerSec);
}

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
* \brief This function will check if the pump has been idle long enough to put the unit to sleep for 20 seconds
*        For debugging the leds are being flashed to show when water is detected (green)and when the unit is sleeping (red)
*/
uint8_t lowPowerMode_check(uint16_t currentFlowRateInMLPerSec)
{
    static volatile int blink_red = 0;
    uint8_t time_elapsed  = 0;

    if (sysExecData.dry_wake_time > 0)
    {
        // do not go to sleep if the modem is busy with updates or send test. or the gps is measuring
         if (sysExecData.send_test_result != SYSEXEC_SEND_TEST_RUNNING && !gps_isActive() && !modemMgr_isAllocated())
         {
                if (currentFlowRateInMLPerSec > 0)
                {
                   sysExecData.dry_count = 0;
#ifdef SLEEP_DEBUG
                   hal_led_none();
                   hal_led_green();    // debug only -- green LED to indicate water present
                   sysExecData.led_on_time = 0;  // prevent the manufRecord_update_LEDs() from clearing the toggle
#endif
                }
                else
                {
                   // capsense interrupts would wake this instantly wait until the sensing is done 
                   if (sysExecData.dry_count++ > sysExecData.dry_wake_time && !CAPSENSE_ACTIVE)
                   {
                       // keep track of consecutive sleeps, if > 5 add a second to the time
                       sysExecData.sleep_count++;
                       sysExecData.sleep_alot++;
#ifdef SLEEP_DEBUG
                       //hal_led_red();  // for debug only - red LED to indicate sleep
                       //sysExecData.led_on_time = 0;  // prevent the manufRecord_update_LEDs() from clearing the toggle
#endif
#ifdef WATER_DEBUG
                       debug_message("*SLEEP*");
                       WATCHDOG_TICKLE();
                       // do the 20 second sleep!  First clear out pending debug messages
                       while(!dbg_uart_txqempty());  // send the rest of the debug data
                       while(dbg_uart_txpend());     // wait for the last character to be consumed
                       _delay_cycles(2000);          // wait one character time afterwards so the terminal is not confused
#endif
                       WATCHDOG_STOP();

                       timerA0_20sec_sleep();
                       hal_low_power_enter();
                       timerA0_inter_sample_sleep();
                       // correct inaccuracy in RTC due to sleeping
                       // every 100 seconds (5*20), add 1 second
                       if (sysExecData.sleep_count>=5);
                       {
                           all_timers_adjust_time(1);
                           sysExecData.sleep_count = 0;
                       }
                       // every 1020 seconds (51*20), add 3 seconds
                       if (sysExecData.sleep_alot>=51)
                       {
                           all_timers_adjust_time(3);
                           sysExecData.sleep_alot = 0;
                       }
                       for (time_elapsed = 0; time_elapsed < 20; time_elapsed++)
                          incrementSeconds();
                       WATCHDOG_TICKLE();

                       // this keeps the unit in "dry mode" without overflowing the counter (if dry a long time > 9 hours)
                       sysExecData.dry_count = sysExecData.dry_wake_time + 1;
                       time_elapsed = 20;
                   }
                   else
                       hal_led_none();
                }
         }
#ifdef SLEEP_DEBUG
         else
         {
            if (sysExecData.send_test_result != SYSEXEC_SEND_TEST_RUNNING && (gps_isActive() || modemMgr_isAllocated()))
            {
                hal_led_none();  //turn off green led
                // flash red when modem is busy
                if (!blink_red)
                {
                   hal_led_blink_red();
                   blink_red = 10;
                }
                else
                   --blink_red;
                sysExecData.led_on_time = 5;  // cause the manufRecord_update_LEDs() to clear the red led after modem stops
            }
         }
#endif
    }

    // tell the caller if 0 or 20 additional seconds passed by
    return (time_elapsed);
}


/**
* \brief Initiate sending the Sensor Data Message
*/
#ifndef WATER_DEBUG
static void sendSensorDataMsg(void)
{
    uint8_t *payloadP;
    uint8_t payloadSize = manufRecord_getSensorDataMessage(&payloadP);

    dataMsgMgr_sendDataMsg(MSG_TYPE_SENSOR_DATA, payloadP, payloadSize);
}
#endif


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
