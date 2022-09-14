/*
 * $QNXLicenseC: 
 * Copyright 2007, QNX Software Systems.  
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
 * Bus Width:       8-bit and 16-bit
 * Buffered Writes: Yes
 * Note:            Derived from the AMD CFI driver
 *
 * Description
 *
 * Use this for buffered writes.
 */

int32_t f3s_s29glxxxs_v2write(f3s_dbase_t * dbase,
                              f3s_access_t * access,
                              uint32_t flags,
                              uint32_t offset,
                              int32_t size,
                              uint8_t * buffer)
{
	volatile uint8_t *	memory;
	volatile uint8_t *	command;
	uintptr_t			amd_cmd1;
	uintptr_t			amd_cmd2;
	intunion_t			value, lvalue;
	int32_t				left;
	int32_t				pad, lpad, res;
	int32_t				shift;
	int32_t				buffer_size, buffer_count;

	/* nothing to do */
	if (size == 0)
		return size;
 
	/* Set the command address according to chip width */
	if (flashcfg.device_width == 1) {
		amd_cmd1 = AMD_CMD_ADDR1_W8;
		amd_cmd2 = AMD_CMD_ADDR2_W8;
	} else {
		amd_cmd1 = AMD_CMD_ADDR1_W16;
		amd_cmd2 = AMD_CMD_ADDR2_W16;
	}
	amd_cmd1 *= flashcfg.bus_width;
	amd_cmd2 *= flashcfg.bus_width;

	/* set command pointer */
	command = access->service->page(&access->socket, F3S_POWER_ALL, offset & amd_command_mask, NULL);
	if (command == NULL) {
		fprintf(stderr, "(devf  t%d::%s:%d) page() returned NULL for offset 0x%x\n",
					pthread_self(), __func__, __LINE__, offset);
		return (-1);
	}

	left = size;
        
	pad = offset & (flashcfg.bus_width - 1);
	if (pad) {
		offset -= pad;
		buffer -= pad;
		left   += pad;
	}
        
	lpad = (offset + left) & (flashcfg.bus_width - 1);
	if (lpad) {
		lpad  = flashcfg.bus_width - lpad;
		left += lpad;
	}

        /* left should now be bus aligned */
	if (left & (flashcfg.bus_width - 1))
		fprintf(stderr,"(devf) internal write error: unable to align data\n");
        
	/* set proper page on socket */
	memory = access->service->page(&access->socket, F3S_POWER_ALL, offset, &left);
	if (memory == NULL) {
		fprintf(stderr, "(devf  t%d::%s:%d) page() returned NULL for offset 0x%x\n",
					pthread_self(), __func__, __LINE__, offset);
		return (-1);
	}
	readmem(memory); //Spansion dummy read workaround

	/* get value to write from buffer */
	memcpy((void *)&value + pad, buffer + pad, min(flashcfg.bus_width - pad, size));

	/* check if padding is neccessary */
	if (pad) {
		/* calculate padding shift */
		shift = (flashcfg.bus_width - pad) << 3;
		pad_value(&value, memory, shift, 1);
	}

	/* check if padding the last value is neccessary */
	if (lpad) {
		/* calculate padding shift */
		shift = (flashcfg.bus_width - lpad) << 3;
		if (left == flashcfg.bus_width) {
			/* pad the first word at the end */
			pad_value(&value, memory, shift, 0);
		}
		else {
			/* pad the last word and save it for later */
			memcpy((void *)&lvalue, buffer + pad + size - (flashcfg.bus_width - lpad), 
			       flashcfg.bus_width - lpad);
			pad_value(&lvalue, memory + left - flashcfg.bus_width, shift, 0);
		}
	}

	/* while there are bytes left to write loop */
	while (left) {
		/* issue unlock cycles */
		send_command(command + (amd_cmd1), AMD_UNLOCK_CMD1);
		send_command(command + (amd_cmd2), AMD_UNLOCK_CMD2);

                send_command(memory, AMD_WRITE_BUFFER);

		buffer_size = min(dbase->buffer_size, left & ~(flashcfg.bus_width - 1));

		/* align buffer with write buffer page */
		buffer_size = min(buffer_size, dbase->buffer_size - (offset & (dbase->buffer_size - 1)));
		left -= buffer_size;
		offset += buffer_size;

		/* calculate buffer count */
		buffer_count = buffer_size  / flashcfg.device_width  / dbase->chip_inter;

		/* write buffer size to flash */
		send_command(memory, buffer_count - 1);

		/* while there is something for buffer */
		while (buffer_count > 0) {
			/* write data */
			write_value(memory, &value);

			/* check if there is still data to write for this operation */
			if (--buffer_count > 0) {
			    	buffer += flashcfg.bus_width;
				memory += flashcfg.bus_width;

				if ((left == 0) && lpad && (buffer_count == 1))
					/* use precomputed last value if padded */
					value = lvalue;
				else
					/* get value to write from buffer */
					memcpy((void *)&value, buffer, flashcfg.bus_width);
			}
		}

		/* issue write confirm command */
		send_command(memory, AMD_BUFFER_CONFIRM);

#ifdef F3S_S29GLXXXS_DQPOLL
		/* DQ1 is "write-to-buffer abort", so only applies here */
		if (amd_poll(&value, memory, 1) == -1) {
			fprintf(stderr,"(devf  t%d::%s:%d) over poll waiting for write completion\n",
						pthread_self(), __func__, __LINE__);
			errno = EIO;
			return (-1);
		}
#else
		do {
			/* wait for 1 us - then check status */
			nanospin_ns(1000);
			res = f3s_s29glxxxs_v2sync(dbase, access, F3S_SYNC_FOR_WRITE, offset & (~(dbase->chip_size - 1)));
		} while (res == EAGAIN);

		/* in case of a fault return error */
		if (res == EIO) {
			fprintf(stderr,"(devf  t%d::%s:%d) write error\n",
						pthread_self(), __func__, __LINE__);
			errno = EIO;
			return (-1);
		}
#endif

		/* check if there is still data to write */
		if (left) {
			buffer += flashcfg.bus_width;
			memory += flashcfg.bus_width;

			if (lpad && left == flashcfg.bus_width)
				/* use precomputed last value if padded */
				value = lvalue;
			else
				/* get value to write from buffer */
				memcpy((void *)&value, buffer, flashcfg.bus_width);
		}
	}

	if (left) 
		fprintf(stderr,"(devf) internal write error: %d remaining bytes skipped\n", left);
        
	/* check if verify is wanted */
	if (flags & F3S_VERIFY_WRITE) {
		/* check if everything was written properly */
		if (memcmp(buffer - size - lpad + flashcfg.bus_width,
		           (const void *)(((uint8_t *)memory) - size - lpad + flashcfg.bus_width), size))
		{
			fprintf(stderr, "(devf  t%d::%s:%d) program verify error\n"
						"between  0x%p and 0x%p\n"
						"memory = 0x%p, offset =  0x%x, size = %d, bus_width = %d\n",
						pthread_self(), __func__, __LINE__,
						(((uint8_t *)memory) - size - lpad + flashcfg.bus_width),
						(((uint8_t *)memory) - lpad + flashcfg.bus_width),
						memory, offset, size, flashcfg.bus_width);
			errno = EIO;
			return (-1);
		}
	}

	/* everything went fine */
	return size;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/spansion/s29glxxxs_v2write.c $ $Rev: 738496 $")
#endif
