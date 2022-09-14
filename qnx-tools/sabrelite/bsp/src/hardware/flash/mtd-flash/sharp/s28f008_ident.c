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
 * Bus Width:   8-bit and 16-bit
 * Boot-Block?: No
 *
 * Description
 *
 * This ident callout uses the "JEDEC" ID code to recognize chips with
 * uniform block sizes.
 *
 * Use this for really old legacy flash chips. If the ID for your chip is not
 * listed, copy this file into your driver's source directory and add an entry
 * for your chip.
 */

int32_t f3s_s28f008_ident(f3s_dbase_t *dbase,
                          f3s_access_t *access,
                          uint32_t flags,
                          uint32_t offset)
{
	volatile void *	memory;
	F3S_BASETYPE	jedec_hi, jedec_lo;

	static f3s_dbase_t *probe;
	static f3s_dbase_t virtual[] =
	{
		{sizeof(f3s_dbase_t), 0xffff, 0xb0, 0x88, "S28F016SU",
			F3S_ERASE_FOR_READ,
			1, 1, 10000, 1000000000, 5000, 5000,
		1, 1000000, 0, 0, 0, 1, {{32, 16}}},

		{sizeof(f3s_dbase_t), 0xffff, 0xb0, 0x6688, "S28F016SU",
			F3S_ERASE_FOR_READ,
			1, 1, 10000, 1000000000, 5000, 5000,
		1, 1000000, 0, 0, 0, 1, {{32, 16}}},

		{0, 0xffff, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	};

	/* check listing flag */
	if (flags & F3S_LIST_ALL)
	{
		/* check if first pass */
		if (!probe) probe = virtual;

		/* check if dbase is valid */
		if (!probe->struct_size) return ENOENT;

		*dbase = *probe;
		probe++;

		return EOK;
	}

	/* set proper page on socket */
	memory = access->service->page(&access->socket, F3S_POWER_ALL, offset, NULL);
	if (!memory) return ERANGE;

	/* issue read ident command */
	send_command(memory,INTEL_READ_IDENT);

	/* set probe pointer to first dbase entry */
	probe = virtual;

	/* while there are dbase entries */
	while (probe->struct_size)
	{
		jedec_hi = probe->jedec_hi * flashcfg.device_mult;
		jedec_lo = probe->jedec_lo * flashcfg.device_mult;

		/* compare jedecs */
		if (jedec_hi == readmem(memory) && jedec_lo == readmem(memory + flashcfg.bus_width))
		{
			/* found proper database entry */
			*dbase = *probe;

			/* set chip interleave */
			dbase->chip_inter = flashcfg.chip_inter;

			/* fix unit size, multiply it by interleave */
			dbase->geo_vect[0].unit_pow2 += (flashcfg.chip_inter >> 1);

			/* return dbase entry found */
			return EOK;
		}

		/* got to next dbase entry */
		probe++;
	}

	/* flash type is not intel */
	return ENOTSUP;
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/sharp/s28f008_ident.c $ $Rev: 710521 $")
#endif
