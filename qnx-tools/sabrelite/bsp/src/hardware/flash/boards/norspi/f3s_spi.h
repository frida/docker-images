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


/*
 * This file contains definitions for SPI serial NOR flash driver callouts.
 */

#ifndef __F3S_SPI_H_INCLUDED
#define __F3S_SPI_H_INCLUDED

#include "spi_cmds.h"
#include <sys/f3s_mtd.h>

int32_t f3s_spi_open(f3s_socket_t *socket,
                     uint32_t flags);

uint8_t *f3s_spi_page(f3s_socket_t *socket,
                      uint32_t page,
                      uint32_t offset,
                      int32_t *size);

int32_t f3s_spi_read (f3s_dbase_t *dbase,
                    f3s_access_t *access,
                    uint32_t flags,
                    uint32_t text_offset,
                    int32_t buffer_size,
                    uint8_t *buffer);
int32_t f3s_spi_status(f3s_socket_t *socket,
                       uint32_t flags);

void f3s_spi_close(f3s_socket_t *socket,
                   uint32_t flags);

_int32 f3s_spi_ident(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

_int32 f3s_spi_write(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset,
			_int32 size, _uint8 * buffer);

int f3s_spi_erase(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

int32_t f3s_spi_sync (f3s_dbase_t *dbase, f3s_access_t *access,
                    uint32_t flags, uint32_t text_offset);


#endif /* __F3S_SPI_H_INCLUDED */


__SRCVERSION( "$URL$ $Rev$" )
