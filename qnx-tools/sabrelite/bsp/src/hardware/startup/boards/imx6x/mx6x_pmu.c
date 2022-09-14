/*
 * $QNXLicenseC: 
 * Copyright 2013, QNX Software Systems.  
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
#include "arm/mx6x.h"
#include "mx6x_startup.h"

#define PMU_REGCORE_REG2_TARG_MASK				(0x1f << 18)
#define PMU_REGCORE_REG2_TARG_OFFSET			(18)
#define PMU_REGCORE_REG1_TARG_MASK				(0x1f << 9)
#define PMU_REGCORE_REG1_TARG_OFFSET			(9)
#define PMU_REGCORE_REG0_TARG_MASK				(0x1f)
#define PMU_REGCORE_REG0_TARG_OFFSET			(0)

#define PMU_MISC2_REG2_STEP_TIME_OFFSET			(28)
#define PMU_MISC2_REG2_STEP_TIME_MASK			(0x3 << 28)
#define PMU_MISC2_REG1_STEP_TIME_OFFSET			(26)
#define PMU_MISC2_REG1_STEP_TIME_MASK			(0x3 << 26)
#define PMU_MISC2_REG0_STEP_TIME_OFFSET			(24)
#define PMU_MISC2_REG0_STEP_TIME_MASK			(0x3 << 24)
#define PMU_MISC2_REG1_ENABLE_BO				(0x1 << 13)

#define CPU_MIN_VOLTAGE_MICROV 					725000		// 0.725 V
#define CPU_MAX_VOLTAGE_MICROV					1450000		// 1.450 V
#define CPU_CORE_VOLTAGE_STEP_MICROV			25000		// 0.025 V
#define CPU_CORE_VOLTAGE_NO_REG_VAL				0x1f		// 11111 in REG0_TRIG field of PMU_REG_CORE
#define CPU_CORE_VOLTAGE_PWR_GATED_OFF_VAL 		0x0			// 00000 in REG0_TRIG field of PMU_REG_CORE
#define CPU_NO_REGULATION_VOLTAGE				0xFFFFFF

/*
 * Values taken from:
 * http://git.freescale.com/git/cgit.cgi/imx/linux-2.6-imx.git/tree/arch/arm/mach-mx6/cpu_op-mx6.c?h=imx_3.0.35_1.1.0
 * One change has been made, we put the PU regulator at 1.20V for the Solo/Dual Lite instead of the Linux code
 * which uses 1.175V, since 1.175V was not stable.
 *
 * More changes to the default LDO voltages may be needed.
 */

#define CPU_SDL_RECOMMENDED_ARM_VOLTAGE_MICROV	1175000		// 1.175 V
#define CPU_SDL_RECOMMENDED_PU_VOLTAGE_MICROV	1200000		// 1.200 V
#define CPU_SDL_RECOMMENDED_SOC_VOLTAGE_MICROV	1175000		// 1.175 V

#define CPU_DQ_RECOMMENDED_ARM_VOLTAGE_MICROV	1150000		// 1.150 V
#define CPU_DQ_RECOMMENDED_PU_VOLTAGE_MICROV	1250000		// 1.250 V
#define CPU_DQ_RECOMMENDED_SOC_VOLTAGE_MICROV	1150000		// 1.250 V

#define LDO_VOLTAGE_POWEROFF					0x0
#define LDO_VOLTAGE_BYPASS						0x1f

#define RAMP_UNIT	(64)	// 64 cycles (24MHz clock) required for each voltage step

#define LDO_DEBUG_LEVEL	1


void enable_gpu_clocks()
{
	uint32_t reg;

	reg = in32(MX6X_CCM_BASE + MX6X_CCM_CCGR1);
	reg |= (CCGR1_CG13_GPU3D | CCGR1_CG12_GPU2D); 
	out32(MX6X_CCM_BASE + MX6X_CCM_CCGR1, reg);

	reg = in32(MX6X_CCM_BASE + MX6X_CCM_CCGR3);
	reg |= (CCGR3_CG15_OPENVG_AXI);
	out32(MX6X_CCM_BASE + MX6X_CCM_CCGR3, reg);
}

