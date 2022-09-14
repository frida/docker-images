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




#ifndef __CANRINGO_H_INCLUDED
#define __CANRINGO_H_INCLUDED

#include <hw/can.h>


/* RINGO CAN */
#define RINGO_CAN0_REG_BASE      		0x53FE4000
#define RINGO_CAN1_REG_BASE      		0x53FE8000
#define RINGO_CAN0_MEM_BASE      		0x53FE4080
#define RINGO_CAN1_MEM_BASE      		0x53FE8080
#define RINGO_CAN_REG_SIZE_FLEXCAN		0x40
#define RINGO_CAN_MEM_SIZE_FLEXCAN		0x400
#define RINGO_CANLAM_MEM_SIZE    		0x100
#define RINGO_CAN_NUM_MAILBOX_FLEXCAN   64
#define FLEXCAN_SET_MODE_RETRIES  		255

/* The crystal oscillator clock should be selected whenever a tight
 * tolerance (up to 0.1%) is required in the CAN bus timing.
 * The crystal oscillator clock has better jitter performance
 * than PLL generated clocks.
 */
#define RINGO_CAN_CLK_EXTAL     		24576000
#define RINGO_CAN_CLK_PLL	    		30000000

/* RINGO CAN Register Offsets */
#define RINGO_CANMC               0x00
#define RINGO_CANCTRL             0x04
#define RINGO_CANTIMER            0x08
#define RINGO_CANRXGMASK          0x10
#define RINGO_CANRX14MASK         0x14
#define RINGO_CANRX15MASK         0x18
#define RINGO_CANECR              0x1C
#define RINGO_CANESR              0x20
#define RINGO_CANIMASK2           0x24
#define RINGO_CANIMASK1           0x28
#define RINGO_CANIFLAG2           0x2C
#define RINGO_CANIFLAG1           0x30

/* Message Buffers MB0–MB15 */
#define RINGO_CANMB0              0x80
#define RINGO_CANMB1              0x90
#define RINGO_CANMB2              0xA0
#define RINGO_CANMB3              0xB0
#define RINGO_CANMB4              0xC0
#define RINGO_CANMB5              0xD0
#define RINGO_CANMB6              0xE0
#define RINGO_CANMB7              0xF0
#define RINGO_CANMB8              0x100
#define RINGO_CANMB9              0x110
#define RINGO_CANMB10             0x120
#define RINGO_CANMB11             0x130
#define RINGO_CANMB12             0x140
#define RINGO_CANMB13             0x150
#define RINGO_CANMB14             0x160
#define RINGO_CANMB15             0x170

/*Message Buffers MB16–MB31 */
#define RINGO_CANMB16             0x180
#define RINGO_CANMB17             0x190
#define RINGO_CANMB18             0x1A0
#define RINGO_CANMB19             0x1B0
#define RINGO_CANMB20             0x1C0
#define RINGO_CANMB21             0x1D0
#define RINGO_CANMB22             0x1E0
#define RINGO_CANMB23             0x1F0
#define RINGO_CANMB24             0x200
#define RINGO_CANMB25             0x210
#define RINGO_CANMB26             0x220
#define RINGO_CANMB27             0x230
#define RINGO_CANMB28             0x240
#define RINGO_CANMB29             0x250
#define RINGO_CANMB30             0x260
#define RINGO_CANMB31             0x270

/* Message Buffers MB32–MB63 */
#define RINGO_CANMB32             0x280
#define RINGO_CANMB33             0x290
#define RINGO_CANMB34             0x2A0
#define RINGO_CANMB35             0x2B0
#define RINGO_CANMB36             0x2C0
#define RINGO_CANMB37             0x2D0
#define RINGO_CANMB38             0x2E0
#define RINGO_CANMB39             0x2F0
#define RINGO_CANMB40             0x300
#define RINGO_CANMB41             0x310
#define RINGO_CANMB42             0x320
#define RINGO_CANMB43             0x330
#define RINGO_CANMB44             0x340
#define RINGO_CANMB45             0x350
#define RINGO_CANMB46             0x360
#define RINGO_CANMB47             0x370
#define RINGO_CANMB48             0x380
#define RINGO_CANMB49             0x390
#define RINGO_CANMB50             0x3A0
#define RINGO_CANMB51             0x3B0
#define RINGO_CANMB52             0x3C0
#define RINGO_CANMB53             0x3D0
#define RINGO_CANMB54             0x3E0
#define RINGO_CANMB55             0x3F0
#define RINGO_CANMB56             0x400
#define RINGO_CANMB57             0x410
#define RINGO_CANMB58             0x420
#define RINGO_CANMB59             0x430
#define RINGO_CANMB60             0x440
#define RINGO_CANMB61             0x450
#define RINGO_CANMB62             0x460
#define RINGO_CANMB63             0x470

