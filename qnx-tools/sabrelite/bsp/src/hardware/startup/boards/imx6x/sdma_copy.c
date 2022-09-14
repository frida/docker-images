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

#include "sdma.h"
#include "sdma_microcode.h"


#define SDMA_CHN0ADDR_SMSZ_MASK		0x4000

#define SDMA_CMD_CH			0
#define SDMA_CMD_CH_PRIO		4
#define SDMA_DATA_CH			1

/*
 * SDMA ROM Memory offset where memory to memory (also called AP_2_AP) SDMA ROM script is stored
 * Note that the memory to memory SDMA ROM script has a max transfer size of (64KB-1) 
 */
#define AP_2_AP_ADDR			642

/*
 * SDMA RAM Memory offset where optimized memory to memory (also called AP_2_AP) SDMA RAM script is stored
 * The maximum SDMA transfer size is 16M - 1
 *
 * Note that the AP_2_AP SDMA RAM script has a slightly different buffer descripter layout than
 * the AP_2_AP SDMA ROM script.  The command field has been replaced by 'Count[23:16]'.
 */
#define RAM_AP_2_AP_ADDR		6144

/*
 * The max SDMA size is dependent upon which SDMA script is being used.  By default
 * the optimized RAM script is used which has a max SDMA transfer size of 16MB-1 byte
 * however for performance reasons SDMA copy size must be at least 4 byte aligned so
 * we set the max transfer size to 16MB - 4.
 */
#define MAX_SDMA_XFER_SIZE              ((16 * 1024 * 1024) - 4)
//#define MAX_SDMA_XFER_SIZE              ((64 * 1024) - 4)

/*
 * The SDMA AP_2_AP RAM script requires a transfer size >= 32 bytes or the memory will not be copied.
 * Note that AP_2_AP ROM script does not have a minimum size limitation.
 */
#define MIN_SDMA_XFER_SIZE		32

paddr_t	imx6x_sdma_base = MX6X_SDMA_BASE;

sdma_ccb_t		ccb_ptr[2] = {{0,},};
sdma_bd_t		cmd_bd_ptr[1] = {{0,},};
sdma_bd_t		chnl_bd_ptr[1] = {{0,},};
sdma_ch_ctx_t	ctx_ptr = {0,};

int sdma_init(void)
{
	int		i;

	// Reset SDMA
	out32(MX6X_SDMA_BASE + MX6X_SDMA_RESET, 1);	

	// Wait for SDMA to come out of reset
	while (in32(imx6x_sdma_base + MX6X_SDMA_RESET) & 1)
		;

	out32(imx6x_sdma_base + MX6X_SDMA_INTR, 0xffffffff);
	out32(imx6x_sdma_base + MX6X_SDMA_EVTOVR, 0xffffffff);

	out32(imx6x_sdma_base + MX6X_SDMA_CHN0ADDR ,
					       in32(imx6x_sdma_base + MX6X_SDMA_CHN0ADDR) | SDMA_CHN0ADDR_SMSZ_MASK);

	for (i = 0; i < SDMA_N_CH; i++)
		out32(imx6x_sdma_base + MX6X_SDMA_CHNENBL(i), 0x0);

	// associate CCB with SDMA
	out32(imx6x_sdma_base + MX6X_SDMA_MC0PTR, (uint32_t)&ccb_ptr[0]);

	// associate command channel descriptor with CCB
	ccb_ptr[SDMA_CMD_CH].base_bd_paddr = (uint32_t)&cmd_bd_ptr[0];
	ccb_ptr[SDMA_CMD_CH].current_bd_paddr = (uint32_t)&cmd_bd_ptr[0];
	// setup registers related to the command channel
	out32(imx6x_sdma_base + MX6X_SDMA_CHNPRI(SDMA_CMD_CH), SDMA_CMD_CH_PRIO);
	out32(imx6x_sdma_base + MX6X_SDMA_EVTOVR, in32(imx6x_sdma_base + MX6X_SDMA_EVTOVR) | (1 << SDMA_CMD_CH) );

	// set command buffer descriptor
	cmd_bd_ptr[0].cmd_and_status = 0;
	cmd_bd_ptr[0].cmd_and_status |= SDMA_CMD_C0_SET_PM;
	cmd_bd_ptr[0].cmd_and_status |= SDMA_CMDSTAT_WRAP_MASK;
	cmd_bd_ptr[0].cmd_and_status |= SDMA_CMDSTAT_DONE_MASK;
	cmd_bd_ptr[0].cmd_and_status |= SDMA_CMDSTAT_INT_MASK;
	cmd_bd_ptr[0].cmd_and_status |= SDMA_CMDSTAT_EXT_MASK;

	/*
	 * The 'count' is expressed in 16-bit half-words when using SET_PM.
	 * It may be possible to transfer _just_ the AP_2_AP script into SDMA RAM
	 * but the time savings is negligable and it's not clear if transferring part
	 * of an SDMA RAM image could cause problems, so we transfer the entire SDMA RAM image.
	 */
	cmd_bd_ptr[0].cmd_and_status |= sizeof(sdma_code) / 2;

	cmd_bd_ptr[0].buf_paddr = (uint32_t)&sdma_code[0];
	cmd_bd_ptr[0].ext_buf_paddr = SDMA_RAM_CODE_START_ADDR;
	
	// start command channel
	out32(imx6x_sdma_base + MX6X_SDMA_HSTART, 1 << SDMA_CMD_CH);

	//wait for command completion
	while (1) {
		if (in32(imx6x_sdma_base + MX6X_SDMA_INTR) & 1) {
			out32(imx6x_sdma_base + MX6X_SDMA_INTR, 1);
			break;
		}
	}

	return 0;
}

