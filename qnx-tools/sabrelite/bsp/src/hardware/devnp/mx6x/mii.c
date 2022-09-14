/*
 * $QNXLicenseC: 
 * Copyright 2010, 2011 QNX Software Systems.  
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

//
// drvr lib callback to read phy register
//
uint16_t    
mx6q_mii_read (void *handle, uint8_t phy_add, uint8_t reg_add)
{
    mx6q_dev_t       *mx6q = (mx6q_dev_t *) handle;
    volatile uint32_t   *base = mx6q->reg;
    int                 timeout = MPC_TIMEOUT;
    uint32_t            val;

	*(base + MX6Q_IEVENT) |= IEVENT_MII;
        val = ((1 << 30) | (0x2 << 28) | (phy_add << 23) | (reg_add << 18) | (2 << 16));
        *(base + MX6Q_MII_DATA) = val;

        while (timeout--) {
                if (*(base + MX6Q_IEVENT) & IEVENT_MII) {
                        *(base + MX6Q_IEVENT) |= IEVENT_MII;
	                       break;
                 }
                nanospin_ns (10000);
                }
        return ((timeout <= 0) ? 0 : (*(base + MX6Q_MII_DATA) & 0xffff));
}


//
// drvr lib callback to write phy register
//
void    
mx6q_mii_write (void *handle, uint8_t phy_add, uint8_t reg_add, uint16_t data)

{
    mx6q_dev_t       *mx6q = (mx6q_dev_t *) handle;
    volatile uint32_t   *base = mx6q->reg;
    int                 timeout = MPC_TIMEOUT;
	uint32_t        phy_data;

	*(base + MX6Q_IEVENT) |= IEVENT_MII;
        phy_data = ((1 << 30) | (0x1 << 28) | (phy_add << 23) | (reg_add << 18) | (2 << 16) | data);
        *(base + MX6Q_MII_DATA) = phy_data;
        while (timeout--) {
                if (*(base + MX6Q_IEVENT) & IEVENT_MII) {
                        *(base + MX6Q_IEVENT) |= IEVENT_MII;
                        break;
                        }
                nanospin_ns (10000);
                }
}


//
// drvr lib callback when PHY link state changes
//
void
mx6q_mii_callback(void *handle, uchar_t phy, uchar_t newstate)
{
	mx6q_dev_t		*mx6q = handle;
	nic_config_t		*cfg  = &mx6q->cfg;
	char			*s;
	int			i;
	int			mode;
	struct ifnet		*ifp = &mx6q->ecom.ec_if;
	volatile uint32_t	*base = mx6q->reg;
	int			phy_idx = cfg->phy_addr;
	uint16_t		advert, lpadvert;
	
	switch(newstate) {
	case MDI_LINK_UP:
		if ((i = MDI_GetActiveMedia(mx6q->mdi, cfg->phy_addr, &mode)) != MDI_LINK_UP) {
			log(LOG_INFO, "%s(): MDI_GetActiveMedia() failed: %x", __FUNCTION__, i);
			mode = 0;  // force default case below - all MDI_ macros are non-zero
		}

		switch(mode) {
		case MDI_10bTFD:
			s = "10 BaseT Full Duplex";
			cfg->duplex = 1;
			cfg->media_rate = 10 * 1000L;
			*(base + MX6Q_ECNTRL) &= ~ECNTRL_ETH_SPEED;
			break;
		case MDI_10bT:
			s = "10 BaseT Half Duplex";
			cfg->duplex = 0;
			cfg->media_rate = 10 * 1000L;
			*(base + MX6Q_ECNTRL) &= ~ECNTRL_ETH_SPEED;
			break;
		case MDI_100bTFD:
			s = "100 BaseT Full Duplex";
			cfg->duplex = 1;
			cfg->media_rate = 100 * 1000L;
			*(base + MX6Q_ECNTRL) &= ~ECNTRL_ETH_SPEED;
			break;
		case MDI_100bT: 
			s = "100 BaseT Half Duplex";
			cfg->duplex = 0;
			cfg->media_rate = 100 * 1000L;
			*(base + MX6Q_ECNTRL) &= ~ECNTRL_ETH_SPEED;
			break;
		case MDI_100bT4:
			s = "100 BaseT4";
			cfg->duplex = 0;
			cfg->media_rate = 100 * 1000L;
			*(base + MX6Q_ECNTRL) &= ~ECNTRL_ETH_SPEED;
			break;
		case MDI_1000bT:
			s = "1000 BaseT Half Duplex";
			cfg->duplex = 0;
			cfg->media_rate = 1000 * 1000L;
			*(base + MX6Q_ECNTRL) |= ECNTRL_ETH_SPEED;
			break;
		case MDI_1000bTFD:
			s = "1000 BaseT Full Duplex";
			cfg->duplex = 1;
			cfg->media_rate = 1000 * 1000L;
			*(base + MX6Q_ECNTRL) |= ECNTRL_ETH_SPEED;
			break;
		default:
			log(LOG_INFO,"%s(): unknown link mode 0x%X",__FUNCTION__,mode);
			s = "Unknown";
			cfg->duplex = 0;
			cfg->media_rate = 0L;
			break;
		}

	// immediately set new speed and duplex in nic config registers
	mx6q_speeduplex(mx6q);

	if (mx6q->flow == -1) {
	    /* Flow control was autoneg'd, set what we got in the MAC */
	    advert = mx6q_mii_read(mx6q, phy_idx, MDI_ANAR);
	    lpadvert = mx6q_mii_read(mx6q, phy_idx, MDI_ANLPAR);
	    if (advert & MDI_FLOW) {
		if (lpadvert & MDI_FLOW) {
		    /* Enable Tx and Rx of Pause */
		    *(base + MX6Q_R_SECTION_EMPTY_ADDR) = 0x82;
		    *(base + MX6Q_R_CNTRL) |= RCNTRL_FCE;
		} else if ((advert & MDI_FLOW_ASYM) &&
			   (lpadvert & MDI_FLOW_ASYM)) {
		    /* Enable Rx of Pause */
		    *(base + MX6Q_R_SECTION_EMPTY_ADDR) = 0;
		    *(base + MX6Q_R_CNTRL) |= RCNTRL_FCE;

		} else {
		    /* Disable all pause */
		    *(base + MX6Q_R_SECTION_EMPTY_ADDR) = 0;
		    *(base + MX6Q_R_CNTRL) &= ~RCNTRL_FCE;
		}
	    } else if ((advert & MDI_FLOW_ASYM) &&
		       (lpadvert & MDI_FLOW) &&
		       (lpadvert & MDI_FLOW_ASYM)) {
		/* Enable Tx of Pause */
		*(base + MX6Q_R_SECTION_EMPTY_ADDR) = 0x82;
		*(base + MX6Q_R_CNTRL) &= ~RCNTRL_FCE;
	    } else {
		/* Disable all pause */
		*(base + MX6Q_R_SECTION_EMPTY_ADDR) = 0;
		*(base + MX6Q_R_CNTRL) &= ~RCNTRL_FCE;
	    }
	}

	if (cfg->media_rate) {
		cfg->flags &= ~NIC_FLAG_LINK_DOWN;
		if (cfg->verbose) {
			log(LOG_INFO, "%s(): link up lan %d idx %d (%s)",
				__FUNCTION__, cfg->lan, cfg->device_index, s);
		}
		if_link_state_change(ifp, LINK_STATE_UP);
		if (!IFQ_IS_EMPTY(&ifp->if_snd)) {
			// Packets were still in the queue when the link
			// went down, call start to get them moving again
			NW_SIGLOCK(&ifp->if_snd_ex, mx6q->iopkt);
			mx6q_start(ifp);
		}
		break;
	} else {
		// fall through to link down handling
	}

	case MDI_LINK_DOWN:
		cfg->media_rate = cfg->duplex = -1;
		cfg->flags |= NIC_FLAG_LINK_DOWN;

		if (cfg->verbose) {
			log(LOG_INFO,
				"%s(): Link down lan %d idx %d, calling MDI_AutoNegotiate()",
				__FUNCTION__, cfg->lan, cfg->device_index);
		}
		MDI_AutoNegotiate(mx6q->mdi, cfg->phy_addr, NoWait);
		if_link_state_change(ifp, LINK_STATE_DOWN);
		break;

	default:
		log(LOG_ERR, "%s(): idx %d: Unknown link state 0x%X",	__FUNCTION__, cfg->device_index, newstate);
		break;
	}
}

