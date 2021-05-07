/** 
 * @file gpsPower.c
 * \n Source File
 * \n Outpour MSP430 Firmware
 * 
 * \brief GPS module responsible for bringing up the GPS and 
 *        shutting it down.
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

// #define GPS_1_8V_ENABLE_PIN BIT3       // Port 1
// #define UART_DEVICE_SELECT_PIN BIT7    // Port 3
// #define TIME_MARK_INPUT_PIN BIT6       // Port 1
// #define GPS_ON_OFF_PIN BIT2            // Port 4
// #define GPS_SYSTEM_ON_PIN BIT7         // Port 1

#if 1
#define GPS_1_8V_ENABLE() (P1OUT |= _1V8_EN)
#define GPS_1_8V_DISABLE() (P1OUT &= ~_1V8_EN)
#define GPS_ON_OFF_HIGH() (P4OUT |= GPS_ON_OFF)
#define GPS_ON_OFF_LOW() (P4OUT &= ~GPS_ON_OFF)
#define GPS_GET_SYSTEM_ON() ((P1IN & GPS_ON_IND)==0)
#else
static int doNothing(void)
{ return 0; }
#define GPS_1_8V_ENABLE() doNothing()
#define GPS_1_8V_DISABLE() doNothing()
#define GPS_ON_OFF_HIGH() doNothing()
#define GPS_ON_OFF_LOW() doNothing()
#define GPS_UART_SELECT_ENABLE() doNothing()
#define MODEM_UART_SELECT_ENABLE() doNothing()
#define GPS_GET_SYSTEM_ON() true
#endif

/**
 * \typedef gpsPowerState_t
 * \brief Define the different states to power on the GPS.
 */
typedef enum {
    GPS_POWERUP_STATE_IDLE,
    GPS_POWERUP_STATE_ALL_OFF,
    GPS_POWERUP_STATE_ENABLE_1_8V,
    GPS_POWERUP_STATE_GPS_ON_OFF_HIGH,
    GPS_POWERUP_STATE_GPS_ON_OFF_LOW,
    GPS_POWERUP_STATE_LOOK_FOR_SYSTEM_ON,
} gpsPowerState_t;

/**
 * \typedef gpsPowerData_t
 * \brief Module data structure.
 */
typedef struct gpsPowerData_s {
    bool active;
    gpsPowerState_t state;
    sys_tick_t startTimestamp;
    uint16_t onTime;
    bool gpsUp;
    bool gpsUpError;
    int retryCount;
} gpsPowerData_t;

/****************************
 * Module Data Declarations
 ***************************/

/**
 * \var gpsPowerData
 * \brief Declare the object that contains the module data.
 */
gpsPowerData_t gpsPowerData;

/********************* 
 * Module Prototypes
 *********************/

static void gpsPower_stateMachine(void);

/***************************
 * Module Public Functions
 **************************/

/**
* \brief Executive that manages sequencing the GPS power up and 
*        power down.  Called on a periodic rate to handle
*        bringing the GPS up and down.
* \ingroup EXEC_ROUTINE
*/
void gpsPower_exec(void)
{

    if (gpsPowerData.active)
    {
        gpsPower_stateMachine();
    }
}

/**
* \brief One time initialization for module.  Call one time 
*        after system starts or (optionally) wakes.
* \ingroup PUBLIC_API
*/
void gpsPower_init(void)
{
    memset(&gpsPowerData, 0, sizeof(gpsPowerData_t));
}

/**
* \brief Start the GPS device power up sequence.  Will reset the
*        state machine that manages the power up steps.
* \ingroup PUBLIC_API
*/
void gpsPower_restart(void)
{
    gpsPowerData.active = true;
    gpsPowerData.gpsUp = false;
    gpsPowerData.gpsUpError = false;
    gpsPowerData.state = GPS_POWERUP_STATE_ALL_OFF;
    gpsPowerData.startTimestamp = GET_SYSTEM_TICK();
    gpsPower_stateMachine();
}

/**
* \brief Remove power from the GPS device. Put the GPS power up 
*        state machine in idle mode.
* 
* \ingroup PUBLIC_API
*/
void gpsPower_powerDownGPS(void)
{
    GPS_1_8V_DISABLE();
    gpsPowerData.active = false;
    gpsPowerData.gpsUp = false;
    gpsPowerData.gpsUpError = false;
    gpsPowerData.state = GPS_POWERUP_STATE_IDLE;
}

/**
* \brief Return the current on/off state of the GPS device.
* 
* @return bool Returns true if on, false otherwise.
* 
* \ingroup PUBLIC_API
*/
bool gpsPower_isGpsOn(void)
{
    return (gpsPowerData.gpsUp);
}

/**
* \brief Return error status.
* 
* @return bool Returns true if no SYSTEM-ON was detected when 
*         powering on the GPS device, false otherwise.
* 
* \ingroup PUBLIC_API
*/
bool gpsPower_isGpsOnError(void)
{
    return (gpsPowerData.gpsUpError);
}

/**
* \brief Return how long the GPS device has been powered in 
*        seconds.
* 
* @return uint16_t Time in seconds that the GPS power has been 
*         enabled.
* 
* \ingroup PUBLIC_API
*/
uint16_t gpsPower_getGpsOnTimeInSecs(void)
{
    return (gpsPowerData.onTime);
}

/*************************
 * Module Private Functions
 ************************/

/**
* \brief State machine to sequence through the GPS device power-on 
*        hardware steps. This is a timebased sequence to
*        control the hardware power on.
*/
static void gpsPower_stateMachine(void)
{

    gpsPowerData.onTime = GET_ELAPSED_TIME_IN_SEC(gpsPowerData.startTimestamp);

    switch (gpsPowerData.state)
    {

        case GPS_POWERUP_STATE_IDLE:
            break;

        case GPS_POWERUP_STATE_ALL_OFF:
            GPS_1_8V_DISABLE();
            gpsPowerData.state = GPS_POWERUP_STATE_ENABLE_1_8V;
            break;

        case GPS_POWERUP_STATE_ENABLE_1_8V:
            if (gpsPowerData.onTime >= (2 * TIME_SCALER))
            {
                GPS_1_8V_ENABLE();
                gpsPowerData.state = GPS_POWERUP_STATE_GPS_ON_OFF_HIGH;
            }
            break;

        case GPS_POWERUP_STATE_GPS_ON_OFF_HIGH:
            if (gpsPowerData.onTime >= (6 * TIME_SCALER))
            {
                GPS_ON_OFF_HIGH();
                gpsPowerData.state = GPS_POWERUP_STATE_GPS_ON_OFF_LOW;
            }
            break;

        case GPS_POWERUP_STATE_GPS_ON_OFF_LOW:
            if (gpsPowerData.onTime >= (8 * TIME_SCALER))
            {
                GPS_ON_OFF_LOW();
                gpsPowerData.state = GPS_POWERUP_STATE_LOOK_FOR_SYSTEM_ON;
            }
            break;

        case GPS_POWERUP_STATE_LOOK_FOR_SYSTEM_ON:
            if (GPS_GET_SYSTEM_ON())
            {
                GPS_UART_SELECT_ENABLE();
                gpsPowerData.state = GPS_POWERUP_STATE_IDLE;
                gpsPowerData.gpsUp = true;
            }
            else if (GET_ELAPSED_TIME_IN_SEC(gpsPowerData.startTimestamp) > (12 * TIME_SCALER))
            {
                gpsPowerData.gpsUpError = true;
            }
            break;

        default:
            gpsPower_powerDownGPS();
            break;
    }
}

