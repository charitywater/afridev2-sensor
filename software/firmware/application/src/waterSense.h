/*
 * waterSense.h
 *
 *  Created on: Dec 1, 2016
 *      Author: robl
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

#ifndef SRC_WATERSENSE_H_
#define SRC_WATERSENSE_H_

#define UNKNOWN_TRICKLE_VOLUME 0x0FFFF
#define TRICKLE_VOLUME_TOL 10
#define AIRWAIT_TIME 4
#define PUMP_ACTIVE_LEVEL 3

void waterSense_takeReading(void);
void waterSenseReadInternalTemp(void);

#endif /* SRC_WATERSENSE_H_ */
