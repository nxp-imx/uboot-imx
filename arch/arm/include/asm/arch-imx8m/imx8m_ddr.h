/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 * Common file for ddr code
 */

#ifndef __IMX8M_DDR_H__
#define __IMX8M_DDR_H__

#include <asm/io.h>
#include <asm/types.h>
#include <asm/arch/ddr_memory_map.h>

/* user data type */
enum fw_type {
	FW_1D_IMAGE,
	FW_2D_IMAGE,
};

struct dram_cfg_param {
	unsigned int reg;
	unsigned int val;
};

struct dram_fsp_msg {
	unsigned int drate;
	enum fw_type fw_type;
	struct dram_cfg_param *fsp_cfg;
	unsigned int fsp_cfg_num;
};

struct dram_timing_info {
	/* umctl2 config */
	struct dram_cfg_param *ddrc_cfg;
	unsigned int ddrc_cfg_num;
	/* ddrphy config */
	struct dram_cfg_param *ddrphy_cfg;
	unsigned int ddrphy_cfg_num;
	/* ddr fsp train info */
	struct dram_fsp_msg *fsp_msg;
	unsigned int fsp_msg_num;
	/* ddr phy trained CSR */
	struct dram_cfg_param *ddrphy_trained_csr;
	unsigned int ddrphy_trained_csr_num;
	/* ddr phy PIE */
	struct dram_cfg_param *ddrphy_pie;
	unsigned int ddrphy_pie_num;
};

extern struct dram_timing_info lpddr4_timing;

void ddr_load_train_firmware(enum fw_type type);
void ddr_init(struct dram_timing_info *timing_info);
void lpddr4_cfg_phy(struct dram_timing_info *timing_info);
void load_lpddr4_phy_pie(void);
void ddrphy_trained_csr_save(struct dram_cfg_param *, unsigned int);
void dram_config_save(struct dram_timing_info *, unsigned long);

/* utils function for ddr phy training */
void wait_ddrphy_training_complete(void);
void ddrphy_init_set_dfi_clk(unsigned int drate);
void ddrphy_init_read_msg_block(enum fw_type type);

static inline void reg32_write(unsigned long addr, u32 val)
{
	writel(val, addr);
}

static inline u32 reg32_read(unsigned long addr)
{
	return readl(addr);
}

static inline void reg32setbit(unsigned long addr, u32 bit)
{
	setbits_le32(addr, (1 << bit));
}

#define dwc_ddrphy_apb_wr(addr, data)	reg32_write(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4*(addr), data)
#define dwc_ddrphy_apb_rd(addr)		reg32_read(IP2APB_DDRPHY_IPS_BASE_ADDR(0) + 4*(addr))

#endif /* __IMX8M_DDR_H__ */
