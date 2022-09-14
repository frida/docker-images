/*
 * $QNXLicenseC: 
 * Copyright 2010, QNX Software Systems.  
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


#include "mx6q.h"

int
mx6q_process_queue(void *arg, struct nw_work_thread *wtp)
{
    mx6q_dev_t		*mx6q = arg;
    struct ifnet	*ifp = &mx6q->ecom.ec_if;
    struct mbuf		*m;

    while(1) {
	pthread_mutex_lock(&mx6q->rx_mutex);
	IF_DEQUEUE(&mx6q->rx_queue, m);
	if (m != NULL) {
	    pthread_mutex_unlock(&mx6q->rx_mutex);
	    (*ifp->if_input)(ifp, m);
	} else {
	    /* Leave mutex locked to prevent any enqueues, unlock in enable */
	    break;
	}
    }
    return 1;
}

int
mx6q_enable_queue(void *arg)
{
    mx6q_dev_t		*mx6q = arg;
    volatile uint32_t	*base = mx6q->reg;

    InterruptLock(&mx6q->spinlock);
#ifdef MX6XSLX
    if (mx6q->rx_full & (1 << 1)) {
	*(base + MX6Q_IMASK) |= IMASK_RXF1EN;
	*(base + MX6Q_R_DES_ACTIVE1) = R_DES_ACTIVE;
    }
    if (mx6q->rx_full & (1 << 2)) {
	*(base + MX6Q_IMASK) |= IMASK_RXF2EN;
	*(base + MX6Q_R_DES_ACTIVE2) = R_DES_ACTIVE;
    }
#endif
    if (mx6q->rx_full & (1 << 0)) {
	*(base + MX6Q_IMASK) |= IMASK_RFIEN;
	*(base + MX6Q_R_DES_ACTIVE) = R_DES_ACTIVE;
    }
    InterruptUnlock(&mx6q->spinlock);
    mx6q->rx_full = 0;
    mx6q->rx_running = 0;
    pthread_mutex_unlock(&mx6q->rx_mutex);

  return 1;
}

//
// this is the interrupt handler which directly masks off
// the hardware interrupt at the nic.
//
const struct sigevent *
mx6q_isr(void *arg, int iid)
{
	mx6q_dev_t		*mx6q;
	struct _iopkt_inter	*ient;
	volatile uint32_t	*base;
	uint32_t		ievent, imask;

	mx6q = arg;
	ient = &mx6q->inter;
	base = mx6q->reg;

	/* Read interrupt cause bits */
	ievent = *(base + MX6Q_IEVENT);

	/* Read current mask */
	InterruptLock(&mx6q->spinlock);
	imask = *(base + MX6Q_IMASK);

	/* Rx interrupts get masked out and a pulse to the Rx thread */
#ifdef MX6XSLX
	if (((imask & IMASK_RXF1EN) != 0) &&
	    ((ievent & IEVENT_RXF1) != 0)) {
		*(base + MX6Q_IMASK) = imask & (~IMASK_RXF1EN);
		InterruptUnlock(&mx6q->spinlock);
		return &mx6q->isr_event[1];
	}
	if (((imask & IMASK_RXF2EN) != 0) &&
	    ((ievent & IEVENT_RXF2) != 0)) {
		*(base + MX6Q_IMASK) = imask & (~IMASK_RXF2EN);
		InterruptUnlock(&mx6q->spinlock);
		return &mx6q->isr_event[2];
	}
#endif
	if (((imask & IMASK_RFIEN) != 0) &&
	    ((ievent & IEVENT_RFINT) != 0)) {
		*(base + MX6Q_IMASK) = imask & (~IMASK_RFIEN);
		InterruptUnlock(&mx6q->spinlock);
		return &mx6q->isr_event[0];
	}

	/* Not an Rx interrupt, mask it out and send it the normal path */
#ifdef MX6XSLX
	*(base + MX6Q_IMASK) = imask &
				(IMASK_RXF2EN | IMASK_RXF1EN | IMASK_RFIEN);
#else
	*(base + MX6Q_IMASK) = imask & IMASK_RFIEN;
#endif
	InterruptUnlock(&mx6q->spinlock);
	return interrupt_queue(mx6q->iopkt, ient);
}

/*
 * The usage of TS_AVAIL and the TXF interrupts is complicated.
 * TS_AVAIL is set on the frame transmission, TXF is set on descriptor update.
 * Without the 802.1Qav credit based shapers enabled then these are the same
 * time. With the shapers enabled, the flow for a single queue is:
 *
 * BD1 Fetch | Data1 Fetch | Tx1 | shaper wait | BD1 Update | BD2 Fetch |
 * Data 2 Fetch | Tx 2 | shaper wait | BD2 Update | BD3 Fetch...
 *
 * Freescale did it this way to improve bus performance. The problem is that
 * it separates out the TS_AVAIL interrupt from the actual value being
 * available in the buffer descriptor. The value is available in ENET_ATSTMP
 * but it would be complex to correlate that with the appropriate PTP frame,
 * especially if PTP was being transmitted on multiple queues. This means that
 * we need to use the TXF interrupt to pick up the timestamp value from the
 * updated buffer descriptor.
 */