void disable_gpu_clocks()
{
	uint32_t reg;

	reg = in32(MX6X_CCM_BASE + MX6X_CCM_CCGR1);
	reg &= ~(CCGR1_CG13_GPU3D | CCGR1_CG12_GPU2D); 
	out32(MX6X_CCM_BASE + MX6X_CCM_CCGR1, reg);

	reg = in32(MX6X_CCM_BASE + MX6X_CCM_CCGR3);
	reg &= ~(CCGR3_CG15_OPENVG_AXI);
	out32(MX6X_CCM_BASE + MX6X_CCM_CCGR3, reg);
}

/*
 * name:         get_voltage
 * description:  Get voltage in micro volts
 * return value: Voltage in micro volts
 */
uint32_t pmu_get_voltage(uint32_t ldo_num)
{
	uint32_t volt_reg_val, ldo_offset, ldo_mask;

	/*
	 * We only support changing the LDO voltages for now
	 */
	switch (ldo_num)
	{
		case LDO_ARM:	// LDO_ARM = REG0
			ldo_offset = PMU_REGCORE_REG0_TARG_OFFSET;
			ldo_mask = PMU_REGCORE_REG0_TARG_MASK;
			break;
		case LDO_PU:	// LDO_PU = REG1
			ldo_offset = PMU_REGCORE_REG1_TARG_OFFSET;
			ldo_mask = PMU_REGCORE_REG1_TARG_MASK;
			break;
		case LDO_SOC:	// LDO_SOC = REG2
			ldo_offset = PMU_REGCORE_REG2_TARG_OFFSET;
			ldo_mask = PMU_REGCORE_REG2_TARG_MASK;
			break;
		default:
			kprintf("%s: Unable to set LDO voltage - unknown LDO", __FUNCTION__);
			return EXIT_FAILURE;
	}

	volt_reg_val = in32(MX6X_ANATOP_BASE + MX6X_ANADIG_REG_CORE) & ldo_mask;
	volt_reg_val >>= ldo_offset;

	switch (volt_reg_val)
	{
        case CPU_CORE_VOLTAGE_NO_REG_VAL:
            return CPU_NO_REGULATION_VOLTAGE;
        case CPU_CORE_VOLTAGE_PWR_GATED_OFF_VAL:
            return CPU_POWER_GATED_OFF_VOLTAGE;
        default:
            return CPU_MIN_VOLTAGE_MICROV + CPU_CORE_VOLTAGE_STEP_MICROV * (volt_reg_val - 1);
	}
}

/*
 * name:         set_voltage
 * description:  Set the voltage of a specified LDO in micro volts
 * return value: Return value indicates success or failure
 */
