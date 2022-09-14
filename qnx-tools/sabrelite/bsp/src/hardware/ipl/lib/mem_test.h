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

#ifndef __MEM_TEST_H__
#define __MEM_TEST_H__

#ifndef NELEMENTS
#define NELEMENTS(x)	(sizeof((x)) / sizeof((x)[0]))
#endif	/* NELEMENTS */

#ifndef KILO	/* if KILO is defined, the others like aren't either */
#define KILO(x)	((unsigned)((x) * 1024))
#define MEG(x)	((unsigned)((x) * KILO(1024)))
#define GIG(x)	((unsigned)((x) * MEG(1024)))
#endif	/* KILO */


typedef enum
{
memtest_e_ALL = -1,
memtest_e_first = 1,

	memtest_e_8bit_1s_walk = memtest_e_first,
	memtest_e_16bit_1s_walk,
	memtest_e_32bit_1s_walk,
	memtest_e_64bit_1s_walk,

	memtest_e_8bit_addr_test,
	memtest_e_16bit_addr_test,
	memtest_e_32bit_addr_test,
	memtest_e_64bit_addr_test,

	memtest_e_8bit_dev_test,
	memtest_e_16bit_dev_test,
	memtest_e_32bit_dev_test,
	memtest_e_64bit_dev_test,

memtest_e_last = memtest_e_64bit_addr_test,
} memtest_e;

/*
 * this function is used to invoke each of the above memory test algorithms from
 * board specific memory test functions. The board specific memory test is
 * entered from the functions defined in mem_test_entry.c and from there, the
 * board specific tests to be executed and the regions in which to test can be
 * uniquely constructed with multiple calls to _mem_test() selecting the test
 * algorithm and region to test.
 *
 * << IMPORTANT >>
 *
 * It is the callers responsibility to ensure that alignment and access size
 * restrictions are met for the hardware on which the tests are to be run.
 *
 * For example, it you choose <test_num> memtest_e_32bit_1s_walk, each memory
 * location accessed will be done so in 4 byte (32bit) transactions (or possibly
 * larger if a cache on test is done). If you provide a <start> address which is
 * not 32bit aligned and the hardware does not support unaligned accesses then
 * you are probably going to fault.
 *
 * Similarly, even if your hardware supports unaligned accesses and you choose
 * to do so, then the size of each access must be taken into consideration when
 * selecting the <mem_size> parameter. For example using the same
 * memtest_e_32bit_1s_walk test num and an 16 bit aligned <start> address, you
 * may (depending on how much of the available memory you are testing) need to
 * reduce the <mem_size> parameter by, in this example, 4 bytes so that the test
 * does not access memory beyond <start> + <mem_size> - 1
 */
uint64_t _mem_test(const paddr_t start, const uint64_t mem_size, memtest_e test_num);


#endif	/* __MEM_TEST_H__ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/mem_test.h $ $Rev: 711024 $")
#endif
