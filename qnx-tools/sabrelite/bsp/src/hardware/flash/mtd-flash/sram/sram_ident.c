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
 * MTD Version: 1 and 2
 * Bus Width:   All
 *
 * Description
 *
 * This is the only ident callout for RAM / SRAM. The extra code simulates a
 * faulty NOR flash part for testing purposes.
 */

uint32_t fail_flag, fail_type, fail_count, fail_action;
uint32_t erase_count, write_count;

#ifndef NDEBUG

static uint32_t f3s_option_parse(char **string)
{
	int base;
	uint32_t result;

	/* check base of string */
	if (**string=='0')
	{
		/* increment string pointer */
		(*string)++;

		/* base could be 8 */
		base=8;

		/* check if base is 16 */
		if (**string=='x' || **string=='X')
		{
			/* increment string pointer */
			(*string)++;

			/* base is 16 */
			base=16;
		}
	}
	else
	{
		/* base is 10 */
		base=10;
	}

	/* read number */
	result=strtol(*string, string, base);

	/* check multiplyer */
	if ((**string=='b') || (**string=='B'))
	{
		(*string)++;
	}
	else if ((**string=='k') || (**string=='K'))
	{
		(*string)++;

		result*=1024;
	}
	else if ((**string=='m') || (**string=='M'))
	{
		(*string)++;

		result*=1024*1024;
	}
	else if ((**string=='g') || (**string=='G'))
	{
		(*string)++;

		result*=1024*1024*1024;
	}

	/* check for comma separator */
	if (**string==',')
	{
		(*string)++;
	}

	/* return result */
	return result;
}

#endif

int32_t f3s_sram_ident(f3s_dbase_t *dbase,
                       f3s_access_t *access,
                       uint32_t flags,
                       uint32_t offset)
{
	F3S_SRAM_TYPE *memory;
	int32_t size=sizeof(F3S_SRAM_TYPE);
	uint32_t temp, temp2;
	static f3s_dbase_t *probe;
	static f3s_dbase_t virtual[]=
	{
		{sizeof(f3s_dbase_t), 0xffff, 0x00, 0x00, "SRAM",
		 0, 1, 1, 10000, 1000000000, 50000, 50000, 1, 0, 0, 0, 0, 1, {{1, 16}}},
		{0, 0xffff, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	};

#ifndef NDEBUG
	char *flash_fail;
#endif

	/* check listing flag */
	if (flags & F3S_LIST_ALL)
	{
		/* check if first pass */
		if (!probe) probe = virtual;
		if (!probe->struct_size) return (ENOENT);

		*dbase = *probe;
		probe++;
		return (EOK);
	}

	/* set proper page on socket */
	memory = (F3S_SRAM_TYPE *)access->service->page(&access->socket, F3S_POWER_VCC, offset, &size);
	if (!memory) return (ERANGE);

	/* write something to sram */
	temp = *memory;
	*memory = 0x55u * F3S_SRAM_MULT;

	/* check if memory was affected */
	if (*memory == (0x55u * F3S_SRAM_MULT))
	{
		/* write something else to sram */
		*memory = 0xaau * F3S_SRAM_MULT;

		/* check if memory was affected */
		if (*memory == (0xaau * F3S_SRAM_MULT))
		{
			/* restore RAM contents */
			*memory = temp;

			/* copy virtual database entry */
			*dbase=virtual[0];

#ifndef NDEBUG
			/* check if there is a flash fail environment variable */
			flash_fail = getenv("FLASH_FAIL");

			if (flash_fail)
			{
				/* set fail flag */
				fail_flag=1;

				/* parse the option strings */
				fail_type   = f3s_option_parse(&flash_fail);
				fail_count  = f3s_option_parse(&flash_fail);
				fail_action = f3s_option_parse(&flash_fail);

			} else
			{
				/* clear fail flag */
				fail_flag = 0;
			}

#endif

			/* figure out the unit_size */
			if (access->socket.unit_size == 0)
			{
				/* default to 64K if possible */
				if (access->socket.array_size >= 64 * 1024)
				{
					access->socket.unit_size = 64 * 1024;
					temp = 16;

				} else
				{
					/* find the largest unit size if total < 64K */
					for (temp = 0; temp < 32; temp++)
					{
						if (access->socket.array_size & (0x80000000 >> temp)) break;
					}
					access->socket.unit_size = 0x80000000 >> temp;
					temp = 31 - temp;
				}

			} else
			{
				for (temp = 0; temp < 32; temp++)
				{
					if (access->socket.unit_size & (1 << temp)) break;
				}
			}
			dbase->geo_vect[0].unit_pow2 = temp;

			/* figure out a power of 2 chip size */
			temp2 = access->socket.array_size >> temp;
			for (temp = 0; temp < 32; temp++)
			{
				if (temp2 & (1 << temp)) break;
			}
			dbase->geo_vect[0].unit_num = 1 << temp;

			return (EOK);
		}
	}

	return (ENOTSUP);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/sram/sram_ident.c $ $Rev: 710521 $")
#endif
