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


/*
 * arm_board_ttb.S
 *
 */

#define SDRAM_BASE  0x10000000
#define SDRAM_SIZE  (128 * 1024 * 1024)

    .data
    .globl  _arm_board_ttb

    /* Domain 15 required */
    /* Has to be aligned to 16K boundry */
    .align  14
_arm_board_ttb:
    /* Uncacheable */
    .set    XCount, (0x02 | (15 << 5) | (3 << 10))
    .rept   (SDRAM_BASE >> 20)
    .word   XCount
    .set    XCount, XCount + 0x00100000
    .endr

    /* SDRAM, cacheable and bufferable */
    .set    XCount, SDRAM_BASE | (0x02 | 0x0C | (15 << 5) | (3 << 10))
    .rept   (SDRAM_SIZE >> 20)
    .word   XCount
    .set    XCount, XCount + 0x00100000
    .endr

    /* Uncacheable */
    .set    XCount, (SDRAM_BASE + SDRAM_SIZE) | (0x02 | (15 << 5) | (3 << 10))
    .rept   (0x1000 - ((SDRAM_BASE + SDRAM_SIZE) >> 20))
    .word   XCount
    .set    XCount, XCount + 0x00100000
    .endr


/* __SRCVERSION( "$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/arm_board_ttb.S $ $Rev: 729057 $" ); */

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/arm_board_ttb.S $ $Rev: 729057 $";
.previous
#endif
#endif 
