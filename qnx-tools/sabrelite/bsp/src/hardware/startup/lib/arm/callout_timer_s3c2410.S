/*
 * $QNXLicenseC:
 * Copyright 2008, QNX Software Systems. 
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
 * Samsung S3C2410 timer support
 */

#include "callout.ah"
#include <arm/s3c2410.h> 

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
	stmdb	sp!,{r4,lr}
	add		r4, r0, r2					// address of callout routine

	/*
	 * Map registers
	 */
	mov		r0, #S3C2410_TIMER_SIZE	// size of registers
	ldr		r1, Lpaddr
	bl		callout_io_map32

	/*
	 * Patch the callout routine
	 */
	CALLOUT_PATCH	r4, r0, r1, r2, ip
	ldmia	sp!,{r4,pc}

Lpaddr:	.word	S3C2410_TIMER_BASE

/*
 * --------------------------------------------------------------------------
 * Set clock resolution, and enable interrupt triggering as a side effect.
 * The interrupt remains masked until it is enabled via intr_unmask_s3c2410
 *
 * On entry:
 *	r0 - pointer to syspage_entry
 *	r1 - pointer to qtime_entry
 * --------------------------------------------------------------------------
 */
CALLOUT_START(timer_load_s3c2410, 0, patch_timer)
	/*
	 * Get the address of the timer registers (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	/*
	 * Trim the load value to 0xffff
	 */
	ldr		r0, [r1, #QT_TIMER_LOAD]
	bic		r0, r0, #0x00ff0000
	bic		r0, r0, #0xff000000
	str		r0, [r1, #QT_TIMER_LOAD]

	/*
	 * load counter value
	 */
	str		r0, [ip, #S3C2410_TCNTB4]

	/*
	 * start the timer
	 */
	ldr		r0, [ip, #S3C2410_TCON]
	orr		r0, r0, #0x00600000		/* set auto-reload and update */
	str		r0, [ip, #S3C2410_TCON]
	bic		r0, r0, #0x00200000		/* clear update bit */
	str		r0, [ip, #S3C2410_TCON]
	orr		r0, r0, #0x00100000		/* set start bit */
	str		r0, [ip, #S3C2410_TCON]


	mov		pc, lr
CALLOUT_END(timer_load_s3c2410)


/*
 * --------------------------------------------------------------------------
 * Read the current timer value, relative to the last clock tick
 *
 * On entry:
 *	r0 - pointer to syspage_entry
 *	r1 - pointer to qtime_entry
 *
 * NOTE : this doesn't deal with the counter wrapping, eg. ClockCycles just
 *		  at the point where the clock interrupt is triggerred.
 * --------------------------------------------------------------------------
 */
CALLOUT_START(timer_value_s3c2410, 0, patch_timer)
	/*
	 * Get the address of the timer registers (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	/*
	 * Latch the current timer value and return qtp->timer_load - value
	 */
	ldr		r0, [ip, #S3C2410_TCNTO4]
	ldr		r1, [r1, #QT_TIMER_LOAD]
	sub		r0, r1, r0

	mov		pc, lr
CALLOUT_END(timer_value_s3c2410)


/*
 * --------------------------------------------------------------------------
 * Clear timer interrupt.
 *
 * On entry:
 *	r0 - pointer to syspage_entry
 *	r1 - pointer to qtime_entry
 * --------------------------------------------------------------------------
 */
CALLOUT_START(timer_reload_s3c2410, 0, patch_timer)
	/*
	 * Get the address of the timer registers (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	/*
	 * we have automatic timer reload, so nothing to do here
	 */

	mov		pc, lr
CALLOUT_END(timer_reload_s3c2410)

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/lib/arm/callout_timer_s3c2410.S $ $Rev: 740407 $";
.previous
#endif
#endif 
