/*
 * $QNXLicenseC: 
 * Copyright 2010, QNX Software Systems.  
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

#include "ipl.h"
#include <stdint.h>
#include <sys/types.h>
#include <hw/inout.h>
#include "mem_test.h"

#ifndef NULL
#define NULL	((void *)0)
#endif

/*
 * Any tests added to this file are expected to have access to a read/write
 * area of memory (presumably different than the memory under test) for use as
 * a stack
*/


/*
 * =============================================================================
 *
 *                         M E M O R Y   T E S T S
 *
 *                              Adapted from
 *
 *       http://www.netrino.com/Embedded-Systems/How-To/Memory-Test-Suite-C
 *       
 * Authors Terms of Use
 * 
 * "      
 * Free source code
 *       
 * The C source code for these memory tests is placed into the public domain and
 * is available in electronic form at http://www.netrino.com/code/memtest.zip.
 * Ports to the 8051 and Phillips XA processors can be found at
 * http://www.esacademy.com/faq/progs/ram.htm.
 * "
 * 
 * =============================================================================
*/
/*******************************************************************************
 * Data Bus Test (1's walk)
 *
 * Test the data bus wiring in a memory region by performing a walking 1's test
 * at a fixed address within that region. The address (and hence the memory
 * region) is selected by the caller.
 *
 * Returns:     0 if the test succeeds.
 *              A non-zero result is the first pattern that failed.
 *
 *
 * Returns: the number of bytes tested on success or 0 on error
*/
#define _1s_WALK(p, v) \
		do { \
			for ((v)=1; (v)!=0; (v)<<=1) \
			{ \
				*(p) = (v); \
				if (*(p) != (v)) break;	/* FAIL */ \
			} \
		} while(0)

__attribute__((section(".rom.text")))
static unsigned _8bit_1s_walk(paddr_t mem_loc, __attribute__((unused))uint64_t mem_size)
{
	volatile uint8_t *p = (volatile uint8_t *)mem_loc;
	uint8_t v;

	_1s_WALK(p, v);
	return (v == 0) ? sizeof(v) : 0;
}

__attribute__((section(".rom.text")))
static unsigned _16bit_1s_walk(paddr_t mem_loc, __attribute__((unused))uint64_t mem_size)
{
	volatile uint16_t *p = (volatile uint16_t *)mem_loc;
	uint16_t v;

	_1s_WALK(p, v);
	return (v == 0) ? sizeof(v) : 0;
}

__attribute__((section(".rom.text")))
static unsigned _32bit_1s_walk(paddr_t mem_loc, __attribute__((unused))uint64_t mem_size)
{
	volatile uint32_t *p = (volatile uint32_t *)mem_loc;
	uint32_t v;

	_1s_WALK(p, v);
	return (v == 0) ? sizeof(v) : 0;
}

__attribute__((section(".rom.text")))
static unsigned _64bit_1s_walk(paddr_t mem_loc, __attribute__((unused))uint64_t mem_size)
{
	volatile uint64_t *p = (volatile uint64_t *)mem_loc;
	uint64_t v;

	_1s_WALK(p, v);
	return (v == 0) ? sizeof(v) : 0;
}

/**********************************************************************
 *
 * Address Bus Test
 *
 * Test the address bus wiring in a memory region by performing a walking 1's
 * test on the relevant bits of the address and checking for aliasing. This test
 * will find single-bit address failures such as stuck-high, stuck-low, and
 * shorted pins.  The base address and size of the region are selected by the
 * caller.
 *
 * For best results, the selected base address should have enough LSB 0's to
 * guarantee single address bit changes. For example, to test a 64-Kbyte region,
 * select a base address on a 64-Kbyte boundary. Also, select the region size as
 * a power-of-two--if at all possible.
 *
 * Returns:     NULL if the test succeeds.
 *              A non-zero result is the first address at which an
 *              aliasing problem was uncovered.  By examining the
 *              contents of memory, it may be possible to gather
 *              additional information about the problem.
 *
*/
#define ADDR_TEST(baseAddress, mask, pattern, pattern_1scomp, r) \
    do { \
        unsigned offset; \
        unsigned testOffset; \
		/* Write the default pattern at each of the power-of-two offsets */ \
		for (offset=1; (offset & (mask))!=0; offset<<=1) { \
			(baseAddress)[offset] = (pattern); \
		} \
		/* Check for address bits stuck high */ \
		testOffset = 0; \
		(baseAddress)[testOffset] = (pattern_1scomp); \
		for (offset=1; (offset & (mask))!=0; offset<<=1) \
		{ \
			r = &(baseAddress)[offset];		/* fault location */ \
			if ((baseAddress)[offset] != (pattern)) goto done; \
		} \
		(baseAddress)[testOffset] = (pattern); \
		/* Check for address bits stuck low or shorted */ \
		for (testOffset=1; (testOffset & (mask))!=0; testOffset<<=1) \
		{ \
			(baseAddress)[testOffset] = (pattern_1scomp); \
			r = &(baseAddress)[testOffset];		/* fault location */ \
			if ((baseAddress)[0] != (pattern)) goto done; \
			for (offset=1; (offset & (mask))!=0; offset<<=1) \
			{ \
				r = &(baseAddress)[testOffset];		/* fault location */ \
				if (((baseAddress)[offset] != (pattern)) && (offset != testOffset)) goto done; \
			} \
			(baseAddress)[testOffset] = (pattern); \
		} \
		r = 0; \
    } while (0); \
    done: do {} while(0)

