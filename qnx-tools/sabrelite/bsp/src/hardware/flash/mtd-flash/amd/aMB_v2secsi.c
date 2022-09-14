/*
 * $QNXLicenseC:
 * Copyright 2011, QNX Software Systems. 
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
 * MTD Version:     2 only
 * Bus Width:       8-bit and 16-bit
 * Buffered Writes: No
 *
 * Description
 *
 */

/* For MirrorBit GL-N/P, the SSR consistes of 256 bytes */
#define GLNP_SSR_SIZE        256
#define GLNP_SSR_OFF         0

/* For GL-S, the SSR consists of 512-byte Factory Locked Secure Silicon Region 
 * and 512-byte Customer Locked Secure Silicon Region, we only access customer
 * locked SSR. 
 */
#define GLS_SSR_OFF          (0x200*flashcfg.chip_inter)
#define GLS_SSR_SIZE         512
#define GLS_VERSION(dbase)   ((dbase)->proc_tech >= 0x1C)

#undef  AMD_LOCK_REG_ENTER
#define AMD_LOCK_REG_ENTER    0x40

/* Lock operations */
#define LOCK_GET              0 /* get lock stat */
#define LOCK_SET              1 /* set lock bit */


static int secsi_poll(volatile void *memory)
{
	F3S_BASETYPE	status[2];
	F3S_BASETYPE	mask6 = (1 << 6) * flashcfg.device_mult;
	F3S_BASETYPE	mask;
	F3S_BASETYPE	toggle;


	/* According to AMD document, the maximam delay required by device is 4us,
	 * which means, the MINIMUM delay is 4us from software's perspective.
	 * Delay 6us here.
	 */
	nanospin_ns(6000);

	/* Poll for write completion. Add an additional read here according to
	 * Spansion's suggestion:  reading status three times to avoid possible 
	 * garbage data in first read 
	 */
	status[1] = readmem(memory);
	while (1) {
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
				nanospin_ns(6000);
				return (-1);
			}
		}
	}
	return (0);
}

static int secsi_lock(f3s_dbase_t *dbase, 
			volatile uint8_t *secsi_base, 
			uintptr_t amd_cmd1, 
			uintptr_t amd_cmd2,
			int ops)
{
	F3S_BASETYPE		status;
	F3S_BASETYPE		mask = 0;
	F3S_BASETYPE		mask0 = (1 << 0) * flashcfg.device_mult;
	F3S_BASETYPE		mask6 = (1 << 6) * flashcfg.device_mult;
	int			ret = EOK;

	/* Enter the Lock Register Mode */
	send_command(secsi_base + amd_cmd1, AMD_UNLOCK_CMD1);
	send_command(secsi_base + amd_cmd2, AMD_UNLOCK_CMD2);
	send_command(secsi_base + amd_cmd1, AMD_LOCK_REG_ENTER);

	/* Check lock status */
	mask = GLS_VERSION(dbase) ? mask6 : mask0;
	status = readmem(secsi_base);
	if (verbose > 2) fprintf(stderr, "%s: lock status %llx \n", __func__, status);
	if (ops == LOCK_GET) {
		if ((status & mask) == mask) {
			ret = 0;
		} else {
			ret = 1;
			if ((status & mask) != 0) {
				fprintf(stderr, "SSR partially locked \n");
			}
		}
		/* Leave the lock register mode */
		send_command(secsi_base + amd_cmd1, AMD_PROTECT_EXIT1);
		send_command(secsi_base + amd_cmd2, AMD_PROTECT_EXIT2);
		return ret;
	}

	/* Program the secsi bit */
	send_command(secsi_base, AMD_PROGRAM);
	
	/* Directly write data, which multiplies out for each 
	 * interleaved part
 	 */
	writemem(secsi_base, ~mask);
	if (secsi_poll(secsi_base) == -1){
		fprintf(stderr, "overpoll error locking secsi\n");
		errno = EIO;
		ret = -1;
	}

	/* Leave the lock register mode */
	send_command(secsi_base + amd_cmd1, AMD_PROTECT_EXIT1);
	send_command(secsi_base + amd_cmd2, AMD_PROTECT_EXIT2);
	return ret;
}

