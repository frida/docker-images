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
 * MTD Version:     2 only
 * Bus Width:       8-bit, 16-bit and 8/16-bit hybrid
 * Buffered Writes: No
 *
 * Description
 *
 * This is the only MTDv2 write callout for non-buffered writes.
 */

int32_t f3s_i28f008_v2write(f3s_dbase_t *dbase,
                          f3s_access_t *access,
                          uint32_t flags,
                          uint32_t offset,
                          int32_t size,
                          uint8_t *buffer)
{
	volatile void *	memory;
	intunion_t		value;
	F3S_BASETYPE	pad;
	F3S_BASETYPE	shift;
	int32_t			left;

	/* Initialize number of bytes left to write */
	left = size;

	/* Allign memory address */
	pad = offset & (flashcfg.bus_width - 1);

	offset -= pad;
	buffer -= pad;

	left += pad;

	memory = access->service->page(&access->socket, F3S_POWER_ALL, offset, &left);
	if (!memory)
	{
		fprintf(stderr, "(devf  t%d::%s:%d) page() returned NULL for offset 0x%x\n",
					pthread_self(), __func__, __LINE__, offset);
		return (-1);
	}

	size = left - pad;

	memcpy((void *)&value, buffer, min(flashcfg.bus_width, left));

	if (pad)
	{
		shift = (flashcfg.bus_width - pad) << 3;
		pad_value(&value, memory, shift,1);
	}

	while (left > 0)
	{
		if (left < flashcfg.bus_width)
		{
			/* Calculate padding shift */
			shift = left << 3;
			pad_value(&value, memory, shift,0);
		}

		if (value.w64 != F3S_I28F008_ERASE)
		{
			/* issue write command */
			send_command(memory, INTEL_WRITE);

			/* write data */
			write_value(memory, &value);

			/* poll for write completion */
			if(intel_poll(memory, F3S_I28F008_POLL) == -1)
			{
				fprintf(stderr,"(devf  t%d::%s:%d) over poll waiting for write completion\n",
							pthread_self(), __func__, __LINE__);
				errno = EIO;
				return (-1);
			}
		}

		left -= flashcfg.bus_width;

		if (left > 0)
		{
			buffer += flashcfg.bus_width;
			memory += flashcfg.bus_width;
			memcpy((void *)&value, buffer, min(flashcfg.bus_width, left));

			if (left < flashcfg.bus_width)
			{
				send_command(memory, INTEL_READ_ARRAY);
			}
		}
	}

	/* Check for errors */
	send_command(memory, INTEL_READ_STATUS);
	if (intel_read_status(readmem(memory)) == -1) return (-1);
	send_command(memory, INTEL_READ_ARRAY);

	if (flags & F3S_VERIFY_WRITE)
	{	/* check if everything was written properly */
		if (memcmp(buffer - size + left + flashcfg.bus_width,
			   (const void *)(((uint8_t *)memory) - size + left + flashcfg.bus_width), size))
		{
			fprintf(stderr, "(devf  t%d::%s:%d) program verify error\n"
						"between  0x%p and 0x%p\n"
						"memory = 0x%p, offset =  0x%x, left = %d, size = %d, bus_width = %d\n",
						pthread_self(), __func__, __LINE__,
						(((uint8_t *) memory) - size + left + flashcfg.bus_width),
						(((uint8_t *) memory) + left + flashcfg.bus_width),
						memory, offset, left, size, flashcfg.bus_width);
			errno = EIO;
			return (-1);
		}
	}

	return (size);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/intel/i28f008_v2write.c $ $Rev: 710521 $")
#endif
