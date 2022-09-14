/*
 * $QNXLicenseC:
 * Copyright 2013 QNX Software Systems.
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

	.text
	.align 2
	.globl arm_a8_dcache_flush_by_addr
	.globl arm_a8_enable_cache
	.type arm_a8_dcache_flush_by_addr, function
	.type arm_a8_enable_cache, function

/*
 * Specifically for Cortex-A8
 * Cache Type Register (MCR p16, 0, <Rd>, c0, c0, 1) DMinLine == 4: sixteen 32-bits word data line
 */
#define LINE_SHIFT	6
#define LINE_SIZE   (1 << LINE_SHIFT)

/*
 * Clean and invalidate D-Cache by address
 * r0: address
 * r1: number of bytes
 */
arm_a8_dcache_flush_by_addr:

	/* Trim the address to a cache line boundary */
	bic		r2, r0, #(LINE_SIZE - 1)

	/* Number of cache lines need to be flushed */
	and 	r3, r0, #(LINE_SIZE - 1)
	add		r1, r1, r3
	add 	r1, r1, #(LINE_SIZE - 1)
	mov		r1, r1, lsr #LINE_SHIFT
	mov		r0, r1

	/*
	* Clean and invalidate cache lines by address
	*/
1:  mcr     p15, 0, r2, c7, c14, 1      // Clean and invalidate data cache line by MVA
	add     r2, r2, #LINE_SIZE
	subs    r1, r1, #1
	bne     1b
	mcr     p15, 0, r1, c7, c10, 4      // DSB
	mov     pc, lr

arm_a8_enable_cache:
	stmfd   r13!, {r4, r12, r14}

	/* invalidation i-cache and d-cache before enabling them */
	bl      arm_v7_icache_invalidate
	bl      arm_v7_dcache_invalidate

    /* Enable L2 d-cache through AUXCR */
    mrc     p15, 0, r2, c1, c0, 1
    orr     r0, r2, #(1 << 1)   // set L2EN

    /* SMI call function ID 0x100: "write ACR by R0 value" */
    mov     r12, #0x100
    .word   0xe1600070

	/*
	 * Enable I/D cache
	 */
	mrc     p15, 0, r0, c1, c0, 0
	orr     r0, r0, #(1 << 12)      // enable I Cache
	orr     r0, r0, #(1 <<  2)      // enable D Cache
	mcr     p15, 0, r0, c1, c0, 0
	dsb
	isb

	ldmfd   r13!, {r4, r12, pc}


#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/arm/arm_a8_cache.S $ $Rev: 719559 $";
.previous
#endif
#endif 
