/*
 * $QNXLicenseC:
 * Copyright 2012, QNX Software Systems.
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

/*
 * tsc2004.c
 *
 * Driver for the TI TSC2004 low-power touch screen controller.
 * A Combination of Device and Protocol modules.
 */


#include <sys/devi.h>
#include "tsc2004.h"

input_module_t  tsc2004 = {
        NULL,                  /* up, filled in at runtime */
        NULL,                  /* down, filled in at runtime */
        NULL,                  /* line we belong to, filled in at runtime */
        0,                     /* flags, leave as zero */
        DEVI_CLASS_ABS | DEVI_MODULE_TYPE_PROTO | DEVI_MODULE_TYPE_DEVICE,
                               /* Our type, we are a
                                * protocol module or class
                                * relative. This info will
                                * tell the runtime system
                                * which filter module to
                                * link above us
                                */
        "tsc2004",             /* name, must match what you specify
                                * on the command line when invoking this
                                * module
                                */
        __DATE__,              /* date of compilation, used for output when
                                * the cmdline option -l is given to the
                                * driver
                                */
        "i:a:c:vd:D:p:j:",   /* command line parameters */
        NULL,                  /* pointer to private data, set this up
                                * in the init() callback
                                */
        tsc2004_init,          /* init() callback, required */
        tsc2004_reset,         /* reset() callback, required */
        NULL,                  /* input() callback */
        NULL,                  /* output(), not used */
        tsc2004_pulse,         /* pulse(), called when the timer expires
                                * used for injecting a release event
                                */
        tsc2004_parm,          /* parm() callback, required */
        tsc2004_devctrl,       /* devctrl() callback */
        tsc2004_shutdown       /* shutdown() callback, required */
};

static tsc_sr_t sndrcv;

static int tsc2004_prepare_for_reading(void *data)
{
	input_module_t    *module = (input_module_t *) data;
	private_data_t    *dp = module->data;
	int sts = EOK;

	/* Reset the TSC, configure for 12 bit */
    /* Fixme: Actually we don't need the Z1 and Z2, but it doesn't work on MEAS_X_Y function */
	sndrcv.data[0] = TSC2004_CMD1(MEAS_X_Y_Z1_Z2, MODE_12BIT, SWRST_TRUE);
	sts = devctl(dp->fd, DCMD_I2C_SENDRECV, &sndrcv, sizeof(sndrcv), NULL);
	if (sts != EOK) {
		slogf(_SLOGC_INPUT, _SLOG_ERROR, "%s - %d: devctl failed (sts=%d)", __FUNCTION__, __LINE__, sts);
		return -1;
	}

	/* Enable interrupt for PENIRQ and DAV */
	sndrcv.sr.send_len = 3;
	sndrcv.data[0] = TSC2004_CMD0(CFR2_REG, PND0_FALSE, WRITE_REG);
	sndrcv.data[1] = PINTS1 | PINTS0 | MEDIAN_VAL_FLTR_SIZE_15 | AVRG_VAL_FLTR_SIZE_7_8;
	sndrcv.data[2] = MAV_FLTR_EN_X | MAV_FLTR_EN_Y | MAV_FLTR_EN_Z;

	sts = devctl(dp->fd, DCMD_I2C_SENDRECV, &sndrcv, sizeof(sndrcv), NULL);
	if (sts != EOK) {
		slogf(_SLOGC_INPUT, _SLOG_ERROR, "%s - %d: devctl failed (sts=%d)", __FUNCTION__, __LINE__, sts);
		return -1;
	}

	/* Configure the TSC in TSMode 1 */
	sndrcv.data[0] = TSC2004_CMD0(CFR0_REG, PND0_FALSE, WRITE_REG);
	sndrcv.data[1] = PEN_STS_CTRL_MODE | RES_CTRL | ADC_CLK_2MHZ | PANEL_VLTG_STB_TIME_1MS;
	sndrcv.data[2] = PRECHARGE_TIME_84US | SENSE_TIME_SEL_96US;
	sts = devctl(dp->fd, DCMD_I2C_SENDRECV, &sndrcv, sizeof(sndrcv), NULL);
	if (sts != EOK) {
		slogf(_SLOGC_INPUT, _SLOG_ERROR, "%s - %d: devctl failed (sts=%d)", __FUNCTION__, __LINE__, sts);
		return -1;
	}

	sndrcv.sr.send_len = 1;

	/* Clear the reset state */
	sndrcv.data[0] = TSC2004_CMD1(MEAS_X_Y_Z1_Z2, MODE_12BIT, SWRST_FALSE);
	sts = devctl(dp->fd, DCMD_I2C_SENDRECV, &sndrcv, sizeof(sndrcv), NULL);
	if (sts != EOK) {
		slogf(_SLOGC_INPUT, _SLOG_ERROR, "%s - %d: devctl failed (sts=%d) for Y", __FUNCTION__, __LINE__, sts);
		return -1;
	}

	return 0;
}

