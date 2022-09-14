/*
 * $QNXLicenseC:
 * Copyright 2011, QNX Software Systems.
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

#include "variant.h"
#include "mxecspi.h"

#define	MXCSPI_POLL_THRESHOLD	8
#define __txdata		(dev->vbase + MX_ECSPI_TXDATA)
#define __rxdata		(dev->vbase + MX_ECSPI_RXDATA)

static const struct sigevent *spi_intr(void *area, int id)
{
	mx_ecspi_t *dev = area;

	// clear all interrupts
	out32(dev->vbase + MX_ECSPI_INTREG, 0);

	return(&dev->spievent);
}

static int spi_wait(mx_ecspi_t const* dev, int len)
{
	struct _pulse pulse;

	while(1)
	{
		if(len)
		{
			uint64_t	to = dev->dtime;
			to *= len * 1000 * 50; /* 50 times for timeout */
			TimerTimeout(CLOCK_REALTIME, _NTO_TIMEOUT_RECEIVE, NULL, &to, NULL);
		}

		if(MsgReceivePulse(dev->chid, &pulse, sizeof(pulse), NULL) == -1)
		{
			fprintf(stderr, "norspi: XFER error!\n");
			errno = EIO;
			return -1;
		}

		if(pulse.code == MX_ECSPI_EVENT)
			return 0;
	}

	return 0;
}

int spi_setcfg(int fd, uint32_t device, spi_cfg_t *cfg)
{
	mx_ecspi_t *dev = (mx_ecspi_t *) fd;
	uint32_t ctrl, div, drate;

	// make sure device number (slave select) is valid and store in ctrl
	// we don't need to set this value each time that spi_exchange is called.
	if (device > 3) {
		errno = EINVAL;
		return -1;
	}
	ctrl &= ~(ECSPI_CONTROLREG_CSEL_MASK);
	ctrl = ( device << ECSPI_CONTROLREG_CSEL_POS);
	
	// set clock divisors
	drate = SCLK_FREQ; 
	div = dev->clock / drate;
	if (div == 0 || div > 16)
	{
		fprintf(stderr, "norspi: Invalid clock divisor\n");
		return -1;
	}

	// actual divisor is predivisor field - 1
	// e.g. a predivisor equal to 1 corresponds to a division by a factor of 2
	div--;
	
	ctrl &= ~(ECSPI_CONREG_PREDIVIDR_MASK | ECSPI_CONREG_POSTDIVIDR_MASK);
	ctrl |= (div << ECSPI_CONREG_PREDIVIDR_POS);

	dev->ctrl = ctrl;
	dev->dtime = dev->dlen * 8 * 1000 * 1000 / drate;

	if(dev->dtime == 0)
		dev->dtime = 1;

	return EOK;
}

static uint32_t spi_setup_exchange(mx_ecspi_t *dev, int len)
{
	assert(len <= MX_ECSPI_BURST_MAX);
	uint32_t ctrl;

	ctrl = dev->ctrl;
	ctrl |= ECSPI_CONTROLREG_ENABLE; // enable ecspi
	ctrl |= ECSPI_CONTROLREG_MASTERMODE(SPI_DEV);
	ctrl |= ((len<<3) - 1) << ECSPI_CONTROLREG_BCNT_POS;

	out32(dev->vbase + MX_ECSPI_CONTROLREG, ctrl);


	out32(dev->vbase + MX_ECSPI_INTREG, ECSPI_INTREG_TCEN);


	return ctrl;
}

static void spi_start_exchange(mx_ecspi_t *dev, uint32_t ctrl)
{

	out32(dev->vbase + MX_ECSPI_CONTROLREG, ctrl | ECSPI_CONTROLREG_XCH);
}

static void spi_end_exchange(mx_ecspi_t *dev, uint32_t ctrl)
{
	out32(dev->vbase + MX_ECSPI_CONTROLREG, ctrl & ~ECSPI_CONTROLREG_ENABLE);
}

