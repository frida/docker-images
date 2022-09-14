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
#include <arm/pxa250.h>
#include <inttypes.h>

/*
 *	user defined defaults
 */
#define	DEFAULT_CLK				14745600

static unsigned char	pollkey_pxa250(void);

static const ser_dev pxa250_dev = {
	get_byte_pxa250,
	put_byte_pxa250,
	pollkey_pxa250
};

static unsigned pxa250_uart;

void
init_pxa250(unsigned port, unsigned baud, unsigned clk)
{
	unsigned tmp;
	unsigned brd;

	/*
	 * Calculate baud rate divisor. H/W fixes divide by 16.
	 */
	if (clk == 0)
		clk = DEFAULT_CLK;

	for (brd = 0, baud *= 16; clk >= baud; brd++)
		clk -= baud;

	/*
	 * disable uart
	 */
	out32(port + PXA250_IER, 0);

	/*
	 * program baud rate divisor
	 */

	tmp = in32(port + PXA250_LCR);

	out32(port + PXA250_LCR, tmp | PXA250_LCR_DLAB);
	out32(port + PXA250_DLL, brd & 0xff);
	out32(port + PXA250_DLH, (brd >> 8) & 0xff);

    /*
     * Enable FIFOs and reset them
     */
    out32(port + PXA250_FCR, PXA250_FCR_TRFIFOE);
    out32(port + PXA250_FCR, PXA250_FCR_TRFIFOE | PXA250_FCR_RESETRF | PXA250_FCR_RESETTF);

	/*
	 * Set 8bit, no-parity, clear DLAB
	 */
	out32(port + PXA250_LCR, PXA250_LCR_WLS_8);

    /*
     * assert DTR and RTS lines
     */
    out32(port + PXA250_MCR, PXA250_MCR_DTR | PXA250_MCR_RTS);

	/*
	 * Enable UART
	 */
	out32(port + PXA250_IER, PXA250_IER_UUE);

	pxa250_uart = port;
}

void
init_serpxa250(unsigned port, unsigned baud, unsigned clk)
{
	init_pxa250(port, baud, clk);

	/*
	 * Register our debug functions
	 */
	init_serdev((ser_dev *)&pxa250_dev);
}

unsigned char 
get_byte_pxa250()
{
	unsigned long port = pxa250_uart;

	while ((in32(port + PXA250_LSR) & PXA250_LSR_DR) == 0) 
		;
	return (in32(port + PXA250_RBR) & 0xff);
}

void
put_byte_pxa250(unsigned char data)
{
	unsigned port = pxa250_uart;

	while ((in32(port + PXA250_LSR) & PXA250_LSR_TDRQ) == 0)
		;
	out32(port + PXA250_THR, data);
}

static unsigned char 
pollkey_pxa250()
{
	unsigned long port = pxa250_uart;

	if ((in32(port + PXA250_LSR) & PXA250_LSR_DR) == 0) 
		return 0;
	else
		return 1;
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/arm/serpxa250.c $ $Rev: 711024 $")
#endif
