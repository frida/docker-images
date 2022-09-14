/*
 * $QNXLicenseC: 
 * Copyright 2011, QNX Software Systems.  
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
#include "ipl_mx6x.h"

static unsigned char	mx6q_uart_pollkey();
static unsigned char	mx6q_uart_getchar();
static void				mx6q_uart_putchar(unsigned char);

static const ser_dev mx6q_dev = {
	mx6q_uart_getchar,
	mx6q_uart_putchar,
	mx6q_uart_pollkey
};

unsigned char mx6q_uart_pollkey(void)
{
	if (MXC_UART_SR2 & MXC_USR2_RDR)
		return 1;
	else
		return 0;
}

unsigned char mx6q_uart_getchar(void)
{
	while (!(mx6q_uart_pollkey()));
    return ((unsigned char)MXC_UART_RXDATA);
}

void mx6q_uart_putchar(unsigned char data1)
{
	while(!(MXC_UART_SR1 & MXC_USR1_TRDY));
    MXC_UART_TXDATA = (unsigned short)data1;
}

void
init_serial_mx6q()
{
    /* Wait for UART to finish transmitting */
    while (!(MXC_UART_TS & MXC_UTS_TXEMPTY));

	/* Disable UART */
	MXC_UART_CR1   &= (unsigned short) (~MXC_UCR1_UARTEN);

	/* Set to default POR state */
    MXC_UART_CR1    = (unsigned short) 0x00000000;
    MXC_UART_CR2    = (unsigned short) 0x00000000;

    while (!(MXC_UART_CR2 & MXC_UCR2_SRST));

    MXC_UART_CR3    = (unsigned short) 0x0704;
	MXC_UART_CR4    = (unsigned short) 0x8000;
    MXC_UART_FCR    = (unsigned short) 0x0801;
    MXC_UART_ESC    = (unsigned short) 0x002B;
    MXC_UART_TIM    = (unsigned short) 0x0000;
    MXC_UART_BIR    = (unsigned short) 0x0000;
    MXC_UART_BMR    = (unsigned short) 0x0000;
    MXC_UART_ONEMS  = 0x0000;
    MXC_UART_TS     = (unsigned short) 0x0000;

     /* Configure FIFOs */
	MXC_UART_FCR    = (unsigned short) ((1 << MXC_UFCR_RXTL_SHIFT) | 
	                  MXC_UFCR_RFDIV_2 | 
	                  (2 << MXC_UFCR_TXTL_SHIFT));

	/* Set to 8N1 */
    MXC_UART_CR2   &= (unsigned short) ~MXC_UCR2_PREN;
    MXC_UART_CR2   |= (unsigned short) MXC_UCR2_WS;
    MXC_UART_CR2   &= (unsigned short) ~MXC_UCR2_STPB;

    /* Ignore RTS */
    MXC_UART_CR2   |= (unsigned short) MXC_UCR2_IRTS;

     /* Enable UART */
	MXC_UART_CR1   |= (unsigned short) MXC_UCR1_UARTEN;

	/* Enable FIFOs */
    MXC_UART_CR2   |= (unsigned short) (MXC_UCR2_SRST | 
                      MXC_UCR2_RXEN | MXC_UCR2_TXEN);

    /* Clear status flags */
    MXC_UART_SR2   |= (unsigned short) (MXC_USR2_ADET  |
	                  MXC_USR2_IDLE  |
	                  MXC_USR2_IRINT |
	                  MXC_USR2_WAKE  |
	                  MXC_USR2_RTSF  |
	                  MXC_USR2_BRCD  |
	                  MXC_USR2_ORE   |
	                  MXC_USR2_RDR);

    /* Clear status flags */
   MXC_UART_SR1    |= (unsigned short) (MXC_USR1_PARITYERR |
	                  MXC_USR1_RTSD      |
	                  MXC_USR1_ESCF      |
	                  MXC_USR1_FRAMERR   |
	                  MXC_USR1_AIRINT    |
	                  MXC_USR1_AWAKE);

	MXC_UART_BIR    = (unsigned short) 0xF;
	MXC_UART_BMR    = (unsigned short) 0x15B;

	/*
	 * Register our debug functions
	 */
	init_serdev((ser_dev *)&mx6q_dev);
}
 

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/boards/mx6q-sabrelite/serial.c $ $Rev: 740617 $")
#endif
