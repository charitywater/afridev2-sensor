/** 
 * @file time.c
 * \n Source File
 * \n AfridevV2 MSP430 Firmware
 * 
 * \brief MSP430 timer control and support routines
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

/*
 *  3276 ticks / 32768 ticks/second = .100006104 seconds = ~10Hz
 */
#define TICKS_PER_100_MS    3276u

//due to resolution of the clock, each interrupt will be -24us from the desired 100.0 ms rate.
//with 3600 * 10 interrupts per hour, this will add up over the course of a day.

//~9.8 seconds behind but we dont have ms resolution for RTC lib
//Add 9 seconds to the seconds counter @ the end of the day to reset the drift

//EDIT: Just add 4 seconds to ensure the clock drifts 3 or 4 seconds slow each day.
//We get a new time sync every 28 days. If the clock drifts "fast" the time sync will not work
#define SECS_PER_DAY_DRIFT  4


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

    TA0CCR0 = TICKS_PER_100_MS;   // 32768Hz/3276=10HZ

    TA0CTL = TASSEL_1 + MC_1 + TACLR;
    TA0CCTL0 &= ~CCIFG;
    TA0CCTL0 |= CCIE;

    ticks_per_second = 0;
    seconds_since_boot = 0;
}

/**
* \brief adjust all the system's clocks with the same adjustment.  This is due to time not accounted for
* due to resolution of the 10Hz timer interrupt.
* \ingroup PUBLIC_API
*/
void all_timers_adjust_time_end_of_day(void)
{
    uint8_t i;
    uint8_t adjustment = SECS_PER_DAY_DRIFT;

    seconds_since_boot += adjustment;
    stData.storageTime_seconds += adjustment;

    //rtc lib
    for (i=0; i<adjustment; i++)
       incrementSeconds();
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
* \brief Timer ISR. Produces the 10HZ system tick interrupt.
*        Uses Timer A0, capture/control channel 0, vector 9,
*        0xFFF2
* \ingroup ISR
*/
#ifndef FOR_USE_WITH_BOOTLOADER
#pragma vector=TIMER0_A0_VECTOR
#endif
__interrupt void ISR_Timer0_A0(void)
{

    TA0CTL |= TACLR;

    ++ticks_per_second;

    // Check if we have reached one second
    if (ticks_per_second >= TIMER_INTERRUPTS_PER_SECOND)
    {
        // Increment the TI calendar library
        incrementSeconds();
        
        // Increment the seconds counter
        seconds_since_boot += 1;
        
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

/**
 * \brief Utility function to calculate a 2 byte time value that
 * is in the format hour*256 + minutes
 *
 * @param *tp time value from RTC library
 *
 * @return uint16_t binary encoded time.
 *
 */
uint16_t time_util_RTC_hms(timePacket_t *tp)
{
    uint16_t answer = 0xffff;

    // validate the time value
    if (tp)
    {
       if(tp->hour24 < 24 && tp->minute < 60)
          answer = tp->hour24*256+tp->minute;
    }
    return (answer);
}