int spi_cmd_read(int fd, uint8_t cmd[4], uint8_t* buf, int len)
{
	int total = sizeof cmd + len;
	assert(total <= MX_ECSPI_BURST_MAX);
	mx_ecspi_t *dev = (mx_ecspi_t *)fd;

	uint32_t	ctrl = spi_setup_exchange(dev, total);
	uint32_t	data;
	int i;

	int remainder = len & 3;
	switch(remainder) {
		case 0:
			out32(__txdata, (cmd[0] << 24) | (cmd[1] << 16) | (cmd[2] << 8) | cmd[3]);
			break;
		case 1:
			out32(__txdata, cmd[0]);
			out32(__txdata, (cmd[1] << 24) | (cmd[2] << 16) | (cmd[3] << 8));	// shift a zero
			break;
		case 2:
			out32(__txdata, (cmd[0] << 8) | cmd[1]);
			out32(__txdata, (cmd[2] << 24) | (cmd[3] << 16));  // shift two zeroes
			break;
		case 3:
		default:
			out32(__txdata, (cmd[0] << 16) | (cmd[1] <<  8) | cmd[2]);
			out32(__txdata, (cmd[3] << 24));  // shift three zeroes
			break;
	}

	i = len >> 2;		// divide by 4
	while(i--) {
		out32(__txdata, 0);		// shift out zeroes; we don't care what
	}
	spi_start_exchange(dev, ctrl);

	// wait until the burst is completed
	if (spi_wait(dev, total)) {
		return -1;
	}

	(void)in32(__rxdata);		// discard first four bytes received (correspond to when command was sent)
	// keep lowest 'remainder' bytes, and discard others that we don't care about
	switch(remainder) {
		case 0:
			break;
		case 1:
			buf[0] = in32(__rxdata);
			break;
		case 2:
			data = in32(__rxdata);
			buf[0] = data >> 8;
			buf[1] = data;
			break;
		default:
		case 3:
			data = in32(__rxdata);
			buf[0] = data >> 16;
			buf[1] = data >>  8;
			buf[2] = data;
			break;
	}
	buf += remainder;

	i = len >> 2;	// divide by 4
	while(i--) {
		data = in32(__rxdata);
		buf[0] = data >> 24;
		buf[1] = data >> 16;
		buf[2] = data >>  8;
		buf[3] = data;
		buf += 4;
	}

	spi_end_exchange(dev, ctrl);
	return EOK;
}

int spi_cmd_write(int fd, uint8_t cmd[4], uint8_t const* buf, int len)
{
	int total = sizeof cmd + len;
	assert(total <= MX_ECSPI_BURST_MAX);
	mx_ecspi_t *dev = (mx_ecspi_t *)fd;

	uint32_t	ctrl = spi_setup_exchange(dev, total);
	int i;

	int remainder = len & 3;
	switch(remainder) {
		case 0:
			out32(__txdata, (cmd[0] << 24) | (cmd[1] << 16) | (cmd[2] << 8) | cmd[3]);
			break;
		case 1:
			out32(__txdata, cmd[0]);
			out32(__txdata, (cmd[1] << 24) | (cmd[2] << 16) | (cmd[3] << 8) | buf[0]);
			break;
		case 2:
			out32(__txdata, (cmd[0] << 8) | cmd[1]);
			out32(__txdata, (cmd[2] << 24) | (cmd[3] << 16) | (buf[0] << 8) | buf[1]);
			break;
		case 3:
		default:
			out32(__txdata, (cmd[0] << 16) | (cmd[1] <<  8) | cmd[2]);
			out32(__txdata, (cmd[3] << 24) | (buf[0] << 16) | (buf[1] << 8) | buf[2]);
			break;
	}
	buf += remainder;

	i = len >> 2;		// divide by 4
	while(i--) {
		out32(__txdata, (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]);
		buf += 4;
	}

	spi_start_exchange(dev, ctrl);

	// wait until the burst is completed
	if (spi_wait(dev, total)) {
		return -1;
	}

	// we don't need to read back anything
	spi_end_exchange(dev, ctrl);
	return EOK;
}


int spi_nor_flash_write(int fd, uint8_t const* buf, int len)
{
	assert(len <= MX_ECSPI_BURST_MAX);
	mx_ecspi_t *dev = (mx_ecspi_t *)fd;

	uint32_t	ctrl = spi_setup_exchange(dev, len);
	int i;

	int remainder = len & 3;
	switch(remainder) {
		case 0:
			out32(__txdata, (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]);
			break;
		case 1:
			out32(__txdata, buf[0]);
			out32(__txdata, (buf[1] << 24) | (buf[2] << 16) | (buf[3] << 8) | buf[4]);
			break;
		case 2:
			out32(__txdata, (buf[0] << 8) | buf[1]);
			out32(__txdata, (buf[2] << 24) | (buf[3] << 16) | (buf[4] << 8) | buf[5]);
			break;
		case 3:
		default:
			out32(__txdata, (buf[0] << 16) | (buf[1] <<  8) | buf[2]);
			out32(__txdata, (buf[3] << 24) | (buf[4] << 16) | (buf[5] << 8) | buf[6]);
			break;
	}
	buf += remainder + 4;

	i = len >> 2;		// divide by 4
	while(i--) {
		out32(__txdata, (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]);
		buf += 4;
	}

	spi_start_exchange(dev, ctrl);

	// wait until the burst is completed
	if (spi_wait(dev, len)) {
		return -1;
	}

	// we don't need to read back anything
	spi_end_exchange(dev, len);
	return EOK;
}


