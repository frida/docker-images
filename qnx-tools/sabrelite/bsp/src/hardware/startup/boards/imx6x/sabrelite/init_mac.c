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
#include <arm/mx6x_iomux.h>
#include <arm/mx6x.h>

#define MX6X_OCOTP_MAC0		0x620
#define MX6X_OCOTP_MAC1		0x630

#define MX6X_ENET_PALR		0xE4
#define MX6X_ENET_PAUR		0xE8
/*
 * mx6q_init_mac reads the MAC address programmed into the MAC0, MAC1 eFuses
 * and writes this MAC address to the ENET MAC address registers
 */
void mx6q_init_mac(void)
{
	unsigned int fuse_val, mac_low, mac_high;
	unsigned char mac_addr[6];

#if 1
	// QEMU leaves the fuse values zeroed
	mac_addr[0] = 0x04;
	mac_addr[1] = 0x13;
	mac_addr[2] = 0x37;
	mac_addr[3] = 0x42;
	mac_addr[4] = 0x24;
	mac_addr[5] = 0x77;
#else
	// read lower 32 bits of MAC address
	fuse_val = in32(MX6X_OCOTP_BASE + MX6X_OCOTP_MAC0);
	mac_addr[5] = fuse_val & 0xff;
	mac_addr[4] = (fuse_val >> 8) & 0xff;
	mac_addr[3] = (fuse_val >> 16) & 0xff;
	mac_addr[2] = (fuse_val >> 24) & 0xff;

	// read upper 16 bits of MAC address
	fuse_val = in32(MX6X_OCOTP_BASE + MX6X_OCOTP_MAC1);
	mac_addr[1] = fuse_val & 0xff;
	mac_addr[0] = (fuse_val >> 8) & 0xff;
#endif

	// program lower 32 bits of MAC addr into ENET
	mac_low = ((mac_addr[0] << 24) + (mac_addr[1] << 16) + (mac_addr[2] << 8) + mac_addr[3]);
	out32(MX6X_FEC_BASE + MX6X_ENET_PALR, mac_low);

	// program upper 16 bits of MAC addr into ENET
	mac_high = ((mac_addr[4] << 24) + (mac_addr[5] << 16) + 0x8808);
	out32(MX6X_FEC_BASE + MX6X_ENET_PAUR, mac_high);
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/sabrelite/init_mac.c $ $Rev: 729057 $")
#endif
