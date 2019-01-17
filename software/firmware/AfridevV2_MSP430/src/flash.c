/** 
 * @file flash.c
 * \n Source File
 * \n Outpour MSP430 Firmware
 * 
 * \brief Flash support routines
 */

#include "outpour.h"

/**
* \brief Erase one segment of flash (one 512 byte area in flash)
* \ingroup PUBLIC_API
* 
* @param flashSegmentAddrP 
*/
void msp430Flash_erase_segment(uint8_t *flashSegmentAddrP)
{
    volatile uint16_t ms_check_count;
    volatile uint8_t contextSaveSR;
    volatile uint8_t checkCount = 0;

    contextSaveSR = __get_SR_register();

    // Clear GIE
    __bic_SR_register(GIE);

    FCTL2 = (FWKEY | FSSEL_1 | FN1);                       // Select clock
    FCTL3 = FWKEY;                                         // Clear Lock bit
    FCTL1 = (FWKEY | ERASE);                               // Set Erase bit

    *flashSegmentAddrP = 0;                                // dummy write

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

    FCTL1 = (FWKEY | LOCK);                                // Set LOCK bit

    // If the GIE was set, restore it.
    if (contextSaveSR & GIE)
    {
        __bis_SR_register(GIE);
    }
}

/**
* 
* \brief Write data bytes to flash
* \ingroup PUBLIC_API
* 
* @param flashP  starting flash addr to write to
* @param srcP starting addr where data is read from
* @param num_bytes number of bytes to write
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

    FCTL2 = (FWKEY | FSSEL_1 | FN1);                       // Set up clock
    FCTL3 = FWKEY;                                         // Clear Lock bit
    FCTL1 = (FWKEY | WRT);                                 // Enable write

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

    FCTL1 = FWKEY;                                         // Clear WRT
    FCTL1 = (FWKEY | LOCK);                                // Set Lock

    // If the GIE was set, restore it.
    if (contextSaveSR & GIE)
    {
        __bis_SR_register(GIE);
    }
}

/**
* 
* \brief Write one 16 bit value to flash.  Write with the 
*        correct endian-ness for transmit to the server (MSB
*        first).
* \ingroup PUBLIC_API
* 
* @param flashP  starting flash addr to write to 
* @param val16 16 bit value to write
*/
void msp430Flash_write_int16(uint8_t *flashP, uint16_t val16)
{
    uint8_t bytes[2];

    bytes[0] = (val16 >> 8) & 0xff;
    bytes[1] = val16 & 0xff;
    msp430Flash_write_bytes(flashP, &bytes[0], ((uint16_t)2));
}

/**
* 
* \brief Write one 32 bit value to flash.  Write with the 
*        correct endian-ness for transmit to the server (MSB
*        first).
* \ingroup PUBLIC_API
* 
* @param flashP  starting flash addr to write to 
* @param val32 32 bit value to write
*/
void msp430Flash_write_int32(uint8_t *flashP, uint32_t val32)
{
    uint8_t bytes[4];

    bytes[0] = (val32 >> 24) & 0xff;
    bytes[1] = (val32 >> 16) & 0xff;
    bytes[2] = (val32 >> 8) & 0xff;
    bytes[3] = val32 & 0xff;
    msp430Flash_write_bytes(flashP, &bytes[0], ((uint16_t)4));
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
