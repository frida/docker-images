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
#include <stdio.h>
#include <sys/f3s_mtd.h>

/*
 * Summary
 *
 * MTD Version: 2 only
 * Bus Width:   8-bit and 16-bit
 *
 * Description
 *
 * This resume callout is for early generation MirrorBit parts that have
 * several known defects. Please contact your AMD representative for more
 * details. The f3s_aMB_*() callouts implement software workarounds to these
 * errata, they should be used as a matched set.
 */

int f3s_aMB_v2resume(f3s_dbase_t *dbase,
                     f3s_access_t *access,
                     uint32_t flags,
                     uint32_t offset)
{
	volatile uint8_t *	memory;
	uintptr_t			amd_cmd1;
	uintptr_t			amd_cmd2;
	intunion_t			value;
	uint32_t			buf_count;
	uint32_t			i;

	/* Use last or second last sector for the workaround */
	i = 1 << dbase->geo_vect[dbase->geo_num - 1].unit_pow2;
	if (offset >= (access->socket.window_size - i)) {
		i = access->socket.window_size - (i << 1);
	} else {
		i = access->socket.window_size - i;
	}

	/* Obtain pointer to another sector */
	memory = access->service->page(&access->socket, F3S_POWER_ALL, i, NULL);
	if (memory == NULL) {
		fprintf(stderr, "(devf  t%d::%s:%d) page() returned NULL for offset 0x%x\n",
					pthread_self(), __func__, __LINE__, offset);
		return (errno);
	}

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

	/* Issue a NOOP buffered write */
	send_command(memory + amd_cmd1, AMD_UNLOCK_CMD1);
	send_command(memory + amd_cmd2, AMD_UNLOCK_CMD2);
	send_command(memory, AMD_WRITE_BUFFER);

	buf_count = dbase->buffer_size / flashcfg.device_width / dbase->chip_inter;
	send_command(memory, buf_count - 1);

	memset (&value, 0xFF, sizeof(value));
	for (i = 0; i < buf_count; i++) {
		write_value(memory + (i * flashcfg.bus_width), &value);
	}

	send_command(memory, AMD_BUFFER_CONFIRM);

	/* Wait for 6 us - according to AMD document, the maximam delay is 4 us */
	nanospin_ns(6000);

	/* Wait for completion (don't care what happens) */
	/* DQ1 is "write-to-buffer abort", so applies here */
	if (amd_poll(&value, memory, 1) == -1) {
		/* do a "write-to-buffer abort sequence" */
		send_command(memory + amd_cmd1, AMD_UNLOCK_CMD1);
		send_command(memory + amd_cmd2, AMD_UNLOCK_CMD2);
		send_command(memory + amd_cmd1, AMD_READ_MODE);
	}

	/* Obtain pointer to the erasing sector */
	memory = access->service->page(&access->socket, F3S_POWER_ALL, offset & amd_command_mask, NULL);
	if (memory == NULL) {
		fprintf(stderr, "(devf  t%d::%s:%d) page() returned NULL for offset 0x%x\n",
					pthread_self(), __func__, __LINE__, offset);
		return (errno);
	}

	/* Issue resume command */
	send_command(memory, AMD_ERASE_RESUME);

	suspended = 0;

	return (EOK);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/amd/aMB_v2resume.c $ $Rev: 710521 $")
#endif
