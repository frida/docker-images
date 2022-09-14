/*
 * $QNXLicenseC:
 * Copyright 2012, QNX Software Systems.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You
 * may not reproduce, modify or distribute this software except in
 * compliance with the License. You may obtain a copy of the License
 * at: http://www.apache.org/licenses/LICENSE-2.0.
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
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/in.h>
#include <sys/proc.h>

extern int hz;

ptp_extts_t	tx_ts[MX6Q_TX_TIMESTAMP_BUF_SZ];
uint32_t	tx_ts_cnt;
ptp_extts_t	rx_ts[MX6Q_RX_TIMESTAMP_BUF_SZ];
uint32_t	rx_ts_cnt;

uint32_t mx6q_clock_period; /* in nanoseconds */
uint32_t mx6q_clock_freq;   /* in Hz */

void mx6q_ptp_cal (mx6q_dev_t *mx6q)
{
    volatile uint32_t	*base = mx6q->reg;
    struct timespec	tv_start, tv_end;
    uint32_t		diff, count;
    static int		wait;
    int			timo;
    
    /* Stop the timer */
    *(base + MX6Q_TIMER_CTRLR) = 0;

    /* Reset it */
    *(base + MX6Q_TIMER_CTRLR) = MX6Q_TIMER_CTRL_RESTART;

    /* Clock it directly */
    *(base + MX6Q_TIMER_INCR) = 1;
    *(base + MX6Q_TIMER_CORR) = 0;

    /* Note when we start it */
    clock_gettime(CLOCK_MONOTONIC, &tv_start);
    *(base + MX6Q_TIMER_CTRLR) |= MX6Q_TIMER_CTRL_EN;

    /*
     * 1/5 sec is first that works, smaller results in same time
     * from clock_gettime() due to io-pkt timer tick frequency.
     * We will do 1/2 second as a compromise between accurate
     * calibration and not holding up driver init too much.
     */

    if (!ISSTART && ISSTACK) {
	timo = hz / 2;
	ltsleep(&wait, 0, "mx6q_ptp_cal", timo, NULL);
    } else {
	delay(500);
    }

    /* Note when we stop it */
    clock_gettime(CLOCK_MONOTONIC, &tv_end);
    *(base + MX6Q_TIMER_CTRLR) = 0;

    /* Read the timer */
    *(base + MX6Q_TIMER_CTRLR) = MX6Q_TIMER_CTRL_CAPTURE;
    timo = 10;
    while (timo) {
	if ((*(base + MX6Q_TIMER_CTRLR) & MX6Q_TIMER_CTRL_CAPTURE) == 0) {
	    break;
	}
	timo--;
    }
    if (timo <= 0) {
	log(LOG_ERR, "Timestamp capture failed");
    }
    count = *(base + MX6Q_TIMER_VALUER);

    diff = tv_end.tv_nsec - tv_start.tv_nsec;

    if (tv_end.tv_sec > tv_start.tv_sec) {
	diff += 1000 * 1000 * 1000;
    }

    if ((diff > 0) && (count > 0)) {
	mx6q_clock_period = (diff + (count / 2 ))/ count;
	log(LOG_INFO, "Timestamp clock %dMHz", 1000 / mx6q_clock_period);
    } else {
	log(LOG_ERR, "Count %d in %d ns so timestamp clock frequency unknown, defaulting to 125MHz", count, diff);
	mx6q_clock_period = 8;
    }
    mx6q_clock_freq = 1000 * 1000 * 1000 / mx6q_clock_period;
}

/*
 * mx6q_ptp_start()
 *
 * Description: This function sets the default values for
 * the counter and resets the timer.
 *
 * Returns: always returns zero
 *
 */
