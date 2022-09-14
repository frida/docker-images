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


int mx51_cfg(void *hdl, spi_cfg_t *cfg)
{
	mx51_cspi_t	*mx51 = hdl;
	uint32_t	ctrl, post_div, div, drate, post_drate;

	if (cfg == NULL) {
		return 0;
	}

	ctrl = cfg->mode & SPI_MODE_CHAR_LEN_MASK;

	if (ctrl > 32 || ctrl < 1) {
		return 0;
	}

	/* Assign the datarate if calculated rate <= desired rate;
	 * OR assign lowest possible rate last time through the loop
	 */
	for (post_div = 0; post_div < 16 ; post_div++) {
		post_drate = mx51->clock >> post_div;
		for (div = 0; div < 16 ; div++) {
			drate = post_drate / (div + 1);	
			if (drate <= cfg->clock_rate) {
				break;
			}
		}
		if (drate <= cfg->clock_rate) {
			break;
		}
	}

	cfg->clock_rate = drate;
	ctrl = (post_div << CSPI_CONREG_POSTDIVIDR_POS) | (div << CSPI_CONREG_PREDIVIDR_POS);

	switch (cfg->mode & SPI_MODE_RDY_MASK) {
		case SPI_MODE_RDY_EDGE:
			ctrl |= CSPI_CONTROLREG_DRCTL_EDGE << CSPI_CONTROLREG_DRCTL_POS;
			break;
		case SPI_MODE_RDY_LEVEL:
			ctrl |= CSPI_CONTROLREG_DRCTL_LEVEL << CSPI_CONTROLREG_DRCTL_POS;
			break;
	}

	return ctrl;
}

__SRCVERSION("$URL$ $Rev$");
