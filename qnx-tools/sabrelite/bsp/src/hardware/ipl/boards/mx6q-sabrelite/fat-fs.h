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


#ifndef _FAT_FS_H_
#define _FAT_FS_H_

#define SECTOR_SIZE     512             // standard block size
#define FAT32           32              // standard block size

#define ENT_UNUSED      0xE5
#define ENT_END         0x00

#define MIN(a,b)        (((a) < (b)) ? (a) : (b))

#define GET_CLUSTER(x)  ((x)->clust_lo | ((x)->clust_hi << 16))
#define GET_LONG(X)     (((X)[0]) + ((X)[1]<<8) + ((X)[2]<<16) + ((X)[3]<<24))
#define GET_WORD(X)     (((X)[0]) + ((X)[1]<<8))

/* partition structure */
typedef struct partition
{
    unsigned char   boot_ind;           // boot indicator
    unsigned char   beg_head;           // begin head
    unsigned char   begin_sect;         // begin sector
    unsigned char   beg_cylinder;       // begin cylinder
    unsigned char   os_type;            // partition type
    unsigned char   end_head;           // end head
    unsigned char   end_sect;           // end sector
    unsigned char   end_cylinder;       // end cylinder
    unsigned char   part_offset[4];     // startsector number
    unsigned char   part_size[4];       // partition size in sectors
} partition_t;

/* Master Boot Record */
typedef struct mbr
{
    unsigned char   pad[446];           // fill bytes
    partition_t     part_entry[4];      // partition entries
    unsigned short sign;                // signature (0xAA55)
} mbr_t;


/* Boot Sector (valid only for FAT12 and FAT16) */
typedef struct bpb
{
    unsigned char   jump[3];
    unsigned char   oem_name[8];
    unsigned char   sec_size[2];
    unsigned char   sec_per_clus;
    unsigned char   num_rsvd_secs[2];
    unsigned char   num_fats;
    unsigned char   num_root_ents[2];
    unsigned char   num16_secs[2];
    unsigned char   media;
    unsigned char   num16_fat_secs[2];
    unsigned char   sec_per_trk[2];
    unsigned char   num_heads[2];
    unsigned char   num_hidden_sec[4];
    unsigned char   num32_secs[4];
    unsigned char   drv_num;
    unsigned char   cur_head;
    unsigned char   boot_sig;
    unsigned char   vol_id[4];
    unsigned char   vol_label[11];
    unsigned char   sys_type[8];
    unsigned char   pad[448];
    unsigned char   sig[2];
} bpb_t;

/* Boot Sector (valid only for FAT32) */
typedef struct bpb32
{
    unsigned char   jump[3];
    unsigned char   oem_name[8];
    unsigned char   sec_size[2];
    unsigned char   sec_per_clus;
    unsigned char   num_rsvd_secs[2];
    unsigned char   num_fats;
    unsigned char   num_root_ents[2];
    unsigned char   num16_secs[2];
    unsigned char   media;
    unsigned char   num16_fat_secs[2];
    unsigned char   sec_per_trk[2];
    unsigned char   num_heads[2];
    unsigned char   num_hidden_sec[4];
    unsigned char   num32_secs[4];
    unsigned char   num32_fat_secs[4];
    unsigned char   ext_flags[2];
    unsigned char   version[2];
    unsigned char   root_clus[4];
    unsigned char   fsinfo_sec[2];
    unsigned char   backup_boot_sec[2];
    unsigned char   reserved[12];
    unsigned char   drv_num;
    unsigned char   cur_head;
    unsigned char   boot_sig;
    unsigned char   vol_id[4];
    unsigned char   vol_label[11];
    unsigned char   sys_type[8];
    unsigned char   code[10];
    unsigned char   pad[410];
    unsigned char   sig[2];
} bpb32_t;

/* file system information */
typedef struct fs_info
{
    unsigned        fs_offset;          // boot sector offset
    unsigned        fat_type;           // type of FAT (12, 16)
    unsigned        total_sectors;      // total number of sectors
    unsigned        root_dir_sectors;   // number of sectors for root irectory
    unsigned        reserved_sectors;   // reserved sectors
    unsigned        data_sectors;       // number of sectors to store data to
    unsigned        fat_size;           // fat size;
    unsigned        number_of_fats;     // number of fats
    unsigned        cluster_size;       // sectors per cluster
    unsigned        count_of_clusters;  // number of clusters

    unsigned        fat1_start;         // start of first FAT
    unsigned        fat2_start;         // start of seconf FAT
    unsigned        root_dir_start;     // start of root directory
    unsigned        root_entry_count;   // number of root directory entries
    unsigned        cluster2_start;     // start of first data cluster

    void            *device;            // pointer to device structure
} fs_info_t;


/* directory entry */
typedef struct dir_entry
{
    unsigned char   short_name[11];
    unsigned char   attrib;
    unsigned char   ntres;
    unsigned char   crt_time_tenth;
    unsigned short  crt_time;
    unsigned short  crt_date;
    unsigned short  lst_acc_date;
    unsigned short  clust_hi;
    unsigned short  wrt_time;
    unsigned short  wrt_date;
    unsigned short  clust_lo;
    unsigned        size;
} dir_entry_t;

#define LAST_LONG_ENTRY 0x40


/* file system function */
extern int fat_read_mbr(mx6x_sdmmc_t *sdmmc, int verbose);
extern int fat_copy_named_file(unsigned char *buf, char *name);

#endif /* #ifndef _FAT_FS_H_ */


/* __SRCVERSION( "$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/boards/mx6q-sabrelite/fat-fs.h $ $Rev: 740617 $" ); */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/boards/mx6q-sabrelite/fat-fs.h $ $Rev: 740617 $")
#endif
