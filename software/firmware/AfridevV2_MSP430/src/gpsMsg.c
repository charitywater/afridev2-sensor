/** 
 * @file gpsMsg.c
 * \n Source File
 * \n Outpour MSP430 Firmware
 * 
 * \brief GPS module responsible for receiving and processing 
 *        GPS messages (from the UART).
 */

// Example valid GGA message
// $GPGGA,220301.000,3716.1823,N,12156.0250,W,1,07,2.1,58.6,M,-25.8,M,,0000*5C

#include "outpour.h"
#include "minmea.h"
#ifdef WATER_DEBUG
#include <debugUart.h>
#endif

/***************************
 * Module Data Definitions
 **************************/

/**
 * \def GPS_MAX_SENTENCE_LENGTH
 * Max GPS message that should ever occur (based on NMEA 
 * standard) 
 */
#define GPS_MAX_SENTENCE_LENGTH ((uint8_t)83)

/**
 * \def GPS_DEFAULT_MIN_SATS_FOR_FIX
 * Minimum number of satellites required to pass fix test
 */
#define GPS_DEFAULT_MIN_SATS_FOR_FIX ((uint8_t)4)

/**
 * \def GPS_DEFAULT_MAX_HDOP_FOR_FIX
 * Max HDOP value allowed to pass fix test
 */
#define GPS_DEFAULT_MAX_HDOP_FOR_FIX ((uint8_t)30)

/**
 * \def GPS_DEFAULT_MIN_TIME_FOR_FIX
 * Minimum on-time allowed to pass fix test
 */
#define GPS_DEFAULT_MIN_SECONDS_FOR_FIX ((uint16_t)1*60)

/**
 * \def GPS_RX_BUF_SIZE
 * Specify the length of the receive buffer used to store the 
 * GGA message from the GPS device 
 */
#define GPS_RX_BUF_SIZE 96

/**
 * \def MAX_GGA_WAIT_TIME_IN_SEC
 * Amount of time to wait to receive an GGA message. They are 
 * sent every second from the GPS device. 
 */
#define MAX_GGA_MSG_WAIT_TIME_IN_SEC (10 * TIME_SCALER)

/**
 * \typedef gps_data_t
 * Structure containing module data
 */
typedef struct gpsMsgdata_s {
    bool busy;                                             /**< Flag to indicate module is waiting for a GGA msg */

    bool ggaMsgAvailable;                                  /**< Flag to indicate a valid GGA msg is ready */
    uint8_t ggaMsgLength;                                  /**< The length of the GGA msg */
    bool gpsFixIsValid;                                    /**< We have a valid satellite location fix */
    sys_tick_t measTime;                                   /**< Time we started the measurement session */

    sys_tick_t waitForGgaTimestamp;                        /**< The time we we started waiting for an GGA msg */
    bool noGgaMsgError;                                    /**< Flag indicating timeout occurred waiting for GGA msg */

    bool ggaMsgFromIsrReady;                               /**< ISR flag to indicate an GGA msg has been received */
    uint8_t isrRxIndex;                                    /**< Current ISR index into receive buffer */
    bool isrGotStart$;                                     /**< ISR processing flag that we got a '$' character */

    struct minmea_sentence_gga frame;                      /**< Container to hold the parsed GGA sentence (using minmea) */

    uint8_t requiredNumSats;                               /**< Required minimum number of satellites for fix criteria */
    uint8_t requiredMaxHdop;                               /**< Required max hdop value for fix criteria (x < hdop) to pass */
    uint16_t requiredMinTimeInSeconds;                     /**< Required min time value for fix criteria (x > time) to pass */

} gpsMsgData_t;


/****************************
 * Module Data Declarations
 ***************************/

/**
* \var gpsRxBuf 
* Where we will put the bytes received from the GPS. This buffer
* location is specified in the linker command file to live right 
* below the stack space.
*/
#pragma SET_DATA_SECTION(".commbufs")
char gpsRxBuf[GPS_RX_BUF_SIZE];
#pragma SET_DATA_SECTION()

/**
* \var gpsMsgData
* Declare module data structure
*/
gpsMsgData_t gpsMsgData;


/**
* \var gga_match_template
* Used to look for the GGA ASCII letters in a NMEA message 
* sentence.
*/
static const uint8_t gga_match_template[] = { '$', 'G', 'P', 'G', 'G', 'A' };

#ifdef SIMULATE_GPS_FIX
/**
* \var ggaTestString
* Used for testing only. Mimic a valid RMC sentance.
*/
static const char ggaTestString[] =
                                    "$GPGGA,220301.000,3716.1823,N,12156.0250,W,1,07,2.1,58.6,M,-25.8,M,,0000*5C";
