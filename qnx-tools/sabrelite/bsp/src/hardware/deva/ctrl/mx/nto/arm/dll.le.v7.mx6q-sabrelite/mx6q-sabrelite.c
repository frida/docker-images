/*
 * $QNXLicenseC: 
 * Copyright 2011, QNX Software Systems.  
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

#include <audio_driver.h>
#include "mxssi.h"
#include "sgtl5000.h"

sgtl5000_config sgtl5000conf;

/******************************
 * Called by audio controller *
 *****************************/
int
codec_mixer (ado_card_t * card, HW_CONTEXT_T * mx)
{

	sgtl5000conf.vdda_voltage_mv = VDDA_VOLTAGE_MV;
	sgtl5000conf.vddio_voltage_mv = VDDIO_VOLTAGE_MV;
	sgtl5000conf.vddd_voltage_mv = VDDD_VOLTAGE_MV;
	return (sgtl5000_mixer (card, &(mx->mixer), mx->mixeropts, mx->pcm1, &sgtl5000conf));
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/deva/ctrl/mx/nto/arm/dll.le.v7.mx6q-sabrelite/mx6q-sabrelite.c $ $Rev: 737078 $")
#endif
