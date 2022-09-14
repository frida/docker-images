/*
 * $QNXLicenseC:  
 * Copyright 2014, QNX Software Systems.
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

#include <ipl.h>

/*
 * Appends a startup_info_hdr to the end of the startup header of the image at
 * imageaddr.
 *
 * Returns: true (!0) or false (0) on success or failure respectively
 */
int image_add_info(void *imaddr, struct startup_info_hdr *info)
{
	struct startup_header	*sh      = (struct startup_header *)imaddr;
	unsigned long			ram_addr = sh->ram_paddr + sh->paddr_bias;
	struct startup_info_hdr	*sih;

	sih = (struct startup_info_hdr *)(ram_addr + ((char *)&sh->info[0] - (char *)sh));

	/* advance to the next free 'info' slot */
	while (sih->size) {
		sih = (struct startup_info_hdr *)((char *)sih + sih->size);
	}

	if ((((unsigned)sih - ram_addr) + info->size) > sizeof(struct startup_header)) {
		return 0;	// no room to add another info structure
	}
	copy((unsigned long)sih, (unsigned long)info, info->size);
	return 1;
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/image_add_info.c $ $Rev: 766062 $")
#endif
