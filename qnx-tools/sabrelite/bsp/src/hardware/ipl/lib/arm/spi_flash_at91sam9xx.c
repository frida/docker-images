/*
 * $QNXLicenseC:
 * Copyright 2009, QNX Software Systems.
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
 * AT91SAM9xx board family use ATMEL AT45DB642D SPI Flash.
 */


#include <hw/inout.h>
#include "ipl.h"
#include <arm/at91sam9xx.h>

#define MD_ID			0x9f
#define	PAGE_READ		0xD2
#define	AT91_SPI_FLASH_PAGESIZE	1056
#define	BLK2PAGE		8

void spi_flash_terminate_read(unsigned spi_base)
{
	/* A low to high transition on avtive low CS */
	out32(spi_base + AT91_SPI_CSR0, (in32(spi_base + AT91_SPI_CSR0) & ~AT91_SPI_CSRx_CSAAT));
	/* Tristate 8-bit serial output pins done by Last Transfer bit*/
	out32(spi_base + AT91_SPI_CR, (in32(spi_base + AT91_SPI_CR) | 0x01000000));
}

unsigned char spi_read_data(unsigned spi_base)
{
	while (!(in32(spi_base + AT91_SPI_SR) & 0x00000001)) {}
	return in32(spi_base + AT91_SPI_RDR);
}

void spi_send_command(unsigned char * command , int size, unsigned spi_base)
{
	int i;

	/* Master Mode already set */
	/* Fixed Peripheral is already selected */
	/* Mode fault detection is already disabled */
	/* PCS is set to 0xe */
	/* DLYBCS is set to zero */
	/* Enable Chip select */

	out32(spi_base + AT91_SPI_CSR0, (in32(spi_base + AT91_SPI_CSR0) | 0x8));

	for (i = 0; i < size; i++) {
		while (!(in32(spi_base+ AT91_SPI_SR) & AT91_SPI_SR_TDRE)) {}

		out32(spi_base + AT91_SPI_TDR, command[i]);

		while (!(in32(spi_base+ AT91_SPI_SR) & AT91_SPI_SR_TDEMPTY)) {}
	}
}

int spi_flash_command(unsigned char *command, int size, unsigned spi_base)
{
	spi_send_command(command, size, spi_base);
	return 0;
}

int spi_flash_read_page(int page, char *mb, unsigned spi_base)
{
	unsigned char	*p1;
	unsigned char	dataflash_cmd[8];
	int		i;

	dataflash_cmd[0] = PAGE_READ;
	dataflash_cmd[1] = (page & 0x00001fe0) >> 5;
	dataflash_cmd[2] = (page & 0x0000001f) << 3;
	dataflash_cmd[3] = 0;

	for (i = 4; i < 8; i++) {
		dataflash_cmd[i] = 0;
	}

	spi_send_command(dataflash_cmd, 8, spi_base);

	/*
	 * Main buffer
	 */
	p1 = (unsigned char *)mb;
	spi_read_data(spi_base);

	if (p1 != 0) {
		for (i = 0; i < AT91_SPI_FLASH_PAGESIZE; i++) {
			out32(spi_base + AT91_SPI_TDR, 0xff);
			while (!(in32(spi_base+ AT91_SPI_SR) & AT91_SPI_SR_TDEMPTY)) {}

			*p1++ = spi_read_data(spi_base);
		}
	}

	spi_flash_terminate_read(spi_base);
	return 0;
}

int spi_flash_read_id(unsigned char *id, unsigned spi_base)
{
	unsigned char	command[4];
	int		i = 0;

	command[0] = MD_ID;
	spi_send_command(command,1, spi_base);
	spi_read_data(spi_base); /* flush RDR */

	for (i = 0; i < 4; i++) {
		out32(spi_base + AT91_SPI_TDR, 0xff);

        	while (!(in32(spi_base+ AT91_SPI_SR) & AT91_SPI_SR_TDEMPTY)) {}
		*id++ = spi_read_data(spi_base);
	}
	spi_flash_terminate_read(spi_base);

	return 0;
}

int spi_flash_probe(unsigned spi_base)
{
	unsigned char	id[4];

	if (spi_flash_read_id(id, spi_base))
		return -1;

	if (id[0] == 0x1f && id[1] == 0x28) {
		ser_putstr("ATMEL AT45DB642D SPI Flash detected.\n");
		return 0;
	}
	else {
		ser_putstr("Serial Flash not detected\n");
	}

	ser_putstr("Unsupported SPI Flash chip.\n");

	return -2;
}
/*
 * Load QNX boot image from SPI Flash
 */
int spi_flash_load_qnx(unsigned address, int page, unsigned spi_base)
{
	unsigned int	offset = 0;
	unsigned char	*buf = (unsigned char *)address;
	unsigned int	*temp;

	/* Avoid detection of IFS header signature in IPL, start searching for page 12*/
	int		expected = 1024 * 1024 * 4;
	int		loaded = 0;
	int		found = 0;
	struct		startup_header   *startup_hdr;
	unsigned int	start_address = address;

	if (spi_flash_probe(spi_base)) {
		ser_putstr("SPI flash probe failed\n");
		return -1;
	}

	while (1) {
		spi_flash_read_page(page++ , (char*)buf, spi_base);
		if (!(page % 100)) ser_putstr("#");
		if (found) {
		        loaded = loaded + 1056;
		}
		else {
			loaded = 0;
		}

		if (!found) {
			for (offset = 0; offset < AT91_SPI_FLASH_PAGESIZE; offset += 4) {
				startup_hdr = (struct startup_header *)(buf + offset);
				if (startup_hdr->signature == STARTUP_HDR_SIGNATURE) {
					ser_putstr("\nQNX IFS image detected on page: ");
					ser_puthex(page);
					ser_putstr(" Offset: ");
					ser_puthex(offset);
					expected = startup_hdr->stored_size;
					ser_putstr(" Size: ");
					ser_puthex(expected);
					ser_putstr("\n");
					start_address = address;
					loaded = AT91_SPI_FLASH_PAGESIZE - offset;
					found = 1;
					temp = (unsigned int *)address;
					buf = (unsigned char *)(address - offset);
					for (; offset < AT91_SPI_FLASH_PAGESIZE; offset += 4, temp++) {
						*temp = *(unsigned int*)(address + offset);
					}
					break;
				}
			}
		}

		if (found) {
			buf = buf + AT91_SPI_FLASH_PAGESIZE;
			if (loaded >= expected) {
				ser_putstr("Done\n");
				return 0;
			}
		}
		if (page >= 8192) {
			ser_putstr("\nstop No image found\n");
			break;
		}
	}
	return -1;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/arm/spi_flash_at91sam9xx.c $ $Rev: 711024 $")
#endif
