/*
 * $QNXLicenseC: 
 * Copyright 2012, 2013 QNX Software Systems.  
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
#include <arm/mx6x.h>
#include "board.h"
#include "mx6x_startup.h"

/*
 * Note that the '594M PFD' is PLL2's PFD1 which is by default 594MHz for Dual/Quad chips, and
 * 528 MHz for Solo/DualLite chips!
 */
#define MX6X_CCM_CBCMR_GPU3D_SHADER_SEL_594M_PFD	(0x2 << MX6X_CCM_CBCMR_GPU3D_SHADER_CLK_SEL_OFFSET)
#define MX6X_CCM_CBCMR_GPU2D_CORE_SEL_PLL3			(0x1 << MX6X_CCM_CBCMR_GPU2D_CLK_SEL_OFFSET)
#define MX6X_CCM_CBCMR_GPU3D_CORE_SEL_MMDC_CH0		(0x0 << MX6X_CCM_CBCMR_GPU3D_CORE_CLK_SEL_OFFSET)
#define MX6X_CCM_CBCMR_GPU3D_CORE_SEL_594M_PFD		(0x2 << MX6X_CCM_CBCMR_GPU3D_CORE_CLK_SEL_OFFSET)

#define IPU2_HSP_CLK_SEL_MMDC_CH0					(0x0 << MX6X_CCM_CSCDR3_IPU2_HSP_CLK_SEL_OFFSET)
#define IPU2_HSP_CLK_DIV2							(0x1 << MX6X_CCM_CSCDR3_IPU2_HSP_PODF_OFFSET)
#define IPU1_HSP_CLK_SEL_MMDC_CH0					(0x0 << MX6X_CCM_CSCDR3_IPU1_HSP_CLK_SEL_OFFSET)
#define IPU1_HSP_CLK_SEL_540MPFD					(0x3 << MX6X_CCM_CSCDR3_IPU1_HSP_CLK_SEL_OFFSET)
#define IPU1_HSP_CLK_DIV2							(0x1 << MX6X_CCM_CSCDR3_IPU1_HSP_PODF_OFFSET)

/* Max PLL lock time doesn't appear to be documented so use delay time used by Linux reference code. */
#define MAX_PLL5_LOCK_TIME_IN_US 1200

/*
 * Gate clocks to unused components.
 */
