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


#ifndef __MX6X_USBREGS_H
#define __MX6X_USBREGS_H


/* USB non-core registers */
#define MX6X_USB_OTG_CTRL			0x00
#define MX6X_USB_UH1_CTRL			0x04
#define MX6X_USB_UH2_CTRL			0x08
#define MX6X_USB_UH3_CTRL			0x0c
#define MX6X_USB_UH2_HSIC_CTRL			0x10
#define MX6X_USB_UH3_HSIC_CTRL			0x14
	#define MX6X_USB_HOST_CTRL_UTMI_ON		(1 << 13)
	#define MX6X_USB_HSIC_ENABLE			(1 << 12)
	#define MX6X_USB_HSIC_CLK_ON			(1 << 11)
#define MX6X_USB_OTG_PHY_CTRL_0			0x18
#define MX6X_USB_UH1_PHY_CTRL_0			0x1c
#define MX6X_USB_UH2_HSIC_DLL_CFG1		0x20
#define MX6X_USB_UH2_HSIC_DLL_CFG2		0x24
#define MX6X_USB_UH2_HSIC_DLL_CFG3		0x28
#define MX6X_USB_UH3_HSIC_DLL_CFG1		0x30
#define MX6X_USB_UH3_HSIC_DLL_CFG2		0x34
#define MX6X_USB_UH3_HSIC_DLL_CFG3		0x38

/* 
 * USB core registers
 * Macros containing "UOG" are only available in the OTG core 
 * Macros containing "HC" are only available in the Host Controller cores (Host 1,2,3)
 */
#define MX6X_USB_ID				0x000
#define MX6X_USB_HWGENERAL			0x004
#define MX6X_USB_HWHOST				0x008
#define MX6X_USB_UOG_HWDEVICE			0x00C
#define MX6X_USB_HWTXBUF			0x010
#define MX6X_USB_HWRXBUF			0x014
#define MX6X_USB_GPTIMER0LD			0x080
#define MX6X_USB_GPTIMER0CTRL			0x084
#define MX6X_USB_GPTIMER1LD			0x088
#define MX6X_USB_GPTIMER1CTRL			0x08C
#define MX6X_USB_SBUSCFG			0x090
#define MX6X_USB_CAPLENGTH			0x100
#define MX6X_USB_HCIVERSION			0x102 
#define MX6X_USB_HCSPARAMS			0x104
#define MX6X_USB_HCCPARAMS			0x108
#define MX6X_USB_UOG_DCIVERSION			0x120
#define MX6X_USB_UOG_DCCPARAMS			0x124
#define MX6X_USB_CMD				0x140
	#define MX6X_USB_CMD_RUN_STOP			(1)
	#define MX6X_USB_CMD_RESET			(1 << 1)
#define MX6X_USB_STS				0x144
#define MX6X_USB_INTR				0x148
#define MX6X_USB_FRINDEX			0x14C
#define MX6X_USB_HC_PERIODICLISTBASE		0x154
#define MX6X_USB_UOG_DEVICEADDR			0x154
#define MX6X_USB_ASYNCLISTADDR			0x158
#define MX6X_USB_UOG_ENDPTLISTADDR		0x158
#define MX6X_USB_BURSTSIZE			0x160
#define MX6X_USB_TXFILLTUNING			0x164
#define MX6X_USB_IC_USB				0x16C
#define MX6X_USB_UOG_ENDPTNAK			0x178
#define MX6X_USB_UOG_ENDPTNAKEN			0x17C
#define MX6X_USB_PORTSC1			0x184
	#define MX6X_PTS_MASK				((0x3<<30 | 0x1<<25))
	#define MX6X_PTS_HSIC				(0x1<<25)
#define MX6X_USB_UOG_OTGSC			0x1A4
#define MX6X_USB_USBMODE			0x1A8
#define MX6X_USB_ENDPTSETUPSTAT			0x1AC
#define MX6X_USB_ENDPTPRIME			0x1B0
#define MX6X_USB_ENDPTFLUSH			0x1B4
#define MX6X_USB_ENDPTSTAT			0x1B8
#define MX6X_USB_ENDPTCOMPLETE			0x1BC
#define MX6X_USB_UOG_ENDPTCTRL0			0x1C0
#define MX6X_USB_UOG_ENDPTCTRL1			0x1C4
#define MX6X_USB_UOG_ENDPTCTRL2			0x1C8
#define MX6X_USB_UOG_ENDPTCTRL3			0x1CC
#define MX6X_USB_UOG_ENDPTCTRL4			0x1D0
#define MX6X_USB_UOG_ENDPTCTRL5			0x1D4
#define MX6X_USB_UOG_ENDPTCTRL6			0x1D8
#define MX6X_USB_UOG_ENDPTCTRL7			0x1DC
#define MX6X_USB_ULPIVIEW			0x770

/*
 * USB PHY registers
 * See macros MX6X_USBPHY0_BASE, MX6X_USBPHY1_BASE for base addresses
 */  
#define MX6X_USBPHY_PWD				0x00
#define MX6X_USBPHY_TX				0x10
#define MX6X_USBPHY_RX				0x20
#define MX6X_USBPHY_CTRL			0x30
	#define MX6X_USBPHY_CTRL_SFTRST			(1 << 31)
	#define MX6X_USBPHY_CTRL_CLKGATE		(1 << 30)
	#define MX6X_ENUTMILEVEL3			(1 << 15)
	#define MX6X_ENUTMILEVEL2			(1 << 14)
#define MX6X_USBPHY_STATUS			0x40
#define MX6X_USBPHY_DEBUG			0x50
#define MX6X_USBPHY_DEBUG0_STATUS		0x60
#define MX6X_USBPHY_DEBUG1_STATUS		0x70
#define MX6X_USBPHY_VERSION			0x80
#define MX6X_USBPHY_IP				0x90

#endif



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/mx6x_usb_regs.h $ $Rev: 729057 $")
#endif