void mx6_br_phy_state(mx6q_dev_t *mx6q)
{
	uint16_t	val;
	nic_config_t	*cfg = &mx6q->cfg;
	struct ifnet	*ifp = &mx6q->ecom.ec_if;

	/* Link state latches low so double read to clear */
	val = mx6q_mii_read(mx6q,  cfg->phy_addr, 1);
	val = mx6q_mii_read(mx6q,  cfg->phy_addr, 1);

	if ((cfg->flags & NIC_FLAG_LINK_DOWN) &&
	    (val & 4)) {
		/* Link was down and is now up */
		if (cfg->verbose) {
			log(LOG_INFO, "%s(): link up", __FUNCTION__);
		}
		cfg->flags &= ~NIC_FLAG_LINK_DOWN;
		if_link_state_change(ifp, LINK_STATE_UP);
		if (!IFQ_IS_EMPTY(&ifp->if_snd)) {
			/*
			 * Packets were still in the queue when the link
			 * went down, call start to get them moving again.
			 */
			NW_SIGLOCK(&ifp->if_snd_ex, mx6q->iopkt);
			mx6q_start(ifp);
		}
	} else if (((cfg->flags & NIC_FLAG_LINK_DOWN) == 0) &&
		   ((val & 4) == 0)) {
		/* Link was up and is now down */
		if (cfg->verbose) {
			log(LOG_INFO, "%s(): link down", __FUNCTION__);
		}
		cfg->flags |= NIC_FLAG_LINK_DOWN;
		if_link_state_change(ifp, LINK_STATE_DOWN);
	}
}

