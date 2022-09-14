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


#include "mxecspi.h"

/*
 * We use the same buffer for transmit and receive
 * For exchange, that's exactly what we wanted
 * For Read, it doesn't matter what we write to SPI, so we are OK.
 * For transmit, the receive data is put at the buffer we just transmitted, we are still OK.
 */

static const struct sigevent *spi_intr(void *area, int id)
{
	mx51_cspi_t	*dev = area;
	uintptr_t	base = dev->vbase;
	uint32_t	data;
	int			tmp, count;

	/* check which interrupt */
	tmp = in32(base + MX51_CSPI_STATREG);
	if (tmp & CSPI_STATREG_TC) {
		/* clear TC interrupt source */
		out32(base + MX51_CSPI_STATREG, CSPI_STATREG_TC);

		/* how many words received in RXFIFO */
		count = ((in32(base + MX51_CSPI_TESTREG) & CSPI_TESTREG_RXCNT_MASK) >> CSPI_TESTREG_RXCNT);
	}
	else if (tmp & CSPI_STATREG_RDR) {
		/* RX FIFO interrupt, at least MX51_CSPI_FIFO_RXMARK bytes in RXFIFO */
		count = MX51_CSPI_FIFO_RXMARK;
	}
	else {							// wrong interrupt
		/* Disable interrupt and return spievent */
		out32(base + MX51_CSPI_INTREG, 0);
		return (&dev->spievent);
	}

	if (dev->burst) {
		/* read the data out */
		/* check if the first word in RXFIFO need data adjustment */	
		if (0 != dev->lsb_adjust) {
			data = in32(base + MX51_CSPI_RXDATA);
			count--;				// read one word data from RXFIFO

			switch (dev->lsb_adjust) {
				case 1:				// only read 8 bits in RXFIFO
					if (1 == dev->dlen) {
						dev->pbuf[dev->rlen++] = data;
						dev->brlen += 1;
					}
					break;
				case 2:				// only read 16 bits in RXFIFO
					if (1 == dev->dlen) {
						dev->pbuf[dev->rlen++] = data >> 8;
						dev->pbuf[dev->rlen++] = data;
						dev->brlen += 2;
					}
					else if (2 == dev->dlen) {
						*(uint16_t *)(&dev->pbuf[dev->rlen]) = data;
						dev->rlen += 2;
						dev->brlen += 2;
					}
					break;
				case 3:				// only read 24 bits in RXFIFO
					if (1 == dev->dlen) {
						dev->pbuf[dev->rlen++] = data >> 16;
						dev->pbuf[dev->rlen++] = data >> 8;
						dev->pbuf[dev->rlen++] = data;
						dev->brlen += 3;
					}
					break;
				default:
					break;
			}
			dev->lsb_adjust = 0;	// clear data adjustment flag	
		}

		/* read the rest RX buffer */
		while (count) {
			data = in32(base + MX51_CSPI_RXDATA);
			count--;

			switch (dev->dlen) {
				case 1:
					dev->pbuf[dev->rlen++] = data >> 24;
					dev->pbuf[dev->rlen++] = data >> 16;
					dev->pbuf[dev->rlen++] = data >> 8;
					dev->pbuf[dev->rlen++] = data;
					break;
				case 2:
					*(uint16_t *)(&dev->pbuf[dev->rlen]) = data >> 16;
					dev->rlen += 2;
					*(uint16_t *)(&dev->pbuf[dev->rlen]) = data;
					dev->rlen += 2;
					break;
				case 4:
					*(uint32_t *)(&dev->pbuf[dev->rlen]) = data;
					dev->rlen += 4;
					break;
				default:
					break;
			}
			dev->brlen += 4;	
		}

		/* fill TXFIFO:
		 * 1: more data need to be transmited for the current burst 
		 * 2: less that MX51_CSPI_FIFO_RXMARK word has been write to TXFIFO
		 * Note: The data left here must be align on 32bits word.
		 * Because the only the firsr word of the every burst may need the data adjustment
		 * that has been handled in mx51_xfer()
		 */
		count = 0; 
		while ((dev->btlen < dev->bxlen) && count < MX51_CSPI_FIFO_RXMARK) {
			switch (dev->dlen) {
				case 1:
					data = (dev->pbuf[dev->tlen] << 24) | (dev->pbuf[dev->tlen + 1] << 16)
							| (dev->pbuf[dev->tlen + 2] << 8) | (dev->pbuf[dev->tlen + 3]);
					break;
				case 2:
					data = (*(uint16_t *)(&dev->pbuf[dev->tlen]) << 16)
							| (*(uint16_t *)(&dev->pbuf[dev->tlen + 2]));
					break;
				case 4:
					data = *(uint32_t *)(&dev->pbuf[dev->tlen]);
					break;
				default:
					data = 0;
					break;
			}
			out32(base + MX51_CSPI_TXDATA, data);
			dev->tlen += 4;
			dev->btlen += 4;
			count++;
		}

		if (dev->brlen >= dev->bxlen || dev->rlen >= dev->xlen) {
			/* Disable interrupt and return spievent */
			out32(base + MX51_CSPI_INTREG, 0);
			return (&dev->spievent);
		}
		else {
			/* Start next burst */
			out32(base + MX51_CSPI_CONTROLREG,
				in32(base + MX51_CSPI_CONTROLREG) | CSPI_CONTROLREG_XCH);
		}
	}
	else {
		/* empty RX buffer and fill tx buffer(if required) */
		while (count) {
			data = in32(base + MX51_CSPI_RXDATA);
			count--;

			switch (dev->dlen) {
				case 1:
					dev->pbuf[dev->rlen++] = data;

					/* More to Xmit */
					if (dev->tlen < dev->xlen) {
						out32(base + MX51_CSPI_TXDATA, dev->pbuf[dev->tlen++]);
					}
					break;
				case 2:
					*(uint16_t *)(&dev->pbuf[dev->rlen]) = data;
					dev->rlen += 2;

					/* More to Xmit */
					if (dev->tlen < dev->xlen) {
						out32(base + MX51_CSPI_TXDATA, *(uint16_t *)(&dev->pbuf[dev->tlen]));
						dev->tlen += 2;
					}
					break;
				case 3:
				case 4:
					*(uint32_t *)(&dev->pbuf[dev->rlen]) = data;
					dev->rlen += 4;

					/* More to Xmit */
					if (dev->tlen < dev->xlen) {
						out32(base + MX51_CSPI_TXDATA, *(uint32_t *)(&dev->pbuf[dev->tlen]));
						dev->tlen += 4;
					}
					break;
				default:
					break;
			}
		}
		
		if (dev->rlen >= dev->xlen) {
			/* Disable interrupt and return spievent */
			out32(base + MX51_CSPI_INTREG, 0);
			return (&dev->spievent);
		}
		else {
			/* Start next burst */
			out32(base + MX51_CSPI_CONTROLREG,
				in32(base + MX51_CSPI_CONTROLREG) | CSPI_CONTROLREG_XCH);
		}
	}

	return 0;
}

int mx51_attach_intr(mx51_cspi_t *mx51)
{
	if ((mx51->chid = ChannelCreate(_NTO_CHF_DISCONNECT | _NTO_CHF_UNBLOCK)) == -1) {
		return -1;
	}

	if ((mx51->coid = ConnectAttach(0, 0, mx51->chid, _NTO_SIDE_CHANNEL, 0)) == -1) { 
		goto fail0;
	}

	mx51->spievent.sigev_notify   = SIGEV_PULSE;
	mx51->spievent.sigev_coid     = mx51->coid;
	mx51->spievent.sigev_code     = MX51_CSPI_EVENT;
	mx51->spievent.sigev_priority = MX51_CSPI_PRIORITY;

	/*
	 * Attach SPI interrupt
	 */
	mx51->iid = InterruptAttach(mx51->irq, spi_intr, mx51, 0, _NTO_INTR_FLAGS_TRK_MSK);

	if (mx51->iid != -1) {
		return 0;
	}

	ConnectDetach(mx51->coid);
fail0:
	ChannelDestroy(mx51->chid);

	return -1;
}

__SRCVERSION("$URL$ $Rev$");
