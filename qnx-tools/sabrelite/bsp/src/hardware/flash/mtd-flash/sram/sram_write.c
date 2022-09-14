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
#include <sys/neutrino.h>

/*
 * Summary
 *
 * MTD Version: 1 only
 * Bus Width:   All
 *
 * Description
 *
 * This is the only MTDv1 write callout for RAM / SRAM. The extra code
 * simulates a faulty NOR flash part for testing purposes.
 */

extern uint32_t *SRAM_LOCK;

#ifndef NDEBUG
static void flashcpy (uint8_t *dst, const uint8_t *src, uint32_t size,
                      uint32_t offset, uint32_t usize)
{
	char		buf1[12];
	char		buf2[12];
	char		buf3[12];
	uint32_t	i;

	for (i = 0; i < size; i++) {
		if (~dst[i] & src[i]) {
			fprintf(stderr, "(devf  t%d::%s:%d) Attempting to change zero to one [%3d,%3d]\n",
					pthread_self(), __func__, __LINE__,
			        offset / usize, (usize - (offset % usize)) / sizeof(f3s_head_t));
			fprintf(stderr, "(devf  t%d::%s:%d) flash[%2X] = b%8s (%2X), bad = b%8s (%2X), written = b%8s (%2X)\n",
			        pthread_self(), __func__, __LINE__,
			        i, ltoa (dst[i], buf1, 2), dst[i],
			           ltoa (src[i], buf2, 2), src[i],
			           ltoa (src[i] & dst[i], buf3, 2), src[i] & dst[i]);

			/* Pretend flash will not crash */
			dst[i] = dst[i] & src[i];

		} else {
			dst[i] = src[i];
		}
	}
}
#endif

