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

/*
 * Disable DMA after 40 consecutive DMA TX transfer timeouts.
 * 40 * 50ms delay = 2000ms.
 */
#ifdef USE_DMA
#define MAX_TIMEOUT (40)
#endif

/* convert reference clock divider to bit value which can be programmed to UFCR register */
#define REF_CLK_DIV_REG(x)	(((x) < 7 ? 6 - (x) : 6) << 7)

#define MIN_REF_FRQ_DIV (1)
#define MAX_REF_FRQ_DIV	(7)

#define UINT_DIFFERENCE(a,b)	((a > b) ? (a-b) : (b-a))

/* Round last digit during division */
#define DIVIDE_AND_ROUND(A,B)    (((A) + ((B)-1))/(B))

int
tto(TTYDEV *ttydev, int action, int arg1)
{
    TTYBUF           *bup = &ttydev->obuf;
    DEV_MX1          *dev = (DEV_MX1 *)ttydev;
    uintptr_t        base = dev->base;
    unsigned char    c;
    unsigned         cr1;
#ifdef USE_DMA
    static uint32_t  byte_cnt = 0;
    dma_transfer_t   tinfo;
    dma_addr_t       dma_addr;
#endif
    switch (action) {
        case TTO_STTY:
            ser_stty(dev);
            return 0;

        case TTO_CTRL:
            if (arg1 & _SERCTL_BRK_CHG) {
                cr1 = in32(base + MX1_UART_CR1);

                if (arg1 &_SERCTL_BRK)
                    cr1 |= MX1_UCR1_SNDBRK;
                else
                    cr1 &= ~MX1_UCR1_SNDBRK;

                out32(base + MX1_UART_CR1, cr1);
            }

            /*
             * Modem ctrl
             */
            if (arg1 & _SERCTL_DTR_CHG) {
                cr1 = in32(base + MX1_UART_CR3);

                if (arg1 & _SERCTL_DTR)
                    cr1 |= MX1_UCR3_DSR;
                else
                    cr1 &= ~MX1_UCR3_DSR;

                out32(base + MX1_UART_CR3, cr1);
            }

            /*
             * RTS Control
             */
            if (arg1 & _SERCTL_RTS_CHG) {
                if(!dev->usedma) {
                    /*
                     * Enable/disable RX interrupts to assert/clear input HW flow control
                     */
                    if (arg1 & _SERCTL_RTS) {
                        out32(base + MX1_UART_CR1, in32(base + MX1_UART_CR1) | (MX1_UCR1_RRDYEN));
                        out32(dev->base + MX1_UART_CR2, in32(dev->base + MX1_UART_CR2) | MX1_UCR2_ATEN);
                    }
                    else {
                        out32(base + MX1_UART_CR1, in32(base + MX1_UART_CR1) & ~(MX1_UCR1_RRDYEN));
                        out32(dev->base + MX1_UART_CR2, in32(dev->base + MX1_UART_CR2) & ~(MX1_UCR2_ATEN));
                    }
                }
#if USE_DMA
                else if (dev->usedma)
                {
                    if (arg1 & _SERCTL_RTS)
                    {
                        atomic_clr (&dev->tty.flags, IHW_PAGED);

                        // ibuf has more space. Resume receive
                        if(dev->rx_dma.buffer0)
                        {
                            dev->rx_dma.status |= tti2(&dev->tty, (unsigned char *)dev->rx_dma.buf, dev->rx_dma.bytes_read, dev->rx_dma.key);
                            memset(&tinfo, 0, sizeof(tinfo));
                            tinfo.xfer_bytes = dma_addr.len = dev->rx_dma.xfer_size;
                            dma_addr.paddr = dev->rx_dma.phys_addr + dev->rx_dma.xfer_size;
                            tinfo.dst_addrs = &dma_addr;
                            tinfo.src_addrs= NULL;
                            tinfo.xfer_unit_size = 8;
                            tinfo.dst_fragments = 1;

                            dev->rx_dma.buffer0 ^= 1;
                            dev->sdmafuncs.setup_xfer(dev->rx_dma.dma_chn, &tinfo);
                            dev->sdmafuncs.xfer_start(dev->rx_dma.dma_chn);
                        }
                        else
                        {
                            dev->rx_dma.status |= tti2(&dev->tty, (unsigned char *)(dev->rx_dma.buf + dev->rx_dma.xfer_size), dev->rx_dma.bytes_read, dev->rx_dma.key);
                            memset(&tinfo, 0, sizeof(tinfo));
                            tinfo.xfer_bytes = dma_addr.len = dev->rx_dma.xfer_size;
                            dma_addr.paddr = dev->rx_dma.phys_addr;
                            tinfo.dst_addrs = &dma_addr;
                            tinfo.src_addrs= NULL;
                            tinfo.xfer_unit_size = 8;
                            tinfo.dst_fragments = 1;

                            dev->rx_dma.buffer0 ^= 1;
                            dev->sdmafuncs.setup_xfer(dev->rx_dma.dma_chn, &tinfo);
                            dev->sdmafuncs.xfer_start(dev->rx_dma.dma_chn);
                        }

                        if (dev->rx_dma.status)
                        {
                            iochar_send_event (&dev->tty);
                        }
                    }
                    // else if ibuf is almost full do nothing since RX DMA handler will not
                    // trigger a new transfer if there is no room in the ibuf.
                }
#endif
            }
            return 0;

        case TTO_LINESTATUS:
            return ((in32(base + MX1_UART_SR1) & 0xFFFF) | (in32(base + MX1_UART_SR2)) << 16);

        case TTO_DATA:
        case TTO_EVENT:
            break;

        default:
            return 0;
    }


    if(dev->usedma)
    {
#if USE_DMA
        if (dev->tx_xfer_active)
        {
            /* If TX timer expires while DMA transfer is active the DMA transfer failed to complete */
            if ((action == TTO_DATA) && (dev->tty.un.s.tx_tmr == 0))
            {

                slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_ERROR,
                    "%s: DMA TX transfer timed out, disabling DMA", __FUNCTION__);
                dev_cleanup((TTYDEV *)dev);
                dev->usedma = FALSE;
                dev->tx_xfer_active = FALSE;

                /* Re-enable io-char TX timer mechanism */
                dev->tty.flags |= LOSES_TX_INTR;
            }
             else
                /* TX DMA transfer is active so wait for the transfer to complete before starting a new transfer */
                return 0;
        }
        else
        {

            while(bup->cnt > 0 && byte_cnt < dev->tx_dma.xfer_size)
            {
                if (dev->tty.flags & (OHW_PAGED|OSW_PAGED) && !(dev->tty.xflags & OSW_PAGED_OVERRIDE)){
                    break;
                }

                if (dev->tty.c_oflag & OPOST || dev->tty.xflags & OSW_PAGED_OVERRIDE)
                {

                    /*
                     * Get the next character to print from the output buffer
                     */
                    dev_lock(&dev->tty);
                    c = tto_getchar(&dev->tty);
                    dev_unlock(&dev->tty);
                    dev->tx_dma.buf[byte_cnt++] = c;

                    /*
                     * Clear the OSW_PAGED_OVERRIDE flag as we only want
                     * one character to be transmitted in this case.
                     */
                    if (dev->tty.xflags & OSW_PAGED_OVERRIDE)
                        atomic_clr (&dev->tty.xflags, OSW_PAGED_OVERRIDE);

                }
                else
                {
                    int buf_n, first_pass;
                    dev_lock(&dev->tty);
                    buf_n = min(bup->cnt, dev->tx_dma.xfer_size - byte_cnt);
                    first_pass = &bup->buff[bup->size] - bup->tail;
                    if(buf_n <= first_pass)
                    {
                        memcpy(dev->tx_dma.buf + byte_cnt, bup->tail, buf_n);
                        bup->tail += buf_n;
                        if(bup->tail == &bup->buff[bup->size])
                            bup->tail = bup->buff;
                    }
                    else
                    {
                        memcpy(dev->tx_dma.buf + byte_cnt, bup->tail, first_pass);
                        memcpy(dev->tx_dma.buf + byte_cnt + first_pass, bup->buff, buf_n - first_pass);
                        bup->tail = bup->buff + (buf_n - first_pass);
                    }
                    bup->cnt -= buf_n;
                    byte_cnt += buf_n;
                    dev_unlock (&dev->tty);
                }
            }
            dev_lock(&dev->tty);
            if(byte_cnt && !(dev->tty.flags & (OHW_PAGED | OSW_PAGED)))
            {
                /* Configure DMA buffer address and transfer size */
                memset(&tinfo, 0, sizeof(tinfo));
                tinfo.xfer_bytes = dma_addr.len = byte_cnt;
                dma_addr.paddr = dev->tx_dma.phys_addr;
                tinfo.src_addrs = &dma_addr;
                tinfo.dst_addrs = NULL;
                tinfo.xfer_unit_size = 8;
                tinfo.src_fragments = 1;

                dev->sdmafuncs.setup_xfer(dev->tx_dma.dma_chn, &tinfo);

                /* Set up timer to call 'tto' if TX DMA transfer times out */
                dev->tty.un.s.tx_tmr = MAX_TIMEOUT;
                timer_queue(&dev->tty);
                dev->tx_xfer_active = TRUE;
                dev->sdmafuncs.xfer_start(dev->tx_dma.dma_chn);

                byte_cnt = 0;
            }
            dev_unlock(&dev->tty);
        }

#endif
    }
    if (!dev->usedma)
    {
        while (bup->cnt > 0 && (in32(base + MX1_UART_SR1) & MX1_USR1_TRDY))
        {
            /*
             * If the OSW_PAGED_OVERRIDE flag is set then allow
             * transmit of character even if output is suspended via
             * the OSW_PAGED flag. This flag implies that the next
             * character in the obuf is a software flow control
             * charater (STOP/START).
             * Note: tx_inject sets it up so that the contol
             *       character is at the start (tail) of the buffer.
             */
            if (dev->tty.flags & (OHW_PAGED|OSW_PAGED) && !(dev->tty.xflags & OSW_PAGED_OVERRIDE))
                break;

            /*
             * Get the next character to print from the output buffer
             */
            dev_lock(&dev->tty);
            c=tto_getchar(&dev->tty);
            dev_unlock(&dev->tty);

            dev->tty.un.s.tx_tmr = 3;        /* Timeout 3 */
            out32(base + MX1_UART_TXDATA, c);

            /* Clear the OSW_PAGED_OVERRIDE flag as we only want
             * one character to be transmitted in this case.
             */
            if (dev->tty.xflags & OSW_PAGED_OVERRIDE)
            {
                atomic_clr(&dev->tty.xflags, OSW_PAGED_OVERRIDE);
                break;
            }
        }
        if (!(dev->tty.flags & (OHW_PAGED|OSW_PAGED)) && bup->cnt) {

            cr1 = in32(base + MX1_UART_CR1);
            out32(base + MX1_UART_CR1, cr1 | MX1_UCR1_TRDYEN);
        }
    }

    return (tto_checkclients(&dev->tty));
}

