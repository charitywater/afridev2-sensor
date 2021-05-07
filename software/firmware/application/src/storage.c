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
#include "appAlgo.h"
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
 * \def FULL_DAY_SECONDS
 * \brief For clarity in the code
 */
#define FULL_DAY_SECONDS ((uint32_t)86400)

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
 * \def ML_PER_LITER
 * \brief milliliters per liters conversion
 */
#define ML_PER_LITER                     1000lu

/**
 * \def TIME_SYNC_REQUEST_RATE_DAYS
 * \brief Send a final assembly message at this rate to adjust clock
 */
#define TIME_SYNC_REQUEST_RATE_DAYS      ((uint8_t)28)


/**
 * \def DAYS_PER_MONTH
 * \brief We consider 28 days ~ 1 month
 */
#define DAYS_PER_MONTH      ((uint8_t)28)

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
    uint8_t reserverd;                                     /**< 01, 53    */
    uint16_t errorBits;                                    /**< 02, 54-55 */ //This is a NEW field for algo v 2
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


static uint8_t xDaysSinceLastTimeSync = 0;

/********************* 
 * Module Prototypes
 *********************/
static void recordLastHour(uint8_t hour_to_store);
static void recordLastDay(void);
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

#if (DO_RED_FLAG_PROCESSING != 0)
static bool redFlagProcessing(int16_t dayLiterSum);
#endif

#ifdef SEND_DEBUG_TIME_DATA
       int8_t TimeStamp_LastHour = 0;
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
	    stData.redFlagThreshTable[i] = 240;

    // we don't have 28 days for testing
    stData.redFlagMapDay = RED_FLAG_TOTAL_MAPPING_DAYS;
    stData.redFlagDataFullyPopulated = 1;
#endif

    // Erase flash for all the weekly data logs
    storageMgr_resetWeeklyLogs();

#ifdef SEND_DEBUG_TIME_DATA
    // the system time starts at 0, before the clock setting message is received
    TimeStamp_LastHour = 0;
#endif

    // Set default transmission rate
    stData.transmissionRateInDays = STORAGE_TRANSMISSION_RATE_DEFAULT;
}

