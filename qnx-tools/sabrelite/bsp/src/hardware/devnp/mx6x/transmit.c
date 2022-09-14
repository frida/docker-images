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
#ifdef MX6XSLX
#include <avb.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif


//
// this function is called only if the packet is ridiculously
// fragmented.  If we are lucky, the entire packet will fit into
// one cluster so we manually defrag.  However, if this is a jumbo
// packet, and io-pkt is running without large values for mclbytes
// and pagesize, we are left with no choice but to dup the packet
//
static struct mbuf *
mx6q_defrag(struct mbuf *m)
{
	struct mbuf *m2;
	int num_frag;

	if (m->m_pkthdr.len > MCLBYTES) {
		//
		// this is a jumbo packet which just happens to
		// be larger than a single cluster, which means
		// that the normal stuff below wont work.  So,
		// we try a "deep" copy of this packet, which is
		// time-consuming, but is probably better than
		// just dropping the packet
		//

		m2 = m_dup(m, 0, m->m_pkthdr.len, M_DONTWAIT);
		m_freem(m);
		if (!m2) {
			return NULL;
		}
		m = m2;  // m is now the defragged packet

		// ensure that dupd packet is not excessively fragmented
		for (num_frag=0; m2 != NULL ; num_frag++) {
			m2 = m2->m_next;
		}
		if (num_frag > MX6Q_MAX_FRAGS) {
			// this should never happen
			m_freem(m);
			return NULL;
		}
		return m;
	}

	// the entire packet should fit into one cluster
	m2 = m_getcl(M_NOWAIT, MT_DATA, M_PKTHDR);
	if (m2 == NULL) {
		m_freem(m);
		return NULL;
	}

	m_copydata(m, 0, m->m_pkthdr.len, mtod(m2, caddr_t));
	m2->m_pkthdr.len = m2->m_len = m->m_pkthdr.len;

	m_freem(m);

	return m2;
}