int mx6q_ptp_start (mx6q_dev_t *mx6q)
{
    volatile uint32_t *base = mx6q->reg;
    static int cal_done = 0;

    if (!cal_done) {
	if (mx6q->clock_freq_set) {
	    mx6q_clock_freq = mx6q->clock_freq;
	    mx6q_clock_period = (1000 * 1000 * 1000) / mx6q_clock_freq;
	} else {
	    mx6q_ptp_cal(mx6q);
	}
	cal_done = 1;
    }

    // Resets the timer to zero
    *(base + MX6Q_TIMER_CTRLR) = MX6Q_TIMER_CTRL_RESTART;

    // Set the clock period for the timestamping
    *(base + MX6Q_TIMER_INCR) = mx6q_clock_period;

    // Set the periodic events value
    *(base + MX6Q_TIMER_PERR) = MX6Q_TIMER_PER1SEC;

    // Enable periodic events
    *(base + MX6Q_TIMER_CTRLR) = MX6Q_TIMER_CTRL_PEREN;

    if (mx6q->cfg.verbose > 3) {
        log(LOG_ERR, "1588 Clock configuration:");
        log(LOG_ERR, "\t..Clock frequency=%u (Hz)", mx6q_clock_freq);
        log(LOG_ERR, "\t..Clock period=%u (ns. per tick)", mx6q_clock_period);
        log(LOG_ERR, "\t..Event period=%u (ns.)", MX6Q_TIMER_PER1SEC);
        log(LOG_ERR, "Enable 1588 clock counter");
    }
    // Enable timer
    *(base + MX6Q_TIMER_CTRLR) |= MX6Q_TIMER_CTRL_EN;

    return 0;
}


/*
 * mx6q_ptp_stop()
 *
 * Description: This function resets the timer and
 * turns it off.
 *
 * Returns: none
 *
 */
void mx6q_ptp_stop (mx6q_dev_t *mx6q)
{

    volatile uint32_t *base = mx6q->reg;

    // Disable timer
    *(base + MX6Q_TIMER_CTRLR) &= ~MX6Q_TIMER_CTRL_EN;

    // Resets the timer to zero
    *(base + MX6Q_TIMER_CTRLR) = MX6Q_TIMER_CTRL_RESTART;

    if (mx6q->cfg.verbose > 3) {
        log(LOG_ERR, "Disable 1588 clock counter");
    }
}


/*
 * mx6q_ptp_is_eventmsg()
 *
 * Description: This function checks the PTP message.
 *
 * Returns: If the frame contains the PTP event
 * message, returns 1, otherwise 0.
 *
 */
int mx6q_ptp_is_eventmsg (struct mbuf *m, ptpv2hdr_t **ph)
{

    int retval = 0;
    struct ether_header *eh;

    if (m == NULL) {
	log(LOG_ERR, "%s: NULL mbuf", __FUNCTION__);
	return 0;
    }

    eh = (struct ether_header *)m->m_data;

    switch(ntohs(eh->ether_type)) {

    case ETHERTYPE_PTP:
        /* This is a native ethernet frame
         * defined for IEEE1588v2
         */
        if (ph != NULL) {
            *ph = (ptpv2hdr_t *)(m->m_data + ETHER_HDR_LEN);
        }
        retval = 1;
        break;

    case ETHERTYPE_IP:
        {
            struct ip *iph = (struct ip *)(m->m_data + ETHER_HDR_LEN);
            struct udphdr *udph = NULL;

            if (((iph != NULL) &&
                (iph->ip_p == IPPROTO_UDP))) {

                // Get UDP header and check for corresponding packet
                udph = (struct udphdr *)((uint32_t *)iph + iph->ip_hl);
                if ((udph != NULL) &&
                    (ntohs(udph->uh_dport) == PTP_UDP_PORT) ) {
                    if (ph != NULL) {
                        *ph = (ptpv2hdr_t *)((uint8_t *)udph + sizeof(struct udphdr));
                    }
                    retval = 1;
                }
            }
        }
    	break;

    default:
        // VLAN & IPv6 protocols are not supported
	break;
    }
    return retval;
}

/*
 * mx6q_ptp_add_rx_timestamp()
 *
 * Description: This function inserts RX timestamp into
 * the corresponding buffer (depends on the message type).
 * If the corresponding buffer already full, the timestamp
 * will be inserted into the start of the buffer
 *
 * Returns: none
 *
 */
void mx6q_ptp_add_rx_timestamp (mx6q_dev_t *mx6q, ptpv2hdr_t *ph, mpc_bd_t *bd)
{
    if ((ph == NULL) || (bd == NULL) ||
        ((ph->version & 0x0f) != 0x2) ) {
        /* Only PTPv2 currently supported */
        return;
    }

    /* Add the details */
    rx_ts[rx_ts_cnt].msg_type = ph->messageId & 0x0f;
    rx_ts[rx_ts_cnt].sequence_id = ntohs(ph->sequenceId);
    memcpy(rx_ts[rx_ts_cnt].clock_identity, ph->clockIdentity,
	   sizeof(rx_ts[rx_ts_cnt].clock_identity));
    rx_ts[rx_ts_cnt].sport_id = ntohs(ph->sportId);
    rx_ts[rx_ts_cnt].ts.nsec = bd->timestamp;
    rx_ts[rx_ts_cnt].ts.sec = mx6q->rtc;

    /* Advance the counter including wrapping */
    rx_ts_cnt = (rx_ts_cnt + 1) % MX6Q_RX_TIMESTAMP_BUF_SZ;

    if (mx6q->cfg.verbose > 4) {
        log(LOG_ERR, "RX 1588 message:");
        log(LOG_ERR, "\t..message = %u", (ph->messageId & 0x0f));
        log(LOG_ERR, "\t..sequence = %u", ntohs(ph->sequenceId));
        log(LOG_ERR, "\t..timestamp = %u.%u\n", mx6q->rtc, bd->timestamp);
    }
}


