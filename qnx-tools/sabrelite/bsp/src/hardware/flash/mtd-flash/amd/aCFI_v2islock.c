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
 * Summary
 *
 * MTD Version:    2 only
 * Bus Width:      8-bit, 16-bit and 8/16-bit hybrid
 * Locking Method: Persistent and Non-persistent
 *
 * Description
 *
 * Use this for Spansion flash capable of block locking. It requires that you use
 * the CFI ident callout.
 */

int f3s_aCFI_v2islock(f3s_dbase_t *dbase,
                      f3s_access_t *access,
                      uint32_t flags,
                      uint32_t offset)
{
	volatile void  *memory;
	F3S_BASETYPE		status;
	uint16_t		amd_cmd1, amd_cmd2;

	memory = access->service->page (&access->socket, F3S_POWER_ALL, offset & amd_command_mask, NULL);
	if (memory == NULL) return (errno);
	
	
	if (flashcfg.device_width == 1) {
		amd_cmd1 = AMD_CMD_ADDR1_W8;
		amd_cmd2 = AMD_CMD_ADDR2_W8;
	} else {
		amd_cmd1 = AMD_CMD_ADDR1_W16;
		amd_cmd2 = AMD_CMD_ADDR2_W16;
	}
	
	amd_cmd1 *= flashcfg.bus_width;
	amd_cmd2 *= flashcfg.bus_width;

	/* check that this chip supports Dynamic protection. */
	if (dbase->flags & F3S_PROTECT_DYN){
		/* Enter the Volatile Sector Protection Mode */
		send_command(memory + amd_cmd1, AMD_UNLOCK_CMD1);
		send_command(memory + amd_cmd2, AMD_UNLOCK_CMD2);
		send_command(memory + amd_cmd1, AMD_DYB_ENTER);

		/* Obtain lock status for the unit
		 * DQ1 is the lock status:
		 * 0 = locked
		 * 1 = not locked
		 */
		status = readmem(memory);
		status = readmem(memory); // read again to avoid garbage data in first read

		/* Leave Volatile Sector Protection Mode */
		send_command(memory, AMD_PROTECT_EXIT1);
		send_command(memory, AMD_PROTECT_EXIT2);
	
		/* Check for lock status */
		if (!(status & (0x01 * flashcfg.device_mult))){
			return (EROFS);
		}
	}



	/* The DYB bits aren't locked, check the PPB bits */

	if (dbase->flags & F3S_PROTECT_PERSISTENT){
		/* Enter the Persistent Sector Protection Mode */
		send_command(memory + amd_cmd1, AMD_UNLOCK_CMD1);
		send_command(memory + amd_cmd2, AMD_UNLOCK_CMD2);
		send_command(memory + amd_cmd1, AMD_PPB_ENTER);

		/* Obtain lock status for the unit
		 * DQ1 is the lock status:
		 * 0 = locked
		 * 1 = not locked
		 */
		status = readmem(memory);
		status = readmem(memory); // read again to avoid garbage data in first read

		/* Leave Persistent Sector Protection Mode */
		send_command(memory, AMD_PROTECT_EXIT1);
		send_command(memory, AMD_PROTECT_EXIT2);

	
		/* Check for lock status */
		if (!(status & (0x01 * flashcfg.device_mult))){
			return (EROFS);
		}
	}

	/* Either the chip doesn't support locking, or this sector is not locked */
	return (EOK);
	
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/amd/aCFI_v2islock.c $ $Rev: 710521 $")
#endif
