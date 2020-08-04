// SPDX-License-Identifier: GPL-2.0
/*
 * Freescale i.MX6 PCI Express Root-Complex driver
 *
 * Copyright (C) 2013 Marek Vasut <marex@denx.de>
 *
 * Copyright (C) 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2019 NXP
 *
 * Based on upstream Linux kernel driver:
 * pci-imx6.c:		Sean Cross <xobs@kosagi.com>
 * pcie-designware.c:	Jingoo Han <jg1.han@samsung.com>
 */

#include <common.h>
#include <init.h>
#include <malloc.h>
#include <pci.h>
#if CONFIG_IS_ENABLED(CLK)
#include <clk.h>
#else
#include <asm/arch/clock.h>
#endif
#include <asm/arch/iomux.h>
#ifdef CONFIG_MX6
#include <asm/arch/crm_regs.h>
#endif
#include <asm/gpio.h>
#include <asm/io.h>
#include <dm.h>
#include <linux/sizes.h>
#include <linux/ioport.h>
#include <errno.h>
#include <asm/arch/sys_proto.h>
#include <syscon.h>
#include <regmap.h>
#include <asm-generic/gpio.h>
#include <dt-bindings/soc/imx8_hsio.h>
#include <power/regulator.h>
#include <dm/device_compat.h>

enum imx_pcie_variants {
	IMX6Q,
	IMX6SX,
	IMX6QP,
	IMX8QM,
	IMX8QXP,
};

#define PCI_ACCESS_READ  0
#define PCI_ACCESS_WRITE 1

#ifdef CONFIG_MX6SX
#define MX6_DBI_ADDR	0x08ffc000
#define MX6_IO_ADDR	0x08f80000
#define MX6_MEM_ADDR	0x08000000
#define MX6_ROOT_ADDR	0x08f00000
#else
#define MX6_DBI_ADDR	0x01ffc000
#define MX6_IO_ADDR	0x01f80000
#define MX6_MEM_ADDR	0x01000000
#define MX6_ROOT_ADDR	0x01f00000
#endif
#define MX6_DBI_SIZE	0x4000
#define MX6_IO_SIZE	0x10000
#define MX6_MEM_SIZE	0xf00000
#define MX6_ROOT_SIZE	0x80000

/* PCIe Port Logic registers (memory-mapped) */
#define PL_OFFSET 0x700
#define PCIE_PL_PFLR (PL_OFFSET + 0x08)
#define PCIE_PL_PFLR_LINK_STATE_MASK		(0x3f << 16)
#define PCIE_PL_PFLR_FORCE_LINK			(1 << 15)
#define PCIE_PHY_DEBUG_R0 (PL_OFFSET + 0x28)
#define PCIE_PHY_DEBUG_R1 (PL_OFFSET + 0x2c)
#define PCIE_PHY_DEBUG_R1_LINK_UP		(1 << 4)
#define PCIE_PHY_DEBUG_R1_LINK_IN_TRAINING	(1 << 29)

#define PCIE_PORT_LINK_CONTROL		0x710
#define PORT_LINK_MODE_MASK		(0x3f << 16)
#define PORT_LINK_MODE_1_LANES		(0x1 << 16)
#define PORT_LINK_MODE_2_LANES		(0x3 << 16)
#define PORT_LINK_MODE_4_LANES		(0x7 << 16)
#define PORT_LINK_MODE_8_LANES		(0xf << 16)


#define PCIE_LINK_WIDTH_SPEED_CONTROL	0x80C
#define PORT_LOGIC_SPEED_CHANGE		(0x1 << 17)
#define PORT_LOGIC_LINK_WIDTH_MASK	(0x1f << 8)
#define PORT_LOGIC_LINK_WIDTH_1_LANES	(0x1 << 8)
#define PORT_LOGIC_LINK_WIDTH_2_LANES	(0x2 << 8)
#define PORT_LOGIC_LINK_WIDTH_4_LANES	(0x4 << 8)
#define PORT_LOGIC_LINK_WIDTH_8_LANES	(0x8 << 8)

#define PCIE_PHY_CTRL (PL_OFFSET + 0x114)
#define PCIE_PHY_CTRL_DATA_LOC 0
#define PCIE_PHY_CTRL_CAP_ADR_LOC 16
#define PCIE_PHY_CTRL_CAP_DAT_LOC 17
#define PCIE_PHY_CTRL_WR_LOC 18
#define PCIE_PHY_CTRL_RD_LOC 19

#define PCIE_PHY_STAT (PL_OFFSET + 0x110)
#define PCIE_PHY_STAT_DATA_LOC 0
#define PCIE_PHY_STAT_ACK_LOC 16

/* PHY registers (not memory-mapped) */
#define PCIE_PHY_RX_ASIC_OUT 0x100D

#define PHY_RX_OVRD_IN_LO 0x1005
#define PHY_RX_OVRD_IN_LO_RX_DATA_EN (1 << 5)
#define PHY_RX_OVRD_IN_LO_RX_PLL_EN (1 << 3)

#define PCIE_PHY_PUP_REQ		(1 << 7)

/* iATU registers */
#define PCIE_ATU_VIEWPORT		0x900
#define PCIE_ATU_REGION_INBOUND		(0x1 << 31)
#define PCIE_ATU_REGION_OUTBOUND	(0x0 << 31)
#define PCIE_ATU_REGION_INDEX2		(0x2 << 0)
#define PCIE_ATU_REGION_INDEX1		(0x1 << 0)
#define PCIE_ATU_REGION_INDEX0		(0x0 << 0)
#define PCIE_ATU_CR1			0x904
#define PCIE_ATU_TYPE_MEM		(0x0 << 0)
#define PCIE_ATU_TYPE_IO		(0x2 << 0)
#define PCIE_ATU_TYPE_CFG0		(0x4 << 0)
#define PCIE_ATU_TYPE_CFG1		(0x5 << 0)
#define PCIE_ATU_CR2			0x908
#define PCIE_ATU_ENABLE			(0x1 << 31)
#define PCIE_ATU_BAR_MODE_ENABLE	(0x1 << 30)
#define PCIE_ATU_LOWER_BASE		0x90C
#define PCIE_ATU_UPPER_BASE		0x910
#define PCIE_ATU_LIMIT			0x914
#define PCIE_ATU_LOWER_TARGET		0x918
#define PCIE_ATU_BUS(x)			(((x) & 0xff) << 24)
#define PCIE_ATU_DEV(x)			(((x) & 0x1f) << 19)
#define PCIE_ATU_FUNC(x)		(((x) & 0x7) << 16)
#define PCIE_ATU_UPPER_TARGET		0x91C

#define PCIE_MISC_CTRL			(PL_OFFSET + 0x1BC)
#define PCIE_MISC_DBI_RO_WR_EN		BIT(0)

/* iMX8 HSIO registers */
#define IMX8QM_LPCG_PHYX2_OFFSET		0x00000
#define IMX8QM_CSR_PHYX2_OFFSET			0x90000
#define IMX8QM_CSR_PHYX1_OFFSET			0xA0000
#define IMX8QM_CSR_PHYX_STTS0_OFFSET		0x4
#define IMX8QM_CSR_PCIEA_OFFSET			0xB0000
#define IMX8QM_CSR_PCIEB_OFFSET			0xC0000
#define IMX8QM_CSR_PCIE_CTRL1_OFFSET		0x4
#define IMX8QM_CSR_PCIE_CTRL2_OFFSET		0x8
#define IMX8QM_CSR_PCIE_STTS0_OFFSET		0xC
#define IMX8QM_CSR_MISC_OFFSET			0xE0000

#define IMX8QM_LPCG_PHY_PCG0			BIT(1)
#define IMX8QM_LPCG_PHY_PCG1			BIT(5)

#define IMX8QM_CTRL_LTSSM_ENABLE		BIT(4)
#define IMX8QM_CTRL_READY_ENTR_L23		BIT(5)
#define IMX8QM_CTRL_PM_XMT_TURNOFF		BIT(9)
#define IMX8QM_CTRL_BUTTON_RST_N		BIT(21)
#define IMX8QM_CTRL_PERST_N			BIT(22)
#define IMX8QM_CTRL_POWER_UP_RST_N		BIT(23)

#define IMX8QM_CTRL_STTS0_PM_LINKST_IN_L2	BIT(13)
#define IMX8QM_CTRL_STTS0_PM_REQ_CORE_RST	BIT(19)
#define IMX8QM_STTS0_LANE0_TX_PLL_LOCK		BIT(4)
#define IMX8QM_STTS0_LANE1_TX_PLL_LOCK		BIT(12)

#define IMX8QM_PCIE_TYPE_MASK			(0xF << 24)

#define IMX8QM_PHYX2_CTRL0_APB_MASK		0x3
#define IMX8QM_PHY_APB_RSTN_0			BIT(0)
#define IMX8QM_PHY_APB_RSTN_1			BIT(1)

