/*
 * $QNXLicenseC: 
 * Copyright 2007, 2008, QNX Software Systems.  
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


#include "ipl.h"
#include <hw/inout.h>
#include <arm/omap.h>

static unsigned char	seromap_pollkey();
static unsigned char	seromap_getchar();
static void				seromap_putchar(unsigned char);

static const ser_dev omap_dev = {
	seromap_getchar,
	seromap_putchar,
	seromap_pollkey
};

static unsigned seromap_base;

static void set_port(unsigned address, unsigned mask, unsigned data) {
	unsigned char c;

	c = in8(address);
	out8(address, (c & ~mask) | (data & mask));
}

void
init_seromap(unsigned address, unsigned baud, unsigned clk, unsigned divisor)
{
	int	baud_val;

	/* Mask all interrupts causes and disable sleep mode and low power mode. */
	set_port(address + OMAP_UART_IER, 0xff, 0x0);

	/* Disable UART (default state) */
	set_port(address + OMAP_UART_MDR1, 0xff, 0x07);

	/* Turn FCR into EFR access */
	set_port(address + OMAP_UART_LCR, 0xff, 0xbf);

	/* This is EFR since LCR = 0xBF */
	set_port(address + OMAP_UART_FCR, 0x50, 0x50);

	/* Divisor latch enable, 8N1 */
	set_port(address + OMAP_UART_LCR, 0xff, 0x83);

	/* Enable access to TCR and TLR registers */
	set_port(address + OMAP_UART_MCR, 0x40, 0x40);

	/*
	 * Reg#6 ?? only when LCR = 0xBF, else it is TCR
	 * TCR set to default (halt RX at 52 bytes, start RX at 0 bytes)
	 */
	set_port(address + OMAP_UART_XOFF1, 0xff, 0x0d);

	/*
	 * Reg#7, XOFF2 when LCR = 0xBF, else it is SPR/TLR
	 * TLR set to 48bytes on RX FIFO, 60 on TX FIFO
	 */
	set_port(address + OMAP_UART_TLR, 0xff, 0xcf);

	/* Disable access to TCR and TLR */
	set_port(address + OMAP_UART_MCR, 0x40, 0x0);

	/* FCR set to 60/32/TXclear/RXclear/Enable */
	set_port(address + OMAP_UART_FCR, 0xff, 0xe7);

#if 1
	for (baud_val = 0, baud *= divisor; clk >= baud; baud_val++)
		clk -= baud;
	if ((clk << 1) >= baud)
		baud_val++;
#else
	baud_val = clk / (baud * divisor);
#endif

	set_port(address + OMAP_UART_DLL, 0xff, baud_val);	/* baud rate. lobyte */
	set_port(address + OMAP_UART_DLH, 0xff, (baud_val >> 8));	/* baud rate. hibyte */

	set_port(address + OMAP_UART_LCR, 0xff, 0x03);		/* Divisor latch disable, 8N1 */
	set_port(address + OMAP_UART_MDR1, 0xff, 0x00);		/* UART mode */

	seromap_base = address;

	/*
	 * Register our debug functions
	 */
	init_serdev((ser_dev *)&omap_dev);
}

static unsigned char seromap_pollkey(void)
{
	if (in8(seromap_base + OMAP_UART_LSR) & OMAP_LSR_RXRDY)
		return 1;
	else
		return 0;
}

static unsigned char seromap_getchar(void)
{
	/*
	 * wait for data to be available	
	 */
	
	while (!seromap_pollkey())
		;

	return (in8(seromap_base));
}

static void seromap_putchar(unsigned char data)
{
	while (!(in8(seromap_base + OMAP_UART_LSR) & OMAP_LSR_TXRDY))
		;

	out8(seromap_base, data);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/arm/seromap.c $ $Rev: 711024 $")
#endif