//
// periodically called by stack to probe phy state
// and to clean out tx descriptor ring
//
void
mx6q_MDI_MonitorPhy (void *arg)
{
	mx6q_dev_t		*mx6q	= arg;
	nic_config_t		*cfg  		= &mx6q->cfg;
	struct ifnet 		*ifp		= &mx6q->ecom.ec_if;

	//
	// we will probe the PHY if:
	//   the user has forced it from the cmd line, or
	//   we have not rxd any packets since the last time we ran, or
	//   the link is considered down
	//
	if (mx6q->probe_phy ||
	  !mx6q->rxd_pkts   ||
	  cfg->media_rate <= 0 ||
	  cfg->flags & NIC_FLAG_LINK_DOWN) {
		if (cfg->verbose > 4) {
			log(LOG_ERR, "%s(): calling MDI_MonitorPhy()\n",  __FUNCTION__);
		}
		if (!mx6_is_br_phy(mx6q)) {
			MDI_MonitorPhy(mx6q->mdi);
		} else {
			mx6_br_phy_state(mx6q);
		}

	} else {
		if (cfg->verbose > 4) {
			log(LOG_ERR, "%s(): NOT calling MDI_MonitorPhy()\n",  __FUNCTION__);
		}
	}
	mx6q->rxd_pkts = 0;  // reset for next time we are called

	//
	// Clean out the tx descriptor ring if it has not
	// been done by the start routine in the last 2 seconds
	//
	if (!mx6q->tx_reaped) {
		NW_SIGLOCK(&ifp->if_snd_ex, mx6q->iopkt);

		mx6q_transmit_complete(mx6q, 0);

		NW_SIGUNLOCK(&ifp->if_snd_ex, mx6q->iopkt);
	}
	mx6q->tx_reaped = 0;  // reset for next time we are called


	// restart timer to call us again in 2 seconds
	callout_msec(&mx6q->mii_callout, 2 * 1000, mx6q_MDI_MonitorPhy, mx6q);
}

int mx6_is_br_phy (mx6q_dev_t *mx6q) {
	uint32_t PhyId, vendor, model;
	uint32_t PhyAddr = mx6q->cfg.phy_addr;
	static int is_br = -1;

	if (is_br == -1) {
		PhyId = (mx6q_mii_read(mx6q, PhyAddr, MDI_PHYID_1)) << 16;
		PhyId |= mx6q_mii_read(mx6q, PhyAddr, MDI_PHYID_2);

		vendor = PHYID_VENDOR(PhyId);
		model = PHYID_MODEL(PhyId);

		switch (vendor) {
		case BROADCOM2:
			switch (model) {
			case BCM89810:
				is_br = 1;
				break;
			default:
				is_br = 0;
				break;
			}
			break;
		default:
			is_br = 0;
			break;
		}
	}
	return is_br;
}

