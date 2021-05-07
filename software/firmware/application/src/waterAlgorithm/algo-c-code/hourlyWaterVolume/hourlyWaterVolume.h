/*
 * File: hourlyWaterVolume.h
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

#ifndef HOURLYWATERVOLUME_H
#define HOURLYWATERVOLUME_H

/* Include Files */
#include <stddef.h>
#include <stdlib.h>
#include "rtwtypes.h"
#include "hourlyWaterVolume_types.h"

/* Function Declarations */
extern void hourlyWaterVolume(waterAlgoData_t *algo_data, ReasonCodes
  *reason_code, uint32_T *volume_ml, uint32_T *volume_l);

#endif

/*
 * File trailer for hourlyWaterVolume.h
 *
 * [EOF]
 */
