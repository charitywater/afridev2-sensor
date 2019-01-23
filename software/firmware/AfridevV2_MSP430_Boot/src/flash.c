/** 
 * @file flash.c
 * \n Source File
 * \n AfridevV2 MSP430 Bootloader Firmware
 * 
 * \brief Flash support routines
 */

#include "outpour.h"
#include "linkAddr.h"

/***************************
 * Module Data Definitions
 **************************/

/***************************
 * Module Data Declarations
 **************************/

/**************************
 * Module Prototypes
 **************************/
static void msp430Flash_eraseAppImage(void);
static void msp420Flash_copyBackupToApp(void);
static bool msp430Flash_doesAppMatchBackup(void);

/***************************
 * Module Public Functions 
 **************************/
/**
* \brief Erase one segment of flash (one 512 byte area in flash)
* 
* @param flashSegmentAddrP 
*
* \ingroup PUBLIC_API
*/
void msp430Flash_erase_segment(uint8_t *flashSegmentAddrP) 
{
    volatile uint16_t ms_check_count;
    volatile uint8_t contextSaveSR;
    volatile uint8_t checkCount = 0;
    contextSaveSR = __get_SR_register();

    // Clear GIE
    __bic_SR_register(GIE);

    FCTL2 = (FWKEY | FSSEL_1 | FN1);  // Select clock
    FCTL3 = FWKEY;                    // Clear Lock bit
    FCTL1 = (FWKEY | ERASE);          // Set Erase bit

    *flashSegmentAddrP = 0;    // dummy write

    /*
     * From the MSP40 Documentation
     * When a byte or word write or any erase operation is initiated 
     * from within flash memory, the flash controller returns op-code 
     * 03FFFh to the CPU at the next instruction fetch. Op-code 03FFFh 
     * is the JMP PC instruction. This causes the CPU to loop until the 
     * flash operation is finished. When the operation is finished and BUSY = 0, 
     * the flash controller allows the CPU to fetch the proper op-code and program
     * execution resumes.
     *
     * Based on the above, the while loop will never be entered below.   But its 
     * kept around just to be safe? 
     *
     * note - from empirical lab testing, it takes ~14.52ms to
     * erase one segment.
     */
    ms_check_count = 0;
    while (FCTL3 & BUSY) 
	{
        // rough loop delay of 1ms (assumes operating @ 1MHZ Clock);
        _delay_cycles(1000);
        ms_check_count++;
        // Check for timeout: ~100MS (1ms*100)
        if (ms_check_count > 100) 
		{
            break;
        }
    }

    FCTL1 = (FWKEY | LOCK);         // Set LOCK bit

    // If the GIE was set, restore it.
    if (contextSaveSR & GIE) 
	{
        __bis_SR_register(GIE);
    }
}

/**
* 
* \brief Write data bytes to flash
* 
* @param flashP  starting flash addr to write to
* @param srcP starting addr where data is read from
* @param num_bytes number of bytes to write
*
* \ingroup PUBLIC_API
*/
void msp430Flash_write_bytes(uint8_t *flashP, uint8_t *srcP, uint16_t num_bytes) 
{
    volatile uint16_t i;
    volatile uint16_t us100_check_count;
    volatile uint8_t contextSaveSR;
    volatile uint8_t checkCount = 0;
    contextSaveSR = __get_SR_register();

    // Clear GIE
    __bic_SR_register(GIE);

    FCTL2 = (FWKEY | FSSEL_1 | FN1);  // Set up clock
    FCTL3 = FWKEY;                    // Clear Lock bit
    FCTL1 = (FWKEY | WRT);            // Enable write

    // Write each byte
    us100_check_count = 0;
    for (i = 0; i < num_bytes; i++) 
	{
        us100_check_count = 0;

        *flashP++ = *srcP++;
        /*
         * From the MSP40 Documentation
         * When a byte or word write or any erase operation is initiated 
         * from within flash memory, the flash controller returns op-code 
         * 03FFFh to the CPU at the next instruction fetch. Op-code 03FFFh 
         * is the JMP PC instruction. This causes the CPU to loop until the 
         * flash operation is finished. When the operation is finished and BUSY = 0, 
         * the flash controller allows the CPU to fetch the proper op-code and program
         * execution resumes.
         *
         * Based on the above, the while loop will never be entered below.   But its 
         * kept around just to be safe? 
         * 
         * note - from empirical lab testing, it takes ~.15125ms to
         * program each byte.
         */
        while (FCTL3 & BUSY) 
		{
            // rough loop delay of 100us (assumes operating @ 1MHZ Clock);
            _delay_cycles(100);
            us100_check_count++;
            // Check for timeout: ~10MS (100us*100)
            if (us100_check_count > 100) 
			{
                break;
            }
        }
    }

    FCTL1 = FWKEY;                 // Clear WRT
    FCTL1 = (FWKEY | LOCK);        // Set Lock

    // If the GIE was set, restore it.
    if (contextSaveSR & GIE) 
	{
        __bis_SR_register(GIE);
    }
}

