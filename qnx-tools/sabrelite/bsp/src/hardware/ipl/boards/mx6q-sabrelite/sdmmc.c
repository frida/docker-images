/*
 * $QNXLicenseC:
 * Copyright 2011, QNX Software Systems.
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

#include "ipl.h"
#include "sdmmc.h"
#include "sdhc_mx6x.h"
#include "ipl_mx6x.h"
#include <hw/inout.h>

static int sdmmc_pio_read(mx6x_sdmmc_t *sdmmc, void *buf, unsigned len);

/*
 * SDMMC command table
 */
static const struct cmd_str
{
    int         cmd;
    unsigned    sdmmc_cmd;
} cmdtab[] =
{
    // MMC_GO_IDLE_STATE
    { 0,            (0 << 24) },
    // MMC_SEND_OP_COND (R3)
    { 1,            (1 << 24)|MMCCMDXTYPE_RSPTYPL48 },
    // MMC_ALL_SEND_CID (R2)
    { 2,            (2 << 24)|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL136 },
    // MMC_SET_RELATIVE_ADDR (R6)
    { 3,            (3 << 24)|MMCCMDXTYPE_CICEN|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL48 },
    // MMC_SWITCH (R1b)
    { 6,            (6 << 24)|MMCCMDXTYPE_CICEN|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL48B },
    // MMC_SEL_DES_CARD (R1b)
    { 7,            (7 << 24)|MMCCMDXTYPE_CICEN|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL48B },
    // MMC_IF_COND (R7)
    { 8,            (8 << 24)|MMCCMDXTYPE_CICEN|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL48 },
    // MMC_SEND_CSD (R2)
    { 9,            (9 << 24)|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL136 },
    // MMC_SEND_STATUS (R1)
    {13,            (13<< 24)|MMCCMDXTYPE_CICEN|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL48 },
    // MMC_SET_BLOCKLEN (R1)
    {16,            (16<< 24)|MMCCMDXTYPE_CICEN|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL48 },
    // MMC_READ_SINGLE_BLOCK (R1)
    {17,            (17<< 24)|MMCCMDXTYPE_CICEN|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL48|MMCCMDXTYPE_DPSEL },
    // MMC_READ_MULTIPLE_BLOCK (R1)
    {18,            (18<< 24)|MMCCMDXTYPE_CICEN|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL48|MMCCMDXTYPE_DPSEL},
    // MMC_APP_CMD (R1)
    {55,            (55<< 24)|MMCCMDXTYPE_CICEN|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL48 },
    // SD_SET_BUS_WIDTH (R1)
    {(55<<8)| 6,    (6 << 24)|MMCCMDXTYPE_CICEN|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL48 },
    // SD_SEND_OP_COND (R3)
    {(55<<8)|41,    (41<< 24)|MMCCMDXTYPE_RSPTYPL48 },
    // end of command list
    {-1 },
};

static
void delay(unsigned dly)
{   
    volatile int j;

    while (dly--) { 
        for (j = 0; j < 132; j++)
        ;
    }
}

/* searches for a command in the command table */
static struct cmd_str *
get_cmd(int cmd)
{
    struct cmd_str *command = (struct cmd_str *)&cmdtab[0];
   
    while (command->cmd != -1) {
        if (command->cmd == cmd)
            return command;
        command++;
    }

    return 0;
}

/* sets the sdmmc clock frequency */
static void
sdmmc_set_frq (mx6x_sdmmc_t *sdmmc, unsigned frq)
{
    unsigned    base = sdmmc->sdmmc_pbase;

	int sdhc_clk = SDMMC_CLK_DEFAULT;
	unsigned div, pre_div, clk;

	if (sdhc_clk / 16 > frq) {
		for (pre_div = 2; pre_div < 256; pre_div *= 2)
			if ((sdhc_clk / pre_div) <= (frq * 16))
				break;
	} else
		pre_div = 2;

	for (div = 1; div <= 16; div++)
		if ((sdhc_clk / (div * pre_div)) <= frq)
			break;

	pre_div >>= 1;
	div -= 1;

	clk = (pre_div << 8) | (div << 4);

    out32(base + MX6X_MMCSYSCTL, in32(base + MX6X_MMCSYSCTL) & ~(MMCSYSCTL_SDCLKEN)); /* Clear SDCLKEN to change the SD clock frequency */	

    out32(base + MX6X_MMCSYSCTL, in32(base + MX6X_MMCSYSCTL) & ~(MMCSYSCTL_SDCLKFSMASK | MMCSYSCTL_DVSMASK)); /* Clear clock masks */	

    out32(base + MX6X_MMCSYSCTL, in32(base + MX6X_MMCSYSCTL) | clk); /* Set the clock */

    while (!(in32(base + MX6X_MMCPRSNTST) & MMCPRSNTST_SDSTB)) ; /* Wait till the SD clock is stable */
    
    // Wait for data and CMD signals to finish reset
    while ((in32(base + MX6X_MMCSYSCTL) & (MMCSYSCTL_RSTC | MMCSYSCTL_RSTD)) != 0) ;

    out32(base + MX6X_MMCSYSCTL, in32(base + MX6X_MMCSYSCTL) | MMCSYSCTL_SDCLKEN); /* Enable the SD clock */
}

