/*
 * $QNXLicenseC:
 * Copyright 2008, QNX Software Systems. 
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




#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <hw/inout.h>
#include <sys/syspage.h>
#include <inttypes.h>
#include <hw/sysinfo.h>
#include <drvr/hwinfo.h>

#include "canmx6x.h"
#include "proto.h"
#include "externs.h"

// Default IRQ's for RINGO board
#define RINGO_CAN0_SYSINTR		43
#define RINGO_CAN1_SYSINTR		44

// Define default bitrate settings for i.mx6xADS RINGO board
// based on CAN Bit Rate Calculation and rules from RINGO 
// FlexCAN Reference Guide.  
// The PSEG1, PSEG2, RJW and PROPSEG values are held constant and the
// PRESDIV value is changed for the different default bitrates.

// The FlexCAN module uses CTRL register to set-up the bit timing parameters required by the CAN protocol.
// Control register (CANCTRL) contains the PROPSEG = PROP_SEG(Bit 0-3),
// PSEG1 = PHASE_SEG1 (Bit 19-21), PSEG2 = PHASE_SEG2 (Bit 16-18),
// and the RJW (Bit 22-23) fields which allow the user to configure the bit timing parameters.
// The prescaler divide register (PRESDIV) allows the user to select the ratio used to derive the clock from the system clock. 
// For the position of the sample point only the relation (SYNC_SEG + PROP_SEG + PHASE_SEG1) / (PHASE_SEG2) is important.
// The values for PRESDIV, PROPSEG, PSEG1 and PSEG2 are as given below.

#define CAN_RJW	    				0
#define CAN_PROPSEG					0x02
#define CAN_PSEG1					0x07
#define CAN_PSEG2					0x01

/* Generated bit rate values by http://www.port.de/engl/canprod/sv_req_form.html */
/* Bitrate values for XTAL 24.5 MHz, desired Sample Point at 87.5% */
#define CAN_PRESDIV_10K_XTAL		0xAE
#define CAN_PRESDIV_50K_XTAL		0x22
#define CAN_PRESDIV_125K_XTAL		0x0d
#define CAN_PRESDIV_250K_XTAL		0x06

/* Bitrate table for PLL3 30 MHz, desired Sample Point at 87.5% */
#define CAN_PRESDIV_50K_PLL			0x4a
#define CAN_PRESDIV_125K_PLL		0x1d
#define CAN_PRESDIV_250K_PLL		0x0d

//#define DEBUG_DRVR

// Function prototypes
void device_init(int argc, char *argv[]);
void create_device(CANDEV_RINGO_INIT *devinit);

typedef struct candev_hwinfo {
	paddr_t regbase;
	paddr_t reglen;
	paddr_t membase;
	paddr_t memlen;
	unsigned irqvector;
} CANDEV_HWINFO;


static int get_can_hwinfo(CANDEV_HWINFO *candev, unsigned unit)
{
	unsigned hwi_off;
        
	hwi_off = hwi_find_bus(HWI_ITEM_BUS_CAN, unit);
	if (hwi_off != HWI_NULL_OFF)
	{
		hwiattr_can_t attr;
		
		hwiattr_get_can(hwi_off, &attr);

		candev->regbase = attr.common.location.base;

		if (attr.common.num_irq > 0)
			candev->irqvector = hwitag_find_ivec(hwi_off, NULL);

		if (attr.num_memaddr > 0)
		{
			unsigned instance = 1;
			hwi_tag *tag = hwi_tag_find(hwi_off, HWI_TAG_NAME_location, &instance);
			candev->membase = tag->location.base;
		}

		return 0;
	}
	else
		return -1;
}


int main(int argc, char *argv[])
{
	// Driver implemented functions called by CAN library
	can_drvr_funcs_t 	drvr_funcs = {can_drvr_transmit, can_drvr_devctl};

	// Get I/O privity
	ThreadCtl( _NTO_TCTL_IO, 0 );

	// Initialize Resource Manager
    can_resmgr_init(&drvr_funcs);

	// Process options and create devices
	device_init(argc, argv);

	// Start Handling Clients
	can_resmgr_start();

	return EXIT_SUCCESS;
}

