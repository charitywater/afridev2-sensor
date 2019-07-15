/** 
 * @file outpour.h
 * \n Source File
 * \n AfridevV2 MSP430 Firmware
 * 
 * \brief MSP430 sleep control and support routines
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "msp430.h"
#include "msp430g2955.h"
#include "RTC_Calendar.h"
#include "modemMsg.h"
#include "waterSense.h"


/******************************************************************************/
/** Water Detection MACROS **/

/**
 * \def NEW_PROCESSOR_IN_USE
 * For the AfridevV1 configuration always define this.
 */
#define NEW_PROCESSOR_IN_USE

/**
 * \def WATERDETECT_READ_WATER_LEVEL_NORMAL
 * Used to identify that the sensor is mounted in a well in its 
 * normal orientation.  For manufacturing testing, this should be disabled 
 * when WATER_DEBUG is defined 
 */
#ifdef WATER_DEBUG
//#define WATERDETECT_READ_WATER_LEVEL_NORMAL
//#define NO_GPS_TEST 1
#define SLEEP_DEBUG 1
#else
#define WATERDETECT_READ_WATER_LEVEL_NORMAL
//#define SLEEP_DEBUG 1
//#define DEBUG_BATTERY_TEST 1
//#define DEBUG_SEND_SENSOR_DATA_NOW 1
//#define DEBUG_DAILY_WATER_REPORTS 1
//#define RED_FLAG_TEST
#endif
/**
 * \def TICKS_PER_TREND
 * Controls how many capacitance measurements to take before 
 * running the water detection algorithm.  One capacitance 
 * measurement is taken on each iteration of the main exec loop. 
 * The main exec loop runs at a rate of (TICKS_PER_TREND / 
 * SECONDS_PER_TREND) hertz (i.e. 2 Hz). 
 */
#define TICKS_PER_TREND 4

/**
 * \def SECONDS_PER_TREND
 * Controls how often (in seconds) to execute the water 
 * detection algorithm. The storage and modem communication 
 * tasks also run at this periodic rate within the main exec 
 * loop. 
 */
#define SECONDS_PER_TREND 2

/**
 * \def SECONDS_PER_TREND_SHIFT
 * Represents the shift to perform a divide of 
 * SECONDS_PER_TREND. 
 */
#define SECONDS_PER_TREND_SHIFT 1

/**
 * \def TIMER_INTERRUPTS_PER_SECOND
 * Identifies how often the system timer interrupts per second. 
 * The main exec loop wakes up from low power mode after each 
 * timer interrupt. 
 */
#define TIMER_INTERRUPTS_PER_SECOND 2

/******************************************************************************/

/**
 * \def OUTPOUR_PRODUCT_ID
 * \brief Specify the outpour product ID number that is sent in 
 *        messages.
 */
#define AFRIDEV2_PRODUCT_ID ((uint8_t)3)

/**
 * \def FW_VERSION_MAJOR
 * \brief Specify the AfridevV2 firmware major version number.
 */
#define FW_VERSION_MAJOR ((uint8_t)0x03)

/**
 * \def FW_VERSION_MINOR
 * \brief Specify the AfridevV2 firmware minor version number. 
 *        The sign bit is set when the orientation of the sensor is inverted
 */
#define FW_MINOR 0x02
#ifndef WATERDETECT_READ_WATER_LEVEL_NORMAL
#define FW_VERSION_MINOR ((uint8_t)(FW_MINOR|0x80))
#else
#define FW_VERSION_MINOR ((uint8_t)FW_MINOR)
#endif
/**
 * \def ACTIVATE_REBOOT_KEY
 * \brief A non-zero value indicating that we need to start a 
 *        system reboot sequence.
 */
#define ACTIVATE_REBOOT_KEY 0xC3

/**
 * \def ACTIVATE_FWUPGRADE_KEY 
 * \brief A non-zero value indicating that we need to start a 
 *        firmware upgrade sequence.  This means re-booting into
 *        the boot loader.
 */
#define ACTIVATE_FWUPGRADE_KEY 0xE7

/*******************************************************************************
* System Tick Access
*******************************************************************************/

#include "waterDetect.h"

// Define the type that a system tick value is represented in
typedef uint32_t sys_tick_t;
// Just return the system tick variable value.
#define GET_SYSTEM_TICK() ((sys_tick_t)getSecondsSinceBoot());
// Return the number of elapsed seconds
#define GET_ELAPSED_TIME_IN_SEC(x) (((sys_tick_t)getSecondsSinceBoot())-(sys_tick_t)(x))

// Used for testing if running the sys tick at faster then
// the standard interval. For normal operation, must be set to 1.
#define TIME_SCALER ((uint8_t)1)

/*******************************************************************************
* MISC Macros
*******************************************************************************/

/**
 * \def TIME_5_SECONDS 
 * \brief Macro for 5 seconds
 */
#define TIME_5_SECONDS ((uint8_t)5)
/**
 * \def TIME_10_SECONDS 
 * \brief Macro for 10 seconds
 */
#define TIME_10_SECONDS ((uint8_t)10)
/**
 * \def TIME_20_SECONDS 
 * \brief Macro for 20 seconds
 */
#define TIME_20_SECONDS ((uint8_t)20)
/**
 * \def TIME_30_SECONDS 
 * \brief Macro for 30 seconds
 */
#define TIME_30_SECONDS ((uint8_t)30)
/**
 * \def TIME_60_SECONDS 
 * \brief Macro for 60 seconds
 */
#define TIME_60_SECONDS ((uint8_t)60)
/**
 * \def SECONDS_PER_MINUTE
 * \brief Macro to specify seconds per minute
 */
#define SECONDS_PER_MINUTE ((uint8_t)60)
/**
 * \def SECONDS_PER_HOUR
 * \brief Macro to specify seconds per hour
 */
