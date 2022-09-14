/*
 * $QNXLicenseC:
 * Copyright 2013, QNX Software Systems.
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
#include "fat-fs.h"


/* file system information structure */
fs_info_t           fs_info;

/* common block buffer */
static unsigned      blk_buf[2*SECTOR_SIZE/sizeof(unsigned)];
static unsigned char *blk;
static unsigned g_fat_sector = -1;
static unsigned cache_valid = 0;

/* memory copy */
#if 0
static void
memcpy(void *dest, const void *src, int length )
{
    unsigned char *sp = (unsigned char *)src;
    unsigned char *dp = (unsigned char *)dest;

    if ((0 != dest) && (0 != src) && (length > 0)) {
        while (length--)
        {
            *dp++ = *sp++;
        }
    }
}
#endif

/* memory length */
static int
strlen(const char *str)
{
    char *s = (char *)str;
    int  len = 0;

    if (0 == str)
        return 0;

    while ('\0' != *s++)
        ++len;

    return len;
}

/* reads a sector relative to the start of the block device */
static int
read_sector(mx6x_sdmmc_t *sdmmc, unsigned blkno, void *buf, unsigned blkcnt)
{
    return sdmmc_read(sdmmc, buf, blkno, blkcnt);
}

/* reads a sector relative to the start of the partition 0 */
static int
read_fsector(unsigned sector, void *buf, unsigned sect_cnt)
{
    return read_sector((mx6x_sdmmc_t *)fs_info.device,
                        sector + fs_info.fs_offset, buf, sect_cnt);
}

/* detects the type of FAT */
static int
fat_detect_type(bpb_t *bpb)
{
    bpb32_t   *bpb32 = (bpb32_t *)bpb;

    if (GET_WORD(bpb->sig) != 0xaa55)
    {
        ser_putstr("BPB signature is wrong\n");
        return -1;
    }

    {
        int rc, bs, ns;

        bs = GET_WORD(bpb->sec_size);
        rc = GET_WORD(bpb->num_root_ents) * 32 + bs - 1;
        for (ns = 0; rc >= bs; ns++)
            rc -= bs;
        fs_info.root_dir_sectors = ns;
    }

    fs_info.fat_size = GET_WORD(bpb->num16_fat_secs);
    if (fs_info.fat_size == 0)
        fs_info.fat_size = GET_LONG(bpb32->num32_fat_secs);

    fs_info.total_sectors = GET_WORD(bpb->num16_secs);
    if (fs_info.total_sectors == 0)
        fs_info.total_sectors = GET_LONG(bpb->num32_secs);

    fs_info.number_of_fats = bpb->num_fats;
    fs_info.reserved_sectors = GET_WORD(bpb->num_rsvd_secs);

    fs_info.data_sectors = fs_info.total_sectors -
        (fs_info.reserved_sectors + fs_info.number_of_fats * fs_info.fat_size + fs_info.root_dir_sectors);

    fs_info.cluster_size = bpb->sec_per_clus;

    {
        int ds, cs, nc;

        ds = fs_info.data_sectors;
        cs = fs_info.cluster_size;
        for (nc = 0; ds >= cs; nc++)
            ds -= cs;
        fs_info.count_of_clusters = nc;
    }

    fs_info.root_entry_count = GET_WORD(bpb->num_root_ents);
    fs_info.fat1_start = fs_info.reserved_sectors;
    fs_info.fat2_start = fs_info.fat1_start + fs_info.fat_size;
    fs_info.root_dir_start = fs_info.fat2_start + fs_info.fat_size;
    fs_info.cluster2_start = fs_info.root_dir_start + ((fs_info.root_entry_count * 32) + (512 - 1)) / 512;

    if (fs_info.count_of_clusters < 4085) {

        return 12;

    } else if (fs_info.count_of_clusters < 65525) {

        return 16;

    } else {

        fs_info.root_dir_start = GET_LONG(bpb32->root_clus);
        return 32;
    }

    return 0;
}

