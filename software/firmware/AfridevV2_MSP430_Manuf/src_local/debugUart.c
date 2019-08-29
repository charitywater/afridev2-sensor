 /**
 * debugUart.c
 *
 *  Created on: Nov 17, 2016
 *      Author: robl
 */



#include "debugUart.h"
#include "outpour.h"
#include "waterDetect.h"
#include "waterSense.h"
#include "minmea.h"

/***************************
 * Module Data Definitions
 **************************/

#define TRUE true
#define FALSE false

/****************************
 * Module Data Declarations
 ***************************/

/**
 * \var isrCommBuf
 * \brief This buffer location is specified in the linker
 *        command file to live right below the stack space in
 *        RAM.
 */
#pragma SET_DATA_SECTION(".commbufs")
uint8_t isrCommBuf[ISR_BUF_SIZE];
uint8_t dbg_line[DEBUG_LINE_SIZE];
#pragma SET_DATA_SECTION()

uint8_t isrCommRecv[3];                                    // the latest byte received
uint8_t isrCommBufHead;
uint8_t isrCommBufTail;
uint8_t first_pad_report = FALSE;
Water_Data_t water_report;
uint8_t parsebuf[GGA_NUM_FIELDS][GGA_FIELD_WIDTH+1];
GPS_Data_t gps_report;

#ifdef GPS_FUNCTIONALITY_ENABLED
extern signed char byteCtr;
extern unsigned char *TI_receive_field;
extern unsigned char *TI_transmit_field;
#endif

/*************************
 * Module Prototypes
 ************************/

static inline void enable_UART_tx(void);
static inline void enable_UART_rx(void);
static inline void disable_UART_tx(void);
static inline void disable_UART_rx(void);

/***************************
 * Module Public Functions
 **************************/

static inline void enable_UART_tx(void)
{
    UC0IE |= UCA0TXIE;
}

static inline void enable_UART_rx(void)
{
    UC0IE |= UCA0RXIE;
}

static inline void disable_UART_tx(void)
{
    UC0IE &= ~UCA0TXIE;
}

static inline void disable_UART_rx(void)
{
    UC0IE &= ~UCA0RXIE;
}
#if 0
static uint8_t add_longval(uint8_t *dest, int32_t meas, uint8_t digits)
{
    uint8_t i=0;
    uint8_t outlen = 0;
    uint8_t bitmove;

    // print the negative sign if applicable
    if (meas < 0) {
    	dest[outlen++]='-';
    	meas = -meas;
    }

    // make hex representation of data
    for (i=0; i < digits; i++)
    {
        bitmove = (((digits - 1) - i) * 4);
        dest[i] = ((0xf << bitmove) & meas) >> bitmove;
        if (dest[i] > 9)
        {
            dest[i] -= 10;
            dest[i] += 'a';
        }
        else
            dest[i] += '0';
    }
    return (outlen+i);
}
#endif

static uint8_t add_wordval(uint8_t *dest, uint16_t meas, uint8_t digits)
{
    uint8_t i;
    uint8_t bitmove;

    // make hex representation of data
    for (i = 0; i < digits; i++)
    {
        bitmove = (((digits - 1) - i) * 4);
        dest[i] = ((0xf << bitmove) & meas) >> bitmove;
        if (dest[i] > 9)
        {
            dest[i] -= 10;
            dest[i] += 'a';
        }
        else
            dest[i] += '0';
    }
    return (i);
}

static int debug_decimal(uint8_t *dest, uint32_t target)
{
    uint8_t i = 0;

    dest[i++] = ((target / 10000) % 10) + '0';
	dest[i++] = ((target / 1000) % 10) + '0';
    dest[i++] = ((target / 100) % 10) + '0';
    dest[i++] = ((target / 10) % 10) + '0';
    dest[i++] = (target % 10) + '0';

	return(i);
}

static int debug_change_out(uint8_t *dest, int16_t change)
{
    uint8_t i = 0;

    if (change<0)
    {
        dest[i++] = '-';
        change = -change;
    }
    dest[i++] = ((change / 1000) % 10) + '0';
    dest[i++] = ((change / 100) % 10) + '0';
    dest[i++] = ((change / 10) % 10) + '0';
    dest[i++] = (change % 10) + '0';
    return (i);
}