#define IMX8QM_MISC_IOB_RXENA			BIT(0)
#define IMX8QM_MISC_IOB_TXENA			BIT(1)
#define IMX8QM_CSR_MISC_IOB_A_0_TXOE		BIT(2)
#define IMX8QM_CSR_MISC_IOB_A_0_M1M0_MASK	(0x3 << 3)
#define IMX8QM_CSR_MISC_IOB_A_0_M1M0_2		BIT(4)
#define IMX8QM_MISC_PHYX1_EPCS_SEL		BIT(12)
#define IMX8QM_MISC_PCIE_AB_SELECT		BIT(13)

#define HW_PHYX2_CTRL0_PIPE_LN2LK_MASK	(0xF << 13)
#define HW_PHYX2_CTRL0_PIPE_LN2LK_0	BIT(13)
#define HW_PHYX2_CTRL0_PIPE_LN2LK_1	BIT(14)
#define HW_PHYX2_CTRL0_PIPE_LN2LK_2	BIT(15)
#define HW_PHYX2_CTRL0_PIPE_LN2LK_3	BIT(16)

#define PHY_PLL_LOCK_WAIT_MAX_RETRIES	2000

#ifdef DEBUG

#ifdef DEBUG_STRESS_WR /* warm-reset stress tests */
#define SNVS_LPGRP 0x020cc068
#endif

#define DBGF(x...) printf(x)

static void print_regs(int contain_pcie_reg)
{
#ifdef CONFIG_MX6
	u32 val;
	struct iomuxc *iomuxc_regs = (struct iomuxc *)IOMUXC_BASE_ADDR;
	struct mxc_ccm_reg *ccm_regs = (struct mxc_ccm_reg *)CCM_BASE_ADDR;
	val = readl(&iomuxc_regs->gpr[1]);
	DBGF("GPR01 a:0x%08x v:0x%08x\n", (u32)&iomuxc_regs->gpr[1], val);
	val = readl(&iomuxc_regs->gpr[5]);
	DBGF("GPR05 a:0x%08x v:0x%08x\n", (u32)&iomuxc_regs->gpr[5], val);
	val = readl(&iomuxc_regs->gpr[8]);
	DBGF("GPR08 a:0x%08x v:0x%08x\n", (u32)&iomuxc_regs->gpr[8], val);
	val = readl(&iomuxc_regs->gpr[12]);
	DBGF("GPR12 a:0x%08x v:0x%08x\n", (u32)&iomuxc_regs->gpr[12], val);
	val = readl(&ccm_regs->analog_pll_enet);
	DBGF("PLL06 a:0x%08x v:0x%08x\n", (u32)&ccm_regs->analog_pll_enet, val);
	val = readl(&ccm_regs->ana_misc1);
	DBGF("MISC1 a:0x%08x v:0x%08x\n", (u32)&ccm_regs->ana_misc1, val);
	if (contain_pcie_reg) {
		val = readl(MX6_DBI_ADDR + 0x728);
		DBGF("dbr0 offset 0x728 %08x\n", val);
		val = readl(MX6_DBI_ADDR + 0x72c);
		DBGF("dbr1 offset 0x72c %08x\n", val);
	}
#endif
}
#else
#define DBGF(x...)
static void print_regs(int contain_pcie_reg) {}
#endif

struct imx_pcie_priv {
	void __iomem		*dbi_base;
	void __iomem		*cfg_base;
	void __iomem		*cfg1_base;
	enum imx_pcie_variants variant;
	struct regmap		*iomuxc_gpr;
	u32					hsio_cfg;
	u32					ctrl_id;
	u32 					ext_osc;
	u32					cpu_base;
	u32 					lanes;
	u32					cfg_size;
	int					cpu_addr_offset;
	struct gpio_desc 		clkreq_gpio;
	struct gpio_desc 		dis_gpio;
	struct gpio_desc 		reset_gpio;
	struct gpio_desc 		power_on_gpio;

	struct pci_region 		*io;
	struct pci_region 		*mem;
	struct pci_region 		*pref;

#if CONFIG_IS_ENABLED(CLK)
	struct clk			pcie_bus;
	struct clk			pcie_phy;
	struct clk			pcie_phy_pclk;
	struct clk			pcie_inbound_axi;
	struct clk			pcie_per;
	struct clk			pciex2_per;
	struct clk			phy_per;
	struct clk			misc_per;
	struct clk			pcie;
	struct clk			pcie_ext_src;
#endif

#if CONFIG_IS_ENABLED(DM_REGULATOR)
	struct udevice 		*epdev_on;
	struct udevice 		*pcie_bus_regulator;
	struct udevice 		*pcie_phy_regulator;
#endif
};

/*
 * PHY access functions
 */
static int pcie_phy_poll_ack(void __iomem *dbi_base, int exp_val)
{
	u32 val;
	u32 max_iterations = 10;
	u32 wait_counter = 0;

	do {
		val = readl(dbi_base + PCIE_PHY_STAT);
		val = (val >> PCIE_PHY_STAT_ACK_LOC) & 0x1;
		wait_counter++;

		if (val == exp_val)
			return 0;

		udelay(1);
	} while (wait_counter < max_iterations);

	return -ETIMEDOUT;
}

static int pcie_phy_wait_ack(void __iomem *dbi_base, int addr)
{
	u32 val;
	int ret;

	val = addr << PCIE_PHY_CTRL_DATA_LOC;
	writel(val, dbi_base + PCIE_PHY_CTRL);

	val |= (0x1 << PCIE_PHY_CTRL_CAP_ADR_LOC);
	writel(val, dbi_base + PCIE_PHY_CTRL);

	ret = pcie_phy_poll_ack(dbi_base, 1);
	if (ret)
		return ret;

	val = addr << PCIE_PHY_CTRL_DATA_LOC;
	writel(val, dbi_base + PCIE_PHY_CTRL);

	ret = pcie_phy_poll_ack(dbi_base, 0);
	if (ret)
		return ret;

	return 0;
}

/* Read from the 16-bit PCIe PHY control registers (not memory-mapped) */
static int pcie_phy_read(void __iomem *dbi_base, int addr , int *data)
{
	u32 val, phy_ctl;
	int ret;

	ret = pcie_phy_wait_ack(dbi_base, addr);
	if (ret)
		return ret;

	/* assert Read signal */
	phy_ctl = 0x1 << PCIE_PHY_CTRL_RD_LOC;
	writel(phy_ctl, dbi_base + PCIE_PHY_CTRL);

	ret = pcie_phy_poll_ack(dbi_base, 1);
	if (ret)
		return ret;

	val = readl(dbi_base + PCIE_PHY_STAT);
	*data = val & 0xffff;

	/* deassert Read signal */
	writel(0x00, dbi_base + PCIE_PHY_CTRL);

	ret = pcie_phy_poll_ack(dbi_base, 0);
	if (ret)
		return ret;

	return 0;
}

static int pcie_phy_write(void __iomem *dbi_base, int addr, int data)
{
	u32 var;
	int ret;

	/* write addr */
	/* cap addr */
	ret = pcie_phy_wait_ack(dbi_base, addr);
	if (ret)
		return ret;

	var = data << PCIE_PHY_CTRL_DATA_LOC;
	writel(var, dbi_base + PCIE_PHY_CTRL);

	/* capture data */
	var |= (0x1 << PCIE_PHY_CTRL_CAP_DAT_LOC);
	writel(var, dbi_base + PCIE_PHY_CTRL);

	ret = pcie_phy_poll_ack(dbi_base, 1);
	if (ret)
		return ret;

	/* deassert cap data */
	var = data << PCIE_PHY_CTRL_DATA_LOC;
	writel(var, dbi_base + PCIE_PHY_CTRL);

	/* wait for ack de-assertion */
	ret = pcie_phy_poll_ack(dbi_base, 0);
	if (ret)
		return ret;

	/* assert wr signal */
	var = 0x1 << PCIE_PHY_CTRL_WR_LOC;
	writel(var, dbi_base + PCIE_PHY_CTRL);

	/* wait for ack */
	ret = pcie_phy_poll_ack(dbi_base, 1);
	if (ret)
		return ret;

	/* deassert wr signal */
	var = data << PCIE_PHY_CTRL_DATA_LOC;
	writel(var, dbi_base + PCIE_PHY_CTRL);

	/* wait for ack de-assertion */
	ret = pcie_phy_poll_ack(dbi_base, 0);
	if (ret)
		return ret;

	writel(0x0, dbi_base + PCIE_PHY_CTRL);

	return 0;
}

#if !CONFIG_IS_ENABLED(DM_PCI)
void imx_pcie_gpr_read(struct imx_pcie_priv *priv, uint offset, uint *valp)
{
	struct iomuxc *iomuxc_regs = (struct iomuxc *)IOMUXC_BASE_ADDR;
	*valp = readl(&iomuxc_regs->gpr[offset >> 2]);
}

