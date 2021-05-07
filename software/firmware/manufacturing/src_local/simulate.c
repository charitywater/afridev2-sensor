/*
 * simulate.c
 *
 *  Created on: Jul 4, 2020
 *      Author: robl
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
#include "simulate.h"
#include "debugUart.h"

const SIM_WATER_SAMPLE_T sim_water_samp[SIM_SAMP_COUNT] = {
    {6,0,15,1,889},
    {6,29,37,18,505},
    {6,58,32,12,501},
    {7,11,11,17,665},
    {7,48,55,34,423},
    {8,6,51,36,511},
    {8,11,30,18,650},
    {8,26,36,19,765},
    {8,59,21,9,835},
    {9,29,29,15,662},
    {10,8,48,14,839},
    {10,16,52,13,776},
    {10,53,28,23,845},
    {11,2,52,23,566},
    {11,14,56,34,532},
    {11,31,0,22,749},
    {11,49,19,17,884},
    {12,1,23,22,611},
    {12,12,16,16,747},
    {12,26,31,25,571},
    {12,41,22,24,567},
    {13,18,29,7,897},
    {13,26,43,19,591},
    {13,32,11,15,731},
    {13,46,27,18,623},
    {13,52,23,8,605},
    {14,1,49,41,454},
    {14,40,50,25,450},
    {14,48,59,6,764},
    {14,56,30,15,874},
    {15,4,10,18,411},
    {15,17,34,35,481},
    {15,52,23,25,714},
    {15,58,3,19,408},
    {16,16,10,24,706},
    {16,32,7,13,565},
    {16,38,44,20,521},
    {16,52,11,12,564},
    {16,58,38,20,632},
    {17,7,50,14,895},
    {17,32,48,31,626},
    {17,43,14,18,869},
    {18,4,27,14,816},
    {18,24,22,17,771},
    {18,44,31,31,480},
    {19,5,31,25,653},
    {19,25,29,15,788},
    {19,45,53,22,897},
};

uint8_t sim_index = 0;

uint32_t decimalTime(uint8_t hour24, uint8_t min, uint8_t sec)
{
    uint32_t answer;

    answer = sec + 60*min + 60*60*hour24;
    return (answer);
}

uint16_t simulateWaterAnalysis(uint8_t num_samples)
{
    timePacket_t tp;
    SIM_WATER_SAMPLE_T *sim;
    uint16_t answer;
    uint32_t rtc,entry;
    uint32_t diff;

    // get rtc
    getBinTime(&tp);
    rtc = decimalTime(tp.hour24,tp.minute,tp.second);

    // if the time crossed midnight, then restart the table search
    if (sim_index >= SIM_SAMP_COUNT && tp.hour24 == 0) {
        sim_index = 0;
    }
    WATCHDOG_TICKLE();

    sim = (SIM_WATER_SAMPLE_T *)&sim_water_samp[sim_index];
    entry = decimalTime(sim->hour24,sim->minute,sim->second);

    if (rtc >= entry)
    {
        diff = rtc - entry;
        if (diff <= sim->repeat) {
            uint32_t sys_time = getSecondsSinceBoot();
            getBinTime(&tp);
            debug_RTC_time(&tp,'F',&stData, sys_time);
            answer = sim->flowrate;
        }
        else {
            // rtc is past current entry, try the next entry
            if (sim_index < SIM_SAMP_COUNT) {
                sim_index++;
            }
            answer = 0;
        }
    }
    else
    {
        // rtc is too early for the current entry, leave with no water volume
        answer = 0;
    }

    return (answer);
}
