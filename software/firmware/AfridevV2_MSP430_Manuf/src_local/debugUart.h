/*
 * debugUart.h
 *
 *  Created on: Nov 17, 2016
 *      Author: robl
 */

#ifndef SRC_LOCAL_DEBUGUART_H_
#define SRC_LOCAL_DEBUGUART_H_

#include "outpour.h"
#include "minmea.h"

#define DEBUG_LINE_SIZE 88
/**
 * \def ISR_BUF_SIZE
 * \brief Define the size of the UART receive buffer.
 */
#define ISR_BUF_SIZE ((uint8_t)200)

#define DBG_DETAILS
#define DBG_SAMPLES
#define DBG_TEMP
#define DBG_POUR

#define GGA_FIELD_WIDTH 10

typedef enum
{
	GGA_TYPE,
	GGA_TIME,
	GGA_LAT_DEG,
	GGA_LAT_DIR,
	GGA_LON_DEG,
	GGA_LON_DIR,
    GGA_FIX_QUAL,
	GGA_SAT_COUNT,
	GGA_HDOP,
	GGA_ALT_DATA,
	GGA_ALT_UNITS,
	GGA_HGT_DATA,
	GGA_HGT_UNITS,
    GGA_CKSUM,
	GGA_NUM_FIELDS
} GGA_FIELD_NAMES;

extern uint8_t dbg_line[DEBUG_LINE_SIZE];
extern Water_Data_t water_report;
extern GPS_Data_t gps_report;

extern void dbg_uart_init(void);
extern void dbg_uart_write(uint8_t *writeCmdP, uint8_t len);
extern uint8_t dbg_uart_txqempty(void);
extern uint8_t dbg_uart_txpend(void);
extern uint8_t dbg_uart_read(void);
extern void debug_message(uint8_t *message);
void debug_padSummary(uint32_t sys_time, uint8_t level, uint8_t unknowns, uint8_t pump_active, uint8_t baseline, uint16_t trickleVol);
void debug_logSummary(uint8_t context, uint32_t sys_time, uint8_t hour, uint16_t  litersForThisHour, uint32_t dayMilliliterSum);
void debug_daySummary(uint8_t context, uint32_t sys_time, uint16_t daysActivated, bool redFlagReady, uint32_t dayMilliliterSum,
		              uint32_t activatedLiterSum, uint16_t dayThreshold, bool newRedFlagCondition);
void debug_chgSummary(uint32_t sys_time);
void debug_sampProgress(void);
void debug_padTargets(void);
void debug_sample_dump(void);
void debug_internalTemp(uint32_t sys_time, int16_t temp);
void debug_RTC_time(timePacket_t *tp, uint8_t marker);

extern void gps_debug_message(uint8_t *message);
extern void gps_debug_minmea_summary(uint8_t *gga, bool valid);
void debug_pour_total(uint32_t sys_time, uint32_t total_pour);
void debugXmit_isr(void);
void debugCmd_isr(void);


#endif /* SRC_LOCAL_DEBUGUART_H_ */
