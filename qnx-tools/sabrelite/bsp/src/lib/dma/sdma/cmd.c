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



#include "sdma.h"

/*
 * command channel configs
 *
 * The command channel MUST use channel 0.
 * which should have the highest priority eg. 5-7, and higher than non realtime
 * xfers which should have low priority eg. 1-3
 */
#define SDMA_CMD_CH                         0
#define SDMA_CMD_CH_PRIO                    4
#define SDMA_CMD_COMPLETE_PULSE_CODE        1
#define SDMA_CMD_TIMOUT_NS                  1000000000  // 1sec

/////////////////
// global vars //
/////////////////
extern uintptr_t sdma_base;
extern volatile sdma_ccb_t * ccb_ptr;
extern uint32_t sdram_base;
extern struct cache_ctrl    cinfo;

////////////////
// local vars //
////////////////

static sdma_bd_t * cmd_bd_ptr;
static int cmd_chid;
static int cmd_coid;
static struct sigevent cmd_complete_event;
static sdma_scriptinfo_t scriptinfo;

///////////////////////////////////////////////////////////////////////////////
//                            PRIVATE FUNCTIONS                              //
///////////////////////////////////////////////////////////////////////////////

// Load a SDMA script into SDMA private context memory (experimental)
int sdmaram_script_load() {
    uint16_t        *script;
    int             status;
    off64_t         script_paddr;
    struct _pulse   pulse;
    int             retval          = -1;
    uint64_t        timeout         = 2 * SDMA_CMD_TIMOUT_NS;

    // map physical memory for holding SDMA script code
    script = mmap(   0,
                     scriptinfo.ram_microcode_info.size,
                     PROT_READ|PROT_WRITE|PROT_NOCACHE,
                     MAP_PHYS|MAP_ANON,
                     NOFD,
                     0   );

    if (script == MAP_FAILED) {
        goto fail1;
    }

    // get the physical address for use in the transfer
    status = mem_offset64(   script,
                             NOFD,
                             scriptinfo.ram_microcode_info.size,
                             &script_paddr,
                             0  );
    if (status != 0) {
        goto fail2;
    }

    // copy SDMA script to memory we have provisioned
    memcpy( script, scriptinfo.ram_microcode_info.p , scriptinfo.ram_microcode_info.size );

    // we are the first process so there isn't any chance of
    // multiple threads here, but go through mutex anyway to
    // maintain uniformity
    pthread_mutex_lock( sdmasync_cmdmutex_get() );

    // attach notificatoin event to the interrupt
    sdmairq_event_add(SDMA_CMD_CH, &cmd_complete_event);

    // set command buffer descriptor
    cmd_bd_ptr[0].cmd_and_status = 0;
    cmd_bd_ptr[0].cmd_and_status |= SDMA_CMD_C0_SET_PM;
    cmd_bd_ptr[0].cmd_and_status |= SDMA_CMDSTAT_WRAP_MASK;
    cmd_bd_ptr[0].cmd_and_status |= SDMA_CMDSTAT_DONE_MASK;
    cmd_bd_ptr[0].cmd_and_status |= SDMA_CMDSTAT_INT_MASK;
    cmd_bd_ptr[0].cmd_and_status |= SDMA_CMDSTAT_EXT_MASK;

	// The 'count' is expressed in 16-bit half-words when using SET_PM
    cmd_bd_ptr[0].cmd_and_status |= scriptinfo.ram_microcode_info.size / 2;

    cmd_bd_ptr[0].buf_paddr = script_paddr;
    cmd_bd_ptr[0].ext_buf_paddr = scriptinfo.ram_microcode_info.addr;

    // start command channel
    out32(sdma_base + SDMA_HSTART, 1 << SDMA_CMD_CH);

    // Wait for Command Completion
    TimerTimeout(CLOCK_REALTIME, _NTO_TIMEOUT_RECEIVE, NULL, &timeout , NULL);
    while (1) {
        if (MsgReceivePulse(cmd_chid, &pulse, sizeof(pulse), NULL) != 0) {
            retval = -1;
            break;
        } else if (pulse.code == SDMA_CMD_COMPLETE_PULSE_CODE) {
            retval = 0;
            break;
        }
    }

    // detach event from the interrupt so we don't receive notifications
    // from other processes using the command channel
    sdmairq_event_remove(SDMA_CMD_CH);

    pthread_mutex_unlock( sdmasync_cmdmutex_get() );

    return retval;

fail2:
    munmap(cmd_bd_ptr,sizeof(sdma_bd_t));
fail1:
    return -1;
}


