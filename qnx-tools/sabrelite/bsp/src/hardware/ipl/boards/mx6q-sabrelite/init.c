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

#include "ipl_mx6x.h"
#include <sys/srcversion.h>
#include <arm/mx6x_iomux.h>
#include <hw/inout.h>


#define MX6Q_PAD_SETTINGS_UART (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | \
                				PAD_CTL_PUS_100K_PU | PAD_CTL_SPEED_MEDIUM | \
                                PAD_CTL_DSE_40_OHM | PAD_CTL_SRE_FAST | PAD_CTL_PUE_PULL)

#define MX6Q_PAD_SETTINGS_USDHC (PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL | PAD_CTL_PUS_47K_PU | \
                                PAD_CTL_SPEED_LOW | PAD_CTL_DSE_80_OHM | PAD_CTL_SRE_FAST | \
                                PAD_CTL_HYS_ENABLE)
/*
 * The Sabre-Lite board has a 50kOhm pull-down resistor connected to SD slot 3's write protect signal (SD3_WP).
 * The pad connected to the SD3_WP signal is called SD3_DAT4, due to the high pull-down resistor we must connect
 * a low internal pull-up resistor to pad SD3_DAT4 to ensure that SD3_WP's voltages are interpreted by the i.MX6Q correctly.
 */
#define MX6Q_PAD_SETTINGS_USDHC3_WP (PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL | PAD_CTL_PUS_22K_PU | PAD_CTL_SPEED_LOW | \
                            PAD_CTL_DSE_80_OHM | PAD_CTL_HYS_ENABLE)


#define MX6Q_PAD_SETTINGS_ECSPI (PAD_CTL_SRE_FAST | PAD_CTL_SPEED_MEDIUM | PAD_CTL_DSE_40_OHM | PAD_CTL_HYS_ENABLE)

static inline void set_mux_cfg(unsigned int pin, unsigned int value)
{
	out32((MX6X_IOMUXC_BASE + MX6X_IOMUX_SWMUX + (pin * 4)), value);
}

static inline void set_pad_cfg(unsigned int pin, unsigned int value)
{
	out32((MX6X_IOMUXC_BASE + MX6X_IOMUX_SWPAD + (pin * 4)), value);
}

static inline void set_pin_as_input(unsigned int pin, unsigned int value)
{
	out32((MX6X_IOMUXC_BASE + MX6X_IOMUX_SWINPUT + (pin * 4)), value);
}

void init_clocks(void)
{
	/* Restore the default values in the Gate registers */
	out32((MX6X_CCM_BASE + MX6X_CCM_CCGR0), MX6X_CCM_CCGR0_RESET);
	out32((MX6X_CCM_BASE + MX6X_CCM_CCGR1), MX6X_CCM_CCGR1_RESET);
#if defined (VARIANT_enableTZASC)
	out32((MX6X_CCM_BASE + MX6X_CCM_CCGR2), 0xFFFFFFFF);	// Enable Trust Zone Address Space Controller clocks
#else
	out32((MX6X_CCM_BASE + MX6X_CCM_CCGR2), MX6X_CCM_CCGR2_RESET);
#endif
	out32((MX6X_CCM_BASE + MX6X_CCM_CCGR3), MX6X_CCM_CCGR3_RESET);
	out32((MX6X_CCM_BASE + MX6X_CCM_CCGR4), MX6X_CCM_CCGR4_RESET);
	out32((MX6X_CCM_BASE + MX6X_CCM_CCGR5), MX6X_CCM_CCGR5_RESET);
	out32((MX6X_CCM_BASE + MX6X_CCM_CCGR6), MX6X_CCM_CCGR6_RESET);
	out32((MX6X_CCM_BASE + MX6X_CCM_CCGR7), MX6X_CCM_CCGR7_RESET);

	/* TODO: No documentation about this? */
	unsigned int val = in32(MX6X_ANATOP_BASE + MX6X_ANADIG_REG_CORE);
	val &= ~ANADIG_REG_CORE_REG2_TRG_MASK;
	val |= ((0x14 << 18) & ANADIG_REG_CORE_REG2_TRG_MASK);
	out32(MX6X_ANATOP_BASE + MX6X_ANADIG_REG_CORE, val);

	/* Gate/ungate PFDs to ensure  proper reset (imX6DQ Erratum ERR006282) */
	out32(MX6X_ANATOP_BASE + MX6X_ANATOP_PFD_480_SET, MX6X_ANATOP_PFD_ALL_CLOCK_GATES);
	out32(MX6X_ANATOP_BASE + MX6X_ANATOP_PFD_528_SET, MX6X_ANATOP_PFD_ALL_CLOCK_GATES);
	out32(MX6X_ANATOP_BASE + MX6X_ANATOP_PFD_480_CLR, MX6X_ANATOP_PFD_ALL_CLOCK_GATES);
	out32(MX6X_ANATOP_BASE + MX6X_ANATOP_PFD_528_CLR, MX6X_ANATOP_PFD_ALL_CLOCK_GATES);
}