/* Rx Individual Mask Registers RXIMR0-RXIMR15 */
#define RINGO_CANRXIMR0           0x880
#define RINGO_CANRXIMR1           0x884
#define RINGO_CANRXIMR2           0x888
#define RINGO_CANRXIMR3           0x88C
#define RINGO_CANRXIMR4           0x890
#define RINGO_CANRXIMR5           0x894
#define RINGO_CANRXIMR6           0x898
#define RINGO_CANRXIMR7           0x89C
#define RINGO_CANRXIMR8           0x8A0
#define RINGO_CANRXIMR9           0x8A4
#define RINGO_CANRXIMR10          0x8A8
#define RINGO_CANRXIMR11          0x8AC
#define RINGO_CANRXIMR12          0x8B0
#define RINGO_CANRXIMR13          0x8B4
#define RINGO_CANRXIMR14          0x8B8
#define RINGO_CANRXIMR15          0x8BC

/* Rx Individual Mask Registers RXIMR16-RXIMR31 */
#define RINGO_CANRXIMR16          0x8C0
#define RINGO_CANRXIMR17          0x8C4
#define RINGO_CANRXIMR18          0x8C8
#define RINGO_CANRXIMR19          0x8CC
#define RINGO_CANRXIMR20          0x8D0
#define RINGO_CANRXIMR21          0x8D4
#define RINGO_CANRXIMR22          0x8D8
#define RINGO_CANRXIMR23          0x8DC
#define RINGO_CANRXIMR24          0x8E0
#define RINGO_CANRXIMR25          0x8E4
#define RINGO_CANRXIMR26          0x8E8
#define RINGO_CANRXIMR27          0x8EC
#define RINGO_CANRXIMR28          0x8F0
#define RINGO_CANRXIMR29          0x8F4
#define RINGO_CANRXIMR30          0x8F8
#define RINGO_CANRXIMR31          0x8FC

/* Rx Individual Mask Registers RXIMR32-RXIMR63 */
#define RINGO_CANRXIMR32          0x900
#define RINGO_CANRXIMR33          0x904
#define RINGO_CANRXIMR34          0x908
#define RINGO_CANRXIMR35          0x90C
#define RINGO_CANRXIMR36          0x910
#define RINGO_CANRXIMR37          0x914
#define RINGO_CANRXIMR38          0x918
#define RINGO_CANRXIMR39          0x91C
#define RINGO_CANRXIMR40          0x920
#define RINGO_CANRXIMR41          0x924
#define RINGO_CANRXIMR42          0x928
#define RINGO_CANRXIMR43          0x92C
#define RINGO_CANRXIMR44          0x930
#define RINGO_CANRXIMR45          0x934
#define RINGO_CANRXIMR46          0x938
#define RINGO_CANRXIMR47          0x93C
#define RINGO_CANRXIMR48          0x940
#define RINGO_CANRXIMR49          0x944
#define RINGO_CANRXIMR50          0x948
#define RINGO_CANRXIMR51          0x94C
#define RINGO_CANRXIMR52          0x950
#define RINGO_CANRXIMR53          0x954
#define RINGO_CANRXIMR54          0x958
#define RINGO_CANRXIMR55          0x95C
#define RINGO_CANRXIMR56          0x960
#define RINGO_CANRXIMR57          0x964
#define RINGO_CANRXIMR58          0x968
#define RINGO_CANRXIMR59          0x96C
#define RINGO_CANRXIMR60          0x970
#define RINGO_CANRXIMR61          0x974
#define RINGO_CANRXIMR62          0x978
#define RINGO_CANRXIMR63          0x97C

