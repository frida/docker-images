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

static inline void
do_stat32 (volatile uint32_t *addr, uint32_t *new, uint32_t *old)
{
    uint32_t value;

    value = *addr;

    if (value == *old) {
	// Do nothing
    }
    else if (value > *old) {
	*new += value - *old;
	*old = value;
    } else {
	// 16bit hardware counter has wrapped
	*new += ((1 << 16) - *old) + value;
	*old = value;
    }
}

static inline void
do_stat64 (volatile uint32_t *addr, uint64_t *new, uint64_t *old)
{
    uint32_t value;

    value = *addr;

    if (value == *old) {
	// Do nothing
    }
    else if (value > *old) {
	*new += value - *old;
	*old = value;
    } else {
	// 32bit hardware counter has wrapped
	*new += ((1LL << 32LL) - *old) + value;
	*old = value;
    }
}

//
// called from devctl or mx6q_process_interrupt() 
//
// read the nic hardware mib registers into our data struct
//
void    
mx6q_update_stats (mx6q_dev_t *mx6q)
{
	volatile uint32_t		*base = mx6q->reg;
	nic_ethernet_stats_t		*estats = &mx6q->stats.un.estats;
	nic_ethernet_stats_t		*old_estats = &mx6q->old_stats.un.estats;

	do_stat64(base + MX6Q_IEEE_T_OCTETS_OK, &mx6q->stats.octets_txed_ok,
		  &mx6q->old_stats.octets_txed_ok);
	do_stat32(base + MX6Q_IEEE_T_FRAME_OK, &mx6q->stats.txed_ok,
		  &mx6q->old_stats.txed_ok);
	do_stat32(base + MX6Q_RMON_T_MC_PKT, &mx6q->stats.txed_multicast,
		  &mx6q->old_stats.txed_multicast);
	do_stat32(base + MX6Q_RMON_T_BC_PKT, &mx6q->stats.txed_broadcast,
		  &mx6q->old_stats.txed_broadcast);
	do_stat32(base + MX6Q_IEEE_T_1COL, &estats->single_collisions,
		  &old_estats->single_collisions);
	do_stat32(base + MX6Q_IEEE_T_MCOL, &estats->multi_collisions,
		  &old_estats->multi_collisions);
	do_stat32(base + MX6Q_IEEE_T_DEF, &estats->tx_deferred,
		  &old_estats->tx_deferred);
	do_stat32(base + MX6Q_IEEE_T_LCOL, &estats->late_collisions,
		  &old_estats->late_collisions);
	do_stat32(base + MX6Q_IEEE_T_MACERR, &estats->internal_tx_errors,
		  &old_estats->internal_tx_errors);
	do_stat32(base + MX6Q_IEEE_T_CSERR, &estats->no_carrier,
		  &old_estats->no_carrier);
	do_stat32(base + MX6Q_RMON_T_JAB, &estats->jabber_detected,
		  &old_estats->jabber_detected);


	do_stat64(base + MX6Q_IEEE_OCTETS_OK, &mx6q->stats.octets_rxed_ok,
		  &mx6q->old_stats.octets_rxed_ok);
	do_stat32(base + MX6Q_IEEE_R_FRAME_OK, &mx6q->stats.rxed_ok,
		  &mx6q->old_stats.rxed_ok);
	do_stat32(base + MX6Q_RMON_R_MC_PKT, &mx6q->stats.rxed_multicast,
		  &mx6q->old_stats.rxed_multicast);
	do_stat32(base + MX6Q_RMON_R_BC_PKT, &mx6q->stats.rxed_broadcast,
		  &mx6q->old_stats.rxed_broadcast);
	do_stat32(base + MX6Q_IEEE_R_ALIGN, &estats->align_errors,
		  &old_estats->align_errors);
	do_stat32(base + MX6Q_IEEE_R_CRC, &estats->fcs_errors,
		  &old_estats->fcs_errors);
	do_stat32(base + MX6Q_RMON_R_OVERSIZE, &estats->oversized_packets,
		  &old_estats->oversized_packets);
	do_stat32(base + MX6Q_RMON_R_UNDERSIZE, &estats->short_packets,
		  &old_estats->short_packets);
}

//
// called from mx6q_init()
//
void    
mx6q_clear_stats (mx6q_dev_t *mx6q)
{

    // clear the old stats
    memset(&mx6q->old_stats, 0, sizeof(mx6q->old_stats));

    // now clear counters in our data structure
    memset(&mx6q->stats, 0, sizeof(mx6q->stats));

    // reset stats stuff for devctl
    mx6q->stats.revision = NIC_STATS_REVISION;

    mx6q->stats.valid_stats =
      NIC_STAT_RXED_MULTICAST | NIC_STAT_RXED_BROADCAST |
      NIC_STAT_TXED_MULTICAST | NIC_STAT_TXED_BROADCAST |
      NIC_STAT_TX_FAILED_ALLOCS | NIC_STAT_RX_FAILED_ALLOCS;

    mx6q->stats.un.estats.valid_stats =
      NIC_ETHER_STAT_SINGLE_COLLISIONS |
      NIC_ETHER_STAT_MULTI_COLLISIONS |
      NIC_ETHER_STAT_TX_DEFERRED |
      NIC_ETHER_STAT_LATE_COLLISIONS |
      NIC_ETHER_STAT_INTERNAL_TX_ERRORS |
      NIC_ETHER_STAT_NO_CARRIER |
      NIC_ETHER_STAT_JABBER_DETECTED |
      NIC_ETHER_STAT_ALIGN_ERRORS |
      NIC_ETHER_STAT_FCS_ERRORS |
      NIC_ETHER_STAT_OVERSIZED_PACKETS |
      NIC_ETHER_STAT_SHORT_PACKETS;
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/io-pkt/sys/dev_qnx/mx6x/stats.c $ $Rev: 708496 $")
#endif
