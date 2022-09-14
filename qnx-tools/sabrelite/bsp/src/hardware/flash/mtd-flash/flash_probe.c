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
** File: f3s_flash_probe.c
**
** Description:
**
** This file contains the array probe function for the f3s flash file system
**
** Ident: $Id: flash_probe.c 714684 2013-07-23 22:08:04Z jgao@qnx.com $
**
*/

/*
** Includes
*/

#include <pthread.h>
#include <sys/f3s_mtd.h>
#include <errno.h>

/* Globals */
void (*writemem)(volatile void *ptr, F3S_BASETYPE value);
F3S_BASETYPE (*readmem)(volatile void *ptr);
f3s_flashcfg_t  flashcfg = {0,0,0,0,0,0};
_uint32 amd_command_mask = 0;
int	suspended = 0;

/*
f3s_flash_probe()

This function is what is it says it is, a probe function.
Unless the configuration has already been set by the user the
function will loop through all idents (trying every configuration) 
passed in through the main function untill it finds one that matches.

This function use to automatically default to recognizing the device as
a ram device. I got rid of that functionallity, now if you want ram to 
be the default add it to the end of you flash[] array in the main function.
I personally want the driver to fail if it didn't recognize the device the
way I told it to.

*/

_int32 
f3s_flash_probe(f3s_access_t * access,
		f3s_flash_t ** flash_ptr,
		f3s_dbase_t * dbase)
{
	f3s_flash_v2_t **flash_ptr_v2 = (f3s_flash_v2_t **)flash_ptr;
	uint8_t		*flash_ptr_inc;
	_int32         error = 0, ident_size = 64, same, chip_size, geo_index,
	                chip_total = 0, text_offset = 0, unit_size;
	_uint8         ident_string[64], compare_string[64], *memory;
	f3s_dbase_t     dummy_dbase;
	unsigned bus_width, chip_inter;

	memory = access->service->page(&access->socket, F3S_POWER_VCC, 0, &ident_size);
	if (!memory) return -1;

	bus_width = flashcfg.bus_width;
	chip_inter = flashcfg.chip_inter;
	dbase->geo_num = 0;

	while ((*flash_ptr)->struct_size)
	{
		/* call curent flash mtd ident function */
		/* Auto detect */
		if (flashcfg.bus_width == 0)
		{
			for (flashcfg.device_width = 4;
			     flashcfg.device_width > 0;
			     flashcfg.device_width >>= 1)
			{ 
				if (verbose >= 2)
					printf("\n(devf  t%d::%s:%d) trying device width = %d\n\n",
								pthread_self(), __func__, __LINE__, flashcfg.device_width);

				for (flashcfg.chip_inter = 8 / flashcfg.device_width;
				     flashcfg.chip_inter > 0;
				     flashcfg.chip_inter >>= 1)
				{
					flashcfg.bus_width = flashcfg.chip_inter * flashcfg.device_width;
					flashcfg.cfi_width = flashcfg.bus_width;
					if (verbose >= 2)
					{
						printf("(devf  t%d::%s:%d) bus width  = %d, interleave = %d\n",
									pthread_self(), __func__, __LINE__,
									flashcfg.bus_width, flashcfg.chip_inter);
					}
					flashcfg_setup();
					error = (*flash_ptr)->ident(dbase, access, 0, 0);
					if (error == EOK) break;
				}
				if (error == EOK) break;
			}
		}
		else /* user has specified flash configuration */
		{
			if (!flashcfg.chip_inter) flashcfg.chip_inter = 1;
			flashcfg.device_width = flashcfg.bus_width / flashcfg.chip_inter;
			flashcfg_setup();
			error = (*flash_ptr)->ident(dbase, access, 0, 0);
		}

		if (!error)
		{
			/* find the largest unit size */
			unit_size = 0;
			for (geo_index = 0; geo_index < dbase->geo_num; geo_index++)
				unit_size = max(unit_size, 1 << dbase->geo_vect[geo_index].unit_pow2);

			/* check if socket requires a unit size */
			if (access->socket.unit_size)
			{
				/* scale each geometry up */
				while ((dbase->geo_vect[0].unit_pow2 < 32) && (unit_size != access->socket.unit_size))
				{
					unit_size <<= 1;
					for (geo_index = 0; geo_index < dbase->geo_num; geo_index++)
					{
						/* XXX - This doesn't scale unit_num down! */
						/*
						 * XXX - If we can't trust the probe, why are we
						 *       modifying it?
						 */
						dbase->geo_vect[geo_index].unit_pow2++;
					}
				}

				if (dbase->geo_vect[0].unit_pow2 == 32)
				{
					errno = ENOTSUP;
					return -1;
				}
			} else {
				access->socket.unit_size = unit_size;
			}

			/* calculate chip ident string size */
			ident_size = 2 * dbase->chip_width * dbase->chip_inter;

			/* check if flash has a read function */
			if (((*flash_ptr)->struct_size == sizeof (f3s_flash_t)) &&
			    ((*flash_ptr)->read))
			{
				/* read ident string from first chip */
				ident_size = (*flash_ptr)->read(dbase, access, 0, 0, ident_size, ident_string);
				if (ident_size < 0) return -1;
			}
			else if (((*flash_ptr_v2)->struct_size == sizeof (f3s_flash_v2_t)) &&
			         ((*flash_ptr_v2)->v2read))
			{
				/* read ident string from next chip */
				ident_size = (*flash_ptr_v2)->v2read(dbase, access, 0, text_offset,
				                                     ident_size, ident_string);

				/* check if ident size makes no sense */
				if (ident_size < 0) break;
			}
			else
			{
				/* set proper page on socket */
				memory = access->service->page(&access->socket, F3S_POWER_VCC, 0, &ident_size);
				if (!memory) return -1;

				/* copy memory into buffer */
				memcpy(ident_string, (void *)memory, ident_size);
			}
			memory = access->service->page(&access->socket, F3S_POWER_VCC, 0, &ident_size);
			if (!memory) return -1;
			(*flash_ptr)->reset(dbase, access, 0, 0);

			chip_size = 0;

			/* for all geometries in vector */
			for (geo_index = 0; geo_index < dbase->geo_num; geo_index++)
				chip_size += dbase->geo_vect[geo_index].unit_num <<
					dbase->geo_vect[geo_index].unit_pow2;

			/* find size of array loop */
			while (1)
			{
				chip_total++;
				text_offset += chip_size;

				/* check if flash has a read function */
				if (((*flash_ptr)->struct_size == sizeof (f3s_flash_t)) &&
				    ((*flash_ptr)->read))
				{
					/* read ident string from next chip */
					ident_size = (*flash_ptr)->read(dbase, access, 0, text_offset,
					                                ident_size, compare_string);

					/* check if ident size makes no sense */
					if (ident_size < 0) break;
				}
				else if (((*flash_ptr_v2)->struct_size == sizeof (f3s_flash_v2_t)) &&
				         ((*flash_ptr_v2)->v2read))
				{
					/* read ident string from next chip */
					ident_size = (*flash_ptr_v2)->v2read(dbase, access, 0, text_offset,
					                                     ident_size, compare_string);

					/* check if ident size makes no sense */
					if (ident_size < 0) break;
				}
				else
				{
					/* set proper page on socket */
					memory = access->service->page(&access->socket, F3S_POWER_VCC,
					                               text_offset, &ident_size);
					if (!memory) break;

					/* copy memory into buffer */
					memcpy(compare_string, (void *)memory, ident_size);
				}
				same = !memcmp(ident_string, compare_string, ident_size);
				if (same) break;

				/* call flash mtd ident function */
				error = (*flash_ptr)->ident(&dummy_dbase, access, 0, text_offset);
				if (error) break;

				(*flash_ptr)->reset(dbase, access, 0, text_offset);
			}

			memory = access->service->page(&access->socket, F3S_POWER_VCC, 0, &ident_size);
			if (!memory) return -1;
			(*flash_ptr)->reset(dbase, access, 0, 0);

			if(verbose >= 2)
			{
				printf("(devf  t%d::%s:%d) chip total = %d, bus_width = %d, interleave = %d\n",
							pthread_self(), __func__, __LINE__,
							chip_total, flashcfg.bus_width, flashcfg.chip_inter);
			}
			/* Stuff the dbase structure with the actual values of the chip */
			dbase->chip_inter = flashcfg.chip_inter;
			dbase->chip_width = (flashcfg.bus_width / flashcfg.chip_inter);

			return chip_total;
		}

		/* reset variables so next ident works properly */	
		flashcfg.bus_width = bus_width;
		flashcfg.device_width = 0;		
		flashcfg.chip_inter = chip_inter;	

		/* go to next flash mtd */
		flash_ptr_inc = (uint8_t*)*flash_ptr;
		flash_ptr_inc += (*flash_ptr)->struct_size;
		(*flash_ptr) = (f3s_flash_t*)flash_ptr_inc;
	}

	fprintf(stderr,"(devf  t%d::%s:%d) Unable to properly identify any flash devices\n",
				pthread_self(), __func__, __LINE__);
	exit(EXIT_FAILURE); 

	return -1;
}


