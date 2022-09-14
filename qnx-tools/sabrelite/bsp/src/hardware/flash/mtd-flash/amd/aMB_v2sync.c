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
 * Note:        For early generation MirrorBit parts with known errata.
 *
 * Description
 *
 * This sync callout is for early generation MirrorBit parts that have several
 * known defects. Please contact your AMD representative for more details. The
 * f3s_aMB_*() callouts implement software workarounds to these errata, they
 * should be used as a matched set.
 */

extern pthread_key_t	MB_Key;

int f3s_aMB_v2sync(f3s_dbase_t *dbase,
                   f3s_access_t *access,
                   uint32_t flags,
                   uint32_t offset)
{
	volatile uint8_t *	memory;
	F3S_BASETYPE *		DQ4;
	F3S_BASETYPE		mask6 = (1 << 6) * flashcfg.device_mult;
	F3S_BASETYPE		mask4 = (1 << 4) * flashcfg.device_mult;
	F3S_BASETYPE		mask;
	F3S_BASETYPE		status[2];
	F3S_BASETYPE		toggle;
	uint32_t			end;
	uint32_t			unit_pow2;
	uint32_t			i;
	uint32_t			temp;

	/* Compute sector size */
	i    = 0;
	temp = 0;
	while (1) {
		temp += dbase->geo_vect[i].unit_num << dbase->geo_vect[i].unit_pow2;

		if (offset < temp) {
			unit_pow2 = dbase->geo_vect[i].unit_pow2;
			break;
		}

		i++;
		if (i >= dbase->geo_num) i = 0;
	}

	/* Compute the offset (relative to the sector) of the last word */
	end = (1 << unit_pow2) - flashcfg.bus_width;

	/* Obtain pointer to the start of the sector */
	memory = access->service->page(&access->socket, F3S_POWER_ALL, offset & (0xFFFFFFFF << unit_pow2), NULL);
	if (memory == NULL) {
		fprintf(stderr, "(devf  t%d::%s:%d) page() returned NULL for offset 0x%x\n",
					pthread_self(), __func__, __LINE__, offset);
		return (errno);
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

		/* If DQ4 is set on *any* of the chips that are still toggling */
		} else {
			mask = (status[0] & (toggle >> 2));
			if (mask) {
				/* Note: Can't check status[1] in case it has gone to read mode */

				/* Remember that we saw DQ4 on that chip(s) */
				DQ4   = (F3S_BASETYPE *)pthread_getspecific(MB_Key);
				*DQ4 |= mask;

				/* Not done yet */
				return (EAGAIN);

			/* Otherwise, some chips are still erasing */
			} else {
				/* Not done yet */
				return (EAGAIN);
			}
		}
	}

	/*
	 * We only get here if DQ6 is not toggling
	 */

	/* If we have seen DQ4 on every chip */
	DQ4 = (F3S_BASETYPE *)pthread_getspecific(MB_Key);
	if ((*DQ4) == mask4) {
		if (flashcfg.device_width == 1) toggle = 0xFF   * flashcfg.device_mult;
		else                            toggle = 0xFFFF * flashcfg.device_mult;

		/* If first word was properly erased */
		if (status[1] == toggle) {
			/* If last word was properly erased */
			if (readmem(memory + end) == toggle) {
				/* Success */
				return (EOK);
			}
		}
	}

	/* Otherwise, suspect that we've hit the errata */
	if (verbose > 0) {
		fprintf(stderr, "(devf  t%d::%s:%d) MirrorBit erase / suspend errata - retrying\n",
					pthread_self(), __func__, __LINE__);
	}

	return (EIO);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/amd/aMB_v2sync.c $ $Rev: 710521 $")
#endif
