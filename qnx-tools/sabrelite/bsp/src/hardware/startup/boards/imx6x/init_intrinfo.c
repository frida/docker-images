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
 * i.MX6x General Interrupt Controller support.
 */

#include "startup.h"
#include <arm/mx6x.h>
#include <arm/mpcore.h>

extern struct callout_rtn	interrupt_id_mx6x_gpio_low;
extern struct callout_rtn	interrupt_eoi_mx6x_gpio_low;
extern struct callout_rtn	interrupt_mask_mx6x_gpio_low;
extern struct callout_rtn	interrupt_unmask_mx6x_gpio_low;

extern struct callout_rtn	interrupt_id_mx6x_gpio_high;
extern struct callout_rtn	interrupt_eoi_mx6x_gpio_high;
extern struct callout_rtn	interrupt_mask_mx6x_gpio_high;
extern struct callout_rtn	interrupt_unmask_mx6x_gpio_high;


static paddr_t mx6x_gpio1_base = MX6X_GPIO1_BASE;
static paddr_t mx6x_gpio2_base = MX6X_GPIO2_BASE;
static paddr_t mx6x_gpio3_base = MX6X_GPIO3_BASE;
static paddr_t mx6x_gpio4_base = MX6X_GPIO4_BASE;
static paddr_t mx6x_gpio5_base = MX6X_GPIO5_BASE;
static paddr_t mx6x_gpio6_base = MX6X_GPIO6_BASE;
static paddr_t mx6x_gpio7_base = MX6X_GPIO7_BASE;

