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

#include <sys/slogcodes.h>
#include <sys/cdefs.h>


#ifndef MTOUCH_H_
#define MTOUCH_H_

__BEGIN_DECLS

void mtouch_log(int severity, const char* devname, const char* format, ...);

#ifdef NDEBUG
#define mtouch_debug(devname, format, args...)
#else
#define mtouch_debug(devname, format, args...) \
	mtouch_log(_SLOG_DEBUG1, devname, format, ##args)
#endif

#define mtouch_info(devname, format, args...) \
	mtouch_log(_SLOG_INFO, devname, format, ##args)
#define mtouch_warn(devname, format, args...) \
	mtouch_log(_SLOG_WARNING, devname, format, ##args)
#define mtouch_error(devname, format, args...) \
	mtouch_log(_SLOG_ERROR, devname, format, ##args)
#define mtouch_critical(devname, format, args...) \
	mtouch_log(_SLOG_CRITICAL, devname, format, ##args)

__END_DECLS

#endif /* MTOUCH_H_ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/inputevents/public/input/mtouch_log.h $ $Rev: 724998 $")
#endif
