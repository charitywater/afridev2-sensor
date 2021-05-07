/**
 * @file appAlgo.c
 * \n Source File
 * \n AfridevV2 MSP430 Firmware
 *
 * \brief Algorithm nest - the interface between MATLAB generated c code and the application
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

/* Algorithm Includes */
#include "algo-c-code/calculateWaterVolume/calculateWaterVolume.h"
#include "algo-c-code/clearPadWindowProcess/clearPadWindowProcess.h"
#include "algo-c-code/hourlyWaterVolume/hourlyWaterVolume.h"
#include "algo-c-code/initializeWaterAlgorithm/initializeWaterAlgorithm.h"
#include "algo-c-code/initializeWindows/initializeWindows.h"
#include "algo-c-code/waterPadFiltering/waterPadFiltering.h"
#include "algo-c-code/writePadSample/writePadSample.h"

#include "../waterDetect.h"
#include "../outpour.h"
#include "appAlgo.h"

#define MAX_RETURNED_REASON_CODES       4

static padWindows_t padWindow;
static padSample_t currentPadSample;
static waterAlgoData_t waterAlgoData;
static padFilteringData_t padFilterData;
static uint16_t algoErrorBits = 0u;
static uint16_t tempErrorBits = 0u;

static void xGetLatestSamples(void);
static void xWaterpadProcess(void);
static void xHandleError(ReasonCodes reason);

void APP_ALGO_init(void)
{
    initializeWindows( &padWindow );
    initializeWaterAlgorithm( &waterAlgoData, &padFilterData );
}

void APP_ALGO_runNest(void)
{
    xGetLatestSamples();

    waterPadFiltering( &currentPadSample, &padFilterData, &currentPadSample );
    writePadSample( &padWindow,  &currentPadSample );

    if (padWindow.process)
    {
       xWaterpadProcess();
    }
}

bool APP_ALGO_isWaterPresent(void)
{
    return waterAlgoData.present;
}

uint32_t APP_ALGO_getHourlyWaterVolume_ml(void)
{
    ReasonCodes reason;
    uint32_T hourlyVolume_l;
    uint32_T hourlyVolume_ml;

    hourlyWaterVolume( &waterAlgoData, &reason, &hourlyVolume_ml, &hourlyVolume_l);

    if ( reason != reason_code_none )
    {
        xHandleError(reason);
    }

    return hourlyVolume_ml;
}

//call this @ end of the day
uint16_t APP_ALGO_reportAlgoErrors(void)
{
    uint16_t currentErrorBits = algoErrorBits;

    //clear algo error bits since we just read them out
    algoErrorBits = 0;

    return currentErrorBits;
}

void APP_ALGO_wakeUpInit(void)
{
   // wakeupDataReset( &padFilterData, &waterAlgoData);
}

static void xGetLatestSamples(void)
{
    // Pad Samples
    currentPadSample.pad5 = waterDetect_getCurrSample(5);
    currentPadSample.pad4 = waterDetect_getCurrSample(4);
    currentPadSample.pad3 = waterDetect_getCurrSample(3);
    currentPadSample.pad2 = waterDetect_getCurrSample(2);
    currentPadSample.pad1 = waterDetect_getCurrSample(1);
    currentPadSample.pad0 = waterDetect_getCurrSample(0);
}

static void xWaterpadProcess(void)
{
    int i = 0;
    ReasonCodes reasonCodes[MAX_RETURNED_REASON_CODES];

    calculateWaterVolume( &waterAlgoData, &padWindow, reasonCodes );

    //get reason codes:
    for (i = 0; i< MAX_RETURNED_REASON_CODES; i++)
    {
        if ( reasonCodes[i] != reason_code_none)
        {
            xHandleError(reasonCodes[i]);
        }
    }

    clearPadWindowProcess( &padWindow );
}

//Map reason code to an error bit to include in the sensor data log for this day
static void xHandleError(ReasonCodes reason)
{
    tempErrorBits = 0;
    switch (reason)
    {
        case water_flow_standing_water:
            tempErrorBits |= WATER_STANDING;
            break;

        case water_bad_sample:
            tempErrorBits |= WATER_BAD_SAMPLE;
            break;

        case water_volume_capped:
            tempErrorBits |= WATER_VOLUME_CAPPED;
            break;
    }

    algoErrorBits |= tempErrorBits;
}
