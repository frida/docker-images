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

#include "ipl.h"
#include <hw/inout.h>
#include "serat91sam9xx.h"

static unsigned char serat91sam9xx_pollkey();
static unsigned char serat91sam9xx_getchar();
static void          serat91sam9xx_putchar(unsigned char);

static const ser_dev at91sam9xx_dev = {
	serat91sam9xx_getchar,
	serat91sam9xx_putchar,
	serat91sam9xx_pollkey
};

static unsigned seratsam9xx_base;

void
init_serat91sam9xx(serat91_t * ser)
{
	unsigned uart_txrx_ena_reg = (*(volatile unsigned int *)((ser->base + ser->ser0_offset) + 0x0000));

	seratsam9xx_base = ser->base +  ser->ser0_offset;

	if (!ser->dbgu_offset)
		ser->dbgu_offset = ser->ser0_offset;

	/* Reset DBGU controller */
	out32((ser->base + ser->ser0_offset) + uart_txrx_ena_reg, ser->disable_txrx);

	/* Disable Serial Interrupts */
	out32((ser->base + ser->ser0_offset) + UART_ID_REG, DISABLE_UART_INTR);

	/* Set up PIOA for DBGU Unit */
	out32((ser->base + ser->pio_offset) + PIO_ASR, ser->dbgu_pins);     /* Select DBGU pins */
	out32((ser->base + ser->pio_offset) + PIO_PDR, ser->dbgu_pins);     /* Enable Peripheral Control of the Pin */

	/* Set No Parity */
	out32((ser->base + ser->dbgu_offset) + UART_MODE_REG, DBGU_MR_VAL);

	/* Set Baud Rate Generator */
	out32((ser->base + ser->dbgu_offset) + UART_BRGR_REG, ser->dbgu_brgr_val);

	/* Enable Serial */
	out32((ser->base + ser->ser0_offset) + uart_txrx_ena_reg, ENABLE_TXRX);

	/*
	 * Register our debug functions
	 */
	init_serdev((ser_dev *)&at91sam9xx_dev);

}

static unsigned char serat91sam9xx_pollkey(void)
{
	if (in32(seratsam9xx_base + UART_STATUS_OFFSET) & UART_RX_RDY)
		return 1;
	else
		return 0;
}

static unsigned char serat91sam9xx_getchar(void)
{
	/*
	 * wait for data to be available	
	 */
	while (!serat91sam9xx_pollkey()) {};

	return (in32(seratsam9xx_base + UART_RXDATA_OFFSET));
}

static void serat91sam9xx_putchar(unsigned char data)
{
	while (!(in32(seratsam9xx_base + UART_STATUS_OFFSET) & UART_TX_EMPTY)) {};

	out32((seratsam9xx_base + UART_TXDATA_OFFSET ), data);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/arm/serat91sam9xx.c $ $Rev: 711024 $")
#endif
