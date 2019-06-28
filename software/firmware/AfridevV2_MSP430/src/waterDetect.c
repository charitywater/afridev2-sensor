/*
 * waterDetect.c
 *
 *  Created on: Nov 7, 2016
 *      Author: robl
 */

#include <math.h>
#include "outpour.h"
#include "waterDetect.h"
#include "waterSense.h"

Pad_Data_t pad_db[NUM_PADS];
Sample_Data_t sample_db[NUM_PADS];
uint8_t outlier_count;

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


// air slope table contains the number of capacitance counts in tenths for every degree of temperature
// change
//                               Pad0,   Pad1,   Pad2,   Pad3,   Pad4,   Pad5
int32_t air_slope[NUM_PADS] = { -401, -302, -345, -312, -345, -299 };

/**
* \brief This function adjusts the air or water target limits based on the change in ambient
*        temperature of the air over the pad.  The following ASCII art chart shows the 6 pads and
*        measurements of how the data changes over 5c increments.
*

*	     //                   43000  |    4444444444444
*	     //                          |                 4444444444444
*        //                          |    0000000000000             4444444444444
*	     //                   42000  |    55555555555550000000000000             4444444444444
*	     //                          |                 55555555555550000000000000             4444444444444
*        //                          |                              55555555555550000000000000
*	     //          Pad      41000  |                                           55555555555550000000000000
*	     //      Capacitance         |                                                        5555555555555
*        //         Level            |
*	     //                   40000  |    2222222222222
*	     //                          |                 2222222222222
*        //                          |                              2222222222222
*	     //                   39000  |    3333333333333333                       2222222222222
*	     //                          |                    33333333333333333                   2222222222222
*        //                          |                                     333333333333333
*	     //                   38000  |                                                    33333333333333333
*	     //                          |
*        //                          |
*	     //                   37000  |    1111111111111111
*	     //                          |                    11111111111111111
*        //                          |                                     111111111111111
*	     //                   36000  |                                                    11111111111111111
*	     //                          |
*        //                          |
*        //                   35000  |________________________________________________________________________
*        //                               + - - - + - - - + - - - + - - - + - - - + - - - + - - - + - - - +
*	     //                               10      15      20      25      30      35      40      45      50
*        //                                                       Pad Ambient Temp
*
*/

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
        // these are already 0 from memset; here for comment
        pad_data->pad.state = STATE_UNKNOWN;
        pad_data->pad.last_mean = pad_data->mean = 0;
    }
}


/**
* \brief record the current pad values and temperature in the Manufacturing Data Record.
* 
*/
void waterDetect_record_pads_baseline(void)
{
    MDRwaterRecord_t wr;
    uint8_t i;

    // get the latest
    manufRecord_getWaterInfo(&wr);

    for (i = 0; i < NUM_PADS; i++)
    {
        wr.padBaseline[i] = pad_db[i].pad.target_air;
    }
    wr.padTemp = waterSense_getTempCelcius();

    manufRecord_updateManufRecord(MDR_Water_Record, (uint8_t *)&wr, sizeof(MDRwaterRecord_t));
}

