/*
 * File: calculateWaterVolume.c
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
#include "calculateWaterVolume.h"
#include "detectWaterChange.h"

/* Function Declarations */
static void add_reason_code(ReasonCodes reason_code, ReasonCodes reason_codes[4]);
static void read_sample(uint16_T b_index, const uint16_T pad_window_blockA_pad5
  [10], const uint16_T pad_window_blockA_pad4[10], const uint16_T
  pad_window_blockA_pad3[10], const uint16_T pad_window_blockA_pad2[10], const
  uint16_T pad_window_blockA_pad1[10], const uint16_T pad_window_blockA_pad0[10],
  const uint16_T pad_window_blockB_pad5[10], const uint16_T
  pad_window_blockB_pad4[10], const uint16_T pad_window_blockB_pad3[10], const
  uint16_T pad_window_blockB_pad2[10], const uint16_T pad_window_blockB_pad1[10],
  const uint16_T pad_window_blockB_pad0[10], const b_padBlock_t
  *pad_window_blockOA, const b_padBlock_t *pad_window_blockOB, Window
  pad_window_read_window, uint8_T *success, uint16_T *sample_pad5, uint16_T
  *sample_pad4, uint16_T *sample_pad3, uint16_T *sample_pad2, uint16_T
  *sample_pad1, uint16_T *sample_pad0);

/* Function Definitions */

/*
 * Arguments    : ReasonCodes reason_code
 *                ReasonCodes reason_codes[4]
 * Return Type  : void
 */
static void add_reason_code(ReasonCodes reason_code, ReasonCodes reason_codes[4])
{
  uint8_T found;
  int16_T i;
  boolean_T exitg1;

  /*  Does the reason code already exist in the list? */
  found = 0U;
  if (reason_code == reason_codes[0]) {
    found = 1U;
  }

  if (reason_code == reason_codes[1]) {
    found = 1U;
  }

  if (reason_code == reason_codes[2]) {
    found = 1U;
  }

  if (reason_code == reason_codes[3]) {
    found = 1U;
  }

  /*  If it doesn't exist, add if there is a free slot */
  if (found == 0) {
    i = 0;
    exitg1 = false;
    while ((!exitg1) && (i < 4)) {
      if (reason_codes[i] == reason_code_none) {
        reason_codes[i] = reason_code;
        exitg1 = true;
      } else {
        i++;
      }
    }
  }
}

/*
 * Arguments    : uint16_T b_index
 *                const uint16_T pad_window_blockA_pad5[10]
 *                const uint16_T pad_window_blockA_pad4[10]
 *                const uint16_T pad_window_blockA_pad3[10]
 *                const uint16_T pad_window_blockA_pad2[10]
 *                const uint16_T pad_window_blockA_pad1[10]
 *                const uint16_T pad_window_blockA_pad0[10]
 *                const uint16_T pad_window_blockB_pad5[10]
 *                const uint16_T pad_window_blockB_pad4[10]
 *                const uint16_T pad_window_blockB_pad3[10]
 *                const uint16_T pad_window_blockB_pad2[10]
 *                const uint16_T pad_window_blockB_pad1[10]
 *                const uint16_T pad_window_blockB_pad0[10]
 *                const b_padBlock_t *pad_window_blockOA
 *                const b_padBlock_t *pad_window_blockOB
 *                Window pad_window_read_window
 *                uint8_T *success
 *                uint16_T *sample_pad5
 *                uint16_T *sample_pad4
 *                uint16_T *sample_pad3
 *                uint16_T *sample_pad2
 *                uint16_T *sample_pad1
 *                uint16_T *sample_pad0
 * Return Type  : void
 */