#endif

/*************************
 * Module Prototypes
 ************************/

static void gpsMsg_isrRestart(void);
static void gpsMSg_processGgaSentence(void);
static bool gpsMsg_verifyChecksum(void);
static uint8_t asciiToHex(uint8_t asciiByte);
static bool gpsMsg_matchGga(void);

static void enable_UART_tx(void);
static void enable_UART_rx(void);
static void disable_UART_tx(void);
static void disable_UART_rx(void);

/***************************
 * Module Public Functions
 **************************/

/**
* \brief Module init routine. Call once at startup.
* 
* \ingroup PUBLIC_API
*/
void gpsMsg_init(void)
{
    memset(&gpsMsgData, 0, sizeof(gpsMsgData_t));

    gpsMsgData.isrRxIndex = 0;
    gpsMsgData.isrGotStart$ = false;
    // Setup default criteria to meet a valid location fix
    gpsMsgData.requiredNumSats = GPS_DEFAULT_MIN_SATS_FOR_FIX;
    gpsMsgData.requiredMaxHdop = GPS_DEFAULT_MAX_HDOP_FOR_FIX;
    gpsMsgData.requiredMinTimeInSeconds = GPS_DEFAULT_MIN_SECONDS_FOR_FIX;
}

/**
* \brief GPS MSG executive. Called every 2 seconds from the main
*        loop to manage the GPS message processing.
*
* \ingroup EXEC_ROUTINE
*/
void gpsMsg_exec(void)
{

    // If we are NOT currently active - just return.
    if (!gpsMsgData.busy)
    {
        return;
    }

    if (gpsMsgData.ggaMsgFromIsrReady)
    {
        gpsMsgData.ggaMsgFromIsrReady = false;
        gpsMSg_processGgaSentence();

#ifdef GPS_DEBUG
        if (!gpsMsgData.busy)
        {
		    // display the current data that we have
            gps_debug_minmea_summary((uint8_t *)gpsRxBuf, gpsMsgData.gpsFixIsValid);
        }
#endif
    }

    if (!gpsMsgData.ggaMsgAvailable &&
        GET_ELAPSED_TIME_IN_SEC(gpsMsgData.waitForGgaTimestamp) > MAX_GGA_MSG_WAIT_TIME_IN_SEC)
    {
        gpsMsgData.noGgaMsgError = true;
    }

}

/**
* \brief Start GPS message processing
* 
* @return bool Returns true if message processing was started. 
*         Returns false if the processing had already been
*         started.
* 
* \ingroup PUBLIC_API
*/
bool gpsMsg_start(void)
{

    // Return false if already busy.
    if (gpsMsgData.busy)
    {
        return (false);
    }

    // Mark module as busy
    gpsMsgData.busy = true;

    // Start processing messages from the GPS
    gpsMsg_isrRestart();

    return (true);
}

/**
* \brief Stop GPS message processing
* 
* \ingroup PUBLIC_API
*/
void gpsMsg_stop(void)
{
    disable_UART_tx();
    disable_UART_rx();
    gpsMsgData.busy = false;
}

/**
* \brief Get the message processing status
* 
* @return bool Returns true if message processing is in 
*         progress. False otherwise.
* 
* \ingroup PUBLIC_API
*/
bool gpsMsg_isActive(void)
{
    return (gpsMsgData.busy);
}

/**
* \brief Get the message processing error status
* 
* @return bool Returns true if a timeout occured waiting for a 
*         message from the GPS. False otherwise.
* 
* \ingroup PUBLIC_API
*/
bool gpsMsg_isError(void)
{
    return (gpsMsgData.noGgaMsgError);
}

/**
* \brief Identify if there is a GGA message available.
* 
* @return bool True if GGA message is available. False 
*         otherwise.
* 
* \ingroup PUBLIC_API
*/
bool gpsMsg_gotGgaMessage(void)
{
    return (gpsMsgData.ggaMsgAvailable);
}

/**
* \brief Get the status of whether the GGA message that is 
*        available contains a valid satellite fix.
* 
* @return bool True if valid satellite fix. False otherwise.
* 
* \ingroup PUBLIC_API
*/
bool gpsMsg_gotValidGpsFix(void)
{
    return (gpsMsgData.gpsFixIsValid);
}

