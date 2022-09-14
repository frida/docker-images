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
#include "mx6x_epit.h"
#include "mx6x_startup.h"

//#define DEBUG_BOOT_TIMING

extern void sdma_load_ifs(uint32_t dst, uint32_t src, int size);
extern void mx6x_mmu_disable_flushcache();

void
load_ifs(paddr32_t ifs_paddr) {
	int			comp;
	paddr32_t	src;
	
#if defined(DEBUG_BOOT_TIMING)
	unsigned int timerVal;
	mx6_epit_timer_init();
	timerVal = mx6_epit_get_timer_val();
#endif
	if (!shdr || !shdr->image_paddr || !shdr->startup_size) {
		crash("startup: shdr: %x, image_paddr: %x, startup_size: %x\nImage header corrupt\n",
				shdr, shdr->image_paddr, shdr->startup_size);
	}

	comp = shdr->flags1 & STARTUP_HDR_FLAGS1_COMPRESS_MASK;
	if(comp != 0) {
		src = shdr->imagefs_paddr;
		if(src == 0) {
 			/* If imagefs_paddr is zero, then we were loaded by some code
			 * that did not do an image_setup(). This was probably because
			 * we were loaded by a non-neutrino IPL (or an emulator).
			 *
			 * We'll copy the compressed FS out of the way and then
			 * uncompress it back to where it belongs. Since we just
			 * need the temporary memory for a short period of time,
			 * we won't bother actually allocating it.
			 *
			 */
			src = find_ram(shdr->stored_size, sizeof(uint64_t), 0, 0);
			if (enable_sdma_copy)
				sdma_load_ifs(src, ifs_paddr, shdr->stored_size);
			else
				copy_memory(src, ifs_paddr, shdr->stored_size);
		}
		uncompress(comp, ifs_paddr, src);
		
		if (debug_flag > 0) kprintf("done\n");
	} else if((shdr->imagefs_paddr != 0) &&
		(shdr->imagefs_paddr != ifs_paddr)) {

		if (enable_sdma_copy)
			sdma_load_ifs(ifs_paddr, shdr->imagefs_paddr, shdr->imagefs_size);
		else
			copy_memory(ifs_paddr, shdr->imagefs_paddr, shdr->imagefs_size);

	}

	mx6x_mmu_disable_flushcache();
	
#if defined(DEBUG_BOOT_TIMING)
	mx6_epit_print_delta(timerVal, mx6_epit_get_timer_val());
#endif
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/load_ifs.c $ $Rev: 729057 $")
#endif
