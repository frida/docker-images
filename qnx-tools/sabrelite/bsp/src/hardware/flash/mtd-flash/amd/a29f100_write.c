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





#include <sys/f3s_mtd.h>

/*
 * Summary
 *
 * MTD Version:     1 only
 * Bus Width:       8-bit and 16-bit
 * Buffered Writes: No
 *
 * Description
 *
 * This is the recommended MTDv1 write callout for non-buffered writes.
 */

int32_t f3s_a29f100_write(f3s_dbase_t * dbase,
                          f3s_access_t * access,
                          uint32_t flags,
                          uint32_t offset,
                          int32_t size,
                          uint8_t * buffer)
{
	/* Just a subset of the corresponding v2 function */
	return (f3s_a29f100_v2write(dbase, access, flags, offset, size, buffer));
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/amd/a29f100_write.c $ $Rev: 710521 $")
#endif