__attribute__((section(".rom.text")))
static unsigned _8bit_addr_test(paddr_t mem_loc, uint64_t mem_size)
{
	volatile uint8_t *p = (volatile uint8_t *)mem_loc;
	const paddr_t mask = (mem_size / sizeof(*p)) - 1;
	volatile uint8_t *r;

	ADDR_TEST(p, mask, 0xAAU, 0x55U, r);
	return (r == 0) ? mem_size : 0;
}

__attribute__((section(".rom.text")))
static unsigned _16bit_addr_test(paddr_t mem_loc, uint64_t mem_size)
{
	volatile uint16_t *p = (volatile uint16_t *)mem_loc;
	const paddr_t mask = (mem_size / sizeof(*p)) - 1;
	volatile uint16_t *r;

	ADDR_TEST(p, mask, 0xAAAAU, 0x5555U, r);
	return (r == 0) ? mem_size : 0;
}

__attribute__((section(".rom.text")))
static unsigned _32bit_addr_test(paddr_t mem_loc, uint64_t mem_size)
{
	volatile uint32_t *p = (volatile uint32_t *)mem_loc;
	const paddr_t mask = (mem_size / sizeof(*p)) - 1;
	volatile uint32_t *r;

	ADDR_TEST(p, mask, 0xAAAAAAAAU, 0x55555555U, r);
	return (r == 0) ? mem_size : 0;
}

__attribute__((section(".rom.text")))
static unsigned _64bit_addr_test(paddr_t mem_loc, uint64_t mem_size)
{
	volatile uint64_t *p = (volatile uint64_t *)mem_loc;
	const paddr_t mask = (mem_size / sizeof(*p)) - 1;
	volatile uint64_t *r;

	ADDR_TEST(p, mask, 0xAAAAAAAAAAAAAAAAULL, 0x5555555555555555ULL, r);
	return (r == 0) ? mem_size : 0;
}

/**********************************************************************
 *
 * Memory Device Test
 *
 * Test the integrity of a physical memory device by performing an
 * increment/decrement test over the entire region. In the process every storage
 * bit in the device is tested as a zero and a one. The base address and the
 * size of the region are selected by the caller.
 *
 * Returns:     NULL if the test succeeds.
 *
 *              A non-zero result is the first address at which an
 *              incorrect value was read back.  By examining the
 *              contents of memory, it may be possible to gather
 *              additional information about the problem.
 *
*/
#define DEVICE_TEST(baseAddress, num, pattern, pattern_1scomp, r) \
		do { \
			unsigned offset; \
			/* Fill memory with a known pattern */ \
			for ((pattern)=1, offset=0; offset<(num); (pattern)++, offset++) { \
				(baseAddress)[offset] = (pattern); \
			} \
			/* Check each location and invert it for the second pass */ \
			for ((pattern)=1, offset=0; offset<(num); (pattern)++, offset++) \
			{ \
				r = &(baseAddress)[offset];		/* fault location */ \
				if ((baseAddress)[offset] != (pattern)) goto done; \
				(pattern_1scomp) = ~(pattern); \
				(baseAddress)[offset] = (pattern_1scomp); \
			} \
			/* Check each location for the inverted pattern and zero it */ \
			for ((pattern)=1, offset=0; offset<(num); (pattern)++, offset++) \
			{ \
				(pattern_1scomp) = ~(pattern); \
				r = &(baseAddress)[offset];		/* fault location */ \
				if ((baseAddress)[offset] != (pattern_1scomp)) goto done; \
			} \
			r = 0; \
		} while (0); \
		done: do {} while(0)


__attribute__((section(".rom.text")))
static unsigned _8bit_dev_test(paddr_t mem_loc, uint64_t mem_size)
{
	volatile uint8_t *p = (volatile uint8_t *)mem_loc;
	const unsigned long num = mem_size / sizeof(*p);
	volatile uint8_t *r;
	uint8_t pattern, pattern_1scomp;

	DEVICE_TEST(p, num, pattern, pattern_1scomp, r);
	return (r == 0) ? mem_size : 0;
}