static int secsi_write (volatile uint8_t *secsi_base, 
			uintptr_t amd_cmd1, 
			uintptr_t amd_cmd2,
			int32_t ssr_off,
			int32_t size,
			uint8_t * buffer)
{
	intunion_t value;
	int offset = 0, ret = EOK;

	/* Write the cmd-line value into SecSi, loop through the buffer
	 * passed in and program each word.
	 */
	while (size > 0) {
		send_command(secsi_base + amd_cmd1, AMD_UNLOCK_CMD1);
		send_command(secsi_base + amd_cmd2, AMD_UNLOCK_CMD2);
		send_command(secsi_base + amd_cmd1, AMD_PROGRAM);
		
		if (size < flashcfg.bus_width) {
			value.w64 = ~0;
			memcpy((void*)&value, &buffer[offset], size);
		} else {
			memcpy((void*)&value, &buffer[offset], flashcfg.bus_width);
		}
			
		write_value(secsi_base + ssr_off + offset, &value);
		if(secsi_poll(secsi_base + ssr_off + offset) == -1) {
			fprintf(stderr, "overpoll error writing secsi \n");
			errno = EIO;
			ret = -1;
			break;
		}
		size -= flashcfg.bus_width;
		offset += flashcfg.bus_width;
	}
	return ret;
}

int f3s_aMB_v2ssrop(f3s_dbase_t * dbase,
			f3s_access_t * access,
			uint32_t op,
			uint32_t offset,
			int32_t size,
			uint8_t * buffer)
{
	volatile uint8_t *	secsi_base;
	uintptr_t		amd_cmd1;
	uintptr_t		amd_cmd2;
	uint32_t 		ssr_maxsz; 
	int			ssr_off;
	int			ret = EOK;

	/* Set the command address according to chip width.
	 * Note: Spansion device only supports two kinds of command addresses,
	 *	 One is for x8 device (0xaaa and 0x555), the other 
	 *       is for x16/x32 device (0x555 and 0x2aa).
	 */
	if (flashcfg.device_width == 1) {
		amd_cmd1 = AMD_CMD_ADDR1_W8;
		amd_cmd2 = AMD_CMD_ADDR2_W8;
	} else {
		amd_cmd1 = AMD_CMD_ADDR1_W16;
		amd_cmd2 = AMD_CMD_ADDR2_W16;
	}
	amd_cmd1 *= flashcfg.bus_width;
	amd_cmd2 *= flashcfg.bus_width;

	/* Set secsi base pointer */
	if (verbose > 2) fprintf(stderr, "SSR offset %X \n", offset);
	secsi_base = access->service->page(&access->socket, F3S_POWER_ALL, offset, NULL);
	if (secsi_base == NULL) {
		fprintf(stderr, "(devf  t%d::%s:%d) page() returned NULL \n",
					pthread_self(), __func__, __LINE__);
		errno = EIO;
		return (-1);
	}

	if (op == F3S_SSR_OP_LOCK) {
		return secsi_lock(dbase, secsi_base, amd_cmd1, amd_cmd2, LOCK_SET);
	} else if (op == F3S_SSR_OP_STAT) {
		*buffer = secsi_lock(dbase, secsi_base, amd_cmd1, amd_cmd2, LOCK_GET);
		return (EOK); 
	} else if (op == F3S_SSR_OP_WRITE) {
		if (secsi_lock(dbase, secsi_base, amd_cmd1, amd_cmd2, LOCK_GET)) {
			fprintf(stderr, "Invalid write to locked SSR \n");
			errno = EINVAL;
			return -1;
		}
	}

	/* Ensure the length of secsi buffer isn't bigger than the chips */
	ssr_maxsz = flashcfg.chip_inter * (GLS_VERSION(dbase) ? GLS_SSR_SIZE : GLNP_SSR_SIZE);
	if (size > ssr_maxsz){
		fprintf(stderr, "buffer size of %d is greater than chip's %d\n", size, ssr_maxsz);
		errno = EINVAL;
		return -1;
	}

	/* Enter the SecSci Mode */
	send_command(secsi_base + amd_cmd1, AMD_UNLOCK_CMD1);
	send_command(secsi_base + amd_cmd2, AMD_UNLOCK_CMD2);
	send_command(secsi_base + amd_cmd1, AMD_SECSI_ENTER);

	ssr_off = GLS_VERSION(dbase) ? GLS_SSR_OFF : GLNP_SSR_OFF;
	if (op == F3S_SSR_OP_READ) { /* read data from SecSi */
			memcpy (buffer, (uint8_t *)secsi_base + ssr_off, size);
	} else if (op == F3S_SSR_OP_WRITE) {
		ret = secsi_write(secsi_base, amd_cmd1, amd_cmd2, ssr_off, size, buffer);
	} else {
		errno = ENOTSUP;
		ret = -1;
	}

	/* Leave the SecSi Mode*/
	send_command(secsi_base + amd_cmd1, AMD_UNLOCK_CMD1);
	send_command(secsi_base + amd_cmd2, AMD_UNLOCK_CMD2);
	send_command(secsi_base + amd_cmd1, AMD_PROTECT_EXIT1);
	send_command(secsi_base + amd_cmd2, AMD_PROTECT_EXIT2);

	return ret;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/amd/aMB_v2secsi.c $ $Rev: 741330 $")
#endif