/*
flashcfg_setup()

This function is used to setup the global variable flashcfg, which
is a struct that contians the bus width, device width and interleave
of the chip. It also contians the multiplier variable which is a variable
you multiply all commands going to flash by so that interleaved chips all
get the commands correctly.

The other variable setup in here is amd_command_mask, which is only used
in amd writes. I really din't want to figure it out dynamically each time
I entered the write, so I just jammed it in here.

*/

void flashcfg_setup()
{
	/* Figure out read and write functions to use */
	if (flashcfg.bus_width == 1)
	{
		readmem = readmem8;
		writemem = writemem8;
	}
	if (flashcfg.bus_width == 2)
	{
		readmem = readmem16;
		writemem = writemem16;
	}
	if (flashcfg.bus_width == 4)
	{
		readmem = readmem32;
		writemem = writemem32;
	}
	if (flashcfg.bus_width == 8)
	{
		readmem = readmem64;
		writemem = writemem64;
	}
	
	/* Figure out Multiplier */
	if(flashcfg.device_width == 1)
	{
		if(flashcfg.chip_inter == 1)
			flashcfg.device_mult = 0x01;
		if(flashcfg.chip_inter == 2)
			flashcfg.device_mult = 0x0101;
		if(flashcfg.chip_inter == 4)
			flashcfg.device_mult = 0x01010101;
		if(flashcfg.chip_inter == 8)
			flashcfg.device_mult = 0x0101010101010101ULL; 
	}
	else if(flashcfg.device_width == 2)
	{
		if(flashcfg.chip_inter == 1)
			flashcfg.device_mult = 0x0001;
		if(flashcfg.chip_inter == 2)
			flashcfg.device_mult = 0x00010001;
		if(flashcfg.chip_inter == 4)
			flashcfg.device_mult = 0x0001000100010001ULL; 
	}
	else if(flashcfg.device_width == 4)
	{
		if(flashcfg.chip_inter == 1)
			flashcfg.device_mult = 0x00000001;
		if(flashcfg.chip_inter == 2)
			flashcfg.device_mult = 0x0000000100000001ULL;
	}


	/* Figure out command mask incase we are using AMD */
	if ((flashcfg.chip_inter * flashcfg.device_width) == 1)
		amd_command_mask = 0xfffff800;
	else if ((flashcfg.chip_inter * flashcfg.device_width) == 2)
		amd_command_mask = 0xfffff000;
	else if ((flashcfg.chip_inter * flashcfg.device_width) == 4)
		amd_command_mask = 0xffffe000;
	else if ((flashcfg.chip_inter * flashcfg.device_width) == 8)
		amd_command_mask = 0xffffc000;
	else
		fprintf(stderr, "(devf  t%d::%s:%d) unknown command mask\n",
					pthread_self(), __func__, __LINE__);

	if (flashcfg.device_width == 1)
			amd_command_mask = amd_command_mask << 1;

}