/*
 * mx6q_ptp_add_tx_timestamp()
 *
 * Description: This function inserts TX timestamp into
 * the corresponding buffer (depends on the message type).
 * If the corresponding buffer already full, the timestamp
 * will be inserted into the start of the buffer
 *
 * Returns: none
 *
 */
void mx6q_ptp_add_tx_timestamp (mx6q_dev_t *mx6q, ptpv2hdr_t *ph, mpc_bd_t *bd)
{
    if ((ph == NULL) || (bd == NULL) ||
        ((ph->version & 0x0f) != 0x2) ) {
        /* Only PTPv2 currently supported */
        return;
    }

    /* Add the details */
    tx_ts[tx_ts_cnt].msg_type = ph->messageId & 0x0f;
    tx_ts[tx_ts_cnt].sequence_id = ntohs(ph->sequenceId);
    memcpy(tx_ts[tx_ts_cnt].clock_identity, ph->clockIdentity,
	   sizeof(tx_ts[tx_ts_cnt].clock_identity));
    tx_ts[tx_ts_cnt].sport_id = ntohs(ph->sportId);
    tx_ts[tx_ts_cnt].ts.nsec = bd->timestamp;
    tx_ts[tx_ts_cnt].ts.sec = mx6q->rtc;

    /* Advance the counter including wrapping */
    tx_ts_cnt = (tx_ts_cnt + 1) % MX6Q_TX_TIMESTAMP_BUF_SZ;

    if (mx6q->cfg.verbose > 4) {
        log(LOG_ERR, "TX 1588 message:");
        log(LOG_ERR, "\t..message = %u", (ph->messageId & 0x0f));
        log(LOG_ERR, "\t..sequence = %u", ntohs(ph->sequenceId));
        log(LOG_ERR, "\t..timestamp = %u.%u\n", mx6q->rtc, bd->timestamp);
    }
}


/*
 * mx6q_ptp_get_rx_timestamp()
 *
 * Description: This function searches a timestamp in the corresponding RX buffer,
 * according to message type, sequence id and the source port id.
 *
 * Returns: Non zero value if the timestamp has been found. Timestamp will
 * be copyied into the passed structure.
 *
 */
int mx6q_ptp_get_rx_timestamp (mx6q_dev_t *mx6q, ptp_extts_t *ts)
{
    int i;

    if (ts == NULL) {
	return 0;
    }

    for (i = 0; i < MX6Q_RX_TIMESTAMP_BUF_SZ; i++) {
	if ((ts->msg_type == rx_ts[i].msg_type) &&
	    (ts->sequence_id == rx_ts[i].sequence_id) &&
	    (ts->sport_id == rx_ts[i].sport_id) &&
	    !memcmp(ts->clock_identity, rx_ts[i].clock_identity,
		    sizeof(ts->clock_identity))) {
	  ts->ts.nsec =  rx_ts[i].ts.nsec;
	  ts->ts.sec =  rx_ts[i].ts.sec;
	  return 1;
	}
    }
    return 0;
}


/*
 * mx6q_ptp_get_tx_timestamp()
 *
 * Description: This function searches a timestamp in the corresponding TX buffer,
 * according to message type, sequence id and the source port id.
 *
 * Returns: Non zero value if the timestamp has been found. Timestamp will
 * be copyied into the passed structure.
 *
 */
int mx6q_ptp_get_tx_timestamp (mx6q_dev_t *mx6q, ptp_extts_t *ts)
{
    int i;

    if (ts == NULL) {
	return 0;
    }

    for (i = 0; i < MX6Q_TX_TIMESTAMP_BUF_SZ; i++) {
	if ((ts->msg_type == tx_ts[i].msg_type) &&
	    (ts->sequence_id == tx_ts[i].sequence_id) &&
	    (ts->sport_id == tx_ts[i].sport_id) &&
	    !memcmp(ts->clock_identity, tx_ts[i].clock_identity,
		    sizeof(ts->clock_identity))) {
	  ts->ts.nsec =  tx_ts[i].ts.nsec;
	  ts->ts.sec =  tx_ts[i].ts.sec;
	  return 1;
	}
    }
    return 0;
}