/**
* \brief Retrieve GPS data to send to the IoT platform. The last
*        parsed GGA message received is sitting in the frame
*        object.
*  
* The data is returned as follows: 
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
* @param bufP Buffer to store the data in
* 
* \ingroup PUBLIC_API
* 
* @return uint8_t Number of bytes stored in the buffer
*/
uint8_t gpsMsg_getGgaParsedData(uint8_t *bufP)
{
    uint16_t temp16;
    uint32_t temp32;

    gpsReportData_t *reportP = (gpsReportData_t *)bufP;

    reportP->hours = gpsMsgData.frame.time.hours;
    reportP->minutes = gpsMsgData.frame.time.minutes;

    temp32 = gpsMsgData.frame.latitude.value;
    reverseEndian32(&temp32);
    reportP->latitude = temp32;

    temp32 = gpsMsgData.frame.longitude.value;
    reverseEndian32(&temp32);
    reportP->longitude = temp32;

    reportP->fixQuality = gpsMsgData.frame.fix_quality;
    reportP->numOfSats = gpsMsgData.frame.satellites_tracked;
    reportP->hdop = gpsMsgData.frame.hdop.value;

    temp16 = gpsPower_getGpsOnTimeInSecs();
    reverseEndian16(&temp16);
    reportP->fixTimeInSecs = temp16;

    reportP->reserved = 0;

    return (sizeof(gpsReportData_t));
}

/**
* \brief Set the GPS measurement criteria used to qualify a 
*        measurement fix.
* 
* @param numSats Required minimum number of satellites
* @param hdop Max allowed HDOP value (x10)
* @param minMeasTime Minimum on time in seconds
*/
void gpsMsg_setMeasCriteria(uint8_t numSats, uint8_t hdop, uint16_t minMeasTime)
{
    gpsMsgData.requiredNumSats = numSats;
    gpsMsgData.requiredMaxHdop = hdop;
    gpsMsgData.requiredMinTimeInSeconds = minMeasTime;
}

/*************************
 * Module Private Functions
 ************************/

/**
* \brief Helper function to setup parameters and enable UART 
*        hardware interrupts to start a tx/rx transaction.
*/
static void gpsMsg_isrRestart(void)
{
    uint8_t __attribute__((unused)) garbage;

    // For safety, disable UART interrupts
    disable_UART_rx();
    disable_UART_tx();

    gpsMsgData.noGgaMsgError = false;
    gpsMsgData.ggaMsgAvailable = false;
    gpsMsgData.ggaMsgLength = 0;
    gpsMsgData.gpsFixIsValid = false;
    gpsMsgData.isrRxIndex = 0;
    gpsMsgData.isrGotStart$ = false;
    gpsMsgData.ggaMsgFromIsrReady = false;

    memset(gpsRxBuf, 0, GPS_RX_BUF_SIZE);

    // Clear out the UART receive buffer
    garbage = UCA0RXBUF;

    // Get time that we start waiting for GGA message
    gpsMsgData.waitForGgaTimestamp = GET_SYSTEM_TICK();

    // Enable interrupts
    enable_UART_rx();
    enable_UART_tx();
}

static void gpsMsg_checkForGoodFix(void)
{

#ifdef SIMULATE_GPS_FIX
    //*******************************************************
    // For Test Only!!!! - Simulate RMC String
    static uint8_t testCount = 0;

    if (++testCount == 5)
    {
        testCount = 0;
        memcpy(gpsRxBuf, ggaTestString, sizeof(ggaTestString));
        gpsMsgData.isrRxIndex = sizeof(ggaTestString);
        gpsMsgData.ggaMsgLength = sizeof(ggaTestString);
    }
    //*******************************************************
#endif

    if ((gpsPower_getGpsOnTimeInSecs() > gpsMsgData.requiredMinTimeInSeconds) &&
        gpsMsgData.frame.fix_quality &&
        (gpsMsgData.frame.satellites_tracked >= gpsMsgData.requiredNumSats) &&
        (gpsMsgData.frame.hdop.value < gpsMsgData.requiredMaxHdop))
    {
        gpsMsgData.gpsFixIsValid = true;
    }
}

/**
* \brief Process a received GGA sentence. Verify the GGA message
*        against its checksum. If the checksum matches, then we
*        perform parsing and look for a good fix. If not
*        verified, restart the sequence to receive another GGA
*        message.
*/
static void gpsMSg_processGgaSentence(void)
{
    bool parseOk = false;

    if (gpsMsg_verifyChecksum())
    {
        gpsMsgData.ggaMsgAvailable = true;
        gpsMsgData.ggaMsgLength = gpsMsgData.isrRxIndex;
        parseOk = minmea_parse_gga(&gpsMsgData.frame, gpsRxBuf);
        if (parseOk)
        {
            // Run until we receive one GGA message.
            gpsMsgData.busy = false;
            // Check if we have a good fix
            gpsMsg_checkForGoodFix();
        }
    }
    else
    {
        // Keep running until we receive a GGA message
#ifdef GPS_DEBUG
        gps_debug_message("[GPS processGga=FailCksum]\n");
#endif
        gpsMsg_isrRestart();
    }
}

