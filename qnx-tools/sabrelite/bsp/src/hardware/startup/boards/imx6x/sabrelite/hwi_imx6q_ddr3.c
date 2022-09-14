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


#include "startup.h"
#include "hwinfo_private.h"
#include <hw/hwinfo_imx6x.h>
#include <drvr/hwinfo.h>                // for hwi support routines in libdrvr
#include <arm/mx6x.h>

/*
 * Add IMX6 devices (Dual, Quad variants) to the hardware info section of the syspage.
*/

extern uint32_t uart_clock;

/* instead of hwibus_add_can() function use the following function to avoid empty tag creation
that happens inside hwibus_add_can(), as facing difficulty to fill up that empty tag */

static unsigned imx6x_hwibus_add_can(unsigned parent_hwi_off, hwiattr_can_t *attr);

void hwi_imx6dq()
{
	unsigned hwi_bus_internal = 0;

	/* add I2C (unless directed not to) */
	{
		unsigned hwi_off;
		hwiattr_i2c_t attr = HWIATTR_I2C_T_INITIALIZER;
		HWIATTR_I2C_SET_NUM_IRQ(&attr, 1);

		/* create I2C controller 1 */
		HWIATTR_I2C_SET_LOCATION(&attr, MX6X_I2C1_BASE, MX6X_I2C_SIZE, 0, hwi_find_as(MX6X_I2C1_BASE, 1));
		hwi_off = hwibus_add_i2c(hwi_bus_internal, &attr);
		ASSERT(hwi_find_unit(hwi_off) == 0);
		hwitag_set_ivec(hwi_off, 0, MX6X_I2C1_IRQ);

		/* create I2C controller 2 */
		HWIATTR_I2C_SET_LOCATION(&attr, MX6X_I2C2_BASE, MX6X_I2C_SIZE, 0, hwi_find_as(MX6X_I2C2_BASE, 1));
		hwi_off = hwibus_add_i2c(hwi_bus_internal, &attr);
		ASSERT(hwi_find_unit(hwi_off) == 1);
		hwitag_set_ivec(hwi_off, 0, MX6X_I2C2_IRQ);

		/* create I2C controller 3 */
		HWIATTR_I2C_SET_LOCATION(&attr, MX6X_I2C3_BASE, MX6X_I2C_SIZE, 0, hwi_find_as(MX6X_I2C3_BASE, 1));
		hwi_off = hwibus_add_i2c(hwi_bus_internal, &attr);
		ASSERT(hwi_find_unit(hwi_off) == 2);
		hwitag_set_ivec(hwi_off, 0, MX6X_I2C3_IRQ);
	}

	/* add  UART */
	{
		unsigned hwi_off;
		hwiattr_uart_t attr = HWIATTR_UART_T_INITIALIZER;
		struct hwi_inputclk clksrc = {.clk = uart_clock, .div = 16};
		HWIATTR_UART_SET_NUM_IRQ(&attr, 1);
		HWIATTR_UART_SET_NUM_CLK(&attr, 1);

		/* create uart0 */
		HWIATTR_UART_SET_LOCATION(&attr, MX6X_UART4_BASE, MX6X_UART_SIZE, 0, hwi_find_as(MX6X_UART4_BASE, 1));
		hwi_off = hwidev_add_uart(IMX6_HWI_UART, &attr, hwi_bus_internal);
		ASSERT(hwi_find_unit(hwi_off) == 0);
		hwitag_set_ivec(hwi_off, 0, MX6X_UART4_IRQ);
		hwitag_set_inputclk(hwi_off, 0, &clksrc);

		/* historically the UART's were called 'mxl' so add these synonyms */
		hwi_add_synonym(hwi_find_device(IMX6_HWI_UART, 0), "sermxl");
	}

	/* add the FEC */
	{
		unsigned hwi_off;
		hwiattr_enet_t attr = HWIATTR_ENET_T_INITIALIZER;
		HWIATTR_USB_SET_NUM_IRQ(&attr, 1);

		/* create eTSEC0 */
		HWIATTR_ENET_SET_LOCATION(&attr, MX6X_FEC_BASE, 0x4000, 0, hwi_find_as(MX6X_FEC_BASE, 1));
		hwi_off = hwidev_add_enet(IMX6_HWI_LEGACY_FEC, &attr, hwi_bus_internal);
		ASSERT(hwi_find_unit(hwi_off) == 0);
		hwitag_set_avail_ivec(hwi_off, 0, MX6X_FEC_IRQ);
	}

	/* add USB OTG Controller */
	{
		unsigned hwi_off;
		hwiattr_usb_t attr = HWIATTR_USB_T_INITIALIZER;
		HWIATTR_USB_SET_NUM_IRQ(&attr, 1);

		/* create USB OTG Host Controller */
		HWIATTR_USB_SET_LOCATION(&attr, MX6X_USBOTG_BASE, MX6X_USB_SIZE, 0, hwi_find_as(MX6X_USBOTG_BASE, 1));
		hwi_off = hwibus_add_usb(hwi_bus_internal, &attr);
		ASSERT(hwi_off != HWI_NULL_OFF);
		hwitag_set_avail_ivec(hwi_off, 0, MX6X_USBOTG_IRQ);
	}

	/* add 1 sdma controller */
	{
		unsigned hwi_off;
		hwiattr_dma_t attr = HWIATTR_DMA_T_INITIALIZER;
		HWIATTR_DMA_SET_NUM_IRQ(&attr, 1);

		/* create DMA controller 0 */
		HWIATTR_USB_SET_LOCATION(&attr, MX6X_SDMA_BASE, MX6X_SDMA_SIZE, 0, hwi_find_as(MX6X_SDMA_BASE, 1));
		hwi_off = hwidev_add_dma(IMX6_HWI_DMA, &attr, hwi_bus_internal);
		ASSERT(hwi_find_unit(hwi_off) == 0);
		hwitag_set_avail_ivec(hwi_off, 0, MX6X_SDMA_IRQ);
	}

	/* add the WATCHDOG device */
	{
		unsigned hwi_off;
		hwiattr_timer_t attr = HWIATTR_TIMER_T_INITIALIZER;
		const struct hwi_inputclk clksrc_kick = {.clk = 10, .div = 1};
		HWIATTR_TIMER_SET_NUM_CLK(&attr, 1);
		HWIATTR_TIMER_SET_LOCATION(&attr, MX6X_WDOG1_BASE, MX6X_WDOG_SIZE, 0, hwi_find_as(MX6X_WDOG1_BASE, 1));
		hwi_off = hwidev_add_timer(IMX6_HWI_WDOG, &attr,  HWI_NULL_OFF);
		ASSERT(hwi_off != HWI_NULL_OFF);
		hwitag_set_inputclk(hwi_off, 0, (struct hwi_inputclk *)&clksrc_kick);
	}

	/* add CAN */
	{
		unsigned hwi_off;
		hwiattr_can_t attr = HWIATTR_CAN_T_INITIALIZER;
		HWIATTR_CAN_SET_NUM_IRQ(&attr, 1);
		HWIATTR_CAN_SET_NUM_MEMADDR(&attr, 1);

		/* create CAN0 */
		HWIATTR_CAN_SET_LOCATION(&attr, MX6X_CAN1_PORT, MX6X_CAN_SIZE, 0, hwi_find_as(MX6X_CAN1_PORT, 1));
		hwi_off = imx6x_hwibus_add_can(hwi_bus_internal, &attr);
		hwitag_add_location(hwi_off, MX6X_CAN1_MEM, MX6X_CAN_SIZE, 0, 0);
		ASSERT(hwi_find_unit(hwi_off) == 0);
		hwitag_set_ivec(hwi_off, 0, MX6X_CAN1_IRQ);

		/* create CAN1 */
		HWIATTR_CAN_SET_LOCATION(&attr, MX6X_CAN2_PORT, MX6X_CAN_SIZE, 0, hwi_find_as(MX6X_CAN2_PORT, 1));
		hwi_off = imx6x_hwibus_add_can(hwi_bus_internal, &attr);
		hwitag_add_location(hwi_off, MX6X_CAN2_MEM, MX6X_CAN_SIZE, 0, 0);
		ASSERT(hwi_find_unit(hwi_off) == 1);
		hwitag_set_ivec(hwi_off, 0, MX6X_CAN2_IRQ);
	}

	/* add MLB */
	{
		unsigned hwi_off = hwidev_add("mlb", hwi_devclass_NONE, HWI_NULL_OFF);
		hwiattr_common_t attr = HWIATTR_COMMON_INITIALIZER;
		HWIATTR_SET_NUM_IRQ(&attr, 1);
		HWIATTR_SET_LOCATION(&attr, MX6X_MLB_BASE, MX6X_MLB_SIZE, 0, hwi_find_as(MX6X_MLB_BASE, 1));
		hwitag_add_common(hwi_off, &attr);
		ASSERT(hwi_off != HWI_NULL_OFF);
		hwitag_set_ivec(hwi_off, 0, MX6X_MLB_IRQ);
	}

	/* add GPU: Graphics Processing Unit */
	{
		/* create GPU2D */
		unsigned hwi_off = hwidev_add(IMX6_HWI_GPU_2D, hwi_devclass_NONE, HWI_NULL_OFF);
		hwiattr_common_t attr = HWIATTR_COMMON_INITIALIZER;
		HWIATTR_SET_LOCATION(&attr, MX6X_GPU2D_BASE, MX6X_GPU2D_SIZE, 0, hwi_find_as(MX6X_GPU2D_BASE, 1));
		hwitag_add_common(hwi_off, &attr);
		ASSERT(hwi_off != HWI_NULL_OFF);

		/* create GPU3D */
		hwi_off = hwidev_add(IMX6_HWI_GPU_3D, hwi_devclass_NONE, HWI_NULL_OFF);
		HWIATTR_SET_NUM_IRQ(&attr, 1);
		HWIATTR_SET_LOCATION(&attr, MX6X_GPU3D_BASE, MX6X_GPU3D_SIZE, 0, hwi_find_as(MX6X_GPU3D_BASE, 1));
		hwitag_add_common(hwi_off, &attr);
		ASSERT(hwi_off != HWI_NULL_OFF);
		hwitag_set_ivec(hwi_off, 0, MX6X_GPU3D_IRQ);
	}

	/* add IPU1: Image Processing Unit */
	{
		unsigned hwi_off = hwidev_add(IMX6_HWI_IPU, hwi_devclass_NONE, HWI_NULL_OFF);
		hwiattr_common_t attr = HWIATTR_COMMON_INITIALIZER;
		HWIATTR_SET_LOCATION(&attr, MX6X_IPU1_BASE, MX6X_IPU_SIZE, 0, hwi_find_as(MX6X_IPU1_BASE, 1));
		hwitag_add_common(hwi_off, &attr);
		ASSERT(hwi_off != HWI_NULL_OFF);
	}
	/* add IPU2: Image Processing Unit */
	{
		unsigned hwi_off = hwidev_add(IMX6_HWI_IPU, hwi_devclass_NONE, HWI_NULL_OFF);
		hwiattr_common_t attr = HWIATTR_COMMON_INITIALIZER;
		HWIATTR_SET_LOCATION(&attr, MX6X_IPU2_BASE, MX6X_IPU_SIZE, 0, hwi_find_as(MX6X_IPU2_BASE, 1));
		hwitag_add_common(hwi_off, &attr);
		ASSERT(hwi_off != HWI_NULL_OFF);
	}


	/* add VPU: Video Processing Unit */
	{
		unsigned hwi_off = hwidev_add(IMX6_HWI_VPU, hwi_devclass_NONE, HWI_NULL_OFF);
		hwiattr_common_t attr = HWIATTR_COMMON_INITIALIZER;
		HWIATTR_SET_NUM_IRQ(&attr, 1);
		HWIATTR_SET_LOCATION(&attr, MX6X_VPU_BASE, MX6X_VPU_SIZE, 0, hwi_find_as(MX6X_VPU_BASE, 1));
		hwitag_add_common(hwi_off, &attr);
		ASSERT(hwi_off != HWI_NULL_OFF);
		hwitag_set_ivec(hwi_off, 0, MX6X_VPU_IRQ);
	}

	/* add Enhanced Serial Audio Interface (ESAI) */
	{
		unsigned hwi_off = hwidev_add(IMX6_HWI_ESAI, hwi_devclass_NONE, HWI_NULL_OFF);
		hwiattr_common_t attr = HWIATTR_COMMON_INITIALIZER;
		HWIATTR_SET_NUM_IRQ(&attr, 1);
		HWIATTR_SET_LOCATION(&attr, MX6X_ESAI_BASE, MX6X_ESAI_SIZE, 0, hwi_find_as(MX6X_ESAI_BASE, 1));
		hwitag_add_common(hwi_off, &attr);
		ASSERT(hwi_off != HWI_NULL_OFF);
	}

	/* add the Synchronous Serial Interface (SSI) 1 device */
	{
		unsigned hwi_off = hwidev_add(IMX6_HWI_SSI, hwi_devclass_NONE, HWI_NULL_OFF);
		hwiattr_common_t attr = HWIATTR_COMMON_INITIALIZER;
		ASSERT(hwi_off != HWI_NULL_OFF);
		HWIATTR_SET_LOCATION(&attr, MX6X_SSI1_BASE, MX6X_SSI_SIZE, 0, hwi_find_as(MX6X_SSI1_BASE, 1));
		hwitag_add_common(hwi_off, &attr);
	}
	/* add the Synchronous Serial Interface (SSI) 2 device */
	{
		unsigned hwi_off = hwidev_add(IMX6_HWI_SSI, hwi_devclass_NONE, HWI_NULL_OFF);
		hwiattr_common_t attr = HWIATTR_COMMON_INITIALIZER;
		ASSERT(hwi_off != HWI_NULL_OFF);
		HWIATTR_SET_LOCATION(&attr, MX6X_SSI2_BASE, MX6X_SSI_SIZE, 0, hwi_find_as(MX6X_SSI2_BASE, 1));
		hwitag_add_common(hwi_off, &attr);
	}
	/* add the Synchronous Serial Interface (SSI) 3 device */
	{
		unsigned hwi_off = hwidev_add(IMX6_HWI_SSI, hwi_devclass_NONE, HWI_NULL_OFF);
		hwiattr_common_t attr = HWIATTR_COMMON_INITIALIZER;
		ASSERT(hwi_off != HWI_NULL_OFF);
		HWIATTR_SET_LOCATION(&attr, MX6X_SSI3_BASE, MX6X_SSI_SIZE, 0, hwi_find_as(MX6X_SSI3_BASE, 1));
		hwitag_add_common(hwi_off, &attr);
	}
}

static unsigned imx6x_hwibus_add_can(unsigned parent_hwi_off, hwiattr_can_t *attr)
{
	unsigned hwi_off = hwibus_add(HWI_ITEM_BUS_CAN, parent_hwi_off);
	if ((hwi_off != HWI_NULL_OFF) && (attr != NULL))
	{
		unsigned i;
		hwitag_add_common(hwi_off, &attr->common);
		for (i=0; i<attr->num_clks; i++)
			hwitag_add_inputclk(hwi_off, 0, 1);
	}
	return hwi_off;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/sabrelite/hwi_imx6q_ddr3.c $ $Rev: 760385 $")
#endif