/*
 * tsc2004_init()
 *
 * This is our init callback. Allocate space for our private data and assign
 * it to the data member of the module structure. Simple initialization.
 */
static int tsc2004_init(input_module_t *module)
{
	private_data_t *dp;

	if (module) {
		dp = module->data;
	} else {
		slogf (_SLOGC_INPUT, _SLOG_ERROR, "%s: module is NULL", __FUNCTION__);
		exit (-1);
	}

	if(!dp) {
		if (!(dp = module->data = scalloc(sizeof *dp)))
			return (-1);

		ThreadCtl (_NTO_TCTL_IO, 0);

		dp->flags = FLAG_RESET;
		dp->irq = TSC2004_PENIRQ;
		dp->irq_pc = DEVI_PULSE_ALLOC;
		dp->param.sched_priority = PULSE_PRIORITY;
		dp->event.sigev_priority = dp->param.sched_priority;
		dp->release_delay = RELEASE_DELAY;
		dp->intr_delay = INTR_DELAY;
		dp->jitter_delta = JITTER_DELTA;
		dp->touch_x = 0;
		dp->touch_y = 0;
		dp->last_buttons = 0;

		dp->speed = TSC_I2C_SPEED;
		dp->slave.addr = TSC_ADDRESS;
		dp->slave.fmt = I2C_ADDRFMT_7BIT;

		dp->tp.x = 0;
		dp->tp.y = 0;
		dp->tp.z = 0;
		dp->tp.buttons = 0;

		dp->i2c = malloc(strlen(TSC_I2C_DEVICE) + 1);
		if (!dp->i2c) {
			slogf(_SLOGC_INPUT, _SLOG_ERROR, "%s: malloc failed for dev name", __FUNCTION__);
			free(dp);
			return (-1);
		}

		strcpy(dp->i2c, TSC_I2C_DEVICE);
		pthread_mutex_init (&dp->mutex, NULL);
		tsc2004_prepare_for_reading(module);
	}
	return (0);
}

/* tsc2004_parm()
 *
 * Our callback to be called by the input runtime system to parse any
 * command line parameters given to the tsc2004 module.
 */
static int tsc2004_parm(input_module_t *module, int opt, char *optarg)
{
	private_data_t *dp;
	long int	v;

	if (module && module->data) {
		dp = module->data;
	} else {
		slogf (_SLOGC_INPUT, _SLOG_ERROR, "%s: module%s is NULL", __FUNCTION__, (module ? "->data" : ""));
		exit (-1);
	}

	switch (opt) {
		case 'v':   /* Verbosity */
				dp->verbose++;
				break;
		case 'i':   /* IRQ number */
				dp->irq = atoi (optarg);
				break;
		case 'a':   /* TSC2004 I2C slave address */
				v = strtol(optarg, 0, 16);
				if (v > 0 && v < 128)
					dp->slave.addr = v;
				break;
		case 'c':   /* TSC2004 I2C device */
				if (strcmp(optarg, dp->i2c) != 0) {
					free(dp->i2c);
					dp->i2c = malloc(strlen(optarg) + 1);

					if (!dp->i2c) {
						slogf(_SLOGC_INPUT, _SLOG_ERROR, "%s: malloc failed for dev name", __FUNCTION__);
						return -1;
					}
					strcpy(dp->i2c, optarg);
				}
				break;
		case 'p':  /* Schedule priority */
				dp->param.sched_priority = atoi (optarg);
				dp->event.sigev_priority = dp->param.sched_priority;
				break;
		case 'd':  /* Timer delay period */
				dp->release_delay = (atol (optarg)) * 1000000;   /* Convert to nsecs */
				break;
		case 'D':
				dp->intr_delay = atoi (optarg);
				break;
		case 'j':
				dp->jitter_delta = atoi (optarg);
				break;
		default:
				fprintf(stderr, "Unknown option %c\n", opt);
				break;
	}

	return (0);
}

/* tsc2004_reset()
 *
 * The reset callback is called when our module is linked into an
 * event bus line. In here we will setup our device for action.
 *
 * We also create a timer, the purpose of this timer is to check whether the
 * Pendown state since the TSC2004 touch controller does not trigger interrupt
 * for the release event
 *
 * Also create a separate thread to handle the IRQs from the TSC2004.
 */
