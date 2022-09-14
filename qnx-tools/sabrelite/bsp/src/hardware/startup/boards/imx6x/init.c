/*
 * $QNXLicenseC: 
 * Copyright 2013 QNX Software Systems.  
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
#include "mx6x_startup.h"
#include <arm/mx6x.h>

#define CCM_CGPR_WAIT_MODE_FIX		(1 << 17)

/*
 * Override the startup/lib's empty board_init function to include any code
 * applicable to ALL i.MX6x startups
 */
void board_init(void)
{

	/* Errata ERR006223 - CCM: Failure to resume from Wait/Stop mode with power gating
	 *
	 * When the Wait For Interrupt (WFI) instruction is executed, the Clock Control Module (CCM)
	 * will begin the WAIT mode preparation sequence. During the WAIT mode preparation sequence,
	 * there is a small window of time where an interrupt could potentially be detected after
	 * the L1 cache clock is gated but before the ARM clock is gated, causing a failure to resume
	 * from WFI.
	 *
	 * Freescale added a bit (offset 17) to CCM_CGPR which should be set at all times except before entering
	 * the STOP low power mode. Unless a BSP is using the STOP low power mode, CCM_CGPR[17] should
	 * always be set.
	 */

	unsigned wait_mode_workaround = FALSE;

	if (get_mx6_chip_type() == MX6_CHIP_TYPE_QUAD_OR_DUAL)
	{
		if (get_mx6_chip_rev() >=  MX6_CHIP_REV_1_2)
			wait_mode_workaround = TRUE;
	}
	else if (get_mx6_chip_type() == MX6_CHIP_TYPE_DUAL_LITE_OR_SOLO)
	{
		if (get_mx6_chip_rev() >=  MX6_CHIP_REV_1_1)
			wait_mode_workaround = TRUE;
	}

	if (wait_mode_workaround == TRUE)
		out32(MX6X_CCM_BASE + MX6X_CCM_CGPR, in32(MX6X_CCM_BASE + MX6X_CCM_CGPR) | CCM_CGPR_WAIT_MODE_FIX);

}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/init.c $ $Rev: 754185 $")
#endif
