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


#include "startup.h"
#include "arm/mpcore.h"
#include <arm/mx6x.h>

/*
 * Board-specific cold boot code
 */
extern void	smp_start_mx6x(void);
unsigned	startup_smp_start;
unsigned	startup_reset_vec;
unsigned	startup_reset_vec_addr;

unsigned
board_smp_num_cpu()
{
	unsigned	num;

	num = in32(mpcore_scu_base + MPCORE_SCU_CONFIG);
	if (debug_flag) {
		kprintf("SCU_CONFIG = %x, %d cpus\n", num, (num & 0xf) + 1);
	}
	return (num & 0xf) + 1;
}

void
board_smp_init(struct smp_entry *smp, unsigned num_cpus)
{
	smp->send_ipi = (void *)&sendipi_gic;
	/*
	 * Use smp_spin_pl310 instead of the default smp_spin routine
	 */
	smp_spin_vaddr = (void (*)(void))&smp_spin_pl310;
	callout_register_data(&smp_spin_vaddr, (void *)0x00A02000);
}

int
board_smp_start(unsigned cpu, void (*start)(void))
{
	unsigned	rval;

	if (debug_flag > 1) {
		kprintf("board_smp_start: cpu%d -> %x\n", cpu, start);
	}
	
	/*
	 * Set reset vector entry point to cold boot code required before
	 * we can execute the real startup SMP initialisation code.
	 */
	startup_smp_start = (unsigned)start;
	startup_reset_vec = in32(MX6X_SRC_BASE + MX6X_SRC_GPR1 + (4 * 2 * cpu));
	startup_reset_vec_addr = MX6X_SRC_BASE + MX6X_SRC_GPR1 + (4 * 2 * cpu);
	out32(MX6X_SRC_BASE + MX6X_SRC_GPR1 + (4 * 2 * cpu), (unsigned)smp_start_mx6x);
	out32(MX6X_SRC_BASE + MX6X_SRC_GPR1 + (4 * 2 * cpu) + 4, 0);

	/*
	 * reset cpu
	 */
	rval = in32(MX6X_SRC_BASE + MX6X_SRC_SCR);
	rval |= 1 << (BP_SRC_SCR_CORE0_RST + cpu);
	rval |= 1 << (BP_SRC_SCR_CORES_DBG_RST + cpu);
	out32(MX6X_SRC_BASE + MX6X_SRC_SCR, rval);

	/* Bring cpu out of loop.  The cpu will now execute the smp_start_mx6x routine */
	out32(MX6X_SRC_BASE + MX6X_SRC_GPR1 + (4 * 2 * cpu) + 4, 0);

	return 1;
}

unsigned
board_smp_adjust_num(unsigned cpu)
{
	return cpu;
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/board_smp.c $ $Rev: 729057 $")
#endif
