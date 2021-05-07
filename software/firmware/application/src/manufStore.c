/** 
 * @file manufStore.c
 * \n Source File
 * \n Afridev MSP430 Firmware
 * 
 * \brief Routines to support processing, storing and 
 *        transmitting manufacturing test statistics. 
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

#ifdef WATER_DEBUG
#include "debugUart.h"
#endif
#include "outpour.h"
#include "waterDetect.h"

/***************************
 * Module Data Definitions
 **************************/

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
