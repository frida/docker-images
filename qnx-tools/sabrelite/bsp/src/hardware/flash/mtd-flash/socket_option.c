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


/*
** File: f3s_socket_option.c
**
** Description:
**
** This file contains the socket command line option parser
**
** Ident: $Id: socket_option.c 719067 2013-09-04 19:25:16Z jgao@qnx.com $
*/

/*
** Includes
*/
#include <sys/f3s_mtd.h>

#include <libc.h>

/*
** Function: f3s_option_parse
*/
extern f3s_flashcfg_t  flashcfg;

static uint64_t 
f3s_option_parse(char **string)
{
	int             base;
	uint64_t        result;

	/* check for space separator */
	if (**string == ' ')
		(*string)++;

	/* check if string is empty */

	if (**string == 0)
		return 0;

	/* check base of string */
	if (**string == '0')
	{
		/* increment string pointer */
		(*string)++;

		/* base could be 8 */
		base = 8;

		/* check if base is 16 */
		if (**string == 'x' || **string == 'X')
		{
			/* increment string pointer */
			(*string)++;

			/* base is 16 */
			base = 16;
		}
	}
	else
	{
		/* base is 10 */
		base = 10;
	}

	/* read number */
	result = strtoull(*string, string, base);

	/* check multiplier */
	if ((**string == 'b') || (**string == 'B'))
		(*string)++;

	else if ((**string == 'k') || (**string == 'K'))
	{
		(*string)++;
		result *= 1024;
	}
	else if ((**string == 'm') || (**string == 'M'))
	{
		(*string)++;
		result *= 1024 * 1024;
	}
	else if ((**string == 'g') || (**string == 'G'))
	{
		(*string)++;
		result *= 1024 * 1024 * 1024;
	}

	/* check for comma separator */
	if (**string == ',')
		(*string)++;

	/* return result */
	return result;
}

/*
** Function: f3s_socket_option
*/

int 
f3s_socket_option(f3s_socket_t * socket)
{
	char           *string;
	_uint32        check_pow2;
	int i;

	/* check if there is no socket option */

	if (!socket->option)

		/* return error result */

		return ENOENT;

	/* set string pointer */
	string = (char *)socket->option;

	if (*string == 'm' || *string == 'M') {
		string++;
		socket->ncont_window_num = f3s_option_parse(&string);
		if (verbose > 2) {
			printf("(devf %s:%d) Non-contiguous memory windows %d\n",
				__func__, __LINE__, socket->ncont_window_num);
		}
		if(socket->ncont_window_num > 64) {
			return ENOTSUP;
		}
		socket->ncont_windows = malloc(socket->ncont_window_num * sizeof(f3s_noncontiguous_t));
		if(socket->ncont_windows == NULL)
			return ENOMEM;
	}

	/* parse socket address */
	socket->address = f3s_option_parse(&string);

	/* parse socket size */
	socket->window_size = f3s_option_parse(&string);

	/* get address and size of non-contiguous windows */
	for (i = 0; i < socket->ncont_window_num; i++) {
		socket->ncont_windows[i].address = f3s_option_parse(&string);
		if (verbose > 2)
			printf("(devf %s:%d) Non-contiguous window address 0x%llx \n", 
				__func__, __LINE__, socket->ncont_windows[i].address); 
	}

	/* parse array offset */
	socket->array_offset = f3s_option_parse(&string);

	/* parse array size */
	socket->array_size = f3s_option_parse(&string);

	/* parse unit size */
	socket->unit_size = f3s_option_parse(&string);

	if ( (flashcfg.bus_width == 0) && (flashcfg.chip_inter == 0) ){
		/* parse bus width and interleave */
		flashcfg.bus_width = f3s_option_parse(&string);
		flashcfg.chip_inter = f3s_option_parse(&string);
	}

	/* parse programing timeout value */
	flashcfg.program_timeout = f3s_option_parse(&string);

	/* parse erase_suspend timeout value */
	flashcfg.erase_susp_timeout = f3s_option_parse(&string);

	/* parse board options */
	flashcfg.options = f3s_option_parse(&string);

	/* find power of two of unit size */
	if (socket->unit_size)
	{
		check_pow2 = 1;
		while (((1 << check_pow2) != socket->unit_size) && (check_pow2 < 32))
			check_pow2++;

		/* check if unit size is not a power of two */
		if (check_pow2 >= 32)
		{
			errno = EINVAL;
			perror("mtd-flash: socket unit size must be a power of two");
			exit(errno);
		}
	}

	/* everything is fine */
	return EOK;
}

/*
** End
*/



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/flash/mtd-flash/socket_option.c $ $Rev: 719067 $")
#endif
