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
 * This is the write callout for SPI serial NOR flash.
 */

int32_t f3s_spi_write(f3s_dbase_t *dbase,
                         f3s_access_t *access,
                         uint32_t flags,
                         uint32_t offset,
                         int32_t size,
                         uint8_t *buffer)
{
	uint8_t verify[PAGE_SIZE];	// page_program maximum size
	int rc;

	if (access->service->page (&access->socket, 0, offset, &size) == NULL)
	{
		return -1;
	}

	rc = page_program((int)access->socket.socket_handle, offset, size, buffer);
	if (-1 == rc) {
		errno = EIO;
		return -1;
	}

	if (!(flags & F3S_VERIFY_WRITE)) {
		return rc;
	}

	// verify data was written correctly
	size = rc;
	int32_t xfer = rc;

	while(xfer) 
	{
		int nbytes = min(sizeof verify, xfer);

		rc = read_from((int)access->socket.socket_handle, offset, nbytes, verify);
		if (-1 == rc) {
			errno = EIO;			
			return -1;
		}

		if (memcmp(verify, buffer, rc))
		{
			fprintf(stderr, "(devf  t%d::%s:%d) program verify error\n"
						"between offset 0x%x and 0x%x, size = %d\n",
						pthread_self(), __func__, __LINE__,
						offset, offset + rc, rc);
			errno = EIO;
			return -1;
		}
		
		// adjust
		xfer -= rc;
		offset += rc;
		buffer += rc;
	}
	// verify successful
	return size;
}


__SRCVERSION( "$URL$ $Rev$" )