/* Bit definitions for RINGO CAN Module Configuration (CANMC) Register */
#define RINGO_CANMC_MDIS                    	(0x01 << 31)
#define RINGO_CANMC_FRZ                     	(0x01 << 30)
#define RINGO_CANMC_FEN							(0x01 << 29)
#define RINGO_CANMC_HALT                    	(0x01 << 28)
#define RINGO_CANMC_NOTRDY						(0x01 << 27)
#define RINGO_CANMC_WAKEMSK                 	(0x01 << 26)
#define RINGO_CANMC_SOFTRST                 	(0x01 << 25)
#define RINGO_CANMC_FRZACK                  	(0x01 << 24)
#define RINGO_CANMC_SUPV                    	(0x01 << 23)
#define RINGO_CANMC_SELFWAKE                	(0x01 << 22)
#define RINGO_CANMC_WRN_EN	                 	(0x01 << 21)
#define RINGO_CANMC_LPM_ACK                 	(0x01 << 20)
#define RINGO_CANMC_WAK_SRC                 	(0x01 << 19)
#define RINGO_CANMC_DOZE                    	(0x01 << 18)
#define RINGO_CANMC_SRX_DIS                 	(0x01 << 17)
#define RINGO_CANMC_BCC                     	(0x01 << 16)
#define RINGO_CANMC_LPRIO_EN                	(0x01 << 13)
#define RINGO_CANMC_AEN	                 		(0x01 << 12)
#define RINGO_CANMC_IDAM_FormatA	         	(0x00 << 8)
#define RINGO_CANMC_IDAM_FormatB	         	(0x01 << 8)
#define RINGO_CANMC_IDAM_FormatC	         	(0x10 << 8)
#define RINGO_CANMC_IDAM_FormatD	         	(0x11 << 8)
#define RINGO_CANMC_MAXMB_MASK    	  		  	0x0000003F
#define RINGO_CANMC_MAXMB_MAXVAL    	  		0x0000003F	/* 64 MBs */

/* Bit definitions for RINGO CAN Control (CTRL) Register */
#define RINGO_CANCTRL_PRESDIV_MASK        	  	0xFF000000
#define RINGO_CANCTRL_PRESDIV_MAXVAL      	  	0xFF
#define RINGO_CANCTRL_PRESDIV_SHIFT       	  	24
#define RINGO_CANCTRL_PRESDIV_SAM_MIN     		5
#define RINGO_CANCTRL_RJW_MASK               	(0x03 << 22)
#define RINGO_CANCTRL_RJW_MAXVAL      			0x3
#define RINGO_CANCTRL_RJW_SHIFT               	22
#define RINGO_CANCTRL_PSEG1_MASK             	(0x07 << 19)
#define RINGO_CANCTRL_PSEG1_MAXVAL    			0x7
#define RINGO_CANCTRL_PSEG1_SHIFT             	19
#define RINGO_CANCTRL_PSEG2_MASK             	(0x07 << 16)
#define RINGO_CANCTRL_PSEG2_MAXVAL    			0x7
#define RINGO_CANCTRL_PSEG2_SHIFT             	16
#define RINGO_CANCTRL_BOFFMSK                	(0x01 << 15)
#define RINGO_CANCTRL_ERRMASK                	(0x01 << 14)
#define RINGO_CANCTRL_CLKSRC                 	(0x01 << 13)
#define RINGO_CANCTRL_LPB	                 	(0x01 << 12)
#define RINGO_CANCTRL_TWRNMSK                	(0x01 << 11)
#define RINGO_CANCTRL_RWRNMSK                	(0x01 << 10)
#define RINGO_CANCTRL_SAMP                   	(0x01 << 7)
#define RINGO_CANCTRL_BOFFREC                	(0x01 << 6)
#define RINGO_CANCTRL_TSYNC                  	(0x01 << 5)
#define RINGO_CANCTRL_LBUF                   	(0x01 << 4)
#define RINGO_CANCTRL_LOM                    	(0x01 << 3)
#define RINGO_CANCTRL_PROPSEG_MASK           	(0x07 << 0)
#define RINGO_CANCTRL_PROPSEG_SHIFT           	0

/* FlexCAN Error Counter Register */
#define RINGO_CANECR_RXECTR	       				0xFF00
#define RINGO_CANECR_TXECTR	       				0x00FF

