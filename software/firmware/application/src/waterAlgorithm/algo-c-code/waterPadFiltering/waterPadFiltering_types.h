/*
 * File: waterPadFiltering_types.h
 *
 * MATLAB Coder version            : 5.0
 * C/C++ source code generated on  : 02-Apr-2021 23:00:40
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

#ifndef WATERPADFILTERING_TYPES_H
#define WATERPADFILTERING_TYPES_H

/* Include Files */
#include "rtwtypes.h"

/* Type Definitions */
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

#ifndef typedef_padSample_t
#define typedef_padSample_t

typedef struct {
  uint16_T pad5;
  uint16_T pad4;
  uint16_T pad3;
  uint16_T pad2;
  uint16_T pad1;
  uint16_T pad0;
} padSample_t;

#endif                                 /*typedef_padSample_t*/
#endif

/*
 * File trailer for waterPadFiltering_types.h
 *
 * [EOF]
 */
