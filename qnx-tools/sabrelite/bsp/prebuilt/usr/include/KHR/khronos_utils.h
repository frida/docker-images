/*
 * $QNXLicenseC:
 * Copyright 2009, QNX Software Systems. All Rights Reserved.
 *
 * You must obtain a written license from and pay applicable
 * license fees to QNX Software Systems before you may reproduce,
 * modify or distribute this software, or any work that includes
 * all or part of this software.   Free development licenses are
 * available for evaluation and non-commercial purposes.  For more
 * information visit http://licensing.qnx.com or email
 * licensing@qnx.com.
 *
 * This file may contain contributions from others.  Please review
 * this entire file for other proprietary rights or license notices,
 * as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

#ifndef _CONF_H_
#define _CONF_H_

#include <KHR/khrplatform.h>

typedef struct {
    int layer;

    enum {
        __KHR_NATIVE_FRAMEBUFFER,
        __KHR_GLES1_FRAMEBUFFER,
        __KHR_GLES2_FRAMEBUFFER,
        __KHR_VG_FRAMEBUFFER,
    } type;

    int format;
    int src_alpha;
    int chroma;
    int srckey;
} __khr_framebuffer_t;

KHRONOS_APICALL FILE * __khrOpenGraphicsConf(void);

KHRONOS_APICALL int __khrGetConfigValue(const char *key, char *val, int size);

KHRONOS_APICALL int __khrGetDisplayConfigValue(int display_id, const char *key, char *val, int size);

KHRONOS_APICALL int __khrGetLayerConfigValue(int display_id, int layer_id, const char *key, char *val, int size);

typedef struct __khr_libraries __khr_libraries_t;
	
KHRONOS_APICALL __khr_libraries_t *__khrLoadLibrariesString(char *dlls, void **handle);
KHRONOS_APICALL void __khrUnloadLibraries( __khr_libraries_t *libs );
KHRONOS_APICALL void *__khrGetLibHandle( __khr_libraries_t *libs );

KHRONOS_APICALL void *__khrLoadLibraryString( char *dlls )
		__attribute__((deprecated));

KHRONOS_APICALL void __khrGetDisplayFramebuffers(int display_id, __khr_framebuffer_t **framebuffers, int *count);

#endif /* _CONF_H_ */
