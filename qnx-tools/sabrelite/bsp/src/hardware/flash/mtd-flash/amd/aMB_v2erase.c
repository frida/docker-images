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

#define MAX_ITER_TIMEOUT	1000000	// safety timeout for polling loop


/*
 * Summary
 *
 * MTD Version: 2 only
 * Bus Width:   8-bit and 16-bit
 * Note:        For early generation MirrorBit parts with known errata.
 *
 * Description
 *
 * This erase callout is for early generation MirrorBit parts that have several
 * known defects. Please contact your AMD representative for more details. The
 * f3s_aMB_*() callouts implement software workarounds to these errata, they
 * should be used as a matched set.
 */

extern pthread_key_t	MB_Key;

int f3s_aMB_v2erase(f3s_dbase_t *dbase,
                    f3s_access_t *access,
                    uint32_t flags,
                    uint32_t offset)
{
	volatile uint8_t *	memory;
	uintptr_t			amd_cmd1;
	uintptr_t			amd_cmd2;
	F3S_BASETYPE *		DQ4;
	F3S_BASETYPE		mask5 = (1 << 5) * flashcfg.device_mult;
	F3S_BASETYPE		mask6 = (1 << 6) * flashcfg.device_mult;
	F3S_BASETYPE		status,status2;
	int					i=0;

	/* Obtain pointer to the sector */
	memory = access->service->page(&access->socket, F3S_POWER_ALL, offset & amd_command_mask, NULL);
	if (memory == NULL) {
		fprintf(stderr, "(devf  t%d::%s:%d) page() returned NULL for offset 0x%x\n",
					pthread_self(), __func__, __LINE__, offset);
		return (errno);
	}

	/* Clear the chip's erase state */
	DQ4 = (F3S_BASETYPE *)pthread_getspecific(MB_Key);
	if (DQ4 == NULL) {
		DQ4 = malloc(sizeof(F3S_BASETYPE));
		if (DQ4 == NULL) {
			fprintf(stderr, "(devf  t%d::%s:%d) malloc(%d) returned NULL\n",
						pthread_self(), __func__, __LINE__, sizeof(F3S_BASETYPE));
			return (ENOMEM);
		}
		pthread_setspecific(MB_Key, DQ4);
	}
	*DQ4 = 0;

	/* Set the command address according to chip width */
	if (flashcfg.device_width == 1) {
		amd_cmd1 = AMD_CMD_ADDR1_W8;
		amd_cmd2 = AMD_CMD_ADDR2_W8;
	} else {
		amd_cmd1 = AMD_CMD_ADDR1_W16;
		amd_cmd2 = AMD_CMD_ADDR2_W16;
	}
	amd_cmd1 *= flashcfg.bus_width;
	amd_cmd2 *= flashcfg.bus_width;

	/* Issue unlock cycles */
	send_command(memory + amd_cmd1, AMD_UNLOCK_CMD1);
	send_command(memory + amd_cmd2, AMD_UNLOCK_CMD2);

	/* Issue erase command */
	send_command(memory + amd_cmd1, AMD_SECTOR_ERASE);

	/* Issue unlock cycles */
	send_command(memory + amd_cmd1, AMD_UNLOCK_CMD1);
	send_command(memory + amd_cmd2, AMD_UNLOCK_CMD2);

	/* Issue erase confirm */
	send_command(memory, AMD_ERASE_CONFIRM);

	/* Wait for 6 us - according to AMD document, the maximam delay is 4 us */
	nanospin_ns(6000);

	// DQ6 toggle algorithm
	do {
		status  = readmem(memory);
		status2  = readmem(memory);
		if ( !((status ^ status2) & mask6) ) {
			// no more toggling... erase complete
			return (EOK);
		}
		nanospin_ns(1000);
	} while ( !(status2  & mask5) && (i++ <= MAX_ITER_TIMEOUT) );
	
	if ( i > MAX_ITER_TIMEOUT ) {
		// DQ5 is still 0... error condition
		fprintf(stderr, "(devf  t%d::%s:%d) Errase failed... Max polling iterations reached\n",
					pthread_self(), __func__, __LINE__);
		return (-1);
	}

	// check DQ6 toggle
	status  = readmem(memory);
	status2  = readmem(memory);
	if ( (status ^ status2) & mask6 ) {
		// DQ6 still toggling...error condition
		fprintf(stderr, "(devf  t%d::%s:%d) Errase failed... DQ5 is high, but DQ6 is toggling\n",
					pthread_self(), __func__, __LINE__);
		return (-1);
	}
	
	return (EOK);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/amd/aMB_v2erase.c $ $Rev: 710521 $")
#endif
