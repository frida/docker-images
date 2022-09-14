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
 * Bus Width:   8-bit, 16-bit and 8/16-bit hybrid
 * Boot-Block?: Yes
 * Note:        Try this first.
 *
 * Description
 *
 * This ident callout uses the Common Flash Interface (CFI) to recognize
 * chips. It supports both uniform and non-uniform block sizes (aka
 * boot-block flash).
 *
 * Use this callout for all modern CFI flash chips. CFI is the preferred
 * method for identifying flash chips.
 */

int32_t f3s_iCFI_ident(f3s_dbase_t *dbase,
                       f3s_access_t *access,
                       uint32_t flags,
                       uint32_t offset)
{
	uint32_t        unit_size, geo_index, geo_pos, geo_num;
	F3S_BASETYPE	temp_mult;
	static f3s_dbase_t *probe;
	volatile void  *memory;
	F3S_BASETYPE        device_size, tmp;
	static f3s_dbase_t virtual[] =
	{
		{sizeof(f3s_dbase_t), 0xffff, 0x89, 0xd0, "28F160S5",
		0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{sizeof(f3s_dbase_t), 0xffff, 0x89, 0xd4, "28F320S5",
		0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{sizeof(f3s_dbase_t), 0xffff, 0x89, 0x14, "28F320J5",
		0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{sizeof(f3s_dbase_t), 0xffff, 0x89, 0x15, "28F640J5",
		0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{sizeof(f3s_dbase_t), 0xffff, 0x89, 0x18, "28F128J3A",
      	0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{sizeof(f3s_dbase_t), 0xffff, 0x89, 0x88c0, "28F800C3-T",
		0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{sizeof(f3s_dbase_t), 0xffff, 0x89, 0x88c1, "28F800C3-B",
		0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{sizeof(f3s_dbase_t), 0xffff, 0x89, 0x88c2, "28F160C3-T",
		0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{sizeof(f3s_dbase_t), 0xffff, 0x89, 0x88c3, "28F160C3-B",
		0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{sizeof(f3s_dbase_t), 0xffff, 0x89, 0x88c4, "28F320C3-T",
		0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{sizeof(f3s_dbase_t), 0xffff, 0x89, 0x88c5, "28F320C3-B",
		0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{sizeof(f3s_dbase_t), 0xffff, 0x89, 0x88cc, "28F640C3-T",
		0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{sizeof(f3s_dbase_t), 0xffff, 0x89, 0x88cd, "28F640C3-B",
		0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{sizeof(f3s_dbase_t), 0xffff, 0x89, 0xff, "CFI",
		0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{sizeof(f3s_dbase_t), 0xffff, 0x89, 0xffff, "CFI",
		0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{0, 0xffff, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	};

	if (flashcfg.device_width == 2)
		device_size = 0xffff;
	else
		device_size = 0xff;

	if (flags & F3S_LIST_ALL)
	{
		/* check if first pass */
		if (!probe) probe = virtual;

		if (!probe->struct_size) return (ENOENT);

		*dbase = *probe;
		probe++;

		return (EOK);
	}
	memory = access->service->page(&access->socket, F3S_POWER_ALL, offset, NULL);
	if (memory == NULL) return (ERANGE);

	/* issue read query command */
	send_command(memory, INTEL_READ_QUERY);

	switch (flashcfg.bus_width)
	{
		case 1:
			temp_mult = 0x01;
			break;
		case 2:
			temp_mult = 0x0101;
			break;
		case 4:
			temp_mult = 0x01010101;
			break;
		case 8:
			temp_mult = 0x0101010101010101ULL;
			break;
		default:
			return ENOTSUP;
	}

	/* Detect funky x8/x16 hybrid mode */
	if ((readmem(memory + (0x20 * flashcfg.bus_width)) == (F3S_BASETYPE)('Q'  * temp_mult)) &&
	    (readmem(memory + (0x50 * flashcfg.bus_width)) == (F3S_BASETYPE)(0x02 * temp_mult)) &&
	    (readmem(memory + (0x52 * flashcfg.bus_width)) == (F3S_BASETYPE)(0x00 * temp_mult)))
	{
		flashcfg.cfi_width = flashcfg.bus_width * 2;
	}
	/* Detect pure x8 or pure x16 mode */
	else if (readmem(memory + (0x10 * flashcfg.bus_width)) == (F3S_BASETYPE)('Q'  * flashcfg.device_mult))
	{
		/* If we think the chip is pure x8, but CFI says it's a funky x8/x16 */
		if ((flashcfg.device_width == 1) &&
		    (readmem(memory + (0x28 * flashcfg.bus_width)) == (F3S_BASETYPE)(0x02 * temp_mult)) &&
		    (readmem(memory + (0x29 * flashcfg.bus_width)) == (F3S_BASETYPE)(0x00 * temp_mult)))
		{
			/* A x8/x16 chip cannot have a pure x8 mode */
			return ENOTSUP;
		}

		flashcfg.cfi_width = flashcfg.bus_width * 1;
	}
	else
	{
		return ENOTSUP;
	}

	if ((readmem(memory) & 0xff) != 0x89 && (readmem(memory) & 0xff) != 0xb0 && 
	    (readmem(memory) & 0xff) != 0x2c && (readmem(memory) & 0xff) != 0x20)
		return ENOTSUP;

	if(verbose > 4)
	{
		printf("(devf  t%d::%s:%d) query string = %c %c %c\n",
				pthread_self(), __func__, __LINE__,
				(char)readmem(memory + (0x10 * flashcfg.cfi_width)),
				(char)readmem(memory + (0x11 * flashcfg.cfi_width)),
				(char)readmem(memory + (0x12 * flashcfg.cfi_width)));
	}

	/* check if "QRY" string is not present */
	if ((readmem(memory + (0x10 * flashcfg.cfi_width)) != (F3S_BASETYPE)('Q' * flashcfg.device_mult)) ||
	    (readmem(memory + (0x11 * flashcfg.cfi_width)) != (F3S_BASETYPE)('R' * flashcfg.device_mult)) ||
	    (readmem(memory + (0x12 * flashcfg.cfi_width)) != (F3S_BASETYPE)('Y' * flashcfg.device_mult)))
	{
		return ENOTSUP;
	}

	/* For Numonyx parts, check that primary algorithm command set is not AMD (0x02).
	 * Note: Numonyx parts can have Numonyx (0x20) or Intel (0x89) manufacturer IDs
	 */
	if ((readmem(memory) & 0xff) == 0x20 || (readmem(memory) & 0xff) == 0x89)
	{
		if (readmem(memory + (0x13 * flashcfg.cfi_width)) == (F3S_BASETYPE)(0x02 * flashcfg.device_mult))
		{
			if(verbose > 2)
			{
				printf("(devf  t%d::%s:%d) cmd set = 0x%llx != 0x%llx (Intel), width %d device_mult 0x%llx\n",
						pthread_self(), __func__, __LINE__,
						readmem(memory + (0x13 * flashcfg.cfi_width)),
						(F3S_BASETYPE)(0x03 * flashcfg.device_mult),
						flashcfg.cfi_width, flashcfg.device_mult);
			}
			return (ENOTSUP);
		}
	}

	dbase->struct_size = sizeof(*dbase);
	dbase->jedec_hi = readmem(memory                     ) & device_size;
	dbase->jedec_lo = readmem(memory + flashcfg.bus_width) & device_size;
	dbase->name = "CFI";

	tmp  = (readmem(memory + (0x2a * flashcfg.cfi_width)) & device_size);
	tmp += (readmem(memory + (0x2b * flashcfg.cfi_width)) & device_size) * 100;
	//2 to the power of tmp
	dbase->buffer_size  = 1 << tmp;
	dbase->buffer_size *= flashcfg.chip_inter;
	geo_num = readmem(memory + (0x2c * flashcfg.cfi_width)) & device_size;
	geo_pos = 0x2d;

	/* for all indexes in geometry vector */
	for (geo_index = dbase->geo_num; geo_index < geo_num; geo_index++)
	{
		tmp =  (readmem(memory + ((geo_pos + 0) * flashcfg.cfi_width)) & device_size);
		tmp += (readmem(memory + ((geo_pos + 1) * flashcfg.cfi_width)) & device_size) * 0x100;
		tmp += 1;
		dbase->geo_vect[geo_index].unit_num = tmp;
		geo_pos += 2;

		tmp =  (readmem(memory + ((geo_pos + 0) * flashcfg.cfi_width)) & device_size);
		tmp += (readmem(memory + ((geo_pos + 1) * flashcfg.cfi_width)) & device_size) * 0x100;
		unit_size = tmp * 0x100 * flashcfg.chip_inter;
		geo_pos += 2;

		/* find power of two of unit size */
		dbase->geo_vect[geo_index].unit_pow2 = 1;

		while (((1 << dbase->geo_vect[geo_index].unit_pow2) != unit_size) && (dbase->geo_vect[geo_index].unit_pow2 < 32))
			dbase->geo_vect[geo_index].unit_pow2++;
	}
	dbase->geo_num += geo_num;

	dbase->chip_inter = flashcfg.chip_inter;
	dbase->chip_width = flashcfg.bus_width;
	dbase->flags = F3S_ERASE_FOR_READ | F3S_ERASE_FOR_WRITE;
	if (!(dbase->buffer_size==1)) dbase->flags |= F3S_WRITE_BUFFER; /* if buffer_size>1 then supporting buffer write */

	return EOK;
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/intel/iCFI_ident.c $ $Rev: 710521 $")
#endif
