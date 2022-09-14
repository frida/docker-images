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
 * This is the open callout for the SPI serial NOR flash driver.
 */

int32_t f3s_spi_open(f3s_socket_t *socket,
                     uint32_t flags)
{
	static int			fd = 0;

	/* check if not initialized */
	if (!fd)
	{
		socket->name = (unsigned char*)"SPI serial flash";

		fd = spi_open("/dev/spi0");
		spi_cfg_t cfg;
		cfg.mode =  SPI_MODE;
		cfg.clock_rate = CLOCK_RATE;
		spi_setcfg(fd, SPI_DEV, &cfg);
		socket->window_size = socket->array_size = TOTAL_SIZE_BYTES;
		socket->socket_handle = (void *)fd;
	}

	return (EOK);
}


__SRCVERSION( "$URL$ $Rev$" )
