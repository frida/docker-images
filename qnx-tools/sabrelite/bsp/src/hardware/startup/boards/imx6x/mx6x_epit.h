/*
 * $QNXLicenseC: 
 * Copyright 2012, QNX Software Systems.  
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

#ifndef _EPIT_H
#define _EPIT_H

#include <hw/inout.h>
#include <arm/mx6x.h>

/*
 * Note that the timer functions do not accomodate for timer rollovers, therefore
 * the functions below should not be used to time durations longer than 65 seconds.
 */
 
/*
 * Enable the timer if it is not already enabled
 * Note that the timer code does not handle rollovers as of now.
 */
static inline void mx6_epit_timer_init() {
	out32(MX6X_EPIT2_BASE,0x1000001);
}

// Read timer value
static inline unsigned int mx6_epit_get_timer_val() {
	out32(MX6X_EPIT2_BASE,0x1000001);
	return in32(MX6X_EPIT2_BASE + MX6X_EPIT_CNR);
}

// Return the time in micro seconds between readings
static inline unsigned int mx6_epit_get_delta(unsigned int t_first, unsigned int t_second) {
	return ((t_first - t_second)/66); 
}

/*
 * Print the time in micro seconds between readings
 * The EPIT clock is 66MHz, therefore we return the time in micro seconds by dividing
 * the number of clocks by 66.
 */
static inline void mx6_epit_print_delta(unsigned int t_first, unsigned int t_second) {
	kprintf("\n");
	kprintf("Startup: time between timer readings (decimal): %d useconds\n", (t_first-t_second)/66);
}

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/mx6x_epit.h $ $Rev: 729057 $")
#endif
