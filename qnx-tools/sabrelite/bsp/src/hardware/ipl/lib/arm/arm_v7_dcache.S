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
	.globl arm_v7_dcache_invalidate
	.globl arm_v7_dcache_flush
	.type arm_v7_dcache_invalidate, function
	.type arm_v7_dcache_flush, function

/*
 * Invalidate the whole D-Cache
 */
arm_v7_dcache_invalidate:
	stmfd   r13!, {r0-r5, r7, r9-r12, r14}
    mrc     p15, 1, r0, c0, c0, 1   /* read clidr  */
    ands    r3, r0, #0x7000000
    mov     r3, r3, lsr #23
    beq     5f
    mov     r10, #0
1:
    add     r2, r10, r10, lsr #1
    mov     r1, r0, lsr r2
    and     r1, r1, #7
    cmp     r1, #2
    blt     4f						/* skip if no cache, or i-cache only */
    mcr     p15, 2, r10, c0, c0, 0  /* write to Cache Size Selection Register */
    isb
    mrc     p15, 1, r1, c0, c0, 0   /* read csidr */
    and     r2, r1, #7
    add     r2, r2, #4
    ldr     r4, =0x3ff
    ands    r4, r4, r1, lsr #3
    clz     r5, r4
    ldr     r7, =0x7fff
    ands    r7, r7, r1, lsr #13
2:
    mov     r9, r4
3:
    orr     r11, r10, r9, lsl r5
    orr     r11, r11, r7, lsl r2
    mcr     p15, 0, r11, c7, c6, 2  /* invalidate d-cache by set/way */
    subs    r9, r9, #1
    bge     3b
    subs    r7, r7, #1
    bge     2b
4:
    add     r10, r10, #2
    cmp     r3, r10
    bgt     1b
5:
    mov     r10, #0
    mcr     p15, 2, r10, c0, c0, 0
    isb
	ldmfd   r13!, {r0-r5, r7, r9-r12, pc}

/*
 * Clean and invalidate the whole D-Cache
 */
arm_v7_dcache_flush:
	stmfd   r13!, {r0-r5, r7, r9-r12, r14}
    mrc     p15, 1, r0, c0, c0, 1	// read cache level ID register
    ands    r3, r0, #0x7000000		// level of coherency
    mov     r3, r3, lsr #23
    beq     5f						// No cache coherency issue, quit
    mov     r10, #0
1:
    add     r2, r10, r10, lsr #1
    mov     r1, r0, lsr r2
    and     r1, r1, #7
    cmp     r1, #2
    blt     4f						// Handle cache level 1-8. CLn < 2 implies no cache in level n 
    mcr     p15, 2, r10, c0, c0, 0  // write Cache Size Selection register, r10==0: CL1, r10==2: CL2 
    isb
    mrc     p15, 1, r1, c0, c0, 0	// read Cache Size Identification register
    and     r2, r1, #7				// Line size
    add     r2, r2, #4
    ldr     r4, =0x3ff
    ands    r4, r4, r1, lsr #3		// "r1 lsr #3": number of ways -1
    clz     r5, r4
    ldr     r7, =0x7fff
    ands    r7, r7, r1, lsr #13		// Number of sets
2:
    mov     r9, r4
3:
    orr     r11, r10, r9, lsl r5
    orr     r11, r11, r7, lsl r2
    mcr     p15, 0, r11, c7, c14, 2  // Clean and invalidate data cache line by set/way
    subs    r9, r9, #1
    bge     3b
    subs    r7, r7, #1
    bge     2b
4:
    add     r10, r10, #2
    cmp     r3, r10
    bgt     1b						// need to check next Cache level?
5:
    mov     r10, #0
    mcr     p15, 2, r10, c0, c0, 0
    isb
	ldmfd   r13!, {r0-r5, r7, r9-r12, pc}

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/arm/arm_v7_dcache.S $ $Rev: 729039 $";
.previous
#endif
#endif 
