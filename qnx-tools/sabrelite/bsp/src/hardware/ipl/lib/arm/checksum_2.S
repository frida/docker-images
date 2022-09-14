/*
 * $QNXLicenseC:
 * Copyright 2013, QNX Software Systems.
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

	.text
	.align	2
	.globl	checksum_2
	.type checksum_2, function

/*
 * unsigned checksum_2(const unsigned src, int len)
 * mkifs generated startup and IFS are aligned to word address
 * Also the length is integer multiple of word size
 */
checksum_2:
	stmdb	sp!,{r3-r11,lr}
	add 	r1, r1, r0			@ end of the source address
	mov		r2, #0				@ initial checksum

	/*
	* Get src 16 byte aligned to optimise write buffer usage
	*/
0:	tst	 	r0, #15
	beq		1f
	ldr		r3, [r0]			@ Load a word
	add		r0, r0, #4
	add		r2, r2, r3			@ Add to checksum
	cmp		r0, r1
	blt		0b					@ check again
	bge		3f					@ done

1:								@ copy 32 bytes at a time
	sub 	r3, r1, r0
	cmp 	r3, #32
	blt 	2f					@ less than 32 bytes

	ldmia	r0!, {r3-r10}		@ copy from source address [r1]
	add		r2, r2, r3
	add		r2, r2, r4
	add		r2, r2, r5
	add		r2, r2, r6
	add		r2, r2, r7
	add		r2, r2, r8
	add		r2, r2, r9
	add		r2, r2, r10
	cmp		r0, r1
	blt		1b
	b 		3f

2:								@ handle the left over < 32 words
	ldr		r3, [r0]
	add		r2, r2, r3
	add		r0, r0, #4
	cmp		r0, r1
	blt		2b

3:
	mov		r0, r2
	ldmfd	sp!, {r3-r11, pc}


#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/arm/checksum_2.S $ $Rev: 723411 $";
.previous
#endif
#endif 
