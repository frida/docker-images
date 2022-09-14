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
#include <device_qnx.h>


static void mx6q_stop(struct ifnet *ifp, int disable);
static void mx6q_destroy(mx6q_dev_t *mx6q, int how);
static void mx6q_reset(mx6q_dev_t *mx6q);
static int mx6q_init(struct ifnet *ifp);

static int mx6q_attach(struct device *, struct device *, void *);
static int mx6q_detach(struct device *, int);

static void mx6q_shutdown(void *);

static void set_phys_addr (mx6q_dev_t *mx6q);
static void get_phys_addr (mx6q_dev_t *mx6q, uchar_t *addr);

struct mx6q_arg {
    void            *dll_hdl;
    char            *options;
};

CFATTACH_DECL(mx6q,
    sizeof(mx6q_dev_t),
    NULL,
    mx6q_attach,
    mx6q_detach,
    NULL);

static  char *mpc_opts [] = {
    "receive",    // 0
    "transmit",   // 1
    "freq",       // 2
    "rmii",       // 3
    "mii",        // 4
    "bcm89810",   // 5
    NULL
};

void dump_mbuf (struct mbuf *m, uint32_t length)
{
#define MX6Q_LINE_BYTES 16
    char        dump_line[(MX6Q_LINE_BYTES * 3) + 1];
    char       *p;
    uint8_t    *data;
    uint32_t    i;

    p = dump_line;
    data = mtod(m, uint8_t*);

    for (i = 0; i < length; i++) {
        sprintf(p, "%02x ", *data++);

        if ((i % MX6Q_LINE_BYTES) == (MX6Q_LINE_BYTES - 1)) {
            log(LOG_DEBUG, "%s", dump_line);
            p = dump_line;
        } else {
            p += 3;
        }
    }
    if (i % MX6Q_LINE_BYTES) {
        log(LOG_DEBUG, "%s", dump_line);
        p = dump_line;
    }
}

//
// called from mx6q_detect()
//
static int
mx6q_parse_options (mx6q_dev_t *mx6q, char *options, nic_config_t *cfg)
{
    char    *value, *restore, *c;
    int     opt;
    int     rc = 0;
    int     err = EOK;

       /* Getting the ENET Base addresss and irq from the Hwinfo Section if available */
    unsigned hwi_off = hwi_find_device("fec", 0);
    if(hwi_off != HWI_NULL_OFF) {
        hwi_tag *tag = hwi_tag_find(hwi_off, HWI_TAG_NAME_location, 0);
            if(tag) {
                mx6q->iobase = tag->location.base;
                cfg->irq[0] = hwitag_find_ivec(hwi_off, NULL);
            }
    }

    /* GPT base address */
    mx6q->tbase = GPT_BASE;

    restore = NULL;
    while (options && *options != '\0') {
        c = options;
        restore = strchr(options, ',');
        opt = getsubopt (&options, mpc_opts, &value);

        switch (opt) {
        case  0:
                if (mx6q) {
                    mx6q->num_rx_descriptors = strtoul (value, 0, 0);
                }
                break;

        case  1:
                if (mx6q) {
                    mx6q->num_tx_descriptors = strtoul (value, 0, 0);
                }
                break;

        case 2:
            if (mx6q) {
                if (value) {
                    mx6q->clock_freq = strtoul(value, 0, 0);
                    if (mx6q->clock_freq) {
                        mx6q->clock_freq_set = 1;
                    }
                }
            }
            break;

        case 3:
            if (mx6q) {
                mx6q->rmii = 1;
            }
            break;

        case 4:
            if (mx6q) {
                mx6q->mii = 1;
            }
            break;

        case 5:
            if (mx6q  && value) {
                mx6q->bcm89810 = strtoul(value, 0, 0);
            }
            break;

        default:
            if (nic_parse_options (cfg, value) != EOK) {
                    log(LOG_ERR, "%s(): unknown option %s", __FUNCTION__, c);
                    err = EINVAL;
                    rc = -1;
            }
            break;
        }

        if (restore != NULL)
            *restore = ',';

    }

    errno = err;
    return (rc);
}

void *mx6q_rx_thread (void *arg)
{
    mx6q_dev_t        *mx6q = arg;
    int                rc, queue;
    struct _pulse      pulse;
    volatile uint32_t *base = mx6q->reg;

    while (1) {
        rc = MsgReceivePulse(mx6q->chid, &pulse, sizeof(pulse), NULL);
        if (rc == EOK) {
            switch (pulse.code) {
            case MX6Q_RX_PULSE:
                queue = pulse.value.sival_int;
#ifdef MX6XSLX
    	        if (queue == 1) {
    	            *(base + MX6Q_IEVENT) = IEVENT_RXF1;
    	            if (mx6q_receive(mx6q, WTP, 1)) {
    	                InterruptLock(&mx6q->spinlock);
    	                *(base + MX6Q_IMASK) |= IMASK_RXF1EN;
    	                InterruptUnlock(&mx6q->spinlock);
    	            }
    	        } else if (queue == 2) {
    	            *(base + MX6Q_IEVENT) = IEVENT_RXF2;
    	            if (mx6q_receive(mx6q, WTP, 2)) {
    	                InterruptLock(&mx6q->spinlock);
    	                *(base + MX6Q_IMASK) |= IMASK_RXF2EN;
    	                InterruptUnlock(&mx6q->spinlock);
    	            }
    	        } else
#endif
                    if (queue == 0) {
                        *(base + MX6Q_IEVENT) = IEVENT_RFINT;
                        if (mx6q_receive(mx6q, WTP, 0)) {
                            InterruptLock(&mx6q->spinlock);
                            *(base + MX6Q_IMASK) |= IMASK_RFIEN;
                            InterruptUnlock(&mx6q->spinlock);
                        }
                    }
                break;
            case MX6Q_QUIESCE_PULSE:
                quiesce_block(pulse.value.sival_int);
                break;
            default:
                log(LOG_ERR, "mx6 Rx: Unknown pulse %d received", pulse.code);
                break;
            }
        } else {
          log(LOG_ERR, "mx6 Rx: MsgReceivePulse: %s", strerror(rc));
        }
    }
    return NULL;
}