#define SECONDS_PER_HOUR (SECONDS_PER_MINUTE*((uint16_t)60))
/**
 * \def TIME_ONE_HOUR
 * \brief Macro to specify one hour in terms of seconds
 */
#define TIME_ONE_HOUR SECONDS_PER_HOUR
/**
 * \def SECONDS_PER_DAY
 * \brief Macro to specify seconds per day
 */
#define SECONDS_PER_DAY ((uint32_t)86400)
/**
 * \def SEC_PER_MINUTE
 * \brief Macro to specify the number of seconds in one minute
 */
#define SEC_PER_MINUTE ((uint8_t)60)
/**
 * \def TIME_45_MINUTES
 * \brief Macro to specify the number of seconds in 45 minutes
 */
#define TIME_45_MINUTES ((uint16_t)(SEC_PER_MINUTE*(uint16_t)45))
/**
 * \def TIME_60_MINUTES
 * \brief Macro to specify the number of seconds in 60 minutes
 */
#define TIME_60_MINUTES ((uint16_t)(SEC_PER_MINUTE*(uint16_t)60))
/**
 * \def TIME_5_MINUTES
 * \brief Macro to specify the number of seconds in 5 minutes
 */
#define TIME_5_MINUTES ((uint16_t)(SEC_PER_MINUTE*(uint16_t)5))
/**
 * \def TIME_10_MINUTES
 * \brief Macro to specify the number of seconds in 10 minutes
 */
#define TIME_10_MINUTES ((uint16_t)(SEC_PER_MINUTE*(uint16_t)10))
/**
 * \def TIME_20_MINUTES
 * \brief Macro to specify the number of seconds in 10 minutes
 */
#define TIME_20_MINUTES ((uint16_t)(SEC_PER_MINUTE*(uint16_t)20))

/**
 * \typedef padId_t
 * \brief Give each pad number a name
 */
typedef enum padId_e {
    PAD0,                                                  /** 0 */
    PAD1,                                                  /** 1 */
    PAD2,                                                  /** 2 */
    PAD3,                                                  /** 3 */
    PAD4,                                                  /** 4 */
    PAD5,                                                  /** 5 */
    TOTAL_PADS = 6,                                        /** there are 6 total pads */
    MAX_PAD = PAD5,                                        /** PAD5 is the max pad value */
} padId_t;

// Give names to the various port pin numbers
#define VBAT_GND BIT1		 // Pin 1.1, Output
#define GSM_DCDC BIT2		 // Pin 1.2, Output
#define _1V8_EN BIT3		 // Pin 1.3, Output
#define GSM_INT BIT4		 // Pin 1.4, Input
#define GSM_STATUS BIT5		 // Pin 1.5, Input
#define TM_GPS BIT6          // Pin 1.6, Input
#define GPS_ON_IND BIT7      // Pin 1.7, Input

#define VBAT_MON BIT0		 // Pin 2.0, ADC
#define I2C_DRV BIT3		 // Pin 2.3, Output
#define GSM_EN BIT4			 // Pin 2.4, Output
#define LS_VCC BIT5          // Pin 2.5, Output

#define LED_GREEN BIT1		 // Pin 3.1, Output
#define LED_RED BIT2		 // Pin 3.2, Output
#define NTC_ENABLE BIT3      // Pin 3.3, Output
#define TXD BIT4			 // Pin 3.4, UART
#define RXD BIT5			 // Pin 3.5, UART
#define MSP_UART_SEL BIT7    // Pin 3.7, UART Select (Modem or GPS)

#define GPS_ON_OFF BIT2		 // Pin 4.2, Output
#define NTC_SENSE_INPUT BIT3 // Pin 4.3, ADC

#define MODEM_UART_SELECT_ENABLE() (P3OUT &= ~MSP_UART_SEL)
#define GPS_UART_SELECT_ENABLE() (P3OUT |= MSP_UART_SEL)
#define LED_GREEN_DISABLE() (P3OUT |= LED_GREEN)
#define LED_GREEN_ENABLE() (P3OUT &= ~LED_GREEN)
#define LED_RED_DISABLE() (P3OUT |= LED_RED)
#define LED_RED_ENABLE() (P3OUT &= ~LED_RED)

/*******************************************************************************
*  Centralized method for enabling and disabling MSP430 interrupts
*******************************************************************************/
static inline void enableGlobalInterrupt(void)
{
    _BIS_SR(GIE);
}
static inline void disableGlobalInterrupt(void)
{
    _BIC_SR(GIE);
}
static inline void enableSysTimerInterrupt(void)
{
    TA1CCTL0 |= CCIE;
}
static inline void disableSysTimerInterrupt(void)
{
    TA1CCTL0 &= ~CCIE;
}
static inline void restoreSysTimerInterrupt(uint16_t val)
{
    TA1CCTL0 &= ~CCIE;                                     // clear the value
    TA1CCTL0 |= val;                                       // set to val
}
static inline uint16_t getAndDisableSysTimerInterrupt(void)
{
    volatile uint16_t current = TA1CCTL0;                  // read reg

    current &= CCIE;                                       // get current interrupt setting
    TA1CCTL0 &= ~CCIE;                                     // disable interrupt
    return (current);                                      // return interrupt setting
}

// #define USE_UART_SIGNALS_FOR_GPIO
#ifdef USE_UART_SIGNALS_FOR_GPIO
static inline void setDebug0(void)
{
    P3OUT |= TXD;
}
static inline void clearDebug0(void)
{
    P3OUT &= ~TXD;
}
static inline void setDebug1(void)
{
    P3OUT |= RXD;
}
static inline void clearDebug1(void)
{
    P3OUT &= ~RXD;
}
#endif

/*******************************************************************************
*  Polling Delays
*******************************************************************************/
void secDelay(uint8_t secCount);
void ms1Delay(uint8_t msCount);
void us10Delay(uint8_t us10);