void mx6x_init_clocks()
{
	unsigned ccgr2;

	out32(MX6X_CCM_BASE + MX6X_CCM_CCGR0, MX6X_CCM_CCGR0_RESET
#ifdef MX6X_DISABLE_CLOCK_CCGR0
		  & ~( MX6X_DISABLE_CLOCK_CCGR0 )
#endif /* MX6X_DISABLE_CLOCK_CCGR0 */
		  );

	out32(MX6X_CCM_BASE + MX6X_CCM_CCGR1, MX6X_CCM_CCGR1_RESET
#ifdef MX6X_DISABLE_CLOCK_CCGR1
		  & ~( MX6X_DISABLE_CLOCK_CCGR1 )
#endif /* MX6X_DISABLE_CLOCK_CCGR1 */
		  );

	/* Preserve TZASC bits if set by bootloader */
	ccgr2 = in32(MX6X_CCM_BASE + MX6X_CCM_CCGR2);
	if ((ccgr2 & CCGR2_CG11_IPSYNC) || (ccgr2 & CCGR2_CG12_IPSYNC))
		ccgr2 |= MX6X_CCM_CCGR2_RESET;
	else
		ccgr2 = MX6X_CCM_CCGR2_RESET;

	out32(MX6X_CCM_BASE + MX6X_CCM_CCGR2, ccgr2 
#ifdef MX6X_DISABLE_CLOCK_CCGR2
		  & ~( MX6X_DISABLE_CLOCK_CCGR2 )
#endif /* MX6X_DISABLE_CLOCK_CCGR2 */
		  );

	out32(MX6X_CCM_BASE + MX6X_CCM_CCGR3, MX6X_CCM_CCGR3_RESET
#ifdef MX6X_DISABLE_CLOCK_CCGR3
		  & ~( MX6X_DISABLE_CLOCK_CCGR3 )
#endif /* MX6X_DISABLE_CLOCK_CCGR3 */
		  );


	out32(MX6X_CCM_BASE + MX6X_CCM_CCGR4, MX6X_CCM_CCGR4_RESET
#ifdef MX6X_DISABLE_CLOCK_CCGR4
		  & ~( MX6X_DISABLE_CLOCK_CCGR4 )
#endif /* MX6X_DISABLE_CLOCK_CCGR4 */		  
		  );
	
	out32(MX6X_CCM_BASE + MX6X_CCM_CCGR5, MX6X_CCM_CCGR5_RESET
#ifdef MX6X_DISABLE_CLOCK_CCGR5
		  & ~( MX6X_DISABLE_CLOCK_CCGR5 )
#endif /* MX6X_DISABLE_CLOCK_CCGR5 */
		  );
	
	out32(MX6X_CCM_BASE + MX6X_CCM_CCGR6, MX6X_CCM_CCGR6_RESET
#ifdef MX6X_DISABLE_CLOCK_CCGR6
		  & ~( MX6X_DISABLE_CLOCK_CCGR6 )
#endif /* MX6X_DISABLE_CLOCK_CCGR6 */		  
		  );

	out32(MX6X_CCM_BASE + MX6X_CCM_CCGR7, MX6X_CCM_CCGR7_RESET
#ifdef MX6X_DISABLE_CLOCK_CCGR7
		  & ~( MX6X_DISABLE_CLOCK_CCGR7 )
#endif /* MX6X_DISABLE_CLOCK_CCGR7 */		  
		  );
}

static void pll5_disable(void)
{
	unsigned val;

	val = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL5_VIDEO);
	val |= ANATOP_PLL5_BYPASS;
	val &= ~ANATOP_PLL5_ENABLE;
	out32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL5_VIDEO, val);
}

/*
 * For proper LVDS operation, PLL5 (Video PLL) should be the clock source for the LDB module
 * Code below follows procedure described in "i.MX6 Clock Switching Procedure.pdf"
 */
