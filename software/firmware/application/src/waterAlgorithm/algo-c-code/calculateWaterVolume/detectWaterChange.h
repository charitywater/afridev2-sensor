/*
 * File: detectWaterChange.h
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

#ifndef DETECTWATERCHANGE_H
#define DETECTWATERCHANGE_H

/* Include Files */
#include <stddef.h>
#include <stdlib.h>
#include "rtwtypes.h"
#include "calculateWaterVolume_types.h"

/* Function Declarations */
extern uint8_T detectWaterChange(int32_T diff, padWaterState_t *water_state,
  uint8_T counter_thresh);

#endif

/*
 * File trailer for detectWaterChange.h
 *
 * [EOF]
 */
