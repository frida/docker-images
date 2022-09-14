/*
 * $QNXLicenseC:
 * Copyright 2014, QNX Software Systems.
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
#include <string.h>
#include <fcntl.h>
#include <hw/i2c.h>


/*
 * ISL1208 RTC Serial Access Timekeeper
 */
#define ISL1208_SEC             0       // Second: 0x00-0x59
#define ISL1208_MIN             1       // Minute: 0x00-0x59
#define ISL1208_HOUR            2       // Hour:   0x00-0x23 (MIL=1, 24 hours mode) or 0x00-0x11 (MIL=0, 12 hours mode)
#define ISL1208_DATE            3       // Date:   0x01-0x31
#define ISL1208_MONTH           4       // Month:  0x01-0x12
#define ISL1208_YEAR            5       // Year:   0x00-0x99/years
#define ISL1208_WEEK            6       // Week:   0x0-0x6
#define ISL1208_STATUS_REG      7       // Control or status of RTC
#define ISL1208_INTERRUPT_REG   8       // Control or status of RTC

/* STATUS reg bit */
#define ISL1208_STATUS_RTCF     0x01    // REAL TIME CLOCK FAIL BIT
#define ISL1208_STATUS_BAT      0x02    // BATTERY BIT
#define ISL1208_STATUS_ALM      0x04    // ALARM BIT
#define ISL1208_STATUS_WRTC     0x10    // WRITE RTC ENABLE BIT
#define ISL1208_STATUS_XTOSCB   0x40    // CRYSTAL OSCILLATOR ENABLE BIT
#define ISL1208_STATUS_ARST     0x80    // AUTO RESET ENABLE BIT

/* Hour reg bit */
#define ISL1208_HOUR_MIL        0x80    // Hour select mode bit: MIL=1, 24 hours mode, MIL=0, 12 hours mode
#define ISL1208_HOUR_AM_PM      0x20    // AM/PM bit(HR21 bit) in 12 hours mode: 1=PM, 0=AM

#define ISL1208_SEC_MASK        0x7F
#define ISL1208_MIN_MASK        0x7F
#define ISL1208_HOUR_MASK       0x3F
#define ISL1208_DATE_MASK       0x3F
#define ISL1208_MONTH_MASK      0x1F
#define ISL1208_YEAR_MASK       0xFF
#define ISL1208_WEEK_MASK       0x07

#define ISL1208_I2C_ADDRESS     (0x6f)
#define ISL1208_I2C_DEVNAME     "/dev/i2c0"

static int fd = -1;
static int slave = ISL1208_I2C_ADDRESS;
static unsigned char starts_value = 0;

