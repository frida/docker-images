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



#ifndef __IPL_H_INCLUDED
#define __IPL_H_INCLUDED

#include <sys/platform.h>
#include <sys/startup.h>

/*Global definitions for some space that we share in the IPL code*/

extern char						scratch [512];
extern struct startup_header	startup_hdr;

/*
	For X86 bios extension
*/

extern char						scratch_ext [512];
extern struct startup_header	startup_hdr_ext;

/* image_download_8250.c */
extern void init_8250(unsigned address, unsigned baud, unsigned clk, unsigned divisor);
int image_download_8250(unsigned baud, unsigned port_address, unsigned dst_address);
unsigned char get_byte(unsigned port_address);
void put_byte(unsigned port_address, unsigned char data);
/* image_scan.c */
extern unsigned long image_scan (unsigned long start, unsigned long end);
extern int small_checksum(int *iray, long len);
extern int checksum (unsigned long addr, long len);
/* image_setup.c */
extern int image_setup (unsigned long addr);
/* image_start.c */
extern int image_start (unsigned long addr);
/* jump.c -- platform dependant */
extern void jump (unsigned long addr);

/*
 * Another set of image_xx function, which aiming to reduce the redundant copy of
 * startup header, startup code, and IFS code
 */
extern int checksum_2 (unsigned long addr, long len);
extern unsigned long image_scan_2 (unsigned long start, unsigned long end, int docksum);
extern int image_setup_2 (unsigned long addr);
extern int image_start_2 (unsigned long addr);

/* copy.c */
extern void copy (unsigned long dst, unsigned long src, unsigned long size);

static __inline__ unsigned short __myds(void){
	unsigned short __seg;
	
	__asm__ __volatile__(
		"movw %%ds,%0\n\t"
		: "=r" (__seg)
		:);
		
	return (__seg);
}

#define PHYS(a)  (((unsigned long) __myds() << 4) + ((unsigned) (a)))

typedef struct {
	unsigned short limit;
	unsigned short base_lo;
	char		base_hi;			/* bits 16 to 23 */
	char		flags;
	char		limflags;			/* limit 16 to 19 and extra flags */
	char		base_xhi;			/* bits 24 to 31 */
} segT;


/*
bioscopy.c
*/

extern unsigned char int15(unsigned __ah, unsigned __cx, void *__si, unsigned __es);
extern unsigned char _int15_copy(long address, long srcaddr, int nbytes);
extern unsigned char int15_copy(long from, long to, long len); 

/*
	io.c
*/

extern void _print_char(int c);
extern void print_char (int c);
extern void print_string (char *msg);
extern void print_var (unsigned long n, int l);
extern void print_long (unsigned long n);
extern void print_word (unsigned short n);
extern void print_byte (int n);
extern void print_sl (char *s, unsigned long n);
extern int _char_waiting(void); 
extern int char_waiting (void); 
extern int _get_char (void);
extern int get_char (void);
extern unsigned short *btimer(void);
extern int get_timed_char (unsigned ms);

/*
image_scan_ext.c  (image_scan for X86 bios extension)
*/

extern unsigned long image_scan_ext (unsigned long start, unsigned long end);

/*
image_setup_ext.c (image_setup for X86 bios extension)
*/

extern int image_setup_ext (unsigned long addr);

/*
image_start_ext.c  (image_start for X86 bios extension)
*/

extern int image_start_ext(void);

/*
image_add_info.c (add a startup info structure to the startup header. Returns
					1 upon success, otherwise 0)
*/
extern int image_add_info(void *imaddr, struct startup_info_hdr *info);

#ifdef  __PPC__
extern void				init_8260scc1uart(unsigned, unsigned, unsigned);
extern void				putchar_8260scc1(unsigned);
extern unsigned char	getchar_8260scc1(void);
extern unsigned char	poll_8260scc1(void);
extern void				putstr_8260scc1(unsigned char *);
extern void				puthex_8260scc1(unsigned long, long);
extern unsigned			image_download_8260scc1(unsigned, void(*sigled)(unsigned));
#endif