/* sets the data bus width */
static void
sd_set_bus_width (mx6x_sdmmc_t *sdmmc, int width)
{
    unsigned    base = sdmmc->sdmmc_pbase;
    unsigned    tmp = in32(base + MX6X_MMCPROTOCTL);

	out32(base + MX6X_MMCPROTOCTL, tmp & MMCPROTOCTL_DTW1BIT); /* Clearing the Data transfer width field */
	tmp = in32(base + MX6X_MMCPROTOCTL); /* Read the register again */

    if (width == 8) {
        out32(base + MX6X_MMCPROTOCTL, tmp | MMCPROTOCTL_EMODLE | MMCPROTOCTL_DTW8BIT);
    } else if (width == 4) {
        out32(base + MX6X_MMCPROTOCTL, tmp | MMCPROTOCTL_EMODLE | MMCPROTOCTL_DTW4BIT);		
    } else {
        out32(base + MX6X_MMCPROTOCTL, tmp | MMCPROTOCTL_EMODLE);	
	}
}

/* initializes the SDMMC controller */
int
sdmmc_init_ctrl(mx6x_sdmmc_t *sdmmc)
{
    unsigned    base = sdmmc->sdmmc_pbase;

    sdmmc->icr  = 0x000001aa;             /* 2.7 V - 3.6 V */
    sdmmc->ocr  = 0x00300000;             /* 3.2 V - 3.4 V */

    /* Disable All interrupts */
    out32(base + MX6X_MMCINTRSTEN, 0);

    out32(base + MX6X_MMCSYSCTL, in32(base + MX6X_MMCSYSCTL) | MMCSYSCTL_RSTA); /* Software Reset for ALL */

	while(in32(base + MX6X_MMCSYSCTL) & MMCSYSCTL_RSTA); /* Wait till the Reset bit gets cleared */

    sdmmc_set_frq(sdmmc, 400000);

    out32(base + MX6X_MMCPROTOCTL, 0x00000020); /* Reset value for Protocol Control Register */

    out32(base + MX6X_MMCINTRSIGEN, 0x017F8033); /* Enable all interrupt signals */
 
    out32(base + MX6X_MMCINTRSTEN, 0x017F8033); /* Enable all interrupt status bits */
   
    return SDMMC_OK;
}

/* clean up the SDMMC controller */
int
sdmmc_fini(mx6x_sdmmc_t *sdmmc)
{
    unsigned base = sdmmc->sdmmc_pbase;

    out32(base + MX6X_MMCINTRSIGEN, 0);
    out32(base + MX6X_MMCINTRSTEN, 0);
    out32(base + MX6X_MMCINTRST, 0x017F0003);
    out32(base + MX6X_MMCSYSCTL, 0);

    return 0;
}

/* setup DMA for data read */
static void
mx6x_setup_dma_read(mx6x_sdmmc_t *sdmmc)
{
    unsigned int sys_ctl;

    out32(sdmmc->sdmmc_pbase + MX6X_MMCBLATR, (sdmmc->cmd.bcnt << 16) | sdmmc->cmd.bsize);
    out32(sdmmc->sdmmc_pbase + MX6X_MMCDMASYSADD, (unsigned)sdmmc->cmd.dbuf);

    sys_ctl = in32(sdmmc->sdmmc_pbase + MX6X_MMCSYSCTL);
    sys_ctl &= ~MMCSYSCTL_DTOCVMASK;
    sys_ctl |= (14 << MMCSYSCTL_DTOCVSHIFT);
    out32(sdmmc->sdmmc_pbase + MX6X_MMCSYSCTL, sys_ctl);
}

