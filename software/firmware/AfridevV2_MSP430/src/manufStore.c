/** 
 * @file manufStore.c
 * \n Source File
 * \n Afridev MSP430 Firmware
 * 
 * \brief Routines to support processing, storing and 
 *        transmitting manufacturing test statistics. 
 */
#ifdef WATER_DEBUG
#include "debugUart.h"
#endif
#include "outpour.h"
#include "waterDetect.h"

/***************************
 * Module Data Definitions
 **************************/


 // this is the definition of the sensor data message payload
 typedef struct
 {
     uint8_t ph[16]; 
     MDRwaterRecord_t wr;
     Pad_Info_t pi;
     uint16_t unknown_limit;
     uint16_t total_flow;
     uint16_t downspout_rate;
     uint16_t water_limit;
     uint16_t water_resets;
     uint16_t trickleVolume;
     uint16_t margin_growth;
 } manufRecord_sensor_T;

/****************************
 * Module Data Declarations
 ***************************/
bool manufRecord_checkForValidManufRecord(void);

/**
 * \def MDR_LOCATION
 * \brief Where in flash the app record is located. It is 
 *        located in the Flash INFO C section.
 */
#define MDR_LOCATION ((uint8_t *)0x1000)  // INFO D

/**
 * \def MDR_MAGIC
 * \brief Used as a known pattern to perform a "quick" verify 
 *        that the data structure is initialized in flash.
 */
#define MDR_MAGIC ((uint16_t)0x2468)

/********************* 
 * Module Prototypes
 *********************/

/**
* \brief Write a fresh manufacturing test record data structure to the 
*        INFO flash location.  The count is set to zero.
*
* @return bool True indicates that a valid Manufacturing data record resides
*              in the INFO buffer
* \ingroup PUBLIC_API
*/
bool manufRecord_initBootloaderRecord(void) {
    manufRecord_t manufRecord;
    uint8_t retryCount = 0;
    bool error = false;

    do
    {
		// init the local RAM structure.
		memset(&manufRecord, 0, sizeof(manufRecord_t));
		manufRecord.magic = MDR_MAGIC;
        manufRecord.recordLength = sizeof(manufRecord_t);

		manufRecord.crc16 = gen_crc16((uint8_t *)&manufRecord, (sizeof(manufRecord_t) - sizeof(uint16_t)));
		msp430Flash_erase_segment(MDR_LOCATION);
		msp430Flash_write_bytes(MDR_LOCATION, (uint8_t *)&manufRecord, sizeof(manufRecord_t));

        // Final check
        if (!manufRecord_checkForValidManufRecord()) {
            error = true;
        } else {
            error = false;
        }

	} while (error && (retryCount++ < 4));

    return !error;
}

/**
* \brief Erase the APP record.  Erases the complete INFO C 
*        section.
* \ingroup PUBLIC_API
*/
void manufRecord_erase(void) {
    msp430Flash_erase_segment(MDR_LOCATION);
}

/**
* \brief Determine if a valid Manufacturing Record is located in 
*        the INFO D section.  The record is written by the
*   	 Application and at startup to validate the startup Air Target
*   	 data.  The recordLength variable is used to identify where
*   	 the CRC is located.  This allows the structure to grow and
*   	 have elements added, and the bootloader will still be able to
*        verify its contents.  But the original elements must remain in
*        the structure at their current defined location (except
*        for the CRC location which always at the end - 2).
* 
* @return bool Returns true if the record is valid.  False 
*         otherwise.
*
* \ingroup PUBLIC_API
*/
bool manufRecord_checkForValidManufRecord(void) {
    bool mrFlag = false;
    manufRecord_t *mrP = (manufRecord_t *)MDR_LOCATION;

    if (mrP->magic == MDR_MAGIC) {
        // Locate the offset to the CRC in the structure based on the
        // stored record length of the appRecord. This is to ensure
        // backward compatibility with future versions.
        uint8_t crcOffset = mrP->recordLength - sizeof(uint16_t);
        // Calculate CRC
        uint16_t calcCrc = gen_crc16(MDR_LOCATION, crcOffset);
        // Access stored CRC based on stored record length, not structure element
        // because structure may be added to in the future.
        uint16_t *storedCrcP = (uint16_t *)(((uint8_t *)mrP) + crcOffset);
        uint16_t storedCrc =  *storedCrcP;
        // Compare calculated CRC to the CRC stored in the structure
        if (calcCrc == storedCrc)
        {
           mrFlag = true;
        }
    }
    return mrFlag;
}

