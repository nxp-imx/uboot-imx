// SPDX-License-Identifier: GPL-2.0+
/*
 * Freescale S32V234 PCI Express Root-Complex driver
 *
 * Copyright (C) 2016 Heinz Wrobel <heinz.wrobel@nxp.com>
 * Copyright (C) 2015 Aurelian Voicu <aurelian.voicu@nxp.com>
 * Copyright (C) 2016,2020 NXP
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
#include <asm/gicsupport.h>
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

/* The following defines are used for EP mode only */
#define MSI_REGION			0x72FB0000
#define PCI_BASE_ADDR			0x72000000
#define PCI_BASE_DBI			0x72FFC000
#define MSI_REGION_NR			3
#define NR_REGIONS			4
#define PCI_REGION_MEM			0x00000000 /* PCI mem space */
#define PCI_REGION_IO			0x00000001 /* PCI IO space */
#define PCI_WIDTH_32b			0x00000000 /* 32-bit BAR */
#define PCI_WIDTH_64b			0x00000004 /* 64-bit BAR */
#define PCI_REGION_PREFETCH		0x00000008 /* prefetch PCI mem */
#define PCI_REGION_NON_PREFETCH		0x00000000 /* non-prefetch PCI mem */
#define PCIE_BAR0_SIZE			SZ_1M		/* 1MB */
#define PCIE_BAR1_SIZE			0
#define PCIE_BAR2_SIZE			SZ_1M		/* 1MB */
#define PCIE_BAR3_SIZE			0
#define PCIE_BAR4_SIZE			0
#define PCIE_BAR5_SIZE			0
#define PCIE_ROM_SIZE			0
#define PCIE_BAR0_EN_DIS		1
#define PCIE_BAR1_EN_DIS		0
#define PCIE_BAR2_EN_DIS		1
#define PCIE_BAR3_EN_DIS		1
#define PCIE_BAR4_EN_DIS		1
#define PCIE_BAR5_EN_DIS		1
#define PCIE_ROM_EN_DIS			0
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
#define PCIE_ROM_INIT	0

/* To do proper EP support, we need to have interrupt driven handlers
 * to keep our EP configuration in proper shape.
 */
#define PCIE_INTERRUPT_link_req_rst_not         135

/* Global variables */
static int ignoreERR009852;

/*
 * PHY access functions
 */
 /* FIX: The RM does not document any of this. How should it be
  * possible to understand any of the PHY handling if it is a hard
  * requirement. Do we have to reinstate PHY docs in the manual?
  * If so, what parts?
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

static void s32v234_pcie_set_bar(int baroffset, int enable, unsigned int size,
				 unsigned int init)
{
	char __iomem *dbi_base = (char __iomem *)S32V234_DBI_ADDR;
	u32 mask = (enable) ? ((size - 1) & ~1) : 0;

	/* According to the RM, you have to enable the BAR before you
	 * can modify the mask value. While it appears that this may
	 * be ok in a single write anyway, we play it safe.
	 */
	writel(1, dbi_base + 0x1000 + baroffset);

	writel(enable | mask, dbi_base + 0x1000 + baroffset);
	writel(init, dbi_base + baroffset);
}