void init_pinmux()
{
    /*
     * UART
     */

	/* UART1 RXD */
	set_mux_cfg(SWMUX_SD3_DAT7, MUX_CTL_MUX_MODE_ALT1);
	set_pad_cfg(SWPAD_SD3_DAT7, MX6Q_PAD_SETTINGS_UART);
	
	/* UART1 TXD */
	set_mux_cfg(SWMUX_SD3_DAT6, MUX_CTL_MUX_MODE_ALT1);
	set_pad_cfg(SWPAD_SD3_DAT6, MX6Q_PAD_SETTINGS_UART);
	set_pin_as_input(SWINPUT_UART1_IPP_UART_RXD_MUX, 0x2);

	/* UART2 RXD */
	set_mux_cfg(SWMUX_EIM_D27, MUX_CTL_MUX_MODE_ALT4);
	set_pad_cfg(SWPAD_EIM_D27, MX6Q_PAD_SETTINGS_UART);
	set_pin_as_input(SWINPUT_UART2_IPP_UART_RXD_MUX, 0x1);
	
	/* UART2 TXD */
	set_mux_cfg(SWMUX_EIM_D26, MUX_CTL_MUX_MODE_ALT4);
	set_pad_cfg(SWPAD_EIM_D26, MX6Q_PAD_SETTINGS_UART);

	/*
     * USDHC interfaces 
     */

	/* SD3 CLK */
	set_mux_cfg(SWMUX_SD3_CLK, MUX_CTL_MUX_MODE_ALT0);
	set_pad_cfg(SWPAD_SD3_CLK, MX6Q_PAD_SETTINGS_USDHC);

	/* SD3 CMD */
	set_mux_cfg(SWMUX_SD3_CMD, MUX_CTL_MUX_MODE_ALT0 | MUX_CTL_SION);
	set_pad_cfg(SWPAD_SD3_CMD, MX6Q_PAD_SETTINGS_USDHC);

	/* SD3 DAT0 */
	set_mux_cfg(SWMUX_SD3_DAT0, MUX_CTL_MUX_MODE_ALT0);
	set_pad_cfg(SWPAD_SD3_DAT0, MX6Q_PAD_SETTINGS_USDHC);

	/* SD3 DAT1 */
	set_mux_cfg(SWMUX_SD3_DAT1, MUX_CTL_MUX_MODE_ALT0);
	set_pad_cfg(SWPAD_SD3_DAT1, MX6Q_PAD_SETTINGS_USDHC);

	/* SD3 DAT2 */
	set_mux_cfg(SWMUX_SD3_DAT2, MUX_CTL_MUX_MODE_ALT0);
	set_pad_cfg(SWPAD_SD3_DAT2, MX6Q_PAD_SETTINGS_USDHC);

	/* SD3 DAT3 */
	set_mux_cfg(SWMUX_SD3_DAT3, MUX_CTL_MUX_MODE_ALT0);
	set_pad_cfg(SWPAD_SD3_DAT3, MX6Q_PAD_SETTINGS_USDHC);

	/* SD3 Write Protect - configure GPIO7[1] as an input */
	set_mux_cfg(SWMUX_SD3_DAT4, MUX_CTL_MUX_MODE_ALT5);
	set_pad_cfg(SWPAD_SD3_DAT4, MX6Q_PAD_SETTINGS_USDHC3_WP);
	out32(MX6X_GPIO7_BASE + MX6X_GPIO_GDIR, in32(MX6X_GPIO7_BASE + MX6X_GPIO_GDIR) & ~(1<<1));

	/* SD3 Card Detect - configure GPIO7[0] as an input */
	set_mux_cfg(SWMUX_SD3_DAT5, MUX_CTL_MUX_MODE_ALT5);
	out32(MX6X_GPIO7_BASE + MX6X_GPIO_GDIR, in32(MX6X_GPIO7_BASE + MX6X_GPIO_GDIR) & ~(1<<0));

	/* SD4 CLK */
	set_mux_cfg(SWMUX_SD4_CLK, MUX_CTL_MUX_MODE_ALT0);
	set_pad_cfg(SWPAD_SD4_CLK, MX6Q_PAD_SETTINGS_USDHC);

	/* SD4 CMD */
	set_mux_cfg(SWMUX_SD4_CMD, MUX_CTL_MUX_MODE_ALT0 | MUX_CTL_SION);
	set_pad_cfg(SWPAD_SD4_CMD, MX6Q_PAD_SETTINGS_USDHC);

	/* SD4 DAT0 */
	set_mux_cfg(SWMUX_SD4_DAT0, MUX_CTL_MUX_MODE_ALT1);
	set_pad_cfg(SWPAD_SD4_DAT0, MX6Q_PAD_SETTINGS_USDHC);

	/* SD4 DAT1 */
	set_mux_cfg(SWMUX_SD4_DAT1, MUX_CTL_MUX_MODE_ALT1);
	set_pad_cfg(SWPAD_SD4_DAT1, MX6Q_PAD_SETTINGS_USDHC);

	/* SD4 DAT2 */
	set_mux_cfg(SWMUX_SD4_DAT2, MUX_CTL_MUX_MODE_ALT1);
	set_pad_cfg(SWPAD_SD4_DAT2, MX6Q_PAD_SETTINGS_USDHC);

	/* SD4 DAT3 */
	set_mux_cfg(SWMUX_SD4_DAT3, MUX_CTL_MUX_MODE_ALT1);
	set_pad_cfg(SWPAD_SD4_DAT3, MX6Q_PAD_SETTINGS_USDHC);

 	/* SD4 Card Detect - configure GPIO2[6] as an input */
	set_mux_cfg(SWMUX_NANDF_D6, MUX_CTL_MUX_MODE_ALT5);
	out32(MX6X_GPIO2_BASE + MX6X_GPIO_GDIR, in32(MX6X_GPIO2_BASE + MX6X_GPIO_GDIR) & ~(1<<6));

	/* SD4 Write Protect - configure GPIO2[7] as an input */
	set_mux_cfg(SWMUX_NANDF_D7, MUX_CTL_MUX_MODE_ALT5);
	out32(MX6X_GPIO2_BASE + MX6X_GPIO_GDIR, in32(MX6X_GPIO2_BASE + MX6X_GPIO_GDIR) & ~(1<<7));

	/*
	* SPI
	*/

	/* SPI SCLK */
	set_mux_cfg(SWMUX_EIM_D16, MUX_CTL_MUX_MODE_ALT1);
	set_pad_cfg(SWPAD_EIM_D16, MX6Q_PAD_SETTINGS_ECSPI);
	set_pin_as_input(SWINPUT_ECSPI1_IPP_CSPI_CLK, 0x0);

	/* SPI MISO */
	set_mux_cfg(SWMUX_EIM_D17, MUX_CTL_MUX_MODE_ALT1);
	set_pad_cfg(SWPAD_EIM_D17, MX6Q_PAD_SETTINGS_ECSPI);
	set_pin_as_input(SWINPUT_ECSPI1_IPP_IND_MISO, 0x0);

	/* SPI MOSI */
	set_mux_cfg(SWMUX_EIM_D18, MUX_CTL_MUX_MODE_ALT1);
	set_pad_cfg(SWPAD_EIM_D18, MX6Q_PAD_SETTINGS_ECSPI);
	set_pin_as_input(SWINPUT_ECSPI1_IPP_IND_MOSI, 0x0);

	/* Select mux mode ALT1 for SS1 */
	set_mux_cfg(SWMUX_EIM_D19, MUX_CTL_MUX_MODE_ALT1);
	set_pad_cfg(SWPAD_EIM_D19, MX6Q_PAD_SETTINGS_ECSPI);
	set_pin_as_input(SWINPUT_ECSPI1_IPP_IND_SS_B_1, 0x0);
}

