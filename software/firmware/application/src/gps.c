/** 
 * @file gpsMsg.c
 * \n Source File
 * \n Outpour MSP430 Firmware
 * 
 * \brief GPS module responsible for controlling the GPS power
 *        (gpsPower.c) and GPS message (gpsMsg.c) modules.
 *
 * \brief When the Start GPS function is called, it will power
 *        on the GPS device and wait for a valid satellite fix.
 *        The GPS will be powered off as soon as a satellite fix
 *        is obtained. If no satellite fix occurs within
 *        MAX_ALLOWED_GPS_FIX_TIME_IN_SEC seconds, the GPS is
 *        powered off.
 *  
 * \brief The gps_start function should be called from the upper
 *        layers to perform GPS processing. After this module
 *        has completed its GPS processing, the last valid GGA 
 *        information can be retrieved using the API from
 *        the gpsMsg module. Regardless of whether the GPS was
 *        able to determine a satellite fix, the last GGA
 *        information is saved by the gpsMsg module for return
 *        to the cloud.
 *  
 * \brief The GPS device will stay powered on until the criteria
 *        for a good location fix is met or the maximum on-time
 *        occurs. There are three criteria parameters: Minimum
 *        Number of satellites, Maximum HDOP threshold and
 *        Minimum on time.
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
#ifdef WATER_DEBUG
#include "debugUart.h"
#endif

/***************************
 * Module Data Definitions
 **************************/

/**
 * \def MAX_ALLOWED_GPS_FIX_TIME_IN_SEC
 * Max time to have the GPS device on waiting for a satellite 
 * fix. 
 */
#define MAX_ALLOWED_GPS_FIX_TIME_IN_SEC ((15*60)*TIME_SCALER)

/**
 * \def MAX_GPS_RETRY_ON_ERROR
 * If an error occurs powering on the GPS device or a timeout 
 * occurs waiting for a GGA message, how many times the state 
 * machine will retry to get a valid satellite fix. 
 */
#define MAX_GPS_RETRY_ON_ERROR 4

/****************************
 * Module Data Declarations
 ***************************/

/**
 * \var gpsData
 * \brief Declare the object that contains the module data.
 */
gpsData_t gpsData;

#ifdef WATER_DEBUG
extern MANUF_STATE_T manuf_test_state;
#endif

/*************************
 * Module Prototypes
 ************************/

static void gps_stateMachine(void);

/***************************
 * Module Public Functions
 **************************/

/**
* \brief Module init routine. Call once at startup.
* 
* \ingroup PUBLIC_API
*/
void gps_init(void)
{
    memset(&gpsData, 0, sizeof(gpsData_t));
}

/**
* \brief GPS MSG executive. Called every 2 seconds from the main
*        loop to manage the GPS.
*
* \ingroup EXEC_ROUTINE
*/
void gps_exec(void)
{
    if (gpsData.active)
    {
        gps_stateMachine();
    }
}

/**
* \brief Start GPS processing. This will power on the GPS device 
*        and wait for a valid satellite fix. The GPS will be
*        powered off as soon as a satellite fix is obtained. If
*        no satellite fix occurs within
*        MAX_ALLOWED_GPS_FIX_TIME_IN_SEC seconds, the GPS is
*        powered off.
*  
* \note This function should be called from the upper layers to 
*       perform GPS processing. After this module has completed
*       its GPS processing, the data from the last received GGA
*       sentence information can be retrieved using the API from
*       the gpsMsg module. Regardless of whether the GPS was
*       able to determine a satellite fix, the last GGA sentence
*       info is saved by the gpsMsg module for return to the
*       cloud.
*  
* \brief The GPS measurement will continue until the criteria 
*        for a good location fix is met or the maximum on-time
*        occurs. There are three criteria parameters: Minimum
*        Number of satellites, Maximum HDOP threshold and
*        Minimum on time.
* 
* @return bool Returns true if processing was started. Returns 
*         false if the processing had already been started.
* 
* \ingroup PUBLIC_API
*/
void gps_start(void)
{
    if (gpsData.active)
    {
        return;
    }
    gpsData.active = true;
    gpsData.state = GPS_STATE_POWER_UP;
    gpsData.gpsOnRetryCount = 0;
    gps_stateMachine();
}

