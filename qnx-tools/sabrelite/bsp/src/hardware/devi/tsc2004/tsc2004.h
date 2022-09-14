/*
 * $QNXLicenseC:
 * Copyright 2012, QNX Software Systems.
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

#ifndef _TSC2004_H_
#define _TSC2004_H_

#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/neutrino.h>
#include <sys/time.h>
#include <hw/i2c.h>

#define TSC2004_PENIRQ		276

#define FLAG_INIT			0x1000
#define FLAG_RESET			0x2000

#define RELEASE_DELAY		100000000  /* 100 ms */
#define INTR_DELAY			100		   /* 100 ms */
#define PULSE_PRIORITY		21

#define PULSE_CODE			1

#define JITTER_DELTA		60

// I2C controller device pathname
#define TSC_I2C_DEVICE		"/dev/i2c3"

// TSC2004 slave address
#define TSC_ADDRESS			0x48

// Speed of I2C communication with TSC
#define TSC_I2C_SPEED		400000

/* Control byte 0 */
#define TSC2004_CMD0(addr, pnd, rw)		((addr<<3)|(pnd<<1)|rw)

/* Control byte 1 */
#define TSC2004_CMD1(cmd, mode, rst)	((1<<7)|(cmd<<4)|(mode<<2)|(rst<<1))

/* Command Bits */
#define READ_REG			1
#define WRITE_REG			0
#define SWRST_TRUE			1
#define SWRST_FALSE			0
#define PND0_TRUE			1
#define PND0_FALSE			0

/* Converter function mapping, which is used in Control Byte 1 */
enum convertor_function {
	MEAS_X_Y_Z1_Z2,	/* Measure X,Y,z1 and Z2:	0x0 */
	MEAS_X_Y,	/* Measure X and Y only:	0x1 */
	MEAS_X,		/* Measure X only:		0x2 */
	MEAS_Y,		/* Measure Y only:		0x3 */
	MEAS_Z1_Z2,	/* Measure Z1 and Z2 only:	0x4 */
	MEAS_AUX,	/* Measure Auxillary input:	0x5 */
	MEAS_TEMP1,	/* Measure Temparature1:	0x6 */
	MEAS_TEMP2,	/* Measure Temparature2:	0x7 */
	MEAS_AUX_CONT,	/* Continuously measure Auxillary input: 0x8 */
	X_DRV_TEST,	/* X-Axis drivers tested	0x9 */
	Y_DRV_TEST,	/* Y-Axis drivers tested	0xA */
	/*Command Reserved*/
	SHORT_CKT_TST = 0xC,	/* Short circuit test:	0xC */
	XP_XN_DRV_STAT,	/* X+,Y- drivers status:	0xD */
	YP_YN_DRV_STAT,	/* X+,Y- drivers status:	0xE */
	YP_XN_DRV_STAT	/* Y+,X- drivers status:	0xF */
};

/* Data register address mapping, for Control Byte 0 */
enum register_address {
	X_REG,		/* X register:		0x0 */
	Y_REG,		/* Y register:		0x1 */
	Z1_REG,		/* Z1 register:		0x2 */
	Z2_REG,		/* Z2 register:		0x3 */
	AUX_REG,	/* AUX register:	0x4 */
	TEMP1_REG,	/* Temp1 register:	0x5 */
	TEMP2_REG,	/* Temp2 register:	0x6 */
	STAT_REG,	/* Status Register:	0x7 */
	AUX_HGH_TH_REG,	/* AUX high threshold register:	0x8 */
	AUX_LOW_TH_REG,	/* AUX low threshold register:	0x9 */
	TMP_HGH_TH_REG,	/* Temp high threshold register:0xA */
	TMP_LOW_TH_REG,	/* Temp low threshold register:	0xB */
	CFR0_REG,	/* Configuration register 0:	0xC */
	CFR1_REG,	/* Configuration register 1:	0xD */
	CFR2_REG,	/* Configuration register 2:	0xE */
	CONV_FN_SEL_STAT	/* Convertor function select register:	0xF */
};

/* Supported Resolution modes */
enum resolution_mode {
	MODE_10BIT,	/* 10 bit resolution */
	MODE_12BIT		/* 12 bit resolution */
};

