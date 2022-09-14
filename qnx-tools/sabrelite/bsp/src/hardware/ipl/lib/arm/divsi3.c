/*
 * $QNXLicenseC:
 * Copyright 2011-2013, QNX Software Systems.
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

#define divnorm(num, den, sign) \
{ 								\
	if (num < 0) { 				\
		num = -num;				\
		sign = 1; 				\
	} else { 					\
		sign = 0; 				\
	} 							\
								\
	if (den < 0) { 				\
		den = - den; 			\
		sign = 1 - sign; 		\
	} 							\
}

static inline unsigned long 
divmodsi4(int modwanted, unsigned long num, unsigned long den)	
{						
	long int bit = 1;
	long int res = 0;
	while (den < num && bit && !(den & (1L<<31))) {
		den <<= 1;
		bit <<= 1;
	}						
	while (bit) {
		if (num >= den) {
			num -= den;				
			res |= bit;				
		}						

		bit >>= 1;
		den >>= 1;
	}

	if (modwanted) return num;
	return res;
}

#define exitdiv(sign, res) if (sign) { res = - res;} return res;

long 
__modsi3 (long numerator, long denominator)
{
	int sign = 0;
	long modul;

	if (numerator < 0) {
		numerator = -numerator;
		sign = 1;
	}

	if (denominator < 0) {
		denominator = -denominator;
	}	
	
	modul =	divmodsi4 (1, numerator, denominator);

	if (sign)
		return - modul;

	return modul;
}

long 
__divsi3 (long numerator, long denominator)
{
	int sign;
	long dividend;

	divnorm (numerator, denominator, sign);
	dividend = divmodsi4 (0, numerator, denominator);
	exitdiv (sign, dividend);
}

long 
__aeabi_uidiv (long numerator, long denominator)
{
	return __divsi3 (numerator, denominator);
}

long 
__umodsi3 (unsigned long numerator, unsigned long denominator)
{
	long modul;
	modul= divmodsi4 (1, numerator, denominator);
	return modul;
}

long 
__udivsi3 (unsigned long numerator, unsigned long denominator)
{
	long dividend;
	dividend =divmodsi4 (0, numerator, denominator);
	return dividend;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/arm/divsi3.c $ $Rev: 721095 $")
#endif