static int script_lookup( void ) {
    int status = EOK;

    memset( &scriptinfo, 0, sizeof( sdma_scriptinfo_t ) );

    // SoC specific lookup
    status = sdmascript_lookup( &scriptinfo );
    if ( status ) {
        return status;
    }

    return EOK;
}



///////////////////////////////////////////////////////////////////////////////
//                            PUBLIC FUNCTIONS                               //
///////////////////////////////////////////////////////////////////////////////

// Create the command channel descriptor buffer, and associate it with the
// command control block.  The command channel lives at channel 0 and is
// used to transfer data to/from private SDMA memory.  Currently, the command
// channel is only used to write contexts, but could be used for other purposes.
int sdmacmd_cmdch_create() {
    off64_t     paddr64;
    uint64_t    physical;
    int         status;

    if ( sdmasync_is_first_process() ) {
        // command channel not yet configured, so create descriptor and
        // init registers

        cmd_bd_ptr = mmap(     0,
                                sizeof(sdma_bd_t),
                                PROT_READ|PROT_WRITE|PROT_NOCACHE,
                                MAP_PHYS|MAP_ANON,
                                NOFD,
                                0   );

        if (cmd_bd_ptr == MAP_FAILED) {
            goto fail1;
        }

        status = mem_offset64(   cmd_bd_ptr,
                                 NOFD,
                                 sizeof(sdma_bd_t),
                                 &paddr64,
                                 0  );
        if (status != 0) {
            goto fail2;
        }

        memset(cmd_bd_ptr,0,sizeof(sdma_bd_t));

        // associate command channel descriptor with CCB
        ccb_ptr[SDMA_CMD_CH].base_bd_paddr = (uint32_t) paddr64;
        ccb_ptr[SDMA_CMD_CH].current_bd_paddr = (uint32_t) paddr64;

        CACHE_FLUSH(    &cinfo,
                        (void *)&ccb_ptr[SDMA_CMD_CH],
                        in32(sdma_base + SDMA_MC0PTR) + SDMA_CMD_CH * sizeof(sdma_ccb_t),
                        sizeof(sdma_ccb_t)    );

       // setup registers related to the command channel
        out32(sdma_base + SDMA_CHNPRI(SDMA_CMD_CH), SDMA_CMD_CH_PRIO);
        out32(sdma_base + SDMA_EVTOVR,
                in32(sdma_base + SDMA_EVTOVR) | (1 << SDMA_CMD_CH) );
    } else {
        // command channel already configured, so map onto the the 
        // buffer descriptor.
        physical = ccb_ptr[SDMA_CMD_CH].base_bd_paddr;

        cmd_bd_ptr = mmap_device_memory(    NULL,
                                            sizeof(sdma_bd_t),
                                            PROT_READ|PROT_WRITE|PROT_NOCACHE,
                                            0,
                                            physical        );

        if (cmd_bd_ptr == MAP_FAILED) {
            goto fail1;
        }
    }

    // setup notification event for when the command completes
    cmd_chid = ChannelCreate(_NTO_CHF_UNBLOCK | _NTO_CHF_DISCONNECT);
    if (cmd_chid == -1) {
        goto fail2;
    }
    cmd_coid = ConnectAttach(0, 0, cmd_chid, _NTO_SIDE_CHANNEL, 0);
    if (cmd_coid == -1) {
        goto fail3;
    }

    cmd_complete_event.sigev_notify   = SIGEV_PULSE;
    cmd_complete_event.sigev_coid     = cmd_coid;
    cmd_complete_event.sigev_code     = SDMA_CMD_COMPLETE_PULSE_CODE;
    cmd_complete_event.sigev_priority = SIGEV_PULSE_PRIO_INHERIT;

    /* lookup the SDMA scripts for this SoC */
    if ( script_lookup() ) {
        goto fail4;
    }

    if ( sdmasync_is_first_process() && scriptinfo.ram_microcode_info.p ) {
       // download SDMA script to SDMA context RAM (experimental)
       // we do it after setting up event notification so we
       // have a chance to poll for completion of the download
       if ( sdmaram_script_load() != 0 ) {
              goto fail4;
       }
    }

    return 0;
fail4:
    ConnectDetach(cmd_coid);
fail3:
    ChannelDestroy(cmd_chid);
fail2:
    munmap(cmd_bd_ptr,sizeof(sdma_bd_t));
fail1:
    return -1;
}


void sdmacmd_cmdch_destroy() {
    ConnectDetach( cmd_coid );
    ChannelDestroy(cmd_chid);
    munmap(cmd_bd_ptr,sizeof(sdma_bd_t));
}