/*
 * mx6q_ptp_get_cnt()
 *
 * Description: This function returns the current timer value.
 *
 * Returns: none
 *
 */
void mx6q_ptp_get_cnt (mx6q_dev_t *mx6q, ptp_time_t *cnt)
{

    volatile uint32_t *base = mx6q->reg;
    uint32_t tmp;
    int timo;

    if (cnt != NULL) {

	/*
	 * Lock the mutex to stop mx6q_process_interrupt() from
	 * handling the timer interrupt.
	 */
	pthread_mutex_lock(&mx6q->mutex);
REDO:
        // Set capture flag
        tmp = *(base + MX6Q_TIMER_CTRLR);
        tmp |= MX6Q_TIMER_CTRL_CAPTURE;
        *(base + MX6Q_TIMER_CTRLR) = tmp;

	timo = 10;
	while (timo) {
	    if ((*(base + MX6Q_TIMER_CTRLR) & MX6Q_TIMER_CTRL_CAPTURE) == 0) {
		break;
	    }
	    timo--;
	}
	if (timo <= 0) {
	    log(LOG_ERR, "Timestamp capture failed");
	}

        // Capture the timer value
        cnt->nsec = *(base + MX6Q_TIMER_VALUER);
        cnt->sec = mx6q->rtc;

	/*
	 * Make sure the timer didn't wrap while it was being read.
	 * If it did, handle the wrap and capture a new value.
	 */
	if ((*(base + MX6Q_IEVENT) & IEVENT_TS_TIMER) != 0) {
	    mx6q->rtc++;
	    *(base + MX6Q_IEVENT) = IEVENT_TS_TIMER;
	    goto REDO;
	}
	pthread_mutex_unlock(&mx6q->mutex);
    }
}


/*
 * mx6q_ptp_set_cnt()
 *
 * Description: This function sets the current timer value.
 *
 * Returns: none
 *
 */
void mx6q_ptp_set_cnt (mx6q_dev_t *mx6q, ptp_time_t cnt)
{
    volatile uint32_t *base = mx6q->reg;

    pthread_mutex_lock(&mx6q->mutex);
    /* Close window where timer wrap interrupt may fire whilst setting time */
    *(base + MX6Q_TIMER_VALUER) = 0;
    *(base + MX6Q_IEVENT) = IEVENT_TS_TIMER;

    /* Now actually set the time */
    mx6q->rtc = cnt.sec;
    *(base + MX6Q_TIMER_VALUER) = cnt.nsec;
    pthread_mutex_unlock(&mx6q->mutex);
}


/*
 * mx6q_ptp_set_compensation()
 *
 * Description: Sets the clock compensation.
 *
 * Inputs: offset is correction of nanoseconds in 1 second,
 *         i.e. parts-per-billion.
 *         ops is direction of offset, 1 means slow down.
 *
 * Returns: none
 *
 */
void mx6q_ptp_set_compensation (mx6q_dev_t *mx6q, ptp_comp_t ptc)
{

    volatile uint32_t	*base = mx6q->reg;
    uint32_t		inc, err = 0, min_err = ~0;
    uint32_t		i, inc_cor, cor_period, rem_offset;
    uint32_t		test_period, test_offset;

    if (ptc.comp == 0) {
        /* Reset back to default */
	*(base + MX6Q_TIMER_CORR) = 0;
	*(base + MX6Q_TIMER_INCR) = mx6q_clock_period;
	return;
    }

    /* Allow a frequency swing of up to half our clock */
    if (ptc.comp / mx6q_clock_freq > (mx6q_clock_freq / 2)) {
	log(LOG_ERR, "%s: offset %d too big, ignoring",
	    __FUNCTION__, ptc.comp);
	return;
    }

    /* Deal with any major correction by updating the base increment */
    inc = mx6q_clock_period;
    if (ptc.comp >= mx6q_clock_freq) {
	if (ptc.comp) {
	    inc += ptc.comp / mx6q_clock_freq;
	} else {
	    inc -= ptc.comp / mx6q_clock_freq;
	}
    }

    /* Find the best correction factor */
    rem_offset = ptc.comp - (ptc.comp / mx6q_clock_freq);
    inc_cor = inc;
    cor_period = 0;
    for (i = 1; i <= inc; i++) {
	test_period = (mx6q_clock_freq * i) / rem_offset;
	test_offset = (mx6q_clock_freq * i) / test_period;

	/* Integer division so never too small */
	err = test_offset - ptc.comp;
	if (err == 0) {
	    inc_cor = i;
	    cor_period = test_period;
	    break;
	} else if (err < min_err) {
	    inc_cor = i;
	    cor_period = test_period;
	    min_err = err;
	}
    }

    if (ptc.positive) {
	inc_cor = inc + inc_cor;
    } else {
	inc_cor = inc - inc_cor;
    }
    inc |= inc_cor << MX6Q_TIMER_INCR_CORR_OFF;

    *(base + MX6Q_TIMER_INCR) = inc;
    *(base + MX6Q_TIMER_CORR) = cor_period;
} 

