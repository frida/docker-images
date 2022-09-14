/*
 * $QNXLicenseC:
 * Copyright 2007,2009 QNX Software Systems.
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <pthread.h>
#include <sys/siginfo.h>
#include <sys/mman.h>
#include <string.h>
#include <hw/inout.h>
#include <sys/neutrino.h>
#include <errno.h>
#include <atomic.h>
#include <fcntl.h>
#include <sys/rsrcdbmgr.h>
#include <sys/rsrcdbmsg.h>
#include <hw/dma.h>
#include <sys/rsrcdbmgr.h>
#include <sys/hwinfo.h>
#include <drvr/hwinfo.h>
#include <sys/cache.h>

#include "override.h"

// Controller Info
#ifndef SDMA_BASE
    #define SDMA_BASE          0x53FD4000
#endif

#ifndef SDMA_SIZE
    #define SDMA_SIZE          0x4000
#endif

#ifndef SDMA_IRQ
    #define SDMA_IRQ           34
#endif

#ifndef SDRAM_BASE
	#define SDRAM_BASE    0x80000000
#endif

#ifndef SDMA_DESCRIPTION_STR
    #define SDMA_DESCRIPTION_STR    "i.MX SDMA Controller"
#endif

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
#define SDMA_CHTYPE_MCU_2_SHP       3
#define SDMA_CHTYPE_SHP_2_MCU       4
#define SDMA_CHTYPE_MCU_2_SPDIF     5
#define SDMA_CHTYPE_SPDIF_2_MCU     6
#define SDMA_CHTYPE_UARTSH_2_MCU    7
#define SDMA_CHTYPE_UART_2_MCU      8

#define SDMA_MAX_CHAN               255


/* register map */

#define SDMA_MC0PTR            0x00    /* AP (MCU) Channel 0 Pointer */
#define SDMA_INTR              0x04    /* Channel Interrupts */
#define SDMA_STOP_STAT         0x08    /* Channel Stop / Channel Status */
#define SDMA_HSTART            0x0C    /* Channel Start */
#define SDMA_EVTOVR            0x10    /* Channel Event Override */
#define SDMA_DSPOVR            0x14    /* Channel DSP (BP) Override */
#define SDMA_HOSTOVR           0x18    /* Channel AP Override */
#define SDMA_EVTPEND           0x1C    /* Channel Event Pending */
#define SDMA_RESET             0x24    /* Reset Register */
#define SDMA_EVTERR            0x28    /* DMA Request Error Register */
#define SDMA_INTRMASK          0x2C    /* Channel AP Interrupt Mask */
#define SDMA_PSW               0x30    /* Schedule Status */
#define SDMA_EVTERRDBG         0x34    /* DMA Request Error Register */
#define SDMA_CONFIG            0x38    /* Configuration Register */
	#define CONFIG_CSM_MSK         0x3
	#define CONFIG_CSM_STATIC      0x0
	#define CONFIG_CSM_DYNAMIC     0x3

#define SDMA_ONCE_ENB          0x40    /* OnCE Enable */
#define SDMA_ONCE_DATA         0x44    /* OnCE Data Register */
#define SDMA_ONCE_INSTR        0x48    /* OnCE Instruction Register */
#define SDMA_ONCE_STAT         0x4C    /* OnCE Status Register */
#define SDMA_ONCE_CMD          0x50    /* OnCE Command Register */
#define SDMA_EVT_MIRROT        0x54    /* DMA Request */
#define SDMA_ILLINSTADDR       0x58    /* Illegal Instruction Trap Address */
#define SDMA_CHN0ADDR          0x5C    /* Channel 0 Boot Address */
#define SDMA_XTRIG_CONF1       0x70    /* Cross-Trigger Events Configuration Register 1 */
#define SDMA_XTRIG_CONF2       0x74    /* Cross-Trigger Events Configuration Register 2 */
#define SDMA_CHNPRI(n)         (0x100 + ((n)<<2))   /* Channel Priority n = 0 to 31 */

#ifdef SDMA_REGISTER_MAP_VERSION2
    #define MAX_DMA_EVENT          48
    #define SDMA_CHNENBL(n)        (0x200 + ((n)<<2))   /* Channel Enable n = 0 to 47 */
#else
    #define MAX_DMA_EVENT          32
    #define SDMA_CHNENBL(n)        (0x80 + ((n)<<2))   /* Channel Enable n = 0 to 31 */
#endif

/*
 * Data Structures
 */

// microcode info structures
typedef struct {
    const uint16_t          *p;
    uint32_t                addr;
    uint32_t                size;
} microcode_info_t;

typedef struct {
    uint32_t                script_addr_arr[SDMA_MAX_CHAN];
    microcode_info_t        ram_microcode_info;
} sdma_scriptinfo_t;



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

// sdma shared memory
typedef struct {
    // used for mutex
    uint32_t process_cnt;
    pthread_mutex_t libinit_mutex;
    pthread_mutex_t command_mutex;
    pthread_mutex_t register_mutex;

    // used for ccb structure
    off64_t paddr64;
    sdma_ccb_t ccb_arr[SDMA_N_CH];
} sdma_shmem_t;

#define SDMA_CTX_WSIZE      32
typedef struct {
    uint32_t pc;
    uint32_t spc;
    uint32_t g_reg[8];
    uint32_t  dma_xfer_regs[14];
    uint32_t scratch[8];
} sdma_ch_ctx_t;


typedef struct {
    unsigned ch_num;
    unsigned ch_type;

    // channel configurations
    unsigned attach_flags;
    unsigned watermark;
    unsigned fifo_paddr;
    unsigned data_width;
    unsigned eventnum;
    unsigned is_event_driven;
    unsigned regen_descr;
    unsigned cont_descr_loop;

    // buffer descriptors
    volatile sdma_bd_t * bd_ptr;
    unsigned n_frags;

    // copy of sdma context that is loaded into SDMA context RAM
    volatile sdma_ch_ctx_t * ctx_ptr;
    uint32_t  ctx_paddr;

} sdma_chan_t;     // channel control struct

typedef void (*sdmairq_callback_t)(unsigned);

// prototypes
int sdmasync_init(void);
void sdmasync_fini(void);
pthread_mutex_t * sdmasync_cmdmutex_get();
pthread_mutex_t * sdmasync_libinit_mutex_get();
pthread_mutex_t * sdmasync_regmutex_get();
int sdmasync_is_first_process();
int sdmasync_is_last_process();
void sdmasync_process_cnt_incr();
void sdmasync_process_cnt_decr();
off64_t sdmasync_ccb_paddr_get();
sdma_ccb_t * sdmasync_ccb_ptr_get();

int sdmacmd_cmdch_create();
void sdmacmd_cmdch_destroy();
void sdmacmd_ctx_config(sdma_chan_t * chan_ptr);
int sdmacmd_ctx_load(sdma_chan_t * chan_ptr);

int sdmairq_init(uint32_t irq);
void sdmairq_fini();
void sdmairq_event_add(uint32_t channel, const struct sigevent *event);
void sdmairq_event_remove(uint32_t channel);
void sdmairq_callback_add(uint32_t channel,sdmairq_callback_t func_ptr);
void sdmairq_callback_remove(uint32_t channel);

int sdmascript_lookup( sdma_scriptinfo_t * scriptinfo );
#endif


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/dma/sdma/sdma.h $ $Rev: 750963 $")
#endif