static int sdma_chnl_cfg(int ch_num)
{
	// clear interrupts on this channel just in case it may still be set
	// from a previous driver crash or something.
	out32(imx6x_sdma_base + MX6X_SDMA_INTR, (1 << ch_num));

	// associate newly created descriptor with ccb
	ccb_ptr[ch_num].base_bd_paddr = (uint32_t)chnl_bd_ptr;
	ccb_ptr[ch_num].current_bd_paddr = (uint32_t)chnl_bd_ptr;

	// Config ctx
	//ctx_ptr.pc = AP_2_AP_ADDR;
	ctx_ptr.pc = RAM_AP_2_AP_ADDR;

	// load ctx
	// set command buffer descriptor
	cmd_bd_ptr[0].cmd_and_status = 0;
	cmd_bd_ptr[0].cmd_and_status |= SDMA_CMD_C0_SETCTX(ch_num);
	cmd_bd_ptr[0].cmd_and_status |= SDMA_CMDSTAT_WRAP_MASK;
	cmd_bd_ptr[0].cmd_and_status |= SDMA_CMDSTAT_DONE_MASK;
	cmd_bd_ptr[0].cmd_and_status |= SDMA_CMDSTAT_INT_MASK;
	cmd_bd_ptr[0].cmd_and_status |= SDMA_CTX_WSIZE;     //ctx size

	cmd_bd_ptr[0].buf_paddr = (uint32_t)&ctx_ptr;

	// start command channel
	out32(imx6x_sdma_base + MX6X_SDMA_HSTART, 1 << SDMA_CMD_CH);

	// wait for command completion
	while (1) {
		if (in32(imx6x_sdma_base + MX6X_SDMA_INTR) & 1) {
			out32(imx6x_sdma_base + MX6X_SDMA_INTR, 1);
			break;
		}
	}

	// set channel priority
	out32(imx6x_sdma_base + MX6X_SDMA_CHNPRI(ch_num), SDMA_CH_PRIO_HI);

	return 0;
}

