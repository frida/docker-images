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
#include <arm/mx6x.h>
#include "mx6x_startup.h"

#define	ONE_MHZ					1000000

/* Out-of-reset PFDs and clock source definitions */
#define MX6X_HCLK_FREQ			24000000
#define MX6X_PLL3_PFD1_FREQ		540000000
#define MX6X_PLL3_80M			80000000
#define MX6X_PLL3_60M			60000000

#define MX6X_ENET_FREQ_0		25000000
#define MX6X_ENET_FREQ_1		50000000
#define MX6X_ENET_FREQ_2		100000000
#define MX6X_ENET_FREQ_3		125000000

// Round last digit during division
#define SAFE_DIVIDE(A,B)	((A + (B-1))/B)

/* PLL1 - System */
static uint32_t mx6x_pll1_freq()
{
	uint32_t div = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL1_SYS) & 0x7F;
	return MX6X_HCLK_FREQ * (div >> 1);
}

/* PLL2 - System Bus */
static uint32_t mx6x_pll2_freq()
{
	uint32_t div = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL2_SYS_BUS) & 0x1;
	return MX6X_HCLK_FREQ * (20 + (div << 1));
}

/* PLL2, PFD clocks */
static uint32_t mx6x_pll2_pfd_freq(int pfd)
{
	uint32_t	reg = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PFD_528);
	int			div;

	switch (pfd) {
		case 0:
			div = (reg & MX6X_ANATOP_PFD0_MASK) >> MX6X_ANATOP_PFD0_OFFSET;
			break;
		case 1:
			div = (reg & MX6X_ANATOP_PFD1_MASK) >> MX6X_ANATOP_PFD1_OFFSET;
			break;
		case 2:
			div = (reg & MX6X_ANATOP_PFD2_MASK) >> MX6X_ANATOP_PFD2_OFFSET;
			break;
		case 3:
		default:
			div = (reg & MX6X_ANATOP_PFD3_MASK) >> MX6X_ANATOP_PFD3_OFFSET;
			break;
	}

	return (mx6x_pll2_freq() / div * 18);
}

/* PLL3 - USB OTG, PHY0 */
static uint32_t mx6x_pll3_freq()
{
	uint32_t div = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL3_USB1_OTG) & 0x3;
	return MX6X_HCLK_FREQ * (20 + (div << 1));
}

/* PLL4 - Audio */
static uint32_t mx6x_pll4_freq()
{
	uint32_t pll_freq, test_div_sel;
	uint32_t div = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL4_AUDIO) & 0x7f;
	uint32_t num = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL4_AUDIO_NUM);
	uint32_t denom = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL4_AUDIO_DENOM);

	pll_freq = (MX6X_HCLK_FREQ * div) + ((MX6X_HCLK_FREQ/denom)*num);

	/* i.MX6 Dual/Quad rev 1.1, i.MX6 Solo/DualLite have a post divider for the Audio PLLs */
	if ((get_mx6_chip_rev() >=  MX6_CHIP_REV_1_1) ||
	    (get_mx6_chip_type() == MX6_CHIP_TYPE_DUAL_LITE_OR_SOLO))
	{
		test_div_sel = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL4_AUDIO);
		test_div_sel &= ANATOP_PLLAUDIOVIDEO_TEST_DIV_SEL_MASK;
		test_div_sel >>= ANATOP_PLLAUDIOVIDEO_TEST_DIV_SEL_OFFSET;
		
		if (test_div_sel == 0x0)
			pll_freq /= 4;
		else if (test_div_sel == 0x1)
			pll_freq /= 2;
	}
	return pll_freq;
}

