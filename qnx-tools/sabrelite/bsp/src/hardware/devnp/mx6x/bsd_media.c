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

#include <mx6q.h>

#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <device_qnx.h>


//
// this is a callback, made by the bsd media code.  We passed
// a pointer to this function during the ifmedia_init() call
// in bsd_mii_initmedia()
//
void
bsd_mii_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	mx6q_dev_t *mx6q = ifp->if_softc;
	nic_config_t	*cfg = &mx6q->cfg;
	int		phy_idx = cfg->phy_addr;
	uint16_t	advert, lpadvert;

	mx6q->bsd_mii.mii_media_active = IFM_ETHER;
	mx6q->bsd_mii.mii_media_status = IFM_AVALID;

	if (mx6q->force_advertise != -1) {	// link is forced

		if (mx6q->cfg.flags & NIC_FLAG_LINK_DOWN) {
			mx6q->bsd_mii.mii_media_active |= IFM_NONE;
			mx6q->bsd_mii.mii_media_status  = 0;

		} else {	// link is up
			mx6q->bsd_mii.mii_media_status |= IFM_ACTIVE;

			switch(mx6q->cfg.media_rate) {
				case 0:
				mx6q->bsd_mii.mii_media_active |= IFM_NONE;
				break;	
	
				case 1000*10:
				mx6q->bsd_mii.mii_media_active |= IFM_10_T;
				break;
	
				case 1000*100:
				mx6q->bsd_mii.mii_media_active |= IFM_100_TX;
				break;
	
				case 1000*1000:
				mx6q->bsd_mii.mii_media_active |= IFM_1000_T;
				break;

				default:	// this shouldnt really happen, but ...
				mx6q->bsd_mii.mii_media_active |= IFM_NONE;
				break;
			}
	
			if (mx6q->cfg.duplex) {
				mx6q->bsd_mii.mii_media_active |= IFM_FDX;
			}

			/* Sort out Flow Control status */
			mx6q->bsd_mii.mii_media_active |= mx6q->flow;
		}

	} else if (!(mx6q->cfg.flags & NIC_FLAG_LINK_DOWN)) {  // link is auto-detect and up

		mx6q->bsd_mii.mii_media_status |= IFM_ACTIVE;

		switch(mx6q->cfg.media_rate) {
			case 1000*10:
			mx6q->bsd_mii.mii_media_active |= IFM_10_T;
			break;

			case 1000*100:
			mx6q->bsd_mii.mii_media_active |= IFM_100_TX;
			break;

			case 1000*1000:
			mx6q->bsd_mii.mii_media_active |= IFM_1000_T;
			break;

			default:	// this shouldnt really happen, but ...
			mx6q->bsd_mii.mii_media_active |= IFM_NONE;
			break;
		}

		if (mx6q->cfg.duplex) {
			mx6q->bsd_mii.mii_media_active |= IFM_FDX;
		}

		/* Sort out Flow Control status */
		advert = mx6q_mii_read(mx6q, phy_idx, MDI_ANAR);
		lpadvert = mx6q_mii_read(mx6q, phy_idx, MDI_ANLPAR);
		if (advert & MDI_FLOW) {
		    if (lpadvert & MDI_FLOW) {
			mx6q->bsd_mii.mii_media_active |= IFM_ETH_TXPAUSE;
			mx6q->bsd_mii.mii_media_active |= IFM_ETH_RXPAUSE;
		    } else if ((advert & MDI_FLOW_ASYM) &&
			       (lpadvert & MDI_FLOW_ASYM)) {
			mx6q->bsd_mii.mii_media_active |= IFM_ETH_RXPAUSE;
		    }
		} else if ((advert & MDI_FLOW_ASYM) &&
			   (lpadvert & MDI_FLOW) &&
			   (lpadvert & MDI_FLOW_ASYM)) {
		    mx6q->bsd_mii.mii_media_active |= IFM_ETH_TXPAUSE;
		}

		// could move this to mii.c so there was no lag
		ifmedia_set(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_AUTO);

	} else {	// link is auto-detect and down
		mx6q->bsd_mii.mii_media_active |= IFM_NONE;
		mx6q->bsd_mii.mii_media_status = 0;

		// could move this to mii.c so there was no lag
		ifmedia_set(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_NONE);
	}

	// stuff parameter values with hoked-up bsd values
    ifmr->ifm_status = mx6q->bsd_mii.mii_media_status;
    ifmr->ifm_active = mx6q->bsd_mii.mii_media_active;
}