/* Bit definitions for RINGO CAN Error and Status (CANES) Register */
#define RINGO_CANES_TWRNINT            			(0x01 << 17)
#define RINGO_CANES_RWRNINT 			  		(0x01 << 16)
#define RINGO_CANES_BITERR_RECESSIVE_DOMINANT 	(0x01 << 15)
#define RINGO_CANES_BITERR_DOMINANT_RECESSIVE 	(0x01 << 14)
#define RINGO_CANES_ACKERR                    	(0x01 << 13)
#define RINGO_CANES_CRCERR                    	(0x01 << 12)
#define RINGO_CANES_FORMERR                   	(0x01 << 11)
#define RINGO_CANES_STUFFERR                  	(0x01 << 10)
#define RINGO_CANES_TXWARN                    	(0x01 << 9)
#define RINGO_CANES_RXWARN                    	(0x01 << 8)
#define RINGO_CANES_IDLE                      	(0x01 << 7)
#define RINGO_CANES_TX_RX                     	(0x01 << 6)
#define RINGO_CANES_FCS_MASK                  	(0x03 << 4)
#define RINGO_CANES_FCS_SHIFT                 	4
#define RINGO_CANES_FCS_ERROR_ACTIVE          	(0x00 << 4)
#define RINGO_CANES_FCS_ERROR_PASSIVE         	(0x01 << 4)
#define RINGO_CANES_BOFFINT                   	(0x01 << 2)
#define RINGO_CANES_ERRINT                    	(0x01 << 1)
#define RINGO_CANES_WAKEINT                   	(0x01 << 0)

/* Interrupt Mask Register */
#define IMASK_BUFnM(x)							(0x1<<(x))
#define IMASK_BUFF_ENABLE_ALL					(0xFFFFFFFF)
#define IMASK_BUFF_DISABLE_ALL					(0x00000000)

/* Interrupt Flag Register */
#define IFLAG_BUFF_SET_ALL						(0xFFFFFFFF)
#define IFLAG_BUFF_CLEAR_ALL					(0x00000000)
#define IFLAG_BUFnM(x)							(0x1<<(x))

/* Message Buffers */
#define MB_CNT_CODE(x)							(((x)&0x0F)<<24)
#define MB_CNT_SRR								(0x00400000)
#define MB_CNT_IDE								(0x00200000)
#define MB_CNT_RTR								(0x00100000)
#define MB_CNT_LENGTH(x)						(((x)&0x0F)<<16)
#define MB_CNT_TIMESTAMP(x)						((x)&0xFFFF)

/* Bit definitions for RINGO CAN Rx Global Mask (CANRXGMASK) Register */
#define RINGO_CANRXGMASK_MASK        			0xFFFFFFFF

#define RINGO_CANLAM_MASK        				0xFFFFFFFF

/*** Receive message object codes ***/
/* Message buffer is not active */
#define REC_CODE_NOT_ACTIVE						0x0
/* Message buffer is active and empty */
#define REC_CODE_EMPTY							0x4
/* Message buffer is full */
#define REC_CODE_FULL							0x2
/* Second frame was received into a full buffer before CPU read the first */
#define REC_CODE_OVERRUN						0x6
/* Message buffer is now being filled with a new receive frame.
 * This condition will be cleared within 20 cycles.
 */
#define REC_CODE_BUSY							0x1

/*** Transmit message object codes ***/
/* Message buffer not ready for transmit */
#define TRANS_CODE_NOT_READY					0x8
/* Message buffer was configured as Tx and ARM aborted the transmission */
#define TRANS_CODE_ABORT						0x9
/* Data frame to be transmitted once, unconditionally */
#define TRANS_CODE_TRANSMIT_ONCE				0xC
/* Remote frame to be transmitted once, and message buffer becomes
 * an RX message buffer for dadat frames;
 * RTR bit must be set
 */
#define TRANS_CODE_TRANSMIT_RTR_ONCE			0xC
/* Data frame to be transmitted only as a response to a remote frame, always */
#define TRANS_CODE_TRANSMIT_ONLY_RTR			0xA
/* Data frame to be transmitted once, unconditionally
 * and then only as a response to remote frame always
 */