/**
* \brief Update the manuacturing data in the manufacturing record
* 
* @param mr_type The section of Manufacturing Data to update
* @param mr_in Address of data to be written into the buffer
* @param len_in Length of data to be written into the buffer  
* 
* @return bool Returns true if storage of parameters into the 
*         manufacturing record was successful.
*
* \ingroup PUBLIC_API
*/
bool manufRecord_updateManufRecord(MDR_type_e mr_type, uint8_t *mr_in, uint8_t len_in) {
    manufRecord_t manufRecord;
    uint8_t retryCount = 0;
    bool error = false;
    // Retry up to four times to write the App Record.
    do {
        manufRecord_t *mrP = (manufRecord_t *)MDR_LOCATION;

        manufRecord.magic = MDR_MAGIC;
        manufRecord.recordLength = sizeof(manufRecord_t);

        // make a ram copy of current data
        memcpy(&manufRecord,mrP,sizeof(manufRecord_t)-sizeof(uint16_t));

        // copy it all from the manufacturing code
        switch (mr_type)
        {
            case MDR_Water_Record:
                memcpy(&manufRecord.wr,mr_in,len_in);
            	break;
            case MDR_GPS_Record:
                memcpy(&manufRecord.gr,mr_in,len_in);
            	break;
            case MDR_Modem_Record:
                memcpy(&manufRecord.mr,mr_in,len_in);
            	break;
            default:
            	// do nothing
            	break;
        }
		
        // Calculate CRC on RAM version
        manufRecord.crc16 = gen_crc16((const unsigned char *)&manufRecord, (sizeof(manufRecord_t) - sizeof(uint16_t)));
        // Erase Flash Version
        msp430Flash_erase_segment(MDR_LOCATION);
        // Copy RAM version to Flash version
        msp430Flash_write_bytes(MDR_LOCATION, (uint8_t *)&manufRecord, sizeof(manufRecord_t));
        // Final check
        if (!manufRecord_checkForValidManufRecord()) {
            error = true;
        } else {
            error = false;
        }
    } while (error && (retryCount++ < 4));
    return !error;
}

/**
* \brief Read the current Manufacturing data pertaining to water detection.  
*
* @param wr_in Address where water data is to be written
*
* \ingroup PUBLIC_API
*/
void manufRecord_getWaterInfo(MDRwaterRecord_t *wr_in)
{
    manufRecord_t manufRecord;
    manufRecord_t *mrP = (manufRecord_t *)MDR_LOCATION;

    // make a ram copy of current data
    memcpy(&manufRecord,mrP,sizeof(manufRecord_t)-sizeof(uint16_t));

	if (wr_in)
	{
        // copy it all from the manufacturing code
        memcpy(wr_in,&manufRecord.wr,sizeof(MDRwaterRecord_t));
	}
}