void ser_stty(DEV_MX1 *dev)
{
    uintptr_t base = dev->base;
    unsigned cr2, ref_clk, rfdiv, fcr, bir, cr4;

    uint32_t timeout, cts_threshold;

    /*
     * Check hardware flow control setting
     * NOTE: On this hardware CTS is the output and RTS is the input.
     * Therefore the CTS output is responsible for input flow control, and the
     * RTS input is responsible for output flow control.
     */

    /* Make sure SRST is set to prevent UART from resetting */
    dev->cr2 = in32(base + MX1_UART_CR2) | MX1_UCR2_SRST;

    /* Check if we need to enable or disable auto-CTS */
    if ((dev->tty.c_cflag & IHFLOW) && !(dev->cr2 & MX1_UCR2_CTSC))
    {
        /*
         * If input flow control is enabled always enable auto-cts (i.e. the UART 
         * toggles the CTS output based on the RX/input buffer level)
         */
        dev->cr2 |= MX1_UCR2_CTSC;
        out32(base + MX1_UART_CR2, dev->cr2);
    }
    else if (!(dev->tty.c_cflag & IHFLOW) && (dev->cr2 & MX1_UCR2_CTSC))
    {
        /* If input flow control is disabled then disable auto-cts */
        dev->cr2 &= ~(MX1_UCR2_CTSC);
        out32(base + MX1_UART_CR2, dev->cr2);

        /* Assert CTS (i.e. CTS pin is low meaning receiver is ready for data) after CTSC has been cleared */
        dev->cr2 |= (MX1_UCR2_CTS);
        out32(base + MX1_UART_CR2, dev->cr2);
    }

    /* Check if the transmitter should transmit based or the RTS input, or ignore the RTS input */
    if ((dev->tty.c_cflag & OHFLOW) && (dev->cr2 & MX1_UCR2_IRTS))
    {
        dev->cr2 &= ~(MX1_UCR2_IRTS);    /* Transmit only when RTS is asserted */
        out32(base + MX1_UART_CR2, dev->cr2);

        /* In case we exit early, enable the RTS delta interrupt now */
        out32(base + MX1_UART_CR1, in32(base + MX1_UART_CR1) | MX1_UCR1_RTSDEN);
    }
    else if (!(dev->tty.c_cflag & OHFLOW) && !(dev->cr2 & MX1_UCR2_IRTS))
    {
        dev->cr2 |= MX1_UCR2_IRTS;     /* Ignore RTS input pin */
        out32(base + MX1_UART_CR2, dev->cr2);

        /* Disable the RTS Delta interrupt */
        out32(base + MX1_UART_CR1, in32(base + MX1_UART_CR1) & ~MX1_UCR1_RTSDEN);
    }

    dev->cr2 = cr2 = in32(base + MX1_UART_CR2);
    /*
     * Calculate baud rate divisor, data size, stop bits and parity
     */
    ref_clk = dev->clk;

    /*
     * Determine the highest allowable Reference Frequency Divider (rfdiv)
     */
    rfdiv = ref_clk / (dev->tty.baud * 16);

    if (rfdiv > MAX_REF_FRQ_DIV)
        rfdiv = MAX_REF_FRQ_DIV;

    if (rfdiv < MIN_REF_FRQ_DIV)
        rfdiv = MIN_REF_FRQ_DIV;

    unsigned best_baud, best_diff, best_bir, current_diff;
    long long unsigned current_baud;

    /* Set initial value to highest allowable value so that 'best_baud' will initially be set to first baud rate */
    best_diff = ULONG_MAX;

    /* These values will always be overwritten */
    best_baud = best_bir = 0;
   
    /* 
     * Loop through all allowable rfdiv values to determine the best rfdiv value 
     * 
     * As per the i.MX Reference Manuals, the baud rate formula is:
     *
     * Ref Freq = module clock (specified via '-c' driver parameter) / rfdiv
     * BaudRate = Ref Freq / (16 * (UBMR+1)/(UBIR+1) )
     *
     * To simplify the baud rate calculation we set UBMR+1 to 10000.
     *
     * We also use two methods of improving accuracy due to integer division:
     * 1) Round result after division instead of C's default truncation.
     * 2) Calculate baud rate using long long (64 bit) and multiply by 100000/100000
     * prior to division to improve accuracy.
     */
    int i;
    for (i=rfdiv; i>=MIN_REF_FRQ_DIV; i--)
    {
        ref_clk = dev->clk / i;
        bir = DIVIDE_AND_ROUND((dev->tty.baud * 16), DIVIDE_AND_ROUND(ref_clk, 10000));

        /* baud rate = reference clock / (16 * (bmr/bir)     bmr is hard coded to 10000 */
        current_baud = DIVIDE_AND_ROUND(((long long unsigned)ref_clk * 100000),
        (long long unsigned)(16 * DIVIDE_AND_ROUND((10000*100000),bir)));
        
        current_diff = UINT_DIFFERENCE(current_baud, dev->tty.baud);

	if (current_diff < best_diff)
        {
            best_baud = current_baud;
            best_diff = current_diff;
            best_bir = bir;
            rfdiv = i;
        }

        /* If we have an exact match break out of loop */
        if (current_baud == dev->tty.baud)
            break;

    }

   /* Use rfdiv value which produces the most accurate baud rate */
    ref_clk = dev->clk / rfdiv;

    /* 
     * The UART's UFCR register has a strange way of specifying the rfdiv fields bits
     * so we use a macro to write rfdiv correctly, see appropriate i.MX Reference Manual
     * for more details.
     */
    fcr = (dev->fifo & 0xFC3F) | REF_CLK_DIV_REG(rfdiv);

    /* Need to program numerator-1 into UBIR register */
    bir = best_bir - 1;

     /* Uncomment for debugging */
#if 0
     printf("requested baud rate: %ubps, (approximate) actual baud rate = %u, rfdiv: %u, UBMR+1: %d, UBIR+1: %d\n", dev->tty.baud,
	 best_baud, rfdiv, 9999+1, bir+1);
#endif

    switch (dev->tty.c_cflag & CSIZE) {
        case CS8:
            cr2 |= MX1_UCR2_WS;
            break;

        case CS7:
            cr2 &= ~MX1_UCR2_WS;
            break;
    }

    if (dev->tty.c_cflag & CSTOPB)
        cr2 |= MX1_UCR2_STPB;
    else
        cr2 &= ~MX1_UCR2_STPB;

    cr2 &= ~(MX1_UCR2_PREN | MX1_UCR2_PROE);
    if (dev->tty.c_cflag & PARENB) {
        cr2 |= MX1_UCR2_PREN;
        if (dev->tty.c_cflag & PARODD)
            cr2 |= MX1_UCR2_PROE;
    }

    if ((dev->fcr == fcr) && (dev->cr2 == cr2) && (dev->bir == bir))
        return;

    dev->fcr = fcr;
    dev->cr2 = cr2;
    dev->bir = bir;

    /*
     * Wait for Tx FIFO and shift register empty if the UART is enabled
     */
    timeout = 100000;
    if ((in32(base + MX1_UART_CR1) & (MX1_UCR1_UARTEN)) == (MX1_UCR1_UARTEN)) {
        if (in32(base + MX1_UART_CR2) & MX1_UCR2_TXEN) {
            while (!(in32(base + MX1_UART_SR2) & MX1_USR2_TXDC) && timeout--)
                ;
        }
    }

    /*
     * Reset UART - i.e. reset transmit, receive state machines, FIFOs and
     * USR1, USR2, UBIR, UBMR, UBRC, URXD, UTXD and UTS[6-3] registers. Once reset
     * is complete the SRST bit will automatically be set.
     */
    out32(base + MX1_UART_CR2, in32(base + MX1_UART_CR2) & ~(MX1_UCR2_SRST));    
    timeout = 0;
    while (!(in32(base + MX1_UART_CR2) & MX1_UCR2_SRST)) {
        timeout++;
        if (timeout >= 100)
            break;
	}
    
    if (dev->mx1) {
        out32(base + MX1_UART_CR3, 0);
        /* Reference clock is 16MHz */
        out32(base + MX1_UART_CR4, 0x8000 | MX1_UCR4_REF16);
    }
    else {
        /* Program RXD muxed input */
        out32(base + MX1_UART_CR3, 4);
	out32(base + MX1_UART_CR4, 0);
    }

    /* Set CTS threshold to RX FIFO threshold + 1 */
    cts_threshold = (dev->fifo & MX1_UFCR_RXTL_MASK) + 1;
    cts_threshold = (cts_threshold <= 32) ? cts_threshold : 32;
    cr4 = in32(base + MX1_UART_CR4);
    cr4 &= ~(MX1_UCR4_CTSTL_MASK);
    cr4 |= (cts_threshold << 10);
    out32(base + MX1_UART_CR4, cr4);

    out32(base + MX1_UART_FCR, fcr);

    /* program ONEMS register for MX21/MX31/MX35 */
    if (!dev->mx1)
    {
           out32(base + MX1_UART_BIPR1, ref_clk / 1000);
    }

    /* Note that the UBIR register MUST be updated before the UBMR register! */
    out32(base + MX1_UART_BIR, bir);
    out32(base + MX1_UART_BMR, 9999);

    /* Enable UART and Receiver Ready Interrupt */
    if(dev->usedma)
        out32(base + MX1_UART_CR1, MX53_UART_UCR1_ATDMAEN | MX1_UCR1_UARTEN | MX1_UCR1_RDMAEN | MX1_UCR1_TDMAEN);
    else
    {
        /*
         * If receiver is enabled but RX FIFO interrupt is disabled then enable the RX FIFO
         * interrupt and the aging timer interrupt
         */
        if ( (dev->tty.c_cflag & CREAD) && !(in32(base + MX1_UART_CR1) & MX1_UCR1_RRDYEN))
        {
            out32(base + MX1_UART_CR1, in32(base + MX1_UART_CR1) | MX1_UCR1_RRDYEN);
	     cr2 |= MX1_UCR2_ATEN;
        }

        /*
         * If receiever is disabled and RX FIFO interrupt is enabled then disable RX FIFO interrupt
         * and aging timer interrupt
         */
        else if ( !(dev->tty.c_cflag & CREAD) && (in32(base + MX1_UART_CR1) & MX1_UCR1_RRDYEN))
        {
            out32(base + MX1_UART_CR1, in32(base + MX1_UART_CR1) & ~(MX1_UCR1_RRDYEN));
	     cr2 &= ~MX1_UCR2_ATEN;
        }
        /* Otherwise nothing to do */
    }


    /* If flow control is enabled then enable the RTS Delta interrupt
     * NOTE: We need to re-enable here after the above UART reset+disable
     *          which clears all interrupts including RTSD
     */
    if (dev->tty.c_cflag & OHFLOW)
        out32(base + MX1_UART_CR1, in32(base + MX1_UART_CR1) | MX1_UCR1_RTSDEN);

    /* Enable Tx/Rx */
    out32(base + MX1_UART_CR2, cr2 | MX1_UCR2_TXEN | MX1_UCR2_RXEN | MX1_UCR2_SRST);
}

