/*
 * $QNXLicenseC: 
 * Copyright 2012, QNX Software Systems.  
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


#ifndef __BOARD_H
#define __BOARD_H

#include <arm/mx6x_iomux.h>
#include <arm/mx6x.h>
#include "mx6x_startup.h"

#define MX6X_SDRAM_SIZE		1024

/* disabled clocks in CCGR1 */
#define MX6X_DISABLE_CLOCK_CCGR1 ( CCGR1_CG8_ESAI |	  \
								   CCGR1_CG4_ECSPI5 | \
								   CCGR1_CG3_ECSPI4 | \
								   CCGR1_CG2_ECSPI3 | \
								   CCGR1_CG1_ECSPI2)
/* disabled clocks in CCGR4 */
#define MX6X_DISABLE_CLOCK_CCGR4   CCGR4_CG0_PCIE

/* disabled clocks in CCGR5 */
#define MX6X_DISABLE_CLOCK_CCGR5 ( CCGR5_CG11_SSI3 | CCGR5_CG9_SSI1 | CCGR5_CG7_SPDIF )

/* disabled clocks in CCGR7 */
#define MX6X_DISABLE_CLOCK_CCGR7 (  CCGR6_CG2_USDHC2 | \
									CCGR6_CG1_USDHC1 )


#endif


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/sabrelite/board.h $ $Rev: 729057 $")
#endif
