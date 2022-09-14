/*
 * $QNXLicenseC: 
 * Copyright 2010, QNX Software Systems.  
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


#include "f3s_spi.h"

/*
 * This is the sync callout for SPI serial NOR flash.
 */

int32_t f3s_spi_sync (f3s_dbase_t *dbase,
                    f3s_access_t *access,
                    uint32_t flags,
                    uint32_t text_offset)
{
	int rc;

	if (access->service->page (&access->socket, 0, text_offset, NULL) == NULL)
	{
		return (ERANGE);
	}

	rc = iswriting((int)access->socket.socket_handle);
	if (-1 == rc) {
		return (EIO);
	}
	if (1 == rc) {
		// write in progress
		return (EAGAIN);
	}
	return (EOK);
}


__SRCVERSION( "$URL$ $Rev$" )