int mx6q_tx (mx6q_dev_t *mx6q, struct mbuf *m, uint8_t queue)
{
    struct mbuf		*m2;
    volatile uint32_t	*base = mx6q->reg;
    volatile mpc_bd_t	*tx_bd = 0;
    volatile mpc_bd_t	*tx_bd_first = 0;
    uint32_t		idx, num_frag, offset, ts_needed = 0;
    struct ifnet	*ifp = &mx6q->ecom.ec_if;

    /* count up mbuf fragments and fix alignment */
    num_frag = 0;
    m2 = m;
    while (m2 != NULL) {
	if (m2->m_len != 0) {
#ifndef MX6XSLX
	    /*
	     * The mx6q has an 8 byte alignment requirement for the data.
	     * If it is unaligned then forcing a defrag will copy to the
	     * start of a cluster. Clusters are on a 2k boundary and hence
	     * aligned. The mx6xslx prefers 64byte aligned for performance,
	     * but a defrag would just burn CPU anyway.
	     */
	    if ((unsigned int)m2->m_data & 7) {
		num_frag += MX6Q_MAX_FRAGS + 1;
	    } else
#endif
	    num_frag++;
	}
	m2 = m2->m_next;
    }

    // ridiculously fragmented?
    if (num_frag > MX6Q_MAX_FRAGS) {
	if ((m2 = mx6q_defrag(m)) == NULL) {
	    log(LOG_ERR, "%s(): mx6q_defrag() failed", __FUNCTION__);
	    mx6q->stats.tx_failed_allocs++;
	    ifp->if_oerrors++;
	    return ENOMEM;
	}

	// we have a new best friend
	m = m2;

	// must count mbuf fragments again
	num_frag=0;
	m2 = m;
	while (m2 != NULL) {
	    if (m2->m_len != 0) {
		num_frag++;
	    }
	    m2 = m2->m_next;
	}
    }

    if (mx6q->cfg.verbose > 6) {
	log(LOG_ERR,
	    "%s(): num_frag %d tx_pidx %d tx_cidx %d tx_descr_inuse %d num_tx_descriptors %d",
	    __FUNCTION__, num_frag, mx6q->tx_pidx[queue],
	    mx6q->tx_cidx[queue], mx6q->tx_descr_inuse[queue],
	  mx6q->num_tx_descriptors);
    }

    // dump header of txd packets if user requested it
    if (mx6q->cfg.verbose > 7) {
	log(LOG_ERR,"Txd dev_idx %d  num_frag %d  first descr bytes %d",
	    mx6q->cfg.device_index, num_frag, m->m_len);
	dump_mbuf(m, min(m->m_len, 64));
    }

    // We have a new frame, clear the timestamp flag
    ts_needed = 0;
    offset = queue * mx6q->num_tx_descriptors;


    // load up descriptors
    m2 = m;
    idx = mx6q->tx_pidx[queue];
    while (m2 != NULL) {
	// skip over zero length fragments
	if (m2->m_len == 0) {
	    m2 = m2->m_next;
	    continue;
	}

	// calc pointer to descriptor we should load
	tx_bd = &mx6q->tx_bd[idx + offset];

	if (tx_bd->status & TXBD_R) {
	    // this should NEVER happen
	    log(LOG_ERR, "%s(): Tx descriptors hit end",
		__FUNCTION__);
	    mx6q->stats.un.estats.internal_tx_errors++;
	    ifp->if_oerrors++;
	    m_freem(m);
	    NW_SIGUNLOCK(&ifp->if_snd_ex, mx6q->iopkt);
	    return EINVAL;
	}

	tx_bd->buffer = mbuf_phys(m2);
	tx_bd->length = m2->m_len;
	CACHE_FLUSH(&mx6q->cachectl, m2->m_data, tx_bd->buffer, tx_bd->length);

	if (tx_bd_first) {
	    // this is NOT first descr, set the READY bit
	    tx_bd->status |= TXBD_R;
	} else {
	    // remember location of first descr, DONT set READY bit
	    tx_bd_first = tx_bd;
	    if (mx6q_ptp_is_eventmsg(m2, NULL)) {
		// It seems, this is a PTP event message
		ts_needed = 1;
		atomic_add(&mx6q->ts_waiting[queue], 1);
	    }
	}
	/* Check if the frame must be time stamped. Only the first
	 * fragment is checked, if it contains PTP header all fragments
	 * of this frame will be time stamped too
	 */
	if (ts_needed) {
	    tx_bd->estatus |= TXBD_ESTATUS_TS;
	}

	if (mx6q->cfg.verbose > 6) {
	    log(LOG_ERR,"normal: idx %d len %d stat 0x%X",
		idx, tx_bd->length, tx_bd->status);
	}

	// we have loaded this descriptor, onto the next
	idx = NEXT_TX(idx);
	if (idx == 0) {
	    mx6q_update_stats(mx6q);
	}
	m2 = m2->m_next;
    }

    // remember the number of tx descriptors used this time
    mx6q->tx_descr_inuse[queue] += num_frag;

    // remember mbuf pointer for after tx.  For multiple descriptor
    // transmissions, middle and last descriptors have a zero mbuf ptr
    mx6q->tx_pkts[mx6q->tx_pidx[queue] + offset] = m;

    // advance producer index to next unused descriptor,
    // using modulo macro above
    mx6q->tx_pidx[queue] = idx;

    //
    // start transmission of this packet by:
    //
    // 1) setting LAST  bit in last descriptor, and
    // 2) setting READY bit in first descriptor
    //
    // at the completion of the loop, tx_bd_first
    // is set, and tx_bd points to last descr filled
    //
    if (tx_bd_first == tx_bd) {  // single descriptor

	// set LAST and READY bits
	tx_bd->status |= TXBD_R | TXBD_L | TXBD_TC;

	if (mx6q->cfg.verbose > 6) {
	    log(LOG_ERR,"enable: single: stat 0x%X", tx_bd->status);
	}
    } else { // multiple descriptors

	// set LAST bit in last descriptor we loaded
	tx_bd->status |= TXBD_L | TXBD_TC;

	// and set READY bit in first descriptor
	tx_bd_first->status |= TXBD_R;

	if (mx6q->cfg.verbose > 6) {
	    log(LOG_ERR,"enable: multi: first stat 0x%X last stat 0x%X",
		tx_bd_first->status, tx_bd->status);
	}
    }

    /* Kick the DMA */
    switch(queue) {
    case 0:
	*(base + MX6Q_X_DES_ACTIVE) = X_DES_ACTIVE;
	break;
    case 1:
	*(base + MX6Q_X_DES_ACTIVE1) = X_DES_ACTIVE;
	break;
    case 2:
	*(base + MX6Q_X_DES_ACTIVE2) = X_DES_ACTIVE;
	break;
    default:
	log(LOG_ERR, "Bad queue %d", queue);
	break;
    }

#if NBPFILTER > 0
    // Pass the packet to any BPF listeners
    if (ifp->if_bpf) {
	bpf_mtap(ifp->if_bpf, m);
    }
#endif
    return EOK;
}

