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
 * MTD Version: 2 only
 * Bus Width:   8-bit, 16-bit and 8/16-bit hybrid
 * Boot-Block?: Yes
 *
 * Description
 *
 * This sync callout is for both uniform and non-uniform block flash.
 * This is the only MTDv2 sync callout for Intel flash.
 */

int f3s_iCFI_v2sync(f3s_dbase_t *dbase,
                    f3s_access_t *access,
                    uint32_t flags,
                    uint32_t offset)
{
	volatile void *	memory;
	F3S_BASETYPE		status;

	memory = access->service->page(&access->socket, F3S_POWER_ALL, offset, NULL);
	if (!memory)
	{
		fprintf(stderr, "(devf  t%d::%s:%d) page() returned NULL for offset 0x%x\n",
						pthread_self(), __func__, __LINE__, offset);
		return (errno);
	}

	/* Read the command status */
	send_command(memory,INTEL_READ_STATUS);
	status = readmem(memory);

	/* If we're done, check for errors */
	if ((status & (0x80 * flashcfg.device_mult)) ==
	              (0x80 * flashcfg.device_mult))
	{
		if (intel_read_status(status) == -1) return (errno);

		/* Return to read array mode and indicate operation complete */
		send_command(memory, INTEL_READ_ARRAY);
		return (EOK);
	}

	/* Still erasing */
	return (EAGAIN);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/intel/iCFI_v2sync.c $ $Rev: 710521 $")
#endif
