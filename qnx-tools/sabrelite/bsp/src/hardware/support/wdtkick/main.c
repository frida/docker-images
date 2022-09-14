/*
 * $QNXLicenseC:
 * Copyright 2009, QNX Software Systems.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resmgr.h>
#include <sys/neutrino.h>
#include <hw/inout.h>
#include <sys/slog.h>
#include <sys/slogcodes.h>
#include <errno.h>
#include <arm/mx35.h>
#include <sys/procmgr.h>
#include <drvr/hwinfo.h>

#define MX35_WDOG_CR_WDE	(1<<2)
#define MX35_WDOG_KEY1		0x5555
#define MX35_WDOG_KEY2		0xaaaa

int main(int argc, char *argv[])
{
	int curr;
	int opt;
	int priority = 10;	/*default priority:default 10 */
	int time = -1;		/*default time for watchdog timer kick*/
	int exit = 0;
	uint32_t base;
	size_t   len = -1;
	uint64_t physbase = 0;
	hwiattr_timer_t wdog_attr;
	unsigned hwi_off;
	int r;

	/* Getting the WDOG Base addresss from the Hwinfo Section if available */
	hwi_off = hwi_find_device("wdog", 0);
	if (hwi_off != HWI_NULL_OFF)
	{
		r = hwiattr_get_timer(hwi_off, &wdog_attr);
		if (r == EOK)
		{
			if(wdog_attr.common.location.len > 0)
				len = wdog_attr.common.location.len;
			if(wdog_attr.common.location.base > 0)
				physbase = wdog_attr.common.location.base;
			time = hwitag_find_clkfreq(hwi_off, NULL);
		}
	}

	/* Process dash options.*/
	while ((opt = getopt(argc, argv, "a:l:p:t:e")) != -1) {
		switch (opt) {
			case 'a':	// WDOG register physics based address
				physbase = strtoull(optarg, NULL, 0);
				break;
			case 'l':
				len = strtoul(optarg, NULL, 0);
				break;
			case 'p':	// priority
				priority = strtoul(optarg, NULL, 0);
				break;
			case 't':
				time = strtoul(optarg, NULL, 0);
				break;
			case 'e':
				exit = 1;
				break;
		}
	}

	/*check if the params are valid*/
	if (physbase == 0)
	{
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,"wdtkick error : Invalid  WDOG register physics based address.Please check the command line or Hwinfo default setting.");
		return EXIT_FAILURE;
	}
	if (len == 0)
	{
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,"wdtkick error : Invalid  WDOG registers size. Please check the command line or Hwinfo default setting.");
		return EXIT_FAILURE;
	}
	if (time == -1)
	{
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,"wdtkick error : Invalid default time for watchdog timer kick. Please check the command line or Hwinfo default setting.");
		return EXIT_FAILURE;
	}

	// Enable IO capability.
	if ( ThreadCtl( _NTO_TCTL_IO, NULL ) == -1 ) {
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,"netio:  ThreadCtl");
		return EXIT_FAILURE;
	}
	//run in the background
	if ( procmgr_daemon( EXIT_SUCCESS, PROCMGR_DAEMON_NOCLOSE | PROCMGR_DAEMON_NODEVNULL ) == -1 ) {
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,"%s:  procmgr_daemon",argv[0]);
		return EXIT_FAILURE;
	}

	// If requested: Change priority.
	curr = getprio (0);
	if (priority != curr && setprio (0, priority) == -1)
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,"WDT:  can't change priority");

	base = mmap_device_io(len, physbase);
	if (base == MAP_DEVICE_FAILED) {
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,"Failed to map WDOG registers");
		return EXIT_FAILURE;
	}

	if (!(in16(base + MX35_WDOG_CR) & MX35_WDOG_CR_WDE))
	{
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,"Watchdog Timer is disable now.");
		if (exit)
		{
			slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,"Terminate Watchdog Timer Module.");
			goto done;
		}
	}
	while (1) {
		out16(base + MX35_WDOG_SR, MX35_WDOG_KEY1);
		out16(base + MX35_WDOG_SR, MX35_WDOG_KEY2);
		delay(time);
	}

done:
	munmap_device_io(base,len);
	return EXIT_SUCCESS;
}


__SRCVERSION("$URL$ $Rev$");
