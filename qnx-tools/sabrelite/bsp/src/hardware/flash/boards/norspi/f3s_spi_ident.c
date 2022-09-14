/*
 * $QNXLicenseC:
 * Copyright 2010, QNX Software Systems. 
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


#include "f3s_spi.h"

/*
 * This is the ident callout for SPI serial NOR flash.
 */

int32_t f3s_spi_ident(f3s_dbase_t *dbase,
                       f3s_access_t *access,
                       uint32_t flags,
                       uint32_t offset)
{
	static f3s_dbase_t *probe;
	static f3s_dbase_t virtual[]=
	{
		{
			sizeof(f3s_dbase_t),   /* size of complete structure with geometries */
			0,                     /* status of structure */
			MAN_ID,                /* jedec high byte - manufacturer ID */
			DEVICE_ID,             /* jedec low byte - device ID */
			DEVICE_IDENT,
			0,                     /* flags for capabilities */
			0,                     /* interleave for chips on bus */
			0,                     /* width of chip */
			640000U,               /* typical write time for cell (ns) */
			600000000U,            /* typical erase time for unit (ns) */
			0,                     /* read mode voltage needed */
			0,                     /* program mode voltage needed */
			0,                     /* number of erase cycles */
			0,                     /* poll count timeout */
			0,                     /* depth of erase queue per chip */
			0,                     /* number of write buffers per chip */
			0,                     /* size of write buffers */
			1,                     /* number of geometries in vector */
			{{NUM_ERASE_UNITS, PWR2_SIZE_UNIT}}             /* number of erase units for geometry; power 2 size of a unit */
		},
		{0, 0xffff, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	};

	int mid;        /* manufacturer id */
	int did;        /* device id */
	uint32_t size;  /* chip total size in bytes */
	uint32_t u_size;

	/* check listing flag */
	if (flags & F3S_LIST_ALL)
	{
		/* check if first pass */
		if (!probe) probe = virtual;
		if (!probe->struct_size) return (ENOENT);

		*dbase = *probe;
		probe++;
		return (EOK);
	}

	/* copy virtual database entry */
	*dbase=virtual[0];

	pd_release((int)access->socket.socket_handle);
	// flash device spec calls for a 30 us delay after 
	// release from power down. For simplicity, we just sleep 1 ms
	delay(1);
	read_ident((int)access->socket.socket_handle, &mid, &did, &size);

	u_size = 1 << dbase->geo_vect[0].unit_pow2;

	if (mid != dbase->jedec_hi || did != dbase->jedec_lo || u_size * dbase->geo_vect[0].unit_num != size ) {
		return (ENOENT);
	}

	access->socket.unit_size = u_size;
	return (EOK);
}


__SRCVERSION( "$URL$ $Rev$" )