static int debug_pad_change(uint8_t *dest, uint8_t pad)
{
    uint8_t i = 0;
    int16_t air_change = waterDetect_getPadChange_Air(pad);
    int16_t water_change = waterDetect_getPadChange_Water(pad);

    dest[i++] = ' ';
    dest[i++] = pad + '0';
    if(air_change != water_change)
    {
		dest[i++] = '(';
		if (air_change != 0)
		{
		   dest[i++] = 'a';
		   i += debug_change_out(&dest[i], air_change);
		}
		if(water_change != 0)
		{
		   dest[i++] = 'w';
		   i += debug_change_out(&dest[i], water_change);
		}
		dest[i++] = ')';
    }
    else
    {
		dest[i++] = '(';
		i += debug_change_out(&dest[i], air_change);
		dest[i++] = ')';
    }
    return (i);
}

static int debug_pad_meas(uint8_t *dest, uint8_t pad, uint8_t baseline)
{
    uint8_t i = 0;
    uint8_t state;
    uint8_t num_samp;
    int16_t change;
    uint16_t mean;

    change = waterDetect_getPadState(pad, &state, &num_samp, &mean);
    dest[i++] = pad + '0';
    dest[i++] = '(';                                       //2
    dest[i++] = state;                                     //3
    dest[i++] = ')';                                       //4
    if (!baseline)
       i += debug_change_out(&dest[i], change);
    else
       i += add_wordval(&dest[i], mean, 4);                   //5-8
    if (num_samp != SAMPLE_COUNT)
        dest[i++] = '*';                                   //9
    else
        dest[i++] = ' ';                                   //9

    return (i);
}


static int debug_level(uint8_t *dest, uint8_t level)
{
    uint8_t i = 0;

    dest[i++] = 'L';
    dest[i++] = level + '0';


    return (i);
}

static int debug_flow_out(uint8_t *dest, uint8_t level, uint8_t unknowns, uint16_t trickleVol)
{
    uint8_t i = 0;
    uint8_t percentile;

    uint16_t flow_rate = waterDetect_get_flow_rate(level,&percentile);

    dest[i++] = 'F';
    dest[i++] = ((flow_rate / 1000) % 10) + '0';
    dest[i++] = ((flow_rate / 100) % 10) + '0';
    dest[i++] = ((flow_rate / 10) % 10) + '0';
    dest[i++] = (flow_rate % 10) + '0';
    dest[i++] = unknowns ? 'u' : ' ';
    dest[i++] = 'P';
    dest[i++] = ((percentile / 100) % 10) + '0';
    dest[i++] = ((percentile / 10) % 10) + '0';
    dest[i++] = (percentile % 10) + '0';
    dest[i++] = 'T';
    if (trickleVol != 0xFFFF)
    {
        dest[i++] = ((trickleVol / 100) % 10) + '0';
        dest[i++] = ((trickleVol / 10) % 10) + '0';
        dest[i++] = (trickleVol % 10) + '0';
    }
    else
    {
        dest[i++] = '?';
        dest[i++] = '?';
        dest[i++] = '?';
    }
    dest[i++] = unknowns ? 'u' : ' ';
    dest[i++] = '\n';
    return (i);
}

static int debug_target_out(uint8_t *dest, uint8_t pad)
{
    uint8_t i = 0;
    uint16_t target = waterDetect_getPadTargetWidth(pad);

    dest[i++] = pad + '0';
    dest[i++] = ':';
    i+= debug_decimal(&dest[i], target);
    dest[i++] = ' ';

    return (i);
}

static int debug_time(uint8_t *dest, uint32_t sys_time)
{
    uint8_t i = 0;

    dest[i++] = 'T';
    i += add_wordval(&dest[i], sys_time & 0xffff, 4);
    dest[i++] = ' ';

    return (i);
}

static int debug_temp_out(uint8_t *dest, int16_t temp)
{
    uint8_t i = 0;

    dest[i++] = 't';
    dest[i++] = '=';
    if (temp<0)
    {
        dest[i++] = '-';
        temp = -temp;
    }
    dest[i++] = ((temp / 100) % 10) + '0';
    dest[i++] = ((temp / 10) % 10) + '0';
    dest[i++] = '.';
    dest[i++] = (temp % 10) + '0';
    dest[i++] = 'C';
    return (i);
}