void imx_pcie_gpr_update_bits(struct imx_pcie_priv *priv, uint offset, uint mask, uint val)
{
	struct iomuxc *iomuxc_regs = (struct iomuxc *)IOMUXC_BASE_ADDR;
	clrsetbits_32(&iomuxc_regs->gpr[offset >> 2], mask, val);
}

#else
void imx_pcie_gpr_read(struct imx_pcie_priv *priv, uint offset, uint *valp)
{
	regmap_read(priv->iomuxc_gpr, offset, valp);
}

void imx_pcie_gpr_update_bits(struct imx_pcie_priv *priv, uint offset, uint mask, uint val)
{
	regmap_update_bits(priv->iomuxc_gpr, offset, mask, val);
}

#endif

static int imx6_pcie_link_up(struct imx_pcie_priv *priv)
{
	u32 rc, ltssm;
	int rx_valid, temp;

	/* link is debug bit 36, debug register 1 starts at bit 32 */
	rc = readl(priv->dbi_base + PCIE_PHY_DEBUG_R1);
	if ((rc & PCIE_PHY_DEBUG_R1_LINK_UP) &&
	    !(rc & PCIE_PHY_DEBUG_R1_LINK_IN_TRAINING))
		return -EAGAIN;

	/*
	 * From L0, initiate MAC entry to gen2 if EP/RC supports gen2.
	 * Wait 2ms (LTSSM timeout is 24ms, PHY lock is ~5us in gen2).
	 * If (MAC/LTSSM.state == Recovery.RcvrLock)
	 * && (PHY/rx_valid==0) then pulse PHY/rx_reset. Transition
	 * to gen2 is stuck
	 */
	pcie_phy_read(priv->dbi_base, PCIE_PHY_RX_ASIC_OUT, &rx_valid);
	ltssm = readl(priv->dbi_base + PCIE_PHY_DEBUG_R0) & 0x3F;

	if (rx_valid & 0x01)
		return 0;

	if (ltssm != 0x0d)
		return 0;

	printf("transition to gen2 is stuck, reset PHY!\n");

	pcie_phy_read(priv->dbi_base, PHY_RX_OVRD_IN_LO, &temp);
	temp |= (PHY_RX_OVRD_IN_LO_RX_DATA_EN | PHY_RX_OVRD_IN_LO_RX_PLL_EN);
	pcie_phy_write(priv->dbi_base, PHY_RX_OVRD_IN_LO, temp);

	udelay(3000);

	pcie_phy_read(priv->dbi_base, PHY_RX_OVRD_IN_LO, &temp);
	temp &= ~(PHY_RX_OVRD_IN_LO_RX_DATA_EN | PHY_RX_OVRD_IN_LO_RX_PLL_EN);
	pcie_phy_write(priv->dbi_base, PHY_RX_OVRD_IN_LO, temp);

	return 0;
}

/* Fix class value */
static void imx_pcie_fix_class(struct imx_pcie_priv *priv)
{
	writew(PCI_CLASS_BRIDGE_PCI, priv->dbi_base + PCI_CLASS_DEVICE);
}

/* Clear multi-function bit */
static void imx_pcie_clear_multifunction(struct imx_pcie_priv *priv)
{
	writeb(PCI_HEADER_TYPE_BRIDGE, priv->dbi_base + PCI_HEADER_TYPE);
}

static void imx_pcie_setup_ctrl(struct imx_pcie_priv *priv)
{
	u32 val;

	writel(PCIE_MISC_DBI_RO_WR_EN, priv->dbi_base + PCIE_MISC_CTRL);

	/* Set the number of lanes */
	val = readl(priv->dbi_base + PCIE_PORT_LINK_CONTROL);
	val &= ~PORT_LINK_MODE_MASK;
	switch (priv->lanes) {
	case 1:
		val |= PORT_LINK_MODE_1_LANES;
		break;
	case 2:
		val |= PORT_LINK_MODE_2_LANES;
		break;
	case 4:
		val |= PORT_LINK_MODE_4_LANES;
		break;
	case 8:
		val |= PORT_LINK_MODE_8_LANES;
		break;
	default:
		printf("num-lanes %u: invalid value\n", priv->lanes);
		return;
	}
	writel(val, priv->dbi_base + PCIE_PORT_LINK_CONTROL);

	/* Set link width speed control register */
	val = readl(priv->dbi_base + PCIE_LINK_WIDTH_SPEED_CONTROL);
	val &= ~PORT_LOGIC_LINK_WIDTH_MASK;
	switch (priv->lanes) {
	case 1:
		val |= PORT_LOGIC_LINK_WIDTH_1_LANES;
		break;
	case 2:
		val |= PORT_LOGIC_LINK_WIDTH_2_LANES;
		break;
	case 4:
		val |= PORT_LOGIC_LINK_WIDTH_4_LANES;
		break;
	case 8:
		val |= PORT_LOGIC_LINK_WIDTH_8_LANES;
		break;
	}
	writel(val, priv->dbi_base + PCIE_LINK_WIDTH_SPEED_CONTROL);

	/* setup RC BARs */
	writel(0, priv->dbi_base + PCI_BASE_ADDRESS_0);
	writel(0, priv->dbi_base + PCI_BASE_ADDRESS_1);

	/* setup bus numbers */
	val = readl(priv->dbi_base + PCI_PRIMARY_BUS);
	val &= 0xff000000;
	val |= 0x00ff0100;
	writel(val, priv->dbi_base + PCI_PRIMARY_BUS);

	/* setup command register */
	val = readl(priv->dbi_base + PCI_COMMAND);
	val &= 0xffff0000;
	val |= PCI_COMMAND_IO | PCI_COMMAND_MEMORY |
		PCI_COMMAND_MASTER;
	writel(val, priv->dbi_base + PCI_COMMAND);

	imx_pcie_fix_class(priv);
	imx_pcie_clear_multifunction(priv);

	writel(0, priv->dbi_base + PCIE_MISC_CTRL);
}

static void imx_pcie_atu_outbound_set(struct imx_pcie_priv *priv, int idx, int type,
				      u64 phys, u64 bus_addr, u32 size)
{
	writel(PCIE_ATU_REGION_OUTBOUND | idx, priv->dbi_base + PCIE_ATU_VIEWPORT);
	writel((u32)(phys + priv->cpu_addr_offset), priv->dbi_base + PCIE_ATU_LOWER_BASE);
	writel((phys + priv->cpu_addr_offset) >> 32, priv->dbi_base + PCIE_ATU_UPPER_BASE);
	writel((u32)(phys + priv->cpu_addr_offset) + size - 1, priv->dbi_base + PCIE_ATU_LIMIT);
	writel((u32)bus_addr, priv->dbi_base + PCIE_ATU_LOWER_TARGET);
	writel(bus_addr >> 32, priv->dbi_base + PCIE_ATU_UPPER_TARGET);
	writel(type, priv->dbi_base + PCIE_ATU_CR1);
	writel(PCIE_ATU_ENABLE, priv->dbi_base + PCIE_ATU_CR2);
}

/*
 * iATU region setup
 */
static int imx_pcie_regions_setup(struct imx_pcie_priv *priv)
{
	if (priv->io)
		/* ATU : OUTBOUND : IO */
		imx_pcie_atu_outbound_set(priv, PCIE_ATU_REGION_INDEX2,
					 PCIE_ATU_TYPE_IO,
					 priv->io->phys_start,
					 priv->io->bus_start,
					 priv->io->size);

	if (priv->mem)
		/* ATU : OUTBOUND : MEM */
		imx_pcie_atu_outbound_set(priv, PCIE_ATU_REGION_INDEX0,
					 PCIE_ATU_TYPE_MEM,
					 priv->mem->phys_start,
					 priv->mem->bus_start,
					 priv->mem->size);


	return 0;
}

/*
 * PCI Express accessors
 */
static void __iomem *get_bus_address(struct imx_pcie_priv *priv,
				     pci_dev_t d, int where)
{
	void __iomem *va_address;

	if (PCI_BUS(d) == 0) {
		/* Outbound TLP matched primary interface of the bridge */
		va_address = priv->dbi_base;
	} else {
		if (PCI_BUS(d) < 2) {
			/* Outbound TLP matched secondary interface of the bridge changes to CFG0 */
			imx_pcie_atu_outbound_set(priv, PCIE_ATU_REGION_INDEX1,
					 PCIE_ATU_TYPE_CFG0,
					 (ulong)priv->cfg_base,
					 (u64)d << 8,
					 priv->cfg_size >> 1);
			va_address = priv->cfg_base;
		} else {
			/* Outbound TLP matched the bus behind the bridge uses type CFG1 */
			imx_pcie_atu_outbound_set(priv, PCIE_ATU_REGION_INDEX1,
					 PCIE_ATU_TYPE_CFG1,
					 (ulong)priv->cfg1_base,
					 (u64)d << 8,
					 priv->cfg_size >> 1);
			va_address = priv->cfg1_base;
		}
	}

	va_address += (where & ~0x3);

	return va_address;

}

