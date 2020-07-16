/*
 * simulate.h
 *
 *  Created on: Jul 4, 2020
 *      Author: robl
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