void mx6q_rx_thread_quiesce (void *arg, int die)
{
    mx6q_dev_t            *mx6q = arg;

    MsgSendPulse(mx6q->coid, SIGEV_PULSE_PRIO_INHERIT,
         MX6Q_QUIESCE_PULSE, die);

    return;
}

static int mx6q_rx_thread_init (void *arg)
{
    mx6q_dev_t            *mx6q = arg;
    struct nw_work_thread    *wtp = WTP;

    pthread_setname_np(gettid(), "mx6 Rx");

    wtp->quiesce_callout = mx6q_rx_thread_quiesce;
    wtp->quiesce_arg = mx6q;
    return EOK;
}

//
// convert virtual to physical address
//
static paddr_t
vtophys (void *addr)
{
    off64_t  offset;

    if (mem_offset64 (addr, NOFD, 1, &offset, 0) == -1) {
        return (-1);
    }
    return (offset);
}


//
// called from mx6q_detect() to allocate resources
//

static int
mx6q_attach(struct device *parent, struct device *self, void *aux)
{
    mx6q_dev_t         *mx6q = (mx6q_dev_t *)self;
    nic_config_t       *cfg = &mx6q->cfg;
    int                 rc;
    uint32_t            queue, offset, i;
    struct ifnet       *ifp;
    size_t              size;
    char               *options;
    struct _iopkt_self *iopkt;
    struct nw_stk_ctl  *sctlp;
    struct mx6q_arg    *mx6q_arg;
    mpc_bd_t           *bd;
    struct mbuf        *m;
    volatile uint32_t  *base;

    iopkt = iopkt_selfp;
    sctlp = &stk_ctl;
    mx6q_arg = aux;

    options = mx6q_arg->options;

    mx6q->dev.dv_dll_hdl = mx6q_arg->dll_hdl;
    mx6q->iopkt = iopkt;
    mx6q->iid = -1;

    ifp = &mx6q->ecom.ec_if;
    ifp->if_softc = mx6q;

    if ((mx6q->sdhook = shutdownhook_establish(mx6q_shutdown, mx6q)) == NULL) {
        return ENOMEM;
    }

    cfg->num_irqs = 1;

    /* set some defaults for the command line options */
    cfg->flags = NIC_FLAG_MULTICAST;
    cfg->media_rate = cfg->duplex = -1;
    cfg->priority = IRUPT_PRIO_DEFAULT;
    cfg->iftype = IFT_ETHER;
    cfg->lan = -1;
    cfg->phy_addr = -1;
    strcpy((char *)cfg->uptype, "en");
    //cfg->verbose = 1;  // XXX debug - set verbose=0 for normal output

    mx6q->num_tx_descriptors = DEFAULT_NUM_TX_DESCRIPTORS;
    mx6q->num_rx_descriptors = DEFAULT_NUM_RX_DESCRIPTORS;

    mx6q->probe_phy = 0;

    // set defaults - only used by nicinfo to display mtu
    cfg->mtu = cfg->mru = ETH_MAX_DATA_LEN;

    cfg->device_index = mx6q->dev.dv_unit;
    cfg->lan = mx6q->dev.dv_unit;

    if ((rc = mx6q_parse_options(mx6q, options, cfg))) {
        log(LOG_ERR, "%s(): mx6q_parse_options() failed: %d", __FUNCTION__, rc);
        mx6q_destroy(mx6q, 1);
        return rc;
    }

    if (cfg->verbose) {
        log(LOG_ERR, "%s(): IF %d: Base register 0x%08X",
            __FUNCTION__, cfg->lan, mx6q->iobase);
        log(LOG_ERR, "%s(): IF %d: IRQ %2d",
            __FUNCTION__, cfg->lan, cfg->irq[0]);

    }

    cfg->media = NIC_MEDIA_802_3;
    cfg->mac_length = ETH_MAC_LEN;

    mx6q->force_advertise = -1;
    mx6q->flow = -1;

    // did user specify either of speed or duplex on the cmd line?
    if ((cfg->media_rate != -1) || (cfg->duplex != -1)) {

        if (cfg->media_rate == -1) {
            log(LOG_ERR, "%s(): must also specify speed when duplex is specified", __FUNCTION__);
            mx6q_destroy(mx6q, 1);
            return EINVAL;
        }
        if (cfg->duplex == -1) {
            log(LOG_ERR, "%s(): must also specify duplex when speed is specified", __FUNCTION__);
            mx6q_destroy(mx6q, 1);
            return EINVAL;
        }

        // we get here, we know both media_rate and duplex are set

        mx6q->flow = 0;
        switch(cfg->media_rate) {
        case 0:
            mx6q->force_advertise = 0;  // disable link
            break;

        case 10*1000:
            mx6q->force_advertise = cfg->duplex ? MDI_10bTFD : MDI_10bT;
            break;

        case 100*1000:
            mx6q->force_advertise = cfg->duplex ? MDI_100bTFD : MDI_100bT;
            break;

        case 1000*1000:
            if (mx6q->rmii) {
                log(LOG_ERR, "%s(): RMII doesn't support gigabit, only 10/100 Mb/s", __func__);
                return EINVAL;
                }
            mx6q->force_advertise = cfg->duplex ? MDI_1000bTFD : MDI_1000bT;
            break;

        default:
            log(LOG_ERR, "%s(): invalid speed: %d", __FUNCTION__, cfg->media_rate/1000);
            mx6q_destroy(mx6q, 1);
            return EINVAL;
            break;
        }
    }

    // initialize - until mii callback says we have a link ...
    cfg->flags |= NIC_FLAG_LINK_DOWN;

    cfg->connector = NIC_CONNECTOR_MII;

    cfg->num_mem_windows = 2;
    cfg->mem_window_base[0] = mx6q->iobase;
    cfg->mem_window_size[0] = MX6Q_MAP_SIZE;
    cfg->mem_window_base[1] = mx6q->tbase;
    cfg->mem_window_size[1] = MX6Q_MAP_SIZE;

    strcpy((char *)cfg->device_description, "i.MX6");

    mx6q->num_rx_descriptors &= ~3;
    if (mx6q->num_rx_descriptors < MIN_NUM_RX_DESCRIPTORS) {
        mx6q->num_rx_descriptors = MIN_NUM_RX_DESCRIPTORS;
    }
    if (mx6q->num_rx_descriptors > MAX_NUM_RX_DESCRIPTORS) {
        mx6q->num_rx_descriptors = MAX_NUM_RX_DESCRIPTORS;
    }

    mx6q->num_tx_descriptors &= ~3;
    if (mx6q->num_tx_descriptors < MIN_NUM_TX_DESCRIPTORS) {
        mx6q->num_tx_descriptors = MIN_NUM_TX_DESCRIPTORS;
    }
    if (mx6q->num_tx_descriptors > MAX_NUM_TX_DESCRIPTORS) {
        mx6q->num_tx_descriptors = MAX_NUM_TX_DESCRIPTORS;
    }

    cfg->revision = NIC_CONFIG_REVISION;

    mx6q->cachectl.fd = NOFD;

    if (cache_init(0, &mx6q->cachectl, NULL) == -1) {
        rc = errno;
        log(LOG_ERR, "mx6q_detect: cache_init: %d", rc);
        mx6q_destroy(mx6q, 1);
        return rc;
    }

    // map nic registers into virtual memory
    if ((mx6q->reg = mmap_device_memory (NULL, MX6Q_MAP_SIZE,
                         PROT_READ | PROT_WRITE |
                         PROT_NOCACHE,
                         MAP_SHARED,
                         mx6q->iobase)) == MAP_FAILED) {
        log(LOG_ERR, "%s(): mmap regs failed: %d", __FUNCTION__, rc);
        mx6q_destroy(mx6q, 2);
        return rc;
    }
    base = mx6q->reg;

    /* default MAC address to current ENET hardware setting (comes from boot loader on first boot) */
    get_phys_addr(mx6q, cfg->permanent_address);
    if (memcmp (cfg->current_address, "\0\0\0\0\0\0", 6) == 0)  {
        memcpy(cfg->current_address, cfg->permanent_address,
               ETH_MAC_LEN);
    }

    if (cfg->verbose) {
        nic_dump_config(cfg);
    }

    // Map in uncached memory for tx descr ring
    if ((mx6q->tx_bd = mmap (NULL, sizeof (mpc_bd_t) * mx6q->num_tx_descriptors * NUM_TX_QUEUES,
        PROT_READ | PROT_WRITE | PROT_NOCACHE , MAP_ANON | MAP_PHYS, NOFD, 0)) == MAP_FAILED) {
        log(LOG_ERR, "%s(): mmap txd failed: %d", __FUNCTION__, rc);
        mx6q_destroy(mx6q, 3);
        return rc;
    }

    // alloc mbuf pointer array, corresponding to tx descr ring
    size = sizeof(struct mbuf *) * mx6q->num_tx_descriptors * NUM_TX_QUEUES;
    mx6q->tx_pkts = malloc(size, M_DEVBUF, M_NOWAIT);
    if (mx6q->tx_pkts == NULL) {
        rc = ENOBUFS;
        log(LOG_ERR, "%s(): malloc tx_pkts failed", __FUNCTION__);
        mx6q_destroy(mx6q, 4);
        return rc;
    }
    memset(mx6q->tx_pkts, 0x00, size);

    // init tx descr ring
    for (queue = 0; queue < NUM_TX_QUEUES; queue++) {
        offset =  mx6q->num_tx_descriptors * queue;
        for (i = 0; i < mx6q->num_tx_descriptors; i++) {
            bd = &mx6q->tx_bd[offset + i];
            /* Clear ownership of the descriptor */
            bd->status = 0;
            bd->estatus = (queue << 20) | TXBD_ESTATUS_INT;
            if (i == (mx6q->num_tx_descriptors - 1)) {
                bd->status |= TXBD_W;
            }
        }
        mx6q->tx_pidx[queue] = mx6q->tx_cidx[queue] = mx6q->tx_descr_inuse[queue] = 0;
    }

    // Map in uncached memory for rx descr ring
    if ((mx6q->rx_bd = mmap (NULL, sizeof (mpc_bd_t) * mx6q->num_rx_descriptors * NUM_RX_QUEUES,
        PROT_READ | PROT_WRITE | PROT_NOCACHE , MAP_ANON | MAP_PHYS, NOFD, 0)) == MAP_FAILED) {
        log(LOG_ERR, "%s(): mmap rxd failed: %d", __FUNCTION__, rc);
        mx6q_destroy(mx6q, 5);
        return rc;
    }

    // alloc mbuf pointer array, corresponding to rx descr ring
    size = sizeof(struct mbuf *) * mx6q->num_rx_descriptors * NUM_RX_QUEUES;
    mx6q->rx_pkts = malloc(size, M_DEVBUF, M_NOWAIT);
    if (mx6q->rx_pkts == NULL) {
        rc = ENOBUFS;
        log(LOG_ERR, "%s(): malloc rx_pkts failed", __FUNCTION__);
        mx6q_destroy(mx6q, 6);
        return rc;
    }
    memset(mx6q->rx_pkts, 0x00, size);

    mx6q->chid = ChannelCreate(0);
    mx6q->coid = ConnectAttach(ND_LOCAL_NODE, 0, mx6q->chid,
                   _NTO_SIDE_CHANNEL, 0);

    // init rx descr ring
    for (queue = 0; queue < NUM_RX_QUEUES; queue++) {
        offset =  mx6q->num_rx_descriptors * queue;
        for (i = 0; i < mx6q->num_rx_descriptors; i++) {
            bd = &mx6q->rx_bd[offset + i];
            bd->status = RXBD_E;
            if (i == (mx6q->num_rx_descriptors - 1)) {
                bd->status |= RXBD_W;
            }
            bd->estatus = RXBD_ESTATUS_INT;

            m = m_getcl(M_NOWAIT, MT_DATA, M_PKTHDR);
            if (m == NULL) {
                mx6q_destroy(mx6q, 7);
                return ENOMEM;
            }
            mx6q->rx_pkts[offset + i] = m;
            bd->buffer = pool_phys(m->m_data, m->m_ext.ext_page);
            CACHE_INVAL(&mx6q->cachectl, m->m_data, bd->buffer,
                    m->m_ext.ext_size);
        }
        mx6q->rx_cidx[queue] = 0;
        /*
         * Create a pulse for each class of traffic. Start at the
         * default rx_prio but increment by 2 for each class to
         * allow a process to run at (class - 1) receiving and still
         * have priority over the lower class traffic without
         * impacting the dequeueing of packets from the limited Rx
         * descriptors.
         */
        SIGEV_PULSE_INIT(&mx6q->isr_event[queue], mx6q->coid,
             sctlp->rx_prio + (2 * queue),
             MX6Q_RX_PULSE, queue);
    }

    // one hardware interrupt
    mx6q->inter.func = mx6q_process_interrupt;
    mx6q->inter.enable = mx6q_enable_interrupt;
    mx6q->inter.arg = mx6q;

    if ((rc = interrupt_entry_init(&mx6q->inter, 0, NULL,
        cfg->priority)) != EOK) {
        log(LOG_ERR, "%s(): interrupt_entry_init(rx) failed: %d", __FUNCTION__, rc);
        mx6q_destroy(mx6q, 7);
        return rc;
    }

    /* pseudo interrupt for Rx queue */
    mx6q->inter_queue.func = mx6q_process_queue;
    mx6q->inter_queue.enable = mx6q_enable_queue;
    mx6q->inter_queue.arg = mx6q;

    if ((rc = interrupt_entry_init(&mx6q->inter_queue, 0, NULL,
        cfg->priority)) != EOK) {
        log(LOG_ERR, "%s(): interrupt_entry_init(rx) failed: %d", __FUNCTION__, rc);
        mx6q_destroy(mx6q, 8);
        return rc;
    }

    if ((rc = pthread_mutex_init(&mx6q->rx_mutex, NULL)) != EOK) {
        log(LOG_ERR, "%s(): rx_mutex init failed: %d",
            __FUNCTION__, rc);
        mx6q_destroy(mx6q, 9);
        return rc;
    }

    IFQ_SET_MAXLEN(&mx6q->rx_queue, IFQ_MAXLEN);

    if ((rc = pthread_mutex_init(&mx6q->mutex, NULL)) != EOK) {
        log(LOG_ERR, "%s(): mutex init failed: %d", __FUNCTION__, rc);
        mx6q_destroy(mx6q, 10);
        return rc;
    }

    memset(&mx6q->spinlock, 0, sizeof(mx6q->spinlock));

    nw_pthread_create(&mx6q->tid, NULL,
              mx6q_rx_thread, mx6q, 0,
              mx6q_rx_thread_init, mx6q);

    /* Reset the chip */
    mx6q_reset(mx6q);

    // set addresses of tx, rx descriptor rings
    *(base + MX6Q_X_DES_START) = vtophys((void *)&mx6q->tx_bd[0]);
    *(base + MX6Q_R_DES_START) = vtophys((void *)&mx6q->rx_bd[0]);
#ifdef MX6XSLX
    *(base + MX6Q_X_DES_START1) = vtophys((void *)&mx6q->tx_bd[mx6q->num_tx_descriptors]);
    *(base + MX6Q_R_DES_START1) = vtophys((void *)&mx6q->rx_bd[mx6q->num_rx_descriptors]);
    *(base + MX6Q_X_DES_START2) = vtophys((void *)&mx6q->tx_bd[mx6q->num_tx_descriptors * 2]);
    *(base + MX6Q_R_DES_START2) = vtophys((void *)&mx6q->rx_bd[mx6q->num_rx_descriptors * 2]);
#endif

    // Set transmit FIFO to store and forward
    *(base + MX6Q_X_WMRK) = X_WMRK_STR_FWD;

    /*
     * Set maximum receive buffer size.
     * Freescale have confirmed the spec is wrong, only bits 6-10 should
     * be used.
     */
    *(base + MX6Q_R_BUFF_SIZE) = min(MCLBYTES, MX6Q_MAX_RBUFF_SIZE) &
                    MX6Q_MAX_RBUFF_SIZE;
#ifdef MX6XSLX
    *(base + MX6Q_R_BUFF_SIZE1) = min(MCLBYTES, MX6Q_MAX_RBUFF_SIZE) &
                    MX6Q_MAX_RBUFF_SIZE;
    *(base + MX6Q_R_BUFF_SIZE2) = min(MCLBYTES, MX6Q_MAX_RBUFF_SIZE) &
                    MX6Q_MAX_RBUFF_SIZE;

    /*
     * DMA classes need to be enabled before enabling the chip.
     * There should be no traffic to them without a set bandwidth
     * ioctl call first.
     */
    *(base + MX6Q_DMACFG1) = DMACFG_DMA_CLASSEN;
    *(base + MX6Q_DMACFG2) = DMACFG_DMA_CLASSEN;
#endif
    // Set Rx FIFO thresholds for Pause generation
    *(base + MX6Q_R_SECTION_FULL_ADDR) = 0x10;
    *(base + MX6Q_R_SECTION_EMPTY_ADDR) = 0x82;
    *(base + MX6Q_R_ALMOST_EMPTY_ADDR) = 0x8;
    *(base + MX6Q_R_ALMOST_FULL_ADDR) = 0x8;

    // Enable Pause
    *(base + MX6Q_R_CNTRL) |= RCNTRL_FCE;
    *(base + MX6Q_OP_PAUSE) = 0xFFF0;

    *(base + MX6Q_IADDR1) = 0x0;
    *(base + MX6Q_IADDR2) = 0x0;
    *(base + MX6Q_GADDR1) = 0x0;
    *(base + MX6Q_GADDR2) = 0x0;

    // Enable and clear MIB Registers
    *(base + MX6Q_MIB_CONTROL) &= ~MIB_DISABLE;
    *(base + MX6Q_MIB_CONTROL) |= MIB_CLEAR;
    *(base + MX6Q_MIB_CONTROL) &= ~MIB_CLEAR;
    mx6q_clear_stats(mx6q);

    // Set media interface type
    if (mx6q->rmii) {
        *(base + MX6Q_R_CNTRL) |= RCNTRL_RMII_MODE;
    } else if (mx6q->mii) {
        *(base + MX6Q_R_CNTRL) &= ~(RCNTRL_RGMII_ENA | RCNTRL_RMII_MODE);
    } else {  //ENET defaults to RGMII mode
        *(base + MX6Q_R_CNTRL) |= RCNTRL_RGMII_ENA;
    }

    // As per reference manual this bit should always be 1 - MII/RMII mode
    *(base + MX6Q_R_CNTRL) |= RCNTRL_MII_MODE;

    /*
     * Internal MAC clock is 66MHz so MII_SPEED=0xD and HOLDTIME=0x2
     * See reference manual for field mapping
     */
    *(base + MX6Q_MII_SPEED) = 0x0000011A;

    // Disable internal loopback
    *(base + MX6Q_R_CNTRL) &= ~RCNTRL_LOOP;

    // MAC automatically inserts MAC address in frames
    *(base + MX6Q_X_CNTRL) |= XCNTRL_TX_ADDR_INS;

    // Full duplex by default
    *(base + MX6Q_X_CNTRL) |= XCNTRL_FDEN;

    // Clear interrupt status
    *(base + MX6Q_IEVENT) = 0xffffffff;

    // Attach to hardware interrupts
    if (mx6q->iid == -1) {
        if ((rc = InterruptAttach_r(mx6q->cfg.irq[0], mx6q_isr,
            mx6q, sizeof(*mx6q), _NTO_INTR_FLAGS_TRK_MSK)) < 0) {
            rc = -rc;
            log(LOG_ERR, "%s(): InterruptAttach_r(rx) failed: %d", __FUNCTION__, rc);
            mx6q_destroy(mx6q, 11);
            return rc;
        }
        mx6q->iid = rc;
    }

    /* Map in the timer registers */
    if ((mx6q->treg = mmap_device_memory (NULL, MX6Q_MAP_SIZE,
                          PROT_READ | PROT_WRITE |
                          PROT_NOCACHE,
                          MAP_SHARED,
                          mx6q->tbase)) == MAP_FAILED) {
        log(LOG_ERR, "%s(): mmap timer regs failed: %d",
            __FUNCTION__, rc);
        mx6q_destroy(mx6q, 12);
        return rc;
    }

    /* Enable the interrupts */
    InterruptLock(&mx6q->spinlock);
#ifdef MX6XSLX
    *(base + MX6Q_IMASK) = IMASK_RXF2EN | IMASK_RXF1EN | IMASK_RFIEN |
      IMASK_TS_TIMER | IMASK_TFIEN | IMASK_TXF1EN | IMASK_TXF2EN;
#else
    *(base + MX6Q_IMASK) = IMASK_RFIEN | IMASK_TS_TIMER | IMASK_TFIEN;
#endif
    InterruptUnlock(&mx6q->spinlock);

    // hook up so media devctls work
    bsd_mii_initmedia(mx6q);

    // interface setup - entry points, etc
    strcpy(ifp->if_xname, mx6q->dev.dv_xname);
    ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
    ifp->if_ioctl = mx6q_ioctl;
    ifp->if_start = mx6q_start;
    ifp->if_init = mx6q_init;
    ifp->if_stop = mx6q_stop;
    IFQ_SET_READY(&ifp->if_snd);

    if_attach(ifp);
    ether_ifattach(ifp, mx6q->cfg.current_address);

    mx6q->ecom.ec_capabilities |= ETHERCAP_VLAN_MTU;
    mx6q->ecom.ec_capabilities |= ETHERCAP_JUMBO_MTU;

#ifdef MX6XSLX
    /* Intercept if_output for pulling off AVB packets */
    mx6q->stack_output = mx6q->ecom.ec_if.if_output;
    mx6q->ecom.ec_if.if_output = mx6q_output;
#endif
    // register callbacks with MII managment library
    callout_init(&mx6q->mii_callout);
    rc = MDI_Register_Extended (mx6q, mx6q_mii_write, mx6q_mii_read,
                    mx6q_mii_callback,
                    (mdi_t **)&mx6q->mdi, NULL, 0, 0);
    if (rc != MDI_SUCCESS) {
        log(LOG_ERR, "%s(): MDI_Register_Extended() failed: %d",
            __FUNCTION__, rc);
        mx6q_destroy(mx6q, 11);
        return ENODEV;
    }

    // Register callbacks with SQI calculation
    callout_init(&mx6q->sqi_callout);

    if (cfg->verbose > 3) {
        log(LOG_ERR, "%s(): ending: idx %d\n", __FUNCTION__, cfg->lan);
    }

    return EOK;
}

