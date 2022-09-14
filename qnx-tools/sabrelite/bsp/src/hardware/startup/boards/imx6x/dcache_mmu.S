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

	.globl 	dcache_mmu_enable
	.globl	dcache_mmu_disable
	
/*enable dcache and mmu*/
dcache_mmu_enable:
	mov 	r0, #0
	mcr 	p15, 0, r0, c2, c0, 2		// Always use TTBR0

	/*
	 * Set the translation table base
	 */
	ldr 	r0, =_arm_board_ttb 		// set start of Translation Table base
	orr 	r0, r0, #((3 << 3) | 3)
	mcr 	p15, 0, r0, c2, c0, 0

	/*
	 * Enable MMU domain 15
	 */
	mov 	r0, #(1 << 30)				// Client mode
	mcr 	p15, 0, r0, c3, c0, 0

	/*
	 * Enable the MMU, using read-modify-write to preserve reserved bits.
	 */
	mrc 	p15, 0, ip, c1, c0, 0
	orr 	ip, ip, #(1 << 12)			// enable I Cache
	orr 	ip, ip, #(1 <<	2)			// enable D Cache
	orr 	ip, ip, #(1 <<	0)			// enable MMU
	dsb
	isb
	mcr 	p15, 0, ip, c1, c0, 0
	dsb
	isb

	/*back to caller*/
	mov 	pc, lr

dcache_mmu_disable:
	/*
	 * disable MMU stuff and caches
	 */
	mrc p15, 0, ip, c1, c0, 0
	bic ip, ip, #0x00002000 @ clear bits 13 (--V-)
	bic ip, ip, #0x00000007 @ clear bits 2:0 (-CAM)
	dsb
	isb
	mcr p15, 0, ip, c1, c0, 0
	dsb
	isb

	mov 	ip, #0
	mov 	ip, ip
	mov 	ip, ip

	.align 5
	mcr p15, 0, ip, c8, c7, 0	// Invalidate entire unified TLB 
	mcr p15, 0, ip, c8, c6, 0	// Invalidate entire data TLB 
	mcr p15, 0, ip, c8, c5, 0	// Invalidate entire instruction TLB 
	dsb
	isb
	mcr 	p15, 0, ip, c7, c5, 0	// invalidate icache
	mcr 	p15, 0, ip, c7, c5, 6  // invalidate BP array  
	dsb
	isb


	/*back to caller*/
	mov 	pc, lr

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/dcache_mmu.S $ $Rev: 729057 $";
.previous
#endif
#endif 