static int debug_pour_out(uint8_t *dest, uint32_t pour)
{
    uint8_t i = 0;

    dest[i++] = 'p';
    dest[i++] = '=';
    i += debug_decimal(&dest[i], pour);
    dest[i++] = 'm';
    dest[i++] = 'l';
    dest[i++] = '\n';
    return (i);
}

static int debug_sample_out(uint8_t *dest, uint8_t pad_number)
{
    uint8_t i = 0;
    uint16_t sample;

    sample = waterDetect_getCurrSample(pad_number);
    i += debug_decimal(&dest[i], sample);
    dest[i++] = ',';
    dest[i++] = pad_number + '0';
    dest[i++] = ',';

    return (i);
}



//called by waterSense.c
void debug_padSummary(uint32_t sys_time, uint8_t level, uint8_t unknowns, uint8_t pump_active, uint8_t baseline, uint16_t trickleVol)
{
    uint8_t dbg_len = 0;

    WATCHDOG_TICKLE();

	if (!gps_isActive())
    {
		memset(&water_report,' ',sizeof(Water_Data_t));
	    MODEM_UART_SELECT_ENABLE();
	    // only display when one or more pads is covered with water
#ifdef DISPLAY_ALL_PADDATA
	    if (1)
#else
	    if (level>0 || baseline || unknowns > 0 || pump_active > 0)
#endif
	    {
			debug_time(water_report.time, sys_time);
			debug_temp_out(water_report.tempc, waterSense_getTempCelcius());
			debug_pad_meas(water_report.pad0,0,baseline);
			debug_pad_meas(water_report.pad1,1,baseline);
			debug_pad_meas(water_report.pad2,2,baseline);
			debug_pad_meas(water_report.pad3,3,baseline);
			debug_pad_meas(water_report.pad4,4,baseline);
			debug_pad_meas(water_report.pad5,5,baseline);
			debug_level(water_report.level, level);
			debug_flow_out(water_report.flow, level, unknowns, trickleVol );
			water_report.zero = 0;
		    memcpy(&dbg_line[0],&water_report,sizeof(Water_Data_t));
		    dbg_len += sizeof(Water_Data_t);
		    dbg_line[dbg_len++]='\n';
		    dbg_uart_write(dbg_line, dbg_len);
		}
	}
}

void debug_chgSummary(uint32_t sys_time)
{
    uint8_t pad_number;
    uint8_t dbg_len = 0;
    uint8_t change_count = 0;
	
    WATCHDOG_TICKLE();

	if (!gps_isActive())
	{
	    MODEM_UART_SELECT_ENABLE();
	    for (pad_number = 0; pad_number < NUM_PADS; pad_number++)
	        if ( waterDetect_getPadChange(pad_number))
	           change_count++;

	    if (change_count)
	    {
			dbg_len = debug_time(&dbg_line[dbg_len], sys_time);
			dbg_len += debug_temp_out(&dbg_line[dbg_len], waterSenseGetTemp());

			for (pad_number = 0; pad_number < NUM_PADS; pad_number++)
			{
	           dbg_len += debug_pad_change(&dbg_line[dbg_len], pad_number);
			}

			dbg_line[dbg_len++]='\n';
			dbg_uart_write(dbg_line, dbg_len);

	    }
	}
}
void debug_daySummary(uint8_t context, uint32_t sys_time, uint16_t daysActivated, bool redFlagReady, uint32_t dayMilliliterSum,
		              uint32_t activatedLiterSum, uint16_t dayThreshold, bool newRedFlagCondition)
{
    uint8_t dbg_len = 0;

    WATCHDOG_TICKLE();

	if (!gps_isActive())
	{
	    MODEM_UART_SELECT_ENABLE();
		dbg_line[dbg_len++]='&';
		dbg_line[dbg_len++]=context;
		dbg_len += debug_time(&dbg_line[dbg_len], sys_time);
		dbg_line[dbg_len++]=',';
		dbg_line[dbg_len++]='a';
		dbg_line[dbg_len++]='=';
		dbg_line[dbg_len++] = ((daysActivated / 10) % 10) + '0';
		dbg_line[dbg_len++] = (daysActivated % 10) + '0';
		dbg_line[dbg_len++]=',';
		dbg_line[dbg_len++]='r';
		dbg_line[dbg_len++]='f';
		dbg_line[dbg_len++]='r';
		dbg_line[dbg_len++]='=';
		dbg_line[dbg_len++]=redFlagReady?'Y':'N';
		dbg_line[dbg_len++]=',';
		dbg_line[dbg_len++]='d';
		dbg_line[dbg_len++]='=';
		dbg_len += debug_decimal(&dbg_line[dbg_len], dayMilliliterSum);
		dbg_line[dbg_len++]=',';
		dbg_line[dbg_len++]='a';
		dbg_line[dbg_len++]='=';
		dbg_len += debug_decimal(&dbg_line[dbg_len], activatedLiterSum);
		dbg_line[dbg_len++]=',';
		dbg_line[dbg_len++]='d';
		dbg_line[dbg_len++]='t';
		dbg_line[dbg_len++]='=';
		dbg_len += debug_decimal(&dbg_line[dbg_len], dayThreshold);
		dbg_line[dbg_len++]=',';
		dbg_line[dbg_len++]='r';
		dbg_line[dbg_len++]='f';
		dbg_line[dbg_len++]='=';
		dbg_line[dbg_len++]=redFlagReady?'Y':'N';
		dbg_line[dbg_len++]='\n';
		dbg_uart_write(dbg_line, dbg_len);
	}
}



