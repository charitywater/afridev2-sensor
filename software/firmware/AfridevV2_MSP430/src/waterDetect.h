/*
 * waterDetect.h
 *
 *  Created on: Nov 7, 2016
 *      Author: robl
 */

#ifndef SRC_waterDetect_H_
#define SRC_waterDetect_H_

#include "outpour.h"

#define SAMPLE_COUNT TICKS_PER_TREND
#define SAMPLE_MIN 0x8000
#define SAMPLE_MAX 0xB000

#define SAMPLE_MAX_JUMP 1500
#define SAMPLE_MIN_STATE_JUMP 450
#define SAMPLE_TEMP_SHIFT_ALLOW 30
#define SAMPLE_MIN_TARGET_RANGE 600

#define SENSOR_MIN_DOWNSPOUT 400
// measured error -6% unit 16 v2.3 14 samples (580)
// returning to old constant with flow bug fix v2.4 469
#define TUNED_DOWNSPOUT_RATE 469
#define SENSOR_MAX_DOWNSPOUT 800

// if water is stuck on for 5 minutes, then reset the detection
#define WATER_STUCK_LIMIT 150

#define MIN_OPERATING_TEMP 100

#define NUM_PADS 6
#define OUTLIER 0xFFFF
#define MAX_PAD_VAL 0xFFF0

#define DBG_LINE_SIZE 48

#define UNKNOWN_MASK 0x80

// the following define must NOT be commented out for production use
//#define waterDetect_READ_WATER_LEVEL_NORMAL

#define STATE_UNKNOWN 0
#define STATE_WATER_MIN 1
#define STATE_WATER_MIDPOINT 2
#define STATE_AIR_MAX 3
#define STATE_AIR_MIDPOINT 4

/**
 * \typedef Pad_Stats_t
 * \brief 12 byte structure to hold water detection telemetry data that is reported in the SENSOR DATA message
 */
typedef struct
{
    uint16_t last_mean;                                    /**< previous second's mean */
    uint16_t target_air;                                   /**< highest air mean */
    int16_t podtemp_air;                                   /**< pod temperature when air target is set */
    uint16_t target_water;                                 /**< lowest water mean */
    int16_t podtemp_water;                                 /**< pod temperature when water target is set */
    uint8_t state;                                         /**< current state of the pad ('w'=water, 'a'=air, '?'=unsure */
    uint8_t num_samp;                                      /**< last number of samples */
} Pad_Stats_t;

/**
 * \typedef Pad_Data_t
 * \brief 21 byte structure to hold all water detection telemetry data for one pad
 */
typedef struct
{
    // public data
    Pad_Stats_t pad;                                       /**< Public telemetry data for water detection */

    // private data
    uint16_t mean;                                         /**< current mean of valid samples */
    uint16_t submerged_count;                              /**< number of seconds the pad was submerged */
    int16_t change_air;                                    /**< latest temperature change to air target */
    int16_t change_water;                                  /**< latest temperature change to water target*/
    uint8_t cursor;                                        /**< current sample to load */
} Pad_Data_t;

/**
 * \typedef Pad_Info_t
 * \brief 76 byte structure to hold data to fill in SENSOR_DATA message
 */
typedef struct
{
    int16_t curr_temp;                                     /**<  2 bytes: the current pad temperature */
    uint16_t sequential_unknowns;                          /**<  2 bytes: the number of sequential measurements with unknowns */
    Pad_Stats_t pad[NUM_PADS];                             /**<  72 bytes: selected data from the pads */
} Pad_Info_t;

/**
 * \typedef Pad_Info_t
 * \brief 76 byte structure to hold data to fill in SENSOR_DATA message
 */
typedef struct
{
    uint16_t sample[SAMPLE_COUNT];                         /**< rolling buffer of samples */
} Sample_Data_t;

void waterDetect_init(void);
void waterDetect_clear_stats(void);
void waterDetect_add_sample(uint8_t pad_number, uint16_t pad_meas);
void waterDetect_update_stats(void);
void waterDetect_mark_outliers(void);
uint8_t waterDetect_debug_msg(uint8_t *dst, uint8_t pad);
uint8_t waterDetect_read_water_level(uint8_t *submergedPadsBitMask, uint8_t *unknowns);
uint8_t waterDetect_read_sample_count(void);
uint16_t waterDetect_get_flow_rate(uint8_t level, uint8_t *percentile);
uint16_t waterDetect_getTargetAir(uint8_t padId);
uint16_t waterDetect_getTargetWater(uint8_t padId);
uint16_t waterDetect_getPadSubmergedCount(uint8_t padId);
uint16_t waterDetect_getCurrSample(uint8_t pad_number);
int16_t waterDetect_getPadState(uint8_t pad_number, uint8_t *state, uint8_t *num_samp, uint16_t *mean);
uint8_t waterDetect_getPadChange(uint8_t pad_number);
int16_t waterDetect_getPadChange_Air(uint8_t pad_number);
int16_t waterDetect_getPadChange_Water(uint8_t pad_number);
void waterDetect_record_pads_baseline(void);

uint16_t waterDetect_getPadTargetWidth(uint8_t padId);
void waterDetect_set_water_target(void);
void waterDetect_set_air_target(void);
void waterDetect_getPadInfo(Pad_Info_t *wr_in);

#ifdef WATER_DEBUG
void waterDetect_record_pads_water(void);
#endif


#endif /* SRC_waterDetect_H_ */
