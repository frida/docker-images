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




#include <pthread.h>
#include <sys/f3s_mtd.h>

/*
 * Summary
 *
 * MTD Version:    2 only
 * Bus Width:      8-bit, 16-bit and 8/16-bit hybrid
 * Locking Method: Non-persistent
 *
 * Description
 *
 * Use this for newer Intel flash such as the "K" model StrataFlash that
 * cannot store the block lock status accross reboots.
 */

int f3s_iCFI_v2unlock(f3s_dbase_t *dbase,
                      f3s_access_t *access,
                      uint32_t flags,
                      uint32_t offset)
{
	volatile void *	memory;
	uint32_t		status;

	memory = access->service->page(&access->socket, F3S_POWER_ALL, offset, NULL);
	if (!memory)
	{
		fprintf(stderr, "(devf  t%d::%s:%d) page() returned NULL for offset 0x%x\n",
					pthread_self(), __func__, __LINE__, offset);
		return (errno);
	}

	/* Issue the unlock command */
	send_command(memory, INTEL_LOCK_CMD);
	send_command(memory, INTEL_UNLOCK);

	/* Wait for unlock to complete */
	do {
		status = readmem(memory);
	} while ((status & (0x80 * flashcfg.device_mult)) != (0x80 * flashcfg.device_mult));
	send_command(memory, INTEL_READ_ARRAY);

	/* Check for errors */
	if (intel_read_status(status) == -1) return (errno);
	return (EOK);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/intel/iCFI_v2unlock.c $ $Rev: 710521 $")
#endif
