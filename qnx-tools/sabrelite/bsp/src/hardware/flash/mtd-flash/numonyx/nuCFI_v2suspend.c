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
 * Bus Width:   8-bit and 16-bit
 *
 * Description
 *
 * This is the recommended MTDv2 suspend callout.
 */

int f3s_nuCFI_v2suspend(f3s_dbase_t *dbase,
                       f3s_access_t *access,
                       uint32_t flags,
                       uint32_t offset)
{
	volatile uint8_t *memory;
	F3S_BASETYPE	mask6 = (1 << 6) * flashcfg.device_mult;
	F3S_BASETYPE	status[2];
	F3S_BASETYPE	toggle;

	/* Do nothing if erase has been suspened */
	if (suspended)  return (EOK);

	/* Obtain pointer to the sector */
	memory = access->service->page(&access->socket, F3S_POWER_ALL, offset & amd_command_mask, NULL);
	if (memory == NULL) {
		fprintf(stderr, "(devf  t%d::%s:%d) page() returned NULL for offset 0x%x\n",
					pthread_self(), __func__, __LINE__, offset);
		return (errno);
	}

	/* Issue suspend command */
	send_command(memory, AMD_ERASE_SUSPEND);

	/* We will be erase-suspended once the following conditions are met:
	 * 1. the erase suspend timeout (5us typ. 20us max) has passed
	 * 2. DQ6 has stopped toggling
	 * 3. DQ2 has started toggling
	 * 4. DQ7 is 1
	 */

	/* Wait for 50us for Numonyx EW */
	nanospin_ns(50000);

	/* Wait until DQ6 stops toggling */
	while (1) {
		/* Cycle the old status around so we only need to read once */
		status[0] = readmem(memory);
		status[1] = readmem(memory);
		toggle = (status[0] ^ status[1]) & mask6;

		/* If DQ6 has stopped toggling */
		if (!toggle) {
			/* Erase completed successfully */
			suspended = 1;
			return (EOK);
		}

		/* If DQ5 is set on *any* of the chips that are still toggling */
		if (status[0] & (toggle >> 1)) {
			/* Double check that DQ6 is still toggling */
			/* If true,  then we must return ECANCELED and wait for sync */
			/* If false, then erase is complete, and still wait for sync */

			return (ECANCELED);
		}
	}

	/* Shouldn't get here */
	return (EOK);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/numonyx/nuCFI_v2suspend.c $ $Rev: 710521 $")
#endif
