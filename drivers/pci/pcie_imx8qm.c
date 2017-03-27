/*
 *
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <linux/errno.h>
#include <asm/io.h>
#include <asm/arch/sci/sci.h>
#include <common.h>
#include <linux/sizes.h>
#include <imx8_hsio.h>

void pcie_ctrlx2_rst(void)
{
	/* gpio config */
	/* dir  wakeup input clkreq and pereset output */
	writel(0x2d, HSIO_GPIO_BASE_ADDR + 0x4);
	writel(0x24, HSIO_GPIO_BASE_ADDR + 0x0); /* do pereset 1 */

	clrbits_le32(HW_PCIEX2_CTRL2_ADDR, HW_PCIEX2_CTRL2_BUTTON_RST_N);
	clrbits_le32(HW_PCIEX2_CTRL2_ADDR, HW_PCIEX2_CTRL2_PERST_N);
	clrbits_le32(HW_PCIEX2_CTRL2_ADDR, HW_PCIEX2_CTRL2_POWER_UP_RST_N);
	udelay(10);
	setbits_le32(HW_PCIEX2_CTRL2_ADDR, HW_PCIEX2_CTRL2_BUTTON_RST_N);
	setbits_le32(HW_PCIEX2_CTRL2_ADDR, HW_PCIEX2_CTRL2_PERST_N);
	setbits_le32(HW_PCIEX2_CTRL2_ADDR, HW_PCIEX2_CTRL2_POWER_UP_RST_N);
}

void pcie_ctrlx1_rst(void)
{
	/* gpio config */
	/* dir  wakeup input clkreq and pereset output */
	writel(0x2d, HSIO_GPIO_BASE_ADDR + 0x4);
	writel(0x24, HSIO_GPIO_BASE_ADDR + 0x0); /* do pereset 1 */

	clrbits_le32(HW_PCIEX1_CTRL2_ADDR, HW_PCIEX1_CTRL2_BUTTON_RST_N);
	setbits_le32(HW_PCIEX1_CTRL2_ADDR, HW_PCIEX1_CTRL2_BUTTON_RST_N);
	clrbits_le32(HW_PCIEX1_CTRL2_ADDR, HW_PCIEX1_CTRL2_PERST_N);
	setbits_le32(HW_PCIEX1_CTRL2_ADDR, HW_PCIEX1_CTRL2_PERST_N);
	clrbits_le32(HW_PCIEX1_CTRL2_ADDR, HW_PCIEX1_CTRL2_POWER_UP_RST_N);
	setbits_le32(HW_PCIEX1_CTRL2_ADDR, HW_PCIEX1_CTRL2_POWER_UP_RST_N);
}