/**
* \brief Move the image that is located in the flash backup
*        image location to the flash main image location.  This
*        is to support a firmware upgrade.   List of steps
*        performed:
*
* \li  Access backup image information in the application
*      record.
* \li  Verify the backup image against the CRC stored in the
*      application record.
* \li  Erase the flash that contains the main image.
* \li  Copy the backup image to the main image.
* \li  Verify the main image against the CRC stored in the
*      applciation record.
* 
* @return bool  Returns true if successful.
*
* \ingroup PUBLIC_API
*/
fwCopyResult_t msp430Flash_moveAndVerifyBackupToApp(void) 
{
    bool backupImageIsValid = false;
    uint16_t calcCrc = 0;
    uint16_t storedCrc = 0;
    bool backupImageExists = false;
    bool copySuccess = false;
    fwCopyResult_t fwCopyResult = FW_COPY_SUCCESS;

    // Read backup image info from app record
    appRecord_getNewFirmwareInfo(&backupImageExists, &storedCrc);

    // Double check backup crc if there is a backup image
    if (backupImageExists) 
	{
        // Perform CRC
        const unsigned char *dataP = (const unsigned char *)getBackupImageStartAddr();
        uint16_t imageLength = getAppImageLength();
        calcCrc = gen_crc16(dataP, imageLength);
        if (calcCrc == storedCrc) 
		{
            backupImageIsValid = true;
        } 
		else 
		{
            fwCopyResult = FW_COPY_ERR_BAD_BACKUP_CRC;
        }
    } 
	else 
	{
        fwCopyResult = FW_COPY_ERR_NO_BACKUP_IMAGE;
    }

    // If back image is valid, perform steps to move backup image to main image
    // location.
    if (backupImageIsValid) 
	{
        uint8_t retryCount = 0;
        // zero the APP reset vector before proceeding.  This is how we know
        // there is not a valid main image in case anything happens (i.e. reboot).
        msp430Flash_zeroAppResetVector();
        do 
		{
            // Erase main image
            msp430Flash_eraseAppImage();
            // Copy the Backup flash image to the App flash area
            msp420Flash_copyBackupToApp();
            // Verify that backup matches app
            copySuccess = msp430Flash_doesAppMatchBackup();
        } while (!copySuccess && ++retryCount < 4);

        if (copySuccess) 
		{
            // Final check - perform CRC on new App image
            const unsigned char *dataP = (const unsigned char *)getAppImageStartAddr();
            uint16_t imageLength = getAppImageLength();
            calcCrc = gen_crc16(dataP, imageLength);
            if (calcCrc != storedCrc) 
			{
                copySuccess = false;
                fwCopyResult = FW_COPY_ERR_BAD_MAIN_CRC;
            }
        } 
		else 
		{
            fwCopyResult = FW_COPY_ERR_COPY_FAILED;
        }
    }

    return fwCopyResult;
}

