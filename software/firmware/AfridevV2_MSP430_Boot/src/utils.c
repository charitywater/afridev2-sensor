/** 
 * @file utils.c
 * \n Source File
 * \n AfridevV2 MSP430 Bootloader Firmware
 * 
 * \brief Miscellaneous support functions.
 */

#include "outpour.h"

/**
 * \def CRC16
 * \brief The CRC-16-ANSI polynomial constant.
 */
#define CRC16 0x8005

/**
* \brief Utility function to calculate a 16 bit CRC on data in a
*        buffer.
* 
* @param data Pointer to the data buffer to calculate the CRC
*             over
* @param size Length of the data in bytes to calculate over.
* 
* @return unsigned int The CRC calculated value
*
* \ingroup PUBLIC_API
*/
unsigned int gen_crc16(const unsigned char *data, unsigned int size)
{
    volatile unsigned int out = 0;
    volatile int bits_read = 0;
    volatile int bit_flag;

    while (size > 0)
    {
        bit_flag = out >> 15;
        /* Get next bit: */
        out <<= 1;
        out |= (*data >> bits_read) & 1;                   // item a) work from the least significant bits
        /* Increment bit counter: */
        bits_read++;
        if (bits_read > 7)
        {
            bits_read = 0;
            data++;
            size--;
            WATCHDOG_TICKLE();
        }
        /* Cycle check: */
        if (bit_flag)
            out ^= CRC16;
    }

    // item b) "push out" the last 16 bits
    unsigned int i;

    for (i = 0; i < 16; ++i)
    {
        bit_flag = out >> 15;
        out <<= 1;
        if (bit_flag)
            out ^= CRC16;
    }

    // item c) reverse the bits
    unsigned int crc = 0;

    i = 0x8000;
    unsigned int j = 0x0001;

    for (; i != 0; i >>= 1, j <<= 1)
    {
        if (i & out)
            crc |= j;
    }
    return (crc);
}

#if 0
/**
* \brief Stop the watchdog timer 
* \ingroup PUBLIC_API
*/
void WATCHDOG_STOP(void)
{
    WDTCTL = WDTPW | WDTHOLD;
}

/**
* \brief Re-start the watchdog timer
* \ingroup PUBLIC_API
*/
void WATCHDOG_TICKLE(void)
{
    WDTCTL = WDT_ARST_1000;
}
#endif

#if 0
uint8_t crc16TestData[9] =
{ 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };
// uint8_t crc16TestData[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
// uint8_t crc16TestData[10] = { 0xFF, 0xFE, 0xFD, 0xFC, 0xFB, 0xFA, 0xF9, 0xF8, 0xF7, 0xF6 };
unsigned int crc16TestResult;
void crc16Test(void)
{
    crc16TestResult = gen_crc16(crc16TestData, sizeof(crc16TestData));
}
#endif
