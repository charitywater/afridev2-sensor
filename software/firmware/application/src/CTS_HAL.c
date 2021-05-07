/* --COPYRIGHT--,BSD
 * Copyright (c) 2012, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
/*!
 *
 *  @file CTS_HAL.c
 *
 *
 * @brief This file contains the source for the different implementations.
 *
 *
 * @par Project:
 *             MSP430 Capacitive Touch Library
 *
 *
 *
 * @par Developed using:
 *             CCS Version : 5.4.0.00048, w/support for GCC extensions (--gcc)
 * \n IAR Version : 5.51.6 [Kickstart]
 *
 *
 * @author C. Sterzik
 * @author T. Hwang
 *
 *
 * @version 1.2 Added HALs for new devices.
 *
 *
 * @par Supported Hardware Implementations:
 *              - TI_CTS_RO_COMPAp_TA0_WDTp_HAL()
 *              - TI_CTS_fRO_COMPAp_TA0_SW_HAL()
 *              - TI_CTS_fRO_COMPAp_SW_TA0_HAL()
 *              - TI_CTS_RO_COMPAp_TA1_WDTp_HAL()
 *              - TI_CTS_fRO_COMPAp_TA1_SW_HAL()
 *              - TI_CTS_RC_PAIR_TA0_HAL()
 *              - TI_CTS_RO_PINOSC_TA0_WDTp_HAL()
 *              - TI_CTS_RO_PINOSC_TA0_HAL()
 *              - TI_CTS_fRO_PINOSC_TA0_SW_HAL()
 *              - TI_CTS_RO_COMPB_TA0_WDTA_HAL()
 *              - TI_CTS_RO_COMPB_TA1_WDTA_HAL()
 *              - TI_CTS_fRO_COMPB_TA0_SW_HAL()
 *              - TI_CTS_fRO_COMPB_TA1_SW_HAL()
 *              - Added in version 1.1
 *              - TI_CTS_fRO_PINOSC_TA0_TA1_HAL()
 *              - TI_CTS_RO_PINOSC_TA0_TA1_HAL()
 *              - TI_CTS_RO_CSIO_TA2_WDTA_HAL()
 *              - TI_CTS_RO_CSIO_TA2_TA3_HAL()
 *              - TI_CTS_fRO_CSIO_TA2_TA3_HAL()
 *              - TI_CTS_RO_COMPB_TB0_WDTA_HAL()
 *              - TI_CTS_RO_COMPB_TA1_TA0_HAL()
 *              - TI_CTS_fRO_COMPB_TA1_TA0_HAL()
 *              - Added in version 1.2
 *              - TI_CTS_RO_PINOSC_TA1_WDTp_HAL()
 *              - TI_CTS_RO_PINOSC_TA1_TB0_HAL()
 *              - TI_CTS_fRO_PINOSC_TA1_TA0_HAL()
 *              - TI_CTS_fRO_PINOSC_TA1_TB0_HAL()
 *
 *
 */

#include "CTS_HAL.h"
#include "outpour.h"

// This variable keeps track of active capsense, in case we try to process a
// non-watchdog ISR. We want to make sure the chip goes back to sleep instead
// of waking up and recording counts early
uint8_t CAPSENSE_ACTIVE;

#ifdef RO_PINOSC_TA1_TB0
/*!
 *
 *  ======== TI_CTS_RO_PINOSC_TA1_TB0_HAL ========
 * @brief RO method capacitance measurement using PinOsc IO, TimerA1, and
 *           TimerB0
 *
 *
 * \n Schematic Description:
 *
 *
 * \n element-----+->Px.y
 *
 *
 * \n The TimerB0 interval represents the gate (measurement) time. The
 *           number of oscillations that have accumulated in TA1R during the
 *           measurement time represents the capacitance of the element.
 *
 *
 * @param group  pointer to the sensor to be measured
 * @param counts pointer to where the measurements are to be written
 * @return none
 *
 *
 *  Additional Details:
 *
 *
 *  This driver works by connecting timerA1 to a capacitive element, and then
 *  counting how many counts that timer counts within the set TimerB0 interval.
 *  We enter Low Power Mode while counting, and the Timer B0 wakes us up
 *  and tells us to record the total counts.
 */
