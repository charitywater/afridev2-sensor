/**
 * @file appAlgo.h
 * \n Header File
 * \n AfridevV2 MSP430 Firmware
 *
 * \brief Algorithm nest - the interface between MATLAB generated c code and the application
 *
 * \par   Copyright Notice
 *        Copyright 2021 charity: water
 *
 *        Licensed under the Apache License, Version 2.0 (the "License");
 *        you may not use this file except in compliance with the License.
 *        You may obtain a copy of the License at
 *
 *            http://www.apache.org/licenses/LICENSE-2.0
 *
 *        Unless required by applicable law or agreed to in writing, software
 *        distributed under the License is distributed on an "AS IS" BASIS,
 *        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *        See the License for the specific language governing permissions and
 *        limitations under the License.
 *
 */

#ifndef SRC_WATERALGORITHM_APPALGO_H_
#define SRC_WATERALGORITHM_APPALGO_H_

extern void APP_ALGO_init(void);
extern void APP_ALGO_wakeUpInit(void);
extern void APP_ALGO_runNest(void);
extern uint32_t APP_ALGO_getHourlyWaterVolume_ml(void);
extern uint16_t APP_ALGO_reportAlgoErrors(void);
extern bool APP_ALGO_isWaterPresent(void);

#endif /* SRC_WATERALGORITHM_APPALGO_H_ */