/* reads the Master Boot Record to get FAT information */
int
fat_read_mbr(mx6x_sdmmc_t *sdmmc, int verbose)
{
    mbr_t           *mbr;
    bpb_t           *bpb;
    partition_t     *pe;
    unsigned short  sign;

    blk = (unsigned char *)blk_buf;
    mbr = (mbr_t *)&blk[0];
    bpb = (bpb_t *)&blk[SECTOR_SIZE];
    pe  = (partition_t *)&(mbr->part_entry[0]);

    /* read MBR from sector 0 */
    if (SDMMC_OK != read_sector(sdmmc, 0, mbr, 1)) {
        ser_putstr("   Error: cannot read MBR\n");
        return SDMMC_ERROR;
    }

    if ((sign = mbr->sign) != 0xaa55) {
        ser_putstr("   Error: MBR signature (");  ser_puthex((unsigned int)sign);   ser_putstr(") is wrong\n");
        return SDMMC_ERROR;
    }

    if (GET_LONG(pe->part_size) == 0) {
        ser_putstr("   Error: No information in partition 0.\n");
        return SDMMC_ERROR;
    }

    /* read BPB structure */
    fs_info.device = sdmmc;
    fs_info.fs_offset = GET_LONG(pe->part_offset);
    if (read_fsector(0, (unsigned char *)bpb, 1)) {
        ser_putstr("   Error: cannot read BPB\n");
        return SDMMC_ERROR;
    }

    /* detect the FAT type of partition 0 */
    if (-1 == (fs_info.fat_type = fat_detect_type(bpb))) {
        ser_putstr("   error detecting BPB type\n");
        return SDMMC_ERROR;
    }

    if (verbose) {
        ser_putstr("Partition entry 0:");

        ser_putstr("      Boot Indicator: ");
            ser_puthex((unsigned int)mbr->part_entry[0].boot_ind); ser_putstr("\n");

        ser_putstr("      FAT type:       ");
            ser_puthex((unsigned int)fs_info.fat_type); ser_putstr("\n");

        ser_putstr("      Partition type: ");
            ser_puthex((unsigned int)pe->os_type); ser_putstr("\n");

        ser_putstr("      Begin C_H_S:    ");
            ser_puthex((unsigned int)pe->beg_head); ser_putstr(" ");
            ser_puthex((unsigned int)pe->begin_sect); ser_putstr(" ");
            ser_puthex((unsigned int)pe->beg_cylinder); ser_putstr("\n");

        ser_putstr("      END C_H_S:      ");
            ser_puthex((unsigned int)pe->end_head); ser_putstr(" ");
            ser_puthex((unsigned int)pe->end_sect); ser_putstr(" ");
            ser_puthex((unsigned int)pe->end_cylinder); ser_putstr("\n");

        ser_putstr("      Start sector:   ");
            ser_puthex((unsigned int)GET_LONG(pe->part_offset)); ser_putstr("\n");

        ser_putstr("      Partition size: ");
            ser_puthex((unsigned int)GET_LONG(pe->part_size)); ser_putstr("\n");
    }

    return SDMMC_OK;
}

/* converts a cluster number into sector numbers to the partition 0 */
static unsigned
cluster2fsector(unsigned cluster)
{
    return fs_info.cluster2_start + (cluster - 2) * fs_info.cluster_size;
}

/* gets the entry of the FAT for FAT12 */
static unsigned
get_fat_entry12(unsigned cluster)
{
    unsigned fat_sector = fs_info.fat1_start + ((cluster + (cluster / 2)) / SECTOR_SIZE);
    unsigned fat_offs   = (cluster + (cluster / 2)) % SECTOR_SIZE;

    if (SDMMC_OK != read_fsector(fat_sector, blk, 2))
        return 0;

    if (cluster & 0x1)
        return GET_WORD(&blk[fat_offs]) >> 4;
    else
        return GET_WORD(&blk[fat_offs]) & 0xfff;
}

/* gets the entry of the FAT for FAT16 */
static unsigned
get_fat_entry16(unsigned cluster)
{
    unsigned       fat_sector = fs_info.fat1_start + ((cluster * 2) / SECTOR_SIZE);
    unsigned       fat_offs   = (cluster * 2) % SECTOR_SIZE;
    unsigned char *data_buf   = (unsigned char *)blk_buf;

	if(cache_valid && g_fat_sector == fat_sector){
		return *(unsigned short *)(data_buf + fat_offs);
	}
    if (SDMMC_OK != read_fsector(fat_sector, data_buf, 1))
        return 0;

	g_fat_sector = fat_sector;

   return *(unsigned short *)(data_buf + fat_offs);
}

/* gets the entry of the FAT for FAT32 */
static unsigned
get_fat_entry32(unsigned cluster)
{
    unsigned        fat_sector   = fs_info.fat1_start + ((cluster * 4) / SECTOR_SIZE);
    unsigned        fat_offs  = (cluster * 4) % SECTOR_SIZE;
    unsigned char   *data_buf = (unsigned char *)blk_buf;

	if(cache_valid && g_fat_sector == fat_sector){
		return (*(unsigned long *)(data_buf + fat_offs)) & 0x0fffffff;
	}
    if (SDMMC_OK != read_fsector(fat_sector, data_buf, 1))
        return 0;
	g_fat_sector = fat_sector;
    return (*(unsigned long *)(data_buf + fat_offs)) & 0x0fffffff;
}

