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
/*
 * IXP1200 specific reboot
 *
 * This should be usable by any board that uses an IXP1200.
 */

#include "callout.ah"

/*
 * -----------------------------------------------------------------------
 * Routine to patch callout code
 *
 * On entry:
 *	r0 - physical address of syspage
 *	r1 - virtual  address of syspage
 *	r2 - offset from start of syspage to start of the callout routine
 *	r3 - offset from start of syspage to read/write data used by callout
 * -----------------------------------------------------------------------
 */
patch_reboot:
	stmdb	sp!,{r4,lr}
	add		r4, r0, r2					// address of callout routine

	/*
	 * Map interrupt controller registers
	 */
	mov		r0, #4						// size of reset register
	ldr		r1, Lreset
	bl		callout_io_map32
	CALLOUT_PATCH	r4, r0, r1, r2, ip

	ldmia	sp!,{r4,pc}

Lreset:	.word	IXP1200_PCI_CFG_BASE + IXP1200_IXP1200_RESET

CALLOUT_START(reboot_ixp1200, 0, patch_reboot)
	mov		ip,     #0x000000ff		// reset controller base (patched)
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	mvn		r0, #0
	str		r0, [ip]
0:	b		0b
CALLOUT_END(reboot_ixp1200)

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/lib/arm/callout_reboot_ixp1200.S $ $Rev: 740407 $";
.previous
#endif
#endif 