/* issues a command on the SDMMC bus */
static int
sdmmc_send_cmd(mx6x_sdmmc_t *sdmmc)
{
    struct cmd_str  *command;
    int             data_txf = 0;
    unsigned        base  = sdmmc->sdmmc_pbase;
    unsigned int    mix_ctrl = 0;
    sdmmc_cmd_t     *cmd  = &sdmmc->cmd;

    if (0 == (command = get_cmd (cmd->cmd)))
        return SDMMC_ERROR;

    /* check if need data transfer */
    data_txf = command->sdmmc_cmd & MMCCMDXTYPE_DPSEL;

    while ((in32(base + MX6X_MMCPRSNTST) & MMCPRSNTST_CDIHB) ||
           (in32(base + MX6X_MMCPRSNTST) & MMCPRSNTST_CIHB));

    while (in32(base + MX6X_MMCPRSNTST) & MMCPRSNTST_DLA);
      
    if (command->cmd == 0) {
	    out32(base + MX6X_MMCSYSCTL, in32(base + MX6X_MMCSYSCTL) | MMCSYSCTL_INITA | (0x2 << 16)); /* Set Initialization Active */
		while (in32(base + MX6X_MMCSYSCTL) & MMCSYSCTL_INITA); /* Wait till the initialization bit gets cleared */
    } else if (command->cmd == 17) {
        mix_ctrl = (MMCCMDXTYPE_DTDSEL | MMCCMDXTYPE_DMAEN);
    } else if (command->cmd == 18) {
        mix_ctrl = (MMCCMDXTYPE_MSBSEL | MMCCMDXTYPE_BCEN | MMCCMDXTYPE_AC12EN | MMCCMDXTYPE_DTDSEL | MMCCMDXTYPE_DMAEN);
    }

    if (data_txf) {
        mx6x_setup_dma_read(sdmmc);
    }

    /* clear SDMMC interrupt status register, write 1 to clear */
    out32(base + MX6X_MMCINTRST, 0xffffffff);

    /*
     * Rev 1.0 i.MX6x chips require the watermark register to be set prior to 
     * every SD command being sent. If the watermark is not set only SD interface 3 works.
     */
    out32(base + MX6X_MMCWATML, (0x80 << MMCWATML_RDWMLSHIFT));

    /* setup the argument register and send command */
    out32(base + MX6X_MMCCMDARG, cmd->arg);

    mix_ctrl |= (in32(base + MX6X_MMCMIX_CTRL) & (0xF << 22));
    out32(base + MX6X_MMCMIX_CTRL, mix_ctrl);

    out32(base + MX6X_MMCCMDXTYPE, command->sdmmc_cmd);

    /* wait for command finish */
    while (!(in32(base + MX6X_MMCINTRST) & (MMCINTR_CC | MMCINTR_ERRI)))
        ;

    /* check error status */
    unsigned temp = in32(base + MX6X_MMCINTRST);
    if (temp & MMCINTR_ERRI) {
        cmd->erintsts = temp;
        out32(base + MX6X_MMCSYSCTL, in32(base + MX6X_MMCSYSCTL) | MMCSYSCTL_RSTC);
        while (in32(base + MX6X_MMCSYSCTL) & MMCSYSCTL_RSTC)
            ;

        if (temp != 2)
        return SDMMC_ERROR;
    }

    /* get command response */
    if (cmd->rsp != 0) {
        cmd->rsp[0] = in32(base + MX6X_MMCCMDRSP0);

        if ((command->sdmmc_cmd & MMCCMDXTYPE_RSPTYPMASK) == MMCCMDXTYPE_RSPTYPL136) {
			cmd->rsp[1] = in32(base + MX6X_MMCCMDRSP1);
			cmd->rsp[2] = in32(base + MX6X_MMCCMDRSP2);
			cmd->rsp[3] = in32(base + MX6X_MMCCMDRSP3);

			/*
			* CRC is not included in the response register,
			* we have to left shift 8 bit to match the 128 CID/CSD structure
			*/
			cmd->rsp[3] = (cmd->rsp[3] << 8) | (cmd->rsp[2] >> 24);
			cmd->rsp[2] = (cmd->rsp[2] << 8) | (cmd->rsp[1] >> 24);
			cmd->rsp[1] = (cmd->rsp[1] << 8) | (cmd->rsp[0] >> 24);
			cmd->rsp[0] = (cmd->rsp[0] << 8);
        }
    }

    if (data_txf) {
        while (1) {
            while (!(in32(base + MX6X_MMCINTRST) & (MMCINTR_TC | MMCINTR_ERRI)))
                ; 

            if (in32(base + MX6X_MMCINTRST) & MMCINTR_ERRI) {
                out32(base + MX6X_MMCSYSCTL, in32(base + MX6X_MMCSYSCTL) | MMCSYSCTL_RSTC);
                while (in32(base + MX6X_MMCSYSCTL) & MMCSYSCTL_RSTC)
                    ;

                return SDMMC_ERROR;
            }
            if (in32(base + MX6X_MMCINTRST) & MMCINTR_TC)
                break;
        }
    }

    return SDMMC_OK;
}

/* Gets the card state */
static int
sdmmc_get_state (mx6x_sdmmc_t *sdmmc)
{
    unsigned    rsp;

    CMD_CREATE (sdmmc->cmd, MMC_SEND_STATUS, sdmmc->card.rca << 16, &rsp, 0, 0, 0);

    if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
        return SDMMC_ERROR;

    /* Bits 9-12 define the CURRENT_STATE */
    sdmmc->card.state = rsp & 0x1e00;

    return SDMMC_OK;
}

