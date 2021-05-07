/*
 * File: wakeupDataReset.c
 *
 * MATLAB Coder version            : 5.0
 * C/C++ source code generated on  : 02-Apr-2021 23:00:24
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
#include "wakeupDataReset.h"
#include <string.h>

/* Function Definitions */

/*
 * Arguments    : padWindows_t *pad_windows
 *                padFilteringData_t *pad_filter_data
 *                waterAlgoData_t *algo_data
 * Return Type  : void
 */
void wakeupDataReset(padWindows_t *pad_windows, padFilteringData_t
                     *pad_filter_data, waterAlgoData_t *algo_data)
{
  /*  Reset the window data */
  pad_windows->write_idx = 0U;
  pad_windows->write_block = b_blockOA;
  pad_windows->read_window = no_window;
  pad_windows->process = 0U;
  pad_windows->first_pass = 1U;

  /*  Clear the water sample filtering buffers */
  pad_filter_data->buffer_idx = 0U;
  memset(&pad_filter_data->pad_5_buffer[0], 0, 4U * sizeof(uint16_T));
  memset(&pad_filter_data->pad_4_buffer[0], 0, 4U * sizeof(uint16_T));
  memset(&pad_filter_data->pad_3_buffer[0], 0, 4U * sizeof(uint16_T));
  memset(&pad_filter_data->pad_2_buffer[0], 0, 4U * sizeof(uint16_T));
  memset(&pad_filter_data->pad_1_buffer[0], 0, 4U * sizeof(uint16_T));
  memset(&pad_filter_data->pad_0_buffer[0], 0, 4U * sizeof(uint16_T));

  /*  Reset the session data and session volume data */
  algo_data->present = 0U;
  algo_data->water_stop_detected = 0U;
  algo_data->pad5_stop_detected = 0U;
  algo_data->not_present_counter = 0UL;
  algo_data->constant_height_counter = 0UL;
  algo_data->prev_water_height = 0UL;

  /*  Reset the pad water state */
  algo_data->pad5_present.present_type = water_not_present;
  algo_data->pad5_present.draining_count = 0U;

  /*  Reset the pad water state */
  algo_data->pad4_present.present_type = water_not_present;
  algo_data->pad4_present.draining_count = 0U;

  /*  Reset the pad water state */
  algo_data->pad3_present.present_type = water_not_present;
  algo_data->pad3_present.draining_count = 0U;

  /*  Reset the pad water state */
  algo_data->pad2_present.present_type = water_not_present;
  algo_data->pad2_present.draining_count = 0U;

  /*  Reset the pad water state */
  algo_data->pad1_present.present_type = water_not_present;
  algo_data->pad1_present.draining_count = 0U;

  /*  Reset the pad water state */
  algo_data->pad0_present.present_type = water_not_present;
  algo_data->pad0_present.draining_count = 0U;
  algo_data->accum_water_height = 0UL;
  algo_data->water_height_counter = 0UL;

  /*  Initialize to water present state */
  algo_data->algo_state = b_water_present;

  /*  NOTE: Do not reset the accumulated water volume - this gets reset when */
  /*  hourly water volume is computed */
}

/*
 * File trailer for wakeupDataReset.c
 *
 * [EOF]
 */
