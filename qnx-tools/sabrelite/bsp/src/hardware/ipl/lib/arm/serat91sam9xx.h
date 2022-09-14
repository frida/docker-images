/*
 * $QNXLicenseC: 
 * Copyright 2009, QNX Software Systems.  
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

#ifndef _SERAT91SAM_H_
#define _SERAT91SAM_H_

#define UART_TX_EMPTY                   0x0200
#define UART_RX_RDY                     0x01
#define UART_STATUS_OFFSET              0x0014
#define UART_RXDATA_OFFSET              0x0018
#define UART_TXDATA_OFFSET              0x001C

/* PIO Registers */
#define PIO_ASR                         0x00000070    /* Select A Peripheral*/
#define PIO_PDR                         0x00000004    /* PIO Disable */ 

/* DBGU Device regiser */
#define UART_MODE_REG                   0x0004
#define UART_ID_REG                     0x000c
#define UART_BRGR_REG                   0x0020
#define DISABLE_UART_INTR               0xffffffff
#define ENABLE_TXRX                     0x00000050
#define DBGU_MR_VAL                     0x00000800

typedef struct serat91 {
    unsigned base;
    unsigned dbgu_offset;
    unsigned ser0_offset;
    unsigned pio_offset;
    unsigned dbgu_pins;
    unsigned disable_txrx;
    unsigned dbgu_brgr_val;
} serat91_t;

void init_serat91sam9xx(serat91_t * ser);

#endif  /*  #ifndef _SERAT91SAM_H_  */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/arm/serat91sam9xx.h $ $Rev: 711024 $")
#endif
