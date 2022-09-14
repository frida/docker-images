/*
 * $QNXLicenseC: 
 * Copyright 2007, QNX Software Systems.  
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
 * Boot-Block?: Yes
 * Note:        Derived from the AMD A29F040 driver
 *
 */


#ifdef F3S_S29GLXXXS_DQPOLL

int f3s_s29glxxxs_v2sync(f3s_dbase_t *dbase,
                         f3s_access_t *access,
                         uint32_t flags,
                         uint32_t offset)
{
	volatile uint8_t *	memory;
	F3S_BASETYPE		mask6 = (1 << 6) * flashcfg.device_mult;
	F3S_BASETYPE		mask;
	F3S_BASETYPE		status[2];
	F3S_BASETYPE		toggle;

	/* Obtain pointer to the sector */
	memory = access->service->page(&access->socket, F3S_POWER_ALL, offset & amd_command_mask, NULL);
	if (memory == NULL) {
		fprintf(stderr, "(devf  t%d::%s:%d) page() returned NULL for offset 0x%x\n",
					pthread_self(), __func__, __LINE__, offset);
		return (errno);
	}

	if (suspended) {
		send_command(memory, AMD_ERASE_RESUME);
		suspended = 0;
		nanospin_ns(100000); /* delay 100us to allow erase make progress */
	}

	/* If DQ6 is still toggling */
	status[0] = readmem(memory);
	status[1] = readmem(memory);
	toggle    = (status[0] ^ status[1]) & mask6;
	if (toggle) {
		/* If DQ5 is *only* set on the chips that are still toggling */
		mask = (status[0] & (toggle >> 1)) << 1;
		if (mask == toggle) {
			/* Look for toggling again */
			status[0] = status[1];
			status[1] = readmem(memory);

			/* If *any* of the same DQ6 bits are still toggling */
			if ((status[0] ^ status[1]) & mask) {
				/*
				 * At this point we know that any good erases have finished
				 */

				/* We have an error */
				return (EIO);

			/* If DQ6 is still toggling */
			} else if ((status[0] ^ status[1]) & mask6) {
				/* Not done yet */
				return (EAGAIN);
			}

			/* Otherwise, DQ6 has stopped toggling */

		/* Otherwise, some chips are still erasing */
		} else {
			/* Not done yet */
			return (EAGAIN);
		}
	}

	/*
	 * We only get here if DQ6 is not toggling
	 */

	return (EOK);
}

#else

int f3s_s29glxxxs_v2sync(f3s_dbase_t *dbase,
                         f3s_access_t *access,
                         uint32_t flags,
                         uint32_t offset)
{
	volatile uint8_t *	memory;
	uintptr_t			amd_cmd;
	F3S_BASETYPE		rdymask = 0x80 * flashcfg.device_mult;
	F3S_BASETYPE		errmask = 0x30 * flashcfg.device_mult;
	F3S_BASETYPE		status;

	/* Obtain pointer to the sector */
	memory = access->service->page(&access->socket, F3S_POWER_ALL, offset & amd_command_mask, NULL);
	if (memory == NULL) {
		fprintf(stderr, "(devf  t%d::%s:%d) page() returned NULL for offset 0x%x\n",
					pthread_self(), __func__, __LINE__, offset);
		return (errno);
	}

	/* Set the command address according to chip width */
	if (flashcfg.device_width == 1)
		amd_cmd = AMD_CMD_ADDR1_W8;
	else
		amd_cmd = AMD_CMD_ADDR1_W16;
	amd_cmd *= flashcfg.bus_width;

	/* Issue RESUME command in case SYNC is required by a sector erase */
	if (!(flags & F3S_SYNC_FOR_WRITE) && suspended) {
		send_command(memory, AMD_ERASE_RESUME);
		suspended = 0;
		nanospin_ns(100000); /* delay 100us to allow erase make progress */
	}

	/* Read status register */
	send_command(memory + amd_cmd, AMD_STATUS_READ);
	status = readmem(memory);

	if ((status & rdymask) == rdymask) {

		if (status & errmask) {
			/* At least one error bit is set, reset device and return error */
			f3s_s29glxxxs_reset(dbase, access, 0, offset);
			return (EIO);
		} else
			/* Done */
			return (EOK);
	} 
	else
		/* Not done yet */
		return (EAGAIN);
}

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/spansion/s29glxxxs_v2sync.c $ $Rev: 738496 $")
#endif
