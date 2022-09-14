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
#include <arm/tuareg.h>
#include <hw/inout.h>



static unsigned char	tuareguart_pollkey();
static unsigned char	tuareguart_getchar();
static void				tuareguart_putchar(unsigned char);

static const ser_dev tuareg_dev = {
	tuareguart_getchar,
	tuareguart_putchar,
	tuareguart_pollkey
};

static unsigned tuareg_uart;

void
init_sertuareg(unsigned port, unsigned baud, unsigned clk)
{

  	unsigned	brd,
				divisor=16;			


	if (port == TUAREG_UART0_BASE) {

		/* Initialize UART pins and clock	 */

		*(unsigned short *)(TUAREG_CGC_BASE + TUAREG_CGC_PUR1)		 |=  TUAREG_PCG1_PUR1_WAKEUART0; 
		*(unsigned short *)(TUAREG_CGC_BASE + TUAREG_CGC_PCG1)		 |=  TUAREG_PCG1_PUR1_WAKEUART0; 

		*(unsigned short *)(TUAREG_GPIO_BASE+ TUAREG_GPIO_PC0)		 |= TUAREG_PC0_C00 | TUAREG_PC0_C01;
		*(unsigned short *)(TUAREG_GPIO_BASE+ TUAREG_GPIO_PC1)		 |= TUAREG_PC1_C00 | TUAREG_PC1_C01;
		*(unsigned short *)(TUAREG_GPIO_BASE+ TUAREG_GPIO_PC2)		 |= TUAREG_PC2_C00 | TUAREG_PC2_C01;

		/* Disable UART A interrupts 		 */

		*(unsigned int *)(TUAREG_EIC_BASE + TUAREG_EIC_IER0) &= ~TUAREG_EIC_UART0;

		/* Initialize UART hardware 		 */

		*(unsigned short *)(TUAREG_UART0_BASE + TUAREG_UART_TXRESET) = 0xFFFF;       /* Reset Tx fifo */
		*(unsigned short *)(TUAREG_UART0_BASE + TUAREG_UART_TXRESET) = 0xFFFF;       /* Reset Rx fifo */


		/*
		 * Set Baud rate
		 */

		/* brd = clk / (baud * divisor); */
		for (brd = 0, divisor *= baud; clk >= divisor; clk -= divisor)
			++brd;
		if ((clk << 1) >= divisor)
			++brd;
   	
        *(unsigned short*)(TUAREG_UART0_BASE + TUAREG_UART_BAUDRATE) = brd;
        *(unsigned short*)(TUAREG_UART0_BASE + TUAREG_UART_CNTL) = 
				TUAREG_CNTL_RX_EN | TUAREG_CNTL_RUN | TUAREG_CNTL_STOP10 | TUAREG_CNTL_MODE8BITS;        /* control register */

	}

	else 
		while(1);

	tuareg_uart = port;

	/*
	 * Register our debug functions
	 */

	init_serdev((ser_dev *)&tuareg_dev);
}

static unsigned char tuareguart_pollkey(void)
{
	if (in16(tuareg_uart + TUAREG_UART_STATUS) & TUAREG_STATUS_RXNOTEMPTY)
		return 1;
	else
		return 0;
}

static unsigned char tuareguart_getchar(void)
{
	while (!(tuareguart_pollkey()))
		;

	return ((unsigned char)in16(tuareg_uart + TUAREG_UART_RX));
}

static void tuareguart_putchar(unsigned char data)
{
	while (!(in16(tuareg_uart + TUAREG_UART_STATUS) & TUAREG_STATUS_TXEMPTY))
		;

	out16(tuareg_uart + TUAREG_UART_TX, (unsigned)data);
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/arm/sertuareg.c $ $Rev: 711024 $")
#endif