/**
* \brief This is the flash storage manager. It is called from 
*        the main processing loop.
* \ingroup EXEC_ROUTINE
*/
void storageMgr_exec(void)
{
    timePacket_t NowTime;
    uint8_t last_hour24 = stData.storageTime_hours;

    // get the latest RTC, and compare to last storage clock
    getBinTime(&NowTime);
    storageMgr_syncStorageTime(NowTime.second, NowTime.minute, NowTime.hour24);

    if (NowTime.hour24 != last_hour24)
    {
        // Record data
        recordLastHour(last_hour24);

        if (NowTime.hour24 == 0)
        {
            // Record data
            // The recordLastDay does a number of house-keeping chores:
            // (1) If the unit is activated, it stores today's water log
            // states to flash.
            // (2) It potentially starts the transmission of
            // the daily water log message.
            // (3) It will also potentially activate the Sensor
            // and starts the Activate message transmission.
            // (4) Adjust time to account for drift accumulated throughout the day due
            // to timer resolution (10 seconds)
            recordLastDay();

            // Update Time
            stData.storageTime_dayOfWeek++;

            // Prepare data storage for next day
            if (stData.storageTime_dayOfWeek < TOTAL_DAYS_IN_A_WEEK)
            {
                // Write daily log header for day if activated
                if (stData.daysActivated)
                {
                    prepareDailyLog();
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

            //Adjust time to account for drift accumulated throughout the day
            all_timers_adjust_time_end_of_day();
        } // end if mignight
    } // end if hour change

#ifdef WATER_DEBUG
    // signal a wakeup if it is not on a minute boundary
    if (time_elapsed > 2)
    {
        uint32_t sys_time = getSecondsSinceBoot();
        getBinTime(&NowTime);
        debug_RTC_time(&NowTime,'W',&stData, sys_time);
    }
#endif
}

/**
* \brief Save the GMT time values into the storage clock.
* 
* \ingroup PUBLIC_API
* 
* @param rtcSecond  GMT storage clock second value 
* @param rtcMinute  GMT storage clock minute value 
* @param rtcHour24  GMT storage clock hour value 
*/
void storageMgr_syncStorageTime(uint8_t rtc_Second, uint8_t rtc_Minute, uint8_t rtc_Hour24)
{  
    // set the storage clock to be the same as the rtc
    stData.storageTime_seconds = rtc_Second;
    stData.storageTime_minutes = rtc_Minute;
    stData.storageTime_hours = rtc_Hour24;
}

/**
* \brief Save the GMT time values into the storage clock and reset storage data.
*
* \ingroup PUBLIC_API
*
* @param rtcSecond  GMT storage clock second value
* @param rtcMinute  GMT storage clock minute value
* @param rtcHour24  GMT storage clock hour value
*/
void storageMgr_setStorageTime(uint8_t rtc_Second, uint8_t rtc_Minute, uint8_t rtc_Hour24)
{
    // set the storage clock to be the same as the rtc
    stData.storageTime_seconds = rtc_Second;
    stData.storageTime_minutes = rtc_Minute;
    stData.storageTime_hours = rtc_Hour24;
    stData.storageTime_dayOfWeek = 0;
    stData.storageTime_week = 0;
    stData.minuteMilliliterSum = 0;
    stData.hourMilliliterSum = 0;
    stData.dayMilliliterSum = 0;
#ifdef SEND_DEBUG_TIME_DATA
    TimeStamp_LastHour = rtc_Hour24;
#endif
}

/**
* \brief Save the GMT time values into the storage clock.
* 
* \ingroup PUBLIC_API
* 
* @param rtcSecond  GMT storage clock second value 
* @param rtcMinute  GMT storage clock minute value 
* @param rtcHour24  GMT storage clock hour value 
*/
void storageMgr_adjustStorageTime(uint8_t hours24Offset)
{   
    uint8_t hours24;
    timePacket_t tp;  
    int8_t hoursdiff;
    
    // get rtc
    getBinTime(&tp);
    
    hours24 = tp.hour24 + hours24Offset;
    
    // handle wrap around of hours
    if (hours24 > 23)
    {
        hours24 -= 24;       
        // figure out what the TZ diff is
        if (hours24 > tp.hour24) 
            hoursdiff = (tp.hour24 + 24 - hours24Offset)*-1;
        else
            hoursdiff = (hours24 + 24 - tp.hour24);         
    }
    else
    {
        // the TZ diff is positive
        hoursdiff = hours24Offset;
    }
    
    if (hoursdiff < 0 && hours24 > tp.hour24)
    {
        // we are going back a day
        if (!stData.storageTime_dayOfWeek)
           stData.storageTime_dayOfWeek = 6;
        else
           stData.storageTime_dayOfWeek--;  
    }
    else if (hoursdiff > 0 && tp.hour24 > hours24)
    {
        // we are going ahead a day
        if (stData.storageTime_dayOfWeek >= 7)
           stData.storageTime_dayOfWeek = 0;
        else
           stData.storageTime_dayOfWeek++;  
    }
    // change the storage clock to be time zone adjusted
    stData.storageTime_hours = hours24;
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
    *bufP++ = false;                                       /**< True if time to align storage time */
    *bufP++ = 0;                                           /**< Time to align at - sec, not needed */
    *bufP++ = 0;                                           /**< Time to align at - min, not needed */
    *bufP++ = 0;                                           /**< Time to align at - hour, not needed */
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
* \brief Build the Final Assembly message for transmission.
*        The shared buffer is used to hold the message. The
*        message consists of only the standard msg head.
*
* @param payloadPP Pointer to fill in with the address of the
*                  message to send.
*
* @return uint16_t Length of the message in bytes.
*/
uint16_t storageMgr_getFinalAssemblyMessage(uint8_t **payloadPP)
{
    // Get the shared buffer (we borrow the ota buffer)
    uint8_t *payloadP = modemMgr_getSharedBuffer();
    // Fill in the buffer with the standard message header
    uint8_t payloadSize = storageMgr_prepareMsgHeader(payloadP, MSG_TYPE_FINAL_ASSEMBLY);

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
*        Also send a final assembly message if it has been 4 weeks
*        since we got a time update
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

        // Increment days since last time sync
        xDaysSinceLastTimeSync++;

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
            // If it has been more than 4 weeks since we got a time sync
            // Then send the final assembly message to start the time
            // Update process. This should be done before sending the
            // water data so that we receive the response before ending
            // the cell connection.
            if ( xDaysSinceLastTimeSync >= TIME_SYNC_REQUEST_RATE_DAYS )
            {
                xDaysSinceLastTimeSync = 0;
                msgSched_scheduleFinalAssemblyMessage();
            }

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
            //Initiate a clock update
            msgSched_scheduleFinalAssemblyMessage();

            // Schedule the monthly check-in message for transmission
            msgSched_scheduleMonthlyCheckInMessage();
        }
        // Reset flag.  We want to identify whether at least one
        // daily log is sent in the upcoming month.
        stData.haveSentDailyLogs = false;
    }
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
static void recordLastHour(uint8_t hour_to_store)
{
    // Get pointer to today's dailyLog in flash.
    dailyLog_t *dailyLogsP = getDailyLogAddr(stData.curWeeklyLogNum, stData.storageTime_dayOfWeek);

    // Get address to liters parameter in the dailyLog
    uint8_t *addr = (uint8_t *)&(dailyLogsP->litersPerHour[hour_to_store]);

    //mL/32
    uint32_t tempmLForCloudMsg = 0;
    uint16_t mLForCloudMsgThisHr = 0;

    // The algorithm reports water in terms of mL:
    stData.hourMilliliterSum = APP_ALGO_getHourlyWaterVolume_ml();

    //divide by 32 - Max value will be 2097 liters (2097000mL)
    //(65535mL*32)/1000mL/L = 2097 liters
    tempmLForCloudMsg = (stData.hourMilliliterSum/32);

    //check for overflow
    if ( tempmLForCloudMsg > (uint16_t)0xFFFF )
    {
        //subtract one - values of 0xFFFF get written to 0 later on in the code
        mLForCloudMsgThisHr = ((uint16_t)0xFFFF - 1);
    }
    else
    {
        mLForCloudMsgThisHr = (uint16_t)tempmLForCloudMsg;
    }

    if (stData.daysActivated)
    {
        // Store the hourly milliliter value to the flash log
        msp430Flash_write_int16(addr, mLForCloudMsgThisHr);
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
    uint8_t log_hour;
    bool newRedFlagCondition = false;
    uint16_t errorBitsForThisDay = 0;
    uint8_t *errBitAddr;
    uint32_t errorBits = 0;
    static uint8_t xDaysWithNoRtcTimeThisMonth = 0;

    //first get error bits to see if we have a timestamp
    errorBits = sysExec_getErrorBits();
    
    // Only write to daily log in flash if unit is activated
    if (stData.daysActivated)
    {
        uint16_t dayLiterSum = stData.dayMilliliterSum / ML_PER_LITER;

        // Get pointer to today's dailyLog in flash.
        dailyLog_t *dailyLogsP = getDailyLogAddr(stData.curWeeklyLogNum, stData.storageTime_dayOfWeek);
        
        // zero out leading liter counts that were not filled in because of a partial day of data
        for (log_hour = 0; log_hour < 24; log_hour++){
            if (dailyLogsP->litersPerHour[log_hour] == 0xFFFF) {
                msp430Flash_write_int16((uint8_t *)&(dailyLogsP->litersPerHour[log_hour]), 0);
                WATCHDOG_TICKLE();
            }
        }

        //update with any error bits recorded during this day
        errorBitsForThisDay = APP_ALGO_reportAlgoErrors();

        //now OR the bits with the system level error bits
        errorBitsForThisDay |= sysExec_getErrorBits();

        // Write error bit value to flash
        errBitAddr = (uint8_t *)&(dailyLogsP->errorBits);
        msp430Flash_write_int16(errBitAddr, errorBitsForThisDay);

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
        //reset error bits for this day, calling the function will clear error bits
        APP_ALGO_reportAlgoErrors();
    }

    // If unit is not activated, check if it should be based on the total liters for the day.
    // If the number of measured daily liters exceeds the threshold, then consider
    // the unit activated.  Unit is considered not-activated if daysActivated is 0.
    // We also need to have a timestamp in order to activate! If we dont have a timestamp, request one
    if ( (errorBits & NO_RTC_TIME) == NO_RTC_TIME )
    {
        //Approach:
        //Send final assembly message once/day for 7 days, then wait 3 weeks
        //repeat
        if ( xDaysWithNoRtcTimeThisMonth < TOTAL_DAYS_IN_A_WEEK )
        {
            msgSched_scheduleFinalAssemblyMessage();
            msgSched_scheduleMonthlyCheckInMessage();
        }

        //increment days counter
        xDaysWithNoRtcTimeThisMonth++;

        //if we've reached 28 days, the next 7 days we will try to get a time once/day
        if ( xDaysWithNoRtcTimeThisMonth >= DAYS_PER_MONTH )
        {
            xDaysWithNoRtcTimeThisMonth = 0;
        }
    }
    else if (!stData.daysActivated && (stData.dayMilliliterSum > DAILY_MILLILITERS_ACTIVATION_THRESHOLD))
    {
        // Schedule the activated message for transmission
        msgSched_scheduleActivatedMessage();
        // As part of becoming activated, perform a GPS measurement and send
        // a GPS Location message at the same time the Activated message is sent
        msgSched_scheduleGpsMeasurement();
        // unit is now activated
        stData.daysActivated++;
        // Save activated liter sum
        stData.activatedLiterSum = stData.dayMilliliterSum / ML_PER_LITER;

        //reset time sync day counter to match days activated
        xDaysSinceLastTimeSync = 1;
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
#if defined(REDFLAG_VERSION) && REDFLAG_VERSION==2
            // this test will exclude when the dayLiterSum is zero and the quarterValue is zero
            if( dayLiterSum == 0)
            {
#elif defined(REDFLAG_VERSION) && REDFLAG_VERSION==1
            // The redflag condition is set if todays liter total falls below 25% of threshold
            // Check if today's daily liters fall below 25% of threshold
            uint16_t quarterValue = redFlagDayThreshValue >> 2;

            if( dayLiterSum < quarterValue)
            {
#else
            if(0)
            {
#endif
#if defined(REDFLAG_VERSION) && REDFLAG_VERSION==2
                // the client requests to trigger a red flag only when the current water measurement is 0,
                // excluding when this 4-week average equals 0
                if  (redFlagDayThreshValue != 0)
                {
                    // Red flag condition is met
                    stData.redFlagCondition = true;
                    stData.redFlagDayCount = 1;
                    newRedFlagCondition = true;
                }
#elif defined(REDFLAG_VERSION) && REDFLAG_VERSION==1
                // this is the original Red Flag criteria from the original requirements
                if (redFlagDayThreshValue > MIN_DAILY_LITERS_TO_SET_REDFLAG_CONDITION)
                {
                    // Red flag condition is met
                    stData.redFlagCondition = true;
                    stData.redFlagDayCount = 1;
                    newRedFlagCondition = true;
                }
#endif
            } // end if dailyTotal is less than 25% of historical data
            if (!newRedFlagCondition)
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
