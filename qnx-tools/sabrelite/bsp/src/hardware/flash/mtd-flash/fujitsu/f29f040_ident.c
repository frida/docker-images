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
 * Summary
 *
 * Method:      "JEDEC" ID only
 * MTD Version: 1 and 2
 * Bus Width:   16-bit only
 * Boot-Block?: No
 *
 * Description
 *
 * This ident callout uses the "JEDEC" ID code to recognize chips with
 * uniform block sizes. It is also assumed that the data bus to the chip is
 * 16-bits.
 *
 * Use this for really old legacy flash chips. If the ID for your chip is not
 * listed, copy this file into your driver's source directory and add an entry
 * for your chip.
 */

int32_t f3s_f29f040_ident(f3s_dbase_t *dbase,
                          f3s_access_t *access,
                          uint32_t flags,
                          uint32_t offset)
{
	volatile uint8_t *	memory;
	uintptr_t			amd_cmd1, amd_cmd2;
	F3S_BASETYPE		jedec_hi, jedec_lo;

	static f3s_dbase_t *probe;
	static f3s_dbase_t virtual[] =
	{
		{sizeof(f3s_dbase_t), 0xffff, 0x04, 0x3d, "MBM29F017",
			F3S_ERASE_FOR_ALL,
			1, 1, 10000, 1000000000, 50000, 50000,
		1, 1000000, 0, 0, 0, 1, {{32, 16}}},
		{sizeof(f3s_dbase_t), 0xffff, 0x04, 0xad, "MBM29F016",
			F3S_ERASE_FOR_ALL,
			1, 1, 10000, 1000000000, 50000, 50000,
		1, 1000000, 0, 0, 0, 1, {{32, 16}}},
		{sizeof(f3s_dbase_t), 0xffff, 0x04, 0xd5, "MBM29F080",
			F3S_ERASE_FOR_ALL,
			1, 1, 10000, 1000000000, 50000, 50000,
		1, 1000000, 0, 0, 0, 1, {{16, 16}}},
		{sizeof(f3s_dbase_t), 0xffff, 0x04, 0xa4, "MBM29F040",
			F3S_ERASE_FOR_ALL,
			1, 1, 10000, 1000000000, 50000, 50000,
		1, 1000000, 0, 0, 0, 1, {{8, 16}}},
		{0, 0xffff, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	};

	amd_cmd1 = AMD_CMD_ADDR1_W16;
	amd_cmd2 = AMD_CMD_ADDR2_W16;
	amd_cmd1 *= flashcfg.bus_width;
	amd_cmd2 *= flashcfg.bus_width;

	/* Check listing flag */
	if (flags & F3S_LIST_ALL)
	{
		/* If first pass */
		if (!probe) probe = virtual;

		/* If dbase is valid */
		if (!probe->struct_size) return (ENOENT);

		*dbase = *probe;
		probe++;

		return (EOK);
	}

	/* Set proper page on socket */
	memory = access->service->page(&access->socket, F3S_POWER_ALL, offset, NULL);
	if (memory == NULL) return (ERANGE);

	/* Issue unlock cycles */
	send_command(memory + amd_cmd1, AMD_UNLOCK_CMD1);
	send_command(memory + amd_cmd2, AMD_UNLOCK_CMD2);

	/* Issue read ident command */
	send_command(memory + amd_cmd1, AMD_AUTOSELECT);

	probe = virtual;

	/* while there are dbase entries */
	while (probe->struct_size) {
		/* Read jedecs */
		jedec_hi = probe->jedec_hi * flashcfg.device_mult;
		jedec_lo = probe->jedec_lo * flashcfg.device_mult;

		/* Compare jedecs */
		if ((jedec_hi == readmem(memory)) &&
		    (jedec_lo == readmem(memory + flashcfg.bus_width)))
		{
			*dbase = *probe;
			dbase->chip_inter = flashcfg.chip_inter;

			/* Fixed unit size, multiply it by interleave */
			dbase->geo_vect[0].unit_pow2 += (flashcfg.chip_inter >> 1);

			return (EOK);
		}

		/* Go to next dbase entry */
		probe++;
	}

	/* Flash type is not amd */
	return (ENOTSUP);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/fujitsu/f29f040_ident.c $ $Rev: 710521 $")
#endif
