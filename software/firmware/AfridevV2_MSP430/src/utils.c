/** 
 * @file utils.c
 * \n Source File
 * \n AfridevV2 MSP430 Firmware
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
* \ingroup PUBLIC_API
* 
* @param data Pointer to the data buffer to calculate the CRC
*             over
* @param size Length of the data in bytes to calculate over.
* 
* @return unsigned int The CRC calculated value
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

/**
* \brief Utility function to calculate a 16 bit CRC on data 
*        located in two buffers. The first buffer is most likely
*        the opcode buffer, and the second buffer is the data
*        buffer.
* 
* \ingroup PUBLIC_API
* 
* @param data1 Pointer to first buffer
* @param size1 Size of data in bytes for first buffer
* @param data2 Pointer to data in second buffer
* @param size2 Size of data in bytes for second buffer
* 
* @return unsigned int The calculated CRC value
*/
unsigned int gen_crc16_2buf(const unsigned char *data1, unsigned int size1, const unsigned char *data2, unsigned int size2)
{
    volatile unsigned int out = 0;
    volatile int bits_read = 0;
    volatile int bit_flag;

    while (size1 > 0)
    {
        bit_flag = out >> 15;
        /* Get next bit: */
        out <<= 1;
        out |= (*data1 >> bits_read) & 1;                  // item a) work from the least significant bits
        /* Increment bit counter: */
        bits_read++;
        if (bits_read > 7)
        {
            bits_read = 0;
            data1++;
            size1--;
            WATCHDOG_TICKLE();
        }
        /* Cycle check: */
        if (bit_flag)
            out ^= CRC16;
    }

    while (size2 > 0)
    {
        bit_flag = out >> 15;

        /* Get next bit: */
        out <<= 1;
        out |= (*data2 >> bits_read) & 1;                  // item a) work from the least significant bits

        /* Increment bit counter: */
        bits_read++;
        if (bits_read > 7)
        {
            bits_read = 0;
            data2++;
            size2--;
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

/**
* \brief Reverse the byte order of a 32 bit value. Result is put 
*        into the pointer passed in (i.e. done in place).
* 
* \ingroup PUBLIC_API
* 
* @param valP The 32 bit value to byte reverse
*/
void reverseEndian32(uint32_t *valP)
{
    uint32_t inVal = *valP;
    char *inValP = (char *)&inVal;
    char *outValP = (char *)valP;

    outValP[0] = inValP[3];
    outValP[1] = inValP[2];
    outValP[2] = inValP[1];
    outValP[3] = inValP[0];
}

/**
* \brief Reverse the byte order of a 16 bit value. Result is put
*        into the pointer passed in (i.e. done in place).
* 
* \ingroup PUBLIC_API
* 
* @param valP The 16 bit value to byte reverse
*/
void reverseEndian16(uint16_t *valP)
{
    uint16_t inVal = *valP;
    char *inValP = (char *)&inVal;
    char *outValP = (char *)valP;

    outValP[0] = inValP[1];
    outValP[1] = inValP[0];
}