static int imx_pcie_addr_valid(pci_dev_t d)
{
	if ((PCI_BUS(d) == 0) && (PCI_DEV(d) > 0))
		return -EINVAL;
	/* ARI forward is not enabled, so non-zero device at downstream must be blocked */
	if ((PCI_BUS(d) == 1) && (PCI_DEV(d) > 0))
		return -EINVAL;
	return 0;
}

/*
 * Replace the original ARM DABT handler with a simple jump-back one.
 *
 * The problem here is that if we have a PCIe bridge attached to this PCIe
 * controller, but no PCIe device is connected to the bridges' downstream
 * port, the attempt to read/write from/to the config space will produce
 * a DABT. This is a behavior of the controller and can not be disabled
 * unfortuatelly.
 *
 * To work around the problem, we backup the current DABT handler address
 * and replace it with our own DABT handler, which only bounces right back
 * into the code.
 */
static void imx_pcie_fix_dabt_handler(bool set)
{
#ifdef CONFIG_MX6
	extern uint32_t *_data_abort;
	uint32_t *data_abort_addr = (uint32_t *)&_data_abort;

	static const uint32_t data_abort_bounce_handler = 0xe25ef004;
	uint32_t data_abort_bounce_addr = (uint32_t)&data_abort_bounce_handler;

	static uint32_t data_abort_backup;

	if (set) {
		data_abort_backup = *data_abort_addr;
		*data_abort_addr = data_abort_bounce_addr;
	} else {
		*data_abort_addr = data_abort_backup;
	}
#endif
}

static int imx_pcie_read_cfg(struct imx_pcie_priv *priv, pci_dev_t d,
			     int where, u32 *val)
{
	void __iomem *va_address;
	int ret;

	ret = imx_pcie_addr_valid(d);
	if (ret) {
		*val = 0xffffffff;
		return 0;
	}

	va_address = get_bus_address(priv, d, where);

	/*
	 * Read the PCIe config space. We must replace the DABT handler
	 * here in case we got data abort from the PCIe controller, see
	 * imx_pcie_fix_dabt_handler() description. Note that writing the
	 * "val" with valid value is also imperative here as in case we
	 * did got DABT, the val would contain random value.
	 */
	imx_pcie_fix_dabt_handler(true);
	writel(0xffffffff, val);
	*val = readl(va_address);
	imx_pcie_fix_dabt_handler(false);

	return 0;
}

static int imx_pcie_write_cfg(struct imx_pcie_priv *priv, pci_dev_t d,
			      int where, u32 val)
{
	void __iomem *va_address = NULL;
	int ret;

	ret = imx_pcie_addr_valid(d);
	if (ret)
		return ret;

	va_address = get_bus_address(priv, d, where);

	/*
	 * Write the PCIe config space. We must replace the DABT handler
	 * here in case we got data abort from the PCIe controller, see
	 * imx_pcie_fix_dabt_handler() description.
	 */
	imx_pcie_fix_dabt_handler(true);
	writel(val, va_address);
	imx_pcie_fix_dabt_handler(false);

	return 0;
}

static int imx8_pcie_assert_core_reset(struct imx_pcie_priv *priv,
				       bool prepare_for_boot)
{
	u32 val;

	switch (priv->variant) {
	case IMX8QXP:
			val = IMX8QM_CSR_PCIEB_OFFSET;
			imx_pcie_gpr_update_bits(priv,
					val + IMX8QM_CSR_PCIE_CTRL2_OFFSET,
					IMX8QM_CTRL_BUTTON_RST_N,
					IMX8QM_CTRL_BUTTON_RST_N);
			imx_pcie_gpr_update_bits(priv,
					val + IMX8QM_CSR_PCIE_CTRL2_OFFSET,
					IMX8QM_CTRL_PERST_N,
					IMX8QM_CTRL_PERST_N);
			imx_pcie_gpr_update_bits(priv,
					val + IMX8QM_CSR_PCIE_CTRL2_OFFSET,
					IMX8QM_CTRL_POWER_UP_RST_N,
					IMX8QM_CTRL_POWER_UP_RST_N);
		break;
	case IMX8QM:
			val = IMX8QM_CSR_PCIEA_OFFSET + priv->ctrl_id * SZ_64K;
			imx_pcie_gpr_update_bits(priv,
					val + IMX8QM_CSR_PCIE_CTRL2_OFFSET,
					IMX8QM_CTRL_BUTTON_RST_N,
					IMX8QM_CTRL_BUTTON_RST_N);
			imx_pcie_gpr_update_bits(priv,
					val + IMX8QM_CSR_PCIE_CTRL2_OFFSET,
					IMX8QM_CTRL_PERST_N,
					IMX8QM_CTRL_PERST_N);
			imx_pcie_gpr_update_bits(priv,
					val + IMX8QM_CSR_PCIE_CTRL2_OFFSET,
					IMX8QM_CTRL_POWER_UP_RST_N,
					IMX8QM_CTRL_POWER_UP_RST_N);
		break;
	default:
		break;
	}

	return 0;
}

static int imx8_pcie_init_phy(struct imx_pcie_priv *priv)
{
	u32 tmp, val;

	if (priv->variant == IMX8QM
		|| priv->variant == IMX8QXP) {
		switch (priv->hsio_cfg) {
		case PCIEAX2SATA:
			/*
			 * bit 0 rx ena 1.
			 * bit12 PHY_X1_EPCS_SEL 1.
			 * bit13 phy_ab_select 0.
			 */
			imx_pcie_gpr_update_bits(priv,
				IMX8QM_CSR_PHYX2_OFFSET,
				IMX8QM_PHYX2_CTRL0_APB_MASK,
				IMX8QM_PHY_APB_RSTN_0
				| IMX8QM_PHY_APB_RSTN_1);

			imx_pcie_gpr_update_bits(priv,
				IMX8QM_CSR_MISC_OFFSET,
				IMX8QM_MISC_PHYX1_EPCS_SEL,
				IMX8QM_MISC_PHYX1_EPCS_SEL);
			imx_pcie_gpr_update_bits(priv,
				IMX8QM_CSR_MISC_OFFSET,
				IMX8QM_MISC_PCIE_AB_SELECT,
				0);
			break;

		case PCIEAX1PCIEBX1SATA:
			tmp = IMX8QM_PHY_APB_RSTN_1;
			tmp |= IMX8QM_PHY_APB_RSTN_0;
			imx_pcie_gpr_update_bits(priv,
				IMX8QM_CSR_PHYX2_OFFSET,
				IMX8QM_PHYX2_CTRL0_APB_MASK, tmp);

			imx_pcie_gpr_update_bits(priv,
				IMX8QM_CSR_MISC_OFFSET,
				IMX8QM_MISC_PHYX1_EPCS_SEL,
				IMX8QM_MISC_PHYX1_EPCS_SEL);
			imx_pcie_gpr_update_bits(priv,
				IMX8QM_CSR_MISC_OFFSET,
				IMX8QM_MISC_PCIE_AB_SELECT,
				IMX8QM_MISC_PCIE_AB_SELECT);

			imx_pcie_gpr_update_bits(priv,
				IMX8QM_CSR_PHYX2_OFFSET,
				HW_PHYX2_CTRL0_PIPE_LN2LK_MASK,
				HW_PHYX2_CTRL0_PIPE_LN2LK_3 | HW_PHYX2_CTRL0_PIPE_LN2LK_0);

			break;

		case PCIEAX2PCIEBX1:
			/*
			 * bit 0 rx ena 1.
			 * bit12 PHY_X1_EPCS_SEL 0.
			 * bit13 phy_ab_select 1.
			 */
			if (priv->ctrl_id)
				imx_pcie_gpr_update_bits(priv,
					IMX8QM_CSR_PHYX1_OFFSET,
					IMX8QM_PHY_APB_RSTN_0,
					IMX8QM_PHY_APB_RSTN_0);
			else
				imx_pcie_gpr_update_bits(priv,
					IMX8QM_CSR_PHYX2_OFFSET,
					IMX8QM_PHYX2_CTRL0_APB_MASK,
					IMX8QM_PHY_APB_RSTN_0
					| IMX8QM_PHY_APB_RSTN_1);

			imx_pcie_gpr_update_bits(priv,
				IMX8QM_CSR_MISC_OFFSET,
				IMX8QM_MISC_PHYX1_EPCS_SEL,
				0);
			imx_pcie_gpr_update_bits(priv,
				IMX8QM_CSR_MISC_OFFSET,
				IMX8QM_MISC_PCIE_AB_SELECT,
				IMX8QM_MISC_PCIE_AB_SELECT);
			break;
		}

		if (priv->ext_osc) {
			imx_pcie_gpr_update_bits(priv,
				IMX8QM_CSR_MISC_OFFSET,
				IMX8QM_MISC_IOB_RXENA,
				IMX8QM_MISC_IOB_RXENA);
			imx_pcie_gpr_update_bits(priv,
				IMX8QM_CSR_MISC_OFFSET,
				IMX8QM_MISC_IOB_TXENA,
				0);
		} else {
			/* Try to used the internal pll as ref clk */
			imx_pcie_gpr_update_bits(priv,
				IMX8QM_CSR_MISC_OFFSET,
				IMX8QM_MISC_IOB_RXENA,
				0);
			imx_pcie_gpr_update_bits(priv,
				IMX8QM_CSR_MISC_OFFSET,
				IMX8QM_MISC_IOB_TXENA,
				IMX8QM_MISC_IOB_TXENA);
			imx_pcie_gpr_update_bits(priv,
				IMX8QM_CSR_MISC_OFFSET,
				IMX8QM_CSR_MISC_IOB_A_0_TXOE
				| IMX8QM_CSR_MISC_IOB_A_0_M1M0_MASK,
				IMX8QM_CSR_MISC_IOB_A_0_TXOE
				| IMX8QM_CSR_MISC_IOB_A_0_M1M0_2);
		}

		val = IMX8QM_CSR_PCIEA_OFFSET
			+ priv->ctrl_id * SZ_64K;
		imx_pcie_gpr_update_bits(priv,
				val, IMX8QM_PCIE_TYPE_MASK,
				0x4 << 24);

		mdelay(10);
	}

	return 0;
}

