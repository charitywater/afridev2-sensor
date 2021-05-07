/** 
 * @file hal.c
 * \n Source File
 * \n AfridevV2 MSP430 Firmware
 * 
 * \brief Hal support routines
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

uint8_t led_state;

/*******************************************************************************
*
*  MSP430 GPIO Info
*
*  For ports:
*
*  PxDIR: Direction Registers PxDIR
*  Each bit in each PxDIR register selects the direction of the corresponding I/O
*  pin, regardless of the selected function for the pin. PxDIR bits for I/O pins that
*  are selected for other functions must be set as required by the other function.
*    Bit = 0: The port pin is switched to input direction
*    Bit = 1: The port pin is switched to output direction
*
*  PxOUT:
*  Each bit in each PxOUT register is the value to be output on
*  the corresponding I/O pin when the pin is configured as I/O function,
*  output direction, and the pull-up/down resistor is disabled.
*    Bit = 0: The output is low
*    Bit = 1: The output is high
*  If the pin’s pull-up/down resistor is enabled, the corresponding bit
*  in the PxOUT register selects pull-up or pull-down.
*    Bit = 0: The pin is pulled down
*    Bit = 1: The pin is pulled up
*
*  PxREN:  Pull-Up/Down Resistor Enable Registers
*  Each bit in each PxREN register enables or disables the pullup/pulldown
*  resistor of the corresponding I/O pin. The corresponding bit in the PxOUT
*  register selects if the pin is pulled up or pulled down.
*     Bit = 0: Pullup/pulldown resistor disabled
*     Bit = 1: Pullup/pulldown resistor enabled
*
*  PxSEL: PxSEL and PxSEL2 bit are used to select the pin function -
*         I/O port or peripheral module function (00 = I/O Port).
*
******************************************************************************/

/**
* \brief One time init of all GPIO port pin related items after 
*        boot up.
* \ingroup PUBLIC_API
*/
void hal_pinInit(void)
{

    /***********/
    /* PORT P1 */
    /**********/

    // Pin 1.0, No Connect
    // Pin 1.1, VBAT_GND, output, controls VBAT sensing, (Active LOW)
    // Pin 1.2, GSM_DCDC, output, controls (U4) (Active HIGH)
    // Pin 1.3, _1V8_EN, output,  controls (U5) (Active HIGH)
    // Pin 1.4, GSM_INT, input, SIM900 interrupt out
    // Pin 1.5, GSM_STATUS, input, SIM900 status out
    // Pin 1.6, TM_GPS, input, GPS Time Pulse
    // Pin 1.7, GPS_ON_IND, input, GPS System ON (inverted)

    // Set P1 Direction - Outputs
    P1DIR |= VBAT_GND + _1V8_EN + GSM_DCDC;                // Output
                                                           // Set P1 Direction - Inputs
    P1DIR &= ~(GSM_INT + GSM_STATUS + TM_GPS + GPS_ON_IND);
    // Initalize P1 Outputs
    P1OUT &= ~(GSM_DCDC | _1V8_EN);
    P1OUT |= VBAT_GND;

    /***********/
    /* PORT P2 */
    /**********/

    // Pin 2.0, VBAT_MON, input, ADC
    // Pin 2.1, PAD1
    // Pin 2.2, PAD2
    // Pin 2.3, I2C_DRV, output, I2C Pull-up, Not Used
    // Pin 2.4, GSM_EN,  output, for modem (Active HIGH)
    // Pin 2.5, LS_VCC,  output, for modem (Active HIGH)
    // Pin 2.6, Crystal XIN, input
    // Pin 2.7, Crystal XOUT, output

    // configure pins for crystal on P2.6,P2.7
    P2SEL |= BIT6 + BIT7;
    // Set P2 Direction - Output
    // Note: BIT7 is for crystal XOUT on P2.7
    P2DIR |= I2C_DRV + GSM_EN + LS_VCC + BIT7;
    // Set P2 Direction - Input
    // Note: BIT6 is for crystal XIN on P2.6
    P2DIR &= ~(BIT6);
    // Initialize P2 Outputs
    P2OUT &= ~(I2C_DRV + GSM_EN + LS_VCC);

    /***********/
    /* PORT P3 */
    /**********/

    // Pin 3.0, PAD5
    // Pin 3.1, LED Green
    // Pin 3.2, LED Red
    // Pin 3.3, NTC_ENABLE, Output, Temperature Meas Enable
    // Pin 3.4, UART TXD, Special
    // Pin 3.5, UART RX, Special
    // Pin 3.7, MSP_UART_SEL, Output, Cell=0,GPS=1

#ifdef USE_UART_SIGNALS_FOR_GPIO
    // If using UART signals for timing GPIO outputs
    P3DIR |= RXD + TXD + NTC_ENABLE + LED_GREEN + LED_RED;
#else
    // Setup I/O for UART
    P3SEL |= RXD + TXD;
#endif
    // Set P3 Direction - Output
    P3DIR |= NTC_ENABLE + MSP_UART_SEL + LED_GREEN + LED_RED;
    // Initialize P3 Outputs
    P3OUT &= ~(NTC_ENABLE + MSP_UART_SEL + LED_GREEN + LED_RED);
    P3OUT |= NTC_ENABLE + LED_GREEN + LED_RED;

    /***********/
    /* PORT P4 */
    /**********/

    // Pin 4.0, NO CONNECTION
    // Pin 4.1, NO CONNECTION
    // Pin 4.2, GPS_ON_OFF
    // Pin 4.3, NTC_SENSE_INPUT
    // Pin 4.4, NO CONNECTION
    // Pin 4.5, PAD4
    // Pin 4.6, PAD2
    // Pin 4.7, PAD0

    // Set P4 Direction - Outputs
    P4DIR |= GPS_ON_OFF;
    // Set P4 Direction - Inputs
    P4DIR &= ~(NTC_SENSE_INPUT);
    // Initialize P4 Outputs
    P4OUT &= ~(GPS_ON_OFF);

    led_state = 0;
}