/* Check OCR for comliance with controller OCR */
static int
sdmmc_check_ocr (mx6x_sdmmc_t *sdmmc, unsigned ocr)
{
    if (ocr & sdmmc->ocr)
        return SDMMC_OK;

    CMD_CREATE (sdmmc->cmd, MMC_GO_IDLE_STATE, 0, 0, 0, 0, 0);

    sdmmc_send_cmd(sdmmc);

    return SDMMC_ERROR;
}
/* parse card speed */
static unsigned 
sdmmc_tran_speed(unsigned char ts)
{
    static const unsigned sdmmc_ts_exp[] = { 100, 1000, 10000, 100000, 0, 0, 0, 0 };
    static const unsigned sdmmc_ts_mul[] = { 0, 1000, 1200, 1300, 1500, 2000, 2500, 3000,
                                        3500, 4000, 4500, 5000, 5500, 6000, 7000, 8000 };

    return sdmmc_ts_exp[(ts & 0x7)] * sdmmc_ts_mul[(ts & 0x78) >> 3];
}

/* parse SD CSD */
static int
sd_parse_csd(mx6x_sdmmc_t *sdmmc, unsigned *resp)
{
    card_t      *card = &sdmmc->card;
    sd_csd_t    *csd  = &sdmmc->csd.sd_csd;
    int         bsize, csize, csizem;

    csd->csd_structure      = resp[3] >> 30;
    csd->taac               = resp[3] >> 16;
    csd->nsac               = resp[3] >> 8;
    csd->tran_speed         = resp[3];
    csd->ccc                = resp[2] >> 20;
    csd->read_bl_len        = (resp[2] >> 16) & 0x0F;
    csd->read_bl_partial    = (resp[2] >> 15) & 1;
    csd->write_blk_misalign = (resp[2] >> 14) & 1;
    csd->read_blk_misalign  = (resp[2] >> 13) & 1;
    csd->dsr_imp            = (resp[2] >> 12) & 1;

    if (csd->csd_structure == 0) {
        csd->csd.csd_ver1.c_size = ((resp[2] & 0x3FF) << 2) | (resp[1] >> 30);
        csd->csd.csd_ver1.vdd_r_curr_min = (resp[1] >> 27) & 0x07;
        csd->csd.csd_ver1.vdd_r_curr_max = (resp[1] >> 24) & 0x07;
        csd->csd.csd_ver1.vdd_w_curr_min = (resp[1] >> 21) & 0x07;
        csd->csd.csd_ver1.vdd_w_curr_max = (resp[1] >> 18) & 0x07;
        csd->csd.csd_ver1.c_size_mult    = (resp[1] >> 15) & 0x07;
    } else {
        csd->csd.csd_ver2.c_size = ((resp[2] & 0x3F) << 16) | (resp[1] >> 16);
    }
    csd->erase_blk_en       = (resp[1] >> 14) & 1;
    csd->sector_size        = (resp[1] >> 7) & 0x7F;
    csd->wp_grp_size        = (resp[1] >> 0) & 0x7F;
    csd->wp_grp_enable      = (resp[0] >> 31);
    csd->r2w_factor         = (resp[0] >> 26) & 0x07;
    csd->write_bl_len       = (resp[0] >> 22) & 0x0F;
    csd->write_bl_partial   = (resp[0] >> 21) & 1;
    csd->file_format_grp    = (resp[0] >> 15) & 1;
    csd->copy               = (resp[0] >> 14) & 1;
    csd->perm_write_protect = (resp[0] >> 13) & 1;
    csd->tmp_write_protect  = (resp[0] >> 12) & 1;
    csd->file_format        = (resp[0] >> 10) & 3;

    if (csd->perm_write_protect || csd->tmp_write_protect) {
        ser_putstr("SD Card write_protect\n");
    }

    if (csd->csd_structure == 0) {
        bsize  = 1 << csd->read_bl_len;
        csize  = csd->csd.csd_ver1.c_size + 1;
        csizem = 1 << (csd->csd.csd_ver1.c_size_mult + 2);
    } else {
        bsize  = SDMMC_BLOCKSIZE;
        csize  = csd->csd.csd_ver2.c_size + 1;
        csizem = 1024;
    }

    /* force to 512 byte block */
    if (bsize > SDMMC_BLOCKSIZE && (bsize % SDMMC_BLOCKSIZE) == 0) {
        unsigned ts = bsize / SDMMC_BLOCKSIZE;
        csize = csize * ts;
        bsize = SDMMC_BLOCKSIZE;
    }

    card->blk_size = bsize;
    card->blk_num  = csize * csizem;
    card->speed    = sdmmc_tran_speed(csd->tran_speed);

//    sdmmc_set_frq (sdmmc, card->speed);

    if (SDMMC_OK != sdmmc_get_state(sdmmc))
        return SDMMC_ERROR;

    return SDMMC_OK;

}

