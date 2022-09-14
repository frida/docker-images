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
** File: f3s_socket_syspage.c
**
** Description:
**
** This file contains the syspage token parser
**
** Ident: $Id: socket_syspage.c 710521 2013-06-17 17:57:28Z targentina@qnx.com $
*/

/*
** Includes
*/

#include <string.h>
#include <sys/f3s_mtd.h>
#include <sys/syspage.h>
#include "errno.h"

/*
** Function: f3s_socket_syspage
*/

int 
f3s_socket_syspage(f3s_socket_t * socket)
{
	int             		index = 0;
	char					*str = SYSPAGE_ENTRY(strings)->data;
	struct asinfo_entry		*as = SYSPAGE_ENTRY(asinfo);
	unsigned				num;

	/* find syspage entry corresponding to socket index */

	for(num = _syspage_ptr->asinfo.entry_size / sizeof(*as); num > 0; --num) 
	{
		if(strcmp(&str[as->name], "flashfsys") == 0) 
		{
			if (index == socket->socket_index)
			{

				/* set socket address and size */

				socket->address = as->start;
				socket->window_size = as->end - as->start + 1;

				/* everything is fine */

				return EOK;
			}
			index++;
		}
		++as;
	}

	/* did not find entry */

	return ENOENT;
}

/*
** End
*/



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/socket_syspage.c $ $Rev: 710521 $")
#endif