static void read_sample(uint16_T b_index, const uint16_T pad_window_blockA_pad5
  [10], const uint16_T pad_window_blockA_pad4[10], const uint16_T
  pad_window_blockA_pad3[10], const uint16_T pad_window_blockA_pad2[10], const
  uint16_T pad_window_blockA_pad1[10], const uint16_T pad_window_blockA_pad0[10],
  const uint16_T pad_window_blockB_pad5[10], const uint16_T
  pad_window_blockB_pad4[10], const uint16_T pad_window_blockB_pad3[10], const
  uint16_T pad_window_blockB_pad2[10], const uint16_T pad_window_blockB_pad1[10],
  const uint16_T pad_window_blockB_pad0[10], const b_padBlock_t
  *pad_window_blockOA, const b_padBlock_t *pad_window_blockOB, Window
  pad_window_read_window, uint8_T *success, uint16_T *sample_pad5, uint16_T
  *sample_pad4, uint16_T *sample_pad3, uint16_T *sample_pad2, uint16_T
  *sample_pad1, uint16_T *sample_pad0)
{
  int16_T sample_pad5_tmp;
  int16_T b_sample_pad5_tmp;
  int16_T c_sample_pad5_tmp;
  int16_T d_sample_pad5_tmp;
  int16_T e_sample_pad5_tmp;
  int16_T f_sample_pad5_tmp;

  /*     %% Static functions */
  *success = 1U;
  if ((b_index < 1U) || (b_index > 60U) || (pad_window_read_window == no_window))
  {
    *success = 0U;
  } else {
    /*  Find which block the index falls into */
    if (b_index <= 25U) {
      /*  Read from the first overlap block */
      if (pad_window_read_window == windowA) {
        b_sample_pad5_tmp = (int16_T)b_index - 1;
        *sample_pad5 = pad_window_blockOA->pad5[b_sample_pad5_tmp];
        *sample_pad4 = pad_window_blockOA->pad4[b_sample_pad5_tmp];
        *sample_pad3 = pad_window_blockOA->pad3[b_sample_pad5_tmp];
        *sample_pad2 = pad_window_blockOA->pad2[b_sample_pad5_tmp];
        *sample_pad1 = pad_window_blockOA->pad1[b_sample_pad5_tmp];
        *sample_pad0 = pad_window_blockOA->pad0[b_sample_pad5_tmp];
      } else {
        sample_pad5_tmp = (int16_T)b_index - 1;
        *sample_pad5 = pad_window_blockOB->pad5[sample_pad5_tmp];
        *sample_pad4 = pad_window_blockOB->pad4[sample_pad5_tmp];
        *sample_pad3 = pad_window_blockOB->pad3[sample_pad5_tmp];
        *sample_pad2 = pad_window_blockOB->pad2[sample_pad5_tmp];
        *sample_pad1 = pad_window_blockOB->pad1[sample_pad5_tmp];
        *sample_pad0 = pad_window_blockOB->pad0[sample_pad5_tmp];
      }
    } else if (b_index <= 35U) {
      /*  Read from the non-overlap block */
      if (pad_window_read_window == windowA) {
        f_sample_pad5_tmp = (int16_T)b_index - 26;
        *sample_pad5 = pad_window_blockA_pad5[f_sample_pad5_tmp];
        *sample_pad4 = pad_window_blockA_pad4[f_sample_pad5_tmp];
        *sample_pad3 = pad_window_blockA_pad3[f_sample_pad5_tmp];
        *sample_pad2 = pad_window_blockA_pad2[f_sample_pad5_tmp];
        *sample_pad1 = pad_window_blockA_pad1[f_sample_pad5_tmp];
        *sample_pad0 = pad_window_blockA_pad0[f_sample_pad5_tmp];
      } else {
        e_sample_pad5_tmp = (int16_T)b_index - 26;
        *sample_pad5 = pad_window_blockB_pad5[e_sample_pad5_tmp];
        *sample_pad4 = pad_window_blockB_pad4[e_sample_pad5_tmp];
        *sample_pad3 = pad_window_blockB_pad3[e_sample_pad5_tmp];
        *sample_pad2 = pad_window_blockB_pad2[e_sample_pad5_tmp];
        *sample_pad1 = pad_window_blockB_pad1[e_sample_pad5_tmp];
        *sample_pad0 = pad_window_blockB_pad0[e_sample_pad5_tmp];
      }
    } else {
      /*  Read from last overlap block */
      if (pad_window_read_window == windowA) {
        d_sample_pad5_tmp = (int16_T)b_index - 36;
        *sample_pad5 = pad_window_blockOB->pad5[d_sample_pad5_tmp];
        *sample_pad4 = pad_window_blockOB->pad4[d_sample_pad5_tmp];
        *sample_pad3 = pad_window_blockOB->pad3[d_sample_pad5_tmp];
        *sample_pad2 = pad_window_blockOB->pad2[d_sample_pad5_tmp];
        *sample_pad1 = pad_window_blockOB->pad1[d_sample_pad5_tmp];
        *sample_pad0 = pad_window_blockOB->pad0[d_sample_pad5_tmp];
      } else {
        c_sample_pad5_tmp = (int16_T)b_index - 36;
        *sample_pad5 = pad_window_blockOA->pad5[c_sample_pad5_tmp];
        *sample_pad4 = pad_window_blockOA->pad4[c_sample_pad5_tmp];
        *sample_pad3 = pad_window_blockOA->pad3[c_sample_pad5_tmp];
        *sample_pad2 = pad_window_blockOA->pad2[c_sample_pad5_tmp];
        *sample_pad1 = pad_window_blockOA->pad1[c_sample_pad5_tmp];
        *sample_pad0 = pad_window_blockOA->pad0[c_sample_pad5_tmp];
      }
    }
  }
}