/**
 * \brief Use the linker symbol to identify where to zero 
 *  the application reset vector in flash.  Write zero to the
 *  reset vector.  We should always be able to zero it
 *  regardless of whether it is erased (since a flash write is
 *  always a 1 -> 0 operation).  This function is used during a
 *  firmware upgrade prior to erasing the main image in flash to
 *  assure any attemp to use the app reset vector during the
 *  upgrade sequence will result in a reboot.
 *
 * \ingroup PUBLIC_API
*/
void msp430Flash_zeroAppResetVector(void) 
{
    void (*tempP)(void) = 0;
    msp430Flash_write_bytes((uint8_t *)&__App_Reset_Vector, (uint8_t *)&tempP, sizeof(void (*)(void)));
}

/***************************
 * Module Private Functions 
 ***************************/

/**
* \brief Erase the main image in flash.
*/
static void msp430Flash_eraseAppImage(void) 
{
    uint8_t i;
    // Get number of sectors in the backup flash
    uint16_t numSectors = getNumSectorsInImage();
    // Get start address
    uint8_t *flashSegmentAddrP = (uint8_t *)getAppImageStartAddr();
    // For protection, get boot address to check against
    uint8_t *bootFlashStartAddrP = (uint8_t *)getBootImageStartAddr();
    // Erase the backup flash segments
    for (i = 0; i < numSectors; i++, flashSegmentAddrP += 0x200) 
	{
        // Double check that the address falls below the bootloader flash area
        if (flashSegmentAddrP < bootFlashStartAddrP) 
		{
            // Tickle the watchdog before erasing
            WATCHDOG_TICKLE();
            msp430Flash_erase_segment(flashSegmentAddrP);
        }
    }
}

/**
* \brief Copy the backup image in flash to the main image 
*        location in flash.
*/
static void msp420Flash_copyBackupToApp(void) 
{
    uint8_t i;
    // Get number of sectors in the backup flash
    uint16_t numSectors = getNumSectorsInImage();
    // Get addresses
    uint8_t *flashSrcAddrP = (uint8_t *)getBackupImageStartAddr();
    uint8_t *flashDstAddrP = (uint8_t *)getAppImageStartAddr();
    uint8_t *checkAddrP = (uint8_t *)getBootImageStartAddr();
    // Copy the flash segments
    for (i = 0; i < numSectors; i++, flashDstAddrP += 0x200, flashSrcAddrP += 0x200) 
	{
        // Double check that we won't write over the bootloader
        if ((flashDstAddrP + 0x1FF) < checkAddrP) 
		{
            // Tickle the watchdog before writing to flash
            WATCHDOG_TICKLE();
            msp430Flash_write_bytes(flashDstAddrP, flashSrcAddrP, 0x200);
        }
    }
}

/**
* \brief Compare the backup image contents in flash to the main 
*        image contents in flash.
* 
* @return bool Returns true if the backup image equals the main 
*         image.
*/
static bool msp430Flash_doesAppMatchBackup(void) 
{
    volatile int result = 0;
    uint8_t i;
    uint16_t numSectors = getNumSectorsInImage();
    uint8_t *flashSrcAddrP = (uint8_t *)getBackupImageStartAddr();
    uint8_t *flashDstAddrP = (uint8_t *)getAppImageStartAddr();
    // Compare the flash segments
    for (i = 0; i < numSectors; i++, flashDstAddrP += 0x200, flashSrcAddrP += 0x200) 
	{
        // Tickle the watchdog before comparing flash segment.
        WATCHDOG_TICKLE();
        result += memcmp(flashDstAddrP, flashSrcAddrP, 0x200);
    }
    // If result is zero, then match was successfull.
    // So return true.  False otherwise.
    return (result == 0 ? true : false);
}

