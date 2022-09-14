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



#ifndef _MXECSPI_H
#define _MXECSPI_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <hw/inout.h>
#include <hw/spi-master.h>


#define MX_ECSPI_EVENT			1
#define MX_ECSPI_PRIORITY		21
#define MX_ECSPI_RXDATA			0x00	/* Receive data register */
#define MX_ECSPI_TXDATA			0x04	/* Transmit data register */
#define MX_ECSPI_CONTROLREG		0x08	/* Control register */
#define MX_ECSPI_CONFIGREG		0x0C	/* Config register */
#define MX_ECSPI_INTREG			0x10	/* Interrupt control register */
#define MX_ECSPI_DMAREG			0x14	/* DMA control register */
#define MX_ECSPI_STATREG		0x18    /* Status register */
#define MX_ECSPI_PERIODREG		0x1C	/* Sample period control register */
#define	MX_ECSPI_TESTREG		0x20	/* Test register */
#define	MX_ECSPI_MSGDATAREG		0x40	/* Message data register */

// CONTROLREG BIT Definitions
#define ECSPI_CONTROLREG_ENABLE		0x1
#define ECSPI_CONTROLREG_HW		0x2
#define ECSPI_CONTROLREG_XCH		0x4
#define ECSPI_CONTROLREG_SMC            	0x8
#define ECSPI_CONTROLREG_CH_MODE_POS	4
#define ECSPI_CONTROLREG_MASTERMODE(i)	( (1 << i) << ECSPI_CONTROLREG_CH_MODE_POS)
#define ECSPI_CONTROLREG_DRCTL_MASK	0x30000
#define ECSPI_CONTROLREG_DRCTL_POS	16
#define ECSPI_CONTROLREG_DRCTL_EDGE      1
#define ECSPI_CONTROLREG_DRCTL_LEVEL     2
#define ECSPI_CONTROLREG_CSEL_MASK	0xC0000
#define ECSPI_CONTROLREG_CSEL_POS	18
#define ECSPI_CONTROLREG_BCNT_MASK	0xFFF00000
#define ECSPI_CONTROLREG_BCNT_POS	20
#define ECSPI_CONREG_PREDIVIDR_MASK     0x0000F000
#define ECSPI_CONREG_PREDIVIDR_POS	12
#define ECSPI_CONREG_POSTDIVIDR_MASK    0x00000F00
#define ECSPI_CONREG_POSTDIVIDR_POS	8


// CONFIGREG BIT Definitions
#define ECSPI_CONFIGREG_SSCTL		0x0f00
#define ECSPI_CONFIGREG_SSCTL_POS	8
#define ECSPI_CONFIGREG_POL		0xf0
#define ECSPI_CONFIGREG_POL_MASK	4
#define ECSPI_CONFIGREG_PHA		0xf
#define ECSPI_CONFIGREG_PHA_MASK	0
#define ECSPI_CONFIGREG_SSPOL		0xf000
#define ECSPI_CONFIGREG_SSPOL_MASK	12
#define ECSPI_CONFIGREG_DATACTL		0xf0000
#define ECSPI_CONFIGREG_CLKCTL		0xf00000


// INTREG BIT Definitions
#define ECSPI_INTREG_TEEN		0x1
#define ECSPI_INTREG_TDREN		0x2
#define ECSPI_INTREG_TFEN		0x4
#define ECSPI_INTREG_RREN		0x8
#define ECSPI_INTREG_RDREN		0x10
#define ECSPI_INTREG_RFEN		0x20
#define ECSPI_INTREG_ROEN		0x40
#define ECSPI_INTREG_TCEN 		0x80


// STATREG (Status Reg) BIT Definitions
#define ECSPI_STATREG_TE		0x1
#define ECSPI_STATREG_TDR		0x2
#define ECSPI_STATREG_TF		0x4
#define ECSPI_STATREG_RR		0x8
#define ECSPI_STATREG_RDR		0x10
#define ECSPI_STATREG_RF		0x20
#define ECSPI_STATREG_RO		0x40
#define ECSPI_STATREG_TC		0x80

// PERIODREG Definitions
#define ECSPI_PERIODREG_SP_MASK		0x7fff;
#define ECSPI_PERIODREG_32K_CLK		0x8000
#define ECSPI_PERIODREG_CSD_CTRL	0x3f0000

// TESTREG BIT Definitions
#define ECSPI_TESTREG_LOOPBACK		(1 << 31)
#define ECSPI_TESTREG_RXCNT		8
#define ECSPI_TESTREG_RXCNT_MASK	0x7f00
#define ECSPI_TESTREG_TXCNT_MASK	0x007F

typedef struct {
	unsigned	pbase;
	uintptr_t	vbase;
	int		irq;
	int		iid;
	int		chid, coid;
	uint32_t	ctrl;
	uint32_t	bitrate;
	uint32_t	clock;
	int		dlen;
	int		dtime;	/* usec per data, for time out use */
	struct sigevent		spievent;
} mx_ecspi_t;

#endif

__SRCVERSION("$URL$ $Rev$");
