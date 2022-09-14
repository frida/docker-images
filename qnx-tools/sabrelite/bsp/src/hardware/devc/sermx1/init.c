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








#include "externs.h"
#include <sys/mman.h>
#include <string.h>

/*
 * Specify parameters for default devices.
 */
void *
query_default_device(TTYINIT_MX1 *dip, void *link)
{
    /*
     * No default device, the base address and irq have be be specified
     */
    return NULL;
}

void
create_device(TTYINIT_MX1 *dip, unsigned unit)
{
    DEV_MX1        *dev;
#ifdef USE_DMA
    char        str[250];
    unsigned     channel;
    dma_transfer_t tinfo;
    dma_addr_t dma_addr;
#endif
    /*
     * Get a device entry and the input/output buffers for it.
     */
    dev = calloc(1, sizeof(*dev));

    if(dev == NULL)
    {
        perror("MX1 UART: Unable to allocate device entry\n");
        exit(1);
    }

    if(dev->usedma && dip->tty.isize < (DMA_XFER_SIZE*2))
    {
        perror("MX1 UART: Invalid input buffer size\n");
        dip->tty.isize = DMA_XFER_SIZE * 2;
    }

    /*
     * Get buffers.
     */
    dev->tty.ibuf.head = dev->tty.ibuf.tail = dev->tty.ibuf.buff = malloc(dev->tty.ibuf.size = dip->tty.isize);
    dev->tty.obuf.head = dev->tty.obuf.tail = dev->tty.obuf.buff = malloc(dev->tty.obuf.size = dip->tty.osize);
    dev->tty.cbuf.head = dev->tty.cbuf.tail = dev->tty.cbuf.buff = malloc(dev->tty.cbuf.size = dip->tty.csize);
    if(dip->usedma)
        dev->tty.highwater = dev->tty.ibuf.size + 1;    // when DMA is enabled never reach the RX FIFO highwater mark.
    else
        dev->tty.highwater = dev->tty.ibuf.size - (dev->tty.ibuf.size < 128 ? dev->tty.ibuf.size/4 : 100);


    strcpy(dev->tty.name, dip->tty.name);

    dev->tty.baud    = dip->tty.baud;

    /*
     * The i.MX SOCs don't technically require the LOSES_TX_INTR flag,
     * but the timer mechanism acts as a failsafe in case we ever miss a TX interrupt.
     */
    dev->tty.flags   = EDIT_INSERT | LOSES_TX_INTR;
    dev->tty.c_cflag = dip->tty.c_cflag;
    dev->tty.c_iflag = dip->tty.c_iflag;
    dev->tty.c_lflag = dip->tty.c_lflag;
    dev->tty.c_oflag = dip->tty.c_oflag;
    dev->tty.verbose = dip->tty.verbose;
    dev->tty.fifo    = dip->tty.fifo;

    dev->fifo        = dip->tty.fifo;
    dev->intr[0]     = dip->intr[0];
    dev->intr[1]     = dip->intr[1];
    dev->clk         = dip->tty.clk;
    dev->div         = dip->tty.div;
    dev->mx1         = dip->mx1;

    dev->usedma      = dip->usedma;
    dev->rx_dma_evt  = dip->rx_dma_evt;
    dev->tx_dma_evt  = dip->tx_dma_evt;
    dev->isr         = dip->isr;
    dev->rx_dma.buffer0 = 1;
    dev->rx_dma.status = 0;
    dev->rx_dma.bytes_read = 0;
    dev->rx_dma.key = 0;

    /*
     * Currently io-char has a limitation that the TX timeout is hard coded to 150ms.
     * At low baud rates the timer could potentially expire before the DMA transfer
     * naturally completes. So when DMA is enabled we disable the LOSES_TX_INTR flag
     * and let the driver specify the timeout value in tto().
     */
#if USE_DMA
    if(dev->usedma)
        dev->tty.flags &= ~LOSES_TX_INTR;
#endif

    /*
     * Map device registers
     */
    dev->base = mmap_device_io(MX1_UART_SIZE, dip->tty.port);
    if (dev->base == (uintptr_t)MAP_FAILED) {
        perror("MX1 UART: MAP_FAILED\n");
        goto fail1;
    }
#if USE_DMA
    if(dev->usedma)
    {
        dev->tx_dma.xfer_size = DMA_XFER_SIZE;
        if((dev->tx_dma.buf = mmap(NULL, dev->tx_dma.xfer_size, PROT_READ | PROT_WRITE | PROT_NOCACHE, MAP_ANON | MAP_PHYS, NOFD, 0)) == MAP_FAILED)
        {
            perror("Unable to allocate DMA memory\n");
            goto fail2;
        }

        mem_offset64(dev->tx_dma.buf, NOFD, 1, &dev->tx_dma.phys_addr, 0);
        msync(dev->tx_dma.buf, dev->tx_dma.xfer_size, MS_INVALIDATE);

        /* Allocte 2x the transfer size for ping-pong buffer (only required for recv size) */
        dev->rx_dma.xfer_size = DMA_XFER_SIZE;
        if((dev->rx_dma.buf = mmap(NULL, dev->rx_dma.xfer_size * 2, PROT_READ | PROT_WRITE | PROT_NOCACHE, MAP_ANON | MAP_PHYS, NOFD, 0)) == MAP_FAILED)
        {
            perror("Unable to allocate DMA memory\n");
            goto fail3;
        }

        mem_offset64(dev->rx_dma.buf, NOFD, 1, &dev->rx_dma.phys_addr, 0);
        msync(dev->rx_dma.buf, dev->rx_dma.xfer_size * 2, MS_INVALIDATE);

        my_attach_pulse(&dev->rx_dma.pulse, &dev->rx_dma.sdma_event, mx53_rx_pulse_hdlr, dev);
        my_attach_pulse(&dev->tx_dma.pulse, &dev->tx_dma.sdma_event, mx53_tx_pulse_hdlr, dev);

        if(get_dmafuncs(&dev->sdmafuncs, sizeof(dma_functions_t)) == -1)
        {
            perror("MX1 UART: Failed to get DMA lib function\n");
            goto fail4;
        }

        if(dev->sdmafuncs.init(NULL) == -1)
        {
            perror("MX1 UART: DMA init failed\n");
            goto fail4;
        }

        // water-mark is set to 1 less than fifo threshold.
        sprintf(str, "eventnum=%ld,watermark=%d,fifopaddr=0x%x", (long int)dev->rx_dma_evt, ((dev->fifo & 0x3f)-1) , (uint32_t) dip->tty.port + 0x0);

#if defined(VARIANT_mx53)
        // For i.MX53 we only support shared UART modules so the UARTSH_2_MCU script is used.
        channel = 7;    // SDMA_CHTYPE_UARTSH_2_MCU
#else
        // For i.MX6x Freescale recommends using the UART_2_MCU SDMA script for all UART modules (shared UARTs and ARM Platform UARTs)
        channel = 8;    // SDMA_CHTYPE_UART_2_MCU
#endif
        if((dev->rx_dma.dma_chn = dev->sdmafuncs.channel_attach(str, &dev->rx_dma.sdma_event, &channel,
            DMA_ATTACH_PRIORITY_HIGHEST, DMA_ATTACH_EVENT_PER_SEGMENT)) == NULL)
        {
            perror("Unable to create rx dma channel\n");
            goto fail5;
        }
        sprintf(str, "eventnum=%ld,watermark=%d,fifopaddr=0x%x\n", (long int)dev->tx_dma_evt, (32 - ((dev->fifo >> 10) & 0x3f))  , (uint32_t)dip->tty.port+MX1_UART_TXDATA);

#if defined(VARIANT_mx53)
        channel = 3;    // SDMA_CHTYPE_MCU_2_SHP
#else
        channel = 1;    // SDMA_CHTYPE_MCU_2_AP
#endif

        if((dev->tx_dma.dma_chn = dev->sdmafuncs.channel_attach(str, &dev->tx_dma.sdma_event,(unsigned *) &channel,
            DMA_ATTACH_PRIORITY_HIGHEST, DMA_ATTACH_EVENT_PER_SEGMENT)) == NULL)
        {
            perror("Unable to create tx dma channel\n");
            goto fail6;
        }
        dev->tx_xfer_active = FALSE;
    }
#endif

    /*
    * Initialize termios cc codes to an ANSI terminal.
    */
    ttc(TTC_INIT_CC, &dev->tty, 0);

    /*
    * Initialize the device's name.
    * Assume that the basename is set in device name.  This will attach
    * to the path assigned by the unit number/minor number combination
    */
    unit = SET_NAME_NUMBER(unit) | NUMBER_DEV_FROM_USER;
    ttc(TTC_INIT_TTYNAME, &dev->tty, unit);

    /*
    * Initialize power management structures before attaching ISR
    */
    ttc(TTC_INIT_POWER, &dev->tty, 0);

    /* Assert DSR/DTR */
    out32 ( dev->base + MX1_UART_CR3, in32(dev->base + MX1_UART_CR3) | MX1_UCR3_DSR);

    /* Clear UART configuration, which won't be cleared by setting UART device */
    out32(dev->base + MX1_UART_CR1, 0);
    out32(dev->base + MX1_UART_CR2, 0);

    /*
    * Only setup IRQ handler for non-pcmcia devices.
    * Pcmcia devices will have this done later when card is inserted.
    */
    if (dip->tty.port != 0 && dev->intr[0] != -1) {
        ser_stty(dev);
        ser_attach_intr(dev);
    }

    /* 
     * Enable UART
     */
    out32(dev->base + MX1_UART_CR1, in32(dev->base + MX1_UART_CR1) | MX1_UCR1_UARTEN);

    /*
    * Attach the resource manager
    */
    ttc(TTC_INIT_ATTACH, &dev->tty, 0);
#if USE_DMA
    if(dev->usedma)
    {
        // Schedule an RX Transfer (upto MAX DMA SIZE)
        tinfo.xfer_bytes = dma_addr.len = dev->rx_dma.xfer_size;
        dma_addr.paddr = dev->rx_dma.phys_addr;
        tinfo.dst_addrs = &dma_addr;
        tinfo.src_addrs= NULL;
        tinfo.xfer_unit_size = 8;
        tinfo.dst_fragments = 1;

        dev->sdmafuncs.setup_xfer(dev->rx_dma.dma_chn, &tinfo);
        dev->sdmafuncs.xfer_start(dev->rx_dma.dma_chn);

        slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO, "sermx1: DMA is enabled for device %s", dev->tty.name);
    }
    else
    {
        slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO, "sermx1: DMA is disabled for device %s", dev->tty.name);
    }
#endif
    return;
#if USE_DMA
fail6:
    dev->sdmafuncs.channel_release(dev->rx_dma.dma_chn);
fail5:
    dev->sdmafuncs.fini();
fail4:
    my_detach_pulse(&dev->rx_dma.pulse);
    my_detach_pulse(&dev->tx_dma.pulse);
    munmap(dev->rx_dma.buf, dev->rx_dma.xfer_size);
fail3:
    munmap(dev->tx_dma.buf, dev->tx_dma.xfer_size);
fail2:
    munmap_device_io(dev->base, MX1_UART_SIZE);
#endif
fail1:
    free(dev->tty.obuf.buff);
    free(dev->tty.ibuf.buff);
    free(dev->tty.cbuf.buff);
    free (dev);
    exit(1);

}

void dinit()
{
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/devc/sermx1/init.c $ $Rev: 756995 $")
#endif
