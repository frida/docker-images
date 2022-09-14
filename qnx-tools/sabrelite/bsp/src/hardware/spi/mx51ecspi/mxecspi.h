/*
 * $QNXLicenseC:
 * Copyright 2010, QNX Software Systems.
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


#ifndef _MX51_CSPI_H_INCLUDED
#define _MX51_CSPI_H_INCLUDED

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

#define	MX51_CSPI_PRIORITY				21
#define	MX51_CSPI_EVENT					1
#define	MX35_DMA_PULSE_PRIORITY			21
#define	MX35_DMA_PULSE_CODE				1

#define MX51_ECSPI1_BASE				0x70010000
#define MX51_ECSPI2_BASE				0x83fac000
#define MX51_ECSPI1_SIZE				0x4000
#define MX51_ECSPI2_SIZE				0x4000
#define MX51_CSPI_BURST_MAX				0x200 
#define MX51_ECSPI1_IRQ					36
#define MX51_ECSPI2_IRQ					37
#define MX51_CSPI_FIFO_SIZE				0x40
#define MX51_CSPI_FIFO_RXMARK			(MX51_CSPI_FIFO_SIZE >> 1)
#define MX51_CSPI_CHANNEL_MAX			4

#define MX51_CSPI_RXDATA				0x00	/* Receive data register */
#define MX51_CSPI_TXDATA				0x04	/* Transmit data register */
#define MX51_CSPI_CONTROLREG			0x08	/* Control register */
#define MX51_CSPI_CONFIGREG				0x0C	/* Config register */
#define MX51_CSPI_INTREG				0x10	/* Interrupt control register */
#define MX51_CSPI_DMAREG				0x14	/* DMA control register */
#define MX51_CSPI_STATREG				0x18    /* Status register */
#define MX51_CSPI_PERIODREG				0x1C	/* Sample period control register */
#define	MX51_CSPI_TESTREG				0x20	/* Test register */
#define	MX51_CSPI_MSGDATAREG			0x40	/* Message data register */

// CONTROLREG BIT Definitions
#define CSPI_CONTROLREG_ENABLE			0x1
#define CSPI_CONTROLREG_HW				0x2
#define CSPI_CONTROLREG_XCH				0x4
#define CSPI_CONTROLREG_SMC            	0x8
#define CSPI_CONTROLREG_CH_MODE_POS		4
#define CSPI_CONTROLREG_CH_MODE_MASK	0x000000f0
#define CSPI_CONTROLREG_POST_DVDR		0x00000f00
#define CSPI_CONTROLREG_PRE_DVDR		0x0000f000
#define CSPI_CONTROLREG_DRCTL_MASK		0x00030000
#define CSPI_CONTROLREG_DRCTL_POS		16
#define CSPI_CONTROLREG_DRCTL_EDGE      1
#define CSPI_CONTROLREG_DRCTL_LEVEL     2
#define CSPI_CONTROLREG_CSEL_MASK		0x000C0000
#define CSPI_CONTROLREG_CSEL_POS		18
#define CSPI_CONTROLREG_BCNT_MASK		0xFFF00000
#define CSPI_CONTROLREG_BCNT_POS		20
#define CSPI_CONREG_PREDIVIDR_POS		12
#define CSPI_CONREG_POSTDIVIDR_POS		8

// CONFIGREG BIT Definitions
#define CSPI_CONFIGREG_PHA_MASK			0x0000000f
#define CSPI_CONFIGREG_PHA_POS			0
#define CSPI_CONFIGREG_POL_MASK			0x000000f0
#define CSPI_CONFIGREG_POL_POS			4
#define CSPI_CONFIGREG_SSCTL_MASK		0x00000f00
#define CSPI_CONFIGREG_SSCTL_POS		8
#define CSPI_CONFIGREG_SSPOL_MASK		0x0000f000
#define CSPI_CONFIGREG_SSPOL_POS		12
#define CSPI_CONFIGREG_DATACTL_MASK 	0x000f0000
#define CSPI_CONFIGREG_DATACTL_POS		16
#define CSPI_CONFIGREG_CLKCTL_MASK		0x00f00000
#define CSPI_CONFIGREG_CLKCTL_POS		20

// INTREG BIT Definitions
#define CSPI_INTREG_TEEN				0x1
#define CSPI_INTREG_TDREN				0x2
#define CSPI_INTREG_TFEN				0x4
#define CSPI_INTREG_RREN				0x8
#define CSPI_INTREG_RDREN				0x10
#define CSPI_INTREG_RFEN				0x20
#define CSPI_INTREG_ROEN				0x40
#define CSPI_INTREG_TCEN				0x80

