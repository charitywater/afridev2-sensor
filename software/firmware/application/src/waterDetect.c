/*
 * waterDetect.c
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

#include <math.h>
#include "outpour.h"
#include "waterDetect.h"
#include "waterSense.h"

#ifdef WATER_DEBUG
#include "debugUart.h"
#endif

Pad_Data_t pad_db[NUM_PADS];
Sample_Data_t sample_db[NUM_PADS];

/**
* \var int highMarkFlowRates[7]
* \brief Array used to hold the milliliter per second flow rates
*        values based on pad coverage.  This method is no longer used
*        but the data is retained for reference
*/
#ifdef OLD_FLOW_DATA
const uint16_t highMarkFlowRates[7] = {
    376,                                                   // Up through PAD 0 is covered with water
    335,                                                   // Up through PAD 1 is covered with water
    218,                                                   // Up through PAD 2 is covered with water
    173,                                                   // Up through PAD 3 is covered with water
    79,                                                    // Up through PAD 4 is covered with water
    0,                                                     // Only PAD 5 is covered with water -not used-ignore
    0                                                      // No pads are covered
};
#endif

// these numbers were needed when there was no foam pad attached to the board.
// I made them all the same so that they have no effect if enabled.
// but this table could be changed if needed
// the lower the number, the sooner a jump is detected
// this has a side effect of detection happening when water is below the pad being measured (so be careful)
#ifdef VARIABLE_JUMP_DETECT
const uint16_t jumpDectectRange[NUM_PADS] = {
    SAMPLE_MIN_STATE_JUMP,                                 // Pad 0
    SAMPLE_MIN_STATE_JUMP,                                 // Pad 1
    SAMPLE_MIN_STATE_JUMP,                                 // Pad 2
    SAMPLE_MIN_STATE_JUMP,                                 // Pad 3
    SAMPLE_MIN_STATE_JUMP,                                 // Pad 4
    SAMPLE_MIN_STATE_JUMP,                                 // Pad 5
};
#endif


/**
* \brief This initializes the data structures used by this 
*        water detection algorithm. 
*  
* \ingroup EXEC_ROUTINE
*/
void waterDetect_init(void)
{
    uint8_t pad_number;
    uint8_t sample_number;
    Pad_Data_t *pad_data;

    // start rotation of pad data with location 0
    for (pad_number = 0; pad_number < NUM_PADS; pad_number++)
    {
        pad_data = &pad_db[pad_number];

        // clear out data for pad
        memset(pad_data, 0, sizeof(Pad_Data_t));
        // set all samples as "Outliers" to start
        for (sample_number = 0; sample_number < SAMPLE_COUNT; sample_number++)
        {
            sample_db[pad_number].sample[sample_number] = OUTLIER;
        }
    }
}

#ifdef WATER_DEBUG
/**
* \brief Records the capacitance data for when all pads are covered with Water.  This function is for debugging 
*        purposes
*  
* \ingroup PUBLIC_API
*/
void waterDetect_record_pads_water(void)
{
    uint16_t mean;
    MDRwaterRecord_t wr;
    uint8_t pad;
    uint8_t state;
    uint8_t num_samp;

    // get the latest
    manufRecord_getWaterInfo(&wr);

    for (pad = 0; pad < NUM_PADS; pad++)
    {
        if (wr.airDeviation[pad] != 0)
            break;

        wr.airDeviation[pad] = waterDetect_getPadState(pad, &state, &num_samp, &mean);
    }

    // do not update flash if data was already written
    if (pad == NUM_PADS)
        manufRecord_updateManufRecord(MDR_Water_Record, (uint8_t *)&wr, sizeof(MDRwaterRecord_t));
}

#endif


/**
* \brief Adds capacitive sensor readings to this object's sample "database"
*       each pad has a cursor where the next sample is written if a pad is
*       skipped, the data will not be corrupted/misaligned
*  
* @param pad_number Number of pad that was measured
* @param pad_meas Capacitance value measured on the pad
*
* \ingroup PUBLIC_API
*/

void waterDetect_add_sample(uint8_t pad_number, uint16_t pad_meas)
{
    Sample_Data_t *sample_data = &sample_db[pad_number];
    uint8_t cursor = pad_db[pad_number].cursor;            // it's a bit field.. save it locally

    sample_data->sample[cursor] = pad_meas;

    // Fix bug. DDL. 5-5-2017.
    // Increment cursor before comparison to SAMPLE_COUNT is performed.
    cursor++;
    if (cursor >= SAMPLE_COUNT)
        cursor = 0;

    pad_db[pad_number].cursor = cursor;                    // write it back to bit field
}

/**
* \brief This function gives access to current capacitance level for a specific PAD
* 
* @param  pad_number The number of pad to read
* @return unit16_t Returns the current capacitance measurement
* \ingroup PUBLIC_API
*/
uint16_t waterDetect_getCurrSample(uint8_t pad_number)
{
    uint8_t cursor = pad_db[pad_number].cursor;

    // print the sample just read (the cursor has to be moved back to see last value)
    if (!cursor)
        cursor = SAMPLE_COUNT - 1;
    else
        cursor--;

    return (sample_db[pad_number].sample[cursor]);
}
