/** 
 * @file time.c
 * \n Source File
 * \n AfridevV2 MSP430 Firmware
 * 
 * \brief MSP430 timer control and support routines
 */

/*
*  Note - the timer naming convention is very confusing on the MSP430
*
*  For the MSP430G2953, there are three timers on the system:  TimerA0, TimerA1
*    & TimerB0.  Each timer has three capture & control channels: 0,1 & 2.
* 
*  Timer0_A3 and Timer1_A3 are 16-bit timers/counters with three capture/compare registers.
*  Timer0_B3 is a 16-bit timer/counter with three capture/compare registers.
* 
*  Naming Usage:
*  TimerA0_0, Timer A0, capture control channel 0
*  TimerA0_1, Timer A0, capture control channel 1
*  TimerA0_2, Timer A0, capture control channel 2
*  TimerA1_0, Timer A1, capture control channel 0
*  TimerA1_1, Timer A1, capture control channel 1
*  TimerA1_2, Timer A1, capture control channel 2
*  TimerB0_0, Timer B1, capture control channel 0
*  TimerB0_1, Timer B1, capture control channel 1
*  TimerB0_2, Timer B1, capture control channel 2
*
*  Timer allocation/usage for AfridevV2 APPLICATION:
*    TimerA0_0, Capture control channel 0 used for system tick (with ISR, vector = 9 @ 0FFF2h)
*    TimerA1_0, Capture control channel 0 used with capacitance reading
*    TimerB0_0, Capture control channel 0 used to time the capacitance reading (with ISR, vector = 12 @ 0FFFAh)
*
*  Timer allocation/usage for AfridevV2 BOOT:
*    TimerA1_0, Capture control channel 0 used for system tick (with ISR, vector = 1 @ 0FFE2h)
* 
* The interrupt vector naming corresponds as follows:
*  TIMER1_A1_VECTOR, ".int00", 0xFFE0, Timer1_A3, CCR1-2, TA1 
*  TIMER1_A0_VECTOR, ".int01", 0xFFE2, Timer1_A3, CCR0 
*  TIMER0_A1_VECTOR, ".int07", 0xFFF0, Timer0_A3, CCR1-2, TA0 
*  TIMER0_A0_VECTOR, ".int08", 0xFFF2, Timer0_A3, CCR0 
*  TIMERB1_VECTOR,   ".int11", 0xFFF8, Timer0_B3, CCR1-2, TB0 
*  TIMERB0_VECTOR,   ".int12", 0xFFFA, Timer0_B3, CCR0 
* =============================================================================== 
* From the MSP430 Documentation:
* 
* Up mode operation 
* The timer repeatedly counts up to the value of compare register TACCR0, which defines the period.
* The number of timer counts in the period is TACCR0+1. When the timer value equals TACCR0 the timer
* restarts counting from zero. If up mode is selected when the timer value is greater than TACCR0, the
* timer immediately restarts counting from zero.
* =============================================================================== 
* Timer Registers for Timer A0 and A1  (x = 0 or 1 accordingly)
*  TAxCTL - reg to setup timer counting type, clock, etc - CONTROLS ALL OF TIMER A1
*  Capture/Control channel 0
*  TAxCCR0 - specify the compare count for channel 0
*  TAxCCTL0 - setup interrupt for channel 0 
*  Capture/Control channel 1
*  TAxCCR1 - specify the compare count for channel 1
*  TAxCCTL1 - setup interrupt for channel 1
*/

#include "outpour.h"
#include "RTC_Calendar.h"

/**
* \var seconds_since_boot
* \brief Declare seconds since boot.  Incremented by timer 
*        interrupt routine.
*/
static volatile uint32_t seconds_since_boot;

/**
* \var ticks_per_second
* \brief This is a tick counter incremented by the timer ISR. It
*        is used to track when one second worth of ticks has
*        occurred.
*/
static volatile uint8_t ticks_per_second;

/**
* \brief Initialize and start timerA0 for the one second system 
*        tick.  Uses Timer A0, capture/control channel 0, vector
*        9, 0xFFF2. Initialize to use ACLK which is running at
*        32768 HZ (from external crystal).
* \ingroup PUBLIC_API
*/
void timerA0_init(void)
{

    TA0CCR0 = 16384 - 1;                                   // 32786Hz/16384=2HZ

    TA0CTL = TASSEL_1 + MC_1 + TACLR;
    TA0CCTL0 &= ~CCIFG;
    TA0CCTL0 |= CCIE;

    ticks_per_second = 0;
    seconds_since_boot = 0;
}

/**
* \brief Retrieve the seconds since boot system value.
* \ingroup PUBLIC_API
* 
* @return uint32_t 32 bit values representing seconds since the 
*         system booted.
*/
uint32_t getSecondsSinceBoot(void)
{
    return (seconds_since_boot);
}

/**
* \brief Timer ISR. Produces the 2HZ system tick interrupt.
*        Uses Timer A0, capture/control channel 0, vector 9,
*        0xFFF2
* \ingroup ISR
*/
#ifndef FOR_USE_WITH_BOOTLOADER
#pragma vector=TIMER0_A0_VECTOR
#endif
__interrupt void ISR_Timer0_A0(void)
{
    //  testing
    //  dbg_line[0]='#';
    //  dbg_uart_write(dbg_line, 1);

    TA0CTL |= TACLR;

    // Increment on every interrupt
    ++ticks_per_second;

    // Check if we have reached one second
    if (ticks_per_second >= TIMER_INTERRUPTS_PER_SECOND)
    {
        // Increment the TI calendar library
        incrementSeconds();
        // Increment the seconds counter
        seconds_since_boot++;
        // Zero
        ticks_per_second = 0;
    }

    // DDL Notes - Clear the low power mode bits on exit so that the MSP430 will come out of sleep
    // on the exit of the ISR.  GIE should already be set on exit.
    __bic_SR_register_on_exit(LPM3_bits);

}

/**
* \brief Utility function to get the time from the TI calender 
*        module and convert to binary format.
* \note Only tens is returned for year (i.e. 15 for 2015, 16 for
*       2016, etc.)
* 
* @param tp Pointer to a time packet structure to fill in with 
*           the time.
*/
void getBinTime(timePacket_t *tpP)
{
    uint16_t mask;

    mask = getAndDisableSysTimerInterrupt();
    tpP->second = bcd_to_char(TI_second);
    tpP->minute = bcd_to_char(TI_minute);
    tpP->hour24 = bcd_to_char(get24Hour());
    tpP->day = bcd_to_char(TI_day);
    tpP->month = bcd_to_char(TI_month) + 1;                // correcting from RTC lib's 0-indexed month
    tpP->year = bcd_to_char((TI_year)&0xFF);
    restoreSysTimerInterrupt(mask);
}

/**
 * \brief Utility function to convert a byte of bcd data to a 
 *        binary byte.  BCD data represents a value of 0-99,
 *        where each nibble of the input byte is one decimal
 *        digit.
 * 
 * @param bcdValue Decimal value, each nibble is a digit of 0-9.
 * 
 * @return uint8_t binary conversion of the BCD value.
 *  
 */
uint8_t bcd_to_char(uint8_t bcdValue)
{
    uint8_t tenHex = (bcdValue >> 4) & 0x0f;
    uint8_t tens = (((tenHex << 2) + (tenHex)) << 1);
    uint8_t ones = bcdValue & 0x0f;

    return (tens + ones);
}