/* parse SD CID */
static void
sd_parse_cid(mx6x_sdmmc_t *sdmmc, unsigned *resp)
{
    sd_cid_t    *cid = &sdmmc->cid.sd_cid;

    cid->mid    = resp[3] >> 24;
    cid->oid[0] = (resp[3] >> 8) & 0xFF;
    cid->oid[1] = (resp[3] >> 16) & 0xFFFF;
    cid->oid[2] = 0;
    cid->pnm[0] = resp[3];
    cid->pnm[1] = resp[2] >> 24;
    cid->pnm[2] = resp[2] >> 16;
    cid->pnm[3] = resp[2] >> 8;
    cid->pnm[4] = resp[2];
    cid->pnm[5] = 0;
    cid->prv    = resp[1] >> 24;
    cid->psn    = (resp[1] << 8) | (resp[0] >> 24);
    cid->mdt    = (resp[0] >> 8) & 0xFFFF;
}

/* parse MMC CSD */
static void
mmc_parse_csd(mx6x_sdmmc_t *sdmmc, unsigned *resp)
{
    card_t      *card = &sdmmc->card;
    mmc_csd_t   *csd  = &sdmmc->csd.mmc_csd;
    int         bsize, csize, csizem;

    csd->csd_structure = resp[3] >> 30;
    csd->mmc_prot      = (resp[3] >> 26) & 0x0F;
    csd->tran_speed    = resp[3];
    csd->read_bl_len   = (resp[2] >> 16) & 0x0F;
    csd->c_size        = ((resp[2] & 0x3FF) << 2) | (resp[1] >> 30);
    csd->c_size_mult   = (resp[1] >> 15) & 0x07;

    bsize  = 1 << csd->read_bl_len;
    csize  = csd->c_size + 1;
    csizem = 1 << (csd->c_size_mult + 2);

    card->blk_size = bsize;
    card->blk_num  = csize * csizem;
}

/* parse MMC CID */
static void
mmc_parse_cid(mx6x_sdmmc_t *sdmmc, unsigned *resp)
{
    mmc_cid_t   *cid = &sdmmc->cid.mmc_cid;
    mmc_csd_t   *csd = &sdmmc->csd.mmc_csd;

    cid->pnm[0] = resp[3];
    cid->pnm[1] = resp[2] >> 24;
    cid->pnm[2] = resp[2] >> 16;
    cid->pnm[3] = resp[2] >> 8;
    cid->pnm[4] = resp[2];
    cid->pnm[5] = resp[1] >> 24;

    if (csd->mmc_prot < 2) {
        cid->mid    = resp[3] >> 8;
        cid->pnm[6] = resp[1] >> 16;
        cid->pnm[7] = 0;
        cid->hwr    = (resp[1] >> 12) & 0x0F;
        cid->fwr    = (resp[1] >> 8) & 0x0F;
    } else {
        cid->mid    = resp[3] >> 24;
        cid->oid    = (resp[3] >> 8) & 0xFFFF;
        cid->pnm[6] = 0;
    }

    cid->psn    = ((resp[1] & 0xFFFF) << 16) | (resp[0] >> 16);
    cid->mcd    = (resp[0] >> 12) & 0x0F;
    cid->ycd    = ((resp[0] >> 8) & 0x0F) + 1997;
}

/* MMC switch mode */
static int
mmc_switch_mode(mx6x_sdmmc_t *sdmmc)
{
    // We know the chip is in 8 bit mode and support high speed (52MHz)

    // switch to 8 bit mode
    CMD_CREATE (sdmmc->cmd, MMC_SWITCH, (3 << 24) | (183 << 16) | (2 << 8), 0, 0, 0, 0);
    if (SDMMC_OK == sdmmc_send_cmd(sdmmc))
        sd_set_bus_width (sdmmc, 8);
    else
        return -1;

    do {
        if (SDMMC_OK != sdmmc_get_state(sdmmc))
            return SDMMC_ERROR;
    } while (MMC_PRG == sdmmc->card.state);  

    // switch to high speed mode
    CMD_CREATE (sdmmc->cmd, MMC_SWITCH, (3 << 24) | (185 << 16) | (1 << 8) | 1, 0, 0, 0, 0);
    if (SDMMC_OK == sdmmc_send_cmd(sdmmc))
        sdmmc_set_frq(sdmmc, 52000000);
    else
        return -1;

    do {
        if (SDMMC_OK != sdmmc_get_state(sdmmc))
            return SDMMC_ERROR;
    } while (MMC_PRG == sdmmc->card.state);

    return SDMMC_OK;
}

