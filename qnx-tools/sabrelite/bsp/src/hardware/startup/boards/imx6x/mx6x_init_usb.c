/*
 * $QNXLicenseC: 
 * Copyright 2012 QNX Software Systems.  
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

#include "startup.h"

#include <arm/mx6x_iomux.h>
#include <arm/mx6x.h>
#include "mx6x_startup.h"
#include "mx6x_usb_regs.h"

// Analog-Digital Module register offsets, bit settings
#define MX6X_ANATOP_USB1_PLL_480_CTRL_SET		0x14
#define MX6X_ANATOP_USB1_PLL_480_CTRL_EN_USB_CLKS	(1 << 6)
#define MX6X_ANATOP_USB1_PLL_480_CTRL_POWER		(1 << 12)
#define MX6X_ANATOP_USB1_PLL_480_CTRL_ENABLE		(1 << 13)
#define MX6X_ANATOP_USB2_PLL_480_CTRL_BYPASS		(1 << 16)

#define MX6X_ANATOP_USB2_PLL_480_CTRL_SET		0x24
#define MX6X_ANATOP_USB2_PLL_480_CTRL_CLR		0x28
#define MX6X_ANATOP_USB2_PLL_480_CTRL_EN_USB_CLKS	(1 << 6)
#define MX6X_ANATOP_USB2_PLL_480_CTRL_POWER		(1 << 12)
#define MX6X_ANATOP_USB2_PLL_480_CTRL_ENABLE		(1 << 13)

// For USB OTG module
#define MX6X_ANATOP_USB1_CHRG_DETECT			0x1b0
#define MX6X_ANATOP_USB1_CHRG_DETECT_EN_B		(1 << 20)
#define MX6X_ANATOP_USB1_CHRG_DETECT_CHK_CHRG_B		(1 << 19)

// For USB Host Controller 1
#define MX6X_ANATOP_USB2_CHRG_DETECT			0x210
#define MX6X_ANATOP_USB2_CHRG_DETECT_EN_B		(1 << 20)
#define MX6X_ANATOP_USB2_CHRG_DETECT_CHK_CHRG_B		(1 << 19)

void mx6x_init_usb_otg()
{
	/* disable external charge detect to improve signal quality */
	out32(MX6X_ANATOP_BASE + MX6X_ANATOP_USB1_CHRG_DETECT, MX6X_ANATOP_USB1_CHRG_DETECT_EN_B |\
		MX6X_ANATOP_USB1_CHRG_DETECT_CHK_CHRG_B);

	/* enable DLL clock */
	out32(MX6X_ANATOP_BASE + MX6X_ANATOP_USB1_PLL_480_CTRL_SET, MX6X_ANATOP_USB1_PLL_480_CTRL_ENABLE |\
		MX6X_ANATOP_USB1_PLL_480_CTRL_POWER | MX6X_ANATOP_USB1_PLL_480_CTRL_EN_USB_CLKS);
	
	/* Stop OTG controller core */
	out32(MX6X_USBOTG_BASE + MX6X_USB_CMD, in32(MX6X_USBOTG_BASE + MX6X_USB_CMD) & ~MX6X_USB_CMD_RUN_STOP);
	while (in32(MX6X_USBOTG_BASE + MX6X_USB_CMD) & MX6X_USB_CMD_RUN_STOP)
		;

	/* Reset OTG controller core */
	out32(MX6X_USBOTG_BASE + MX6X_USB_CMD, in32(MX6X_USBOTG_BASE) | MX6X_USB_CMD_RESET);
	while (in32(MX6X_USBOTG_BASE + MX6X_USB_CMD) & MX6X_USB_CMD_RESET)
		;
}

