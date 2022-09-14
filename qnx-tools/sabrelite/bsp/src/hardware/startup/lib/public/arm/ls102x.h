/*
 * $QNXLicenseC:
 * Copyright 2014, QNX Software Systems.
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

#ifndef __ARM_LS102X_H_INCLUDED
#define __ARM_LS102X_H_INCLUDED

/* Based on LS1021A QorIQ Advanced Multicore Processor Reference Manual
 * Document Number: LS1021ARM
 * Rev D, 09/2014
 */

/*********************
 * System Memory Map *
 *********************/

/* 32b accessible                                 44332211 */
#define LS102X_BOOTROM_BASE                     0x00000000UL
#define LS102X_EXTENDED_BOOTROM_BASE            0x00100000UL
#define LS102X_CCSR_BASE                        0x01000000UL
#define LS102X_OCRAM1_BASE                      0x10000000UL
#define LS102X_OCRAM2_BASE                      0x10010000UL
#define LS102X_QSPI_BASE                        0x40000000UL
#define LS102X_IFC1_BASE                        0x60000000UL
#define LS102X_DRAM1_BASE                       0x80000000UL
/* 36b accessible                                544332211 */
#define LS102X_IFC2_BASE                       0x620000000ULL
#define LS102X_DRAM2_BASE                      0x880000000ULL
/* 40b accessible                               5544332211 */
#define LS102X_PCIE1_BASE                     0x4000000000ULL
#define LS102X_PCIE2_BASE                     0x4800000000ULL
#define LS102X_DRAM3_BASE                     0x8800000000ULL

/**************************
 * CCSR Block Address Map *
 **************************                        4332211 */
#define LS102X_CCSR_DDR_CTRL_BASE               (0x0080000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_CCI_BASE                    (0x0180000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_SMMU1_BASE                  (0x0200000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_SMMU2_BASE                  (0x0280000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_SMMU3_BASE                  (0x0300000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_SMMU4_BASE                  (0x0380000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_GIC_BASE                    (0x0400000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_TZASC_BASE                  (0x0500000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_CSU_BASE                    (0x0510000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_PLATFORM_CTRL_BASE          (0x0520000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_IFC_BASE                    (0x0530000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_QSPI_BASE                   (0x0550000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_SDHC_BASE                   (0x0560000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_SCFG_BASE                   (0x0570000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_PBL_BASE                    (0x0610000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_SEC_BASE                    (0x0700000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_SFP_BASE                    (0x0E80000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_SECMON_BASE                 (0x0E90000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_SERDES_CTRL_1_BASE          (0x0EA0000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_DCFG_BASE                   (0x0EE0000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_CLK_BASE                    (0x0EE1000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_RCPM_BASE                   (0x0EE2000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_SPI1_BASE                   (0x1100000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_SPI2_BASE                   (0x1110000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_I2C1_BASE                   (0x1180000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_I2C2_BASE                   (0x1190000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_I2C3_BASE                   (0x11A0000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_DUART1_BASE                 (0x11C0000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_DUART2_BASE                 (0x11D0000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_GPIO1_BASE                  (0x1300000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_GPIO2_BASE                  (0x1310000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_GPIO3_BASE                  (0x1320000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_GPIO4_BASE                  (0x1330000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_uQE_BASE                    (0x1400000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_LPUART1_BASE                (0x1950000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_LPUART2_BASE                (0x1960000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_LPUART3_BASE                (0x1970000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_LPUART4_BASE                (0x1980000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_LPUART5_BASE                (0x1990000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_LPUART6_BASE                (0x19A0000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_FTM1_BASE                   (0x19D0000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_FTM2_BASE                   (0x19E0000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_FTM3_BASE                   (0x19F0000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_FTM4_BASE                   (0x1A00000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_FTM5_BASE                   (0x1A10000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_FTM6_BASE                   (0x1A20000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_FTM7_BASE                   (0x1A30000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_FTM8_BASE                   (0x1A40000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_FCAN1_BASE                  (0x1A70000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_FCAN2_BASE                  (0x1A80000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_FCAN3_BASE                  (0x1A90000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_FCAN4_BASE                  (0x1AA0000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_WDOG1_BASE                  (0x1AD0000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_WDOG2_BASE                  (0x1AE0000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_SAI1_BASE                   (0x1B50000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_SAI2_BASE                   (0x1B60000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_SAI3_BASE                   (0x1B70000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_SAI4_BASE                   (0x1B80000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_SPDIF_BASE                  (0x1BA0000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_ASRC_BASE                   (0x1BD0000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_EDMA_BASE                   (0x1C00000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_DMACHMUX1_BASE              (0x1C10000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_DMACHMUX2_BASE              (0x1C20000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_INTERCONNECT_BASE           (0x1CA0000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_2D_ACE_BASE                 (0x1CE0000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_ETSEC1_GROUP1_BASE          (0x1D10000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_ETSEC1_GROUP2_BASE          (0x1D14000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_ETSEC1_MDIO_BASE            (0x1D24000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_ETSEC2_GROUP1_BASE          (0x1D50000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_ETSEC2_GROUP2_BASE          (0x1D54000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_ETSEC2_MDIO_BASE            (0x1D64000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_ETSEC3_GROUP1_BASE          (0x1D90000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_ETSEC3_GROUP2_BASE          (0x1D94000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_ETSEC3_MDIO_BASE            (0x1DA4000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_USB3_BASE                   (0x2100000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_SATA_BASE                   (0x2200000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_PCIE1_CTRL_BASE             (0x2400000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_PCIE2_CTRL_BASE             (0x2500000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_QDMA_BASE                   (0x7390000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_USB3_PHY_BASE               (0x7510000UL+LS102X_CCSR_BASE)
#define LS102X_CCSR_USB2_BASE                   (0x7600000UL+LS102X_CCSR_BASE)

