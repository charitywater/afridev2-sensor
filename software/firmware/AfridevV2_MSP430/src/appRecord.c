/** 
 * @file appRecord.c
 * \n Source File
 * \n Cascade MSP430 Firmware
 * 
 * \brief Support functions to read/write the Application Record
 *        which is stored in INFO section C.
 */

#include "outpour.h"

/***************************
 * Module Data Definitions
 **************************/

/**
 * \def APR_LOCATION
 * \brief Where in flash the app record is located. It is 
 *        located in the Flash INFO C section.
 */
#define APR_LOCATION ((uint8_t *)0x1040)  // INFO C

/**
 * \def PARAM_MAGIC
 * \brief Used as a known pattern to perform a "quick" verify 
 *        that the data structure is initialized in flash.
 */
#define APR_MAGIC ((uint16_t)0x5678)

/**
 * \def APR_VERSION 
 * \brief Version number of structure format.
 */
#define APR_VERSION ((uint16_t)0x0)

/**
 * \typedef appRecord_t
 * \brief This structure is used to put an application record in 
 *        one of the INFO sections.  The structure is used to
 *        tell the bootloader that the application has
 *        successfully started and also to contain info on
 *        whether a new firmware image was received by the app
 *        and stored into the backup image location.
 */
typedef struct appRecord_e {
    uint16_t magic;                                        /**< A known pattern for "quick" test of structure validity */
    uint16_t recordLength;                                 /**< Length of structure */
    uint16_t version;                                      /**< Version of structure format */
    uint16_t newFwReady;                                   /**< Parameter */
    uint16_t newFwCrc;                                     /**< Parameter */
    uint16_t crc16;                                        /**< Used to validate the data */
} appRecord_t;

/***************************
 * Module Data Declarations
 **************************/

/**************************
 * Module Prototypes
 **************************/

/***************************
 * Module Public Functions 
 **************************/

/**
* \brief Erase the APP record.  Erases the complete INFO C 
*        section.
* \ingroup PUBLIC_API
*/
void appRecord_erase(void)
{
    msp430Flash_erase_segment(APR_LOCATION);
}

/**
* \brief Write a fresh Application Record in the INFO C section.
*        The record is written by the Application and used by
*        the bootloader to identify that the Application started
*        successfully.
* 
* @return bool Returns true if initialization was successful.
*
* \ingroup PUBLIC_API
*/
bool appRecord_initAppRecord(void)
{
    appRecord_t appRecord;
    uint8_t retryCount = 0;
    bool error = false;

    // Retry up to four times to write the App Record.
    do
    {
        // Zero RAM version and update its parameters
        memset(&appRecord, 0, sizeof(appRecord_t));
        appRecord.magic = APR_MAGIC;
        appRecord.recordLength = sizeof(appRecord_t);
        appRecord.version = APR_VERSION;
        appRecord.crc16 = gen_crc16((const unsigned char *)&appRecord, (sizeof(appRecord_t) - sizeof(uint16_t)));

        // Erase flash version
        msp430Flash_erase_segment(APR_LOCATION);
        // Write RAM version to flash version
        msp430Flash_write_bytes(APR_LOCATION, (uint8_t *)&appRecord, sizeof(appRecord_t));

        // Final check
        if (!appRecord_checkForValidAppRecord())
        {
            error = true;
        }
        else
        {
            error = false;
        }

    } while (error && (retryCount++ < 4));

    return (!error);
}

/**
* \brief Determine if a valid Application Record is located in 
*        the INFO C section.  The record is written by the
*        Application and used by the bootloader to identify that
*        the Application started successfully.  The recordLength
*        variable is used to identify where the CRC is located.
*        This allows the structure to grow and have elements
*        added, and the bootloader will still be able to verify
*        its contents.  But the original elements must remain in
*        the structure at their current defined location (except
*        for the CRC location which always at the end - 2).
* 
* @return bool Returns true if the record is valid.  False 
*         otherwise.
*
* \ingroup PUBLIC_API
*/
bool appRecord_checkForValidAppRecord(void)
{
    bool aprFlag = false;
    appRecord_t *aprP = (appRecord_t *)APR_LOCATION;

    if (aprP->magic == APR_MAGIC)
    {
        // Locate the offset to the CRC in the structure based on the
        // stored record length of the appRecord. This is to ensure
        // backward compatibility with future versions.
        uint8_t crcOffset = aprP->recordLength - sizeof(uint16_t);
        // Calculate CRC
        uint16_t calcCrc = gen_crc16(APR_LOCATION, crcOffset);
        // Access stored CRC based on stored record length, not structure element
        // because structure may be added to in the future.
        uint16_t *storedCrcP = (uint16_t *)(((uint8_t *)aprP) + crcOffset);
        uint16_t storedCrc = *storedCrcP;

        // Compare calculated CRC to the CRC stored in the structure
        if (calcCrc == storedCrc)
        {
            aprFlag = true;
        }
    }
    return (aprFlag);
}

