/*
 * $QNXLicenseC:
 * Copyright 2009,2012 QNX Software Systems.
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
 *
 *    mxssi_dll.c
 *      The primary interface into the mx DLL.
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <hw/inout.h>
#include <sys/asoundlib.h>
#include <devctl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mxssi.h"
int codec_mixer (ado_card_t * card, HW_CONTEXT_T * hwc);
int set_ssi_clock_rate ( HW_CONTEXT_T *mx, int rate);

#define MIN(A, B)              ((A)<(B)?(A):(B))

/**
 * This function returns the number of open or active capture channels
 */
static uint32_t
num_open_capture_subChnl(HW_CONTEXT_T * mx)
{
    uint32_t num; /* number of capture subchn open*/
    uint32_t idx = 0;

    num = 0;
    for(idx=0; idx < mx->cap_subchn; idx++)
    {
        if(NULL != mx->cap_strm[idx].pcm_subchn)
        {
            num++;
        }
    }
    return num;
}

int32_t
mx_capabilities (HW_CONTEXT_T * mx, ado_pcm_t *pcm, snd_pcm_channel_info_t * info)
{
	int   chn_avail = 1;

	if (info->channel == SND_PCM_CHANNEL_PLAYBACK)
	{
		if (pcm == mx->pcm1 && mx->play_strm.pcm_subchn)
			chn_avail = 0;
		else if ((mx->clk_mode & CLK_MODE_MASTER) && mx->sample_rate_min != mx->sample_rate_max)
		{
			ado_mutex_lock(&mx->hw_lock);
			/* Playback and Capture are Rate locked, so adjust rate capabilities
			 * if the other side has been aquried.
			 */
			if (num_open_capture_subChnl(mx) != 0)
			{
				info->min_rate = info->max_rate = mx->sample_rate;
				info->rates = ado_pcm_rate2flag(mx->sample_rate);
			}
			ado_mutex_unlock(&mx->hw_lock);
		}
	}
	else if (info->channel == SND_PCM_CHANNEL_CAPTURE)
	{
		if (pcm == mx->pcm1 && num_open_capture_subChnl(mx) == mx->cap_subchn)
			chn_avail = 0;
		else if (mx->sample_rate_min != mx->sample_rate_max) 
		{
			ado_mutex_lock(&mx->hw_lock);
			/* Playback and Capture are Rate locked, so adjust rate capabilities
			 * if the other side has been aquried.
			 */
			if (mx->play_strm.aquired)
			{
				info->min_rate = info->max_rate = mx->sample_rate;
				info->rates = ado_pcm_rate2flag(mx->sample_rate);
			}
			ado_mutex_unlock(&mx->hw_lock);
		}
	}

	if (chn_avail == 0)
	{
		info->formats = 0;
		info->rates = 0;
		info->min_rate = 0;
		info->max_rate = 0;
		info->min_voices = 0;
		info->max_voices = 0;
		info->min_fragment_size = 0;
		info->max_fragment_size = 0;
	}

	return (0);
}

int32_t
mx_playback_aquire (HW_CONTEXT_T * mx, PCM_SUBCHN_CONTEXT_T ** pc,
    ado_pcm_config_t * config, ado_pcm_subchn_t * subchn, uint32_t * why_failed)
{
    dma_transfer_t tinfo;
    int     frag_idx;

    ado_mutex_lock (&mx->hw_lock);
    if (mx->play_strm.pcm_subchn)
    {
        *why_failed = SND_PCM_PARAMS_NO_CHANNEL;
        ado_mutex_unlock(&mx->hw_lock);
        return (EAGAIN);
    }

	/* If multiple rates supported check for rate switch */
    if ((mx->clk_mode & CLK_MODE_MASTER) && mx->sample_rate_min != mx->sample_rate_max)
    {
        if (config->format.rate != mx->sample_rate && (num_open_capture_subChnl(mx) == 0))
        {
			set_ssi_clock_rate(mx, config->format.rate);
        }
        else if (config->format.rate != mx->sample_rate && (num_open_capture_subChnl(mx) > 0))
        {
            ado_mutex_unlock(&mx->hw_lock);
            return (EBUSY);
        }
    }

    if ((config->dmabuf.addr = ado_shm_alloc (config->dmabuf.size, config->dmabuf.name,
                ADO_SHM_DMA_SAFE, &config->dmabuf.phys_addr)) == NULL)
    {
        ado_mutex_unlock (&mx->hw_lock);
        return (errno);
    }

    memset (&tinfo, 0, sizeof (tinfo));
    tinfo.src_addrs = malloc (config->mode.block.frags_total * sizeof (dma_addr_t));
    for (frag_idx = 0; frag_idx < config->mode.block.frags_total; frag_idx++)
    {
        tinfo.src_addrs[frag_idx].paddr =
            config->dmabuf.phys_addr + (frag_idx * config->mode.block.frag_size);
        tinfo.src_addrs[frag_idx].len = config->mode.block.frag_size;
    }
    tinfo.src_fragments = config->mode.block.frags_total;
    tinfo.xfer_unit_size = mx->sample_size == 2 ?  16 : 32;
    tinfo.xfer_bytes = config->dmabuf.size;

    mx->sdmafuncs.setup_xfer (mx->play_strm.dma_chn, &tinfo);
    free (tinfo.src_addrs);

    mx->play_strm.pcm_subchn = subchn;
    mx->play_strm.pcm_config = config;
    mx->play_strm.aquired = 1;
    *pc = &mx->play_strm;
    ado_mutex_unlock (&mx->hw_lock);
    return (EOK);
}


int32_t
mx_playback_release (HW_CONTEXT_T * mx, PCM_SUBCHN_CONTEXT_T * pc,
    ado_pcm_config_t * config)
{
    ado_mutex_lock (&mx->hw_lock);
    mx->play_strm.pcm_subchn = NULL;
   	mx->play_strm.aquired = 0; 
    ado_shm_free (config->dmabuf.addr, config->dmabuf.size, config->dmabuf.name);
    ado_mutex_unlock (&mx->hw_lock);
    return (0);
}

int32_t
mx_capture_aquire (HW_CONTEXT_T * mx, PCM_SUBCHN_CONTEXT_T ** pc,
    ado_pcm_config_t * config, ado_pcm_subchn_t * subchn, uint32_t * why_failed)
{
    dma_transfer_t tinfo;
    int     frag_idx;

    ado_mutex_lock (&mx->hw_lock);
    if (mx->cap_strm[0].pcm_subchn)
    {
        *why_failed = SND_PCM_PARAMS_NO_CHANNEL;
        ado_mutex_unlock(&mx->hw_lock);
        return (EAGAIN);
    }

	/* If multiple rates supported check for rate switch */
    if ((mx->clk_mode & CLK_MODE_MASTER) && mx->sample_rate_min != mx->sample_rate_max)
    {
        if (config->format.rate != mx->sample_rate && !(mx->play_strm.aquired))
        {
			set_ssi_clock_rate(mx, config->format.rate);
        }
        else if (config->format.rate != mx->sample_rate && mx->play_strm.aquired)
        {
            ado_mutex_unlock(&mx->hw_lock);
            return (EBUSY);
        }
    }

    if ((config->dmabuf.addr = ado_shm_alloc (config->dmabuf.size, config->dmabuf.name,
                ADO_SHM_DMA_SAFE, &config->dmabuf.phys_addr)) == NULL)
    {
        ado_mutex_unlock (&mx->hw_lock);
        return (errno);
    }

    memset (&tinfo, 0, sizeof (tinfo));
    tinfo.dst_addrs = malloc (config->mode.block.frags_total * sizeof (dma_addr_t));
    for (frag_idx = 0; frag_idx < config->mode.block.frags_total; frag_idx++)
    {
        tinfo.dst_addrs[frag_idx].paddr =
            config->dmabuf.phys_addr + (frag_idx * config->mode.block.frag_size);
        tinfo.dst_addrs[frag_idx].len = config->mode.block.frag_size;
    }
    tinfo.dst_fragments = config->mode.block.frags_total;
    tinfo.xfer_unit_size = mx->sample_size == 2 ? 16 : 32;
    tinfo.xfer_bytes = config->dmabuf.size;

    mx->sdmafuncs.setup_xfer (mx->cap_strm[0].dma_chn, &tinfo);
    free (tinfo.dst_addrs);

    mx->cap_strm[0].pcm_subchn = subchn;
    mx->cap_strm[0].pcm_config = config;
    mx->cap_strm[0].aquired = 1;
    *pc = &mx->cap_strm[0];
    ado_mutex_unlock (&mx->hw_lock);
    return (EOK);
}

