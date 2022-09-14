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


/*
** File: f3s_flash_list.c
**
** Description:
**
** This file contains the flash list function for the f3s flash file system
**
** Ident: $Id: flash_list.c 710521 2013-06-17 17:57:28Z targentina@qnx.com $
**
*/

/*
** Includes
*/

#include <sys/f3s_mtd.h>
/*
** Function: f3s_flash_list
*/

void 
f3s_flash_list(f3s_flash_t * flash_vect)
{
	_int32         error;
	f3s_dbase_t     dbase;
	static const f3s_flash_t sram =
	{
		sizeof(f3s_flash_t),
		f3s_sram_ident,
		NULL,
		NULL,
		f3s_sram_write,
		f3s_sram_erase,
		NULL,
		NULL,
		f3s_sram_sync
	};
	static const f3s_flash_t rom =
	{
		sizeof(f3s_flash_t),
		f3s_rom_ident,
		NULL,
		NULL,
		f3s_rom_write,
		f3s_rom_erase,
		NULL,
		NULL,
		f3s_rom_sync
	};

	/* list the sram mtd */

	error = sram.ident(&dbase, NULL, F3S_LIST_ALL, 0);

	/* display dbase info */

	printf("%s\n", dbase.name);

	/* check all flash mtds loop */

	while (flash_vect->struct_size)
	{

		/* check all available dbases */

		for (;;)
		{

			/* call curent flash mtd ident function */

			error = flash_vect->ident(&dbase, NULL, F3S_LIST_ALL, 0);

			if (error)
				break;

			/* display dbase name and jedec */

			printf("%-14s\n", dbase.name);
		}

		/* go to next flash mtd */

		flash_vect = (f3s_flash_t *)(((uint8_t *)flash_vect) + flash_vect->struct_size);
	}

	/* list the rom mtd */

	error = rom.ident(&dbase, NULL, F3S_LIST_ALL, 0);

	/* display dbase info */

	printf("%s\n", dbase.name);

	/* everything went fine */

	exit(EOK);
}

/*
** End
*/



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/flash_list.c $ $Rev: 710521 $")
#endif
