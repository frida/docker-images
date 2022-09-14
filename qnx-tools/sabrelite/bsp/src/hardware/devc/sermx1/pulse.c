/*
 * $QNXLicenseC:
 * Copyright 2008, QNX Software Systems.
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


#include "externs.h"


#define PULSE_CODE_INTERRUPT		1


struct my_pulse_struct
{
    pthread_t tid;
    int      rtn;
    int      chid;
    int      coid;
    struct   sigevent *event;
    void     (*handler) (DEV_MX1 * dev, struct sigevent * event);
    DEV_MX1 *dev;
};


static void *
my_pulse_thread (void *args)
{
    int    priority = 51;
    struct my_pulse_struct *mps = args;
    struct _pulse pulse;

    if ((mps->chid = ChannelCreate (_NTO_CHF_DISCONNECT | _NTO_CHF_UNBLOCK)) == -1 ||
        (mps->coid = ConnectAttach (0, 0, mps->chid, _NTO_SIDE_CHANNEL, 0)) == -1)
    {
        mps->rtn = -1;
        return (NULL);
    }
    SIGEV_PULSE_INIT (mps->event, mps->coid, priority, PULSE_CODE_INTERRUPT, 0);
    mps->rtn = 1;					   /* we're ok signal parent thread */

    for (;;)
    {
        if (MsgReceivePulse (mps->chid, &pulse, sizeof (pulse), NULL) != -1)
        {
            pthread_testcancel ();
            mps->handler (mps->dev, mps->event);
        }
        else
            printf ("MsgReceivePulse() error %s", strerror (errno));
    }

    return (NULL);
}

int
my_attach_pulse (void **x, struct sigevent *event,
    void (*handler) (DEV_MX1 * dev, struct sigevent * event), DEV_MX1 * dev)
{
    struct my_pulse_struct *mps;
    int     policy, err;
    pthread_attr_t attr;
    struct sched_param param;

    if ((mps = (struct my_pulse_struct *) calloc (1, sizeof (struct my_pulse_struct))) == NULL)
        return (-1);

    mps->event = event;
    mps->handler = handler;
    mps->dev = dev;

    pthread_attr_init (&attr);
    pthread_attr_setinheritsched (&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_getschedparam (pthread_self (), &policy, &param);
    param.sched_priority = sched_get_priority_max(SCHED_RR);
    pthread_attr_setschedparam (&attr, &param);
    pthread_attr_setschedpolicy (&attr, SCHED_RR);
    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);

    if ((err = pthread_create (&mps->tid, &attr, my_pulse_thread, mps)) != EOK)
    {
        printf ("unable to attach to pulse (%s)", strerror (err));
        return -err;
    }

    /* wait for the intr thread to fail or succeed */
    while (mps->rtn == 0)
        delay (1);
    if (mps->rtn == -1)
    {
        errno = EINVAL;
        return (-errno);
    }

    *x = mps;
    return (0);
}

int
my_detach_pulse (void **x)
{
    struct my_pulse_struct *mps = *x;

    pthread_cancel (mps->tid);
    ConnectDetach (mps->coid);
    ChannelDestroy (mps->chid);
    return (0);
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/devc/sermx1/pulse.c $ $Rev: 725861 $")
#endif