void debug_logSummary(uint8_t context, uint32_t sys_time, uint8_t hour, uint16_t  litersForThisHour, uint32_t dayMilliliterSum)
{
    uint8_t dbg_len = 0;

    WATCHDOG_TICKLE();

	if (!gps_isActive())
	{
	    MODEM_UART_SELECT_ENABLE();
		dbg_line[dbg_len++]='&';
		dbg_line[dbg_len++]=context;
		dbg_len += debug_time(&dbg_line[dbg_len], sys_time);
		dbg_line[dbg_len++]=',';
		dbg_line[dbg_len++]='h';
		dbg_line[dbg_len++]='=';
		dbg_line[dbg_len++] = ((hour / 10) % 10) + '0';
		dbg_line[dbg_len++] = (hour % 10) + '0';
		dbg_line[dbg_len++]=',';
		dbg_line[dbg_len++]='l';
		dbg_line[dbg_len++]='=';
		dbg_len += debug_decimal(&dbg_line[dbg_len], litersForThisHour);
		dbg_line[dbg_len++]=',';
		dbg_line[dbg_len++]='d';
		dbg_line[dbg_len++]='=';
		dbg_len += debug_decimal(&dbg_line[dbg_len], dayMilliliterSum);
		dbg_line[dbg_len++]='\n';
		dbg_uart_write(dbg_line, dbg_len);
	}

}


void debug_padTargets(void)  {
    uint8_t pad_number;
    uint8_t dbg_len = 0;

    WATCHDOG_TICKLE();

    if (!gps_isActive())
    {
        MODEM_UART_SELECT_ENABLE();
		dbg_line[dbg_len++]='<';
		dbg_line[dbg_len++]='.';
		dbg_line[dbg_len++]='>';
	    for (pad_number = 0; pad_number < NUM_PADS; pad_number++)
	    	dbg_len += debug_target_out(&dbg_line[dbg_len], pad_number);
		dbg_line[dbg_len++]='\n';
	    dbg_uart_write(dbg_line, dbg_len);
	}
}


//called by waterSense.c
void debug_internalTemp(uint32_t sys_time, int16_t temp)
{
    uint8_t dbg_len = 0;

    dbg_len = debug_time(&dbg_line[dbg_len], sys_time);
    dbg_len += debug_temp_out(&dbg_line[dbg_len], temp);

    dbg_uart_write(dbg_line, dbg_len);
}
//called by waterSense.c
void debug_pour_total(uint32_t sys_time, uint32_t total_pour)
{
    uint8_t dbg_len = 0;
    if (!gps_isActive())
	{
        MODEM_UART_SELECT_ENABLE();
	    dbg_len = debug_time(&dbg_line[dbg_len], sys_time);
	    dbg_len += debug_pour_out(&dbg_line[dbg_len], total_pour);

	    dbg_uart_write(dbg_line, dbg_len);
	}
}

void debug_message(uint8_t *message)
{
    uint8_t dbg_len = 0;
    uint8_t i;

    WATCHDOG_TICKLE();

    if (!gps_isActive())
    {	
        MODEM_UART_SELECT_ENABLE();
	    for (i = 0; message[i] && i < DEBUG_LINE_SIZE-2; i++)
	       dbg_line[dbg_len++] = message[i];
	    dbg_line[dbg_len++] = '\n';
	    dbg_uart_write(dbg_line, dbg_len);
	}
    // wait for message to transmit
    while(!dbg_uart_txqempty());  // send the rest of the debug data
}


