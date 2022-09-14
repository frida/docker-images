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

#define NUM_GADDR 2


// 
// called from mx6q_init() and mx6q_ioctl()
// to calculate the multicast group address hash 
// mask for the current set of multicast addresses
//

void
mx6q_set_multicast(mx6q_dev_t *mx6q)
{
	struct ethercom			*ec = &mx6q->ecom;
	struct ifnet			*ifp = &ec->ec_if;
	volatile uint32_t		*gaddr_reg = mx6q->reg + MX6Q_GADDR1;
	struct ether_multi		*enm;
	struct ether_multistep		step;
	int				i;
	uint32_t			gaddr_val[NUM_GADDR];
	uint32_t			h;

	// wipe our temporary image of gaddr registers on the stack
	for(i=0; i<NUM_GADDR; i++) {
		gaddr_val[i] = 0;
	}	

	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
                if (memcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
                        /*
                         * We must listen to a range of multicast addresses.
                         * For now, just accept all multicasts, rather than
                         * trying to set only those filter bits needed to match
                         * the range.  (At this time, the only use of address
                         * ranges is for IP multicast routing, for which the
                         * range is big enough to require all bits set.)
                         */
                        goto allmulti;
                }

		/* Single address, calculate hash bits */
		h = ether_crc32_le(enm->enm_addrlo, ETHER_ADDR_LEN);

		/* Just want the 6 most-significant bits. */
                h = (h >> 26) & 0x3f;

		if (h & 0x20) {
			gaddr_val[0] |= 1 << (h & 0x1f);
		} else {
			gaddr_val[1] |= 1 << (h & 0x1f);
		}

		ETHER_NEXT_MULTI(step, enm);
	}

	ifp->if_flags &= ~IFF_ALLMULTI;
	goto setit;

allmulti:
	for(i=0; i<NUM_GADDR; i++) {
		gaddr_val[i] = 0xffffffff;
	}	
	ifp->if_flags |= IFF_ALLMULTI;

	//
	// write our calculated gaddr vals out to hardware registers
	//
	// N.B.  this is the only place in this function where we
	// actually hit the hardware.  We do not attempt to mutex
	// anyone else out while we are doing this, because we are
	// either called during initialization, or from the devctl
	// and I believe in that case, that you can hit the gaddrs 
	// when the nic is running without any ill effect
	//
setit:
	for(i=0; i<NUM_GADDR; i++) {
		gaddr_reg[i] = gaddr_val[i];
	}
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/io-pkt/sys/dev_qnx/mx6x/multicast.c $ $Rev: 708496 $")
#endif
