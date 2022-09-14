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





#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <pthread.h>
#include <sys/f3s_mtd.h>

/*
 * Summary
 *
 * Method:      CFI only
 * MTD Version: 1 and 2
 * Bus Width:   8-bit and 16-bit
 * Boot-Block?: Yes
 * Note:        Legacy, don't know if it even works anymore.
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

int32_t f3s_hyCFI_ident(f3s_dbase_t *dbase,
                        f3s_access_t *access,
                        uint32_t flags,
                        uint32_t offset)
{
	int32_t			unit_size, geo_index, geo_pos, size, mxic = 0;
	volatile void	*memory;
	F3S_BASETYPE	jedec_hi, jedec_lo;
	F3S_BASETYPE	amd_mult, amd_cmd1, amd_cmd2;
	static f3s_dbase_t *probe;
	static f3s_dbase_t virtual[] =
	{
		{sizeof(f3s_dbase_t), 0xffff, 0xad, 0x22c4, "Hy29LV160T",
		0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{sizeof(f3s_dbase_t), 0xffff, 0xad, 0x2249, "Hy29LV160B",
		0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{sizeof(f3s_dbase_t), 0xffff, 0xad, 0xff, "CFI",
		0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{sizeof(f3s_dbase_t), 0xffff, 0xad, 0xffff, "CFI",
		0, 0, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 0, 0},

		{0, 0xffff, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	};

	/* check listing flag */

	if (flags & F3S_LIST_ALL)
	{
		/* check if first pass */
		if (!probe)
			probe = virtual;

		/* check if dbase is valid */
		if (!probe->struct_size)
			return ENOENT;

		*dbase = *probe;
		probe++;

		return EOK;
	}

	if(flashcfg.device_width == 1)
	{
		amd_mult = 2;
		amd_cmd1 = AMD_CMD_ADDR1_W8;
		amd_cmd2 = AMD_CMD_ADDR2_W8;
	}
	else
	{
		amd_mult = 1;
		amd_cmd1 = AMD_CMD_ADDR1_W16;
		amd_cmd2 = AMD_CMD_ADDR2_W16;
	}


	/* set necessary ident size */
	size = (amd_cmd1 + 1) * flashcfg.bus_width;

	/* set proper page on socket */
	memory = access->service->page(&access->socket, F3S_POWER_ALL, offset, &size);
	if (!memory) 
		return ERANGE;

	/* check if size is ok */
	if (size < (amd_cmd1 + 1) * flashcfg.bus_width) 
		return ENOTSUP;

	/* issue unlock cycles */
	send_command(memory + (amd_cmd1 * flashcfg.bus_width), AMD_UNLOCK_CMD1);
	send_command(memory + (amd_cmd2 * flashcfg.bus_width), AMD_UNLOCK_CMD2);

	/* issue read JEDEC command */
	send_command(memory + (amd_cmd1 * flashcfg.bus_width), AMD_AUTOSELECT);

	/*
	 * check jedec hi
	 * 0xad - Hynix
	 * 0xc2 - Macronix
	 */
	if (readmem(memory) != 0xad * flashcfg.device_mult &&
	    readmem(memory) != 0xc2 * flashcfg.device_mult)
		return ENOTSUP;

	if (readmem(memory) == 0xc2 * flashcfg.device_mult) {
		printf("MXIC flash chip. \n");
		mxic = 1;
	}

	/* read JEDEC id */
	jedec_hi = readmem(memory) / flashcfg.device_mult;
	jedec_lo = readmem(memory + flashcfg.bus_width) / flashcfg.device_mult;

	/* issue read CFI command */
	send_command(memory + ((AMD_CFI_ADDR * amd_mult)*flashcfg.bus_width), AMD_CFI_QUERY);

	/* check if "QRY" string is not present */

	if(verbose > 4)
	{
		printf("(devf  t%d::%s:%d) QRY string = %c%c%c\n",
						pthread_self(), __func__, __LINE__,
						(char)readmem(memory + (0x10 *amd_mult * flashcfg.bus_width)),
						(char)readmem(memory + (0x11 *amd_mult * flashcfg.bus_width)),
						(char)readmem(memory + (0x12 *amd_mult * flashcfg.bus_width)));
	}

	if (readmem(memory + (0x10 * amd_mult * flashcfg.bus_width)) != (F3S_BASETYPE)('Q' * flashcfg.device_mult) ||
		readmem(memory + (0x11 * amd_mult * flashcfg.bus_width)) != (F3S_BASETYPE)('R' * flashcfg.device_mult) ||
		readmem(memory + (0x12 * amd_mult * flashcfg.bus_width)) != (F3S_BASETYPE)('Y' * flashcfg.device_mult))
			return ENOTSUP;

	/* setup database with proper info */

	dbase->struct_size = sizeof(*dbase);

	dbase->jedec_hi = jedec_hi;
	dbase->jedec_lo = jedec_lo;

	dbase->name = "CFI";

	/* read buffer size information */
	dbase->buffer_size = (1 << (readmem(memory + ((0x2a *amd_mult)*flashcfg.bus_width)) / flashcfg.device_mult +
	      readmem(memory + ((0x2b *amd_mult)*flashcfg.bus_width)) / flashcfg.device_mult * 0x100)) * flashcfg.chip_inter;

	/* read number of geometries in vector */
	dbase->geo_num = readmem(memory + ((0x2c *amd_mult)*flashcfg.bus_width)) / flashcfg.device_mult;
	
	if(verbose > 3)
		printf("(devf  t%d::%s:%d) dbase->geo_num = %d\n",
				pthread_self(), __func__, __LINE__, dbase->geo_num);

	/* initialize geometry position */
	geo_pos = 0x2d;

	/* for all indexes in geometry vector */
	for (geo_index = 0; geo_index < dbase->geo_num; geo_index++)
	{
		/* read geometry information */
		dbase->geo_vect[geo_index].unit_num = ((readmem(memory + ((geo_pos *amd_mult)*flashcfg.bus_width)) / flashcfg.device_mult +
		     readmem(memory + (((geo_pos + 1) *amd_mult)*flashcfg.bus_width)) / flashcfg.device_mult * 0x100)) + 1;

		geo_pos += 2;

		unit_size = ((readmem(memory + ((geo_pos *amd_mult)*flashcfg.bus_width)) / flashcfg.device_mult +
			      readmem(memory+ ((geo_pos + 1) *amd_mult * flashcfg.bus_width)) / flashcfg.device_mult * 0x100)) * 0x100 * flashcfg.chip_inter;

		geo_pos += 2;

		/* find power of two of unit size */
		dbase->geo_vect[geo_index].unit_pow2 = 1;

		while (((1 << dbase->geo_vect[geo_index].unit_pow2) != unit_size) &&
		       (dbase->geo_vect[geo_index].unit_pow2 < 32))
			dbase->geo_vect[geo_index].unit_pow2++;

		if(verbose > 3)
		{
			printf("(devf  t%d::%s:%d) dbase->geo_vect[%d].unit_pow2 = %d\n",
						pthread_self(), __func__, __LINE__,
						geo_index,dbase->geo_vect[geo_index].unit_pow2);
			printf("(devf  t%d::%s:%d) dbase->geo_vect[%d].unit_num = %d\n",
						pthread_self(), __func__, __LINE__,
						geo_index,dbase->geo_vect[geo_index].unit_num);
		}
	}

	/* set bus and chip parameters */
	dbase->chip_inter = flashcfg.chip_inter;
	dbase->chip_width = flashcfg.bus_width;

	/* set capabilities flags */
	dbase->flags = F3S_ERASE_FOR_READ | F3S_ERASE_FOR_WRITE;
	if (mxic)
		dbase->flags |= 0x10000000; // hardcode for MXIC workaround

	/* flash type is Hynix CFI */
	return EOK;
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/hynix/hyCFI_ident.c $ $Rev: 710521 $")
#endif