/******************************
 * Internal Interrupt Sources *
 ******************************/
#define LS102X_FTM5_IRQ                         ( 64)
#define LS102X_FTM6_IRQ                         ( 65)
#define LS102X_SMMU1_NON_SECURE_IRQ             (101)
#define LS102X_SMMU1_SECURE_IRQ                 (102)
#define LS102X_SMMU2_NON_SECURE_IRQ             (103)
#define LS102X_SMMU2_SECURE_IRQ                 (104)
#define LS102X_SMMU3_NON_SECURE_IRQ             (105)
#define LS102X_SMMU3_SECURE_IRQ                 (106)
#define LS102X_IFC_IRQ                          (106)
#define LS102X_LPUART1_IRQ                      (112)
#define LS102X_LPUART2_IRQ                      (113)
#define LS102X_LPUART3_IRQ                      (114)
#define LS102X_LPUART4_IRQ                      (115)
#define LS102X_LPUART5_IRQ                      (116)
#define LS102X_LPUART6_IRQ                      (117)
#define LS102X_DUART1_IRQ                       (118)
#define LS102X_DUART2_IRQ                       (119)
#define LS102X_I2C1_IRQ                         (120)
#define LS102X_I2C2_IRQ                         (121)
#define LS102X_I2C3_IRQ                         (122)
#define LS102X_PEX1_INTA_IRQ                    (123)
#define LS102X_PEX2_INTA_IRQ                    (124)
#define LS102X_USB30_IRQ                        (125)
#define LS102X_SDHC_IRQ                         (126)
#define LS102X_SPI1_IRQ                         (128)
#define LS102X_SPI2_IRQ                         (129)
#define LS102X_GPIO1_IRQ                        (130)
#define LS102X_GPIO2_IRQ                        (131)
#define LS102X_GPIO3_IRQ                        (132)
#define LS102X_SATA30_IRQ                       (133)
#define LS102X_EPU1_IRQ                         (134)
#define LS102X_SEC_JOBQ_1_IRQ                   (135)
#define LS102X_SEC_JOBQ_2_IRQ                   (136)
#define LS102X_SEC_JOBQ_3_IRQ                   (137)
#define LS102X_SEC_JOBQ_4_IRQ                   (138)
#define LS102X_SEC_GLOBAL_IRQ                   (139)
#define LS102X_PLATFORM_CTRL_IRQ                (140)
#define LS102X_uQE1_IRQ                         (141)
#define LS102X_SECMON_SECURE_IRQ                (142)
#define LS102X_SECMON_NON_SECURE_IRQ            (143)
#define LS102X_CSU_IRQ                          (144)
#define LS102X_ASRC_IRQ                         (145)
#define LS102X_SPDIF_IRQ                        (146)
#define LS102X_WDOG1_IRQ                        (147)
#define LS102X_WDOG2_IRQ                        (148)
#define LS102X_FTM1_IRQ                         (150)
#define LS102X_FTM2_IRQ                         (151)
#define LS102X_FTM3_IRQ                         (152)
#define LS102X_FTM4_IRQ                         (153)
#define LS102X_FTM7_IRQ                         (155)
#define LS102X_FTM8_IRQ                         (156)
#define LS102X_TZASC_IRQ                        (157)
#define LS102X_FCAN1_IRQ                        (158)
#define LS102X_FCAN2_IRQ                        (159)
#define LS102X_FCAN3_IRQ                        (160)
#define LS102X_FCAN4_IRQ                        (161)
#define LS102X_QSPI_IRQ                         (163)
#define LS102X_SAI1_IRQ                         (164)
#define LS102X_SAI2_IRQ                         (165)
#define LS102X_SAI3_IRQ                         (166)
#define LS102X_EDMA_IRQ                         (167)
#define LS102X_A7_CORE0_CTI_IRQ                 (168)
#define LS102X_A7_CORE1_CTI_IRQ                 (169)
#define LS102X_A7_CORE0_PMU_IRQ                 (170)
#define LS102X_A7_CORE1_PMU_IRQ                 (171)
#define LS102X_A7_AXI_ERROR_IRQ                 (172)
#define LS102X_CCI400_IRQ                       (173)
#define LS102X_ETSEC1_TX_GROUP1_IRQ             (176)
#define LS102X_ETSEC1_RX_GROUP1_IRQ             (177)
#define LS102X_ETSEC1_ERROR_GROUP1_IRQ          (178)
#define LS102X_ETSEC1_TX_GROUP2_IRQ             (179)
#define LS102X_ETSEC1_RX_GROUP2_IRQ             (180)
#define LS102X_ETSEC1_ERROR_GROUP2_IRQ          (181)
#define LS102X_ETSEC2_TX_GROUP1_IRQ             (182)
#define LS102X_SAI4_IRQ                         (183)
#define LS102X_ETSEC2_RX_GROUP1_IRQ             (184)
#define LS102X_ETSEC2_ERROR_GROUP1_IRQ          (185)
#define LS102X_ETSEC2_TX_GROUP2_IRQ             (186)
#define LS102X_ETSEC2_RX_GROUP2_IRQ             (187)
#define LS102X_ETSEC2_ERROR_GROUP2_IRQ          (188)
#define LS102X_ETSEC3_TX_GROUP1_IRQ             (189)
#define LS102X_ETSEC3_RX_GROUP1_IRQ             (190)
#define LS102X_ETSEC3_ERROR_GROUP1_IRQ          (191)
#define LS102X_ETSEC3_TX_GROUP2_IRQ             (192)
#define LS102X_ETSEC3_RX_GROUP2_IRQ             (193)
#define LS102X_ETSEC3_ERROR_GROUP2_IRQ          (194)
#define LS102X_0_IRQ                            (195)
#define LS102X_1_IRQ                            (196)
#define LS102X_2_IRQ                            (197)
#define LS102X_GPIO4_IRQ                        (198)
#define LS102X_3_IRQ                            (199)
#define LS102X_4_IRQ                            (200)
#define LS102X_5_IRQ                            (201)
#define LS102X_QDMA_IRQ                         (202)
#define LS102X_USB20_IRQ                        (203)
#define LS102X_2D_ACE_IRQ                       (204)
#define LS102X_1588_TIMER_IRQ                   (205)
#define LS102X_SMMU4_NON_SECURE_IRQ             (206)
#define LS102X_SMMU4_SECURE_IRQ                 (207)
#define LS102X_DDR_CTRL_IRQ                     (208)
#define LS102X_PCIE1_IRQ                        (209)
#define LS102X_PCIE2_IRQ                        (210)
#define LS102X_PEX1_MSI_IRQ                     (211)
#define LS102X_PEX2_MSI_IRQ                     (212)
#define LS102X_PEX1_PME_IRQ                     (213)
#define LS102X_PEX2_PME_IRQ                     (214)
#define LS102X_PEX1_CFG_ERR_IRQ                 (215)
#define LS102X_PEX2_CFG_ERR_IRQ                 (216)
#define LS102X_QDMA_QM_LITE_IRQ                 (217)
#define LS102X_PEX1_INTB_IRQ                    (220)
#define LS102X_PEX2_INTB_IRQ                    (221)
#define LS102X_PEX1_INTC_IRQ                    (222)
#define LS102X_PEX2_INTC_IRQ                    (223)
#define LS102X_PEX1_INTD_IRQ                    (224)
#define LS102X_PEX2_INTD_IRQ                    (225)
#define LS102X_PEX1_PM_DATA_IRQ                 (226)
#define LS102X_PEX2_PM_DATA_IRQ                 (227)
#define LS102X_SOFT_RESET_CORE0_IRQ             (228)
#define LS102X_SOFT_RESET_CORE1_IRQ             (229)
#define LS102X_DEBUG_EVENT0_IRQ                 (230)
#define LS102X_DEBUG_EVENT1_IRQ                 (231)

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/lib/public/arm/ls102x.h $ $Rev: 766657 $")
#endif
