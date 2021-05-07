/*
 * simulate.h
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

#ifndef SRC_LOCAL_SIMULATE_H_
#define SRC_LOCAL_SIMULATE_H_

#define SIM_SAMP_COUNT  48

typedef struct
{
    uint8_t hour24;
    uint8_t minute;
    uint8_t second;
    uint8_t repeat;
    uint16_t flowrate;
} SIM_WATER_SAMPLE_T;

uint16_t simulateWaterAnalysis(uint8_t num_samples);

#endif /* SRC_LOCAL_SIMULATE_H_ */