/*******************************************************************************
* sysExec.c
*******************************************************************************/

typedef enum
{
    NORMAL_SYSTEM_OP,
    MANUF_TEST_WATER,
    MANUF_TEST_MODEM,
    MANUF_WATER_PASS,
    MANUF_MODEM_PASS,
    MANUF_TEST_GPS,
    MANUF_GPS_DONE,
    MANUF_UNIT_PASS
} MANUF_STATE_T;

/**
 * \typedef sysExecData_t
 * \brief Container to hold data for the sysExec module.
 */
typedef struct sysExecData_s {
    uint32_t total_flow;                                   /**< Number of milliliters of water poured in current session */
    uint16_t downspout_rate;                               /**< Maximum downspout rate setting for tuning accuracy based on board thickness */
    uint16_t dry_count;                                    /**< Current number of consecutive trends with no water */
    uint16_t dry_wake_time;                                /**< Number of consecutive trends with no water before sleeping */
    bool FAMsgWasSent : 1;                                 /**< Flag specifying if Final Assembly msg was sent */
    bool mCheckInMsgWasSent : 1;                           /**< Flag specifying if Monthly Check In msg was sent */
    bool appRecordWasSet: 1;                               /**< Flag specifying if App record was set */
    bool sendTestMsgWasSent : 1;                           /**< Flag specifying if send_test msg was sent */
    bool sendTestRespWasSeen : 1;                          /**< Flag specifying if send_test response was seen */
    bool saveCapSensorBaselineData : 1;                    /**< Flag indicating that baseline cap sensor data must be stored */
    bool sendSensorDataMessage :1;                         /**< flag specifying if a sensor data report is requested */
    bool sendSensorDataNow: 1;                             /**< flag specifying that water data is immediately reported over Modem */
    bool faultWaterDetect: 1;                              /**< flag specifying that water water or unknowns are stuck  */
    uint8_t waterDetectResets;                             /**< number of times the water detect algorithm data was cleared */
    uint8_t send_test_result;                              /**< result of sending the SEND_TEST message to the Modem */
    uint8_t led_on_time;                                   /**< number of iterations of the main loop that LED should remain on (2 sec resolution) */
    int8_t secondsTillStartUpMsgTx;                        /**< Seconds until startup messages are transmitted */
    int8_t secondsTillReboot;                              /**< How long to wait to perform a MSP430 reset */
    uint8_t rebootCountdownIsActive;                       /**< Specify if a reboot countdown sequence is in-progress */
    uint8_t noWaterMeasCount;                              /**< Maintain a count of sequential no water detected measurements */
    uint8_t waterMeasDelayCount;                           /**< Delay next water measurement count-down */
    uint8_t time_elapsed;                                  /**< Time that the unit slept during low power mode check */
    MANUF_STATE_T mtest_state;                             /**< Manufacturing test State */
} sysExecData_t;


/**
 * \def REBOOT_KEY1
 * \def REBOOT_KEY2
 * \def REBOOT_KEY3
 * \def REBOOT_KEY4
 * \brief These keys are used to validate the OTA reset command.
 */
#define REBOOT_KEY1 ((uint8_t)0xAA)
#define REBOOT_KEY2 ((uint8_t)0x55)
#define REBOOT_KEY3 ((uint8_t)0xCC)
#define REBOOT_KEY4 ((uint8_t)0x33)

void sysExec_exec(void);
bool sysExec_startRebootCountdown(uint8_t activateReboot);
void sysError(void);

// half second accuracy! 60 = 30 seconds; 3600 = 30 minutes
#ifdef WATER_DEBUG
#define SYSEXEC_NO_WATER_SLEEP_DELAY 60
#else
#ifndef DEBUG_BATTERY_TEST
#define SYSEXEC_NO_WATER_SLEEP_DELAY 3600
#else
#define SYSEXEC_NO_WATER_SLEEP_DELAY 60
#endif
#endif

extern sysExecData_t sysExecData;


/*******************************************************************************
*  time.c
*******************************************************************************/
void timerA0_inter_sample_sleep(void);
void timerA0_20sec_sleep(void);

/*******************************************************************************
* Utils.c
*******************************************************************************/
typedef struct timeCompare_s {
    uint8_t hoursA;
    uint8_t minutesA;
    uint8_t secondsA;
    uint8_t hoursB;
    uint8_t minutesB;
    uint8_t secondsB;
    uint32_t timeDiffInSeconds;
} timeCompare_t;

unsigned int gen_crc16(const unsigned char *data, unsigned int size);
unsigned int gen_crc16_2buf(const unsigned char *data1, unsigned int size1, const unsigned char *data2, unsigned int size2);
uint32_t timeInSeconds(uint8_t hours, uint8_t minutes, uint8_t seconds);
void calcTimeDiffInSeconds(timeCompare_t *timeCompareP);
void reverseEndian32(uint32_t *valP);
void reverseEndian16(uint16_t *valP);

/*******************************************************************************
* modemCmd.h
*******************************************************************************/

/**
 * \typedef modemCmdWriteData_t 
 * \brief Container to pass parmaters to the modem command write 
 *        function.
 */
typedef struct modemCmdWriteData_s {
    outpour_modem_command_t cmd;                           /**< the modem command */
    MessageType_t payloadMsgId;                            /**< the payload type (Afridev message type) */
    uint8_t *payloadP;                                     /**< the payload pointer (if any) */
    uint16_t payloadLength;                                /**< size of the payload in bytes */
    uint16_t payloadOffset;                                /**< for receiving partial data */
    bool statusOnly;                                       /**< only perform status retrieve from modem - no cmd */
} modemCmdWriteData_t;

/**
 * \typedef modemCmdReadData_t 
 * \brief Container to read the raw response returned from the 
 *        modem as a result of sending it a command. 
 */
