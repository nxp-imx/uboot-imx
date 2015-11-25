// SPDX-License-Identifier: GPL-2.0+
/*
 * Freescale S32V234 PCI Express Root-Complex driver
 *
 * Copyright (C) 2013 Marek Vasut <marex@denx.de>
 * Copyright 2020 NXP
 *
 * Based on upstream iMX U-Boot driver:
 * pcie_imx.c:		Marek Vasut <marex@denx.de>
 *
 */

#include <common.h>
#include <errno.h>
#include <pci.h>
#include <asm/arch/clock.h>
#include <asm/arch/siul.h>
#include <asm/arch/src.h>
#include <asm/arch/mc_me_regs.h>
#include <asm/arch/mc_cgm_regs.h>
#include <asm/io.h>
#include <linux/sizes.h>

#define PCI_ACCESS_READ  0
#define PCI_ACCESS_WRITE 1

#define MMDC0_ARB_BASE_ADDR	0x80000000
#define S32V234_DBI_ADDR	0x72FFC000
#define S32V234_IO_ADDR		0x72000000
#define S32V234_MEM_ADDR	0x72100000
#define S32V234_ROOT_ADDR	0x72f00000
#define S32V234_DBI_SIZE	0x4000
#define S32V234_IO_SIZE		0x100000
#define S32V234_MEM_SIZE	0xe00000
#define S32V234_ROOT_SIZE	0xfc000

/* PCIe Port Logic registers (memory-mapped) */
#define PL_OFFSET				0x700
#define PCIE_PHY_DEBUG_R0			(PL_OFFSET + 0x28)
#define PCIE_PHY_DEBUG_R1			(PL_OFFSET + 0x2c)
#define PCIE_PHY_DEBUG_R1_LINK_UP		BIT(4)
#define PCIE_PHY_DEBUG_R1_LINK_IN_TRAINING	BIT(29)

#define PCIE_PHY_CTRL			(PL_OFFSET + 0x114)
#define PCIE_PHY_CTRL_DATA_LOC		0
#define PCIE_PHY_CTRL_CAP_ADR_LOC	16
#define PCIE_PHY_CTRL_CAP_DAT_LOC	17
#define PCIE_PHY_CTRL_WR_LOC		18
#define PCIE_PHY_CTRL_RD_LOC		19

#define PCIE_PHY_STAT			(PL_OFFSET + 0x110)
#define PCIE_PHY_STAT_DATA_LOC		0
#define PCIE_PHY_STAT_ACK_LOC		16

/* PHY registers (not memory-mapped) */
#define PCIE_PHY_RX_ASIC_OUT		0x100D

#define PHY_RX_OVRD_IN_LO		0x1005
#define PHY_RX_OVRD_IN_LO_RX_DATA_EN	BIT(5)
#define PHY_RX_OVRD_IN_LO_RX_PLL_EN	BIT(3)

/* iATU registers */
#define PCIE_ATU_VIEWPORT		0x900
#define PCIE_ATU_REGION_INBOUND		(0x1 << 31)
#define PCIE_ATU_REGION_OUTBOUND	(0x0 << 31)
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

#ifdef CONFIG_PCIE_EP_MODE
#define MSI_REGION			0x72FB0000
#define PCI_BASE_ADDR		0x72000000
#define PCI_BASE_DBI		0x72FFC000
#define MSI_REGION_NR	3
#define NR_REGIONS		4
#define PCI_REGION_MEM			0x00000000	/* PCI mem space */
#define PCI_REGION_IO			0x00000001	/* PCI IO space */
#define PCI_WIDTH_32b			0x00000000	/* 32-bit BAR */
#define PCI_WIDTH_64b			0x00000004	/* 64-bit BAR */
#define PCI_REGION_PREFETCH		0x00000008	/* prefetch PCI mem */
#define PCI_REGION_NON_PREFETCH	0x00000000	/* non-prefetch PCI mem */
#define PCIE_BAR0_SIZE		SZ_1M	/* 1MB */
#define PCIE_BAR1_SIZE		0
#define PCIE_BAR2_SIZE		SZ_1M	/* 1MB */
#define PCIE_BAR3_SIZE		0		/* 256B Fixed sizing  */
#define PCIE_BAR4_SIZE		0		/* 4K Fixed sizing  */
#define PCIE_BAR5_SIZE		0		/* 64K Fixed sizing  */
#define PCIE_BAR0_EN_DIS		1
#define PCIE_BAR1_EN_DIS		0
#define PCIE_BAR2_EN_DIS		1
#define PCIE_BAR3_EN_DIS		1
#define PCIE_BAR4_EN_DIS		1
#define PCIE_BAR5_EN_DIS		1
#define PCIE_BAR0_INIT	(PCI_REGION_MEM | PCI_WIDTH_32b | \
		PCI_REGION_NON_PREFETCH)
