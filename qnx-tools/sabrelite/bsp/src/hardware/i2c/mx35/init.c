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


#include "proto.h"

void *
mx35_init(int argc, char *argv[])
{
    mx35_dev_t      *dev;
  
    if (-1 == ThreadCtl(_NTO_TCTL_IO, 0)) {
        perror("ThreadCtl");
        return NULL;
    }

    dev = malloc(sizeof(mx35_dev_t));
    if (!dev)
        return NULL;


    if(strstr(argv[argc-1], "controller") !=NULL){
	sscanf(argv[argc-1],"controller=%u",&dev->unit);
	argc--;
    }
    else
    	dev->unit= -1;

    if (-1 == mx35_options(dev, argc, argv))
        goto fail;
	
    if(dev->physbase == 0 || dev->intr ==0){
	slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,"i2c-mx35 error : Invalid  I2C controller physics based address or IRQ value.");
	slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,"i2c-mx35 error : Please check the command line or Hwinfo default setting.");
	goto fail;
     }

    dev->regbase = mmap_device_io(dev->reglen, dev->physbase);
    if (dev->regbase == (uintptr_t)MAP_FAILED) {
        perror("mmap_device_io");
        goto fail;
    }

    /* Setup I2C module */
	out16(dev->regbase + MX35_I2C_CTRREG_OFF, 0);
	out16(dev->regbase + MX35_I2C_STSREG_OFF, 0);
	delay(1);
	out16(dev->regbase + MX35_I2C_CTRREG_OFF, CTRREG_IEN);

    /* Initialize interrupt handler */
    SIGEV_INTR_INIT(&dev->intrevent);
    dev->iid = InterruptAttachEvent(dev->intr, &dev->intrevent, 
            _NTO_INTR_FLAGS_TRK_MSK);
    if (dev->iid == -1) {
        perror("InterruptAttachEvent");
        goto fail;
    }

    /* Set clock prescaler using default baud*/
	mx35_set_bus_speed(dev, 100000, NULL);

#if 0
    /* Set Own Address */
	out16(dev->regbase + MX35_I2C_ADRREG_OFF , (dev->own_addr << 1));
#endif

    /* enable interrupts */
	out16(dev->regbase + MX35_I2C_CTRREG_OFF, in16(dev->regbase + MX35_I2C_CTRREG_OFF) | CTRREG_IIEN );
    return dev;

fail:
    free(dev);
    return NULL;
}

