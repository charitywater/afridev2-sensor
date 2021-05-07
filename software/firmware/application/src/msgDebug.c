/** 
 * @file msgDebug.c
 * \n Source File
 * \n Outpour MSP430 Firmware
 * 
 * \brief Manage the details of sending debug messages.
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

#include "outpour.h"

/***************************
 * Module Data Definitions
 **************************/

/****************************
 * Module Data Declarations
 ***************************/

/*************************
 * Module Prototypes
 ************************/

/***************************
 * Module Public Functions
 **************************/

/**
* \brief Send an outpour debug msg to the uart. 
* \ingroup PUBLIC_API
* 
* @param msgId The outpour message identifier
* @param dataP Pointer to the data to send
* @param lengthInBytes The length of the data to send.
*/
void dbgMsgMgr_sendDebugMsg(MessageType_t msgId, uint8_t *dataP, uint16_t lengthInBytes)
{

    modemCmdWriteData_t cmdWrite;

    // Check if modem is busy.  If so, just exit
    if (modemMgr_isAllocated())
    {
        return;
    }
    memset(&cmdWrite, 0, sizeof(modemCmdWriteData_t));
    cmdWrite.cmd = OUTPOUR_M_COMMAND_SEND_DEBUG_DATA;
    cmdWrite.payloadMsgId = msgId;                         /* the payload type */
    cmdWrite.payloadP = dataP;                             /* the payload pointer */
    cmdWrite.payloadLength = lengthInBytes;                /* size of the payload in bytes */
    modemCmd_write(&cmdWrite);
    do
    {
        modemCmd_exec();
    } while (modemCmd_isBusy());
}