#define PCIE_BAR1_INIT	(PCI_REGION_MEM | PCI_WIDTH_32b | \
		PCI_REGION_NON_PREFETCH)
#define PCIE_BAR2_INIT	(PCI_REGION_MEM | PCI_WIDTH_32b | \
		PCI_REGION_NON_PREFETCH)
#define PCIE_BAR3_INIT	(PCI_REGION_MEM | PCI_WIDTH_32b | \
		PCI_REGION_NON_PREFETCH)
#define PCIE_BAR4_INIT	0
#define PCIE_BAR5_INIT	0
#endif
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
static int pcie_phy_read(void __iomem *dbi_base, int addr, int *data)
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

static int s32v234_pcie_link_up(void)
{
	u32 rc, ltssm;
	int rx_valid, temp;

	/* link is debug bit 36, debug register 1 starts at bit 32 */
	rc = readl(S32V234_DBI_ADDR + PCIE_PHY_DEBUG_R1);
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
	pcie_phy_read((void *)S32V234_DBI_ADDR,
		      PCIE_PHY_RX_ASIC_OUT, &rx_valid);
	ltssm = readl(S32V234_DBI_ADDR + PCIE_PHY_DEBUG_R0) & 0x3F;

	if (rx_valid & 0x01)
		return 0;

	if (ltssm != 0x0d)
		return 0;

	printf("transition to gen2 is stuck, reset PHY!\n");

	pcie_phy_read((void *)S32V234_DBI_ADDR, PHY_RX_OVRD_IN_LO, &temp);
	temp |= (PHY_RX_OVRD_IN_LO_RX_DATA_EN | PHY_RX_OVRD_IN_LO_RX_PLL_EN);
	pcie_phy_write((void *)S32V234_DBI_ADDR, PHY_RX_OVRD_IN_LO, temp);

	mdelay(3);

	pcie_phy_read((void *)S32V234_DBI_ADDR, PHY_RX_OVRD_IN_LO, &temp);
	temp &= ~(PHY_RX_OVRD_IN_LO_RX_DATA_EN | PHY_RX_OVRD_IN_LO_RX_PLL_EN);
	pcie_phy_write((void *)S32V234_DBI_ADDR, PHY_RX_OVRD_IN_LO, temp);

	return 0;
}

/*
 * iATU region setup
 */
