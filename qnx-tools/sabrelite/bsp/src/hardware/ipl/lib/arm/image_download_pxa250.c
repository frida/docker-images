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
#include <inttypes.h>

typedef struct	data_record {
	uint8_t	cmd;
	uint8_t	seq;
	uint8_t	cksum;
	uint8_t	nbytes;
	long	daddr;
} data_record_t;

#define DATA_RECORD_HEADER		8

#define	START_CMD				0x80
#define	DATA_CMD				0x81
#define	GO_CMD				    0x82
#define	ECHO_CMD				0x83
#define	ABORT_CMD				0x88

#define	ABORT_CKSUM				1
#define	ABORT_SEQ				2
#define	ABORT_PROTOCOL		    3

void
abort_cmd_pxa250(char abort)
{
	put_byte_pxa250(ABORT_CMD);
	put_byte_pxa250(abort);
}

unsigned
image_download_pxa250(unsigned dst_address)
{
	char			seq=0;
	int				i;
	data_record_t	record;
	char 			*src;
	char			*dst;	

	/*
	 * set destination address within memory
	 */
	dst = (char *)dst_address;

	/*
	 * Wait for initial start record
	 */
	while (get_byte_pxa250() != START_CMD)
		;

	while (1) {
		/*
		 * start processing the data/go records
		 */
		record.cmd = get_byte_pxa250();
		if (record.cmd != DATA_CMD) {
			/*
			 * check for a GO cmd to return control to the IPL
			 */
			if (record.cmd == GO_CMD) 	
				return(0);

			abort_cmd_pxa250(ABORT_PROTOCOL);
			return(1);
		}
		

		/*
		 *	read data_record header
		 *  (DATA_RECORD_HEADER -1 since cmd already consumed by get_byte)
		 */
	    src = (char *)&record.seq;

		for (i = 0; i < (DATA_RECORD_HEADER - 1) ; i++) {
			*src = get_byte_pxa250();
			src++;
		}
			
		if (seq != record.seq) {
			abort_cmd_pxa250(ABORT_SEQ);
			return(1);
		}
		else {
			seq = (seq + 1) & 0x7f;
		}

		/*
		 * Get rest of data
		 */
		for (i = 0; i <= record.nbytes; i++) {
            *dst = get_byte_pxa250();
			dst++;
		}
	}
}


void
debug_char_pxa250(char c)
{
	if (c == '\n')
		put_byte_pxa250('\r');
	put_byte_pxa250(c);
}

void
debug_string_pxa250(const char *str)
{
	while (*str) {
		debug_char_pxa250(*str++);
	}
}

void
debug_hex_pxa250(unsigned x)
{
	int					i;
	char				buf[9];
	static const char	hex[] = "0123456789ABCDEF";

	for (i = 0; i < 8; i++) {
		buf[7-i] = hex[x & 15];
		x >>= 4;
	}
	buf[8] = '\0';
	debug_string_pxa250(buf);
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/arm/image_download_pxa250.c $ $Rev: 711024 $")
#endif
