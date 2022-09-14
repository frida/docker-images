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


#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "spi_cmds.h"

///Instruction set
#define CMD_WREN 		0x06		// Write Enable 0000 0110 06h 0 0 0
#define CMD_WRDI 		0x04		// Write Disable 0000 0100 04h 0 0 0
#define CMD_RDID 		0x9F		// Read Identification 1001 1111 9Fh 0 0 1 to 20
#define CMD_RDSR 		0x05		// Read Status Register 0000 0101 05h 0 0 1 to inf
#define CMD_WRSR 		0x01		// Write Status Register 0000 0001 01h 0 0 1
#define CMD_READ 		0x03		// Read Data bytes 0000 0011 03h 3 0 1 to inf
#define CMD_FAST_READ 	0x0B		// Read Data bytes at higher speed 0000 1011 0Bh 3 1 1 to inf
#define CMD_PP			0x02		// Page Program 0000 0010 02h 3 0 1 to 256
#define CMD_SE			0xD8		// Sector Erase 1101 1000 D8h 3 0 0
#define CMD_P4E			0x20		// 4KB Parameter Sector Erase
#define CMD_P8E			0x40		// 8KB (two 4KB) Parameter Sector Erase
#define CMD_BE			0xC7		// Bulk Erase 1100 0111 C7h 0 0 0
#define CMD_DP			0xB9		// Deep Power-down 1011 1001 B9h 0 0 0
#define CMD_RES			0xAB		// Release from Deep Powerdown, and Read Electronic Signature 1010 1011 ABh 0 3 1 to inf
#define CMD_AAIP		0xAD		// Auto Address Increment Programming 
#define CMD_EWSR		0x50		// Enable Write Status Register

#define DEVICE_SR_WIP	(1 << 0)	// Write in Progress (WIP) bit
#define DEVICE_SR_WEL	(1 << 1)	// Write Enable Latch (WEL) bit
#define DEVICE_SR_BP0	(1 << 2)	// Block Protect bit BP0
#define DEVICE_SR_BP1	(1 << 3)	// Block Protect bit BP1
#define DEVICE_SR_BP2	(1 << 4)	// Block Protect bit BP2
#define DEVICE_SR_E_ERR (1 << 5)	// Erase Error Occurred
#define DEVICE_SR_P_ERR (1 << 5)	// Programming Error Occurred
#define DEVICE_SR_RSVD  (3 << 5)	// bits 5 and 6 are reserved and always zero
#define DEVICE_SR_SRWD	(1 << 7)	// Status Register Write Disable (SRWD) bit


// set 24-bit address in big endian
static void set_address(uint8_t* buf, const int addr)
{
	buf[0] = (addr & 0xff0000) >> 16;
	buf[1] = (addr & 0x00ff00) >>  8;
	buf[2] = (addr & 0x0000ff) >>  0;
}

#if defined(VARIANT_mx6_sabrelite)
static int write_status_enable(const int spi_fd)
{
	int rc = spi_write_1byte(spi_fd, CMD_EWSR);
	if (-1 == rc) {
		return rc;
	}
	return EOK;
}

static int write_status(const int spi_fd, uint8_t val)
{
    uint8_t buf[2];

    buf[0] = CMD_WRSR;
    buf[1] = val;

	int rc = spi_nor_flash_write(spi_fd, buf, 2);
	if (-1 == rc) {
		return rc;
	}
	return EOK;
}

// must do a write disable after the AAIP command for the SST SPI Nor Flash
static int write_disable(const int spi_fd)
{
	int rc = spi_write_1byte(spi_fd, CMD_WRDI);
	if (-1 == rc) {
		return rc;
	}
	return EOK;
}
#endif


// must do a write enable immediately before a sector erase or program command
static int write_enable(const int spi_fd)
{
	int rc = spi_write_1byte(spi_fd, CMD_WREN);
	if (-1 == rc) {
		return rc;
	}
	return EOK;
}

// wait until program is done
static int wait_for_completion(const int spi_fd)
{
	int count = 2500;
	int rc = 0;
	do
	{
		rc = iswriting(spi_fd);
		if (-1 == rc) {
			return rc;
		}
	}while (rc && --count);
	if (count == 0) {
		errno = EIO;
		return -1;
	}
	return EOK;
}


