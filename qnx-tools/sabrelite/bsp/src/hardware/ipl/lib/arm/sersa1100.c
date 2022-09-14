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
#include <arm/sa1100.h>

unsigned char	poll_sa1100(void);
void			put_byte_sa1100(unsigned char);

static const ser_dev sa1100_dev = {
	get_byte_sa1100,
	put_byte_sa1100,
	poll_sa1100
};

static unsigned	sa1100_uart_base;

/*
 *	user defined defaults
 */
#define	DEFAULT_CLK				3686400

void
init_sa1100(unsigned port, unsigned baud, unsigned clk, int alt)
{
	int		div;

	/*
	 * Calculate baud rate divisor. H/W fixes divide by 16.
	 */
	if (clk == 0)
		clk = DEFAULT_CLK;

	for (div = 0, baud *= 16; clk >= baud; div++)
		clk -= baud;

	if (port == SA1100_UART1_BASE) {
		if (alt) {
			unsigned	tmp;

			/*
			 * Re-assign UART1 to drive GPIO pins 14/15
			 */
			out32(SA1100_PPC_BASE + SA1100_PPAR, SA1100_PPAR_UPR);

			/*
			 * Configure GPIO_14 as output, GPIO_15 as input
			 */
			tmp  = in32(SA1100_GPIO_BASE + SA1100_GPDR);
			tmp |= SA1100_GPIO_14;
			tmp &= ~SA1100_GPIO_15;
			out32(SA1100_GPIO_BASE + SA1100_GPDR, tmp);

			/*
			 * Configure GPIO pins 14/15 for alternate function
			 */
			tmp  = in32(SA1100_GPIO_BASE + SA1100_GAFR);
			tmp |= SA1100_GPIO_14 | SA1100_GPIO_15;
			out32(SA1100_GPIO_BASE + SA1100_GAFR, tmp);
		}
		else {
			/*
			 * Enable UART1 on serial port 1 data pins
			 */
			out32(SA1100_SDLC_BASE + SA1100_SDCR0, SA1100_SDCR0_SUS);
		}
	}

	/*
	 * Disable uart and program baud rate divisor
	 */
	div--;
	out32(port + SA1100_UTCR3, 0);
	out32(port + SA1100_UTBRD_HI, (div >> 8) & 0xff);
	out32(port + SA1100_UTBRD_LO, div & 0xff);

	/*
	 * Clear status bits
	 * Set 8-bit, no parity, 1 stop bit
	 */
	out32(port + SA1100_UTSR0, 0xff);
	out32(port + SA1100_UTCR0, SA1100_UTCR0_DSS);

	/*
	 * Enable receive and transmit
	 */
	out32(port + SA1100_UTCR3, SA1100_UTCR3_RXE | SA1100_UTCR3_TXE);
	sa1100_uart_base = port;
}

void
init_sersa1100(unsigned port, unsigned baud, unsigned clk, int alt)
{
	init_sa1100(port, baud, clk, alt);

	/*
	 * Register our debug functions
	 */
	init_serdev((ser_dev *)&sa1100_dev);
}

unsigned char
poll_sa1100()
{
	if ((in32(sa1100_uart_base + SA1100_UTSR1) & SA1100_UTSR1_RNE) == 0)
		return 0;
	else
		return 1;
}

unsigned char
get_byte_sa1100()
{
	while ((in32(sa1100_uart_base + SA1100_UTSR1) & SA1100_UTSR1_RNE) == 0)
		;
	return in32(sa1100_uart_base + SA1100_UTDR);
}

void
put_byte_sa1100(unsigned char data)
{
	while ((in32(sa1100_uart_base + SA1100_UTSR1) & SA1100_UTSR1_TNF) == 0)
		;
	out32(sa1100_uart_base + SA1100_UTDR, data);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/arm/sersa1100.c $ $Rev: 711024 $")
#endif