int mx6_get_phy_addr (mx6q_dev_t *mx6q)
{
	int		phy_idx;

	// Get PHY address
	for (phy_idx = 0; phy_idx < 32; phy_idx++) {
		if (MDI_FindPhy(mx6q->mdi, phy_idx) == MDI_SUCCESS) {
			if (mx6q->cfg.verbose) {
				log(LOG_ERR, "%s(): PHY found at address %d",
				    __FUNCTION__, phy_idx);
			}
			break;
		}
	}

	if (phy_idx == 32) {
		log(LOG_ERR,"Unable to detect PHY");
		return -1;
	} else {
		return phy_idx;
	}
}

int mx6_setup_phy (mx6q_dev_t *mx6q)
{
	int		status;
	nic_config_t	*cfg = &mx6q->cfg;
	int		phy_idx = cfg->phy_addr;

	status = MDI_InitPhy(mx6q->mdi, phy_idx);
	if (status != MDI_SUCCESS) {
		log(LOG_ERR, "%s(): Failed to init the PHY: %d",
		    __FUNCTION__, status);
		return -1;
	}
	status = MDI_ResetPhy(mx6q->mdi, phy_idx, WaitBusy);
	if (status != MDI_SUCCESS) {
		log(LOG_ERR, "%s(): Failed to reset the PHY: %d",
		    __FUNCTION__, status);
		return -1;
	}

	if (mx6q->force_advertise == 0) {    // the user has forced the link down
		MDI_IsolatePhy(mx6q->mdi, phy_idx);
		MDI_DisableMonitor(mx6q->mdi);  // neuter MDI_MonitorPhy()
		return 0;
	} else {  // forced or auto-neg
		// in case the user previously forced the link
		// down, bring it back up again
		MDI_DeIsolatePhy(mx6q->mdi, phy_idx);
		delay(10);
	}
	return 0;

}
/* this function disables Wirespeed mode (register 0x18, shadow 7, bit 4).
 */
int mx6_paves3_phy_init(mx6q_dev_t *mx6q)
{
	nic_config_t	*cfg	= &mx6q->cfg;
	int		phy_idx	= cfg->phy_addr;
	unsigned short val;
    
	mx6q_mii_write(mx6q, phy_idx, 0x18, 0x7007);
	val = mx6q_mii_read(mx6q, phy_idx, 0x18);
    val &= 0xffef;
    val |= 0x8000;
	mx6q_mii_write(mx6q, phy_idx, 0x18, val);
	
	return 0;
}

int mx6_paves3_twelve_inch_phy_init(mx6q_dev_t *mx6q)
{
	nic_config_t	*cfg	= &mx6q->cfg;
	int		phy_idx	= cfg->phy_addr;
	unsigned short val;
    /* set page to 0 */
	mx6q_mii_write(mx6q, phy_idx, 0x16, 0);

	/* force MDI mode, disable polarity reversal */
	val = mx6q_mii_read(mx6q, phy_idx, 0x0);
	val &= 0xff9f;
	val |= 0x2;
	mx6q_mii_write(mx6q, phy_idx, 0x0, val);
	
	return 0;
}

void mx6_broadreach_phy_init(mx6q_dev_t *mx6q)
{
    int phy_idx = mx6q->cfg.phy_addr;

    /*
     * The following came from Broadcom as 89810A2_script_v2_2.vbs
     *
     * Broadcom refuse to document what exactly is going on.
     * They insist these register writes are correct despite the
     * way sometimes the same register is written back-to-back with
     * contradictory values and others are written with default values.
     * There is also much writing to undocumented registers and reserved
     * fields in documented registers.
     */
    mx6q_mii_write(mx6q, phy_idx, 0, 0x8000); //reset

    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0F93);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x107F);
    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0F90);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x0001);
    mx6q_mii_write(mx6q, phy_idx, 0x00, 0x3000);
    mx6q_mii_write(mx6q, phy_idx, 0x00, 0x0200);

    mx6q_mii_write(mx6q, phy_idx, 0x18, 0x0C00);

    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0F90);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x0000);
    mx6q_mii_write(mx6q, phy_idx, 0x00, 0x0100);

    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0001);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x0027);

    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x000E);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x9B52);

    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x000F);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0xA04D);

    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0F90);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x0001);

    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0F92);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x9225);

    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x000A);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x0323);

    /* Shut off unused clocks */
    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0FFD);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x1C3F);

    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0FFE);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x1C3F);

    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0F99);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x7180);
    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0F9A);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x34C0);

    /* RGMII config */
    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0F0E);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x0000);

    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0F9F);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x0000);

    mx6q_mii_write(mx6q, phy_idx, 0x18, 0xF1E7);

    /* Disable LDS, 100Mb/s, one pair */
    if ( mx6q->bcm89810 == 1) {
    	/* Set phy to slave mode */
	    log(LOG_INFO, "devnp-mx6x: Setting BroadrReach phy to slave");
    	mx6q_mii_write(mx6q, phy_idx, 0, 0x0200);
    } else {
    	/* Set phy to master mode */
	    log(LOG_INFO, "devnp-mx6x: Setting BroadrReach phy to master");
        mx6q_mii_write(mx6q, phy_idx, 0, 0x0208);
    }
}