const static struct startup_intrinfo	intrs[] = {

	/* ARM General Interrupt Controller */
	{	.vector_base     = _NTO_INTR_CLASS_EXTERNAL,
		.num_vectors     = 160,
		.cascade_vector  = _NTO_INTR_SPARE,			
		.cpu_intr_base   = 0,						
		.cpu_intr_stride = 0,						
		.flags           = 0,						
        .id = { INTR_GENFLAG_LOAD_SYSPAGE,	0, &interrupt_id_gic },
        .eoi = { INTR_GENFLAG_LOAD_SYSPAGE | INTR_GENFLAG_LOAD_INTRMASK, 0, &interrupt_eoi_gic },
		.mask            = &interrupt_mask_gic,
		.unmask          = &interrupt_unmask_gic,
		.config          = &interrupt_config_gic,
		.patch_data      = &mpcore_scu_base,
	},

	/* GPIO 1 low interrupts (160 -175) */
	{	160,					// vector base
		16,					// number of vectors
		98,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_low },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_low },
		&interrupt_mask_mx6x_gpio_low,		// mask   callout
		&interrupt_unmask_mx6x_gpio_low,	// unmask callout
		0,					// config callout
		&mx6x_gpio1_base,
	},
	/* GPIO 1 high interrupts (176 -191) */
	{	176,					// vector base
		16,					// number of vectors
		99,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_high },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_high },
		&interrupt_mask_mx6x_gpio_high,		// mask   callout
		&interrupt_unmask_mx6x_gpio_high,	// unmask callout
		0,					// config callout
		&mx6x_gpio1_base,
	},
	// GPIO 2 low interrupts (192 -207)
	{	192,					// vector base
		16,					// number of vectors
		100,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_low },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_low },
		&interrupt_mask_mx6x_gpio_low,		// mask   callout
		&interrupt_unmask_mx6x_gpio_low,	// unmask callout
		0,					// config callout
		&mx6x_gpio2_base,
	},
	// GPIO 2 high interrupts (208 -223)
	{	208,					// vector base
		16,					// number of vectors
		101,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_high },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_high },
		&interrupt_mask_mx6x_gpio_high,		// mask   callout
		&interrupt_unmask_mx6x_gpio_high,	// unmask callout
		0,					// config callout
		&mx6x_gpio2_base,
	},
	// GPIO 3 low interrupts (224 -239)
	{	224,					// vector base
		16,					// number of vectors
		102,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_low },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_low },
		&interrupt_mask_mx6x_gpio_low,		// mask   callout
		&interrupt_unmask_mx6x_gpio_low,	// unmask callout
		0,					// config callout
		&mx6x_gpio3_base,
	},
	// GPIO 3 high interrupts (240 -255)
	{	240,					// vector base
		16,					// number of vectors
		103,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_high },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_high },
		&interrupt_mask_mx6x_gpio_high,		// mask   callout
		&interrupt_unmask_mx6x_gpio_high,	// unmask callout
		0,					// config callout
		&mx6x_gpio3_base,
	},
	// GPIO 4 low interrupts (256 -271)
	{	256,					// vector base
		16,					// number of vectors
		104,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_low },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_low },
		&interrupt_mask_mx6x_gpio_low,		// mask   callout
		&interrupt_unmask_mx6x_gpio_low,	// unmask callout
		0,					// config callout
		&mx6x_gpio4_base,
	},
	// GPIO 4 high interrupts (272 -287)
	{	272,					// vector base
		16,					// number of vectors
		105,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_high },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_high },
		&interrupt_mask_mx6x_gpio_high,		// mask   callout
		&interrupt_unmask_mx6x_gpio_high,	// unmask callout
		0,					// config callout
		&mx6x_gpio4_base,
	},
	// GPIO 5 low interrupts (288 -303)
	{	288,					// vector base
		16,					// number of vectors
		106,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_low },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_low },
		&interrupt_mask_mx6x_gpio_low,		// mask   callout
		&interrupt_unmask_mx6x_gpio_low,	// unmask callout
		0,					// config callout
		&mx6x_gpio5_base,
	},
	// GPIO 5 high interrupts (304 -319)
	{	304,					// vector base
		16,					// number of vectors
		107,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_high },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_high },
		&interrupt_mask_mx6x_gpio_high,		// mask   callout
		&interrupt_unmask_mx6x_gpio_high,	// unmask callout
		0,					// config callout
		&mx6x_gpio5_base,
	},
	// GPIO 6 low interrupts (320 -335)
	{	320,					// vector base
		16,					// number of vectors
		108,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_low },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_low },
		&interrupt_mask_mx6x_gpio_low,		// mask   callout
		&interrupt_unmask_mx6x_gpio_low,	// unmask callout
		0,					// config callout
		&mx6x_gpio6_base,
	},
	// GPIO 6 high interrupts (336 -351)
	{	336,					// vector base
		16,					// number of vectors
		109,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_high },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_high },
		&interrupt_mask_mx6x_gpio_high,		// mask   callout
		&interrupt_unmask_mx6x_gpio_high,	// unmask callout
		0,					// config callout
		&mx6x_gpio6_base,
	},
	// GPIO 7 low interrupts (352 -367)
	{	352,					// vector base
		16,					// number of vectors
		110,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_low },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_low },
		&interrupt_mask_mx6x_gpio_low,		// mask   callout
		&interrupt_unmask_mx6x_gpio_low,	// unmask callout
		0,					// config callout
		&mx6x_gpio7_base,
	},
	// GPIO 7 high interrupt (368 - 383)
	{	368,					// vector base
		16,					// number of vectors
		111,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_high },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_high },
		&interrupt_mask_mx6x_gpio_high,		// mask   callout
		&interrupt_unmask_mx6x_gpio_high,	// unmask callout
		0,					// config callout
		&mx6x_gpio7_base,
	},
};

void init_intrinfo()
{
	/* mpcore_scu_base is set in function armv_detect_a9 */
	unsigned	gic_dist = mpcore_scu_base + MPCORE_GIC_DIST_BASE;
	unsigned	gic_cpu  = mpcore_scu_base + MPCORE_GIC_CPU_BASE;

	/*
	 * Initialise GIC distributor and our cpu interface
	 */
	arm_gic_init(gic_dist, gic_cpu);

	add_interrupt_array(intrs, sizeof(intrs));
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/init_intrinfo.c $ $Rev: 729057 $")
#endif