/*
set_flash_config()

This function is used so that when you enter f3s_flash_probe(),
you don't try and auto-identify the flash device's configuration
(i.e. don't loop around calling the ident functions a bunch of
times).

The loop around shouldn't be harmfull and is handy for boards that
that have more then one flash configuration, but theoretically
sending commands down to flash untill you recognize it could cause
something to go wrong. Hence this function. Remember bus_width is in 
bytes, not bits.  

Just add it to your main before you call f3s_start(), if the function 
fails because you sent it an invalid config, it will leave the values set 
to zero and the auto-ident will run.

*/

int set_flash_config(unsigned bus_width, unsigned chip_interleave)
{
	if((bus_width != 1)&&(bus_width != 2)&&(bus_width != 4)&&(bus_width != 8))
	{
		if(verbose)
			fprintf(stderr,"(devf  t%d::%s:%d) invalid user defined bus width %d\n",
						pthread_self(), __func__, __LINE__, bus_width);
		return(-1);
	}

	if((chip_interleave != 1)&&(chip_interleave != 2)&&(chip_interleave != 4)&&(chip_interleave != 8))
	{
		if(verbose)
			fprintf(stderr,"(devf  t%d::%s:%d) invalid user defined chip interleave %d\n",
						pthread_self(), __func__, __LINE__, chip_interleave);
		return(-1);
	}

	flashcfg.bus_width = bus_width;
	flashcfg.chip_inter = chip_interleave;
	flashcfg.device_width = bus_width / chip_interleave;

	return(1);
}

const char *f3s_mtd_buildInfo(void)
{
	static const char buildInfo[] = "MTD Build: " __DATE__", " __TIME__;
	return(buildInfo);
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/flash_probe.c $ $Rev: 714684 $")
#endif
