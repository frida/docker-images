/*
 * $QNXLicenseC: 
 * Copyright 2007, 2008, QNX Software Systems.  
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



#include "ipl.h"
#include <hw/inout.h>

static ser_dev *ser = 0;

void init_serdev(ser_dev *dev)
{
	ser = dev;
}

unsigned char 
ser_poll(void)
{
	if (ser == 0) return '\0';
	return (ser->poll());
}

unsigned char 
ser_getchar(void)
{
	if (ser == 0) return '\0';
	return (ser->get_byte());
}

void
ser_putchar(char c)
{
	if (ser == 0) return;
	if (c == '\n')
		ser->put_byte('\r');
	ser->put_byte(c);
}

void
ser_putstr(const char *str)
{
	if (ser == 0) return;
	while (*str)
		ser_putchar(*str++);
}

static void
_ser_puthex(unsigned x, unsigned size)
{
	int					i;
	char				buf[9];
	static const char	hex[] = "0123456789ABCDEF";

	for (i = 0; i < size; i++) {
		buf[(size - 1) - i] = hex[x & 15];
		x >>= 4;
	}
	buf[i] = '\0';
	ser_putstr(buf);
}

void 
ser_putdec(unsigned n)
{
	int		i = 0;
	char	buf[10];
	unsigned	m;
	static const char digits[] = "0123456789";

	do {
		m = n;
		n = n / 10;
		buf[i++] = digits[m - n * 10];
	} while (n > 0);

	for (i = i - 1; i >= 0; i--) {
		ser_putchar(buf[i]);
	}
}

void ser_puthex32(unsigned x) {_ser_puthex(x, 8); /* 8 nibbles*/}
void ser_puthex16(unsigned x) {_ser_puthex(x, 4); /* 4 nibbles*/}
void ser_puthex8(unsigned x) {_ser_puthex(x, 2); /* 2 nibbles*/}
void ser_puthex(unsigned x) {ser_puthex32(x);}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/ser_dev.c $ $Rev: 722094 $")
#endif
