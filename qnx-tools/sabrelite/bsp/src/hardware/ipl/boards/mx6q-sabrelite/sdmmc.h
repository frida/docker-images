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

#ifndef _SDMMC_H_
#define _SDMMC_H_

/*
 * SDMMC Block Size
 */
#define SDMMC_BLOCKSIZE             512

/* this speed meets the requirement that during ident the clock speed
 * should be lower tha 400k
 */
#define SDMMC_CLK_DEFAULT 198000000

/*
 * SDMMC card power up wait loop
 */
#define POWER_UP_WAIT               100000

/* MultiMediaCard Command definitions */
#define MMC_GO_IDLE_STATE           0
#define MMC_SEND_OP_COND            1
#define MMC_ALL_SEND_CID            2
#define MMC_SET_RELATIVE_ADDR       3
#define MMC_SET_DSR                 4
#define MMC_SWITCH                  6
#define MMC_SEL_DES_CARD            7
#define MMC_IF_COND                 8
#define MMC_SEND_EXT_CSD            8
#define MMC_SEND_CSD                9
#define MMC_SEND_CID                10
#define MMC_READ_DAT_UNTIL_STOP     11
#define MMC_STOP_TRANSMISSION       12
#define MMC_SEND_STATUS             13
#define MMC_GO_INACTIVE_STATE       15
#define MMC_SET_BLOCKLEN            16
#define MMC_READ_SINGLE_BLOCK       17
#define MMC_READ_MULTIPLE_BLOCK     18
#define MMC_WRITE_DAT_UNTIL_STOP    20
#define MMC_WRITE_BLOCK             24
#define MMC_WRITE_MULTIPLE_BLOCK    25
#define MMC_PROGRAM_CID             26
#define MMC_PROGRAM_CSD             27
#define MMC_SET_WRITE_PROT          28
#define MMC_CLR_WRITE_PROT          29
#define MMC_SEND_WRITE_PROT         30
#define MMC_TAG_SECTOR_START        32
#define MMC_TAG_SECTOR_END          33
#define MMC_UNTAG_SECTOR            34
#define MMC_TAG_ERASE_GROUP_START   35
#define MMC_TAG_ERASE_GROUP_END     36
#define MMC_UNTAG_ERASE_GROUP       37
#define MMC_ERASE                   38
#define MMC_FAST_IO                 39
#define MMC_GO_IRQ_STATE            40
#define MMC_LOCK_UNLOCK             42
#define MMC_SEND_SCR                51
#define MMC_APP_CMD                 55
#define MMC_GEN_CMD                 56
#define MMC_READ_OCR                58
#define MMC_CRC_ON_OFF              59

#define SD_SET_BUS_WIDTH            ((55<<8)|6)
#define SD_SEND_OP_COND             ((55<<8)|41)

/*
 * SDMMC error codes
 */
#define SDMMC_OK                    0       // no error
#define SDMMC_ERROR                 -1      // SDMMC error

/* Card Status Response Bits */
#define MMC_OUT_OF_RANGE            (1 << 31)
#define MMC_ADDRESS_ERROR           (1 << 30)
#define MMC_BLOCK_LEN_ERROR         (1 << 29)
#define MMC_ERASE_SEQ_ERROR         (1 << 28)
#define MMC_ERASE_PARAM             (1 << 27)
#define MMC_WP_VIOLATION            (1 << 26)
#define MMC_CARD_IS_LOCKED          (1 << 25)
#define MMC_LOCK_UNLOCK_FAILED      (1 << 24)
#define MMC_COM_CRC_ERROR           (1 << 23)
#define MMC_ILLEGAL_COMMAND         (1 << 22)
#define MMC_CARD_ECC_FAILED         (1 << 21)
#define MMC_CC_ERROR                (1 << 20)
#define MMC_ERROR                   (1 << 19)
#define MMC_UNDERRUN                (1 << 18)
#define MMC_OVERRUN                 (1 << 17)
#define MMC_CID_CSD_OVERWRITE       (1 << 16)
#define MMC_WP_ERASE_SKIP           (1 << 15)
#define MMC_CARD_ECC_DISABLED       (1 << 14)
#define MMC_ERASE_RESET             (1 << 13)
/* Bits 9-12 define the CURRENT_STATE */
#define MMC_IDLE                    (0 << 9)
#define MMC_READY                   (1 << 9)
#define MMC_IDENT                   (2 << 9)
#define MMC_STANDBY                 (3 << 9)
#define MMC_TRAN                    (4 << 9)
#define MMC_DATA                    (5 << 9)
#define MMC_RCV                     (6 << 9)
#define MMC_PRG                     (7 << 9)
#define MMC_DIS                     (8 << 9)
/* End CURRENT_STATE */
#define MMC_READY_FOR_DATA          (1 << 8)
#define MMC_SWITCH_ERROR            (1 << 7)
#define MMC_URGENT_BKOPS            (1 << 6)
#define MMC_APP_CMD_S               (1 << 5)

/*
 * SDMMC card type
 */
typedef enum
{
    NONE = 0, eMMC, eSDC, eSDC_V200, eSDC_HC, eMMC_HC
} card_type_t;