typedef struct modemCmdReadData_s {
    modem_command_t modemCmdId;                            /**< the cmd we are sending to the modem */
    bool valid;                                            /**< indicates that the response is correct (crc passed, etc) */
    uint8_t *dataP;                                        /**< the pointer to the raw buffer */
    uint16_t lengthInBytes;                                /**< the length of the data in the buffer */
} modemCmdReadData_t;

/**
 * \def OTA_PAYLOAD_MAX_READ_LENGTH
 * \brief Specify the max length of space allowed in the RX ISR
 *        Buffer for OTA message payload data. This length is in
 *        addition to the message header overhead which is read
 *        into the RX ISR buffer as well. Note that this max
 *        length only applies to the data portion received via
 *        the M_COMMAND_GET_INCOMING_PARTIAL command from the
 *        Modem. If the OTA data length is larger than the
 *        allowed max length, than an iterative approach is used
 *        to read the data from the modem in chunks. The only
 *        case where the iterative approach is needed is for a
 *        firmware upgrade.  All other OTA payload data length
 *        is currently less than 16 bytes.
 */
#define OTA_PAYLOAD_MAX_RX_READ_LENGTH ((uint16_t)512)

/**
 * \def OTA_RESPONSE_LENGTH
 * \brief Specify the total size of an OTA response.  This is 
 *        the length of an OTA response message being sent to
 *        the cloud by the unit. It is a constant value. It
 *        consists of the header and the data.  The header is
 *        always 16 bytes and the data is always 32 bytes -
 *        regardless of whether it is all used by the message
 *        sent.  Example OTA Response messages include
 *        OTA_OPCODE_GMT_CLOCKSET, OTA_OPCODE_LOCAL_OFFSET, etc.
 * \note An OTA Response should not be confused with a data 
 *       message (also sent to the cloud). Data messages are not
 *       a constant length.  Examples include
 *       MSG_TYPE_DAILY_LOG, MSG_TYPE_CHECKIN, etc.
 */
#define OTA_RESPONSE_LENGTH (OTA_RESPONSE_HEADER_LENGTH+OTA_RESPONSE_DATA_LENGTH)

/**
 * \def OTA_RESPONSE_HEADER_LENGTH
 * \brief Define the header length of an OTA response message 
 */
#define OTA_RESPONSE_HEADER_LENGTH ((uint8_t)16)

/**
 * \def OTA_RESPONSE_DATA_LENGTH
 * \brief Define the data length of an OTA response message. The
 *        data follows the header in the message.
 */
#define OTA_RESPONSE_DATA_LENGTH ((uint8_t)32)

/**
 * \typedef otaResponse_t
 * \brief Define a container to hold a partial OTA response.
 */
typedef struct otaResponse_s {
    uint8_t *buf;                                          /**< A buffer to hold one OTA message */
    uint16_t lengthInBytes;                                /**< how much valid data is in the buf */
    uint16_t remainingInBytes;                             /**< how much remaining of the total OTA */
}otaResponse_t;

/*******************************************************************************
* manufStore.c
*******************************************************************************/
/**
 * \typedef GPS_Data_t
 * \brief Define a container to hold GPS debug data messages
 */
typedef struct GPS_Data_s {
    uint8_t time[10];                                      /**< time reported by GPS module. Format:@xx:xx:xx  */ 
    uint8_t latitude[12];                                  /**< latitude reported by GPS module. Format: Nxx xx.xxx */
    uint8_t longitude[14];                                 /**< longitude reported by GPS module. Format: Wxxx xx.xxx */
    uint8_t fix_quality[4];                                /**< fix quality reported by GPS module. Format: q=x */
    uint8_t sat_count[5];                                  /**< satellite count reported by GPS module. Format: s=xx */
    uint8_t hdop_score[7];                                 /**< hdop score in meters reported by GPS module. Format: h=xx.x */
    uint8_t fix_is_valid[4];                               /**< fix is valid as determined by Afridev2 rules. Format: v=Y or N */
    uint8_t time_to_fix[8];                                /**< time to get GPS fix determined by Afridev2 code. Format: t=xxxxx */
    uint8_t zero;                                          /**< end of string, always zero */
} GPS_Data_t;

/**
 * \typedef Water_Data_t
 * \brief Define a container to hold Water detection debug data messages
 */
typedef struct Water_Data_s {
    uint8_t time[6];                                       /**< system time of data in seconds.*/ 
    uint8_t tempc[8];                                      /**< current pad temperature in tenths of degrees C.*/
    uint8_t pad0[9];                                       /**< current pad0 capacitance value OR current air deviation*/
    uint8_t pad1[9];                                       /**< current pad1 capacitance value OR current air deviation*/
    uint8_t pad2[9];                                       /**< current pad2 capacitance value OR current air deviation*/
    uint8_t pad3[9];                                       /**< current pad3 capacitance value OR current air deviation*/
    uint8_t pad4[9];                                       /**< current pad4 capacitance value OR current air deviation*/
    uint8_t pad5[9];                                       /**< current pad5 capacitance value OR current air deviation*/
    uint8_t level[2];                                      /**< current water level detected L1 to L6 */
    uint8_t flow[10];                                      /**< current water flow estimate for last 2 seconds in ml */
    uint8_t zero;                                          /**< end of string, always zero */
} Water_Data_t;

/**
 * \typedef Version_Data_t
 * \brief Define a container to hold software release information
 */
typedef struct Version_Data_s {
    uint8_t prefix;                                        /**< character that identifies this message */
    uint8_t release[12];                                   /**< name of firmware release */
    uint8_t version[12];                                   /**< version of firmware release */
    uint8_t date[12];                                      /**< date of firmware release */
    uint8_t zero;                                          /**< end of string, always zero */
} Version_Data_t;

