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

#include "ipl.h"
#include "sdmmc.h"
#include "fat-fs.h"
#include "sdhc_mx6x.h"
#include "ipl_mx6x.h"
#include <hw/inout.h>

//#define SKIP_IMAGE_SCAN
#define QNX_LOAD_ADDR   0x16000000


extern  fs_info_t       fs_info;

extern	void init_aips();
extern	void init_clocks();
extern	void init_pinmux();
extern	void init_serial_mx6q();
extern	void enableTZASC(unsigned tzasc_base_addr);

void delay(unsigned dly)
{
	volatile int j;

	while (dly--) {
		for (j = 0; j < 32; j++)
			;
	}
}

static inline int
sdmmc_load_file (unsigned address, const char *fn)
{
	mx6x_sdmmc_t	sdmmc;
	int				status;
	unsigned		reg;

	/*
	 * Initialize the SDMMC interface
	 */
	reg= in32(MX6X_GPIO2_BASE+MX6X_GPIO_PSR);
	if(!(reg & (1<<6))){ //card in top slot use that as default
		sdmmc.sdmmc_pbase = MX6X_USDHC4_BASE;      // SDMMC base address
	}else{
		//try bottom slot
		reg = in32(MX6X_GPIO7_BASE+MX6X_GPIO_PSR);
		if(!(reg & 0x1)){
			sdmmc.sdmmc_pbase = MX6X_USDHC3_BASE;      // SDMMC base address
		}else{
			return SDMMC_ERROR;
		}
	}
	
	/* initialize the sdmmc interface and card */
	if (SDMMC_OK != sdmmc_init_ctrl(&sdmmc)) {
		return SDMMC_ERROR;
	}

	if (sdmmc_init_card(&sdmmc)) {
		return SDMMC_ERROR;
	}

	ser_putstr("Load QNX image from SDMMC...\n");
	if (fat_read_mbr(&sdmmc, 0) != 0) {
		return SDMMC_ERROR;
	}

    status = fat_copy_named_file((unsigned char *)address, (char *)fn);

	sdmmc_fini(&sdmmc);

	return status;
}

int main()
{
	unsigned image = QNX_LOAD_ADDR;

	init_aips();

	init_clocks();

	init_pinmux();

	/* Init serial interface */
	init_serial_mx6q();

#if defined (VARIANT_enableTZASC)
	enableTZASC(MX6X_TZASC1);
#endif

	ser_putstr("\nWelcome to QNX Neutrino Initial Program Loader for Freescale i.mx6 Q Sabre-Lite (ARM Cortex-A9 MPCore)\n");

	ser_putstr("ARM TrustZone ASC is ");
#if defined (VARIANT_enableTZASC)
	ser_putstr("enabled");
#else
	ser_putstr("disabled");
#endif
	ser_putstr("\nIPL compiled ");
	ser_putstr(__DATE__);
	ser_putstr(" ");
	ser_putstr(__TIME__);
	ser_putstr("\n");

	//try boot from SD first
	if (sdmmc_load_file(image, "QNX-IFS") == SDMMC_OK){
#ifndef SKIP_IMAGE_SCAN 
		image = image_scan(image, image + 0x200);
#else
		image = image + 0x8;
#endif
		if (image != 0xffffffff) {
			ser_putstr("Found image               @ 0x");
			ser_puthex(image);
			ser_putstr("\n");
			image_setup(image);

			ser_putstr("Jumping to startup        @ 0x");
			ser_puthex(startup_hdr.startup_vaddr);
			ser_putstr("\n\n");
			image_start(image);

			/* Never reach here */
			return 0;
		}

	}

	while (1) {
		ser_putstr("Command:\n");
		ser_putstr("Press 'D' for serial download, using the 'sendnto' utility\n");
		ser_putstr("Press 'M' for SDMMC download, IFS filename MUST be 'QNX-IFS'.\n");
		switch (ser_getchar()) {
		case 'D':
		case 'd':
			ser_putstr("send image now...\n");
			if (image_download_ser(image)) {
				ser_putstr("download failed...\n");
				continue;
			}   
			else
				ser_putstr("download OK...\n");
			break;
		case 'M': case 'm':
			if (sdmmc_load_file(image, "QNX-IFS") == 0)
				break;

				ser_putstr("load image failed\n");
				continue;
		default:
			continue;
		}

		image = image_scan(image, image + 0x200);

		if (image != 0xffffffff) {
			ser_putstr("Found image               @ 0x");
			ser_puthex(image);
			ser_putstr("\n");
			image_setup(image);

			ser_putstr("Jumping to startup        @ 0x");
			ser_puthex(startup_hdr.startup_vaddr);
			ser_putstr("\n\n");
			image_start(image);

			/* Never reach here */
			return 0;
		}

		ser_putstr("Image_scan failed...\n");
	}

	return 0;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/boards/mx6q-sabrelite/main.c $ $Rev: 740617 $")
#endif