#define TRANS_CODE_TRANSMIT_ONCE_RTR_ALWAYS		0xE

/* Definitions for RINGO CAN Message Control Field */
#define RINGO_CANMCF_DLC_BYTE0   	0x0
#define RINGO_CANMCF_DLC_BYTE1   	0x1
#define RINGO_CANMCF_DLC_BYTE2   	0x2
#define RINGO_CANMCF_DLC_BYTE3   	0x3
#define RINGO_CANMCF_DLC_BYTE4   	0x4
#define RINGO_CANMCF_DLC_BYTE5   	0x5
#define RINGO_CANMCF_DLC_BYTE6   	0x6
#define RINGO_CANMCF_DLC_BYTE7   	0x7
#define RINGO_CANMCF_DLC_BYTE8   	0x8
#define RINGO_CANMCF_RTR         	0x00100000
#define RINGO_CANMCF_TPL_SHIFT   	29
#define RINGO_CANMCF_TPL_MAXVAL  	0x7
#define RINGO_CANMCF_TPL_MASK    	0xE0000000
#define RINGO_CANMCF_IDE         	0x200000

/* Definitions for RINGO CAN Message Identifier */
#define RINGO_CANMID_MASK_STD    	0x1FFC0000
#define RINGO_CANMID_MASK_EXT    	0x1FFFFFFF

#define CANDEV_RINGO_TX_ENABLED 		0x00000001

#define INIT_FLAGS_LOOPBACK 			0x00000001	/* Enable self-test/loopback mode */
#define INIT_FLAGS_EXTENDED_MID			0x00000002	/* Enable 29 bit extended message ID */
#define INIT_FLAGS_AUTOBUS				0x00000004	/* Disable auto bus on */
#define INIT_FLAGS_TIMESTAMP			0x00000008	/* Set initial local network time */
#define INIT_FLAGS_RX_FULL_MSG			0x00000010	/* Receiver should store message ID, timestamp, etc. */
#define INIT_FLAGS_BITRATE_SAM			0x00000020	/* Enable Bitrate Triple Sample Mode */
#define INIT_FLAGS_MDRIVER_INIT			0x00000040	/* Initialize from mini-driver (if it exists and is running) */
#define INIT_FLAGS_MDRIVER_SORT			0x00000080	/* Sort buffered mdriver message based on MID */
#define INIT_FLAGS_CLKSRC				0x00000100	/* External reference clock source */
#define INIT_FLAGS_TSYN					0x00000200	/* Enable Timer Sync feature */
#define INIT_FLAGS_LOM					0x00000400	/* Listen Only Mode */
#define INIT_FLAGS_LBUF					0x00000800	/* Lowest number buffer is transmitted first */

#define INFO_FLAGS_RX_FULL_MSG 			0x00000001	/* Receiver should store message ID, timestamp, etc. */
#define INFO_FLAGS_ENDIAN_SWAP 			0x00000002	/* Data is TX/RX'd MSB, need to perform ENDIAN conversions */


struct candev_ringo_entry;

/* CAN Message Object Data Structure */
typedef struct can_msg_obj
{
	uint32_t 		canmcf; 				/* Message Control Field */
	uint32_t		canmid; 				/* Message Identifier */
	uint32_t 		canmdl; 				/* Message Data Low Word */
	uint32_t 		canmdh; 				/* Message Data High Word */
} CAN_MSG_OBJ;

/* Mini-driver Data in memory - this must exactly match the definition used by the mdriver in Startup */
typedef struct minican_data
{
	uintptr_t 		canport;        /* CAN base register mapping */
	uintptr_t 		canport_k;      /* CAN base register mapping */
	CAN_MSG_OBJ 	*canmem;        /* CAN base memory mapping */
	CAN_MSG_OBJ 	*canmem_k;      /* CAN base memory mapping */
	uint16_t 		nstartup;       /* Stats for number of calls to MDRIVER_STARTUP */
	uint16_t 		nstartupp;      /* Stats for number of calls to MDRIVER_STARTUP_PREPARE */
	uint16_t 		nstartupf;      /* Stats for number of calls to MDRIVER_STARTUP_FINI */
	uint16_t 		nkernel;        /* Stats for number of calls to MDRIVER_KERNEL */
	uint16_t 		nprocess;       /* Stats for number of calls to MDRIVER_PROCESS */
	uint16_t 		nrx;            /* Number of received messages */
	uint32_t 		tx_enabled;     /* Flags to keep track of which mailboxes have a tx in progress */
	/* Buffered messages of type CAN_MSG_OBJ follow this data structure in memory */
} MINICAN_DATA;