int32_t
mx_capture_release (HW_CONTEXT_T * mx, PCM_SUBCHN_CONTEXT_T * pc,
    ado_pcm_config_t * config)
{
    ado_mutex_lock (&mx->hw_lock);
    mx->cap_strm[0].pcm_subchn = NULL;
    ado_shm_free (config->dmabuf.addr, config->dmabuf.size, config->dmabuf.name);
    ado_mutex_unlock (&mx->hw_lock);
    return (0);
}

int32_t
mx_prepare (HW_CONTEXT_T * mx, PCM_SUBCHN_CONTEXT_T * pc, ado_pcm_config_t * config)
{
    return (0);
}

int32_t
mx_playback_trigger (HW_CONTEXT_T * mx, PCM_SUBCHN_CONTEXT_T * pc, uint32_t cmd)
{
    int rtn = EOK, i = 0;

    ado_mutex_lock (&mx->hw_lock);

    if (cmd == ADO_PCM_TRIGGER_GO)
    {
        if (pc->pcm_subchn == mx->play_strm.pcm_subchn)
        {
#if !defined (VARIANT_MX6X) && !defined (VARIANT_BT) && !defined (VARIANT_AIC3104)
            mx->ssi->sor |= SOR_TX_CLR;    /* Flush TX FIFO */
#endif

            if(mx->sdmafuncs.xfer_start (mx->play_strm.dma_chn) == -1)
            {
                rtn = errno;
                ado_error ("MX SSI: Audio DMA Start failed (%s)", strerror(rtn));
            }


            /* Give DMA time to fill the FIFO */
            while ((SFCSR_TXFIFO0_CNT(mx->ssi->sfcsr) == 0) && (i<100))
            {
                i++;
                nanospin_ns(1000);
            }

            /* Report an error if DMA didn't fill the FIFO on time, but still keep running */
            if (i>=100)
            {
                rtn = ETIME;
                ado_error ("MX SSI: Audio TX FIFO underrun");
            }

			/* Enable TX */
			mx->ssi->scr |= SCR_TX_EN;

			/* Un-mute codec after serializer is enabled to minimize pop */
			CODEC_PLAYBACK_UNMUTE
        }
    }
    else
    {
        if (pc->pcm_subchn == mx->play_strm.pcm_subchn)
        {
			/* Mute codec before serializer is disabled to minimize pop */
			CODEC_PLAYBACK_MUTE

            if (mx->sdmafuncs.xfer_abort (mx->play_strm.dma_chn) == -1)
            {
                rtn = errno;
                ado_error ("MX SSI: Audio DMA Stop failed (%s)", strerror(rtn));
            }

#if defined (VARIANT_MX6X) || defined (VARIANT_BT) || defined (VARIANT_AIC3104)
			/* Let it empty the Tx FIFO0 */
            while (SFCSR_TXFIFO0_CNT(mx->ssi->sfcsr));

			/* 
			 * Leave time for the last word to be transfered to the STX port 
			 * Assuming the slowest sample rate @ 8K 
             */
			usleep(125);
#endif
			mx->ssi->scr &= ~SCR_TX_EN;
        }
    }
    ado_mutex_unlock (&mx->hw_lock);
    return (rtn);
}

int32_t
mx_capture_trigger (HW_CONTEXT_T * mx, PCM_SUBCHN_CONTEXT_T * pc, uint32_t cmd)
{
    int rtn = EOK;

    ado_mutex_lock (&mx->hw_lock);

    if (cmd == ADO_PCM_TRIGGER_GO)
    {
        if (pc->pcm_subchn == mx->cap_strm[0].pcm_subchn)
        {
#if !defined (VARIANT_MX6X) && !defined (VARIANT_BT) && !defined (VARIANT_AIC3104)
            mx->ssi->sor |= SOR_RX_CLR;    /* Flush RX FIFO */
#endif
			if (SFCSR_RXFIFO0_CNT(mx->ssi->sfcsr) != 0)
				ado_error("MX SSI: AUDIO RX FIFO not empty");

            if (mx->sdmafuncs.xfer_start (mx->cap_strm[0].dma_chn) == -1)
            {
                rtn = errno;
                ado_error ("MX SSI: Audio DMA Start failed (%s)", strerror(rtn));
            }

            mx->ssi->scr |= SCR_RX_EN;    /* Enable RX */
        }
    }
    else
    {
        if (pc->pcm_subchn == mx->cap_strm[0].pcm_subchn)
        {
            mx->ssi->scr &= ~(SCR_RX_EN);

            if (mx->sdmafuncs.xfer_abort (mx->cap_strm[0].dma_chn) == -1)
            {
                rtn = errno;
                ado_error ("MX SSI: Audio DMA Stop failed (%s)", strerror(rtn));
            }

#if defined (VARIANT_MX6X) || defined (VARIANT_BT) || defined (VARIANT_AIC3104)
			uint32_t tmp;
			uint32_t cnt = 0;
			/* Wait until Reciever disable is complete */
			while (!(mx->ssi->sisr & SISR_RFRC) && cnt < 1000)
				cnt++;
			if (cnt > 1000)
				ado_error("MX SSI: Audio RX disable failed - 0x%x", mx->ssi->sisr);

			/* Clear RX FIFO by reading out all data */
			while (SFCSR_RXFIFO0_CNT(mx->ssi->sfcsr) )
				tmp  = mx->ssi->srx0;

			/* Clear Receive frame complete status */
			mx->ssi->sisr |= SISR_RFRC;
#endif
        }
    }

    ado_mutex_unlock (&mx->hw_lock);

    return (rtn);
}

/*
 * No position function as we are unable to get the transfer count of the
 * current DMA operation from the SDMA microcode. The resolution of the
 * positional information returned to the client will be limited to the
 * fragment size.
 *
 * If we get new SDMA microcode that supports this and the dma library's
 * bytes_left() function is updated to use this info to return the actual
 * bytes left, uncomment the below function and the function pointer
 * assignments in ctrl_init().
 */

#if 0
uint32_t
mx_position (HW_CONTEXT_T * mx, PCM_SUBCHN_CONTEXT_T * pc, ado_pcm_config_t * config)
{
    uint32_t pos;

    ado_mutex_lock (&mx->hw_lock);

    if (pc == mx->play_strm.pcm_subchn)
    {
        pos =
            ado_pcm_dma_int_size (config) -
            mx->sdmafuncs.bytes_left (mx->play_strm.dma_chn);
    }
    else
    {
        pos =
            ado_pcm_dma_int_size (config) -
            mx->sdmafuncs.bytes_left (mx->cap_strm[0].dma_chn);
    }

    ado_mutex_unlock (&mx->hw_lock);

    return (pos);
}
#endif

/**
 * This function is used when more than 1 capture inputs have been enabled. It is called from
 * rx interrupt handler (or pulse). It manually copies data from global dma buffer to client
 * dma buffer and triggers notifies io-audio when one complete fragment has been transferred.
 */
