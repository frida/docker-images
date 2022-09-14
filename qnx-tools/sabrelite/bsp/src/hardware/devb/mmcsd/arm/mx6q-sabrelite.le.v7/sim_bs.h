/*
 * $QNXLicenseC: 
 * Copyright 2011, QNX Software Systems.  
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

// Module Description:  board specific header file


#ifndef _BS_H_INCLUDED
#define _BS_H_INCLUDED

#define MX6X_USDHC_DEFAULT_CLOCK 198000000

// add new chipset externs here
#define MMCSD_VENDOR_FREESCALE_MX35
#define IMX_USDHC

int bs_init(SIM_HBA *hba);
int bs_dinit(SIM_HBA *hba);




#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/devb/mmcsd/arm/mx6q-sabrelite.le.v7/sim_bs.h $ $Rev: 711024 $")
#endif