void gps_debug_message(uint8_t *message)
{
    uint8_t dbg_len = 0;
    uint8_t i;
	
    WATCHDOG_TICKLE();

    MODEM_UART_SELECT_ENABLE();	
	
    for (i = 0; message[i] && i < DEBUG_LINE_SIZE-2; i++)
       dbg_line[dbg_len++] = message[i];
    dbg_line[dbg_len++] = '\n';
    dbg_uart_write(dbg_line, dbg_len);
	
    // wait for message to transmit
    while(!dbg_uart_txqempty());  // send the rest of the debug data
    GPS_UART_SELECT_ENABLE();
}


#ifdef DBG_SAMPLES
//called by sysExec.c
void debug_sample_dump(void)
{
    uint8_t pad_number;
    uint8_t dbg_len = 0;
    if (!gps_isActive())
    {
        MODEM_UART_SELECT_ENABLE();
	    for (pad_number = 0; pad_number < NUM_PADS; pad_number++) dbg_len += debug_sample_out(&dbg_line[dbg_len], pad_number);
	    dbg_line[dbg_len++] = '\n';
	    dbg_uart_write(dbg_line, dbg_len);
	}
}
#endif

// stub isr for modem
void modemCmd_isr(void)
{
}




void gps_parse_gga(uint8_t *gga)
{
	uint8_t curr_field = 0;
	uint8_t field_len = 0;
	uint8_t cursor = 0;

	// clear out the buffer
	memset(parsebuf,0,sizeof(parsebuf));

	// there are 14 fields in gga sentence
    while (gga[cursor] && curr_field < GGA_NUM_FIELDS)
    {
    	if(gga[cursor]==',')
    	{
    		curr_field++;
            cursor++;
            field_len = 0;
    	}
    	else
    	{
    		if (field_len < GGA_FIELD_WIDTH)
    		   parsebuf[curr_field][field_len++] = gga[cursor++];
    		else
    		{
    		   // flush out remaining field data, not stored
    		   while( gga[cursor] && gga[cursor] !=',')
    			   cursor++;

    		   // close out flushing data
        	   if(gga[cursor]==',')
        	   {
        		   curr_field++;
                   cursor++;
                   field_len = 0;
        	   }
        	}
    	}
    }
}

// fld desc
// 00  GGA_TYPE,
// 01  GGA_TIME,
// 02  GGA_LAT_DEG,
// 03  GGA_LAT_DIR,
// 04  GGA_LON_DEG,
// 05  GGA_LON_DIR,
// 06  GGA_FIX_QUAL,
// 07  GGA_SAT_COUNT,
// 08  GGA_HDOP,
// 09  GGA_ALT_DATA,
// 10  GGA_ALT_UNITS,
// 11  GGA_HGT_DATA,
// 12  GGA_HGT_UNITS,
// 13  GGA_CKSUM

const uint8_t fieldnam[16]={'0','t','X','3','Y','5','q','s','h','9','a','b','c','d','e','f'};
const uint8_t fieldsel[16]={ 0,  1,  1,  0,  1,  0,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0 };
//const uint8_t testgps[]="$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";

int gps_debug_field(uint8_t *dest, GGA_FIELD_NAMES field_num, uint16_t cursor, uint16_t len)
{
	uint16_t i;

	for(i=0; parsebuf[field_num][i+cursor] && i+cursor < GGA_FIELD_WIDTH && i< len; i++)
		dest[i]=parsebuf[field_num][i+cursor];

	return(i);
}


