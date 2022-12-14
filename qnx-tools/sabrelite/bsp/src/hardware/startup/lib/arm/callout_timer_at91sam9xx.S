/*
 * $QNXLicenseC:
 * Copyright 2009, QNX Software Systems. 
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


#include "callout.ah"
#include <arm/at91sam9xx.h>
/*
 * --------------------------------------------------------------------------
 * Routine to patch callout code
 *
 * On entry:
 *  r0 - physical address of syspage
 *  r1 - virtual  address of syspage
 *  r2 - offset from start of syspage to start of the callout routine
 *  r3 - offset from start of syspage to read/write data used by callout
 * --------------------------------------------------------------------------
 */
patch_timer:
    stmdb   sp!,{r4,lr}
    add     r4, r0, r2                  // address of callout routine

    /*
     * Map registers
     */
    mov     r0, #AT91_PIT_SIZE // size of registers
    ldr     r1, Lpaddr
    bl      callout_io_map32

    /*
     * Patch the callout routine
     */
    CALLOUT_PATCH   r4, r0, r1, r2, ip

    ldmia   sp!,{r4,pc}

Lpaddr: .word   AT91_PIT_BASE

/*
 * --------------------------------------------------------------------------
 * Set clock resolution, and enable interrupt triggering as a side effect.
 * The interrupt remains masked until it is enabled via intr_unmask_at91sam9xx
 *
 * On entry:
 *  r0 - pointer to syspage_entry
 *  r1 - pointer to qtime_entry
 * --------------------------------------------------------------------------
 */
CALLOUT_START(timer_load_at91sam9xx, 0, patch_timer)
    /*
     * Get the address of the timer registers (patched)
     */
    mov     ip,     #0x000000ff
    orr     ip, ip, #0x0000ff00
    orr     ip, ip, #0x00ff0000
    orr     ip, ip, #0xff000000

    /*
     *  start the timer running
     */
    mov     r0, #3
    mov     r0, r0, lsl #24
    ldr     r1, [r1, #QT_TIMER_LOAD]
    add     r0, r0, r1
    str     r0, [ip, #AT91_PIT_MR]

    mov     pc, lr
CALLOUT_END(timer_load_at91sam9xx)


/*
 * --------------------------------------------------------------------------
 * Read the current timer value, relative to the last clock tick
 *
 * On entry:
 *  r0 - pointer to syspage_entry
 *  r1 - pointer to qtime_entry
 *
 * NOTE: If the counter wraps, eg. ClockCycles just at the point where the
 *       clock interrupt is triggerred, we simply return the wrapped counter
 *       because the wrap is taken care of in the ClockCycles.S arm code. 
 * --------------------------------------------------------------------------
 */
CALLOUT_START(timer_value_at91sam9xx, 0, patch_timer)
    /*
     * Get the address of the timer registers (patched)
     */
    mov     ip,     #0x000000ff
    orr     ip, ip, #0x0000ff00
    orr     ip, ip, #0x00ff0000
    orr     ip, ip, #0xff000000

    /*
     * Latch the current timer value and mask out the overflow counter
     */
    ldr     r0, [ip, #AT91_PIT_PIIR]

    /*
     * The upper 12 bits are not part of the value. To save one instruction,
     * we will only clear the 8 least significant bits of the 12 because we know 
     * the value of the upper 12 bits will never be greater than 255 (in fact it 
     * should never be > 1).
     */
    bic     r0, r0, #0x0ff00000

    mov     pc, lr
CALLOUT_END(timer_value_at91sam9xx)


/*
 * --------------------------------------------------------------------------
 * Clear timer interrupt.
 *
 * On entry:
 *  r0 - pointer to syspage_entry
 *  r1 - pointer to qtime_entry
 * --------------------------------------------------------------------------
 */
CALLOUT_START(timer_reload_at91sam9xx, 0, patch_timer)
    /*
     * Get the address of the timer registers (patched)
     */
    mov     ip,     #0x000000ff
    orr     ip, ip, #0x0000ff00
    orr     ip, ip, #0x00ff0000
    orr     ip, ip, #0xff000000

    /* Return the b0 bit of PITS; 0: not a timer IRQ, 1: timer IRQ */
    ldr     r0, [ip, #AT91_PIT_SR]
    and     r0, r0, #1

    /* If not a timer IRQ, jump to the end, otherwise clear interrupt */
    teq     r0, #1
    bne     1f

    /* Read PIVR register to the clear interrupt */
    ldr     r1, [ip, #AT91_PIT_PIVR]
    
1:
    /* We don't need to reload and enable PIT as it just continues running */
    mov     pc, lr
CALLOUT_END(timer_reload_at91sam9xx)

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/lib/arm/callout_timer_at91sam9xx.S $ $Rev: 740407 $";
.previous
#endif
#endif 