int
drain_check (TTYDEV * ttydev, uintptr_t * count)
{
    TTYBUF *bup = &ttydev->obuf;
    DEV_MX1 *dev = (DEV_MX1 *) ttydev;
    uint32_t drain_size = 0;

    if(dev->usedma)
    {
#if USE_DMA
        // if the device has DRAINED, return 1
        if((bup->cnt == 0) && (dev->sdmafuncs.bytes_left(dev->tx_dma.dma_chn) == 0) &&
                (in32(dev->base + MX1_UART_SR2) & MX1_USR2_TXDC))
            return 1;
        // set drain_size
        drain_size = DMA_XFER_SIZE + FIFO_SIZE;
#endif
    }
    else
    {
        // if the device has DRAINED, return 1
        if((bup->cnt == 0) && (in32(dev->base + MX1_UART_SR2) & MX1_USR2_TXDC))
            return 1;
        // set drain_size
        drain_size = FIFO_SIZE;
    }


    // if the device has not DRAINED, set a timer based on 50ms counts
    // wait for the time it takes for one character to be transmitted
    // out the shift register.  We do this dynamically since the
    // baud rate can change.
    if (count != NULL)
        *count =
            (ttydev->baud ==
            0) ? 0 : ((IO_CHAR_DEFAULT_BITSIZE * drain_size) / ttydev->baud) + 1;
    return 0;
}





#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/devc/sermx1/tto.c $ $Rev: 763889 $")
#endif