static void set_non_sticky_config_regs(void)
{
	const int socmask_info = readl(SIUL2_MIDR1) & 0x000000ff;
	const struct src *src_regs = (struct src *)SRC_SOC_BASE_ADDR;
	const int ep_mode = (readl(&src_regs->gpr5) & 0x0000001c) == 0;

	/* We need this function because the PCIe IP loses some
	 * configuration values when it loses the link.
	 * THIS FUNCTION MUST BE INTERRUPT SAFE, so we don't use
	 * external complex functions.
	 */
	if (!ep_mode) {
		/* Set the CLASS_REV of RC CFG header to PCI_CLASS_BRIDGE_PCI */
		setbits_le32(S32V234_DBI_ADDR + PCI_CLASS_REVISION,
			     PCI_CLASS_BRIDGE_PCI << 16);

		/* CMD reg:I/O space, MEM space, and Bus Master Enable */
		setbits_le32(S32V234_DBI_ADDR | PCI_COMMAND,
			     PCI_COMMAND_IO | PCI_COMMAND_MEMORY |
			     PCI_COMMAND_MASTER);

		/* Region #0 is used for Outbound CFG space access. */
		writel(0, S32V234_DBI_ADDR + PCIE_ATU_VIEWPORT);
		writel(S32V234_ROOT_ADDR,
		       S32V234_DBI_ADDR + PCIE_ATU_LOWER_BASE);
		writel(0, S32V234_DBI_ADDR + PCIE_ATU_UPPER_BASE);
		writel(S32V234_ROOT_ADDR + S32V234_ROOT_SIZE,
		       S32V234_DBI_ADDR + PCIE_ATU_LIMIT);
		writel(0, S32V234_DBI_ADDR + PCIE_ATU_LOWER_TARGET);
		writel(0, S32V234_DBI_ADDR + PCIE_ATU_UPPER_TARGET);
		writel(PCIE_ATU_TYPE_CFG0, S32V234_DBI_ADDR + PCIE_ATU_CR1);
		writel(PCIE_ATU_ENABLE, S32V234_DBI_ADDR + PCIE_ATU_CR2);
	} else {
		/* Set the CLASS_REV of RC CFG header to something that
		 * makes sense for this SoC by itself. For a product,
		 * the class setting should be board/product specific,
		 * so we'd technically need a CONFIG_PCIE_CLASS as part
		 * of the board configuration.
		 */
		setbits_le32(S32V234_DBI_ADDR + PCI_CLASS_REVISION,
			     (PCI_BASE_CLASS_PROCESSOR << 24) |
			     (0x80 /* other */ << 16));

		/* Preconfigure the BAR registers, so that the RC can
		 * enumerate us properly and assign address spaces.
		 * Mask registers are W only!
		 */
		if (!ignoreERR009852 && socmask_info == 0x00) {
			/* Erratum ERR009852 requires us to avoid
			 * any memory access from the RC! We solve this
			 * by disabling all BARs and ROM access
			 */
			s32v234_pcie_set_bar(PCI_BASE_ADDRESS_0, 0, 0, 0);
			s32v234_pcie_set_bar(PCI_BASE_ADDRESS_1, 0, 0, 0);
			s32v234_pcie_set_bar(PCI_BASE_ADDRESS_2, 0, 0, 0);
			s32v234_pcie_set_bar(PCI_BASE_ADDRESS_3, 0, 0, 0);
			s32v234_pcie_set_bar(PCI_BASE_ADDRESS_4, 0, 0, 0);
			s32v234_pcie_set_bar(PCI_BASE_ADDRESS_5, 0, 0, 0);
			s32v234_pcie_set_bar(PCI_ROM_ADDRESS, 0, 0, 0);
		} else {
			s32v234_pcie_set_bar(PCI_BASE_ADDRESS_0,
					     PCIE_BAR0_EN_DIS, PCIE_BAR0_SIZE,
					     PCIE_BAR0_INIT);
			s32v234_pcie_set_bar(PCI_BASE_ADDRESS_1,
					     PCIE_BAR1_EN_DIS, PCIE_BAR1_SIZE,
					     PCIE_BAR1_INIT);
			s32v234_pcie_set_bar(PCI_BASE_ADDRESS_2,
					     PCIE_BAR2_EN_DIS, PCIE_BAR2_SIZE,
					     PCIE_BAR2_INIT);
			s32v234_pcie_set_bar(PCI_BASE_ADDRESS_3,
					     PCIE_BAR3_EN_DIS, PCIE_BAR3_SIZE,
					     PCIE_BAR3_INIT);
			s32v234_pcie_set_bar(PCI_BASE_ADDRESS_4,
					     PCIE_BAR4_EN_DIS, PCIE_BAR4_SIZE,
					     PCIE_BAR4_INIT);
			s32v234_pcie_set_bar(PCI_BASE_ADDRESS_5,
					     PCIE_BAR5_EN_DIS, PCIE_BAR5_SIZE,
					     PCIE_BAR5_INIT);
			s32v234_pcie_set_bar(PCI_ROM_ADDRESS,
					     PCIE_ROM_EN_DIS, PCIE_ROM_SIZE,
					     PCIE_ROM_INIT);

			/* Region #0 is used for Inbound Mem
			 * space access on BAR2.
			 */
			writel(0x80000000,
			       S32V234_DBI_ADDR + PCIE_ATU_VIEWPORT);
			writel(0xcff00000,
			       S32V234_DBI_ADDR + PCIE_ATU_LOWER_TARGET);
			writel(0, S32V234_DBI_ADDR + PCIE_ATU_UPPER_TARGET);
			writel(PCIE_ATU_TYPE_MEM,
			       S32V234_DBI_ADDR + PCIE_ATU_CR1);
			writel(0xC0000200, S32V234_DBI_ADDR + PCIE_ATU_CR2);

			/* CMD reg:I/O space, MEM space, Bus Master Enable */
			setbits_le32(S32V234_DBI_ADDR | PCI_COMMAND,
				     PCI_COMMAND_IO | PCI_COMMAND_MEMORY |
				     PCI_COMMAND_MASTER);
		}
	}
}

