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
	.globl arm_enable_mmu
	.type arm_enable_mmu, function

/*
 * r0: the address holds the translation table which is supposed
 * to have been filled properly
 * Assume both i-cache and d-cache are clean
 */
arm_enable_mmu:

    mov     r1, #0
    mcr     p15, 0, r1, c2, c0, 2       // Always use TTBR0

    /*
     * Set the translation table base
     */
    orr     r0, r0, #((3 << 3) | 3)
    mcr     p15, 0, r0, c2, c0, 0

    /*
     * Enable MMU domain 15
     */
    mov     r0, #(1 << 30)              // Client mode
    mcr     p15, 0, r0, c3, c0, 0

    /* Enable I/D cache, Enable MMU*/
    mrc     p15, 0, r0, c1, c0, 0
    orr     r0, r0, #(1 <<  0)      // enable MMU
    mcr     p15, 0, r0, c1, c0, 0
    dsb
    isb

	mov		pc, lr


#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/arm/arm_mmu.S $ $Rev: 719559 $";
.previous
#endif
#endif 