/* PLL5 - Video */
static uint32_t mx6x_pll5_freq()
{
	uint32_t test_div_sel, pll_freq, control3; 
	uint32_t div = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL5_VIDEO) & 0x7f;
	uint32_t num = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL5_VIDEO_NUM);
	uint32_t denom = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL5_VIDEO_DENOM);

	pll_freq = (MX6X_HCLK_FREQ * div) + ((MX6X_HCLK_FREQ/denom)*num);
	/* i.MX6 Dual/Quad rev 1.1, i.MX6 Solo/DualLite have a post divider for the Video PLLs */
	if ((get_mx6_chip_rev() >=  MX6_CHIP_REV_1_1) ||
	    (get_mx6_chip_type() == MX6_CHIP_TYPE_DUAL_LITE_OR_SOLO))
	{
		test_div_sel = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL5_VIDEO);
		test_div_sel &= ANATOP_PLLAUDIOVIDEO_TEST_DIV_SEL_MASK;
		test_div_sel >>= ANATOP_PLLAUDIOVIDEO_TEST_DIV_SEL_OFFSET;
		
		if (test_div_sel == 0x0)
			pll_freq /= 4;
		else if (test_div_sel == 0x1)
			pll_freq /= 2;

		control3 = in32(MX6X_ANATOP_BASE + MX6X_ANADIG_MISC2);
		control3 &= ANADIG_MISC2_CONTROL3_MASK;
		control3 >>= ANADIG_MISC2_CONTROL3_OFFSET;

		if (control3 == 0x1)
			pll_freq /= 2;
		else if (control3 == 0x3)
			pll_freq /= 4;

	}
	return pll_freq;
}

/* PLL7 - USB PHY1 */
static uint32_t mx6x_pll7_freq()
{
	uint32_t div = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL7_USB2_HOST) & 0x3;
	return MX6X_HCLK_FREQ * (20 + (div << 1));
}

/* PLL8 - ENET */
static uint32_t mx6x_pll8_freq()
{
	uint32_t  freq = 0;
	uint32_t div = in32(MX6X_ANATOP_BASE + MX6X_ANATOP_PLL8_ENET) & 0x3;
	switch (div) {
		case 0: 
			freq = MX6X_ENET_FREQ_0;
			break;
		case 1: 
			freq = MX6X_ENET_FREQ_1;
			break;
		case 2: 
			freq = MX6X_ENET_FREQ_2;
			break;
		case 3: freq = MX6X_ENET_FREQ_3;
		break;
	}
	return freq;
}

uint32_t mx6x_get_cpu_clk()
{
	uint32_t div = in32(MX6X_CCM_BASE + MX6X_CCM_CACCR) & 0x7;
	return  mx6x_pll1_freq() / (div + 1);
}

static uint32_t mx6x_get_periph_clk(void)
{
    uint32_t reg;
    reg = in32(MX6X_CCM_BASE + MX6X_CCM_CBCDR);
    if (reg & MX6X_CCM_CBCDR_PERIPH_CLK_SEL) {
        reg = in32(MX6X_CCM_BASE + MX6X_CCM_CBCMR);
        switch ((reg & MX6X_CCM_CBCMR_PERIPH_CLK2_SEL_MASK) >>
            MX6X_CCM_CBCMR_PERIPH_CLK2_SEL_OFFSET) {
        case 0:
            return mx6x_pll7_freq();
        case 1:
        case 2:
            return MX6X_HCLK_FREQ;
        default:
            return 0;
        }
    } else {
        reg = in32(MX6X_CCM_BASE + MX6X_CCM_CBCMR);
        switch ((reg & MX6X_CCM_CBCMR_PRE_PERIPH_CLK_SEL_MASK) >>
            MX6X_CCM_CBCMR_PRE_PERIPH_CLK_SEL_OFFSET) {
        default:
        case 0:
            return mx6x_pll2_freq();
        case 1:
            return mx6x_pll2_pfd_freq(2);
        case 2:
            return mx6x_pll2_pfd_freq(0);
        case 3:
            return mx6x_pll2_pfd_freq(2) / 2;
        }
    }
}

