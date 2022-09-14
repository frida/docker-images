/*
 * $QNXLicenseC:
 * Copyright 2009, QNX Software Systems.
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

#include "sdma.h"


////////////////////////////////////////////////////////////////////////////////
//                            PRIVATE FUNCTIONS                               //
////////////////////////////////////////////////////////////////////////////////


static void shared_mem_init( sdma_shmem_t * shmem_ptr ) {
    pthread_mutexattr_t mutex_attr;

    // Initialized multi-process mutexes
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr,PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(shmem_ptr->libinit_mutex),&mutex_attr);
    pthread_mutex_init(&(shmem_ptr->command_mutex),&mutex_attr);
    pthread_mutex_init(&(shmem_ptr->register_mutex),&mutex_attr);

    // init other state variables
    shmem_ptr->process_cnt = 0;
}

static sdma_shmem_t * shared_mem_create() {
    int fd;
    int status;
    sdma_shmem_t * shmem_ptr;
    off64_t paddr64;

    fd = shm_open("/SDMA_MUTEX",O_RDWR | O_CREAT | O_EXCL, 0666);

    if (fd >= 0) {
        //size the newly allocated memory
        status = ftruncate(fd,sizeof(sdma_shmem_t));
        if (status == -1) {
            perror("shared_mem_create() ftruncate failed\n");
            goto fail2;
        }

        //map it to our control structure
        // Note: CCB structre in this shared memory region need the physically contiguous memory,
        // Once the allcated shared memory size large than 4K Bytes, it is not guaranteed
        // to get the physically contiguous memory by using mmap with fd as below.
        // So please make sure the structure size is less than 4K Bytes.
        shmem_ptr = mmap(    0,
                             sizeof(sdma_shmem_t),
                             PROT_READ|PROT_WRITE,
                             MAP_SHARED,
                             fd,
                             0          );
        if (shmem_ptr == MAP_FAILED) {
            perror("shared_mem_create() Couldn't mmap shared memory\n");
            goto fail2;
        }

        //get the physical memory address of the CCB structure
        status = mem_offset64((void*)shmem_ptr->ccb_arr,
                             NOFD,
                             SDMA_N_CH * sizeof(sdma_ccb_t),
                             &paddr64,
                             0 );
        if (status != 0) {
            perror("shared_mem_create() Couldn't get physical address of shared memory\n");
            goto fail3;
        }

        //initial sdma shmem structure
        memset((void*)shmem_ptr, 0, sizeof(sdma_shmem_t));

        //save the physical memory address of the CCB structure
        shmem_ptr->paddr64 = paddr64;

    } else {
        // Couldn't create shared memory because it either already exists,
        // or for some other reason... it doesn't matter at this point, because
        // sdmasync_init() will try and open the shared memory object if it exists.
        goto fail1;
    }

    close(fd);
    return shmem_ptr;

fail3:
	munmap((void*)shmem_ptr, sizeof(sdma_shmem_t));

fail2:
    close(fd);

fail1:
    return NULL;
}



////////////////////////////////////////////////////////////////////////////////
//                            PUBLIC FUNCTIONS                                //
////////////////////////////////////////////////////////////////////////////////

void ctor(void) __attribute__((__constructor__));
void dtor(void) __attribute__((__destructor__));

void ctor(void) {
     rsrc_alloc_t    ralloc;
     sdma_shmem_t    *shmem_ptr;

    shmem_ptr = shared_mem_create();
    if ( shmem_ptr ) {
        // initialize shared memory on creation
        shared_mem_init( shmem_ptr );

        // seed the resource db manager
        memset(&ralloc, 0, sizeof(ralloc));
        ralloc.start    = SDMA_CH_LO;
        ralloc.end      = SDMA_CH_HI;    
        ralloc.flags    = RSRCDBMGR_DMA_CHANNEL | RSRCDBMGR_FLAG_NOREMOVE; 
        if ( rsrcdbmgr_create(&ralloc, 1) != EOK ) {
            perror("SDMA ctor() Unable to seed dma channels\n");
        }
    }
}

void dtor(void) {
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/dma/sdma/init.c $ $Rev: 725865 $")
#endif
