/** 
 * @file storage.c
 * \n Source File
 * \n Afridev MSP430 Firmware
 * 
 * \brief Routines to support processing, storing and 
 *        transmitting water data statistics. This module
 *        maintains the storage clock which is an independent
 *        clock from the GMT time. The storage clock is used to
 *        schedule data storage and message transmission.
 */

#include "outpour.h"
#ifdef WATER_DEBUG
#include "debugUart.h"
#endif

/***************************
 * Module Data Definitions
 **************************/

/**
 * \def TOTAL_WEEKLY_LOGS
 * \brief Specify the number of weekly logs in flash.
 */
#define TOTAL_WEEKLY_LOGS ((uint8_t)5)

/**
 * \def WEEKLY_LOG_SIZE
 * \brief Specify the total size of a weekly block allocated in 
 *        flash.
 */
#define WEEKLY_LOG_SIZE ((uint16_t)0x400)

/**
 * \def TOTAL_DAYS_IN_A_WEEK
 * \brief For clarity in the code
 */
#define TOTAL_DAYS_IN_A_WEEK ((uint8_t)7)

/**
 * \def TOTAL_HOURS_IN_A_DAY
 * \brief For clarity in the code
 */
#define TOTAL_HOURS_IN_A_DAY ((uint8_t)24)

/**
 * \def TOTAL_MINUTES_IN_A_HOUR
 * \brief For clarity in the code
 */
#define TOTAL_MINUTES_IN_A_HOUR ((uint8_t)60)

/**
 * \def TOTAL_SECONDS_IN_A_MINUTE
 * \brief For clarity in the code
 */
#define TOTAL_SECONDS_IN_A_MINUTE ((uint8_t)60)

/**
 * \def FLASH_WRITE_ONE_BYTE
 * \brief For clarity in the code
 */
#define FLASH_WRITE_ONE_BYTE ((uint8_t)1)

/**
 * \def DAILY_MILLILITERS_ACTIVATION_THRESHOLD
 * \brief Used as the daily milliliters threshold to consider 
 *        the unit activated.
 */
#define DAILY_MILLILITERS_ACTIVATION_THRESHOLD ((uint16_t)50*1000)

/**
 * \def MIN_DAILY_LITERS_TO_SET_REDFLAG_CONDITION
 * \brief The daily liters must meet this threshold before a red
 *        flag condition can be set.
 */
#define MIN_DAILY_LITERS_TO_SET_REDFLAG_CONDITION ((uint16_t)200)

/**
 * \def FLASH_BLOCK_SIZE
 * \brief Define how big a flash block is.  Represents the 
 *        minimum size that can be erased.
 */
#define FLASH_BLOCK_SIZE ((uint16_t)512)

/**
 * \def DO_RED_FLAG_PROCESSSING
 * \brief If set to non-zero value, then the red flag processing 
 *        function will be called.
 */
#define DO_RED_FLAG_PROCESSING 1

/**
 * \def DO_RED_FLAG_TRANSMISSION
 * \brief If set to non-zero value, then a new red flag 
 *        condition will initiate daily log transmission.
 */
#define DO_RED_FLAG_TRANSMISSION 1

/**
 * \def RED_FLAG_TOTAL_MAPPING_DAYS
 * \brief Specify how many days to perform the redflag mapping.
 *        Set to 4 weeks (4 x 7)
 */
#define RED_FLAG_TOTAL_MAPPING_DAYS 28

/**
 * \def RED_FLAG_MAPPING_WEEKS_BIT_SHIFT
 * \brief Specify a bit shift representing the number of weeks 
 *        that the red flag mapping occurs (4 weeks).
 */
#define RED_FLAG_MAPPING_WEEKS_BIT_SHIFT 2

/**
 * \typedef dailyLog_t
 * \brief Define the structure of the daily log data that is 
 *        sent inside the daily water log message.
 */
typedef struct __attribute__((__packed__))dailyLog_s {
    uint16_t litersPerHour[24];                            /**< 48, 00-47 */
    uint16_t totalLiters;                                  /**< 02, 48-49 */
    uint16_t averageLiters;                                /**< 02, 50-51 */
    uint8_t redFlag;                                       /**< 01, 52    */
    uint8_t outOfSpec;                                     /**< 01, 53    */
    uint16_t unknowns;                                     /**< 02, 54-55 */
    uint16_t padSubmergedCount[6];                         /**< 12, 56-67 */
} dailyLog_t;

typedef union packetHeader_s {
    msgHeader_t msgHeader;
    uint8_t bytes[16];                                     // force to 16 bytes
} packetHeader_t;

typedef union packetData_s {
    dailyLog_t dailyLog;
    uint8_t bytes[112];                                    // force to 112 bytes
} packetData_t;

/**
 * \typedef dailyPacket_t
 * \brief Define the structure of the daily log packet that is 
 *        sent to the server.  Note that it consists of a header
 *        and the data.  Both the header and the data sections
 *        are set to a specific size by using unions.  The total
 *        size of the packet is set at 128 bytes. 
 */
typedef struct dailyPacket_s {
    packetHeader_t packetHeader;
    packetData_t packetData;
} dailyPacket_t;

/**
 * \typedef weeklyLog_t
 * \brief  Define the layout of the weekly log in flash.  It 
 *         currently consists of the 7 daily log packets and
 *         meta data.
 */
typedef struct weeklyLog_s {
    dailyPacket_t dailyPackets[7];                         /**< The seven daily logs of the week */
    uint8_t clearOnTransmit[7];                            /**< Byte cleared for day when log transmitted */
    uint8_t clearOnReady[7];                               /**< Byte cleared for day when log ready to send */
} weeklyLog_t;


/****************************
 * Module Data Declarations
 ***************************/

/*
 *  These are the weekly log entries in flash.  Each weekly log entry
 *  contains 7 daily log entries plus some meta data.  The storage module
 *  rotates through each weekly log, storing the water stats for each day
 *  of the week.  The day's water stats are stored at the end of each
 *  day (midnight of the following day).
 */
#pragma DATA_SECTION(week1Log, ".week1Data")
const weeklyLog_t week1Log;
#pragma DATA_SECTION(week2Log, ".week2Data")
const weeklyLog_t week2Log;
#pragma DATA_SECTION(week3Log, ".week3Data")
const weeklyLog_t week3Log;
#pragma DATA_SECTION(week4Log, ".week4Data")
const weeklyLog_t week4Log;
#pragma DATA_SECTION(week5Log, ".week5Data")
const weeklyLog_t week5Log;

// Force this table to be located in the .text area
#pragma DATA_SECTION(weeklyLogAddrTable, ".text")
static const weeklyLog_t *weeklyLogAddrTable[] = {
    &week1Log,
    &week2Log,
    &week3Log,
    &week4Log,
    &week5Log,
};