/**
* \brief Check to be sure that there is baseline data stored for Water detection.  The values
*        stored are the capacitance values when the pads are covered with AIR.  If the data exists
*        then the water detection targets are set with the stored data.   This is "insurance" in
*        case the unit restarts with a WATCHDOG RESET and the unit is pumping water.
*        If valid data is not stored, then the current capacitance Target data is stored into the flash
*
* @return bool Returns true if there is baseline data stored in flash
*
* \ingroup PUBLIC_API
*/
bool manufRecord_setBaselineAirTargets(void) {
	
	bool answer = false;
    manufRecord_t manufRecord;
    manufRecord_t *mrP = (manufRecord_t *)MDR_LOCATION;
    uint8_t i;
#ifdef WATER_DEBUG
	uint32_t sys_time = getSecondsSinceBoot();
#endif
    
    // make a ram copy of current data
    memcpy(&manufRecord,mrP,sizeof(manufRecord_t)-sizeof(uint16_t));
    
    // if there is valid data there
    if (manufRecord_checkForValidManufRecord())
    {
 	    // it is possible there is no baseline data, eventhough the flash record is correct
 	    for(i=0; i< NUM_PADS; i++)
 	        if ( manufRecord.wr.padBaseline[i] == 0 )
 	    		break;

 	    // if they are all non-zero
 		if (i == NUM_PADS)
 		{
 			if (waterDetect_restore_pads_baseline(&manufRecord.wr))
			{
#ifdef WATER_DEBUG
				debug_message("***AIR Targets Set From Baseline***");
				__delay_cycles(1000);
			    debug_padSummary(sys_time, 0, 0, 0, 1, 0xffff, 0xffff);
				__delay_cycles(1000);
				debug_chgSummary(sys_time);
				__delay_cycles(1000);
#else
				;
#endif
				answer = true;
	        }
 		}
    }

#ifdef WATER_DEBUG
	if (answer != true)
	{
		debug_message("***AIR Targets Set Now***");
		__delay_cycles(1000);
		debug_padSummary(sys_time, 0, 0, 0, 1, 0xffff, 0xffff);
		__delay_cycles(1000);
		waterDetect_record_pads_baseline();
		answer = true;
	}
#endif

 	return (answer);
}

/**
* \brief Check if the GPS test has logged data.  
* 
* @return bool Returns true if storage of parameters into the 
*         manufacturing record was successful.
*
* \ingroup PUBLIC_API
*/
#ifdef WATER_DEBUG
bool mTestGPSDone()
{
    manufRecord_t manufRecord;

    manufRecord_t *mrP = (manufRecord_t *)MDR_LOCATION;

    // make a ram copy of current data
    memcpy(&manufRecord,mrP,sizeof(manufRecord_t)-sizeof(uint16_t));

    if (manufRecord.gr.gpsQuality==1)
	   return(true);
    else
       return(false);
}
#endif


/**
* \brief Check if the Water Detect test has logged data.  
* 
* @return bool Returns true if storage of parameters into the 
*         manufacturing record was successful.
*
* \ingroup PUBLIC_API
*/
#ifdef WATER_DEBUG
bool mTestWaterDone()
{
    manufRecord_t manufRecord;
    int i;
    manufRecord_t *mrP = (manufRecord_t *)MDR_LOCATION;

    // make a ram copy of current data
    memcpy(&manufRecord,mrP,sizeof(manufRecord_t)-sizeof(uint16_t));

    // first sight of a non-zero deviation and we're done
    for(i=0;i<MDR_NUMPADS; i++)
       if (manufRecord.wr.airDeviation[i]!=0)
	      return(true);

    return(false);
}
#endif

/**
* \brief Check if the Water Detect Baseline has logged data.
*
* @return bool Returns true if storage of parameters into the
*         manufacturing record was successful.
*
* \ingroup PUBLIC_API
*/
bool mTestBaselineDone()
{
    manufRecord_t manufRecord;
    int i;
    manufRecord_t *mrP = (manufRecord_t *)MDR_LOCATION;

    // make a ram copy of current data
    memcpy(&manufRecord,mrP,sizeof(manufRecord_t)-sizeof(uint16_t));

    // first sight of a non-zero deviation and we're done
    for(i=0;i<MDR_NUMPADS; i++)
       if (manufRecord.wr.padBaseline[i]!=0)
	      return(true);

    return(false);
}

