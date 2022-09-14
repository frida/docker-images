/*
 * $QNXLicenseC:
 * Copyright 2012, QNX Software Systems.
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



#ifndef _VARIANT_H
#define _VARIANT_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <hw/inout.h>
#include <hw/spi-master.h>

#define DEVICE_IDENT        "SST 25VF016B Serial NOR Flash"
#define CLOCK_RATE			60000000	// frequency of ECSPI reference clock

/* SST 25VF016B supports 4KB sector, 32KB and 64 64KB block erase.
 * We only do erase on the 4KB sector basis to make life easy
 */
#define PAGE_SIZE			4096    // bytes
#define	SECTOR_SIZE			1		// number of pages in one sector
#define TOTAL_SIZE			512		// sectors
#define ADDR_LEN		  	3		// 24 bit address
#define CMD_LEN			  	1		// one byte commands

/* According to the i.MX6x Data Sheet (IMX6AEC) the minimum SCLK Cycle time for a read is 30ns,
 * therefore the max acceptable SCLK frequency is 33.333333MHz.
 */
#define SCLK_FREQ			30000000
#define SPI_MODE			0
#define DEVICE_ID			0x25
#define MAN_ID				0xBF
#define NUM_ERASE_UNITS     TOTAL_SIZE   // Number of sectors, this is the erase unit
#define PWR2_SIZE_UNIT      12      // Size of one sectors: 4K bytes
#define SPI_DEV				1	    // Slave Select 1


#define MX_ECSPI_BURST_MAX  0x100
#define MAX_BURST           MX_ECSPI_BURST_MAX   // the maximum number of bytes for a SPI data exchange

#define MX_ECSPI_BASE       0x02008000
#define MX_ECSPI_SIZE       0x4000
#define MX_ECSPI_IRQ        63

#endif

//each returns EOK if success, -1 on error, with errno set 
int spi_cmd_read(int fd, uint8_t cmd[4], uint8_t* buf, int len);
int spi_cmd_write(int fd, uint8_t cmd[4], uint8_t const* buf, int len);
int spi_nor_flash_write(int fd, uint8_t const* buf, int len);
int spi_write_1byte(int fd, uint8_t cmd);
int spi_word_exchange(int fd, uint32_t* buf);

__SRCVERSION("$URL$ $Rev$");
/*
 * $QNXLicenseC:
 * Copyright 2012, QNX Software Systems.
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



#ifndef _VARIANT_H
#define _VARIANT_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <hw/inout.h>
#include <hw/spi-master.h>

#define DEVICE_IDENT        "SST 25VF016B Serial NOR Flash"
#define CLOCK_RATE			60000000	// frequency of ECSPI reference clock

/* SST 25VF016B supports 4KB sector, 32KB and 64 64KB block erase.
 * We only do erase on the 4KB sector basis to make life easy
 */
#define PAGE_SIZE			4096    // bytes
#define	SECTOR_SIZE			1		// number of pages in one sector
#define TOTAL_SIZE			512		// sectors
#define ADDR_LEN		  	3		// 24 bit address
#define CMD_LEN			  	1		// one byte commands

/* According to the i.MX6x Data Sheet (IMX6AEC) the minimum SCLK Cycle time for a read is 30ns,
 * therefore the max acceptable SCLK frequency is 33.333333MHz.
 */
#define SCLK_FREQ			30000000
#define SPI_MODE			0
#define DEVICE_ID			0x25
#define MAN_ID				0xBF
#define NUM_ERASE_UNITS     TOTAL_SIZE   // Number of sectors, this is the erase unit
#define PWR2_SIZE_UNIT      12      // Size of one sectors: 4K bytes
#define SPI_DEV				1	    // Slave Select 1


#define MX_ECSPI_BURST_MAX  0x100
#define MAX_BURST           MX_ECSPI_BURST_MAX   // the maximum number of bytes for a SPI data exchange

#define MX_ECSPI_BASE       0x02008000
#define MX_ECSPI_SIZE       0x4000
#define MX_ECSPI_IRQ        63

#endif

//each returns EOK if success, -1 on error, with errno set 
int spi_cmd_read(int fd, uint8_t cmd[4], uint8_t* buf, int len);
int spi_cmd_write(int fd, uint8_t cmd[4], uint8_t const* buf, int len);
int spi_nor_flash_write(int fd, uint8_t const* buf, int len);
int spi_write_1byte(int fd, uint8_t cmd);
int spi_word_exchange(int fd, uint32_t* buf);

__SRCVERSION("$URL$ $Rev$");
