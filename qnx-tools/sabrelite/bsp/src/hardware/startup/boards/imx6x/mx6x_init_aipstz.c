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


#include "startup.h"
#include <arm/mx6x.h>

/*
 * The AHB to IP Bridge (AIPSTZ) is a configurable bus bridge which can restrict access to peripherals on the
 * IP bus (such as FlexCAN Controllers, UART-2,3,4,5).  Note that the AIPSTZ restrictions do not apply to shared peripherals - see
 * AIPS section, SPBA section of i.MX6x reference manual for more information.  Additionally note that the AIPSTZ
 * registers have reset values such that by default all bus masters except SDMA, CAAM are trusted.
 *
 * The code below removes any restrictions on access to the IP bus to prevent bus errors when accessing IP Peripherals.
 * Removing IP bus access restrictions means that SDMA accesses to non shared IP Bus peripherals will succeed, otherwise a bus error would have occurred.
 */
void mx6x_init_aipstz(void)
{
	/*
	 * Set AIPS master access levels like so:
	 * Master 0 (All i.MX6x masters excluding ARM core, SDMA, CAMM) - trusted master
	 * Master 1 (ARM CORE) - trusted master
	 * Master 2 (CAAM) - non trusted master (user mode)
	 * Master 3 (SDMA) - non trusted master (user mode)
	 */

	/*
	 * Set all AIPS peripherals to not require master to need superlevel privilege for peripheral access
	 */
	out32(MX6X_AIPS1_CONFIG + MX6X_AIPS_OPACR0, 0x0);
	out32(MX6X_AIPS1_CONFIG + MX6X_AIPS_OPACR1, 0x0);
	out32(MX6X_AIPS1_CONFIG + MX6X_AIPS_OPACR2, 0x0);
	out32(MX6X_AIPS1_CONFIG + MX6X_AIPS_OPACR3, 0x0);
	out32(MX6X_AIPS1_CONFIG + MX6X_AIPS_OPACR4, in32(MX6X_AIPS1_CONFIG + MX6X_AIPS_OPACR4) & 0x00FFFFFF);

	out32(MX6X_AIPS2_CONFIG + MX6X_AIPS_OPACR0, 0x0);
	out32(MX6X_AIPS2_CONFIG + MX6X_AIPS_OPACR1, 0x0);
	out32(MX6X_AIPS2_CONFIG + MX6X_AIPS_OPACR2, 0x0);
	out32(MX6X_AIPS2_CONFIG + MX6X_AIPS_OPACR3, 0x0);
	out32(MX6X_AIPS2_CONFIG + MX6X_AIPS_OPACR4, in32(MX6X_AIPS2_CONFIG + MX6X_AIPS_OPACR4) & 0x00FFFFFF);
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/mx6x_init_aipstz.c $ $Rev: 729057 $")
#endif