//
// this is a callback, made by the bsd media code.  We passed
// a pointer to this function during the ifmedia_init() call
// in bsd_mii_initmedia().  This function is called when
// someone makes an ioctl into us, we call into the generic
// ifmedia source, and it make this callback to actually 
// force the speed and duplex, just as if the user had
// set the cmd line options
//
int
bsd_mii_mediachange(struct ifnet *ifp)
{
	mx6q_dev_t	*mx6q		= ifp->if_softc;
	int		old_media_rate	= mx6q->cfg.media_rate;
	int		old_duplex	= mx6q->cfg.duplex;
	int		old_flow	= mx6q->flow;
	struct ifmedia	*ifm		= &mx6q->bsd_mii.mii_media;
	int		user_duplex	= ifm->ifm_media & IFM_FDX ? 1 : 0;
	int		user_media	= ifm->ifm_media & IFM_TMASK;
	int		user_flow	= ifm->ifm_media & IFM_ETH_FMASK;
	int		media;

	if (!(ifp->if_flags & IFF_UP)) {
		slogf( _SLOGC_NETWORK, _SLOG_WARNING,
		       "%s(): mx6q interface isn't up, ioctl ignored", __FUNCTION__);
		return 0;
	}

	if (!(ifm->ifm_media & IFM_ETHER)) {
		slogf( _SLOGC_NETWORK, _SLOG_WARNING,
		  "%s(): mx6q interface - bad media: 0x%X", 
		  __FUNCTION__, ifm->ifm_media);
		return 0;	// should never happen
	}

	switch (user_media) {
		case IFM_AUTO:		// auto-select media
		if (mx6q->force_advertise != -1) {
			// we are currently in a forced mode (or disabled) - now go auto-select
			mx6q->force_advertise	= -1;
			mx6q->cfg.media_rate	= -1;
			mx6q->cfg.duplex	= -1;
			mx6q->flow		= -1;
			ifmedia_set(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_AUTO);
			// must re-initialize hardware with new parameters
			mx6q_init_phy(mx6q);
		}
		return 0;
		break;

		case IFM_NONE:		// disable media
		if (mx6q->force_advertise != 0) {
			// we are currently either auto or forced - now disable link
			mx6q->force_advertise	= 0;
			mx6q->cfg.media_rate	= 0;
			mx6q->cfg.duplex	= 0;
			mx6q->flow		= 0;
			ifmedia_set(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_NONE);
			// must re-initialize hardware with new parameters
			mx6q_init_phy(mx6q);
		}
		return 0;
		break;

		case IFM_10_T:		// force 10baseT
		mx6q->force_advertise = user_duplex ? MDI_10bTFD : MDI_10bT;
		mx6q->cfg.media_rate = 10 * 1000;
		media = IFM_ETHER | IFM_10_T;
		break;

		case IFM_100_TX:	// force 100baseTX
		mx6q->force_advertise	= user_duplex ? MDI_100bTFD : MDI_100bT;
		mx6q->cfg.media_rate	= 100 * 1000;
		media = IFM_ETHER | IFM_100_TX;
		break;

                case IFM_1000_T:        // force 1000baseT
                mx6q->force_advertise	= user_duplex ? MDI_1000bTFD : MDI_1000bT;
                mx6q->cfg.media_rate	= 1000 * 1000;
		media = IFM_ETHER | IFM_1000_T;
		break;

		default:			// should never happen
		slogf( _SLOGC_NETWORK, _SLOG_WARNING,
		  "%s(): mx6q interface - unknown media: 0x%X", 
		  __FUNCTION__, user_media);
		return 0;
		break;
	}

	// Forced duplex
	mx6q->cfg.duplex = user_duplex;
	if (user_duplex) {
		media |= IFM_FDX;
	}

	// Forced flow control
	mx6q->flow = user_flow;
	if (user_flow & IFM_FLOW) {
		mx6q->force_advertise |= MDI_FLOW;
	}
	if (user_flow & IFM_ETH_TXPAUSE) {
		mx6q->force_advertise |= MDI_FLOW_ASYM;
	}
	if (user_flow & IFM_ETH_RXPAUSE) {
		mx6q->force_advertise |= MDI_FLOW|MDI_FLOW_ASYM;
	}
	media |= user_flow;

	ifmedia_set(&mx6q->bsd_mii.mii_media, media);

	// does the user want something different than it already is?
	if ((mx6q->cfg.media_rate != old_media_rate) || 
	    (mx6q->cfg.duplex != old_duplex) ||
	    (mx6q->flow != old_flow) ||
	    (mx6q->cfg.flags & NIC_FLAG_LINK_DOWN)) {

		// must re-initialize hardware with new parameters
		mx6q_init_phy(mx6q);
	}

    return 0;
}


