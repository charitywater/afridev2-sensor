/** 
 * @file waterSense.c
 * \n Source File
 * \n Outpour MSP430 Firmware
 * 
 * \brief Routines to support water sensing algorithm.
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

/**
* \var wsData 
* \brief Module data structure 
*/
// static
sensorStats_t padStats;                                    /**< Array to hold stats */

/*************************
 * Module Prototypes
 ************************/
void waterSenseReadInternalTemp(void);

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
    sysExecData.waterDetectResets = 0;

    // zero module data structure
    memset(&padStats, 0, sizeof(sensorStats_t));

    //  Clear the stats
    waterSense_clearStats();

    // set system default tuned value
    sysExecData.downspout_rate = TUNED_DOWNSPOUT_RATE;
    //padStats.water_limit = WATER_STUCK_LIMIT;  // this feature is no longer needed by default
}

/**
* \brief Send water sense debug information to the uart.  Dumps 
*        the complete sensorStats data structure.
* \ingroup PUBLIC_API
* 
*/
void waterSense_sendDebugDataToUart(void)
{
    // Output debug information
    dbgMsgMgr_sendDebugMsg(MSG_TYPE_DEBUG_PAD_STATS, (uint8_t *)&padStats, sizeof(sensorStats_t));
}


/**
* \brief Return the last flow rate measured
* \ingroup PUBLIC_API
* 
* @return uint16_t  Flow rate in mL per second
*/
uint16_t waterSense_getLastMeasFlowRateInML(void)
{
    return (padStats.lastMeasFlowRateInMl);
}

/**
* \brief Return current max statistic
* \ingroup PUBLIC_API
*
* @return uint16_t Current max measured value for pad
*/
uint16_t waterSense_getPadStatsMax(padId_t padId)
{
    return (waterDetect_getTargetAir((uint8_t)padId));
}

/**
* \brief Return current min statistic
* \ingroup PUBLIC_API
*
* @return uint16_t Current min measured value for pad
*/
uint16_t waterSense_padStatsMin(padId_t padId)
{
    return (waterDetect_getTargetWater((uint8_t)padId));
}

/**
* \brief Return subMerged count statistic for pad
* \ingroup PUBLIC_API
*
* @return uint16_t Current submerged measured value for pad
*/
uint16_t waterSense_getPadStatsSubmerged(padId_t padId)
{
    return (waterDetect_getPadSubmergedCount((uint8_t)padId));
}

/**
* \brief Return Current unknown count statistic for pad
* \ingroup PUBLIC_API
*
* @return uint16_t Current unknowns count for pad
*/
uint16_t waterSense_getPadStatsUnknowns(void)
{
    return (padStats.unknowns);
}

/**
* \brief Return the temperature taken from the internal sensor.
* \ingroup PUBLIC_API
*
* @return int16_t Temperature in Celcius
*/
int16_t waterSense_getTempCelcius(void)
{
    return (padStats.tempCelcius);
}

/**
* \brief Clear saved statistics
*
* \ingroup PUBLIC_API
*/
void waterSense_clearStats(void)
{
    padStats.tempCelcius = 0;
    padStats.unknowns = 0;
    padStats.sequential_unknowns = 0;
    waterDetect_clear_stats();
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

    // Loop for each pad.

    for (pad_number = 0; pad_number < NUM_PADS; pad_number++)
    {
        waterDetect_add_sample(pad_number, padCounts[pad_number]);
    }
}

uint8_t waterSense_waterPresent(void)
{
	uint8_t answer;
    uint16_t padCounts[TOTAL_PADS];

    // Perform the capacitive measurements
    TI_CAPT_Raw(&pad_sensors, &padCounts[0]);

#ifdef WATERDETECT_READ_WATER_LEVEL_NORMAL
    if (waterDetect_waterPresent(padCounts[5], 5))
#else
    if (waterDetect_waterPresent(padCounts[0], 0))
#endif
       answer = true;
    else
       answer = false;

    return (answer);
}




/**
 * 
 * @brief Perform pad coverage and flow rate calculations. 
 *        Returns number of submerged pads.
 * 
 * @return uint8_t numOfSubmergedPads: Raw count of submerged
 *         sensors
 */