/**
 * \var stData 
 * \brief Declare the module data container 
 */
storageData_t stData;

/********************* 
 * Module Prototypes
 *********************/

static bool doesAlignTimeMatch(void);
static void recordLastMinute(void);
static void recordLastHour(void);
static void recordLastDay(void);
static void writeStatsToDailyLog(void);
static weeklyLog_t* getWeeklyLogAddr(uint8_t weeklyLogNum);
static dailyLog_t* getDailyLogAddr(uint8_t weeklyLogNum, uint8_t dayOfTheWeek);
static msgHeader_t* getDailyHeaderAddr(uint8_t weeklyLogNum, uint8_t dayOfTheWeek);
static dailyPacket_t* getDailyPacketAddr(uint8_t weeklyLogNum, uint8_t dayOfTheWeek);
static uint8_t getNextWeeklyLogNum(uint8_t weeklyLogNum);
static void eraseWeeklyLog(uint8_t weeklyLogNum);
static void prepareNextWeeklyLog(void);
static void prepareDailyLog(void);
static void markDailyLogAsReady(uint8_t dayOfTheWeek, uint8_t weeklyLogNum);
static bool isDailyLogReady(uint8_t dayOfTheWeek, uint8_t weeklyLogNum);
static void markDailyLogAsTransmitted(uint8_t dayOfTheWeek, uint8_t weeklyLogNum);
static bool wasDailyLogTransmitted(uint8_t dayOfTheWeek, uint8_t weeklyLogNum);
static void checkAndTransmitMonthlyCheckin(void);
static void checkAndTransmitDailyLogs(bool overrideTransmissionRate);
static void clearAlignStats(void);

#if (DO_RED_FLAG_PROCESSING != 0)
static bool redFlagProcessing(int16_t dayLiterSum);
#endif

/***************************
 * Module Public Functions
 **************************/

/**
* \brief Call once as system startup to initialize the storage 
*        module.
* \ingroup PUBLIC_API
*/
void storageMgr_init(void)
{
#ifdef RED_FLAG_TEST
    uint8_t i;
#endif

    memset(&stData, 0, sizeof(storageData_t));
#ifdef RED_FLAG_TEST
    // set the threshold to start for testing purposes
    for (i = 0; i < TOTAL_DAYS_IN_A_WEEK; i++) 
	    stData.redFlagThreshTable[1] = 240;

    // we don't have 28 days for testing
    stData.redFlagMapDay = RED_FLAG_TOTAL_MAPPING_DAYS;
    stData.redFlagDataFullyPopulated = 1;
#endif

    // Erase flash for all the weekly data logs
    storageMgr_resetWeeklyLogs();

    // Set default transmission rate
    stData.transmissionRateInDays = STORAGE_TRANSMISSION_RATE_DEFAULT;
}

/**
* \brief This is the flash storage manager. It is called from 
*        the main processing loop.
* \ingroup EXEC_ROUTINE
*/
void storageMgr_exec(uint16_t currentFlowRateInMLPerSec, uint8_t time_elapsed)
{

    // If we are waiting for an alignment event to occur, see if there
    // is a match (GMT time == alignment time).
    if (stData.alignStorageFlag == true)
    {
        // Decrement our safety down counter.
        // We use a safety down-counter to make sure that we don't wait more
        // than 24 hours for the alignment event to occur.
        stData.alignSafetyCheckInSec -= time_elapsed;
        if (doesAlignTimeMatch() || (stData.alignSafetyCheckInSec < 0))
        {
            // If the current time is equal to the storage offset,
            // then zero storage time and clear storage memory
            stData.alignStorageFlag = false;
            clearAlignStats();
            storageMgr_resetWeeklyLogs();
            // Write daily log header for today if activated
            if (stData.daysActivated)
            {
                prepareDailyLog();
            }
        }
        // Don't start storing any data until we are officially aligned.
#ifdef WATER_DEBUG
        debug_message("*ALIGN*");
#endif
        return;
    }

    // Update the flow rate per minute running sum (in milliliters)
    stData.minuteMilliliterSum += currentFlowRateInMLPerSec;

    // Increment the unit of time, record the amount of water, reset the previous unit of time

    // Note: this function is called every two seconds by the main loop.
    // unless we were in sleep mode
    stData.storageTime_seconds += time_elapsed;

    if (stData.storageTime_seconds >= TOTAL_SECONDS_IN_A_MINUTE)
    {
        // Update time
        stData.storageTime_minutes++;
        stData.storageTime_seconds -= TOTAL_SECONDS_IN_A_MINUTE;

        // Record data
        recordLastMinute();
    }
#ifdef WATER_DEBUG
    // signal a wakeup if it is not on a minute boundary
    else if (time_elapsed > 2)
    {
        timePacket_t NowTime;
        uint32_t sys_time = getSecondsSinceBoot();

        getBinTime(&NowTime);
        debug_RTC_time(&NowTime,'W',&stData, sys_time);
    }
#endif
    if (stData.storageTime_minutes >= TOTAL_MINUTES_IN_A_HOUR)
    {
        // Update time
        stData.storageTime_hours++;
        stData.storageTime_minutes -= TOTAL_MINUTES_IN_A_HOUR;

        // Record data
        recordLastHour();
#ifdef SEND_DEBUG_TIME_DATA
        sysExecData.sendTimeStamp = true;
#endif
    }
    if (stData.storageTime_hours >= TOTAL_HOURS_IN_A_DAY)
    {
        // Record data
        // The recordLastDay does a number of house-keeping chores:
        // (1) If the unit is activated, it stores today's water log
        // states to flash.
        // (2) It potentially starts the transmission of
        // the daily water log message.
        // (3) It will also potentially activate the Sensor
        // and starts the Activate message transmission.
        recordLastDay();
        // Update Time
        stData.storageTime_dayOfWeek++;
        stData.storageTime_hours -= TOTAL_HOURS_IN_A_DAY;

        // Prepare data storage for next day
        if (stData.storageTime_dayOfWeek < TOTAL_DAYS_IN_A_WEEK)
        {
            // Write daily log header for day if activated
            if (stData.daysActivated)
            {
                prepareDailyLog();
            }
        }
    }
    if (stData.storageTime_dayOfWeek >= TOTAL_DAYS_IN_A_WEEK)
    {
        // Update Time
        stData.storageTime_dayOfWeek = 0;
        stData.storageTime_week++;
        // Prepare data storage for next week and day
        prepareNextWeeklyLog();
        // Write daily log header for day if activated
        if (stData.daysActivated)
        {
            prepareDailyLog();
        }
        // Check if its time to send a monthly checkin message.
        // We only send the message if we are not activated and four
        // weeks have passed.
        checkAndTransmitMonthlyCheckin();
    }
}

