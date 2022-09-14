/*
 * $QNXLicenseC: 
 * Copyright 2012 QNX Software Systems.  
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


/*
 * i.MX6x specific timer support.
 *
 * This should be usable by any board that uses an i.MX6 Quad or i.MX6 Dual or i.MX6 Solo
 * Note that if the peripheral clock (IPG PER CLK) is not 66MHz the CLOCK_* definitions
 * below will need to be updated.
 */

#include "startup.h"
#include <arm/mx6x.h>

#define MX6X_CLOCK_FREQ		66000000UL
#define MX6X_CLOCK_RATE		15151515UL
#define MX6X_CLOCK_SCALE		-15

extern struct callout_rtn	timer_load_mx6x;
extern struct callout_rtn	timer_value_mx6x;
extern struct callout_rtn	timer_reload_mx6x;

extern unsigned		mx6x_per_clock;

static uintptr_t	mx6x_epit_base;


static const struct callout_slot	timer_callouts[] = {
	{ CALLOUT_SLOT(timer_load, _mx6x) },
	{ CALLOUT_SLOT(timer_value, _mx6x) },
	{ CALLOUT_SLOT(timer_reload, _mx6x) },
};

static unsigned
timer_start_mx6x()
{
	return in32(mx6x_epit_base + MX6X_EPIT_CNR);
}

static unsigned
timer_diff_mx6x(unsigned start)
{
	unsigned now = in32(mx6x_epit_base + MX6X_EPIT_CNR);

	return (unsigned)((int)start - (int)now);
}

void
mx6x_init_qtime()
{
	struct qtime_entry	*qtime = alloc_qtime();

	/*
	 * Map the timer registers
	 */
	mx6x_epit_base = startup_io_map(MX6X_EPIT_SIZE, MX6X_EPIT1_BASE);

	/*
	 * IPL_CLK, EPIT enabled
	 */
	out32(mx6x_epit_base + MX6X_EPIT_CR, (1 << 24) | (1 << 19) | (1 << 0));

	timer_start = timer_start_mx6x;
	timer_diff = timer_diff_mx6x;

	qtime->intr = MX6X_EPIT1_IRQ;

#define MX6X_CLOCK_FREQ		66000000UL
#define MX6X_CLOCK_RATE		15151515UL

#define MX6X_CLOCK_SCALE	-15
	if (mx6x_per_clock == 0) {
		qtime->timer_rate  = MX6X_CLOCK_RATE;
		qtime->timer_scale = MX6X_CLOCK_SCALE;
		qtime->cycles_per_sec = (uint64_t)MX6X_CLOCK_FREQ;
	} else {
		qtime->timer_rate  = (unsigned long)((uint64_t)1000000000 * (uint64_t)1000000 / (uint64_t)mx6x_per_clock);
		qtime->timer_scale = -15;
		qtime->cycles_per_sec = (uint64_t)mx6x_per_clock;
	}

	add_callout_array(timer_callouts, sizeof(timer_callouts));
}





#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/mx6x_init_qtime.c $ $Rev: 729057 $")
#endif