static int sdma_xfer(int ch_num, uint32_t src, uint32_t dst, uint32_t len)
{

	//kprintf("%s: ch_num: 0x%x, src: 0x%x, dst: 0x%x, len: %d\n",
	//	__FUNCTION__, ch_num, src, dst, len);

	// fill-in buffer descriptor, note that there is not improvement in throughput
	// between differrent xfer sizes, e.g. 32 bit xfer size vs 16 bit.
	uint32_t length_low, length_high;
	length_low = len & 0xffff;
	length_high = ((len >> 16) & 0xff) << 24;

	/* Note that the SDMA AP_2_AP RAM script has a slightly different Buffer Descriptor
	 * than the AP_2_AP ROM script therefore 'SDMA_CMD_XFER_SIZE' is not applicable
	 * when the RAM script is being used.
	 */
	chnl_bd_ptr[0].cmd_and_status = length_high | SDMA_CMDSTAT_EXT_MASK |
								//SDMA_CMD_XFER_SIZE(16) |
								SDMA_CMDSTAT_WRAP_MASK |
								SDMA_CMDSTAT_INT_MASK |
								length_low;
	chnl_bd_ptr[0].buf_paddr = src;
	chnl_bd_ptr[0].ext_buf_paddr = dst;
	// enable the descr last
	chnl_bd_ptr[0].cmd_and_status |= SDMA_CMDSTAT_DONE_MASK;

	// trigger the dma using software
	out32(imx6x_sdma_base + MX6X_SDMA_HSTART, 1 << ch_num);

	while (1) {
		if (in32(imx6x_sdma_base + MX6X_SDMA_INTR) & (1 << ch_num)) {
			out32(imx6x_sdma_base + MX6X_SDMA_INTR, 1 << ch_num);
			break;
		}
	}

	return 0;
}

/*
 * sdma_copy can be used throughout startup for SDMA copies. The only restriction is that 
 * sdma_init() be called prior to calling sdma_copy.  For example if a mini-driver needs
 * to use sdma_copy, sdma_init should be called prior to mdriver_add() in main.c.
 * Also the 'src' and 'dst' addresses should be 32 bit aligned for optimal throughput.
 */
void sdma_copy(uint32_t dst, uint32_t src, uint32_t size)
{
	int dmasz;

	/*
	 * If source or destination address are not 4 byte aligned we cannot use the AP_2_AP
	 * RAM script
	 */
	if ((src % 4) || (dst % 4))
	{
		if (debug_flag)
			kprintf("%s: SDMA cannot be used for unaligned transfers, using regular memory copy");
		copy_memory(dst, src, size);
		return;
	}

	/*
	 * The SDMA AP_2_AP RAM script requires all transfer sizes to be divisible by 4.
	 * Any leftover bytes will be transfered via a regular memory copy.
	 */
	unsigned leftover;

	leftover = size % 4;
	size -= leftover;

	if (size >= MIN_SDMA_XFER_SIZE)
		sdma_chnl_cfg(SDMA_DATA_CH);

	while (size > 0) {

		// Take a break from image copy to allow each mini-driver handler function to run
		mdriver_check();

		// Limit copy size based on mdriver_max
		if (lsp.mdriver.size > 0)
			dmasz = ((size >= mdriver_max) ? mdriver_max : size);
		else
			dmasz = size;

		/*
		 * Regardless of mdriver_max the maximum SDMA copy size is still limited by the maximum
		 * copy size supported by the AP_2_AP SDMA script.
		 */
		if (dmasz >= MAX_SDMA_XFER_SIZE)
			dmasz = MAX_SDMA_XFER_SIZE;

		/*
		 * Use a regular memory copy for small transfers.
		 */
		if (dmasz < MIN_SDMA_XFER_SIZE)
			copy_memory(dst, src, dmasz);
		else
			sdma_xfer(SDMA_DATA_CH, src, dst, dmasz);

		size -= dmasz;
		dst += dmasz;
		src += dmasz;
	}

	if (leftover)
		copy_memory(dst, src, leftover);

	return;
}
void sdma_load_ifs(uint32_t dst, uint32_t src, uint32_t size)
{
	sdma_copy(dst, src, size);
	return;
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/sdma_copy.c $ $Rev: 760385 $")
#endif
