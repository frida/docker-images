/*
 * $QNXLicenseC:
 * Copyright 2008, QNX Software Systems. 
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









#ifndef __F3S_MTD_H_INCLUDED
#define __F3S_MTD_H_INCLUDED

#if F3S_VER == 3
#include <fs/f3s_api.h>
#else
#if MTD_VER >= 2
#error "Cannot use new MTD with old flash file system"
#endif
#include <sys/f3s_api.h>
#endif

#include <inttypes.h>
#include <sys/neutrino.h>
#include <sys/syspage.h>
#include <libc.h>

#define F3S_BASETYPE	_uint64

typedef union intunion_s
{
	_uint8 		w8;
	_uint16 	w16;
	_uint32		w32;
	_uint64		w64;
}               intunion_t;

typedef struct f3s_flashcfg_s
{
	_uint32			bus_width;
	_uint32			device_width;
	_uint32			cfi_width;		/* bus_width except on x8/x16 flash */
	_uint32			chip_inter;
	_uint32			program_timeout;
	_uint32			erase_susp_timeout;
	_uint32			options; 		/* handle board specific options */
	F3S_BASETYPE	device_mult;
}f3s_flashcfg_t;

/* board specific options */
#define PAGE_MODE_ENABLE    0x1

extern f3s_flashcfg_t flashcfg;
extern unsigned verbose;
extern int		suspended;

extern void     (*writemem) (volatile void *ptr, F3S_BASETYPE value);
extern F3S_BASETYPE (*readmem) (volatile void *ptr);

void            writemem8(volatile void *ptr, F3S_BASETYPE value);
F3S_BASETYPE        readmem8(volatile void *ptr);

void            writemem16(volatile void *ptr, F3S_BASETYPE value);
F3S_BASETYPE        readmem16(volatile void *ptr);

void            writemem32(volatile void *ptr, F3S_BASETYPE value);
F3S_BASETYPE        readmem32(volatile void *ptr);

void writemem64(volatile void *ptr, F3S_BASETYPE value);
F3S_BASETYPE readmem64(volatile void *ptr);

int pad_value(intunion_t *value, volatile void *memory, unsigned shift, int front);
void write_value(volatile void *ptr, intunion_t *value);
void send_command(volatile void *ptr, F3S_BASETYPE command);
int set_flash_config(unsigned bus_width, unsigned chip_interleave);
void flashcfg_setup();
/*
**	Intel Function Prototypes and Constant Definitions
*/

#define	INTEL_READ_ARRAY		0xFF
#define INTEL_READ_IDENT		0x90
#define INTEL_READ_QUERY		0x98
#define INTEL_READ_STATUS		0x70
#define INTEL_CLEAR_STATUS		0x50
#define INTEL_WRITE_BUFFER		0xE8
#define INTEL_BUFFER_CONFIRM	0xD0
#define INTEL_WRITE				0x40
#define INTEL_ERASE_BLOCK		0x20
#define INTEL_ERASE_SUSPEND		0xB0
#define	INTEL_ERASE_RESUME		0xD0
#define INTEL_CONFIG			0xB8
#define INTEL_LOCK_CMD			0x60
#define INTEL_LOCK				0x01
#define INTEL_UNLOCK			0xD0
#define INTEL_UNLOCKALL			0xD0

#define F3S_I28F008_POLL  	0x10000000	/* max poll count */
#define F3S_I28F008_ERASE  	~0ULL          /* erase data mask */
#define F3S_I28F008_SHIFT  	3	/* buffer size shift */