void
mx6q_start(struct ifnet *ifp)
{
    mx6q_dev_t		*mx6q = ifp->if_softc;
    struct mbuf		*m;
    int			desc_avail;

    if ((ifp->if_flags_tx & IFF_RUNNING) == 0) {
	NW_SIGUNLOCK(&ifp->if_snd_ex, mx6q->iopkt);
	return;
    }

    ifp->if_flags_tx |= IFF_OACTIVE;

    for (;;) {
	desc_avail = mx6q->num_tx_descriptors - mx6q->tx_descr_inuse[0];
	if (desc_avail <= MX6Q_MAX_FRAGS) {
	    mx6q_transmit_complete(mx6q, 0);
	    desc_avail = mx6q->num_tx_descriptors - mx6q->tx_descr_inuse[0];
	    if (desc_avail <= MX6Q_MAX_FRAGS) {
		/* Leave IFF_OACTIVE so the stack doesn't call us again */
		NW_SIGUNLOCK_P (&ifp->if_snd_ex, mx6q->iopkt, WTP);
		return;
	    }
	}

	/* Grab an outbound packet/mbuf chain */
	IFQ_DEQUEUE (&ifp->if_snd, m);
	/* If none are available break out of the loop */
	if (m == NULL) {
	    break;
	}
	mx6q_tx(mx6q, m, 0);

    }

    ifp->if_flags_tx &= ~IFF_OACTIVE;
    NW_SIGUNLOCK_P (&ifp->if_snd_ex, mx6q->iopkt, WTP);
}

//
// process completed tx descriptors
//
void
mx6q_transmit_complete(mx6q_dev_t *mx6q, uint8_t queue)
{
    int			idx;
    mpc_bd_t		*bd;
    uint16_t		status;
    uint32_t		bdu, offset;
    struct mbuf		*m;
    struct ifnet	*ifp = &mx6q->ecom.ec_if;
    ptpv2hdr_t		*ph = NULL;


    if (mx6q->cfg.verbose > 5) {
	log(LOG_ERR,
	    "%s(): starting: queue %d tx_pidx %d  tx_cidx %d  tx_descr_inuse %d",
	    __FUNCTION__, queue, mx6q->tx_pidx[queue], mx6q->tx_cidx[queue],
	    mx6q->tx_descr_inuse[queue]);
    }

    offset = queue * mx6q->num_tx_descriptors;

    if (queue == 0) {
	// set flag indicating we have clean out descr ring recently
	mx6q->tx_reaped = 1;
    }

    while (mx6q->tx_pidx[queue] != mx6q->tx_cidx[queue]) {
	idx = mx6q->tx_cidx[queue];
	bd = &mx6q->tx_bd[idx + offset];

        status = bd->status;
	bdu = bd->bdu;

	if (mx6q->cfg.verbose > 6) {
	    log(LOG_ERR,"tx done: idx %d status 0x%x estatus 0x%x bdu 0x%x",
		idx, status, bd->estatus, bd->bdu);
	}

        if ((status & TXBD_R) ||
	    ((status & TXBD_L) && !(bdu & BD_BDU))) {
	    break; // nic still owns this descriptor
	}

	//
	// only FIRST descriptor will have corresponding mbuf pointer
	//
	if ((m = mx6q->tx_pkts[idx + offset])) {

	    /* Collect any timestamp */
	    if ((bd->estatus & TXBD_ESTATUS_TS) &&
		mx6q_ptp_is_eventmsg(m, &ph)) {
		mx6q_ptp_add_tx_timestamp(mx6q, ph, bd);
		atomic_sub(&mx6q->ts_waiting[queue], 1);
	    }

	    m_freem(m);
	    mx6q->tx_pkts[idx + offset] = NULL;
	    ifp->if_opackets++;
	}

	// leave only WRAP bit if was already set
	bd->status = status & TXBD_W;
	bd->estatus = queue << 20 | TXBD_ESTATUS_INT;
	bd->bdu = 0;

	// this tx descriptor is available for use now
	mx6q->tx_descr_inuse[queue]--;
        mx6q->tx_cidx[queue] = NEXT_TX(idx);
    }

    if (mx6q->cfg.verbose > 5) {
	log(LOG_ERR, "%s(): ending: tx_pidx %d tx_cidx %d tx_descr_inuse %d",
	    __FUNCTION__, mx6q->tx_pidx[queue], mx6q->tx_cidx[queue],
	    mx6q->tx_descr_inuse[queue]);
    }
}

