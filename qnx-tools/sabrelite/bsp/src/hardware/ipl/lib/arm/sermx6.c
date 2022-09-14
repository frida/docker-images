/*
 * $QNXLicenseC:
 * Copyright 2014, QNX Software Systems.
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
#include <arm/mx6x.h>

static unsigned char	mx6_uart_pollkey();
static unsigned char	mx6_uart_getchar();
static void		mx6_uart_putchar(unsigned char);

static const ser_dev mx6_dev = {
	mx6_uart_getchar,
	mx6_uart_putchar,
	mx6_uart_pollkey
};

static unsigned sermx6_base;

void
init_sermx6(unsigned port, unsigned baud, unsigned clk, unsigned divisor)
{
	unsigned tmp, rfdiv = MX6_UFCR_RFDIV_2;

	/* Wait for UART to finish transmitting */
	while (!(in32(port + MX6_UART_TS) & MX6_UTS_TXEMPTY));

	/* Disable UART */
	tmp = in32(port + MX6_UART_CR1) & (~MX6_UCR1_UARTEN);
	out32(port + MX6_UART_CR1, tmp);

	/* Set to default POR state */
	out32(port + MX6_UART_CR1,	0x0000);
	out32(port + MX6_UART_CR2,	0x0000);

	while (!(in32(port + MX6_UART_CR2) & MX6_UCR2_SRST));

	out32(port + MX6_UART_CR3,	0x0704);
	out32(port + MX6_UART_CR4,	0x8000);
	out32(port + MX6_UART_FCR,	0x0801);
	out32(port + MX6_UART_ESC,	0x002B);
	out32(port + MX6_UART_TIM,	0x0000);
	out32(port + MX6_UART_BIR,	0x0000);
	out32(port + MX6_UART_BMR,	0x0000);
	out32(port + MX6_UART_ONEMS,	0x0000);
	out32(port + MX6_UART_TS,	0x0000);

	/* Configure FIFOs, Default divisor (RFDIV) 2 */
	out32(port + MX6_UART_FCR, ((1 << MX6_UFCR_RXTL_SHIFT) |
				MX6_UFCR_RFDIV_2 |
				(2 << MX6_UFCR_TXTL_SHIFT)) );

	/* Configure clock divisor RFDIV allowed clk div 1-7 */
	if ((divisor >= 1) && (divisor < 7))
		rfdiv = (6 - divisor) << MX6_UFCR_RFDIV_SHIFT;
	else if (divisor == 7)
		rfdiv = MX6_UFCR_RFDIV_7;
	else
		rfdiv = MX6_UFCR_RFDIV_2;

	tmp = in32(port + MX6_UART_FCR) | rfdiv;
	out32(port + MX6_UART_FCR, tmp);

	/* Set to 8N1 */
	tmp = in32(port + MX6_UART_CR2) & (~MX6_UCR2_PREN);
	out32(port + MX6_UART_CR2, tmp);

	tmp = in32(port + MX6_UART_CR2) | MX6_UCR2_WS;
	out32(port + MX6_UART_CR2, tmp);

	tmp = in32(port + MX6_UART_CR2) & (~MX6_UCR2_STPB);
	out32(port + MX6_UART_CR2, tmp);

	/* Ignore RTS */
	tmp = in32(port + MX6_UART_CR2) | MX6_UCR2_IRTS;
	out32(port + MX6_UART_CR2, tmp);

	/* Enable UART */
	tmp = in32(port + MX6_UART_CR1) | MX6_UCR1_UARTEN;
	out32(port + MX6_UART_CR1, tmp);

	/* Enable FIFOs */
	tmp = in32(port + MX6_UART_CR2) | (MX6_UCR2_SRST |
			MX6_UCR2_RXEN | MX6_UCR2_TXEN);
	out32(port + MX6_UART_CR2, tmp);

	/* Clear status flags */
	tmp = in32(port + MX6_UART_SR2) | (MX6_USR2_ADET |
				MX6_USR2_IDLE	|
				MX6_USR2_IRINT	|
				MX6_USR2_WAKE	|
				MX6_USR2_RTSF	|
				MX6_USR2_BRCD	|
				MX6_USR2_ORE	|
				MX6_USR2_RDR);
	out32(port + MX6_UART_SR2, tmp);

	/* Clear status flags */
	tmp = in32(port + MX6_UART_SR1) | (MX6_USR1_PARITYERR |
				MX6_USR1_RTSD		|
				MX6_USR1_ESCF		|
				MX6_USR1_FRAMERR	|
				MX6_USR1_AIRINT	|
				MX6_USR1_AWAKE);
	out32(port + MX6_UART_SR1, tmp);

	/* A fixed BIR for auto calculation */
	out32(port + MX6_UART_BIR, MX6_BIR_INC_AUTO);

	/* Calculate BMR. Refer TRM "64.5 Binary Rate Multiplier (BRM)" */
	/* BRM sub block receives ref_clk (clk/divisor) after the divider */
	out32(port + MX6_UART_BMR, (((clk/divisor) / (16 * baud)) * (MX6_BIR_INC_AUTO + 1)) - 1);

	sermx6_base = port;

	/*
	* Register our debug functions
	*/
	init_serdev((ser_dev *)&mx6_dev);
}

static unsigned char mx6_uart_pollkey(void)
{
	if (in32(sermx6_base + MX6_UART_SR2) & MX6_USR2_RDR)
		return 1;
	else
		return 0;
}

static unsigned char mx6_uart_getchar(void)
{
	while (!(mx6_uart_pollkey()));
		return ((unsigned char) in32(sermx6_base + MX6_UART_RXDATA));
}

static void mx6_uart_putchar(unsigned char data1)
{
	while(!(in32(sermx6_base + MX6_UART_SR1) & MX6_USR1_TRDY));
		out32(sermx6_base + MX6_UART_TXDATA, (unsigned) data1);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/arm/sermx6.c $ $Rev: 739242 $")
#endif