static int imx8_pcie_wait_for_phy_pll_lock(struct imx_pcie_priv *priv)
{
	u32 val, tmp, orig;
	unsigned int retries = 0;

	if (priv->variant == IMX8QXP
			|| priv->variant == IMX8QM) {
		for (retries = 0; retries < PHY_PLL_LOCK_WAIT_MAX_RETRIES;
				retries++) {
			if (priv->hsio_cfg == PCIEAX1PCIEBX1SATA) {
				imx_pcie_gpr_read(priv,
					    IMX8QM_CSR_PHYX2_OFFSET + 0x4,
					    &tmp);
				if (priv->ctrl_id == 0) /* pciea 1 lanes */
					orig = IMX8QM_STTS0_LANE0_TX_PLL_LOCK;
				else /* pcieb 1 lanes */
					orig = IMX8QM_STTS0_LANE1_TX_PLL_LOCK;
				tmp &= orig;
				if (tmp == orig) {
					imx_pcie_gpr_update_bits(priv,
						IMX8QM_LPCG_PHYX2_OFFSET,
						IMX8QM_LPCG_PHY_PCG0
						| IMX8QM_LPCG_PHY_PCG1,
						IMX8QM_LPCG_PHY_PCG0
						| IMX8QM_LPCG_PHY_PCG1);
					break;
				}
			}

			if (priv->hsio_cfg == PCIEAX2PCIEBX1) {
				val = IMX8QM_CSR_PHYX2_OFFSET
					+ priv->ctrl_id * SZ_64K;
				imx_pcie_gpr_read(priv,
					    val + IMX8QM_CSR_PHYX_STTS0_OFFSET,
					    &tmp);
				orig = IMX8QM_STTS0_LANE0_TX_PLL_LOCK;
				if (priv->ctrl_id == 0) /* pciea 2 lanes */
					orig |= IMX8QM_STTS0_LANE1_TX_PLL_LOCK;
				tmp &= orig;
				if (tmp == orig) {
					val = IMX8QM_CSR_PHYX2_OFFSET
						+ priv->ctrl_id * SZ_64K;
					imx_pcie_gpr_update_bits(priv,
						val, IMX8QM_LPCG_PHY_PCG0,
						IMX8QM_LPCG_PHY_PCG0);
					break;
				}
			}
			udelay(10);
		}
	}

	if (retries >= PHY_PLL_LOCK_WAIT_MAX_RETRIES) {
		printf("pcie phy pll can't be locked.\n");
		return -ENODEV;
	} else {
		debug("pcie phy pll is locked.\n");
		return 0;
	}
}

static int imx8_pcie_deassert_core_reset(struct imx_pcie_priv *priv)
{
	int ret, i;
	u32 val, tmp;

#if CONFIG_IS_ENABLED(CLK)
	ret = clk_enable(&priv->pcie);
	if (ret) {
		printf("unable to enable pcie clock\n");
		return ret;
	}

	ret = clk_enable(&priv->pcie_phy);
	if (ret) {
		printf("unable to enable pcie_phy clock\n");
		goto err_pcie;
	}

	ret = clk_enable(&priv->pcie_bus);
	if (ret) {
		printf("unable to enable pcie_bus clock\n");
		goto err_pcie_phy;
	}

	ret = clk_enable(&priv->pcie_inbound_axi);
	if (ret) {
		printf("unable to enable pcie_axi clock\n");
		goto err_pcie_bus;
	}
	ret = clk_enable(&priv->pcie_per);
	if (ret) {
		printf("unable to enable pcie_per clock\n");
		goto err_pcie_inbound_axi;
	}

	ret = clk_enable(&priv->phy_per);
	if (ret) {
		printf("unable to enable phy_per clock\n");
		goto err_pcie_per;
	}

	ret = clk_enable(&priv->misc_per);
	if (ret) {
		printf("unable to enable misc_per clock\n");
		goto err_phy_per;
	}

	if (priv->variant == IMX8QM && priv->ctrl_id == 1) {
		ret = clk_enable(&priv->pcie_phy_pclk);
		if (ret) {
			printf("unable to enable pcie_phy_pclk clock\n");
			goto err_misc_per;
		}

		ret = clk_enable(&priv->pciex2_per);
		if (ret) {
			printf("unable to enable pciex2_per clock\n");
			clk_disable(&priv->pcie_phy_pclk);
			goto err_misc_per;
		}
	}
#endif
	/* allow the clocks to stabilize */
	udelay(200);

	/* bit19 PM_REQ_CORE_RST of pciex#_stts0 should be cleared. */
	for (i = 0; i < 100; i++) {
		val = IMX8QM_CSR_PCIEA_OFFSET
			+ priv->ctrl_id * SZ_64K;
		imx_pcie_gpr_read(priv,
				val + IMX8QM_CSR_PCIE_STTS0_OFFSET,
				&tmp);
		if ((tmp & IMX8QM_CTRL_STTS0_PM_REQ_CORE_RST) == 0)
			break;
		udelay(10);
	}

	if ((tmp & IMX8QM_CTRL_STTS0_PM_REQ_CORE_RST) != 0)
		printf("ERROR PM_REQ_CORE_RST is still set.\n");

	/* wait for phy pll lock firstly. */
	if (imx8_pcie_wait_for_phy_pll_lock(priv)) {
		ret = -ENODEV;
		goto err_ref_clk;;
	}

	if (dm_gpio_is_valid(&priv->reset_gpio)) {
		dm_gpio_set_value(&priv->reset_gpio, 1);
		mdelay(20);
		dm_gpio_set_value(&priv->reset_gpio, 0);
		mdelay(20);
	}

	return 0;

err_ref_clk:
#if CONFIG_IS_ENABLED(CLK)
	if (priv->variant == IMX8QM && priv->ctrl_id == 1) {
		clk_disable(&priv->pciex2_per);
		clk_disable(&priv->pcie_phy_pclk);
	}
err_misc_per:
	clk_disable(&priv->misc_per);
err_phy_per:
	clk_disable(&priv->phy_per);
err_pcie_per:
	clk_disable(&priv->pcie_per);
err_pcie_inbound_axi:
	clk_disable(&priv->pcie_inbound_axi);
err_pcie_bus:
	clk_disable(&priv->pcie_bus);
err_pcie_phy:
	clk_disable(&priv->pcie_phy);
err_pcie:
	clk_disable(&priv->pcie);
#endif

	return ret;
}

#ifdef CONFIG_MX6
/*
 * Initial bus setup
 */