uint32_t mx6x_get_ipg_clk(void)
{
    uint32_t ahb_podf, ipg_podf;

    ahb_podf = in32(MX6X_CCM_BASE + MX6X_CCM_CBCDR);
    ipg_podf = (ahb_podf & MX6X_CCM_CBCDR_IPG_PODF_MASK) >>
            MX6X_CCM_CBCDR_IPG_PODF_OFFSET;
    ahb_podf = (ahb_podf & MX6X_CCM_CBCDR_AHB_PODF_MASK) >>
            MX6X_CCM_CBCDR_AHB_PODF_OFFSET;
    return mx6x_get_periph_clk() / ((ahb_podf + 1) * (ipg_podf + 1));
}

uint32_t mx6x_get_ipg_per_clk(void)
{
    uint32_t podf;
    uint32_t clk_root = mx6x_get_ipg_clk();

    podf = in32(MX6X_CCM_BASE + MX6X_CCM_CSCMR1) & MX6X_CCM_CSCMR1_PERCLK_PODF_MASK;
    return clk_root / (podf + 1);
}

uint32_t mx6x_get_uart_clk(void)
{
    uint32_t freq = MX6X_PLL3_80M, reg, podf;

    reg = in32(MX6X_CCM_BASE + MX6X_CCM_CSCDR1);
    podf = (reg & MX6X_CCM_CSCDR1_UART_CLK_PODF_MASK) >>
        MX6X_CCM_CSCDR1_UART_CLK_PODF_OFFSET;
    freq /= (podf + 1);

    return freq;
}

uint32_t mx6x_get_cspi_clk(void)
{
    uint32_t freq = MX6X_PLL3_60M, reg, podf;

    reg = in32(MX6X_CCM_BASE + MX6X_CCM_CSCDR2);
    podf = (reg & MX6X_CCM_CSCDR2_ECSPI_CLK_PODF_MASK) >>
        MX6X_CCM_CSCDR2_ECSPI_CLK_PODF_OFFSET;
    freq /= (podf + 1);

    return freq;
}

uint32_t mx6x_get_ahb_clk(void)
{
    uint32_t cbcdr =  in32(MX6X_CCM_BASE + MX6X_CCM_CBCDR);
    uint32_t podf = (cbcdr & MX6X_CCM_CBCDR_AHB_PODF_MASK) \
            >> MX6X_CCM_CBCDR_AHB_PODF_OFFSET;

    return  mx6x_get_periph_clk() / (podf + 1);
}

uint32_t mx6x_get_axi_clk(void)
{
    uint32_t clkroot;
    uint32_t cbcdr =  in32(MX6X_CCM_BASE + MX6X_CCM_CBCDR);
    uint32_t podf = (cbcdr & MX6X_CCM_CBCDR_AXI_PODF_MASK) >>
        MX6X_CCM_CBCDR_AXI_PODF_OFFSET;

    if (cbcdr & MX6X_CCM_CBCDR_AXI_SEL) {
        if (cbcdr & MX6X_CCM_CBCDR_AXI_ALT_SEL)
            clkroot = MX6X_PLL3_PFD1_FREQ;
        else
            clkroot = mx6x_pll2_pfd_freq(2);
    } else
        clkroot = mx6x_get_periph_clk();

    return  clkroot / (podf + 1);
}

uint32_t mx6x_get_emi_slow_clk(void)
{
    uint32_t cscmr1 =  in32(MX6X_CCM_BASE + MX6X_CCM_CSCMR1);
    uint32_t emi_clk_sel = (cscmr1 & MX6X_CCM_CSCMR1_ACLK_EMI_SLOW_MASK) >>
        MX6X_CCM_CSCMR1_ACLK_EMI_SLOW_OFFSET;
    uint32_t podf = (cscmr1 & MX6X_CCM_CSCMR1_ACLK_EMI_SLOW_PODF_MASK) >>
            MX6X_CCM_CSCMR1_ACLK_EMI_PODF_OFFSET;

    switch (emi_clk_sel) {
    default:
    case 0:
        return mx6x_get_axi_clk() / (podf + 1);
    case 1:
        return mx6x_pll7_freq() / (podf + 1);
    case 2:
        return mx6x_pll2_pfd_freq(2) / (podf + 1);
    case 3:
        return mx6x_pll2_pfd_freq(0) / (podf + 1);
    }
}