int pcie_ctrla_init_rc(int lane)
{
	u32 val, i = 0;

	setbits_le32(HW_PHYX2_CTRL0_ADDR, HW_PHYX2_CTRL0_APB_RSTN_0
			| HW_PHYX2_CTRL0_APB_RSTN_1); /* APB_RSTN_0/1 */

	clrbits_le32(HW_PCIEX2_CTRL0_ADDR, HW_PCIEX2_CTRL0_DEVICE_TYPE_MASK);
	setbits_le32(HW_PCIEX2_CTRL0_ADDR, HW_PCIEX2_CTRL0_DEVICE_TYPE_RC);

	if (lane == 1) {
		/*
		 * bit 0 rx ena. bit 11 fast_init.
		 * bit12 PHY_X1_EPCS_SEL 1.
		 * bit13 phy_ab_select 1.
		 */
		setbits_le32(HW_MISC_CTRL0_ADDR, HW_MISC_CTRL0_IOB_RXENA
				| HW_MISC_CTRL0_PHY_X1_EPCS_SEL
				| HW_MISC_CTRL0_PCIE_AB_SELECT);
		/* pipe_ln2lk = 1001 */
		clrbits_le32(HW_PHYX2_CTRL0_ADDR,
				HW_PHYX2_CTRL0_PIPE_LN2LK_MASK);
		setbits_le32(HW_PHYX2_CTRL0_ADDR, HW_PHYX2_CTRL0_PIPE_LN2LK_3
				| HW_PHYX2_CTRL0_PIPE_LN2LK_0);
		for (i = 0; i < 100; i++) {
			val = readl(HW_PHYX2_STTS0_ADDR);
			val &= HW_PHYX2_STTS0_LANE0_TX_PLL_LOCK;
			if (val == HW_PHYX2_STTS0_LANE0_TX_PLL_LOCK)
				break;
			udelay(10);
		}

		if (val != HW_PHYX2_STTS0_LANE0_TX_PLL_LOCK) {
			printf("TX PLL is not locked.\n");
			return -ENODEV;
		}
		setbits_le32(GPR_LPCG_PHYX2APB_0_APB, BIT(1));
		/* Set the link_capable to be lane1 */
		clrbits_le32(PORT0_LINK_CTRL, PORT_LINK_CTRL_LNK_EN_MASK);
		setbits_le32(PORT0_LINK_CTRL, PORT_LINK_CTRL_LNK_LANE1);
		clrbits_le32(PORT0_GEN2_CTRL, PORT_GEN2_CTRL_NUM_LANES_MASK);
		setbits_le32(PORT0_GEN2_CTRL, PORT_GEN2_CTRL_NUM_LANES_1);
	} else if (lane == 2) {
		/*
		 * bit 0 rx ena. bit 11 fast_init.
		 * bit12 PHY_X1_EPCS_SEL 1.
		 */
		setbits_le32(HW_MISC_CTRL0_ADDR, HW_MISC_CTRL0_IOB_RXENA
				| HW_MISC_CTRL0_PHY_X1_EPCS_SEL);
		/* pipe_ln2lk = 0011 */
		clrbits_le32(HW_PHYX2_CTRL0_ADDR,
				HW_PHYX2_CTRL0_PIPE_LN2LK_MASK);
		setbits_le32(HW_PHYX2_CTRL0_ADDR, HW_PHYX2_CTRL0_PIPE_LN2LK_1
				| HW_PHYX2_CTRL0_PIPE_LN2LK_0);
		for (i = 0; i < 100; i++) {
			val = readl(HW_PHYX2_STTS0_ADDR);
			val &= (HW_PHYX2_STTS0_LANE0_TX_PLL_LOCK | HW_PHYX2_STTS0_LANE1_TX_PLL_LOCK);
			if (val == (HW_PHYX2_STTS0_LANE0_TX_PLL_LOCK | HW_PHYX2_STTS0_LANE1_TX_PLL_LOCK))
				break;
			udelay(10);
		}

		if (val != (HW_PHYX2_STTS0_LANE0_TX_PLL_LOCK | HW_PHYX2_STTS0_LANE1_TX_PLL_LOCK)) {
			printf("TX PLL is not locked.\n");
			return -ENODEV;
		}
		setbits_le32(GPR_LPCG_PHYX2APB_0_APB, BIT(1) + BIT(5));
		/* Set the link_capable to be lane2 */
		clrbits_le32(PORT0_LINK_CTRL, PORT_LINK_CTRL_LNK_EN_MASK);
		setbits_le32(PORT0_LINK_CTRL, PORT_LINK_CTRL_LNK_LANE2);
		clrbits_le32(PORT0_GEN2_CTRL, PORT_GEN2_CTRL_NUM_LANES_MASK);
		setbits_le32(PORT0_GEN2_CTRL, PORT_GEN2_CTRL_NUM_LANES_2);
	} else {
		printf("%s %d lane %d is invalid.\n", __func__, __LINE__, lane);
	}

	/* bit19 PM_REQ_CORE_RST of pciex2_stts0 should be cleared. */
	for (i = 0; i < 100; i++) {
		val = readl(HW_PCIEX2_STTS0_ADDR);
		if ((val & HW_PCIEX2_STTS0_PM_REQ_CORE_RST) == 0)
			break;
		udelay(10);
	}

	if ((val & HW_PCIEX2_STTS0_PM_REQ_CORE_RST) != 0)
		printf("ERROR PM_REQ_CORE_RST is set.\n");

	/* DBI_RO_WR_EN =1 to write PF0_SPCIE_CAP_OFF_0CH_REG */
	writel(0x1, PORT0_MISC_CONTROL_1);
	writel(0x35353535, PF0_SPCIE_CAP_OFF_0CH_REG); /* set preset not golden */
	setbits_le32(PORT0_LINK_CTRL, PORT_LINK_CTRL_LNK_FAST_LNK);
	setbits_le32(HW_PCIEX2_CTRL2_ADDR, HW_PCIEX2_CTRL2_APP_LTSSM_ENABLE);

	do {
		udelay(100);
		val = readl(PORT0_LINK_DEBUG1);
	} while (((val & PORT_LINK_DEBUG1_LINK_UP) == 0) && (i++ < 100));

	if ((val & PORT_LINK_DEBUG1_LINK_UP) == PORT_LINK_DEBUG1_LINK_UP)
		printf("[%s] LNK UP %x\r\n", __func__, val);
	else {
		printf("[%s] LNK DOWN %x\r\n", __func__, val);
		clrbits_le32(HW_PCIEX2_CTRL2_ADDR, HW_PCIEX2_CTRL2_APP_LTSSM_ENABLE);
		return -ENODEV;
	}

	clrbits_le32(PORT0_LINK_CTRL, PORT_LINK_CTRL_LNK_FAST_LNK);

	val = readl(PF0_LINK_CONTROL_LINK_STATUS_REG);
	printf("[%s] PCIe GEN[%d] Lane[%d] is up.\n", __func__,
		(val >> 16) & 0xF, (val >> 20) & 0x3F);

	/* EQ phase 3 finish
	 * wait_read_check(LINK_CONTROL2_LINK_STATUS2_REG,BIT(17),BIT(17),1000);
	 */
	/* make sure that pciea is L0 state now */
	for (i = 0; i < 100; i++) {
		val = readl(HW_PCIEX2_STTS0_ADDR);
		if ((val & 0x3f) == 0x11)
			break;
		udelay(10);
	}

	if ((val & 0x3f) != 0x11)
		printf("can't return back to L0 state.\n");

	writel(PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER,
		PF0_TYPE1_STATUS_COMMAND_REG);
	printf("pcie ctrla initialization is finished.\n");

	return 0;
}

