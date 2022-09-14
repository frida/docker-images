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

#include <sys/trace.h>
#include "input/event_types.h"

#ifndef INPUTTRACE_H_
#define INPUTTRACE_H_

/* The user trace IDs passed to TraceEvent() */
#define INPUTTRACE_SYSTEM_TRACE_ID 1
#define INPUTTRACE_WINMGR_TRACE_ID 2
#define INPUTTRACE_FLASH_TRACE_ID 3
#define INPUTTRACE_SWMTOUCH_TRACE_ID 4
#define INPUTTRACE_MUNIN_TRACE_ID 5
#define INPUTTRACE_AIR_TRACE_ID 6
#define INPUTTRACE_SCREEN_TRACE_ID 7

#ifdef __QNXNTO__
__BEGIN_DECLS
#else
#ifdef __cplusplus
extern "C" {
#endif
#endif

#define INPUTTRACE_SEQ_TYPE_MASK 0xfu
#define INPUTTRACE_SEQ_TYPE_SHIFT 28
#define INPUTTRACE_SEQ_TYPE(x) (((x) & INPUTTRACE_SEQ_TYPE_MASK) << INPUTTRACE_SEQ_TYPE_SHIFT)
#define INPUTTRACE_SEQ_GET_TYPE(x) (((x) >> INPUTTRACE_SEQ_TYPE_SHIFT) & INPUTTRACE_SEQ_TYPE_MASK)

typedef enum {
	INPUTTRACE_SEQ_TYPE_MTOUCH = 0,
	INPUTTRACE_SEQ_TYPE_SCREEN
} inputtrace_seq_type_t;

typedef enum {
	INPUTTRACE_SYSTEM_USB_SERVER = 0,
	INPUTTRACE_SYSTEM_USB_CLIENT,
	INPUTTRACE_SYSTEM_HID_SERVER,
	INPUTTRACE_SYSTEM_HID_CLIENT,
	INPUTTRACE_SYSTEM_PROCESS_PACKET
} inputtrace_system_e;

typedef enum {
	INPUTTRACE_WINMGR_DRIVER_QUEUE = 0,
	INPUTTRACE_WINMGR_CLIENT_DEQUEUE,
	INPUTTRACE_WINMGR_WIN_QUEUE,
	INPUTTRACE_WINMGR_WIN_DEQUEUE,
} inputtrace_winmgr_e;

typedef enum {
	INPUTTRACE_SCREEN_POST_START,
	INPUTTRACE_SCREEN_POST_DIRTY,
	INPUTTRACE_SCREEN_POST_DONE,
	INPUTTRACE_SCREEN_RENDER_START,
	INPUTTRACE_SCREEN_RENDER_DONE,
	INPUTTRACE_SCREEN_COMMIT_START,
	INPUTTRACE_SCREEN_COMMIT_DONE,
} inputtrace_screen_e;

typedef enum {
	INPUTTRACE_STAGE_FLASH_QUEUE = 0,
	INPUTTRACE_STAGE_FLASH_OFFER
} inputtrace_flash_e;

typedef enum {
	INPUTTRACE_FLASH_TYPE_TOUCH = 0,
	INPUTTRACE_FLASH_TYPE_GESTURE,
} inputtrace_flash_type_e;

typedef enum {
	INPUTTRACE_STAGE_SWMTOUCH_DIRTY_BLIT = 0,
	INPUTTRACE_STAGE_SWMTOUCH_DRAW_BLIT,
	INPUTTRACE_STAGE_SWMTOUCH_BLIT_DONE
} inputtrace_swmtouch_e;

typedef enum {
	INPUTTRACE_STAGE_TPDRIVER_INTERRUPT = 0,
	INPUTTRACE_STAGE_TPDRIVER_I2C,
	INPUTTRACE_STAGE_TPDRIVER_RESET
} inputtrace_tpdriver_e;

typedef enum {
	INPUTTRACE_STAGE_AIR_IOW = 0,
	INPUTTRACE_STAGE_AIR_TOUCH,
	INPUTTRACE_STAGE_AIR_GESTURE,
	INPUTTRACE_STAGE_STARTDOPLAY,
	INPUTTRACE_STAGE_AIR_DOPLAY,
	INPUTTRACE_STAGE_AIR_POST
} inputtrace_air_e;

/* Size must be a multiple of sizeof(unsigned) */
typedef struct {
	unsigned stage;
	unsigned event_type;
	unsigned seq_id;
	unsigned dirty[4];
} winmgr_ev_data_t;

/* Size must be a multiple of sizeof(unsigned) */
typedef struct {
	unsigned stage;
	unsigned seq_id;
} sequence_ev_data_t;

/* Size must be a multiple of sizeof(unsigned) */
typedef struct {
	unsigned stage;
	unsigned flash_type;
	unsigned event_type;
	unsigned seq_id;
} flash_ev_data_t;

/* Size must be a multiple of sizeof(unsigned) */
typedef struct {
	unsigned stage;
	unsigned seq_id;
} swmtouch_ev_data_t;

/* Size must be a multiple of sizeof(unsigned) */
typedef struct {
	unsigned stage;
	unsigned seq_id;
} munin_ev_data_t;

/* Size must be a multiple of sizeof(unsigned) */
typedef struct {
	unsigned stage;
	unsigned seq_id;
} air_ev_data_t;

static inline void
inputtrace_system(inputtrace_system_e stage)
{
	TraceEvent(_NTO_TRACE_INSERTSUSEREVENT, INPUTTRACE_SYSTEM_TRACE_ID, stage);
}

static inline void
inputtrace_winmgr(inputtrace_winmgr_e stage, input_event_e event_type, unsigned seq_id)
{
	winmgr_ev_data_t ev_data = {
		stage,
		event_type,
		seq_id,
		{ 0, 0, 0, 0 }
	};

	TraceEvent(_NTO_TRACE_INSERTCUSEREVENT, INPUTTRACE_WINMGR_TRACE_ID, (unsigned*)&ev_data, sizeof(ev_data)/4);
}

static inline void
inputtrace_screen(inputtrace_screen_e stage, unsigned seq_id, pid_t pid, unsigned flags, unsigned window_id, int index, unsigned *dirty_rect)
{
	struct {
		uint32_t stage;
		uint32_t sequence;
		uint32_t pid;
		uint32_t flags;
		uint32_t window_id;
		uint32_t index;
		uint32_t x;
		uint32_t y;
		uint32_t w;
		uint32_t h;
	} data = { stage, INPUTTRACE_SEQ_TYPE(INPUTTRACE_SEQ_TYPE_SCREEN) | seq_id, pid, flags, window_id, index, 0, 0, 0, 0 };
	int len = sizeof(data)/sizeof(uint32_t);

	if ( dirty_rect ) {
		data.x = dirty_rect[0];
		data.y = dirty_rect[1];
		data.w = dirty_rect[2];
		data.h = dirty_rect[3];
	} else {
		len -= 4;
	}

	TraceEvent(_NTO_TRACE_INSERTCUSEREVENT, INPUTTRACE_SCREEN_TRACE_ID, &data, len);
}

static inline void
inputtrace_flash(inputtrace_flash_e stage, inputtrace_flash_type_e flash_type, input_event_e event_type, unsigned seq_id)
{
	flash_ev_data_t ev_data = {
		stage,
		flash_type,
		event_type,
		seq_id
	};

	TraceEvent(_NTO_TRACE_INSERTCUSEREVENT, INPUTTRACE_FLASH_TRACE_ID, (unsigned*)&ev_data, sizeof(ev_data)/4);
}

static inline void
inputtrace_swmtouch(inputtrace_swmtouch_e stage, unsigned seq_id)
{
	swmtouch_ev_data_t ev_data = {
		stage,
		seq_id
	};

	TraceEvent(_NTO_TRACE_INSERTCUSEREVENT, INPUTTRACE_SWMTOUCH_TRACE_ID, (unsigned*)&ev_data, sizeof(ev_data)/4);
}

static inline void
inputtrace_tpdriver(inputtrace_tpdriver_e stage, unsigned seq_id)
{
	munin_ev_data_t ev_data = {
		stage,
		seq_id
	};

	TraceEvent(_NTO_TRACE_INSERTCUSEREVENT, INPUTTRACE_MUNIN_TRACE_ID, (unsigned*)&ev_data, sizeof(ev_data)/4);
}

static inline void
inputtrace_air(inputtrace_air_e stage, unsigned seq_id)
{
	munin_ev_data_t ev_data = {
		stage,
		seq_id
	};

	TraceEvent(_NTO_TRACE_INSERTCUSEREVENT, INPUTTRACE_AIR_TRACE_ID, (unsigned*)&ev_data, sizeof(ev_data)/4);
}

#ifdef __QNXNTO__
__END_DECLS
#else
#ifdef __cplusplus
};
#endif
#endif

#endif /* INPUTTRACE_H_ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/inputevents/public/input/inputtrace.h $ $Rev: 724998 $")
#endif