// STATREG (Status Reg) BIT Definitions
#define CSPI_STATREG_TE					0x1
#define CSPI_STATREG_TDR				0x2
#define CSPI_STATREG_TF					0x4
#define CSPI_STATREG_RR					0x8
#define CSPI_STATREG_RDR				0x10
#define CSPI_STATREG_RF					0x20
#define CSPI_STATREG_RO					0x40
#define CSPI_STATREG_TC					0x80

// PERIODREG Definitions
#define CSPI_PERIODREG_SP_MASK			0x00007fff
#define CSPI_PERIODREG_32K_CLK			0x00008000
#define CSPI_PERIODREG_CSD_CTRL_MASK	0x003f0000
#define CSPI_PERIODREG_CSD_CTRL_POS		16

// TESTREG BIT Definitions
#define CSPI_TESTREG_LOOPBACK			(1 << 31)
#define CSPI_TESTREG_RXCNT				8
#define CSPI_TESTREG_RXCNT_MASK			0x00007f00
#define CSPI_TESTREG_TXCNT_MASK			0x0000007F

struct ss_errata_info;
enum { GPIO1, GPIO2, GPIO3, GPIO4, NUM_GPIO };

#define MX53_GPIO_SZ				(0x3fff)
#define IMX53_GPIO_DR				(0x0)
#define IMX53_GPIO_GDIR				(0x4)

typedef struct {
	unsigned  pbase;							/* the physical base address of GPIO port used as chip select */
	uintptr_t vbase;							/* the virtual base address of GPIO port used as chip select */
#define GPIOCS_CSHOLD				(1 << 31)	/* SPI_MODE_CSHOLD_HIGH is set by spi_setcfg */
#define GPIOCS_EN					(1 << 30)	/* GPIO based chip selection is enabled */
#define MSK_GPIO					0x0000001f	/* get only the GPIO number used for chip selection */
	uint32_t gpio[4];							/* combination of the GPIO number & flags mentioned previously */
} gpiocs_info_t;

typedef struct {
	SPIDEV			spi;	/* has to be the first element */

	unsigned		pbase;
	uintptr_t		vbase;
	uintptr_t		iomuxc_vbase;
	uintptr_t 		gpio_vbase[NUM_GPIO];
	int				irq;
	int				iid;
	int				chid, coid;

	uint32_t		clock;
	uint32_t		waitstates;
	uint32_t		csdelay;

	uint8_t			*pbuf;
	int				xlen, tlen, rlen;
	int				bxlen;	/* used for mutiply burst, exchange length for the current burst*/
	int				btlen;	/* used for mutiply burst, transmit length for the current burst*/
	int				brlen;	/* used for mutiply burst, recive length for the current burst*/
	int				dlen;
	int				dtime;	/* usec per burst, for time out use */

	int				loopback;
	int				burst;
	int				lsb_adjust;
	int				errata_en;
	const struct 	spi_errata_info *errata;
	struct sigevent	spievent;
	gpiocs_info_t	gpiocs;
} mx51_cspi_t;

extern void *mx51_init(void *hdl, char *options);
extern void mx51_dinit(void *hdl);
extern void *mx51_xfer(void *hdl, uint32_t device, uint8_t *buf, int *len);
extern int mx51_setcfg(void *hdl, uint16_t device, spi_cfg_t *cfg);

extern int mx51_devinfo(void *hdl, uint32_t device, spi_devinfo_t *info);
extern int mx51_drvinfo(void *hdl, spi_drvinfo_t *info);

extern int mx51_xchange(mx51_cspi_t *mx51, uint8_t *buf, int len);
extern int mx51_read(mx51_cspi_t *mx51, uint8_t *buf, int len);
extern int mx51_write(mx51_cspi_t *mx51, uint8_t *buf, int len);

extern int mx51_attach_intr(mx51_cspi_t *mx51);
extern int mx51_wait(mx51_cspi_t *dev, int len);

extern int mx51_cfg(void *hdl, spi_cfg_t *cfg);

extern int mx51_dma_init(mx51_cspi_t *mx51);
extern void mx51_dma_config_xfer(mx51_cspi_t *mx51,uint8_t *buf,int len,int xfer_width);
extern int mx51_dma_wait(mx51_cspi_t *mx51);


#endif


__SRCVERSION("$URL$ $Rev$");
