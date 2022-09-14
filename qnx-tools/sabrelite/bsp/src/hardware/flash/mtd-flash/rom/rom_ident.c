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





#include <sys/f3s_mtd.h>

/*
 * Summary
 *
 * Note: Unsupported
 *
 * Description
 *
 * This is a dummy driver for reading ROM devices. It is depricated and should
 * not be used.
 */

int32_t f3s_rom_ident(f3s_dbase_t *dbase,
                      f3s_access_t *access,
                      uint32_t flags,
                      uint32_t offset)
{
  uint8_t *memory;
  static f3s_dbase_t virtual=
  {sizeof(f3s_dbase_t), 0xffff, 0x00, 0x00, "PROM",
   0, 1, 1, 10000, 1000000000, 50000, 50000, 0, 0, 0, 0, 0, 1, {{1, 16}}};

  /* check listing flag */

  if (flags&F3S_LIST_ALL)
  {
    /* copy virtual database entry */
    *dbase=virtual;

    /* return right here */
    return EOK;
  }

  /* set proper page on socket */
  memory=access->service->page(&access->socket, F3S_POWER_VCC, offset, NULL);
  if (!memory) return ERANGE;

  /* make sure previous idents were reset */
  *(F3S_ROM_TYPE *) memory=0xf0u*F3S_ROM_MULT;
  *(F3S_ROM_TYPE *) memory=0xffu*F3S_ROM_MULT;

  /* copy virtual database entry */

  *dbase=virtual;

  /* memory type is supported */
  return EOK;
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/rom/rom_ident.c $ $Rev: 710521 $")
#endif