static void inthandler_pcie_link_req_rst_not(struct pt_regs *pt_regs,
					     unsigned int esr)
{
	const struct src *src_regs = (struct src *)SRC_SOC_BASE_ADDR;

	/* Clear link_req_rst_not interrupt signal */
	clrsetbits_le32(&src_regs->pcie_config0, 0x00000001, 0x00000001);

	/* Once we get this interrupt, the link came down and all the
	 * non sticky registers in our configuration space got reset.
	 * We reestablish the register values now and finally
	 * permit configuration transactions
	 */
	 set_non_sticky_config_regs();

	/* Accept inbound configuration requests now */
	clrsetbits_le32(&src_regs->gpr11, 0x00400000, 0x00400000);
}

/*
 * iATU region setup
 */
static int s32v234_pcie_regions_setup(const int ep_mode)
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

	/* We set up the ID for all Rev 1.x chips */
	if (get_siul2_midr1_major() == 0x00) {
		/*
		 * Vendor ID is Freescale (now NXP): 0x1957
		 * Device ID is split as follows
		 * Family 15:12, Device 11:6, Personality 5:0
		 * S32V is in the automotive family: 0100
		 * S32V is the first auto device with PCIe: 000000
		 * S32V does not have export controlled cryptography: 00001
		 */
		printf("Setting PCIE Vendor and Device ID\n");
		writel((0x4001 << 16) | 0x1957,
		       S32V234_DBI_ADDR + PCI_VENDOR_ID);
	}

#if defined(CONFIG_PCIE_SUBSYSTEM_VENDOR_ID) && \
	defined(CONFIG_PCIE_SUBSYSTEM_ID)
	writel((CONFIG_PCIE_SUBSYSTEM_ID << 16) |
	       CONFIG_PCIE_SUBSYSTEM_VENDOR_ID,
	       S32V234_DBI_ADDR + PCI_SUBSYSTEM_VENDOR_ID);
#endif

	ignoreERR009852 = env_get("ignoreERR009852") ? true : false;

	set_non_sticky_config_regs();

	if (ep_mode) {
		struct src *src_regs = (struct src *)SRC_SOC_BASE_ADDR;

		if (ignoreERR009852)
			printf("\n Ignoring errata ERR009852\n");

		/* Ensure that if the link comes down we do not react
		 * to config accesses anymore until we have reconfigured
		 * ourselves properly! A link down event unfortunately
		 * clears non-sticky registers.
		 * Note that we permit automatic link training. This
		 * puts the responsibility on us to reconfigure and
		 * set PCIE_CFG_READY again if the link comes down.
		 */
		clrsetbits_le32(&src_regs->gpr10, 0x40000000, 0x40000000);

		/* Assume the link is up and reset the link down event,
		 * so that we can properly try to set PCIE_CFG_READY.
		 */
		clrsetbits_le32(&src_regs->pcie_config0, 0x00000001,
				0x00000001);

		/* Ensure that we can fix up our configuration again
		 * if the link came down!
		 */
		gic_register_handler(PCIE_INTERRUPT_link_req_rst_not,
				     inthandler_pcie_link_req_rst_not,
				     0, "PCIE_INTERRUPT_link_req_rst_not");

		/* Accept inbound configuration requests now */
		clrsetbits_le32(&src_regs->gpr11, 0x00400000, 0x00400000);
	}

	return 0;
}

