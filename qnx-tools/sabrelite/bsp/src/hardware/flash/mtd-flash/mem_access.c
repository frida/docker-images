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


#include <inttypes.h>
#include <sys/f3s_mtd.h>

/*
These collection of functions are used to allow us to 
deal with flash of all different widths. Once the correct
bus width is identified in f3s_flash_probe(), writemem() and
readmem() will point to the function of the corresponding width.

write_value() is called from the write when the value is ready to be 
writen down to flash. The reason for all the if()'s is that we don't
know the width of the flash when the function is being compiled.

send_command() is a wrapper for commands being sent to flash, it 
just makes it easier when dealing with some custom flash to override
all the commands. If this extra call is a concern you could always
just define it as writemem().
*/ 

void 
writemem8(volatile void *ptr, F3S_BASETYPE value)
{
	*(uint8_t *) ptr = value;
}

F3S_BASETYPE 
readmem8(volatile void *ptr)
{
	return (*(uint8_t *) ptr);
}

void 
writemem16(volatile void *ptr, F3S_BASETYPE value)
{
	*(uint16_t *) ptr = value;
}

F3S_BASETYPE 
readmem16(volatile void *ptr)
{
	return (*(uint16_t *) ptr);
}

void 
writemem32(volatile void *ptr, F3S_BASETYPE value)
{
	*(uint32_t *) ptr = value;
}

F3S_BASETYPE 
readmem32(volatile void *ptr)
{
	return (*(uint32_t *) ptr);
}

void 
writemem64(volatile void *ptr, F3S_BASETYPE value)
{
	*(uint64_t *) ptr = value;
}

F3S_BASETYPE 
readmem64(volatile void *ptr)
{
	return (*(uint64_t *) ptr);
}

void write_value(volatile void *ptr, intunion_t *value)
{
	if(flashcfg.bus_width == 1)
		writemem(ptr, value->w8);
	else if(flashcfg.bus_width == 2)
		writemem(ptr, value->w16);
	else if(flashcfg.bus_width == 4)
		writemem(ptr, value->w32);
	else if(flashcfg.bus_width == 8)
		writemem(ptr, value->w64);
}

void send_command(volatile void *ptr, F3S_BASETYPE command)
{
	writemem(ptr, (F3S_BASETYPE)(command * flashcfg.device_mult));
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/mem_access.c $ $Rev: 710521 $")
#endif
