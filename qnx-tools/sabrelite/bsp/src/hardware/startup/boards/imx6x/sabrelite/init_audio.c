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


#include "startup.h"
#include <arm/mx6x.h>

#define RXDSRC(_PORT_) ((((_PORT_) - 1) & 0x7) << 13)
#define TXFS(_DIR_, _PORT_) ((((_DIR_) & 1) << 30) | ((((_PORT_) - 1) & 0x7) << 27))
#define TXCLK(_DIR_, _PORT_) ((((_DIR_) & 1) << 25) | ((((_PORT_) - 1) & 0x7) << 22))

#define PTCR(_pno_)	(((_pno_) - 1) * 8)
#define PDCR(_pno_)	((((_pno_) - 1) * 8) + 4)

static void init_audiomux(uint32_t portout, uint32_t portin)
{
	uint32_t pi_ptcr, po_ptcr, pi_pdcr, po_pdcr;
	pi_ptcr = po_ptcr = pi_pdcr = po_pdcr = 0;
	
	pi_ptcr |= (AUDMUX_SYNC);
	po_ptcr |= (AUDMUX_SYNC);
	
	pi_pdcr |= RXDSRC(portout);
	po_pdcr |= RXDSRC(portin);
	
	/* Set TxFS direction & source */
	pi_ptcr |= (AUDMUX_TFS_DIROUT);		/* Output */
	pi_ptcr |= TXFS(0, portout);

	/* Set TxClk direction and source */
	pi_ptcr |= (AUDMUX_TCLK_DIROUT);	/* Output */
	pi_ptcr |= TXCLK(0, portout);

	out32 (MX6X_AUDMUX_BASE + PTCR(portin), pi_ptcr);
	out32 (MX6X_AUDMUX_BASE + PDCR(portin), pi_pdcr);
	out32 (MX6X_AUDMUX_BASE + PTCR(portout), po_ptcr);
	out32 (MX6X_AUDMUX_BASE + PDCR(portout), po_pdcr);
}

/*
 * Connect external port 4 to internal Port 2.  Port 2 is always connected to SSI-2.
 * The codec (SGTL5000) acts as the clock master, providing both the frame clock, and
 * the bit clock to the i.MX6 Q.
 */
void init_audio (void)
{
	init_audiomux(4, 2);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/sabrelite/init_audio.c $ $Rev: 729057 $")
#endif
