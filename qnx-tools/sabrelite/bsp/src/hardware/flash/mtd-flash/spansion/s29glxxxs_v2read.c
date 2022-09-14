/*
 * $QNXLicenseC: 
 * Copyright 2007, QNX Software Systems.  
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


#include <pthread.h>
#include <sys/f3s_mtd.h>

/*
 * Summary
 *
 * MTD Version: 2 only
 * Bus Width:   8-bit and 16-bit
 * Note:        Derived from the AMD CFI driver
 *
 */

int f3s_s29glxxxs_v2read (f3s_dbase_t *dbase, f3s_access_t *access,
                        uint32_t flags, uint32_t offset, int32_t size,
                        uint8_t *buffer)
{
	uint8_t *memory;
	int32_t togo;
	int32_t xfer;

	/* Obtain pointer to the sector */
	memory = access->service->page(&access->socket, F3S_POWER_ALL, offset, NULL);
	if (memory == NULL) {
		if (verbose) {
			fprintf(stderr, "(devf  t%d::%s:%d) page() returned NULL for offset 0x%x\n",
					pthread_self(), __func__, __LINE__, offset);
		}
		return (-1);
	}

	/* Spansion dummy read on first sector. 
	 *   Note: alignment should be considered.
	 */
	readmem((uint8_t *)(((uintptr_t)(memory) & ~(flashcfg.bus_width - 1))));

	/* Read the data in first sector */
	xfer = access->socket.unit_size - (((uintptr_t)memory) & (access->socket.unit_size - 1));
	togo = size;
	xfer = min(xfer, togo);
	memcpy(buffer, memory, xfer);

	/* Read data in subsequent sectors if necessary */
	togo -= xfer;
	while (togo) {
		memory += xfer;
		buffer += xfer;

		xfer = min(togo, access->socket.unit_size);
		readmem(memory); // Spansion dummy read workaround
		memcpy (buffer, memory, xfer);
		togo -= xfer;
	}

	return (size);
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION( "$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/spansion/s29glxxxs_v2read.c $ $Rev: 738496 $" );
#endif