uint32_t mx6x_get_ddr_clk(void)
{
    uint32_t cbcdr = in32(MX6X_CCM_BASE + MX6X_CCM_CBCDR);
    uint32_t podf = (cbcdr & MX6X_CCM_CBCDR_MMDC_CH0_PODF_MASK) >>
        MX6X_CCM_CBCDR_MMDC_CH0_PODF_OFFSET;

    return mx6x_get_periph_clk() / (podf + 1);
}

uint32_t mx6x_get_usdhc1_clk()
{
    uint32_t clkroot;
    uint32_t cscmr1 = in32(MX6X_CCM_BASE + MX6X_CCM_CSCMR1);
    uint32_t cscdr1 = in32(MX6X_CCM_BASE + MX6X_CCM_CSCDR1);
    uint32_t podf = (cscdr1 & MX6X_CCM_CSCDR1_USDHC1_PODF_MASK) >>
        MX6X_CCM_CSCDR1_USDHC1_PODF_OFFSET;

    if (cscmr1 & MX6X_CCM_CSCMR1_USDHC1_CLK_SEL)
        clkroot = mx6x_pll2_pfd_freq(0);
    else
        clkroot = mx6x_pll2_pfd_freq(2);

    return clkroot / (podf + 1);
}

uint32_t mx6x_get_usdhc2_clk(void)
{
    uint32_t clkroot;
    uint32_t cscmr1 = in32(MX6X_CCM_BASE + MX6X_CCM_CSCMR1);
    uint32_t cscdr1 = in32(MX6X_CCM_BASE + MX6X_CCM_CSCDR1);
    uint32_t podf = (cscdr1 & MX6X_CCM_CSCDR1_USDHC2_PODF_MASK) >>
        MX6X_CCM_CSCDR1_USDHC2_PODF_OFFSET;

    if (cscmr1 & MX6X_CCM_CSCMR1_USDHC2_CLK_SEL)
        clkroot = mx6x_pll2_pfd_freq(0);
    else
        clkroot = mx6x_pll2_pfd_freq(2);

    return clkroot / (podf + 1);
}

uint32_t mx6x_get_usdhc3_clk(void)
{
    uint32_t clkroot;
    uint32_t cscmr1 = in32(MX6X_CCM_BASE + MX6X_CCM_CSCMR1);
    uint32_t cscdr1 = in32(MX6X_CCM_BASE + MX6X_CCM_CSCDR1);
    uint32_t podf = (cscdr1 & MX6X_CCM_CSCDR1_USDHC3_PODF_MASK) >>
        MX6X_CCM_CSCDR1_USDHC3_PODF_OFFSET;

    if (cscmr1 & MX6X_CCM_CSCMR1_USDHC3_CLK_SEL)
        clkroot = mx6x_pll2_pfd_freq(0);
    else
        clkroot = mx6x_pll2_pfd_freq(2);

    return clkroot / (podf + 1);
}

uint32_t mx6x_get_usdhc4_clk(void)
{
    uint32_t clkroot;
    uint32_t cscmr1 = in32(MX6X_CCM_BASE + MX6X_CCM_CSCMR1);
    uint32_t cscdr1 = in32(MX6X_CCM_BASE + MX6X_CCM_CSCDR1);
    uint32_t podf = (cscdr1 & MX6X_CCM_CSCDR1_USDHC4_PODF_MASK) >>
        MX6X_CCM_CSCDR1_USDHC4_PODF_OFFSET;

    if (cscmr1 & MX6X_CCM_CSCMR1_USDHC4_CLK_SEL)
        clkroot = mx6x_pll2_pfd_freq(0);
    else
        clkroot = mx6x_pll2_pfd_freq(2);

    return clkroot / (podf + 1);
}

