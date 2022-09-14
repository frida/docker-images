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
#include <arm/mx1.h>

static unsigned char	mx1uart_pollkey();
static unsigned char	mx1uart_getchar();
static void				mx1uart_putchar(unsigned char);

static const ser_dev mx1_dev = {
	mx1uart_getchar,
	mx1uart_putchar,
	mx1uart_pollkey
};

static unsigned mx1_uart;

void
init_sermx1(unsigned port, unsigned baud, unsigned clk)
{
	unsigned tmp;
	unsigned gpio_base, gpio_mask;

	if (port == MX1_UART1_BASE) {
		gpio_base = MX1_GPIOC_BASE;
		gpio_mask = ~0x00001E00;    /* Port C, bit 9, 10, 11, 12 */
	}
	else if (port == MX1_UART2_BASE) {
		gpio_base = MX1_GPIOB_BASE;
		gpio_mask = ~0xF0000000;    /* Port B, bit 28, 29, 30, 31 */
	}
	else {
		while (1)
			;
	}

	/*
	 * Wait for Tx FIFO and shift register empty if the UART is enabled
	 */
	if ((in32(port + MX1_UART_CR1) & (MX1_UCR1_UARTCLKEN|MX1_UCR1_UARTEN)) == (MX1_UCR1_UARTCLKEN|MX1_UCR1_UARTEN)) {
		if (in32(port + MX1_UART_CR2) & MX1_UCR2_TXEN) {
			while (!(in32(port + MX1_UART_SR2) & MX1_USR2_TXDC))
				;
		}
	}

	/* Enable UART clock, Disable UART */
	out32(port + MX1_UART_CR1, MX1_UCR1_UARTCLKEN);

	/* Reset UART */
	out32(port + MX1_UART_CR2, 0);

	/* Disable Port */
	tmp = in32(gpio_base + MX1_GPIO_GIUS) & gpio_mask;
	out32(gpio_base + MX1_GPIO_GIUS, tmp);
	out32(gpio_base + MX1_GPIO_GIUS, tmp);

	tmp = in32(gpio_base + MX1_GPIO_GPR) & gpio_mask;
	out32(gpio_base + MX1_GPIO_GPR, tmp);

	/* 
	 * 8-bit, no-parity, 1 stop bit
	 * ignore RTS, active CTS
	 */
	out32(port + MX1_UART_CR2, 0x5021);

	/* The Reference Frequency = 16MHz */
	out32(port + MX1_UART_CR3, 0);
	out32(port + MX1_UART_CR4, 0x41);

	/*
	 * Assume Peripheral Clock is 96MHz, divide it by 6
	 */
	out32(port + MX1_UART_FCR, 0x00000801);

#define	BMR_DEFAULT	((1000000 >> 7) - 1)

	out32(port + MX1_UART_BIR, (baud >> 7) - 1);
	out32(port + MX1_UART_BMR, BMR_DEFAULT);

	/* Enable UART */
	out32(port + MX1_UART_CR1, MX1_UCR1_UARTCLKEN | MX1_UCR1_UARTEN);

	/* Enable Tx/Rx */
	out32(port + MX1_UART_CR2, 0x5027);

	for (tmp = 0; tmp < 100; tmp++)
		in32(port + MX1_UART_RXDATA);

	mx1_uart = port;

	/*
	 * Register our debug functions
	 */
	init_serdev((ser_dev *)&mx1_dev);
}

static unsigned char mx1uart_pollkey(void)
{
	if (in32(mx1_uart + MX1_UART_SR2) & MX1_USR2_RDR)
		return 1;
	else
		return 0;
}

static unsigned char mx1uart_getchar(void)
{
	while (!(mx1uart_pollkey()))
		;

	return ((unsigned char)in32(mx1_uart + MX1_UART_RXDATA));
}

static void mx1uart_putchar(unsigned char data)
{
	while (!(in32(mx1_uart + MX1_UART_SR1) & MX1_USR1_TRDY))
		;

	out32(mx1_uart + MX1_UART_TXDATA, (unsigned)data);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/arm/sermx1.c $ $Rev: 711024 $")
#endif
