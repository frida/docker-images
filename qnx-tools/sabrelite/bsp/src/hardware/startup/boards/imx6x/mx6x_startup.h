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


#ifndef __MX6X_STARTUP_H
#define __MX6X_STARTUP_H

enum mx6_chip_type_list {
	MX6_CHIP_TYPE_QUAD_OR_DUAL 	= 0x63,
	MX6_CHIP_TYPE_DUAL_LITE_OR_SOLO = 0x61,
};
enum mx6_chip_rev_list {
	MX6_CHIP_REV_1_0		= 0x0,
	MX6_CHIP_REV_1_1		= 0x1,
	MX6_CHIP_REV_1_2		= 0x2,
};

enum mx6_ldo_list {
	LDO_PU					= 0x0,
	LDO_ARM					= 0x1,
	LDO_SOC					= 0x2,
};

/*
 * LDO power gated off in PMU_REG_CORE (REG#_TARG bits = 00000)
 */
#define CPU_POWER_GATED_OFF_VOLTAGE		0x0   

/*
 * IPG CLK = 66MHz, therefore period = 15.15ns = 0.015us
 * Therefore 66 clocks per us
 */
#define IPG_CLK                     66000000
#define IPG_CLKS_IN_ONE_US          66

#define AHB_CLOCK					132000000

/*
 * i.MX 6Dual/6Quad Reference Manual indicates that the max lock time for
 * PLL8 is 7500 reference clocks (reference clock = 24MHz).  The period of 
 * the reference clock is 41.6667ns, 7500ref clks * 41.6667ns = 312.5 us.
 */
#define MAX_PLL8_LOCK_TIME_IN_US    313
#ifndef TRUE
	#define TRUE 1
#endif
#ifndef FALSE
	#define FALSE 0
#endif

/* Startup command line arguments */
#define MX6X_WDOG_ENABLE            (1 << 0)
#define MX6X_CAN1_ENABLE            (1 << 1)
#define MX6X_AUDIO_CAPTURE_ENABLE   (1 << 2)
#define MX6X_SDMA_ENABLE            (1 << 3)

extern unsigned mx6_chip_rev;
extern unsigned mx6_chip_type;

/* 
 * SDMA can be used to significantly improve boot time by speeding up copy throughput 
 * SDMA can be enabled via a startup option (see main.c)
 * Note that SDMA will not improve the performance of uncompression (such as for copying an IFS with the '+compress' option
 * in the build file.
 */
int enable_sdma_copy;

#define NO_PAD_CTRL					((uint64_t)1 << (41 + 16))

#define MX6X_PAD_SETTINGS_GENERAL 	(PAD_CTL_HYS_ENABLE | PAD_CTL_PUS_100K_PU | \
                                  	PAD_CTL_PUE_PULL | PAD_CTL_PKE_ENABLE | PAD_CTL_ODE_DISABLE | \
                                 	PAD_CTL_SPEED_MEDIUM | PAD_CTL_DSE_40_OHM | PAD_CTL_SRE_SLOW)

#define MX6X_PAD_SETTINGS_SLC_NAND (PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL | PAD_CTL_PUS_100K_PU \
									| PAD_CTL_SPEED_MEDIUM | PAD_CTL_DSE_40_OHM | PAD_CTL_SRE_FAST | \
									PAD_CTL_HYS_ENABLE)

#define MX6X_PAD_SETTINGS_USB   	(PAD_CTL_SRE_FAST | PAD_CTL_ODE_DISABLE | PAD_CTL_PKE_ENABLE | \
 									PAD_CTL_PUE_PULL | PAD_CTL_DSE_40_OHM | PAD_CTL_PUS_100K_PU | \
									PAD_CTL_HYS_ENABLE | PAD_CTL_SPEED_LOW)

#define MX6X_PAD_SETTINGS_USB_HSIC_RESET (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL | PAD_CTL_DSE_80_OHM)
#define MX6X_PAD_SETTINGS_USB_HSIC_IDLE  (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL | PAD_CTL_DSE_80_OHM | PAD_CTL_PUS_47K_PU)

#define MX6X_PAD_SETTINGS_I2C   	(PAD_CTL_SRE_FAST | PAD_CTL_ODE_ENABLE | PAD_CTL_PKE_ENABLE | \
                            		PAD_CTL_PUE_PULL | PAD_CTL_DSE_40_OHM | PAD_CTL_PUS_100K_PU | \
							        PAD_CTL_HYS_ENABLE | PAD_CTL_SPEED_MEDIUM)

#define MX6X_PAD_SETTINGS_ESAI  	(PAD_CTL_DSE_40_OHM | PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | \
								 	PAD_CTL_PUE_PULL | PAD_CTL_PUS_100K_PU    )
#define MX6X_PAD_SETTINGS_ESAI_DAT 	(PAD_CTL_DSE_40_OHM | PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_DISABLE | \
								 	PAD_CTL_PUE_PULL | PAD_CTL_PUS_100K_PU    )