/*
 * PCI Express accessors
 */
static u8 *get_bus_address(pci_dev_t d, int where)
{
	u8 *va_address;

	/* Reconfigure Region #0 */
	writel(0, S32V234_DBI_ADDR + PCIE_ATU_VIEWPORT);

	if (PCI_BUS(d) < 2)
		writel(PCIE_ATU_TYPE_CFG0, S32V234_DBI_ADDR + PCIE_ATU_CR1);
	else
		writel(PCIE_ATU_TYPE_CFG1, S32V234_DBI_ADDR + PCIE_ATU_CR1);

	if (PCI_BUS(d) == 0) {
		va_address = (u8 *)S32V234_DBI_ADDR;
	} else {
		writel(d << 8, S32V234_DBI_ADDR + PCIE_ATU_LOWER_TARGET);
		va_address = (u8 *)(S32V234_IO_ADDR + SZ_16M - SZ_1M);
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
	u8 *va_address;
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
	void *va_address = 0;
	int ret;

	ret = s32v234_pcie_addr_valid(d);
	if (ret)
		return ret;

	va_address = get_bus_address(d, where);
	writel(val, va_address);

	return 0;
}

static int s32v234_pcie_init_phy(const int ep_mode)
{
	struct src *src_regs = (struct src *)SRC_SOC_BASE_ADDR;

	clrbits_le32(&src_regs->gpr5, SRC_GPR5_PCIE_APP_LTSSM_ENABLE);

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

static int s32v_pcie_link_up(const int ep_mode)
{
	struct src *src_regs = (struct src *)SRC_SOC_BASE_ADDR;

	u32 tmp;
	int count = 0;

	s32v234_pcie_init_phy(ep_mode);
	s32v234_pcie_deassert_core_reset();
	s32v234_pcie_regions_setup(ep_mode);

	/*
	 * FIXME: Force the PCIe RC to Gen1 operation
	 * The RC must be forced into Gen1 mode before bringing the link
	 * up, otherwise no downstream devices are detected. After the
	 * link is up, a managed Gen1->Gen2 transition can be initiated.
	 */
	printf("\nForcing PCIe to Gen1 operation\n");

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

void s32v234_pcie_init(const int ep_mode)
{
	/* Static instance of the controller. */
	static struct pci_controller	pcc;
	struct pci_controller		*hose = &pcc;
	int ret;
	struct src *src_regs = (struct src *)SRC_SOC_BASE_ADDR;

	/* Set device type */
	clrsetbits_le32(&src_regs->gpr5,
			SRC_GPR5_PCIE_DEVICE_TYPE_MASK,
			(ep_mode) ? SRC_GPR5_PCIE_DEVICE_TYPE_EP :
				    SRC_GPR5_PCIE_DEVICE_TYPE_RC);

	if (!ep_mode) {
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
		pci_set_region(&hose->regions[2],
			       MMDC0_ARB_BASE_ADDR, MMDC0_ARB_BASE_ADDR,
			       0x3FFFFFFF,
			       PCI_REGION_MEM | PCI_REGION_SYS_MEMORY);

		hose->region_count = 3;

		pci_set_ops(hose,
			    pci_hose_read_config_byte_via_dword,
			    pci_hose_read_config_word_via_dword,
			    s32v234_pcie_read_config,
			    pci_hose_write_config_byte_via_dword,
			    pci_hose_write_config_word_via_dword,
			    s32v234_pcie_write_config);
	}

	/* Start the controller. */
	ret = s32v_pcie_link_up(ep_mode);

	if (!ep_mode) {
		if (!ret) {
			pci_register_hose(hose);
			hose->last_busno = pci_hose_scan(hose);
		}
	}
}

void pci_init_board(void)
{
#ifdef CONFIG_PCIE_EP_MODE
	s32v234_pcie_init(1);
#else
	s32v234_pcie_init(0);
#endif
}
