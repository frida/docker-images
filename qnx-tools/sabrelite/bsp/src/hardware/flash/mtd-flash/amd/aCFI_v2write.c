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
 * Bus Width:       8-bit and 16-bit
 * Buffered Writes: Yes
 *
 * Description
 *
 * Use this for buffered writes.
 */

int32_t f3s_aCFI_v2write(f3s_dbase_t * dbase,
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
	intunion_t			value;
	int32_t				left;
	int32_t				unlock = 0;
	int32_t				pad;
	int32_t				shift;
	int dump_loop;

	dump_loop = 0;

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

	offset -= pad;
	buffer -= pad;

	/* allign memory address */
	left += pad;

	/* set proper page on socket */
	memory = access->service->page(&access->socket, F3S_POWER_ALL, offset, &left);
	if (memory == NULL) {
		fprintf(stderr, "(devf  t%d::%s:%d) page() returned NULL for offset 0x%x\n",
					pthread_self(), __func__, __LINE__, offset);
		return (-1);
	}

	/* set writeable size */
	size = left - pad;

	/* get value to write from buffer */
	memcpy((void *)&value, buffer, min(flashcfg.bus_width, left));

	/* check if padding is neccessary */
	if (pad) {
		/* calculate padding shift */
		shift = (flashcfg.bus_width - pad) << 3;
		pad_value(&value, memory, shift,1);
	}

	/* check if write buffer is possible */
	if ((dbase->buffer_size > (2 * flashcfg.bus_width)) && left >= (4 * flashcfg.bus_width)) {
		int32_t			buffer_size, buffer_count;

		/* while there are bytes left to write loop */
		while (left >= flashcfg.bus_width) {
			/* issue unlock cycles */
			send_command(command + (amd_cmd1), AMD_UNLOCK_CMD1);
			send_command(command + (amd_cmd2), AMD_UNLOCK_CMD2);

			send_command(memory, AMD_WRITE_BUFFER);

			buffer_size = min(dbase->buffer_size, left & ~(flashcfg.bus_width - 1));

			/* try to align buffer with page for best performance */
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

					/* get value to write from buffer */
					memcpy((void *)&value, buffer, flashcfg.bus_width);
				}
			}

			/* issue write confirm command */
			send_command(memory, AMD_BUFFER_CONFIRM);

			/* Wait for 6 us - according to AMD document, the maximam delay is 4 us */
			nanospin_ns(6000);

			/* DQ1 is "write-to-buffer abort", so only applies here */
			if (amd_poll(&value, memory, 1) == -1) {
				fprintf(stderr,"(devf  t%d::%s:%d) over poll waiting for write completion\n",
							pthread_self(), __func__, __LINE__);
				/* do a "write-to-buffer abort sequence" */
				send_command(command + amd_cmd1, AMD_UNLOCK_CMD1);
				send_command(command + amd_cmd2, AMD_UNLOCK_CMD2);
				send_command(command + amd_cmd1, AMD_READ_MODE);
				errno = EIO;
				return (-1);
			}

			/* check if there is still data to write */
			if (left >= flashcfg.bus_width) {
				buffer += flashcfg.bus_width;
				memory += flashcfg.bus_width;

				/* get value to write from buffer */
				memcpy((void *)&value, buffer, flashcfg.bus_width);
			}
		}
		/* check if there is something left to write */
		if (left) {
			buffer += flashcfg.bus_width;
			memory += flashcfg.bus_width;

			/* get value to write from buffer */
			memcpy((void *)&value, buffer, min(left, flashcfg.bus_width));
		}
	}

	/* while there are bytes left to write loop */
	while (left > 0) {
		/* check if left is smaller than bus size */
		if (left < flashcfg.bus_width) {
			/* calculate padding shift */
			shift = left << 3;
			pad_value(&value, memory, shift,0);
		}

		/* check if no bits are cleared */
		if (value.w64 != F3S_A29F040_CLEAR) {
			/* check if unlock bypass is not done */
			if (!unlock) {
				/* check if unlock bypass is not optimal */
				if (left <= (2 * flashcfg.bus_width)) {
					/* issue unlock cycles */
					send_command(command + amd_cmd1, AMD_UNLOCK_CMD1);
					send_command(command + amd_cmd2, AMD_UNLOCK_CMD2);

					/* issue write command */
					send_command(command + amd_cmd1, AMD_PROGRAM);

				} else {
					/* set unlock flag */
					unlock = 1;

					/* issue unlock cycles */
					send_command(command + amd_cmd1, AMD_UNLOCK_CMD1);
					send_command(command + amd_cmd2, AMD_UNLOCK_CMD2);

					/* issue unlock bypass command */
					send_command(command + amd_cmd1, AMD_UNLOCK_BYPASS);
				}
			}

			/* check if unlock was done */
			if (unlock) {
				/* issue unlock bypass write */
				send_command(command, AMD_PROGRAM);
			}

			/* write value */
			write_value(memory, &value);

			/* Wait for 6 us - according to AMD document, the maximam delay is 4 us */
			nanospin_ns(6000);

			/* DQ1 is "write-to-buffer abort", so never applies here */
			if (amd_poll(&value, memory, 0) == -1) {
				fprintf(stderr,"(devf  t%d::%s:%d) over poll waiting for write completion\n",
							pthread_self(), __func__, __LINE__);
				/* do a reset */
				send_command(memory, AMD_READ_MODE);
				errno = EIO;
				return (-1);
			}
		}

		/* decrement size left */
		left -= flashcfg.bus_width;

		/* check if there is still something to write */
		if (left > 0) {
			/* increment buffer pointer and memory pointer */
			buffer += flashcfg.bus_width;
			memory += flashcfg.bus_width;

			/* get value to write from buffer */
			memcpy((void *)&value, buffer, min(flashcfg.bus_width, left));
		}
	}

	/* check if unlocked */
	if (unlock) {
		/* unlock bypass reset */
		send_command(command, AMD_BYPASS_RESET1);
		send_command(command, AMD_BYPASS_RESET2);
	}
	/* Wait for 6 us - according to AMD document, the maximam delay is 4 us */
	nanospin_ns(6000);

	/* check if verify is wanted */
	if (flags & F3S_VERIFY_WRITE) {
		/* check if everything was written properly */
		if (memcmp(buffer - size + left + flashcfg.bus_width,
		           (const void *)(((uint8_t *)memory) - size + left + flashcfg.bus_width), size))
		{
			fprintf(stderr, "(devf  t%d::%s:%d) program verify error\n"
						"between  0x%p and 0x%p\n"
						"memory = 0x%p, offset =  0x%x, left = %d, size = %d, bus_width = %d\n",
						pthread_self(), __func__, __LINE__,
						(((uint8_t *)memory) - size + left + flashcfg.bus_width),
						(((uint8_t *)memory) + left + flashcfg.bus_width),
						memory, offset, left, size, flashcfg.bus_width);

			if (verbose >= 1) {
			/* Dump the bytes we wanted */
			fprintf(stderr, "Bytes in buffer: ");
			for (dump_loop = 0; dump_loop < size; dump_loop++){
				fprintf(stderr, "%2X ", *((uint8_t*)(buffer - size + left + flashcfg.bus_width)+dump_loop));
			}
			fprintf(stderr, "\n");

			/* Dump the bytes we got */
			fprintf(stderr, "Bytes on flash: ");
			for (dump_loop = 0; dump_loop < size; dump_loop++){
				fprintf(stderr, "%2X ", *((uint8_t *)memory - size + left + flashcfg.bus_width + dump_loop));
			}
			fprintf(stderr, "\n");
			}
			
			errno = EIO;
			return (-1);
		}
	}

	/* everything went fine */
	return size;
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/amd/aCFI_v2write.c $ $Rev: 710521 $")
#endif
