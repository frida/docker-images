/*
 * $QNXLicenseC:
 * Copyright 2008, 2012 QNX Software Systems.
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


#ifndef SDMA_H
#define SDMA_H

#include <inttypes.h>

// Channel Configurations
#define SDMA_N_CH                   32
#define SDMA_CH_LO                  1   // 0 reserved for command channel
#define SDMA_CH_HI                  31
#define SDMA_CH_PRIO_LO             1
#define SDMA_CH_PRIO_HI             7
#define SDMA_CH_DEFAULT_PRIO        SDMA_CH_PRIO_LO

// channel types
#define SDMA_CHTYPE_AP_2_AP         0
#define SDMA_CHTYPE_MCU_2_AP        1
#define SDMA_CHTYPE_AP_2_MCU        2
#define SDMA_CHTYPE_MCU_2_SPDIF		3	// experimental
#define SDMA_CHTYPE_SPDIF_2_MCU		4	// experimental

// Buffer Descriptor Structure 
typedef struct {
    uint32_t cmd_and_status;
#define SDMA_CMDSTAT_COUNT_MASK     0xffff
#define SDMA_CMDSTAT_DONE_MASK      0x10000
#define SDMA_CMDSTAT_WRAP_MASK      0x20000
#define SDMA_CMDSTAT_CONT_MASK      0x40000
#define SDMA_CMDSTAT_INT_MASK       0x80000
#define SDMA_CMDSTAT_ERROR_MASK     0x100000
#define SDMA_CMDSTAT_LAST_MASK      0x200000
#define SDMA_CMDSTAT_EXT_MASK       0x800000
#define SDMA_CMDSTAT_CMD_MASK       0xff000000
#define SDMA_CMDSTAT_CMD_POS        24

// Supported Buffer Descriptor Commands
#define SDMA_CMD_C0_SET_PM          (0x4 << SDMA_CMDSTAT_CMD_POS)
#define SDMA_CMD_C0_SET_DM          (0x1 << SDMA_CMDSTAT_CMD_POS)
#define SDMA_CMD_C0_GET_PM          (0x8 << SDMA_CMDSTAT_CMD_POS)
#define SDMA_CMD_C0_GET_DM          (0x2 << SDMA_CMDSTAT_CMD_POS)
#define SDMA_CMD_C0_SETCTX(n)       ((((n)<<3) | 7) << SDMA_CMDSTAT_CMD_POS)    // 0 <= n <= 31
#define SDMA_CMD_C0_GETCTX(n)       ((((n)<<3) | 6) << SDMA_CMDSTAT_CMD_POS)    // 0 <= n <= 31
#define SDMA_CMD_XFER_SIZE(s)       ((((s)==8)?1:((s)==16)?2:((s)==24)?3:0) << 24)    // 32 is default

    uint32_t buf_paddr;
    uint32_t ext_buf_paddr;
} sdma_bd_t;

// Channel Control Block Structure
typedef struct {
    uint32_t current_bd_paddr;
    uint32_t base_bd_paddr; // Base BufferDescriptor Ptr
    uint32_t status;
#define SDMA_CCBSTAT_EXE_MASK           0x1
#define SDMA_CCBSTAT_STATEDIR_MASK      0x2
#define SDMA_CCBSTAT_OPENINIT_MASK      0x4
#define SDMA_CCBSTAT_INST_MASK          0xc0000000
    uint32_t reserved;  //no channel descriptor implemented
} sdma_ccb_t;    


#define SDMA_CTX_WSIZE      32  
typedef struct {
    uint32_t pc;
    uint32_t spc;
    uint32_t g_reg[8];
    uint32_t  dma_xfer_regs[14]; 
    uint32_t scratch[8];
} sdma_ch_ctx_t;     


#endif



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/sdma.h $ $Rev: 729057 $")
#endif