void mx6x_init_usb_host1()
{
	/* disable external charge detect to improve signal quality */
	out32(MX6X_ANATOP_BASE + MX6X_ANATOP_USB2_CHRG_DETECT, MX6X_ANATOP_USB2_CHRG_DETECT_EN_B |\
		MX6X_ANATOP_USB2_CHRG_DETECT_CHK_CHRG_B);

	out32(MX6X_ANATOP_BASE + MX6X_ANATOP_USB2_PLL_480_CTRL_CLR, MX6X_ANATOP_USB2_PLL_480_CTRL_BYPASS);

	/* set EN_USB_CLKS bit */
	out32(MX6X_ANATOP_BASE + MX6X_ANATOP_USB2_PLL_480_CTRL_SET, MX6X_ANATOP_USB2_PLL_480_CTRL_ENABLE |\
		MX6X_ANATOP_USB2_PLL_480_CTRL_POWER | MX6X_ANATOP_USB2_PLL_480_CTRL_EN_USB_CLKS);
	
	/* Stop Host1 controller */
	out32(MX6X_USBH1_BASE + MX6X_USB_CMD, in32(MX6X_USBH1_BASE + MX6X_USB_CMD) & ~MX6X_USB_CMD_RUN_STOP);
	while (in32(MX6X_USBH1_BASE + MX6X_USB_CMD) & MX6X_USB_CMD_RUN_STOP)
		;

	/* Reset Host1 controller */
	out32(MX6X_USBH1_BASE + MX6X_USB_CMD, in32(MX6X_USBH1_BASE) | MX6X_USB_CMD_RESET);
	while (in32(MX6X_USBH1_BASE + MX6X_USB_CMD) & MX6X_USB_CMD_RESET)
		;
}

void mx6x_init_usb_phy(uint32_t phy_addr)
{
	/* Reset USB PHY 0 */
	out32(phy_addr + MX6X_USBPHY_CTRL, in32(phy_addr + MX6X_USBPHY_CTRL) | MX6X_USBPHY_CTRL_SFTRST);
	mx6x_usleep(10);

	/* Remove clock gate and soft reset */
	out32(phy_addr + MX6X_USBPHY_CTRL, in32(phy_addr + MX6X_USBPHY_CTRL) & ~(MX6X_USBPHY_CTRL_SFTRST \
			| MX6X_USBPHY_CTRL_CLKGATE));
	mx6x_usleep(10);
	
	/* Power up PHY */
	out32(phy_addr + MX6X_USBPHY_PWD, 0);

	/* enable FS/LS device */
	out32(phy_addr + MX6X_USBPHY_CTRL, in32(phy_addr + MX6X_USBPHY_CTRL) \
		| MX6X_ENUTMILEVEL3 | MX6X_ENUTMILEVEL2);
}

void mx6x_init_usb_host2(void)
{
	unsigned chip_type = get_mx6_chip_type();
	unsigned tmp_reg_val;

	/* Initialize H2 DATA, STROBE signals */
	if ( chip_type == MX6_CHIP_TYPE_QUAD_OR_DUAL)
	{
		// USB H2_DATA
		pinmux_set_swmux(SWMUX_RGMII_TXC, MUX_CTL_MUX_MODE_ALT0 | MUX_CTL_SION);
		pinmux_set_padcfg(SWPAD_RGMII_TXC, MX6X_PAD_SETTINGS_USB_HSIC_RESET);

		// USB H2_STROBE
		pinmux_set_swmux(SWMUX_RGMII_TX_CTL, MUX_CTL_MUX_MODE_ALT0 | MUX_CTL_SION);
		pinmux_set_padcfg(SWPAD_RGMII_TX_CTL, MX6X_PAD_SETTINGS_USB_HSIC_RESET);
	}
	else if (get_mx6_chip_type() == MX6_CHIP_TYPE_DUAL_LITE_OR_SOLO)
	{
		// USB H2_DATA
		pinmux_set_swmux(SWMUX_SDL_RGMII_TXC, MUX_CTL_MUX_MODE_ALT0 | MUX_CTL_SION);
		pinmux_set_padcfg(SWPAD_SDL_RGMII_TXC, MX6X_PAD_SETTINGS_USB_HSIC_RESET);

		// USB H2_STROBE
		pinmux_set_swmux(SWMUX_SDL_RGMII_TX_CTL, MUX_CTL_MUX_MODE_ALT0 | MUX_CTL_SION);
		pinmux_set_padcfg(SWPAD_SDL_RGMII_TX_CTL, MX6X_PAD_SETTINGS_USB_HSIC_RESET);
	}
	
	/* Force UTMI Clock ouput even in suspend mode */
	out32(MX6X_USB_OTHER_BASE + MX6X_USB_UH2_CTRL, in32(MX6X_USB_OTHER_BASE + MX6X_USB_UH2_CTRL) | MX6X_USB_HOST_CTRL_UTMI_ON);

	/* Force HSIC Clock on even in suspend mode */
	out32(MX6X_USB_OTHER_BASE + MX6X_USB_UH2_HSIC_CTRL, in32(MX6X_USB_OTHER_BASE + MX6X_USB_UH2_HSIC_CTRL) | MX6X_USB_HSIC_CLK_ON);
	
	/* Enable HSIC */
	out32(MX6X_USB_OTHER_BASE + MX6X_USB_UH2_HSIC_CTRL, in32(MX6X_USB_OTHER_BASE + MX6X_USB_UH2_HSIC_CTRL) | MX6X_USB_HSIC_ENABLE);

	/* Configure USB H2 to use HSIC interface */
	tmp_reg_val = in32(MX6X_USBH2_BASE + MX6X_USB_PORTSC1);
	tmp_reg_val &= ~MX6X_PTS_MASK;
	tmp_reg_val |= MX6X_PTS_HSIC;  
	out32(MX6X_USBH2_BASE + MX6X_USB_PORTSC1, tmp_reg_val); 	

	/* 
	 * Note that it is the USB driver's responsibility to bring the HSIC device out of reset,
	 * and to pull the HSIC STROBE signal high (i.e. set HSIC interface state to IDLE).
	 */
}

