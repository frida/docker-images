/*
 * $QNXLicenseC: 
 * Copyright 2007, 2008, QNX Software Systems.  
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



/*
 * This is used to define our ipl memory copy mechanism 
*/

#include "ipl.h"
#include <string.h>


int copy_memory(unsigned long dest, unsigned long src, unsigned long sz) {

/*    Easiest way is to copy over 1 byte at a time ...
	while (sz > 0) {
		*(char*)dest = *(char*)src;
		dest++;
		src++;
		sz--;
	}
*/

//  More efficient to copy over a 4 bytes at a time

	short	  remainder;

	remainder = sz & 0x3;
	sz = sz >> 2;
	while (sz > 0) {
		*(long*)dest = *(long*)src;
		dest += 4;
		src += 4;
		sz--;
	}
	while (remainder > 0) {
		*(char*)dest = *(char*)src;
		dest++;
		src++;
		remainder--;
	}

	return(0);
}

/*
 All copying goes through here.  If paging/windowing is
 required then it will go through the utility functions
 above, otherwise copy_memory will just be called
*/
void copy (unsigned long dst, unsigned long src, unsigned long size) {
	copy_memory (dst, src, size);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/copy.c $ $Rev: 711024 $")
#endif