uint32_t mx6x_get_enfc_clk(void)
{
    uint32_t clkroot;
    uint32_t cs2cdr =  in32(MX6X_CCM_BASE + MX6X_CCM_CS2CDR);
    uint32_t podf = (cs2cdr & MX6X_CCM_CS2CDR_ENFC_CLK_PODF_MASK) >> MX6X_CCM_CS2CDR_ENFC_CLK_PODF_OFFSET;
    uint32_t pred = (cs2cdr & MX6X_CCM_CS2CDR_ENFC_CLK_PRED_MASK) >> MX6X_CCM_CS2CDR_ENFC_CLK_PRED_OFFSET;

    switch ((cs2cdr & MX6X_CCM_CS2CDR_ENFC_CLK_SEL_MASK) >>
        MX6X_CCM_CS2CDR_ENFC_CLK_SEL_OFFSET) {
        default:
        case 0:
            clkroot = mx6x_pll2_pfd_freq(0);
            break;
        case 1:
            clkroot = mx6x_pll2_freq();
            break;
        case 2:
            clkroot = mx6x_pll7_freq();
            break;
        case 3:
            clkroot = mx6x_pll2_pfd_freq(2);
            break;
    }

    return  clkroot / (pred+1) / (podf+1);
}

void mx6x_dump_clocks(void)
{
	if (debug_flag) {
		kprintf("\n");
		print_chip_info();
		kprintf("\n");
		kprintf("PLL1 - System     : %dMHz\n", SAFE_DIVIDE(mx6x_pll1_freq(), ONE_MHZ));
		kprintf("PLL2 - System Bus : %dMHz\n", SAFE_DIVIDE(mx6x_pll2_freq(), ONE_MHZ));
		kprintf("PLL3 - OTG USB    : %dMHz\n", SAFE_DIVIDE(mx6x_pll3_freq(), ONE_MHZ));
		kprintf("PLL4 - Audio      : %dMHz\n", SAFE_DIVIDE(mx6x_pll4_freq(), ONE_MHZ));
		kprintf("PLL5 - Video      : %dMHz\n", SAFE_DIVIDE(mx6x_pll5_freq(), ONE_MHZ));
		kprintf("PLL7 - Host USB   : %dMHz\n", SAFE_DIVIDE(mx6x_pll7_freq(), ONE_MHZ));
		kprintf("PLL8 - Enet       : %dMHz\n", SAFE_DIVIDE(mx6x_pll8_freq(), ONE_MHZ));
		kprintf("IPG clock     : %dHz\n", mx6x_get_ipg_clk());
		kprintf("IPG per clock : %dHz\n", mx6x_get_ipg_per_clk());
		kprintf("UART clock    : %dHz\n", mx6x_get_uart_clk());
		kprintf("CSPI clock    : %dHz\n", mx6x_get_cspi_clk());
		kprintf("AHB clock     : %dHz\n", mx6x_get_ahb_clk());
		kprintf("AXI clock     : %dHz\n", mx6x_get_axi_clk());
		kprintf("EMI_SLOW clock: %dHz\n", mx6x_get_emi_slow_clk());
		kprintf("DDR clock     : %dHz\n", mx6x_get_ddr_clk());
		kprintf("USDHC1 clock  : %dHz\n", mx6x_get_usdhc1_clk());
		kprintf("USDHC2 clock  : %dHz\n", mx6x_get_usdhc2_clk());
		kprintf("USDHC3 clock  : %dHz\n", mx6x_get_usdhc3_clk());
		kprintf("USDHC4 clock  : %dHz\n", mx6x_get_usdhc4_clk());
		kprintf("ENFC clock    : %dHz\n", mx6x_get_enfc_clk());
		kprintf("\n");
	}
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/mx6x_get_clocks.c $ $Rev: 739370 $")
#endif
