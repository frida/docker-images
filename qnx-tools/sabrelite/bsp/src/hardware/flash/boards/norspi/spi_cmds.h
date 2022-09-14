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

#ifndef SPI_CMDS_H_
#define SPI_CMDS_H_

#include <stdint.h>
#include "variant.h"

// test defines
#define KILO(__n) ((__n) << 10)
#define TOTAL_SIZE_BYTES	(TOTAL_SIZE * SECTOR_SIZE * PAGE_SIZE)

int iswriting(const int spid_fd);

int read_ident(
	const int spi_fd, 
	int* manufact_id, 
	int* device_id, 
	uint32_t* size);

int pd_release(const int spi_fd);

int sector_erase(
	const int spi_fd, 
	const int offset);

int page_program(
	const int spi_fd, 
	int offset, 
	int len, 
	uint8_t const* data);

int read_from(
	const int spi_fd, 
	int offset, 
	int len, 
	uint8_t* buffer);

#endif /* SPI_CMDS_H_ */


__SRCVERSION( "$URL$ $Rev$" )