int
mx6q_enable_interrupt(void *arg)
{
	mx6q_dev_t		*mx6q = arg;
	volatile uint32_t	*base = mx6q->reg;

	InterruptLock(&mx6q->spinlock);
#ifdef MX6XSLX
	*(base + MX6Q_IMASK) |= (IMASK_TS_TIMER | IMASK_TFIEN |
				 IMASK_TXF1EN | IMASK_TXF2EN);
#else
	*(base + MX6Q_IMASK) |= (IMASK_TS_TIMER | IMASK_TFIEN);
#endif
	InterruptUnlock(&mx6q->spinlock);

	return 1;
}

int
mx6q_process_interrupt(void *arg, struct nw_work_thread *wtp)
{
	mx6q_dev_t		*mx6q = arg;
	volatile uint32_t	*base = mx6q->reg;
	uint32_t		ievent;
	int			idx;
	volatile mpc_bd_t	*bd;


	for (;;) {

		pthread_mutex_lock(&mx6q->mutex);

		/* Read and clear interrupt cause bits, ignoring Rx */
#ifdef MX6XSLX
		ievent = *(base + MX6Q_IEVENT) & ~(IEVENT_RXF2 | IEVENT_RXF1 |
						   IEVENT_RFINT);
#else
		ievent = *(base + MX6Q_IEVENT) & ~(IEVENT_RFINT);
#endif
		*(base + MX6Q_IEVENT) = ievent;

		/* Do the timer wrap interrupt early so we can unlock the mutex */
		if ((ievent & IEVENT_TS_TIMER) != 0) {
			mx6q->rtc++;
		}
		pthread_mutex_unlock(&mx6q->mutex);

		if (mx6q->cfg.verbose > 6) {
			log(LOG_ERR, "%s(): ievent 0x%x",
			    __FUNCTION__, ievent);
		}

		if (!ievent) {
			break;
		}

#ifdef MX6XSLX
		if ((ievent & IEVENT_TXF1) != 0) {
		    if (mx6q->ts_waiting[1]) {
			mx6q_transmit_complete(mx6q, 1);
		    }
		    /* If mis-queued, kick the DMA engine */
		    if (*(base + MX6Q_X_DES_ACTIVE1) == 0) {
			idx = mx6q->tx_cidx[1];
			while (idx != mx6q->tx_pidx[1]) {
				bd = &mx6q->tx_bd[idx +
				     (1 * mx6q->num_tx_descriptors)];
			    if (bd->status & TXBD_R) {
				*(base + MX6Q_X_DES_ACTIVE1) =  X_DES_ACTIVE;
				break;
			    }
			    idx = NEXT_TX(idx);
			}
		    }
		}

		if ((ievent & IEVENT_TXF2) != 0) {
		    if (mx6q->ts_waiting[2]) {
			mx6q_transmit_complete(mx6q, 2);
		    }
		    /* If mis-queued, kick the DMA engine */
		    if (*(base + MX6Q_X_DES_ACTIVE2) == 0) {
			idx = mx6q->tx_cidx[2];
			while (idx != mx6q->tx_pidx[2]) {
				bd = &mx6q->tx_bd[idx +
				     (2 * mx6q->num_tx_descriptors)];
			    if (bd->status & TXBD_R) {
				*(base + MX6Q_X_DES_ACTIVE2) =  X_DES_ACTIVE;
				break;
			    }
			    idx = NEXT_TX(idx);
			}
		    }
		}
#endif
		if ((ievent & IEVENT_TFINT) != 0) {
		    if (mx6q->ts_waiting[0]) {
			mx6q_transmit_complete(mx6q, 0);
		    }
		    NW_SIGLOCK_P(&mx6q->ecom.ec_if.if_snd_ex,
				 mx6q->iopkt, WTP);
		    /*
		     * If Tx complete on queue 0 and was out of
		     * descriptors then call start to reap and Tx more.
		     */
		    if (mx6q->ecom.ec_if.if_flags_tx & IFF_OACTIVE) {
			mx6q_start(&mx6q->ecom.ec_if);
		    } else {
			/* If mis-queued, kick the DMA engine */
			if (*(base + MX6Q_X_DES_ACTIVE) == 0) {
			    idx = mx6q->tx_cidx[0];
			    while (idx != mx6q->tx_pidx[0]) {
				bd = &mx6q->tx_bd[idx];
				if (bd->status & TXBD_R) {
				    *(base + MX6Q_X_DES_ACTIVE) =  X_DES_ACTIVE;
				    break;
				}
				idx = NEXT_TX(idx);
			    }
			}
			NW_SIGUNLOCK_P(&mx6q->ecom.ec_if.if_snd_ex,
				       mx6q->iopkt, WTP);
		    }
		}
	}

	return 1;
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/io-pkt/sys/dev_qnx/mx6x/event.c $ $Rev: 764005 $")
#endif