uint8_t waterSense_analyzeData(void)
{
    uint8_t unknowns = 0;
    uint8_t percentile;
    uint8_t submergedPadsBitMask;                          /* Which pads are submerged for this meas */
    uint8_t numOfSubmergedPads;                            /* Number of pads submerged from last meas */

#ifdef WATER_DEBUG   // Exclude this section for water debug

    if (!gps_isActive())
    {
        // debug messages are selected/deselected in debugUart.h
        uint32_t sys_time = getSecondsSinceBoot();

        debug_chgSummary(sys_time);
    }
#endif

    waterDetect_mark_outliers();
    waterDetect_update_stats();

    numOfSubmergedPads = waterDetect_read_water_level(&submergedPadsBitMask, &unknowns);

    if (numOfSubmergedPads)
    {
        padStats.lastMeasFlowRateInMl = waterDetect_get_flow_rate(numOfSubmergedPads, &percentile);
        sysExecData.total_flow += padStats.lastMeasFlowRateInMl;
        padStats.sequential_waters++;

        // prevent stuck on from occurring.. this is just insurance
        if (padStats.water_limit > 0)
        {
            if (padStats.sequential_waters > padStats.water_limit)
            {
            	sysExecData.faultWaterDetect = true;
                padStats.sequential_waters = 0;
                sysExecData.waterDetectResets++;
#ifdef WATER_DEBUG
                debug_message("***WATER STUCK!***");
                __delay_cycles(1000);
#endif
            }
        }
    }
    else
    {
        padStats.lastMeasFlowRateInMl = 0;
        padStats.sequential_waters = 0;
    }



#ifndef WATER_DEBUG   // Exclude this section for water debug
    // keep track of number of times unknowns occurs in a row
    // stuck unknowns would be an occasion for resetting the air_targets
    if (unknowns > 0)
    {
        padStats.unknowns++;
        padStats.sequential_unknowns++;
        // reset the measurements if we are "stuck" with unknowns more than a programmed limit
        if (padStats.unknown_limit > 0)
        {
            if (padStats.sequential_unknowns > padStats.unknown_limit)
            {
            	sysExecData.faultWaterDetect = true;
                padStats.sequential_unknowns = 0;
                sysExecData.waterDetectResets++;
#ifdef WATER_DEBUG
                debug_message("***UNKNOWNS STUCK!***");
                __delay_cycles(1000);
#endif
            }
        }
    }
    else 
        padStats.sequential_unknowns = 0;

#endif
    // a pour session ended, log it to debug
    if (!padStats.lastMeasFlowRateInMl && sysExecData.total_flow)
    {
        // display total and reset
#ifdef WATER_DEBUG
        uint32_t sys_time = getSecondsSinceBoot();

        debug_pour_total(sys_time, sysExecData.total_flow);
        sysExecData.total_flow = 0;
#else
        // if the debug feature to report data immediately is engaged, then report the data
        if (sysExecData.sendSensorDataNow)
        {
            sysExecData.sendSensorDataMessage = true;
        }
#endif

    }

#ifdef WATER_DEBUG
    // debug messages are selected/deselected in debugUart.h
    uint32_t sys_time = getSecondsSinceBoot();

    debug_padSummary(sys_time, numOfSubmergedPads, unknowns, padStats.pump_active, 0);
#endif

    if (!padStats.pump_active && (numOfSubmergedPads == NUM_PADS))
    {
#ifdef WATER_DEBUG
        waterDetect_set_water_target();

        if (sysExecData.mtest_state != MANUF_UNIT_PASS)
            waterDetect_record_pads_water();
#endif
        padStats.air_wait = 2;                             // wait an extra 4 second period for the water to drip away from pads to re-establish
                                                           // air target
        padStats.pump_active = 1;
    }
    else if (padStats.pump_active && !numOfSubmergedPads)
    {
        if (!padStats.air_wait)
        {
            padStats.pump_active = 0;
#ifdef WATER_DEBUG
            waterDetect_set_air_target();

            if (sysExecData.mtest_state != MANUF_UNIT_PASS)
                sysExecData.mtest_state = MANUF_WATER_PASS;
#endif
        }
        else
            --padStats.air_wait;

    }

    return (numOfSubmergedPads);
}

int16_t waterSenseGetTemp(void)
{
    waterSenseReadInternalTemp();
    return (padStats.tempCelcius);
}

/**
* @brief Read the internal temp sensor ADC and convert to degrees celcius.
*
* @note Temperature processing code copied from:
* http://forum.43oh.com/topic/1954-using-the-internal-temperature-sensor/
*
* The call to this function was moved to SysExec.
 
*/
void waterSenseReadInternalTemp(void)
{
    int c;

    unsigned adc = 0;
    //    int i;
    volatile uint16_t regVal;
    volatile uint16_t timeoutCount;

    // Configure ADC
    ADC_ENABLE();
    __delay_cycles(2000);  // let it take effect

    ADC10CTL0 = 0;
    ADC10CTL1 = INCH_12 | ADC10DIV_3;
    ADC10CTL0 = SREF_1 | ADC10SHT_3 | REFON | ADC10ON | REF2_5V;

    //    // Take four readings of the ADC and average
    //    adc = 0;
    //    for (i = 0; i < 4; i++)
    //    {
    ADC10CTL0 &= ~ADC10IFG;                                // Clear conversion complete flag
    ADC10CTL0 |= (ENC | ADC10SC);                          // Begin ADC conversion
    timeoutCount = ~0x0;
    while (timeoutCount--)
    {
        regVal = ADC10CTL0;
        if (regVal & ADC10IFG)
        {
            break;
        }
    }
    adc += ADC10MEM;                                       // Read ADC
                                                           //    }
                                                           //
                                                           //    // Average
                                                           //    adc >>= 2;

    ADC_DISABLE();
    //	c = (int)((6003L*adc-1722286L)>>16); //Example 23C
    c = (int)((60026L * adc - 17222860L) >> 16);           //Example 23.3C

    // record the latest temperature
    padStats.tempCelcius = c;
}
