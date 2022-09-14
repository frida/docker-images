/*
 * $QNXLicenseC:
 * Copyright 2008,2009, QNX Software Systems.
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
 * The purpose of this module is to control the access to shared variables
 * to shared mutexes that are used to implement
 * synchronization control for multiple instances of the dma library.
 * The 'mx35_dma_cfg' utility must be called before the dmalib is initialized
 * in order to generate the shared resources.
 */

#include "sdma.h"

// local variables
static int fd;
static sdma_shmem_t * shmem_ptr;

////////////////////////////////////////////////////////////////////////////////
//                              PUBLIC FUNCTIONS                              //
////////////////////////////////////////////////////////////////////////////////

// This function opens and maps shared memory that was created by the
// 'mx35_dma_cfg' utility

int sdmasync_init(void) {

    fd = shm_open("/SDMA_MUTEX",O_RDWR , 0666);
    if (fd == -1) {
        goto fail1;
    }

    //map it to our control structure
    shmem_ptr = mmap(    0,
                         sizeof(sdma_shmem_t),
                         PROT_READ|PROT_WRITE,
                         MAP_SHARED,
                         fd,
                         0          );
    if (shmem_ptr == MAP_FAILED) {
        goto fail2;
    }

    return 0;
fail2:
    close(fd);
fail1:
    return -1;
}


void sdmasync_fini(void) {
    munmap(shmem_ptr,sizeof(sdma_shmem_t));
    close(fd);
}

// Shared-memory variable access functions

pthread_mutex_t * sdmasync_cmdmutex_get() {
    return &(shmem_ptr->command_mutex);
}

pthread_mutex_t * sdmasync_libinit_mutex_get() {
    return &(shmem_ptr->libinit_mutex);
}

pthread_mutex_t * sdmasync_regmutex_get() {
    return &(shmem_ptr->register_mutex);
}


int sdmasync_is_first_process() {
    if (shmem_ptr->process_cnt == 1)
        return 1;
    else
        return 0;
}

int sdmasync_is_last_process() {
    if (shmem_ptr->process_cnt == 0)
        return 1;
    else
        return 0;
}

void sdmasync_process_cnt_incr() {
    shmem_ptr->process_cnt++;
}

void sdmasync_process_cnt_decr() {
    shmem_ptr->process_cnt--;
}

off64_t sdmasync_ccb_paddr_get() {
    return shmem_ptr->paddr64;
}

sdma_ccb_t * sdmasync_ccb_ptr_get() {
	return shmem_ptr->ccb_arr;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/dma/sdma/sync.c $ $Rev: 725865 $")
#endif
