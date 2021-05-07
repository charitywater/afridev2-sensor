/** 
 * @file waterSense.c
 * \n Source File
 * \n Outpour MSP430 Firmware
 * 
 * \brief Routines to support water sensing algorithm.
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
#include "CTS_Layer.h"
#include "waterDetect.h"
#include "waterSense.h"
#ifdef WATER_DEBUG
#include "debugUart.h"
#endif

/***************************
 * Module Data Definitions
 **************************/

/****************************
 * Module Data Declarations
 ***************************/

/*************************
 * Module Prototypes
 ************************/

/***************************
 * Module Public Functions
 **************************/

/**
* \brief Call once as system startup to initialize the 
*        waterSense module.
* \ingroup PUBLIC_API
*/
void waterSense_init(void)
{
    // initialize pad-data analysis
    waterDetect_init();
}

/*************************
 * Module Private Functions
 ************************/

/**
* 
* \brief Returns estimate of mL for this second. Will
*    update pad max values and next pad max values when
*    appropriate.
*
* \li INPUTS:  
* \li padCounts[TOTAL_PADS]: Taken from TI_CAPT_Raw. Generated
*     internally
*
* \li OUTPUTS:
*/
void waterSense_takeReading(void)
{
    uint8_t pad_number;
    uint16_t padCounts[TOTAL_PADS];

    // Perform the capacitive measurements
    TI_CAPT_Raw(&pad_sensors, &padCounts[0]);

    // make sure measurement is done
    while (CAPSENSE_ACTIVE);

    // Loop for each pad.

    for (pad_number = 0; pad_number < NUM_PADS; pad_number++)
    {
        waterDetect_add_sample(pad_number, padCounts[pad_number]);
    }
}
