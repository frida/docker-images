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

#include <sys/cdefs.h>

#ifndef __INPUT_PARSEOPTS_H_INCLUDED
#define __INPUT_PARSEOPTS_H_INCLUDED

__BEGIN_DECLS

typedef int (*set_option_t)(const char* option, const char* value, void* arg);

void input_parseopts(const char* options, set_option_t set_option, void* arg);

int input_parse_unsigned(const char* option, const char* value, unsigned* out);
int input_parse_signed(const char* option, const char* value, int* out);
int input_parse_bool(const char* option, const char* value, unsigned* out);
int input_parse_string(const char* option, const char* value, char** out);
int input_parse_double(const char* option, const char* value, double* out);

__END_DECLS

#endif /* __INPUT_PARSEOPTS_H_INCLUDED */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/inputevents/public/input/parseopts.h $ $Rev: 724998 $")
#endif