int32_t f3s_sram_write(f3s_dbase_t *dbase,
                       f3s_access_t *access,
                       uint32_t flags,
                       uint32_t offset,
                       int32_t size,
                       uint8_t *buffer)
{
	uint8_t *	memory;

	/* Don't write to a protected unit */
	if ((SRAM_LOCK != NULL) &&
	    (SRAM_LOCK[(offset / (access->socket.unit_size ? access->socket.unit_size : 65536)) -
	               ((offset / (access->socket.unit_size ? access->socket.unit_size : 65536)) % 32)] & (1 << ((offset / (access->socket.unit_size ? access->socket.unit_size : 65536)) % 32))))
	{
		errno = EROFS;
		return (-1);
	}

	/* Set proper page on socket */
	memory = access->service->page(&access->socket, F3S_POWER_VPP, offset, &size);
	if (!memory) return -1;

#ifdef NDEBUG

	/* Copy the memory into sram */
	memcpy(memory, buffer, size);

#else

#	undef FAULT_SIMULATOR
#	ifdef FAULT_SIMULATOR
#		define FAULT_RATE		1	// Fault rate (integer percent)
#		define FAULT_DELAY		200	// Number of writes to delay simulator
#		define FAULT_DENSITY	25	// Amount of bytes to corrupt (integer percent)
#		define CRASH_RATE		0	// Power off rate (integer percent)
#		define SHOW_DELAY		0x01
#		define SHOW_WRITE		0x02
#		define SHOW_FAULT		0x04
	{
		static uint32_t	silent = 0;
		static uint32_t	writes = 0;
		static uint32_t crash  = -1;
		const uint32_t	show   = SHOW_WRITE | SHOW_FAULT;
		uint32_t		usize = access->socket.unit_size ? access->socket.unit_size : 65536;
		char			buf1[12];
		char			buf2[12];
		char			buf3[12];
		uint32_t		i;
		uint8_t			bad;

		/* Randomly determine a write corruption fault */
		if (flags & F3S_VERIFY_WRITE) {
			/*
			 * NOTE: We only simulate faults if on meta-data (ie. with verify
			 *       after write enabled).
			 */

			/* If we must delay the fault simulator */
			if (writes < FAULT_DELAY) {
				if ((show & SHOW_DELAY) && (writes >= silent)) {
					fprintf(stderr, "(devf  t%d::%s:%d) [0x%8.8X]Delaying write fault simulation: %d/%d\n",
							pthread_self(), __func__, __LINE__,
					        writes, writes, FAULT_DELAY);
				}

				flashcpy (memory, buffer, size, offset, usize);

			/* If we are going to simulate a fault */
			} else if (rand() < ((FAULT_RATE * RAND_MAX) / 100)) {
				if ((show & SHOW_FAULT) && (writes >= silent)) {
					fprintf(stderr, "(devf  t%d::%s:%d) [0x%8.8X]Simulating fault writing offset    = 0x%8.8X, size = 0x%8.8X, [%3d,%3d]\n",
					        pthread_self(), __func__, __LINE__,
					        writes, offset, size, offset / usize, (usize - (offset % usize)) / sizeof(f3s_head_t));
				}

				/* Main copy loop */
				for (i = 0; i < size; i++) {
					/* Randomly determine whether we corrupt this byte or not */
					if (rand() < ((FAULT_DENSITY * RAND_MAX) / 100)) {
						/* Compute the bad byte */
						bad = ((memory[i] & rand()) | (buffer[i] & rand()) & memory[i]);
						if ((show & SHOW_FAULT) && (writes >= silent)) {
							fprintf(stderr, "(devf  t%d::%s:%d) flash[%2X] = b%8s (%2X), good = b%8s (%2X), bad = b%8s (%2X)\n",
							        pthread_self(), __func__, __LINE__,
							        i, ltoa (memory[i], buf1, 2), memory[i],
							           ltoa (buffer[i], buf2, 2), buffer[i],
							           ltoa (bad,       buf3, 2), bad);
						}
						memory[i] = bad;

					/* This byte has been spared */
					} else {
						if (~memory[i] & buffer[i]) {
							fprintf(stderr, "(devf  t%d::%s:%d) Attempting to change zero to one [%3d,%3d]\n",
							        pthread_self(), __func__, __LINE__,
							        offset / usize, (usize - (offset % usize)) / sizeof(f3s_head_t));
							fprintf(stderr, "(devf  t%d::%s:%d) flash[%2X] = b%8s (%2X), bad = b%8s (%2X), written = b%8s (%2X)\n",
							        pthread_self(), __func__, __LINE__,
							        i, ltoa (memory[i], buf1, 2), memory[i],
							           ltoa (buffer[i], buf2, 2), buffer[i],
							           ltoa (memory[i] & buffer[i], buf3, 2), memory[i] & buffer[i]);

							/* Pretend flash will not crash */
							memory[i] = memory[i] & buffer[i];

						} else {
							memory[i] = buffer[i];
						}
					}
				}

			/* Otherwise, just copy the memory */
			} else {
				if ((show & SHOW_WRITE) && (writes >= silent)) {
					fprintf(stderr, "(devf  t%d::%s:%d) [0x%8.8X]Simulating successful write offset = 0x%8.8X, size = 0x%8.8X, [%3d,%3d]\n",
					        pthread_self(), __func__, __LINE__,
					        writes, offset, size, offset / usize, (usize - (offset % usize)) / sizeof(f3s_head_t));
				}

				flashcpy (memory, buffer, size, offset, usize);
			}

			/* If we are going to simulate a power failure */
			if ((writes == crash) ||
			    (rand() < ((CRASH_RATE * RAND_MAX) / 100)))
			{
				fprintf (stderr, "(devf  t%d::%s:%d) Simulating power off\n",
							pthread_self(), __func__, __LINE__);
				exit(-1);
			}

			writes++;

		/* Otherwise, just copy the memory */
		} else {
			if ((show & SHOW_WRITE) && (writes >= silent)) {
				fprintf(stderr, "(devf  t%d::%s:%d) [0x%8.8X]Writing to [%3d,%3d]\n",
				        pthread_self(), __func__, __LINE__,
				        writes, offset / usize, (usize - (offset % usize)) / sizeof(f3s_head_t));
			}

			flashcpy (memory, buffer, size, offset, usize);

			/* If we are going to simulate a power failure */
			if (writes == crash)
			{
				fprintf (stderr, "(devf  t%d::%s:%d) Simulating power off\n",
							pthread_self(), __func__, __LINE__);
				exit(-1);
			}

			writes++;
		}
	}
#	else
	{
		uint32_t	usize = access->socket.unit_size ? access->socket.unit_size : 65536;
		flashcpy (memory, buffer, size, offset, usize);
	}
#	endif

#endif

	/* Perform verify after write */
	if (memcmp(memory, buffer, size))
	{
		fprintf(stderr, "(devf  t%d::%s:%d) program verify error\n"
					"between  0x%p and 0x%p\n"
					"memory = 0x%p, offset =  0x%x, size = %d\n",
					pthread_self(), __func__, __LINE__,
					memory, memory + size, memory, offset, size);
		return -1;
	}

	return (size);
}






#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/sram/sram_write.c $ $Rev: 710521 $")
#endif