/**
* \brief Check if the "SEND TEST" message was successfully sent over the Modem. This is the first step
*        of provisioning the Modem.
*
* @return bool Returns true if the SEND_TEST message was not yet sucessfully sent.
*
* \ingroup PUBLIC_API
*/
bool manufRecord_send_test(void)
{
    manufRecord_t manufRecord;
    manufRecord_t *mrP = (manufRecord_t *)MDR_LOCATION;

    // make a ram copy of current data
    memcpy(&manufRecord,mrP,sizeof(manufRecord_t)-sizeof(uint16_t));

    // check if modem "send test" message was sent yet for this board
    if (manufRecord.mr.send_test!=0)
	   return(false);
    else
       return(true);
}

/**
* \brief State machine that coordinates the Manufacturing Test process and other initilization items
*        at startup.  In Normal Operation, this code ensures that AIR Targets are recorded and loaded
*        before water detection begins.
*
* @return bool Returns true if the AIR Target baselines need to be taken and set at a later time
*
* \ingroup PUBLIC_API
*/
bool manufRecord_manuf_test_init()
{
	bool take_baseline = 0;

#ifdef WATER_DEBUG
    // Restart the one-second watchdog timeout
    WATCHDOG_TICKLE();
	debug_message("***Afridev2 V2 Manufacturing Test");
	__delay_cycles(1000);
#endif
    // If the record is not found, write one.
    if (!manufRecord_checkForValidManufRecord())
    	manufRecord_initBootloaderRecord();

    //only take baseline if the original baseline data was erased
	//preferably this is only set in the factory
	if (!mTestBaselineDone())
	{
		take_baseline = 1;
#ifdef WATER_DEBUG
		debug_message("***AIR Target Missing***");
		__delay_cycles(1000);
#endif
	}
	else
	{
		MDRwaterRecord_t wr;

		manufRecord_getWaterInfo(&wr);
#ifdef MANUF_RESTORE_BASELINE_TARGETS
		if (waterDetect_restore_pads_baseline(&wr))
#ifdef WATER_DEBUG
		debug_message("***AIR Target Data Loaded***");
		__delay_cycles(1000);
#else
		;
#endif
#endif
	}
#ifdef WATER_DEBUG
#ifdef NO_GPS_TEST
    if (1) {
#else
	if (mTestGPSDone()) {
#endif
		if (mTestWaterDone())
		{
			if (padStats.tempCelcius < 0)
				debug_message("***Thermistor Failure***");
			else
				debug_message("***Manufacturing Test Pass***\n");
			__delay_cycles(1000);
			sysExecData.mtest_state = MANUF_UNIT_PASS;
		}
		else
		{
			sysExecData.mtest_state = MANUF_TEST_WATER;
			debug_message("***Water Test Begin***");
		}
	}
	else
	{
		// kick off GPS testing
		debug_message("***GPS Test Begin***");
		sysExecData.mtest_state = MANUF_TEST_GPS;
		gps_start();
	}
#endif
	return (take_baseline);
}


/**
* \brief Part two of the State machine that coordinates the Manufacturing Test process 
*
* \ingroup PUBLIC_API
*/
void manufRecord_manuf_test_result()
{
#ifdef WATER_DEBUG
	if (sysExecData.mtest_state == MANUF_WATER_PASS)
	{
#ifdef NO_GPS_TEST
		if (1)   // if both passed, then we are done
#else
	    if (mTestGPSDone())   // if both passed, then we are done
#endif
		{
			if (padStats.tempCelcius < 0)
				debug_message("***Thermistor Failure***");
			else
				debug_message("***Manufacturing Test Pass***");
			sysExecData.mtest_state = MANUF_UNIT_PASS;
		}
		else
		{
			debug_message("***GPS Test Begin***");
			sysExecData.mtest_state = MANUF_TEST_GPS;
			gps_start();
		}
	}
	if (sysExecData.mtest_state == MANUF_GPS_DONE)
	{
		debug_message("***GPS Measurement Done***");
		 // commit result to flash

		gps_record_last_fix();

		if (mTestWaterDone())
		{
			if (padStats.tempCelcius < 0)
			  debug_message("***Thermistor Failure***");
			else
			  debug_message("***Manufacturing Test Pass***");
			sysExecData.mtest_state = MANUF_UNIT_PASS;
		}
		else
		{
			debug_message("***Water Test Begin***");
			sysExecData.mtest_state = MANUF_TEST_WATER;
		}
	}
#else
	;
#endif
}

/**
* \brief a little state machine to coordinate the state of the indicator LEDs.  These LEDs indicate
*        the status and result of Modem Provisioning.   They flash alternately when the test is running.
*        They display Green when the test succeeds and Red if the SEND TEST message fails to be responded
*        to by the Network.   The LEDs will only stay on 5 minutes after test completion (to preserve battery)
*
* \ingroup PUBLIC_API
*/
void manufRecord_update_LEDs()
{
    MDRmodemRecord_t mr;
	
    mr.future_use = 0;
    mr.send_test = sysExecData.send_test_result;

	// when SEND_TEST is a pass or if it is a fail and the FA message and Monthly Checkin
	// was received successfully
	switch( sysExecData.send_test_result )
	{
	    case SYSEXEC_SEND_TEST_RUNNING:
		    // show modem progress by flashing leds
	    	hal_led_toggle();
		    sysExecData.led_on_time = 0;  // this prevents the toggle from being shut off
            break;

	    case SYSEXEC_SEND_TEST_PASS:
	        hal_led_none();
			hal_led_green();
#ifndef SLEEP_DEBUG
			sysExecData.led_on_time = 150;  // 5 minute MAX led on time
#else
			sysExecData.led_on_time = 5;  // 10 second led on time (while sleep is tested)
#endif
		    manufRecord_updateManufRecord(MDR_Modem_Record, (uint8_t*)&mr,sizeof(MDRmodemRecord_t));
			sysExecData.send_test_result = 0;	
	        break;
		
	    case SYSEXEC_SEND_TEST_FAIL:
            hal_led_none();
	        hal_led_red();
#ifndef SLEEP_DEBUG
			sysExecData.led_on_time = 150;  // 5 minute MAX led on time
#else
			sysExecData.led_on_time = 5;  // 10 second led on time (while sleep is tested)
#endif
		    manufRecord_updateManufRecord(MDR_Modem_Record, (uint8_t*)&mr,sizeof(MDRmodemRecord_t));
			sysExecData.send_test_result = 0;
            break;
		
        default:
            ;
    }
	// make sure any indicator LEDs are turned off
	if (sysExecData.led_on_time)
	{
		sysExecData.led_on_time--;
		if (!sysExecData.led_on_time)
			hal_led_none();
	}
}

/**
* \brief Build a "SENSOR DATA" message to be sent to the server.  This message shows telemetry data on 
*        the Water Detection software.  It can be used to remotely diagnose problems with water detection
*
* @param wr_in Address where water data is to be written
*
* @return uint16_t Returns the length of the SENSOR_DATA message
*
* \ingroup PUBLIC_API
*/
uint16_t manufRecord_getSensorDataMessage(uint8_t **payloadPP)
{
    // Get the shared buffer (we borrow the ota buffer)
    manufRecord_sensor_T *ptr = (manufRecord_sensor_T *)modemMgr_getSharedBuffer();

    // Fill in the buffer with the standard message header
    storageMgr_prepareMsgHeader(ptr->ph, MSG_TYPE_SENSOR_DATA);

    // Return sensor data
    manufRecord_getWaterInfo((MDRwaterRecord_t *)&ptr->wr); 
    waterDetect_getPadInfo((Pad_Info_t *)&ptr->pi); 
    ptr->unknown_limit = padStats.unknown_limit;
    ptr->total_flow = sysExecData.total_flow;
    ptr->downspout_rate = sysExecData.downspout_rate;
    ptr->water_limit = padStats.water_limit;
    ptr->water_resets = sysExecData.waterDetectResets;
    ptr->trickleVolume = padStats.trickleVolume;
    sysExecData.total_flow = 0;

    // Assign pointer
    *payloadPP = (uint8_t*)ptr;

    // return payload size
    return (sizeof(manufRecord_sensor_T));
}
