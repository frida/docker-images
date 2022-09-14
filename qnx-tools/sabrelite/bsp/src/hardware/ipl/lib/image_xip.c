/*
 * $QNXLicenseC: 
 * Copyright 2013, QNX Software Systems. 
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

#include "ipl.h"
#include <hw/inout.h>
#include <stdlib.h>
#include <image_xip.h>

image_xip_t *xip_dev;

static void dump_data(unsigned long dst, unsigned long src, int len)
{
	unsigned *p1 = (unsigned *)dst;
	unsigned *p2 = (unsigned *)src;
	int i;

	len = len >> 2;
	for (i=0; i<len; i++, p1++, p2++) {
		if (*p1 != *p2) {
			ser_putstr("offset 0x");
			ser_puthex(src + i * sizeof(unsigned));
			ser_putstr(" src 0x");
			ser_puthex(*p2);
			ser_putstr(" dst 0x");
			ser_puthex(*p1);
			ser_putstr("\n");
		}
	}
}

unsigned long xip_image_load(unsigned long dst, unsigned long src)
{
	unsigned long addr;
	unsigned long ram_addr = -1L;
	struct startup_header *hdr;

	/* It has no reason why startup header is not within 16 bytes */
	addr = image_scan_2(src, src + 0x10, 0);

	/* Found valid image */
	if (addr != -1L) {

		/* The desired address of startup */
		ram_addr = startup_hdr.ram_paddr + startup_hdr.paddr_bias;	

		if (startup_hdr.flags1 & STARTUP_HDR_FLAGS1_COMPRESS_MASK) {
			/* 
			 * For compressed image, load startup directly to where it belongs to
			 * while load imagefs to different offset so that startup can uncompress it
			 * and place it to the final location
			 */
		} else {
			/* Load uncompressed image directly to the final address */
			dst = ram_addr + startup_hdr.startup_size - startup_hdr.paddr_bias;
		}

		/* Load startup code */
		if (xip_dev->xip_load(xip_dev->ext, ram_addr, addr, startup_hdr.startup_size, xip_dev->verbose)) {
			ser_putstr("startup checksum error\n");
			if (xip_dev->verbose > 1) {
				dump_data(ram_addr, addr, startup_hdr.startup_size);
			}
			ser_putstr("\n");
			return (-1L);
		}

		/* Load imagefs, it may or may not be consecutive to startup code */
		if (xip_dev->xip_load(xip_dev->ext, dst, addr + startup_hdr.startup_size, startup_hdr.stored_size - startup_hdr.startup_size, xip_dev->verbose)) {
			ser_putstr("imagefs checksum error\n");
			ser_putstr("\n");
			if (xip_dev->verbose > 1) {
				dump_data(dst, addr + startup_hdr.startup_size, startup_hdr.stored_size - startup_hdr.startup_size);
			}
			return (-1L);
		}

		/* Everything went well, setup the image */
		hdr = (struct startup_header *)(ram_addr);
		hdr->imagefs_paddr = dst;
	}

	return (ram_addr);
}

int xip_image_init(image_xip_t *dev)
{
	xip_dev = dev;

	/* We need the callback to the do the actual load work */
	if (!xip_dev->xip_load)
		return -1;

	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/image_xip.c $ $Rev: 723411 $")
#endif