/**
* \brief Save the GMT time values that the storage clock 
*        midnight should be aligned with. These will be compared
*        to current GMT time for a match.  All storage
*        operations are stopped until the GMT match time occurs.
* \ingroup PUBLIC_API
* 
* @param alignSecond  GMT storage clock alignment second value 
* @param alignMinute  GMT storage clock alignment minute value 
* @param alignHour24  GMT storage clock alignment hour value 
*/
void storageMgr_setStorageAlignmentTime(uint8_t alignSecond, uint8_t alignMinute, uint8_t alignHour24)
{

    bool alignTimeIsValid = true;

    stData.alignSecond = alignSecond;
    stData.alignMinute = alignMinute;
    stData.alignHour24 = alignHour24;

    // Validate for legal values.
    if (alignSecond > 59)
    {
        alignTimeIsValid = false;
    }
    else if (alignMinute > 59)
    {
        alignTimeIsValid = false;
    }
    else if (alignHour24 > 23)
    {
        alignTimeIsValid = false;
    }

    if (alignTimeIsValid)
    {
        // Set the flag that we are waiting for an alignment event
        stData.alignStorageFlag = true;
        clearAlignStats();

        // We create a safety down-counter in seconds to make sure that we don't
        // miss the alignment event.  The alignment event is currently
        // dependent on matching the alignment time against the unit's GMT
        // clock.  The two must match exactly for the alignment to occur.  If
        // for some reason the unit is busy and the check is not performed at
        // that exact time, then the unit will never get out of the alignment
        // waiting phase. The safety down-counter will catch that situation.

        // Set safety alignment check time counter
        stData.alignSafetyCheckInSec = ((sys_tick_t)SECONDS_PER_DAY);
    }
}

/**
* \brief Override the unit activation.  Either enable or 
*        disable.
* \ingroup PUBLIC_API
*
* @param flag set true or false 
*/
void storageMgr_overrideUnitActivation(bool flag)
{
    if ((flag == true) && (stData.daysActivated == 0))
    {
        stData.daysActivated = 1;
    }
    else if (flag == false)
    {
        stData.daysActivated = 0;
    }
}

/**
* \brief Return the number of days the unit has been activated.
* \ingroup PUBLIC_API
* 
* @return uint16_t Number of days unit has been activated.
*/
uint16_t storageMgr_getDaysActivated(void)
{
    return (stData.daysActivated);
}

/**
* \brief Clear the redFlag in the records
* \ingroup PUBLIC_API
*/
void storageMgr_resetRedFlag(void)
{
    stData.redFlagCondition = false;
}

/**
* \brief Return the redFlag condition
* 
* @return bool True if red flag condition is set. 
*/
bool storageMgr_getRedFlagConditionStatus(void)
{
    return (stData.redFlagCondition);
}

/**
* \brief Clear the redFlag and redFlag map in the records
* \ingroup PUBLIC_API
*/
void storageMgr_resetRedFlagAndMap(void)
{
    stData.redFlagCondition = false;
    stData.redFlagDataFullyPopulated = false;
    stData.redFlagMapDay = 0;
    stData.redFlagDayCount = 0;
    // Clear the thresh table containing the daily threshold mapping
    memset(stData.redFlagThreshTable, 0, sizeof(stData.redFlagThreshTable));
}

/**
* \brief Resets flash for all weekly logs.  This erases all 
*        weekly log containers and resets the current weekly log
*        number.
* \ingroup PUBLIC_API
*/
void storageMgr_resetWeeklyLogs(void)
{
    int i;

    stData.curWeeklyLogNum = 0;
    for (i = 0; i < TOTAL_WEEKLY_LOGS; i++)
    {
        eraseWeeklyLog(i);
    }
    return;
}

/**
 * \brief This function is used to identify the next daily water
 *        log that is ready to transmit.  If there is a log
 *        ready for transmit, the pointer and size are returned.
 *        If no daily log is ready for transmit, the function
 *        returns 0.  If a daily log pointer is returned, then
 *        that daily log is marked as being transmitted in the
 *        weekly info meta data.
 * \brief To Determine if a daily log is ready or has been 
 *        transmitted, it looks at the weekly info meta data
 *        associated with each weekly log.  Status bits are
 *        maintained in the meta data area to mark if a daily
 *        log is ready for transmit and if it has already been
 *        transmitted.
 * \brief The function will sequentially search all the daily 
 *        logs of every weekly log from oldest to newest looking
 *        for any daily logs that are marked as ready but not
 *        yet transmitted.
 *  
 * \ingroup PUBLIC_API
 * 
 * \param dataPP Pointer to a pointer that is filled in with the
 *               address of the daily log.
 * 
 * \return uint16_t Size of the daily log to send, otherwise set
 *         to zero if no daily log is ready to transmit
 */
uint16_t storageMgr_getNextDailyLogToTransmit(uint8_t **dataPP)
{
    uint8_t i;
    uint16_t length = 0;
    bool allDailyLogsTransmitted = false;

    do
    {
        // Loop for each day of the week until a daily log is found
        // that is ready and has not been transmitted.
        for (i = 0; i < TOTAL_DAYS_IN_A_WEEK; i++)
        {
            // Get the flags to check if daily log is ready
            // and has not yet been transmitted.
            bool logReady = isDailyLogReady(i, stData.curTxWeek);
            bool wasTransmitted = wasDailyLogTransmitted(i, stData.curTxWeek);

            if (logReady && !wasTransmitted)
            {
                // Get the address of the daily log
                dailyPacket_t *dpP = getDailyPacketAddr(stData.curTxWeek, i);

                *dataPP = (uint8_t *)dpP;
                length = sizeof(dailyPacket_t);
                // Mark this daily log as being transmitted
                markDailyLogAsTransmitted(i, stData.curTxWeek);
                break;
            }
        }

        // Check if we transmitted all days of current transmit week.
        // If so, move to following week.
        if (i == TOTAL_DAYS_IN_A_WEEK)
        {
            stData.curTxWeek = getNextWeeklyLogNum(stData.curTxWeek);
            // If we have wrapped around all weekly logs, then halt transmitting.
            // We assume all daily logs that were marked as ready have been
            // transmitted.
            if (stData.curTxWeek == stData.startTxWeek)
            {
                length = 0;
                allDailyLogsTransmitted = true;
            }
        }
    } while ((length == 0) && !allDailyLogsTransmitted);

    // Perform a safety check to makes sure we are not stuck transmitting
    // a daily log over and over.  We should never transmit more than the
    // total number of daily logs stored.
    if (length != 0)
    {
        if (stData.totalDailyLogsTransmitted < (TOTAL_DAYS_IN_A_WEEK * TOTAL_WEEKLY_LOGS))
        {
            stData.totalDailyLogsTransmitted++;
            // Mark flag to indicate that at least one daily log has been sent
            stData.haveSentDailyLogs = true;
        }
        else
        {
            length = 0;
        }
    }

    return (length);
}

