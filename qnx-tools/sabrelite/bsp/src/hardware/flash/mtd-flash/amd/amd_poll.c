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
 * Description
 *
 * This is not a valid MTD callout. For internal use by other MTD callouts.
 */

int  amd_poll(intunion_t *value, volatile void *memory, int dq1)
{
	uintptr_t				amd_cmd1;
	uintptr_t				amd_cmd2;
	volatile F3S_BASETYPE	status[2];
	F3S_BASETYPE			mask6 = (1 << 6) * flashcfg.device_mult;
	F3S_BASETYPE			mask7 = (1 << 7) * flashcfg.device_mult;
	F3S_BASETYPE			mask;
	F3S_BASETYPE			toggle;
	F3S_BASETYPE			data = 0, check;
	uint32_t    			loops;

	/* get data dq7 for polling */
	if (flashcfg.bus_width == 1)
		data = value->w8 & mask7;
	else if (flashcfg.bus_width == 2)
		data = value->w16 & mask7;
	else if (flashcfg.bus_width == 4)
		data = value->w32 & mask7;
	else if (flashcfg.bus_width == 8)
		data = value->w64 & mask7;
					
	if (flashcfg.device_width == 1) {
		amd_cmd1 = AMD_CMD_ADDR1_W8;
		amd_cmd2 = AMD_CMD_ADDR2_W8;
	} else {
		amd_cmd1 = AMD_CMD_ADDR1_W16;
		amd_cmd2 = AMD_CMD_ADDR2_W16;
	}
	amd_cmd1 *= flashcfg.bus_width;
	amd_cmd2 *= flashcfg.bus_width;


	/* According to Spansion spec, the typical programing time is 240us.
	 * We set the max to 10000us (CFI says 2**5 = 32x typ).
	 * Since the real loop is much slower than empty loop it was
	 * caliberated, the real delay should be longer.
	 */
	loops = UINT32_MAX;
	if (flashcfg.program_timeout != 0) {
		loops = nanospin_ns_to_count(flashcfg.program_timeout*1000);
		if (loops == 1) loops++;
	}

	while (--loops) {
		/* Cycle the old status around so we only need to read once */
		status[0] = readmem(memory);
		status[1] = readmem(memory);

		/* Stop if DQ6 has stopped toggling */
		toggle = (status[0] ^ status[1]) & mask6;
		if (!toggle) break;

		/* If DQ5 is *only* set on the chips that are still toggling */
		mask = (status[0] & (toggle >> 1)) << 1;
		if (mask == toggle) {
			/* Poll again */
			status[0] = readmem(memory);
			status[1] = readmem(memory);

			/* If *any* of the same DQ6 bits are still toggling */
			if ((status[0] ^ status[1]) & mask) {
				/*
				 * At this point we know that any good writes have finished
				 */
				fprintf(stderr, "%s: %d DQ5 set, program error \n", __func__, __LINE__);
				return (-1);
			}

			/* Otherwise, DQ6 has stopped toggling */

		/* If this chip supports DQ1 errors */
		} else if (dq1) {
			/* If DQ1 is *only* set on the chips that are still toggling */
			mask = (status[0] & (toggle >> 5)) << 5;
			if (mask == toggle) {
				/* Poll again */
				status[0] = readmem(memory);
				status[1] = readmem(memory);

				/* If *any* of the same DQ6 bits are still toggling */
				if ((status[0] ^ status[1]) & mask) {
					/*
					 * At this point we know that any good writes have finished
					 */
					fprintf(stderr, "%s: %d DQ1 set, program error \n", __func__, __LINE__);
					return (-1);
				}
			}
		}
	}
	if(loops == 0) {
		fprintf(stderr, "%s: %d DQ6 toggling timeout \n", __func__, __LINE__);
		return (-1); 
	}

	/* poll for write completion with DQ7 (DATA#) */
	while (--loops)
	{
		/* Check, data, and mask7 are all the full data
		 * width, and have all be multiplied against
		 * flashcfg.device_mult.
		 */
		status[0] = readmem(memory);
		check = (status[0] ^ data) & mask7;
		if (!check){
			/* all flash parts are done the operation */
			break;
		}
		
		/*
		 * check dq5 for timeout (with shifted result
		 * from dq7 test).
		 *
		 * Because one flash device may complete before the others,
		 * we have to mask the 5th data bit on each chip against
		 * "check",so that we only look at the 5th bit on the chips
		 * which are still in progress.
		 * 
		 */
		if ((status[0] & (check >> 2)) == (check >> 2)){
			/* It's possible the chip finished just as we read dq5,
			 * so better go take a peek at dq7 again to make sure
			 * it's still in progress.
			 * */
			if ((readmem(memory) & mask7) ^ data)
			{
				fprintf(stderr, "%s: %d DQ5 set, program error\n", __func__, __LINE__);
				return -1;
			}
		}
		/* If this chip supports DQ1 errors */
		else if (dq1) {
			/* "check" has bit 7 set for each chip that is still in-progress.
			 * so we shift it so that bit 1 set, then AND that against status*/
			if (status[0] & (check>>6)) {
				/* Re-check that DQ7 is still showing in-progress.
				 * Don't worry about which chip, just look to see if 
				 * ANY are still in-progress.
				 * */
				if ((readmem(memory) & mask7) ^ data) {
					/*
					 * At this point we know that any good writes have finished
					 */
					fprintf(stderr, "%s: %d DQ1 set, program error\n", __func__, __LINE__);

					return (-1);
				}
			}
		}

	}
	if(loops == 0) {
		fprintf(stderr, "%s: %d DQ7 polling Timeout \n", __func__, __LINE__);
		return (-1); 
	}

	return (0);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/amd/amd_poll.c $ $Rev: 710521 $")
#endif
