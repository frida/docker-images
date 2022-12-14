/*
 * Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.  */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA  */

/*!
 * @file sdma_script_code.h
 * @brief This file contains functions of SDMA scripts code initialization
 * 
 * The file was generated automatically. Based on sdma scripts library.
 * 
 * @ingroup SDMA
 */
/************************************************************************************

           SDMA RELEASE LABEL:     "SDMA_MX61_REV_1.1"

*************************************************************************************/
 

#ifndef SDMA_MICROCODE_H
#define SDMA_MICROCODE_H

#include <stdint.h>

/*
 * SDMA RAM image start address and size
 */

// SDMA RAM image start address and size
#define SDMA_RAM_CODE_START_ADDR     6144

// Buffer that holds the SDMA RAM image
static const uint16_t sdma_code[] = {
0xc1e3, 0x57db, 0x5fe3, 0x57e3, 0x52f3, 0x6a01, 0x008f, 0x00d5,
0x7d01, 0x008d, 0x05a0, 0x5deb, 0x0478, 0x7d03, 0x0479, 0x7d2c,
0x7c36, 0x0479, 0x7c1f, 0x56ee, 0x0f00, 0x0660, 0x7d05, 0x6509,
0x7e43, 0x620a, 0x7e41, 0x9820, 0x620a, 0x7e3e, 0x6509, 0x7e3c,
0x0512, 0x0512, 0x02ad, 0x0760, 0x7d03, 0x55fb, 0x6dd3, 0x982b,
0x55fb, 0x1d04, 0x6dd3, 0x6ac8, 0x7f2f, 0x1f01, 0x2003, 0x4800,
0x7ce4, 0x9853, 0x55fb, 0x6dd7, 0x0015, 0x7805, 0x6209, 0x6ac8,
0x6209, 0x6ac8, 0x6dd7, 0x9852, 0x55fb, 0x6dd7, 0x0015, 0x0015,
0x7805, 0x620a, 0x6ac8, 0x620a, 0x6ac8, 0x6dd7, 0x9852, 0x55fb,
0x6dd7, 0x0015, 0x0015, 0x0015, 0x7805, 0x620b, 0x6ac8, 0x620b,
0x6ac8, 0x6dd7, 0x7c09, 0x6ddf, 0x7f07, 0x0000, 0x55eb, 0x4d00,
0x7d07, 0xc1fa, 0x57e3, 0x9806, 0x0007, 0x68cc, 0x680c, 0xc213,
0xc20a, 0x9803, 0xc1d9, 0xc1e3, 0x57db, 0x5fe3, 0x57e3, 0x52f3,
0x6a21, 0x008f, 0x00d5, 0x7d01, 0x008d, 0x05a0, 0x5deb, 0x56fb,
0x0478, 0x7d03, 0x0479, 0x7d2a, 0x7c31, 0x0479, 0x7c20, 0x0b70,
0x0311, 0x53eb, 0x0f00, 0x0360, 0x7d05, 0x6509, 0x7e37, 0x620a,
0x7e35, 0x9886, 0x620a, 0x7e32, 0x6509, 0x7e30, 0x0512, 0x0512,
0x02ad, 0x0760, 0x7c02, 0x5a06, 0x988e, 0x5a26, 0x7f27, 0x1f01,
0x2003, 0x4800, 0x7ce8, 0x0b70, 0x0311, 0x5313, 0x98af, 0x0015,
0x7804, 0x6209, 0x5a06, 0x6209, 0x5a26, 0x98ae, 0x0015, 0x0015,
0x7804, 0x620a, 0x5a06, 0x620a, 0x5a26, 0x98ae, 0x0015, 0x0015,
0x0015, 0x7804, 0x620b, 0x5a06, 0x620b, 0x5a26, 0x7c07, 0x0000,
0x55eb, 0x4d00, 0x7d06, 0xc1fa, 0x57e3, 0x9869, 0x0007, 0x680c,
0xc213, 0xc20a, 0x9866, 0x0b70, 0x0311, 0x5313, 0x076c, 0x7c01,
0xc1d9, 0x5efb, 0x068a, 0x076b, 0x7c01, 0xc1d9, 0x5ef3, 0x59db,
0x58d3, 0x018f, 0x0110, 0x390f, 0x008b, 0xc13c, 0x7d2b, 0x5ac0,
0x5bc8, 0xc14e, 0x7c27, 0x0388, 0x0689, 0x5ce3, 0x0dff, 0x0511,
0x1dff, 0x05bc, 0x073e, 0x4d00, 0x7d18, 0x0870, 0x0011, 0x077e,
0x7d09, 0x077d, 0x7d02, 0x5228, 0x98e6, 0x52f8, 0x54db, 0x02bc,
0x02cc, 0x7c09, 0x077c, 0x7d02, 0x5228, 0x98ef, 0x52f8, 0x54d3,
0x02bc, 0x02cc, 0x7d09, 0x0400, 0x98dd, 0x008b, 0x52c0, 0x53c8,
0xc159, 0x7dd6, 0x0200, 0x98cd, 0x08ff, 0x00bf, 0x077f, 0x7d15,
0x0488, 0x00d5, 0x7d01, 0x008d, 0x05a0, 0x5deb, 0x028f, 0x0212,
0x0212, 0x3aff, 0x05da, 0x7c02, 0x073e, 0x9918, 0x02a4, 0x02dd,
0x7d02, 0x073e, 0x9918, 0x075e, 0x9918, 0x55eb, 0x0598, 0x5deb,
0x52f3, 0x54fb, 0x076a, 0x7d26, 0x076c, 0x7d01, 0x9955, 0x076b,
0x7c57, 0x0769, 0x7d04, 0x0768, 0x7d02, 0x0e01, 0x992f, 0x5893,
0x00d6, 0x7d01, 0x008e, 0x5593, 0x05a0, 0x5d93, 0x06a0, 0x7802,
0x5502, 0x5d04, 0x7c1d, 0x4e00, 0x7c08, 0x0769, 0x7d03, 0x5502,
0x7e17, 0x993c, 0x5d04, 0x7f14, 0x0689, 0x5093, 0x4800, 0x7d01,
0x9927, 0x99a0, 0x0015, 0x7806, 0x5502, 0x5d04, 0x074f, 0x5502,
0x5d24, 0x072f, 0x7c01, 0x99a0, 0x0017, 0x076f, 0x7c01, 0x2001,
0x5593, 0x009d, 0x0007, 0xd9a7, 0x98f5, 0x6cd3, 0x0769, 0x7d04,
0x0768, 0x7d02, 0x0e01, 0x9964, 0x5893, 0x00d6, 0x7d01, 0x008e,
0x5593, 0x05a0, 0x5d93, 0x06a0, 0x7802, 0x5502, 0x6dc8, 0x7c0f,
0x4e00, 0x7c08, 0x0769, 0x7d03, 0x5502, 0x7e09, 0x9971, 0x6dc8,
0x7f06, 0x0689, 0x5093, 0x4800, 0x7d01, 0x995c, 0x99a0, 0x999a,
0x6ac3, 0x0769, 0x7d04, 0x0768, 0x7d02, 0x0e01, 0x9987, 0x5893,
0x00d6, 0x7d01, 0x008e, 0x5593, 0x05a0, 0x5d93, 0x06a0, 0x7802,
0x65c8, 0x5d04, 0x7c0f, 0x4e00, 0x7c08, 0x0769, 0x7d03, 0x65c8,
0x7e09, 0x9994, 0x5d04, 0x7f06, 0x0689, 0x5093, 0x4800, 0x7d01,
0x997f, 0x99a0, 0x5593, 0x009d, 0x0007, 0x6cff, 0xd9a7, 0x98f5,
0x0000, 0x54e3, 0x55eb, 0x4d00, 0x7c01, 0x98f5, 0x98dd, 0x54e3,
0x55eb, 0x0aff, 0x0211, 0x1aff, 0x077f, 0x7c02, 0x05a0, 0x99b4,
0x009d, 0x058c, 0x05ba, 0x05a0, 0x0210, 0x04ba, 0x04ad, 0x0454,
0x0006, 0xc1e3, 0x57db, 0x52fb, 0x6ac3, 0x52f3, 0x6a05, 0x008f,
0x00d5, 0x7d01, 0x008d, 0x05a0, 0x5deb, 0x0478, 0x7d03, 0x0479,
0x7d2b, 0x7c1e, 0x0479, 0x7c33, 0x56ee, 0x0f00, 0x55fb, 0x0760,
0x7d02, 0x6dc3, 0x99d5, 0x1d04, 0x6dc3, 0x62c8, 0x7e3b, 0x0660,
0x7d02, 0x0210, 0x0212, 0x6a09, 0x7f35, 0x0212, 0x6a09, 0x7f32,
0x0212, 0x6a09, 0x7f2f, 0x1f01, 0x2003, 0x4800, 0x7ce7, 0x9a09,
0x55fb, 0x6dc7, 0x0015, 0x0015, 0x0015, 0x7805, 0x62c8, 0x6a0b,
0x62c8, 0x6a0b, 0x6dc7, 0x9a08, 0x55fb, 0x6dc7, 0x0015, 0x0015,
0x7805, 0x62c8, 0x6a0a, 0x62c8, 0x6a0a, 0x6dc7, 0x9a08, 0x55fb,
0x6dc7, 0x0015, 0x7805, 0x62c8, 0x6a09, 0x62c8, 0x6a09, 0x6dc7,
0x7c09, 0x6a28, 0x7f07, 0x0000, 0x55eb, 0x4d00, 0x7d05, 0xc1fa,
0x57db, 0x99bf, 0xc277, 0x0454, 0xc20a, 0x99ba, 0xc1d9, 0xc1e3,
0x57db, 0x52f3, 0x6a05, 0x008f, 0x00d5, 0x7d01, 0x008d, 0x05a0,
0x56fb, 0x0478, 0x7d03, 0x0479, 0x7d29, 0x7c1f, 0x0479, 0x7c2e,
0x5de3, 0x0d70, 0x0511, 0x55ed, 0x0f00, 0x0760, 0x7d02, 0x5206,
0x9a32, 0x5226, 0x7e33, 0x0560, 0x7d02, 0x0210, 0x0212, 0x6a09,
0x7f2d, 0x0212, 0x6a09, 0x7f2a, 0x0212, 0x6a09, 0x7f27, 0x1f01,
0x2003, 0x4800, 0x7cea, 0x55e3, 0x9a5d, 0x0015, 0x0015, 0x0015,
0x7804, 0x5206, 0x6a0b, 0x5226, 0x6a0b, 0x9a5c, 0x0015, 0x0015,
0x7804, 0x5206, 0x6a0a, 0x5226, 0x6a0a, 0x9a5c, 0x0015, 0x7804,
0x5206, 0x6a09, 0x5226, 0x6a09, 0x7c09, 0x6a28, 0x7f07, 0x0000,
0x57db, 0x4d00, 0x7d05, 0xc1fa, 0x57db, 0x9a1b, 0xc277, 0x0454,
0xc20a, 0x9a18, 0xc1e3, 0x57db, 0x52f3, 0x6a05, 0x56fb, 0x028e,
0x1a94, 0x6ac3, 0x62c8, 0x0269, 0x7d24, 0x1e94, 0x6ec3, 0x6ed3,
0x62c8, 0x0248, 0x6ac8, 0x2694, 0x1e98, 0x6ec3, 0x6ed3, 0x62c8,
0x024c, 0x6ac8, 0x2698, 0x6ec3, 0x1e98, 0x6ec3, 0x62c8, 0x2698,
0x6ec3, 0x0260, 0x7c09, 0x62c8, 0x026e, 0x7d24, 0x6a09, 0x7f1e,
0x2501, 0x4d00, 0x7d25, 0x9a84, 0x6a28, 0x7f18, 0x6204, 0xc27a,
0x9ab8, 0x6ee3, 0x008f, 0x05d8, 0x7d01, 0x008d, 0x05a0, 0x62c8,
0x026e, 0x7d10, 0x6a09, 0x7f0a, 0x2001, 0x7cf9, 0x6a28, 0x7f06,
0x0000, 0x4d00, 0x7d0d, 0xc1fa, 0x57db, 0x9a6e, 0x0007, 0x6204,
0x6a0c, 0x9ab5, 0x6a28, 0x7ffa, 0x6204, 0xc27a, 0x0458, 0x0454,
0x6a28, 0x7ff4, 0xc20a, 0x9a6b, 0xc1d9, 0xc1e3, 0x57db, 0x52f3,
0x6a05, 0x56fb, 0x028e, 0x1a94, 0x5202, 0x0269, 0x7d1d, 0x1e94,
0x5206, 0x0248, 0x5a06, 0x2694, 0x1e98, 0x5206, 0x024c, 0x5a06,
0x2698, 0x1e98, 0x5206, 0x0260, 0x7c0a, 0x2698, 0x5206, 0x026e,
0x7d23, 0x6a09, 0x7f1d, 0x2501, 0x4d00, 0x7d24, 0x9ad1, 0x6a28,
0x7f17, 0x6204, 0xc27a, 0x9b02, 0x008f, 0x05d8, 0x7d01, 0x008d,
0x05a0, 0x5206, 0x026e, 0x7d10, 0x6a09, 0x7f0a, 0x2001, 0x7cf9,
0x6a28, 0x7f06, 0x0000, 0x4d00, 0x7d0d, 0xc1fa, 0x57db, 0x9ac1,
0x0007, 0x6204, 0x6a0c, 0x9aff, 0x6a28, 0x7ffa, 0x6204, 0xc27a,
0x0458, 0x0454, 0x6a28, 0x7ff4, 0xc20a, 0x9abe, 0x6e01, 0x610b,
0x7e2f, 0x620b, 0x7e2d, 0x630b, 0x7e2b, 0x0d0c, 0x0417, 0x0417,
0x0417, 0x049d, 0x1d08, 0x05cc, 0x7c01, 0x0d0c, 0x6ad1, 0x0f00,
0x0742, 0x6fc8, 0x6fdd, 0x7f1c, 0x008e, 0x009d, 0x6801, 0x670b,
0x7e17, 0x6bd5, 0x0804, 0x7802, 0x6fc8, 0x0712, 0x7c11, 0x670b,
0x7e0f, 0x0804, 0x7802, 0x6fc8, 0x0712, 0x7c0a, 0x6fdd, 0x7f08,
0x69d1, 0x0f01, 0x6fc8, 0x6fdd, 0x7f03, 0x0101, 0x0400, 0x9b12,
0x0007, 0x68ff, 0x680c, 0x0200, 0x9b12
};
#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/lib/dma/sdma/imx6x/microcode.h $ $Rev: 733037 $")
#endif