/**
* \brief Stop managing the GPS. This will power off the GPS 
*        device and stop any current GPS message processing.
* 
* \ingroup PUBLIC_API
*/
void gps_stop(void)
{
    gpsPower_powerDownGPS();
    gpsMsg_stop();
    gpsData.active = false;
    // Always set UART mux for use with modem after GPS is done
    MODEM_UART_SELECT_ENABLE();
}

/**
* \brief Identify if the gps module is currently busy managing 
*        the GPS.
* 
* @return bool Returns true if busy, false otherwise.
*/
bool gps_isActive(void)
{
    return (gpsData.active);
}

/*************************
 * Module Private Functions
 ************************/

/**
* \brief Statemachine to sequence through the steps of powering 
*        on the GPS and waiting for a valid satellite fix. The
*        gpsPower and gpsMsg API's are called to manage the
*        operations.
*/
static void gps_stateMachine(void)
{

    switch (gpsData.state)
    {
        case GPS_STATE_IDLE:
            break;

        case GPS_STATE_POWER_UP:
#ifdef GPS_DEBUG
            gps_debug_message("[GPS state=PowerupWait]\n");
#endif
            gpsPower_restart();
            gpsData.state = GPS_STATE_POWER_UP_WAIT;
            gpsData.startGpsTimestamp = GET_SYSTEM_TICK();
            break;

        case GPS_STATE_POWER_UP_WAIT:
            if (gpsPower_isGpsOn())
            {
#ifdef GPS_DEBUG
                gps_debug_message("[GPS state=Starting]\n");
#endif
                gpsData.state = GPS_STATE_MSG_RX_START;
            }
            else if (gpsPower_isGpsOnError())
            {
                // Houston - we have a problem
#ifdef GPS_DEBUG
                gps_debug_message("[GPS state=Retry1]\n");
#endif
                gpsData.state = GPS_STATE_RETRY;
            }
            break;

        case GPS_STATE_MSG_RX_START:
            // no debug messages until stop
            gpsMsg_start();
            gpsData.state = GPS_STATE_MSG_RX_WAIT;
            break;

        case GPS_STATE_MSG_RX_WAIT:
            if (gpsMsg_gotGgaMessage())
            {
                // We have a GGA message available.
                if (gpsMsg_gotValidGpsFix())
                {
#ifdef GPS_DEBUG
                    gps_debug_message("[GPS state=Success]\n");
#endif
                    // The GGA message data has met the criteria for a good fix. We are done.
                    gpsData.state = GPS_STATE_DONE;
                }
                else if (GET_ELAPSED_TIME_IN_SEC(gpsData.startGpsTimestamp) > MAX_ALLOWED_GPS_FIX_TIME_IN_SEC)
                {
                    // Timeout waiting for a satellite fix - so we are done.
#ifdef GPS_DEBUG
                    gps_debug_message("[GPS state=Timed Out]\n");
#endif
                    gpsData.state = GPS_STATE_DONE;
                }
                else
                {
                    // Keep trying for a satellite fix
                    gpsData.state = GPS_STATE_MSG_RX_START;
                }
            }
            else if (gpsMsg_isError())
            {
                // Houston - we have a problem
#ifdef GPS_DEBUG
                gps_debug_message("[GPS state=Retry2]\n");
#endif
                gpsData.state = GPS_STATE_RETRY;
            }
            break;

        case GPS_STATE_RETRY:
            gpsPower_powerDownGPS();
            gpsData.gpsOnRetryCount++;
            if (gpsData.gpsOnRetryCount < MAX_GPS_RETRY_ON_ERROR)
            {
#ifdef GPS_DEBUG
                gps_debug_message("[GPS state=Powerup]\n");
#endif
                gpsData.state = GPS_STATE_POWER_UP;
            }
            else
            {
#ifdef GPS_DEBUG
                gps_debug_message("[GPS state=Done3]\n");
#endif
                gpsData.state = GPS_STATE_DONE;
            }
            break;

        default:
        case GPS_STATE_DONE:
            gps_stop();
            gpsData.state = GPS_STATE_IDLE;
            // Always schedule a GPS message to be sent after the
            // GPS measurement is complete.
#ifndef WATER_DEBUG
            msgSched_scheduleGpsLocationMessage();
#else
            sysExecData.mtest_state = MANUF_GPS_DONE;
#endif
            break;
    }
}

