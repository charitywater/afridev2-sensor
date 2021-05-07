/*
 * File: initializeWindows.c
 *
 * MATLAB Coder version            : 5.0
 * C/C++ source code generated on  : 02-Apr-2021 22:59:42
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
#include "initializeWindows.h"

/* Function Definitions */

/*
 * Arguments    : padWindows_t *pad_windows
 * Return Type  : void
 */
void initializeWindows(padWindows_t *pad_windows)
{
  pad_windows->write_idx = 0U;
  pad_windows->write_block = b_blockOA;
  pad_windows->read_window = no_window;
  pad_windows->process = 0U;
  pad_windows->first_pass = 1U;
}

/*
 * File trailer for initializeWindows.c
 *
 * [EOF]
 */