static void
mx6q_destroy(mx6q_dev_t *mx6q, int how)
{
    struct ifnet        *ifp;
    int            i;
    struct mbuf        *m;

    ifp = &mx6q->ecom.ec_if;

    /* FALLTHROUGH all of these */
    switch (how) {

    //
    // called from mx6q_destroy()
    //
    case -1:
        /*
         * Don't init() while we're dying.  Yes it can happen:
         * ether_ifdetach() calls bridge_ifdetach() which
         * tries to take us out of promiscuous mode with an
         * init().
         */
        mx6q->dying = 1;

        // shut down callbacks
        callout_stop(&mx6q->mii_callout);
        callout_stop(&mx6q->sqi_callout);

        mx6q_reset(mx6q);

        ether_ifdetach(ifp);
        if_detach(ifp);

    //
    // called from mx6q_attach()
    //
    case 13:
        munmap_device_memory(mx6q->treg, MX6Q_MAP_SIZE);
    case 12:
        InterruptDetach(mx6q->iid);
    case 11:
        nw_pthread_reap(mx6q->tid);
        pthread_mutex_destroy(&mx6q->mutex);
    case 10:
        pthread_mutex_destroy(&mx6q->rx_mutex);
    case 9:
        interrupt_entry_remove(&mx6q->inter_queue, NULL);
    case 8:
        interrupt_entry_remove(&mx6q->inter, NULL);
    case 7:
        IF_PURGE(&mx6q->rx_queue);
        pthread_mutex_destroy(&mx6q->rx_mutex);
        ConnectDetach(mx6q->coid);
        ChannelDestroy(mx6q->chid);
        for (i = 0; i < mx6q->num_rx_descriptors * NUM_RX_QUEUES;
             i++) {
            if ((m = mx6q->rx_pkts[i])) {
                m_freem(m);
            }
        }
        free(mx6q->rx_pkts, M_DEVBUF);

    case 6:
        munmap(mx6q->rx_bd, sizeof(mpc_bd_t) *
               mx6q->num_rx_descriptors * NUM_RX_QUEUES);

    case 5:
        for (i = 0; i < mx6q->num_tx_descriptors * NUM_TX_QUEUES;
             i++) {
            if ((m = mx6q->tx_pkts[i])) {
                m_freem(m);
            }
        }
        free(mx6q->tx_pkts, M_DEVBUF);

    case 4:
        munmap(mx6q->tx_bd, sizeof(mpc_bd_t) *
               mx6q->num_tx_descriptors * NUM_TX_QUEUES);

    case 3:
        munmap_device_memory(mx6q->reg, MX6Q_MAP_SIZE);

    case 2:
        cache_fini(&mx6q->cachectl);

    case 1:
        shutdownhook_disestablish(mx6q->sdhook);
        break;
    }
}

