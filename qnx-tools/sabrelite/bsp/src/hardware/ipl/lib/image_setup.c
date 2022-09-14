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
 * image_setup: Setup an existing IPL header for execution
*/

#include "ipl.h"

int image_setup (unsigned long addr) {
	unsigned long	ram_addr;

	//
	// Copy the data from the address into our structure in memory
    //

	copy ((unsigned long)(&startup_hdr), addr, sizeof(startup_hdr));
	
	//
	// get ram_addr and patch startup with the images physical
	// location.  Startup will handle the rest ...
	//

	ram_addr = startup_hdr.ram_paddr + startup_hdr.paddr_bias;	
	startup_hdr.imagefs_paddr = addr + startup_hdr.startup_size - startup_hdr.paddr_bias;
    	
	//
	// Copy startup to ram_addr.
	//

	copy(ram_addr,(unsigned long)(&startup_hdr),sizeof(startup_hdr));

	copy ((ram_addr+sizeof(startup_hdr)),(addr+sizeof(startup_hdr)), (startup_hdr.startup_size - sizeof(startup_hdr)));
		
	//
	// All set now for image_start 
	//

	return(0);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/image_setup.c $ $Rev: 711024 $")
#endif