void mx6x_init_usb_host3(void)
{
	unsigned chip_type = get_mx6_chip_type();
	unsigned tmp_reg_val;

	/* Initialize H3 DATA, STROBE signals */
	if ( chip_type == MX6_CHIP_TYPE_QUAD_OR_DUAL)
	{
		// USB H3_DATA
		pinmux_set_swmux(SWMUX_RGMII_RX_CTL, MUX_CTL_MUX_MODE_ALT0 | MUX_CTL_SION);
		pinmux_set_padcfg(SWPAD_RGMII_RX_CTL, MX6X_PAD_SETTINGS_USB_HSIC_RESET);

		// USB H3_STROBE
		pinmux_set_swmux(SWMUX_RGMII_RXC, MUX_CTL_MUX_MODE_ALT0 | MUX_CTL_SION);
		pinmux_set_padcfg(SWPAD_RGMII_RXC, MX6X_PAD_SETTINGS_USB_HSIC_RESET);
	}
	else if (get_mx6_chip_type() == MX6_CHIP_TYPE_DUAL_LITE_OR_SOLO)
	{
		// USB H3_DATA
		pinmux_set_swmux(SWMUX_SDL_RGMII_RX_CTL, MUX_CTL_MUX_MODE_ALT0 | MUX_CTL_SION);
		pinmux_set_padcfg(SWPAD_SDL_RGMII_RX_CTL, MX6X_PAD_SETTINGS_USB_HSIC_RESET);

		// USB H3_STROBE
		pinmux_set_swmux(SWMUX_SDL_RGMII_RXC, MUX_CTL_MUX_MODE_ALT0 | MUX_CTL_SION);
		pinmux_set_padcfg(SWPAD_SDL_RGMII_RXC, MX6X_PAD_SETTINGS_USB_HSIC_RESET);
	}

	/* Force UTMI Clock ouput even in suspend mode */
	out32(MX6X_USB_OTHER_BASE + MX6X_USB_UH3_CTRL, in32(MX6X_USB_OTHER_BASE + MX6X_USB_UH3_CTRL) | MX6X_USB_HOST_CTRL_UTMI_ON);

	/* Force HSIC Clock on even in suspend mode */
	out32(MX6X_USB_OTHER_BASE + MX6X_USB_UH3_HSIC_CTRL, in32(MX6X_USB_OTHER_BASE + MX6X_USB_UH3_HSIC_CTRL) | MX6X_USB_HSIC_CLK_ON);
	
	/* Enable HSIC */
	out32(MX6X_USB_OTHER_BASE + MX6X_USB_UH3_HSIC_CTRL, in32(MX6X_USB_OTHER_BASE + MX6X_USB_UH3_HSIC_CTRL) | MX6X_USB_HSIC_ENABLE);
	
	/* Configure USB H3 to use HSIC interface */
	tmp_reg_val = in32(MX6X_USBH3_BASE + MX6X_USB_PORTSC1);
	tmp_reg_val &= ~MX6X_PTS_MASK;
	tmp_reg_val |= MX6X_PTS_HSIC;
	out32(MX6X_USBH3_BASE + MX6X_USB_PORTSC1, tmp_reg_val);
	
	/* 
	 * Note that it the USB driver's responsibility to bring the HSIC device out of reset,
	 * and to pull the HSIC STROBE signal high (i.e. set HSIC interface state to IDLE).
	 */
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/mx6x_init_usb.c $ $Rev: 729057 $")
#endif
