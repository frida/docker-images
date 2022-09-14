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
 * MTD Version: 2 only
 * Bus Width:   All
 *
 * Description
 *
 * This is the only MTDv2 erase callout for RAM / SRAM. The extra code
 * simulates a faulty NOR flash part for testing purposes.
 */

extern uint32_t *SRAM_LOCK;

int f3s_sram_v2erase(f3s_dbase_t *dbase,
                     f3s_access_t *access,
                     uint32_t flags,
                     uint32_t offset)
{
	uint8_t *	memory;
	uint32_t	usize = access->socket.unit_size ? access->socket.unit_size : 65536;
	uint32_t	size;

	/* Rount offset to beginning of flash unit */
	offset = offset & ~(usize - 1);

	/* Don't write to a protected unit */
	if ((SRAM_LOCK != NULL) &&
	    (SRAM_LOCK[(offset / (access->socket.unit_size ? access->socket.unit_size : 65536)) -
	               ((offset / (access->socket.unit_size ? access->socket.unit_size : 65536)) % 32)] & (1 << ((offset / (access->socket.unit_size ? access->socket.unit_size : 65536)) % 32))))
	{
		return (EROFS);;
	}

	/* Set proper page on socket */
	size = usize;
	memory = access->service->page(&access->socket, F3S_POWER_VPP, offset, (int32_t *)&size);
	if (!memory) return (errno);

#ifndef NDEBUG
#	undef FAULT_SIMULATOR
#	ifdef FAULT_SIMULATOR
#		define FAULT_RATE		0	// Fault rate (integer percent)
#		define FAULT_DELAY		0	// Number of erases to delay simulator
#		define FAULT_DENSITY	1	// Amount of bytes to corrupt (integer percent 0.1)
#		define CRASH_RATE		0	// Power off rate (integer percent)
#		define SHOW_DELAY		0x01
#		define SHOW_ERASE		0x02
#		define SHOW_FAULT		0x04
	{
		static uint32_t	silent = 0;
		static uint32_t	erases = 0;
		static uint32_t crash  = ~0;
		const uint32_t	show   = SHOW_ERASE | SHOW_FAULT;
		char			buf1[12];
		char			buf2[12];
		uint32_t		i;
		uint8_t			bad;

		/* if we must delay the fault simulator */
		if (erases < FAULT_DELAY) {
			if ((show & SHOW_DELAY) && (erases >= silent)) {
				fprintf(stderr, "(devf  t%d::%s:%d) [0x%8.8X]Delaying erase fault simulation: %d/%d\n",
						pthread_self(), __func__, __LINE__,
				        erases, erases, FAULT_DELAY);
			}

			memset (memory, 0xFF, usize);

		/* If we are going to simulate a fault */
		} else if (rand() < ((FAULT_RATE * RAND_MAX) / 100)) {
			if ((show & SHOW_FAULT) && (erases >= silent)) {
				fprintf(stderr, "(devf  t%d::%s:%d) [0x%8.8X]Simulating fault erasing unit    = 0x%4.4X\n",
						pthread_self(), __func__, __LINE__,
				        erases, offset / usize);
			}

			/* Main erase loop */
			for (i = 0; i < size; i++) {
				/* Randomly determine whether we corrupt this byte or not */
				if (rand() < ((FAULT_DENSITY * RAND_MAX) / 1000)) {
					/* Compute the bad byte */
					bad = (memory[i] ^ rand());
					if ((show & SHOW_FAULT) && (erases >= silent)) {
						fprintf(stderr, "(devf  t%d::%s:%d) flash[%2X] = b%8s (%2X), good = b11111111 (FF), bad = b%8s (%2X)\n",
						        pthread_self(), __func__, __LINE__,
						        i, ltoa (memory[i], buf1, 2), memory[i],
						           ltoa (bad,       buf2, 2), bad);
					}
					memory[i] = bad;

				/* This byte has been spared */
				} else {
					memory[i] = 0xFF;
				}
			}

			/* XXX - We can't detect and/or recover from a bad erase yet */
			fprintf (stderr, "(devf  t%d::%s:%d) Simulating power off\n",
						pthread_self(), __func__, __LINE__);
			exit(-1);

		/* Otherwise, just erase the memory */
		} else {
			if ((show & SHOW_ERASE) && (erases >= silent)) {
				fprintf(stderr, "(devf  t%d::%s:%d) [0x%8.8X]Simulating successful erase unit = 0x%4.4X\n",
						pthread_self(), __func__, __LINE__,
				        erases, offset / usize);
			}

			memset (memory, 0xFF, usize);
		}

		/* If we are going to simulate a power failure */
		if ((erases >= crash) ||
		    (rand() < ((CRASH_RATE * RAND_MAX) / 100)))
		{
			if ((show & SHOW_FAULT) && (erases >= silent)) {
				fprintf(stderr, "(devf  t%d::%s:%d) [0x%8.8X]Simulating fault erasing unit    = 0x%4.4X\n",
						pthread_self(), __func__, __LINE__,
				        erases, offset / usize);
			}

			/* Main erase loop */
			for (i = 0; i < size; i++) {
				/* Randomly determine whether we corrupt this byte or not */
				if (rand() < ((FAULT_DENSITY * RAND_MAX) / 1000)) {
					/* Compute the bad byte */
					bad = (memory[i] ^ rand());
					if ((show & SHOW_FAULT) && (erases >= silent)) {
						fprintf(stderr, "(devf  t%d::%s:%d) flash[%2X] = b%8s (%2X), good = b11111111 (FF), bad = b%8s (%2X)\n",
						        pthread_self(), __func__, __LINE__,
						        i, ltoa (memory[i], buf1, 2), memory[i],
						           ltoa (bad,       buf2, 2), bad);
					}
					memory[i] = bad;

				/* This byte has been spared */
				} else {
					memory[i] = 0xFF;
				}
			}

			fprintf (stderr, "(devf  t%d::%s:%d) Simulating power off\n",
							pthread_self(), __func__, __LINE__);
			exit(-1);
		}

		erases++;
	}
#	else
	{
		/* Pretend to erase the block */
		memset (memory, 0xFF, usize);
	}
#	endif
#else
	/* Pretend to erase the block */
	memset (memory, 0xFF, usize);
#endif

	return (EOK);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/sram/sram_v2erase.c $ $Rev: 710521 $")
#endif