#ifdef MX6XSLX
int mx6q_output (struct ifnet *ifp, struct mbuf *m,
		 struct sockaddr *dst, struct rtentry *rt)
{
    mx6q_dev_t			*mx6q = ifp->if_softc;
    struct m_tag		*tag;
    uint8_t			priority, queue = 0;
    uint32_t			num_free;
    int				error;

    tag = GET_TXQ_TAG(m);
    if (tag == NULL) {
	priority = 0;
    } else {
	priority = EXTRACT_TXQ_TAG(tag);
    }

    /* Map the eight priorities to the 3 queues */
    switch (priority) {
    case 0:
    case 1:
	/* Do a normal if_output via the stack, will be on TXQ 0 */
      return mx6q->stack_output(ifp, m, dst, rt);
      break;

    case 2:
    case 3:
        /* Send the mbuf on TXQ 2 */
	queue = 2;
	break;

    default:
	/* Send the mbuf on TXQ 1 */
	queue = 1;
	break;
    }

    NW_SIGLOCK_P(&mx6q->ecom.ec_if.if_snd_ex, mx6q->iopkt, WTP);

    /* Check for space */
    num_free = mx6q->num_tx_descriptors - mx6q->tx_descr_inuse[queue];
    if (num_free <= MX6Q_MAX_FRAGS) {
	mx6q_transmit_complete(mx6q, queue);

	num_free = mx6q->num_tx_descriptors - mx6q->tx_descr_inuse[queue];
	if (num_free <= MX6Q_MAX_FRAGS) {
	    m_freem(m);
	    NW_SIGUNLOCK_P(&mx6q->ecom.ec_if.if_snd_ex, mx6q->iopkt, WTP);
	    return ENOBUFS;
	}
    }

    /*
     * The packet already has an ether header on it but is moved down to let
     * ether_output() do the header as well. As this is going direct, just
     * restore the current header.
     */
    m->m_data -= sizeof(struct ether_header);
    m->m_len += sizeof(struct ether_header);
    m->m_pkthdr.len += sizeof(struct ether_header);

    error = mx6q_tx(mx6q, m, queue);
    NW_SIGUNLOCK_P(&mx6q->ecom.ec_if.if_snd_ex, mx6q->iopkt, WTP);
    return error;
}

static uint32_t valid_idleslope (uint32_t value)
{
    uint32_t tmp;

    /* Only certain values are valid, see the spec for ENET_DMAnCFG */
    if (value < 128) {
	/* Round up to power of two */
	for (tmp = 128; tmp !=0; tmp >>= 1) {
	    if (value & tmp) {
		break;
	    }
	}
	if (value > tmp) {
	    value = tmp << 1;
	}
    } else {
	/* Round up to multiple of 128 */
	tmp = (value + 127) / 128;
	value = tmp * 128;
    }
    if (value == 0) {
	value = 1;
    }
    return value;
}