// return 1 if and only if WIP is set
// return -1 for error
// return 0 if not writing
int iswriting(const int spi_fd)
{

	uint8_t cmd[4] = {CMD_RDSR, 0, 0, 0};
	uint8_t status[4];
	int rc = 0;

	rc = spi_cmd_read(spi_fd, cmd, status, 4);
	if (rc)
	{
		errno = EIO;
		return -1;
	}
	return (status[3] & DEVICE_SR_WIP);
}


// read identification
// prereq: no write in progress
int read_ident(const int spi_fd, int* manufact_id, int* device_id, uint32_t* size)
{
	uint8_t ident[4] = {0, 0, 0, CMD_RDID};
	int rc = 0;

	rc = spi_word_exchange(spi_fd, (uint32_t*)ident);
	if (-1 == rc) {
		return rc;
	}
	*manufact_id = ident[2];
	*device_id = ident[1];
#if defined(VARIANT_draco_spansion) || defined(VARIANT_mx6_sabrelite) || defined(VARIANT_mx6_foryou)
	*size = TOTAL_SIZE_BYTES;
#else
	*size = 1 << ident[0];
#endif

	return (EOK);
}


// release from power down
// prereq: no write in progress
int pd_release(const int spi_fd)
{
	uint8_t cmd[4] = {CMD_RES, 0, 0, 0};
	uint8_t ident;

	int rc;
	rc = spi_cmd_read(spi_fd, cmd, &ident, sizeof ident);
	if (-1 == rc) {
		return rc;
	}
	return (EOK);
}


// erase sector at given offset
// prereq: no write in progress
int sector_erase(const int spi_fd, const int offset)
{
	uint8_t buf[4];
	int rc = 0;
#if defined VARIANT_mx6_sabrelite
	buf[0] = CMD_P4E;
#else
	buf[0] = CMD_SE;
#endif

	set_address(&buf[1], offset);

#if defined VARIANT_mx6_sabrelite
    // Unprotect the whole Flash device 
    if ((write_status_enable(spi_fd) == -1) || (write_status(spi_fd, 0) == -1)) {
        return -1;
    }
#endif
	rc = write_enable(spi_fd);
	if (rc) {
		return -1;
	}

	rc = spi_cmd_write(spi_fd, buf, NULL, 0);
	if (-1 == rc) {
		return rc;
	}
	return EOK;
}


