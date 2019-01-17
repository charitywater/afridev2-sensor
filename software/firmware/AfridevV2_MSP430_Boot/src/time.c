/** 
 * @file time.c
 * \n Source File
 * \n AfridevV2 MSP430 Bootloader Firmware
 * 
 * \brief MSP430 timer control and support routines
 */

#include "outpour.h"

/**
* \var seconds_since_boot
* \brief Declare seconds since boot.  Incremented by timer 
*        interrupt routine.
*/
static volatile uint32_t sysTicks_since_boot = 0;

/**
* \brief Retrieve the number of system ticks since the system 
*        booted.
* 
* @return uint32_t 32 bit values representing the system tick 
*         count since the system booted.
*/
uint32_t getSysTicksSinceBoot(void)
{
    return (sysTicks_since_boot);
}

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
*    TimerA1_1, Capture control channel 0 used with capacitance reading (counts oscillations)
*    TimerB0_0, Capture control channel 0 used to time the capacitance reading (gate timer)
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

/**
* \brief Initialize and start timerA1 for the short system tick. 
*        Use capture control channel 0 (timerA0_0)
*/
void timerA1_0_init_for_sys_tick(void)
{
    // Halt the timer A1
    TA1CTL = 0;
    // Set up TimerA1_0 to set interrupt flag every 0.03125 seconds,
    // counting to 2^10 aclk cycles
    TA1CCR0 = 0x400;
    // Clear all of TimerA1_0 control
    TA1CCTL0 = 0;
    // Start TimerA1 using ACLK, UP MODE, Input clock divider = 1
    TA1CTL = TASSEL_1 + MC_1 + TACLR;
}

/**
 * \brief Returns true if a compare event has occured. If the
 *        interrupt flag is set, the interrupt flag is cleared.
 */
bool timerA1_0_check_for_sys_tick(void)
{
    // If the interrupt flag for TimerA1_0 is not set, just return
    if ((CCIFG & TA1CCTL0) == 0)
    {
        return (false);
    }
    // Clear TimerA1_0 interrupt flag
    TA1CCTL0 &= ~CCIFG;
    sysTicks_since_boot++;
    return (true);
}

/**
* \brief Initialize and start timerA1 for the long system tick.
*/
void timerA1_0_init_for_sleep_tick(void)
{
    // Halt the timer A1
    TA1CTL = 0;
    // Set up TimerA1_0 to set interrupt flag every 0.5 seconds,
    // counting to 2^14 aclk cycles
    // For 12 hour SOS
    // Note: documentation states match will occur at (count+1)
    TA1CCR0 = ((uint16_t)0x4000) - 1;                      // 16384 * 1/32768 = 0.5 seconds
                                                           // For 5 minute SOS (DEBUG ONLY)
                                                           // TA1CCR0 = 114;  // 0x72 [114*(1/32768) = 0.003479 seconds]
                                                           // Clear all of TimerA1_0 channel 1 control
    TA1CCTL0 = 0;
    // Start TimerA1 using ACLK, UP MODE, Input clock divider = 1
    // TA1CTL = TASSEL_1 + MC_1 + TACLR + ID_3;
    TA1CTL = TASSEL_1 + MC_1 + TACLR + ID_0;
    // Enable timer TimerA1_0 to interrupt the CPU
    TA1CCTL0 |= CCIE;
}

/**
 * \brief TimerA1_0 ISR for Capture/Control channel 1.
 */
#pragma vector=TIMER1_A0_VECTOR
__interrupt void ISR_Timer1_A0(void)
{
    // Clear TimerA0_0 interrupt flag
    TA1CCTL0 &= ~CCIFG;
    __bic_SR_register_on_exit(LPM3_bits);
}

