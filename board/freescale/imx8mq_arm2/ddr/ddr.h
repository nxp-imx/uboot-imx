/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef SRC_DDRC_RCR_ADDR
#define SRC_DDRC_RCR_ADDR SRC_IPS_BASE_ADDR +0x1000
#endif
#ifndef DDR_CSD1_BASE_ADDR
#define DDR_CSD1_BASE_ADDR 0x40000000
#endif

enum fw_type {
	FW_1D_IMAGE,
	FW_2D_IMAGE,
};

void ddr_init(void);
void ddr_load_train_code(enum fw_type type);
void wait_ddrphy_training_complete(void);
void ddr3_phyinit_train_1600mts(void);
void ddr4_phyinit_train_2400mts(void);

static inline void reg32_write(unsigned long addr, u32 val)
{
	writel(val, addr);
}

static inline uint32_t reg32_read(unsigned long addr)
{
	return readl(addr);
}
/*
static void inline dwc_ddrphy_apb_wr(unsigned long addr, u32 val)
{
    writel(val, addr);
}
*/

static inline void reg32setbit(unsigned long addr, u32 bit)
{
	setbits_le32(addr, (1 << bit));
}

static inline void reg32clrbit(unsigned long addr, u32 bit)
{
	clrbits_le32(addr, (1 << bit));
}