extern Water_Data_t water_report;
extern GPS_Data_t gps_report;

#ifdef WATER_DEBUG
extern bool mTestGPSDone(void);
extern bool mTestWaterDone(void);
#endif
extern bool mTestBaselineDone(void);

void modemCmd_exec(void);
void modemCmd_init(void);
bool modemCmd_write(modemCmdWriteData_t *writeCmdP);
void modemCmd_read(modemCmdReadData_t *readDataP);
bool modemCmd_isResponseReady(void);
bool modemCmd_isError(void);
bool modemCmd_isBusy(void);
void modemCmd_isr(void);

/*******************************************************************************
* modemPower.c
*******************************************************************************/
void modemPower_exec(void);
void modemPower_init(void);
void modemPower_restart(void);
void modemPower_powerDownModem(void);
bool modemPower_isModemOn(void);
uint16_t modemPower_getModemOnTimeInSecs(void);
bool modemPower_isModemOnError(void);

/*******************************************************************************
* modemMgr.c
*******************************************************************************/

/**
 * \def SHARED_BUFFER_SIZE
 * The OTA payload buffer is shared for other things that need a
 * buffer when the modem OTA processing is not active.  
 */
#define SHARED_BUFFER_SIZE OTA_PAYLOAD_MAX_RX_READ_LENGTH

// wait up to 5 minutes for a SEND_TEST "Success"
#define SEND_TEST_RETRIES 150
#define SYSEXEC_SEND_TEST_IDLE 0
#define SYSEXEC_SEND_TEST_RUNNING 1
#define SYSEXEC_SEND_TEST_PASS 2
#define SYSEXEC_SEND_TEST_FAIL 0xFF

void sysExec_set_send_test_result(uint8_t result);

/**
 * \typedef msgHeader_t
 * \brief Define the structure of the header that sits on top of
 *        of all outbound messages.
 */
typedef struct __attribute__((__packed__))msgHeader_s {
    uint8_t payloadStartByte;                              /**< 0 */
    uint8_t payloadMsgId;                                  /**< 1 */
    uint8_t productId;                                     /**< 2 */
    uint8_t GMTsecond;                                     /**< 3 */
    uint8_t GMTminute;                                     /**< 4 */
    uint8_t GMThour;                                       /**< 5 */
    uint8_t GMTday;                                        /**< 6 */
    uint8_t GMTmonth;                                      /**< 7 */
    uint8_t GMTyear;                                       /**< 8 */
    uint8_t fwMajor;                                       /**< 9 */
    uint8_t fwMinor;                                       /**< 10 */
    uint8_t daysActivatedMsb;                              /**< 11 */
    uint8_t daysActivatedLsb;                              /**< 12 */
    uint8_t storageWeek;                                   /**< 14 */
    uint8_t storageDay;                                    /**< 13 */
    uint8_t reserve1;                                      /**< 15 */
} msgHeader_t;


/**
 * \typedef mwBatchState_t
 * \brief Define the states that make up the modem write batch
 *        job states.
 */
typedef enum mwBatchState_e {
    MWBATCH_STATE_IDLE,
    MWBATCH_STATE_PING,
    MWBATCH_STATE_PING_WAIT,
    MWBATCH_STATE_WRITE_CMD,
    MWBATCH_STATE_WRITE_CMD_WAIT,
    MWBATCH_STATE_MODEM_STATUS,
    MWBATCH_STATE_MODEM_STATUS_WAIT,
    MWBATCH_STATE_MSG_STATUS,
    MWBATCH_STATE_MSG_STATUS_WAIT,
    MWBATCH_STATE_DONE,
}mwBatchState_t;

/**
 * \typedef mwState_t
 * \brief Define the states that make up the modem shutdown
 *        states.
 */
typedef enum mmShutdownState_e {
    M_SHUTDOWN_STATE_IDLE,
    M_SHUTDOWN_STATE_WRITE_CMD,
    M_SHUTDOWN_STATE_WRITE_CMD_WAIT,
    M_SHUTDOWN_STATE_WAIT,
    M_SHUTDOWN_STATE_DONE,
}mmShutdownState_t;

/**
 * \typedef mwBatchData_t
 * \brief Define a container to hold data specific to the modem
 *        write batch job.
 */
typedef struct mwBatchData_s {
    bool allocated;                                        /**< flag to indicate modem is owned by client */
    bool batchWriteActive;                                 /**< currently sending out a write batch job */
    bool commError;                                        /**< A modem UART comm error occurred during the job */
    uint8_t sendTestActive;                                /**< A flag for a special handling of "transmission in progress" */
    uint8_t modemNetworkStatus;                            /**< network connection status received from modem */
    mwBatchState_t mwBatchState;                           /**< current batch job state */
    modemCmdWriteData_t *cmdWriteP;                        /**< A pointer to the command info object provided by user */
    otaResponse_t otaResponse;                             /**< payload of the last ota message received */
    uint8_t numOfOtaMsgsAvailable;                         /**< parsed from modem message status command */
    uint16_t sizeOfOtaMsgsAvailable;                       /**< parsed from modem message status command */
    bool shutdownActive;                                   /**< currently performing a modem shutdown */
    mmShutdownState_t mmShutdownState;                     /**< current shutdown state */
    sys_tick_t shutdownTimestamp;                          /**< for determing time delay for modem power down */
} mwBatchData_t;

extern mwBatchData_t mwBatchData;