/* Initialization and Options Structure */
typedef struct candev_ringo_init_entry
{
	CANDEV_INIT					cinit;		/* Generic CAN Device Init Params */
	_Paddr32t 					port;		/* Device Physical Register Address */
	_Paddr32t 					mem;		/* CAN Message Object Physical Memory Address */
	uint32_t					clk;		/* CAN Clock */
	/* Bitrate related parameters */
	uint32_t					bitrate;	/* Bitrate */
	uint8_t						br_presdiv;	/* Bitrate Prescaler */
	uint8_t						br_propseg;	/* Propagation Segment Time */
	uint8_t						br_rjw;		/* Bitrate Resync Jump Width */
	uint8_t						br_pseg1;	/* Bitrate Phase Buffer Segment 1 */
	uint8_t						br_pseg2;	/* Bitrate Phase Buffer Segment 2 */
	int							irqsys;		/* Device Message System Vector */
	uint32_t 					flags;		/* Initialization flags */
	uint32_t 					numrx;		/* Number of RX Mailboxes (TX = Total Mailboxes - RX) */
	uint32_t 					midrx;		/* RX Message ID */
	uint32_t 					midtx;		/* TX Message ID */
	uint32_t 					timestamp;	/* Initial value for local network time */
} CANDEV_RINGO_INIT;

/* General Device Information Structure */
typedef struct candev_ringo_info_entry
{
	uintptr_t 					base;		/* Device Virtual Register Mapping */
	CAN_MSG_OBJ 				*canmsg;	/* Array of CAN message objects */ 
	uintptr_t 					canlam;		/* Array of CAN message object local area masks */
	uint32_t 					timer;		/* CAN message time stamps */
	volatile uint32_t			canestat;	/* Keep track of CANESR register status for devctl */
	struct candev_ringo_entry	*devlist;	/* Array of all device mailboxes */
	int							iidsys;		/* Return iid from InterruptAttach */
	uint32_t 					numtx;		/* Number of TX Mailboxes */
	uint32_t 					numrx;		/* Number of RX Mailboxes */
	uint32_t 					iflags;		/* Info flags */
	MINICAN_DATA				*mdata;		/* Mini-driver data */
	CAN_MSG_OBJ 				*mcanmsg;	/* Mini-driver buffered CAN messages */
} CANDEV_RINGO_INFO;

/* Device specific extension of CANDEV struct */
typedef struct candev_ringo_entry
{
	CANDEV						cdev; 		/* CAN Device - MUST be first entry */
	int							mbxid;		/* Index into mailbox memory */
	volatile uint32_t 			dflags;		/* Device specific flags */
	CANDEV_RINGO_INFO			*devinfo;	/* Common device information */
} CANDEV_RINGO;

/* Driver implemented CAN library function prototypes */
void can_drvr_transmit(CANDEV *cdev);
int can_drvr_devctl(CANDEV *cdev, int dcmd, DCMD_DATA *data);

/* Function prototypes */
void can_options(CANDEV_RINGO_INIT *devinit, int argc, char *argv[]);
void can_init_mailbox(CANDEV_RINGO_INFO *devinfo, CANDEV_RINGO_INIT *devinit);
void can_init_hw(CANDEV_RINGO_INFO *devinfo, CANDEV_RINGO_INIT *devinit);
void can_init_intr(CANDEV_RINGO_INFO *devinfo, CANDEV_RINGO_INIT *devinit);
void can_print_reg(CANDEV_RINGO_INFO *devinfo);
void can_print_mailbox(CANDEV_RINGO_INFO *devinfo);
void set_port32(unsigned port, uint32_t mask, uint32_t data);

#endif

__SRCVERSION( "$URL: http://svn/product/branches/6.5.0/trunk/hardware/can/mx6x/canmx6x.h $ $Rev: 219612 $" )