/*
 * SDMMC command structure
 */
typedef struct
{
   int       cmd;                   // command to be issued
   unsigned  arg;                   // command argument
   unsigned *rsp;                   // pointer to response buffer
   unsigned  bsize;                 // data block size
   unsigned  bcnt;                  // data block count
   void     *dbuf;                  // pointer to data buffer
   short     erintsts;              // interrupt error status
} sdmmc_cmd_t;

/*
 * create a command structure
 */
#define CMD_CREATE(_cr, _cmd, _arg, _rsp, _bsize, _bcnt, _dbuf)         \
   do                                                                   \
   {                                                                    \
      (_cr).cmd      =(_cmd);                                           \
      (_cr).arg      =(_arg);                                           \
      (_cr).rsp      =(_rsp);                                           \
      (_cr).bsize    =(_bsize);                                         \
      (_cr).bcnt     =(_bcnt);                                          \
      (_cr).dbuf     =(_dbuf);                                          \
      (_cr).erintsts = 0;                                               \
   } while (0)

/*
 * SD CID
 */
typedef struct _sd_cid_t {
    unsigned char   mid;            // Manufacture ID
    unsigned char   oid[3];         // OEM/Application ID
    unsigned char   pnm[6];         // Product name
    unsigned char   prv;            // Product revision
    unsigned        psn;            // Product serial number
    unsigned short  mdt;            // Manufacture date
} sd_cid_t;

/*
 * SD CSD
 */
typedef struct _sd_csd_t {
    unsigned char   csd_structure;  // CSD structure
    unsigned char   taac;
    unsigned char   nsac;
    unsigned char   tran_speed;
    unsigned short  ccc;
    unsigned char   read_bl_len;
    unsigned char   read_bl_partial;
    unsigned char   write_blk_misalign;
    unsigned char   read_blk_misalign;
    unsigned char   dsr_imp;
    union {
        struct {
            unsigned short  c_size;
            unsigned char   vdd_r_curr_min;
            unsigned char   vdd_r_curr_max;
            unsigned char   vdd_w_curr_min;
            unsigned char   vdd_w_curr_max;
            unsigned char   c_size_mult;
        } csd_ver1;
        struct {
            unsigned        c_size;
        } csd_ver2;
    }csd;
    unsigned char   erase_blk_en;
    unsigned char   sector_size;
    unsigned char   wp_grp_size;
    unsigned char   wp_grp_enable;
    unsigned char   r2w_factor;
    unsigned char   write_bl_len;
    unsigned char   write_bl_partial;
    unsigned char   file_format_grp;
    unsigned char   copy;
    unsigned char   perm_write_protect;
    unsigned char   tmp_write_protect;
    unsigned char   file_format;
} sd_csd_t;

/*
 * MMC CID
 */
typedef struct _mmc_cid_t {
    unsigned        mid;            // Manufacture ID
    unsigned short  oid;            // OEM ID
    unsigned short  ycd;            // Year code
    unsigned char   pnm[8];         // Product name
    unsigned char   hwr;            // HW revision
    unsigned char   fwr;            // FW revision
    unsigned char   mcd;            // Month code
    unsigned        psn;            // Product serial number
} mmc_cid_t;

/*
 * MMC CSD
 */
typedef struct _mmc_csd_t {
    unsigned char   csd_structure;  // CSD structure
    unsigned char   mmc_prot;
    unsigned char   tran_speed;
    unsigned char   read_bl_len;
    unsigned short  c_size;
    unsigned char   c_size_mult;
} mmc_csd_t;

/*
 * SDMMC card structure
 */
typedef struct card_s
{
    unsigned        state;          // current state
    card_type_t     type;           // card type
    unsigned        blk_size;       // block size
    unsigned        blk_num;        // number of blocks
    unsigned        speed;          // clock frequency in kHz
    unsigned short  rca;            // relative card address
} card_t;

/*
 * SDMMC structure
 */
typedef struct mx6x_sdmmc
{
   unsigned     sdmmc_pbase;        // base address
   unsigned     icr;                // interface condition register
   unsigned     ocr;                // operation condition register

   sdmmc_cmd_t  cmd;                // command request structure
   card_t       card;               // card structure

   union {
    sd_cid_t    sd_cid;             // SD CID
    mmc_cid_t   mmc_cid;            // MMC CID
   } cid;

   union {
    sd_csd_t    sd_csd;             // SD CSD
    mmc_csd_t   mmc_csd;            // MMC CSD
   } csd;
} mx6x_sdmmc_t;

/*
 * SDMMC functions
 */
extern int sdmmc_read (mx6x_sdmmc_t *sdmmc, void *buf, unsigned blkno, unsigned blkcnt);
extern int sdmmc_init_ctrl(mx6x_sdmmc_t *sdmmc);
extern int sdmmc_init_card(mx6x_sdmmc_t *sdmmc);
extern int sdmmc_fini(mx6x_sdmmc_t *sdmmc);

#endif /* #ifndef _SDMMC_H_ */


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/boards/mx6q-sabrelite/sdmmc.h $ $Rev: 740617 $")
#endif