int spi_word_exchange(int fd, uint32_t* buf)
{
	mx_ecspi_t *dev = (mx_ecspi_t *)fd;
	uint32_t ctrl = spi_setup_exchange(dev, sizeof(uint32_t));

	out32(__txdata, *buf);
	spi_start_exchange(dev, ctrl);
	if(spi_wait(dev, sizeof(uint32_t)))
		return -1;

	*buf = in32(__rxdata);
	spi_end_exchange(dev, ctrl);

	return EOK;
}

int spi_write_1byte(int fd, uint8_t cmd)
{
	mx_ecspi_t *dev = (mx_ecspi_t *)fd;
	uint32_t ctrl = spi_setup_exchange(dev, 1);

	out32(__txdata, cmd);
	spi_start_exchange(dev, ctrl);
	if(spi_wait(dev, 1))
		return -1;
	spi_end_exchange(dev, ctrl);
	return EOK;
}



int spi_open(const char *path)
{
	static mx_ecspi_t *dev;

	if(ThreadCtl(_NTO_TCTL_IO, 0) == -1)
	{
		fprintf(stderr, "norspi: ThreadCtl Failed\n");
		return -1;
	}

	if((dev = calloc(1, sizeof(mx_ecspi_t))) == NULL)
	{
		fprintf(stderr, "norspi: Could not allocate mx_ecspi_t memory\n");
		return -1;
	}

	dev->pbase = MX_ECSPI_BASE;
	dev->irq = MX_ECSPI_IRQ;
	dev->clock = CLOCK_RATE;
	
	if((dev->vbase = mmap_device_io(MX_ECSPI_SIZE, dev->pbase)) == (uintptr_t)MAP_FAILED)
		goto fail0;

	// disable ecspi
	out32(dev->vbase + MX_ECSPI_CONTROLREG, 0x0);

	// re-enable ecspi
	out32(dev->vbase + MX_ECSPI_CONTROLREG, ECSPI_CONTROLREG_MASTERMODE(SPI_DEV) | ECSPI_CONTROLREG_ENABLE);

	// set wait states between xfers
	out32(dev->vbase + MX_ECSPI_PERIODREG, 0);

	// disable all interrupts
	out32(dev->vbase + MX_ECSPI_INTREG, 0);

	// attach interrupt
	if((dev->chid = ChannelCreate(_NTO_CHF_DISCONNECT | _NTO_CHF_UNBLOCK)) == -1)
		goto fail1;

	if((dev->coid = ConnectAttach(0, 0, dev->chid, _NTO_SIDE_CHANNEL, 0)) == -1)
		goto fail2;

	dev->spievent.sigev_notify 		= SIGEV_PULSE;
	dev->spievent.sigev_coid 		= dev->coid;
	dev->spievent.sigev_code 		= MX_ECSPI_EVENT;
	dev->spievent.sigev_priority 	= MX_ECSPI_PRIORITY;

	dev->iid = InterruptAttach(dev->irq, spi_intr, dev, 0, _NTO_INTR_FLAGS_TRK_MSK);

	if(dev->iid == -1)
		goto fail3;

	return((int)dev);

fail3:
	ConnectDetach(dev->coid);
fail2:
	ChannelDestroy(dev->chid);
fail1:
	munmap_device_io(dev->vbase, MX_ECSPI_SIZE);
fail0:
	free(dev);
	return -1;
}

int spi_close(int fd)
{
	mx_ecspi_t *dev = (mx_ecspi_t *)fd;

	out32(dev->vbase + MX_ECSPI_CONTROLREG, 0);
	InterruptDetach(dev->iid);
	ConnectDetach(dev->coid);
	ChannelDestroy(dev->chid);
	munmap_device_io(dev->vbase, MX_ECSPI_SIZE);
	free(dev);

	return EOK;
}

__SRCVERSION("$URL$ $Rev$");

