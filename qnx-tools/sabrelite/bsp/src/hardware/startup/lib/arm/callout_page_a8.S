#
# Copyright 2007, 2008, QNX Software Systems. 
# 
# Licensed under the Apache License, Version 2.0 (the "License"). You 
# may not reproduce, modify or distribute this software except in 
# compliance with the License. You may obtain a copy of the License 
# at: http://www.apache.org/licenses/LICENSE-2.0 
# 
# Unless required by applicable law or agreed to in writing, software 
# distributed under the License is distributed on an "AS IS" basis, 
# WITHOUT WARRANTIES OF ANY KIND, either express or implied.
#
# This file may contain contributions from others, either as 
# contributors under the License or as licensors under other terms.  
# Please review this entire file for other proprietary rights or license 
# notices, as well as the QNX Development Suite License Guide at 
# http://licensing.qnx.com/license-guide/ for other information.
#

/*
 * Cortex A8 specific page flush callouts
 */

#include "callout.ah"

#define LINE_SIZE	64

/*
 * int	page_flush(unsigned vaddr, int cache)
 *
 * This routine is called to flush the caches and TLBs for a single page.
 * vaddr. If the processor cannot flush cache lines or TLB entries by address,
 * those caches and TLBs can be flushed in page_flush_deferred()
 *
 * vaddr is the virtual address of the page
 * cache is non-zero if the cache needs to be flushed
 *
 * Returns 0 if all caches/TLBs were flushed.
 *         1 if the page_flush_deferred() callout needs to be called.
 */
CALLOUT_START(page_flush_a8, 0, 0)
	mov		r2, r0
	teq		r1, #0
	beq		1f									// only flush TLBs

	bic		r0, r0, #0xff						// kernel sets ASID in vaddr
	mov		r1, #4096							// page size
0:	mcr		p15, 0, r0, c7, c14, 1				// DCCIMVAC
	mcr		p15, 0, r0, c7, c5, 1				// ICIMVAU
	add		r0, r0, #LINE_SIZE					// next cache line
	subs	r1, r1, #LINE_SIZE
	bne		0b
	mcr		p15, 0, r1, c7, c10, 4				// DSB (deprecated encoding)

1:	// Note: kernel sets ASID in bottom 7 bits of address
	mcr		p15, 0, r2, c8, c7, 1				// DTLBIMVA
	mcr		p15, 0, r1, c7, c5, 6				// BPIALL
	mcr		p15, 0, r1, c7, c10, 4				// DSB (deprecated encoding)

	mov		r0, #0								// flush complete
	mov		pc, lr
CALLOUT_END(page_flush_a8)

/*
 * void	page_flush_deferred(int cache)
 *
 * This routine performs any cache/TLB flushing that could not be done in
 * the page_flush() callout.
 *
 * cache is non-zero if the cache needs to be flushed
 */
CALLOUT_START(page_flush_deferred_a8, 0, 0)
	mov		pc, lr
CALLOUT_END(page_flush_deferred_a8)

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/lib/arm/callout_page_a8.S $ $Rev: 728928 $";
.previous
#endif
#endif 
