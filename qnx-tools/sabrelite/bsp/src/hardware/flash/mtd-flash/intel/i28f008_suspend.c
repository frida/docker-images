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
 * MTD Version: 1 only
 * Bus Width:   8-bit, 16-bit and 8/16-bit hybrid
 *
 * Description
 *
 * This is the only MTDv1 erase suspend function for Intel flash.
 */

int32_t f3s_i28f008_suspend(f3s_dbase_t *dbase,
                            f3s_access_t *access,
                            uint32_t flags,
                            uint32_t offset)
{
	uint32_t status;
	volatile void *memory;

	memory= access->service->page(&access->socket, F3S_POWER_ALL, offset, NULL);
	if (!memory)
		return -1;

	/* issue suspend command */
	send_command(memory, INTEL_ERASE_SUSPEND);
	/* issue read status command */
	send_command(memory, INTEL_READ_STATUS);

	if(intel_poll(memory, F3S_I28F008_POLL) == -1)
	{
		fprintf(stderr,"(devf  t%d::%s:%d) over poll reading status\n",
					pthread_self(), __func__, __LINE__);
		return(-1);
	}

	status=readmem(memory);
	send_command(memory, INTEL_READ_ARRAY);
	return 1;
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/intel/i28f008_suspend.c $ $Rev: 710521 $")
#endif