int pcie_ctrlb_sata_phy_init_rc(void)
{
	u32 val, i = 0;

	setbits_le32(HW_PHYX1_CTRL0_ADDR, HW_PHYX1_CTRL0_APB_RSTN); /* APB_RSTN */

	clrbits_le32(HW_PCIEX1_CTRL0_ADDR, HW_PCIEX1_CTRL0_DEVICE_TYPE_MASK);
	setbits_le32(HW_PCIEX1_CTRL0_ADDR, HW_PCIEX1_CTRL0_DEVICE_TYPE_RC);

	setbits_le32(HW_PCIEX1_CTRL2_ADDR, HW_PCIEX1_CTRL2_BUTTON_RST_N);
	clrbits_le32(HW_PCIEX1_CTRL2_ADDR, HW_PCIEX1_CTRL2_PERST_N);
	setbits_le32(HW_PCIEX1_CTRL2_ADDR, HW_PCIEX1_CTRL2_PERST_N);
	clrbits_le32(HW_PCIEX1_CTRL2_ADDR, HW_PCIEX1_CTRL2_POWER_UP_RST_N);
	setbits_le32(HW_PCIEX1_CTRL2_ADDR, HW_PCIEX1_CTRL2_POWER_UP_RST_N);

	/*
	 * bit 0 rx ena. bit 11 fast_init.
	 * bit13 phy_ab_select 1.
	 */
	setbits_le32(HW_MISC_CTRL0_ADDR, HW_MISC_CTRL0_IOB_RXENA);
	clrbits_le32(HW_MISC_CTRL0_ADDR, HW_MISC_CTRL0_PHY_X1_EPCS_SEL);

	/* pipe_ln2lk = 0011 */
	clrbits_le32(HW_PHYX1_CTRL0_ADDR,
			HW_PHYX1_CTRL0_PIPE_LN2LK_MASK);
	setbits_le32(HW_PHYX1_CTRL0_ADDR, HW_PHYX1_CTRL0_PIPE_LN2LK_1
			| HW_PHYX2_CTRL0_PIPE_LN2LK_0);
	for (i = 0; i < 100; i++) {
		val = readl(HW_PHYX1_STTS0_ADDR);
		val &= HW_PHYX1_STTS0_LANE0_TX_PLL_LOCK;
		if (val == HW_PHYX1_STTS0_LANE0_TX_PLL_LOCK)
			break;
		udelay(10);
	}

	if (val != HW_PHYX1_STTS0_LANE0_TX_PLL_LOCK) {
		printf("TX PLL is not locked.\n");
		return -ENODEV;
	}

	setbits_le32(GPR_LPCG_PHYX1_APB, BIT(1));

	/* bit19 PM_REQ_CORE_RST of pciex1_stts0 should be cleared. */
	for (i = 0; i < 100; i++) {
		val = readl(HW_PCIEX1_STTS0_ADDR);
		if ((val & HW_PCIEX1_STTS0_PM_REQ_CORE_RST) == 0)
			break;
		udelay(10);
	}

	if ((val & HW_PCIEX1_STTS0_PM_REQ_CORE_RST) != 0)
		printf("ERROR PM_REQ_CORE_RST is set.\n");

	/* DBI_RO_WR_EN =1 to write PF1_SPCIE_CAP_OFF_0CH_REG */
	writel(0x1, PORT1_MISC_CONTROL_1);
	writel(0x35353535, PF1_SPCIE_CAP_OFF_0CH_REG); /* set preset not golden */
	setbits_le32(PORT1_LINK_CTRL, PORT_LINK_CTRL_LNK_FAST_LNK);
	setbits_le32(HW_PCIEX1_CTRL2_ADDR, HW_PCIEX1_CTRL2_APP_LTSSM_ENABLE);

	do {
		udelay(100);
		val = readl(PORT1_LINK_DEBUG1);
	} while (((val & PORT_LINK_DEBUG1_LINK_UP) == 0) && (i++ < 100));

	if ((val & PORT_LINK_DEBUG1_LINK_UP) == PORT_LINK_DEBUG1_LINK_UP) {
		printf("[%s] LNK UP %x\r\n", __func__, val);
	} else {
		printf("[%s] LNK DOWN %x\r\n", __func__, val);
		clrbits_le32(HW_PCIEX1_CTRL2_ADDR, HW_PCIEX1_CTRL2_APP_LTSSM_ENABLE);
		return -ENODEV;
	}
	clrbits_le32(PORT1_LINK_CTRL, PORT_LINK_CTRL_LNK_FAST_LNK);

	val = readl(PF1_LINK_CONTROL_LINK_STATUS_REG);
	printf("[%s] PCIe GEN[%d] Lane[%d] is up.\n", __func__,
	       (val >> 16) & 0xF, (val >> 20) & 0x3F);

	/* EQ phase 3 finish
	 * wait_read_check(LINK_CONTROL2_LINK_STATUS2_REG,BIT(17),BIT(17),1000);
	 */
	/* make sure that pcieb is L0 state now */
	for (i = 0; i < 100; i++) {
		val = readl(HW_PCIEX1_STTS0_ADDR);
		if ((val & 0x3f) == 0x11)
			break;
		udelay(10);
	}

	if ((val & 0x3f) != 0x11) {
		printf("can't return back to L0 state.\n");
		return -ENODEV;
	}

	writel(PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER,
		PF1_TYPE1_STATUS_COMMAND_REG);

	return 0;
}

DECLARE_GLOBAL_DATA_PTR;
void mx8qxp_pcie_init(void)
{
	pcie_ctrlx1_rst();
	if (!pcie_ctrlb_sata_phy_init_rc())
		mx8x_pcie_ctrlb_setup_regions();
}

void mx8qm_pcie_init(void)
{
	pcie_ctrlx2_rst();
	if (!pcie_ctrla_init_rc(1))
		mx8x_pcie_ctrla_setup_regions();

#ifdef CONFIG_IMX_PCIEB
	pcie_ctrlx1_rst();
	if (!pcie_ctrlb_sata_phy_init_rc())
		mx8x_pcie_ctrlb_setup_regions();
#endif
}