static void
mx6q_shutdown(void *arg)
{
    mx6q_dev_t    *mx6q = arg;

    mx6q_reset(mx6q);
}

static int
mx6q_detach(struct device *dev, int flags)
{
    mx6q_dev_t    *mx6q;

    mx6q = (mx6q_dev_t *)dev;
    mx6q_destroy(mx6q, -1);

    return EOK;
}

static void
mx6q_reset (mx6q_dev_t *mx6q)
{
    volatile uint32_t    *base = mx6q->reg;

    /* reset */
    *(base + MX6Q_ECNTRL) = ECNTRL_RESET;

    /* We use little endian enhanced descriptors */
    *(base + MX6Q_ECNTRL) = ECNTRL_DBSWP | ECNTRL_ENA_1588;

    if (*(base + MX6Q_ECNTRL) & ECNTRL_RESET) {
        log(LOG_ERR, "%s(): Reset bit didn't clear", __FUNCTION__);
    }

    // re program MAC address to PADDR registers
    set_phys_addr(mx6q);
}

static void
set_phys_addr (mx6q_dev_t *mx6q)
{
    // Program MAC address provided from command line argument
    uint32_t    *base = mx6q->reg;
    *(base + MX6Q_PADDR1) = (mx6q->cfg.current_address [0] << 24 |
        mx6q->cfg.current_address [1] << 16 | mx6q->cfg.current_address [2] << 8 |
        mx6q->cfg.current_address [3]);
    *(base + MX6Q_PADDR2) = (mx6q->cfg.current_address [4] << 24 |
        mx6q->cfg.current_address [5] << 16 | 0x8808);

}