static int imx6_pcie_assert_core_reset(struct imx_pcie_priv *priv,
				       bool prepare_for_boot)
{
	if (priv->variant == IMX6QP)
		imx_pcie_gpr_update_bits(priv, 4, IOMUXC_GPR1_PCIE_SW_RST, IOMUXC_GPR1_PCIE_SW_RST);

#if defined(CONFIG_MX6SX)
	if (priv->variant == IMX6SX) {
		struct gpc *gpc_regs = (struct gpc *)GPC_BASE_ADDR;

		/* SSP_EN is not used on MX6SX anymore */
		imx_pcie_gpr_update_bits(priv, 48, IOMUXC_GPR12_TEST_POWERDOWN, IOMUXC_GPR12_TEST_POWERDOWN);
		/* Force PCIe PHY reset */
		imx_pcie_gpr_update_bits(priv, 20, IOMUXC_GPR5_PCIE_BTNRST, IOMUXC_GPR5_PCIE_BTNRST);
		/* Power up PCIe PHY */
		setbits_le32(&gpc_regs->cntr, PCIE_PHY_PUP_REQ);
		pcie_power_up();

		return 0;
	}
#endif
	/*
	 * If the bootloader already enabled the link we need some special
	 * handling to get the core back into a state where it is safe to
	 * touch it for configuration.  As there is no dedicated reset signal
	 * wired up for MX6QDL, we need to manually force LTSSM into "detect"
	 * state before completely disabling LTSSM, which is a prerequisite
	 * for core configuration.
	 *
	 * If both LTSSM_ENABLE and REF_SSP_ENABLE are active we have a strong
	 * indication that the bootloader activated the link.
	 */
	if (priv->variant == IMX6Q && prepare_for_boot) {
		u32 val, gpr1, gpr12;

		imx_pcie_gpr_read(priv, 4, &gpr1);
		imx_pcie_gpr_read(priv, 48, &gpr12);
		if ((gpr1 & IOMUXC_GPR1_PCIE_REF_CLK_EN) &&
		    (gpr12 & IOMUXC_GPR12_PCIE_CTL_2)) {
			val = readl(priv->dbi_base + PCIE_PL_PFLR);
			val &= ~PCIE_PL_PFLR_LINK_STATE_MASK;
			val |= PCIE_PL_PFLR_FORCE_LINK;

			imx_pcie_fix_dabt_handler(true);
			writel(val, priv->dbi_base + PCIE_PL_PFLR);
			imx_pcie_fix_dabt_handler(false);

			imx_pcie_gpr_update_bits(priv, 48, IOMUXC_GPR12_PCIE_CTL_2, 0);
		}
	}

	if (priv->variant == IMX6QP || priv->variant == IMX6Q) {
		imx_pcie_gpr_update_bits(priv, 4, IOMUXC_GPR1_TEST_POWERDOWN,
			IOMUXC_GPR1_TEST_POWERDOWN);
		imx_pcie_gpr_update_bits(priv, 4, IOMUXC_GPR1_REF_SSP_EN, 0);
	}

	return 0;
}

static int imx6_pcie_init_phy(struct imx_pcie_priv *priv)
{
#ifndef DEBUG
	imx_pcie_gpr_update_bits(priv, 48, IOMUXC_GPR12_APPS_LTSSM_ENABLE, 0);
#endif

	imx_pcie_gpr_update_bits(priv, 48,
			IOMUXC_GPR12_DEVICE_TYPE_MASK,
			IOMUXC_GPR12_DEVICE_TYPE_RC);
	imx_pcie_gpr_update_bits(priv, 48,
			IOMUXC_GPR12_LOS_LEVEL_MASK,
			IOMUXC_GPR12_LOS_LEVEL_9);

	if (priv->variant == IMX6SX) {
		imx_pcie_gpr_update_bits(priv, 48,
				IOMUXC_GPR12_RX_EQ_MASK,
				IOMUXC_GPR12_RX_EQ_2);
	}

	imx_pcie_gpr_update_bits(priv, 32, 0xffffffff,
		(0x0 << IOMUXC_GPR8_PCS_TX_DEEMPH_GEN1_OFFSET) |
	       (0x0 << IOMUXC_GPR8_PCS_TX_DEEMPH_GEN2_3P5DB_OFFSET) |
	       (20 << IOMUXC_GPR8_PCS_TX_DEEMPH_GEN2_6DB_OFFSET) |
	       (127 << IOMUXC_GPR8_PCS_TX_SWING_FULL_OFFSET) |
	       (127 << IOMUXC_GPR8_PCS_TX_SWING_LOW_OFFSET));

	return 0;
}

__weak int imx6_pcie_toggle_power(void)
{
#ifdef CONFIG_PCIE_IMX_POWER_GPIO
	gpio_request(CONFIG_PCIE_IMX_POWER_GPIO, "pcie_power");
	gpio_direction_output(CONFIG_PCIE_IMX_POWER_GPIO, 0);
	mdelay(20);
	gpio_set_value(CONFIG_PCIE_IMX_POWER_GPIO, 1);
	mdelay(20);
	gpio_free(CONFIG_PCIE_IMX_POWER_GPIO);
#endif
	return 0;
}

__weak int imx6_pcie_toggle_reset(void)
{
	/*
	 * See 'PCI EXPRESS BASE SPECIFICATION, REV 3.0, SECTION 6.6.1'
	 * for detailed understanding of the PCIe CR reset logic.
	 *
	 * The PCIe #PERST reset line _MUST_ be connected, otherwise your
	 * design does not conform to the specification. You must wait at
	 * least 20 ms after de-asserting the #PERST so the EP device can
	 * do self-initialisation.
	 *
	 * In case your #PERST pin is connected to a plain GPIO pin of the
	 * CPU, you can define CONFIG_PCIE_IMX_PERST_GPIO in your board's
	 * configuration file and the condition below will handle the rest
	 * of the reset toggling.
	 *
	 * In case your #PERST toggling logic is more complex, for example
	 * connected via CPLD or somesuch, you can override this function
	 * in your board file and implement reset logic as needed. You must
	 * not forget to wait at least 20 ms after de-asserting #PERST in
	 * this case either though.
	 *
	 * In case your #PERST line of the PCIe EP device is not connected
	 * at all, your design is broken and you should fix your design,
	 * otherwise you will observe problems like for example the link
	 * not coming up after rebooting the system back from running Linux
	 * that uses the PCIe as well OR the PCIe link might not come up in
	 * Linux at all in the first place since it's in some non-reset
	 * state due to being previously used in U-Boot.
	 */
#ifdef CONFIG_PCIE_IMX_PERST_GPIO
	gpio_request(CONFIG_PCIE_IMX_PERST_GPIO, "pcie_reset");
	gpio_direction_output(CONFIG_PCIE_IMX_PERST_GPIO, 0);
	mdelay(20);
	gpio_set_value(CONFIG_PCIE_IMX_PERST_GPIO, 1);
	mdelay(20);
	gpio_free(CONFIG_PCIE_IMX_PERST_GPIO);
#else
	puts("WARNING: Make sure the PCIe #PERST line is connected!\n");
#endif

	return 0;
}

static int imx6_pcie_deassert_core_reset(struct imx_pcie_priv *priv)
{
#if !CONFIG_IS_ENABLED(DM_PCI)
	imx6_pcie_toggle_power();
#endif

	enable_pcie_clock();

	if (priv->variant == IMX6QP)
		imx_pcie_gpr_update_bits(priv, 4, IOMUXC_GPR1_PCIE_SW_RST, 0);

	/*
	 * Wait for the clock to settle a bit, when the clock are sourced
	 * from the CPU, we need about 30 ms to settle.
	 */
	mdelay(50);

	if (priv->variant == IMX6SX) {
		/* SSP_EN is not used on MX6SX anymore */
		imx_pcie_gpr_update_bits(priv, 48, IOMUXC_GPR12_TEST_POWERDOWN, 0);
		/* Clear PCIe PHY reset bit */
		imx_pcie_gpr_update_bits(priv, 20, IOMUXC_GPR5_PCIE_BTNRST, 0);
	} else {
		/* Enable PCIe */
		imx_pcie_gpr_update_bits(priv, 4, IOMUXC_GPR1_TEST_POWERDOWN, 0);
		imx_pcie_gpr_update_bits(priv, 4, IOMUXC_GPR1_REF_SSP_EN, IOMUXC_GPR1_REF_SSP_EN);
	}

#if !CONFIG_IS_ENABLED(DM_PCI)
	imx6_pcie_toggle_reset();
#else
	if (dm_gpio_is_valid(&priv->reset_gpio)) {
		dm_gpio_set_value(&priv->reset_gpio, 1);
		mdelay(20);
		dm_gpio_set_value(&priv->reset_gpio, 0);
		mdelay(20);
	}
#endif

	return 0;
}
#endif

static int imx_pcie_assert_core_reset(struct imx_pcie_priv *priv,
				       bool prepare_for_boot)
{
	switch (priv->variant) {
#ifdef CONFIG_MX6
	case IMX6Q:
	case IMX6QP:
	case IMX6SX:
		return imx6_pcie_assert_core_reset(priv, prepare_for_boot);
#endif
	case IMX8QM:
	case IMX8QXP:
		return imx8_pcie_assert_core_reset(priv, prepare_for_boot);
	default:
		return -EPERM;
	}
}

static int imx_pcie_init_phy(struct imx_pcie_priv *priv)
{
	switch (priv->variant) {
#ifdef CONFIG_MX6
	case IMX6Q:
	case IMX6QP:
	case IMX6SX:
		return imx6_pcie_init_phy(priv);
#endif
	case IMX8QM:
	case IMX8QXP:
		return imx8_pcie_init_phy(priv);
	default:
		return -EPERM;
	}
}

