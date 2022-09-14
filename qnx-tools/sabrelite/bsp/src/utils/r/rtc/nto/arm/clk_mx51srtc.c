/*
 * $QNXLicenseC:
 * Copyright 2013, QNX Software Systems.
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

#define	MX51_SRTC_BASE			0x73FA4000
#define	MX51_SRTC_SIZE			0x100

/* Bit Definitions */
#define	MX51_SRTC_LPSCMR		0
#define	MX51_SRTC_LPSCLR		0x04
#define	MX51_SRTC_LPCR			0x10
#define	MX51_SRTC_LPCR_EN_LP	(1<<3)

int
RTCFUNC(init,mx51srtc)(struct chip_loc *chip, char *argv[])
{
    if (chip->phys == NIL_PADDR) {
        chip->phys = MX51_SRTC_BASE;
    }
    if (chip->access_type == NONE) {
        chip->access_type = MEMMAPPED;
    }

    return MX51_SRTC_SIZE;
}

int
RTCFUNC(get, mx51srtc)(struct tm *tm, int cent_reg)
{
    uint32_t counter_sec;

    counter_sec = chip_read32(MX51_SRTC_LPSCMR);

#ifdef  VERBOSE_SUPPORTED
    if (verbose) {
        printf("rtc read: %d\n", counter_sec);
    }
#endif

    gmtime_r(&counter_sec, tm);

    return(0);
}

int
RTCFUNC(set, mx51srtc)(struct tm *tm, int cent_reg)
{
    uint32_t    lp_cr;
    time_t      t;

    t = mktime(tm);

    /*
     *  mktime assumes local time.  We will subtract nd timezmezone
     */
    t -= timezone;

#ifdef  VERBOSE_SUPPORTED
    if (verbose) {
        printf("rtc write: %d\n", t);
    }
#endif

    /* Disable RTC first */
    lp_cr = chip_read32(MX51_SRTC_LPCR) & ~MX51_SRTC_LPCR_EN_LP;
    chip_write32(MX51_SRTC_LPCR, lp_cr);
    while (chip_read32(MX51_SRTC_LPCR) & MX51_SRTC_LPCR_EN_LP);

    /* Write 32-bit time to LPSCMR register, clear LPSCLR register */
    chip_write32(MX51_SRTC_LPSCLR, 0);
    chip_write32(MX51_SRTC_LPSCMR, t);

    /* Enable RTC again */
    chip_write32(MX51_SRTC_LPCR, lp_cr | MX51_SRTC_LPCR_EN_LP);
    while (!(chip_read32(MX51_SRTC_LPCR) & MX51_SRTC_LPCR_EN_LP));

    return(0);
}


__SRCVERSION( "$URL$ $Rev$" );
