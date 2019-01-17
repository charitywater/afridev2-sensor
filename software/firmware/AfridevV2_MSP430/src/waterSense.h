/*
 * waterSense.h
 *
 *  Created on: Dec 1, 2016
 *      Author: robl
 */

#ifndef SRC_WATERSENSE_H_
#define SRC_WATERSENSE_H_

void waterSense_takeReading(void);
uint8_t waterSense_analyzeData(void);
void waterSenseReadInternalTemp(void);
int16_t waterSenseGetTemp(void);

/**
 * \typedef sensorStats_t
 * \brief define a type to hold stats from the water sensing
 *        algorithm
 */
typedef struct sensorStats_s {
    uint16_t lastMeasFlowRateInMl;                         /**< Last flow rate calculated */
    uint16_t unknowns;                                     /**< Overall count of unknown case (skipped pad) */
    uint16_t sequential_unknowns;                          /**< Number of Unknowns reported in a row */
    uint16_t unknown_limit;                                /**< Number of sequential unknowns until water detection is reset */
    uint16_t sequential_waters;                            /**< Number of sequential pumping while water is seen */
    uint16_t water_limit;                                  /**< Number of sequential measurements with water before restting */
    int16_t tempCelcius;                                   /**< Temperature reading from chip */
    uint8_t pump_active;                                   /**< Current status of pump */
    uint8_t air_wait;                                      /**< wait to reestablish air target */
} sensorStats_t;

extern sensorStats_t padStats;

#endif /* SRC_WATERSENSE_H_ */
