/*
 * $QNXLicenseC: 
 * Copyright 2007, 2008, 2010, 2011 QNX Software Systems.  
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
 

/*
 * Freescale i.MX6x (i.MX6 Q, i.MX6 D, i.MX6 S) specific timer support
 */

#include "callout.ah"
#include <arm/mx6x.h>

/*
 * --------------------------------------------------------------------------
 * Routine to patch callout code
 *
 * On entry:
 *	r0 - physical address of syspage
 *	r1 - virtual  address of syspage
 *	r2 - offset from start of syspage to start of the callout routine
 *	r3 - offset from start of syspage to read/write data used by callout
 * --------------------------------------------------------------------------
 */
patch_timer:
	stmdb	sp!, {r4, lr}
	add		r4, r0, r2					// address of callout routine

	ldr		r0, Lvaddr
	cmp		r0, #-1
	bne		1f

	/*
	 * Map registers
	 */
	mov		r0, #0x20					// size of registers
	ldr		r1, Lpaddr
	bl		callout_io_map
	str		r0, Lvaddr

1:
	/*
	 * Patch the callout routine
	 */
	CALLOUT_PATCH	r4, r0, r1, r2, ip
	ldmia	sp!, {r4, pc}

Lpaddr:	.word	MX6X_EPIT1_BASE
Lvaddr:	.word	-1

/*
 * --------------------------------------------------------------------------
 * Set clock resolution, and enable interrupt triggering as a side effect.
 * The interrupt remains masked until it is enabled via intr_unmask_mx1
 *
 * On entry:
 *	r0 - pointer to syspage_entry
 *	r1 - pointer to qtime_entry
 * --------------------------------------------------------------------------
 */
CALLOUT_START(timer_load_mx6x, 0, patch_timer)
	/*
	 * Get the address of the timer registers (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	/*
	 * Disable Timer 
	 * Timer reload mode
	 * Compare interrupt enable
	 * EPIT enable mode
	 */
	ldr		r0, [ip, #MX6X_EPIT_CR]
	bic		r0, r0, #0x00000001
	orr		r0, r0, #0x0000000E
	str		r0, [ip, #MX6X_EPIT_CR]

	/*
	 * load counter value
	 */
	ldr		r2, [r1, #QT_TIMER_LOAD]
	str		r2, [ip, #MX6X_EPIT_LR]

	/*
	 * Compare counter is 0
	 */
	mov		r2, #0
	str		r2, [ip, #MX6X_EPIT_CMPR]

	/*
	 * Clear status register 
	 */
	mov		r2, #1
	str		r2, [ip, #MX6X_EPIT_SR]

	/*
	 * Restart Timer, Enable interrupt
	 */
	orr		r0, r0, #0x0F
	str		r0, [ip, #MX6X_EPIT_CR]

	mov		pc, lr
CALLOUT_END(timer_load_mx6x)


/*
 * --------------------------------------------------------------------------
 * Read the current timer value, relative to the last clock tick
 *
 * On entry:
 *	r0 - pointer to syspage_entry
 *	r1 - pointer to qtime_entry
 *
 * FIXME: this doesn't deal with the counter wrapping, eg. ClockCycles just
 *		  at the point where the clock interrupt is triggerred.
 * --------------------------------------------------------------------------
 */
CALLOUT_START(timer_value_mx6x, 0, patch_timer)
	/*
	 * Get the address of the timer registers (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	/*
	 * Return current timer counter value
	 */
	ldr		r0, [ip, #MX6X_EPIT_CNR]
	ldr		r1, [r1, #QT_TIMER_LOAD]
	sub		r0, r1, r0

	mov		pc, lr
CALLOUT_END(timer_value_mx6x)


/*
 * --------------------------------------------------------------------------
 * Clear timer interrupt.
 *
 * On entry:
 *	r0 - pointer to syspage_entry
 *	r1 - pointer to qtime_entry
 * --------------------------------------------------------------------------
 */
CALLOUT_START(timer_reload_mx6x, 0, patch_timer)
	/*
	 * Get the address of the timer registers (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	/*
	 * Clear status register 
	 */
	ldr		r0, [ip, #MX6X_EPIT_SR]
	str		r0, [ip, #MX6X_EPIT_SR]

	mov		pc, lr
CALLOUT_END(timer_reload_mx6x)

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/callout_timer_mx6x.S $ $Rev: 729057 $";
.previous
#endif
#endif 