// This function is used to configure a 'context image' based on the
// channel type.  The necessary image configuration can be found in the
// Freescale SDMA Scripts Library Specification.
void sdmacmd_ctx_config(sdma_chan_t * chan_ptr) {
    sdma_ch_ctx_t * ctx_ptr = (sdma_ch_ctx_t *) chan_ptr->ctx_ptr;

    switch(chan_ptr->ch_type){

    case SDMA_CHTYPE_AP_2_AP :

        ctx_ptr->pc = scriptinfo.script_addr_arr[SDMA_CHTYPE_AP_2_AP];
        ctx_ptr->g_reg[7] = sdram_base; //SDRAM-base
        break;

    case SDMA_CHTYPE_UART_2_MCU:
    case SDMA_CHTYPE_MCU_2_AP:
    case SDMA_CHTYPE_AP_2_MCU:
    case SDMA_CHTYPE_SHP_2_MCU:
    case SDMA_CHTYPE_UARTSH_2_MCU:
    case SDMA_CHTYPE_MCU_2_SHP:
    case SDMA_CHTYPE_MCU_2_SPDIF:
    case SDMA_CHTYPE_SPDIF_2_MCU:
    default:
        ctx_ptr->pc       = scriptinfo.script_addr_arr[chan_ptr->ch_type];

    /*
     * If the SDMA event is 32 or higher we need to use register 0
     * to store the event number, as mentioned in the i.MX
     * Reference Manuals in the "Appendix A: SDMA Scripts" section
     */
    if (chan_ptr->eventnum >= 32)
    {
        ctx_ptr->g_reg[1] = 0;
        ctx_ptr->g_reg[0] = (1 << (chan_ptr->eventnum - 32));
    }
    else
    {
        ctx_ptr->g_reg[0] = 0;
        ctx_ptr->g_reg[1] = (1 << chan_ptr->eventnum);
    }

    ctx_ptr->g_reg[6] = chan_ptr->fifo_paddr;
    ctx_ptr->g_reg[7] = chan_ptr->watermark;
    break;
    }
}

// Load the 'context image' configured by the sdmacmd_ctx_config() func
// into SDMA private context memory.
int sdmacmd_ctx_load(sdma_chan_t * chan_ptr) {
    int             retval;
    struct _pulse   pulse;
    uint64_t        timeout     = SDMA_CMD_TIMOUT_NS;
    unsigned        ch_num      = chan_ptr->ch_num;
    uint32_t        ctx_paddr   = chan_ptr->ctx_paddr;
    struct sched_param param;

    // Only have 1 command channel, so control access with mutex
    pthread_mutex_lock( sdmasync_cmdmutex_get() );

    // Configure event pulse priority to that of the calling thread priority
    pthread_getschedparam (pthread_self (), NULL, &param);
    cmd_complete_event.sigev_priority = param.sched_priority;
    // attach notificatoin event to the interrupt
    sdmairq_event_add(SDMA_CMD_CH, &cmd_complete_event);

    // set command buffer descriptor
    cmd_bd_ptr[0].cmd_and_status = 0;
    cmd_bd_ptr[0].cmd_and_status |= SDMA_CMD_C0_SETCTX(ch_num);
    cmd_bd_ptr[0].cmd_and_status |= SDMA_CMDSTAT_WRAP_MASK;
    cmd_bd_ptr[0].cmd_and_status |= SDMA_CMDSTAT_DONE_MASK;
    cmd_bd_ptr[0].cmd_and_status |= SDMA_CMDSTAT_INT_MASK;
    cmd_bd_ptr[0].cmd_and_status |= SDMA_CTX_WSIZE;     //ctx size

    cmd_bd_ptr[0].buf_paddr = ctx_paddr;

    // start command channel
    out32(sdma_base + SDMA_HSTART, 1 << SDMA_CMD_CH);

    // Wait for Command Completion
    TimerTimeout(CLOCK_REALTIME, _NTO_TIMEOUT_RECEIVE, NULL, &timeout , NULL);
    while (1) {
        if (MsgReceivePulse(cmd_chid, &pulse, sizeof(pulse), NULL) != 0) {
            retval = -1;
            break;
        } else if (pulse.code == SDMA_CMD_COMPLETE_PULSE_CODE) {
            retval = 0;
            break;
        }
    }

    // detach event from the interrupt so we don't receive notifications
    // from other processes using the command channel
    sdmairq_event_remove(SDMA_CMD_CH);

    pthread_mutex_unlock( sdmasync_cmdmutex_get() );
    return retval;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/dma/sdma/cmd.c $ $Rev: 750963 $")
#endif
