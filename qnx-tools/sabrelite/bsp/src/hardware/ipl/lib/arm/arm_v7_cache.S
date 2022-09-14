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
	.globl arm_v7_enable_cache
	.type arm_v7_enable_cache, function

arm_v7_enable_cache:
 	stmfd   r13!, {r14}

	/* invalidation i-cache and d-cache before enabling them */
	bl      arm_v7_icache_invalidate
	bl      arm_v7_dcache_invalidate

	/*
	 * Enable I/D cache
	 */
	mrc     p15, 0, r0, c1, c0, 0
	orr     r0, r0, #(1 << 12)      // enable I Cache
	orr     r0, r0, #(1 <<  2)      // enable D Cache
	mcr     p15, 0, r0, c1, c0, 0
	dsb
	isb

 	ldmfd   r13!, {pc}

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/arm/arm_v7_cache.S $ $Rev: 720800 $";
.previous
#endif
#endif 
