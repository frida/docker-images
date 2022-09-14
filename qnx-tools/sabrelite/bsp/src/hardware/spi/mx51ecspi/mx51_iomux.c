/*
 * $QNXLicenseC: 
 * Copyright 2010, QNX Software Systems.  
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


#include "mx51_iomux.h"
#include "mx51.h"
#include <hw/inout.h>
#include <stdint.h>
#include <sys/srcversion.h>

/*
 * i.MX51 IOMUX Controller Routines
 *
 * The i.MX51 SOC has a fairly complex subsystem for pin multiplexing and pad
 * configuration. Various peripherals require that their pins be configured via
 * this interface. In addition, there are some board control registers in the
 * Peripheral Bus Controller CPLD of the i.MX51PDK board, which require additional 
 * configuration, before various peripherals can be used. These routines will 
 * configure the pad pin multiplexing, the pad configuration, and the board 
 * control register configuration, based on calls from routines which specify
 * the necessary configuration bits for a particular subsystem.
 */

/*
 * pinmux_set_swmux() - write the bits to configure a pad for a particular 
 * input / output configuration.
 */
void pinmux_set_swmux(uintptr_t iomuxc_base, int pin, int mux_config)
{
	unsigned	offset;

	offset = iomuxc_base + MX51_IOMUX_SWMUX + (pin * 4);
	out32(offset, mux_config);
}

/*
 * pinmux_set_padcfg() - configure the pin mux pads for drive strength, slew rate, etc.
 */
void pinmux_set_padcfg(uintptr_t iomuxc_base, int pin, int pad_config)
{
	unsigned	offset;

	offset = iomuxc_base + MX51_IOMUX_SWPAD + (pin * 4);
	out32(offset, pad_config);
}

/*
 * pinmux_set_input() - configure the pads for input path to module input ports.
 */
void pinmux_set_input(uintptr_t iomuxc_base, int pin, int input_config)
{
	unsigned	offset;

	offset = iomuxc_base + MX51_IOMUX_SWINPUT + (pin * 4);
	out32(offset, input_config);
}

__SRCVERSION("$URL$ $Rev$");
