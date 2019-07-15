/** 
 * @file hal.c
 * \n Source File
 * \n AfridevV2 MSP430 Firmware
 * 
 * \brief Hal support routines
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
    // Pin 1.3, _1V8_EN, output,  controls (U5) (Active HIGH)
    // Pin 1.4, GSM_INT, input, SIM900 interrupt out
    // Pin 1.5, GSM_STATUS, input, SIM900 status out
    // Pin 1.6, TM_GPS, input, GPS Time Pulse
    // Pin 1.7, GPS_ON_IND, input, GPS System ON (inverted)

    P1DIR = (uint8_t) ~(VBAT_GND + GSM_INT + GSM_STATUS + TM_GPS + GPS_ON_IND); //all inputs except 1.2,1.3
    P1DIR |= _1V8_EN + GSM_DCDC;              // Output
    P1OUT = 0;                                // All P1.x reset
    P1REN &= (uint8_t) ~(VBAT_GND + GSM_INT + GSM_STATUS + TM_GPS + GPS_ON_IND); // disable internal pullup resisitor

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

    P2SEL |= BIT6 + BIT7;                     // 32.768 kHz crystal input and output pins
    P2DIR = ~(VBAT_MON+I2C_DRV+BIT6);         // set as input, VBAT_MON, I2C_DRV and 32.768 kHz crystal input, the rest outputs
    // Note: BIT7 is for crystal XOUT on P2.7
    P2DIR |= GSM_EN + LS_VCC + BIT7;
    P2OUT = 0;                                // All P2.x reset
    P2REN &= ~(VBAT_MON+I2C_DRV+BIT6);        // disable internal pullup resisitor

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

    // Setup I/O for UART
    P3SEL |= RXD + TXD;                       // enable UART
    P3DIR = 0xFF;
    P3OUT = LED_GREEN + LED_RED;

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

    P4DIR = ~NTC_SENSE_INPUT;                 // All P4.x outputs except NTC_SENSE_INPUT
    P4OUT = 0;                                // All P4.x reset

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

/**
* \brief Blinks the red_led every other time it is called
*/
void hal_led_blink_red(void)
{

    if (!led_state)
    {
        LED_RED_ENABLE();
        led_state = 1;
    }
    else
    {
        LED_RED_DISABLE();
        led_state = 0;
    }
}

/**
* \brief Toggle the indicator LEDs from Red to Green and Green to Red
* \ingroup PUBLIC_API
*/
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

/**
* \brief Set the indicator LEDs to Green on and Red off
* \ingroup PUBLIC_API
*/
void hal_led_green(void)
{
    LED_GREEN_ENABLE();
    led_state = 0;
}

/**
* \brief Set the indicator LEDs to Red on and Green off
* \ingroup PUBLIC_API
*/
void hal_led_red(void)
{
    LED_RED_ENABLE();
    led_state = 0;
}

/**
* \brief Turn both indicator LEDs off
* \ingroup PUBLIC_API
*/
void hal_led_none(void)
{
    LED_GREEN_DISABLE();
    LED_RED_DISABLE();
    led_state = 0;
}

/**
* \brief Turn both indicator LEDs on
* \ingroup PUBLIC_API
*/
void hal_led_both(void)
{
    LED_GREEN_ENABLE();
    LED_RED_ENABLE();
    led_state = 0;
}


void hal_low_power_enter(void)
{
    // ultra-low_power sleep
    __bis_SR_register(LPM3_bits + GIE);
}