/*
 *    Read MAC address from ENET hardware.
 *    Set by bootloader on reset (likely from Fuse registers
 *    but can be overridden).
 */
static void
get_phys_addr (mx6q_dev_t *mx6q, uchar_t *addr)
{
    uint32_t    *base = mx6q->reg;
    uint32_t    mx6q_paddr;
    mx6q_paddr = *(base + MX6Q_PADDR1);
    addr [0] = (mx6q_paddr >> 24) & ~0x01; /* clear multicast bit for sanity's sake */
    addr [1] = mx6q_paddr >> 16;
    addr [2] = mx6q_paddr >> 8;
    addr [3] = mx6q_paddr;
    mx6q_paddr = *(base + MX6Q_PADDR2);
    addr [4] = mx6q_paddr >> 24;
    addr [5] = mx6q_paddr >> 16;
}

void DumpMAC(mx6q_dev_t *mx6q)
{
volatile uint32_t    *base = mx6q->reg;

    uint32_t i,j;
    log(LOG_INFO,"--------DumpMAC----------");
    for(i=0,j= (uint32_t)base;i<0x200/4;i++){
        if(!(i%4)){
            log(LOG_INFO,"%08x %08x %08x %08x %08x",j, *((uint32_t*)base + i),*((uint32_t*)base + i+1),*((uint32_t*)base + i+2),*((uint32_t*)base + i+3));
            j+=0x10;
        }
    }
    log(LOG_INFO,"--------DumpMAC END----------\r\n");


//    EdbgOutputDebugString("EIR:0x%08x\r\n",HW_ENET_MAC_EIR_RD(MAC0));
//    EdbgOutputDebugString("EIMR:0x%08x\r\n",HW_ENET_MAC_EIMR_RD(MAC0));
//    EdbgOutputDebugString("RDAR:0x%08x\r\n",HW_ENET_MAC_RDAR_RD(MAC0));
//    EdbgOutputDebugString("TDAR:0x%08x\r\n",HW_ENET_MAC_TDAR_RD(MAC0));
//    EdbgOutputDebugString("ECR:0x%08x\r\n",HW_ENET_MAC_ECR_RD(MAC0));
//    EdbgOutputDebugString("MMFR:0x%08x\r\n",HW_ENET_MAC_MMFR_RD(MAC0));
//    EdbgOutputDebugString("MSCR:0x%08x\r\n",HW_ENET_MAC_MSCR_RD(MAC0));
//    EdbgOutputDebugString("MIBC:0x%08x\r\n",HW_ENET_MAC_MIBC_RD(MAC0));
//    EdbgOutputDebugString("RCR:0x%08x\r\n",HW_ENET_MAC_RCR_RD(MAC0));
//    EdbgOutputDebugString("TCR:0x%08x\r\n",HW_ENET_MAC_TCR_RD(MAC0));
//    EdbgOutputDebugString("PALR:0x%08x\r\n",HW_ENET_MAC_PALR_RD(MAC0));
//    EdbgOutputDebugString("PAUR:0x%08x\r\n",HW_ENET_MAC_PAUR_RD(MAC0));
//    EdbgOutputDebugString("OPD:0x%08x\r\n",HW_ENET_MAC_OPD_RD(MAC0));
//    EdbgOutputDebugString("IAUR:0x%08x\r\n",HW_ENET_MAC_IAUR_RD(MAC0));
//    EdbgOutputDebugString("IALR:0x%08x\r\n",HW_ENET_MAC_IALR_RD(MAC0));
//    EdbgOutputDebugString("GAUR:0x%08x\r\n",HW_ENET_MAC_GAUR_RD(MAC0));
//    EdbgOutputDebugString("DMA1:0x%08x\r\n",HW_ENET_MAC_ERDSR_RD(MAC0));
//    EdbgOutputDebugString("DMA2:0x%08x\r\n",HW_ENET_MAC_ETDSR_RD(MAC0));
//    EdbgOutputDebugString("--------DumpMAC END----------\r\n");
}


