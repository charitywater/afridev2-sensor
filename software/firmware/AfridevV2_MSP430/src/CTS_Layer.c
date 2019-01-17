/* --COPYRIGHT--,BSD
 * Copyright (c) 2012, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
/*!
 *
 *  @file CTS_Layer.c
 *
 *
 * @brief This source file contains the API calls and one support function.
 *
 *
 *
 * @par Project:
 *             MSP430 Capacitive Touch Library
 *
 *
 *
 * @par Developed using:
 *             CCS Version : 5.4.0.00048, w/support for GCC extensions (--gcc)
 * \n IAR Version : 5.51.6 [Kickstart]
 *
 *
 * @author C. Sterzik
 * @author T. Hwang
 *
 *
 * @version 1.2
 *     Updated HALs for new devices.
 *
 *
 * @par Supported API Calls:
 *              - TI_CAPT_Raw()
 */

#include "CTS_Layer.h"

/***************************************************************************//**
 * @brief   Measure the capacitance of each element within the Sensor
 * 
 *          This function selects the appropriate HAL to perform the capacitance
 *          measurement based upon the halDefinition found in the sensor 
 *          structure. 
 *          The order of the elements within the Sensor structure is arbitrary 
 *          but must be consistent between the application and configuration. 
 *          The first element in the array (counts) corresponds to the first 
 *          element within the Sensor structure.
 * @param   groupOfElements Pointer to Sensor structure to be measured
 * @param   counts Address to where the measurements are to be written
 * @return  none
 ******************************************************************************/
void TI_CAPT_Raw(const struct Sensor *groupOfElements, uint16_t *counts)
{
#ifdef RO_PINOSC_TA1_TB0
    if (groupOfElements->halDefinition == RO_PINOSC_TA1_TB0)
    {
        TI_CTS_RO_PINOSC_TA1_TB0_HAL(groupOfElements, counts);
    }
#endif

}

