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

/*
 * Routines to initialize the various hardware subsystems
 * on the i.MX6Q Sabre-Lite
 */

#include "startup.h"
#include "board.h"

/*
 * The Sabre-Lite board has a 50kOhm pull-down resistor connected to SD slot 3's write protect signal (SD3_WP).
 * The pad connected to the SD3_WP signal is called SD3_DAT4, due to the high pull-down resistor we must connect
 * a low internal pull-up resistor to pad SD3_DAT4 to ensure that SD3_WP's voltages are interpreted by the i.MX6Q correctly.
 */
#define MX6Q_PAD_SETTINGS_USDHC3_WP	(PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL | PAD_CTL_PUS_22K_PU | PAD_CTL_SPEED_LOW | \
							PAD_CTL_DSE_80_OHM | PAD_CTL_HYS_ENABLE)

#define MX6Q_PAD_SETTINGS_USDHC3_WP_SDIO (PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL | PAD_CTL_PUS_22K_PU | PAD_CTL_SPEED_MEDIUM | \
                            PAD_CTL_DSE_40_OHM | PAD_CTL_HYS_ENABLE)


#define CCM_CCOSR_CLKO1_EN	(0x1 << 7)
#define CCM_CCOSR_CLKO1_DIV_8	(0x7 << 4)
#define CCM_CCOSR_CLKO1_AHB	(0xb << 0)

void mx6q_init_i2c1(void)
{
	/* I2C1  SCL */
	pinmux_set_swmux(SWMUX_EIM_D21, MUX_CTL_MUX_MODE_ALT6 | MUX_CTL_SION);
	pinmux_set_padcfg(SWPAD_EIM_D21, MX6X_PAD_SETTINGS_I2C);
	pinmux_set_input(SWINPUT_I2C1_IPP_SCL_IN, 0x0);

	/* I2C1  SDA */
	pinmux_set_swmux(SWMUX_EIM_D28, MUX_CTL_MUX_MODE_ALT1 | MUX_CTL_SION);
	pinmux_set_padcfg(SWPAD_EIM_D28, MX6X_PAD_SETTINGS_I2C);
	pinmux_set_input(SWINPUT_I2C1_IPP_SDA_IN, 0x0);
}

void mx6q_init_i2c2(void)
{
	/* I2C2  SCL */
	pinmux_set_swmux(SWMUX_KEY_COL3, MUX_CTL_MUX_MODE_ALT4 | MUX_CTL_SION);
	pinmux_set_padcfg(SWPAD_KEY_COL3, MX6X_PAD_SETTINGS_I2C);
	pinmux_set_input(SWINPUT_I2C2_IPP_SCL_IN, 0x1);

	/* I2C2  SDA */
	pinmux_set_swmux(SWMUX_KEY_ROW3, MUX_CTL_MUX_MODE_ALT4 | MUX_CTL_SION);
	pinmux_set_padcfg(SWPAD_KEY_ROW3, MX6X_PAD_SETTINGS_I2C);
	pinmux_set_input(SWINPUT_I2C2_IPP_SDA_IN, 0x1);
}

