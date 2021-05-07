/*
 * File: initializeWaterAlgorithm_types.h
 *
 * MATLAB Coder version            : 5.0
 * C/C++ source code generated on  : 02-Apr-2021 23:00:10
 *
 * \par  Copyright Notice
 *       Copyright 2021 charity: water
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *           http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 *
 */

#ifndef INITIALIZEWATERALGORITHM_TYPES_H
#define INITIALIZEWATERALGORITHM_TYPES_H

/* Include Files */
#include "rtwtypes.h"

/* Type Definitions */
#ifndef typedef_AlgoState
#define typedef_AlgoState

typedef uint8_T AlgoState;

#endif                                 /*typedef_AlgoState*/

#ifndef AlgoState_constants
#define AlgoState_constants

/* enum AlgoState */
#define b_water_present                ((AlgoState)0U)
#define water_volume                   ((AlgoState)1U)
#endif                                 /*AlgoState_constants*/

#ifndef typedef_PresentType
#define typedef_PresentType

typedef uint8_T PresentType;

#endif                                 /*typedef_PresentType*/

#ifndef PresentType_constants
#define PresentType_constants

/* enum PresentType */
#define water_not_present              ((PresentType)0U)
#define water_draining                 ((PresentType)1U)
#define water_present                  ((PresentType)2U)
#endif                                 /*PresentType_constants*/

#ifndef typedef_padFilteringData_t
#define typedef_padFilteringData_t

typedef struct {
  uint8_T buffer_idx;
  uint16_T pad_5_buffer[4];
  uint16_T pad_4_buffer[4];
  uint16_T pad_3_buffer[4];
  uint16_T pad_2_buffer[4];
  uint16_T pad_1_buffer[4];
  uint16_T pad_0_buffer[4];
} padFilteringData_t;

#endif                                 /*typedef_padFilteringData_t*/

#ifndef typedef_padWaterState_t
#define typedef_padWaterState_t

typedef struct {
  PresentType present_type;
  uint8_T draining_count;
} padWaterState_t;

#endif                                 /*typedef_padWaterState_t*/

#ifndef typedef_waterAlgoData_t
#define typedef_waterAlgoData_t

typedef struct {
  uint8_T present;
  uint8_T water_stop_detected;
  uint8_T pad5_stop_detected;
  padWaterState_t pad5_present;
  padWaterState_t pad4_present;
  padWaterState_t pad3_present;
  padWaterState_t pad2_present;
  padWaterState_t pad1_present;
  padWaterState_t pad0_present;
  uint32_T accum_water_height;
  uint32_T accum_water_volume;
  uint32_T water_height_counter;
  uint32_T prev_water_height;
  uint32_T not_present_counter;
  uint32_T constant_height_counter;
  AlgoState algo_state;
} waterAlgoData_t;

#endif                                 /*typedef_waterAlgoData_t*/
#endif

/*
 * File trailer for initializeWaterAlgorithm_types.h
 *
 * [EOF]
 */