int sdmmc_highspeed(mx6x_sdmmc_t *sdmmc)
{
    unsigned char tmp[64];
    card_t   *card = &sdmmc->card;

    CMD_CREATE (sdmmc->cmd, MMC_APP_CMD, card->rca << 16, 0, 0, 0, 0);
    if (SDMMC_OK != sdmmc_send_cmd(sdmmc)){
        ser_putstr("sdmmc_highspeed:  MMC_APP_CMD failed\n");
        return SDMMC_ERROR;
    }

    CMD_CREATE (sdmmc->cmd, MMC_SEND_SCR, 0, 0, 0, 0, 0);
    if (SDMMC_OK != sdmmc_pio_read(sdmmc, tmp, 8)){
        ser_putstr("sdmmc_highspeed:  MMC_SEND_SCR failed\n");
        return SDMMC_ERROR;
    }

    if((tmp[0] & 0x0F)<1){
        ser_putstr("sdmmc_highspeed:  SD version <1, no high speed support\n");
        return SDMMC_ERROR;
    }

    CMD_CREATE (sdmmc->cmd, MMC_SWITCH, 0x00ffff01, 0, 0, 0, 0);
    if (SDMMC_OK != sdmmc_pio_read(sdmmc, tmp, 64)){
        ser_putstr("sdmmc_highspeed:  check mode failed\n");
        return SDMMC_ERROR;
    }

    do {
        if (SDMMC_OK != sdmmc_get_state(sdmmc))
            return SDMMC_ERROR;
    } while (MMC_PRG == sdmmc->card.state);  


    if ((tmp[13] & 0x02) == 0){
        ser_putstr("sdmmc_highspeed:  No high speed support\n");
        return SDMMC_ERROR;
    }

    CMD_CREATE (sdmmc->cmd, MMC_SWITCH, 0x80ffff01, 0, 0, 0, 0);
    if(SDMMC_OK != sdmmc_pio_read(sdmmc, tmp, 64))
    {
        ser_putstr("Set high speed failed FAILED\n");
        return SDMMC_ERROR;
    }

    do {
        if (SDMMC_OK != sdmmc_get_state(sdmmc))
            return SDMMC_ERROR;
    } while (MMC_PRG == sdmmc->card.state);  


    if ((tmp[16] & 0x0F) == 1){
        ser_putstr("sdmmc_highspeed:  Switch to high speed mode successful\n");
        sdmmc->card.speed = 50000000;
    }

    return SDMMC_OK;
}

