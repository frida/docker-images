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

i2c_status_t
mx35_send(void *hdl, void *buf, unsigned int len, unsigned int stop)
{
    mx35_dev_t      *dev = hdl;
    i2c_status_t    status;

    if (len <= 0)
        return I2C_STATUS_DONE;

	if(mx35_wait_bus_not_busy(dev))
		return I2C_STATUS_ERROR;

	if (dev->slave_addr_fmt == I2C_ADDRFMT_7BIT)
		status = mx35_sendaddr7(dev, dev->slave_addr, MX35_I2C_ADDR_WR, dev->restart);
	else
		status = mx35_sendaddr10(dev, dev->slave_addr, MX35_I2C_ADDR_WR, dev->restart);

	if (status)
		return status;

	while (len > 0) {
		status = mx35_sendbyte(dev, *(uint8_t *)buf);
		if (status)
			return status;
		++buf; --len;
	}
	if (stop)
		out16(dev->regbase + MX35_I2C_CTRREG_OFF, CTRREG_IEN);

	dev->restart = !stop;
	return I2C_STATUS_DONE;
}

__SRCVERSION( "$URL: http://svn/product/branches/6.5.0/trunk/hardware/i2c/mx35/send.c $ $Rev: 390064 $" );