uint32_t pmu_set_voltage(uint32_t ldo_num, uint32_t req_voltage)
{
	uint32_t volt_reg_val, regVal;
	uint32_t misc2_reg, ldo_offset, ldo_mask;
	uint32_t ramping_delay, num_of_steps, curr_cpu_voltage, step_period;

	/*
	 * We only support changing the LDO voltages for now
	 */
	switch (ldo_num)
	{
		case LDO_ARM:	// LDO_ARM = REG0
			ldo_offset = PMU_REGCORE_REG0_TARG_OFFSET;
			ldo_mask = PMU_REGCORE_REG0_TARG_MASK;
			break;
		case LDO_PU:	// LDO_PU = REG1
			ldo_offset = PMU_REGCORE_REG1_TARG_OFFSET;
			ldo_mask = PMU_REGCORE_REG1_TARG_MASK;
			break;
		case LDO_SOC:	// LDO_SOC = REG2
			ldo_offset = PMU_REGCORE_REG2_TARG_OFFSET;
			ldo_mask = PMU_REGCORE_REG2_TARG_MASK;
			break;
		default:
			kprintf("%s: Unable to set LDO voltage - unknown LDO", __FUNCTION__);
			return EXIT_FAILURE;
	}

	// Get current CPU voltage
	curr_cpu_voltage = pmu_get_voltage(ldo_num);

	// If voltage is not changing then return
	if (req_voltage == curr_cpu_voltage)
		return EXIT_SUCCESS;

	// TODO - add support for Internal Bypass mode which can be used for boards w/ external PMIC chips
	if (req_voltage < CPU_MIN_VOLTAGE_MICROV || req_voltage > CPU_MAX_VOLTAGE_MICROV)
	{
		kprintf("%s: Voltage value out of range, unable to set voltage.", __FUNCTION__);
		return EXIT_FAILURE;
	}

	/*
	 * As per Freescale's recommendation, set voltage step time to default values, as Boot ROM
	 * may have overriden default values (all 0s corresponding to step time = 64 clocks)
	 * based on fuse settings.
	 */
	misc2_reg = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_MISC2);
	misc2_reg &= ~(PMU_MISC2_REG0_STEP_TIME_MASK | PMU_MISC2_REG1_STEP_TIME_MASK |
					PMU_MISC2_REG2_STEP_TIME_MASK);
	out32(MX6X_ANATOP_BASE + MX6X_ANATOP_MISC2, misc2_reg);

	// Calculate register value
	volt_reg_val = ((req_voltage - CPU_MIN_VOLTAGE_MICROV)/CPU_CORE_VOLTAGE_STEP_MICROV) + 1;

	// Write calculated value into register
	regVal = in32(MX6X_ANATOP_BASE + MX6X_ANADIG_REG_CORE) & ~ldo_mask;
	regVal |= (volt_reg_val << ldo_offset);
	out32(MX6X_ANATOP_BASE + MX6X_ANADIG_REG_CORE, regVal);

	/*
	 * If voltage is increasing, we need to delay for a specific duration of time to allow the voltage
	 * to stabilize. But if voltage is decreasing then there is no need to delay.
	 */
	if (req_voltage > curr_cpu_voltage)
	{
		/*
		 * Convert step period from register bit value to actual number of clocks required for each
		 * step (0x0 = 64 clocks, 0x1 = 128 clocks, 0x2 = 256 clocks, 0x3 = 512 clocks)
		 *
		 * Note that Freescale recommends using the default value of 0x0 which is 64 clocks.
		 */
		num_of_steps = (req_voltage - curr_cpu_voltage) / 25000;
		step_period = RAMP_UNIT;

		/* 
		 * delay (in us) = number of voltage steps * ramp up time (in cycles) / 24 Mcycles/s
		 * Fractional component after a division gets discarded we add 1 to always round up.
		 */
		ramping_delay = ((num_of_steps * step_period) / 24) + 1;
		mx6x_usleep(ramping_delay);

		if (debug_flag > LDO_DEBUG_LEVEL)
		{
			kprintf("%s: delaying for (at least) %d useconds after increasing voltage from %d -> %d",
			__FUNCTION__, ramping_delay, curr_cpu_voltage, req_voltage);
		}
	}
	return EXIT_SUCCESS;
}

/*
 * name:         pmu_set_standard_ldo_voltages
 * description:  Set the recommended voltages for LDOs. The recommended voltages are based on the values
 *               used by Linux, however the PU voltage was unstable and it has been increased.
 * return value: Return value indicates success or failure
 */