void TI_CTS_RO_PINOSC_TA1_TB0_HAL(const struct Sensor *group, uint16_t *counts)
{
    uint8_t i = 0;

    /*!
     *
     *  Allocate Context Save Variables
     *  Status Register: GIE bit only
     *  TIMERA0: TA1CTL, TA1CCTL1, TA1CCR1
     *  TIMERB1: TB0CTL, TB0CCTL0, TB0CCR0
     *  Ports: PxSEL, PxSEL2
     */
    uint8_t contextSaveSR;
    uint16_t contextSaveTA1CTL, contextSaveTA1CCTL1, contextSaveTA1CCR1;
    uint16_t contextSaveTB0CTL, contextSaveTB0CCTL0, contextSaveTB0CCR0;
    uint8_t contextSaveSel, contextSaveSel2;

    /*
     *  Perform context save of registers used except port registers which are
     *  saved and restored within the for loop as each element within the
     *  sensor is measured.
     */
    contextSaveSR = __get_SR_register();
    contextSaveTA1CTL = TA1CTL;
    contextSaveTA1CCTL1 = TA1CCTL1;
    contextSaveTA1CCR1 = TA1CCR1;
    contextSaveTB0CTL = TB0CTL;
    contextSaveTB0CCTL0 = TB0CCTL0;
    contextSaveTB0CCR0 = TB0CCR0;

    CAPSENSE_ACTIVE = 1;

    //** Setup Measurement timer***************************************************
    // Choices are TA0,TA1,TB0,TB1,TD0,TD1 these choices are pushed up into the
    // capacitive touch layer.

    // Configure and Start Timer
    TA1CTL = TASSEL_3;                                     // + MC_2;              // INCLK, cont mode
                                                           // TA0CCTL1 = CM_3 + CCIS_2 + CAP;         // Pos&Neg,GND,Cap

    /*
     *  TimerB0 is the gate (measurement interval) timer.  The number of
     *  oscillations counted within the gate interval represents the measured
     *  capacitance.
     */
    TB0CCR0 = (group->accumulationCycles);
    // Establish source and scale of timerA1, but halt the timer.
    TB0CTL = group->measGateSource + group->sourceScale;
    TB0CCTL0 = CCIE;                                       // Enable Interrupt when timer counts to TB0CCR0.

    // Loop for each defined element
    for (i = 0; i < (group->numElements); i++)
    {

        // Context Save Port Registers
        contextSaveSel = *((group->arrayPtr[i])->inputPxselRegister);
        contextSaveSel2 = *((group->arrayPtr[i])->inputPxsel2Register);

        // Configure Ports for relaxation oscillator
        *((group->arrayPtr[i])->inputPxselRegister) &= ~((group->arrayPtr[i])->inputBits);
        *((group->arrayPtr[i])->inputPxsel2Register) |= ((group->arrayPtr[i])->inputBits);

        TA1CTL |= (MC_2);                                  // Clear Timer_A TAR
        TA1R = 0;
        TA1CTL &= ~TAIFG;

        // Clear and start Gate Timer
        TB0CTL |= (TACLR + MC_1);

        /*!
         *
         *  The measGateSource represents the gate source for timer TIMERA1,
         *  which can be sourced from TACLK, ACLK, SMCLK, or INCLK. The
         *  interrupt handler is defined in TIMER1_A0_VECTOR, which simply
         *  clears the low power mode bits in the Status Register before
         *  returning from the ISR.
         */
        if (group->measGateSource == TIMER_ACLK)
        {
            __bis_SR_register(LPM3_bits + GIE);            // Enable GIE and wait for ISR
        }
        else
        {
            __bis_SR_register(LPM0_bits + GIE);            // Enable GIE and wait for ISR
        }

        // every other pad the reference signal is alternated between GND and VCC
        // (this causes rising and falling trigger events)
        // this causes a transition that starts the calculation of capacitance
        TA1CTL &= ~MC_2;                                   // Stop Timer_A TAR
        TB0CTL &= ~MC_1;                                   // Halt Timer_B
        if (TA1CTL & TAIFG)
        {
            // check for timer overflow
            counts[i] = 0xFFFF;
        }
        else
        {
            counts[i] = TA1R;                              // Save result, used to be from TA1CCR1 ^= CCIS0;
        }

        // Context Restore
        *((group->arrayPtr[i])->inputPxselRegister) = contextSaveSel;
        *((group->arrayPtr[i])->inputPxsel2Register) = contextSaveSel2;
    }

    /*
     *  Context restore GIE within Status Register and all timer registers
     *  used.
     */
    if (!(contextSaveSR & GIE))
    {
        __bic_SR_register(GIE);
    }
    TA1CTL = contextSaveTA1CTL;
    TA1CCTL1 = contextSaveTA1CCTL1;
    TA1CCR1 = contextSaveTA1CCR1;
    TB0CTL = contextSaveTB0CTL;
    TB0CCTL0 = contextSaveTB0CCTL0;
    TB0CCR0 = contextSaveTB0CCR0;

    CAPSENSE_ACTIVE = 0;
}
#endif

#ifdef TIMERB0_GATE
/*!
 *
 *  ======== TIMER0_B0_ISR ========
 * @ingroup ISR_GROUP
 * @brief TIMER0_B0_ISR
 *
 *
 *  This ISR clears the LPM bits found in the Status Register (SR/R2).
 *
 *
 * @param none
 * @return none
 */
#ifndef FOR_USE_WITH_BOOTLOADER
#pragma vector=TIMERB0_VECTOR
#endif
__interrupt void TIMERB0_ISR(void)
{
    __bic_SR_register_on_exit(LPM3_bits);                  // Exit LPM3 on reti
}
#endif
