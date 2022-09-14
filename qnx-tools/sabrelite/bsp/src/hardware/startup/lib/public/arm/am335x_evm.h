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

#ifndef __AM335X_BDID_H_INCLUDED
#define __AM335X_BDID_H_INCLUDED

#define RGMII 1
#define GMII 2
#define RMII 3

#define AM335X_I2C0_CPLD					0x35
#define AM335X_I2C0_BBID					0x50
#define AM335X_I2C0_DRID					0x51
#define AM335X_I2C0_DYID					0x52

#define AM335X_BDID_HEADER_LEN				 4
#define AM335X_BDID_BDNAME_LEN				 8
#define AM335X_BDID_VERSION_LEN				 4
#define AM335X_BDID_SERIAL_LEN				12
#define AM335X_BDID_CONFIG_LEN				32
#define AM335X_BDID_MAC_LEN					 6

#define AM335X_BDID_SKU_LEN					 6

#define AM335X_BDID_BDNAME_OFFSET			(AM335X_BDID_HEADER_LEN)
#define AM335X_BDID_VERSION_OFFSET			(AM335X_BDID_BDNAME_OFFSET  +AM335X_BDID_BDNAME_LEN)
#define AM335X_BDID_SERIAL_OFFSET			(AM335X_BDID_VERSION_OFFSET +AM335X_BDID_VERSION_LEN)
#define AM335X_BDID_CONFIG_OFFSET			(AM335X_BDID_SERIAL_OFFSET  +AM335X_BDID_SERIAL_LEN)
#define AM335X_BDID_MAC1_OFFSET				(AM335X_BDID_CONFIG_OFFSET  +AM335X_BDID_CONFIG_LEN)
#define AM335X_BDID_MAC2_OFFSET				(AM335X_BDID_MAC1_OFFSET    +AM335X_BDID_MAC_LEN)

#define AM335X_MACS							3

typedef struct board_identity
{
	unsigned int	header;
	char			bdname [AM335X_BDID_BDNAME_LEN+1];
	char			version[AM335X_BDID_VERSION_LEN+1];
	char			serial [AM335X_BDID_SERIAL_LEN+1];
	char			config [AM335X_BDID_CONFIG_LEN+1];
	uint8_t			macaddr[AM335X_MACS][AM335X_BDID_MAC_LEN];
} BDIDENT;

#define AM335X_CPLDREG_DEVICE_HDR			0x00
#define AM335X_CPLDREG_DEVICE_ID			0x04
#define AM335X_CPLDREG_DEVICE_REV			0x0C
#define AM335X_CPLDREG_CFG_REG				0x10

#define AM335X_CPLD_HEADER_LEN				 4
#define AM335X_CPLD_IDENT_LEN				 8
#define AM335X_CPLD_REV_LEN					 4
#define AM335X_CPLD_CONFIG_LEN				 2

typedef struct cpld_profile
{
	unsigned int	header;
	char			identification[AM335X_CPLD_IDENT_LEN+1];
	char			revision[AM335X_CPLD_REV_LEN+1];
	unsigned int	configuration;
} CPLDPROF;

enum enum_basebd_type
{
	bb_not_detected	= 0,
	bb_A33515BB		= 15,		/* AM335x 15x15 Base Board */
	bb_A33513BB		= 13,		/* AM335x 13x13 Base Board */
	bb_unknown		= 99,
};

enum enum_basebd_cfg
{
	bb_sku00		= 0,		/* base board for low cost evm                 */
	bb_sku01		= 1,		/* base board for gen purpose evm              */
	bb_sku02		= 2,		/* base board for industrial motor control evm */
	bb_sku03		= 3,		/* base board for ip phone evm                 */
};

enum enum_daughter_type
{
	dr_not_detected	= 0,
	dr_A335GPBD		= 1,		/* AM335x General Purpose Daughterboard       */
	dr_A335IAMC		= 2,		/* AM335x Industrial Automation Daughterboard */
	dr_A335IPPH		= 3,		/* AM335x IP Phone Daughterboard              */
	dr_unknown		= 99,
};

enum enum_daughter_cfg
{
	dr_sku00		= 0,		/* base board for low cost evm                 */
};

enum enum_display_type
{
	dy_not_detected	= 0,
	dy_A335LCDA		= 1,		/* AM335x LCD A Board */
	dy_unknown		= 99,
};


typedef struct am335x_evm_id
{
	unsigned int			lowcost;	/* low cost or general purpose board */
	unsigned int			evmsize;	/* 13 for 13x13, 15 for 15x15        */

	/* Base board */
	enum enum_basebd_type	basebd_type;
	unsigned int			basebd_cfg;

	/* Daughter board */
	enum enum_daughter_type	daughterbd_type;
	unsigned int			daughterbd_cfg;

	/* Display board */
	enum enum_display_type	displaybd_type;
	unsigned int			displaybd_cfg;

	/* profile */
	unsigned int			profile;

} AM335X_EVM_ID;

void am335x_evm_init_raminfo(int locost);


#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/lib/public/arm/am335x_evm.h $ $Rev: 739323 $")
#endif
