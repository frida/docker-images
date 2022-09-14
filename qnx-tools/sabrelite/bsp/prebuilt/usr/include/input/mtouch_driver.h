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
#include <sys/neutrino.h>

#include "input/mtouch_params.h"

#ifndef MTOUCH_DRIVER_H_
#define MTOUCH_DRIVER_H_

/* Multitouch device capabilities. */
#define MTOUCH_CAPABILITIES_CONTACT_ID (1 << 0)
#define MTOUCH_CAPABILITIES_COORDS (1 << 1)
#define MTOUCH_CAPABILITIES_CONTACT_COUNT (1 << 2)
#define MTOUCH_CAPABILITIES_WIDTH (1 << 3)
#define MTOUCH_CAPABILITIES_HEIGHT (1 << 4)
#define MTOUCH_CAPABILITIES_ORIENTATION (1 << 5)
#define MTOUCH_CAPABILITIES_PRESSURE (1 << 6)
#define MTOUCH_CAPABILITIES_RATE_SET (1 << 7)
#define MTOUCH_CAPABILITIES_SEQ_ID (1 << 8)
#define MTOUCH_CAPABILITIES_CONTACT_TYPE (1 << 9)
#define MTOUCH_CAPABILITIES_SELECT (1 << 10)

/* Default values for unsupported features */
#define MTOUCH_DEFAULT_WIDTH 1
#define MTOUCH_DEFAULT_HEIGHT 1
#define MTOUCH_DEFAULT_ORIENTATION 0
#define MTOUCH_DEFAULT_PRESSURE 1

/* Multitouch device flags */
#define MTOUCH_FLAGS_INCONSISTENT_DIGIT_ORDER (1 << 0) /* Devices that report touch data for individual fingers in an incosistent order should set this flag */
#define MTOUCH_FLAGS_INCONSISTENT_CONTACT_IDS (1 << 1) /* Devices that don't have contact ids assigned as a zero based index should set this flag */

/* Multitouch parser special params */
#define MTOUCH_PARSER_FLAG_NONE                       (0)
#define MTOUCH_PARSER_FLAG_CANCEL                     (1<<0)
#define MTOUCH_PARSER_FLAG_FLUSH                      (1<<1)
#define MTOUCH_PARSER_FLAG_LARGE_OBJECT               (1<<2)

#ifdef __QNXNTO__
__BEGIN_DECLS
#else
#ifdef __cplusplus
extern "C" {
#endif
#endif

struct mtouch_device;

/**
 * These functions are to be implemented by the mtouch driver and are called
 * into by libinputevents to fetch the data required to transform the device
 * data into mtouch events.
 *
 * The following functions are mandatory:
 * - get_contact_id()
 * - is_contact_down()
 * - get_coords()
 *
 * packet is the data that was passed to mtouch_driver_process_packet(). Note
 * that it should point to data that can be used to retrieve information about
 * all digits (whether they are touching or not). In other words, you can't
 * call mtouch_driver_process_packet() once for each digit.
 *
 * digit_idx is a zero based index that goes up to (max_touchpoints - 1). There
 * doesn't need to be a correlation between the digit_idx and the contact_id.
 * See the MTOUCH_FLAGS_INCONSISTENT_DIGIT_ORDER and
 * MTOUCH_FLAGS_INCONSISTENT_CONTACT_IDS flags to describe the relationship
 * between digit_idx and contact_id.
 */
typedef struct {
	int (*get_contact_id)(void* packet, _Uint8t digit_idx, _Uint32t* contact_id, void* arg);
	int (*is_contact_down)(void* packet, _Uint8t digit_idx, int* valid, void* arg);
	int (*get_coords)(void* packet, _Uint8t digit_idx, _Int32t* x, _Int32t* y, void* arg);
	int (*get_down_count)(void* packet, _Uint32t* down_count, void* arg);
	int (*get_touch_width)(void* packet, _Uint8t digit_idx, _Uint32t* touch_width, void* arg);
	int (*get_touch_height)(void* packet, _Uint8t digit_idx, _Uint32t* touch_height, void* arg);
	int (*get_touch_orientation)(void* packet, _Uint8t digit_idx, _Uint32t* touch_orientation, void* arg);
	int (*get_touch_pressure)(void* packet, _Uint8t digit_idx, _Uint32t* touch_pressure, void* arg);
	void (*get_seq_id)(void* packet, _Uint32t* seq_id, void* arg);
	int (*get_contact_type)(void* packet, _Uint8t digit_idx, _Uint32t* contact_type, void* arg);
	int (*get_select)(void* packet, _Uint8t digit_idx, _Uint32t* select, void* arg);

	/* The following will be called with the device mutex held */
	int (*set_event_rate)(void* dev, _Uint32t min_event_interval); /* The passed interval is in usecs */
} mtouch_driver_funcs_t;

struct mtouch_device* mtouch_driver_attach(mtouch_driver_params_t* params, mtouch_driver_funcs_t* funcs);
void mtouch_driver_detach(struct mtouch_device* device);
void mtouch_driver_process_packet(struct mtouch_device* device, void* packet, void* arg, unsigned int flags);


enum {
	MTOUCH_FILTER_FLUSH_PULSE_CODE = _PULSE_CODE_MINAVAIL,
	MTOUCH_PULSE_CODE_MINAVAIL
};

void mtouch_driver_set_chid(struct mtouch_device* device, int chid);



/**
 * These functions make up the test interface for an mtouch driver.  The init routine
 * returns a handle that must be passed into the individual test functions.
 * Each test function will return one of the values below.  For the read functions, the
 * first byte in the output buffer is the number of additional bytes in the response.
 */
enum {
    MTOUCH_TEST_RESULT_COMPLETED,     // Test has completed
    MTOUCH_TEST_RESULT_NOT_SUPPORTED, // Test is not supported
    MTOUCH_TEST_RESULT_NOT_NOW,       // Can't run test now because a firmware update, single scan, or BIST is occurring
    MTOUCH_TEST_RESULT_I2C_FAILURE,   // I2C communication with controller failed
    MTOUCH_TEST_RESULT_TIMEOUT        // The controller did not respond in time
};

void* mtouch_test_init(void);
void mtouch_test_fini(void* dev);
int mtouch_test_bist(void* dev, uint8_t* passfail, uint16_t* max, uint16_t* min);
int mtouch_test_read_supplier_id(void* dev, uint8_t** supplier);
int mtouch_test_read_serial_id(void* dev, uint8_t** serial);
int mtouch_test_read_product_id(void *dev, uint8_t** product);
int mtouch_test_read_firmware_version(void* dev, uint8_t** fwv);

#ifdef __QNXNTO__
__END_DECLS
#else
#ifdef __cplusplus
};
#endif
#endif

#endif /* MTOUCH_DRIVER_H_ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/inputevents/public/input/mtouch_driver.h $ $Rev: 724998 $")
#endif