int mx6_sabrelite_phy_init(mx6q_dev_t *mx6q)
{
	nic_config_t	*cfg	= &mx6q->cfg;
	int		phy_idx	= cfg->phy_addr;
	
	// 1000 Base-T full duplex capable, single port device
	mx6q_mii_write(mx6q, phy_idx, 0x9, 0x0600);

	// min rx data delay
	mx6q_mii_write(mx6q, phy_idx, 0xb, 0x8105);
	mx6q_mii_write(mx6q, phy_idx, 0xc, 0x0000);

	// max rx/tx clock delay, min rx/tx control delay
	mx6q_mii_write(mx6q, phy_idx, 0xb, 0x8104);
	mx6q_mii_write(mx6q, phy_idx, 0xc, 0xf0f0);
	mx6q_mii_write(mx6q, phy_idx, 0xb, 0x104);
	
	return 0;
}

int mx6_sabreauto_rework(mx6q_dev_t *mx6q)
{
	nic_config_t        *cfg 		= &mx6q->cfg;
	int                 phy_idx 	= cfg->phy_addr;
	unsigned short val;

	/* To enable AR8031 ouput a 125MHz clk from CLK_25M */
	mx6q_mii_write(mx6q, phy_idx, 0xd, 0x7);
	mx6q_mii_write(mx6q, phy_idx, 0xe, 0x8016);
	mx6q_mii_write(mx6q, phy_idx, 0xd, 0x4007);
	val = mx6q_mii_read(mx6q, phy_idx, 0xe);

	val &= 0xffe3;
	val |= 0x18;
	mx6q_mii_write(mx6q, phy_idx, 0xe, val);

	/* introduce tx clock delay */
	mx6q_mii_write(mx6q, phy_idx, 0x1d, 0x5);
	val = mx6q_mii_read(mx6q, phy_idx, 0x1e);
	val |= 0x0100;
	mx6q_mii_write(mx6q, phy_idx, 0x1e, val);

	/*
	 * Disable SmartEEE
	 * The Tx delay can mean late pause and bad timestamps.
	 */
	mx6q_mii_write(mx6q, phy_idx, 0xd, 0x3);
	mx6q_mii_write(mx6q, phy_idx, 0xe, 0x805d);
	mx6q_mii_write(mx6q, phy_idx, 0xd, 0x4003);
	val = mx6q_mii_read(mx6q, phy_idx, 0xe);
	val &= ~(1 << 8);
	mx6q_mii_write(mx6q, phy_idx, 0xe, val);

	/* As above for EEE (802.3az) */
	mx6q_mii_write(mx6q, phy_idx, 0xd, 0x7);
	mx6q_mii_write(mx6q, phy_idx, 0xe, 0x3c);
	mx6q_mii_write(mx6q, phy_idx, 0xd, 0x4007);
	mx6q_mii_write(mx6q, phy_idx, 0xd,0);

	return 0;
}

