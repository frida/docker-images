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
#include <hw/8250.h>


static unsigned char	ser8250_getchar(void);
static unsigned char	ser8250_pollkey(void);
static void				ser8250_putchar(unsigned char);


static const ser_dev ser8250_dev = {
	ser8250_getchar,
	ser8250_putchar,
	ser8250_pollkey
};

//
//	user defined defaults
//

#define	DEFAULT_CLK				1843200
#define DEFAULT_DIV				16

static unsigned port_base;
static unsigned port_size;
static unsigned reg_shift;


static void ser8250_writeport(unsigned address, unsigned data)
{
	switch (port_size) {
		case 4:
			out32(address, data);
			break;
		case 2:
			out16(address, (unsigned short)data);
			break;
		default:
			out8(address, (unsigned char)data);
			break;
	}
}

static unsigned ser8250_readport(unsigned address)
{
	switch (port_size) {
		case 4:
			return (in32(address));
		case 2:
			return (in16(address));
		default:
			return (in8(address));
	}
}

static void ser8250_setport(unsigned address, unsigned mask, unsigned data) 
{
	unsigned char c;

	c = ser8250_readport(address);
	ser8250_writeport(address, (c & ~mask) | (data & mask));
}

void
init_ser8250(unsigned address, unsigned size, unsigned shift, unsigned baud, unsigned clk, unsigned divisor)
{
	unsigned	brd;

	/*
	 * Initialize port base, size and shift count
	 */
	port_base = address;
	reg_shift = shift;
	port_size = size;

	/*
	 * This routine will initialize the selected 8250 serial port
	 * to 8N1 parameters. 
	 */

	/*
	 * Set Baud rate
	 */
	/* brd = clk / (baud * divisor); */
	for (brd = 0, divisor *= baud; clk >= divisor; clk -= divisor)
		++brd;
	if ((clk << 1) >= divisor)
		++brd;

	ser8250_setport(address + (REG_LC << shift), LCR_DLAB, LCR_DLAB);
	ser8250_setport(address + (REG_DL0 << shift), 0xFF, brd & 0xFF);
	ser8250_setport(address + (REG_DL1 << shift), 0xFF, brd >> 8);
	ser8250_setport(address + (REG_LC << shift), LCR_DLAB, 0);

	/*
	 * Set data bits to 8
	 */
	ser8250_setport(address + (REG_LC << shift), 0xFF, 0x03);

	/*
	 * Register our debug functions
	 */
	init_serdev((ser_dev *)&ser8250_dev);
}

static unsigned char ser8250_pollkey(void)
{
	if (ser8250_readport(port_base + (REG_LS << reg_shift)) & LSR_RXRDY)
		return 1;
	else
		return 0;
}

static unsigned char ser8250_getchar(void)
{
	/*
	 *	wait for data to be available	
	 */
	while (!ser8250_pollkey());

	return (ser8250_readport(port_base));
}

static void ser8250_putchar(unsigned char data)
{
	/*
	 *	wait for transmitter ready
	 */
	while (!(ser8250_readport(port_base + (REG_LS << reg_shift)) & LSR_TXRDY));

	ser8250_writeport(port_base, data);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/ser8250.c $ $Rev: 711024 $")
#endif