static int tsc2004_reset(input_module_t *module)
{
	private_data_t	*dp;
	int sts;

	if (module && module->data) {
		dp = module->data;
	} else {
		slogf (_SLOGC_INPUT, _SLOG_ERROR, "%s: module%s is NULL", __FUNCTION__, (module ? "->data" : ""));
		exit (-1);
	}

	if((dp->flags & FLAG_INIT) == 0) {
		/* Initialize I2C interface */
		dp->fd = open(dp->i2c, O_RDWR);
		if (dp->fd < 0) {
			slogf(_SLOGC_INPUT, _SLOG_ERROR, "%s: failure in opening I2C device %s", __FUNCTION__, dp->i2c);
			exit (-1);
		}

		sts = devctl(dp->fd, DCMD_I2C_SET_BUS_SPEED, &dp->speed, sizeof(dp->speed), NULL);
		if (sts != EOK) {
			slogf(_SLOGC_INPUT, _SLOG_ERROR, "%s: failed to set speed", __FUNCTION__);
			exit (-1);
		}

		memset(&sndrcv, 0, sizeof(sndrcv));
		sndrcv.sr.slave = dp->slave;
		sndrcv.sr.stop = 1;
		sndrcv.sr.send_len = 1;
		sndrcv.sr.recv_len = 2;

		/* Create touch release timer */
		dp->timerid = devi_register_timer (module, 15, &dp->irq_pc, NULL);

		/* Setup the interrupt handler thread */
		if ((dp->chid = ChannelCreate (_NTO_CHF_DISCONNECT | _NTO_CHF_UNBLOCK)) == -1) {
			perror ("Error: ChannelCreate");
			exit (-1);
		}

		if ((dp->coid = ConnectAttach (0, 0, dp->chid, _NTO_SIDE_CHANNEL, 0)) == -1)
		{
			perror ("Error: ConnectAttach");
			exit (-1);
		}

		pthread_attr_init (&dp->pattr);
		pthread_attr_setschedpolicy (&dp->pattr, SCHED_RR);
		pthread_attr_setschedparam (&dp->pattr, &dp->param);
		pthread_attr_setinheritsched (&dp->pattr, PTHREAD_EXPLICIT_SCHED);
		pthread_attr_setdetachstate (&dp->pattr, PTHREAD_CREATE_DETACHED);
		pthread_attr_setstacksize (&dp->pattr, 4096);

		dp->event.sigev_notify     = SIGEV_PULSE;
		dp->event.sigev_coid       = dp->coid;
		dp->event.sigev_code       = 1;

		/* Create interrupt handler thread */
		if (pthread_create (NULL, &dp->pattr, (void *)intr_thread, module)) {
			perror ("Error: pthread_create");
			exit (-1);
		}

		/* Attach interrupt. */
		if (dp->verbose >= 3)
			fprintf (stderr, "Attaching to interrupt %d\n", dp->irq);

		if ((dp->iid = InterruptAttachEvent (dp->irq, &dp->event, _NTO_INTR_FLAGS_TRK_MSK)) == -1) {
			perror ("Error: InterruptAttachEvent");
			exit (-1);
		}

		dp->flags |= FLAG_INIT;
	}
	return (0);
}

/*
 * tsc2004_devctrl()
 *
 * Our callback to be used by modules in an event bus line to send
 * information further up the line to other modules (e.g. abs). This
 * allows the other modules to know how many buttons we have, pointer
 * coordinates, and the range of the coordinates.
 */
static int tsc2004_devctrl(input_module_t *module, int event, void *ptr)
{
	private_data_t	*dp;

	if (module && module->data) {
		dp = module->data;
	} else {
		slogf (_SLOGC_INPUT, _SLOG_ERROR, "%s: module%s is NULL", __FUNCTION__, (module ? "->data" : ""));
		exit (-1);
	}

	switch(event) {
		case DEVCTL_GETDEVFLAGS:
			*(unsigned short *)ptr = (dp->flags & FLAGS_GLOBAL);
			break;
		case DEVCTL_GETPTRBTNS:
			*(unsigned long *)ptr = 1L;
			break;
		case DEVCTL_GETPTRCOORD:
			*(unsigned char *)ptr = '\02';
			break;
		case DEVCTL_GETCOORDRNG:
		{
			struct devctl_coord_range *range = ptr;

			range->min = 0;
			range->max = 4096;
			break;
		}
		default:
			return (-1);
	}

	return (0);
}

