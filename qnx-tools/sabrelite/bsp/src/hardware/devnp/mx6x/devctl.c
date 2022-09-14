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

#include <net/ifdrvcom.h>
#include <sys/sockio.h>
#include <hw/imx6x_devnp_ioctl.h>

#ifdef MX6XSLX
#include <avb.h>
#endif


static void
mx6q_get_stats(mx6q_dev_t *mx6q, void *data)
{
	nic_stats_t				*stats = data;

	// read nic hardware registers into mx6q data struct stats
	mx6q_update_stats(mx6q);

	// copy it to the user buffer
	memcpy(stats, &mx6q->stats, sizeof(mx6q->stats));
}

int
mx6q_ioctl(struct ifnet *ifp, unsigned long cmd, caddr_t data)
{
	mx6q_dev_t			*mx6q = ifp->if_softc;
	int				error = 0;
	struct ifdrv_com		*ifdc;
        struct ifreq			*ifr;
	struct ifdrv			*ifd;
	struct drvcom_config		*dcfgp;
	struct drvcom_stats		*dstp;


	switch (cmd) {
	case SIOCGDRVCOM:
		ifdc = (struct ifdrv_com *)data;
		switch (ifdc->ifdc_cmd) {
		case DRVCOM_CONFIG:
			dcfgp = (struct drvcom_config *)ifdc;

			if (ifdc->ifdc_len != sizeof(nic_config_t)) {
				error = EINVAL;
				break;
			}
			memcpy(&dcfgp->dcom_config, &mx6q->cfg, sizeof(mx6q->cfg));
			break;

		case DRVCOM_STATS:
			dstp = (struct drvcom_stats *)ifdc;

			if (ifdc->ifdc_len != sizeof(nic_stats_t)) {
				error = EINVAL;
				break;
			}
			mx6q_get_stats(mx6q, &dstp->dcom_stats);
			break;

		default:
			error = EOPNOTSUPP;
			break;

		}
		break;


	case SIOCSIFMEDIA:
		if (mx6_is_br_phy(mx6q)) {
			error = ENOTTY;
			break;
		}
		/* Fall through for normal PHY */
	case SIOCGIFMEDIA:
		ifr = (struct ifreq *)data;
		struct ifmediareq *ifmr = (struct ifmediareq *) ifr;

		if (mx6_is_br_phy(mx6q)) {
			ifmr->ifm_current = IFM_ETHER | IFM_MANUAL;
			if (mx6q->bcm89810 == 0) {
				/* BCM89810 phy is assuming a master role (far-end phy is slave) */
				ifmr->ifm_current |= IFM_ETH_MASTER;
			}
			ifmr->ifm_active = ifmr->ifm_current;
			ifmr->ifm_count = 0;
			ifmr->ifm_status = IFM_AVALID;
			if ((mx6q->cfg.flags & NIC_FLAG_LINK_DOWN) == 0) {
				ifmr->ifm_status |= IFM_ACTIVE;
			}
		} else {
			error = ifmedia_ioctl(ifp, ifr,
					      &mx6q->bsd_mii.mii_media, cmd);
		}
		break;


	case SIOCSDRVSPEC:
	case SIOCGDRVSPEC:
		ifd = (struct ifdrv *)data;

		switch (ifd->ifd_cmd) {
#ifdef MX6XSLX
		case AVB_SET_BW:
			error = mx6q_set_tx_bw(mx6q, ifd);
			break;

		case PRECISE_TIMER_DELAY:
			error = mx6q_timer_delay(mx6q, ifd);
			break;
#endif
		case GET_BRCM_SQI:
			error = mx6q_sqi_ioctl(mx6q, ifd);
			break;

		default:
			error = mx6q_ptp_ioctl(mx6q, ifd);
			break;
		}
		break;

    default:
		error = ether_ioctl(ifp, cmd, data);
		if (error == ENETRESET) {
			//
			// Multicast list has changed; set the
			// hardware filter accordingly.
			//
			if ((ifp->if_flags_tx & IFF_RUNNING) == 0) {
				//
				// interface is currently down: mx6q_init()
				// will call mx6q_set_multicast() so
				// nothing to do
				//
			} else {
				//
				// interface is up, recalc and hit gaddrs
				//
				mx6q_set_multicast(mx6q);
			}
			error = 0;
		}
		break;
	}

	return error;
}

int mx6q_sqi_ioctl(mx6q_dev_t *mx6q, struct ifdrv *ifd)
{
    	uint8_t		sqi = mx6q->sqi;

    if (!mx6_is_br_phy(mx6q)) {
		return ENODEV;
	}
	if (ifd->ifd_len != sizeof(sqi)) {
		return EINVAL;
	}
	if (ISSTACK) {
		return (copyout(&sqi, (((uint8_t *)ifd) + sizeof(*ifd)),
					sizeof(sqi)));
	} else {
		memcpy((((uint8_t *)ifd) + sizeof(*ifd)), &sqi, sizeof(sqi));
		return EOK;
	}

    	return ENOTTY;
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/io-pkt/sys/dev_qnx/mx6x/devctl.c $ $Rev: 762475 $")
#endif
