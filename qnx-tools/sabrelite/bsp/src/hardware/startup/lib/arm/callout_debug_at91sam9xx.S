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
 * -----------------------------------------------------------------------
 * Routine to patch callout code
 *
 * On entry:
 *  r0     - physical address of syspage
 *  r1     - virtual  address of syspage
 *  r2     - offset from start of syspage to start of the callout routine
 *  r3     - offset from start of syspage to read/write data used by callout
 *  [sp]   - data registered with callout via callout_register_data()
 *  [sp+4] - address of callout definition (CALLOUT_START)
 * -----------------------------------------------------------------------
 */
patch_debug:
    stmdb       sp!,{r4,lr}
    add         r4, r0, r2                  // address of callout routine

    /*
     * Map DBUG control registers
     */
    ldr         r1, [sp, #8]				// debug_device[...]
    ldr	        r1, [r1, #DDI_BASE]

    mov         r0, #AT91_DBGU_SIZE  // size of reset control registers
/*    ldr         r1, Lpaddr */
    bl          callout_io_map32

    /* Patch the callout routine */
    CALLOUT_PATCH   r4, r0, r1, r2, ip

    ldmia       sp!,{r4,pc}


/* Lpaddr: .word   AT91_DBGU_BASE */

/*
 * -----------------------------------------------------------------------
 * void display_char_at91sam9xx(struct sypage_entry *, char)
 * -----------------------------------------------------------------------
 */
CALLOUT_START(display_char_at91sam9xx, 0, patch_debug)
    mov         ip,     #0x000000ff      // DGBU base address
    orr         ip, ip, #0x0000ff00
    orr         ip, ip, #0x00ff0000
    orr         ip, ip, #0xff000000

    /*
     * Wait for TXRDY
     */
0:  ldr         r3, [ip, #AT91_DBGU_SR]
    tst         r3, #AT91_DBGU_IE_SR_TXRDY
    beq         0b

    /*
     * Output character
     */
    str         r1, [ip, #AT91_DBGU_THR]
                
    mov         pc, lr
CALLOUT_END(display_char_at91sam9xx)


/*
 * -----------------------------------------------------------------------
 * char display_char_at91sam9xx(struct sypage_entry *)
 * -----------------------------------------------------------------------
 */
CALLOUT_START(poll_key_at91sam9xx, 0, patch_debug)
    mov         ip,     #0x000000ff      // DGBU base address
    orr         ip, ip, #0x0000ff00
    orr         ip, ip, #0x00ff0000
    orr         ip, ip, #0xff000000

    /*
     * If RXRDY, read character, otherwise return -1
     */
0:  ldr         r3, [ip, #AT91_DBGU_SR]
    tst         r3, #AT91_DBGU_IE_SR_RXRDY
    ldrne       r0, [ip, #AT91_DBGU_RHR]
    mvneq       r0, #1

    mov         pc, lr
CALLOUT_END(poll_key_at91sam9xx)


/*
 * -----------------------------------------------------------------------
 * int break_detect(struct syspage_entry *)
 *
 * Return 1 if break detected, 0 otherwise
 * -----------------------------------------------------------------------
 */
CALLOUT_START(break_detect_at91sam9xx, 0, patch_debug)
    mov         ip,     #0x000000ff     // DGBU base address
    orr         ip, ip, #0x0000ff00
    orr         ip, ip, #0x00ff0000
    orr         ip, ip, #0xff000000

    /*
     * Not capable of detecting break character, always return 0.
     */
    mov         r0, #0
    mov         pc, lr
CALLOUT_END(break_detect_at91sam9xx)

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/lib/arm/callout_debug_at91sam9xx.S $ $Rev: 740407 $";
.previous
#endif
#endif 
