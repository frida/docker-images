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
 * Method:      CFI only
 * MTD Version: 1 and 2
 * Bus Width:   8-bit and 16-bit
 * Boot-Block?: Yes
 * Note:        For older CFI compliant AMD flash, try f3s_aCFI_ident() first.
 *
 * Description
 *
 * This ident callout uses the Common Flash Interface (CFI) to recognize
 * chips. It supports both uniform and non-uniform block sizes (aka
 * boot-block flash).
 *
 * This callout is an alternate CFI routine. Some older chips used a different
 * command to enter CFI query mode. Try f3s_aCFI_ident() first.
 */

int32_t f3s_aCFIsm_ident(f3s_dbase_t * dbase,
                         f3s_access_t * access,
                         uint32_t flags,
                         uint32_t offset)
{
	volatile void *	memory;
	uintptr_t		amd_mult, amd_cmd1, amd_cmd2;
	F3S_BASETYPE	jedec_hi, jedec_lo;
	int32_t			unit_size;
	int32_t			geo_index;
	int32_t			geo_pos;
	int32_t			size;

	static f3s_dbase_t *probe;
	static f3s_dbase_t virtual[] =
	{
		{sizeof(f3s_dbase_t), 0xffff, 0x01, 0xc8, "Am29LV017B",
		0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{sizeof(f3s_dbase_t), 0xffff, 0x01, 0xa3, "Am29LV033C",
		0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{sizeof(f3s_dbase_t), 0xffff, 0x01, 0xc7, "Am29LV116B-T",
		0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{sizeof(f3s_dbase_t), 0xffff, 0x01, 0x4c, "Am29LV116B-B",
		0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{sizeof(f3s_dbase_t), 0xffff, 0x01, 0x22d7, "Am29LV640DU",
		0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{sizeof(f3s_dbase_t), 0xffff, 0x01, 0x22d7, "Am29LV641DU",
		0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{0, 0xffff, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	};

	/* If listing flag */
	if (flags & F3S_LIST_ALL) {
		/* Initialize on first pass */
		if (!probe) probe = virtual;

		/* If dbase is valid */
		if (probe->struct_size == 0) return (ENOENT);

		*dbase = *probe;
		probe++;

		return (EOK);
	}

	/* Prepare for x8 or x16 operation */
	if (flashcfg.device_width == 1) {
		amd_cmd1 = AMD_CMD_ADDR1_W8;
		amd_cmd2 = AMD_CMD_ADDR2_W8;
		amd_mult = 1;
	} else {
		amd_cmd1 = AMD_CMD_ADDR1_W16;
		amd_cmd2 = AMD_CMD_ADDR2_W16;
		amd_mult = 2;
	}
	amd_mult *= flashcfg.bus_width;
	amd_cmd1 *= flashcfg.bus_width;
	amd_cmd2 *= flashcfg.bus_width;

	size = amd_cmd1 + flashcfg.bus_width;

	/* Set proper page on socket */
	memory = access->service->page(&access->socket, F3S_POWER_ALL, offset & amd_command_mask, &size);
	if (memory == NULL) return (ERANGE);

	/* If size is ok */
	if (size < (amd_cmd1 + flashcfg.bus_width)) return (ENOTSUP);

	/* Issue unlock cycles */
	send_command(memory + (amd_cmd1 * flashcfg.bus_width), AMD_UNLOCK_CMD1);

	/* Wait for 6 us - according to AMD document, the maximam delay is 4 us */
	nanospin_ns(6000);

	send_command(memory + (amd_cmd2 * flashcfg.bus_width), AMD_UNLOCK_CMD2);

	/* Wait for 6 us - according to AMD document, the maximam delay is 4 us */
	nanospin_ns(6000);

	/* Issue read ident command */
	send_command(memory + (amd_cmd1 * flashcfg.bus_width), AMD_AUTOSELECT);

	/* Wait for 6 us - according to AMD document, the maximam delay is 4 us */
	nanospin_ns(6000);

	/* If jedec hi is not for AMD */
	if (readmem(memory) != (0x01 * flashcfg.device_mult)) return (ENOTSUP);

	/* Read JEDEC ID */
	jedec_hi = readmem(memory) / flashcfg.device_mult;
	jedec_lo = readmem(memory + flashcfg.bus_width) / flashcfg.device_mult;

	/* Issue read CFI command */
	send_command(memory + (AMD_CFI_ADDR * amd_mult), AMD_CFI_QUERY);

	/* Wait for 6 us - according to AMD document, the maximam delay is 4 us */
	nanospin_ns(6000);

	/* Look for "QRY" string */
	if(verbose > 4)
	{
		printf("(devf  t%d::%s:%d) QRY string = %c%c%c\n",
				pthread_self(), __func__, __LINE__,
		       (char)readmem(memory + (0x10 * amd_mult)),
		       (char)readmem(memory + (0x11 * amd_mult)),
		       (char)readmem(memory + (0x12 * amd_mult)));
	}

	if (readmem(memory + (0x10 * amd_mult)) != (F3S_BASETYPE)('Q' * flashcfg.device_mult) ||
		readmem(memory + (0x11 * amd_mult)) != (F3S_BASETYPE)('R' * flashcfg.device_mult) ||
		readmem(memory + (0x12 * amd_mult)) != (F3S_BASETYPE)('Y' * flashcfg.device_mult))
	{
			return (ENOTSUP);
	}

	/* setup database with proper info */

	/* Fill dbase entry */
	dbase->struct_size = sizeof(*dbase);
	dbase->jedec_hi    = jedec_hi;
	dbase->jedec_lo    = jedec_lo;

	dbase->name = "CFI";

	/* Read buffer size information */
	dbase->buffer_size   = (readmem(memory + (0x2b * amd_mult)) / flashcfg.device_mult);
	dbase->buffer_size <<= 8;
	dbase->buffer_size  += (readmem(memory + (0x2a * amd_mult)) / flashcfg.device_mult);

	/* Value is 2^N bytes per chip */
	dbase->buffer_size = 1 << dbase->buffer_size;

	/* Take interleave into consideration */
	dbase->buffer_size *= flashcfg.chip_inter;

	/* Read number of geometries */
	dbase->geo_num = readmem(memory + (0x2c * amd_mult)) / flashcfg.device_mult;
	if (verbose > 3)
		printf("(devf  t%d::%s:%d) dbase->geo_num = %d\n",
					pthread_self(), __func__, __LINE__, dbase->geo_num);

	/* Read geometry information */
	for (geo_index = 0, geo_pos = 0x2d; geo_index < dbase->geo_num; geo_index++, geo_pos += 4) {
		/* Read number of units */
		dbase->geo_vect[geo_index].unit_num   = (readmem(memory + ((geo_pos + 1) * amd_mult)) / flashcfg.device_mult);
		dbase->geo_vect[geo_index].unit_num <<= 8;
		dbase->geo_vect[geo_index].unit_num  += (readmem(memory + ((geo_pos + 0) * amd_mult)) / flashcfg.device_mult);
		dbase->geo_vect[geo_index].unit_num  += 1;

		/* Read size of unit */
		unit_size   = (readmem(memory + ((geo_pos + 3) * amd_mult)) / flashcfg.device_mult);
		unit_size <<= 8;
		unit_size  += (readmem(memory + ((geo_pos + 2) * amd_mult)) / flashcfg.device_mult);

		/* Interpret according to the CFI specs */
		if (unit_size == 0) unit_size  = 128;
		else                unit_size *= 256;

		/* Take interleave into consideration */
		unit_size *= flashcfg.chip_inter;

		/* Convert size to power of 2 */
		dbase->geo_vect[geo_index].unit_pow2 = 0;
		while (unit_size > 1) {
			unit_size >>= 1;
			dbase->geo_vect[geo_index].unit_pow2++;
		}

		if (verbose > 3) {
			printf("(devf  t%d::%s:%d) dbase->geo_vect[%d].unit_pow2 = %d\n",
					pthread_self(), __func__, __LINE__,
					geo_index,dbase->geo_vect[geo_index].unit_pow2);
			printf("(devf  t%d::%s:%d) dbase->geo_vect[%d].unit_num  = %d\n",
					pthread_self(), __func__, __LINE__,
					geo_index,dbase->geo_vect[geo_index].unit_num);
		}
	}

	/* Set bus and chip parameters */
	dbase->chip_inter = flashcfg.chip_inter;
	dbase->chip_width = flashcfg.bus_width;

	/* Set capabilities flags */
	dbase->flags = F3S_ERASE_FOR_READ | F3S_ERASE_FOR_WRITE;

	return (EOK);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/amd/aCFIsm_ident.c $ $Rev: 710521 $")
#endif
