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




#include <pthread.h>
#include <sys/f3s_mtd.h>

/*
 * Description
 *
 * This is not a valid MTD callout. For internal use by other MTD callouts.
 */

int intel_read_status(F3S_BASETYPE status)
{
	/* check for vpp range error */
	if (status & (0x08 * flashcfg.device_mult))
	{
		fprintf(stderr, "(devf  t%d::%s:%d) program vpp error\n", pthread_self(), __func__, __LINE__);
		errno = EIO;
		return (-1);
	}

	/* check for device protect error */
	if (status & (0x02 * flashcfg.device_mult))
	{
		fprintf(stderr, "(devf  t%d::%s:%d) program protect error\n", pthread_self(), __func__, __LINE__);
		errno = EROFS;
		return (-1);
	}

	/* check for command sequence error */
	if ((status & (0x30 * flashcfg.device_mult)) == (0x30 * flashcfg.device_mult))
	{
		fprintf(stderr, "(devf  t%d::%s:%d) write sequence error\n", pthread_self(), __func__, __LINE__);
		errno = EINVAL;
		return (-1);
	}

	/* check for program error */
	if (status & (0x10 * flashcfg.device_mult))
	{
		fprintf(stderr, "(devf  t%d::%s:%d) program error\n", pthread_self(), __func__, __LINE__);
		errno = EIO;
		return -1;
	}
	
	/* check for block erase error */
	if (status & (0x20 * flashcfg.device_mult))
	{
		fprintf(stderr, "(devf  t%d::%s:%d) erase block error\n", pthread_self(), __func__, __LINE__);
		errno = EIO;
		return -1;
	}

	/* no errors */
	return (1);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/intel/intel_read_status.c $ $Rev: 710521 $")
#endif