static int tsc2004_read_data(unsigned int *x, unsigned int *y, int *pendown, void *data)
{
	input_module_t    *module = (input_module_t *) data;
	private_data_t    *dp = module->data;
	int sts;

	/*
	 * Read X and Y Measurements
	 * Sequencial read to prevent X or Y coordinate is updated while it's being reading
     */
	sndrcv.sr.recv_len = 4;
	sndrcv.data[0] = TSC2004_CMD0(X_REG, PND0_FALSE, READ_REG);
	sts = devctl(dp->fd, DCMD_I2C_SENDRECV, &sndrcv, sizeof(sndrcv), NULL);
	if (sts != EOK) {
		slogf(_SLOGC_INPUT, _SLOG_ERROR, "%s: devctl failed (sts=%d) for X", __FUNCTION__, sts);
		return -1;
	}

	/* All other I2C comunications only need to read 2 bytes */
	sndrcv.sr.recv_len = 2;

	*x = (sndrcv.data[0] << 8);
	*x |= sndrcv.data[1];

	*y = (sndrcv.data[2] << 8);
	*y |= sndrcv.data[3];

	*x &= MEAS_MASK;
	*y &= MEAS_MASK;

	/* PSM bit in the CFR0 register determines the Pendown state */
	sndrcv.data[0] = TSC2004_CMD0(CFR0_REG, PND0_FALSE, READ_REG);
	sts = devctl(dp->fd, DCMD_I2C_SENDRECV, &sndrcv, sizeof(sndrcv), NULL);
	if (sts != EOK) {
		slogf(_SLOGC_INPUT, _SLOG_ERROR, "%s - %d: devctl failed (sts=%d)", __FUNCTION__, __LINE__, sts);
		return -1;
	}

	/*
     * It happens that the PSM bit won't accurately reflect the PENDOWN status,
     * in case of if X or Y is not zero, it's highly possibly
     * the Pen is still down no matter what value the PSM is
     */
	if (!(*x) && !(*y) && !(sndrcv.data[0] & PEN_STS_CTRL_MODE)) {
		*pendown = 0;
	} else {
		*pendown = 1;
	}

	/* Prepare for next touch reading */
	tsc2004_prepare_for_reading(module);
	return (0);
}

/* Do some basic processing on touch data to remove jitter.
 * Basically, this function "holds" successive co-ordinate values at the
 * first known reference of touch which we assume is accurate enough.
 */
static void process_data (private_data_t  *dp)
{
    static uint16_t sample_x = 0;
    static uint16_t sample_y = 0;
    static int xcount = 0, ycount = 0;
    uint16_t touch_x, touch_y;

    touch_x = dp->tp.x;
    touch_y = dp->tp.y;

    /* first touch or genuine delta */
    if (((sample_x == 0) && (sample_y == 0)) ||
            ((abs(sample_x - touch_x) > dp->jitter_delta) ||
            (abs(sample_y - touch_y) > dp->jitter_delta))) {
        sample_x = touch_x;
        sample_y = touch_y;
        xcount = ycount = 0;
        if (dp->verbose >= 4)
            fprintf(stderr, "TOUCH OR GENUINE DELTA X:%d Y:%d\n",
                                touch_x, touch_y);
        return;
    }

    /* correct X jitter */
    if ((abs(sample_x - touch_x) < dp->jitter_delta) || xcount) {
        dp->tp.x = sample_x;
        xcount++;
        if (dp->verbose >= 4)
            fprintf(stderr, "JITTER CORRECTED X-DELTA:%d\n",
                                abs(sample_x - touch_x));
    }

    /* correct Y jitter */
    if ((abs(sample_y - touch_y) < dp->jitter_delta) || ycount) {
        dp->tp.y = sample_y;
        ycount++;
        if (dp->verbose >= 4)
            fprintf(stderr, "JITTER CORRECTED Y-DELTA:%d\n",
                                abs(sample_y - touch_y));
    }
}