// program up to MAX_BURST-4 bytes at any given offset
// prereq: no write in progress
// returns number of bytes written
int page_program(const int spi_fd, int offset, int len, uint8_t const* data)
{
	const int header_size = CMD_LEN + ADDR_LEN;
	int rc;

	int nbytes = min(len, MAX_BURST - header_size);
	// if writing all nbytes crosses a page boundary, then we reduce nbytes so that we write to the
	// end of the current page, but not beyond.
	nbytes = min(nbytes, (offset & ~(PAGE_SIZE-1)) + PAGE_SIZE - offset);

#ifdef _EXTRA_VERIFY
	uint8_t pre_prog[MAX_BURST - header_size];
	rc = read_from(spi_fd, offset, nbytes, pre_prog);
	if (rc != nbytes) {
		fprintf(stderr, "t%d:%s:%d expected %d bytes, but read_from returned %d\n",
				pthread_self(), __func__, __LINE__, nbytes, rc);
		return -1;
	}
#endif

#if defined VARIANT_mx6_sabrelite
    // Unprotect the whole Flash device 
    if ((write_status_enable(spi_fd) == -1) || (write_status(spi_fd, 0) == -1)) {
        return -1;
    }
#endif

	rc = write_enable(spi_fd);
	if (rc) {
		return -1;
	}

#if defined VARIANT_mx6_sabrelite
    uint8_t const* sbuf = data;
    uint8_t buf[8];
    int offset1 = offset;     
    len = nbytes;

    if (offset1 & 1) {
	    buf[0] = CMD_PP;
	    set_address(&buf[1], offset1++);
        buf[4] = *sbuf++;

        rc = spi_nor_flash_write(spi_fd, buf, 5);

        if ((rc == -1) || wait_for_completion(spi_fd)) {
            return -1;
        }

        if (--len == 0)
            return 1;

        // Need to enable write again before the AAIP command
        rc = write_enable(spi_fd);
        if (rc) {
            return -1;
        }
    }

    // Need to specify the addr for the first AAIP command
    if (len >= 2) {
        buf[0] = CMD_AAIP;
	    set_address(&buf[1], offset1);
        buf[4] = sbuf[0];
        buf[5] = sbuf[1];

        rc = spi_nor_flash_write(spi_fd, buf, 6);
        if ((rc == -1) || wait_for_completion(spi_fd)) {
            return -1;
        }
        
        len -= 2;
        offset1 += 2;
        sbuf += 2;
    }

    // Don't need to specify the addr for the remaining AAIP commands
    for (; len > 1; len -= 2, offset1 += 2, sbuf += 2) {
        buf[0] = CMD_AAIP;
        buf[1] = sbuf[0];
        buf[2] = sbuf[1];

        rc = spi_nor_flash_write(spi_fd, buf, 3);
        if ((rc == -1) || wait_for_completion(spi_fd)) {
            return -1;
        }
    }

    // Exit the AAIP status
	rc = write_disable(spi_fd);
	if (rc) {
		return -1;
    }

    // Only 1 byte left
    if (len == 1) {
        rc = write_enable(spi_fd);
        if (rc) {
            return -1;
        }

	    buf[0] = CMD_PP; 
	    set_address(&buf[1], offset1);
        buf[4] = *sbuf++;

        rc = spi_nor_flash_write(spi_fd, buf, 5);

        if ((rc == -1) || wait_for_completion(spi_fd)) {
            return -1;
        }
    }

    // Protect the whole device
    if ((write_status_enable(spi_fd) == -1) || (write_status(spi_fd, 0x1c) == -1)) {
        return -1;
    }
#else  // End of VARIANT_mx6_sabrelite
	uint8_t header[header_size];

	// copy part of data to program into buf
	header[0] = CMD_PP;
	set_address(&header[1], offset);

	rc = spi_cmd_write(spi_fd, header, data, nbytes);

	// attempt wait for write to complete even if previous spi_cmd_write failed
	if (wait_for_completion(spi_fd)) {
		return -1;
	}

	// return with error if previous spi_cmd_write failed
	if (-1 == rc) {
		return -1;
	}
#endif


#ifdef _EXTRA_VERIFY
	// do a verify here
	{
		uint8_t readback[MAX_BURST - header_size];
		rc = read_from(spi_fd, offset, nbytes, readback);
		if (rc != nbytes) {
			fprintf(stderr, "t%d:%s:%d expected %d bytes, but read_from returned %d\n",
					pthread_self(), __func__, __LINE__, nbytes, rc);
		}
		int i = 0;
		int hitbad = 0;

		for (i=0 ; i< nbytes; ++i) {
			if (readback[i] != data[i]) {
				fprintf(stderr, "t%d: offset=%06x, pre_prog[%02x]=%02x, data[%02x]=%02x, readback[%02x]=%02x\n",
						pthread_self(),
						offset,
						i, pre_prog[i],
						i, data[i],
						i, readback[i]);
				hitbad = 1;
			}
		}
	}
#endif
	return nbytes;
}


// read from open SPI flash from offset, up to MAX_BURST - header_size bytes
// prereq: no write in progress
// return len bytes read
// return -1 if error
int read_from(const int spi_fd, int offset, int len, uint8_t* buffer)
{
	const int header_size = CMD_LEN + ADDR_LEN;
	const int max_read = MAX_BURST - header_size;
	uint8_t header[header_size];
	int rc;

	if (len == 0) {
		return 0;
	}

	header[0] = CMD_READ;
	set_address(&header[CMD_LEN], offset);

	len = min(len, max_read);
	rc = spi_cmd_read(spi_fd, header, buffer, len);
	if (-1 == rc) {
		return rc;
	}
	else {
		return len;
	}
}

__SRCVERSION( "$URL$ $Rev$" )