void gps_debug_minmea_summary(uint8_t *gga, bool valid)
{
    uint8_t dbg_len = 0;
    GGA_FIELD_NAMES field_num;
    uint8_t i,loc;

    WATCHDOG_TICKLE();

    // clear the gps_report
    memset(&gps_report,' ',sizeof(gps_report));

    MODEM_UART_SELECT_ENABLE();
    gps_parse_gga(gga);

    for(field_num=GGA_TYPE; field_num < GGA_NUM_FIELDS; field_num++)
    {
    	if (fieldsel[field_num])
    	{
    		switch(field_num)
    		{
    		case GGA_TIME:
    			i = 0;
    			gps_report.time[i++]='@';
    			if(parsebuf[GGA_TIME][0])
    			{
				    i += gps_debug_field(&gps_report.time[i],GGA_TIME, 0, 2);
				    gps_report.time[i++]=':';
					i += gps_debug_field(&gps_report.time[i],GGA_TIME, 2, 2);
					gps_report.time[i++]=':';
					i += gps_debug_field(&gps_report.time[i],GGA_TIME, 4, 2);
    			}
    			else
    			{
    				gps_report.time[i++]='T';
    				gps_report.time[i++]='i';
    				gps_report.time[i++]='m';
    				gps_report.time[i++]='e';
    			}
    			break;
    		case GGA_LAT_DEG:
    			i = 0;
				gps_report.latitude[i++]='[';
    			for(loc=0; loc<GGA_FIELD_WIDTH; loc++)
    				if (parsebuf[GGA_LAT_DEG][loc]=='.')
    					break;
    			if(loc!=GGA_FIELD_WIDTH)
    			{

					i += gps_debug_field(&gps_report.latitude[i],GGA_LAT_DIR, 0, 1);
					i += gps_debug_field(&gps_report.latitude[i],GGA_LAT_DEG, 0, loc-2);
					gps_report.latitude[i++]=' ';
					i += gps_debug_field(&gps_report.latitude[i],GGA_LAT_DEG, loc-2, 7);

    			}
    			else
    			{
					gps_report.latitude[i++]='L';
					gps_report.latitude[i++]='a';
					gps_report.latitude[i++]='t';
    			}
    			break;
    		case GGA_LON_DEG:
    			i = 0;
				gps_report.longitude[i++]=',';
    			for(loc=0; loc<GGA_FIELD_WIDTH; loc++)
    				if (parsebuf[GGA_LON_DEG][loc]=='.')
    					break;
    			if(loc!=GGA_FIELD_WIDTH)
    			{
					i += gps_debug_field(&gps_report.longitude[i],GGA_LON_DIR, 0, 1);
					i += gps_debug_field(&gps_report.longitude[i],GGA_LON_DEG, 0, loc-2);
					gps_report.longitude[i++]=' ';
					i += gps_debug_field(&gps_report.longitude[i],GGA_LON_DEG, loc-2, 7);
    			}
    			else
    			{
					gps_report.longitude[i++]='L';
					gps_report.longitude[i++]='o';
					gps_report.longitude[i++]='n';
    			}
				gps_report.longitude[i++]=']';
    			break;
    		case GGA_FIX_QUAL:
    			i=0;
				gps_report.fix_quality[i++]=',';
        		gps_report.fix_quality[i++]='q';
        		gps_report.fix_quality[i++]='=';
			    i += gps_debug_field(&gps_report.fix_quality[i],GGA_FIX_QUAL, 0, 1);
                break;
    		case GGA_SAT_COUNT:
    			i=0;
				gps_report.sat_count[i++]=',';
        		gps_report.sat_count[i++]='s';
        		gps_report.sat_count[i++]='=';
			    i += gps_debug_field(&gps_report.sat_count[i],GGA_SAT_COUNT, 0, 2);
                break;
    		case GGA_HDOP:
    			i=0;
				gps_report.hdop_score[i++]=',';
				gps_report.hdop_score[i++]='h';
				gps_report.hdop_score[i++]='=';
    			if(parsebuf[GGA_HDOP][0])
    			{
					i += gps_debug_field(&gps_report.hdop_score[i],GGA_HDOP, 0, GGA_FIELD_WIDTH);
    			}
    			else
    			{
					gps_report.hdop_score[i++]='h';
					gps_report.hdop_score[i++]='d';
					gps_report.hdop_score[i++]='o';
					gps_report.hdop_score[i++]='p';
    			}
    			break;
    		default:
                ;// do nothing;
    		}
    	}
    }
    i=0;
    gps_report.fix_is_valid[i++]=',';
    gps_report.fix_is_valid[i++]='v';
    gps_report.fix_is_valid[i++]='=';
    gps_report.fix_is_valid[i++]= valid?'y':'n';
    i=0;
    gps_report.time_to_fix[i++]=',';
    gps_report.time_to_fix[i++]='e';
    gps_report.time_to_fix[i++]='=';
    debug_decimal(&gps_report.time_to_fix[i], GET_ELAPSED_TIME_IN_SEC(gpsData.startGpsTimestamp));

    gps_report.zero = 0;
    memcpy(&dbg_line[0],&gps_report,sizeof(gps_report));
    dbg_len += sizeof(gps_report);
    dbg_line[dbg_len++]='\n';
    dbg_uart_write(dbg_line, dbg_len);

    // wait for message to transmit
    while(!dbg_uart_txqempty());  // send the rest of the debug data
    GPS_UART_SELECT_ENABLE();
}