static void
subchn_dmacapture(HW_CONTEXT_T *mx, uint8_t *srcDMAAddr, uint32_t size)
{
    int32_t idx               = 0;
    int32_t bytesToTransfer   = 0;
    int32_t bytesTransferred  = 0;
    mx_strm_t *ctx;

    if((NULL == srcDMAAddr) || (0 == size))
    {
        ado_error("mxssi: subchn_dmacapture invalid data or size");
        return;
    }

    for(idx = 0; idx < mx->cap_subchn; idx++)
    {
        ctx = &mx->cap_strm[idx];
        /* 'size' check is provided to restrict copying large DMA data into subchn fragment.
         * When a new subchn is added with smaller frag_size than following 1st DMA
         * read data can be more than what the new subchn can handle */
        if((NULL != ctx->pcm_subchn) && (1 == ctx->go) && (ctx->dma_frag_size >= size))
        {
            bytesTransferred = 0;
            bytesToTransfer  = 0;
            while(bytesTransferred < size)
            {
                /* Make sure we interrupt for each fragment. Below check ensures that we
                 * copy enough bytes to complete one fragment at a time and interrupts io-audio. */
                if(ctx->dma_offset < ctx->dma_frag_size)
                {
                    bytesToTransfer = MIN((ctx->dma_frag_size-ctx->dma_offset), (size - bytesTransferred));
                }
                else
                {
                    bytesToTransfer = MIN((ctx->dma_size-ctx->dma_offset), (size - bytesTransferred));
                }
                memcpy((uint8_t *)(ctx->dma_addr + ctx->dma_offset), &srcDMAAddr[bytesTransferred], bytesToTransfer);
                ctx->dma_offset = (ctx->dma_offset + bytesToTransfer) % ctx->dma_size;
                if ((ctx->dma_offset % ctx->dma_frag_size) == 0)
                {
                    // Signal to io-audio (DMA transfer was completed)
                    dma_interrupt(ctx->pcm_subchn);
                }
                bytesTransferred += bytesToTransfer;
            }
        }
    }
}

/**
 * This function is an alternative to capture_acquire function used when more than 1 capture
 * channels are supported. For easier code readability, it was decided to keep two separate
 * capture acquire functions as opposed to merging this with mx_capture_acquire function.
 */
int32_t
mx_capture_aquire2 (HW_CONTEXT_T * mx, PCM_SUBCHN_CONTEXT_T ** pc,
    ado_pcm_config_t * config, ado_pcm_subchn_t * subchn, uint32_t * why_failed)
{
    int     frag_idx = 0;
    int     idx = 0;
    PCM_SUBCHN_CONTEXT_T *ctx = NULL;

    // check for available sub-channel
    for(idx = 0; idx < mx->cap_subchn; idx++)
    {
        if(NULL == mx->cap_strm[idx].pcm_subchn)
        {
            ctx = &mx->cap_strm[idx];
            break;
        }

    }

    if(NULL == ctx)
    {
        *why_failed = SND_PCM_PARAMS_NO_CHANNEL;
        return (EAGAIN);
    }

    ado_mutex_lock (&mx->hw_lock);
   
	/* If multiple rates supported check for rate switch */
    if ((mx->clk_mode & CLK_MODE_MASTER) && mx->sample_rate_min != mx->sample_rate_max)
    {
        if (config->format.rate != mx->sample_rate && !(mx->play_strm.aquired))
        {
			set_ssi_clock_rate(mx, config->format.rate);
        }
        else if (config->format.rate != mx->sample_rate && mx->play_strm.aquired)
        {
            ado_mutex_unlock(&mx->hw_lock);
            return (EBUSY);
        }
    }

    // Allocate DMA transfer buffer
    if(mx->cap_subchn > 1)
    {
        if((config->dmabuf.addr = ado_shm_alloc(config->dmabuf.size, config->dmabuf.name, ADO_SHM_DMA_SAFE, &config->dmabuf.phys_addr)) == NULL)
        {
            ado_mutex_unlock(&mx->hw_lock);
            return (errno);
        }
    }

    // Set DMA Capture Buffer size to minimum required by any of the clients
    if(config->dmabuf.size < mx->capture_buff_size)
        mx->capture_buff_size_mod = config->dmabuf.size;

    // Set capture DMA only once for subchn capture
    if(0 == num_open_capture_subChnl(mx))
    {
        mx->capture_buff_size = config->dmabuf.size;
        mx->capture_buff_size_mod = mx->capture_buff_size;
        mx->capture_shmem_size = mx->capture_buff_size;
        mx->capture_shmem_name[strlen("/snd1234")] = '\0';

        if((mx->capture_virtual_addr = ado_shm_alloc(mx->capture_buff_size, &mx->capture_shmem_name[0], ADO_SHM_DMA_SAFE, (off64_t*)&mx->capture_phys_addr)) == NULL)
        {
            ado_mutex_unlock((&mx->hw_lock));
            return(errno);
        }

        // Allocate DMA transfer buffer
        if(1 == mx->cap_subchn)
        {
            config->dmabuf.addr = (int8_t*)mx->capture_virtual_addr;
        }

        memset(&mx->tinfo_cap, 0, sizeof(mx->tinfo_cap));

        mx->tinfo_cap.dst_addrs = malloc(config->mode.block.frags_total * sizeof(dma_addr_t));


        for (frag_idx = 0; frag_idx < config->mode.block.frags_total; frag_idx++)
        {
            mx->tinfo_cap.dst_addrs[frag_idx].paddr =
                mx->capture_phys_addr + (frag_idx * config->mode.block.frag_size);
            mx->tinfo_cap.dst_addrs[frag_idx].len = config->mode.block.frag_size;
        }
        mx->tinfo_cap.dst_fragments = config->mode.block.frags_total;
        mx->tinfo_cap.xfer_unit_size = mx->sample_size == 2 ? 16 : 32;
        mx->tinfo_cap.xfer_bytes = config->dmabuf.size;

        mx->sdmafuncs.setup_xfer (mx->cap_strm[0].dma_chn, &mx->tinfo_cap);
    }

    // Populate fields in subchn struct
    ctx->dma_addr = config->dmabuf.addr;
    ctx->dma_size = config->dmabuf.size;
    ctx->dma_frag_size = ado_pcm_dma_int_size(config);
    ctx->dma_offset = 0;
    ctx->pcm_subchn = subchn;
    ctx->aquired = 1;
    *pc = ctx;

    ado_mutex_unlock(&mx->hw_lock);
    return (EOK);
}

/**
 * This function is used as an alternative to capture_release function
 * used when more than 1 capture channels are supported.
 */
int32_t
mx_capture_release2 (HW_CONTEXT_T * mx, PCM_SUBCHN_CONTEXT_T * pc,
    ado_pcm_config_t * config)
{
    ado_mutex_lock (&mx->hw_lock);
    pc->pcm_subchn = NULL;
    pc->aquired = 0;
    if(0 == num_open_capture_subChnl(mx))
    {
        // Free global capture transfer buffer
        ado_shm_free((void*)mx->capture_virtual_addr, mx->capture_shmem_size, mx->capture_shmem_name);
        // free dma transfer buffer
        free(mx->tinfo_cap.dst_addrs);
    }

    // Free DMA transfer buffer of subchn
    if(mx->cap_subchn > 1)
        ado_shm_free(config->dmabuf.addr, config->dmabuf.size, config->dmabuf.name);

    ado_mutex_unlock (&mx->hw_lock);
    return (0);
}
/**
 * This function is an alternative capture_trigger function used when more than 1 capture
 * channels are supported.
 */