void modemMgr_exec(void);
void modemMgr_init(void);
bool modemMgr_grab(void);
bool modemMgr_isModemUp(void);
bool modemMgr_isModemUpError(void);
void modemMgr_sendModemCmdBatch(modemCmdWriteData_t *cmdWriteP);
void modemMgr_stopModemCmdBatch(void);
bool modemMgr_isModemCmdComplete(void);
bool modemMgr_isModemCmdError(void);
void modemMgrRelease(void);
void modemMgr_restartModem(void);
bool modemMgr_isAllocated(void);
void modemMgr_release(void);
bool modemMgr_isReleaseComplete(void);
otaResponse_t* modemMgr_getLastOtaResponse(void);
bool modemMgr_isLinkUp(void);
bool modemMgr_isLinkUpError(void);
uint8_t modemMgr_getNumOtaMsgsPending(void);
uint16_t modemMgr_getSizeOfOtaMsgsPending(void);
uint8_t* modemMgr_getSharedBuffer(void);

/*******************************************************************************
* msgData.c
*******************************************************************************/
void dataMsgMgr_exec(void);
void dataMsgMgr_init(void);
bool dataMsgMgr_isSendMsgActive(void);
bool dataMsgMgr_sendDataMsg(MessageType_t msgId, uint8_t *dataP, uint16_t lengthInBytes);
bool dataMsgMgr_sendTestMsg(MessageType_t msgId, uint8_t *dataP, uint16_t lengthInBytes);
bool dataMsgMgr_startSendingScheduled(void);
bool dataMsgMgr_sendTestMsg_Passed(void);

/*******************************************************************************
* msgOta.c
*******************************************************************************/
void otaMsgMgr_exec(void);
void otaMsgMgr_init(void);
void otaMsgMgr_getAndProcessOtaMsgs(void);
void otaMsgMgr_stopOtaProcessing(void);
bool otaMsgMgr_isOtaProcessingDone(void);

#define SENSOR_REQ_SENSOR_DATA 0
#define SENSOR_OVERWRITE_FACTORY 1
#define SENSOR_RESET_WATER_DETECT 2
#define SENSOR_SET_UNKNOWN_LIMIT 3
#define SENSOR_REPORT_NOW 4
#define SENSOR_DOWNSPOUT_RATE 5
#define SENSOR_SET_WATER_LIMIT 6
#define SENSOR_SET_WAKE_TIME 7

/*******************************************************************************
* msgOtaUpgrade.c
*******************************************************************************/

/**
 * \typedef fwUpdateResult_t
 * \brief Specify the different status results that the firmware
 *        update state machine can exit in.
 */
typedef enum fwUpdateResult_e {
    RESULT_NO_FWUPGRADE_PERFORMED = 0,
    RESULT_DONE_SUCCESS = 1,
    RESULT_DONE_ERROR = -1,
} fwUpdateResult_t;

fwUpdateResult_t otaUpgrade_processOtaUpgradeMessage(void);
fwUpdateResult_t otaUpgrade_getFwUpdateResult(void);
uint16_t otaUpgrade_getFwMessageCrc(void);
uint16_t otaUpgrade_getFwCalculatedCrc(void);
uint16_t otaUpgrade_getFwLength(void);
uint8_t otaUpgrade_getErrorCode(void);

/*******************************************************************************
* msgDebug.c
*******************************************************************************/

void dbgMsgMgr_sendDebugMsg(MessageType_t msgId, uint8_t *dataP, uint16_t lengthInBytes);

/*******************************************************************************
* msgData.c
*******************************************************************************/
/**
 * \typedef dataMsgState_t
 * \brief Specify the states for sending a data msg to the 
 *        modem.
 */
typedef enum dataMsgState_e {
    DMSG_STATE_IDLE,
    DMSG_STATE_GRAB,
    DMSG_STATE_WAIT_FOR_MODEM_UP,
    DMSG_STATE_SEND_MSG,
    DMSG_STATE_SEND_MSG_WAIT,
    DMSG_STATE_WAIT_FOR_LINK,
    DMSG_STATE_PROCESS_OTA,
    DMSG_STATE_PROCESS_OTA_WAIT,
    DMSG_STATE_RELEASE,
    DMSG_STATE_RELEASE_WAIT,
} dataMsgState_t;

/**
 * \typedef dataMsgSm_t
 * \brief Define a contiainer to hold the information needed by 
 *        the data message module to perform sending a data
 *        command to the modem.
 *  
 * \note To save memory, this object can potentially be a common
 *       object that all clients use because only one client
 *       will be using the modem at a time?
 */
typedef struct dataMsgSm_s {
    dataMsgState_t dataMsgState;                           /**< current data message state */
    modemCmdWriteData_t cmdWrite;                          /**< the command info object */
    uint8_t modemResetCount;                               /**< for error recovery, count times modem is power cycled */
    bool sendCmdDone;                                      /**< flag to indicate sending the current command to modem is complete */
    bool allDone;                                          /**< flag to indicate send session is complete and modem is off */
    bool connectTimeout;                                   /**< flag to indicate the modem was not able to connect to the network */
    bool commError;                                        /**< flag to indicate an modem UART comm error occured - not currently used - can remove */
} dataMsgSm_t;

void dataMsgSm_init(void);
void dataMsgSm_initForNewSession(dataMsgSm_t *dataMsgP);
void dataMsgSm_sendAnotherDataMsg(dataMsgSm_t *dataMsgP);
void dataMsgSm_stateMachine(dataMsgSm_t *dataMsgP);

/*******************************************************************************
* time.c
*******************************************************************************/
/**
 * \typedef timePacket_t 
 * \brief Specify a structure to hold time data that will be 
 *        sent as part of the final assembly message.
 */
typedef struct __attribute__((__packed__))timePacket_s {
    uint8_t second;
    uint8_t minute;
    uint8_t hour24;
    uint8_t day;
    uint8_t month;
    uint8_t year;
} timePacket_t;

void timerA0_init(void);
void getBinTime(timePacket_t *tpP);
uint8_t bcd_to_char(uint8_t bcdValue);
uint32_t getSecondsSinceBoot(void);

// Watchdog Macros
// 1 second time out, uses ACLK
#define WATCHDOG_TICKLE() (WDTCTL = WDT_ARST_1000)
#define WATCHDOG_STOP() (WDTCTL = (WDTPW | WDTHOLD))