void DumpPhy(mx6q_dev_t *mx6q)
{
    uint32_t i;
    nic_config_t    *cfg    = &mx6q->cfg;
    int        phy_idx    = cfg->phy_addr;


//    TCHAR PhyReg[]={
//        {TEXT("Control")},
//        {TEXT("Status")},
//        {TEXT("Phy1")},
//        {TEXT("Phy2")},
//        {TEXT("Auto-Nego")},
//        {TEXT("Link partner")},
//        {TEXT("Auto-Nego_ex")},
//        {TEXT("Next page")},
//        {TEXT("Link Partner next")},
//        {TEXT("1000 BT ctl")},
//        {TEXT("100 BT stat")},
//        {TEXT("RSVD")},
//        {TEXT("RSVD")},
//        {TEXT("MMD control")},
//        {TEXT("MMD data")},
//        {TEXT("ext status")},
//        {TEXT("func ctl")},
//        {TEXT("phy status")},
//        {TEXT("intr en")},
//        {TEXT("intr stat")},
//        {TEXT("smart speed")},
//    };
    log(LOG_INFO,"--------DumpPhy----------\r\n");
    for(i=0;i<0x20;i+=4)
    {
        log(LOG_INFO,"phy[%d]:0x%x phy[%d]:0x%x phy[%d]:0x%x phy[%d]:0x%x",
            i,mx6q_mii_read(mx6q, phy_idx,i),i+1,mx6q_mii_read(mx6q, phy_idx,i+1),i+2,mx6q_mii_read(mx6q, phy_idx,i+2),i+3,mx6q_mii_read(mx6q, phy_idx,i+3));
    }
    log(LOG_INFO,"--------DumpPhy END----------\r\n");
}