int32_t
mx_capture_trigger2 (HW_CONTEXT_T * mx, PCM_SUBCHN_CONTEXT_T * pc, uint32_t cmd)
{
    int rtn = EOK;

    ado_mutex_lock (&mx->hw_lock);

    if (cmd == ADO_PCM_TRIGGER_GO)
    {
        if(1 == num_open_capture_subChnl(mx))
        {
            // This is the first capture subchn and DMA xfer needs to be started
#if !defined (VARIANT_MX6X) && !defined (VARIANT_BT) && !defined (VARIANT_AIC3104)
            mx->ssi->sor |= SOR_RX_CLR;    /* Flush RX FIFO */
#endif

            if (mx->sdmafuncs.xfer_start (mx->cap_strm[0].dma_chn) == -1)
            {
                rtn = errno;
                ado_error ("MX SSI: Audio DMA Start failed (%s)", strerror(rtn));
            }

            mx->ssi->scr |= SCR_RX_EN;    /* Enable RX */
        }
        pc->go = 1;

    }
    else
    {
        if(1 == num_open_capture_subChnl(mx))
        {
            // there is only one capture subchn and dma xfer needs to be stopped
            mx->ssi->scr &= ~(SCR_RX_EN);

            if (mx->sdmafuncs.xfer_abort (mx->cap_strm[0].dma_chn) == -1)
            {
                rtn = errno;
                ado_error ("MX SSI: Audio DMA Stop failed (%s)", strerror(rtn));
            }

#if defined (VARIANT_MX6X) || defined (VARIANT_BT) || defined (VARIANT_AIC3104)
			uint32_t tmp;
			while (SFCSR_RXFIFO0_CNT(mx->ssi->sfcsr) ) { 
				tmp  = mx->ssi->srx0;
			}
#endif
        }
        pc->go = 0;
    }

    ado_mutex_unlock (&mx->hw_lock);

    return (rtn);
}


void
mx_play_pulse_hdlr (HW_CONTEXT_T * mx, struct sigevent *event)
{
    if (mx->play_strm.pcm_subchn)
        dma_interrupt (mx->play_strm.pcm_subchn);
}

/**
 * this pulse handler (for capture) is shared with cases where up to 3 simultaneous
 * capture points is needed.
 */
void
mx_cap_pulse_hdlr (HW_CONTEXT_T * mx, struct sigevent *event)
{
    int32_t  capture_frag_size = mx->capture_buff_size / 2;
    void *dma = mx->cap_strm[0].dma_chn;
    int frag_idx;

    if(1 == mx->cap_subchn)
    {
        if (mx->cap_strm[0].pcm_subchn)
            dma_interrupt (mx->cap_strm[0].pcm_subchn);
    }
    else
    {

        if(1 == mx->frag_index)
        {
			/* Apply any fragment size changes here, since next transfer is from the start of the dma ping-pong buffer */
            if(mx->capture_buff_size_mod != mx->capture_buff_size)
            {
                for (frag_idx = 0; frag_idx < 2; frag_idx++)
                {
                    mx->tinfo_cap.dst_addrs[frag_idx].paddr =
                        mx->capture_phys_addr + (frag_idx * (mx->capture_buff_size_mod/2));
                    mx->tinfo_cap.dst_addrs[frag_idx].len = mx->capture_buff_size_mod/2;
                }
                mx->tinfo_cap.xfer_bytes = mx->capture_buff_size_mod;
            }

            ado_mutex_lock(&mx->hw_lock);
            mx->sdmafuncs.xfer_complete(dma);
            mx->sdmafuncs.setup_xfer(dma, &mx->tinfo_cap);
            mx->sdmafuncs.xfer_start(dma);
            ado_mutex_unlock(&mx->hw_lock);

            mx->frag_index = 0;
            /* process pong buffer */
            subchn_dmacapture(mx, (uint8_t*)(&mx->capture_virtual_addr[capture_frag_size]), capture_frag_size);

            if(mx->capture_buff_size_mod != mx->capture_buff_size)
            {
                mx->capture_buff_size = mx->capture_buff_size_mod;
            }
            return;
        }
        if(0 == mx->frag_index)
        {
            mx->frag_index = 1;
            /* process ping buffer */
            subchn_dmacapture(mx, (uint8_t*)(&mx->capture_virtual_addr[0]), capture_frag_size);
            return;
        }
    }
}
int 
set_ssi_clock_rate ( HW_CONTEXT_T *mx, int rate)
{
	/* Disable SSI */
	mx->ssi->scr &= ~(SCR_SSI_EN | SCR_TX_EN | SCR_RX_EN);
 
	mx->sample_rate = rate;   
	if(mx->clk_mode & CLK_MODE_MASTER)
	{
		int pm;
		uint32_t f_bit_clk;

		if (!(mx->clk_mode & CLK_MODE_NORMAL))
		{
			mx->ssi->scr |= SCR_I2S_MASTER;
			/* In I2S Master mode slot size is fixed at 32 bits, WL control which bits are valid data bits */
			f_bit_clk = mx->sample_rate * mx->voices * 32;
			pm = (mx->sys_clk / f_bit_clk / 2) - 1;
			mx->ssi->stccr = (mx->ssi->stccr & ~STCCR_PM_MSK) | pm;
		}
		else
		{
			mx->ssi->scr &= ~(SCR_I2SMODE_MSK);
			/* In Normal mode only first word contains valid data, any other words are zero filled.
			 * WL controls the word/slot size (i.e it not fixed like in the I2S master case)
			 * Since in normal mode the number of voices can be different then the number of
			 * words/slots in the frame, we use the nslots argument to calculate the frame size.
			 */
			f_bit_clk = mx->sample_rate * mx->nslots * (mx->sample_size * _BITS_BYTE);
			pm = (mx->sys_clk / f_bit_clk / 2) - 1;
			mx->ssi->stccr = (mx->ssi->stccr & ~STCCR_PM_MSK) | pm;
		}
		mx->ssi->stcr |= STCR_TFDIR_INTERNAL | STCR_TXDIR_INTERNAL;
	}
	else
	{
		if (!(mx->clk_mode & CLK_MODE_NORMAL))
			mx->ssi->scr |= SCR_I2S_SLAVE;
		mx->ssi->stcr &= ~(STCR_TFDIR_INTERNAL | STCR_TXDIR_INTERNAL);
	}

	/* Enable SSI */
	mx->ssi->scr |= SCR_SSI_EN;

#if defined (VARIANT_MX6X) || defined (VARIANT_BT) || defined (VARIANT_AIC3104)
	mx->ssi->scr |= SCR_TX_EN;
	while (SFCSR_TXFIFO0_CNT(mx->ssi->sfcsr));

	/* 
	 * Though the Tx FIFO is empty, if no usleep() here, 
	 * it could transfer one additional word at the first time playback, which causes the
	 * voice in left/right channel swapped.
	 */
	usleep(125);

	mx->ssi->scr &= ~SCR_TX_EN;
#endif
	return (EOK);
}