/**
* \brief Build the GPS Location message for transmission. The 
*        shared buffer is used to hold the message. The standard
*        msg header is added first followed by the GPS data. See
*        the gpsMsg_getGgaParsedData for a description of how
*        the GPA data is stored in the payload buffer.
* 
* @param payloadPP Pointer to fill in with the address of the 
*                  message to send.
* 
* @return uint16_t Length of the message in bytes.
*/
uint16_t gps_getGpsMessage(uint8_t **payloadPP)
{
    uint8_t *ptr = NULL;
    // Get the shared buffer (we borrow the ota buffer)
    uint8_t *payloadP = modemMgr_getSharedBuffer();
    // Fill in the buffer with the standard message header
    uint8_t payloadSize = storageMgr_prepareMsgHeader(payloadP, MSG_TYPE_GPS_LOCATION);

    // Add the GPS data if available
    ptr = &payloadP[payloadSize];
    // Retrieve the GPS data
    payloadSize += gpsMsg_getGgaParsedData(ptr);
    // Assign pointer
    *payloadPP = payloadP;
    // return payload size
    return (payloadSize);
}

/**
* \brief Copy the data from the last parsed GPS GGA sentence 
*        that was received into the buffer. See the
*        gpsMsg_getGgaParsedData for a description of how the
*        GPA data is stored in the buffer.
* 
* @param bufP Buffer to copy the GPS data into.
* 
* @return uint16_t The number of bytes copied.
*/
uint16_t gps_getGpsData(uint8_t *bufP)
{
    uint8_t length;

    // Retrieve the GPS data
    length = gpsMsg_getGgaParsedData(bufP);
    // return length of data put in the buffer
    return (length);
}

#ifdef WATER_DEBUG
/**
* \brief For Manufacturing testing.  This function records the latest GPS fix 
*        into flash for later retrieval
*/
void gps_record_last_fix(void)
{
    MDRgpsRecord_t gr;
    uint16_t hdop = 0;
    uint8_t i;

    _delay_cycles(1000);
    debug_message((uint8_t *)&gps_report);
    _delay_cycles(1000);

    gr.gpsTime = GET_ELAPSED_TIME_IN_SEC(gpsData.startGpsTimestamp);

    if (gps_report.hdop_score[3] != 'h')
    {
        hdop = gps_report.hdop_score[3] - '0';
        for (i = 4; i < 7; i++)
        {
            if (gps_report.hdop_score[i] == '.')
                continue;
            else if (gps_report.hdop_score[i] == ' ')
                break;
            else
            {
                hdop *= 10;
                hdop += gps_report.hdop_score[i] - '0';
            }
        }
    }
    gr.gpsHdop = hdop;
    gr.gpsQuality = gps_report.fix_quality[3] == '1' ? 1 : 0;
    gr.gpsSatellites = (gps_report.sat_count[3] - '0') * 10 + (gps_report.sat_count[4] - '0');
    memcpy(gr.gpsLatitude, &gps_report.latitude[1], MDR_GPS_LAT_LEN);
    gr.zero = 0;
    memcpy(gr.gpsLongitude, &gps_report.longitude[1], MDR_GPS_LON_LEN);

    WATCHDOG_TICKLE();
	// record the GPS results into the Manufacturing record
    manufRecord_updateManufRecord(MDR_GPS_Record, (uint8_t *)&gr, sizeof(MDRgpsRecord_t));
    _delay_cycles(1000);
}

#endif
