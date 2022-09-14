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

// Module Description:  board specific interface

#include <sim_mmc.h>
#include <sim_mx35.h>
#include <arm/mx6x.h>

static int disable_cd = 0;

uintptr_t	gpio_base;
uint32_t	cpin, ppin;

static int mx6dq_detect(SIM_HBA *hba)
{
	SIM_MMC_EXT	*ext;
	mx35_ext_t	*mx35;
	uint32_t	status;

	if (disable_cd)
		return MMC_SUCCESS;
	
	ext  = (SIM_MMC_EXT *)hba->ext;
	mx35 = (mx35_ext_t *)ext->handle;

	status = in32(gpio_base + MX6X_GPIO_PSR);

	if(!((status >> cpin) & 1)){
		if (status & (1 << ppin))           // Write protected
		{
			ext->eflags |= MMC_EFLAG_WP;
		}	
		else
		{
			ext->eflags &= ~MMC_EFLAG_WP;
		}	
		return MMC_SUCCESS;
	}

	return MMC_FAILURE;
}

int bs_init(SIM_HBA *hba)
{
	CONFIG_INFO	*cfg;
	SIM_MMC_EXT	*ext;
	cfg = (CONFIG_INFO *)&hba->cfg;
	mx35_ext_t 	*mx35;

	// Ports 3,4 have CD and WP functionality
	if(cfg->IOPort_Base[0] == MX6X_USDHC3_BASE) {
		//port 1 uses GPIO7_0 as card detect and GPIO7_1 as write protect
		cpin = 0;
		ppin = 1;
		if ((gpio_base = mmap_device_io(0x100, MX6X_GPIO7_BASE))
			== (uintptr_t)MAP_FAILED) {
			slogf (_SLOGC_SIM_MMC, _SLOG_ERROR, "IMX6Q MMCSD: GPIO mmap_device_io failed");
			return MMC_FAILURE;
		}
	}
	else if (cfg->IOPort_Base[0] == MX6X_USDHC4_BASE) {
		//port 1 uses GPIO2_6 as card detect and GPIO2_7 as write protect
		cpin = 6;
		ppin = 7;
		if ((gpio_base = mmap_device_io(0x100, MX6X_GPIO2_BASE))
			== (uintptr_t)MAP_FAILED) {
			slogf (_SLOGC_SIM_MMC, _SLOG_ERROR, "IMX6Q MMCSD: GPIO mmap_device_io failed");
			return MMC_FAILURE;
		}	
	}
	else
		disable_cd = 1;

	if (mx35_attach(hba) != MMC_SUCCESS)
		return MMC_FAILURE;

	ext = (SIM_MMC_EXT *)hba->ext;
	ext->detect = mx6dq_detect;
	mx35 = (mx35_ext_t *)ext->handle;

	// overwrite reference clock value	
	mx35->clock = MX6X_USDHC_DEFAULT_CLOCK;
	
	// overwrite the SD Host Controller name
	strncpy(cfg->Description, "FREESCALE USDHC", sizeof(cfg->Description));

	return MMC_SUCCESS;
}

int bs_dinit(SIM_HBA *hba)
{
	SIM_MMC_EXT	*ext;
	mx35_ext_t	*mx35;

	ext  = (SIM_MMC_EXT *)hba->ext;
	mx35 = (mx35_ext_t *)ext->handle;

	return (CAM_SUCCESS);
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/devb/mmcsd/arm/mx6q-sabrelite.le.v7/sim_bs.c $ $Rev: 711024 $")
#endif