/**
* \brief Send debug information to the uart.  
* \ingroup PUBLIC_API
*/
void storageMgr_sendDebugDataToUart(void)
{
    // Get the shared buffer (we borrow the ota buffer)
    uint8_t *payloadP = modemMgr_getSharedBuffer();
    // Add structure size plus two for start byte and payload msg ID
    uint8_t payloadSize = sizeof(storageData_t) + 2;

    // Add payload start byte
    payloadP[0] = 0x1;
    // Add payload msg ID
    payloadP[1] = MSG_TYPE_DEBUG_STORAGE_INFO;
    // Add debug data
    memcpy(&payloadP[2], &stData, sizeof(storageData_t));
    // Output debug information
    dbgMsgMgr_sendDebugMsg(MSG_TYPE_DEBUG_STORAGE_INFO, payloadP, payloadSize);
}

/**
 *  \brief Set how often to transmit the daily logs (in days).
 *         We currently limit the max rate to 4 weeks worth of
 *         data even though there is storage allocated for 5
 *         weeks of daily logs. That way when the transmission
 *         rate is set to max (4 weeks x 7 days = 28 days) the
 *         current week that is collecting data is not one of
 *         the weekly logs that has to be transmitted. It makes
 *         things simpler by not having to worry about the logic
 *         for erasing and preparing the current weekly log
 *         which would contain daily logs that need to be
 *         transmitted first. 
 */
void storageMgr_setTransmissionRate(uint8_t transmissionRateInDays)
{
    uint8_t maxAllowedDays = (TOTAL_DAYS_IN_A_WEEK * (TOTAL_WEEKLY_LOGS - 1));

    stData.transmissionRateInDays = transmissionRateInDays;
    if ((stData.transmissionRateInDays < 1) || (stData.transmissionRateInDays > maxAllowedDays))
    {
        stData.transmissionRateInDays = 1;
    }
}

/**
* \brief Read the storage time parameters.
* 
* @param bufP  Pointer to buffer to store the data.
* 
* @return uint8_t Returns the number of data bytes stored into 
*         the buffer (9 bytes).
*/
uint8_t storageMgr_getStorageClockInfo(uint8_t *bufP)
{
    *bufP++ = stData.storageTime_seconds;                  /**< Current storage time - sec  */
    *bufP++ = stData.storageTime_minutes;                  /**< Current storage time - min  */
    *bufP++ = stData.storageTime_hours;                    /**< Current storage time - hour */
    *bufP++ = stData.storageTime_dayOfWeek;                /**< Current storage time - day  */
    *bufP++ = stData.storageTime_week;                     /**< Current storage time - week */
    *bufP++ = stData.alignStorageFlag;                     /**< True if time to align storage time */
    *bufP++ = stData.alignSecond;                          /**< Time to align at - sec */
    *bufP++ = stData.alignMinute;                          /**< Time to align at - min */
    *bufP++ = stData.alignHour24;                          /**< Time to align at - hour */
    return (9);
}

/**
* \brief Get the storage clock current hour value.
* 
* @return uint8_t Returns 0 - 23. 
*/
uint8_t storageMgr_getStorageClockHour(void)
{
    return (stData.storageTime_hours);
}

/**
* \brief Get the storage clock current minute value.
* 
* @return uint8_t Returns 0 - 59. 
*/
uint8_t storageMgr_getStorageClockMinute(void)
{
    return (stData.storageTime_minutes);
}

/**
* \brief Initialize the header portion of an outgoing message.
* 
* @param dataPtr  The buffer to store the header data
* @param payloadMsgId The outgoing message type
* 
* @return uint8_t Returns the length of the header added to the 
*         buffer.
*/
uint8_t storageMgr_prepareMsgHeader(uint8_t *dataPtr, uint8_t payloadMsgId)
{

    // The daily header structure is defined so that it will be exactly 16
    // bytes. The daily header is used to start all messages sent to the cloud.
    msgHeader_t *msgHeaderP = (msgHeader_t *)dataPtr;
    timePacket_t tp;

    // Get current time
    getBinTime(&tp);

    // Add Payload start byte
    msgHeaderP->payloadStartByte = 0x1;
    // Add Payload Message ID
    msgHeaderP->payloadMsgId = payloadMsgId;
    // Add Product ID
    msgHeaderP->productId = AFRIDEV2_PRODUCT_ID;
    // Add Time
    msgHeaderP->GMTsecond = tp.second;
    msgHeaderP->GMTminute = tp.minute;
    msgHeaderP->GMThour = tp.hour24;
    msgHeaderP->GMTday = tp.day;
    msgHeaderP->GMTmonth = tp.month;
    msgHeaderP->GMTyear = tp.year;
    // Add FW Version
    msgHeaderP->fwMajor = FW_VERSION_MAJOR;
    msgHeaderP->fwMinor = FW_VERSION_MINOR;
    // Add Days Activated
    msgHeaderP->daysActivatedMsb = stData.daysActivated >> 8;
    msgHeaderP->daysActivatedLsb = stData.daysActivated & 0xFF;
    // Add storage week
    msgHeaderP->storageWeek = stData.storageTime_week;
    // Add storage Day of the week
    msgHeaderP->storageDay = stData.storageTime_dayOfWeek;
    // Just set to an easy byte to recognize byte for now.
    msgHeaderP->reserve1 = 0xA5;
    return (sizeof(packetHeader_t));
}

/**
* \brief Build the Monthly Check-In message for transmission. 
*        The shared buffer is used to hold the message. The
*        message consists of only the standard msg head.
* 
* @param payloadPP Pointer to fill in with the address of the 
*                  message to send.
* 
* @return uint16_t Length of the message in bytes.
*/
uint16_t storageMgr_getMonthlyCheckinMessage(uint8_t **payloadPP)
{
    // Get the shared buffer (we borrow the ota buffer)
    uint8_t *payloadP = modemMgr_getSharedBuffer();
    // Fill in the buffer with the standard message header
    uint8_t payloadSize = storageMgr_prepareMsgHeader(payloadP, MSG_TYPE_CHECKIN);

    // Assign pointer
    *payloadPP = payloadP;
    // return payload size
    return (payloadSize);
}