/**
* \brief Returns the newFwAvailable flag value located in the 
*        appRecord.  If the newFwAvailable flag is set to true,
*        then it means there is upgrade firmware located in the
*        backup image location.
* 
* @return bool Returns true or false.
*
* \ingroup PUBLIC_API
*/
bool appRecord_checkForNewFirmware(void)
{
    bool newFwAvailable = false;

    // Check for valid record available
    if (appRecord_checkForValidAppRecord())
    {
        appRecord_t *aprP = (appRecord_t *)APR_LOCATION;

        if (aprP->newFwReady)
        {
            newFwAvailable = true;
        }
    }
    return (newFwAvailable);
}

/**
* \brief Returns the three variables that contain new firmware 
*        image information.
* 
* @param readyP  Pointer that will be filled in.
* @param crcP Pointer that will be filled in.
* 
* @return bool Returns true if the appRecord is valid and the 
*         new Fw image flag is set.
*
* \ingroup PUBLIC_API
*/
bool appRecord_getNewFirmwareInfo(bool *newFwReadyP, uint16_t *newFwCrcP)
{
    bool newFwAvailable = false;

    // Check for valid record available
    if (appRecord_checkForValidAppRecord())
    {
        appRecord_t *aprP = (appRecord_t *)APR_LOCATION;

        if (aprP->newFwReady)
        {
            newFwAvailable = true;
            *newFwReadyP = aprP->newFwReady;
            *newFwCrcP = aprP->newFwCrc;
        }
        else
        {
            newFwAvailable = false;
            *newFwReadyP = false;
            *newFwCrcP = 0;
        }
    }
    return (newFwAvailable);
}

/**
* \brief Update the new firmware parameters in the application 
*        record.  This is filled in by the OTA message
*        processing after a new firmware image has been stored
*        to the backup image flash location.
*  
* \note Currently this will completely erase the existing
*       appRecored structure stored in the INFO section and
*       overwrite it with a new copy, regardless of what values
*       were stored.  That is, is does not try to read the
*       original values out before overwriting it.
* 
* @param newFwIsReady  True/False flag
* @param crc CRC of the new FW
* 
* @return bool Returns true if storage of parameters into the 
*         application record was successful.
*
* \ingroup PUBLIC_API
*/
bool appRecord_updateFwInfo(bool newFwIsReady, uint16_t newFwCrc)
{
    appRecord_t appRecord;
    uint8_t retryCount = 0;
    bool error = false;

    // Retry up to four times to write the App Record.
    do
    {
        // Create fresh RAM version
        memset(&appRecord, 0, sizeof(appRecord_t));
        appRecord.magic = APR_MAGIC;
        appRecord.recordLength = sizeof(appRecord_t);
        appRecord.version = APR_VERSION;
        appRecord.newFwReady = newFwIsReady;
        appRecord.newFwCrc = newFwCrc;
        // Calculate CRC on RAM version
        appRecord.crc16 = gen_crc16((const unsigned char *)&appRecord, (sizeof(appRecord_t) - sizeof(uint16_t)));
        // Erase Flash Version
        msp430Flash_erase_segment(APR_LOCATION);
        // Copy RAM version to Flash version
        msp430Flash_write_bytes(APR_LOCATION, (uint8_t *)&appRecord, sizeof(appRecord_t));
        // Final check
        if (!appRecord_checkForValidAppRecord())
        {
            error = true;
        }
        else
        {
            error = false;
        }
    } while (error && (retryCount++ < 4));
    return (!error);
}

#if 0
/**
* \brief For testing appRecord functions.
*/
void appRecord_test(void)
{
    appRecord_t *aprP = (appRecord_t *)APR_LOCATION;
    bool result = false;
    bool ready = false;
    uint16_t crc;
    bool newFwIsReady = false;
    uint16_t newFwCrc;

    result = appRecord_checkForValidAppRecord();
    if (result)
    {
        result = appRecord_getNewFirmwareInfo(&newFwIsReady, &newFwCrc);
    }

    result = appRecord_initAppRecord();
    result = appRecord_checkForValidAppRecord();

    ready = true;
    crc = 0x1234;
    result = appRecord_updateFwInfo(ready, crc);

    // while (1) {
    //     WATCHDOG_TICKLE();
    // }

    result = appRecord_checkForNewFirmware();
    result = appRecord_getNewFirmwareInfo(&newFwIsReady, &newFwCrc);

    result = appRecord_updateFwInfo(0, 0);
    result = appRecord_checkForNewFirmware();
    result = appRecord_getNewFirmwareInfo(&newFwIsReady, &newFwCrc);

    crc = 0;
    msp430Flash_write_bytes((uint8_t *)&aprP->crc16, (uint8_t *)&crc, 2);
    result = appRecord_checkForValidAppRecord();
}
#endif
