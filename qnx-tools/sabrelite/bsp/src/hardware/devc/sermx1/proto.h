/*
 * $QNXLicenseC:
 * Copyright 2007, 2008, QNX Software Systems.
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








void        create_device(TTYINIT_MX1 *dip, unsigned unit);
void        ser_stty(DEV_MX1 *dev);
void        ser_ctrl(DEV_MX1 *dev, unsigned flags);
void        ser_attach_intr(DEV_MX1 *dev);
void *      query_default_device(TTYINIT_MX1 *dip, void *link);
unsigned    options(int argc, char *argv[]);

/* pulse.c */
#if USE_DMA
int my_attach_pulse ( void **x , struct sigevent *event , void (*handler )(DEV_MX1 *dev ,struct sigevent *event ), DEV_MX1 *dev );
int my_detach_pulse ( void **x );
void mx53_rx_pulse_hdlr(DEV_MX1 *dev, struct sigevent *event);
void mx53_tx_pulse_hdlr(DEV_MX1 *dev, struct sigevent *event);
#endif


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/devc/sermx1/proto.h $ $Rev: 725861 $")
#endif