/* gets the entry of the FAT for the given cluster number */
static unsigned
fat_get_fat_entry(unsigned cluster)
{
    switch (fs_info.fat_type)
    {
    case 12:
        return get_fat_entry12(cluster);
        break;

    case 16:
        return get_fat_entry16(cluster);
        break;

    case 32:
        return get_fat_entry32(cluster);
        break;

    default:
      return 0;
      break;
    }
}

/* checks for end of file condition */
static int
end_of_file (unsigned cluster)
{
    switch (fs_info.fat_type)
    {
    case 12:
        return ((cluster == 0x0ff8) || (cluster == 0x0fff));
        break;

    case 16:
        return ((cluster == 0xfff8) || (cluster == 0xffff));
        break;

    case 32:
        return ((cluster == 0x0ffffff8) || (cluster == 0x0fffffff));
        break;

    default:
        return 1;
        break;
    }
}

/* read a cluster */
#if 0
static int
read_cluster(unsigned cluster, unsigned char *buf, int size)
{
    unsigned sectors = (size / SECTOR_SIZE) >= fs_info.cluster_size ?
                            fs_info.cluster_size : size / SECTOR_SIZE;
    unsigned rest    = (size / SECTOR_SIZE) >= fs_info.cluster_size ?
                            0 : size % SECTOR_SIZE;

    if (sectors) {
        if (SDMMC_OK != read_fsector(cluster2fsector(cluster), buf, sectors)) {
            return SDMMC_ERROR;
        }
    }

    if (rest) {
        if (SDMMC_OK != read_fsector(cluster2fsector(cluster) + sectors, blk, 1)) {
            return SDMMC_ERROR;
        }
        memcpy(buf + sectors * SECTOR_SIZE, blk, rest);
    }

    return SDMMC_OK;
}
#endif

/*  copy a file from to a memory location */
static int
fat_copy_file(unsigned cluster, unsigned size, unsigned char *buf)
{
  #if 1
    int        result, txf;
    unsigned   prev_c, next_c, curr_c;
    int     sz  = (int)size;
    int     cbytes = fs_info.cluster_size*SECTOR_SIZE;

#if 0
     ser_putstr((char *)"fat_copy_file: cs %x");
     ser_puthex(fs_info.cluster_size);
     ser_putstr((char *)" size %x");
     ser_puthex(sz);
     ser_putstr((char *)"type %x");
     ser_puthex(fs_info.fat_type );
     ser_putstr((char *)"\n");
#endif

#if 1
	cache_valid = 1;
#endif
	g_fat_sector = -1;
	
    /*
    *Note that this impl assume the following:
    * 1) The max DMA transfer size is bigger than the max consolidate transfer size
    * Otherwise, we need to break down into smaller transfer.
    * 2) we always do at least one whole cluster transfer. This might overwrite the client buffer, but
    * since this is purely used for IPL, we don't care about that now.
    */
    curr_c = cluster;
    while(sz>0){
        txf = cbytes;
        prev_c = curr_c;
        while(sz>txf){
            //try consolidate contigus entry;
#if 0
			ser_putstr((char *)"Get FatEntry: %x");
			ser_puthex(curr_c);
			ser_putstr((char *)"\n");
#endif
            next_c = fat_get_fat_entry(curr_c);
            if(next_c == (curr_c+1)){
                txf +=cbytes;
                curr_c = next_c;
            }else{
                curr_c = next_c;
                break;
            }
        }
#if 0
        ser_putstr((char *)"blkcnt %x");
        ser_puthex(txf/SECTOR_SIZE);
        ser_putstr((char *)" p %x");
        ser_puthex(prev_c);
        ser_putstr((char *)" n %x");
        ser_puthex(curr_c);
        ser_putstr((char *)"\n");
#endif
        //read the contig cluster out
        result= read_fsector(cluster2fsector(prev_c), buf, txf/SECTOR_SIZE) ;
        if (result != SDMMC_OK)
           return result;
        sz     -= txf;
        buf  += txf;
    }
#else

  int     sz  = (int)size;

    while(!end_of_file(cluster) && (sz > 0)) {
        int txf = MIN(sz, fs_info.cluster_size * SECTOR_SIZE);

        if (SDMMC_OK != read_cluster(cluster, buf, txf)) {
            return SDMMC_ERROR;
        }
        ser_putstr((char *)"cluster %x");
        ser_puthex(cluster);
        ser_putstr((char *)"\n");

        sz   -= txf;
        buf  += txf;
        cluster = fat_get_fat_entry(cluster);
    }
#endif

    return SDMMC_OK;
}

