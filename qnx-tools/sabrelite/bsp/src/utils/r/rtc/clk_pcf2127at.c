/*
 * $QNXLicenseC:
 * Copyright 2010, QNX Software Systems.
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
#include <time.h>
#include <fcntl.h>
#include <hw/i2c.h>

/*
 *	offset in date[] array  
 */ 

#define	PCF2127AT_SEC			0			/* 0x00-0x59 */
#define	PCF2127AT_MIN			1			/* 0x00-0x59 */
#define	PCF2127AT_HOUR			2			/* 0x00-0x23, bit[5] is AM(0)/PM(1) */
#define	PCF2127AT_DATE			3			/* 0x01-0x31 */
#define	PCF2127AT_WEEKDAY		4			/* 0x00-0x6 */
#define	PCF2127AT_MONTH			5			/* 0x01-0x12 */
#define	PCF2127AT_YEAR			6			/* 0x00-0x99 years */	

/* Register offset definition */
#define	PCF2127AT_REG_CTRL1			0
#define PCF2127AT_REG_SEC			0x02			

/* CTRL reg bits */
#define	PCF2127AT_HOUR_12_N24	0x4			/* hour select mode bit. 24hr(0)/12hr(1) */	


/* AM/PM flag in hour reg */
#define	PCF2127AT_HOUR_AM_PM	0x20		/* AM/PM bit */

#define	PCF2127AT_SEC_MASK		0x7F
#define	PCF2127AT_MIN_MASK		0x7F
#define	PCF2127AT_HOUR_MASK		0x3F
#define	PCF2127AT_DATE_MASK		0x3F
#define	PCF2127AT_WEEKDAY_MASK	0x07
#define	PCF2127AT_MONTH_MASK	0x1F
#define	PCF2127AT_YEAR_MASK		0xFF

/* RTC chip address and name */
#define	PCF2127AT_I2C_ADDRESS	0x51
#define	PCF2127AT_I2C_DEVNAME	"dev/i2c0"

static int fd = -1;
static unsigned char ctrl_value = 0;

static int
pcf2127at_i2c_read(unsigned char reg, unsigned char val[], unsigned char num)
{
    iov_t           siov[2], riov[2];
    i2c_sendrecv_t  hdr;

    hdr.slave.addr  = PCF2127AT_I2C_ADDRESS;
    hdr.slave.fmt   = I2C_ADDRFMT_7BIT;
    hdr.send_len    = 1;
    hdr.recv_len    = num;
    hdr.stop        = 1;

    SETIOV(&siov[0], &hdr, sizeof(hdr));
    SETIOV(&siov[1], &reg, sizeof(reg));

    SETIOV(&riov[0], &hdr, sizeof(hdr));
    SETIOV(&riov[1], val, num);

    return devctlv(fd, DCMD_I2C_SENDRECV, 2, 2, siov, riov, NULL);
}

static int
pcf2127at_i2c_write(unsigned char reg, unsigned char val[], unsigned char num)
{
    iov_t           siov[3];
    i2c_send_t      hdr;

    hdr.slave.addr  = PCF2127AT_I2C_ADDRESS;
    hdr.slave.fmt   = I2C_ADDRFMT_7BIT;
    hdr.len         = num + 1;
    hdr.stop        = 1;

    SETIOV(&siov[0], &hdr, sizeof(hdr));
    SETIOV(&siov[1], &reg, sizeof(reg));
    SETIOV(&siov[2], val, num);

    return devctlv(fd, DCMD_I2C_SEND, 3, 0, siov, NULL, NULL);
}

int
RTCFUNC(init,pcf2127at)(struct chip_loc *chip, char *argv[])
{
    fd = open((argv && argv[0] && argv[0][0])? argv[0]: PCF2127AT_I2C_DEVNAME, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Unable to open I2C device\n");
        return -1;
    }
    return 0;
}

int
RTCFUNC(get,pcf2127at)(struct tm *tm, int cent_reg)
{
    unsigned char   date[7];

    if (EOK != pcf2127at_i2c_read(PCF2127AT_REG_SEC, date, 7)) {
        fprintf(stderr, "RTC: pcf2127_i2c_read() failed\n");
        return (-1);
    }

    tm->tm_sec  = BCD2BIN(date[PCF2127AT_SEC]   & PCF2127AT_SEC_MASK);
    tm->tm_min  = BCD2BIN(date[PCF2127AT_MIN]   & PCF2127AT_MIN_MASK);
    tm->tm_hour = BCD2BIN(date[PCF2127AT_HOUR]  & PCF2127AT_HOUR_MASK);

    if ((ctrl_value & PCF2127AT_HOUR_12_N24) &&
        (date[PCF2127AT_HOUR] & PCF2127AT_HOUR_AM_PM))
    {
      tm->tm_hour += 12;
    }

    tm->tm_mday = BCD2BIN(date[PCF2127AT_DATE]  & PCF2127AT_DATE_MASK);
    tm->tm_wday = BCD2BIN(date[PCF2127AT_WEEKDAY] & PCF2127AT_WEEKDAY_MASK);    
    tm->tm_mon  = BCD2BIN(date[PCF2127AT_MONTH] & PCF2127AT_MONTH_MASK) - 1;
    tm->tm_year = BCD2BIN(date[PCF2127AT_YEAR] & PCF2127AT_YEAR_MASK);
    if (tm->tm_year < 70)
    	tm->tm_year += 100;		/* assume valid year 1970 ~ 2069 */ 

    return(0);
}

int

RTCFUNC(set,pcf2127at)(struct tm *tm, int cent_reg)
{
    unsigned char   date[7];

    date[PCF2127AT_SEC]   = BIN2BCD(tm->tm_sec);
    date[PCF2127AT_MIN]   = BIN2BCD(tm->tm_min);

    if ((ctrl_value & PCF2127AT_HOUR_12_N24) &&
        (tm->tm_hour==12))
    {
      date[PCF2127AT_HOUR]  = BIN2BCD(tm->tm_hour);
      date[PCF2127AT_HOUR] |= PCF2127AT_HOUR_AM_PM;
    }
    else if ((ctrl_value & PCF2127AT_HOUR_12_N24) &&
             (tm->tm_hour>12))
    {
      date[PCF2127AT_HOUR]  = BIN2BCD(tm->tm_hour-12);
      date[PCF2127AT_HOUR] |= PCF2127AT_HOUR_AM_PM;
    }
    else
      date[PCF2127AT_HOUR]  = BIN2BCD(tm->tm_hour);
    
    date[PCF2127AT_DATE]  = BIN2BCD(tm->tm_mday);
    date[PCF2127AT_WEEKDAY] = tm->tm_wday & (PCF2127AT_WEEKDAY_MASK);
    date[PCF2127AT_MONTH] = BIN2BCD(tm->tm_mon + 1);
    if (tm->tm_year<100)
    {
      fprintf(stderr, "RTC: year must be >=2000\n");
      return (-1);
    }
    date[PCF2127AT_YEAR] = BIN2BCD((tm->tm_year) % 100);

	if (EOK !=  pcf2127at_i2c_write((PCF2127AT_REG_SEC + 1), date, 7)) {
        fprintf(stderr, "RTC: pcf2127at_i2c_write() failed\n");
        return (-1);
    }

    return(0);
}
































































	