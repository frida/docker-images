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



#ifndef __INPUT_CALIB_H_
#define __INPUT_CALIB_H_

#define DEFAULT_SCALING_CONF "/etc/system/config/scaling.conf"

/**
 * These allow the client to select how the coordinate mapping is done:
 *
 * - MTOUCH_COORD_MAP_DIRECT: Touch coordinates are reported exactly as they
 *                            come in from the driver
 *
 * - MTOUCH_COORD_MAP_SCALE:  Touch coordinates are scaled to the client screen
 *                            resolution
 *
 * - MTOUCH_COORD_MAP_RECT:   Touch coordinates are ignored if they fall outside
 *                            the specified rectangle. They are also rebased at
 *                            the upper left corner of the rectangle
 *
 * - MTOUCH_COORD_MAP_DIM:    Touch coordinates are scaled and shifted based on
 *                            the physical dimensions of the touch panel and
 *                            display
 *
 * - MTOUCH_COORD_MAP_CALIB:  Touch coordinates are scaled and shifted based on
 *                            calibration data
 */
typedef enum {
	MTOUCH_COORD_MAP_DIRECT = 1,
	MTOUCH_COORD_MAP_SCALE,
	MTOUCH_COORD_MAP_RECT,
	MTOUCH_COORD_MAP_DIM,
	MTOUCH_COORD_MAP_CALIB,
	MTOUCH_COORD_MAP_SPEC
} mtouch_coord_map_e;

typedef struct {
	/* The width and height of the display on which the calibration took place */
	_Uint32t screen_width_pix;
	_Uint32t screen_height_pix;

	/*
	 * These are the touch and display coordinates of the 4 calibration points:
	 *
	 * ----------------
	 * | 0          1 |
	 * |              |
	 * | 3          2 |
	 * ----------------
	 */
	_Int32t screen_x[4];
	_Int32t screen_y[4];
	_Int32t mtouch_x[4];
	_Int32t mtouch_y[4];
} mtouch_calib_t;

typedef struct mtouch_scaling_params {
	mtouch_coord_map_e mode;
	union {
		/**
		 * Direct coordinate mapping returns the touch coordinates without
		 * converting to screen coordinates.
		 */
		struct {
			void* null[0];
		} direct;

		/**
		 * Scale coordinate mapping does a simple scaling not taking any touch
		 * panel overlap (borders) into account
		 */
		struct {
			_Uint32t screen_width_pix;
			_Uint32t screen_height_pix;
		} scale;

		/**
		 * Rectangle coordinate mapping limits the touch area of touch panel to
		 * the specified rectangle. Touches on the panel outside the rectangle
		 * will be ignored. This mode was initially added for VMware support on
		 * x86 with an Acer T230H.
		 */
		struct {
			_Uint32t screen_width_pix;
			_Uint32t screen_height_pix;
			_Uint32t offset_x_pix;
			_Uint32t offset_y_pix;
		} rect;

		/**
		 * Dimension coordinate mapping will use the physical dimensions of the
		 * touch panel and display to shift and scale the touch coordinates to
		 * screen coordinates. This mode supports borders all around and reports
		 * touches above and to the left of the display as negative values.
		 */
		struct {
			_Uint32t screen_width_mm;
			_Uint32t screen_height_mm;
			_Uint32t screen_width_pix;
			_Uint32t screen_height_pix;

			/* Positive offset values means the touch area is larger than the display */
			_Int32t offset_left_mm; /* The offset of the left edge of the display relative to the left edge of the touch area, also known as the left border */
			_Int32t offset_right_mm; /* The offset of the right edge of the display relative to the right edge of the touch area, also known as the right border */
			_Int32t offset_top_mm; /* The offset of the top edge of the display relative to the top edge of the touch area, also known as the top border */
			_Int32t offset_bottom_mm; /* The offset of the bottom edge of the display relative to the bottom edge of the touch area, also known as the bottom border */
		} dim;

		/**
		 * Specification coordinate mapping will use the specifications of the
		 * touch panel & display to allow scaling and shifting of the touch
		 * coordinates to screen coordinates.
		 */
		struct {
			_Uint32t screen_width_pix;
			_Uint32t screen_height_pix;

			//todo: comments
			_Uint32t offset_left_tp; /* The offset (in touch panel coordinates) of the left edge of the display relative to the left edge of the touch area, also known as the left border */
			_Uint32t offset_right_tp; /* The offset (in touch panel coordinates) of the right edge of the display relative to the right edge of the touch area, also known as the right border */
			_Uint32t offset_top_tp; /* The offset (in touch panel coordinates) of the top edge of the display relative to the top edge of the touch area, also known as the top border */
			_Uint32t offset_bottom_tp; /* The offset (in touch panel coordinates) of the bottom edge of the display relative to the bottom edge of the touch area, also known as the bottom border */
		} spec;

		/**
		 * Calib coordinate mapping takes calibration parameters to determine
		 * scale and shift of the touch coordinates. The calibration data format
		 * can be found in libmtouch-calib; see calib.h
		 */
		mtouch_calib_t calib;
	} u;
} mtouch_scaling_params_t;

int read_scaling_conf(char* filename, mtouch_scaling_params_t* scaling_params, int screen_width, int screen_height);

#endif /* __INPUT_CALIB_H_ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/mtouch-calib/public/input/calib.h $ $Rev: 680336 $")
#endif
