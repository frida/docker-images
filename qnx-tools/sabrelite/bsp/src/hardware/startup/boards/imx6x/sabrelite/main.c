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
 * i.MX6Q Sabre-Lite board with Cortex-A9 MPCore
 */

#include "startup.h"
#include <time.h>
#include "board.h"

extern int dcache_enable;

extern struct callout_rtn reboot_mx6x;
extern void mx6q_usb_otg_host_init(void);
extern void mx6q_usb_host1_init(void);
extern void mx6q_init_ecspi(void);
extern void mx6q_init_i2c1(void);
extern void mx6q_init_i2c2(void);
extern void mx6q_init_i2c3(void);
extern void mx6q_init_usdhc3(void);
extern void mx6q_init_usdhc3_sdio(void);
extern void mx6q_init_usdhc4(void);
extern void mx6q_init_displays(void);
extern void mx6q_init_lvds(void);
extern void mx6q_init_lcd_panel(void);
extern void mx6q_init_can(void);
extern void init_audio(void);
extern void mx6q_init_audmux_pins(void);
extern void mx6q_init_enet(void);
extern void mx6q_init_mac(void);
extern void mx6q_init_camera(void);
extern void mx6x_init_lvds_clock(void);

const struct callout_slot callouts[] = {
	{ CALLOUT_SLOT( reboot, _mx6x) },
};

const struct debug_device debug_devices[] = {
	{ 	"mx1",
		{"0x021E8000^0.115200.80000000.16",
		},
		init_mx1,
		put_mx1,
		{	&display_char_mx1,
			&poll_key_mx1,
			&break_detect_mx1,
		}
	},
};


unsigned mx6x_per_clock = 0;
unsigned sdio_mux = FALSE;
uint32_t uart_clock;


/*
 * main()
 *	Startup program executing out of RAM
 *
 * 1. It gathers information about the system and places it in a structure
 *    called the system page. The kernel references this structure to
 *    determine everything it needs to know about the system. This structure
 *    is also available to user programs (read only if protection is on)
 *    via _syspage->.
 *
 * 2. It (optionally) turns on the MMU and starts the next program
 *    in the image file system.
 */
int
main(int argc, char **argv, char **envv)
{
	int		opt, options = 0;

	/*
	 * Initialise debugging output
	 */
	select_debug(debug_devices, sizeof(debug_devices));

	add_callout_array(callouts, sizeof(callouts));

	// common options that should be avoided are:
	// "AD:F:f:I:i:K:M:N:o:P:R:S:Tvr:j:Z:w"
	while ((opt = getopt(argc, argv, COMMON_OPTIONS_STRING "mWc")) != -1) {
		switch (opt) {
			case 'm':
				dcache_enable = TRUE;
				break;
			case 'c':
				sdio_mux = TRUE;
				break;
			case 'W':
				options |= MX6X_WDOG_ENABLE;
				break;

			default:
				handle_common_option(opt);
				break;
		}
	}

	if (options &  MX6X_WDOG_ENABLE) {
		/*
		* Enable WDT
		*/
		mx6x_wdg_reload();
		mx6x_wdg_enable();
	}

	/*
	 * Collect information on all free RAM in the system
	 */
	mx6x_init_raminfo(MX6X_SDRAM_SIZE);

	/*
	 * set CPU frequency, currently max stable CPU freq is 792MHz
	 */
	if (cpu_freq == 0)
		cpu_freq = mx6x_get_cpu_clk();

	/*
	 * Remove RAM used by modules in the image
	 */
	alloc_ram(shdr->ram_paddr, shdr->ram_size, 1);

	/*
	  * Initialize SMP
	  */
	init_smp();

	/*
	  * Initialize MMU
	  */
	if (shdr->flags1 & STARTUP_HDR_FLAGS1_VIRTUAL)
		init_mmu();

	/* Initialize the Interrupts related Information */
	init_intrinfo();

	/* Initialize the Timer related information */
	mx6x_init_qtime();

	/* Init Clocks (must happen after timer is initialized */
	mx6x_init_clocks();

	/* Init L2 Cache Controller */
	init_cacheattr();

	/* Initialize the CPU related information */
	init_cpuinfo();
	
	/* Initialize the Hwinfo section of the Syspage */
 	init_hwinfo(); 

	add_typed_string(_CS_MACHINE, "i.MX6Q Sabre-Lite Board");
	
	/* AIPSTZ init */
	mx6x_init_aipstz();
	
	/* ENET */
	mx6q_init_mac();
	mx6q_init_enet();

	/* Pull GPIO3[23] down to reset Micrel KSZ9021RN */
  	mx6x_reset_gpio_pin(MX6X_GPIO3_BASE, 23, 0);

	/* Init CAN Controller */
	mx6q_init_can();

	/* Configure PIN MUX to enable i2c */
	mx6q_init_i2c1();
	mx6q_init_i2c2();
	mx6q_init_i2c3();

	/* Init audio to support SGTL5000 codec */
	mx6q_init_audmux_pins();
	init_audio();

	/* Test if LDO_PU is gated (responsible for VPU/GPU power domain) */
	if (pmu_get_voltage(LDO_PU) == CPU_POWER_GATED_OFF_VOLTAGE)
	{
		pmu_power_up_gpu();
	}

	/* Set GPU3D clocks */
	mx6x_init_gpu3D();

	/*
	 * If using an LVDS display mx6x_init_lvds_clock() should be called which will select
	 * PLL5 as the LDB clock source.
	 */
	mx6x_init_lvds_clock();

	/*
	 * Support SDIO based Silex WLAN adapter or a regular SD card
	 * in the bottom SD slot.
	 */
	if (sdio_mux)
		mx6q_init_usdhc3_sdio();
	else
		mx6q_init_usdhc3();
	
	/* Pin muxing for top micro SD slot */
	mx6q_init_usdhc4();
	
	/* Init USB OTG, H1 */
	mx6q_usb_otg_host_init();
	mx6q_usb_host1_init();
	
	/* Configure PIN MUX to enable SPI */
	mx6q_init_ecspi();

	/* Configure pins for LVDS, LCD display*/
	mx6q_init_displays();

	/* SATA */
	mx6x_init_sata(ANATOP_PLL8_ENET_REF_ENET_50M);

	/* Micrel KSZ9021RN requires 10ms reset time */
	mx6x_reset_gpio_pin_fin(10000);

	mx6x_dump_clocks();

	mx6q_init_camera();

	/*
	 * Load bootstrap executables in the image file system and Initialise
	 * various syspage pointers. This must be the _last_ initialisation done
	 * before transferring control to the next program.
	 */
	init_system_private();

	/*
	 * This is handy for debugging a new version of the startup program.
	 * Commenting this line out will save a great deal of code.
	 */
	print_syspage();
	return 0;
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/sabrelite/main.c $ $Rev: 749637 $")
#endif