/*
 * Arguments    : waterAlgoData_t *algo_data
 *                const padWindows_t *pad_window
 *                ReasonCodes reason_codes[4]
 * Return Type  : void
 */
void calculateWaterVolume(waterAlgoData_t *algo_data, const padWindows_t
  *pad_window, ReasonCodes reason_codes[4])
{
  Window u;
  int16_T idx;
  uint8_T curr_sample_success;
  uint16_T curr_sample_pad5;
  uint16_T curr_sample_pad4;
  uint16_T curr_sample_pad3;
  uint16_T curr_sample_pad2;
  uint16_T curr_sample_pad1;
  uint16_T curr_sample_pad0;
  uint8_T present_sample_success;
  uint16_T prev_present_sample_pad5;
  uint16_T prev_present_sample_pad4;
  uint16_T prev_present_sample_pad3;
  uint16_T expl_temp;
  uint16_T b_expl_temp;
  uint16_T c_expl_temp;
  uint8_T vol_sample_success;
  uint16_T prev_vol_sample_pad5;
  uint16_T prev_vol_sample_pad4;
  uint16_T prev_vol_sample_pad3;
  uint16_T prev_vol_sample_pad2;
  uint16_T prev_vol_sample_pad1;
  uint16_T prev_vol_sample_pad0;
  int32_T pad5_present_diff;
  int32_T present_diff_sum;
  int32_T pad4_diff;
  int32_T pad5_diff;
  uint8_T pad0_state_changed;
  uint8_T pad1_state_changed;
  uint8_T pad2_state_changed;
  uint8_T pad3_state_changed;
  uint8_T pad4_state_changed;
  uint8_T pad5_state_changed;
  uint8_T state_changed;
  int16_T water_height;
  uint8_T b_water_not_present;
  uint32_T a;
  int64_T c;

  /*  Verify that the correct data is present */
  /*  Water Volume Algorithm */
  /*  Initialize reason code */
  reason_codes[0] = reason_code_none;
  reason_codes[1] = reason_code_none;
  reason_codes[2] = reason_code_none;
  reason_codes[3] = reason_code_none;
  u = pad_window->read_window;
  for (idx = 0; idx < 35; idx++) {
    /*  Read the current data */
    read_sample((uint16_T)(idx + 26), pad_window->blockA.pad5,
                pad_window->blockA.pad4, pad_window->blockA.pad3,
                pad_window->blockA.pad2, pad_window->blockA.pad1,
                pad_window->blockA.pad0, pad_window->blockB.pad5,
                pad_window->blockB.pad4, pad_window->blockB.pad3,
                pad_window->blockB.pad2, pad_window->blockB.pad1,
                pad_window->blockB.pad0, &pad_window->blockOA,
                &pad_window->blockOB, u, &curr_sample_success, &curr_sample_pad5,
                &curr_sample_pad4, &curr_sample_pad3, &curr_sample_pad2,
                &curr_sample_pad1, &curr_sample_pad0);

    /*  Read the past present sample */
    read_sample(idx + 5U, pad_window->blockA.pad5, pad_window->blockA.pad4,
                pad_window->blockA.pad3, pad_window->blockA.pad2,
                pad_window->blockA.pad1, pad_window->blockA.pad0,
                pad_window->blockB.pad5, pad_window->blockB.pad4,
                pad_window->blockB.pad3, pad_window->blockB.pad2,
                pad_window->blockB.pad1, pad_window->blockB.pad0,
                &pad_window->blockOA, &pad_window->blockOB, u,
                &present_sample_success, &prev_present_sample_pad5,
                &prev_present_sample_pad4, &prev_present_sample_pad3, &expl_temp,
                &b_expl_temp, &c_expl_temp);

    /*  Read the past volume sample */
    read_sample(idx + 21U, pad_window->blockA.pad5, pad_window->blockA.pad4,
                pad_window->blockA.pad3, pad_window->blockA.pad2,
                pad_window->blockA.pad1, pad_window->blockA.pad0,
                pad_window->blockB.pad5, pad_window->blockB.pad4,
                pad_window->blockB.pad3, pad_window->blockB.pad2,
                pad_window->blockB.pad1, pad_window->blockB.pad0,
                &pad_window->blockOA, &pad_window->blockOB, u,
                &vol_sample_success, &prev_vol_sample_pad5,
                &prev_vol_sample_pad4, &prev_vol_sample_pad3,
                &prev_vol_sample_pad2, &prev_vol_sample_pad1,
                &prev_vol_sample_pad0);
    if ((curr_sample_success != 0) && (present_sample_success != 0) &&
        (vol_sample_success != 0)) {
      /*  Differential signal for present detection */
      pad5_present_diff = (int32_T)curr_sample_pad5 - prev_present_sample_pad5;
      present_diff_sum = (((pad5_present_diff + curr_sample_pad4) -
                           prev_present_sample_pad4) + curr_sample_pad3) -
        prev_present_sample_pad3;
      pad4_diff = (int32_T)curr_sample_pad4 - prev_vol_sample_pad4;
      pad5_diff = (int32_T)curr_sample_pad5 - prev_vol_sample_pad5;

      /*  Debug */
      if (algo_data->algo_state == b_water_present) {
        /*  Detect the front of the water ON point */
        /*  These tend to be high frequency (high difference from point to point) */
        if ((present_diff_sum <= -1000L) || (pad5_present_diff <= -30L)) {
          algo_data->present = 1U;
          algo_data->algo_state = water_volume;

          /*  Debug */
        }
      } else {
        /*  Find the water height for this sample */
        /*  Water present for each pad */
        pad0_state_changed = detectWaterChange((int32_T)curr_sample_pad0 -
          prev_vol_sample_pad0, &algo_data->pad0_present, 10U);
        pad1_state_changed = detectWaterChange((int32_T)curr_sample_pad1 -
          prev_vol_sample_pad1, &algo_data->pad1_present, 10U);
        pad2_state_changed = detectWaterChange((int32_T)curr_sample_pad2 -
          prev_vol_sample_pad2, &algo_data->pad2_present, 10U);
        pad3_state_changed = detectWaterChange((int32_T)curr_sample_pad3 -
          prev_vol_sample_pad3, &algo_data->pad3_present, 20U);
        pad4_state_changed = detectWaterChange(pad4_diff,
          &algo_data->pad4_present, 30U);
        pad5_state_changed = detectWaterChange(pad5_diff,
          &algo_data->pad5_present, 40U);

        /*  Calculate water height - start with the highest pad and work down */
        /*  until we find a pad that is covered */
        /*  Check pad 0 */
        state_changed = 0U;
        water_height = 0;
        if ((algo_data->pad0_present.present_type == water_present) &&
            ((algo_data->pad1_present.present_type != water_not_present) ||
             (algo_data->pad2_present.present_type != water_not_present))) {
          water_height = 197;
        }

        if (water_height != 0) {
          if (pad0_state_changed != 0) {
            state_changed = 1U;
          }

          /*  Promote the state to the master state */
          if (algo_data->pad1_present.present_type <
              algo_data->pad0_present.present_type) {
            algo_data->pad1_present.present_type =
              algo_data->pad0_present.present_type;
            algo_data->pad1_present.draining_count = 0U;
          }

          /*  Promote the state to the master state */
          if (algo_data->pad2_present.present_type <
              algo_data->pad0_present.present_type) {
            algo_data->pad2_present.present_type =
              algo_data->pad0_present.present_type;
            algo_data->pad2_present.draining_count = 0U;
          }

          /*  Promote the state to the master state */
          if (algo_data->pad3_present.present_type <
              algo_data->pad0_present.present_type) {
            algo_data->pad3_present.present_type =
              algo_data->pad0_present.present_type;
            algo_data->pad3_present.draining_count = 0U;
          }

          /*  Promote the state to the master state */
          if (algo_data->pad4_present.present_type <
              algo_data->pad0_present.present_type) {
            algo_data->pad4_present.present_type =
              algo_data->pad0_present.present_type;
            algo_data->pad4_present.draining_count = 0U;
          }

          /*  Promote the state to the master state */
          if (algo_data->pad5_present.present_type <
              algo_data->pad0_present.present_type) {
            algo_data->pad5_present.present_type =
              algo_data->pad0_present.present_type;
            algo_data->pad5_present.draining_count = 0U;
          }
        }

        /*  Check pad 1 */
        if (water_height == 0) {
          if ((algo_data->pad1_present.present_type == water_present) &&
              ((algo_data->pad2_present.present_type != water_not_present) ||
               (algo_data->pad3_present.present_type != water_not_present))) {
            water_height = 164;
          }

          if (water_height != 0) {
            if (pad1_state_changed != 0) {
              state_changed = 1U;
            }

            /*  Promote the state to the master state */
            if (algo_data->pad2_present.present_type <
                algo_data->pad1_present.present_type) {
              algo_data->pad2_present.present_type =
                algo_data->pad1_present.present_type;
              algo_data->pad2_present.draining_count = 0U;
            }

            /*  Promote the state to the master state */
            if (algo_data->pad3_present.present_type <
                algo_data->pad1_present.present_type) {
              algo_data->pad3_present.present_type =
                algo_data->pad1_present.present_type;
              algo_data->pad3_present.draining_count = 0U;
            }

            /*  Promote the state to the master state */
            if (algo_data->pad4_present.present_type <
                algo_data->pad1_present.present_type) {
              algo_data->pad4_present.present_type =
                algo_data->pad1_present.present_type;
              algo_data->pad4_present.draining_count = 0U;
            }

            /*  Promote the state to the master state */
            if (algo_data->pad5_present.present_type <
                algo_data->pad1_present.present_type) {
              algo_data->pad5_present.present_type =
                algo_data->pad1_present.present_type;
              algo_data->pad5_present.draining_count = 0U;
            }
          }
        }

        /*  Check pad 2 */
        if (water_height == 0) {
          if ((algo_data->pad2_present.present_type == water_present) &&
              ((algo_data->pad3_present.present_type != water_not_present) ||
               (algo_data->pad4_present.present_type != water_not_present))) {
            water_height = 131;
          }

          if (water_height != 0) {
            if (pad2_state_changed != 0) {
              state_changed = 1U;
            }

            /*  Promote the state to the master state */
            if (algo_data->pad3_present.present_type <
                algo_data->pad2_present.present_type) {
              algo_data->pad3_present.present_type =
                algo_data->pad2_present.present_type;
              algo_data->pad3_present.draining_count = 0U;
            }

            /*  Promote the state to the master state */
            if (algo_data->pad4_present.present_type <
                algo_data->pad2_present.present_type) {
              algo_data->pad4_present.present_type =
                algo_data->pad2_present.present_type;
              algo_data->pad4_present.draining_count = 0U;
            }

            /*  Promote the state to the master state */
            if (algo_data->pad5_present.present_type <
                algo_data->pad2_present.present_type) {
              algo_data->pad5_present.present_type =
                algo_data->pad2_present.present_type;
              algo_data->pad5_present.draining_count = 0U;
            }
          }
        }

        /*  Check pad 3 */
        if (water_height == 0) {
          if ((algo_data->pad3_present.present_type == water_present) &&
              ((algo_data->pad4_present.present_type != water_not_present) ||
               (algo_data->pad5_present.present_type != water_not_present))) {
            water_height = 98;
          }

          if (water_height != 0) {
            if (pad3_state_changed != 0) {
              state_changed = 1U;
            }

            /*  Promote the state to the master state */
            if (algo_data->pad4_present.present_type <
                algo_data->pad3_present.present_type) {
              algo_data->pad4_present.present_type =
                algo_data->pad3_present.present_type;
              algo_data->pad4_present.draining_count = 0U;
            }

            /*  Promote the state to the master state */
            if (algo_data->pad5_present.present_type <
                algo_data->pad3_present.present_type) {
              algo_data->pad5_present.present_type =
                algo_data->pad3_present.present_type;
              algo_data->pad5_present.draining_count = 0U;
            }
          }
        }

        /*  Check pad 4 */
        if (water_height == 0) {
          if ((algo_data->pad4_present.present_type == water_present) &&
              (algo_data->pad5_present.present_type != water_not_present)) {
            water_height = 66;
          }

          if (water_height != 0) {
            if (pad4_state_changed != 0) {
              state_changed = 1U;
            }

            /*  Promote the state to the master state */
            if (algo_data->pad5_present.present_type <
                algo_data->pad4_present.present_type) {
              algo_data->pad5_present.present_type =
                algo_data->pad4_present.present_type;
              algo_data->pad5_present.draining_count = 0U;
            }
          }
        }

        /*  Check pad 5 */
        if ((water_height == 0) && (algo_data->pad5_present.present_type !=
             water_not_present)) {
          water_height = 33;
          if (pad5_state_changed != 0) {
            state_changed = 1U;
          }
        }

        /*  Check for water not present - this is when the pad 4, 5 are */
        /*  all stable for a certain amount of time - keep the height */
        /*  at pad 5 for the duration this time */
        if ((algo_data->prev_water_height > 0UL) && (water_height == 0) &&
            (pad4_diff < 15L) && (pad5_diff < 15L)) {
          /*  Increment the not present counter */
          if (algo_data->not_present_counter < MAX_uint32_T) {
            algo_data->not_present_counter++;
          }

          /*  Keep pad5 in the draining state */
          water_height = 33;
          algo_data->pad5_present.present_type = water_draining;
          algo_data->pad5_present.draining_count = 40U;

          /*  Go sample by sample at this point - no timeout */
        } else {
          /*  Reset the counter  */
          algo_data->not_present_counter = 0UL;
        }

        /*  Compute features for scaler selection */
        if (water_height > 0) {
          if (algo_data->water_height_counter < MAX_uint32_T) {
            algo_data->water_height_counter++;
          }

          /*  Only count no changes when below pad 0 */
          /*                      if ~state_changed */
          /*                          if algo_data.no_change_counter < Constants.MAX_UINT32 */
          /*                              algo_data.no_change_counter = algo_data.no_change_counter + uint32(1); */
          /*                          end */
          /*                      end */
        }

        /*                  % Track the number of samples where water height is at or */
        /*                  % above pad 0 */
        /*                  if water_height == Constants.WTR_VOLUME_PAD0_HEIGHT && ... */
        /*                          algo_data.pad0_above_counter < Constants.MAX_UINT32 */
        /*                      algo_data.pad0_above_counter = algo_data.pad0_above_counter + uint32(1); */
        /*                  end */
        /*  Global timer to timeout when water height is constant for a long */
        /*  period of time */
        /*  Since the water can be over the top pad for a prolonged */
        /*  period of time, only count when the height is less than */
        /*  the top pad */
        if ((water_height > 0) && (algo_data->prev_water_height == (uint32_T)
             water_height) && (state_changed == 0)) {
          if (algo_data->constant_height_counter < MAX_uint32_T) {
            algo_data->constant_height_counter++;
          }

          /*  Check for standing water or clogged pump */
          if ((algo_data->constant_height_counter >= 3000UL) && ((uint16_T)
               water_height <= 66U)) {
            add_reason_code(water_flow_standing_water, reason_codes);
          }
        } else {
          algo_data->constant_height_counter = 0UL;
        }

        /*  Detect the water OFF point - this is the closest point when the water */
        /*  is only dribbling */
        b_water_not_present = 0U;
        if (present_diff_sum >= 300L) {
          algo_data->water_stop_detected = 1U;
        }

        /*  Reset water stopped flag if we get down to average again */
        if ((present_diff_sum < 0L) && (algo_data->water_stop_detected != 0)) {
          algo_data->pad5_stop_detected = 0U;
          algo_data->water_stop_detected = 0U;
        }

        /*  Look for pad 8 to change significantly */
        if ((algo_data->water_stop_detected != 0) && (pad5_present_diff >= 50L))
        {
          algo_data->pad5_stop_detected = 1U;
        }

        /*  Set water stopped if all conditions are met */
        if ((present_diff_sum < 20L) && (algo_data->water_stop_detected != 0) &&
            (algo_data->pad5_stop_detected != 0)) {
          b_water_not_present = 1U;
        }

        /*  Add height to integral value */
        if (algo_data->accum_water_height <= MAX_uint32_T - water_height) {
          algo_data->accum_water_height += water_height;
        } else {
          algo_data->accum_water_height = MAX_uint32_T;
        }

        algo_data->prev_water_height = (uint32_T)water_height;

        /*  Check for end of session */
        if ((algo_data->not_present_counter > 10UL) ||
            (algo_data->constant_height_counter >= 3000UL) ||
            (b_water_not_present != 0)) {
          /*  End the session */
          /*  Calculate the session volume */
          if (algo_data->water_height_counter != 0UL) {
            a = algo_data->accum_water_height / algo_data->water_height_counter;

            /*      if no_change_counter >= water_height_counter */
            /*          % Cap to 100% */
            /*          no_change_percent = Constants.WTR_VOLUME_MAX_NO_CHANGE_PERCENT; */
            /*      else */
            /*          if no_change_counter < idivide(Constants.MAX_UINT32, uint32(100), 'fix') */
            /*              num = no_change_counter * uint32(100); */
            /*              no_change_percent = idivide(num, water_height_counter, 'fix'); */
            /*          else */
            /*              % Cap to 100% */
            /*              no_change_percent = Constants.WTR_VOLUME_MAX_NO_CHANGE_PERCENT; */
            /*          end */
            /*      end */
          } else {
            a = 0UL;

            /*      no_change_percent = Constants.WTR_VOLUME_DEFAULT_NO_CHANGE_PERCENT; */
          }

          /*  Constants computed here (but will be magic numbers when generated to C */
          /*  code) */
          /*  Calculate the scaler (Y = b - mx , x = no_change_percentage) */
          /*  NOTE: This should never overflow with max values in each element - no */
          /* need to check for overflow */
          /* scaler = intercept - slope * int64(bitshift(no_change_percent, Constants.FI32_DEC_LOCATION)); */
          /*  Calculate the water volume */
          c = (algo_data->accum_water_height * ((842LL * (a << 15L) +
                 3221225472LL) >> 15L) * 1000LL) >> 30L;

          /*  Add session volume to total volume */
          if (algo_data->accum_water_volume <= MAX_uint32_T - (uint32_T)c) {
            algo_data->accum_water_volume += (uint32_T)c;
          } else {
            algo_data->accum_water_volume = MAX_uint32_T;
            add_reason_code(water_volume_capped, reason_codes);
          }

          /*  Debug */
          /*  Move to water present state */
          algo_data->algo_state = b_water_present;

          /*  Reset session variables */
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

          /*  Reset session volume variables */
          algo_data->accum_water_height = 0UL;
          algo_data->water_height_counter = 0UL;

          /*  NOTE: do not reset the accumulated water volume - */
          /*  this is reset when hourly water volume is computed */
        }
      }
    } else {
      add_reason_code(water_bad_sample, reason_codes);
    }

    /*  Debug */
  }
}

/*
 * File trailer for calculateWaterVolume.c
 *
 * [EOF]
 */