static int
isl1208_i2c_read(unsigned char reg, unsigned char val[], unsigned char num)
{
    iov_t           siov[2], riov[2];
    i2c_sendrecv_t  hdr;

    hdr.slave.addr  = slave;
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
isl1208_i2c_write(unsigned char reg, unsigned char val[], unsigned char num)
{
    iov_t           siov[3];
    i2c_send_t      hdr;

    hdr.slave.addr  = slave;
    hdr.slave.fmt   = I2C_ADDRFMT_7BIT;
    hdr.len         = num + 1;
    hdr.stop        = 1;

    SETIOV(&siov[0], &hdr, sizeof(hdr));
    SETIOV(&siov[1], &reg, sizeof(reg));
    SETIOV(&siov[2], val, num);

    return devctlv(fd, DCMD_I2C_SEND, 3, 0, siov, NULL, NULL);
}

int
RTCFUNC(init,isl1208)(struct chip_loc *chip, char *argv[])
{
#define MAX_NAME  20
    char        i2c_dev[MAX_NAME] = ISL1208_I2C_DEVNAME;
    int         opt;
    char        *value;
    char        *options;
    static char *opts[] = {
                    "i2c",          // I2C device name (default: /dev/i2c0)
                    "slave",        // slave address (default: 0x6f)
                    NULL };

    /*
     * If the I2C device information is different from the default one,
     * specify the i2c device infomation in command line, for example:
     *    rtc isl1208 i2c=/dev/i2c3
     *    rtc isl1208 i2c=/dev/i2c3,slave=0x6f
     */
    if (argv && argv[0]) {
        options = argv[0];
        while (options && *options != '\0') {
            if ((opt = getsubopt( &options, opts, &value)) == -1) {
                break;
            }

            switch (opt) {
                case 0:
                    strncpy(i2c_dev, value, MAX_NAME);
                    break;

                case 1:
                    slave = strtol(value, 0, 0);
                    break;

                default:
                    break;
            }
        }
    }

    fd = open((const char *)i2c_dev, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Unable to open I2C device\n");
        return -1;
    }

    if (EOK != isl1208_i2c_read(ISL1208_STATUS_REG, &starts_value, 1)) {
        fprintf(stderr, "RTC: isl1208_i2c_read() failed\n");
        return (-1);
    }

    if (0 == (starts_value & ISL1208_STATUS_WRTC)) {
        fprintf(stderr, "RTC is disabled. Enabling RTC...\n");
        starts_value |= ISL1208_STATUS_WRTC;
        if (EOK !=  isl1208_i2c_write(ISL1208_STATUS_REG, &starts_value, 1)) {
            fprintf(stderr, "RTC: isl1208_i2c_write() failed\n");
            return (-1);
        }
    }

    return 0;
}

int
RTCFUNC(get,isl1208)(struct tm *tm, int cent_reg)
{
    unsigned char   date[7];

    if (EOK != isl1208_i2c_read(ISL1208_SEC, date, 7)) {
        fprintf(stderr, "RTC: isl1208_i2c_read() failed\n");
        return (-1);
    }

    tm->tm_sec  = BCD2BIN(date[ISL1208_SEC]   & ISL1208_SEC_MASK);
    tm->tm_min  = BCD2BIN(date[ISL1208_MIN]   & ISL1208_MIN_MASK);
    tm->tm_hour = BCD2BIN(date[ISL1208_HOUR]  & ISL1208_HOUR_MASK);     // always use the 24 hours mode
    tm->tm_mday = BCD2BIN(date[ISL1208_DATE]  & ISL1208_DATE_MASK);
    tm->tm_mon  = BCD2BIN(date[ISL1208_MONTH] & ISL1208_MONTH_MASK) - 1;
    tm->tm_year = BCD2BIN(date[ISL1208_YEAR]  & ISL1208_YEAR_MASK) + 2000 - 1900;
    tm->tm_wday = BCD2BIN(date[ISL1208_WEEK]  & ISL1208_WEEK_MASK);

    return(0);
}

int
RTCFUNC(set,isl1208)(struct tm *tm, int cent_reg)
{
    unsigned char   date[7];

    date[ISL1208_SEC]   = BIN2BCD(tm->tm_sec);
    date[ISL1208_MIN]   = BIN2BCD(tm->tm_min);
    date[ISL1208_HOUR]  = BIN2BCD(tm->tm_hour);
    date[ISL1208_HOUR] |= ISL1208_HOUR_MIL;                             // always use the 24 hours mode
    date[ISL1208_DATE]  = BIN2BCD(tm->tm_mday);
    date[ISL1208_MONTH] = BIN2BCD(tm->tm_mon + 1);
    if ((tm->tm_year<100) | (tm->tm_year>199)) {
        fprintf(stderr, "RTC: year must be between 2000 and 2099\n");
        return (-1);
    }
    date[ISL1208_YEAR] = BIN2BCD((tm->tm_year) % 100);
    date[ISL1208_WEEK] = BIN2BCD(tm->tm_wday);

    if (EOK !=  isl1208_i2c_write(ISL1208_SEC, date, 7)) {
        fprintf(stderr, "RTC: isl1208_i2c_write() failed\n");
        return (-1);
    }

    return(0);
}

__SRCVERSION( "$URL$ $Rev$" );
