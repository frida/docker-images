/*
 * $QNXLicenseC: 
 * Copyright 2008,2009 QNX Software Systems.  
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

////////////////
// local vars //
////////////////
static uint32_t channel_mask;   //channels that belong to THIS process
static const struct sigevent * event_array[SDMA_N_CH];
static sdmairq_callback_t callback_array[SDMA_N_CH];
static int id;

/////////////////
// global vars //
/////////////////
extern uintptr_t sdma_base;


////////////////////////////////////////////////////////////////////////////////
//                                  ISR                                       //
////////////////////////////////////////////////////////////////////////////////

const struct sigevent *irq_handler(void *area, int id) {
    uint32_t irq_status;
    uint32_t i;
    
    irq_status = in32(sdma_base + SDMA_INTR) & channel_mask;
    
    // find first active interrupt that belongs to this process
    for(i=0; i < SDMA_N_CH; i++) {
       if (irq_status & (1 << i)) {
            // clear irq status bit i
            out32(sdma_base + SDMA_INTR,(1 << i));
            
            //call the callback if present
            if (callback_array[i]) {
                callback_array[i](i);   
            }
            return event_array[i];
        }
    }
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
//                             PUBLIC FUNCTIONS                               //
////////////////////////////////////////////////////////////////////////////////

int sdmairq_init(uint32_t irq) {
    int i;
    
    ThreadCtl( _NTO_TCTL_IO, 0 );
    id = InterruptAttach( irq, irq_handler,NULL,0,_NTO_INTR_FLAGS_TRK_MSK);
    
    channel_mask=0;
    for(i=0;i<SDMA_N_CH;i++) {
        event_array[i] = NULL;
        callback_array[i] = NULL;
    }
    return 0;
}

void sdmairq_fini() {
    InterruptDetach(id);    
}

void sdmairq_event_add(uint32_t channel, const struct sigevent *event) {
    atomic_set(&channel_mask,1 << channel);
    event_array[channel] = event;
}

void sdmairq_event_remove(uint32_t channel) {
    atomic_clr(&channel_mask,1 << channel);
    event_array[channel] = NULL;
}

void sdmairq_callback_add(uint32_t channel,sdmairq_callback_t func_ptr) {
    callback_array[channel] = func_ptr;
}

void sdmairq_callback_remove(uint32_t channel) {
    callback_array[channel] = NULL;
}
    



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/dma/sdma/irq.c $ $Rev: 725865 $")
#endif
