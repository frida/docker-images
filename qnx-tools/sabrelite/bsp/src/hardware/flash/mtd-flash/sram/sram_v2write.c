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
#include <sys/neutrino.h>
#include <pthread.h>

/*
 * Summary
 *
 * MTD Version: 2 only
 * Bus Width:   All
 *
 * Description
 *
 * This is the only MTDv2 write callout for RAM / SRAM. The extra code
 * simulates a faulty NOR flash part for testing purposes.
 */

extern uint32_t *SRAM_LOCK;

#ifndef f3s_assert
#define f3s_assert(FLAG) \
				do { \
						/* check if flag is not true */\
					if(!(FLAG)) \
					{ \
						printf("(devf  t%d::%s:%d)", pthread_self(), __func__, __LINE__); \
						DebugBreak(); \
					} \
				} while(0)
#endif	// f3s_assert

/* turn off FAULT_SIMULATOR by default */
#undef FAULT_SIMULATOR

/*
 * Do a copy, but behave more like a flash device. Can knock down 1's but cannot change a 0 to a 1 by writing
 * This functions copies <size> bytes from <src> to <dst> at <offset>. The size of an erase unit is also provided
 * in order to aid in the output.
*/
static void flashcpy (uint8_t *dst, const uint8_t *src, uint32_t size,
                      uint32_t offset, uint32_t usize)
{
	uint32_t	i;

	for (i = 0; i < size; i++) {
		if (~dst[i] & src[i]) {
			fprintf(stderr, "(devf  t%d::%s:%d) Attempting to change 0's to 1's at offset %d in unit %d\n"
									"address 0x%.8x[0x%x] == 0x%.2x cannot be changed to 0x%.2x, "
									"bits 0x%.2x already 0\n",
									pthread_self(), __func__, __LINE__,
									(offset + i) % usize, (offset + i) / usize,
									(unsigned int)dst, i, dst[i], src[i], ~dst[i] & src[i]);
			/* behave like flash and don't allow 0's to be set back to 1's */
			dst[i] = dst[i] & src[i];
		}
		else
		{
			dst[i] = src[i];
		}
	}
}

int32_t f3s_sram_v2write(f3s_dbase_t *dbase,
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
	if (!memory) return (-1);

#ifndef FAULT_SIMULATOR

	f3s_assert(access->socket.unit_size != 0);
	flashcpy (memory, buffer, size, offset, access->socket.unit_size);

#else	// FAULT_SIMULATOR

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
		uint32_t		usize  = access->socket.unit_size ? access->socket.unit_size : 65536;
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
					fprintf(stderr, "[0x%8.8X]Delaying write fault simulation: %d/%d\n",
					        writes, writes, FAULT_DELAY);
				}

				flashcpy (memory, buffer, size, offset, usize);

			/* If we are going to simulate a fault */
			} else if (rand() < ((FAULT_RATE * RAND_MAX) / 100)) {
				if ((show & SHOW_FAULT) && (writes >= silent)) {
					fprintf(stderr, "[0x%8.8X]Simulating fault writing offset    = 0x%8.8X, size = 0x%8.8X, [%3d,%3d]\n",
					        writes, offset, size, offset / usize, (usize - (offset % usize)) / sizeof(f3s_head_t));
				}

				/* Main copy loop */
				for (i = 0; i < size; i++) {
					/* Randomly determine whether we corrupt this byte or not */
					if (rand() < ((FAULT_DENSITY * RAND_MAX) / 100)) {
						/* Compute the bad byte */
						bad = ((memory[i] & rand()) | (buffer[i] & rand()) & memory[i]);
						if ((show & SHOW_FAULT) && (writes >= silent)) {
							fprintf(stderr, " flash[%2X] = b%8s (%2X), good = b%8s (%2X), bad = b%8s (%2X)\n",
							        i, ltoa (memory[i], buf1, 2), memory[i],
							           ltoa (buffer[i], buf2, 2), buffer[i],
							           ltoa (bad,       buf3, 2), bad);
						}
						memory[i] = bad;

					/* This byte has been spared */
					} else {
						if (~memory[i] & buffer[i]) {
							fprintf(stderr, "Attempting to change zero to one [%3d,%3d]\n",
							        offset / usize, (usize - (offset % usize)) / sizeof(f3s_head_t));
							fprintf(stderr, " flash[%2X] = b%8s (%2X), bad = b%8s (%2X), written = b%8s (%2X)\n",
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
					fprintf(stderr, "[0x%8.8X]Simulating successful write offset = 0x%8.8X, size = 0x%8.8X, [%3d,%3d]\n",
					        writes, offset, size, offset / usize, (usize - (offset % usize)) / sizeof(f3s_head_t));
				}

				flashcpy (memory, buffer, size, offset, usize);
			}

			/* If we are going to simulate a power failure */
			if ((writes == crash) ||
			    (rand() < ((CRASH_RATE * RAND_MAX) / 100)))
			{
				fprintf (stderr, "Simulating power off\n");
				exit(-1);
			}

			writes++;

		/* Otherwise, just copy the memory */
		} else {
			if ((show & SHOW_WRITE) && (writes >= silent)) {
				fprintf(stderr, "[0x%8.8X]Writing to [%3d,%3d]\n",
				        writes, offset / usize, (usize - (offset % usize)) / sizeof(f3s_head_t));
			}

			flashcpy (memory, buffer, size, offset, usize);

			/* If we are going to simulate a power failure */
			if (writes == crash)
			{
				fprintf (stderr, "Simulating power off\n");
				exit(-1);
			}

			writes++;
		}
	}
#endif	// FAULT_SIMULATOR

#ifdef NDEBUG
	/* Perform verify after write  (unconditionally if debug load (ie. NDEBUG not defined)) */
	if (flags & F3S_VERIFY_WRITE)
#endif	// NDEBUG
	{
		if (memcmp(memory, buffer, size))
		{
			fprintf(stderr, "(devf  t%d::%s:%d) program verify error\n"
						"between  0x%p and 0x%p\n"
						"memory = 0x%p, offset =  0x%x, size = %d\n",
						pthread_self(), __func__, __LINE__,
						memory, memory + size, memory, offset, size);
		errno = EIO;
		return (-1);
		}
	}

	return (size);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/sram/sram_v2write.c $ $Rev: 710521 $")
#endif
