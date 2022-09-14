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


#include "mx6q.h"


static int mx6q_entry(void *dll_hdl, struct _iopkt_self *iopkt, char *options);

struct _iopkt_drvr_entry IOPKT_DRVR_ENTRY_SYM(mx6q) = IOPKT_DRVR_ENTRY_SYM_INIT(mx6q_entry);

#ifdef VARIANT_a
#include <nw_dl.h>
/* This is what gets specified in the stack's dl.c */
struct nw_dll_syms mx6q_syms[] = {
        {"iopkt_drvr_entry", &IOPKT_DRVR_ENTRY_SYM(mx6q)},
        {NULL, NULL}
};
#endif


/*
 * mx6q_entry()
 *
 * Description: main entry point for the driver
 *
 * Returns: 0 if OK else -1 on error with errno set
 *
 */
int
mx6q_entry(void *dll_hdl, struct _iopkt_self *iopkt, char *options)
{
	return  mx6q_detect(dll_hdl, iopkt, options);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/io-pkt/sys/dev_qnx/mx6x/mx6q.c $ $Rev: 746167 $")
#endif
