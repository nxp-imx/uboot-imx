/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef __DDR4_CONFIG_H__
#define __DDR4_CONFIG_H__

#include "../ddr.h"

#define RUN_ON_SILICON
#define DDR4_SW_FFC
#define ENABLE_RETENTION

#define DRAM_VREF 0x1f

#define SAVE_DDRPHY_TRAIN_ADDR 0x184000

/* choose p2 state data rate, define just one of below macro */
#define PLLBYPASS_400MBPS

/* //////////////////////////////////////////////////////////////////////////////////////////////////////////////////// */
/* for DDR4 */
/* Note:DQ SI RON=40ohm, RTT=48ohm */
/*      CA SI RON=40ohm, RTT=65ohm */
/* //////////////////////////////////////////////////////////////////////////////////////////////////////////////////// */
/* for DDR RTT NOM/PARK */
#define DDR4_ODT_DIS 0
#define DDR4_ODT_60  1
#define DDR4_ODT_120 2
#define DDR4_ODT_40  3
#define DDR4_ODT_240 4
#define DDR4_ODT_48  5
#define DDR4_ODT_80  6
#define DDR4_ODT_34  7

/* for DDR RON */
#define DDR4_RON_34  0
#define DDR4_RON_48  1
#define DDR4_RON_40  2
#define DDR4_RON_RES 3

/* for DDR RTT write */
#define DDR4_RTT_WR_DIS 0
#define DDR4_RTT_WR_120 1
#define DDR4_RTT_WR_240 2
#define DDR4_RTT_WR_HZ  3
#define DDR4_RTT_WR_80  4

/* for DDR4 PHY data RON */
#define DDR4_PHY_DATA_RON_34    0xeba
#define DDR4_PHY_DATA_RON_40    0xe38
#define DDR4_PHY_DATA_RON_48    ((0x1a << 6) | 0x1a)
#define DDR4_PHY_DATA_RON_60    ((0x18 << 6) | 0x18)
#define DDR4_PHY_DATA_RON_80    ((0x0a << 6) | 0x0a)
#define DDR4_PHY_DATA_RON_120   ((0x08 << 6) | 0x08)
#define DDR4_PHY_DATA_RON_240   ((0x02<<6)|0x02)

/* for DDR4 PHY data RTT */
#define DDR4_PHY_DATA_RTT_34    0x3a
#define DDR4_PHY_DATA_RTT_40    0x38
#define DDR4_PHY_DATA_RTT_48    0x1a
#define DDR4_PHY_DATA_RTT_60    0x18
#define DDR4_PHY_DATA_RTT_80    0x0a
#define DDR4_PHY_DATA_RTT_120   0x08
#define DDR4_PHY_DATA_RTT_240   0x02

/* for DDR4 PHY address RON */
#define DDR4_PHY_ADDR_RON_30    ((0x07 << 5) | 0x07)
#define DDR4_PHY_ADDR_RON_40    0x63
#define DDR4_PHY_ADDR_RON_60    ((0x01 << 5) | 0x01)
#define DDR4_PHY_ADDR_RON_120   ((0x00 << 5) | 0x00)

#define DDR4_PHY_ADDR_RON DDR4_PHY_ADDR_RON_40

/* read DDR4 */
#ifdef DDR_ONE_RANK
#define DDR4_RON            DDR4_RON_34
#define DDR4_PHY_DATA_RTT   DDR4_PHY_DATA_RTT_48
#define DDR4_PHYREF_VALUE   91
#else
#define DDR4_RON            DDR4_RON_40
#define DDR4_PHY_DATA_RTT   DDR4_PHY_DATA_RTT_48
#define DDR4_PHYREF_VALUE   93
#endif

/* write DDR4 */
#ifdef DDR_ONE_RANK
/* one lank */
#define DDR4_PHY_DATA_RON DDR4_PHY_DATA_RON_34
#define DDR4_RTT_NOM      DDR4_ODT_60
#define DDR4_RTT_WR       DDR4_RTT_WR_DIS
#define DDR4_RTT_PARK     DDR4_ODT_DIS
#define DDR4_MR6_VALUE    0x0d
#else
/* two lank */
#define DDR4_PHY_DATA_RON DDR4_PHY_DATA_RON_40
#define DDR4_RTT_NOM      DDR4_ODT_60
#define DDR4_RTT_WR       DDR4_RTT_WR_DIS
#define DDR4_RTT_PARK     DDR4_ODT_DIS
#define DDR4_MR6_VALUE    0x10
#endif

/* voltage:delay */
#define DDR4_2D_WEIGHT (31 << 8 | 127)

#define ANAMIX_PLL_BASE_ADDR         0x30360000
#define HW_DRAM_PLL_CFG0_ADDR (ANAMIX_PLL_BASE_ADDR + 0x60)
#define HW_DRAM_PLL_CFG1_ADDR (ANAMIX_PLL_BASE_ADDR + 0x64)
#define HW_DRAM_PLL_CFG2_ADDR (ANAMIX_PLL_BASE_ADDR + 0x68)
#define GPC_PU_PWRHSK 0x303A01FC
#define GPC_TOP_CONFIG_OFFSET        0x0000
#define AIPS1_ARB_BASE_ADDR             0x30000000
#define AIPS_TZ1_BASE_ADDR              AIPS1_ARB_BASE_ADDR
#define AIPS1_OFF_BASE_ADDR             (AIPS_TZ1_BASE_ADDR + 0x200000)
#define CCM_IPS_BASE_ADDR              (AIPS1_OFF_BASE_ADDR + 0x180000)
#define CCM_SRC_CTRL_OFFSET     (CCM_IPS_BASE_ADDR + 0x800)
#define CCM_SRC_CTRL(n)             (CCM_SRC_CTRL_OFFSET + 0x10 * n)


#define dwc_ddrphy_apb_wr(addr, data)  reg32_write(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4 * (addr), data)
#define dwc_ddrphy_apb_rd(addr)        (reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4 * (addr)))
#define  reg32clrbit(addr, bitpos)       reg32_write((addr), (reg32_read((addr)) & (0xFFFFFFFF ^ (1 << (bitpos)))))
#define DDR_CSD1_BASE_ADDR 0x40000000
#define DDR_CSD2_BASE_ADDR 0x80000000

void restore_1d2d_trained_csr_ddr4_p012(unsigned int addr);
void save_1d2d_trained_csr_ddr4_p012(unsigned int addr);
void ddr4_mr_write(unsigned int mr, unsigned int data, unsigned int read, unsigned int rank);
void ddr4_phyinit_train_sw_ffc(unsigned int Train2D);

#endif
