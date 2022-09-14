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

static void mx35_i2c_reset(mx35_dev_t *dev)
{
	out16(dev->regbase + MX35_I2C_CTRREG_OFF, 0);
	out16(dev->regbase + MX35_I2C_STSREG_OFF, 0);
	delay(1);
	out16(dev->regbase + MX35_I2C_CTRREG_OFF, CTRREG_IEN);
	out16( dev->regbase + MX35_I2C_FRQREG_OFF, dev->i2c_freq_val );
	dev->restart = 0;
	delay(1);	
}

int mx35_wait_bus_not_busy(mx35_dev_t *dev)
{
    unsigned        tries = 1000000;
	if(dev->restart) {
		return 0;
	}else {
		while (in16(dev->regbase + MX35_I2C_STSREG_OFF) & STSREG_IBB){
			if (tries-- == 0){
				_i2c_slogf("wait bus idle failed (%x %x)",in16(dev->regbase + MX35_I2C_CTRREG_OFF), in16(dev->regbase + MX35_I2C_STSREG_OFF));
				/* reset the controller to see if it's able to recover*/
				mx35_i2c_reset(dev);
				//try again to see if it's OK now after reset
				if(in16(dev->regbase + MX35_I2C_STSREG_OFF) & STSREG_IBB){
					delay(1);
					return -1;
				}else{
					break;
				}
			}
		}
		return 0;
	}
	return -1;
}

uint32_t
mx35_wait_status(mx35_dev_t *dev)
{
	uint16_t	status;
	uint64_t	ntime = 500000000ULL;
	int			interr = EOK;

	while ( interr != ETIMEDOUT ){
		TimerTimeout(CLOCK_MONOTONIC, _NTO_TIMEOUT_INTR, NULL, &ntime, NULL);
		interr = InterruptWait_r(0, NULL);
		status = in16(dev->regbase + MX35_I2C_STSREG_OFF);
		if (status & STSREG_IIF){
			out16(dev->regbase + MX35_I2C_STSREG_OFF, in16(dev->regbase + MX35_I2C_STSREG_OFF) & ~(STSREG_IIF));
			InterruptUnmask(dev->intr, dev->iid);
			return status;
		}
	}
	_i2c_slogf("mx35_wait_status timedout (%x %x)",in16(dev->regbase + MX35_I2C_CTRREG_OFF), in16(dev->regbase + MX35_I2C_STSREG_OFF));
	//timeout case, we need reset
	mx35_i2c_reset(dev);
	
	return 0;
}

i2c_status_t
mx35_recvbyte(mx35_dev_t *dev, uint8_t *byte, int nack, int stop){
	uint32_t	status;

	status = mx35_wait_status(dev);

	if (!(status & STSREG_ICF)){
		if(status)
			mx35_i2c_reset(dev);
		return I2C_STATUS_ERROR;
	}

	if (nack){
		out16(dev->regbase + MX35_I2C_CTRREG_OFF, CTRREG_IEN | CTRREG_IIEN | CTRREG_MSTA | CTRREG_TXAK);
	}else if(stop){
		out16(dev->regbase + MX35_I2C_CTRREG_OFF, CTRREG_IEN | CTRREG_TXAK);
	}

	*byte = in16(dev->regbase + MX35_I2C_DATREG_OFF) & 0xFF;
	return 0;
}

i2c_status_t
mx35_sendbyte(mx35_dev_t *dev, uint8_t byte){
	uint32_t status;

	out16(dev->regbase + MX35_I2C_CTRREG_OFF, in16(dev->regbase + MX35_I2C_CTRREG_OFF) | CTRREG_MTX);
	out16(dev->regbase + MX35_I2C_DATREG_OFF, byte);

	status = mx35_wait_status(dev);
	if (!(status & STSREG_ICF)){
		if(status)
			mx35_i2c_reset(dev);
		return I2C_STATUS_ERROR;
	}

	if (!(in16(dev->regbase + MX35_I2C_CTRREG_OFF) & CTRREG_MSTA)){
		if (status & STSREG_IAL) {
			out16(dev->regbase + MX35_I2C_STSREG_OFF, in16(dev->regbase + MX35_I2C_STSREG_OFF) & ~STSREG_IAL);
			mx35_i2c_reset(dev);
			return I2C_STATUS_ARBL;
		}
		if (status & STSREG_IAAS){
			mx35_i2c_reset(dev);
			return I2C_STATUS_ERROR;
		}
	}

	if (status & STSREG_RXAK){
		out16(dev->regbase + MX35_I2C_CTRREG_OFF, CTRREG_IEN);
		mx35_i2c_reset(dev);
		return I2C_STATUS_NACK;
	}
	return 0;
}

static inline i2c_status_t mx35_wait_busy(mx35_dev_t *dev)
{
	//wait for 500us
	int timeout = 5000;
	while(!(in16(dev->regbase + MX35_I2C_STSREG_OFF) & STSREG_IBB) && timeout--){
		nanospin_ns(100);
	}
	if(timeout<=0){
		_i2c_slogf("mx35_wait_busy timedout (%x %x)",in16(dev->regbase + MX35_I2C_CTRREG_OFF), in16(dev->regbase + MX35_I2C_STSREG_OFF));
		mx35_i2c_reset(dev);
		return (I2C_STATUS_ERROR);
	}

	return (0);
}

i2c_status_t
mx35_sendaddr7(mx35_dev_t *dev, unsigned addr, int read, int restart){
	out16(dev->regbase + MX35_I2C_CTRREG_OFF, CTRREG_IEN | CTRREG_IIEN | CTRREG_MSTA |
												(restart? CTRREG_RSTA: 0) | CTRREG_MTX);
	if(mx35_wait_busy(dev)){
		return I2C_STATUS_ERROR;
	}

	return mx35_sendbyte(dev, (addr << 1) | read);
}

i2c_status_t
mx35_sendaddr10(mx35_dev_t *dev, unsigned addr, int read, int restart){
	i2c_status_t	err;

	out16(dev->regbase + MX35_I2C_CTRREG_OFF, CTRREG_IEN | CTRREG_IIEN | CTRREG_MSTA |
												(restart? CTRREG_RSTA: 0) | CTRREG_MTX);
	if(mx35_wait_busy(dev)){
		return I2C_STATUS_ERROR;
	}
	err = mx35_sendbyte(dev, MX35_I2C_XADDR1(addr));
	if (err!=0)
		return err;
	err = mx35_sendbyte(dev, MX35_I2C_XADDR2(addr));
	if (err!=0)
		return err;

	if (read){
		out16(dev->regbase + MX35_I2C_CTRREG_OFF, CTRREG_IEN | CTRREG_IIEN | CTRREG_MSTA |
													CTRREG_RSTA | CTRREG_MTX);
		err = mx35_sendbyte(dev, MX35_I2C_XADDR1(addr) | read);
		if (err!=0)
			return err;
	}
	return 0;
}

__SRCVERSION( "$URL: http://svn/product/branches/6.5.0/trunk/hardware/i2c/mx35/wait.c $ $Rev: 408944 $" );