/**
* \brief One time initialization for module.  Call one time
*        after system starts or (optionally) wakes.
* \ingroup PUBLIC_API
*/
void dbg_uart_init(void)
{
    uint8_t __attribute__((unused)) garbage;

    memset(isrCommBuf, 0, ISR_BUF_SIZE);
    isrCommBufHead = 0;
    isrCommBufTail = ISR_BUF_SIZE - 1;
    memset(isrCommRecv, 0xff, 3);

    // clear out receive buffer
    disable_UART_rx();
    garbage = UCA0RXBUF;
    enable_UART_rx();

    first_pad_report = FALSE;
}

void dbg_uart_write(uint8_t *writeCmdP, uint8_t len)
{
    uint8_t i;

    disable_UART_tx();
    disable_UART_rx();

    for (i = 0; i < len; i++)
    {
        // if the queue isn't full
        if (isrCommBufHead != isrCommBufTail)
        {
            isrCommBuf[isrCommBufHead] = writeCmdP[i];
            if (isrCommBufHead + 1 < ISR_BUF_SIZE)
                isrCommBufHead++;
            else
                isrCommBufHead = 0;
        }
        else
            break;                                         // we sent all we could
    }

    enable_UART_tx();
    enable_UART_rx();
}

uint8_t dbg_uart_read(void)
{
    uint8_t answer;

    disable_UART_rx();
    answer = isrCommRecv[0];
    enable_UART_rx();

    return (answer);
}

void debug_sampProgress(void)
{
	dbg_line[0]='@';
    dbg_uart_write(dbg_line, 1);
}

uint8_t dbg_uart_txqempty(void)
{
    uint8_t next, answer;

    if (isrCommBufTail + 1 >= ISR_BUF_SIZE)
        next = 0;
    else
        next = isrCommBufTail + 1;

	if (next == isrCommBufHead)
		answer = TRUE;
	else
		answer = FALSE;

    return (answer);
}

uint8_t dbg_uart_txpend(void)
{
	return(UC0IFG & UCA0TXIFG);
}

/*****************************
 * UART Interrupt Functions
 ****************************/

/**
* \brief Uart Transmit Interrupt Service Routine.
* \ingroup ISR
*/
extern uint8_t CAPSENSE_ACTIVE;
#ifndef FOR_USE_WITH_BOOTLOADER
#pragma vector=USCIAB0TX_VECTOR
#endif
__interrupt void USCI0TX_ISR(void)
{

    //UART Tx
    if ((IFG2 & UCA0TXIFG) && (IE2 & UCA0TXIE)) 
    {
        uint8_t next;

        if (isrCommBufTail + 1 >= ISR_BUF_SIZE)
            next = 0;
        else
            next = isrCommBufTail + 1;

        // do we have data to send?
        if (next != isrCommBufHead)
        {
            isrCommBufTail = next;

            UCA0TXBUF = isrCommBuf[next];
        }
        else
            disable_UART_tx();
    }

        // Catch to go back into LPM in case we try to process UART/I2C while taking capsense data
    if (CAPSENSE_ACTIVE){
        __bis_SR_register_on_exit(LPM3_bits + GIE); // continue waiting for the WDT interrupt
    }

}

#if 0
// there is no need for a debug receive interrupt, it interferes with modem and gps
/**
* \brief Uart Receive Interrupt Service Routine.
* \ingroup ISR
*/
#ifndef FOR_USE_WITH_BOOTLOADER
#pragma vector=USCIAB0RX_VECTOR
#endif
__interrupt void USCI0RX_ISR(void)
{
	// uart Rx
	if ((IFG2 & UCA0RXIFG)&&(IE2 & UCA0RXIE)){
    // only save the last byte received
    isrCommRecv[0] = UCA0RXBUF;
	}

    if (CAPSENSE_ACTIVE){
    	__bis_SR_register_on_exit(LPM3_bits + GIE); // continue waiting for the WDT interrupt
    }
}
#endif