void mx6q_init_i2c3(void)
{
	/* I2C3 SCL */
	pinmux_set_swmux(SWMUX_GPIO_5, MUX_CTL_MUX_MODE_ALT6 | MUX_CTL_SION);
	pinmux_set_padcfg(SWPAD_GPIO_5, MX6X_PAD_SETTINGS_I2C);
	pinmux_set_input(SWINPUT_I2C3_IPP_SCL_IN, 0x2);

	/* I2C3  SDA */
	pinmux_set_swmux(SWMUX_GPIO_16, MUX_CTL_MUX_MODE_ALT6 | MUX_CTL_SION);
	pinmux_set_padcfg(SWPAD_GPIO_16, MX6X_PAD_SETTINGS_I2C);
	pinmux_set_input(SWINPUT_I2C3_IPP_SDA_IN, 0x2);
}
void mx6q_init_audmux_pins(void)
{
	// CCM CLKO pin muxing
	pinmux_set_swmux(SWMUX_GPIO_0, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_GPIO_0, MX6X_PAD_SETTINGS_CLKO);

	// AUDMUX pin muxing (AUD4_RXD, AUD4_TXC, AUD4_TXD, AUD4_TXFS)	
	pinmux_set_swmux(SWMUX_SD2_DAT0, MUX_CTL_MUX_MODE_ALT3);
	pinmux_set_input(SWINPUT_AUDMUX_P4_DA_AMX, 0x0);

	pinmux_set_swmux(SWMUX_SD2_DAT3, MUX_CTL_MUX_MODE_ALT3);
	pinmux_set_input(SWINPUT_AUDMUX_P4_TXCLK_AMX, 0x1);

	pinmux_set_swmux(SWMUX_SD2_DAT2, MUX_CTL_MUX_MODE_ALT3);
	pinmux_set_input(SWINPUT_AUDMUX_P4_DB_AMX, 0x0);

	pinmux_set_swmux(SWMUX_SD2_DAT1, MUX_CTL_MUX_MODE_ALT3);
	pinmux_set_input(SWINPUT_AUDMUX_P4_TXFS_AMX, 0x0);

	// Configure CLKO to produce 16.5MHz master clock
	out32(MX6X_CCM_BASE + MX6X_CCM_CCOSR, CCM_CCOSR_CLKO1_EN | CCM_CCOSR_CLKO1_AHB | CCM_CCOSR_CLKO1_DIV_8); 
	
}
void mx6q_init_enet(void)
{
	// RGMII MDIO - transfers control info between MAC and PHY
	pinmux_set_swmux(SWMUX_ENET_MDIO, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_input(SWINPUT_ENET_IPP_IND_MAC0_MDIO,0);

	// RGMII MDC - output from MAC to PHY, provides clock reference for MDIO
	pinmux_set_swmux(SWMUX_ENET_MDC, MUX_CTL_MUX_MODE_ALT1);

	// RGMII TXC - output from MAC, provides clock used by RGMII_TXD[3:0], RGMII_TX_CTL
	pinmux_set_swmux(SWMUX_RGMII_TXC, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_RGMII_TXC, MX6X_PAD_SETTINGS_ENET);

	// RGMII TXD[3:0] - Transmit Data Output
	pinmux_set_swmux(SWMUX_RGMII_TD0, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_RGMII_TD0, MX6X_PAD_SETTINGS_ENET);

	pinmux_set_swmux(SWMUX_RGMII_TD1, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_RGMII_TD1, MX6X_PAD_SETTINGS_ENET);

	pinmux_set_swmux(SWMUX_RGMII_TD2, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_RGMII_TD2, MX6X_PAD_SETTINGS_ENET);

	pinmux_set_swmux(SWMUX_RGMII_TD3, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_RGMII_TD3, MX6X_PAD_SETTINGS_ENET);

	// RGMII TX_CTL - contains TXEN on TXC rising edge, TXEN XOR TXERR on TXC falling edge
	pinmux_set_swmux(SWMUX_RGMII_TX_CTL, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_RGMII_TX_CTL, MX6X_PAD_SETTINGS_ENET);

	// set ENET_REF_CLK to mux mode 1 - TX_CLK, this is a 125MHz input which is driven by the PHY
	pinmux_set_swmux(SWMUX_ENET_REF_CLK, MUX_CTL_MUX_MODE_ALT1);	

	pinmux_set_swmux(SWMUX_RGMII_RXC, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_RGMII_RXC, MX6X_PAD_SETTINGS_ENET);
	pinmux_set_input(SWINPUT_ENET_IPP_IND_MAC0_RXCLK,0);

	pinmux_set_swmux(SWMUX_RGMII_RD0, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_RGMII_RD0, MX6X_PAD_SETTINGS_ENET);
	pinmux_set_input(SWINPUT_ENET_IPP_IND_MAC_RXDATA_0,0);

	pinmux_set_swmux(SWMUX_RGMII_RD1, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_RGMII_RD1, MX6X_PAD_SETTINGS_ENET);
	pinmux_set_input(SWINPUT_ENET_IPP_IND_MAC_RXDATA_1,0);

	pinmux_set_swmux(SWMUX_RGMII_RD2, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_RGMII_RD2, MX6X_PAD_SETTINGS_ENET);
	pinmux_set_input(SWINPUT_ENET_IPP_IND_MAC_RXDATA_2,0);

	pinmux_set_swmux(SWMUX_RGMII_RD3, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_RGMII_RD3, MX6X_PAD_SETTINGS_ENET);
	pinmux_set_input(SWINPUT_ENET_IPP_IND_MAC_RXDATA_3,0);

	pinmux_set_swmux(SWMUX_RGMII_RX_CTL, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_RGMII_RX_CTL, MX6X_PAD_SETTINGS_ENET);
	pinmux_set_input(SWINPUT_ENET_IPP_IND_MAC0_RXEN,0);

	// TX EN set to GPIO1[28]
	pinmux_set_swmux(SWMUX_ENET_TX_EN, MUX_CTL_MUX_MODE_ALT5);

	//RGMII reset
	pinmux_set_swmux(SWMUX_EIM_D23, MUX_CTL_MUX_MODE_ALT5);
}

void mx6q_init_usdhc3(void)
{
	/* SD3 CLK */
	pinmux_set_swmux(SWMUX_SD3_CLK, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD3_CLK, MX6X_PAD_SETTINGS_USDHC);

	/* SD3 CMD */
	pinmux_set_swmux(SWMUX_SD3_CMD, MUX_CTL_MUX_MODE_ALT0 | MUX_CTL_SION);
	pinmux_set_padcfg(SWPAD_SD3_CMD, MX6X_PAD_SETTINGS_USDHC);

	/* SD3 DAT0 */
	pinmux_set_swmux(SWMUX_SD3_DAT0, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD3_DAT0, MX6X_PAD_SETTINGS_USDHC);

	/* SD3 DAT1 */
	pinmux_set_swmux(SWMUX_SD3_DAT1, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD3_DAT1, MX6X_PAD_SETTINGS_USDHC);

	/* SD3 DAT2 */
	pinmux_set_swmux(SWMUX_SD3_DAT2, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD3_DAT2, MX6X_PAD_SETTINGS_USDHC);

	/* SD3 DAT3 */
	pinmux_set_swmux(SWMUX_SD3_DAT3, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD3_DAT3, MX6X_PAD_SETTINGS_USDHC);

	/* SD3 Write Protect - configure GPIO7[1] as an input */
	pinmux_set_swmux(SWMUX_SD3_DAT4, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_padcfg(SWPAD_SD3_DAT4, MX6Q_PAD_SETTINGS_USDHC3_WP);
	out32(MX6X_GPIO7_BASE + MX6X_GPIO_GDIR, in32(MX6X_GPIO7_BASE + MX6X_GPIO_GDIR) & ~(1<<1));

	/* SD3 Card Detect - configure GPIO7[0] as an input */
	pinmux_set_swmux(SWMUX_SD3_DAT5, MUX_CTL_MUX_MODE_ALT5);
	out32(MX6X_GPIO7_BASE + MX6X_GPIO_GDIR, in32(MX6X_GPIO7_BASE + MX6X_GPIO_GDIR) & ~(1<<0));
}

void mx6q_init_usdhc3_sdio(void)
{
	/* SD3 CLK */
	pinmux_set_swmux(SWMUX_SD3_CLK, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD3_CLK, MX6X_PAD_SETTINGS_USDHC_SDIO);

	/* SD3 CMD */
	pinmux_set_swmux(SWMUX_SD3_CMD, MUX_CTL_MUX_MODE_ALT0 | MUX_CTL_SION);
	pinmux_set_padcfg(SWPAD_SD3_CMD, MX6X_PAD_SETTINGS_USDHC_SDIO);

	/* SD3 DAT0 */
	pinmux_set_swmux(SWMUX_SD3_DAT0, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD3_DAT0, MX6X_PAD_SETTINGS_USDHC_SDIO);

	/* SD3 DAT1 */
	pinmux_set_swmux(SWMUX_SD3_DAT1, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD3_DAT1, MX6X_PAD_SETTINGS_USDHC_SDIO);

	/* SD3 DAT2 */
	pinmux_set_swmux(SWMUX_SD3_DAT2, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD3_DAT2, MX6X_PAD_SETTINGS_USDHC_SDIO);

	/* SD3 DAT3 */
	pinmux_set_swmux(SWMUX_SD3_DAT3, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD3_DAT3, MX6X_PAD_SETTINGS_USDHC_SDIO);

	/* SD3 Card Detect - configure GPIO7[0] as an input */
	pinmux_set_swmux(SWMUX_SD3_DAT5, MUX_CTL_MUX_MODE_ALT5);

	/* SD3 Write Protect - configure GPIO7[1] as an input */
	pinmux_set_swmux(SWMUX_SD3_DAT4, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_padcfg(SWPAD_SD3_DAT4, MX6Q_PAD_SETTINGS_USDHC3_WP_SDIO);
	out32(MX6X_GPIO7_BASE + MX6X_GPIO_GDIR, in32(MX6X_GPIO7_BASE + MX6X_GPIO_GDIR) & ~(1<<1));
}

void mx6q_init_usdhc4(void)
{
	/* SD4 CLK */
	pinmux_set_swmux(SWMUX_SD4_CLK, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD4_CLK, MX6X_PAD_SETTINGS_USDHC);

	/* SD4 CMD */
	pinmux_set_swmux(SWMUX_SD4_CMD, MUX_CTL_MUX_MODE_ALT0 | MUX_CTL_SION);
	pinmux_set_padcfg(SWPAD_SD4_CMD, MX6X_PAD_SETTINGS_USDHC);

	/* SD4 DAT0 */
	pinmux_set_swmux(SWMUX_SD4_DAT0, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_SD4_DAT0, MX6X_PAD_SETTINGS_USDHC);

	/* SD4 DAT1 */
	pinmux_set_swmux(SWMUX_SD4_DAT1, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_SD4_DAT1, MX6X_PAD_SETTINGS_USDHC);

	/* SD4 DAT2 */
	pinmux_set_swmux(SWMUX_SD4_DAT2, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_SD4_DAT2, MX6X_PAD_SETTINGS_USDHC);

	/* SD4 DAT3 */
	pinmux_set_swmux(SWMUX_SD4_DAT3, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_SD4_DAT3, MX6X_PAD_SETTINGS_USDHC);

	/* SD4 Card Detect - configure GPIO2[6] as an input */
	pinmux_set_swmux(SWMUX_NANDF_D6, MUX_CTL_MUX_MODE_ALT5);
	out32(MX6X_GPIO2_BASE + MX6X_GPIO_GDIR, in32(MX6X_GPIO2_BASE + MX6X_GPIO_GDIR) & ~(1<<6));

	/* SD4 Write Protect - configure GPIO2[7] as an input */
	pinmux_set_swmux(SWMUX_NANDF_D7, MUX_CTL_MUX_MODE_ALT5);
	out32(MX6X_GPIO2_BASE + MX6X_GPIO_GDIR, in32(MX6X_GPIO2_BASE + MX6X_GPIO_GDIR) & ~(1<<7));
}

void mx6q_init_ecspi(void)
{
	/* SPI SCLK */
	pinmux_set_swmux(SWMUX_EIM_D16, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_EIM_D16, MX6X_PAD_SETTINGS_ECSPI);
	pinmux_set_input(SWINPUT_ECSPI1_IPP_CSPI_CLK, 0x0);

	/* SPI MISO */
	pinmux_set_swmux(SWMUX_EIM_D17, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_EIM_D17, MX6X_PAD_SETTINGS_ECSPI);
	pinmux_set_input(SWINPUT_ECSPI1_IPP_IND_MISO, 0x0);
	
	/* SPI MOSI */
	pinmux_set_swmux(SWMUX_EIM_D18, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_EIM_D18, MX6X_PAD_SETTINGS_ECSPI);
	pinmux_set_input(SWINPUT_ECSPI1_IPP_IND_MOSI, 0x0);
	
	/* Select mux mode ALT1 for SS1 */
	pinmux_set_swmux(SWMUX_EIM_D19, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_EIM_D19, MX6X_PAD_SETTINGS_ECSPI);
	pinmux_set_input(SWINPUT_ECSPI1_IPP_IND_SS_B_1, 0x0);
}

void mx6q_init_can(void)
{
	/*
	 * The code below initializes the i.MX6 Q CAN1 module pins,
	 * and the corresponding MC33902 CAN transceiver chips.
	 */

	/* CAN1 TX: signal name: KEY_COL2, pad name: KEY_COL2, connected to module can1's TXCAN pin */
	pinmux_set_swmux(SWMUX_KEY_COL2, MUX_CTL_MUX_MODE_ALT2);	

	/* CAN1 RX: signal name: KEY_ROW2, pad name: KEY_ROW2, connected to module can1's RXCAN pin */
	pinmux_set_swmux(SWMUX_KEY_ROW2, MUX_CTL_MUX_MODE_ALT2);
	pinmux_set_input(SWINPUT_CAN1_IPP_IND_CANRX, 0);
	
	/*
	 * CAN1 NERR: signal name: CAN1_NERR, pad name: GPIO_7, connected to GPIO1[7] pin
	 * active low error signal which is driven by the CAN transceiver chip (i.e. an _input_ to the i.MX6 Q)
	 */
	pinmux_set_swmux(SWMUX_GPIO_7, MUX_CTL_MUX_MODE_ALT5);
	out32(MX6X_GPIO1_BASE + MX6X_GPIO_GDIR, in32(MX6X_GPIO1_BASE + MX6X_GPIO_GDIR) & ~(1<<7));
	
	/*
	 * CAN1 EN: signal name: CAN1_EN, pad name: GPIO_4, connected to GPIO1[4] pin
	 * Code below also sets the EN signal to be high
	 */
	pinmux_set_swmux(SWMUX_GPIO_4, MUX_CTL_MUX_MODE_ALT5);
	out32(MX6X_GPIO1_BASE + MX6X_GPIO_GDIR, in32(MX6X_GPIO1_BASE + MX6X_GPIO_GDIR) | (1<<4));
	out32(MX6X_GPIO1_BASE + MX6X_GPIO_DR, in32(MX6X_GPIO1_BASE + MX6X_GPIO_DR) | (1<<4));
	
	/*
	 * CAN1 STBY: signal CAN1_STBY, pad name: GPIO_2, connected to GPIO1[2] pin
	 * On REV-D boards, the CAN transceiver chip has an active high standby pin,
     * on older boards it's active low. Code below ensures that STBY is 
	 * low to prevent the transceiver chip from being in standby mode.
	 */

	pinmux_set_swmux(SWMUX_GPIO_2, MUX_CTL_MUX_MODE_ALT5);
	out32(MX6X_GPIO1_BASE + MX6X_GPIO_GDIR, in32(MX6X_GPIO1_BASE + MX6X_GPIO_GDIR) | (1<<2));
    if (get_mx6_chip_rev() >=  MX6_CHIP_REV_1_2)
        out32(MX6X_GPIO1_BASE + MX6X_GPIO_DR, in32(MX6X_GPIO1_BASE + MX6X_GPIO_DR) & ~(1<<2));
    else
        out32(MX6X_GPIO1_BASE + MX6X_GPIO_DR, in32(MX6X_GPIO1_BASE + MX6X_GPIO_DR) | (1<<2));
}

void mx6q_init_lvds(void)
{
	/* Enable PWM - Used to configure backlight brightness (GPIO1[18]) */
	pinmux_set_swmux(SWMUX_SD1_CMD, MUX_CTL_MUX_MODE_ALT5);
	
	/* 
	 * Configure GPIO controlling PWM as an output and drive the GPIO high.  In this case PWM is always high, meaning
	 * a 100% duty cycle, a lower duty cycle could be used to decrease the brightness of the display. 
	 */
	out32(MX6X_GPIO1_BASE + MX6X_GPIO_GDIR, in32(MX6X_GPIO1_BASE + MX6X_GPIO_GDIR) | (0x1 << 18));
	out32(MX6X_GPIO1_BASE + MX6X_GPIO_DR, in32(MX6X_GPIO1_BASE + MX6X_GPIO_DR) | (0x1 << 18));

	/* Set pad GPIO_9 mux mode to GPIO1[9] which allows an external LVDS display to send touch interrupts to the i.MX6Q */
	pinmux_set_swmux(SWMUX_GPIO_9, MUX_CTL_MUX_MODE_ALT5);
}
void mx6q_init_lcd_panel(void)
{
	/* Enable PWM1 pin in GPIO mode - Used to configure backlight brightness (GPIO1[21]) - set to 100% duty cycle - full brightness */
	pinmux_set_swmux(SWMUX_SD1_DAT3, MUX_CTL_MUX_MODE_ALT5);
	out32(MX6X_GPIO1_BASE + MX6X_GPIO_GDIR, in32(MX6X_GPIO1_BASE + MX6X_GPIO_GDIR) | (0x1 << 21));
	out32(MX6X_GPIO1_BASE + MX6X_GPIO_DR, in32(MX6X_GPIO1_BASE + MX6X_GPIO_DR) | (0x1 << 21));

	/* Configure 7" LCD touch screen interrupt signal to be an input (with respect to i.MX6Q) */
	pinmux_set_swmux(SWMUX_DI0_PIN4, MUX_CTL_MUX_MODE_ALT5);
	out32(MX6X_GPIO4_BASE + MX6X_GPIO_GDIR, in32(MX6X_GPIO4_BASE + MX6X_GPIO_GDIR) & ~(0x1 << 20));

	/* IPU1 Display Interface 0 clock */
	pinmux_set_swmux(SWMUX_DI0_DISP_CLK, MUX_CTL_MUX_MODE_ALT0);

	/* LCD EN */
	pinmux_set_swmux(SWMUX_DI0_PIN15, MUX_CTL_MUX_MODE_ALT0);

	/* LCD HSYNC */
	pinmux_set_swmux(SWMUX_DI0_PIN2, MUX_CTL_MUX_MODE_ALT0);

	/* LCD VSYNC */
	pinmux_set_swmux(SWMUX_DI0_PIN3, MUX_CTL_MUX_MODE_ALT0);

	/* Data Lines */
	pinmux_set_swmux(SWMUX_DISP0_DAT0, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT1, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT2, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT3, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT4, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT5, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT6, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT7, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT8, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT9, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT10, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT11, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT12, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT13, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT14, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT15, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT16, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT17, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT18, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT19, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT20, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT21, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT22, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT23, MUX_CTL_MUX_MODE_ALT0);

	/* Configure EIM_D24 pin as GPIO3_24 (Power Enable) */
	pinmux_set_swmux(SWMUX_EIM_D24, MUX_CTL_MUX_MODE_ALT5);

	/* DISP0 DET */
	pinmux_set_swmux(SWMUX_EIM_D31, MUX_CTL_MUX_MODE_ALT5);

	/* DISP0 RESET */
	pinmux_set_swmux(SWMUX_EIM_WAIT, MUX_CTL_MUX_MODE_ALT5);

	/* Configure detection pin as a GPIO input */
	out32(MX6X_GPIO3_BASE + MX6X_GPIO_GDIR, in32(MX6X_GPIO3_BASE + MX6X_GPIO_GDIR) & ~(0x1 << 31));

	/* Reset DISP0 */
	out32(MX6X_GPIO5_BASE + MX6X_GPIO_GDIR, in32(MX6X_GPIO5_BASE + MX6X_GPIO_GDIR) | 0x1);
	out32(MX6X_GPIO5_BASE + MX6X_GPIO_DR, in32(MX6X_GPIO5_BASE + MX6X_GPIO_GDIR) & ~(0x1));

	/* LCD_EN */
	out32(MX6X_GPIO3_BASE + MX6X_GPIO_GDIR, in32(MX6X_GPIO3_BASE + MX6X_GPIO_GDIR) | (0x1 << 24));
	out32(MX6X_GPIO3_BASE + MX6X_GPIO_DR, in32(MX6X_GPIO3_BASE + MX6X_GPIO_GDIR) | (0x1 << 24));
	
}
void mx6q_init_camera(void)
{
	
	// GPIO pin muxing
	pinmux_set_swmux(SWMUX_GPIO_3, MUX_CTL_MUX_MODE_ALT4);	
	pinmux_set_swmux(SWMUX_GPIO_6, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_swmux(SWMUX_GPIO_8, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_swmux(SWMUX_SD1_DAT0, MUX_CTL_MUX_MODE_ALT5);

	// CSI0 pin muxing
	pinmux_set_swmux(SWMUX_CSI0_DAT8, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSI0_DAT9, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSI0_DAT10, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSI0_DAT11, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSI0_DAT12, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSI0_DAT13, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSI0_DAT14, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSI0_DAT15, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSI0_DAT16, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSI0_DAT17, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSI0_DAT18, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSI0_DAT19, MUX_CTL_MUX_MODE_ALT0);

	pinmux_set_swmux(SWMUX_CSI0_DATA_EN, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSI0_MCLK, MUX_CTL_MUX_MODE_ALT0);	
	pinmux_set_swmux(SWMUX_CSI0_PIXCLK, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSI0_VSYNC, MUX_CTL_MUX_MODE_ALT0);	

	// configure CLKO2 to drive a 24MHz clock to the camera module
	out32(MX6X_CCM_BASE + MX6X_CCM_CCOSR, 0x010E00FB);

	// power down camera
	out32(MX6X_GPIO1_BASE + MX6X_GPIO_GDIR, in32(MX6X_GPIO1_BASE + MX6X_GPIO_GDIR) | (0x1 << 6));
	out32(MX6X_GPIO1_BASE + MX6X_GPIO_DR, in32(MX6X_GPIO1_BASE + MX6X_GPIO_DR) | (0x1 << 6));
	mx6x_usleep(1000);
	out32(MX6X_GPIO1_BASE + MX6X_GPIO_DR, in32(MX6X_GPIO1_BASE + MX6X_GPIO_DR) & ~(0x1 << 6));

	// reset camera
	out32(MX6X_GPIO1_BASE + MX6X_GPIO_GDIR, in32(MX6X_GPIO1_BASE + MX6X_GPIO_GDIR) | (0x1 << 8));
	out32(MX6X_GPIO1_BASE + MX6X_GPIO_DR, in32(MX6X_GPIO1_BASE + MX6X_GPIO_DR) | (0x1 << 8));
	out32(MX6X_GPIO1_BASE + MX6X_GPIO_DR, in32(MX6X_GPIO1_BASE + MX6X_GPIO_DR) & ~(0x1 << 8));
	mx6x_usleep(1000);
	out32(MX6X_GPIO1_BASE + MX6X_GPIO_DR, in32(MX6X_GPIO1_BASE + MX6X_GPIO_DR) | (0x1 << 8));

	// GPMC configuration
	out32(MX6X_IOMUXC_BASE + MX6X_IOMUX_GPR1, in32(MX6X_IOMUXC_BASE + MX6X_IOMUX_GPR1) | (1 << 19));
}

void mx6q_init_displays(void)
{
	mx6q_init_lcd_panel();
	mx6q_init_lvds();
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/sabrelite/mx6q_gpio.c $ $Rev: 729057 $")
#endif