/* copy file by name (FAT12/16) */
static int
copy_named_file_fat16(unsigned char *buf, char *name)
{
    unsigned    dir_sector = fs_info.root_dir_start;
    int         i, len;
    int         ent = 0;

    while(dir_sector <  fs_info.root_dir_start + fs_info.root_dir_sectors)
    {
        if (SDMMC_OK != read_fsector(dir_sector, blk, 1)) {
            ser_putstr("read_fsector failed!");
            return SDMMC_ERROR;
        }

        ent = 0;
        while (ent < SECTOR_SIZE / sizeof(dir_entry_t)) {
            dir_entry_t *dir_entry = (dir_entry_t *)&(blk[ent * sizeof(dir_entry_t)]);

            if (dir_entry->short_name[0]== ENT_END)
                break;

            if (dir_entry->short_name[0]== ENT_UNUSED) {
                ent++;
                continue;
            }

            len = strlen (name) < 11 ? strlen (name) : 11;
            for (i = 0; i < len; i++) {
                if (name[i] != dir_entry->short_name[i])
                break;
            }

            if ((i < len) || (dir_entry->clust_lo == 0)) {
                ent++;
                continue;
            }

            return fat_copy_file(dir_entry->clust_lo, dir_entry->size, buf);
        }

        if (ent < SECTOR_SIZE / sizeof(dir_entry_t))
            break;

        dir_sector++;
    }

    return SDMMC_ERROR;;
}

/* copy file by name (FAT32) */
static int
copy_named_file_fat32(unsigned char *buf, char *name)
{
    unsigned    dir_clust = fs_info.root_dir_start;
    unsigned    clust_sz  = fs_info.cluster_size;
    unsigned    dir_sector;
    int         i, len;
    int         ent = 0;

    while(!end_of_file(dir_clust)) {
        int sub_sect = 0;
        dir_sector = cluster2fsector(dir_clust);

        while(sub_sect < clust_sz) {
            if (SDMMC_OK != read_fsector(dir_sector + sub_sect, blk, 1)) {
                return SDMMC_ERROR;
            }

            ent = 0;
            while (ent < SECTOR_SIZE / sizeof(dir_entry_t)) {
                dir_entry_t *dir_entry = (dir_entry_t *)&(blk[ent * sizeof(dir_entry_t)]);

                if (dir_entry->short_name[0]== ENT_END)
                    break;

                if (dir_entry->short_name[0]== ENT_UNUSED) {
                    ent++;
                    continue;
                }

                len = strlen (name) < 11 ? strlen (name) : 11;
                for (i = 0; i < len; i++) {
                    if (name[i] != dir_entry->short_name[i])
                        break;
                }

                if ((i < len) || (GET_CLUSTER(dir_entry) == 0)) {
                    ent++;
                    continue;
                }
                return fat_copy_file(GET_CLUSTER(dir_entry), dir_entry->size, buf);
            }

            if (ent < SECTOR_SIZE / sizeof(dir_entry_t))
                break;

            sub_sect++;
        }

        dir_clust = fat_get_fat_entry(dir_clust);
    }

    return SDMMC_ERROR;
}

/* copy file by name */
int
fat_copy_named_file(unsigned char *buf, char *name)
{
    switch (fs_info.fat_type)
    {
    case 12:
    case 16:
        if(SDMMC_OK != copy_named_file_fat16(buf, name)) {
            ser_putstr("copy_named_file_fat16 failed!\n");
            return SDMMC_ERROR;
        }
        return SDMMC_OK;
        break;

    case 32:
        if(SDMMC_OK != copy_named_file_fat32(buf, name)) {
            ser_putstr("copy_named_file_fat32 failed!\n");
            return SDMMC_ERROR;
        }
        return SDMMC_OK;
        break;

    default:
        ser_putstr("Unsupport file system: "); ser_puthex(fs_info.fat_type); ser_putstr(" \n");
        return SDMMC_ERROR;
        break;
   }
}




#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/ipl/boards/mx6q-sabrelite/fat-fs.c $ $Rev: 740617 $")
#endif
