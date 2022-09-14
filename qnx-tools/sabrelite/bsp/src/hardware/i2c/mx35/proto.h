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


#ifndef __PROTO_H_INCLUDED
#define __PROTO_H_INCLUDED

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <sys/mman.h>
#include <hw/inout.h>
#include <hw/i2c.h>
#include <sys/hwinfo.h>
#include <drvr/hwinfo.h>
#include <sys/slog.h>
#include <sys/slogcodes.h>

typedef struct _mx35_dev {

    void                *buf;
    unsigned            buflen;
    int                 unit;
    unsigned            reglen;
    uintptr_t           regbase;
    unsigned            physbase;

    int                 intr;
    int                 iid;
    struct sigevent     intrevent;

    unsigned            own_addr;
    unsigned            slave_addr;
	i2c_addrfmt_t       slave_addr_fmt;
	unsigned			restart;
    unsigned            options;
	unsigned			input_clk;
	unsigned			speed;
	unsigned			i2c_freq_val;
} mx35_dev_t;

#define MX35_I2C_XADDR1(addr)	(0xf0 | (((addr) >> 7) & 0x6))
#define MX35_I2C_XADDR2(addr)	((addr) & 0xff)
#define MX35_I2C_ADDR_RD			1
#define MX35_I2C_ADDR_WR			0

#define MX35_OPT_VERBOSE       	0x00000002
#define MX35_I2C_INPUT_CLOCK	66500000

#define MX35_I2C1_BASE_ADDR		0x43F80000
#define MX35_I2C2_BASE_ADDR		0x43F98000
#define MX35_I2C3_BASE_ADDR		0x43F84000

#define MX35_I2C1_IRQ				10
#define MX35_I2C2_IRQ				4
#define MX35_I2C3_IRQ				3

#define MX35_I2C_ADRREG_OFF		0x00
#define MX35_I2C_FRQREG_OFF		0x04
#define MX35_I2C_CTRREG_OFF		0x08
	#define CTRREG_IEN				(1 << 7)
	#define CTRREG_IIEN				(1 << 6)
	#define CTRREG_MSTA				(1 << 5)
	#define CTRREG_MTX				(1 << 4)
	#define CTRREG_TXAK				(1 << 3)
	#define CTRREG_RSTA				(1 << 2)
#define MX35_I2C_STSREG_OFF		0x0C
	#define STSREG_ICF				(1 << 7)
	#define STSREG_IAAS				(1 << 6)
	#define STSREG_IBB				(1 << 5)
	#define STSREG_IAL				(1 << 4)
	#define STSREG_SRW				(1 << 2)
	#define STSREG_IIF				(1 << 1)
	#define STSREG_RXAK				(1 << 0)
#define MX35_I2C_DATREG_OFF		0x10
#define MX35_I2C_REGLEN			0x18
#define MX35_I2C_OWN_ADDR			0x66

#define MX35_I2C_REVMAJOR(rev)      (((rev) >> 8) & 0xff)
#define MX35_I2C_REVMINOR(rev)      ((rev) & 0xff)

void *mx35_init(int argc, char *argv[]);
void mx35_fini(void *hdl);
int mx35_options(mx35_dev_t *dev, int argc, char *argv[]);

int mx35_wait_bus_not_busy(mx35_dev_t *dev);
int mx35_set_slave_addr(void *hdl, unsigned int addr, i2c_addrfmt_t fmt);
int mx35_set_bus_speed(void *hdl, unsigned int speed, unsigned int *ospeed);
int mx35_version_info(i2c_libversion_t *version);
int mx35_driver_info(void *hdl, i2c_driver_info_t *info);
int mx35_devctl(void *hdl, int cmd, void *msg, int msglen, 
        int *nbytes, int *info);
i2c_status_t mx35_recv(void *hdl, void *buf, 
        unsigned int len, unsigned int stop);
i2c_status_t mx35_send(void *hdl, void *buf, 
        unsigned int len, unsigned int stop);
uint32_t mx35_wait_status(mx35_dev_t *dev);
i2c_status_t mx35_sendaddr7(mx35_dev_t *dev,
                              unsigned addr, int read, int restart);
i2c_status_t mx35_sendaddr10(mx35_dev_t *dev,
                                unsigned addr, int read, int restart);
i2c_status_t mx35_sendbyte(mx35_dev_t *dev, uint8_t byte);
i2c_status_t mx35_recvbyte(mx35_dev_t *dev, uint8_t *byte,
                              int nack, int stop);
extern int _i2c_slogf(const char *fmt, ...);

#endif