/**
* \brief Interrupt subroutine called when a character is 
*        received by the UART.
* 
* @param rxByte 
*/
void gpsMsg_isr(void)
{

    uint8_t rxByte = UCA0RXBUF;

    // Look for the start of a new NMEA sentence.
    if (rxByte == '$')
    {
        gpsMsgData.isrRxIndex = 0;
        gpsMsgData.isrGotStart$ = true;
    }

    // Continue processing the sentence if we found the starting delimiter.
    if (gpsMsgData.isrGotStart$)
    {

        // Add byte to the receive buffer
        if (gpsMsgData.isrRxIndex < GPS_RX_BUF_SIZE)
        {
            gpsRxBuf[gpsMsgData.isrRxIndex] = rxByte;
            gpsMsgData.isrRxIndex++;
        }

        // Check for LF character which marks the end of the NMEA sentence.
        if (rxByte == 0xA)
        {
            // Reset flag
            gpsMsgData.isrGotStart$ = false;
            // Check if we might have a GGA message
            if (gpsMsg_matchGga())
            {
                // Disable interrutps and process the received NMEA sentence.
                disable_UART_rx();
                // Set flag to indicate GGA message is ready
                gpsMsgData.ggaMsgFromIsrReady = true;
            }
        }
    }
}

/**
* \brief Look for $GNGGA in the beginning of a NMEA message.
* 
* \notes Assumes an NMEA sentence is located in the rxBuf. 
* 
* @return bool Return true if found, false other wise.
*/
static bool gpsMsg_matchGga(void)
{
    int i;
    bool match = true;

    for (i = 0; i < sizeof(gga_match_template); i++)
    {
        if (gpsRxBuf[i] != gga_match_template[i])
        {
            match = false;
        }
    }
    return (match);
}

/**
* \brief Verify the checksum value in the NMEA sentence. The 
*        checksum field is an 8-bit exclusive OR of all the
*        bytes between the $ and the * (not including the
*        delimiters themselves). The checksum is calculated and
*        compared against the received value.
* 
* \notes Assumes the NMEA sentence is located in the rxBuf. 
*        Assumes the gps_data.index represents the length of the
*        NMEA sentence located in the rxBuf.
* 
* @return bool true if match, false otherwise.
*/
static bool gpsMsg_verifyChecksum(void)
{
    int i;
    uint8_t calculatedChecksum = 0;
    uint8_t rxChecksumMSN = 0;
    uint8_t rxChecksumLSN = 0;
    uint8_t rxChecksum = 0;

    // Verify that we received a minimal length and the asterisk before the checksum.
    // The '*' (0x2A) will be at five characters from the end.
    // An example end of sentence: 0x2A,0x37,0x33,0xD,0xA
    if ((gpsMsgData.isrRxIndex < 20) || (gpsRxBuf[gpsMsgData.isrRxIndex - 5] != '*'))
    {
        return (false);
    }

    // Calculate the checksum over the NMEA sentence.
    // Exclude the first '$' and calculate up to the '*'.
    for (i = 1; i < (gpsMsgData.isrRxIndex - 5); i++)
    {
        calculatedChecksum ^= gpsRxBuf[i];
    }

    // Get the received checksum value as a hex number
    // The Most significant nibble of the checksum is 4 characters from the end.
    // The least significant nibble of the checksum is 3 characters from the end.
    rxChecksumMSN = asciiToHex(gpsRxBuf[gpsMsgData.isrRxIndex - 4]);
    rxChecksumLSN = asciiToHex(gpsRxBuf[gpsMsgData.isrRxIndex - 3]);
    rxChecksum = (rxChecksumMSN << 4) | rxChecksumLSN;

    return (calculatedChecksum == rxChecksum);
}

/**
* \brief Perform an ASCII to hex conversion for one ASCII 
*        character. Assumes the character is a hex number in
*        ASCII form. Returns the hex number.
*  
* \note Assumes the ASCII number will only be in uppercase form 
*       (A-F).
* 
* @param asciiByte An ASCII byte representing one hex number.
*  
* For reference - ASCII values 
* \li A = 65, 0x41
* \li G = 71, 0x47
* 
* @return uint8_t The hex number from the ASCII byte.
*/
static uint8_t asciiToHex(uint8_t asciiByte)
{
    uint8_t val = 0;

    if (asciiByte > 0x2F && asciiByte < 0x40)
    {
        val = asciiByte - 0x30;
    }
    else if (asciiByte > 0x40 && asciiByte < 0x47)
    {
        val = 10 + (asciiByte - 0x41);
    }
    return (val);
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