/* SDMMC card identification and initialisation */
int
sdmmc_init_card(mx6x_sdmmc_t *sdmmc)
{
    int         i;
    card_t      *card = &sdmmc->card;
    unsigned    rsp[4], cid[4];

    /* initial relative card address */
    card->rca = 0;

    /* Probe for SDC card */
    CMD_CREATE(sdmmc->cmd, MMC_GO_IDLE_STATE, 0, 0, 0, 0, 0);
    if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
        return SDMMC_ERROR;

    card->state = MMC_IDLE;

    CMD_CREATE (sdmmc->cmd, MMC_IF_COND, sdmmc->icr, 0, 0, 0, 0);
    if (SDMMC_OK == sdmmc_send_cmd(sdmmc)) {
        card->type = eSDC_V200;
    } else {
        CMD_CREATE (sdmmc->cmd, MMC_APP_CMD, 0, 0, 0, 0, 0);
        if (SDMMC_OK == sdmmc_send_cmd(sdmmc)) {
            card->type = eSDC;
        } else {
            card->type = eMMC;
        }
    }

    /* Start Power Up process and get Operation Condition Register */
    CMD_CREATE (sdmmc->cmd, MMC_GO_IDLE_STATE, 0, 0, 0, 0, 0);
    if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
        return SDMMC_ERROR;

    switch (card->type) {
        case eSDC_V200:
            CMD_CREATE (sdmmc->cmd, MMC_IF_COND, sdmmc->icr, 0, 0, 0, 0);
            if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
               return SDMMC_ERROR;

            for (i = 0; i < POWER_UP_WAIT; i++)
            {
                /* when ACMDx(SD_SEND_OP_COND) is to be issued, send CMD55 first */
                CMD_CREATE (sdmmc->cmd, MMC_APP_CMD, card->rca << 16, 0, 0, 0, 0);
                if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
                    return SDMMC_ERROR;

                CMD_CREATE (sdmmc->cmd, SD_SEND_OP_COND, 0xc0000000 | sdmmc->ocr, rsp, 0, 0, 0);
                if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
                    return SDMMC_ERROR;

                if ((i == 0) && (SDMMC_OK != sdmmc_check_ocr(sdmmc, rsp[0])))
                    return SDMMC_ERROR;

                if ((rsp[0] & 0x80000000) == 0x80000000)    // check if card power up process finished
                    break;

                delay(1);
            }

            /* test for HC bit set */
            if ((rsp[0] & 0x40000000) == 0x40000000)
            {
                card->type = eSDC_HC;
            }
            else    // no HC card detected
            {
                card->type = eSDC;
            }
        break;
      
        case eSDC:
            CMD_CREATE (sdmmc->cmd, SD_SEND_OP_COND, 0x80000000 | sdmmc->ocr, rsp, 0, 0, 0);
            for (i = 0; i < POWER_UP_WAIT; i++)
            {
                if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
                    return SDMMC_ERROR;

                if ((i == 0) && (SDMMC_OK != sdmmc_check_ocr(sdmmc, rsp[0])))
                    return SDMMC_ERROR;

                if (rsp[0] & 0x80000000)        // check if card power up process finished
                    break;

                delay(1);
            }
        break;

        case eMMC:
            CMD_CREATE (sdmmc->cmd, MMC_SEND_OP_COND, 0xc0000000 | sdmmc->ocr, rsp, 0, 0, 0);
            for (i = 0; i < POWER_UP_WAIT; i++) {
                if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
                    return SDMMC_ERROR;

                if ((i == 0) && (SDMMC_OK != sdmmc_check_ocr(sdmmc, rsp[0])))
                    return SDMMC_ERROR;

                if (rsp[0] & 0x80000000)        // check if card power up process finished
                    break;

                delay(1);
            }
            /* test for HC bit set */
            if ((rsp[0] & 0x40000000) == 0x40000000)
                card->type = eMMC_HC;
            break;

        default:
            return SDMMC_ERROR;
    }

    /* check if time out */
    if (i >= POWER_UP_WAIT)
        return SDMMC_ERROR;

    card->state = MMC_READY;
   
    /* Identification of Device */
    CMD_CREATE (sdmmc->cmd, MMC_ALL_SEND_CID, 0, rsp, 0, 0, 0);
    if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
        return SDMMC_ERROR;

    card->state = MMC_IDENT;
    cid[0] = rsp[0], cid[1] = rsp[1], cid[2] = rsp[2], cid[3] = rsp[3];

    /* SDMMC relative address */
    switch (card->type) {
        case eSDC:
        case eSDC_HC:
            card->rca = 0x0001;
            CMD_CREATE (sdmmc->cmd, MMC_SET_RELATIVE_ADDR, card->rca << 16, rsp, 0, 0, 0);
            if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
                return SDMMC_ERROR;

            card->rca = rsp[0] >> 16;
            if (SDMMC_OK != sdmmc_get_state(sdmmc))
                return SDMMC_ERROR;
            break;
      
        case eMMC:
        case eMMC_HC:
            card->rca = 0x0001;
            CMD_CREATE (sdmmc->cmd, MMC_SET_RELATIVE_ADDR, card->rca << 16, rsp, 0, 0, 0);
            if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
                return SDMMC_ERROR;

            if (SDMMC_OK != sdmmc_get_state(sdmmc))
                return SDMMC_ERROR;
            break;
      
        default:
            return SDMMC_ERROR;
    }

    /* Get card CSD */
    CMD_CREATE (sdmmc->cmd, MMC_SEND_CSD, card->rca << 16, rsp, 0, 0, 0);
    if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
        return SDMMC_ERROR;

    /* Parse CSD and CID */
    switch (card->type) {
        case eSDC:
        case eSDC_HC:
            if (SDMMC_OK != sd_parse_csd(sdmmc, rsp))
                return SDMMC_ERROR;

            sd_parse_cid(sdmmc, cid);
            break;

        case eMMC:
        case eMMC_HC:
            mmc_parse_csd(sdmmc, rsp);
            mmc_parse_cid(sdmmc, cid);
            break;
      
        default:
            return SDMMC_ERROR;
    }

    /* Select the Card */
    CMD_CREATE (sdmmc->cmd, MMC_SEL_DES_CARD, card->rca << 16, 0, 0, 0, 0);
    if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
        return SDMMC_ERROR;

    if (SDMMC_OK != sdmmc_get_state(sdmmc))
        return SDMMC_ERROR;

    /* Set block size in case the default blocksize differs from 512 */
    card->blk_num *= card->blk_size / SDMMC_BLOCKSIZE;
    card->blk_size = SDMMC_BLOCKSIZE;

    CMD_CREATE (sdmmc->cmd, MMC_SET_BLOCKLEN, card->blk_size, 0, 0, 0, 0);
    if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
        return SDMMC_ERROR;

    if (SDMMC_OK != sdmmc_get_state(sdmmc))
        return SDMMC_ERROR;

    /* Switch mode */
    switch (card->type)
    {
        case eSDC:
        case eSDC_HC:
            /* when ACMDx(SD_SEND_OP_COND) is to be issued, send CMD55 first */
            CMD_CREATE (sdmmc->cmd, MMC_APP_CMD, card->rca << 16, 0, 0, 0, 0);
            if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
                return SDMMC_ERROR;

            /* switch to 4 bits bus */
            CMD_CREATE (sdmmc->cmd, SD_SET_BUS_WIDTH, 2, 0, 0, 0, 0);
            if (SDMMC_OK != sdmmc_send_cmd(sdmmc))
                return SDMMC_ERROR;

			sd_set_bus_width(sdmmc, 4);

            // set to high speed if supported
            sdmmc_highspeed(sdmmc);

            sdmmc_set_frq (sdmmc, card->speed);

        break;

        case eMMC:
        case eMMC_HC:
            if (SDMMC_OK != mmc_switch_mode(sdmmc))
                return SDMMC_ERROR;
        break;
         
        default:    /* impossible */
            return SDMMC_ERROR;
    }

    return SDMMC_OK;
}

