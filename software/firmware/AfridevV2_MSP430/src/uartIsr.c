/** 
 * @file msgData.c
 * \n Source File
 * \n Outpour MSP430 Firmware
 *
 * \brief This is the central UART RX isr that is shared between
 *        the Modem and the GPS. Only one is active at any one
 *        time.
 */

#include "outpour.h"

/**
* \brief This is the central UART RX interrupt service routine 
*        that is shared between the Modem and the GPS. Only one
*        is active at a time.
*
* \ingroup ISR
*/
#ifndef FOR_USE_WITH_BOOTLOADER
#pragma vector=USCIAB0RX_VECTOR
#endif
__interrupt void USCI0RX_ISR(void)
{
#ifndef GPS_DEBUG
    if (modemMgr_isAllocated())
    {
        modemCmd_isr();
    }
    if (gpsMsg_isActive())
    {
#endif
        gpsMsg_isr();
#ifndef GPS_DEBUG
    }
#endif
}
