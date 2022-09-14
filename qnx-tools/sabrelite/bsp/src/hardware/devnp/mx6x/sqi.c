/*
 * $QNXLicenseC:
 * Copyright 2014, QNX Software Systems.
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

#include <math.h>
#include <stdbool.h>

#define SQI_SAMPLE_COUNT 100

static unsigned 	sqi_head = 0;
static float 		sqi_sample_data[SQI_SAMPLE_COUNT]={0};
static double 		sqi_sum = 0;
static bool 		sqi_filled = false;

/* clear out the SQI sample buffer*/
void mx6_clear_sample()
{
	unsigned i = 0;
	
	/*check if buffer is clean*/
	if ( (!sqi_filled) && (sqi_head == 0) ) {
		return;
	}
	for (i = 0; i < SQI_SAMPLE_COUNT; i++) {
		sqi_sample_data[i] = 0;	
	}
	sqi_filled = false;
	sqi_head = 0;
	sqi_sum = 0;
}

/* calculate the SQI*/
int mx6_calculate_sqi(unsigned val)
{
	/*
	According to LGE
	
	3. Convert resulting value to decimal (Q)
	4. Convert Q to a logarithmic value D:
	D = 10*log10(Q/32768)
	5. Average over 100 readings:
	A = avg(D[0:99])
	6. Subtract baseline value:
	M = –20.0 – A
	7. Calculate SQI according to the following ranges:
	• SQI = 0 when M is smaller than 0.0
	• SQI = 1 when M is between 0.0 and 2.0
	• SQI = 2 when M is between 2.0 and 4.0
	• SQI = 3 when M is between 4.0 and 6.0
	• SQI = 4 when M is between 6.0 and 8.0
	• SQI = 5 when M is greater than 8.0 
	*/
		
	double q=0,m=0;
	
	//convert val to decimal q;
	q = val;
	q = 10 * log10(q/32768);
	sqi_sum -= sqi_sample_data[sqi_head];
	sqi_sum += q;
	sqi_sample_data[sqi_head] = q;
	sqi_head++;
	if (sqi_head >= SQI_SAMPLE_COUNT) 
		sqi_head = 0;
	
	/*
	  if the sample buffer is not filled fully and when the sqi_head back to 0
	  the sample buffer is filled with number of SQI_SAMPLE_COUNT data.
	  from now on, new data come in will overwrite the old data.
	*/
	if ( (!sqi_filled) && (sqi_head == 0) )
		sqi_filled = true;
	 
	if (sqi_filled)
		m = -20.0 - sqi_sum/SQI_SAMPLE_COUNT;
	else
		m = -20.0 - sqi_sum/sqi_head;
	
	if (m < 0.0)
		return 0;
	else if ( (m >= 0.0) && (m<2.0) )
		return 1;
	else if ( (m >= 2.0) && (m<4.0) )
		return 2; 
	else if ( (m >= 4.0) && (m<6.0) )
		return 3; 
	else if ( (m >= 6.0) && (m<8.0) )
		return 4; 
	else if ( m >= 8.0) 
		return 5; 
	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL$ $Rev$")
#endif