// For testing, don't enable watchdog
// #define WATCHDOG_TICKLE() (WDTCTL = WDTPW | WDTHOLD)


/*******************************************************************************
* manufStore.c
*******************************************************************************/

/**
 * \typedef MDR_type_e
 * \brief Define the sections of the Manufacturing data
 */
typedef enum
{
    MDR_Water_Record,
    MDR_GPS_Record,
    MDR_Modem_Record
} MDR_type_e;

#define MDR_NUMPADS 6
 
/**
 * \typedef MDRwaterRecord_t
 * \brief Water Detect Manufacturing test data (26 bytes)
 */
typedef struct __attribute__((__packed__)){
        uint16_t padBaseline[MDR_NUMPADS];                 /**< Pad Baseline Capacitance values for a DRY board */
        uint16_t airDeviation[MDR_NUMPADS];                /**< Air Deviation values for first water measurement  */
        int16_t padTemp;                                   /**< Temperature of pads at padBaseline */
} MDRwaterRecord_t;

#define MDR_GPS_LAT_LEN 11
#define MDR_GPS_LON_LEN 12

/**
 * \typedef MDRgpsRecord_t
 * \brief GPS Manufacturing test data (30 bytes)
 */
typedef struct __attribute__((__packed__))mdr_gps_s {
    uint16_t gpsTime;                                      /**< Time of GPS Test Fix */
    uint16_t gpsHdop;                                      /**< HDOP metric of GPS Test Fix */
    uint8_t gpsQuality;                                    /**< Quality of GPS Test Fix */
    uint8_t gpsSatellites;                                 /**< Numbr of Sattelites Quality with GPS Test Fix */
    uint8_t gpsLatitude[MDR_GPS_LAT_LEN];                  /**< Latitiude of GPS Test Fix */
    uint8_t zero;                                          /**< word alignment padding */
    uint8_t gpsLongitude[MDR_GPS_LON_LEN];                 /**< Longitude of GPS Test Fix */
} MDRgpsRecord_t;

/**
 * \typedef MDRmodemRecord_t
 * \brief Modem Manufacturing test data (2 bytes)
 */
typedef struct __attribute__((__packed__))mdr_modem_s {
    uint8_t send_test;                                     /**< Count of number of times SEND_TEST message was sent */
    uint8_t future_use;
} MDRmodemRecord_t;

extern bool manufRecord_manuf_test_init(void);
extern void manufRecord_manuf_test_result(void);
extern bool waterDetect_restore_pads_baseline(MDRwaterRecord_t *wr);

/**
 * \typedef manufRecord_t
 * \brief This structure is used to store the results of Manufacturing testing in the factory
 *
 */
typedef struct __attribute__((__packed__))manufRecord_s {
    uint16_t magic;                                        /**< A known pattern for "quick" test of structure validity */
    uint16_t recordLength;                                 /**< Length of structure */
    MDRwaterRecord_t wr;                                   /**< Water detect test data, 26 bytes */
    MDRgpsRecord_t gr;                                     /**< GPS test data, 30 bytes */
    MDRmodemRecord_t mr;                                   /**< modem test data, 2 bytes */
    uint16_t crc16;                                        /**< crc16 Used to validate the data */
} manufRecord_t;

uint16_t gps_getGpsMessage(uint8_t **payloadPP);
bool manufRecord_updateManufRecord(MDR_type_e mr_type, uint8_t *mr_in, uint8_t len_in);
void manufRecord_getWaterInfo(MDRwaterRecord_t *wr_in);
bool manufRecord_checkForValidManufRecord(void);
bool manufRecord_initBootloaderRecord(void);
bool manufRecord_setBaselineAirTargets(void);
bool manufRecord_send_test(void);
void manufRecord_update_LEDs(void);
uint16_t manufRecord_getSensorDataMessage(uint8_t **payloadPP);

/*******************************************************************************
* storage.c
*******************************************************************************/
void storageMgr_init(void);
void storageMgr_exec(uint16_t currentFlowRateInSecML, uint8_t time_elapsed);
void storageMgr_overrideUnitActivation(bool flag);
uint16_t storageMgr_getDaysActivated(void);
void storageMgr_resetRedFlag(void);
bool storageMgr_getRedFlagConditionStatus(void);
void storageMgr_resetRedFlagAndMap(void);
void storageMgr_resetWeeklyLogs(void);
void storageMgr_setStorageAlignmentTime(uint8_t alignSecond, uint8_t alignMinute, uint8_t alignHour24);
void storageMgr_setTransmissionRate(uint8_t transmissionRateInDays);
uint16_t storageMgr_getNextDailyLogToTransmit(uint8_t **dataPP);
void storageMgr_sendDebugDataToUart(void);
uint8_t storageMgr_getStorageClockInfo(uint8_t *bufP);
uint8_t storageMgr_getStorageClockHour(void);
uint8_t storageMgr_getStorageClockMinute(void);
uint8_t storageMgr_prepareMsgHeader(uint8_t *dataPtr, uint8_t payloadMsgId);
uint16_t storageMgr_getMonthlyCheckinMessage(uint8_t **payloadPP);
uint16_t storageMgr_getActivatedMessage(uint8_t **payloadPP);

#ifdef DEBUG_DAILY_WATER_REPORTS
#define STORAGE_TRANSMISSION_RATE_DEFAULT 1
#else
#define STORAGE_TRANSMISSION_RATE_DEFAULT 7
#endif