/*
 * mx6q_ptp_ioctl()
 *
 * Description: This is a PTP IO control function
 *
 * Returns: Non zero value if the error has been occured.
 *
 */
int mx6q_ptp_ioctl (mx6q_dev_t *mx6q, struct ifdrv *ifd)
{
    ptp_time_t		time;
    ptp_comp_t		comp;
    ptp_extts_t		ts;
    uint8_t		found;

    if (ifd != NULL) {
        switch(ifd->ifd_cmd) {

        case PTP_GET_RX_TIMESTAMP:
        case PTP_GET_TX_TIMESTAMP:
	    if (ifd->ifd_len != sizeof(ts)) {
		return EINVAL;
	    }

	    if (ISSTACK) {
		if (copyin((((uint8_t *)ifd) + sizeof(*ifd)),
			   &ts, sizeof(ts))) {
		    return EINVAL;
		}
	    } else {
		memcpy(&ts, (((uint8_t *)ifd) + sizeof(*ifd)), sizeof(ts));
	    }

	    if (ifd->ifd_cmd == PTP_GET_RX_TIMESTAMP) {
		found = mx6q_ptp_get_rx_timestamp(mx6q, &ts);
	    } else {
		found = mx6q_ptp_get_tx_timestamp(mx6q, &ts);
	    }

	    if (found) {
		if (ISSTACK) {
		    return (copyout(&ts, (((uint8_t *)ifd) + sizeof(*ifd)),
				    sizeof(ts)));
		} else {
		    memcpy((((uint8_t *)ifd) + sizeof(*ifd)), &ts, sizeof(ts));
		    return EOK;
		}
	    }
	    return ENOENT;
	    break;

        case PTP_GET_TIME:
	    if (ifd->ifd_len != sizeof(time)) {
		return EINVAL;
	    }
	    mx6q_ptp_get_cnt(mx6q, &time);
	    if (ISSTACK) {
		return (copyout(&time, (((uint8_t *)ifd) + sizeof(*ifd)),
				sizeof(time)));
	    } else {
		memcpy((((uint8_t *)ifd) + sizeof(*ifd)), &time, sizeof(time));
		return EOK;
	    }
	    break;

        case PTP_SET_TIME:
	    if (ifd->ifd_len != sizeof(time)) {
		return EINVAL;
	    }
	    if (ISSTACK) {
		if (copyin((((uint8_t *)ifd) + sizeof(*ifd)),
			   &time, sizeof(time))) {
		    return EINVAL;
		}
	    } else {
		memcpy(&time, (((uint8_t *)ifd) + sizeof(*ifd)), sizeof(time));
	    }
	    mx6q_ptp_set_cnt(mx6q, time);
	    /* Clock has changed so all old ts are invalid */
	    memset(tx_ts, 0, sizeof(tx_ts));
	    memset(rx_ts, 0, sizeof(rx_ts));
	    return EOK;
	    break;

        case PTP_SET_COMPENSATION:
	    if (ifd->ifd_len != sizeof(comp)) {
		return EINVAL;
	    }
	    if (ISSTACK) {
		if (copyin((((uint8_t *)ifd) + sizeof(*ifd)),
			   &comp, sizeof(comp))) {
		    return EINVAL;
		}
	    } else {
		memcpy(&comp, (((uint8_t *)ifd) + sizeof(*ifd)), sizeof(comp));
	    }
	    mx6q_ptp_set_compensation(mx6q, comp);
	    return EOK;
	    break;

        case PTP_GET_COMPENSATION:
            return ENOTTY;

        default:
            break;
        }
    }
    return ENOTTY;
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/io-pkt/sys/dev_qnx/mx6x/ptp.c $ $Rev: 746169 $")
#endif
