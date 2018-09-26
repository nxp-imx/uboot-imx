/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef DDR3_DEFINE_H
#define DDR3_DEFINE_H

#include "../ddr.h"

#define RUN_ON_SILICON

#ifdef DDR3_1600MTS_SWFFC_RET
	#define DDR3_SW_FFC
#endif

#define SAVE_DDRPHY_TRAIN_ADDR 0x180000
#define DDR_CSD1_BASE_ADDR 0x40000000
#define DDR_CSD2_BASE_ADDR 0x80000000

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

void restore_1d2d_trained_csr_ddr3_p012(unsigned int addr);
void save_1d2d_trained_csr_ddr3_p012(unsigned int addr);
void ddr3_phyinit_train_sw_ffc(unsigned int after_retention);

#endif
