/*
 * File: hourlyWaterVolume.c
 *
 * MATLAB Coder version            : 5.0
 * C/C++ source code generated on  : 02-Apr-2021 23:00:51
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
#include "hourlyWaterVolume.h"

/* Function Definitions */

/*
 * Arguments    : waterAlgoData_t *algo_data
 *                ReasonCodes *reason_code
 *                uint32_T *volume_ml
 *                uint32_T *volume_l
 * Return Type  : void
 */
void hourlyWaterVolume(waterAlgoData_t *algo_data, ReasonCodes *reason_code,
  uint32_T *volume_ml, uint32_T *volume_l)
{
  uint32_T session_volume;
  uint32_T a;

  /*  Function used to take teh hourly integration sum and convert it into a */
  /*  volume */
  session_volume = 0UL;
  *reason_code = reason_code_none;

  /*  Calculate the remaining volume if a session is going */
  if (algo_data->present != 0) {
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
    session_volume = (uint32_T)((algo_data->accum_water_height * ((842LL * (a <<
      15L) + 3221225472LL) >> 15L) * 1000LL) >> 30L);

    /*  Reset the session volume variables but not the pad variables - this could be */
    /*  called mid-session so we are cutting the session in half */
    algo_data->accum_water_height = 0UL;
    algo_data->water_height_counter = 0UL;
  }

  /*  Calculate volume */
  if (algo_data->accum_water_volume <= MAX_uint32_T - session_volume) {
    *volume_ml = algo_data->accum_water_volume + session_volume;
  } else {
    *volume_ml = MAX_uint32_T;
    *reason_code = water_volume_capped;
  }

  *volume_l = (*volume_ml + 500UL) / 1000UL;

  /*  Reset the accumulated water */
  algo_data->accum_water_volume = 0UL;
}

/*
 * File trailer for hourlyWaterVolume.c
 *
 * [EOF]
 */