static void process_event( void *data)
{
	input_module_t    *module = (input_module_t *) data;
	private_data_t    *dp;
	input_module_t    *up;
	unsigned int    x = 0, y = 0;
	int restart_timer = 0;
	int pendown = -1;

	if (!module || !module->data || !module->up)
		return;

	dp = module->data;
	up = module->up;

	pthread_mutex_lock (&dp->mutex);

	/* Stop timer */
	dp->itime.it_value.tv_sec = 0;
	dp->itime.it_value.tv_nsec = 0;
	dp->itime.it_interval.tv_sec = 0;
	dp->itime.it_interval.tv_nsec = 0;
	timer_settime(dp->timerid, 0, &dp->itime, NULL);

	/* read data */
	tsc2004_read_data(&x, &y, &pendown, data);

	if (dp->verbose >= 4)
		fprintf(stderr, "PRE FILTER  X:%d Y:%d, Pen %s\n", x, y, (pendown) ? "DOWN" : "UP");

	if (pendown == 1) {
		restart_timer = 1;

		/*
         * If it has valid coordinates
		 * and the key was previous released
		 * accept this press event
		 */
		if (x && y) {
			dp->tp.x = x;
			dp->tp.y = y;

			/* preprocess data for removing jitter */
			process_data(dp);

			/* Save values for the release event */
			dp->touch_x = dp->tp.x;
			dp->touch_y = dp->tp.y;

			if (dp->verbose >= 3)
				fprintf(stderr, "X:%d Y:%d Touched, reporting DOWN events\n", dp->tp.x, dp->tp.y);

			dp->last_buttons = dp->tp.buttons = _POINTER_BUTTON_LEFT;
			clk_get(&dp->tp.timestamp);
			(up->input)(up, 1, &dp->tp);
		} else if (dp->verbose > 4) {
			fprintf(stderr, "REJECTED    X:%d Y:%d\n", x, y);
		}
	} else if (dp->last_buttons && dp->touch_x && dp->touch_y) {
		/* Assume the key is released */
		dp->tp.x = dp->touch_x;
		dp->tp.y = dp->touch_y;
		dp->touch_x = dp->touch_y = 0;

		/* Save the button state so we can identify the first press and reject it */
		dp->last_buttons = dp->tp.buttons = 0;

		clk_get(&dp->tp.timestamp);
		(up->input)(up, 1, &dp->tp);

		if (dp->verbose >= 3)
			fprintf(stderr, "X:%d Y:%d Released, REPORTING UP EVENTS\n", dp->tp.x, dp->tp.y);
	} else {
		if (dp->verbose >= 4)
			fprintf(stderr, "REJECTED    X:%d Y:%d\n", x, y);
	}

	pthread_mutex_unlock (&dp->mutex);

	/* (Re)Start timer, need to guaranteed that it won't miss the release event  */
	if (restart_timer) {
		dp->itime.it_value.tv_sec = 0;
		dp->itime.it_value.tv_nsec = dp->release_delay;
		dp->itime.it_interval.tv_sec = 0;
		dp->itime.it_interval.tv_nsec = 0;
		timer_settime(dp->timerid, 0, &dp->itime, NULL);
	}

	return;
}

/*
 * This is the code for the interrupt handler thread. It simply
 * waits on a pulse that is generated by the interrupt and then
 * requests the X and Y coordinates from the TSC2004 over I2C.
 */
static void *intr_thread( void *data )
{
    input_module_t    *module = (input_module_t *) data;
    private_data_t    *dp;
	struct _pulse   pulse;
	iov_t           iov;
	int             rcvid;

	if (!module || !module->data)
		return;

	dp = module->data;
	SETIOV (&iov, &pulse, sizeof(pulse));

	while (1) {
		if ((rcvid = MsgReceivev (dp->chid, &iov, 1, NULL)) == -1) {
			if (errno == ESRCH)
				pthread_exit (NULL);

			continue;
		}

		switch (pulse.code) {
			case PULSE_CODE:
				process_event(data);

				/* We don't want to waste out time to handle too much not necessary interrupts */
				delay (dp->intr_delay);
				InterruptUnmask (dp->irq, dp->iid);
				break;

			default:
				if (rcvid) {
					MsgReplyv (rcvid, ENOTSUP, &iov, 1);
				}
				break;
			}
		}

	return( NULL );
}

/*
 * tsc2004_pulse()
 *
 * This is the callback for event notifications from the input runtime
 * system. In our case, this is the timer pulse handler that gets called
 * when the timer has expired.
 */
static int tsc2004_pulse (message_context_t *ctp, int code, unsigned flags, void *data)
{
	input_module_t    *module = (input_module_t *) data;
	private_data_t    *dp;

	if (!module || !module->data)
		return (-1);

	dp = module->data;

	/*
     * This handler could be preempted by the interrupt handler,
	 * so that this doesn't need to proceed
     */
	if (dp->itime.it_value.tv_nsec)
		process_event(data);

	return (0);
}

/*
 * tsc2004_shutdown()
 *
 * This callback performs the cleanup of the driver when the input
 * runtime system is shutting down.
 */
static int tsc2004_shutdown(input_module_t *module, int shutdown_delay)
{
	private_data_t  *dp;

	if (module) {
		dp = module->data;
	} else {
		slogf (_SLOGC_INPUT, _SLOG_ERROR, "%s: module is NULL", __FUNCTION__);
		exit (-1);
	}

	delay (shutdown_delay);

	close(dp->fd);
	free(dp->i2c);

	free (module->data);

	return (0);
}