/**
* \brief Build the Activated message for transmission. The 
*        shared buffer is used to hold the message. The standard
*        msg header is added first followed by the day liter
*        sum.
* 
* @param payloadPP Pointer to fill in with the address of the 
*                  message to send.
* 
* @return uint16_t Length of the message in bytes.
*/
uint16_t storageMgr_getActivatedMessage(uint8_t **payloadPP)
{
    uint16_t dayLiterSum = stData.activatedLiterSum;
    // Get the shared buffer (we borrow the ota buffer)
    uint8_t *payloadP = modemMgr_getSharedBuffer();
    // Fill in the buffer with the standard message header
    uint8_t payloadSize = storageMgr_prepareMsgHeader(payloadP, MSG_TYPE_ACTIVATED);

    // Add total liters for the day
    payloadP[payloadSize++] = dayLiterSum >> 8;
    payloadP[payloadSize++] = dayLiterSum & 0xFF;
    // Assign pointer
    *payloadPP = payloadP;
    // return payload size
    return (payloadSize);
}

/*************************
 * Module Private Functions
 ************************/

/**
* \brief This function determines if its time to transmit the 
*        daily logs.  At a maximum, we will send up to
*        WEEKLY_LOG_NUM_MAX total weeks worth of data.
* 
* @param overrideTransmissionRate  If set to true, then daily 
*                                  log transmission will be
*                                  initiated regardless of the
*                                  transmissionRate.
*/
static void checkAndTransmitDailyLogs(bool overrideTransmissionRate)
{
    // Only consider transmitting data if we are activated
    if (stData.daysActivated)
    {
        bool transmissionRateMet = false;

        // Increment days since last transmit
        stData.daysSinceLastTransmission++;

        // Check if we reached transmit rate in days
        if (stData.daysSinceLastTransmission >= stData.transmissionRateInDays)
        {
            // Reset counter
            stData.daysSinceLastTransmission = 0;
            // Set flag
            transmissionRateMet = true;
        }

        if (transmissionRateMet || overrideTransmissionRate)
        {
            // Its time to transmit the accumulated daily logs
            // Start with the oldest week, which is the next weekly log from
            // the current week.  We will march through all the weekly logs
            // looking for any daily logs that are marked as ready but have
            // not been transmitted.
            stData.startTxWeek = getNextWeeklyLogNum(stData.curWeeklyLogNum);
            stData.curTxWeek = stData.startTxWeek;
            stData.totalDailyLogsTransmitted = 0;
            // Schedule transmitting all the daily water logs that are ready.
            msgSched_scheduleDailyWaterLogMessage();
        }
    }
}

/**
* \brief This function determines if its time to transmit a
*        monthly check in message.  If we are not activated yet
*        or have not transmitted a daily log in the last four
*        weeks, then send a monthly check-in message.
*/
static void checkAndTransmitMonthlyCheckin(void)
{
    if ((stData.storageTime_week % 4) == 0)
    {
        if (!stData.daysActivated || !stData.haveSentDailyLogs)
        {
            // Schedule the monthly check-in message for transmission
            msgSched_scheduleMonthlyCheckInMessage();
        }
        // Reset flag.  We want to identify whether at least one
        // daily log is sent in the upcoming month.
        stData.haveSentDailyLogs = false;
    }
}

/**
 * \brief Maintain the running sum for the hour.  At the end of 
 *        each minute, add currentMinuteML into the hourly
 *        running sum.
 */
static void recordLastMinute(void)
{
    stData.hourMilliliterSum += stData.minuteMilliliterSum;
#ifdef WATER_DEBUG   // Exclude this section for water debug
    if (!gps_isActive())
    {
        timePacket_t NowTime;

        // debug messages are selected/deselected in debugUart.h
        uint32_t sys_time = getSecondsSinceBoot();

        //debug_logSummary('M', sys_time, stData.storageTime_hours, stData.minuteMilliliterSum, stData.hourMilliliterSum);

        getBinTime(&NowTime);
        debug_RTC_time(&NowTime,'M',&stData, sys_time);
    }
#endif
    stData.minuteMilliliterSum = 0;
}

/**
 * \brief Write the total liters for the current hour into 
 *        flash. Update the running sum for the total daily
 *        liters. The liters for the hour is stored in the
 *        current daily log contained in the current weekly log
 *        section.
 * \note The hourly water volume is stored in the log as Total 
 *     Milliliters/32
 */
static void recordLastHour(void)
{

    // Get pointer to today's dailyLog in flash.
    dailyLog_t *dailyLogsP = getDailyLogAddr(stData.curWeeklyLogNum, stData.storageTime_dayOfWeek);

    // Get address to liters parameter in the dailyLog
    uint8_t *addr = (uint8_t *)&(dailyLogsP->litersPerHour[stData.storageTime_hours]);
    uint16_t litersForThisHour = 0;

    // The hourly water volume is stored in the log as Total Milliliters/32
    litersForThisHour = (stData.hourMilliliterSum >> 5) & 0xffff;

    if (stData.daysActivated)
    {
        // Store the hourly milliliter value to the flash log
        msp430Flash_write_int16(addr, litersForThisHour);
    }

    // Track the total daily milliliters
    stData.dayMilliliterSum += stData.hourMilliliterSum;


#ifdef WATER_DEBUG   // Exclude this section for water debug
    if (!gps_isActive())
    {
        // debug messages are selected/deselected in debugUart.h
        uint32_t sys_time = getSecondsSinceBoot();

        debug_logSummary('H', sys_time, stData.storageTime_hours, litersForThisHour, stData.dayMilliliterSum);
    }
#endif
    // Zero hour sum
    stData.hourMilliliterSum = 0;
}

