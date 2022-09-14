/*
 * $QNXLicenseC: 
 * Copyright 2009, 2010, QNX Software Systems.  
 *  
 * Licensed under the Apache License, Version 2.0 (the "License"). You  
 * may not reproduce, modify or distribute this software except in  
 * compliance with the License. You may obtain a copy of the License  
 * at: http://www.apache.org/licenses/LICENSE-2.0  
 *  
 * Unless required by applicable law or agreed to in writing, software  
 * distributed under the License is distributed on an "AS IS" basis,  
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied. 
 * 
 * This file may contain contributions from others, either as  
 * contributors under the License or as licensors under other terms.   
 * Please review this entire file for other proprietary rights or license  
 * notices, as well as the QNX Development Suite License Guide at  
 * http://licensing.qnx.com/license-guide/ for other information. 
 * $ 
 */

#include "rtc.h"

/* Macros definations for AT91SAM9XX  */
#define RTC_SIZE		48
#define AT91SAM_TIME		0x08
#define AT91SAM_CAL		0x0c
#define AT91SAM_CR		0x0 
#define AT91SAM_SR		0x18  
#define AT91SAM_SCCR		0x1c       
#define AT91SAM_ACKUPD		(1 << 0) 
#define AT91SAM_UPDTIM		(1 << 0) 
#define AT91SAM_UPDCAL		(1 << 1)  
#define AT91SAM_SEC		(0x7f << 0)
#define AT91SAM_MIN		(0x7f << 8)
#define AT91SAM_HOUR		(0x3f << 16) 
#define AT91SAM_CENT		(0x7f << 0)
#define AT91SAM_YEAR		(0xff << 8)
#define AT91SAM_MONTH		(0x1f << 16)
#define AT91SAM_DAY		(7 << 21)
#define AT91SAM_DATE		(0x3f << 24)

#define DBGU_SIZE		0x50
#define DBGU_BASE		0xFFFFF200 
#define RL64_ID			0x019B03A0
#define M10_ID			0x819b05a1
#define RTC_BASE_AT91SAM9M10	0xFFFFFDB0
#define RTC_BASE_AT91SAM9RL64	0xFFFFFE00

static unsigned baseaddr;

int
RTCFUNC(init,at91rtc) (struct chip_loc *chip, char *argv[]) {
	uint32_t dbgu_base;
	uint32_t cpu_id;
	if (chip->phys == NIL_PADDR) {
		dbgu_base = mmap_device_io (DBGU_SIZE, (uint32_t)(DBGU_BASE));
		cpu_id = in32 (dbgu_base + 0x40);
		switch (cpu_id) {
			case RL64_ID:
				baseaddr = RTC_BASE_AT91SAM9RL64;
				break;
			case M10_ID:
				baseaddr = RTC_BASE_AT91SAM9M10;
				break;
			default:
				baseaddr = RTC_BASE_AT91SAM9M10;
		}
		chip->phys = baseaddr;
	}
	chip->access_type = NONE; 
	return(0);
}

int
RTCFUNC(get,at91rtc)(struct tm *tm, int cent_reg) {
	unsigned	cent;
	unsigned	sec;
	unsigned	min;
	unsigned	hour;
	unsigned	mday;
	unsigned	mon;
	unsigned	year;
	unsigned	week_day;
	unsigned long	date, time;
	uintptr_t	time_ptr, cal_ptr;
	unsigned 	base = baseaddr;
	
	time_ptr = (uintptr_t ) mmap_device_io (4, base + AT91SAM_TIME);
	cal_ptr = (uintptr_t ) mmap_device_io (4, base + AT91SAM_CAL);
	
	/* Get current date and time */
	time = in32 (time_ptr);
	date = in32 (cal_ptr);

	/* convert BCD to binary */
	sec 	 = (time & AT91SAM_SEC) >> 0;
	min 	 = (time & AT91SAM_MIN) >> 8;
	hour	 = (time & AT91SAM_HOUR) >> 16;
	mday	 = (date & AT91SAM_DATE) >> 24;
	mon	 = (date & AT91SAM_MONTH) >> 16; 
	week_day = (date & AT91SAM_DAY) >> 21; 
	year	 = (date & AT91SAM_YEAR) >> 8 ;
	cent	 = (date & AT91SAM_CENT);

	tm->tm_sec 	= BCD2BIN(sec);
	tm->tm_min 	= BCD2BIN(min);
	tm->tm_hour	= BCD2BIN(hour);
	tm->tm_mday	= BCD2BIN(mday);
	tm->tm_wday	= BCD2BIN(week_day) - 1;
	tm->tm_mon	= BCD2BIN(mon) - 1;
	tm->tm_year	= BCD2BIN(year) + (BCD2BIN(cent) - 19) * 100;

	munmap_device_io (time_ptr, 4);
	munmap_device_io (cal_ptr, 4);

	return(0);
}

int
RTCFUNC(set,at91rtc)(struct tm *tm, int cent_reg) {
	unsigned	seconds;
	unsigned	minutes;
	unsigned	hours;
	unsigned	day;
	unsigned	month;
	unsigned	year;
	unsigned	cent;
	unsigned	week_day;
	unsigned long	ctrl_val;
	uintptr_t	rtc_base;
	unsigned 	base = baseaddr;

	/* convert binary to BCD */
	seconds	 = BIN2BCD(tm->tm_sec);
	minutes	 = BIN2BCD(tm->tm_min);
	hours	 = BIN2BCD(tm->tm_hour);
	day 	 = BIN2BCD(tm->tm_mday);
	week_day = BIN2BCD(tm->tm_wday + 1);
	month	 = BIN2BCD(tm->tm_mon + 1);
	year	 = BIN2BCD(tm->tm_year % 100);
	cent	 = BIN2BCD((tm->tm_year / 100) + 19);

	rtc_base = (uintptr_t ) mmap_device_io (RTC_SIZE, base);

	/* Prep for updating*/
	ctrl_val =  in32 (rtc_base + AT91SAM_CR);
	out32 (rtc_base + AT91SAM_CR, ctrl_val|AT91SAM_UPDCAL|AT91SAM_UPDTIM);  /* stop rtc */

	while (!(in32(rtc_base + AT91SAM_SR ) & 0x1)){};	/* poll for ACKUPD */

	out32 (rtc_base + AT91SAM_TIME, (seconds << 0 | minutes << 8 | hours << 16));
	out32 (rtc_base + AT91SAM_CAL, (cent | year << 8 | month << 16 | day << 24 | week_day << 21));

	/* Indicate end of updating */
	ctrl_val = in32 (rtc_base + AT91SAM_CR);
	out32 (rtc_base + AT91SAM_CR, ctrl_val & ~(AT91SAM_UPDCAL | AT91SAM_UPDTIM));	/* RESET after update*/
	out32 (rtc_base + AT91SAM_SCCR, AT91SAM_ACKUPD);

	munmap_device_io (rtc_base, RTC_SIZE);

	return(0);
}