/*******************************************************************************
*  FLASH TESTING
*******************************************************************************/
#if 0
extern uint8_t isrCommBuf[48];
void msp430flash_test(void)
{
    uint16_t i = 0;
    uint16_t j = 0;
    uint16_t val;
    uint8_t *bufP = isrCommBuf;
    uint8_t *baseAddr = ((uint8_t *)0xC000);
    uint8_t *addrP;
    uint16_t *addr16P;

#if 0
    P1DIR |= BIT3;
    P1OUT &= ~BIT3;
    P1DIR |= BIT4;
    P1OUT &= ~BIT4;
#endif

    while (1)
    {

        baseAddr = ((uint8_t *)0xC000);
        msp430Flash_erase_segment(baseAddr);
        // VERIFY ALL FF's
        addr16P = (uint16_t *)baseAddr;
        for (j = 0; j < 256; j++, addr16P++)
        {
            val = *addr16P;
            if (val != ((uint16_t)0xFFFF))
            {
                while (1);
            }
        }
        // WRITE
        addrP = baseAddr;
        for (j = 0; j < 512; j += 32, addrP += 32)
        {
            for (i = 0; i < 32; i += 2)
            {
                val = ((uint16_t)addrP) + i;
                bufP[i] = val & 0xFF;
                bufP[i+1] = val >> 8;
            }
            msp430Flash_write_bytes(addrP, bufP, 32);
        }

        baseAddr = ((uint8_t *)0xC200);
        msp430Flash_erase_segment(baseAddr);
        // VERIFY ALL FF's
        addr16P = (uint16_t *)baseAddr;
        for (j = 0; j < 256; j++, addr16P++)
        {
            val = *addr16P;
            if (val != ((uint16_t)0xFFFF))
            {
                while (1);
            }
        }
        // WRITE
        addrP = baseAddr;
        for (j = 0; j < 512; j += 32, addrP += 32)
        {
            for (i = 0; i < 32; i += 2)
            {
                val = ((uint16_t)addrP) + i;
                bufP[i] = val & 0xFF;
                bufP[i+1] = val >> 8;
            }
            msp430Flash_write_bytes(addrP, bufP, 32);
        }

        baseAddr = ((uint8_t *)0xC400);
        msp430Flash_erase_segment(baseAddr);
        // VERIFY ALL FF's
        addr16P = (uint16_t *)baseAddr;
        for (j = 0; j < 256; j++, addr16P++)
        {
            val = *addr16P;
            if (val != ((uint16_t)0xFFFF))
            {
                while (1);
            }
        }
        // WRITE
        addrP = baseAddr;
        for (j = 0; j < 512; j += 32, addrP += 32)
        {
            for (i = 0; i < 32; i += 2)
            {
                val = ((uint16_t)addrP) + i;
                bufP[i] = val & 0xFF;
                bufP[i+1] = val >> 8;
            }
            msp430Flash_write_bytes(addrP, bufP, 32);
        }
        baseAddr = ((uint8_t *)0xC600);
        msp430Flash_erase_segment(baseAddr);
        // VERIFY ALL FF's
        addr16P = (uint16_t *)baseAddr;
        for (j = 0; j < 256; j++, addr16P++)
        {
            val = *addr16P;
            if (val != ((uint16_t)0xFFFF))
            {
                while (1);
            }
        }
        // WRITE
        addrP = baseAddr;
        for (j = 0; j < 512; j += 32, addrP += 32)
        {
            for (i = 0; i < 32; i += 2)
            {
                val = ((uint16_t)addrP) + i;
                bufP[i] = val & 0xFF;
                bufP[i+1] = val >> 8;
            }
            msp430Flash_write_bytes(addrP, bufP, 32);
        }

        // VERIFY ALL
        baseAddr = ((uint8_t *)0xC000);
        uint16_t *addr16P = (uint16_t *)baseAddr;
        for (j = 0; j < 1024; j++, addr16P++)
        {
            val = *addr16P;
            if (val != (((uint16_t)addr16P)))
            {
                while (1);
            }
        }
        _delay_cycles(1000000);
    }
}
#endif