/**
* \brief This function does a number of house-keeping 
*        chores.
* \li If the Sensor is activated, write the pad statistics to 
*     the flash.
* \li If the Sensor is activated, start transmitting daily water
*     logs if transmission rate in days has been met.
* \li If the Sensor is not activated, clear water stats
* \li If Sensor is not currently activated, but has met the 
*     daily required liters to be activated, then activate the
*     unit and send the Activation message.
*/
static void recordLastDay(void)
{

    uint16_t temp;
    bool newRedFlagCondition = false;

    // Only write to daily log in flash if unit is activated
    if (stData.daysActivated)
    {

        uint16_t dayLiterSum = stData.dayMilliliterSum / 1000;

        // Get pointer to today's dailyLog in flash.
        dailyLog_t *dailyLogsP = getDailyLogAddr(stData.curWeeklyLogNum, stData.storageTime_dayOfWeek);

        // Write data stats to dailyLog
        writeStatsToDailyLog();

        // Mark the current daily log as ready in the weekly log meta data.
        markDailyLogAsReady(stData.storageTime_dayOfWeek, stData.curWeeklyLogNum);

        // Write the total liters for the day to the daily log
        msp430Flash_write_int16((uint8_t *)&(dailyLogsP->totalLiters), dayLiterSum);

#if (DO_RED_FLAG_PROCESSING != 0)
#if (DO_RED_FLAG_TRANSMISSION != 0)
        // A red flag condition can initiate daily log transmission.
        newRedFlagCondition = redFlagProcessing(dayLiterSum);
#else
        // A red flag condition will not initiate daily log transmission.
        redFlagProcessing(dayLiterSum);
#endif
#endif

        // Write the redFlag condition to the daily log
        msp430Flash_write_bytes((uint8_t *)&(dailyLogsP->redFlag), (uint8_t *)&stData.redFlagCondition, FLASH_WRITE_ONE_BYTE);

        // Write the red flag threshold value for today to the daily log
        // Only write the value if the red flag threshold table has been fully populated, otherwise write zero
        temp = 0;
        if (stData.redFlagDataFullyPopulated)
        {
            temp = stData.redFlagThreshTable[stData.storageTime_dayOfWeek];
        }
        msp430Flash_write_int16((uint8_t *)&(dailyLogsP->averageLiters), temp);

        // Check if its time to transmit data
        // Data is only sent if we are activated and we have reached
        // the transmissionRateInDays since the last transmission.
        // A new red flag condition overrides the transmission rate setting.
        checkAndTransmitDailyLogs(newRedFlagCondition);

        // Increment total days activated
        stData.daysActivated++;
    }
    else
    {
        // If we are not writing to the daily log, then we need to clear the flow
        // stats accumulated on a daily basis.  These are collected by
        // the writeStatsToDailyLog function if the unit is activated.
        // But because we are not activated, we need to clear them here instead.

        // Clear all the pad statistics
        waterSense_clearStats();
    }

    // If unit is not activated, check if it should be based on the total liters for the day.
    // If the number of measured daily liters exceeds the threshold, then consider
    // the unit activated.  Unit is considered not-activated if daysActivated is 0.
    if (!stData.daysActivated && (stData.dayMilliliterSum > DAILY_MILLILITERS_ACTIVATION_THRESHOLD))
    {
        // Schedule the activated message for transmission
        msgSched_scheduleActivatedMessage();
        // As part of becoming activated, perform a GPS measurement and send
        // a GPS Location message at the same time the Activated message is sent
        msgSched_scheduleGpsMeasurement();
        // unit is now activated
        stData.daysActivated++;
        // Save activated liter sum
        stData.activatedLiterSum = stData.dayMilliliterSum / 1000;
    }

#ifdef WATER_DEBUG   // Exclude this section for water debug
    if (!gps_isActive())
    {
        // debug messages are selected/deselected in debugUart.h
        uint32_t sys_time = getSecondsSinceBoot();
        uint8_t dayOfTheWeek = stData.storageTime_dayOfWeek;

        debug_daySummary('D', sys_time, stData.daysActivated, stData.redFlagDataFullyPopulated, stData.dayMilliliterSum,
                         stData.activatedLiterSum, stData.redFlagThreshTable[dayOfTheWeek], newRedFlagCondition);
    }
#endif

    // Reset the daily based statistics
    stData.dayMilliliterSum = 0;
}

/**
* \brief Fill in the water stats into the daily log.
*/
static void writeStatsToDailyLog(void)
{
    uint8_t *addr;
    uint16_t i = 0;
    uint16_t u16Val;
    uint8_t u8Val;

    // Get pointer to today's dailyLog in flash.
    dailyLog_t *dailyLogsP = getDailyLogAddr(stData.curWeeklyLogNum, stData.storageTime_dayOfWeek);

    // Write per PAD stats to flash
    for (i = 0; i < 6; i++)
    {
        addr = (uint8_t *)&(dailyLogsP->padSubmergedCount[i]);
        u16Val = waterSense_getPadStatsSubmerged((padId_t)i);
        msp430Flash_write_int16(addr, u16Val);
    }
    // Write unknown stat to flash
    addr = (uint8_t *)&(dailyLogsP->unknowns);
    u16Val = waterSense_getPadStatsUnknowns();
    msp430Flash_write_int16(addr, u16Val);

    // Write outOfSpec status to flash
    u8Val = sysExecData.waterDetectStopped;
    msp430Flash_write_bytes((uint8_t *)&(dailyLogsP->outOfSpec), (uint8_t *)&u8Val, FLASH_WRITE_ONE_BYTE);

    // Clear all the pad statistics
    waterSense_clearStats();
}

#if (DO_RED_FLAG_PROCESSING != 0)
/**
* @brief Monitor for a redFlag condition.
* 
* @return bool Returns true if a new red flag condition is 
*         detected.
*/
static bool redFlagProcessing(int16_t dayLiterSum)
{

    bool newRedFlagCondition = false;
    uint32_t temp = 0;

    // check if the Red Flag mapping table is fully populated
    if (stData.redFlagDataFullyPopulated)
    {

        uint8_t dayOfTheWeek = stData.storageTime_dayOfWeek;
        uint16_t redFlagDayThreshValue = stData.redFlagThreshTable[dayOfTheWeek];

        if (stData.redFlagCondition)
        {
            // Identify if existing redFlag condition needs to be cleared
            // The redflag condition is cleared if todays liter total exceeds 75% of threshold
            // Calculate 75% of redflag threshold value
            temp = redFlagDayThreshValue + redFlagDayThreshValue + redFlagDayThreshValue;
            uint16_t threeFourths = (temp >> 2) & 0xffff;

            // If today's dailyLiters value is greater than 3/4 of the threshold value,
            // then clear the redFlag condition.
            if (dayLiterSum > threeFourths)
            {
                // Reset red flag
                storageMgr_resetRedFlag();
            }
        }

        // Re-check redflag condition - as it may have been cleared above
        if (!stData.redFlagCondition)
        {
            // The redflag condition is set if todays liter total falls below 25% of threshold
            // Check if today's daily liters fall below 25% of threshold
            uint16_t quarterValue = redFlagDayThreshValue >> 2;

            if ((dayLiterSum < quarterValue) && (redFlagDayThreshValue > MIN_DAILY_LITERS_TO_SET_REDFLAG_CONDITION))
            {
                // Red flag condition is met
                stData.redFlagCondition = true;
                stData.redFlagDayCount = 1;
                newRedFlagCondition = true;
            }
            else
            {
                // Update the threshold table with a new value based on 75% threshold and 25% today's dailyLiters
                temp = redFlagDayThreshValue + redFlagDayThreshValue + redFlagDayThreshValue + dayLiterSum;
                uint16_t newAverage = 0;

                newAverage = (temp >> 2) & 0xffff;
                stData.redFlagThreshTable[dayOfTheWeek] = newAverage;
            }
        }
    }
    else
    {
        // Add today's daily liters sum to the existing value in today's threshold table entry.
        // We are trying to get an average value of each days water usage during the mapping period.
        // We sum the values for each day over the mapping period, then we will divide by total weeks
        // at the end of the mapping period to get the daily average value.
        stData.redFlagThreshTable[stData.storageTime_dayOfWeek] += dayLiterSum;
        stData.redFlagMapDay++;
        if (stData.redFlagMapDay >= RED_FLAG_TOTAL_MAPPING_DAYS)
        {
            int i;

            // Take the sum for each days entry and divide by the
            // number of weeks that the mapping was performed.
            for (i = 0; i < TOTAL_DAYS_IN_A_WEEK; i++)
            {
                temp = stData.redFlagThreshTable[i];
                temp >>= RED_FLAG_MAPPING_WEEKS_BIT_SHIFT;
                stData.redFlagThreshTable[i] = temp;
            }
            // Fully populated after one week.
            stData.redFlagDataFullyPopulated = true;
        }
    }

    return (newRedFlagCondition);
}
#endif