/* Configuraton register bit fields */
/* CFR0 */
#define PEN_STS_CTRL_MODE			(1 << (15 - 8))
#define ADC_STS						(1 << (14 - 8))
#define RES_CTRL					(1 << (13 - 8))
#define ADC_CLK_4MHZ				(0 << (11 - 8))
#define ADC_CLK_2MHZ				(1 << (11 - 8))
#define ADC_CLK_1MHZ				(2 << (11 - 8))
#define PANEL_VLTG_STB_TIME_0US		(0 << (8- 8))
#define PANEL_VLTG_STB_TIME_100US	(1 << (8 - 8))
#define PANEL_VLTG_STB_TIME_500US	(2 << (8 - 8))
#define PANEL_VLTG_STB_TIME_1MS		(3 << (8 - 8))
#define PANEL_VLTG_STB_TIME_5MS		(4 << (8 - 8))
#define PANEL_VLTG_STB_TIME_10MS	(5 << (8 - 8))
#define PANEL_VLTG_STB_TIME_50MS	(6 << (8 - 8))
#define PANEL_VLTG_STB_TIME_100MS	(7 << (8 - 8))
#define PRECHARGE_TIME_20US			(0 << 5)
#define PRECHARGE_TIME_84US			(1 << 5)
#define PRECHARGE_TIME_276US		(2 << 5)
#define PRECHARGE_TIME_340US		(3 << 5)
#define PRECHARGE_TIME_1044MS		(4 << 5)
#define PRECHARGE_TIME_1108MS		(5 << 5)
#define PRECHARGE_TIME_1300MS		(6 << 5)
#define PRECHARGE_TIME_1364MS		(7 << 5)
#define SENSE_TIME_SEL_32US			(0 << 2)
#define SENSE_TIME_SEL_96US			(1 << 2)
#define SENSE_TIME_SEL_544US		(2 << 2)
#define SENSE_TIME_SEL_608US		(3 << 2)
#define SENSE_TIME_SEL_2080MS		(4 << 2)
#define SENSE_TIME_SEL_2144MS		(5 << 2)
#define SENSE_TIME_SEL_2592MS		(6 << 2)
#define SENSE_TIME_SEL_2656MS		(7 << 2)
#define DETECTION_IN_WAIT			(1 << 1)
#define LONG_SAMPLE_MODE            (1 << 0)

/* CFR2 */
#define PINTS1						(1 << (15 - 8))
#define PINTS0						(1 << (14 - 8))
#define MEDIAN_VAL_FLTR_SIZE_1		(0 << (12 - 8))
#define MEDIAN_VAL_FLTR_SIZE_3		(1 << (12 - 8))
#define MEDIAN_VAL_FLTR_SIZE_7		(2 << (12 - 8))
#define MEDIAN_VAL_FLTR_SIZE_15		(3 << (12 - 8))
#define AVRG_VAL_FLTR_SIZE_1		(0 << (10 - 8))
#define AVRG_VAL_FLTR_SIZE_3_4		(1 << (10 - 8))
#define AVRG_VAL_FLTR_SIZE_7_8		(2 << (10 - 8))
#define AVRG_VAL_FLTR_SIZE_16		(3 << (10 - 8))
#define MAV_FLTR_EN_X				(1 << 4)
#define MAV_FLTR_EN_Y				(1 << 3)
#define MAV_FLTR_EN_Z				(1 << 2)

#define	MAX_12BIT					((1 << 12) - 1)
#define MEAS_MASK					MAX_12BIT

typedef struct
{
	i2c_sendrecv_t	sr;
	uint8_t		data[8];
} tsc_sr_t;

static int tsc2004_init(input_module_t *module);
static int tsc2004_devctrl(input_module_t *module, int event, void *ptr);
static int tsc2004_reset(input_module_t *module);
static int tsc2004_pulse(message_context_t *, int, unsigned, void *);
static int tsc2004_parm(input_module_t *module,int opt,char *optarg);
static int tsc2004_shutdown(input_module_t *module, int delay);
static void *intr_thread(void *data);

/* driver private data */
typedef struct _private_data
{
	int					irq;    /* IRQ to attach to */
	int					iid;    /* Interrupt ID */
	int					irq_pc; /* IRQ pulse code */

	int					chid;
	int					coid;
	pthread_attr_t		pattr;
	struct sched_param	param;
	struct sigevent		event;
	pthread_mutex_t mutex;

	struct packet_abs	tp;
	unsigned char		verbose;
	int					flags;

	unsigned			touch_x, touch_y;

	/* I2C related stuff */
	char				*i2c;
	int					fd;
	unsigned int		speed;
	i2c_addr_t			slave;

	/* Timer related stuff */
	timer_t				timerid;
	struct itimerspec	itime;

	/* conversion params */
	long				release_delay;
	int					intr_delay;
	int					jitter_delta;
	int					last_buttons;
} private_data_t;

#endif
