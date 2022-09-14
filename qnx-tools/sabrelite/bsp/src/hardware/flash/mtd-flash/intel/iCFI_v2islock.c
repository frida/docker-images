/*
 * $QNXLicenseC:
 * Copyright 2008, QNX Software Systems. 
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





#include <sys/f3s_mtd.h>

/*
 * Summary
 *
 * MTD Version:    2 only
 * Bus Width:      8-bit, 16-bit and 8/16-bit hybrid
 * Locking Method: Persistent and Non-persistent
 *
 * Description
 *
 * Use this for Intel flash capable of block locking. It requires that you use
 * the CFI ident callout.
 */

int f3s_iCFI_v2islock(f3s_dbase_t *dbase,
                      f3s_access_t *access,
                      uint32_t flags,
                      uint32_t offset)
{
	volatile void  *memory;
	uint32_t		status;

	memory = access->service->page (&access->socket, F3S_POWER_ALL, offset, NULL);
	if (memory == NULL) return (errno);

	/* Obtain lock status for the unit */
	send_command (memory, INTEL_READ_QUERY);
	status = readmem (memory + (0x02 * flashcfg.cfi_width));

	/* Return to read array mode */
	send_command(memory, INTEL_READ_ARRAY);

	/* Check for lock status */
	if (status & (0x01 * flashcfg.device_mult)) return (EROFS);
	return (EOK);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/intel/iCFI_v2islock.c $ $Rev: 710521 $")
#endif