int
mx_ssi_init (HW_CONTEXT_T * mx)
{
	/* Disable SSI */
	mx->ssi->scr = 0;

    /* Mask all the tx/rx time slots */
    mx->ssi->stmsk = STMSK_ALL;
    mx->ssi->srmsk = SRMSK_ALL;

    /* Disable all interrupts */
    mx->ssi->sier = 0x0;

    /* Disable Tx/Rx FIFOs */
    mx->ssi->stcr &= ~(STCR_TXFIFO0_EN | STCR_TXFIFO1_EN);
    mx->ssi->srcr &= ~(SRCR_RXFIFO0_EN | SRCR_RXFIFO1_EN);

	/* Clock idle state high, Synchronous clocks */
    mx->ssi->scr = SCR_CLK_IST | SCR_SYNC_MODE;

#if defined (VARIANT_MX6X) || defined (VARIANT_BT) || defined (VARIANT_AIC3104)
    mx->ssi->scr |= SCR_SYNC_TX_FS;
#endif

	if (mx->xclk_pol)
	{
		/* Data is clocked out on rising edge of bit clock */
		mx->ssi->stcr &= ~(STCR_TSCKP_FE);
	}
	else
	{
		/* Data is clocked out on falling edge of bit clock */
		mx->ssi->stcr |= STCR_TSCKP_FE;
	}

	if (mx->rclk_pol)
	{
		/* Data is clocked in on rising edge of bit clock */
		mx->ssi->srcr |= SRCR_RSCKP_RE;
	}
	else
	{
		/* Data is clocked in on falling edge of bit clock */
		mx->ssi->srcr &= ~(SRCR_RSCKP_RE);
	}

	if (mx->xfsync_len == FSYNC_LEN_BIT)
	{
		/* Frame sync length is 1 clock bit long */
		mx->ssi->stcr |= STCR_TFSL_BIT;
		mx->ssi->srcr |= SRCR_RFSL_BIT;
	}
	else
	{
		/* Frame sync length is 1 word long */
		mx->ssi->stcr &= ~(STCR_TFSL_BIT);
		mx->ssi->srcr &= ~(SRCR_RFSL_BIT);
	}


	if (mx->xfsync_pol)
	{
		/* Frame sync is active high */
		mx->ssi->stcr &= ~(STCR_TFSI_AL);
		mx->ssi->srcr &= ~(SRCR_RFSI_AL);
	}
	else
	{
		/* Frame sync is active low  */
		mx->ssi->stcr |= STCR_TFSI_AL;
		mx->ssi->srcr |= SRCR_RFSI_AL;
	}
	
	if (mx->bit_delay)
	{
		/* 1-bit delay */
		mx->ssi->stcr |= STCR_TEFS_EARLY;
		mx->ssi->srcr |= SRCR_REFS_EARLY;
	}
	else
	{
		/* 0-bit delay */
		mx->ssi->stcr &= ~(STCR_TEFS_EARLY);
		mx->ssi->srcr &= ~(SRCR_REFS_EARLY);
	}

	/* Bypass divide by 2 and prescaler divided by 8 */
	mx->ssi->stccr &= ~(STCCR_DIV2 | STCCR_PSR);
	mx->ssi->srccr &= ~(SRCCR_DIV2 | SRCCR_PSR);

    /* Sample size */
    mx->ssi->stccr = (mx->ssi->stccr & ~STCCR_WL_MSK) |
        (mx->sample_size == 2 ? STCCR_WL_16BIT : STCCR_WL_24BIT);
	mx->ssi->srccr = (mx->ssi->srccr & ~SRCCR_WL_MSK) |
        (mx->sample_size == 2 ? SRCCR_WL_16BIT : SRCCR_WL_24BIT);
	
	/* Words per frame */
    mx->ssi->stccr = (mx->ssi->stccr & ~STCCR_DC_MSK) | STCCR_DC(mx->nslots);
    mx->ssi->srccr = (mx->ssi->srccr & ~SRCCR_DC_MSK) | SRCCR_DC(mx->nslots);

    /* Set fifo water mark */
    mx->ssi->sfcsr = (mx->ssi->sfcsr & ~SFCSR_TFWM0_MSK) | FIFO_WATERMARK;
    mx->ssi->sfcsr = (mx->ssi->sfcsr & ~SFCSR_RFWM0_MSK) | (FIFO_WATERMARK << 4);

    /* Enable DMA TX Event */
    mx->ssi->sier |= SIER_TDMAE;

    /* Enable DMA Event */
    mx->ssi->sier |= SIER_RDMAE;

    /* Enable TX FIFO */
    mx->ssi->stcr |= STCR_TXFIFO0_EN;

    /* Enable RX FIFO */
    mx->ssi->srcr |= SRCR_RXFIFO0_EN;

    /* Enable/unmask first transmit time slot */
    mx->ssi->stmsk &= ~(STMSK_SLOT0 | STMSK_SLOT1);
    mx->ssi->srmsk &= ~(SRMSK_SLOT0 | STMSK_SLOT1);

	/* set_ssi_clock_rate() will enable the SSI for us */
	set_ssi_clock_rate ( mx, mx->sample_rate_max);

    return 0;
}

ado_dll_version_t ctrl_version;
void
ctrl_version (int *major, int *minor, char *date)
{
    *major = ADO_MAJOR_VERSION;
    *minor = 1;
    date = __DATE__;
}

static void
build_dma_string (char * dmastring, uint32_t fifopaddr,
        int dmaevent, int watermark, int num_of_subchn)
{
    char str[50];

    strcpy (dmastring, "eventnum=");
    strcat (dmastring, itoa (dmaevent, str, 10));
    strcat (dmastring, ",watermark=");
    strcat (dmastring, itoa (watermark, str, 10));
    strcat (dmastring, ",fifopaddr=0x");
    strcat (dmastring, ultoa (fifopaddr, str, 16));

    if(num_of_subchn == 1)
        strcat (dmastring, ",regen,contloop");
}

/* This function configures the various protocol specific flags
 */
void configure_default_protocol_flags(HW_CONTEXT_T * mx)
{
    switch (mx->protocol) {
        case PROTOCOL_PCM:
			/* In normal mode configure a short frame sync pulse to 
			 * allow for a single word frame.
			 */
			if (mx->clk_mode & CLK_MODE_NORMAL)
			{
				mx->xfsync_len = FSYNC_LEN_BIT;
				mx->nslots = 1;
			}
			else
				mx->xfsync_len = FSYNC_LEN_WORD;
            mx->xclk_pol = 1;	/* tx on rising edge */
            mx->rclk_pol = 1;	/* rx on rising edge */
            mx->bit_delay = 0;	/* 0 bit delay */
            mx->xfsync_pol = 1;	/* Active high frame sync */
            break;
        case PROTOCOL_I2S:
			if (mx->clk_mode & CLK_MODE_NORMAL)
			{
				/* In normal mode only the first slot/word is valid so the number of voices must be 1,
				 * but to get a proper I2S/TDM frame we need at least two slots/words (additional slots will be zero
				 * filled by the hardware).
				 */
				if (mx->nslots < 2)
					mx->nslots = 2;
			}
			mx->xfsync_len = FSYNC_LEN_WORD;
            mx->xclk_pol = 0;	/* tx on falling edge */
            mx->rclk_pol = 1;	/* rx on rising edge */
            mx->bit_delay = 1;	/* 1 bit delay */           
            mx->xfsync_pol = 0;	/* Active low frame sync */
            break;
        default:
            break;
    }
}

