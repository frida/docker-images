/*
 * $QNXLicenseC: 
 * Copyright 2011,2012  QNX Software Systems.  
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

#include "startup.h"
#include <hw/inout.h>
#include <arm/mx6x.h>
#include "board.h"

// The watchdog timeout value should be specified in board.h.  Set default value to 30 seconds.
#if !defined(WDOG_TIMEOUT)
#define WDOG_TIMEOUT 30
#endif

#define WDOG_SECONDS_TO_TIMEOUT_BITS	(((WDOG_TIMEOUT * 2) - 1) << 8)

/* Enable watch dog */
void mx6x_wdg_enable(void)
{
	out16(MX6X_WDOG1_BASE + MX6X_WDOG_CR, in16(MX6X_WDOG1_BASE + MX6X_WDOG_CR) | WDE);
}

/* Re-load the value of watch-dog timer */
void mx6x_wdg_reload(void)
{
	uint16_t control_val;
	control_val = in16(MX6X_WDOG1_BASE + MX6X_WDOG_CR);

	// set timeout value	
	control_val &= ~WT_MASK;
	control_val |= WDOG_SECONDS_TO_TIMEOUT_BITS; 

	// only assert wdog_rst upon time-out event
	control_val &= ~WDT;

	// suspend (disable) the watchdog timer in low-power modes (STOP and DOZE mode) 
	control_val |= WDZST;

	// make sure watchdog is not enabled yet
	control_val &= ~WDE;

	out16(MX6X_WDOG1_BASE + MX6X_WDOG_CR, control_val);
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/mx6x_init_wdg.c $ $Rev: 729057 $")
#endif