unsigned mx6x_init_lvds_clock(void)
{

	unsigned ccgr3, ccm_cs2cdr, ldb_di0_clk, ldb_di1_clk, val;

	ccm_cs2cdr = in32(MX6X_CCM_BASE + MX6X_CCM_CS2CDR);
	ldb_di0_clk = (ccm_cs2cdr & MX6X_CCM_CS2CDR_LDB_DI0_CLK_SEL_MASK) >> MX6X_CCM_CS2CDR_LDB_DI0_CLK_SEL_OFFSET;
	ldb_di1_clk = (ccm_cs2cdr & MX6X_CCM_CS2CDR_LDB_DI1_CLK_SEL_MASK) >> MX6X_CCM_CS2CDR_LDB_DI1_CLK_SEL_OFFSET;

	/*
	 * If the LDB clock source for display 0 and display 1 is 0x0 (i.e. PLL5) then exit
	 * since the LDB clock source is already set correctly.
	 */
	if ((ldb_di0_clk == 0x0) && (ldb_di1_clk == 0))
	{
		if (debug_flag > 1)
			kprintf("%s: LDB clock is already set to PLL5, no clock change required.", __FUNCTION__);  
		
		return TRUE;
	}
	/*
	 * For Dual/Quad 1.0 silicon PLL5 is not available.
	 */
	if ((get_mx6_chip_rev() == MX6_CHIP_REV_1_0) &&
	    (get_mx6_chip_type() == MX6_CHIP_TYPE_QUAD_OR_DUAL))
	{
		kprintf("%s: i.MX6 Dual/Quad silicon rev 1.0 detected, LDB clock source will not be changed.", __FUNCTION__);
		return FALSE;
	}

	if (debug_flag > 1)
		kprintf("%s: Changing ldb_di0_clk_sel (initial value: 0x%x)\nand ldb_di1_clk_sel (initial value: 0x%x) to 0x0 (PLL5)",
			__FUNCTION__, ldb_di0_clk, ldb_di1_clk);

	/* Disable IPU_DIx LDB_DIx clocks*/
	ccgr3 = in32(MX6X_CCM_BASE + MX6X_CCM_CCGR3);
	ccgr3 &= ~(CCGR3_CG1_IPU1_DI0 | CCGR3_CG2_IPU1_DI1 | CCGR3_CG6_LDB_DI0 | CCGR3_CG7_LDB_DI1);
	out32(MX6X_CCM_BASE + MX6X_CCM_CCGR3, ccgr3);

	pll5_disable();

	/* Set periph2_clk2 clock parent to pll3_sw_clk to make MMDC_CH1 clock parent pll3_sw_clk */
	val = in32(MX6X_CCM_BASE + MX6X_CCM_CBCMR);
	val &= ~(1 << 20);
	out32(MX6X_CCM_BASE + MX6X_CCM_CBCMR, val);

	/* Mask handshake with MMDC CH1 */
	val = in32(MX6X_CCM_BASE + MX6X_CCM_CCDR);
	val |= (1 << 16);
	out32(MX6X_CCM_BASE + MX6X_CCM_CCDR, val);

	/* Set periph2_clk parent to periph_clk2_clk. This causes mmdc_ch1 to be sourced by pll3_sw_clk */
	val = in32(MX6X_CCM_BASE + MX6X_CCM_CBCDR);
	val |= (1 << 26);
	out32(MX6X_CCM_BASE + MX6X_CCM_CBCDR, val);

	/* Wait for clock switch */ 
	while (in32(MX6X_CCM_BASE + MX6X_CCM_CDHIPR))
		;

	/* Disable pll3_sw_clk by setting clock source to pll3 bypass clock */
	val = in32(MX6X_CCM_BASE + MX6X_CCM_CCSR);
	val |= (1 << 0);
	out32(MX6X_CCM_BASE + MX6X_CCM_CCSR, val);

	/* Set ldb_di0_clk and ldb_di1_clk clock source to Restricted value */
	val = in32(MX6X_CCM_BASE + MX6X_CCM_CS2CDR);
	val |= ((7 << 9) | (7 << 12));
	out32(MX6X_CCM_BASE + MX6X_CCM_CS2CDR, val);

	/* Set ldb_di0_clk and ldb_di1_clk clock source to pll3 clock*/
	val = in32(MX6X_CCM_BASE + MX6X_CCM_CS2CDR);
	val &= ~((7 << 9) | (7 << 12));
	val |= ((4 << 9) | (4 << 12));
	out32(MX6X_CCM_BASE + MX6X_CCM_CS2CDR, val);

	/* Set ldb_di0_clk and ldb_di1_clk clock source to 0 i.e. PLL5 */
	val = in32(MX6X_CCM_BASE + MX6X_CCM_CS2CDR);
	val &= ~((7 << 9) | (7 << 12));
	out32(MX6X_CCM_BASE + MX6X_CCM_CS2CDR, val);

	/* Source pll3_sw_clk from pll3_main_clk instead of bypass clock */
	val = in32(MX6X_CCM_BASE + MX6X_CCM_CCSR);
	val &= ~(1 << 0);
	out32(MX6X_CCM_BASE + MX6X_CCM_CCSR, val);

	/* Set periph2_clk parent to pll2_sw_clk (bottom mux). This causes mmdc_ch1 to be set to it's initial clock source  */
	val = in32(MX6X_CCM_BASE + MX6X_CCM_CBCDR);
	val &= ~(1 << 26);
	out32(MX6X_CCM_BASE + MX6X_CCM_CBCDR, val);

	/* Wait for periph2_clk_sel clock switch */
	while (in32(MX6X_CCM_BASE + MX6X_CCM_CDHIPR))
		;

	/* Reset MMDC_CH1 mask */
	val = in32(MX6X_CCM_BASE + MX6X_CCM_CCDR);
	val &= ~(1 << 16);
	out32(MX6X_CCM_BASE + MX6X_CCM_CCDR, val);

	 /* Leave PLL5 off for now, WFD driver will power up PLL5 and set the frequency */

	return TRUE;
}
/*
 * Init Vivante 3D GPU, 2D GPU (blitter)
 * Solo / DualLite:
 * 3D Shader Clock: 528 MHz
 * 3D Core Clock: 528 MHz
 *
 * Dual / Quad:
 * 3D Shader Clock: 594 MHz
 * 3D Core Clock:   528 MHz
 */

