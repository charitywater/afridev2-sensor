/** 
 * @file msgData.c
 * \n Source File
 * \n Outpour MSP430 Firmware
 *
 * \brief This is the central UART RX isr that is shared between
 *        the Modem and the GPS. Only one is active at any one
 *        time.
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
