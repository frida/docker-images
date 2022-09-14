/*
 * $QNXLicenseC:
 * Copyright 2008, QNX Software Systems. 
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


#include <sys/f3s_mtd.h>

/*
This function is used when a write doesn't span the entire bus width,
so bytes either at the front or back of the value must be masked in so
the write doesn't corrupt other data already on the device.
*/

int pad_value(intunion_t *value, volatile void *memory, unsigned shift, int front)
{
	intunion_t current, mask;

	/* read current from flash */
	memcpy((void *) &current, (void *) memory, flashcfg.bus_width);

	/* set mask */
	/* this if() decides if we mask out the bytes in the front or back */
	if(!front) 
	{
#if defined (__BIGENDIAN__)
		if (flashcfg.bus_width == 2)
			mask.w16 = (_uint16)F3S_A29F040_CLEAR >> shift;
		else if(flashcfg.bus_width == 4)
			mask.w32 = (_uint32)F3S_A29F040_CLEAR >> shift;
		else if(flashcfg.bus_width == 8)
			mask.w64 = (_uint64)F3S_A29F040_CLEAR >> shift;
#else
		if (flashcfg.bus_width == 2)
			mask.w16 = (_uint16)F3S_A29F040_CLEAR << shift;
		else if(flashcfg.bus_width == 4)
			mask.w32 = (_uint32)F3S_A29F040_CLEAR << shift;
		else if(flashcfg.bus_width == 8)
			mask.w64 = (_uint64)F3S_A29F040_CLEAR << shift;
#endif
	}
	else
	{
#if defined (__BIGENDIAN__)
		if (flashcfg.bus_width == 2)
			mask.w16 = (_uint16)F3S_A29F040_CLEAR << shift;
		else if(flashcfg.bus_width == 4)
			mask.w32 = (_uint32)F3S_A29F040_CLEAR << shift;
		else if(flashcfg.bus_width == 8)
			mask.w64 = (_uint64)F3S_A29F040_CLEAR << shift;
#else
		if (flashcfg.bus_width == 2)
			mask.w16 = (_uint16)F3S_A29F040_CLEAR >> shift;
		else if(flashcfg.bus_width == 4)
			mask.w32 = (_uint32)F3S_A29F040_CLEAR >> shift;
		else if(flashcfg.bus_width == 8)
			mask.w64 = (_uint64)F3S_A29F040_CLEAR >> shift;
#endif
	}

	/* apply mask to value */
	if (flashcfg.bus_width == 2)
	{
		value->w16 &= ~mask.w16;
		value->w16 |= mask.w16 & current.w16;
	}
	else if(flashcfg.bus_width == 4)
	{
		value->w32 &= ~mask.w32;
		value->w32 |= mask.w32 & current.w32;
	}
	else if(flashcfg.bus_width == 8)
	{
		value->w64 &= ~mask.w64;
		value->w64 |= mask.w64 & current.w64;
	}

	return(1);
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/pad_value.c $ $Rev: 710521 $")
#endif
