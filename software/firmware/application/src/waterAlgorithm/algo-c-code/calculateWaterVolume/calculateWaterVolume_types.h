/*
 * File: calculateWaterVolume_types.h
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

#ifndef CALCULATEWATERVOLUME_TYPES_H
#define CALCULATEWATERVOLUME_TYPES_H

/* Include Files */
#include "rtwtypes.h"

/* Type Definitions */
#ifndef typedef_AlgoState
#define typedef_AlgoState

typedef uint8_T AlgoState;

#endif                                 /*typedef_AlgoState*/

#ifndef AlgoState_constants
#define AlgoState_constants

/* enum AlgoState */
#define b_water_present                ((AlgoState)0U)
#define water_volume                   ((AlgoState)1U)
#endif                                 /*AlgoState_constants*/

#ifndef typedef_PresentType
#define typedef_PresentType

typedef uint8_T PresentType;

#endif                                 /*typedef_PresentType*/

#ifndef PresentType_constants
#define PresentType_constants

/* enum PresentType */
#define water_not_present              ((PresentType)0U)
#define water_draining                 ((PresentType)1U)
#define water_present                  ((PresentType)2U)
#endif                                 /*PresentType_constants*/

#ifndef typedef_ReasonCodes
#define typedef_ReasonCodes

typedef uint8_T ReasonCodes;

#endif                                 /*typedef_ReasonCodes*/

#ifndef ReasonCodes_constants
#define ReasonCodes_constants

/* enum ReasonCodes */
#define reason_code_none               ((ReasonCodes)0U)
#define water_flow_standing_water      ((ReasonCodes)1U)
#define water_bad_sample               ((ReasonCodes)10U)
#define water_volume_capped            ((ReasonCodes)20U)
#endif                                 /*ReasonCodes_constants*/

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

#ifndef typedef_padWaterState_t
#define typedef_padWaterState_t

typedef struct {
  PresentType present_type;
  uint8_T draining_count;
} padWaterState_t;

#endif                                 /*typedef_padWaterState_t*/

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

#ifndef typedef_waterAlgoData_t
#define typedef_waterAlgoData_t

typedef struct {
  uint8_T present;
  uint8_T water_stop_detected;
  uint8_T pad5_stop_detected;
  padWaterState_t pad5_present;
  padWaterState_t pad4_present;
  padWaterState_t pad3_present;
  padWaterState_t pad2_present;
  padWaterState_t pad1_present;
  padWaterState_t pad0_present;
  uint32_T accum_water_height;
  uint32_T accum_water_volume;
  uint32_T water_height_counter;
  uint32_T prev_water_height;
  uint32_T not_present_counter;
  uint32_T constant_height_counter;
  AlgoState algo_state;
} waterAlgoData_t;

#endif                                 /*typedef_waterAlgoData_t*/
#endif

/*
 * File trailer for calculateWaterVolume_types.h
 *
 * [EOF]
 */