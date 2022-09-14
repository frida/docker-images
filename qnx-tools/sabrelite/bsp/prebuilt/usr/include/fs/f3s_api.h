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
** File: f3s_api.h
**
** Description:
**
** This file contains the application interface definitions for the f3s
** resource manager for Neutrino
** These functions are mainly to start the flashio library giving it
** access to command line parameters and vectors of socket services and
** flash services.
**
** Ident: $Id: f3s_api.h 710521 2013-06-17 17:57:28Z targentina@qnx.com $
*/


/*
** Include Loop Prevention
*/

#ifndef __F3S_API_H_V3_INCLUDED
#define __F3S_API_H_V3_INCLUDED

#ifdef __F3S_API_H_INCLUDED
#error Cannot include old and new flash file system headers
#endif

#ifndef __PLATFORM_H_INCLUDED
#include <sys/platform.h>
#endif

#include <sys/types.h>
#include <inttypes.h>

#include <fs/f3s_spec.h>
#include <fs/f3s_socket.h>
#include <fs/f3s_flash.h>

#include <sys/dcmd_f3s.h>

/*
** Function Prototypes
*/

/*
the f3s_init api function call initializes the flashio library with
the command line arguments and the full set of features
*/

void f3s_init(int argc,
              char **argv,
              f3s_flash_t *flash_vect);

/*
the f3s_start api function call starts the flashio resource manager with
null terminated vectors of socket services and flash services (MTD)
*/

int f3s_start(f3s_service_t *service_vect,
              f3s_flash_t *flash_vect);

/*/
the f3s_socket_option api function parses the socket option string
and sets the address and size members of the socket structure
it returns EOK if socket options were found and parsed properly
this function should be called by the socket services open function
*/

int f3s_socket_option(f3s_socket_t *socket);

/*
the f3s_socket_syspage api function parses the syspage tokens
and sets the address and size members of the socket structure
it returns EOK if the syspage token was found and parsed properly
this function should be called by the socket services open function
*/

int f3s_socket_syspage(f3s_socket_t *socket);

/*
the f3s_socket_event api function call signals a socket status change event
to the flash file system resource manager (like an array removal)
it can be safely called in the context of an interrupt handler
*/

void f3s_socket_event(f3s_access_t *access);

/*
the f3s_flash_event api function call signals a flash status ready event
to the flash file system resource manager (mostly WSM ready after erase)
it can be safely called in the context of an interrupt handler
*/

void f3s_flash_event(f3s_access_t *access,
                     f3s_dbase_t *dbase);

/*
the f3s_flash_probe api function call receives a double pointer on a flash
mtd structure vector and places it on the right flash structure to access
the flash array, it also fills in the database and returns the number of
chips detected
*/

int32_t f3s_flash_probe(f3s_access_t *access,
                        f3s_flash_t **flash_ptr,
                        f3s_dbase_t *dbase);

/*
the f3s_flash_list api function call receives a pointer on a flash
mtd structure vector and lists all flash dbases available, also exits
*/

void f3s_flash_list(f3s_flash_t *flash);

#endif /* __F3S_API_H_V3_INCLUDED */

/*
** End
*/



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/fs-flash3/public/fs/f3s_api.h $ $Rev: 710521 $")
#endif