/*******************************************************************************
* waterSense.c
*******************************************************************************/
void waterSense_init(void);
void waterSense_exec(void);
bool waterSense_isInstalled(void);
uint16_t waterSense_getLastMeasFlowRateInML(void);
void waterSense_clearStats(void);
uint16_t waterSense_getPadStatsMax(padId_t padId);
uint16_t waterSense_padStatsMin(padId_t padId);
uint16_t waterSense_getPadStatsSubmerged(padId_t padId);
uint16_t waterSense_getPadStatsUnknowns(void);
uint16_t padStats_zeros(void);
int16_t waterSense_getTempCelcius(void);
void waterSense_writeConstants(uint8_t *dataP);
void waterSense_sendDebugDataToUart(void);

/*******************************************************************************
* CTS_HAL.c
*******************************************************************************/
extern uint8_t CAPSENSE_ACTIVE;

/*******************************************************************************
* hal.c
*******************************************************************************/
void hal_sysClockInit(void);
void hal_uartInit(void);
void hal_pinInit(void);
void hal_led_toggle(void);
void hal_led_blink_red(void);
void hal_led_green(void);
void hal_led_red(void);
void hal_led_none(void);
void hal_led_both(void);
void hal_low_power_enter(void);

/*******************************************************************************
* flash.c
*******************************************************************************/
void msp430Flash_erase_segment(uint8_t *flashSectorAddrP);
void msp430Flash_write_bytes(uint8_t *flashP, uint8_t *srcP, uint16_t num_bytes);
void msp430Flash_write_int16(uint8_t *flashP, uint16_t val16);
void msp430Flash_write_int32(uint8_t *flashP, uint32_t val32);

/*******************************************************************************
* For Firmware Upgrade Support
*******************************************************************************/
/**
 * \def FLASH_UPGRADE_KEY1
 * \def FLASH_UPGRADE_KEY2
 * \def FLASH_UPGRADE_KEY3
 * \def FLASH_UPGRADE_KEY4
 * \brief These keys are used to validate the OTA firmware
 *        upgrade command.
 */
#define FLASH_UPGRADE_KEY1 ((uint8_t)0x31)
#define FLASH_UPGRADE_KEY2 ((uint8_t)0x41)
#define FLASH_UPGRADE_KEY3 ((uint8_t)0x59)
#define FLASH_UPGRADE_KEY4 ((uint8_t)0x26)

/*******************************************************************************
* appRecord.c
*******************************************************************************/
bool appRecord_initAppRecord(void);
bool appRecord_checkForValidAppRecord(void);
bool appRecord_checkForNewFirmware(void);
bool appRecord_updateFwInfo(bool newFwIsReady, uint16_t newFwCrc);
bool appRecord_getNewFirmwareInfo(bool *newFwReadyP, uint16_t *newFwCrcP);
void appRecord_erase(void);
#if 0
void appRecord_test(void);
#endif

/*******************************************************************************
* MsgSchedule.c 
*******************************************************************************/
void msgSched_init(void);
void msgSched_exec(void);
void msgSched_scheduleDailyWaterLogMessage(void);
void msgSched_scheduleActivatedMessage(void);
void msgSched_scheduleMonthlyCheckInMessage(void);
void msgSched_scheduleGpsLocationMessage(void);
void msgSched_scheduleGpsMeasurement(void);
void msgSched_scheduleSensorDataMessage(void);
void msgSched_getNextMessageToTransmit(modemCmdWriteData_t *cmdWriteP);

/*******************************************************************************
* gps.c 
*******************************************************************************/
void gps_init(void);
void gps_exec(void);
void gps_start(void);
void gps_stop(void);
bool gps_isActive(void);
uint16_t gps_getGpsMessage(uint8_t **payloadP);
uint16_t gps_getGpsData(uint8_t *bufP);

/**
 * \typedef gpsState_t
 * \brief Define the different states to manage on the GPS.
 */
typedef enum {
    GPS_STATE_IDLE,
    GPS_STATE_POWER_UP,
    GPS_STATE_POWER_UP_WAIT,
    GPS_STATE_MSG_RX_START,
    GPS_STATE_MSG_RX_WAIT,
    GPS_STATE_RETRY,
    GPS_STATE_DONE,
} gpsState_t;

/**
 * \typedef gpsData_t
 * \brief Data used to track the progress of GPS data acquisition
 */
typedef struct gpsData_s {
    bool active;
    gpsState_t state;
    uint8_t gpsOnRetryCount;
    uint8_t retryCount;
    sys_tick_t startGpsTimestamp;
} gpsData_t;

extern gpsData_t gpsData;

#ifdef GPS_DEBUG
void gps_record_last_fix(void);
#endif

/*******************************************************************************
* gpsPower.c 
*******************************************************************************/
void gpsPower_exec(void);
void gpsPower_init(void);
void gpsPower_restart(void);
void gpsPower_powerDownGPS(void);
bool gpsPower_isGpsOn(void);
bool gpsPower_isGpsOnError(void);
uint16_t gpsPower_getGpsOnTimeInSecs(void);

/*******************************************************************************
* gpsMsg.c 
*******************************************************************************/

typedef struct __attribute__((__packed__))gpsReportData_s {
    uint8_t hours;
    uint8_t minutes;
    int32_t latitude;
    int32_t longitude;
    uint8_t fixQuality;
    uint8_t numOfSats;
    uint8_t hdop;
    uint8_t reserved;
    uint16_t fixTimeInSecs;
}gpsReportData_t;

void gpsMsg_init(void);
void gpsMsg_exec(void);
bool gpsMsg_start(void);
void gpsMsg_stop(void);
bool gpsMsg_isActive(void);
bool gpsMsg_isError(void);
bool gpsMsg_gotGgaMessage(void);
bool gpsMsg_gotValidGpsFix(void);
void gpsMsg_isr(void);
uint8_t gpsMsg_getGgaParsedData(uint8_t *bufP);
void gpsMsg_setMeasCriteria(uint8_t numSats, uint8_t hdop, uint16_t minMeasTime);
void gps_record_last_fix(void);

