/*
 * waterDetect.h
 *
 *  Created on: Nov 7, 2016
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

#ifndef SRC_waterDetect_H_
#define SRC_waterDetect_H_

#include "outpour.h"

#define SAMPLE_COUNT 1
#define NUM_PADS 6
#define OUTLIER 0xFFFF
#define SENSOR_MIN_DOWNSPOUT 200
#define SENSOR_MAX_DOWNSPOUT 800

/**
 * \typedef Pad_Data_t
 * \brief 21 byte structure to hold all water detection telemetry data for one pad
 */
typedef struct
{
    uint8_t cursor;                                        /**< current sample to load */
} Pad_Data_t;



/**
 * \typedef Pad_Info_t
 * \brief 76 byte structure to hold data to fill in SENSOR_DATA message
 */
typedef struct
{
    uint16_t sample[SAMPLE_COUNT];                         /**< rolling buffer of samples */
} Sample_Data_t;

void waterDetect_init(void);
void waterDetect_add_sample(uint8_t pad_number, uint16_t pad_meas);
uint16_t waterDetect_getCurrSample(uint8_t pad_number);

#ifdef WATER_DEBUG
void waterDetect_record_pads_water(void);
#endif

#endif /* SRC_waterDetect_H_ */
