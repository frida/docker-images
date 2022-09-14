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
 * Boot-Block?: Yes
 *
 * Description
 *
 * This is the only MTDv2 sync callout for Micron(Numonyx) flash.
 */

int f3s_nuCFI_v2sync(f3s_dbase_t *dbase,
                       f3s_access_t *access,
                       uint32_t flags,
                       uint32_t offset)
{
	volatile uint8_t *	memory;
	F3S_BASETYPE		mask2 = (1 << 2) * flashcfg.device_mult;
	F3S_BASETYPE		mask6 = (1 << 6) * flashcfg.device_mult;
	F3S_BASETYPE		mask7 = (1 << 7) * flashcfg.device_mult;
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
		usleep(500);
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
				fprintf(stderr, "%s: %d DQ5 error \n", __func__, __LINE__);

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

	/* Use DATA# polling to see if the erase is still in-progress,
	 * if it is, dq7 will be a zero.*/
	/*FYI: DQ7 is a zero during erase, but a ONE during erase-suspend,
	 * therefore DQ7 is only useful if we know which state the hw is in,
	 * which we don't...
	 *
	 * Adding variable "suspended", which will be 0 if not suspended,
	 * and 1 if suspended.  If we're suspended, then the erase
	 * is clearly not going to be finished
	 *
	 */
	if ((readmem(memory) & mask7) == (mask7 * suspended)){
		/* We're still erasing. DQ6 polling messed up somehow */
		return EAGAIN;
	}
	

	/*
	 * We only get here if DQ6 is not toggling
	 */

	/* Double check DQ2 */
	if ((status[0] ^ status[1]) & mask2) {
		send_command(memory, AMD_ERASE_RESUME);
		usleep(500);
		fprintf(stderr, "%s: %d DQ2 toggling, still in erase-suspend \n", __func__, __LINE__);
		return (EAGAIN);
	}

	return (EOK);
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/numonyx/nuCFI_v2sync.c $ $Rev: 710521 $")
#endif
