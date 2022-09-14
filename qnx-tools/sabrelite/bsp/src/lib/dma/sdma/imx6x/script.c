/*
 * $QNXLicenseC: 
 * Copyright 2010,2011, QNX Software Systems.  
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


#include "sdma.h"
#include "microcode.h"

/* ROM Script locations */
#define AP_2_AP_ADDR		642
#define AP_2_MCU_ADDR		683
#define MCU_2_AP_ADDR		747
#define MCU_2_SHP_ADDR		960
#define SHP_2_MCU_ADDR		891
#define UARTSH_2_MCU_ADDR	1032
#define MCU_2_SPDIF_ADDR	1134
#define SPDIF_2_MCU_ADDR	1100

/* RAM Script locations */
#define MCU_2_SSIAPP_ADDR	6144
#define SSIAPP_2_MCU_ADDR	6585
#define UART_2_MCU_ADDR		6762


///////////////////////////////////////////////////////////////////////////////
//                            PUBLIC FUNCTIONS                               //
///////////////////////////////////////////////////////////////////////////////


int sdmascript_lookup( sdma_scriptinfo_t * scriptinfo ) {
     
	// get ram microcode data...
	scriptinfo->ram_microcode_info.p    = sdma_code;
	scriptinfo->ram_microcode_info.addr = SDMA_RAM_CODE_START_ADDR;
	scriptinfo->ram_microcode_info.size = sizeof(sdma_code);	// get size of SDMA firmware in bytes
       
	// populate the scriptinfo struct with script addresses
	scriptinfo->script_addr_arr[SDMA_CHTYPE_AP_2_AP]      = AP_2_AP_ADDR;
	scriptinfo->script_addr_arr[SDMA_CHTYPE_MCU_2_AP]     = MCU_2_AP_ADDR;
	scriptinfo->script_addr_arr[SDMA_CHTYPE_AP_2_MCU]     = AP_2_MCU_ADDR;
	scriptinfo->script_addr_arr[SDMA_CHTYPE_MCU_2_SHP]    = MCU_2_SHP_ADDR;
	scriptinfo->script_addr_arr[SDMA_CHTYPE_SHP_2_MCU]    = SHP_2_MCU_ADDR;
	scriptinfo->script_addr_arr[SDMA_CHTYPE_UARTSH_2_MCU] = UARTSH_2_MCU_ADDR;
	scriptinfo->script_addr_arr[SDMA_CHTYPE_MCU_2_SPDIF]  = MCU_2_SPDIF_ADDR;
	scriptinfo->script_addr_arr[SDMA_CHTYPE_SPDIF_2_MCU]  = SPDIF_2_MCU_ADDR;
	scriptinfo->script_addr_arr[SDMA_CHTYPE_UART_2_MCU]   = UART_2_MCU_ADDR;
	return EOK;
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/dma/sdma/imx6x/script.c $ $Rev: 733037 $")
#endif