//
// called from mx6q_create_instance() in detect.c to hook up
// to the bsd media structure.  Not entirely unlike kissing
// a porcupine, we must do so carefully, because we do not
// want to use the bsd mii management structure, because
// this driver uses the io-net2 lib/drvr mii code
//
void
bsd_mii_initmedia(mx6q_dev_t *mx6q)
{
    nic_config_t *cfg = &mx6q->cfg;

    mx6q->bsd_mii.mii_ifp = &mx6q->ecom.ec_if;

	ifmedia_init(&mx6q->bsd_mii.mii_media, IFM_IMASK, bsd_mii_mediachange,
	  bsd_mii_mediastatus);

	// we do NOT call mii_attach() - we do our own link management

	//
	// must create these entries to make ifconfig media work
	// see lib/socket/public/net/if_media.h for defines
	//

	// ifconfig wm0 none (x22)
    ifmedia_add(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_NONE, 0, NULL);

	// ifconfig wm0 auto (x20)
    ifmedia_add(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_AUTO, 0, NULL);

	// ifconfig wm0 10baseT (x23 - half duplex)
    ifmedia_add(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_10_T, 0, NULL);

	// ifconfig wm0 10baseT-FDX (x100023)
    ifmedia_add(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_10_T|IFM_FDX, 0, NULL);

	// ifconfig wm0 10baseT-FDX mediaopt flow (x500023)
    ifmedia_add(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_10_T|IFM_FDX|IFM_FLOW, 0, NULL);

	// ifconfig wm0 10baseT-FDX mediaopt txpause (x100423)
    ifmedia_add(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_10_T|IFM_FDX|IFM_ETH_TXPAUSE, 0, NULL);

	// ifconfig wm0 10baseT-FDX mediaopt rxpause (x100223)
    ifmedia_add(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_10_T|IFM_FDX|IFM_ETH_RXPAUSE, 0, NULL);

	// ifconfig wm0 100baseTX (x26 - half duplex)
    ifmedia_add(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_100_TX, 0, NULL);

	// ifconfig wm0 100baseTX-FDX (x100026 - full duplex)
    ifmedia_add(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_100_TX|IFM_FDX, 0, NULL);

	// ifconfig wm0 100baseTX-FDX mediaopt flow (x500026 - full duplex)
    ifmedia_add(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_100_TX|IFM_FDX|IFM_FLOW, 0, NULL);

	// ifconfig wm0 100baseTX-FDX mediaopt txpause (x100426 - full duplex)
    ifmedia_add(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_100_TX|IFM_FDX|IFM_ETH_TXPAUSE, 0, NULL);

	// ifconfig wm0 100baseTX-FDX mediaopt rxpause (x100226 - full duplex)
    ifmedia_add(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_100_TX|IFM_FDX|IFM_ETH_RXPAUSE, 0, NULL);

       // ifconfig wm0 1000baseT mediaopt fdx (x100030 - full duplex)
    ifmedia_add(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_1000_T|IFM_FDX, 0, NULL);

       // ifconfig wm0 1000baseT mediaopt fdx,flow (x500030 - full duplex)
    ifmedia_add(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_1000_T|IFM_FDX|IFM_FLOW, 0, NULL);

       // ifconfig wm0 1000baseT mediaopt fdx,txpause (x100430 - full duplex)
    ifmedia_add(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_1000_T|IFM_FDX|IFM_ETH_TXPAUSE, 0, NULL);

       // ifconfig wm0 1000baseT mediaopt fdx,rxpause (x100230 - full duplex)
    ifmedia_add(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_1000_T|IFM_FDX|IFM_ETH_RXPAUSE, 0, NULL);

    switch (cfg->media_rate) {
    case -1:
	ifmedia_set(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_AUTO);
	break;
    case 10*1000:
	if (cfg->duplex) {
	    ifmedia_set(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_10_T|IFM_FDX);
	} else {
	    ifmedia_set(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_10_T);
	}
	break;
    case 100*1000:
	if (cfg->duplex) {
	    ifmedia_set(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_100_TX|IFM_FDX);
	} else {
	    ifmedia_set(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_100_TX);
	}
	break;
    case 1000*1000:
	if (cfg->duplex) {
	    ifmedia_set(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_1000_T|IFM_FDX);
	} else {
	    ifmedia_set(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_1000_T);
	}
	break;
    default:
	ifmedia_set(&mx6q->bsd_mii.mii_media, IFM_ETHER|IFM_NONE);
	break;
    }
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/io-pkt/sys/dev_qnx/mx6x/bsd_media.c $ $Rev: 749194 $")
#endif