void init_aips()
{
    /*
     * Set all MPROTx to be non-bufferable, trusted for R/W,
     * not forced to user-mode.
     */
	out32(MX6X_AIPS1_CONFIG, 0x77777777);
	out32((MX6X_AIPS1_CONFIG + 0x04), 0x77777777);
	out32((MX6X_AIPS1_CONFIG + 0x40), 0x0);
	out32((MX6X_AIPS1_CONFIG + 0x44), 0x0);
	out32((MX6X_AIPS1_CONFIG + 0x48), 0x0);
	out32((MX6X_AIPS1_CONFIG + 0x4C), 0x0);
	out32((MX6X_AIPS1_CONFIG + 0x50), 0x0);

	out32(MX6X_AIPS2_CONFIG, 0x77777777);
	out32((MX6X_AIPS2_CONFIG + 0x04), 0x77777777);
	out32((MX6X_AIPS2_CONFIG + 0x40), 0x0);
	out32((MX6X_AIPS2_CONFIG + 0x44), 0x0);
	out32((MX6X_AIPS2_CONFIG + 0x48), 0x0);
	out32((MX6X_AIPS2_CONFIG + 0x4C), 0x0);
	out32((MX6X_AIPS2_CONFIG + 0x50), 0x0);
}

#define SDRAM_SIZE_MB	(1024)	
void enableTZASC( unsigned tzasc_base_addr)
{
	/*
	 * Configure TZASC action register to set tzasc_int high and issue a DECERR response when
	 * region permission failure occurs.
	 *
	 * Alternatively the TZASC interrupt polarity could be set to low. Or an OK response could
	 * be generated when and a region permission failure occurs (i.e. the offending application would not SIGBUS).
	 */
	out32(tzasc_base_addr + TZASC_ACTION, 3);

	/* Disable read and write access speculation control */
	out32(tzasc_base_addr + TZASC_SPECULATION_CONTROL, 3);

	/* 
	 * Allow secure and non-secure AXI read and write transactions to region 0 (background region) 
	 * Note that region 0 is the lowest priority, e.g. if region 1 occupies the entire DDR address
	 * space, the region 0 settings will be ignored.
	 */
	out32(tzasc_base_addr + TZASC_REGION_ATTRIBUTES(0), 0);

	/*
	 * Note that Regions can only start at offsets that are a multiple of their size!
	 * Therefore on most i.MX6 boards (RAM starts at 0x10000000) the maximum region size
	 * is 256MB.
	 */
	int i;
	for (i=1; i<=(SDRAM_SIZE_MB/256); i++)
	{
		/* Each region is 256 MB */
		out32(tzasc_base_addr + TZASC_REGION_SETUP_LOW(i), (i<<28));
		out32(tzasc_base_addr + TZASC_REGION_ATTRIBUTES(i), TZASC_REGION_PERM_ENABLEALL | TZASC_REGION_SIZE_256MB | TZASC_REGION_ATTR_ENABLE);
	}

	/* Do not bypass TZASC-1 and monitor the transactions to DDR */
	out32(MX6X_IOMUXC_BASE + MX6X_IOMUX_GPR9, 0x1);
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/boards/mx6q-sabrelite/init.c $ $Rev: 740617 $")
#endif
