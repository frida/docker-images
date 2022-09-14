/*
 * $QNXLicenseC: 
 * Copyright 2007, 2011, 2012 QNX Software Systems.  
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
#include <arm/mx6x_iomux.h>
#include <arm/mx6x.h>

/*
 * i.MX6x IOMUX Controller Routines
 *
 * The i.MX6x SOCs have a fairly complex subsystem for pin multiplexing and pad
 * configuration. Various peripherals require that their pins be configured via
 * this interface. These routines will configure the pad pin multiplexing, the pad 
 * configuration, and the board control register configuration, based on calls from 
 * routines which specify the necessary configuration bits for a particular subsystem.
 */

/*
 * pinmux_set_swmux() - write the bits to configure a pad for a particular 
 * input / output configuration.
 */
void pinmux_set_swmux(int pin, int mux_config)
{
	unsigned offset;

	offset = -1;
	if ( get_mx6_chip_type() == MX6_CHIP_TYPE_QUAD_OR_DUAL)
	{
		offset = MX6X_IOMUXC_BASE + MX6X_IOMUX_SWMUX + (pin * 4);
	}
	else if (get_mx6_chip_type() == MX6_CHIP_TYPE_DUAL_LITE_OR_SOLO)
	{
		offset = MX6X_IOMUXC_BASE + MX6SDL_IOMUX_SWMUX + (pin * 4);	
	}
	
	out32(offset, mux_config);
}

/*
 * pinmux_set_padcfg() - configure the pin mux pads for drive strength, slew rate, etc.
 */
void pinmux_set_padcfg(int pin, int pad_config)
{
	unsigned offset;

	offset = -1;
	if ( get_mx6_chip_type() == MX6_CHIP_TYPE_QUAD_OR_DUAL)
	{
		offset = MX6X_IOMUXC_BASE + MX6X_IOMUX_SWPAD + (pin * 4);
	}
	else if (get_mx6_chip_type() == MX6_CHIP_TYPE_DUAL_LITE_OR_SOLO)
	{
		offset = MX6X_IOMUXC_BASE + MX6SDL_IOMUX_SWPAD + (pin * 4);
	}

	out32(offset, pad_config);
}

/*
 * pinmux_set_input() - configure the pads for input path to module input ports.
 */
void pinmux_set_input(int pin, int input_config)
{
	unsigned offset;

	offset = -1;

	if ( get_mx6_chip_type() == MX6_CHIP_TYPE_QUAD_OR_DUAL)
	{
		offset = MX6X_IOMUXC_BASE + MX6X_IOMUX_SWINPUT + (pin * 4);
	}
	else if (get_mx6_chip_type() == MX6_CHIP_TYPE_DUAL_LITE_OR_SOLO)
	{
		offset = MX6X_IOMUXC_BASE + MX6SDL_IOMUX_SWINPUT + (pin * 4);
	}
	
	out32(offset, input_config);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/mx6x_iomux.c $ $Rev: 729057 $")
#endif