uint32_t pmu_set_standard_ldo_voltages()
{
	uint32_t chip_type = get_mx6_chip_type();
	switch (chip_type)
	{
		case MX6_CHIP_TYPE_QUAD_OR_DUAL:
			pmu_set_voltage(LDO_ARM, CPU_DQ_RECOMMENDED_ARM_VOLTAGE_MICROV);
			pmu_set_voltage(LDO_PU, CPU_DQ_RECOMMENDED_PU_VOLTAGE_MICROV);
			pmu_set_voltage(LDO_SOC, CPU_DQ_RECOMMENDED_SOC_VOLTAGE_MICROV);
			break;
		case MX6_CHIP_TYPE_DUAL_LITE_OR_SOLO:
			pmu_set_voltage(LDO_ARM, CPU_SDL_RECOMMENDED_ARM_VOLTAGE_MICROV);
			pmu_set_voltage(LDO_PU, CPU_SDL_RECOMMENDED_PU_VOLTAGE_MICROV);
			pmu_set_voltage(LDO_SOC, CPU_SDL_RECOMMENDED_SOC_VOLTAGE_MICROV);
			break;
		default:
			kprintf("%s: Unkown chip detected, unable to set LDO voltages\n", __FUNCTION__);
			return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/*
 * name:         pmu_power_up_gpu
 * description:  Power up the GPU, if the bootloader disables the GPU this function should be called in startup.
 *               Note that the LDO_PU voltage will be set to the same voltage level as LDO_SOC
 * return value: Return value indicates success or failure
 */
uint32_t pmu_power_up_gpu()
{
	uint32_t reg, pu_voltage, soc_voltage;

	disable_gpu_clocks();

	// Set GPU voltage to SOC voltage as per Linux code.
	soc_voltage = pmu_get_voltage(LDO_SOC);
	if (pmu_set_voltage(LDO_PU, soc_voltage) != EXIT_SUCCESS) {
		kprintf("%s: Unable to set LDO_PU voltage");
		return EXIT_FAILURE;
	}

	enable_gpu_clocks();

	// Enable power up request
	reg = in32(MX6X_GPC_BASE + MX6X_GPC_GPU_CTRL);
	reg |= 0x1;
	out32(MX6X_GPC_BASE + MX6X_GPC_GPU_CTRL, reg);

	// Power up request
	reg = in32(MX6X_GPC_BASE + MX6X_GPC_CNTR);
	reg |= 0x2;
	out32(MX6X_GPC_BASE + MX6X_GPC_CNTR, reg);

	// Wait for the power up bit to clear
	while (in32(MX6X_GPC_BASE + MX6X_GPC_CNTR) & 0x2);

	// Enable the Brown Out detection.
	reg = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_MISC2);
	reg |= PMU_MISC2_REG1_ENABLE_BO;
	out32(MX6X_ANATOP_BASE + MX6X_ANATOP_MISC2, reg);

	pu_voltage = in32(MX6X_ANATOP_BASE + MX6X_ANADIG_REG_CORE);
	pu_voltage &= PMU_REGCORE_REG1_TARG_MASK;
	pu_voltage >>= PMU_REGCORE_REG1_TARG_OFFSET;

	if (pu_voltage != LDO_VOLTAGE_BYPASS)
	{
		// Unmask ANATOP brown out interrupt in GPC
		reg = in32(MX6X_GPC_BASE + MX6X_GPC_IMR4);
		reg &= ~0x80000000;
		out32(MX6X_GPC_BASE + MX6X_GPC_IMR4, reg);
	}

	return EXIT_SUCCESS;
}

/*
 * name:         pmu_power_down_gpu
 * description:  Power down the GPU
 * return value: Return value indicates success or failure
 */
uint32_t pmu_power_down_gpu()
{
	uint32_t reg, pu_voltage;

	disable_gpu_clocks();

	// Disable the Brown Out detection.
	reg = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_MISC2);
	reg &= ~PMU_MISC2_REG1_ENABLE_BO;
	out32(MX6X_ANATOP_BASE + MX6X_ANATOP_MISC2, reg);

	// Switch off power when pdn_req is asserted
	reg = in32(MX6X_GPC_BASE + MX6X_GPC_GPU_CTRL);
	reg |= 0x1;
	out32(MX6X_GPC_BASE + MX6X_GPC_GPU_CTRL, reg);

	// Power down request
	reg = in32(MX6X_GPC_BASE + MX6X_GPC_CNTR);
	reg |= 0x1;
	out32(MX6X_GPC_BASE + MX6X_GPC_CNTR, reg);

	// Wait for the power down bit to clear
	while (in32(MX6X_GPC_BASE + MX6X_GPC_CNTR) & 0x1);

	// Mask ANATOP brownout interrupt
	reg = in32(MX6X_GPC_BASE + MX6X_GPC_IMR4);
	reg |= 0x80000000;
	out32(MX6X_GPC_BASE + MX6X_GPC_IMR4, reg);

	// Power down LDO_PU (responsible for VPU/GPU power domain)
	pu_voltage = in32(MX6X_ANATOP_BASE + MX6X_ANADIG_REG_CORE);
	pu_voltage &= ~PMU_REGCORE_REG1_TARG_MASK;
	out32(MX6X_ANATOP_BASE + MX6X_ANADIG_REG_CORE, pu_voltage);

	// Clear the B0 interrupt
	reg = in32(MX6X_ANATOP_BASE + MX6X_ANADIG_MISC1);
	reg |= 0x80000000;
	out32(MX6X_ANATOP_BASE + MX6X_ANADIG_MISC1, reg);

	return EXIT_SUCCESS;
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/mx6x_pmu.c $ $Rev: 752267 $")
#endif