void
mx6q_init_phy(mx6q_dev_t *mx6q)
{
	nic_config_t	*cfg = &mx6q->cfg;
	int		phy_idx = cfg->phy_addr;
	uint16_t	reg;
	int		i;

	if (cfg->verbose) {
		log(LOG_ERR, "%s(): media_rate: %d, duplex: %d, PHY: %d",
		    __FUNCTION__, cfg->media_rate, cfg->duplex, phy_idx);
	}

	if (mx6q->force_advertise == 0) {
		if (cfg->verbose) {
			log(LOG_ERR, "%s(): Powering down PHY", __FUNCTION__);
		}
		MDI_IsolatePhy(mx6q->mdi, phy_idx);
		MDI_PowerdownPhy(mx6q->mdi, phy_idx);
		MDI_DisableMonitor(mx6q->mdi);
	} else {
		MDI_DeIsolatePhy(mx6q->mdi, phy_idx);
		MDI_PowerupPhy(mx6q->mdi, phy_idx);

		if (mx6q->force_advertise != -1) {
			/*
			 * If we force the speed, but the link partner
			 * is autonegotiating, there is a greater chance
			 * that everything will work if we advertise with
			 * the speed that we are forcing to.
			 */
			MDI_SetAdvert(mx6q->mdi, phy_idx,
				      mx6q->force_advertise);

			if (cfg->verbose) {
				log(LOG_ERR, "%s(): restricted autonegotiate (%dMbps only)",
					__FUNCTION__, cfg->media_rate / 1000);
			}
			reg = mx6q_mii_read(mx6q, phy_idx, MDI_BMCR);
			reg &= ~(BMCR_SPEED_100 | BMCR_SPEED_1000 |
				 BMCR_FULL_DUPLEX);
			reg |= BMCR_RESTART_AN | BMCR_AN_ENABLE;
			mx6q_mii_write(mx6q, phy_idx, MDI_BMCR, reg);
			MDI_EnableMonitor(mx6q->mdi, 1);
		} else { /* Normal auto-negotiation mode */
			if (cfg->verbose) {
				log(LOG_ERR, "%s(): autonegotiate",
				    __FUNCTION__);
			}
			/* Enable Pause in autoneg */
			MDI_GetMediaCapable(mx6q->mdi, phy_idx, &i);
			if ((mx6q->mdi->PhyData[phy_idx]->VendorOUI == KENDIN) &&
			    (mx6q->mdi->PhyData[phy_idx]->Model == KSZ9021)) {
				/* Fails to autoneg with ASYM */
				i |= MDI_FLOW;
			} else {
				i |= MDI_FLOW | MDI_FLOW_ASYM;
			}
			MDI_SetAdvert(mx6q->mdi, phy_idx, i);
			MDI_AutoNegotiate(mx6q->mdi, phy_idx, NoWait);
			MDI_EnableMonitor(mx6q->mdi, 1);
		}
	}
}

//
// periodically called by stack to probe sqi
//
void
mx6q_BRCM_SQI_Monitor (void *arg)
{
	mx6q_dev_t		*mx6q	= arg;
	nic_config_t		*cfg = &mx6q->cfg;

	if (cfg->flags & NIC_FLAG_LINK_DOWN) {
		if (cfg->verbose) {
			log(LOG_INFO, "%s(): link down, clear sqi sample buffer:  \n", __FUNCTION__);
		}
		mx6_clear_sample();
	}
	else 
		mx6_mii_sqi(mx6q);

	// restart timer to call us again in MX6Q_SQI_SAMPLING_INTERVAL seconds
	callout_msec(&mx6q->sqi_callout, MX6Q_SQI_SAMPLING_INTERVAL * 1000, mx6q_BRCM_SQI_Monitor, mx6q);
}

/*read the SQI register*/
void mx6_mii_sqi(mx6q_dev_t *mx6q)
{
	/*
	According to LGE, follow the 2 steps to read SQI register.
	1. Set up register access:
		Write 0C00h to register 18h
		Write 0002h to register 17h
	2. Read register 15h
	
	Refer to the datasheet, April 20, 2012 • 89810-DS03-R - Table 10, page 50
	*/
	unsigned short val = 0;
	nic_config_t	*cfg = &mx6q->cfg;
	int		phy_idx = cfg->phy_addr;

	mx6q_mii_write(mx6q, phy_idx, 0x18, 0x0C00);
	mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0002);
	val = mx6q_mii_read(mx6q, phy_idx, 0x15);
	if (cfg->verbose) {
		log(LOG_INFO, "%s(): SQI read val:  %d\n", __FUNCTION__, val);
	}
	
	if (val > 0)
		mx6q->sqi = mx6_calculate_sqi(val);

	if (cfg->verbose) {
		log(LOG_INFO, "%s(): sqi = :  %d\n",  __FUNCTION__, mx6q->sqi);
	}
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/io-pkt/sys/dev_qnx/mx6x/mii.c $ $Rev: 766080 $")
#endif
