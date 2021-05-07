/*
 * File: initializeWaterAlgorithm.c
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

/* Include Files */
#include "initializeWaterAlgorithm.h"
#include <string.h>

/* Function Definitions */

/*
 * Arguments    : waterAlgoData_t *Algorithm_Data
 *                padFilteringData_t *Pad_Filter_Data
 * Return Type  : void
 */
void initializeWaterAlgorithm(waterAlgoData_t *Algorithm_Data,
  padFilteringData_t *Pad_Filter_Data)
{
  Pad_Filter_Data->buffer_idx = 0U;
  memset(&Pad_Filter_Data->pad_5_buffer[0], 0, 4U * sizeof(uint16_T));
  memset(&Pad_Filter_Data->pad_4_buffer[0], 0, 4U * sizeof(uint16_T));
  memset(&Pad_Filter_Data->pad_3_buffer[0], 0, 4U * sizeof(uint16_T));
  memset(&Pad_Filter_Data->pad_2_buffer[0], 0, 4U * sizeof(uint16_T));
  memset(&Pad_Filter_Data->pad_1_buffer[0], 0, 4U * sizeof(uint16_T));
  memset(&Pad_Filter_Data->pad_0_buffer[0], 0, 4U * sizeof(uint16_T));

  /*  Initialize to water present state */
  Algorithm_Data->algo_state = b_water_present;

  /*  Reset session variables */
  Algorithm_Data->present = 0U;
  Algorithm_Data->water_stop_detected = 0U;
  Algorithm_Data->pad5_stop_detected = 0U;
  Algorithm_Data->not_present_counter = 0UL;
  Algorithm_Data->constant_height_counter = 0UL;
  Algorithm_Data->prev_water_height = 0UL;

  /*  Reset the pad water state */
  Algorithm_Data->pad5_present.present_type = water_not_present;
  Algorithm_Data->pad5_present.draining_count = 0U;

  /*  Reset the pad water state */
  Algorithm_Data->pad4_present.present_type = water_not_present;
  Algorithm_Data->pad4_present.draining_count = 0U;

  /*  Reset the pad water state */
  Algorithm_Data->pad3_present.present_type = water_not_present;
  Algorithm_Data->pad3_present.draining_count = 0U;

  /*  Reset the pad water state */
  Algorithm_Data->pad2_present.present_type = water_not_present;
  Algorithm_Data->pad2_present.draining_count = 0U;

  /*  Reset the pad water state */
  Algorithm_Data->pad1_present.present_type = water_not_present;
  Algorithm_Data->pad1_present.draining_count = 0U;

  /*  Reset the pad water state */
  Algorithm_Data->pad0_present.present_type = water_not_present;
  Algorithm_Data->pad0_present.draining_count = 0U;

  /*  Reset session volume variables */
  Algorithm_Data->accum_water_height = 0UL;
  Algorithm_Data->water_height_counter = 0UL;

  /*  Reset accumulated water volume */
  Algorithm_Data->accum_water_volume = 0UL;
}

/*
 * File trailer for initializeWaterAlgorithm.c
 *
 * [EOF]
 */