_int32 f3s_i28f008_ident(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

void f3s_i28f008_reset(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

_int32 f3s_i28f008_write(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset,
			_int32 size, _uint8 * buffer);

void f3s_i28f008_erase(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset, _int32 size);

_int32 f3s_i28f008_suspend(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

void f3s_i28f008_resume(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

_int32 f3s_i28f008_sync(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset, _int32 size);

_int32 f3s_i28f800_ident(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

_int32 f3s_i28f800_sync(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset, _int32 size);

_int32 f3s_iCFI_ident(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

_int32 f3s_iCFI_write(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset,
			_int32 size, _uint8 * buffer);

int intel_read_status(F3S_BASETYPE status);
int intel_poll(volatile void *memory, unsigned poll_count);

_int32 f3s_i28f008_v2write(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset,
			_int32 size, _uint8 * buffer);

_int32 f3s_iCFI_v2write(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset,
			_int32 size, _uint8 * buffer);

int f3s_iCFI_v2erase(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

int f3s_iCFI_v2suspend(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

int f3s_iCFI_v2resume(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

int f3s_iCFI_v2sync(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

int f3s_iCFI_v2islock(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

int f3s_iCFI_v2lock(f3s_dbase_t *dbase, f3s_access_t *access,
			_uint32 flags, _uint32 offset);

int f3s_iCFI_v2unlock(f3s_dbase_t *dbase, f3s_access_t *access,
			_uint32 flags, _uint32 offset);

int f3s_iCFI_v2unlockall(f3s_dbase_t *dbase, f3s_access_t *access,
			_uint32 flags, _uint32 offset);

int f3s_aMB_v2ssrop(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 op, _uint32 offset, _int32 size,
			_uint8 * buffer);

/*
**	AMD Function Prototypes and Constant Definitions
*/

#define AMD_UNLOCK_CMD1 	0xAA
#define AMD_UNLOCK_CMD2		0x55
#define AMD_CMD_ADDR1_W8	0xAAA
#define AMD_CMD_ADDR2_W8	0x555
#define AMD_CMD_ADDR1_W16	0x555
#define AMD_CMD_ADDR2_W16	0x2AA
#define AMD_AUTOSELECT		0x90
#define AMD_SECTOR_ERASE	0x80
#define	AMD_ERASE_CONFIRM	0x30
#define AMD_PROGRAM			0xA0
#define AMD_UNLOCK_BYPASS	0x20
#define AMD_CFI_QUERY		0x98
#define AMD_CFI_ADDR		0x55
#define AMD_BYPASS_RESET1 	0x90
#define AMD_BYPASS_RESET2 	0x00
#define AMD_READ_MODE		0xf0
#define AMD_ERASE_SUSPEND	0xB0
#define AMD_ERASE_RESUME	0x30
#define AMD_WRITE_BUFFER	0x25
#define AMD_BUFFER_CONFIRM	0x29
#define AMD_DYB_ENTER		0xE0
#define AMD_DYB_SET			0x00
#define AMD_DYB_CLEAR		0x01
#define AMD_PPB_ENTER		0xC0
#define AMD_PPB_SET			0x00
#define AMD_PPB_CLEAR		0x80
#define AMD_PPB_CLEAR_CONFIRM 0x30
#define AMD_PROTECT_EXIT1	0x90
#define AMD_PROTECT_EXIT2	0x00
#define AMD_SECSI_ENTER		0x88
#define AMD_LOCK_REG_ENTER  0xE0
#define AMD_STATUS_READ		0x70
#define AMD_STATUS_CLEAR	0x71

extern _uint32 amd_command_mask;

#define F3S_A29F040_POLL	0x10000000	/* max poll count */
#define F3S_A29F040_CLEAR	~0ULL	/* erased state */

_int32 f3s_a29f004_ident(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

_int32 f3s_a29f004_sync(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset, _int32 size);

_int32 f3s_a29f040_ident(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

void f3s_a29f040_reset(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

_int32 f3s_a29f040_write(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset,
			_int32 size, _uint8 * buffer);

void f3s_a29f040_erase(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset, _int32 size);

_int32 f3s_a29f040_suspend(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

void f3s_a29f040_resume(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

_int32 f3s_a29f040_sync(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset, _int32 size);

_int32 f3s_a29f100_ident(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

_int32 f3s_a29f100_write(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset,
			_int32 size, _uint8 * buffer);

void f3s_a29f100_erase(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset, _int32 size);

_int32 f3s_a29f100_sync(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset, _int32 size);

_int32 f3s_aCFI_ident(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

_int32 f3s_aCFIsm_ident(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

_int32 f3s_aCFI_write(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset,
			_int32 size, _uint8 * buffer);

_int32 f3s_aCFI_suspend(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

int f3s_a29f040_v2erase(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

int f3s_a29f040_v2suspend(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

int f3s_a29f040_v2resume(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

int f3s_a29f040_v2sync(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

_int32 f3s_a29f040_v2write(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset,
			_int32 size, _uint8 * buffer);

int f3s_a29f100_v2erase(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

_int32 f3s_a29f100_v2write(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset,
			_int32 size, _uint8 * buffer);

_int32 f3s_aCFI_v2write(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset,
			_int32 size, _uint8 * buffer);

int f3s_aCFI_v2suspend(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

int f3s_aCFI_v2islock(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

int f3s_aCFI_v2lock(f3s_dbase_t *dbase, f3s_access_t *access,
			_uint32 flags, _uint32 offset);

int f3s_aCFI_v2dlock(f3s_dbase_t *dbase, f3s_access_t *access,
			_uint32 flags, _uint32 offset);

int f3s_aCFI_v2plock(f3s_dbase_t *dbase, f3s_access_t *access,
			_uint32 flags, _uint32 offset);

int f3s_aCFI_v2unlock(f3s_dbase_t *dbase, f3s_access_t *access,
			_uint32 flags, _uint32 offset);

int f3s_aCFI_v2unlockall(f3s_dbase_t *dbase, f3s_access_t *access,
			_uint32 flags, _uint32 offset);

int amd_poll(intunion_t *value, volatile void *memory, int dq1);

_int32 amd_v2wordwrite(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset,
			_int32 size, _uint8 * buffer,
			_uintptr amd_cmd1, _uintptr amd_cmd2);

_int32 f3s_aMB_ident(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

void f3s_aMB_erase(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset, _int32 size);

_int32 f3s_aMB_sync(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset, _int32 size);

void f3s_aMB_resume(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

int f3s_aMB_v2erase(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

int f3s_aMB_v2resume(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

int f3s_aMB_v2sync(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

/*
**	Spansion Function Prototypes and Constant Definitions
*/

#define F3S_INTERLEAVE 2
/*#define F3S_S29GLXXXS_DQPOLL*/

int32_t f3s_s29glxxxs_ident(f3s_dbase_t * dbase, f3s_access_t * access,
                            uint32_t flags, uint32_t offset);

void f3s_s29glxxxs_reset(f3s_dbase_t *dbase, f3s_access_t *access,
                         uint32_t flags, uint32_t offset);

int32_t f3s_s29glxxxs_v2write(f3s_dbase_t * dbase, f3s_access_t * access,
                              uint32_t flags, uint32_t offset,
                              int32_t size, uint8_t * buffer);

int f3s_s29glxxxs_v2erase(f3s_dbase_t * dbase, f3s_access_t * access,
                          uint32_t flags, uint32_t offset);

int f3s_s29glxxxs_v2suspend(f3s_dbase_t *dbase, f3s_access_t *access,
                            uint32_t flags, uint32_t offset);

int f3s_s29glxxxs_v2resume(f3s_dbase_t *dbase, f3s_access_t *access,
                           uint32_t flags, uint32_t offset);

int f3s_s29glxxxs_v2sync(f3s_dbase_t *dbase, f3s_access_t *access,
                         uint32_t flags, uint32_t offset);

int f3s_s29glxxxs_v2read (f3s_dbase_t *dbase, f3s_access_t *access,
                        uint32_t flags, uint32_t offset, int32_t size,
                        uint8_t *buffer);

/*
**	Sharp Function Prototypes and Constant Definitions
*/

#define F3S_S28F008_POLL	0x10000000	/* max poll count */
#define F3S_S28F008_ERASE	~0ULL	/* erase data mask */

_int32 f3s_s28f008_ident(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

/*
**	Fujitsu Function Prototypes and Constant Definitions
*/

_int32 f3s_f29f100_ident(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

_int32 f3s_f29f040_ident(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

/*
**	Hynix Function Prototypes and Constant Definitions
*/
_int32 f3s_hyCFI_ident(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

/*
**	Numonyx Function Prototypes and Constant Definitions
*/
_int32 f3s_nuCFI_ident(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

int f3s_nuCFI_v2suspend(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

int f3s_nuCFI_v2sync(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

/*
**	Macronix Function Prototypes and Constant Definitions
*/
int f3s_mx29f040_v2erase(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

/*
**	Sram Function Prototypes and Constant Definitions
*/

extern _uint32 fail_flag, fail_type, fail_count, fail_action;
extern _uint32 write_count, erase_count;

#define F3S_SRAM_MULT   0x01010101	/* command multiplyer */
#define F3S_SRAM_TYPE   volatile _uint32	/* pointer type caster */

_int32 f3s_sram_ident(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

_int32 f3s_sram_write(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset,
			_int32 size, _uint8 * buffer);

void f3s_sram_erase(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset, _int32 size);

_int32 f3s_sram_sync(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset, _int32 size);

void f3s_sram_reset(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

_int32 f3s_sram_v2write(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset,
			_int32 size, _uint8 * buffer);

int f3s_sram_v2erase(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

int f3s_sram_v2sync(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

int f3s_sram_v2islock(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

int f3s_sram_v2lock(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

int f3s_sram_v2unlock(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

int f3s_sram_v2unlockall(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

/*
**	Rom Function Prototypes and Constant Definitions
*/

#define F3S_ROM_MULT  0x01010101/* command multiplyer */
#define F3S_ROM_TYPE  volatile _uint32	/* pointer type caster */

_int32 f3s_rom_ident(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

_int32 f3s_rom_write(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset,
			_int32 size, _uint8 * buffer);

void f3s_rom_erase(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset, _int32 size);

_int32 f3s_rom_sync(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset, _int32 size);

void f3s_rom_reset(f3s_dbase_t * dbase, f3s_access_t * access,
			_uint32 flags, _uint32 offset);

#endif



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/public/sys/f3s_mtd.h $ $Rev: 736616 $")
#endif