/**
* \brief One time init of the UART subsystem after boot up.
* \ingroup PUBLIC_API
*/
void hal_uartInit(void)
{
    //  ACLK source for UART
    UCA0CTL1 |= UCSSEL_1;                                  // ACLK
    UCA0BR0 = 0x03;                                        // 32 kHz 9600
    UCA0BR1 = 0x00;                                        // 32 kHz 9600
    UCA0MCTL = UCBRS0 + UCBRS1;                            // Modulation UCBRSx = 3
    UCA0CTL1 &= ~UCSWRST;                                  // **Initialize USCI state machine**
}

/**
* \brief One time init of all clock related subsystems after 
*        boot up.
* \ingroup PUBLIC_API
*/
void hal_sysClockInit(void)
{
    DCOCTL = 0;                                            // Select lowest DCOx and MODx settings
    BCSCTL1 = CALBC1_1MHZ;                                 // Set DCO
    DCOCTL = CALDCO_1MHZ;

    BCSCTL1 |= DIVA_0;                                     // ACLK/1 [ACLK/(0:1,1:2,2:4,3:8)]
    BCSCTL2 = 0;                                           // SMCLK [SMCLK/(0:1,1:2,2:4,3:8)]
    BCSCTL3 |= LFXT1S_0;                                   // LFXT1S0 32768-Hz crystal on LFXT1
}


void hal_led_toggle(void)
{

    if (!led_state)
    {
        LED_GREEN_DISABLE();
        LED_RED_ENABLE();
        led_state = 1;
    }
    else
    {
        LED_GREEN_ENABLE();
        LED_RED_DISABLE();
        led_state = 0;
    }
}

void hal_led_green(void)
{ 
    LED_GREEN_ENABLE();
    LED_RED_DISABLE();
    led_state = 0;
}

void hal_led_red(void)
{
    LED_GREEN_DISABLE();
    LED_RED_ENABLE();
    led_state = 0;
}

void hal_led_none(void)
{
    LED_GREEN_DISABLE();
    LED_RED_DISABLE();
    led_state = 0;
}