int mx6q_set_tx_bw (mx6q_dev_t *mx6q, struct ifdrv *ifd)
{
    volatile uint32_t	*base = mx6q->reg;
    avb_bw_t		*avb_bw;
    uint32_t		q1_idle, q1_bw, q2_idle, q2_bw, port_bw;

    /*
     * If this is the first set bandwidth call then we need to enable all
     * the Tx shaping and Rx class match setup.
     */
    if ((*(base + MX6Q_QOS_SCHEME) & QOS_SCHEME_RX_FLUSH0) == 0) {

	/* Credit based Tx shapers and flush Rx0 on queue full */
	*(base + MX6Q_QOS_SCHEME) = QOS_SCHEME_RX_FLUSH0;

	/* Disable Pause as we flush on Rx full*/
	*(base + MX6Q_R_CNTRL) &= ~RCNTRL_FCE;
	*(base + MX6Q_R_SECTION_EMPTY_ADDR) = 0;

	/* Enable priority 2,3 to class 2, 4-7 to class 1 */
	*(base + MX6Q_R_CLS_MATCH2) = RCV_CLS_MATCHEN |
	  (2 << RCV_CMP0_SHIFT) | (3 << RCV_CMP1_SHIFT) |
	  (2 << RCV_CMP2_SHIFT) | (2 << RCV_CMP3_SHIFT);
	*(base + MX6Q_R_CLS_MATCH1) = RCV_CLS_MATCHEN |
	  (4 << RCV_CMP0_SHIFT) | (5 << RCV_CMP1_SHIFT) |
	  (6 << RCV_CMP2_SHIFT) | (7 << RCV_CMP3_SHIFT);

	/* Instruct MAC to process recieved frames on class 1 and 2 */
	*(base + MX6Q_R_DES_ACTIVE1) = R_DES_ACTIVE;
	*(base + MX6Q_R_DES_ACTIVE2) = R_DES_ACTIVE;
    }

    /* Determine current port bandwidth */
    if (mx6_is_br_phy(mx6q)) {
	/* Hard coded to 100mbps */
	port_bw = 100;
    } else {
	port_bw = mx6q->cfg.media_rate / 1000; /* In Mbps */
	if (port_bw == 0) {
	    /* Link is down, we will recalculate when it comes up */
	    return EOK;
	}
    }

    avb_bw = (avb_bw_t*)&ifd->ifd_data;

    /* Priority 0 & 1 is queue 0 with no shaping so ignore them */

    /* Priority 2 & 3 is queue 2, convert kbps to Mbps with roundup */
    q2_bw = (avb_bw->bandwidth[2] + avb_bw->bandwidth[3] + 1023) / 1024;

    /* Priority 4+ is queue 2 */
    q1_bw = (avb_bw->bandwidth[4] + avb_bw->bandwidth[5] +
	     avb_bw->bandwidth[6] +avb_bw->bandwidth[7] + 1023) / 1024;
 
    /*
     * q1_bw + q2_bw must be < 0.75 of port_bw
     */
    if (((4 * (q1_bw + q2_bw)) / port_bw) >= 3) {
	log(LOG_ERR,
	    "Total AVB bandwidth %dMbps exceeds 0.75 of port speed %dMbps",
	    q1_bw + q2_bw, port_bw);
	/* Invalid config so set shapers wide open */
	*(base + MX6Q_DMACFG1) = DMACFG_DMA_CLASSEN | 1536;
	*(base + MX6Q_DMACFG2) = DMACFG_DMA_CLASSEN | 1536;
	return EINVAL;
    }

    /*
     * Rearranging the equation given in the spec for ENET_DMAnCFG
     * gives:
     * idleslope = (512 * q_bw) / (port_bw - q_bw)
     * But note that only certain values are valid
     */
    q1_idle = valid_idleslope((512 * q1_bw) / (port_bw - q1_bw));
    q2_idle = valid_idleslope((512 * q2_bw) / (port_bw - q2_bw));

    if (mx6q->cfg.verbose) {
	log(LOG_ERR, "%s: q1_bw %u q2_bw %u q1_idle %u q2_idle %u",
	    __FUNCTION__, q1_bw, q2_bw, q1_idle, q2_idle);
    }

    /* Write out to the hardware */
    *(base + MX6Q_DMACFG1) = DMACFG_DMA_CLASSEN | q1_idle;
    *(base + MX6Q_DMACFG2) = DMACFG_DMA_CLASSEN | q2_idle;

    return EOK;
}
#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/io-pkt/sys/dev_qnx/mx6x/transmit.c $ $Rev: 762005 $")
#endif
