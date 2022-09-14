/*
 * $QNXLicenseC:
 * Copyright 2007, 2008, QNX Software Systems.
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







#ifdef DEFN
    #define EXT
    #define INIT1(a)                = { a }
#else
    #define EXT extern
    #define INIT1(a)
#endif

#ifndef TRUE
    #define TRUE 1
#endif
#ifndef FALSE
    #define FALSE 0
#endif

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <malloc.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/neutrino.h>
#include <termios.h>
#include <devctl.h>
#include <sys/dcmd_chr.h>
#include <sys/iomsg.h>
#include <atomic.h>
#include <hw/inout.h>
#include <arm/mx1.h>
#include <sys/io-char.h>
#include <sys/hwinfo.h>
#include <drvr/hwinfo.h>
#include <pthread.h>
#include <sys/rsrcdbmgr.h>
#include <sys/dispatch.h>
#include <sys/slog.h>
#include <sys/slogcodes.h>

#ifdef USE_DMA
#include <hw/dma.h>
#endif
#define MX53_UART3_BASE_ADDR 0x5000c000
#define MX53_UART_UCR1_ATDMAEN (1<<2) // Aging DMA timer enable

#define FIFO_SIZE 32

typedef struct mx53_dma
{
    char            *buf;
    off64_t         phys_addr;
    int             xfer_size;
    struct sigevent sdma_event;
    void             *pulse;
    void             *dma_chn;
    int              bytes_read;
    int              buffer0;
    int              status;
    unsigned         key;
} mx53_dma_t;

typedef struct dev_mx1 {
    TTYDEV          tty;
    unsigned        intr[2];
    int             iid[2];
    unsigned        clk;
    unsigned        div;
    unsigned        fifo;
    uintptr_t       base;
    unsigned        fcr;
    unsigned        cr2;
    unsigned        bir;
    unsigned        mx1;
    mx53_dma_t      rx_dma;
    mx53_dma_t      tx_dma;
    unsigned        usedma;
    int             rx_dma_evt;
    int             tx_dma_evt;
    int             isr;
#ifdef USE_DMA
    dma_functions_t sdmafuncs;
    unsigned        tx_xfer_active;
#endif
} DEV_MX1;

typedef struct ttyinit_mx1 {
    TTYINIT     tty;
    unsigned    mx1;
    unsigned    intr[2];   /* Interrupts */
    unsigned    usedma;
    int         rx_dma_evt;
    int         tx_dma_evt;
    int         isr;
} TTYINIT_MX1;

EXT TTYCTRL        ttyctrl;

#define DMA_XFER_SIZE    512

extern unsigned mphys(void *);

#include "proto.h"



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/devc/sermx1/externs.h $ $Rev: 739763 $")
#endif
