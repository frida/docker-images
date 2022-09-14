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
download_abort(char abort)
{
	ser_putchar(ABORT_CMD);
	ser_putchar(abort);
}

unsigned
image_download_ser(unsigned dst_address)
{
	char			seq = 0;
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
	while (ser_getchar() != START_CMD)
		;

	while (1) {
		/*
		 * start processing the data/go records
		 */
		record.cmd = ser_getchar();

		if (record.cmd != DATA_CMD) {
			/*
			 * check for a GO cmd to return control to the IPL
			 */
			if (record.cmd == GO_CMD) 	
				return (0);

			download_abort(ABORT_PROTOCOL);

			return (1);
		}
		

		/*
		 *	read data_record header
		 *  (DATA_RECORD_HEADER -1 since cmd already consumed by get_byte)
		 */
	    src = (char *)&record.seq;

		for (i = 0; i < (DATA_RECORD_HEADER - 1) ; i++) {
			*src = ser_getchar();
			src++;
		}
			
		if (seq != record.seq) {
			download_abort(ABORT_SEQ);
			return (1);
		}
		else {
			seq = (seq + 1) & 0x7f;
		}

		/*
		 * Get rest of data
		 */
		for (i = 0; i <= record.nbytes; i++) {
            *dst = ser_getchar();
			dst++;
		}
	}
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/image_download_ser.c $ $Rev: 711024 $")
#endif
