/*
 * $QNXLicenseC:
 * Copyright 2014, QNX Software Systems. 
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

#include "startup.h"


#if (_PADDR_BITS-0) == 64

/*
 * ARMv7 page table entries extended for LPAE usage
 * based on v7mp
 *
 * It has been pointed out that we don't really need these structure anymore
 * As of v7 the encodings are fixed values.
 *
 * LPAE uses different values and even fewer of the fields.
 *
 */

const struct armv_pte	armv_pte_lpae = {
	ARM_LPAE_DESC_NG | ARM_LPAE_DESC_AP_ALL_RO,	 // upte_ro; non-global
	ARM_LPAE_DESC_NG | ARM_LPAE_DESC_AP_ALL_RW,  // upte_rw; non-global
	ARM_LPAE_DESC_AP_PRIV_RO,                    // kpte_ro
	ARM_LPAE_DESC_AP_PRIV_RW,                    // kpte_rw
	0,	                                         // mask_nc
	0,                                           // l1_pgtable
	ARM_LPAE_DESC_AP_PRIV_RO,                    // kscn_ro  block not section 
	ARM_LPAE_DESC_AP_PRIV_RW,                    // kscn_rw
	0                                            // kscn_cb
};


#endif



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/lib/arm/armv_pte_lpae.c $ $Rev: 740407 $")
#endif