void device_init(int argc, char *argv[])
{
	int 					opt, hwi_can0, hwi_can1;
	int						numcan = 0;
	char					*cp;
	CANDEV_HWINFO			can0, can1;

	// Set default options
	CANDEV_RINGO_INIT 	devinit = 
	{
		{ 	CANDEV_TYPE_RX, 				/* devtype */	
			0,								/* can_unit - set this later */
			0,								/* dev_unit - set this later*/
			100, 							/* msgnum */
			RINGO_CANMCF_DLC_BYTE8, 		/* datalen */ 
		},
		RINGO_CAN0_REG_BASE, 				/* port */
		RINGO_CAN0_MEM_BASE, 				/* mem */
		RINGO_CAN_CLK_PLL, 					/* clk */
		0,						 			/* bitrate */
		CAN_PRESDIV_50K_PLL,				/* br_presdiv */
		CAN_PROPSEG,						/* br_propseg */
		CAN_RJW,							/* br_rjw */
		CAN_PSEG1,							/* br_pseg1 */
		CAN_PSEG2,							/* br_pseg2 */
		RINGO_CAN0_SYSINTR,					/* irqsys for RINGO board */
		0,//(INIT_FLAGS_MDRIVER_INIT),		/* flags */
		RINGO_CAN_NUM_MAILBOX_FLEXCAN/2,	/* numrx */
		0x100C0000,							/* midrx */
		0x100C0000,							/* midtx */
		0x0,								/* timestamp */
	};

    hwi_can0 = hwi_can1 = -1;
	hwi_can0 = get_can_hwinfo(&can0, 0);
	hwi_can1 = get_can_hwinfo(&can1, 1);

	// Process command line options and create associated devices
	while(optind < argc)
	{
    	// Process dash options
		while((opt = getopt(argc, argv, "ab:B:c:Di:l:m:Mn:psStu:vwxz")) != -1)
		{
			switch(opt){
			case 'a':
				devinit.flags |= INIT_FLAGS_AUTOBUS;
				break;
			case 'b':
				// Set CANCTRL params to default
				devinit.br_propseg = CAN_PROPSEG;
			    devinit.br_pseg1 = CAN_PSEG1;
			    devinit.br_pseg2 = CAN_PSEG2;
			    devinit.br_rjw = CAN_RJW;

				// Determine BRP value for desired bitrate
				if(strncmp(optarg, "50K", 3) == 0)
					devinit.br_presdiv = CAN_PRESDIV_50K_PLL;
				else if(strncmp(optarg, "125K", 4) == 0)
					devinit.br_presdiv = CAN_PRESDIV_125K_PLL;
				else if(strncmp(optarg, "250K", 4) == 0)
					devinit.br_presdiv = CAN_PRESDIV_250K_PLL;
				else
					// Set default to 50K
					devinit.br_presdiv = CAN_PRESDIV_50K_PLL;
				break;
			case 'B':
				// Values to program bitrate manually
				devinit.br_presdiv = strtoul(optarg, &optarg, 0);
				
				if((cp = strchr(optarg, ',')))
				{
					cp += 1;	// Skip over the ','
				    devinit.br_propseg = strtoul(cp, &cp, 0);
				}
				if((cp = strchr(cp, ',')))
				{
					cp += 1;	// Skip over the ','
				    devinit.br_pseg1 = strtoul(cp, &cp, 0);
				}
				if((cp = strchr(cp, ',')))
				{
					cp += 1;	// Skip over the ','
				    devinit.br_pseg2 = strtoul(cp, &cp, 0);
				}
				if((cp = strchr(cp, ',')))
				{
					cp += 1;	// Skip over the ','
				    devinit.br_rjw = strtoul(cp, &cp, 0);
				}

				// Check for valid bitrate settings
				if(devinit.br_rjw > RINGO_CANCTRL_RJW_MAXVAL ||
				   devinit.br_pseg1 > RINGO_CANCTRL_PSEG1_MAXVAL ||
				   devinit.br_pseg2 > RINGO_CANCTRL_PSEG2_MAXVAL ||
				   devinit.br_pseg2 == 0) 
				{
					fprintf(stderr, "Invalid manual bitrate settings\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 'c':
				if(strncmp(optarg, "24M", 3) == 0)
					devinit.clk = RINGO_CAN_CLK_EXTAL;
				else if(strncmp(optarg, "30M", 3) == 0)
					devinit.clk = RINGO_CAN_CLK_PLL;
				else
					devinit.clk = strtoul(optarg, NULL, 0);
				break;
			case 'D':
				devinit.flags &= ~INIT_FLAGS_MDRIVER_INIT;
				break;
			case 'i':
				devinit.midrx = strtoul(optarg, &optarg, 16);
				if((cp = strchr(optarg, ',')))
				{
				    devinit.midtx = strtoul(cp + 1, NULL, 0);
				}
				break;
			case 'l':
				devinit.cinit.datalen = strtoul(optarg, NULL, 0);
				if(devinit.cinit.datalen > RINGO_CANMCF_DLC_BYTE8)
				{
					fprintf(stderr, "Invalid CAN message data length, setting to %d\n", RINGO_CANMCF_DLC_BYTE8);
					devinit.cinit.datalen = RINGO_CANMCF_DLC_BYTE8;			/* limit CAN message length to 8 */
				}
				break;
			case 'm':
				devinit.flags |= INIT_FLAGS_TIMESTAMP;
				devinit.timestamp = strtoul(optarg, NULL, 16);
				break;
			case 'M':
				devinit.flags |= INIT_FLAGS_RX_FULL_MSG;
				break;
			case 'n':
				devinit.cinit.msgnum = strtoul(optarg, NULL, 0);
				break;
			case 'p':
				devinit.flags |= INIT_FLAGS_CLKSRC;
				break;
			case 's':
				devinit.flags |= INIT_FLAGS_BITRATE_SAM;
				break;
			case 'S':
				devinit.flags |= INIT_FLAGS_MDRIVER_SORT;
				break;
			case 't':
				devinit.flags |= INIT_FLAGS_LOOPBACK;
				break;
			case 'u':
				devinit.cinit.can_unit = strtoul(optarg, NULL, 0);
				break;
			case 'v':
				devinit.flags |= INIT_FLAGS_LOM;
				break;
			case 'w':
				devinit.flags |= INIT_FLAGS_LBUF;
				break;
			case 'x':
				devinit.flags |= INIT_FLAGS_EXTENDED_MID;
				break;
			case 'z':
				devinit.flags |= INIT_FLAGS_TSYN;
				break;
			default:
				break;
			}
		}

		// Ensure message ID is valid
		if(devinit.flags & INIT_FLAGS_EXTENDED_MID)
		{
			devinit.midrx &= RINGO_CANMID_MASK_EXT;
			devinit.midtx &= RINGO_CANMID_MASK_EXT;
		}
		else
		{
			devinit.midrx &= RINGO_CANMID_MASK_STD;
			devinit.midtx &= RINGO_CANMID_MASK_STD;
		}

		// Process ports and interrupt
		while(optind < argc && *(optarg = argv[optind]) != '-')
		{
			// Set defaults for RINGO Board CAN 1 
			if(strncmp(optarg, "ringocan0", 9) == 0)
			{
				if (0 == hwi_can0) {
					devinit.port = can0.regbase;
					devinit.mem = can0.membase;
					devinit.irqsys = can0.irqvector;
				} else {	// default values, They should be removed once all startups have been changed
					devinit.port = RINGO_CAN0_REG_BASE;
					devinit.mem = RINGO_CAN0_MEM_BASE;
					devinit.irqsys = RINGO_CAN0_SYSINTR;
				}
				// Set default can unit number
				if(!devinit.cinit.can_unit)
					devinit.cinit.can_unit = 0;
			}
			// Set defaults for RINGO Board CAN 2 
			else if(strncmp(optarg, "ringocan1", 9) == 0)
			{
				if (0 == hwi_can1) {
					devinit.port = can1.regbase;
					devinit.mem = can1.membase;
					devinit.irqsys = can1.irqvector;
				} else {	// default values, They should be removed once all startups have been changed
					devinit.port = RINGO_CAN1_REG_BASE;
					devinit.mem = RINGO_CAN1_MEM_BASE;
					devinit.irqsys = RINGO_CAN1_SYSINTR;
				}
				// Set default can unit number
				if(!devinit.cinit.can_unit)
					devinit.cinit.can_unit = 1;
			}
			// Set user defined irq's
			else
			{
				// Set default port for CAN 1 
				if(strncmp(optarg, "can0", 4) == 0)
				{
					if (0 == hwi_can0) {
						devinit.port = can0.regbase;
						devinit.mem = can0.membase;
						devinit.irqsys = can0.irqvector;
					} else {	// default values, They should be removed once all startups have been changed
						devinit.port = RINGO_CAN0_REG_BASE;
						devinit.mem = RINGO_CAN0_MEM_BASE;
						// Set defaults even though user may override them
						devinit.irqsys = RINGO_CAN0_SYSINTR;
					}
					// Set default can unit number
					if(!devinit.cinit.can_unit)
						devinit.cinit.can_unit = 0;
					// Increment optarg
					optarg += 4; //sizeof("can1") - 1;
				}
				// Set default port for CAN 2 
				else if(strncmp(optarg, "can1", 4) == 0)
				{
					if (0 == hwi_can1) {
						devinit.port = can1.regbase;
						devinit.mem = can1.membase;
						devinit.irqsys = can1.irqvector;
					} else {	// default values, They should be removed once all startups have been changed
						devinit.port = RINGO_CAN1_REG_BASE;
						devinit.mem = RINGO_CAN1_MEM_BASE;
						// Set defaults even though user may override them
						devinit.irqsys = RINGO_CAN1_SYSINTR;
					}
					// Set default can unit number
					if(!devinit.cinit.can_unit)
						devinit.cinit.can_unit = 1;
					// Increment optarg
					optarg += 4; //sizeof("can2") - 1;
				}
				else
				{
					fprintf(stderr, "Invalid options\n");
					exit(EXIT_FAILURE);
				}
				// Set system interrupt vector
				if(*optarg == ',') devinit.irqsys = strtoul(optarg + 1, NULL, 0);
			}
			++optind;

			// Create the CAN device
			create_device(&devinit);
			// Reset unit number for next device
			devinit.cinit.can_unit = 0;
			numcan++;
		}	
	}

	// If no devices have been created yet, create the default device 
	if(numcan == 0)
	{
		// Create the default CAN device
		devinit.cinit.can_unit = 1;
		create_device(&devinit);
	}
}

void create_device(CANDEV_RINGO_INIT *devinit)
{
	CANDEV_RINGO_INFO		*devinfo;
	CANDEV_RINGO 			*devlist;
	int						mdriver_intr = -1;
	int 					i;

#ifdef DEBUG_DRVR
	fprintf(stderr, "port = 0x%X\n", devinit->port);
	fprintf(stderr, "mem = 0x%X\n", devinit->mem);
	fprintf(stderr, "clk = %d\n", devinit->clk);
	fprintf(stderr, "bitrate = %d\n", devinit->bitrate);
	fprintf(stderr, "presdiv = %d\n", devinit->br_presdiv);
	fprintf(stderr, "propseg = %d\n", devinit->br_propseg);
	fprintf(stderr, "rjw = %d\n", devinit->br_rjw);
	fprintf(stderr, "pseg1 = %d\n", devinit->br_pseg1);
	fprintf(stderr, "pseg2 = %d\n", devinit->br_pseg2);
	fprintf(stderr, "irqsys = %d\n", devinit->irqsys);
	fprintf(stderr, "msgnum = %u\n", devinit->cinit.msgnum);
	fprintf(stderr, "datalen = %u\n", devinit->cinit.datalen);
	fprintf(stderr, "unit = %u\n", devinit->cinit.can_unit);
	fprintf(stderr, "flags = %u\n", devinit->flags);
	fprintf(stderr, "numrx = %u\n", devinit->numrx);
	fprintf(stderr, "midrx = 0x%X\n", devinit->midrx);
	fprintf(stderr, "midtx = 0x%X\n", devinit->midtx);
#endif

	// Allocate device info
	devinfo = (void *) _smalloc(sizeof(*devinfo));
	if(!devinfo)
	{
		fprintf(stderr, "devinfo: _smalloc failed\n");
		exit(EXIT_FAILURE);
	}
	memset(devinfo, 0, sizeof(*devinfo));

	// Allocate an array of devices - one for each mailbox
	devlist = (void *) _smalloc(sizeof(*devlist) * RINGO_CAN_NUM_MAILBOX_FLEXCAN);
	if(!devlist)
	{
		fprintf(stderr, "devlist: _smalloc failed\n");
		exit(EXIT_FAILURE);
	}
	memset(devlist, 0, sizeof(*devlist) * RINGO_CAN_NUM_MAILBOX_FLEXCAN);

	// Map device registers
	devinfo->base = mmap_device_io(RINGO_CAN_REG_SIZE_FLEXCAN, devinit->port);
	if(devinfo->base == MAP_DEVICE_FAILED)
	{
		perror("CAN REG: Can't map device I/O");
		exit(EXIT_FAILURE);
	}

	// Determine if there is an active mini-driver and initialize driver to support it
	if(devinit->flags & INIT_FLAGS_MDRIVER_INIT)
	{
		mdriver_intr = mdriver_init(devinfo, devinit);
	}

	// Map device message memory
	devinfo->canmsg = mmap_device_memory(NULL, RINGO_CAN_MEM_SIZE_FLEXCAN,
				PROT_READ|PROT_WRITE|PROT_NOCACHE, 0, devinit->mem);
	if(devinfo->canmsg == MAP_FAILED)
	{
		perror("CAN MSG: Can't map device memory");
		exit(EXIT_FAILURE);
	}
	// Clear the mailbox memory if there is no mini-driver
	if(!(devinit->flags & INIT_FLAGS_MDRIVER_INIT) || mdriver_intr == -1)
		memset(devinfo->canmsg, 0, RINGO_CAN_MEM_SIZE_FLEXCAN);

	// Map CANLAM memory
	devinfo->canlam = mmap_device_io(RINGO_CANLAM_MEM_SIZE, devinit->port + RINGO_CANRXIMR0);
	if(devinfo->canlam == MAP_DEVICE_FAILED)
	{
		perror("CAN IMR: Can't map device I/O");
		exit(EXIT_FAILURE);
	}

	// Setup device info
	devinfo->devlist = devlist;
	// Setup the RX and TX mailbox sizes
	devinfo->numrx = devinit->numrx;
	devinfo->numtx = RINGO_CAN_NUM_MAILBOX_FLEXCAN - devinit->numrx;

	// Initialize flags
	if(devinit->flags & INIT_FLAGS_RX_FULL_MSG)
		devinfo->iflags |= INFO_FLAGS_RX_FULL_MSG;

   	devinfo->iflags |= INFO_FLAGS_ENDIAN_SWAP;

	// Initialize all device mailboxes
	for(i = 0; i < RINGO_CAN_NUM_MAILBOX_FLEXCAN; i++)
	{
		// Set index into device mailbox memory
    	devlist[i].mbxid = i;
		// Store a pointer to the device info
		devlist[i].devinfo = devinfo;

		// Set device mailbox unit number 
   		devinit->cinit.dev_unit = i;
		// Set device mailbox as transmit or receive
		if(i < devinfo->numrx)
    		devinit->cinit.devtype = CANDEV_TYPE_RX;
		else
    		devinit->cinit.devtype = CANDEV_TYPE_TX;

		// Initialize the CAN device
		can_resmgr_init_device(&devlist[i].cdev, &devinit->cinit);

		// Create the resmgr device
    	can_resmgr_create_device(&devlist[i].cdev);
	}

	// Initialize device hardware and device mailboxes if there is no mini-driver
	if(!(devinit->flags & INIT_FLAGS_MDRIVER_INIT) || mdriver_intr == -1)
		can_init_hw(devinfo, devinit);

#ifdef DEBUG_DRVR
	can_print_reg(devinfo);
	can_print_mailbox(devinfo);
#endif

	// Initialize interrupts and attach interrupt handler
	can_init_intr(devinfo, devinit);

	// Add mini-driver's bufferred CAN messages if mini-driver is active.
	// Note1: This must be done BEFORE we start handling client requests and AFTER calling 
	// InterruptAttach() to ensure we don't miss any data - mini-driver has not ended until 
	// InterruptAttach is called.
	// Note2: We may receive new CAN messages (via the interrupt handler) while we add the
	// mini-driver's buffered messages to the driver's message queue.  The interrupt handler
	// will had new messages to the "head" of the message queue while the mini-driver messages
	// are added to the "tail" of the message queue - this allows us to add both old and new
	// messages to the queue in parrallel and in the correct order.
	// Note3: It is important that enough CAN messages are pre-allocated on the message 
	// queue to ensure that there is room to add both mini-driver buffered messages and
	// new message.  The number required will be be dependent on the CAN data rate and the
	// time it takes for the system to boot and the full driver to take over. 
	if(devinit->flags & INIT_FLAGS_MDRIVER_INIT && mdriver_intr != -1)
	{
		mdriver_init_data(devinfo, devinit);
	}
}



__SRCVERSION( "$URL: http://svn/product/branches/6.5.0/trunk/hardware/can/mx6x/driver.c $ $Rev: 590811 $" );