static int s32v234_pcie_regions_setup(void)
{
	/*
	 * S32V234 defines 16MB in the AXI address map for PCIe.
	 *
	 * That address space excepted the pcie registers is
	 * split and defined into different regions by iATU,
	 * with sizes and offsets as follows:
	 *
	 * 0x0100_0000 --- 0x010F_FFFF 1MB IORESOURCE_IO
	 * 0x0110_0000 --- 0x01EF_FFFF 14MB IORESOURCE_MEM
	 * 0x01F0_0000 --- 0x01FF_FFFF 1MB Cfg + Registers
	 */

	/* CMD reg:I/O space, MEM space, and Bus Master Enable */
	setbits_le32(S32V234_DBI_ADDR | PCI_COMMAND,
		     PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
#ifndef CONFIG_PCIE_EP_MODE
	/* Set the CLASS_REV of RC CFG header to PCI_CLASS_BRIDGE_PCI */
	setbits_le32(S32V234_DBI_ADDR + PCI_CLASS_REVISION,
		     PCI_CLASS_BRIDGE_PCI << 16);
#else
		/* Set the CLASS_REV of RC CFG header to PCI_CLASS_MEMORY_RAM */
		setbits_le32(S32V234_DBI_ADDR + PCI_CLASS_REVISION,
			     PCI_CLASS_MEMORY_RAM << 16);
		/* 1MB  32bit none-prefetchable memory on BAR0 */
		writel(PCIE_BAR0_INIT, S32V234_DBI_ADDR + PCI_BASE_ADDRESS_0);
#if PCIE_BAR0_EN_DIS
		writel((PCIE_BAR0_EN_DIS | (PCIE_BAR0_SIZE - 1)),
		       S32V234_DBI_ADDR + (1 << 12) + PCI_BASE_ADDRESS_0);
#else
		writel((PCIE_BAR0_EN_DIS & (PCIE_BAR0_SIZE - 1)),
		       S32V234_DBI_ADDR + (1 << 12) + PCI_BASE_ADDRESS_0);
#endif

		/* None used BAR1 */
		writel(PCIE_BAR1_INIT, S32V234_DBI_ADDR + PCI_BASE_ADDRESS_1);
#if PCIE_BAR1_EN_DIS
		writel((PCIE_BAR1_EN_DIS | (PCIE_BAR1_SIZE - 1)),
		       S32V234_DBI_ADDR + (1 << 12) + PCI_BASE_ADDRESS_1);
#else
		writel((PCIE_BAR1_EN_DIS & (PCIE_BAR1_SIZE - 1)),
		       S32V234_DBI_ADDR + (1 << 12) + PCI_BASE_ADDRESS_1);
#endif

		/* 64KB 32bit none-prefetchable memory on BAR2 */
		writel(PCIE_BAR2_INIT, S32V234_DBI_ADDR + PCI_BASE_ADDRESS_2);
#if PCIE_BAR2_EN_DIS
		writel((PCIE_BAR2_EN_DIS | (PCIE_BAR2_SIZE - 1)),
		       S32V234_DBI_ADDR + (1 << 12) + PCI_BASE_ADDRESS_2);
#else
		writel((PCIE_BAR2_EN_DIS & (PCIE_BAR2_SIZE - 1)),
		       S32V234_DBI_ADDR + (1 << 12) + PCI_BASE_ADDRESS_2);
#endif

		/* Default size BAR3 */
		writel(PCIE_BAR3_INIT, S32V234_DBI_ADDR + PCI_BASE_ADDRESS_3);
#if PCIE_BAR3_EN_DIS
		writel((PCIE_BAR3_EN_DIS | (PCIE_BAR3_SIZE - 1)),
		       S32V234_DBI_ADDR + (1 << 12) + PCI_BASE_ADDRESS_3);
#else
		writel((PCIE_BAR3_EN_DIS & (PCIE_BAR3_SIZE - 1)),
		       S32V234_DBI_ADDR + (1 << 12) + PCI_BASE_ADDRESS_3);
#endif

		/* Default size  BAR4 */
		writel(PCIE_BAR4_INIT, S32V234_DBI_ADDR + PCI_BASE_ADDRESS_4);
#if PCIE_BAR4_EN_DIS
		writel((PCIE_BAR4_EN_DIS | (PCIE_BAR4_SIZE - 1)),
		       S32V234_DBI_ADDR + (1 << 12) + PCI_BASE_ADDRESS_4);
#else
		writel((PCIE_BAR4_EN_DIS & (PCIE_BAR4_SIZE - 1)),
		       S32V234_DBI_ADDR + (1 << 12) + PCI_BASE_ADDRESS_4);
#endif

		/* Default size BAR5 */
		writel(PCIE_BAR5_INIT, S32V234_DBI_ADDR + PCI_BASE_ADDRESS_5);
#if PCIE_BAR5_EN_DIS
		writel((PCIE_BAR5_EN_DIS | (PCIE_BAR5_SIZE - 1)),
		       S32V234_DBI_ADDR + (1 << 12) + PCI_BASE_ADDRESS_5);
#else
		writel((PCIE_BAR5_EN_DIS & (PCIE_BAR5_SIZE - 1)),
		       S32V234_DBI_ADDR + (1 << 12) + PCI_BASE_ADDRESS_5);
#endif
#endif /* CONFIG_PCIE_EP_MODE */

#ifndef CONFIG_PCIE_EP_MODE
	/* Region #0 is used for Outbound CFG space access. */
	writel(0, S32V234_DBI_ADDR + PCIE_ATU_VIEWPORT);
	writel(S32V234_ROOT_ADDR, S32V234_DBI_ADDR + PCIE_ATU_LOWER_BASE);
	writel(0, S32V234_DBI_ADDR + PCIE_ATU_UPPER_BASE);
	writel(S32V234_ROOT_ADDR + S32V234_ROOT_SIZE,
	       S32V234_DBI_ADDR + PCIE_ATU_LIMIT);
	writel(0, S32V234_DBI_ADDR + PCIE_ATU_LOWER_TARGET);
	writel(0, S32V234_DBI_ADDR + PCIE_ATU_UPPER_TARGET);
	writel(PCIE_ATU_TYPE_CFG0, S32V234_DBI_ADDR + PCIE_ATU_CR1);
	writel(PCIE_ATU_ENABLE, S32V234_DBI_ADDR + PCIE_ATU_CR2);
#else
	/* Region #0 is used for Inbound Mem space access on BAR2. */
	writel(0x80000000, S32V234_DBI_ADDR + PCIE_ATU_VIEWPORT);
	writel(0xcff00000, S32V234_DBI_ADDR + PCIE_ATU_LOWER_TARGET);
	writel(0, S32V234_DBI_ADDR + PCIE_ATU_UPPER_TARGET);
	writel(PCIE_ATU_TYPE_MEM, S32V234_DBI_ADDR + PCIE_ATU_CR1);
	writel(0xC0000200, S32V234_DBI_ADDR + PCIE_ATU_CR2);
#endif
	return 0;
}

#ifndef CONFIG_PCIE_EP_MODE
/*
 * PCI Express accessors
 */
static u32 get_bus_address(pci_dev_t d, int where)
{
	u32 va_address;

	/* Reconfigure Region #0 */
	writel(0, S32V234_DBI_ADDR + PCIE_ATU_VIEWPORT);

	if (PCI_BUS(d) < 2)
		writel(PCIE_ATU_TYPE_CFG0, S32V234_DBI_ADDR + PCIE_ATU_CR1);
	else
		writel(PCIE_ATU_TYPE_CFG1, S32V234_DBI_ADDR + PCIE_ATU_CR1);

	if (PCI_BUS(d) == 0) {
		va_address = S32V234_DBI_ADDR;
	} else {
		writel(d << 8, S32V234_DBI_ADDR + PCIE_ATU_LOWER_TARGET);
		va_address = S32V234_IO_ADDR + SZ_16M - SZ_1M;
	}

	va_address += (where & ~0x3);

	return va_address;
}

static int s32v234_pcie_addr_valid(pci_dev_t d)
{
	if ((PCI_BUS(d) == 0) && (PCI_DEV(d) > 1))
		return -EINVAL;
	if ((PCI_BUS(d) == 1) && (PCI_DEV(d) > 0))
		return -EINVAL;
	return 0;
}

static int s32v234_pcie_read_config(struct pci_controller *hose, pci_dev_t d,
				    int where, u32 *val)
{
	u32 va_address;
	int ret;

	ret = s32v234_pcie_addr_valid(d);
	if (ret) {
		*val = 0xffffffff;
		return ret;
	}

	va_address = get_bus_address(d, where);
	writel(0xffffffff, val);
	*val = readl(va_address);

	return 0;
}

static int s32v234_pcie_write_config(struct pci_controller *hose, pci_dev_t d,
				     int where, u32 val)
{
	u32 va_address = 0;
	int ret;

	ret = s32v234_pcie_addr_valid(d);
	if (ret)
		return ret;

	va_address = get_bus_address(d, where);
	writel(val, va_address);

	return 0;
}
#endif /* CONFIG_PCIE_EP_MODE */

static int s32v234_pcie_init_phy(void)
{
	struct src *src_regs = (struct src *)SRC_SOC_BASE_ADDR;

	clrbits_le32(&src_regs->gpr5, SRC_GPR5_PCIE_APP_LTSSM_ENABLE);

#ifndef CONFIG_PCIE_EP_MODE
	clrsetbits_le32(&src_regs->gpr5,
			SRC_GPR5_PCIE_DEVICE_TYPE_MASK,
			SRC_GPR5_PCIE_DEVICE_TYPE_RC);
#else
	clrsetbits_le32(&src_regs->gpr5,
			SRC_GPR5_PCIE_DEVICE_TYPE_MASK,
			SRC_GPR5_PCIE_DEVICE_TYPE_EP);
#endif
	 clrsetbits_le32(&src_regs->gpr5,
			 SRC_GPR5_PCIE_PHY_LOS_LEVEL_MASK,
			 SRC_GPR5_PCIE_PHY_LOS_LEVEL_9);
	 clrsetbits_le32(&src_regs->gpr5,
			 SRC_GPR5_PCIE_PHY_RX0_EQ_MASK,
			 SRC_GPR5_PCIE_PHY_RX0_EQ_2);

	 writel((0x0 << SRC_GPR6_PCIE_PCS_TX_DEEMPH_GEN1_OFFSET) |
		(0x0 << SRC_GPR6_PCIE_PCS_TX_DEEMPH_GEN2_3P5DB_OFFSET) |
		(20 << SRC_GPR6_PCIE_PCS_TX_DEEMPH_GEN2_6DB_OFFSET) |
		(127 << SRC_GPR6_PCIE_PCS_TX_SWING_FULL_OFFSET) |
		(127 << SRC_GPR6_PCIE_PCS_TX_SWING_LOW_OFFSET),
		&src_regs->gpr6);

	return 0;
}

static int s32v234_pcie_deassert_core_reset(void)
{
	struct src *src_regs = (struct src *)SRC_SOC_BASE_ADDR;

	/* Enable PCIe */

	clrbits_le32(&src_regs->gpr5, SRC_GPR5_GPR_PCIE_BUTTON_RST_N);
	mdelay(50);
	return 0;
}

static int s32v_pcie_link_up(void)
{
	struct src *src_regs = (struct src *)SRC_SOC_BASE_ADDR;

	u32 tmp;
	int count = 0;

	s32v234_pcie_init_phy();
	s32v234_pcie_deassert_core_reset();
	s32v234_pcie_regions_setup();

#ifdef CONFIG_PCIE_EP_MODE
	writel(readl(&src_regs->gpr11) | 0x00400000, &src_regs->gpr11);
#endif

	/*
	 * FIXME: Force the PCIe RC to Gen1 operation
	 * The RC must be forced into Gen1 mode before bringing the link
	 * up, otherwise no downstream devices are detected. After the
	 * link is up, a managed Gen1->Gen2 transition can be initiated.
	 */
	printf("\n Forcing PCIe RC to Gen1 operation");

	tmp = readl(S32V234_DBI_ADDR + 0x7c);
	tmp &= ~0xf;
	tmp |= 0x1;
	writel(tmp, S32V234_DBI_ADDR + 0x7c);

	/* LTSSM enable, starting link. */
	setbits_le32(&src_regs->gpr5, SRC_GPR5_PCIE_APP_LTSSM_ENABLE);

	while (!s32v234_pcie_link_up()) {
		udelay(10);
		count++;
		if (count >= 2000) {
			printf("phy link never came up\n");
			printf("DEBUG_R0: 0x%08x, DEBUG_R1: 0x%08x\n",
			       readl(S32V234_DBI_ADDR + PCIE_PHY_DEBUG_R0),
			       readl(S32V234_DBI_ADDR + PCIE_PHY_DEBUG_R1));
			return -EINVAL;
		}
	}

	return 0;
}

void s32v234_pcie_init(void)
{
#ifndef CONFIG_PCIE_EP_MODE
	/* Static instance of the controller. */
	static struct pci_controller	pcc;
	struct pci_controller		*hose = &pcc;
#endif
	int ret;

#ifndef CONFIG_PCIE_EP_MODE
	memset(&pcc, 0, sizeof(pcc));

	/* PCI I/O space */
	pci_set_region(&hose->regions[0],
		       S32V234_IO_ADDR, S32V234_IO_ADDR,
		       S32V234_IO_SIZE, PCI_REGION_IO);

	/* PCI memory space */
	pci_set_region(&hose->regions[1],
		       S32V234_MEM_ADDR, S32V234_MEM_ADDR,
		       S32V234_MEM_SIZE, PCI_REGION_MEM);

	/* System memory space */
	/* For now, allocating only 1024MB from address 0x80000000 */

	pci_set_region(&hose->regions[2],
		       MMDC0_ARB_BASE_ADDR, MMDC0_ARB_BASE_ADDR,
		       0x3FFFFFFF, PCI_REGION_MEM | PCI_REGION_SYS_MEMORY);

	hose->region_count = 3;

	pci_set_ops(hose,
		    pci_hose_read_config_byte_via_dword,
		    pci_hose_read_config_word_via_dword,
		    s32v234_pcie_read_config,
		    pci_hose_write_config_byte_via_dword,
		    pci_hose_write_config_word_via_dword,
		    s32v234_pcie_write_config);
#endif
	/* Start the controller. */
	ret = s32v_pcie_link_up();
#ifndef CONFIG_PCIE_EP_MODE
	if (!ret) {
		pci_register_hose(hose);
		hose->last_busno = pci_hose_scan(hose);
	}
#endif
}

void pci_init_board(void)
{
	s32v234_pcie_init();
}