static int imx_pcie_deassert_core_reset(struct imx_pcie_priv *priv)
{
	switch (priv->variant) {
#ifdef CONFIG_MX6
	case IMX6Q:
	case IMX6QP:
	case IMX6SX:
		return imx6_pcie_deassert_core_reset(priv);
#endif
	case IMX8QM:
	case IMX8QXP:
		return imx8_pcie_deassert_core_reset(priv);
	default:
		return -EPERM;
	}
}

static void imx_pcie_ltssm_enable(struct imx_pcie_priv *priv, bool enable)
{
	u32 val;

	switch (priv->variant) {
#ifdef CONFIG_MX6
	case IMX6Q:
	case IMX6SX:
	case IMX6QP:
		if (enable)
			imx_pcie_gpr_update_bits(priv, 48, IOMUXC_GPR12_APPS_LTSSM_ENABLE,
				IOMUXC_GPR12_APPS_LTSSM_ENABLE); /* LTSSM enable, starting link. */
		else
			imx_pcie_gpr_update_bits(priv, 48, IOMUXC_GPR12_APPS_LTSSM_ENABLE, 0);

		break;
#endif
	case IMX8QXP:
	case IMX8QM:
		/* Bit4 of the CTRL2 */
		val = IMX8QM_CSR_PCIEA_OFFSET
			+ priv->ctrl_id * SZ_64K;
		if (enable) {
			imx_pcie_gpr_update_bits(priv,
				val + IMX8QM_CSR_PCIE_CTRL2_OFFSET,
				IMX8QM_CTRL_LTSSM_ENABLE,
				IMX8QM_CTRL_LTSSM_ENABLE);
		} else {
			imx_pcie_gpr_update_bits(priv,
				val + IMX8QM_CSR_PCIE_CTRL2_OFFSET,
				IMX8QM_CTRL_LTSSM_ENABLE,
				0);
		}
		break;
	default:
		break;
	}

}


static int imx_pcie_link_up(struct imx_pcie_priv *priv)
{
	uint32_t tmp;
	int count = 0;

	imx_pcie_assert_core_reset(priv, false);
	imx_pcie_init_phy(priv);
	imx_pcie_deassert_core_reset(priv);

	imx_pcie_setup_ctrl(priv);
	imx_pcie_regions_setup(priv);

	/*
	 * By default, the subordinate is set equally to the secondary
	 * bus (0x01) when the RC boots.
	 * This means that theoretically, only bus 1 is reachable from the RC.
	 * Force the PCIe RC subordinate to 0xff, otherwise no downstream
	 * devices will be detected if the enumeration is applied strictly.
	 */
	tmp = readl(priv->dbi_base + 0x18);
	tmp |= (0xff << 16);
	writel(tmp, priv->dbi_base + 0x18);

	/*
	 * FIXME: Force the PCIe RC to Gen1 operation
	 * The RC must be forced into Gen1 mode before bringing the link
	 * up, otherwise no downstream devices are detected. After the
	 * link is up, a managed Gen1->Gen2 transition can be initiated.
	 */
	tmp = readl(priv->dbi_base + 0x7c);
	tmp &= ~0xf;
	tmp |= 0x1;
	writel(tmp, priv->dbi_base + 0x7c);

	/* LTSSM enable, starting link. */
	imx_pcie_ltssm_enable(priv, true);

	while (!imx6_pcie_link_up(priv)) {
		udelay(10);
		count++;
		if (count == 1000) {
			print_regs(1);
			/* link down, try reset ep, and re-try link here */
			DBGF("pcie link is down, reset ep, then retry!\n");

#if CONFIG_IS_ENABLED(DM_PCI)
			if (dm_gpio_is_valid(&priv->reset_gpio)) {
				dm_gpio_set_value(&priv->reset_gpio, 1);
				mdelay(20);
				dm_gpio_set_value(&priv->reset_gpio, 0);
				mdelay(20);
			}
#elif defined(CONFIG_MX6)
			imx6_pcie_toggle_reset();
#endif
			continue;
		}
#ifdef DEBUG
		else if (count >= 2000) {
			print_regs(1);
			/* link is down, stop here */
			env_set("bootcmd", "sleep 2;");
			DBGF("pcie link is down, stop here!\n");
			imx_pcie_ltssm_enable(priv, false);
			return -EINVAL;
		}
#endif
		if (count >= 4000) {
#ifdef CONFIG_PCI_SCAN_SHOW
			puts("PCI:   pcie phy link never came up\n");
#endif
			debug("DEBUG_R0: 0x%08x, DEBUG_R1: 0x%08x\n",
			      readl(priv->dbi_base + PCIE_PHY_DEBUG_R0),
			      readl(priv->dbi_base + PCIE_PHY_DEBUG_R1));
			imx_pcie_ltssm_enable(priv, false);
			return -EINVAL;
		}
	}

	return 0;
}

#if !CONFIG_IS_ENABLED(DM_PCI)
static struct imx_pcie_priv imx_pcie_priv = {
	.dbi_base	= (void __iomem *)MX6_DBI_ADDR,
	.cfg_base	= (void __iomem *)MX6_ROOT_ADDR,
	.cfg1_base 	= (void __iomem *)(MX6_ROOT_ADDR + MX6_ROOT_SIZE / 2),
	.cfg_size		= MX6_ROOT_SIZE,
	.lanes		= 1,
};

static struct imx_pcie_priv *priv = &imx_pcie_priv;


static int imx_pcie_read_config(struct pci_controller *hose, pci_dev_t d,
				int where, u32 *val)
{
	struct imx_pcie_priv *priv = hose->priv_data;

	return imx_pcie_read_cfg(priv, d, where, val);
}

static int imx_pcie_write_config(struct pci_controller *hose, pci_dev_t d,
				 int where, u32 val)
{
	struct imx_pcie_priv *priv = hose->priv_data;

	return imx_pcie_write_cfg(priv, d, where, val);
}

void imx_pcie_init(void)
{
	/* Static instance of the controller. */
	static struct pci_controller	pcc;
	struct pci_controller		*hose = &pcc;
	int ret;
#ifdef DEBUG_STRESS_WR
	u32 dbg_reg_addr = SNVS_LPGRP;
	u32 dbg_reg = readl(dbg_reg_addr) + 1;
#endif

	memset(&pcc, 0, sizeof(pcc));

	if (is_mx6sx())
		priv->variant = IMX6SX;
	else if (is_mx6dqp())
		priv->variant = IMX6QP;
	else
		priv->variant = IMX6Q;

	hose->priv_data = priv;

	/* PCI I/O space */
	pci_set_region(&hose->regions[0],
		       0, MX6_IO_ADDR,
		       MX6_IO_SIZE, PCI_REGION_IO);

	/* PCI memory space */
	pci_set_region(&hose->regions[1],
		       MX6_MEM_ADDR, MX6_MEM_ADDR,
		       MX6_MEM_SIZE, PCI_REGION_MEM);

	/* System memory space */
	pci_set_region(&hose->regions[2],
		       MMDC0_ARB_BASE_ADDR, MMDC0_ARB_BASE_ADDR,
		       0xefffffff, PCI_REGION_MEM | PCI_REGION_SYS_MEMORY);

	priv->io = &hose->regions[0];
	priv->mem = &hose->regions[1];

	hose->region_count = 3;

	pci_set_ops(hose,
		    pci_hose_read_config_byte_via_dword,
		    pci_hose_read_config_word_via_dword,
		    imx_pcie_read_config,
		    pci_hose_write_config_byte_via_dword,
		    pci_hose_write_config_word_via_dword,
		    imx_pcie_write_config);

	/* Start the controller. */
	ret = imx_pcie_link_up(priv);

	if (!ret) {
		pci_register_hose(hose);
		hose->last_busno = pci_hose_scan(hose);
#ifdef DEBUG_STRESS_WR
		dbg_reg += 1<<16;
#endif
	}
#ifdef DEBUG_STRESS_WR
	writel(dbg_reg, dbg_reg_addr);
	DBGF("PCIe Successes/Attempts: %d/%d\n",
			dbg_reg >> 16, dbg_reg & 0xffff);
#endif
}

void imx_pcie_remove(void)
{
	imx6_pcie_assert_core_reset(priv, true);
}

/* Probe function. */
void pci_init_board(void)
{
	imx_pcie_init();
}

int pci_skip_dev(struct pci_controller *hose, pci_dev_t dev)
{
	return 0;
}

#else
static int imx_pcie_dm_read_config(const struct udevice *dev, pci_dev_t bdf,
				   uint offset, ulong *value,
				   enum pci_size_t size)
{
	struct imx_pcie_priv *priv = dev_get_priv(dev);
	u32 tmpval;
	int ret;

	ret = imx_pcie_read_cfg(priv, bdf, offset, &tmpval);
	if (ret)
		return ret;

	*value = pci_conv_32_to_size(tmpval, offset, size);
	return 0;
}

static int imx_pcie_dm_write_config(struct udevice *dev, pci_dev_t bdf,
				    uint offset, ulong value,
				    enum pci_size_t size)
{
	struct imx_pcie_priv *priv = dev_get_priv(dev);
	u32 tmpval, newval;
	int ret;

