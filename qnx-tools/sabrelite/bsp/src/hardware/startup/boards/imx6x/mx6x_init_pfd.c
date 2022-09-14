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

#include "startup.h"
#include <arm/mx6x.h>
#include "board.h"

/*
 * Gate clocks to unused components.
 */
void mx6x_init_pfd_gates(void)
{
    uint32_t reg;

    reg = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PFD_480);
    reg |= MX6X_ANATOP_PFD_ALL_CLOCK_GATES;
    out32(MX6X_ANATOP_BASE + MX6X_ANATOP_PFD_480_SET, reg);

    reg = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PFD_528);
    reg |=  MX6X_ANATOP_PFD_ALL_CLOCK_GATES;
    out32(MX6X_ANATOP_BASE + MX6X_ANATOP_PFD_528_SET, reg);

    reg = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PFD_480);
    reg |= MX6X_ANATOP_PFD_ALL_CLOCK_GATES;
    out32(MX6X_ANATOP_BASE + MX6X_ANATOP_PFD_480_CLR, reg);

    reg = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PFD_528);
    reg |= MX6X_ANATOP_PFD_ALL_CLOCK_GATES;
    out32(MX6X_ANATOP_BASE + MX6X_ANATOP_PFD_528_CLR, reg);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/mx6x_init_pfd.c $ $Rev: 729057 $")
#endif
