/** 
 * @file bootRecord.c
 * \n Source File
 * \n AfridevV2 MSP430 Bootloader Firmware
 * 
 * \brief Routines to support boot and application information 
 *        stored in the INFO-B and INFO-C Sections of flash.
 */

// Includes
#include "outpour.h"

/***************************
 * Module Data Definitions
 **************************/

/***************************
 * Module Data Declarations
 **************************/

/**
* \var ramBlr
* \brief Scratch structure for intermediate updates to the
*        BootRecord structure.
*/
static bootloaderRecord_t ramBlr;

/**
* \brief Write a fresh bootloader record data structure to the 
*        INFO flash location.  The count is set to zero.
* \ingroup PUBLIC_API
*/
void bootRecord_initBootloaderRecord(void)
{
    // init the local RAM structure.
    memset(&ramBlr, 0, sizeof(bootloaderRecord_t));
    ramBlr.magic = BLR_MAGIC;
    ramBlr.crc16 = gen_crc16((uint8_t *)&ramBlr, (sizeof(bootloaderRecord_t) - sizeof(uint16_t)));
    msp430Flash_erase_segment(BLR_LOCATION);
    msp430Flash_write_bytes(BLR_LOCATION, (uint8_t *)&ramBlr, sizeof(bootloaderRecord_t));
}

/**
* \brief Return the count located in the Bootloader Record.  The 
*        bootloader record is a data structure located at INFO B
*        section (Flash address 0x1080).  The structure contains
*        a counter that identifies the number of boot attempts
*        since a valid application last ran.
* \ingroup PUBLIC_API
* 
* @return int The count in the BLR data structure.  If the data
*         structure is invalid, it returns -1.
*/
int bootRecord_getBootloaderRecordCount(void)
{
    int blrCount = -1;
    bootloaderRecord_t *blrP = (bootloaderRecord_t *)BLR_LOCATION;
    unsigned int crc16 = gen_crc16(BLR_LOCATION, (sizeof(bootloaderRecord_t) - sizeof(uint16_t)));

    // Verify the CRC16 in the data structure.
    if (crc16 == blrP->crc16)
    {
        blrCount = blrP->bootRetryCount;
    }
    return (blrCount);
}

/**
* \brief Increment the count variable in the bootloader data 
*        structure located in the flash INFO section.
* \ingroup PUBLIC_API
*/
void bootRecord_incrementBootloaderRecordCount(void)
{
    memcpy(&ramBlr, BLR_LOCATION, sizeof(bootloaderRecord_t));
    ramBlr.bootRetryCount++;
    ramBlr.crc16 = gen_crc16((uint8_t *)&ramBlr, (sizeof(bootloaderRecord_t) - sizeof(uint16_t)));
    msp430Flash_erase_segment(BLR_LOCATION);
    msp430Flash_write_bytes(BLR_LOCATION, (uint8_t *)&ramBlr, sizeof(bootloaderRecord_t));
}

/**
* \brief Add debug information to the bootloader data structure 
*        located in the flash INFO section.
* \ingroup PUBLIC_API
*/
void bootRecord_addDebugInfo(void)
{
    memcpy(&ramBlr, BLR_LOCATION, sizeof(bootloaderRecord_t));
    ramBlr.rebootMagic = 0x5AA5;
    ramBlr.networkErrorCount += (modemMgr_isLinkUpError() ? 1 : 0);
    ramBlr.modemShutdownTick = modemMgr_getShutdownTick();
    ramBlr.crc16 = gen_crc16((uint8_t *)&ramBlr, (sizeof(bootloaderRecord_t) - sizeof(uint16_t)));
    msp430Flash_erase_segment(BLR_LOCATION);
    msp430Flash_write_bytes(BLR_LOCATION, (uint8_t *)&ramBlr, sizeof(bootloaderRecord_t));
}

/**
* \brief Copy the bootRecord data structure into the buffer 
*        passed in.
* \ingroup PUBLIC_API
* 
* @param bufP Buffer to copy bootRecord into
* 
* @return int Number of bytes copied into buffer
*/
int bootRecord_copy(uint8_t *bufP)
{
    memcpy(bufP, &ramBlr, sizeof(bootloaderRecord_t));
    return (sizeof(bootloaderRecord_t));
}

