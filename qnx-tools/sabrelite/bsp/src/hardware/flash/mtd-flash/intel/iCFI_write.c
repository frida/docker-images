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
 * MTD Version:     1 only
 * Bus Width:       8-bit, 16-bit and 8/16-bit hybrid
 * Buffered Writes: Yes
 *
 * Description
 *
 * This is the only MTDv1 write callout for buffered writes.
 */

int32_t f3s_iCFI_write(f3s_dbase_t *dbase,
                       f3s_access_t *access,
                       uint32_t flags,
                       uint32_t offset,
                       int32_t size,
                       uint8_t *buffer)
{
	int32_t         left, buffer_size, buffer_count, pad, shift;
	uint32_t		poll;
	volatile void  *memory;
	intunion_t 		value;

	/* initialize bytes left to write */
	left = size;

	/* allign memory address */
	pad = offset & (flashcfg.bus_width - 1);

	offset -= pad;
	buffer -= pad;

	/* adjust left with padding */
	left += pad;

	/* set proper page on socket */

	memory = access->service->page(&access->socket, F3S_POWER_ALL, offset, &left);
	if (!memory)
	{
		fprintf(stderr, "(devf  t%d::%s:%d) page() returned NULL for offset 0x%x\n",
					pthread_self(), __func__, __LINE__, offset);
		return -1;
	}

	/* initialize number of bytes writeable */
	size = left - pad;

	/* get value to write from buffer */
	memcpy((void *) (pad + (char *) &value), buffer + pad, min(flashcfg.bus_width, left) - pad);

	/* check if padding is neccessary */

	if (pad)
	{
		shift = (flashcfg.bus_width - pad) << 3;
		pad_value(&value, memory, shift,1);
	}

	/* check if write to buffer is possible */
	if (!(flags & F3S_ERASE_FOR_WRITE) && left >= (4 * flashcfg.bus_width))
	{
		/* while there are bytes left to write loop */
		while (left >= flashcfg.bus_width)
		{
			/* 
			 * Intel specs say we have to keep issuing INTEL_WRITE_BUFFER to
			 * poll for when the write buffer is available. So, we can't use
			 * intel_poll() for this.
			 */
			poll = F3S_I28F008_POLL;
			do
			{
				/* Prepare for buffered write */
				send_command(memory, INTEL_WRITE_BUFFER);
				if (poll == 0)
				{
					fprintf(stderr,"(devf  t%d::%s:%d) over poll waiting for write buffer\n",
								pthread_self(), __func__, __LINE__);
					return(-1);
				}
				poll--;
			} while ((readmem(memory) & (0x80 * flashcfg.device_mult)) != (0x80 * flashcfg.device_mult));

			buffer_size = min(dbase->buffer_size, left & ~(flashcfg.bus_width - 1));

			/* try to align buffer with page for best performance */
			buffer_size = min(buffer_size, dbase->buffer_size -
				       (offset & (dbase->buffer_size - 1)));
			left -= buffer_size;
			offset += buffer_size;

			/* calculate buffer count */
			buffer_count = buffer_size / flashcfg.bus_width;

			/* write buffer size to flash */
			send_command(memory, buffer_count - 1);

			/* while there is something for buffer */
			while (buffer_count > 0)
			{
				/* write data */
				write_value(memory, &value);

				buffer_count--;

				/* check if there is still data to write */
				if (buffer_count > 0 || left >= flashcfg.bus_width)
				{
					buffer += flashcfg.bus_width;
					memory += flashcfg.bus_width;

					/* get value to write from buffer */
					memcpy((void *) &value, buffer, flashcfg.bus_width);
				}
			}

			/* issue write confirm command */
			send_command(memory, INTEL_BUFFER_CONFIRM);
			if(intel_poll(memory,F3S_I28F008_POLL) == -1)
			{
				fprintf(stderr,"(devf  t%d::%s:%d) over poll waiting for buffered write completion\n",
							pthread_self(), __func__, __LINE__);
				return(-1);
			}
		}

		/* check if there is something left to write */
		if (left)
		{
			buffer += flashcfg.bus_width;
			memory += flashcfg.bus_width;

			/* get value to write from buffer */
			memcpy((void *) &value, buffer, min(left, flashcfg.bus_width));

			/* go back to read mode */
			send_command(memory, INTEL_READ_ARRAY);
		}
	}

	/* while there are bytes left to write loop */
	while (left > 0)
	{
		if (left < flashcfg.bus_width)
		{
			shift = left << 3;
			pad_value(&value, memory, shift,0);
		}


		/* check if no bits are cleared */
		if (value.w64 != F3S_I28F008_ERASE)
		{
			/* issue write command */
			send_command(memory, INTEL_WRITE);

			/* write data */
			write_value(memory, &value);
			if(intel_poll(memory,F3S_I28F008_POLL) == -1)
			{
				fprintf(stderr,"(devf  t%d::%s:%d) over poll waiting for write completion\n",
							pthread_self(), __func__, __LINE__);
				return(-1);
			}
		}

		/* decrement size left */
		left -= flashcfg.bus_width;
		/* check if there is still data to write */
		if (left > 0)
		{

			/* increment buffer pointer and memory pointer */

			buffer += flashcfg.bus_width;
			memory += flashcfg.bus_width;

			/* get value to write from buffer */
			memcpy((void *) &value, buffer, min(flashcfg.bus_width, left));

			/* check if padding is neccessary */
			if (left < flashcfg.bus_width)
			{
				/* go back to read mode */
				send_command(memory, INTEL_READ_ARRAY);
			}
		}
	}

	/* issue read status command */
	send_command(memory, INTEL_READ_STATUS);
	if(intel_read_status(readmem(memory)) == -1) return (-1);

	/* go back to read mode */
	send_command(memory, INTEL_READ_ARRAY);
	if (flags & F3S_VERIFY_WRITE)
	{
		/* check if everything was written properly */
		if (memcmp(buffer - size + left + flashcfg.bus_width,
			   (const void *) (((uint8_t *) memory) - size + left + flashcfg.bus_width), size))
		{
			fprintf(stderr, "(devf  t%d::%s:%d) program verify error\n"
						"between  0x%p and 0x%p\n"
						"memory = 0x%p, offset =  0x%x, left = %d, size = %d, bus_width = %d\n",
						pthread_self(), __func__, __LINE__,
						(((uint8_t *) memory) - size + left + flashcfg.bus_width),
						(((uint8_t *) memory) + left + flashcfg.bus_width),
						memory, offset, left, size, flashcfg.bus_width);
			return -1;
		}

	}

	return size;
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/intel/iCFI_write.c $ $Rev: 710521 $")
#endif
