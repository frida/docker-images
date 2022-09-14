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




#include <hw/inout.h>
#include <hw/8250.h>
#include <inttypes.h>

typedef struct	data_record{
	uint8_t	cmd;
	uint8_t	seq;
	uint8_t	cksum;
	uint8_t	nbytes;
	long	daddr;
}data_record_t;

#define	START_CMD				0x80
#define	DATA_CMD				0x81
#define	GO_CMD				    0x82
#define	ECHO_CMD				0x83
#define	ABORT_CMD				0x88

#define	ABORT_CKSUM				1
#define	ABORT_SEQ				2
#define	ABORT_PROTOCOL		    3


//
//	user defined defaults
//

#define	DEFAULT_CLK				1843200
#define DEFAULT_DIV				16

#define DATA_RECORD_HEADER		8



void set_port(unsigned address, unsigned mask, unsigned data) {
	unsigned char c;

	c = in8(address);
	out8(address, (c & ~mask) | (data & mask));
}

void init_8250(unsigned address, unsigned baud, unsigned clk, unsigned divisor){
	unsigned 		lcr = 0;
	unsigned		value;
	

	//
	//	This routine will initialize the selected 8250 serial port
	//  to 8N1 parameters. 
	//

	//
	// Set Baud rate
	//

	value = clk/(baud * divisor);


	set_port(address+REG_LC, LCR_DLAB, LCR_DLAB);
	set_port(address+REG_DL0, 0xff, value & 0xff);
	set_port(address+REG_DL1, 0xff, value >> 8);
	set_port(address+REG_LC, LCR_DLAB, 0);

	//
	// Set data bits to 8
	//

	lcr |= 3;

	set_port(address+REG_LC, 0xFF, lcr);
}


unsigned char get_byte(unsigned port_address){
	int value =0 ;
	//
	//	wait for data to be available	
	//
	while(!(value & LSR_RXRDY)){
		value = in8(port_address+REG_LS);
	}
	return(in8(port_address));
}

void put_byte(unsigned port_address, unsigned char data){
	
	//
	//	wait for data to be available	
	//

	while(!(in8(port_address+REG_LS) & LSR_TXRDY));
	out8(port_address,data);
}


void abort_cmd(unsigned port_address, char abort){
	put_byte(port_address,ABORT_CMD);
	put_byte(port_address,abort);
}

unsigned image_download_8250(unsigned baud, unsigned port_address, unsigned dst_address){

	char			seq=0;
	int				i;
						
	data_record_t	record;
	char 			*src;
	char			*dst;	


	
	//
	//	setup serial port
	//

	init_8250(port_address, baud, DEFAULT_CLK, DEFAULT_DIV);

	//
	//	set destination address within memory
	//
		
	dst = (char *)dst_address;

	//
	//	wait for initial start record
	//

	while(get_byte(port_address) != START_CMD);

	while(1){
		//
		//	start processing the data/go records
		//

		record.cmd = get_byte(port_address);
		if (record.cmd != DATA_CMD){

			//
			//	check for a GO cmd 
			//  return control to the IPL
			//
	
			if (record.cmd == GO_CMD) 	
				return(0);

			abort_cmd(port_address,ABORT_PROTOCOL);
			return(1);
		}
		

		//
		//	now read data_record header  (DATA_RECORD_HEADER -1 since cmd
		//	already consumed by get_byte)
		//

	    src = (char *)&record.seq;

		for (i=0;i<(DATA_RECORD_HEADER -1) ;i++){
			*src = get_byte(port_address);
			src++;
		}
			
		if (seq != record.seq){
			abort_cmd(port_address,ABORT_SEQ);
			return(1);
		}
		else
			seq = (seq + 1) & 0x7f;

		//
		//	Get rest of data
		//

		for (i=0;i<=record.nbytes;i++){
            *dst = get_byte(port_address);
			dst++;
		}
	}
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/image_download_8250.c $ $Rev: 711024 $")
#endif
