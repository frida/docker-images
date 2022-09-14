/*
 * $QNXLicenseC:
 * Copyright 2008, QNX Software Systems. 
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
** File: f3s_usage.c
**
** Description:
**
** This file contains the usage function for the f3s flash file system
**
** Ident: $Id: usage.c 710521 2013-06-17 17:57:28Z targentina@qnx.com $
**
*/

/*
** Includes
*/

#include <sys/f3s_mtd.h>
#include <libc.h>

/*
** Function: f3s_usage
*/

void f3s_usage(int argc,
               char **argv,
               char *message)
{
  int error=EOK;

  /* check if a message is wanted */

  if (message)
  {
    error=EINVAL;

    printf("%s: invalid %s\n", *argv, message);
  }

  exit(error);
}

/*
** End
*/



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/usage.c $ $Rev: 710521 $")
#endif
