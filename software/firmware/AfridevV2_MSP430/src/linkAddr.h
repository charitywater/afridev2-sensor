/** 
 * @file linkAddr.h
 * \n Source File
 * \n AfridevV2 MSP430 Firmware
 * 
 * \brief Access address variables defined in the linker command
 *        file (lnk_msp430g2955.cmd).
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