int mx_parse_commandline(HW_CONTEXT_T * mx, char *args)
{
    char    str[100];
    char    *value;
	/*
	 * ssibase      = [#]; SSI number or base address
 	 * tevt         = [#]; ssi TX DMA event number
	 * tchn         = [#]; ssi TX DMA channel type
	 * revt         = [#]; ssi RX DMA event number
	 * rchn         = [#]; ssi RX DMA channel type
	 * rate         = [#]; sample rate of audio
	 * mixer        = [info:[mixer option1]:[mixer options2][:[other options]]]
	 *                mixer=info to dump the details of mixer options
	*/
	char * mx_opts[] = {
    	"ssibase",
    	"tevt",
    	"tchn",
    	"revt",
    	"rchn",
    	"rate",
    	"mixer",
    	"clk_mode",
    	"i2c_bus",
    	"sys_clk",
    	"capture_subchn",
		"voices",
    	"sample_size",
		"protocol",
		"xclk_pol",
		"rclk_pol",
		"xfsync_pol",
		"bit_delay",
		"xfsync_size",
		"nslots",
		NULL
	};
   
#if !defined(DEFAULT_SSI)
	/* Getting the SSI Base addresss from the Hwinfo Section if available */
    unsigned hwi_off = hwi_find_device("ssi", 0);
    if(hwi_off != HWI_NULL_OFF) 
	{
        hwi_tag *tag = hwi_tag_find(hwi_off, HWI_TAG_NAME_location, 0);
        if(tag)
            mx->ssibase = tag->location.base;
    }
	else
		mx->ssibase = 1;
#else
		mx->ssibase = DEFAULT_SSI;
#endif

	mx->sample_rate_min = SAMPLE_RATE_MIN;
	mx->sample_rate_max = SAMPLE_RATE_MAX;
    mx->clk_mode = DEFAULT_CLK_MODE;
	mx->sample_size = SAMPLE_SIZE;
    mx->i2c_dev = 0xff; // codec layer will overwrite this value
    mx->sys_clk = SSI_CLK;
    mx->cap_subchn = 1; /* by default, only support 1 capture subchannel */
	mx->voices = mx->nslots = 2; /* Default the number of slots to equal the number of voices */
    strcpy (mx->mixeropts, "");
#if defined(SSI_PROTOCOL_PCM)
	mx->protocol = PROTOCOL_PCM;
#else
	mx->protocol = PROTOCOL_I2S;
#endif
    configure_default_protocol_flags(mx);

    /* initialize variables used for multiple subchn capture*/
    mx->frag_index = 0;
    mx->capture_phys_addr = 0;
    mx->capture_virtual_addr = NULL;
    mx->capture_buff_size = 0;
    mx->capture_buff_size_mod = 0;
    mx->capture_shmem_size = 0;
    sprintf(mx->capture_shmem_name, "/snd1234");

	switch (DEFAULT_SSI)
	{
		case 1:
			mx->ssibase = SSI1_BASE_ADDR;
			mx->tevt = SSI1_TX_DMA_EVENT;
			mx->revt = SSI1_RX_DMA_EVENT;
			mx->tchn = SSI1_TX_DMA_CTYPE;	
			mx->rchn = SSI1_RX_DMA_CTYPE;	
			break;
		case 2:
			mx->ssibase = SSI2_BASE_ADDR;
			mx->tevt = SSI2_TX_DMA_EVENT;
			mx->revt = SSI2_RX_DMA_EVENT;
			mx->tchn = SSI2_TX_DMA_CTYPE;	
			mx->rchn = SSI2_RX_DMA_CTYPE;	
			break;
#if defined(SSI3_BASE_ADDR)			
		case 3:
			mx->ssibase = SSI3_BASE_ADDR;
			mx->tevt = SSI3_TX_DMA_EVENT;
			mx->revt = SSI3_RX_DMA_EVENT;
			mx->tchn = SSI3_TX_DMA_CTYPE;	
			mx->rchn = SSI3_RX_DMA_CTYPE;	
			break;
#endif
		default:
			break;
	}

    while (*args != '\0')
    {
        switch (getsubopt (&args, mx_opts, &value)) {
            case 0:
                mx->ssibase = strtoul (value, NULL, 0);
                switch (mx->ssibase)
                {
                     case 1:
                     case SSI1_BASE_ADDR:
                         mx->ssibase = SSI1_BASE_ADDR;
                         mx->tevt = SSI1_TX_DMA_EVENT;
                         mx->revt = SSI1_RX_DMA_EVENT;
                         mx->tchn = SSI1_TX_DMA_CTYPE;	
                         mx->rchn = SSI1_RX_DMA_CTYPE;	
                         break;
                     case 2:
                     case SSI2_BASE_ADDR:
                         mx->ssibase = SSI2_BASE_ADDR;
                         mx->tevt = SSI2_TX_DMA_EVENT;
                         mx->revt = SSI2_RX_DMA_EVENT;
                         mx->tchn = SSI2_TX_DMA_CTYPE;	
                         mx->rchn = SSI2_RX_DMA_CTYPE;	
                         break;
#if defined(SSI3_BASE_ADDR)			
                     case 3:
                     case SSI3_BASE_ADDR:
                         mx->ssibase = SSI3_BASE_ADDR;
                         mx->tevt = SSI3_TX_DMA_EVENT;
                         mx->revt = SSI3_RX_DMA_EVENT;
                         mx->tchn = SSI3_TX_DMA_CTYPE;	
                         mx->rchn = SSI3_RX_DMA_CTYPE;	
                         break;
#endif
                     default:
                         break;
                }
				break;
            case 1:
                mx->tevt = strtol (value, NULL, 0);
                break;
            case 2:
                mx->tchn = strtol (value, NULL, 0);
                break;
            case 3:
                mx->revt = strtol (value, NULL, 0);
                break;
            case 4:
				/* Channel Type -> Peripheral vs. Shared Peripheral 
				 * Look at the Shared Peripheral Bus Arbiter section of the reference
				 * manual to see if your SSI controller is on shared bus or not.
				 */
                mx->rchn = strtol (value, NULL, 0);
                break;
            case 5:
                {
                    char *value2;
                    mx->sample_rate_min = mx->sample_rate_max = strtoul(value, 0, 0);
                    if (ado_pcm_rate2flag(mx->sample_rate_min) == 0)
                    {
                        ado_error("Invalid sample rate - %d", mx->sample_rate_min);
                        return EINVAL;
                    }
                    if ((value2 = strchr(value, ':')) != NULL)
                    {
                        mx->sample_rate_max = strtoul(value2 + 1, 0, 0);
                        if (ado_pcm_rate2flag(mx->sample_rate_max) == 0)
                        {
                            ado_error("Invalid sample rate - %d", mx->sample_rate_max);
                            return EINVAL;
                        }
                    }
                }
                break;
            case 6:
                if (strlen (value) > MAX_MIXEROPT)
                {
                    ado_error ("MX SSI: Board specific options pass maximum len %d",
                            MAX_MIXEROPT);
                    ado_free (mx);
                    return (-1);
                }
                strncat (mx->mixeropts, value, MAX_MIXEROPT);
                break;
            case 7:
				if (value && *value != NULL)
                {
                    if (strcmp(value, "i2s_master") == 0)
                    {
                        mx->clk_mode = CLK_MODE_MASTER;
                        ado_debug (DB_LVL_DRIVER, "%s: Audio clock mode = I2S Master", __FUNCTION__);
                    }
                    else if (strcmp(value, "i2s_slave") == 0)
                    {
                        mx->clk_mode = CLK_MODE_SLAVE;
                        ado_debug (DB_LVL_DRIVER, "%s: Audio clock mode = I2S Slave", __FUNCTION__);
                    }
					else if (strcmp(value, "normal_master") == 0)
                    {
                        mx->clk_mode = CLK_MODE_NORMAL | CLK_MODE_MASTER;
                        ado_debug (DB_LVL_DRIVER, "%s: Audio clock mode = Normal Master", __FUNCTION__);
                    }
					else if (strcmp(value, "normal_slave") == 0)
                    {
                        mx->clk_mode = CLK_MODE_NORMAL | CLK_MODE_SLAVE;
                        ado_debug (DB_LVL_DRIVER, "%s: Audio clock mode = Normal slave", __FUNCTION__);
                    }
                    else
                    {
                        ado_debug (DB_LVL_DRIVER, "%s: Audio clock mode not supported", __FUNCTION__);
                        return EINVAL;
                    }
                }
                break;
            case 8:
                mx->i2c_dev = strtol (value, NULL, 0);
                if(mx->i2c_dev < 0 || mx->i2c_dev > 2)
                {
                    mx->i2c_dev = 0xff; // let codec layer determine the i2c device
                    ado_debug(DB_LVL_DRIVER, "mxssi: Invalid i2c_dev value (card will decide what to connect to)");
                }
                break;
            case 9:
                mx->sys_clk = strtol(value, NULL, 0);
                break;
            case 10:
                mx->cap_subchn = strtol(value, NULL, 0);
                if(mx->cap_subchn < 1 || mx->cap_subchn > MAX_CAP_SUBCHN_COUNT)
                {
                    ado_debug(DB_LVL_DRIVER, "mxssi: Invalid number of capture subchannels (must be between 1 and %d) (default to 1)", MAX_CAP_SUBCHN_COUNT);
                    mx->cap_subchn = 1;
                }
                break;
			case 11:
				mx->voices = strtoul(value, NULL, 0);
				break;
			case 12:
				mx->sample_size = strtoul(value, NULL, 0);
				switch (mx->sample_size)
				{
					case 2:
					case 16:
						mx->sample_size = 2;
						break;
					case 4:
					case 32:
						mx->sample_size = 4;
						break;
					default:
						ado_error("MX SSI: Invalid sample size");
					break;
				}
				break;
			case 13:
                if (value && *value != NULL)
                {
                    if (strcmp(value, "i2s") == 0)
                    {
                        mx->protocol = PROTOCOL_I2S;
                        ado_debug (DB_LVL_DRIVER, "%s: Audio Protocol = I2S", __FUNCTION__);
                    }
                    else if (strcmp(value, "pcm") == 0)
                    {
                        mx->protocol = PROTOCOL_PCM;
                        ado_debug (DB_LVL_DRIVER, "%s: Audio Protocol = PCM", __FUNCTION__);
                    }
                    else
                    {
                        ado_debug (DB_LVL_DRIVER, "%s: Audio Protocol not supported", __FUNCTION__);
                        return EINVAL;
                    }

                    configure_default_protocol_flags(mx);
                }
                break;
			case 14:
                mx->xclk_pol = atoi (value);
                if (mx->xclk_pol > 1 || mx->xclk_pol < 0)
                {
                    ado_error ("Invalid xclk polarity mode");
                    return EINVAL;
                }
                break;
			case 15:
                mx->rclk_pol = atoi (value);
                if (mx->rclk_pol > 1 || mx->rclk_pol < 0)
                {
                    ado_error ("Invalid rclk polarity mode");
                    return EINVAL;
                }
                break;
            case 16:
                mx->xfsync_pol = atoi (value);
                if (mx->xfsync_pol > 1 || mx->xfsync_pol < 0)
                {
                    ado_error ("Invalid xfsync_pol value");
                    return EINVAL;
                }
                break;
            case 17:
                mx->bit_delay = atoi (value);
                if (mx->bit_delay > 1 || mx->bit_delay < 0)
                {
                    ado_error ("Invalid bit_delay value (0 or 1)");
                    return EINVAL;
                }
                break;
			case 18:
				if (value && *value != NULL)
                {
                    if (strcmp(value, "bit") == 0)
                    {
                        mx->xfsync_len = FSYNC_LEN_BIT;
                        ado_debug (DB_LVL_DRIVER, "%s: Audio Frame sync lenght = bit", __FUNCTION__);
                    }
                    else if (strcmp(value, "word") == 0)
                    {
                        mx->xfsync_len = FSYNC_LEN_WORD;
                        ado_debug (DB_LVL_DRIVER, "%s: Audio Frame sync length = word", __FUNCTION__);
                    }
                    else
                    {
                        ado_debug (DB_LVL_DRIVER, "%s: Audio Frame sync length not supported", __FUNCTION__);
                        return EINVAL;
                    }
				}
			case 19:
				if (value && *value != NULL)
					mx->nslots = atoi(value);
				break;
            default:
                break;
        }
    }
   
	if (mx->clk_mode & CLK_MODE_NORMAL)
		mx->voices = 1; /* Normal mode only supports valid data in the first slot/word */

    /* If mclk was not provided as a mixer option then use the sys_clk as the mclk to the mixer */
    if (strstr(mx->mixeropts, "mclk=") == NULL)
    {
        if (strcmp (mx->mixeropts, "") != 0)
            strncat (mx->mixeropts, ":", MAX_MIXEROPT);

        strncat (mx->mixeropts, "mclk=", MAX_MIXEROPT);
        strncat (mx->mixeropts, itoa (mx->sys_clk, &str[0], 10), MAX_MIXEROPT);
    }

    if (strcmp (mx->mixeropts, "") != 0)
        strncat (mx->mixeropts, ":", MAX_MIXEROPT);

    strncat (mx->mixeropts, "samplerate=", MAX_MIXEROPT);
    strncat (mx->mixeropts, itoa (mx->sample_rate_max, &str[0], 10), MAX_MIXEROPT);
	
	return EOK;
}

