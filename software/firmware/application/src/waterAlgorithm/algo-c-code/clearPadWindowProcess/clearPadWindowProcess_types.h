/*
 * File: clearPadWindowProcess_types.h
 *
 * MATLAB Coder version            : 5.0
 * C/C++ source code generated on  : 02-Apr-2021 23:00:01
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

#ifndef CLEARPADWINDOWPROCESS_TYPES_H
#define CLEARPADWINDOWPROCESS_TYPES_H

/* Include Files */
#include "rtwtypes.h"

/* Type Definitions */
#ifndef typedef_Window
#define typedef_Window

typedef uint8_T Window;

#endif                                 /*typedef_Window*/

#ifndef Window_constants
#define Window_constants

/* enum Window */
#define no_window                      ((Window)0U)
#define windowA                        ((Window)1U)
#define windowB                        ((Window)2U)
#endif                                 /*Window_constants*/

#ifndef typedef_WindowBlock
#define typedef_WindowBlock

typedef uint8_T WindowBlock;

#endif                                 /*typedef_WindowBlock*/

#ifndef WindowBlock_constants
#define WindowBlock_constants

/* enum WindowBlock */
#define b_blockA                       ((WindowBlock)1U)
#define b_blockB                       ((WindowBlock)2U)
#define b_blockOA                      ((WindowBlock)3U)
#define b_blockOB                      ((WindowBlock)4U)
#endif                                 /*WindowBlock_constants*/

#ifndef typedef_b_padBlock_t
#define typedef_b_padBlock_t

typedef struct {
  uint16_T pad5[25];
  uint16_T pad4[25];
  uint16_T pad3[25];
  uint16_T pad2[25];
  uint16_T pad1[25];
  uint16_T pad0[25];
} b_padBlock_t;

#endif                                 /*typedef_b_padBlock_t*/

#ifndef typedef_padBlock_t
#define typedef_padBlock_t

typedef struct {
  uint16_T pad5[10];
  uint16_T pad4[10];
  uint16_T pad3[10];
  uint16_T pad2[10];
  uint16_T pad1[10];
  uint16_T pad0[10];
} padBlock_t;

#endif                                 /*typedef_padBlock_t*/

#ifndef typedef_padWindows_t
#define typedef_padWindows_t

typedef struct {
  padBlock_t blockA;
  padBlock_t blockB;
  b_padBlock_t blockOA;
  b_padBlock_t blockOB;
  uint16_T write_idx;
  WindowBlock write_block;
  Window read_window;
  uint8_T process;
  uint8_T first_pass;
} padWindows_t;

#endif                                 /*typedef_padWindows_t*/
#endif

/*
 * File trailer for clearPadWindowProcess_types.h
 *
 * [EOF]
 */
