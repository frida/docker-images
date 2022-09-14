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




#include <pthread.h>
#include <sys/f3s_mtd.h>

/*
 * Summary
 *
 * MTD Version:    2 only
 * Bus Width:      8-bit, 16-bit and 8/16-bit hybrid
 *
 * Description
 *
 * Use this for spansion flash capable of block locking.
 */

int f3s_aCFI_v2lock(f3s_dbase_t *dbase,
                    f3s_access_t *access,
                    uint32_t flags,
                    uint32_t offset)
{
	/* Are the capability bits set? */
	if (dbase->flags & F3S_PROTECT_PERSISTENT) {
		return (f3s_aCFI_v2plock(dbase, access, flags, offset));
	}

	if (dbase->flags & F3S_PROTECT_DYN) {
		return (f3s_aCFI_v2dlock(dbase, access, flags, offset));
	}

	/* nope, not supported */
	return ENOTSUP;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/amd/aCFI_v2lock.c $ $Rev: 710521 $")
#endif
