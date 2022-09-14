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







#ifndef __CAN_DCMD_H_INCLUDED
#define __CAN_DCMD_H_INCLUDED

#include <stdint.h>

#ifndef _DEVCTL_H_INCLUDED
	#include <devctl.h>
#endif

#define CAN_MSG_DATA_MAX		0x8		/* Max number of data bytes in a CAN message as defined by CAN spec */

/* Extended CAN Message */
typedef struct can_msg_ext {
	uint32_t 		timestamp;  	/* CAN message timestamp */
} CAN_MSG_EXT;

/* CAN Message */
typedef struct can_msg {
	/* Pre-allocate CAN messages to the max data size */
	uint8_t 		dat[CAN_MSG_DATA_MAX];	/* CAN message data */
	uint8_t			len;					/* Actual CAN message data length */
	uint32_t 		mid;   					/* CAN message identifier */
	CAN_MSG_EXT		ext;					/* Extended CAN message info */
} CAN_MSG;

/* Generic CAN Devctl Error Structure - meaning is device specific */
typedef struct can_devctl_error {
	uint32_t 		drvr1;     
	uint32_t 		drvr2;     
	uint32_t 		drvr3;     
	uint32_t 		drvr4;     
} CAN_DEVCTL_ERROR;

/* CAN Devctl Data */
typedef union
{
	uint32_t 				mid; 		/* CAN message identifier */
	uint32_t 				mfilter; 	/* Device driver defined CAN message filter */
	uint32_t 				prio;	 	/* Device driver defined CAN priority */
	uint32_t 				timestamp; 	/* Device CAN message timestamp */
	uint32_t 				print_debug_all; 	/* when printing debug info, print all or specific device */
	CAN_DEVCTL_ERROR		error;		/* Device driver specific error info */
	CAN_MSG					canmsg;		/* CAN message */
} DCMD_DATA;

#define CAN_CMD_CODE      			1
#define CAN_DEVCTL_GET_MID 			__DIOF(_DCMD_MISC, CAN_CMD_CODE + 0, uint32_t)
#define CAN_DEVCTL_SET_MID 			__DIOT(_DCMD_MISC, CAN_CMD_CODE + 1, uint32_t)
#define CAN_DEVCTL_GET_MFILTER		__DIOF(_DCMD_MISC, CAN_CMD_CODE + 2, uint32_t)
#define CAN_DEVCTL_SET_MFILTER		__DIOT(_DCMD_MISC, CAN_CMD_CODE + 3, uint32_t)
#define CAN_DEVCTL_GET_PRIO			__DIOF(_DCMD_MISC, CAN_CMD_CODE + 4, uint32_t)
#define CAN_DEVCTL_SET_PRIO			__DIOT(_DCMD_MISC, CAN_CMD_CODE + 5, uint32_t)
#define CAN_DEVCTL_GET_TIMESTAMP	__DIOF(_DCMD_MISC, CAN_CMD_CODE + 6, uint32_t)
#define CAN_DEVCTL_SET_TIMESTAMP	__DIOT(_DCMD_MISC, CAN_CMD_CODE + 7, uint32_t)
#define CAN_DEVCTL_READ_CANMSG_EXT	__DIOF(_DCMD_MISC, CAN_CMD_CODE + 8, struct can_msg)
#define CAN_DEVCTL_ERROR 			__DIOF(_DCMD_MISC, CAN_CMD_CODE + 100, struct can_devctl_error)
#define CAN_DEVCTL_DEBUG_INFO 		__DION(_DCMD_MISC, CAN_CMD_CODE + 101)
#define CAN_DEVCTL_DEBUG_INFO2 		__DIOT(_DCMD_MISC, CAN_CMD_CODE + 102, uint32_t)
#endif

__SRCVERSION( "$URL: http://svn/product/branches/6.5.0/trunk/lib/io-can/public/sys/can_dcmd.h $ $Rev: 740311 $" )
