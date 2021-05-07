/*
 * File: detectWaterChange.c
 *
 * MATLAB Coder version            : 5.0
 * C/C++ source code generated on  : 02-Apr-2021 23:01:04
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
#include "detectWaterChange.h"
#include "calculateWaterVolume.h"

/* Function Definitions */

/*
 * Arguments    : int32_T diff
 *                padWaterState_t *water_state
 *                uint8_T counter_thresh
 * Return Type  : uint8_T
 */
uint8_T detectWaterChange(int32_T diff, padWaterState_t *water_state, uint8_T
  counter_thresh)
{
  uint8_T state_changed;

  /*  Variable to hold differential value */
  state_changed = 0U;
  switch (water_state->present_type) {
   case water_not_present:
    if (diff <= -30L) {
      water_state->present_type = water_present;
      water_state->draining_count = 0U;
      state_changed = 1U;
    }
    break;

   case water_present:
    if (diff >= 10L) {
      water_state->present_type = water_draining;
      water_state->draining_count = 0U;
      state_changed = 1U;
    }
    break;

   default:
    if (water_state->draining_count < 255) {
      water_state->draining_count++;
    }

    if (diff <= -30L) {
      water_state->present_type = water_present;
      water_state->draining_count = 0U;
      state_changed = 1U;
    } else {
      if ((water_state->draining_count > counter_thresh) && (diff < 10L)) {
        water_state->present_type = water_not_present;
        water_state->draining_count = 0U;
        state_changed = 1U;
      }
    }
    break;
  }

  return state_changed;
}

/*
 * File trailer for detectWaterChange.c
 *
 * [EOF]
 */
