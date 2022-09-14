/*
 * $QNXLicenseC:
 * Copyright 2007, QNX Software Systems. All Rights Reserved.
 *
 * You must obtain a written license from and pay applicable 
 * license fees to QNX Software Systems before you may reproduce, 
 * modify or distribute this software, or any work that includes 
 * all or part of this software.   Free development licenses are 
 * available for evaluation and non-commercial purposes.  For more 
 * information visit http://licensing.qnx.com or email 
 * licensing@qnx.com.
 * 
 * This file may contain contributions from others.  Please review 
 * this entire file for other proprietary rights or license notices, 
 * as well as the QNX Development Suite License Guide at 
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

/*
 *  dcmd_sim_eide.h   Non-portable low-level devctl definitions
 *
*/

#ifndef __DCMD_SIM_EIDE_H_INCLUDED
#define __DCMD_SIM_EIDE_H_INCLUDED

#ifndef _DEVCTL_H_INCLUDED
 #include <devctl.h>
#endif

#ifndef __DCMD_CAM_H_INCLUDED
 #include <sys/dcmd_cam.h>
#endif

#include <_pack64.h>

#define EIDE_APM_LEVEL				0x00     /* Set apm level */
#define EIDE_APM_CPM				0x01     /* Check Power Mode */
	#define EIDE_CPM_STANDBY		0x00     /* drive in standby (returned) */
	#define EIDE_CPM_STANDBY_MEDIA	0x01     /* drive in standby media (qnx specific) */
	#define EIDE_CPM_IDLE			0x80     /* drive idle (returned) */
	#define EIDE_CPM_ACTIVE			0xff     /* drive active (returned) */
	#define EIDE_CPM_IDLE_ACTIVE	0xff     /* drive idle or active (returned) */
#define EIDE_APM_MODE				0x02     /* Set apm mode */
	#define EIDE_MODE_ACTIVE		0x01
	#define EIDE_MODE_IDLE			0x02
	#define EIDE_MODE_STANDBY		0x03
	#define EIDE_MODE_SLEEP			0x05
	#define EIDE_MODE_STANDBY_MEDIA	0x80     /* Standby media (fail media access) */
#define EIDE_APM_STANDBY_INTERVAL	0x03     /* Set apm standby interval */
   /* level         timeout period */
   /* 0           timeout disabled */
   /* 1-240       ( value * 5 ) seconds */
   /* 241-251     ( ( value - 240 ) * 30 )  minutes */
   /* 252         21 minutes */
   /* 253         8 to 12 hours */
   /* 254         reserved */
   /* 255         21 minutes 15 seconds */
   /* Note:  times are approximate ) */

typedef struct _eide_apm {
	_Uint32t		action;
	_Uint32t		level;
} eide_apm_t;

#define EIDE_NOTIFY_ACTION_REMOVE		0x00
#define EIDE_NOTIFY_ACTION_ADD			0x01

#define EIDE_NOTIFY_TYPE_MEDIUM_ERROR	0x00
typedef struct _eide_notify {
	_Uint32t		action;
	_Uint32t		type;
	struct sigevent	sigev;
} eide_notify_t;

#define EIDE_DEVICE_STATE_DISABLE       0
#define EIDE_DEVICE_STATE_ENABLE        1
typedef struct _eide_device_state {
	_Uint32t		state;
} eide_device_state_t;

#define DCMD_SIM_EIDE_APM                __DIOTF(_DCMD_CAM, _SIM_EIDE + 1, struct _eide_apm)
#define DCMD_SIM_EIDE_NOTIFY             __DIOT(_DCMD_CAM, _SIM_EIDE + 2, struct _eide_notify)
#define DCMD_SIM_EIDE_DEVICE_STATE       __DIOT(_DCMD_CAM, _SIM_EIDE + 3, struct _eide_device_state)

#include <_packpop.h>

#endif



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/devb/eide/public/hw/dcmd_sim_eide.h $ $Rev: 711024 $")
#endif
