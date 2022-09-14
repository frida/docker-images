/*
 * $QNXLicenseC:
 * Copyright 2012, QNX Software Systems. 
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

	.globl	smp_start_mx6x

smp_start_mx6x:
	/*
	 * Restore original reset vector
	 */
	ldr		r0, =startup_reset_vec_addr
	ldr		r1, =startup_reset_vec
	ldr		r1, [r1]
	str		r1, [r0]

	/*
	 * Caches are in an undefined state on cold reset so invalidate them
	 */
	mov		r0, #0
	mcr		p15, 2, r0, c0, c0, 0
	mcr		p15, 0, ip, c7, c5, 0
	mrc		p15, 1, r0, c0, c0, 0

	ldr		r1, =0x7fff
	and		r2, r1, r0, lsr #13

	ldr		r1, =0x3ff
	and		r3, r1, r0, lsr #3	// NumWays - 1
	add		r2, r2, #1			// NumSets

	and		r0, r0, #0x7
	add		r0, r0, #4			// SetShift

	clz		r1, r3				// WayShift
	add		r4, r3, #1			// NumWays
1:	sub		r2, r2, #1			// NumSets--
	mov		r3, r4				// Temp = NumWays
2:	subs	r3, r3, #1			// Temp--
	mov		r5, r3, lsl r1
	mov		r6, r2, lsl r0
	orr		r5, r5, r6			// Reg = (Temp<<WayShift)|(NumSets<<SetShift)
	mcr		p15, 0, r5, c7, c6, 2
	bgt		2b
	cmp		r2, #0
	bgt		1b
	dsb
	isb

	/*
	 * Jump to the real startup smp entry point
	 */
	ldr		r0, =startup_smp_start
	ldr		pc, [r0]

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/smp_start_mx6x.S $ $Rev: 729057 $";
.previous
#endif
#endif 
