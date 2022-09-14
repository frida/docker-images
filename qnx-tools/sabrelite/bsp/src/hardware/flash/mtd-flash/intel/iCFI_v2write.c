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
 * Buffered Writes: Yes
 *
 * Description
 *
 * This is the only MTDv2 write callout for buffered writes.
 */

int32_t f3s_iCFI_v2write(f3s_dbase_t *dbase,
                         f3s_access_t *access,
                         uint32_t flags,
                         uint32_t offset,
                         int32_t size,
                         uint8_t *buffer)
{
	volatile void *	memory;
	intunion_t 		value;
	uint32_t		poll;
	int32_t			left;
	int32_t			buffer_size;
	int32_t			buffer_count;
	int32_t			pad;
	int32_t			shift;

	/* Initialize bytes left to write */
	left = size;

	/* Allign memory address */
	pad = offset & (flashcfg.bus_width - 1);

	offset -= pad;
	buffer -= pad;

	/* Adjust left with padding */
	left += pad;

	/* Set proper page on socket */
	memory = access->service->page(&access->socket, F3S_POWER_ALL, offset, &left);
	if (!memory)
	{
		fprintf(stderr, "(devf  t%d::%s:%d) page() returned NULL for offset 0x%x\n",
					pthread_self(), __func__, __LINE__, offset);
		return (-1);
	}

	/* Initialize number of bytes writeable */
	size = left - pad;

	/* Get value to write from buffer */
	memcpy((void *)(pad + (char *)&value), buffer + pad, min(flashcfg.bus_width, left) - pad);

	/* If padding is neccessary */
	if (pad)
	{
		shift = (flashcfg.bus_width - pad) << 3;
		pad_value(&value, memory, shift,1);
	}

	/* If write to buffer is possible */
	if ((dbase->flags & F3S_WRITE_BUFFER) && (left >= (4 * flashcfg.bus_width)))
	{
		/* Write in bus sized chunks */
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
				if (poll == 0) break;
				poll--;
			} while ((readmem(memory) & (0x80 * flashcfg.device_mult)) != (0x80 * flashcfg.device_mult));

			if (!poll)
			{
				fprintf(stderr,"(devf  t%d::%s:%d) over poll waiting for write buffer\n",
							pthread_self(), __func__, __LINE__);
				errno = EIO;
				return(-1);
			}

			/* try to align buffer with page for best performance */
			buffer_size = min(dbase->buffer_size, left & ~(flashcfg.bus_width - 1));
			buffer_size = min(buffer_size, dbase->buffer_size - (offset & (dbase->buffer_size - 1)));

			left -= buffer_size;
			offset += buffer_size;

			/* Calculate buffer count */
			buffer_count = buffer_size / flashcfg.bus_width;

			/* Write buffer size to flash */
			send_command(memory, buffer_count - 1);

			/* Write data to the buffer */
			while (buffer_count > 0)
			{
				write_value(memory, &value);
				buffer_count--;

				/* If there is still data to write */
				if (buffer_count > 0 || left >= flashcfg.bus_width)
				{
					buffer += flashcfg.bus_width;
					memory += flashcfg.bus_width;
					memcpy((void *)&value, buffer, flashcfg.bus_width);
				}
			}

			/* Confirm the buffered write */
			send_command(memory, INTEL_BUFFER_CONFIRM);
			if (intel_poll(memory,F3S_I28F008_POLL) == -1)
			{
				fprintf(stderr,"(devf  t%d::%s:%d) over poll waiting for buffered write completion\n",
							pthread_self(), __func__, __LINE__);
				errno = EIO;
				return (-1);
			}
		}

		/* If there is something left to write */
		if (left)
		{
			buffer += flashcfg.bus_width;
			memory += flashcfg.bus_width;

			/* Get value to write from buffer */
			memcpy((void *)&value, buffer, min(left, flashcfg.bus_width));
			send_command(memory, INTEL_READ_ARRAY);
		}
	}

	/* Write remaining bytes */
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
			/* Write the data */
			send_command(memory, INTEL_WRITE);
			write_value(memory, &value);
			if (intel_poll(memory,F3S_I28F008_POLL) == -1)
			{
				fprintf(stderr,"(devf  t%d::%s:%d) over poll waiting for write completion\n",
							pthread_self(), __func__, __LINE__);
				errno = EIO;
				return (-1);
			}
		}

		/* If there is still data to write */
		left -= flashcfg.bus_width;
		if (left > 0)
		{
			buffer += flashcfg.bus_width;
			memory += flashcfg.bus_width;

			/* Get value to write from buffer */
			memcpy((void *)&value, buffer, min(flashcfg.bus_width, left));

			/* If padding is neccessary */
			if (left < flashcfg.bus_width)
			{
				send_command(memory, INTEL_READ_ARRAY);
			}
		}
	}

	/* Check the write status */
	send_command(memory, INTEL_READ_STATUS);
	if(intel_read_status(readmem(memory)) == -1) return (-1);
	send_command(memory, INTEL_READ_ARRAY);

	if (flags & F3S_VERIFY_WRITE)
	{
		/*
		 *PR 26211.  J3 flash requires a delay after programming.  This
		 *nanospin corrects the one instance we've seen of this errata.
		 */
		nanospin_ns(2000);
		
		/* If everything was written properly */
		if (memcmp(buffer - size + left + flashcfg.bus_width,
		           (const void *)(((uint8_t *) memory) - size + left + flashcfg.bus_width), size))
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
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/intel/iCFI_v2write.c $ $Rev: 710521 $")
#endif