#ifdef  __SH__
extern void				scif_putchar(int c);
extern int				scif_getchar(void);
extern void				scif_init(unsigned baud, unsigned pfclk);
extern void				scif_putstring(unsigned char *str);
extern void				scif_puthex(unsigned long n, long depth);
extern unsigned			scif_image_download(unsigned dst);
#endif


#ifdef	__ARM__
extern void arm_v7_dcache_invalidate();
extern void arm_v7_dcache_flush();
extern void arm_v7_icache_invalidate();
extern void arm_a8_dcache_flush_by_addr(unsigned long addr, unsigned len);
extern void arm_a8_enable_cache();
extern void arm_v7_enable_cache();
extern void arm_enable_mmu(unsigned long tlb);

/*
image_download_sa1100.c
NOTE: you must call init_sa1100() before calling image_download_sa1100()
*/
extern void				init_sa1100(unsigned port, unsigned baud, unsigned clk, int alt);
extern unsigned			image_download_sa1100(unsigned dst);
extern unsigned char	get_byte_sa1100();
extern void				debug_char_sa1100(char c);
extern void				debug_string_sa1100(const char *s);
extern void				debug_hex_sa1100(unsigned x);

/*
image_download_primecell.c
NOTE: you must call init_primecell() before calling image_download_primecell()
*/
extern void				init_primecell(unsigned port, unsigned baud, unsigned clk);
extern unsigned			image_download_primecell(unsigned dst);
extern unsigned char	get_byte_primecell();
extern void				debug_char_primecell(char c);
extern void				debug_string_primecell(const char *s);
extern void				debug_hex_primecell(unsigned x);

/*
image_download_pxa250.c
NOTE: you must call init_pxa250() before calling image_download_pxa250()
*/

extern void            init_pxa250(unsigned port, unsigned baud, unsigned clk);
extern void            debug_char_pxa250(char c);
extern void            debug_string_pxa250(const char *str);
extern void            debug_hex_pxa250(unsigned x);
extern unsigned char   get_byte_pxa250();
extern void            put_byte_pxa250(unsigned char data);
extern unsigned        image_download_pxa250(unsigned dst_address);

#endif


/*
 * These are the new stuffs for generic serial image download.
 * Use init_serdev() to register device specific character
 * get/put and poll(option) functions, after this, generic
 * image_download(), ser_getchar(), ser_putchar(), ser_putstr(),
 * ser_puthex() and ser_poll() will function properly.
 * NOTE: you must call init_serxxx() before calling any of those
 * functions.
 */

typedef struct _ser_dev_t {
	unsigned char	(*get_byte)(void);
	void			(*put_byte)(unsigned char);
	unsigned char	(*poll)(void);
} ser_dev;

extern void				init_serdev(ser_dev *);
extern unsigned			image_download_ser(unsigned);
extern unsigned char	ser_getchar(void);
extern unsigned char	ser_poll(void);
extern void				ser_putchar(char);
extern void				ser_putstr(const char *);
extern void				ser_puthex(unsigned);
extern void				ser_puthex8(unsigned);
extern void				ser_puthex16(unsigned);
extern void				ser_puthex32(unsigned);
extern void				ser_putdec(unsigned);

extern void				init_ser8250(unsigned address, unsigned size, unsigned shift, unsigned baud, unsigned clk, unsigned divisor);

#ifdef	__ARM__
extern void				init_sersa1100(unsigned port, unsigned baud, unsigned clk, int alt);
extern void				init_sermx1(unsigned port, unsigned baud, unsigned clk);
extern void				init_serpxa250(unsigned port, unsigned baud, unsigned clk);
extern void				init_seromap(unsigned address, unsigned baud, unsigned clk, unsigned divisor);
extern void				init_sermx6(unsigned port, unsigned baud, unsigned clk, unsigned divisor);
#endif

#ifdef  __PPC__
extern void				init_serppc8260scc1(unsigned immr, unsigned off_dpram, unsigned baud, unsigned clk, unsigned div);
#endif

#ifdef	__SH__
void					init_serscif(unsigned base, int baud, int clk, int div, int extclk);
#endif

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/lib/ipl.h $ $Rev: 739242 $")
#endif
