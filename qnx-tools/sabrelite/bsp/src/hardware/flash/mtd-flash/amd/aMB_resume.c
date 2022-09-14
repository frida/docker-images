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
 * MTD Version: 1 only
 * Bus Width:   8-bit and 16-bit
 * Note:        Converts f3s_aMB_v2resume() to version 1 API
 *
 * Description
 *
 * This resume callout is for early generation MirrorBit parts that have
 * several known defects. Please contact your AMD representative for more
 * details. The f3s_aMB_*() callouts implement software workarounds to these
 * errata, they should be used as a matched set.
 */

void f3s_aMB_resume(f3s_dbase_t *dbase,
                    f3s_access_t *access,
                    uint32_t flags,
                    uint32_t offset)
{
	/* Just a subset of the corresponding v2 function */
	f3s_aMB_v2resume(dbase, access, flags, offset);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/amd/aMB_resume.c $ $Rev: 710521 $")
#endif
