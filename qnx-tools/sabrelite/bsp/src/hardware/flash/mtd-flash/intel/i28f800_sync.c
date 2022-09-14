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
 * Summary
 *
 * MTD Version: 1 only
 * Bus Width:   8-bit, 16-bit and 8/16-bit hybrid
 * Boot-Block?: Yes
 *
 * Description
 *
 * This sync callout is for non-uniform block flash (ie boot-block flash).
 * This is the only MTDv1 API for non-uniform block flash.
 */

int32_t f3s_i28f800_sync(f3s_dbase_t *dbase,
                         f3s_access_t *access,
                         uint32_t flags,
                         uint32_t offset,
                         int32_t size)
{
	int32_t done, geo_size;
	uint32_t geo_lo, geo_hi, geo_index;
	uint32_t status;
	volatile void *memory;

	memory= access->service->page(&access->socket, F3S_POWER_ALL, offset, NULL);
	if (!memory)
	{
		fprintf(stderr, "(devf  t%d::%s:%d) page() returned NULL for offset 0x%x\n",
					pthread_self(), __func__, __LINE__, offset);
		return -1;
	}

	/* issue read status command */
	send_command(memory,INTEL_READ_STATUS);

	/* check for erase operation completion */
	done=((readmem(memory) &(0x80*flashcfg.device_mult))==(0x80*flashcfg.device_mult));

	if (done)
	{
		status=readmem(memory);
		if(intel_read_status(status) == -1)
			return(-1);	

		geo_hi=0;
		while (geo_hi<=offset)
		{
			/* find size of current block */
			for (geo_index=0; geo_index<dbase->geo_num; geo_index++)
			{	
				geo_lo=geo_hi;
				geo_hi+=dbase->geo_vect[geo_index].unit_num<<
				 dbase->geo_vect[geo_index].unit_pow2;

				if (offset>=geo_lo && offset<geo_hi)
				{
  					geo_size=1<<dbase->geo_vect[geo_index].unit_pow2;
					if (size<=geo_size)/* go back to read mode */
						send_command(memory,INTEL_READ_ARRAY);
					return geo_size;
				}
			}
			offset-=geo_hi;
			geo_hi=0;
		}
		/* if we ever get here, this is bad */
		fprintf(stderr, "(devf  t%d::%s:%d) inconsistant geometry. offset = 0x%x\n",
					pthread_self(), __func__, __LINE__, offset);
		return -1;
	}
/* erase has not finished */
return 0;
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/intel/i28f800_sync.c $ $Rev: 710521 $")
#endif
