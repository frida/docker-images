/*
 * $QNXLicenseC: 
 * Copyright 2011, 2012 QNX Software Systems.  
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

/*
 * The i.MX6 series SOCs contain 4 USB controllers:
 * One OTG controller which can function in Host or Device mode.  The OTG module uses an on chip UTMI PHY
 * One Host controller which uses an on chip UTMI phy
 * Two Host controllers which use on chip HS-IC PHYs
 */

/*
 * The i.MX6 Q Sabre-Lite BSP currently supports the OTG and Host1 controller.  Note that the Host1
 * controller is connected to USB hub which provides two USB Standard Type-A reeptacles
 */

#include "startup.h"
#include "board.h"
							
void mx6q_usb_otg_host_init(void)
{
	/* ID pin muxing */
	pinmux_set_swmux(SWMUX_GPIO_1, MUX_CTL_MUX_MODE_ALT3);
	pinmux_set_padcfg(SWPAD_GPIO_1, MX6X_PAD_SETTINGS_USB);

	/* setup USB_CLK_EN_B line */
	pinmux_set_swmux(SWMUX_EIM_D22, MUX_CTL_MUX_MODE_ALT5);

	/* enable USB OTG Power */
	out32(MX6X_GPIO3_BASE + MX6X_GPIO_GDIR, in32(MX6X_GPIO3_BASE + MX6X_GPIO_GDIR) | (0x1 << 22));
	out32(MX6X_GPIO3_BASE + MX6X_GPIO_DR, in32(MX6X_GPIO3_BASE + MX6X_GPIO_DR) | (0x1 << 22));    

	/* USB OTG select GPIO_1 */
	out32(MX6X_IOMUXC_BASE + MX6X_IOMUX_GPR1, in32(MX6X_IOMUXC_BASE + MX6X_IOMUX_GPR1) | (1 << 13));

	/* Initialize OTG core */
	mx6x_init_usb_otg();

	/* OTG Host connects to PHY0  */
	mx6x_init_usb_phy(MX6X_USBPHY0_BASE);
}

void mx6q_usb_host1_init(void)
{
	/* Initialize Host1 */
	mx6x_init_usb_host1();

	/* USB Host1 connects to PHY1  */
	mx6x_init_usb_phy(MX6X_USBPHY1_BASE);

	/* Bring USB hub out of reset by bringing GPIO7[12] high */
	pinmux_set_swmux(SWMUX_GPIO_17, MUX_CTL_MUX_MODE_ALT5);
	out32(MX6X_GPIO7_BASE + MX6X_GPIO_GDIR, in32(MX6X_GPIO7_BASE + MX6X_GPIO_GDIR) | (0x1 << 12));
	out32(MX6X_GPIO7_BASE + MX6X_GPIO_DR, in32(MX6X_GPIO7_BASE + MX6X_GPIO_DR) | (0x1 << 12));
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/sabrelite/init_usb.c $ $Rev: 729057 $")
#endif
