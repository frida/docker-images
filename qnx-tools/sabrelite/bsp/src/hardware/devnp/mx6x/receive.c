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


#include "bpfilter.h"

#include "mx6q.h"
#include <net/if_vlanvar.h>
#include <netinet/in.h>

#ifdef MX6XSLX
#include <avb.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

static void
mx6q_add_pkt (mx6q_dev_t *mx6q, struct mbuf *new, int idx)
{
    mpc_bd_t	*bd = &mx6q->rx_bd[idx];
    uint16_t	status = RXBD_E;
    off64_t	phys = pool_phys(new->m_data, new->m_ext.ext_page);

    CACHE_FLUSH(&mx6q->cachectl, new->m_data, phys, new->m_ext.ext_size);

    // set wrap bit if on last rx descriptor
    if ((idx % mx6q->num_rx_descriptors) == (mx6q->num_rx_descriptors - 1)) {
	status |= RXBD_W;
    }

    // remember mbuf pointer for this rx descriptor
    mx6q->rx_pkts[idx] = new;

    // stuff rx descriptor
    bd->buffer = (uint32_t)phys;
    bd->length = 0;
    bd->status = status;
}

int
mx6q_receive (mx6q_dev_t *mx6q, struct nw_work_thread *wtp, uint8_t queue)
{
    static struct mbuf		*rpkt[NUM_RX_QUEUES];
    static struct mbuf		*rpkt_tail[NUM_RX_QUEUES];
    static uint32_t		length[NUM_RX_QUEUES];
    struct mbuf			*new;
    ptpv2hdr_t			*ph;
    uint32_t			this_idx, offset, len;
    uint16_t			status;
    mpc_bd_t			*rx_bd;
    struct ifnet		*ifp = &mx6q->ecom.ec_if;
    volatile uint32_t		*base = mx6q->reg;
#ifdef MX6XSLX
    struct ether_vlan_header	*vlan_hdr;
#endif
    const struct sigevent	*evp;


    // probe phy optimization - rx pkt activity
    mx6q->rxd_pkts = 1;

    offset = queue * mx6q->num_rx_descriptors;

    for(;;) {

	if (mx6q->cfg.verbose > 5) {
	    log(LOG_ERR, "%s(): rx_cidx %d queue %d",
		__FUNCTION__, mx6q->rx_cidx[queue], queue);
	}

	// is there an rxd packet at the next descriptor?
	this_idx	= mx6q->rx_cidx[queue];
	rx_bd		= &mx6q->rx_bd[offset + this_idx];
	status		= rx_bd->status;
	if (status & RXBD_E) {
	    break;
	}

	// update rx descriptor consumer index for next loop iteration
	mx6q->rx_cidx[queue] = NEXT_RX(this_idx);
	if (mx6q->rx_cidx[queue] == 0) {
	    mx6q_update_stats(mx6q);
	}

	// any problems with this rxd packet?
	if (status & RXBD_ERR) {
	    // give old packet back to nic
	    mx6q_add_pkt(mx6q, mx6q->rx_pkts[offset + this_idx],
			 offset + this_idx);
	    log(LOG_ERR, "%s(): status RXBD_ERR 0x%X", __FUNCTION__, status);
	    ifp->if_ierrors++;
	    if (rpkt[queue] != NULL) {
		/* Half way through a packet, discard it all */
		m_freem(rpkt[queue]);
		rpkt[queue] = rpkt_tail[queue] = NULL;
		length[queue] = 0;
	    }
	    continue;
	}

	// get an empty mbuf
	new = m_getcl_wtp(M_DONTWAIT, MT_DATA, M_PKTHDR, wtp);
	if (new == NULL) {
	    // give old mbuf back to nic
	    mx6q_add_pkt(mx6q, mx6q->rx_pkts[offset + this_idx],
			 offset + this_idx);
	    log(LOG_ERR, "%s(): mbuf alloc failed!", __FUNCTION__);
	    mx6q->stats.rx_failed_allocs++;
	    ifp->if_ierrors++;
	    if (rpkt[queue] != NULL) {
		/* Half way through a packet, discard it all */
		m_freem(rpkt[queue]);
		rpkt[queue] = rpkt_tail[queue] = NULL;
		length[queue] = 0;
	    }
	    continue;
	}

	CACHE_INVAL(&mx6q->cachectl, mx6q->rx_pkts[offset + this_idx]->m_data,
		    rx_bd->buffer,
		    mx6q->rx_pkts[offset + this_idx]->m_ext.ext_size);

	// pull rxd packet out of corresponding queue
	if (rpkt[queue] == NULL) {
	    rpkt[queue] = mx6q->rx_pkts[offset + this_idx];
	    rpkt_tail[queue] = rpkt[queue];
	} else {
	    rpkt_tail[queue]->m_next = mx6q->rx_pkts[offset + this_idx];
	    rpkt_tail[queue] = rpkt_tail[queue]->m_next;
	}
	mx6q->rx_pkts[offset + this_idx] = NULL;

	/* If last then this length gets fixed later */
	rpkt_tail[queue]->m_len = rx_bd->length;

	// dump frag if user requested it with verbose=8
	if (mx6q->cfg.verbose > 7) {
	    len = rpkt_tail[queue]->m_len;
	    if (status & RXBD_L) {
		len -= length[queue];
	    }

	    log(LOG_ERR,"Rxd dev_idx %d bytes %d\n",
		mx6q->cfg.device_index, len);
	    dump_mbuf(rpkt_tail[queue], min(len,80));
	}

	// give new mbuf to nic - modifies what rx_bd points to!!
	mx6q_add_pkt(mx6q, new, offset + this_idx);

	if (status & RXBD_L) {
	    rpkt[queue]->m_pkthdr.rcvif = ifp;
	    /* Fix the lengths */
	    rpkt[queue]->m_pkthdr.len = rpkt_tail[queue]->m_len;
	    rpkt_tail[queue]->m_len -= length[queue];
	    rpkt[queue]->m_flags |= M_HASFCS;
#if NBPFILTER > 0
	    /* Pass this up to any BPF listeners. */
	    if (ifp->if_bpf) {
	      bpf_mtap(ifp->if_bpf, rpkt[queue]);
	    }
#endif
	    if (mx6q_ptp_is_eventmsg(rpkt[queue], &ph)) {
		mx6q_ptp_add_rx_timestamp(mx6q, ph, rx_bd);
	    }

	    /* finally, pass this received pkt up */
	    ifp->if_ipackets++;

#ifdef MX6XSLX
	    vlan_hdr = mtod(rpkt[queue], struct ether_vlan_header*);
	    if ((ntohs(vlan_hdr->evl_encap_proto) == ETHERTYPE_VLAN) &&
		(ntohs(vlan_hdr->evl_proto) == ETHERTYPE_1722)) {
		/* 1722 packet, send it straight up for minimum latency */
		(*ifp->if_input)(ifp, rpkt[queue]);

		/* Reset for the next packet */
		rpkt[queue] = rpkt_tail[queue] = NULL;
		length[queue] = 0;
	    } else 
#endif	    
	    {
		/*
		 * Send it up from a stack thread so bridge and
		 * fastforward work. Without this we get logs of "no flow"
		 */
		pthread_mutex_lock(&mx6q->rx_mutex);
		if (IF_QFULL(&mx6q->rx_queue)) {
		    m_freem(rpkt[queue]);
		    ifp->if_ierrors++;
		    mx6q->stats.rx_failed_allocs++;
		} else {
		    IF_ENQUEUE(&mx6q->rx_queue, rpkt[queue]);
		    if (!mx6q->rx_running) {
			mx6q->rx_running = 1;
			evp = interrupt_queue(mx6q->iopkt, &mx6q->inter_queue);
			if (evp != NULL) {
			    MsgSendPulse(evp->sigev_coid, evp->sigev_priority,
					 evp->sigev_code,
					 (int)evp->sigev_value.sival_ptr);
			}
		    }
		}

		/* Reset for the next packet */
		rpkt[queue] = rpkt_tail[queue] = NULL;
		length[queue] = 0;

		if ((mx6q->flow & (IFM_ETH_TXPAUSE | IFM_FLOW)) &&
		    (mx6q->rx_queue.ifq_len >=
		     (mx6q->rx_queue.ifq_maxlen - 1))) {
		    /*
		     * Flow control is enabled and we are about to overflow
		     * the queue. Stop receiving and return 0 so the interrupt
		     * isn't enabled until the queue is drained. This will
		     * force the RxFIFO to fill and the MAC to send a pause
		     * frame.
		     */
		    mx6q->rx_full |= 1 << queue;
		    pthread_mutex_unlock(&mx6q->rx_mutex);
		    return 0;
		}
		pthread_mutex_unlock(&mx6q->rx_mutex);
	    }

	} else {
	    length[queue] += rpkt_tail[queue]->m_len;
	}
    }
    // If descriptors were full we've now cleared space, restart receive
    switch(queue) {
    case 0:
	*(base + MX6Q_R_DES_ACTIVE) = R_DES_ACTIVE;
	break;
    case 1:
	*(base + MX6Q_R_DES_ACTIVE1) = R_DES_ACTIVE;
	break;
    case 2:
	*(base + MX6Q_R_DES_ACTIVE2) = R_DES_ACTIVE;
	break;
    default:
	log(LOG_ERR, "Bad queue %d", queue);
	break;
    }
    return 1;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/io-pkt/sys/dev_qnx/mx6x/receive.c $ $Rev: 762005 $")
#endif