static int
mx6q_init (struct ifnet *ifp)
{
    mx6q_dev_t        *mx6q = ifp->if_softc;
    nic_config_t      *cfg = &mx6q->cfg;
    volatile uint32_t *base = mx6q->reg;
    int                mtu;
    uint32_t           rcntrl;
    static int         phy_init_done = 0;

    if (cfg->verbose > 3) {
        log(LOG_ERR, "%s(): starting: idx %d\n",
            __FUNCTION__, cfg->device_index);
    }

    if (mx6q->dying) {
        log(LOG_ERR, "%s(): dying", __FUNCTION__);
        return EOK;
    }

    // Sort out MAC address
    memcpy(cfg->current_address, LLADDR(ifp->if_sadl), ifp->if_addrlen);
        if (memcmp (cfg->current_address, "\0\0\0\0\0\0", 6) == 0) {
            log(LOG_ERR, "%s():You must specify a MAC address", __FUNCTION__);
            return EINVAL;
        }

    // write to MX6Q_PADDR1 and MX6Q_PADDR2 from cfg->current_address
    set_phys_addr(mx6q);

    // Promiscuous if required
    if (ifp->if_flags & IFF_PROMISC) {
        *(base + MX6Q_R_CNTRL) |= RCNTRL_PROM;
    } else {
        *(base + MX6Q_R_CNTRL) &= ~RCNTRL_PROM;
    }

    // Get mtu from stack for nicinfo
    cfg->mtu = ifp->if_mtu;
    cfg->mru = ifp->if_mtu;

    // Program it in to the MAC
    mtu = (ifp->if_mtu + ETHER_HDR_LEN + ETHER_CRC_LEN);
    if (mx6q->ecom.ec_capenable & ETHERCAP_VLAN_MTU) {
        mtu += ETHER_VLAN_ENCAP_LEN;
    }
    if (mtu != cfg->mtu) {
        rcntrl = *(base + MX6Q_R_CNTRL);
        rcntrl &= 0xc000ffff;
        rcntrl |= (mtu << 16);
        *(base + MX6Q_R_CNTRL) = rcntrl;
        *(base + MX6Q_TRUNC_FL_ADDR) = mtu;
    }

    // get PHY address
    if (cfg->phy_addr == -1) {
        cfg->phy_addr = mx6_get_phy_addr(mx6q);
    }

    // Do one time PHY initialisation
    if ((cfg->phy_addr != -1) && (phy_init_done == 0)) {
        phy_init_done = 1;
        if (mx6_is_br_phy(mx6q)) {
            mx6_broadreach_phy_init(mx6q);
        } else {
            mx6_setup_phy(mx6q);
            switch (mx6q->mdi->PhyData[cfg->phy_addr]->VendorOUI) {
            case ATHEROS:
                switch (mx6q->mdi->PhyData[cfg->phy_addr]->Model) {
                case AR8031:
                    if (cfg->verbose > 3)
                        log(LOG_INFO, "Detected Atheros AR8031 PHY");
                    mx6_sabreauto_rework(mx6q);
                    break;
                default:
                    break;
                }
                break;

            case KENDIN:
                switch (mx6q->mdi->PhyData[cfg->phy_addr]->Model) {
                case KSZ9021:
                    if (cfg->verbose > 3)
                        log(LOG_INFO, "Detected Micrel KSZ9021 PHY");
                    mx6_sabrelite_phy_init(mx6q);
                    break;
                case KSZ8081:
                    if (cfg->verbose > 3)
                        log (LOG_INFO, "Detected Micrel KSZ8081 PHY");
                default:
                    break;
                }
                break;

            case BROADCOM2:
                switch (mx6q->mdi->PhyData[cfg->phy_addr]->Model) {
                case BCM54616:
                    mx6_paves3_phy_init(mx6q);
                break;
                case BCM89810:
                    if (cfg->verbose > 3) {
                        log(LOG_INFO, "Detected Broadcom BCM89810 PHY");
                    }
                    break;
                default:
                    break;
                }
                break;

            case MARVELL:
                switch (mx6q->mdi->PhyData[cfg->phy_addr]->Model) {
                case ALASKA:
                    mx6_paves3_twelve_inch_phy_init(mx6q);
                    break;
                default:
                    break;
                }
                break;

            default:
                break;
            }
            /*
             * Now that PHY specific initialisation is finished,
             * perform auto negotiation or bypass auto-negotiation if
             * user passed command line link settings.
             */
            mx6q_init_phy(mx6q);
        }
    }

    // arm callout which detects PHY link state changes
    callout_msec(&mx6q->mii_callout, 2 * 1000, mx6q_MDI_MonitorPhy, mx6q);

    // arm callout to check sqi
    if (mx6_is_br_phy(mx6q))
        callout_msec(&mx6q->sqi_callout, MX6Q_SQI_SAMPLING_INTERVAL * 1000, mx6q_BRCM_SQI_Monitor, mx6q);

    // PHY is now ready, turn on MAC
    *(base + MX6Q_ECNTRL) |= ECNTRL_ETHER_EN;
    *(base + MX6Q_ECNTRL) &= ~ECNTRL_SLEEP;

    // Instruct MAC to process recieve frames
    *(base + MX6Q_R_DES_ACTIVE) = R_DES_ACTIVE;

    // Start 1588 timer
    mx6q_ptp_start(mx6q);

    NW_SIGLOCK(&ifp->if_snd_ex, mx6q->iopkt);
    ifp->if_flags_tx |= IFF_RUNNING;
    NW_SIGUNLOCK(&ifp->if_snd_ex, mx6q->iopkt);
    ifp->if_flags |= IFF_RUNNING;

    if (cfg->verbose > 3) {
        log(LOG_ERR, "%s(): ending: idx %d\n",
          __FUNCTION__, cfg->device_index);
    }

    //DumpMAC(mx6q);
    //DumpPhy(mx6q);

    return EOK;
}