/**
* \brief Utility routine to check if the alignment time matches 
*        the current GMT time for seconds, minutes and hours.
* \note We currently don't match seconds, as it should not be 
*       needed and adds risk to missing the align time window.
*       We need to add some leeway with minute matching because of
*       the 20 sleep mode and the possible lag of the clock over a day
* 
* @return bool Returns true if a match occurred.
*/
static bool doesAlignTimeMatch(void)
{
    timePacket_t NowTime;
    uint8_t hour_diff;
    uint8_t minute_diff;

    getBinTime(&NowTime);

    // we need to know how many minutes are between the align point and NOW
    // this might wrap around midnight so it is tricky
    if (NowTime.hour24 != stData.alignHour24)
    {
       if (NowTime.hour24 > stData.alignHour24)
          hour_diff = 24 - NowTime.hour24 + stData.alignHour24;
       else
          hour_diff = stData.alignHour24 - NowTime.hour24;
       hour_diff--;
    }
    else
       hour_diff = 0;

    if (!hour_diff && NowTime.minute > stData.alignMinute)
       minute_diff = 60 - NowTime.minute + stData.alignMinute;
    else
       minute_diff = stData.alignMinute - NowTime.minute;

    // if the NOW time is within 5 minutes of the align time it's good
    // the range is so the system will still work even if the time jumps due to
    // 20 second sleep cycles
    return (minute_diff < 5);
}

/**
*  \brief Utility function to get the address from the weekly
*         log number.
* 
* @param weeklyLogNum Weekly log number
* 
* @return weeklyLog_t*  Returns a pointer to the weekly log 
*         section.
*/
static weeklyLog_t* getWeeklyLogAddr(uint8_t weeklyLogNum)
{
    const weeklyLog_t *wlP;

    if (weeklyLogNum < TOTAL_WEEKLY_LOGS)
    {
        wlP = weeklyLogAddrTable[weeklyLogNum];
    }
    else
    {
        sysError();
    }
    return ((weeklyLog_t *)wlP);
}

/**
* \brief Utility function to get the daily log address contained
*        in the weekly log.
* 
* @param weeklyLogNum  Which weekly log container
* @param dayOfTheWeek  Which day of the week.
* 
* @return dailyLog_t* Returns a pointer to the daily log
*/
static dailyLog_t* getDailyLogAddr(uint8_t weeklyLogNum, uint8_t dayOfTheWeek)
{
    weeklyLog_t *wlP = getWeeklyLogAddr(weeklyLogNum);
    dailyLog_t *dailyLogP = &wlP->dailyPackets[dayOfTheWeek].packetData.dailyLog;

    return (dailyLogP);
}

/**
* \brief   Utility function to get the address to the header 
*          portion of a daily packet.
* 
* @param weeklyLogNum Which weekly log container to access
* @param dayOfTheWeek Which day of the week
* 
* @return msgHeader_t*  Pointer to the packet header.
*/
static msgHeader_t* getDailyHeaderAddr(uint8_t weeklyLogNum, uint8_t dayOfTheWeek)
{
    weeklyLog_t *wlP = getWeeklyLogAddr(weeklyLogNum);
    msgHeader_t *msgHeaderP = &wlP->dailyPackets[dayOfTheWeek].packetHeader.msgHeader;

    return (msgHeaderP);
}

/**
* \brief Utility function to get the address to a daily packet.
* 
* @param weeklyLogNum Which weekly log container to access
* @param dayOfTheWeek Which day of the week
* 
* @return msgHeader_t*  Pointer to the packet.
*/
static dailyPacket_t* getDailyPacketAddr(uint8_t weeklyLogNum, uint8_t dayOfTheWeek)
{
    weeklyLog_t *wlP = getWeeklyLogAddr(weeklyLogNum);
    dailyPacket_t *dailyPacketP = &wlP->dailyPackets[dayOfTheWeek];

    return (dailyPacketP);
}

/**
* \brief   Utility function to increment to the next weekly log.
*          Handles rollover condition.
* 
* @param weeklyLogNum Current weekly log number
* 
* @return uint8_t Next sequential weekly log number
*/
static uint8_t getNextWeeklyLogNum(uint8_t weeklyLogNum)
{
    uint8_t nextWeeklyLogNum = weeklyLogNum + 1;

    if (nextWeeklyLogNum >= TOTAL_WEEKLY_LOGS)
    {
        nextWeeklyLogNum = 0;
    }
    return (nextWeeklyLogNum);
}

/**
* \brief Erase the weekly log container (in flash).
* 
* @param weeklyLogNum  The weekly log number.
*/
static void eraseWeeklyLog(uint8_t weeklyLogNum)
{
    uint8_t *addr = (uint8_t *)getWeeklyLogAddr(weeklyLogNum);

    msp430Flash_erase_segment(addr);
    msp430Flash_erase_segment(addr + FLASH_BLOCK_SIZE);
}

/**
* \brief Advance to the next weekly log container (rollover if 
*        required).  Erase the identified weekly log container.
*/
static void prepareNextWeeklyLog(void)
{
    volatile uint8_t curWeeklyLogNum = stData.curWeeklyLogNum;
    volatile uint8_t nextWeeklyLogNum = getNextWeeklyLogNum(curWeeklyLogNum);

    stData.curWeeklyLogNum = nextWeeklyLogNum;
    eraseWeeklyLog(nextWeeklyLogNum);
}