ado_ctrl_dll_init_t ctrl_init;
int
ctrl_init (HW_CONTEXT_T ** hw_context, ado_card_t * card, char *args)
{
	int     i;
    mx_t    *mx;
	uint32_t rate;
    char    str[100];
	dma_driver_info_t sdma_info;
#define NUM_RATES  (sizeof(ratelist) / sizeof(ratelist[0]))
	uint32_t ratelist[] = { SND_PCM_RATE_8000, SND_PCM_RATE_16000, SND_PCM_RATE_32000, SND_PCM_RATE_48000 };

    ado_debug (DB_LVL_DRIVER, "CTRL_DLL_INIT: MX");

    if ((mx = (mx_t *) ado_calloc (1, sizeof (mx_t))) == NULL)
    {
        ado_error ("MX SSI: Unable to allocate memory for mx (%s)",
            strerror (errno));
        return -1;
    }
    *hw_context = mx;
    
	if (mx_parse_commandline(mx, args) != EOK)
    {
        ado_free(mx);
        return -1;
    }

    ado_card_set_shortname (card, "MX");
    ado_card_set_longname (card, "Freescale i.MX", 0);

	ado_error("Using SSI Base 0x%x", mx->ssibase);
    mx->ssi = mmap_device_memory (0, sizeof (ssi_t),
        PROT_READ | PROT_WRITE | PROT_NOCACHE, 0, mx->ssibase);
    if (mx->ssi == MAP_FAILED)
    {
        ado_error ("MX SSI: Unable to mmap SSI (%s)", strerror (errno));
        ado_free (mx);
        return -1;
    }

    ado_mutex_init (&mx->hw_lock);

    if (get_dmafuncs (&mx->sdmafuncs, sizeof (dma_functions_t)) == -1)
    {
        ado_error ("MX SSI: Failed to get DMA lib functions");
        ado_mutex_destroy (&mx->hw_lock);
        munmap_device_memory (mx->ssi, sizeof (ssi_t));
        ado_free (mx);
        return -1;
    }

    my_attach_pulse (&mx->play_strm.pulse,
        &mx->play_strm.sdma_event, mx_play_pulse_hdlr, mx);

    my_attach_pulse (&mx->cap_strm[0].pulse,
        &mx->cap_strm[0].sdma_event, mx_cap_pulse_hdlr, mx);

    if (mx->sdmafuncs.init (NULL) == -1)
    {
        ado_error ("MX SSI: DMA init failed");
        my_detach_pulse (&mx->cap_strm[0].pulse);
        my_detach_pulse (&mx->play_strm.pulse);
        ado_mutex_destroy (&mx->hw_lock);
        munmap_device_memory (mx->ssi, sizeof (ssi_t));
        ado_free (mx);
        return -1;
    }

	mx->sdmafuncs.driver_info(&sdma_info);
    /*
     * DMA channel setup for Playback
     * 1) watermark = must match the TX FIFO watermark in SSI
     * 2) eventnum = SSI TX0 DMA event
     * 3) fifopaddr = Physical address of SSI TX0 FIFO
     * 4) regen,contloop = DMA in repeat/loop mode so we only need to configure
     *    the DMA transfer on channel acquire and not on every interrupt.
     */
    build_dma_string (str, mphys ((void *)&(mx->ssi->stx0)), mx->tevt, FIFO_WATERMARK, 1);

	ado_debug(DB_LVL_DRIVER, "MX SSI: Playback sdma priority = %d", sdma_info.max_priority); 	
    mx->play_strm.dma_chn =
        mx->sdmafuncs.channel_attach(str, &mx->play_strm.sdma_event, &mx->tchn,
            sdma_info.max_priority, DMA_ATTACH_EVENT_PER_SEGMENT);

    if (mx->play_strm.dma_chn == NULL)
    {
        ado_error ("MX SSI: SDMA playback channel attach failed");
        my_detach_pulse (&mx->cap_strm[0].pulse);
        my_detach_pulse (&mx->play_strm.pulse);
        mx->sdmafuncs.fini ();
        ado_mutex_destroy (&mx->hw_lock);
        munmap_device_memory (mx->ssi, sizeof (ssi_t));
        ado_free (mx);
        return -1;
    }

    /*
     * DMA channel setup for Capture
     * 1) watermark = must match the RX FIFO watermark in SSI
     * 2) eventnum = SSI RX0 DMA event
     * 3) fifopaddr = Physical address of SSI RX0 FIFO
     * 4) regen,contloop = DMA in repeat/loop mode so we only need to configure
     *    the DMA transfer on channel acquire and not on every interrupt.
     *
     *    NOTE: For multiple capture support, we need to manually trigger DMA
     *          transfers. therefore, we do not use regen,contloop option
     */
    build_dma_string (str, mphys ((void *)&(mx->ssi->srx0)), mx->revt, FIFO_WATERMARK, mx->cap_subchn);


	ado_debug(DB_LVL_DRIVER, "MX SSI: Capture sdma priority = %d", sdma_info.max_priority); 	
    mx->cap_strm[0].dma_chn =
            mx->sdmafuncs.channel_attach(str, &mx->cap_strm[0].sdma_event, &mx->rchn,
            sdma_info.max_priority, DMA_ATTACH_EVENT_PER_SEGMENT);

    if (mx->cap_strm[0].dma_chn == NULL)
    {
        ado_error ("MX SSI: SDMA capture channel attach failed");
        mx->sdmafuncs.channel_release (mx->play_strm.dma_chn);
        my_detach_pulse (&mx->cap_strm[0].pulse);
        my_detach_pulse (&mx->play_strm.pulse);
        mx->sdmafuncs.fini ();
        ado_mutex_destroy (&mx->hw_lock);
        munmap_device_memory (mx->ssi, sizeof (ssi_t));
        ado_free (mx);
        return -1;
    }

    mx_ssi_init (mx);

    mx->play_strm.pcm_caps.chn_flags =
        SND_PCM_CHNINFO_BLOCK | SND_PCM_CHNINFO_STREAM |
        SND_PCM_CHNINFO_INTERLEAVE | SND_PCM_CHNINFO_BLOCK_TRANSFER |
        SND_PCM_CHNINFO_MMAP | SND_PCM_CHNINFO_MMAP_VALID;
    mx->play_strm.pcm_caps.formats = mx->sample_size == 2 ? SND_PCM_FMT_S16_LE : SND_PCM_FMT_S32_LE;

    for (i = 0; i < NUM_RATES; i++)
    {    
        rate = ado_pcm_flag2rate(ratelist[i]);
        if (rate >= mx->sample_rate_min && rate <= mx->sample_rate_max)
            mx->play_strm.pcm_caps.rates |= ratelist[i];
    }
    mx->play_strm.pcm_caps.min_rate = mx->sample_rate_min;
    mx->play_strm.pcm_caps.max_rate = mx->sample_rate_max;
    mx->play_strm.pcm_caps.min_voices = mx->voices;
    mx->play_strm.pcm_caps.max_voices = mx->voices;
    mx->play_strm.pcm_caps.min_fragsize = 64;
    mx->play_strm.pcm_caps.max_fragsize = 32 * 1024;

    memcpy (&mx->cap_strm[0].pcm_caps, &mx->play_strm.pcm_caps,
        sizeof (mx->cap_strm[0].pcm_caps));
 
    /* if more than 1 capture subchannel is desired, limit number of fragments to 2 */
    if(mx->cap_subchn > 1)
        mx->cap_strm[0].pcm_caps.max_frags = 2;

    mx->play_strm.pcm_funcs.capabilities2 = mx_capabilities;
    mx->play_strm.pcm_funcs.aquire = mx_playback_aquire;
    mx->play_strm.pcm_funcs.release = mx_playback_release;
    mx->play_strm.pcm_funcs.prepare = mx_prepare;
    mx->play_strm.pcm_funcs.trigger = mx_playback_trigger;
#if 0
    mx->play_strm.pcm_funcs.position = mx_position;
#endif

    mx->cap_strm[0].pcm_funcs.prepare = mx_prepare;
    mx->cap_strm[0].pcm_funcs.capabilities2 = mx_capabilities;
    if(1 == mx->cap_subchn)
    {
        mx->cap_strm[0].pcm_funcs.aquire = mx_capture_aquire;
        mx->cap_strm[0].pcm_funcs.release = mx_capture_release;
        mx->cap_strm[0].pcm_funcs.trigger = mx_capture_trigger;
    }
    else
    {
        mx->cap_strm[0].pcm_funcs.aquire = mx_capture_aquire2;
        mx->cap_strm[0].pcm_funcs.release = mx_capture_release2;
        mx->cap_strm[0].pcm_funcs.trigger = mx_capture_trigger2;
    }
#if 0
    mx->cap_strm[0].pcm_funcs.position = mx_position;
#endif

    if (ado_pcm_create (card, "mx PCM 0", 0, "mx-0",
            1, &mx->play_strm.pcm_caps, &mx->play_strm.pcm_funcs,
            1, &mx->cap_strm[0].pcm_caps, &mx->cap_strm[0].pcm_funcs,
            &mx->pcm1))
    {
        ado_error ("MX SSI: Unable to create pcm devices (%s)", strerror (errno));
        mx->sdmafuncs.channel_release (mx->cap_strm[0].dma_chn);
        mx->sdmafuncs.channel_release (mx->play_strm.dma_chn);
        my_detach_pulse (&mx->cap_strm[0].pulse);
        my_detach_pulse (&mx->play_strm.pulse);
        mx->sdmafuncs.fini ();
        ado_mutex_destroy (&mx->hw_lock);
        munmap_device_memory (mx->ssi, sizeof (ssi_t));
        ado_free (mx);
        return -1;
    }

    if (codec_mixer (card, mx))
    {
        ado_error ("MX SSI: Unable to create codec mixer");
        mx->sdmafuncs.channel_release (mx->cap_strm[0].dma_chn);
        mx->sdmafuncs.channel_release (mx->play_strm.dma_chn);
        my_detach_pulse (&mx->cap_strm[0].pulse);
        my_detach_pulse (&mx->play_strm.pulse);
        mx->sdmafuncs.fini ();
        ado_mutex_destroy (&mx->hw_lock);
        munmap_device_memory (mx->ssi, sizeof (ssi_t));
        ado_free (mx);
        return -1;
    }

    return 0;
}

ado_ctrl_dll_destroy_t ctrl_destroy;
int
ctrl_destroy (HW_CONTEXT_T * mx)
{
    ado_debug (DB_LVL_DRIVER, "CTRL_DLL_DESTROY: MX");
    mx->sdmafuncs.channel_release (mx->cap_strm[0].dma_chn);
    mx->sdmafuncs.channel_release (mx->play_strm.dma_chn);
    my_detach_pulse (&mx->cap_strm[0].pulse);
    my_detach_pulse (&mx->play_strm.pulse);
    mx->sdmafuncs.fini ();
    ado_mutex_destroy (&mx->hw_lock);
    munmap_device_memory (mx->ssi, sizeof (ssi_t));
    ado_free (mx);
    return 0;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/deva/ctrl/mx/mxssi_dll.c $ $Rev: 746212 $")
#endif
