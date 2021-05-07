/** 
 * @file linkAddr.h
 * \n Source File
 * \n AfridevV2 MSP430 Firmware
 * 
 * \brief Access address variables defined in the linker command
 *        file (lnk_msp430g2955.cmd).
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

#pragma once

//
//   External variables from linker file
//
extern uint16_t __Flash_Image_Length;
extern uint16_t __Flash_Image_Num_Sectors;
extern uint16_t __Flash_Main_Image_Start;
extern uint16_t __Flash_Main_Image_End;
extern uint16_t __Flash_Backup_Image_Start;
extern uint16_t __Flash_Backup_Image_End;
extern uint16_t __App_Reset_Vector;
extern uint16_t __Boot_Start;
extern uint16_t __App_Proxy_Vector_Start[];

static inline uint16_t getBackupImageStartAddr(void)
{
    return ((uint16_t)&__Flash_Backup_Image_Start);
}
static inline uint16_t getBackupImageEndAddr(void)
{
    return ((uint16_t)&__Flash_Backup_Image_End);
}
static inline uint16_t getAppImageStartAddr(void)
{
    return ((uint16_t)&__Flash_Main_Image_Start);
}
static inline uint16_t getAppImageEndAddr(void)
{
    return ((uint16_t)&__Flash_Main_Image_End);
}
static inline uint16_t getNumSectorsInImage(void)
{
    return ((uint16_t)&__Flash_Image_Num_Sectors);
}
static uint16_t getAppImageLength(void)
{
    return ((uint16_t)&__Flash_Image_Length);
}
static inline uint16_t getAppVectorTableAddr(void)
{
    return ((uint16_t)&__App_Proxy_Vector_Start);
}
static inline uint16_t getAppResetVector(void)
{
    return ((uint16_t)&__App_Reset_Vector);
}
static inline uint16_t getBootImageStartAddr(void)
{
    return ((uint16_t)&__Boot_Start);
}
