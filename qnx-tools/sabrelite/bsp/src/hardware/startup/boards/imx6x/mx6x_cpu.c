/*
 * $QNXLicenseC: 
 * Copyright 2012 QNX Software Systems.  
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
#include "mx6x_startup.h"
#include <arm/mx6x.h>

#define CHIP_STRING_SIZE 30

uint32_t get_mx6_chip_rev()
{
	uint32_t chip_rev = in32(MX6X_ANATOP_BASE + MX6X_ANADIG_CHIP_INFO);
	chip_rev &= 0xff;
	return chip_rev;
}

uint32_t get_mx6_chip_type()
{
	uint32_t chip_id = in32(MX6X_ANATOP_BASE + MX6X_ANADIG_CHIP_INFO);
	chip_id >>= 16;
	return chip_id;
}
void print_chip_info()
{
	uint32_t chip_type = get_mx6_chip_type();
	uint32_t chip_rev = get_mx6_chip_rev();

	char chip_type_str[CHIP_STRING_SIZE];
	char chip_rev_str[CHIP_STRING_SIZE];

	switch(chip_type)
	{
		case MX6_CHIP_TYPE_QUAD_OR_DUAL:
			strcpy(chip_type_str, "Dual/Quad");
			break;

		case MX6_CHIP_TYPE_DUAL_LITE_OR_SOLO:
			strcpy(chip_type_str, "Solo/DualLite");
			break;
		default:
			strcpy(chip_type_str, "Unknown Variant");
			break;
	}

	switch(chip_rev)
	{
		case MX6_CHIP_REV_1_0:
			strcpy(chip_rev_str, "TO1.0");
			break;
		case MX6_CHIP_REV_1_1:
			strcpy(chip_rev_str, "TO1.1");
			break;
		case MX6_CHIP_REV_1_2:
			strcpy(chip_rev_str, "TO1.2");
			break;
		default:
			strcpy(chip_rev_str, "Unknown Revision");
			break;
	}
	kprintf("Detected i.MX6 %s, revision %s\n", chip_type_str, chip_rev_str);
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/mx6x_cpu.c $ $Rev: 729057 $")
#endif