void
mx6q_speeduplex (mx6q_dev_t *mx6q)
{
    nic_config_t        *cfg = &mx6q->cfg;
    volatile uint32_t    *base = mx6q->reg;

    if (cfg->duplex) {
        *(base + MX6Q_X_CNTRL) |= XCNTRL_FDEN;
        *(base + MX6Q_R_CNTRL) &= ~RCNTRL_DRT;
    } else {
        *(base + MX6Q_X_CNTRL) &= ~XCNTRL_FDEN;
        *(base + MX6Q_R_CNTRL) |= RCNTRL_DRT;
    }

    switch (cfg->media_rate) {
    case 1000 * 1000L:
        *(base + MX6Q_ECNTRL) |= ECNTRL_ETH_SPEED;
        break;
    case 100 * 1000L:
        *(base + MX6Q_ECNTRL) &= ~ECNTRL_ETH_SPEED;
        *(base + MX6Q_R_CNTRL) &= ~RCNTRL_RMII_10T;
        break;
    case 10 * 1000L:
        *(base + MX6Q_R_CNTRL) |= RCNTRL_RMII_10T;
        *(base + MX6Q_ECNTRL) &= ~ECNTRL_ETH_SPEED;
        break;
    default:
        /* Speed 0, PHY powered down, MAC doesn't matter */
        break;
    }
}

static void
mx6q_stop(struct ifnet *ifp, int disable)
{
    mx6q_dev_t        *mx6q = ifp->if_softc;
    uint32_t           rx, tx, tx_active, i, queue, offset;
    mpc_bd_t          *bd;
    struct mbuf       *m;
    volatile uint32_t *base = mx6q->reg;

    // shut down mii probing
    callout_stop(&mx6q->mii_callout);

    callout_stop(&mx6q->sqi_callout);

    // stop the MAC by putting it to sleep
    *(base + MX6Q_ECNTRL) |= ECNTRL_SLEEP;
    for(;;) {
        /*
         * Receive is always active so wait for GRS.
         * If transmit is active then wait for GTS.
         */
        rx = *(base + MX6Q_R_CNTRL);
        tx = *(base + MX6Q_X_CNTRL);
        tx_active = *(base + MX6Q_X_DES_ACTIVE);
#ifdef MX6XSLX
        tx_active |= *(base + MX6Q_X_DES_ACTIVE1);
        tx_active |= *(base + MX6Q_X_DES_ACTIVE2);
#endif
        if ((rx & RCNTRL_GRS) &&
            ((tx_active == 0) || (tx & XCNTRL_GTS))) {
            break;
        }
    }

    // Stop 1588 timer
    mx6q_ptp_stop(mx6q);

    /* Lock out the transmit side */
    NW_SIGLOCK(&ifp->if_snd_ex, mx6q->iopkt);
    ifp->if_flags_tx &= ~IFF_OACTIVE;

    for (queue = 0; queue < NUM_TX_QUEUES; queue++) {
        offset = mx6q->num_tx_descriptors * queue;

        /* Reap any Tx descriptors */
        mx6q_transmit_complete(mx6q, queue);

        /* Clear any pending Tx buffers */
        for (i = 0; i < mx6q->num_tx_descriptors; i++) {
            if ((m = mx6q->tx_pkts[offset + i])) {
                m_freem(m);
                mx6q->tx_pkts[offset + i] = NULL;
                bd = &mx6q->tx_bd[offset + i];
                /* Clear the TXBD_R */
                bd->status &= TXBD_W;
            }
        }
        /*
         * Make sure we start queueing descriptors
         * from where the hardware stopped
         */
        mx6q->tx_pidx[queue] = mx6q->tx_cidx[queue];
        mx6q->tx_descr_inuse[queue] = 0;
    }

    /* Tx is clean, unlock ready for next time */
    NW_SIGUNLOCK(&ifp->if_snd_ex, mx6q->iopkt);

    /* Mark the interface as down */
    ifp->if_flags &= ~(IFF_RUNNING);
}

//
// called from mx6q_entry() in mx6q.c
//
int
mx6q_detect(void *dll_hdl, struct _iopkt_self *iopkt, char *options)
{
    int              single;
    struct mx6q_arg  mx6q_arg;
    struct device   *dev;

    mx6q_arg.dll_hdl = dll_hdl;
    mx6q_arg.options = options;

    dev = NULL; /* No parent */
    return (dev_attach("fec", options, &mx6q_ca, &mx6q_arg, &single,
               &dev, NULL));
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/io-pkt/sys/dev_qnx/mx6x/detect.c $ $Rev: 767545 $")
#endif