/**
* \brief This function restores the water detection software's target data to the baseline data.
         The restoration is needed to prevent an error where a unit restarts due to a Watchdog reset 
		 and a new air target is measured while there is water over the pads.
		 The values of capacitance measurements are affected by ambient temperature. So the baseline
		 values are adjusted to the current temperature according to the "air_slope" data.  This is 
		 necessary to prevent false water detection due to temperature shifting.
* 
* @param wr Pointer to current baseline data stored in Flash
*  
* \ingroup PUBLIC_API
*/
bool waterDetect_restore_pads_baseline(MDRwaterRecord_t *wr)
{

    int16_t curr_temp;
    int32_t air_temp_diff;
    uint8_t pad_number;
    Pad_Data_t *pad_data;
    bool answer = false;

    curr_temp = waterSenseGetTemp();

    // check if the temperature is within normal operating range
    // if we are under temp, then do not try to adjust targets
    if (curr_temp >= MIN_OPERATING_TEMP && manufRecord_checkForValidManufRecord())
    {
        // look for invalid baseline data, don't use it if any value is < SAMPLE_MIN (0x08000)
        for (pad_number = 0; pad_number < NUM_PADS; pad_number++) 
		    if (wr->padBaseline[pad_number] < SAMPLE_MIN)
                break;

        if (pad_number == NUM_PADS)
        {
            air_temp_diff = curr_temp - wr->padTemp;

            for (pad_number = 0; pad_number < NUM_PADS; pad_number++)
            {
                pad_data = &pad_db[pad_number];
                pad_data->change_air = air_temp_diff * air_slope[pad_number] / 100;

                if (pad_data->change_air)
                {
                    // use the historical air data, adjusted by temperature to start
                    // this is insurance in case that the system resets while the pads are covered in
                    // water
                    pad_data->mean = wr->padBaseline[pad_number] + pad_data->change_air;
                    pad_data->pad.last_mean = pad_data->pad.target_air = pad_data->pad.target_water = pad_data->mean;
                    pad_data->pad.podtemp_air = curr_temp;
                }
            }
            answer = true;
        }
    }
    return (answer);
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
* \brief Clears out the statistics data collected by the unit. This data is 
*        reported up to the server on request. 
*  
* \ingroup PUBLIC_API
*/
void waterDetect_clear_stats(void)
{
    uint8_t pad_number;

    for (pad_number = 0; pad_number < NUM_PADS; pad_number++)
    {
        pad_db[pad_number].submerged_count = 0;
    }
}

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
* \brief This function acts like a filter to remove corrupt data samples.  For this
*        application, a sample is invalid if it falls outside the range on sample values
*        for all pads (30000 to 49000), or for a single pad if a sample jumps more than 1200 counts
*        from the mean.  Outliers are marked with 0xFFFF and are not used in analysis.
*        This function calculates the mean of the remaining samples that are good, and it
*        saves the last_mean for jump analysis to detect water state changes.
*
* \ingroup PUBLIC_API
*/
void waterDetect_mark_outliers(void)
{
    uint8_t pad_number;

    outlier_count = 0;

    // first pass weed out any obvious bad samples
    for (pad_number = 0; pad_number < NUM_PADS; pad_number++)
    {
        uint8_t sample_number;
        uint32_t sum;
        uint16_t *samp = sample_db[pad_number].sample;

        pad_db[pad_number].pad.num_samp = 0;
        sum = 0;
        for (sample_number = 0; sample_number < SAMPLE_COUNT; sample_number++)
        {
            if (pad_db[pad_number].pad.last_mean &&
                (samp[sample_number] < pad_db[pad_number].pad.last_mean - SAMPLE_MAX_JUMP ||
                 samp[sample_number] > pad_db[pad_number].pad.last_mean + SAMPLE_MAX_JUMP))
            {
                samp[sample_number] = OUTLIER;
                outlier_count++;
            }
            if (samp[sample_number] != OUTLIER)
            {
                pad_db[pad_number].pad.num_samp++;
                sum += samp[sample_number];
            }

        }
        pad_db[pad_number].pad.last_mean = pad_db[pad_number].mean;
        if (pad_db[pad_number].pad.num_samp)
            pad_db[pad_number].mean = sum / pad_db[pad_number].pad.num_samp;
        else
            pad_db[pad_number].mean = 0;                   // this should NOT happen ever
    }
}

/**
* \brief The Afridev2 software contunally watches the temperature of the air over the pad sensors to
*        be sure that the target capacitance values for AIR and WATER tracks with the current temperatures.
*        A change of a tenth of a degree C or more is significant.  The air_slope formula is applied
*        using baseline data collected in the development of this algorithm.   The formula is linear in this
*        interpretation, and this is an approximation.  The limited air flow within the Afridev2 unit can lead to 
*        pad temperature measurements that are time lagged from the actual temperature over the pads.
* 
* @param pad_number number of pad to be analyzed
* @param *pad_data address of water detect telemetry data to use for analysis
*  
* \ingroup PUBLIC_API
*/
void heat_analysis(uint8_t pad_number, Pad_Data_t *pad_data)
{
    // when the air over the pads gets warmer/colder the capacitance level shifts down/up. The targets need
    // to proportionately change with temperature

    int16_t curr_temp;
    int32_t air_temp_diff, water_temp_diff;

    curr_temp = waterSense_getTempCelcius();

    // check if the temperature is within normal operating range
    // if we are under temp, then do not try to adjust targets
    if (curr_temp >= MIN_OPERATING_TEMP)
    {
        if (pad_data->pad.podtemp_air)
        {
            air_temp_diff = curr_temp - pad_data->pad.podtemp_air;
            pad_data->change_air = air_temp_diff * air_slope[pad_number] / 100;
            if (pad_data->change_air)
            {
                pad_data->pad.target_air = pad_data->pad.target_air + pad_data->change_air;
                pad_data->pad.podtemp_air = curr_temp;
            }
        }
        else
        {
            pad_data->pad.podtemp_air = curr_temp;
        }

        if (pad_data->pad.podtemp_water)
        {
            water_temp_diff = curr_temp - pad_data->pad.podtemp_water;
            pad_data->change_water = water_temp_diff * air_slope[pad_number] / 100;
            if (pad_data->change_water)
            {
                pad_data->pad.target_water = pad_data->pad.target_water + pad_data->change_water;
                pad_data->pad.podtemp_water = curr_temp;
            }
        }
        else
        {
            pad_data->pad.podtemp_water = curr_temp;
        }
    }
}



/**
* \brief This function adjusts the air and water target limits based on a jump algorithm.  when a
*        pad goes from being covered with air to water or vice versa, the capacitance value will
*        greatly change (500 points or more).   In case 1, the mean jumped greatly from the last
*        second, so we immediately set the new target (air or water) to that value.  In case 2, the
*        mean is only changed when it is outside the current range.  In case 3 there is no change
*        at all.   These targets are used later to perform midpoint analysis.
*
*        // case 1: Only applies when mean makes a big jump from last mean seen
*        //         When current mean is in the jump allowed area, then the mean can
*        //         shift in or out to that point.  This jump allows for "recalibration"
*        //         when the actual target min and max changes due to environmental changes
*        //         big jumps are unmistakable events of a shift from water to air and vice
*        //         versa, so we take advantage of that event and give it great notice
*        //
*	     //                                    last
*	     //                                    mean
         //                          |    250   |    250   |
         //        <--jump allowed-->|<--jump exclusion -->|<--jump allowed-->
         //      TW>>>>>TW           |          |          |             TA<<<<<<<<TA
         //             TW<<<<<TW                                   TA>>>TA
         //              ^                                                ^
         //            curr                                             curr
         //            mean                                             mean
*        //
*        // case 2: When current mean is outside the midpoint area, only change air or water
*        //         target only when it exceeds the previous set TW or TA limits
*        //
         //              water  |        in midpoint area         | air
         //              target |        no change needed         | target
         //         TW<<<<<<<<<<|<------------------------------->|>>>>>>>>>>TA
         //         ^                                                        ^
         //        curr                                                    curr
         //        mean                                                    mean

         // case 3: When current mean is in midpoint area, there is no change to TA/TW
*        //
         //              water  |        in midpoint area         | air
         //              target |        no change needed         | target
         //                    TW<------------------------------->TA
         //                              ^
         //                             curr
         //                             mean
*  
* @param *pad_data address of water detect telemetry data to use for analysis
*
*
* \ingroup PUBLIC_API
*/

void jump_analysis(Pad_Data_t *pad_data)
{
    uint16_t curr_diff;

    // detect "big-jumps".. if one is seen then a new target is established.
    // this way if the ambient temperature changes the targets will track with
    // the current environment
    //tolerance = 500 counts

    // scrap all samples for this second interval for trending if outliers are seen on ANY pad
    if (!outlier_count)
    {

        if (pad_data->mean > pad_data->pad.last_mean)
            curr_diff = pad_data->mean - pad_data->pad.last_mean;
        else
            curr_diff = pad_data->pad.last_mean - pad_data->mean;

        // check if the outlier detection caught a jump of a mean to another state
#ifdef VARIABLE_JUMP_DETECT
        if (curr_diff > jumpDectectRange[pad_number])      // a big jump in mean seen over the last second
#else
        if (curr_diff > SAMPLE_MIN_STATE_JUMP)             // a big jump in mean seen over the last second
#endif
            {
                if (pad_data->mean >= pad_data->pad.last_mean)
                {
                    // a jump in the "air" direction, did it jump more than 250 counts from the current target?
#ifdef VARIABLE_JUMP_DETECT
                    if ((pad_data->mean + jumpDectectRange[pad_number] / 2) >= pad_data->pad.target_air)
#else
                    if ((pad_data->mean + SAMPLE_MIN_STATE_JUMP / 2) >= pad_data->pad.target_air)
#endif
                        {
                            pad_data->pad.target_air = pad_data->mean;
                            pad_data->pad.podtemp_air = waterSense_getTempCelcius();
                        }
                }
                else
                {
                    if (pad_data->mean < pad_data->pad.last_mean)
                    {
                        // a jump in the "water" direction,  did it jump more than 250 counts from the current target?
#ifdef VARIABLE_JUMP_DETECT
                        if ((pad_data->mean - jumpDectectRange[pad_number] / 2) < pad_data->pad.target_water)
#else
                        if ((pad_data->mean - SAMPLE_MIN_STATE_JUMP / 2) < pad_data->pad.target_water)
#endif
                            {
                                pad_data->pad.target_water = pad_data->mean;
                                pad_data->pad.podtemp_water = waterSense_getTempCelcius();
                            }
                    }
                }
            }                                              // a big jump in mean seen over the last second

        // update air and water targets to the latest
        if (pad_data->mean > pad_data->pad.target_air)
        {
            pad_data->pad.target_air = pad_data->mean;
            pad_data->pad.podtemp_air = waterSense_getTempCelcius();
        }
        else if (pad_data->mean < pad_data->pad.target_water)
        {
            pad_data->pad.target_water = pad_data->mean;
            pad_data->pad.podtemp_water = waterSense_getTempCelcius();
        }
    }
}

/**
\brief This function is called at the start of a pumping cycle. The water target is set
*      when all pads are covered with water (just when being "cooled" by pumping water).
*
* \ingroup PUBLIC_API
*/

void waterDetect_set_water_target(void)
{
    uint8_t pad_number;
    Pad_Data_t *pad_data;
    int16_t new_range;

    // first pass weed out any obvious bad samples
    for (pad_number = 0; pad_number < NUM_PADS; pad_number++)
    {
        pad_data = &pad_db[pad_number];
        new_range = abs(pad_data->mean - pad_data->pad.target_air);

        // do not allow the range to shrink below the minimum target threshold
        if (new_range >= SAMPLE_MIN_TARGET_RANGE)
        {
            pad_data->pad.target_water = pad_data->mean;
            pad_data->pad.podtemp_water = waterSense_getTempCelcius();
        }
    }
}

/**
\brief This function is called at the end of a pumping cycle. The air target is set
*      when all pads are covered with air (just after being "cooled" by pumping water).
*
* \ingroup PUBLIC_API
*/
void waterDetect_set_air_target(void)
{
    uint8_t pad_number;
    Pad_Data_t *pad_data;
    int16_t new_range;

    // first pass weed out any obvious bad samples
    for (pad_number = 0; pad_number < NUM_PADS; pad_number++)
    {
        pad_data = &pad_db[pad_number];
        new_range = abs(pad_data->mean - pad_data->pad.target_water);

        // do not allow the range to shrink below the minimum target threshold
        if (new_range >= SAMPLE_MIN_TARGET_RANGE)
        {
            pad_data->pad.target_air = pad_data->mean;
            pad_data->pad.podtemp_air = waterSense_getTempCelcius();
        }
    }

}


/**
* \brief This function characterizes whether the current state of a pad is covered with "air" or "water".
*        This is done by calculating the mid point between the air and water targets.  If the current
*        mean is greater or equal to the midpoint then the pad is covered with "air", otherwise the pad
*        is covered with "water".   This assessment is only made when the difference between the air and water
*        targets is greater than 450.  When the difference is less than 450, then we have not likely
*        fully characterized the true minimum and maximum of the capacitance value based on the presence of air
*        and water (GIVEN THE CURRENT ENVIRONMENT).
*
*        //calculate the percentile of the current mean within the targets, multiply to zoneFlowRate
         // and add to total
         //
         //                         midpoint
         // water  |                    |                       | air
         // target |<--------------target width---------------->| target
         //        |                    |                       |
         //        ^         ^          |           ^           ^
         //      water     water        |          air         air     <---- PAD STATE
         //       min     midpoint      |       midpoint       max     <---- PAD STATE
         //
*
* @param *pad_data address of water detect telemetry data to use for analysis
*
* \ingroup PUBLIC_API
*/

void midpoint_analysis(Pad_Data_t *pad_data)
{

    int16_t target_width;
    uint16_t target_midpoint;

    if (pad_data->pad.target_air && pad_data->pad.target_water)
    {
        target_width = pad_data->pad.target_air - pad_data->pad.target_water;

        // make sure the air and water targets are well established before midpoint analyisis is done
#ifdef VARIABLE_JUMP_DETECT
        if (target_width > jumpDectectRange[pad_number])
#else
        if (target_width > SAMPLE_MIN_STATE_JUMP)
#endif
            {
                target_midpoint = pad_data->pad.target_water + (target_width / 2);

                if (pad_data->mean >= target_midpoint)
                {
                    if (pad_data->pad.target_air == pad_data->mean)
                    {
                        pad_data->pad.state = STATE_AIR_MAX; //3
                    }
                    else
                    {
                        pad_data->pad.state = STATE_AIR_MIDPOINT; //4
                    }
                }
                else if (pad_data->mean < target_midpoint)
                {
                    if (pad_data->pad.target_water == pad_data->mean)
                    {
                        pad_data->pad.state = STATE_WATER_MIN; //1
                    }
                    else
                    {
                        pad_data->pad.state = STATE_WATER_MIDPOINT; //2
                    }
                }
            }                                              //enough definition of targets seen

    }
}


/**
* \brief This function reviews the means of the last second's sample data for all pads to detect
*        a state change for each pad from water to air and vice versa.  Target means are maintained
*        for air and water.  Whenever a large jump up (450+ counts) is seen then a new target air
*        value is established.  A large jump down sets a new target water mean.   The jump "re-calibrates
*        the system for each state change so that the targets "move" with temperature fluctuations.
*        For each pad, the midpoint count is calculated between the two targets.k
*
* \ingroup PUBLIC_API
*/
void waterDetect_update_stats(void)
{
    uint8_t pad_number;
    Pad_Data_t *pad_data;

    for (pad_number = 0; pad_number < NUM_PADS; pad_number++)
    {
        pad_data = &pad_db[pad_number];

        if (pad_data->pad.num_samp)
        {
            if (pad_data->pad.last_mean)
            {
                heat_analysis(pad_number, pad_data);
                jump_analysis(pad_data);
                midpoint_analysis(pad_data);

            }                                              // if there is a previous mean (otherwise wait one more second)
            else
            {
                // first time only set targets to first mean that was measured
                pad_data->pad.target_air = pad_data->pad.target_water = pad_data->mean;
            }
        }                                                  // if samples exist
    }                                                      // end for
}

/**
* \brief This function calculates the number of samples across all pads for the current time period
* 
* @return uint8_t  Returns number of capacitance measurement samples in the current time period
*/

uint8_t waterDetect_read_sample_count(void)
{
    uint8_t pad_number;
    uint8_t answer = 0;

    for (pad_number = 0; pad_number < NUM_PADS; pad_number++) 
	    answer += pad_db[pad_number].pad.num_samp;

    return (answer);
}

#ifndef WATERDETECT_READ_WATER_LEVEL_NORMAL
// Test Mounting
// Lev 6  00111111 0x3f
// Lev 5  00011111 0x1f
// Lev 4  00001111 0x0f
// Lev 3  00000111 0x07
// Lev 2  00000011 0x03
// Lev 1  00000001 0x01
// Lev 0  00000000 0x00
static const uint8_t pad_coverage[NUM_PADS] = { 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f };
#else
// Normal Mounting
// Lev 6  00111111 0x3f
// Lev 5  00111110 0x3e
// Lev 4  00111100 0x3c
// Lev 3  00111000 0x38
// Lev 2  00110000 0x30
// Lev 1  00100000 0x20
// Lev 0  00000000 0x00
static const uint8_t pad_coverage[NUM_PADS] = { 0x20, 0x30, 0x38, 0x3c, 0x3e, 0x3f };
#endif


/**
* \brief This function determines how many pads are covered with water and detects any unknown states
*        where a pad is detected with water above another pad with air (possibly due to splashing).
* 
* @return uint8_t  Returns number of pads covered with water
*/
uint8_t waterDetect_read_water_level(uint8_t *submergedPadsBitMask, uint8_t *unknowns)
{
    uint8_t pad_number;
    uint8_t answer = 0;
    uint8_t submerged_mask = 0;

    *unknowns = 0;                                         // assume no unknowns

    // fill a bit mask with pad states
    for (pad_number = 0; pad_number < NUM_PADS; pad_number++)
    {
        if (pad_db[pad_number].pad.state == STATE_WATER_MIN || pad_db[pad_number].pad.state == STATE_WATER_MIDPOINT)
        {
            submerged_mask |= (uint8_t)1 << pad_number;
            if (pad_db[pad_number].submerged_count < 0xffff)
                pad_db[pad_number].submerged_count++;
        }
    }
#ifndef WATERDETECT_READ_WATER_LEVEL_NORMAL
    for (pad_number = 0; pad_number < NUM_PADS; pad_number++)
    {
        if (submerged_mask & (uint8_t)1 << pad_number)
            ++answer;
        else
            break;
    }
#else
    for (pad_number = NUM_PADS; pad_number; pad_number--)
    {
        if (submerged_mask & (uint8_t)1 << (pad_number - 1))
            ++answer;
        else
            break;
    }
#endif

    // all the submerged bits leading up to the water level must be set, if not we have a "splashing" scenario
    // this test makes a bit mask of all 1's up to the detected water level, the submergedPadsBitMask must
    // match it.
    if (answer)
    {
        if (pad_coverage[answer - 1] != submerged_mask)
        {
            submerged_mask |= (uint8_t)UNKNOWN_MASK;
            *unknowns = 1;
        }
    }
    else if (submerged_mask)
    {
        submerged_mask |= (uint8_t)UNKNOWN_MASK;
        *unknowns = 1;
    }

    // return the current mask
    *submergedPadsBitMask = submerged_mask;

    return (answer);
}

/**
* \brief These constants contain the downspout percentage as described in the waterDetect_get_flow_rate function
*        narritive below. The units are in tenths of a percent.
*
*                                                     L1   L2   L3   L4   L5  L6  */
static const uint16_t pad_drain_volume[NUM_PADS] = { 161, 193, 199, 186, 168, 93 };

/**
* \brief This function calculates the current flow rate of the pump by multiplying a maximum
*        flow rate constant for the pipe.  The exit pipe will restrict water flow based on the
*        height of the water in the collection chamber.  So to calculate the flow the code
*        adds up the volume for all the pads up to but not including the highest pad
*        the well's output pipe governs the flow of the water out.  The circular shape
*        of the opening restricts flow more at the top and bottom	(pardon the ASCII art
*        it has its limits)
*
*        The flow rate attributed to each pad is highest for pads 3 and 4, the flow rate is
*        slower for pads 1, 2 and 5, and even slower for pad 0.   When the water in the collection
*        chamber goes higher than the exit pipe, it will restrict higher measurements until gravity
*        evacuates the water out.
*
*                Top of Level 6
*                 , - ~ ~ - ,   (pad 0 - 9.3%)
*             , '      |      ' , (pad 1 - 16.8 %)
*            ,         |etc.     ,   (pad 2 - 18.6 %)
*           ,-------------------- ,
*          ,        Level 3        , (pad 3 - 19.9 %)
*          , --------------------- ,
*          ,        Level 2        , (pad 4 - 19.3 %)
*           ,-------------------- ,
*            ,      Level 1      ,  (pad 5 - 16.1 %)
*              ,-----metal----, '
*               ' - , _ _ , '
*
*
*        Pad Drain Volume:  This is the percentage of total volume, should the output pipe be completely
*        filled.  In tenths of percent (area calculations: https://planetcalc.com/1421/)
*
*        pipe radius			1.5
*        weld radius			0.22
*        remaining			    1.28
*        segment height			0.213
*        height  total   pad     downspout
*        			area    area    percent
*        6	1.50	1.77	0.15	9.3%
*        5	1.29	1.62	0.27	16.8%
*        4	1.07	1.35	0.3	    18.6%
*        3	0.86	1.05	0.32	19.9%
*        2	0.65	0.73	0.31	19.3%
*        1	0.43	0.42	0.26	16.1%
*       	metal	0.22	0.16
*         					1.61	100.0%  
*
*
* @param level the current detected water level
* @param *percentile address where calculated percentile value is returned. this is the percentage that 
*                    the highest pad covered with water is covered with water
*
* 
* @return uint16_t  Returns the water flow rate detected in ml
* \ingroup PUBLIC_API
*/

uint16_t waterDetect_get_flow_rate(uint8_t level, uint8_t *percentile)
{
    Pad_Data_t *pad_data;

    uint16_t answer = 0;
    uint32_t volume;
    uint8_t pad_number;
    uint16_t pad_diff;
    uint32_t mean_diff;

    if (level && level <= NUM_PADS)
    {
        // pad data points to the highest pad that is covered with water
#ifndef WATERDETECT_READ_WATER_LEVEL_NORMAL
        pad_data = &pad_db[level - 1];
#else
        pad_data = &pad_db[NUM_PADS - level];
#endif
        // anything above Level 1 and there are "whole" pads covered with water.  These are counted 100%
        // the highest "partially covered" pad is proportionally counted.
        // volume is the total percentage of the output pipe filled with water
        if (level > 1)
        {
            volume = 0;

            for (pad_number = 0; pad_number < level - 1; pad_number++)
            {
                volume += pad_drain_volume[pad_number];
            }
            volume /= 10;                                  // convert to whole percent
            answer = (volume * sysExecData.downspout_rate) / 100;
        }
        //calculate the percentile of the current mean within the targets, multiply to zoneFlowRate
        // and add to total
        //

        //        | lowest<---------------------------> highest| cap value
        //        |100%<------------------0%| not significant  |
        //        |                         | trickle water    |
        // water  |            <---%fill--->|<======450=======>| air
        // target |<-------Pad Diff-------->|                  | target
        //        |            <--MeanDiff->|                  |
        //        |   WATER    XxxxxxAIRxxAIRxxAIRxxAIRxxAIRxxx|
        //        |            |            |                  |
        //        |           Mean          |                  |

        pad_diff = pad_data->pad.target_air - pad_data->pad.target_water - SAMPLE_MIN_STATE_JUMP;
        mean_diff = (uint32_t)(pad_data->pad.target_air - pad_data->mean);
        if (mean_diff > SAMPLE_MIN_STATE_JUMP)
        {
            //when the mean equals water target the entire pad is covered with water
            if (pad_data->mean != pad_data->pad.target_water)
            {
                mean_diff -= SAMPLE_MIN_STATE_JUMP;
                mean_diff *= 100;
                mean_diff /= pad_diff;
                // as the proportion of pad diff to mean diff grows, the coverage approaches 100%
                mean_diff = mean_diff % 100;
            }
            else
                mean_diff = 100;

            *percentile = (uint8_t)mean_diff;

            // eliminate trickle volume levels
            if (level == 1 && *percentile < 50)
                mean_diff = 0;

#ifndef WATERDETECT_READ_WATER_LEVEL_NORMAL
            volume = (mean_diff * pad_drain_volume[level - 1] * sysExecData.downspout_rate) / 100000;
#else
            volume = (mean_diff * pad_drain_volume[NUM_PADS - level] * sysExecData.downspout_rate) / 100000;
#endif

            answer = (answer + volume) * SECONDS_PER_TREND;
        }
        else
        {
            // no significant water measured over "top pad"
            *percentile = 0;
        }

    }
    else
        *percentile = 0;

    return (answer);
}
/**
* \brief This function gives access to the current Target AIR capacitance level for a specific PAD
* 
* @param  padId The number of pad to read
* @return unit16_t Returns the current capacitance target level for AIR
* \ingroup PUBLIC_API
*/
uint16_t waterDetect_getTargetAir(uint8_t padId)
{
    return (pad_db[padId].pad.target_air);
}

/**
* \brief This function gives access to the current Target WATER capacitance level for a specific PAD
* 
* @param  padId The number of pad to read
* @return unit16_t Returns the current capacitance target level for WATER
* \ingroup PUBLIC_API
*/

uint16_t waterDetect_getTargetWater(uint8_t padId)
{
    return (pad_db[padId].pad.target_water);
}

/**
* \brief Return subMerged count statistic for pad
* @param  padId The number of pad to read
*
* @return uint16_t Current submerged measured value for pad
* \ingroup PUBLIC_API
*/
uint16_t waterDetect_getPadSubmergedCount(uint8_t padId)
{
    return (pad_db[padId].submerged_count);
}

/**
* \brief Return gap between target air and target water
*
* @param  padId The number of pad to read
*
* @return uint16_t Current submerged measured value for pad
* \ingroup PUBLIC_API
*/
uint16_t waterDetect_getPadTargetWidth(uint8_t padId)
{
    return (pad_db[padId].pad.target_air - pad_db[padId].pad.target_water);
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

/**
* \brief This function provides current information a pad's state
* 
* @param  pad_number The number of pad to read
* @param  state Address where the state of the pad (Water, Air, Unknown) is written
* @param  *num_samp Address where the number of samples read in the last 2 seconds is written
* @param  *mean Address where the mean of all samples in the last 2 seconds is written 
*
* @return unit16_t Returns the current air-deviation between the air target and the mean
* \ingroup PUBLIC_API
*/
int16_t waterDetect_getPadState(uint8_t pad_number, uint8_t *state, uint8_t *num_samp, uint16_t *mean)
{
    int16_t answer;

    switch (pad_db[pad_number].pad.state)
    {
        case STATE_WATER_MIN:
            *state = 'W';
            break;
        case STATE_WATER_MIDPOINT:
            *state = 'w';
            break;
        case STATE_AIR_MAX:
            *state = 'A';
            break;
        case STATE_AIR_MIDPOINT:
            *state = 'a';
            break;
        default:
            *state = '?';
    }
    *num_samp = pad_db[pad_number].pad.num_samp;
    *mean = pad_db[pad_number].mean;
    answer = pad_db[pad_number].pad.target_air - pad_db[pad_number].mean;
    return (answer);
}

/**
* \brief This function gives access to the last calculated temperature change for the air target for a specific PAD
* 
* @param  pad_number The number of pad to read
* @return unit16_t Returns the last calculated air target change 
* \ingroup PUBLIC_API
*/
int16_t waterDetect_getPadChange_Air(uint8_t pad_number)
{
    return (pad_db[pad_number].change_air);
}

/**
* \brief This function gives access to the last calculated temperature change for the water target for a specific PAD
* 
* @param  pad_number The number of pad to read
* @return unit16_t Returns the last calculated air target change 
* \ingroup PUBLIC_API
*/
int16_t waterDetect_getPadChange_Water(uint8_t pad_number)
{
    return (pad_db[pad_number].change_water);
}

/**
* \brief This function reports if a temperature change occurred for water or air for a specific PAD
* 
* @param  pad_number The number of pad to read
* @return unit8_t Returns 1 if a change ocurred 
* \ingroup PUBLIC_API
*/
uint8_t waterDetect_getPadChange(uint8_t pad_number)
{
    if (pad_db[pad_number].change_air || pad_db[pad_number].change_water)
        return (1);
    else
        return (0);
}

/**
* \brief This function fills in the selected telemetry data for water detection. This data is used
*        to fill in the SENSOR DATA message.
*
* @param *wr_in address where SENSOR DATA message data is written
*  
* \ingroup PUBLIC_API
*/
void waterDetect_getPadInfo(Pad_Info_t *wr_in)
{
    uint8_t i;

    if (wr_in)
    {
        wr_in->curr_temp = waterSense_getTempCelcius();
        wr_in->sequential_unknowns = padStats.sequential_unknowns;
        // copy it all from the detect data
        for (i = 0; i < NUM_PADS; i++) 
		    memcpy(&wr_in->pad[i], &pad_db[i].pad, sizeof(Pad_Stats_t));
    }
}


uint8_t waterDetect_waterPresent(uint16_t sample, uint8_t pad)
{
	uint8_t answer = false;
	uint16_t air_deviation;

	// make sure we have a valid air target
	if (pad_db[pad].pad.state != STATE_UNKNOWN )
	{
		if (pad_db[pad].pad.target_air > sample)
		{
	       air_deviation = pad_db[pad].pad.target_air - sample;
	       if (air_deviation >= SAMPLE_MIN_STATE_JUMP)
	    	   answer = true;
		}
	}
	else
	{
	    // if we don't have established air/water targets assume water is there (stay awake)
		answer = true;
	}

	return (answer);
}