	ret = imx_pcie_read_cfg(priv, bdf, offset, &tmpval);
	if (ret)
		return ret;

	newval = pci_conv_size_to_32(tmpval, value, offset, size);
	return imx_pcie_write_cfg(priv, bdf, offset, newval);
}

static int imx_pcie_dm_probe(struct udevice *dev)
{
	int ret = 0;
	struct imx_pcie_priv *priv = dev_get_priv(dev);

#if CONFIG_IS_ENABLED(DM_REGULATOR)
	ret = device_get_supply_regulator(dev, "epdev_on", &priv->epdev_on);
	if (ret) {
		priv->epdev_on = NULL;
		dev_dbg(dev, "no epdev_on\n");
	} else {
		ret = regulator_set_enable(priv->epdev_on, true);
		if (ret) {
			dev_err(dev, "fail to enable epdev_on\n");
			return ret;
		}
	}

	mdelay(100);
#endif

	/* Enable the osc clk */
	ret = gpio_request_by_name(dev, "clkreq-gpio", 0, &priv->clkreq_gpio,
				   (GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE));
	if (ret) {
		dev_info(dev, "%d unable to get clkreq.\n", ret);
	}

	/* enable */
	ret = gpio_request_by_name(dev, "disable-gpio", 0, &priv->dis_gpio,
				   (GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE));
	if (ret) {
		dev_info(dev, "%d unable to get disable-gpio.\n", ret);
	}

	/* Set to power on */
	ret = gpio_request_by_name(dev, "power-on-gpio", 0, &priv->power_on_gpio,
				   (GPIOD_IS_OUT |GPIOD_IS_OUT_ACTIVE));
	if (ret) {
		dev_info(dev, "%d unable to get power-on-gpio.\n", ret);
	}

	/* Set to reset status */
	ret = gpio_request_by_name(dev, "reset-gpio", 0, &priv->reset_gpio,
				   (GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE));
	if (ret) {
		dev_info(dev, "%d unable to get power-on-gpio.\n", ret);
	}

#if CONFIG_IS_ENABLED(CLK)
	ret = clk_get_by_name(dev, "pcie_phy", &priv->pcie_phy);
	if (ret) {
		printf("Failed to get pcie_phy clk\n");
		return ret;
	}

	ret = clk_get_by_name(dev, "pcie_bus", &priv->pcie_bus);
	if (ret) {
		printf("Failed to get pcie_bus clk\n");
		return ret;
	}

	ret = clk_get_by_name(dev, "pcie", &priv->pcie);
	if (ret) {
		printf("Failed to get pcie clk\n");
		return ret;
	}
#endif

	if (priv->variant == IMX8QM || priv->variant == IMX8QXP) {
#if CONFIG_IS_ENABLED(CLK)
		ret = clk_get_by_name(dev, "pcie_per", &priv->pcie_per);
		if (ret) {
			printf("Failed to get pcie_per clk\n");
			return ret;
		}

		ret = clk_get_by_name(dev, "pcie_inbound_axi", &priv->pcie_inbound_axi);
		if (ret) {
			printf("Failed to get pcie_inbound_axi clk\n");
			return ret;
		}

		ret = clk_get_by_name(dev, "phy_per", &priv->phy_per);
		if (ret) {
			printf("Failed to get phy_per clk\n");
			return ret;
		}

		ret = clk_get_by_name(dev, "misc_per", &priv->misc_per);
		if (ret) {
			printf("Failed to get misc_per clk\n");
			return ret;
		}

		if (priv->variant == IMX8QM && priv->ctrl_id == 1) {
			ret = clk_get_by_name(dev, "pcie_phy_pclk", &priv->pcie_phy_pclk);
			if (ret) {
				printf("Failed to get pcie_phy_pclk clk\n");
				return ret;
			}

			ret = clk_get_by_name(dev, "pciex2_per", &priv->pciex2_per);
			if (ret) {
				printf("Failed to get pciex2_per clk\n");
				return ret;
			}
		}
#endif
		priv->iomuxc_gpr =
			 syscon_regmap_lookup_by_phandle(dev, "hsio");
		if (IS_ERR(priv->iomuxc_gpr)) {
			dev_err(dev, "unable to find gpr registers\n");
			return PTR_ERR(priv->iomuxc_gpr);
		}
	} else {
#if CONFIG_IS_ENABLED(DM_REGULATOR)
		if (priv->variant == IMX6QP) {
			ret = device_get_supply_regulator(dev, "pcie-bus", &priv->pcie_bus_regulator);
			if (ret) {
				dev_dbg(dev, "no pcie_bus_regulator\n");
				priv->pcie_bus_regulator = NULL;
			}
		} else if (priv->variant == IMX6SX) {
			ret = device_get_supply_regulator(dev, "pcie-phy", &priv->pcie_phy_regulator);
			if (ret) {
				dev_dbg(dev, "no pcie_phy_regulator\n");
				priv->pcie_phy_regulator = NULL;
			}
		}
#endif

		priv->iomuxc_gpr =
			 syscon_regmap_lookup_by_phandle(dev, "gpr");
		if (IS_ERR(priv->iomuxc_gpr)) {
			dev_err(dev, "unable to find gpr registers\n");
			return PTR_ERR(priv->iomuxc_gpr);
		}
	}

	pci_get_regions(dev, &priv->io, &priv->mem, &priv->pref);

	if (priv->cpu_base)
		priv->cpu_addr_offset = priv->cpu_base
				- priv->mem->phys_start;
	else
		priv->cpu_addr_offset = 0;

	return imx_pcie_link_up(priv);
}

static int imx_pcie_dm_remove(struct udevice *dev)
{
	struct imx_pcie_priv *priv = dev_get_priv(dev);

	imx_pcie_assert_core_reset(priv, true);

	return 0;
}

static int imx_pcie_ofdata_to_platdata(struct udevice *dev)
{
	struct imx_pcie_priv *priv = dev_get_priv(dev);
	int ret;
	struct resource cfg_res;

	priv->dbi_base = (void __iomem *)devfdt_get_addr_index(dev, 0);
	if (!priv->dbi_base)
		return -EINVAL;

	ret = dev_read_resource_byname(dev,  "config", &cfg_res);
	if (ret) {
		printf("can't get config resource(ret = %d)\n", ret);
		return -ENOMEM;
	}

	priv->cfg_base = map_physmem(cfg_res.start,
				 resource_size(&cfg_res),
				 MAP_NOCACHE);
	priv->cfg1_base = priv->cfg_base + resource_size(&cfg_res) / 2;
	priv->cfg_size = resource_size(&cfg_res);

	priv->variant = (enum imx_pcie_variants)dev_get_driver_data(dev);

	if (dev_read_u32u(dev, "hsio-cfg", &priv->hsio_cfg))
		priv->hsio_cfg = 0;

	if (dev_read_u32u(dev, "ctrl-id", &priv->ctrl_id))
		priv->ctrl_id = 0;

	if (dev_read_u32u(dev, "ext_osc", &priv->ext_osc))
		priv->ext_osc = 0;

	if (dev_read_u32u(dev, "cpu-base-addr", &priv->cpu_base))
		priv->cpu_base = 0;

	if (dev_read_u32u(dev, "num-lanes", &priv->lanes))
		priv->lanes = 1;

	debug("hsio-cfg %u, ctrl-id %u, ext_osc %u, cpu-base 0x%x\n",
		priv->hsio_cfg, priv->ctrl_id, priv->ext_osc, priv->cpu_base);

	return 0;
}

static const struct dm_pci_ops imx_pcie_ops = {
	.read_config	= imx_pcie_dm_read_config,
	.write_config	= imx_pcie_dm_write_config,
};

static const struct udevice_id imx_pcie_ids[] = {
	{ .compatible = "fsl,imx6q-pcie",  .data = (ulong)IMX6Q,  },
	{ .compatible = "fsl,imx6sx-pcie", .data = (ulong)IMX6SX, },
	{ .compatible = "fsl,imx6qp-pcie", .data = (ulong)IMX6QP, },
	{ .compatible = "fsl,imx8qm-pcie", .data = (ulong)IMX8QM, },
	{ .compatible = "fsl,imx8qxp-pcie", .data = (ulong)IMX8QXP, },
	{ }
};

U_BOOT_DRIVER(imx_pcie) = {
	.name			= "imx_pcie",
	.id			= UCLASS_PCI,
	.of_match		= imx_pcie_ids,
	.ops			= &imx_pcie_ops,
	.probe			= imx_pcie_dm_probe,
	.remove			= imx_pcie_dm_remove,
	.ofdata_to_platdata	= imx_pcie_ofdata_to_platdata,
	.priv_auto_alloc_size	= sizeof(struct imx_pcie_priv),
	.flags			= DM_FLAG_OS_PREPARE,
};
#endif
