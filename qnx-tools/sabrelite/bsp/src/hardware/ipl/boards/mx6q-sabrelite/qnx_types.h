/*
 * $QNXLicenseC: 
 * Copyright 2011, QNX Software Systems.  
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

#ifndef __QNX_TYPES_H_
#define __QNX_TYPES_H_

#define in8(a)          (*(volatile unsigned char *)(a))
#define in16(a)         (*(volatile unsigned short *)(a))
#define in32(a)         (*(volatile unsigned int *)(a))
#define out8(a, v)      (*(volatile unsigned char *)(a) = (v))
#define out16(a, v)     (*(volatile unsigned short *)(a) = (v))
#define out32(a, v)     (*(volatile unsigned int *)(a) = (v))

#define  reg32clrbit(addr,bitpos) \
         out32((addr),(in32((addr)) & (0xFFFFFFFF ^ (1<<(bitpos)))))

#define  reg32setbit(addr,bitpos) \
         out32((addr),(in32((addr)) | (1<<(bitpos))))

#endif /* __QNX_TYPES_H_ */


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/boards/mx6q-sabrelite/qnx_types.h $ $Rev: 740617 $")
#endif