/* read blocks from SDMMC */
int
sdmmc_read(mx6x_sdmmc_t *sdmmc, void *buf, unsigned blkno, unsigned blkcnt)
{
    int             cmd;
    unsigned        arg;

    if ((blkno >= sdmmc->card.blk_num) || ((blkno + blkcnt) > sdmmc->card.blk_num) || ((blkno + blkcnt) < blkno)) {
        return SDMMC_ERROR;
    }

    if (blkcnt == 0)
        return SDMMC_OK;

    if (blkcnt == 1)
        cmd = MMC_READ_SINGLE_BLOCK;
    else
        cmd = MMC_READ_MULTIPLE_BLOCK;

    arg = blkno;

    if ((sdmmc->card.type == eMMC) || (sdmmc->card.type == eSDC))
        arg *= SDMMC_BLOCKSIZE;

    CMD_CREATE (sdmmc->cmd, cmd, arg, 0, SDMMC_BLOCKSIZE, blkcnt, buf);

    if (SDMMC_OK != sdmmc_send_cmd(sdmmc)) {
        return SDMMC_ERROR;
    }

    while (sdmmc->card.state != MMC_TRAN) {
        if (SDMMC_OK != sdmmc_get_state(sdmmc)) {
            return SDMMC_ERROR;
        }
    }

    return SDMMC_OK;
}


/*only support 1 time read such as read SCR and MMC_SWITCH command*/
static int sdmmc_pio_read(mx6x_sdmmc_t *sdmmc, void *buf, unsigned len)
{
    unsigned        base  = sdmmc->sdmmc_pbase;
    sdmmc_cmd_t     *cmd  = &sdmmc->cmd;
    unsigned int    mix_ctrl = 0;
    unsigned        *pbuf = (unsigned *)buf;

	/*wait for ready */
    while ((in32(base + MX6X_MMCPRSNTST) & MMCPRSNTST_CDIHB) ||
           (in32(base + MX6X_MMCPRSNTST) & MMCPRSNTST_CIHB));
    while (in32(base + MX6X_MMCPRSNTST) & MMCPRSNTST_DLA);

    /*setup PIO read */
    out32(base + MX6X_MMCBLATR, (1<<16) | len);

    /* clear SDMMC status */
    out32(base + MX6X_MMCINTRST, 0xffffffff);

    mix_ctrl = MMCCMDXTYPE_DTDSEL |((in32(base + MX6X_MMCMIX_CTRL) & (0xF << 22)));
    out32(base + MX6X_MMCMIX_CTRL, mix_ctrl);
    /*
     * Rev 1.0 i.MX6x chips require the watermark register to be set prior to 
     * every SD command being sent. If the watermark is not set only SD interface 3 works.
     */
    out32(base + MX6X_MMCWATML, (0x80 << MMCWATML_RDWMLSHIFT));

    /* setup the argument register and send command */
    out32(base + MX6X_MMCCMDARG, cmd->arg);
    out32(base + MX6X_MMCCMDXTYPE, (cmd->cmd<< 24)|MMCCMDXTYPE_CICEN|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL48|MMCCMDXTYPE_DPSEL);

    /* wait for command finish */
    while (!(in32(base + MX6X_MMCINTRST) & (MMCINTR_BRR | MMCINTR_ERRI)))
        ;

    /* check error status */
    if (in32(base + MX6X_MMCINTRST) & MMCINTR_ERRI) {
        cmd->erintsts = in32(base + MX6X_MMCINTRST);
        out32(base + MX6X_MMCSYSCTL, in32(base + MX6X_MMCSYSCTL) | MMCSYSCTL_RSTC);
        while (in32(base + MX6X_MMCSYSCTL) & MMCSYSCTL_RSTC)
            ;
		if(cmd->erintsts != 2)
			return SDMMC_ERROR;
    }

    /* get command responce */
    if (cmd->rsp != 0) {
        cmd->rsp[0] = in32(base + MX6X_MMCCMDRSP0);
    }

    /*now read from FIFO */
    for (; len > 0; len -= 4){
        *pbuf++ = in32(base + MX6X_MMCDATAPORT);
    }
#if 0
    if ( len < MMCHS_FIFO_SIZE ) {
        int    cnt;
        for ( cnt = 2000; cnt; cnt-- ) {
            if ( !( in32( base + OMAP4_MMCHS_PSTATE ) & PSTATE_RTA ) ) {
                break;
            }
            delay( 100 );
        }
    }
#endif
    return SDMMC_OK;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/boards/mx6q-sabrelite/sdmmc.c $ $Rev: 740617 $")
#endif