__attribute__((section(".rom.text")))
static unsigned _16bit_dev_test(paddr_t mem_loc, uint64_t mem_size)
{
	volatile uint16_t *p = (volatile uint16_t *)mem_loc;
	const unsigned long num = mem_size / sizeof(*p);
	volatile uint16_t *r;
	uint16_t pattern, pattern_1scomp;

	DEVICE_TEST(p, num, pattern, pattern_1scomp, r);
	return (r == 0) ? mem_size : 0;
}

__attribute__((section(".rom.text")))
static unsigned _32bit_dev_test(paddr_t mem_loc, uint64_t mem_size)
{
	volatile uint32_t *p = (volatile uint32_t *)mem_loc;
	const unsigned long num = mem_size / sizeof(*p);
	volatile uint32_t *r;
	uint32_t pattern, pattern_1scomp;

	DEVICE_TEST(p, num, pattern, pattern_1scomp, r);
	return (r == 0) ? mem_size : 0;
}

__attribute__((section(".rom.text")))
static unsigned _64bit_dev_test(paddr_t mem_loc, uint64_t mem_size)
{
	volatile uint64_t *p = (volatile uint64_t *)mem_loc;
	const unsigned long num = mem_size / sizeof(*p);
	volatile uint64_t *r;
	uint64_t pattern, pattern_1scomp;

	DEVICE_TEST(p, num, pattern, pattern_1scomp, r);
	return (r == 0) ? mem_size : 0;
}

/*******************************************************************************
 * _mem_test
 *
 * This is the test point entry routine. It will run <test_num> starting at
 * address <start> to address (<start> + <mem_size> - 1).
 *
 * The particular test to be run is indicated by <test_num> (numbered starting
 * at memtest_e_first) however if <test_num> is memtest_e_ALL then all tests
 * are run.
 *
 * Although all tests can be run over the specified memory range when
 * <test_num> == memtest_e_ALL, it is more useful to call this function with a
 * specific <test_num> so that upon error, the caller can provide some
 * indication as to what test failed and perhaps at which address.
 *
 * Returns: the amount of memory successfully tested. Since the caller knows
 * 			the range for the memory test and which test was being run, they
 * 			can deduce the specific address at which a problem occurs
 *
 * see mem_tests.h for some usage restrictions
*/
__attribute__((section(".rom.text")))
uint64_t _mem_test(const paddr_t start, const uint64_t mem_size, memtest_e test_num)
{
	/* array of tests (MUST be a stack variable) */
	struct
	{
		unsigned (*f)(paddr_t mem_loc, uint64_t mem_size);
	} _tests[] =
	{
		[memtest_e_8bit_1s_walk] = {_8bit_1s_walk},
		[memtest_e_16bit_1s_walk] = {_16bit_1s_walk},
		[memtest_e_32bit_1s_walk] = {_32bit_1s_walk},
		[memtest_e_64bit_1s_walk] = {_64bit_1s_walk},
		[memtest_e_8bit_addr_test] = {_8bit_addr_test},
		[memtest_e_16bit_addr_test] = {_16bit_addr_test},
		[memtest_e_32bit_addr_test] = {_32bit_addr_test},
		[memtest_e_64bit_addr_test] = {_64bit_addr_test},
		[memtest_e_8bit_dev_test] = {_8bit_dev_test},
		[memtest_e_16bit_dev_test] = {_16bit_dev_test},
		[memtest_e_32bit_dev_test] = {_32bit_dev_test},
		[memtest_e_64bit_dev_test] = {_64bit_dev_test},
	};
	unsigned i;
	const unsigned test_num_start = (test_num == memtest_e_ALL) ? memtest_e_first : test_num;
	const unsigned test_num_end = (test_num == memtest_e_ALL) ? NELEMENTS(_tests) - 1: test_num;
	const paddr_t last_mem_loc = start + mem_size;
	uint64_t bytes_tested = 0;

	/* it is sufficient to only check 'test_num_start' for a bogus test number */
	if ((test_num_start < memtest_e_first) || (test_num_start >= NELEMENTS(_tests))) return 0;

	for (i=test_num_start; i<=test_num_end; i++)
	{
		paddr_t cur_mem_loc = start;
		unsigned (*test_fn)(paddr_t, uint64_t) = _tests[i].f;

		if (test_fn != NULL)
		{
			bytes_tested = 0;
			while (cur_mem_loc < last_mem_loc)
			{
				unsigned n = test_fn(cur_mem_loc, mem_size);

				if (n == 0) break;
				else bytes_tested += n;

				cur_mem_loc += n;
			}
		}
	}
	return bytes_tested;
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/mem_tests.c $ $Rev: 711024 $")
#endif
