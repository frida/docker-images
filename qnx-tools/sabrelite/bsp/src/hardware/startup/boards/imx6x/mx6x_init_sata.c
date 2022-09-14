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
#include "board.h"

/* SATA init code, returns TRUE upon success, FALSE upon failure */
int mx6x_init_sata(uint32_t freq_enet)
{
	uint32_t reg_val;
	int i;

	// Clear Power down bit for PLL8
	reg_val = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL8_ENET);
	reg_val &= ~ANATOP_PLL8_ENET_POWERDOWN;
	out32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL8_ENET, reg_val);

	// Enable PLL8
	reg_val = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL8_ENET);
	reg_val |= ANATOP_PLL8_ENET_ENABLE;
	out32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL8_ENET, reg_val);
    reg_val &= ~ANATOP_PLL8_ENET_REF_MASK;
	reg_val |= freq_enet;
	out32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL8_ENET, reg_val);

	// Ensure PLL8 is locked.
	i = MAX_PLL8_LOCK_TIME_IN_US;
	while (i--)
	{   
		if (in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL8_ENET) & ANATOP_PLL8_ENET_LOCK) break;
		mx6x_usleep(1);
	}
	if (i <= 0)
	{   
		kprintf("Unable to lock PLL8 (ENET PLL8) during SATA init, value of PLL8 lock register was: 0x%x\n", in32(MX6X_ANATOP_BASE + ANATOP_PLL8_ENET_LOCK));
		return FALSE;
	}
	
	// Disable bypass
	reg_val = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL8_ENET);
	reg_val &= ~ANATOP_PLL8_ENET_BYPASS;
	out32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL8_ENET, reg_val); 
	
	// Enable Ref_SATA, frequency is 100MHz and is not adjustable
	reg_val = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL8_ENET);
	reg_val |= ANATOP_PLL8_ENET_ENABLE_SATA;
	out32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL8_ENET, reg_val);
	
	// Enable SATA clock
	out32(MX6X_CCM_BASE + MX6X_CCM_CCGR5, in32(MX6X_CCM_BASE + MX6X_CCM_CCGR5) | CCGR5_CG2_SATA);

	// Set PHY parameters
	reg_val = in32(MX6X_IOMUXC_BASE + MX6X_IOMUX_GPR13);
	out32(MX6X_IOMUXC_BASE + MX6X_IOMUX_GPR13, ((reg_val & ~0x07FFFFFD) | 0x0593A044));

	// Enable SATA_PHY PLL
	reg_val = in32(MX6X_IOMUXC_BASE + MX6X_IOMUX_GPR13);
	out32(MX6X_IOMUXC_BASE + MX6X_IOMUX_GPR13, ((reg_val & ~0x2) | 0x2));

	mx6x_usleep(100);

	// Support Staggered spin up
	out32(MX6X_SATA_BASE + MX6X_SATA_CAP, in32(MX6X_SATA_BASE + MX6X_SATA_CAP) | MX6X_SATA_CAP_SSS);

	// Ports implemented
	out32(MX6X_SATA_BASE + MX6X_SATA_PI, in32(MX6X_SATA_BASE + MX6X_SATA_PI) | 0x1);

	i = 500;

	// Reset HBA
	out32(MX6X_SATA_BASE + MX6X_SATA_GHC, MX6X_SATA_GHC_RESET);
	while (i--)
	{
		if (!(in32(MX6X_SATA_BASE + MX6X_SATA_GHC) & MX6X_SATA_GHC_RESET)) break;
	}
	if (i <= 0)
	{
		kprintf("Unable to reset HBA");
		return FALSE;
	}

	// write to TIMER1MS
	out32(MX6X_SATA_BASE + MX6X_TIMER1MS, (AHB_CLOCK/1000)); 
	return TRUE;
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/mx6x_init_sata.c $ $Rev: 729057 $")
#endif