void mx6x_init_gpu3D(void)
{
    int32_t tmp;
    unsigned int regval = 0;

	disable_gpu_clocks();
    
    /*
     * For i.MX6 Dual/Quad chips the section below will set GPU shader parent clock to 594 MHz PFD clock.
     * For i.MX6 Solo/DualLite chips the same register bits configure the GPU2D core clock, therefore
     * for i.MX6 Solo/DualLite chips the section below sets the GPU2D core clock parent to the 528 MHz PFD.
     * Note that the PFD clock frequencies are different between Solo/DualLite and Dual/Quad.
     */
    tmp = in32(MX6X_CCM_BASE + MX6X_CCM_CBCMR) & ~MX6X_CCM_CBCMR_GPU3D_SHADER_CLK_SEL_MASK;
    tmp |= MX6X_CCM_CBCMR_GPU3D_SHADER_SEL_594M_PFD;
    out32(MX6X_CCM_BASE + MX6X_CCM_CBCMR, tmp);
    
    tmp = in32(MX6X_CCM_BASE + MX6X_CCM_CBCMR) & ~MX6X_CCM_CBCMR_GPU3D_SHADER_PODF_MASK;
    out32(MX6X_CCM_BASE + MX6X_CCM_CBCMR, tmp);

	if (get_mx6_chip_type() == MX6_CHIP_TYPE_DUAL_LITE_OR_SOLO)
	{
		/* set GPU3D core parent clock to 594MHz PFD which is actually 528MHz on Solo/DualLite */
		tmp = in32(MX6X_CCM_BASE + MX6X_CCM_CBCMR) & ~MX6X_CCM_CBCMR_GPU3D_CORE_CLK_SEL_MASK;
		tmp |= MX6X_CCM_CBCMR_GPU3D_CORE_SEL_594M_PFD;
		out32(MX6X_CCM_BASE + MX6X_CCM_CBCMR, tmp);

		// set GPU3D core clock divisor to 1 (bit value 000)
		tmp = in32(MX6X_CCM_BASE + MX6X_CCM_CBCMR) & ~MX6X_CCM_CBCMR_GPU3D_CORE_PODF_MASK;
		out32(MX6X_CCM_BASE + MX6X_CCM_CBCMR, tmp);

		// Note that on Solo/DualLite 2D core clock source is GPU 3D shader clock and is not configurable.

		// GPU3D axi source from mmdc0
		tmp = in32(MX6X_CCM_BASE + MX6X_CCM_CBCMR) & ~MX6X_CCM_CBCMR_GPU3D_AXI_CLK_SEL_MASK;
		out32(MX6X_CCM_BASE + MX6X_CCM_CBCMR, tmp);

		// GPU2D axi source from mmdc0
		tmp = in32(MX6X_CCM_BASE + MX6X_CCM_CBCMR) & ~MX6X_CCM_CBCMR_GPU2D_AXI_CLK_SEL_MASK;
		out32(MX6X_CCM_BASE + MX6X_CCM_CBCMR, tmp);

		// Source IPU clock (ipu1_hsp) from 540M PFD and scale by 2 producing a 270MHz IPU clock 
		tmp = in32(MX6X_CCM_BASE + MX6X_CCM_CSCDR3);
		tmp &= ~(MX6X_CCM_CSCDR3_IPU1_HSP_CLK_SEL_MASK | MX6X_CCM_CSCDR3_IPU1_HSP_PODF_MASK);
		tmp |= (IPU1_HSP_CLK_SEL_540MPFD | IPU1_HSP_CLK_DIV2);
		out32(MX6X_CCM_BASE + MX6X_CCM_CSCDR3, tmp);

	}
	else
	{
		// Set GPU3D core parent clock to multi port DRAM/DDR controller (mmdc).
		tmp = in32(MX6X_CCM_BASE + MX6X_CCM_CBCMR) & ~MX6X_CCM_CBCMR_GPU3D_CORE_CLK_SEL_MASK;
		tmp |= MX6X_CCM_CBCMR_GPU3D_CORE_SEL_MMDC_CH0;
		out32(MX6X_CCM_BASE + MX6X_CCM_CBCMR, tmp);

		// Set GPU3D core clock divisor to 1 (bit value 000)
		tmp = in32(MX6X_CCM_BASE + MX6X_CCM_CBCMR) & ~MX6X_CCM_CBCMR_GPU3D_CORE_PODF_MASK;
		out32(MX6X_CCM_BASE + MX6X_CCM_CBCMR, tmp);

		// Source IPU clocks (ipu1_hsp, ipu2_hsp) from MMDC_CH0 clock and scale by 2 producing a 264MHz IPU clock 
		tmp = in32(MX6X_CCM_BASE + MX6X_CCM_CSCDR3);
		tmp &= ~(MX6X_CCM_CSCDR3_IPU1_HSP_CLK_SEL_MASK | MX6X_CCM_CSCDR3_IPU1_HSP_PODF_MASK |
				MX6X_CCM_CSCDR3_IPU2_HSP_CLK_SEL_MASK | MX6X_CCM_CSCDR3_IPU2_HSP_PODF_MASK);
		tmp |= (IPU1_HSP_CLK_SEL_MMDC_CH0 | IPU1_HSP_CLK_DIV2 | IPU2_HSP_CLK_SEL_MMDC_CH0 |
				IPU2_HSP_CLK_DIV2);
		out32(MX6X_CCM_BASE + MX6X_CCM_CSCDR3, tmp);

		if (get_mx6_chip_type() > MX6_CHIP_REV_1_1)
		{
			tmp = in32(MX6X_CCM_BASE + MX6X_CCM_CBCMR) & 
				~(MX6X_CCM_CBCMR_GPU2D_CORE_PODF_MASK | MX6X_CCM_CBCMR_GPU2D_CLK_SEL_MASK);
			tmp |= MX6X_CCM_CBCMR_GPU2D_CORE_SEL_PLL3;
			out32(MX6X_CCM_BASE + MX6X_CCM_CBCMR, tmp);
		}
	}

	enable_gpu_clocks();

	/*
	 * This clock initialization is only for the rev1.1 CPU
	 * PLL5 clock initialization
	 */
    if(get_mx6_chip_rev() >=  MX6_CHIP_REV_1_1)
    {
	out32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL5_VIDEO_SET, 0x00012000);

    	// For TO1.1 cpu, select pll5
	out32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL5_VIDEO_NUM, 50000);
	out32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL5_VIDEO_DENOM, 100000);
	out32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL5_VIDEO, 0x00012038);

	regval = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL5_VIDEO);
	while ((!regval) & 0x80000000);

	out32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL5_VIDEO_CLR, 0x00011000);
    }

}

void mx6_init_usdhc_clk(void)
{
	/* Setting the divider for the esdhc interfaces
	 * to default value which is divide by 2
	 */
	out32(MX6X_CCM_BASE + MX6X_CCM_CSCDR1, MX6X_CCM_CSCDR1_USDHC1_PODF_DIV_2 |
			                               MX6X_CCM_CSCDR1_USDHC2_PODF_DIV_2 |
			                               MX6X_CCM_CSCDR1_USDHC3_PODF_DIV_2 |
			                               MX6X_CCM_CSCDR1_USDHC4_PODF_DIV_2);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/mx6x_init_clocks.c $ $Rev: 757305 $")
#endif