#define MX6X_PAD_SETTINGS_CLKO 		(PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL | PAD_CTL_PUS_47K_PU | \
									PAD_CTL_SPEED_LOW | PAD_CTL_DSE_80_OHM | PAD_CTL_SRE_FAST | PAD_CTL_HYS_ENABLE)

#define MX6X_PAD_SETTINGS_ECSPI 	(PAD_CTL_SRE_FAST | PAD_CTL_SPEED_MEDIUM | PAD_CTL_DSE_40_OHM | PAD_CTL_HYS_ENABLE)

#define MX6X_PAD_SETTINGS_HDMI 		(PAD_CTL_HYS_DISABLE| PAD_CTL_PUS_100K_PU | \
                                  	PAD_CTL_PUE_PULL | PAD_CTL_PKE_ENABLE | PAD_CTL_ODE_DISABLE | \
                                  	PAD_CTL_SPEED_MEDIUM | PAD_CTL_DSE_40_OHM | PAD_CTL_SRE_FAST)


#define MX6X_PAD_SETTINGS_USDHC_SDIO (PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL | PAD_CTL_PUS_47K_PU | \
                                PAD_CTL_SPEED_LOW | PAD_CTL_DSE_80_OHM | PAD_CTL_SRE_FAST | \
                                PAD_CTL_HYS_ENABLE)

#define MX6SDL_PAD_SETTINGS_USDHC	(PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL | PAD_CTL_PUS_22K_PU | \
									PAD_CTL_SPEED_LOW | PAD_CTL_DSE_40_OHM | PAD_CTL_SRE_FAST | \
									PAD_CTL_HYS_ENABLE)
#define MX6X_PAD_SETTINGS_AUDMUX    MX6X_PAD_SETTINGS_GENERAL
#define MX6X_PAD_SETTINGS_CAN       MX6X_PAD_SETTINGS_GENERAL
#define MX6X_PAD_SETTINGS_GPIO      MX6X_PAD_SETTINGS_GENERAL
#define MX6X_PAD_SETTINGS_IPU       MX6X_PAD_SETTINGS_GENERAL
#define MX6X_PAD_SETTINGS_MLB       MX6X_PAD_SETTINGS_GENERAL
#define MX6X_PAD_SETTINGS_ENET		MX6X_PAD_SETTINGS_GENERAL
#define MX6X_PAD_SETTINGS_USDHC		MX6X_PAD_SETTINGS_GENERAL
#define MX6X_PAD_SETTINGS_UART		MX6X_PAD_SETTINGS_GENERAL
#define MX6X_PAD_SETTINGS_SPDIF     MX6X_PAD_SETTINGS_GENERAL

void mx6x_usleep(uint32_t sleep_duration);
void mx6x_init_raminfo(unsigned ram_size);
void mx6x_init_qtime(void);
void mx6x_init_clocks(void);
void mx6x_init_aipstz(void);
void mx6x_dump_clocks(void);
uint32_t mx6x_get_usdhc1_clk(void);
uint32_t mx6x_get_usdhc2_clk(void);
uint32_t mx6x_get_usdhc3_clk(void);
uint32_t mx6x_get_usdhc4_clk(void);
uint32_t mx6x_get_cpu_clk(void);
uint32_t mx6x_get_enfc_clk(void);
uint32_t mx6x_get_ddr_clk(void);
uint32_t mx6x_get_ipg_clk(void);
uint32_t mx6x_get_ipg_per_clk(void);
uint32_t mx6x_get_uart_clk(void);
uint32_t mx6x_get_cspi_clk(void);
uint32_t mx6x_get_ahb_clk(void);
uint32_t mx6x_get_axi_clk(void);
uint32_t mx6x_get_emi_slow_clk(void);
void mx6x_init_gpu3D(void);
void mx6_init_usdhc_clk(void);
void mx6x_wdg_enable(void);
void mx6x_wdg_reload(void);
void mx6x_init_usb_otg();
void mx6x_init_usb_host1();
void mx6x_init_usb_host2();
void mx6x_init_usb_host3();
void mx6x_init_usb_phy(uint32_t phy_addr);
int mx6x_init_sata(uint32_t freq_enet);
uint32_t mx6x_set_gpio_output(uint32_t base, uint32_t pin, uint32_t level);
uint32_t mx6x_set_gpio_input(uint32_t base, uint32_t pin);
uint32_t mx6x_reset_gpio_pin(uint32_t base, uint32_t pin,  uint32_t level);
uint32_t mx6x_reset_gpio_pin_fin(uint32_t dur);
uint32_t mx6x_get_gpio_value(uint32_t base, uint32_t pin, uint32_t *gpio_val);
uint32_t get_mx6_chip_rev();
uint32_t get_mx6_chip_type();
void print_chip_info();
uint32_t pmu_get_voltage(uint32_t ldo_num);
uint32_t pmu_set_voltage(uint32_t ldo_num, uint32_t req_voltage);
uint32_t pmu_set_standard_ldo_voltages(void);
uint32_t pmu_power_up_gpu(void);
uint32_t pmu_power_down_gpu(void);
void disable_gpu_clocks();
void enable_gpu_clocks();

#endif




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/mx6x_startup.h $ $Rev: 738158 $")
#endif
