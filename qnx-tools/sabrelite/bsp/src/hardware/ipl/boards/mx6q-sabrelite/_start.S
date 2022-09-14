/*
 * $QNXLicenseC: 
 * Copyright 2011, QNX Software Systems.  
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

/* invalidate the D-CACHE */
.macro inv_dcache
    mov     r0, #0
    mcr     p15, 2, r0, c0, c0, 0  /* cache size selection register, select dcache */
    mrc     p15, 1, r0, c0, c0, 0  /* cache size ID register */
    mov     r0, r0, ASR #13
    ldr     r3, =0xfff
    and     r0, r0, r3
    cmp     r0, #0x7f
    moveq   r6, #0x1000
    beq     size_done
    cmp     r0, #0xff
    moveq   r6, #0x2000
    movne   r6, #0x4000

size_done:
    mov     r2, #0
    mov     r3, #0x40000000
    mov     r4, #0x80000000
    mov     r5, #0xc0000000

d_inv_loop:
    mcr     p15, 0, r2, c7, c6, 2  /* invalidate dcache by set / way */
    mcr     p15, 0, r3, c7, c6, 2  /* invalidate dcache by set / way */
    mcr     p15, 0, r4, c7, c6, 2  /* invalidate dcache by set / way */
    mcr     p15, 0, r5, c7, c6, 2  /* invalidate dcache by set / way */
    add     r2, r2, #0x20
    add     r3, r3, #0x20
    add     r4, r4, #0x20
    add     r5, r5, #0x20

    cmp     r2, r6
    bne     d_inv_loop
.endm

/*
 Disable L2Cache because ROM turn it on when uboot use plug-in.
 If L2Cache is on default, there are cache coherence problem if kernel have
 not config L2Cache.
*/
.macro init_l2cc
    ldr     r1, =0xa02000
    ldr     r0, =0x0
    str     r0, [r1, #0x100]
.endm /* init_l2cc */

.globl _start

_start:
    /*
     * Set the CPU to SVC32 mode
     */
    mrs    r0, cpsr
    bic    r0, r0, #0x1f
    orr    r0, r0, #0xd3
    msr    cpsr,r0

    /*
     * Invalidate L1 I/D and TLBs
     */
    mov    r0, #0                   @ set up for MCR
    mcr    p15, 0, r0, c8, c7, 0    @ invalidate TLBs
    mcr    p15, 0, r0, c7, c5, 0    @ invalidate icache

    /*
     * Disable MMU and Caches
     */
    mrc    p15, 0, r0, c1, c0, 0
    bic    r0, r0, #0x00002000    @ clear bits 13 (--V-)
    bic    r0, r0, #0x00000007    @ clear bits 2:0 (-CAM)
    orr    r0, r0, #0x00000002    @ set bit 1 (--A-) Align
    orr    r0, r0, #0x00000800    @ set bit 12 (Z---) BTB
    mcr    p15, 0, r0, c1, c0, 0

    /*
    * Enableing ICache
    */
    mrc        p15, 0, ip, c1, c0, 0
    orr        ip, ip, #(1 << 12)
    mcr        p15, 0, ip, c1, c0, 0
    dsb
    isb
    mov        r0, r0
    mov        r0, r0
    mov        r0, r0
    mov        r0, r0

    inv_dcache
    init_l2cc

    /* Setup the Stack */
    ldr        sp, =_STACK_TOP

    bl        main

oops:
    b        oops