/**
* \brief Update the packet header portion of the daily log based
*        on beginning-of-day info.
*/
static void prepareDailyLog(void)
{
    msgHeader_t *msgHeaderP = getDailyHeaderAddr(stData.curWeeklyLogNum, stData.storageTime_dayOfWeek);
    timePacket_t tp;
    uint8_t temp8;

    // Get current time
    getBinTime(&tp);

    // Payload start byte
    temp8 = 0x1;
    msp430Flash_write_bytes((uint8_t *)&(msgHeaderP->payloadStartByte), &temp8, FLASH_WRITE_ONE_BYTE);

    // Payload Message Type
    temp8 = MSG_TYPE_DAILY_LOG;
    msp430Flash_write_bytes((uint8_t *)&(msgHeaderP->payloadMsgId), &temp8, FLASH_WRITE_ONE_BYTE);

    // Product ID
    temp8 = AFRIDEV2_PRODUCT_ID;
    msp430Flash_write_bytes((uint8_t *)&(msgHeaderP->productId), &temp8, FLASH_WRITE_ONE_BYTE);

    // Time
    temp8 = 0;
    msp430Flash_write_bytes((uint8_t *)&(msgHeaderP->GMTsecond), &temp8, FLASH_WRITE_ONE_BYTE);
    msp430Flash_write_bytes((uint8_t *)&(msgHeaderP->GMTminute), &temp8, FLASH_WRITE_ONE_BYTE);
    msp430Flash_write_bytes((uint8_t *)&(msgHeaderP->GMThour), &temp8, FLASH_WRITE_ONE_BYTE);
    msp430Flash_write_bytes((uint8_t *)&(msgHeaderP->GMTday), &tp.day, FLASH_WRITE_ONE_BYTE);
    msp430Flash_write_bytes((uint8_t *)&(msgHeaderP->GMTmonth), &tp.month, FLASH_WRITE_ONE_BYTE);
    msp430Flash_write_bytes((uint8_t *)&(msgHeaderP->GMTyear), &tp.year, FLASH_WRITE_ONE_BYTE);

    // FW Version
    temp8 = FW_VERSION_MAJOR;
    msp430Flash_write_bytes((uint8_t *)&(msgHeaderP->fwMajor), &temp8, FLASH_WRITE_ONE_BYTE);
    temp8 = FW_VERSION_MINOR;
    msp430Flash_write_bytes((uint8_t *)&(msgHeaderP->fwMinor), &temp8, FLASH_WRITE_ONE_BYTE);

    // Days Activated
    msp430Flash_write_int16((uint8_t *)&(msgHeaderP->daysActivatedMsb), stData.daysActivated);

    // Storage Week Number
    temp8 = stData.storageTime_week;
    msp430Flash_write_bytes((uint8_t *)&(msgHeaderP->storageWeek), &temp8, FLASH_WRITE_ONE_BYTE);

    // Storage day of the week
    temp8 = stData.storageTime_dayOfWeek;
    msp430Flash_write_bytes((uint8_t *)&(msgHeaderP->storageDay), &temp8, FLASH_WRITE_ONE_BYTE);

    // Unused
    temp8 = 0xA5;
    msp430Flash_write_bytes((uint8_t *)&(msgHeaderP->reserve1), &temp8, FLASH_WRITE_ONE_BYTE);
}

/**
* \brief Updates the record for tracking that a daily log is
*        ready for transmit.
* 
* @param dayOfTheWeek The day of the week to record
* @param weeklyLogNum The weekly log container to use
*/
static void markDailyLogAsReady(uint8_t dayOfTheWeek, uint8_t weeklyLogNum)
{
    uint8_t zeroVal = 0;
    weeklyLog_t *wlP = getWeeklyLogAddr(weeklyLogNum);

    if (dayOfTheWeek >= TOTAL_DAYS_IN_A_WEEK)
    {
        return;
    }
    uint8_t *entryP = &(wlP->clearOnReady[dayOfTheWeek]);

    msp430Flash_write_bytes(entryP, &zeroVal, FLASH_WRITE_ONE_BYTE);
}

/**
* \brief Utility function to check if a daily log is ready for
*        transmit.
* 
* @param dayOfTheWeek The day of the week to record
* @param weeklyLogNum The weekly log container to use
*/
static bool isDailyLogReady(uint8_t dayOfTheWeek, uint8_t weeklyLogNum)
{
    weeklyLog_t *wlP = getWeeklyLogAddr(weeklyLogNum);
    bool isReady = !wlP->clearOnReady[dayOfTheWeek];

    return (isReady);
}

/**
* \brief Updates the record for tracking that a daily log has 
*        been transmitted.
* 
* @param dayOfTheWeek The day of the week to record
* @param weeklyLogNum The weekly log container to use
*/
static void markDailyLogAsTransmitted(uint8_t dayOfTheWeek, uint8_t weeklyLogNum)
{
    uint8_t zeroVal = 0;
    weeklyLog_t *wlP = getWeeklyLogAddr(weeklyLogNum);
    uint8_t *entryP = (uint8_t *)&(wlP->clearOnTransmit[dayOfTheWeek]);

    if (dayOfTheWeek >= TOTAL_DAYS_IN_A_WEEK)
    {
        return;
    }
    msp430Flash_write_bytes(entryP, &zeroVal, FLASH_WRITE_ONE_BYTE);
}

/**
* \brief Utility function to check if a daily log has been 
*        transmitted.
* 
* @param dayOfTheWeek The day of the week to record
* @param weeklyLogNum The weekly log container to use
*/
static bool wasDailyLogTransmitted(uint8_t dayOfTheWeek, uint8_t weeklyLogNum)
{
    weeklyLog_t *wlP = getWeeklyLogAddr(weeklyLogNum);

    // If zero, it means we transmitted packet.
    return (wlP->clearOnTransmit[dayOfTheWeek] ? false : true);
}

/**
* \brief clear stats that are reset when the storage clock is 
*        restarted/aligned.
*/
static void clearAlignStats(void)
{
    stData.storageTime_seconds = 0;
    stData.storageTime_minutes = 0;
    stData.storageTime_hours = 0;
    stData.storageTime_dayOfWeek = 0;
    stData.storageTime_week = 0;
    stData.minuteMilliliterSum = 0;
    stData.hourMilliliterSum = 0;
    stData.dayMilliliterSum = 0;
}

#ifdef SEND_DEBUG_TIME_DATA
uint16_t storageMgr_getTimestampMessage(uint8_t **payloadPP)
{
    // Get the shared buffer (we borrow the ota buffer)
    storageTimeStamp_T *ptr = (storageTimeStamp_T *)modemMgr_getSharedBuffer();

    // Fill in the buffer with the standard message header
    storageMgr_prepareMsgHeader(ptr->ph, MSG_TYPE_TIMESTAMP);

    // Return sensor data
    getBinTime(&ptr->tp);
    ptr->storageTime_seconds = stData.storageTime_seconds;
    ptr->storageTime_minutes = stData.storageTime_minutes;
    ptr->storageTime_hours = stData.storageTime_hours;
    ptr->sys_time = getSecondsSinceBoot();
    ptr->unused = 0;

    // Assign pointer
    *payloadPP = (uint8_t*)ptr;

    // return payload size
    return (sizeof(storageTimeStamp_T));
}
#endif
