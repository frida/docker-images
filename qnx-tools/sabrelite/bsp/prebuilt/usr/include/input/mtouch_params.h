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

#include <inttypes.h>
#include <sys/types.h>
#include "input/calib.h"

#ifndef MTOUCH_PARAMS_H_
#define MTOUCH_PARAMS_H_

#ifdef __QNXNTO__
__BEGIN_DECLS
#else
#ifdef __cplusplus
extern "C" {
#endif
#endif

/*
 * This is somewhat arbitrary and is only defined so screen knows how big to
 * make its filters parameter array. There is no libinputevents limit to how
 * many filters can be chained.
 */
#define MTOUCH_MAX_FILTERS 8

#define MTOUCH_PARAMS_VENDOR_SZ      64
#define MTOUCH_PARAMS_PRODUCT_ID_SZ  64
#define MTOUCH_PARAMS_SENSOR_ID_SZ   64

typedef struct mtouch_driver_params {
	_Uint32t capabilities;
	_Uint32t flags;
	_Uint8t max_touchpoints;
	_Uint32t width; /* The width in touch units */
	_Uint32t height; /* The height in touch units */
	 char    vendor[MTOUCH_PARAMS_VENDOR_SZ];
	 char    product_id[MTOUCH_PARAMS_PRODUCT_ID_SZ];
	 char    sensor_id[MTOUCH_PARAMS_SENSOR_ID_SZ];
	_Uint32t sensor_sz_x;
	_Uint32t sensor_sz_y;
	_Uint32t max_refresh;
} mtouch_driver_params_t;

typedef enum {
	MTOUCH_FILTER_NONE = 0,
	MTOUCH_FILTER_BALLISTIC,
	MTOUCH_FILTER_EDGE_SWIPE,
	MTOUCH_FILTER_KALMAN,
	MTOUCH_FILTER_BEZEL_TOUCH,
#ifdef HAVE_MTOUCH_FILTER_DOA
	MTOUCH_FILTER_DOA,
#endif
} mtouch_filter_e;

typedef struct mtouch_filter_config {
	mtouch_filter_e type;
	char * options;
} mtouch_filter_config_t;

typedef struct mtouch_client_params {
	_Uint32t min_event_interval; /* In usecs */
	mtouch_scaling_params_t scaling;
} mtouch_client_params_t;

#ifdef __QNXNTO__
__END_DECLS
#else
#ifdef __cplusplus
};
#endif
#endif

#endif /* MTOUCH_PARAMS_H_ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/inputevents/public/input/mtouch_params.h $ $Rev: 724998 $")
#endif
