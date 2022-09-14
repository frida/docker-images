/*
 * $QNXLicenseC:
 * Copyright 2009,2012 QNX Software Systems.
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
 * $QNXLicenseC:
 * Copyright 2007, QNX Software Systems. All Rights Reserved.
 * 
 * You must obtain a written license from and pay applicable license fees to QNX 
 * Software Systems before you may reproduce, modify or distribute this software, 
 * or any work that includes all or part of this software.   Free development 
 * licenses are available for evaluation and non-commercial purposes.  For more 
 * information visit http://licensing.qnx.com or email licensing@qnx.com.
 *  
 * This file may contain contributions from others.  Please review this entire 
 * file for other proprietary rights or license notices, as well as the QNX 
 * Development Suite License Guide at http://licensing.qnx.com/license-guide/ 
 * for other information.
 * $
 */




/*
** File: f3s_socket.h
**
** Description:
**
** This file contains the socket definitions for the f3s resource manager for
** Neutrino
**
** Ident: $Id: f3s_socket.h 710521 2013-06-17 17:57:28Z targentina@qnx.com $
*/

/*
** Include Loop Prevention
*/

#ifndef __F3S_SOCKET_H_V3_INCLUDED
#define __F3S_SOCKET_H_V3_INCLUDED

#ifdef __F3S_SOCKET_H_INCLUDED
#error Cannot include old and new flash file system headers
#endif

#ifndef __PLATFORM_H_INCLUDED
#include <sys/platform.h>
#endif

/*
** Constant Definitions
*/

/* Capabilities Definitions */

#define F3S_POWER_VCC     0x00000001  /* read array power */
#define F3S_POWER_VPP     0x00000002  /* program power */
#define F3S_POWER_ALL     0x00000003  /* both powers */

#define F3S_OPER_SOCKET   0x00000004  /* socket operation */
#define F3S_OPER_WINDOW   0x00000008  /* window operation */

#define F3S_INTER_STATUS  0x00000010  /* interrupt on status change */
#define F3S_INTER_RELAY   0x00000020  /* interrupt relay for flash services */

#define F3S_POWER_VHH     0x00000040  /* protection power */
#define F3S_POWER_OFF     0x00000080  /* turn power off (cleared flags) */

/*
** Structure Definitions
*/

/* Note: all chips in non-contiguous windows should be same; 
 * otherwise, a lot of things in library and mtd layer should be changed
 * 	library  : chip_pow2, unit_pow2, unit_size in f3s_array_t should
 *		   be changed to data array to support different chips.
 * 	mtd-layer: flashcfg is a global variable, it should be put into 
 *		   f3s_access_t.
 */
typedef struct f3s_non_contiguous_window_t
{
  _Paddr64t address;       /* physical address */
  _Uint8t* memory;         /* access pointer for window memory */
} f3s_noncontiguous_t;

/* Socket Services Info */

typedef struct f3s_socket_s
{

  /*
  	these fields are initialized by the flash file system and later
    validated and set by the socket services
  */

  _Uint16t struct_size;    /* size of this structure */
  _Uint16t status;         /* status of this structure */
  _Uint8t *option;         /* option string from flashio */
  _Uint16t socket_index;   /* index of socket */
  _Uint16t window_index;   /* index of window */

  /*
	these fields are initialized by the socket services and later
	referenced by the flash file system
  */

  _Uint8t *name;           /* name of driver */
  _Paddr64t address;       /* physical address 0 for allocated */
  _Uint32t window_size;    /* size of window power of two mandatory */
  _Uint32t array_offset;   /* offset of array 0 for based */
  _Uint32t array_size;     /* size of array 0 for window_size */
  _Uint32t unit_size;      /* size of unit 0 for probed */
  _Uint32t flags;          /* flags for capabilities */
  _Uint16t bus_width;      /* width of bus */
  _Uint16t window_num;     /* number of windows 0 for not windowed */

  /*
  	these fields are initialized by the socket services and later
    referenced by the socket services
  */

  _Uint8t* memory;         /* access pointer for window memory */
  void *socket_handle;     /* socket handle pointer for external library */
  void *window_handle;     /* window handle pointer for external library */

  f3s_noncontiguous_t *ncont_windows;
  _Uint8t ncont_window_num;

  /*
	this field is modified by the socket services as different window
    pages are selected
  */

  _Uint32t window_offset;  /* offset of window */
}
f3s_socket_t;

/* Socket Services Callouts */

typedef struct f3s_service_s
{
  _Uint32t struct_size;                     /* size of this structure */
  _Int32t (*open)(f3s_socket_t *socket,
                  _Uint32t flags);
  _Uint8t *(*page)(f3s_socket_t *socket,
                   _Uint32t flags,
                   _Uint32t text_offset,
                   _Int32t *size);
  _Int32t (*status)(f3s_socket_t *socket,
                    _Uint32t flags);
  void (*close)(f3s_socket_t *socket,
                _Uint32t flags);
}
f3s_service_t;

/* Access Super Structure */

typedef struct f3s_access_s
{
  f3s_socket_t socket;     /* socket information */
  f3s_service_t *service;  /* socket service functions */
}
f3s_access_t;

#endif /* __F3S_SOCKET_H_V3_INCLUDED */

/*
** End
*/



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/fs-flash3/public/fs/f3s_socket.h $ $Rev: 710521 $")
#endif
