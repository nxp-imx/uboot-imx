// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 */

#include <common.h>
#include <dm.h>
#include <ahci.h>
#include <scsi.h>
#include <sata.h>
#include <asm/io.h>
#if CONFIG_IS_ENABLED(CLK)
#include <clk.h>
#else
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#endif
#include <asm/arch-mx6/iomux.h>
#include <syscon.h>
#include <regmap.h>
#include <asm-generic/gpio.h>
#include <dm/device_compat.h>

enum {
	/* Timer 1-ms Register */
	IMX_TIMER1MS				= 0x00e0,
	/* Port0 PHY Control Register */
	IMX_P0PHYCR				= 0x0178,
	IMX_P0PHYCR_TEST_PDDQ			= 1 << 20,
	IMX_P0PHYCR_CR_READ			= 1 << 19,
	IMX_P0PHYCR_CR_WRITE			= 1 << 18,
	IMX_P0PHYCR_CR_CAP_DATA			= 1 << 17,
	IMX_P0PHYCR_CR_CAP_ADDR			= 1 << 16,
	/* Port0 PHY Status Register */
	IMX_P0PHYSR				= 0x017c,
	IMX_P0PHYSR_CR_ACK			= 1 << 18,
	IMX_P0PHYSR_CR_DATA_OUT			= 0xffff << 0,
	/* Lane0 Output Status Register */
	IMX_LANE0_OUT_STAT			= 0x2003,
	IMX_LANE0_OUT_STAT_RX_PLL_STATE		= 1 << 1,
	/* Clock Reset Register */
	IMX_CLOCK_RESET				= 0x7f3f,
	IMX_CLOCK_RESET_RESET			= 1 << 0,
	/* IMX8QM HSIO AHCI definitions */
	IMX8QM_SATA_PHY_REG03_RX_IMPED_RATIO		= 0x03,
	IMX8QM_SATA_PHY_REG09_TX_IMPED_RATIO		= 0x09,
	IMX8QM_SATA_PHY_REG10_TX_POST_CURSOR_RATIO	= 0x0a,
	IMX8QM_SATA_PHY_GEN1_TX_POST_CURSOR_RATIO	= 0x15,
	IMX8QM_SATA_PHY_IMPED_RATIO_85OHM		= 0x6c,
	IMX8QM_SATA_PHY_REG22_TX_POST_CURSOR_RATIO	= 0x16,
	IMX8QM_SATA_PHY_GEN2_TX_POST_CURSOR_RATIO	= 0x00,
	IMX8QM_SATA_PHY_REG24_TX_AMP_RATIO_MARGIN0	= 0x18,
	IMX8QM_SATA_PHY_TX_AMP_RATIO_MARGIN0		= 0x64,
	IMX8QM_SATA_PHY_REG25_TX_AMP_RATIO_MARGIN1	= 0x19,
	IMX8QM_SATA_PHY_TX_AMP_RATIO_MARGIN1		= 0x70,
	IMX8QM_SATA_PHY_REG26_TX_AMP_RATIO_MARGIN2	= 0x1a,
	IMX8QM_SATA_PHY_TX_AMP_RATIO_MARGIN2		= 0x69,
	IMX8QM_SATA_PHY_REG48_PMA_STATUS		= 0x30,
	IMX8QM_SATA_PHY_REG48_PMA_RDY			= BIT(7),
	IMX8QM_SATA_PHY_REG128_UPDATE_SETTING		= 0x80,
	IMX8QM_SATA_PHY_UPDATE_SETTING			= 0x01,
	IMX8QM_LPCG_PHYX2_OFFSET		= 0x00000,
	IMX8QM_CSR_PHYX2_OFFSET			= 0x90000,
	IMX8QM_CSR_PHYX1_OFFSET			= 0xa0000,
	IMX8QM_CSR_PHYX_STTS0_OFFSET		= 0x4,
	IMX8QM_CSR_PCIEA_OFFSET			= 0xb0000,
	IMX8QM_CSR_PCIEB_OFFSET			= 0xc0000,
	IMX8QM_CSR_SATA_OFFSET			= 0xd0000,
	IMX8QM_CSR_PCIE_CTRL2_OFFSET		= 0x8,
	IMX8QM_CSR_MISC_OFFSET			= 0xe0000,
	/* IMX8QM SATA specific control registers */
	IMX8QM_SATA_PPCFG_OFFSET			= 0xa8,
	IMX8QM_SATA_PPCFG_FORCE_PHY_RDY			= BIT(20),
	IMX8QM_SATA_PPCFG_BIST_PATTERN_MASK		= 0x7 << 21,
	IMX8QM_SATA_PPCFG_BIST_PATTERN_OFFSET		= 21,
	IMX8QM_SATA_PPCFG_BIST_PATTERN_EN		= BIT(24),
	IMX8QM_SATA_PPCFG_BIST_PATTERN_NOALIGNS		= BIT(26),
	IMX8QM_SATA_PP2CFG_OFFSET			= 0xac,
	IMX8QM_SATA_PP2CFG_COMINIT_NEGATE_MIN		= 0x28 << 24,
	IMX8QM_SATA_PP2CFG_COMINT_BURST_GAP		= 0x18 << 16,
	IMX8QM_SATA_PP2CFG_COMINT_BURST_GAP_MAX		= 0x2b << 8,
	IMX8QM_SATA_PP2CFG_COMINT_BURST_GAP_MIN		= 0x1b << 0,
	IMX8QM_SATA_PP3CFG_OFFSET			= 0xb0,
	IMX8QM_SATA_PP3CFG_COMWAKE_NEGATE_MIN		= 0x0e << 24,
	IMX8QM_SATA_PP3CFG_COMWAKE_BURST_GAP		= 0x08 << 16,
	IMX8QM_SATA_PP3CFG_COMWAKE_BURST_GAP_MAX	= 0x0f << 8,
	IMX8QM_SATA_PP3CFG_COMWAKE_BURST_GAP_MIN	= 0x01 << 0,

	IMX8QM_LPCG_PHYX2_PCLK0_MASK		= (0x3 << 16),
	IMX8QM_LPCG_PHYX2_PCLK1_MASK		= (0x3 << 20),
	IMX8QM_PHY_APB_RSTN_0			= BIT(0),
	IMX8QM_PHY_MODE_SATA			= BIT(19),
	IMX8QM_PHY_MODE_MASK			= (0xf << 17),
	IMX8QM_PHY_PIPE_RSTN_0			= BIT(24),
	IMX8QM_PHY_PIPE_RSTN_OVERRIDE_0		= BIT(25),
	IMX8QM_PHY_PIPE_RSTN_1			= BIT(26),
	IMX8QM_PHY_PIPE_RSTN_OVERRIDE_1		= BIT(27),
	IMX8QM_STTS0_LANE0_TX_PLL_LOCK		= BIT(4),
	IMX8QM_MISC_IOB_RXENA			= BIT(0),
	IMX8QM_MISC_IOB_TXENA			= BIT(1),
	IMX8QM_MISC_PHYX1_EPCS_SEL		= BIT(12),
	IMX8QM_MISC_CLKREQN_OUT_OVERRIDE_1	= BIT(24),
	IMX8QM_MISC_CLKREQN_OUT_OVERRIDE_0	= BIT(25),
	IMX8QM_MISC_CLKREQN_IN_OVERRIDE_1	= BIT(28),
	IMX8QM_MISC_CLKREQN_IN_OVERRIDE_0	= BIT(29),
	IMX8QM_SATA_CTRL_RESET_N		= BIT(12),
	IMX8QM_SATA_CTRL_EPCS_PHYRESET_N	= BIT(7),
	IMX8QM_SATA_CTRL_EPCS_TXDEEMP_SEL	= BIT(6),
	IMX8QM_SATA_CTRL_EPCS_TXDEEMP		= BIT(5),
	IMX8QM_CTRL_BUTTON_RST_N		= BIT(21),
	IMX8QM_CTRL_POWER_UP_RST_N		= BIT(23),
	IMX8QM_CTRL_LTSSM_ENABLE		= BIT(4),
};

enum ahci_imx_type {
	AHCI_IMX6Q,
	AHCI_IMX6QP,
	AHCI_IMX8QM,
};

struct imx_ahci_priv {
#if CONFIG_IS_ENABLED(CLK)
	struct clk sata_clk;
	struct clk sata_ref_clk;
	struct clk ahb_clk;
	struct clk epcs_tx_clk;
	struct clk epcs_rx_clk;
	struct clk phy_apbclk;
	struct clk phy_pclk0;
	struct clk phy_pclk1;
#endif
	enum ahci_imx_type type;
	void __iomem *phy_base;
	void __iomem *mmio;
	struct regmap *gpr;
	struct gpio_desc clkreq_gpio;
	u32 phy_params;
	u32 imped_ratio;
	u32 ext_osc;
};

static int imx_phy_crbit_assert(void __iomem *mmio, u32 bit, bool assert)
{
	int timeout = 10;
	u32 crval;
	u32 srval;

	/* Assert or deassert the bit */
	crval = readl(mmio + IMX_P0PHYCR);
	if (assert)
		crval |= bit;
	else
		crval &= ~bit;
	writel(crval, mmio + IMX_P0PHYCR);

	/* Wait for the cr_ack signal */
	do {
		srval = readl(mmio + IMX_P0PHYSR);
		if ((assert ? srval : ~srval) & IMX_P0PHYSR_CR_ACK)
			break;
		udelay(100);
	} while (--timeout);

	return timeout ? 0 : -ETIMEDOUT;
}

static int imx_phy_reg_addressing(u16 addr, void __iomem *mmio)
{
	u32 crval = addr;
	int ret;

	/* Supply the address on cr_data_in */
	writel(crval, mmio + IMX_P0PHYCR);

	/* Assert the cr_cap_addr signal */
	ret = imx_phy_crbit_assert(mmio, IMX_P0PHYCR_CR_CAP_ADDR, true);
	if (ret)
		return ret;

	/* Deassert cr_cap_addr */
	ret = imx_phy_crbit_assert(mmio, IMX_P0PHYCR_CR_CAP_ADDR, false);
	if (ret)
		return ret;

	return 0;
}

static int imx_phy_reg_write(u16 val, void __iomem *mmio)
{
	u32 crval = val;
	int ret;

	/* Supply the data on cr_data_in */
	writel(crval, mmio + IMX_P0PHYCR);

	/* Assert the cr_cap_data signal */
	ret = imx_phy_crbit_assert(mmio, IMX_P0PHYCR_CR_CAP_DATA, true);
	if (ret)
		return ret;

	/* Deassert cr_cap_data */
	ret = imx_phy_crbit_assert(mmio, IMX_P0PHYCR_CR_CAP_DATA, false);
	if (ret)
		return ret;

	if (val & IMX_CLOCK_RESET_RESET) {
		/*
		 * In case we're resetting the phy, it's unable to acknowledge,
		 * so we return immediately here.
		 */
		crval |= IMX_P0PHYCR_CR_WRITE;
		writel(crval, mmio + IMX_P0PHYCR);
		goto out;
	}

	/* Assert the cr_write signal */
	ret = imx_phy_crbit_assert(mmio, IMX_P0PHYCR_CR_WRITE, true);
	if (ret)
		return ret;

	/* Deassert cr_write */
	ret = imx_phy_crbit_assert(mmio, IMX_P0PHYCR_CR_WRITE, false);
	if (ret)
		return ret;

out:
	return 0;
}

static int imx_phy_reg_read(u16 *val, void __iomem *mmio)
{
	int ret;

	/* Assert the cr_read signal */
	ret = imx_phy_crbit_assert(mmio, IMX_P0PHYCR_CR_READ, true);
	if (ret)
		return ret;

	/* Capture the data from cr_data_out[] */
	*val = readl(mmio + IMX_P0PHYSR) & IMX_P0PHYSR_CR_DATA_OUT;

	/* Deassert cr_read */
	ret = imx_phy_crbit_assert(mmio, IMX_P0PHYCR_CR_READ, false);
	if (ret)
		return ret;

	return 0;
}

static int imx_sata_phy_reset(struct imx_ahci_priv *priv)
{
	void __iomem *mmio = priv->mmio;
	int timeout = 10;
	u16 val;
	int ret;

	/* Reset SATA PHY by setting RESET bit of PHY register CLOCK_RESET */
	ret = imx_phy_reg_addressing(IMX_CLOCK_RESET, mmio);
	if (ret)
		return ret;
	ret = imx_phy_reg_write(IMX_CLOCK_RESET_RESET, mmio);
	if (ret)
		return ret;

	/* Wait for PHY RX_PLL to be stable */
	do {
		udelay(100);
		ret = imx_phy_reg_addressing(IMX_LANE0_OUT_STAT, mmio);
		if (ret)
			return ret;
		ret = imx_phy_reg_read(&val, mmio);
		if (ret)
			return ret;
		if (val & IMX_LANE0_OUT_STAT_RX_PLL_STATE)
			break;
	} while (--timeout);

	return timeout ? 0 : -ETIMEDOUT;
}

static int imx8_sata_enable(struct udevice *dev)
{
	u32 val, reg;
	int i, ret;
	struct imx_ahci_priv *imxpriv = dev_get_priv(dev);

#if CONFIG_IS_ENABLED(CLK)
	/* configure the hsio for sata */
	ret = clk_enable(&imxpriv->phy_pclk0);
	if (ret < 0) {
		dev_err(dev, "can't enable phy pclk0.\n");
		return ret;
	}
	ret = clk_enable(&imxpriv->phy_pclk1);
	if (ret < 0) {
		dev_err(dev, "can't enable phy pclk1.\n");
		goto disable_phy_pclk0;
	}
	ret = clk_enable(&imxpriv->epcs_tx_clk);
	if (ret < 0) {
		dev_err(dev, "can't enable epcs tx clk.\n");
		goto disable_phy_pclk1;
	}
	ret = clk_enable(&imxpriv->epcs_rx_clk);
	if (ret < 0) {
		dev_err(dev, "can't enable epcs rx clk.\n");
		goto disable_epcs_tx_clk;
	}
	ret = clk_enable(&imxpriv->phy_apbclk);
	if (ret < 0) {
		dev_err(dev, "can't enable phy pclk1.\n");
		goto disable_epcs_rx_clk;
	}
#endif

	/* Configure PHYx2 PIPE_RSTN */
	regmap_read(imxpriv->gpr, IMX8QM_CSR_PCIEA_OFFSET
			+ IMX8QM_CSR_PCIE_CTRL2_OFFSET, &val);
	if ((val & IMX8QM_CTRL_LTSSM_ENABLE) == 0) {
		 /* PCIEA of HSIO is down too */
		regmap_update_bits(imxpriv->gpr,
				IMX8QM_CSR_PHYX2_OFFSET,
				IMX8QM_PHY_PIPE_RSTN_0
				| IMX8QM_PHY_PIPE_RSTN_OVERRIDE_0,
				IMX8QM_PHY_PIPE_RSTN_0
				| IMX8QM_PHY_PIPE_RSTN_OVERRIDE_0);
	}
	regmap_read(imxpriv->gpr, IMX8QM_CSR_PCIEB_OFFSET
			+ IMX8QM_CSR_PCIE_CTRL2_OFFSET, &reg);
	if ((reg & IMX8QM_CTRL_LTSSM_ENABLE) == 0) {
		 /* PCIEB of HSIO is down */
		regmap_update_bits(imxpriv->gpr,
				IMX8QM_CSR_PHYX2_OFFSET,
				IMX8QM_PHY_PIPE_RSTN_1
				| IMX8QM_PHY_PIPE_RSTN_OVERRIDE_1,
				IMX8QM_PHY_PIPE_RSTN_1
				| IMX8QM_PHY_PIPE_RSTN_OVERRIDE_1);
	}

	/* set PWR_RST and BT_RST of csr_pciea */
	val = IMX8QM_CSR_PCIEA_OFFSET + IMX8QM_CSR_PCIE_CTRL2_OFFSET;
	regmap_update_bits(imxpriv->gpr,
			val,
			IMX8QM_CTRL_BUTTON_RST_N,
			IMX8QM_CTRL_BUTTON_RST_N);
	regmap_update_bits(imxpriv->gpr,
			val,
			IMX8QM_CTRL_POWER_UP_RST_N,
			IMX8QM_CTRL_POWER_UP_RST_N);

	/* PHYX1_MODE to SATA */
	regmap_update_bits(imxpriv->gpr,
			IMX8QM_CSR_PHYX1_OFFSET,
			IMX8QM_PHY_MODE_MASK,
			IMX8QM_PHY_MODE_SATA);

	if (imxpriv->ext_osc) {
		dev_info(dev, "external osc is used.\n");
		/*
		 * bit0 rx ena 1, bit1 tx ena 0
		 * bit12 PHY_X1_EPCS_SEL 1.
		 */
		regmap_update_bits(imxpriv->gpr,
				IMX8QM_CSR_MISC_OFFSET,
				IMX8QM_MISC_IOB_RXENA,
				IMX8QM_MISC_IOB_RXENA);
		regmap_update_bits(imxpriv->gpr,
				IMX8QM_CSR_MISC_OFFSET,
				IMX8QM_MISC_IOB_TXENA,
				0);
	} else {
		dev_info(dev, "internal pll is used.\n");
		regmap_update_bits(imxpriv->gpr,
				IMX8QM_CSR_MISC_OFFSET,
				IMX8QM_MISC_IOB_RXENA,
				0);
		regmap_update_bits(imxpriv->gpr,
				IMX8QM_CSR_MISC_OFFSET,
				IMX8QM_MISC_IOB_TXENA,
				IMX8QM_MISC_IOB_TXENA);

	}
	regmap_update_bits(imxpriv->gpr,
			IMX8QM_CSR_MISC_OFFSET,
			IMX8QM_MISC_PHYX1_EPCS_SEL,
			IMX8QM_MISC_PHYX1_EPCS_SEL);
	/*
	 * It is possible, for PCIe and SATA are sharing
	 * the same clock source, HPLL or external oscillator.
	 * When PCIe is in low power modes (L1.X or L2 etc),
	 * the clock source can be turned off. In this case,
	 * if this clock source is required to be toggling by
	 * SATA, then SATA functions will be abnormal.
	 */
	regmap_update_bits(imxpriv->gpr,
			IMX8QM_CSR_MISC_OFFSET,
			IMX8QM_MISC_CLKREQN_OUT_OVERRIDE_1
			| IMX8QM_MISC_CLKREQN_OUT_OVERRIDE_0
			| IMX8QM_MISC_CLKREQN_IN_OVERRIDE_1
			| IMX8QM_MISC_CLKREQN_IN_OVERRIDE_0,
			IMX8QM_MISC_CLKREQN_OUT_OVERRIDE_1
			| IMX8QM_MISC_CLKREQN_OUT_OVERRIDE_0
			| IMX8QM_MISC_CLKREQN_IN_OVERRIDE_1
			| IMX8QM_MISC_CLKREQN_IN_OVERRIDE_0);

	/* clear PHY RST, then set it */
	regmap_update_bits(imxpriv->gpr,
			IMX8QM_CSR_SATA_OFFSET,
			IMX8QM_SATA_CTRL_EPCS_PHYRESET_N,
			0);

	regmap_update_bits(imxpriv->gpr,
			IMX8QM_CSR_SATA_OFFSET,
			IMX8QM_SATA_CTRL_EPCS_PHYRESET_N,
			IMX8QM_SATA_CTRL_EPCS_PHYRESET_N);
	regmap_update_bits(imxpriv->gpr,
			IMX8QM_CSR_SATA_OFFSET,
			IMX8QM_SATA_CTRL_EPCS_TXDEEMP,
			IMX8QM_SATA_CTRL_EPCS_TXDEEMP);
	regmap_update_bits(imxpriv->gpr,
			IMX8QM_CSR_SATA_OFFSET,
			IMX8QM_SATA_CTRL_EPCS_TXDEEMP_SEL,
			IMX8QM_SATA_CTRL_EPCS_TXDEEMP_SEL);

	/* CTRL RST: SET -> delay 1 us -> CLEAR -> SET */
	regmap_update_bits(imxpriv->gpr,
			IMX8QM_CSR_SATA_OFFSET,
			IMX8QM_SATA_CTRL_RESET_N,
			IMX8QM_SATA_CTRL_RESET_N);
	udelay(1);
	regmap_update_bits(imxpriv->gpr,
			IMX8QM_CSR_SATA_OFFSET,
			IMX8QM_SATA_CTRL_RESET_N,
			0);
	regmap_update_bits(imxpriv->gpr,
			IMX8QM_CSR_SATA_OFFSET,
			IMX8QM_SATA_CTRL_RESET_N,
			IMX8QM_SATA_CTRL_RESET_N);

	/* APB reset */
	regmap_update_bits(imxpriv->gpr,
			IMX8QM_CSR_PHYX1_OFFSET,
			IMX8QM_PHY_APB_RSTN_0,
			IMX8QM_PHY_APB_RSTN_0);

	for (i = 0; i < 100; i++) {
		reg = IMX8QM_CSR_PHYX1_OFFSET
			+ IMX8QM_CSR_PHYX_STTS0_OFFSET;
		regmap_read(imxpriv->gpr, reg, &val);
		val &= IMX8QM_STTS0_LANE0_TX_PLL_LOCK;
		if (val == IMX8QM_STTS0_LANE0_TX_PLL_LOCK)
			break;
		udelay(1);
	}

	if (val != IMX8QM_STTS0_LANE0_TX_PLL_LOCK) {
		dev_err(dev, "TX PLL of the PHY is not locked\n");
		ret = -ENODEV;
	} else {
		for (i = 0; i < 1000; i++) {
			reg = readb(imxpriv->phy_base +
					IMX8QM_SATA_PHY_REG48_PMA_STATUS);
			if (reg & IMX8QM_SATA_PHY_REG48_PMA_RDY)
				break;
			udelay(10);
		}
		if ((reg & IMX8QM_SATA_PHY_REG48_PMA_RDY) == 0) {
			dev_err(dev, "Calibration is NOT finished.\n");
			ret = -ENODEV;
			goto err_out;
		}

		writeb(imxpriv->imped_ratio, imxpriv->phy_base
				+ IMX8QM_SATA_PHY_REG03_RX_IMPED_RATIO);
		writeb(imxpriv->imped_ratio, imxpriv->phy_base
				+ IMX8QM_SATA_PHY_REG09_TX_IMPED_RATIO);
		reg = readb(imxpriv->phy_base
				+ IMX8QM_SATA_PHY_REG03_RX_IMPED_RATIO);
		if (unlikely(reg != imxpriv->imped_ratio))
			dev_info(dev, "Can't set PHY RX impedance ratio.\n");
		reg = readb(imxpriv->phy_base
				+ IMX8QM_SATA_PHY_REG09_TX_IMPED_RATIO);
		if (unlikely(reg != imxpriv->imped_ratio))
			dev_info(dev, "Can't set PHY TX impedance ratio.\n");

		/* Configure the tx_amplitude to pass the tests. */
		writeb(IMX8QM_SATA_PHY_TX_AMP_RATIO_MARGIN0, imxpriv->phy_base +
				IMX8QM_SATA_PHY_REG24_TX_AMP_RATIO_MARGIN0);
		writeb(IMX8QM_SATA_PHY_TX_AMP_RATIO_MARGIN1, imxpriv->phy_base +
				IMX8QM_SATA_PHY_REG25_TX_AMP_RATIO_MARGIN1);
		writeb(IMX8QM_SATA_PHY_TX_AMP_RATIO_MARGIN2, imxpriv->phy_base +
				IMX8QM_SATA_PHY_REG26_TX_AMP_RATIO_MARGIN2);

		/* Adjust the OOB COMINIT/COMWAKE to pass the tests. */
		writeb(IMX8QM_SATA_PHY_GEN1_TX_POST_CURSOR_RATIO,
				imxpriv->phy_base +
				IMX8QM_SATA_PHY_REG10_TX_POST_CURSOR_RATIO);
		writeb(IMX8QM_SATA_PHY_GEN2_TX_POST_CURSOR_RATIO,
				imxpriv->phy_base +
				IMX8QM_SATA_PHY_REG22_TX_POST_CURSOR_RATIO);

		writeb(IMX8QM_SATA_PHY_UPDATE_SETTING, imxpriv->phy_base +
				IMX8QM_SATA_PHY_REG128_UPDATE_SETTING);

		reg = IMX8QM_SATA_PP2CFG_COMINIT_NEGATE_MIN |
			IMX8QM_SATA_PP2CFG_COMINT_BURST_GAP |
			IMX8QM_SATA_PP2CFG_COMINT_BURST_GAP_MAX |
			IMX8QM_SATA_PP2CFG_COMINT_BURST_GAP_MIN;
		writel(reg, imxpriv->mmio + IMX8QM_SATA_PP2CFG_OFFSET);
		reg = IMX8QM_SATA_PP3CFG_COMWAKE_NEGATE_MIN |
			IMX8QM_SATA_PP3CFG_COMWAKE_BURST_GAP |
			IMX8QM_SATA_PP3CFG_COMWAKE_BURST_GAP_MAX |
			IMX8QM_SATA_PP3CFG_COMWAKE_BURST_GAP_MIN;
		writel(reg, imxpriv->mmio + IMX8QM_SATA_PP3CFG_OFFSET);

		udelay(100);

		/*
		 * To reduce the power consumption, gate off
		 * the PHY clks
		 */
#if CONFIG_IS_ENABLED(CLK)
		clk_disable(&imxpriv->phy_apbclk);
		clk_disable(&imxpriv->phy_pclk1);
		clk_disable(&imxpriv->phy_pclk0);
#endif
		return ret;
	}

err_out:
#if CONFIG_IS_ENABLED(CLK)
		clk_disable(&imxpriv->phy_apbclk);
disable_epcs_rx_clk:
		clk_disable(&imxpriv->epcs_rx_clk);
disable_epcs_tx_clk:
		clk_disable(&imxpriv->epcs_tx_clk);
disable_phy_pclk1:
		clk_disable(&imxpriv->phy_pclk1);
disable_phy_pclk0:
		clk_disable(&imxpriv->phy_pclk0);
#endif
	return ret;
}

static int imx8_sata_probe(struct udevice *dev, struct imx_ahci_priv *imxpriv)
{
	int ret = 0;
	fdt_addr_t addr;

	if (dev_read_u32u(dev, "ext_osc", &imxpriv->ext_osc)) {
		dev_info(dev, "ext_osc is not specified.\n");
		/* Use the external osc as ref clk defaultly. */
		imxpriv->ext_osc = 1;
	}

	if (dev_read_u32u(dev, "fsl,phy-imp", &imxpriv->imped_ratio)) {
		/*
		 * Regarding to the differnet Hw designs,
		 * Set the impedance ratio to 0x6c when 85OHM is used.
		 * Keep it to default value 0x80, when 100OHM is used.
		 */
		dev_info(dev, "phy impedance ratio is not specified.\n");
		imxpriv->imped_ratio = IMX8QM_SATA_PHY_IMPED_RATIO_85OHM;
	}

	addr = dev_read_addr_name(dev, "phy");
	if (addr == FDT_ADDR_T_NONE){
		dev_err(dev, "no phy space\n");
		return -ENOMEM;
	}

	imxpriv->phy_base = (void __iomem *)addr;

	imxpriv->gpr =
		 syscon_regmap_lookup_by_phandle(dev, "hsio");
	if (IS_ERR(imxpriv->gpr)) {
		dev_err(dev, "unable to find gpr registers\n");
		return PTR_ERR(imxpriv->gpr);
	}

#if CONFIG_IS_ENABLED(CLK)
	ret = clk_get_by_name(dev, "epcs_tx", &imxpriv->epcs_tx_clk);
	if (ret) {
		dev_err(dev, "can't get sata_epcs tx clock.\n");
		return ret;
	}

	ret = clk_get_by_name(dev, "epcs_rx", &imxpriv->epcs_rx_clk);
	if (ret) {
		dev_err(dev, "can't get sata_epcs rx clock.\n");
		return ret;
	}

	ret = clk_get_by_name(dev, "phy_pclk0", &imxpriv->phy_pclk0);
	if (ret) {
		dev_err(dev, "can't get sata_phy_pclk0 clock.\n");
		return ret;
	}

	ret = clk_get_by_name(dev, "phy_pclk1", &imxpriv->phy_pclk1);
	if (ret) {
		dev_err(dev, "can't get sata_phy_pclk1 clock.\n");
		return ret;
	}

	ret = clk_get_by_name(dev, "phy_apbclk", &imxpriv->phy_apbclk);
	if (ret) {
		dev_err(dev, "can't get sata_phy_apbclk clock.\n");
		return ret;
	}
#endif

	/* Fetch GPIO, then enable the external OSC */
	ret = gpio_request_by_name(dev, "clkreq-gpio", 0, &imxpriv->clkreq_gpio,
				   (GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE));
	if (ret) {
		dev_err(dev, "%d unable to get clkreq.\n", ret);
		return ret;
	}

	return 0;
}


static int imx_sata_enable(struct udevice *dev)
{
	struct imx_ahci_priv *imxpriv = dev_get_priv(dev);
	int ret = 0;

	if (imxpriv->type == AHCI_IMX6Q || imxpriv->type == AHCI_IMX6QP) {
		/*
		 * set PHY Paremeters, two steps to configure the GPR13,
		 * one write for rest of parameters, mask of first write
		 * is 0x07ffffff, and the other one write for setting
		 * the mpll_clk_en.
		 */
		regmap_update_bits(imxpriv->gpr, 0x34,
				   IOMUXC_GPR13_SATA_MASK,
				   imxpriv->phy_params);
		regmap_update_bits(imxpriv->gpr, 0x34,
				   IOMUXC_GPR13_SATA_PHY_1_MASK,
				   IOMUXC_GPR13_SATA_PHY_1_SLOW);

		udelay(200);
	}

	if (imxpriv->type == AHCI_IMX6Q) {
		ret = imx_sata_phy_reset(imxpriv);
	} else if (imxpriv->type == AHCI_IMX6QP) {
		/* 6qp adds the sata reset mechanism, use it for 6qp sata */
		regmap_update_bits(imxpriv->gpr, 0x14,
				   BIT(10), 0);

		regmap_update_bits(imxpriv->gpr, 0x14,
				   BIT(11), 0);
		udelay(50);
		regmap_update_bits(imxpriv->gpr, 0x14,
				   BIT(11), BIT(11));
	} else if (imxpriv->type == AHCI_IMX8QM) {
		ret = imx8_sata_enable(dev);
	}

	if (ret) {
		dev_err(dev, "failed to reset phy: %d\n", ret);
		return ret;
	}

	udelay(2000);

	return 0;
}

static void imx_sata_disable(struct udevice *dev)
{
	struct imx_ahci_priv *imxpriv = dev_get_priv(dev);

	if (imxpriv->type == AHCI_IMX6QP)
		regmap_update_bits(imxpriv->gpr, 0x14,
				   BIT(10), BIT(10));

	if (imxpriv->type == AHCI_IMX6Q || imxpriv->type == AHCI_IMX6QP) {
		regmap_update_bits(imxpriv->gpr, 0x34,
				   IOMUXC_GPR13_SATA_PHY_1_MASK,
				   0);
	}

	if (imxpriv->type == AHCI_IMX8QM) {
#if CONFIG_IS_ENABLED(CLK)
		clk_disable(&imxpriv->epcs_rx_clk);
		clk_disable(&imxpriv->epcs_tx_clk);
#endif
	}
}


static int imx_ahci_bind(struct udevice *dev)
{
	struct udevice *scsi_dev;

	return ahci_bind_scsi(dev, &scsi_dev);
}

static int imx_ahci_probe(struct udevice *dev)
{
	int ret = 0;
	struct imx_ahci_priv *priv = dev_get_priv(dev);
	fdt_addr_t addr;
	unsigned int reg_val;

	priv->type = (enum ahci_imx_type)dev_get_driver_data(dev);

#if CONFIG_IS_ENABLED(CLK)
	ret = clk_get_by_name(dev, "sata", &priv->sata_clk);
	if (ret) {
		printf("Failed to get sata clk\n");
		return ret;
	}

	ret = clk_get_by_name(dev, "sata_ref", &priv->sata_ref_clk);
	if (ret) {
		printf("Failed to get sata_ref clk\n");
		return ret;
	}
#endif

	addr = dev_read_addr(dev);
	if (addr == FDT_ADDR_T_NONE) {
		dev_err(dev, "no mmio space\n");
		return -EINVAL;
	}

	priv->mmio = (void __iomem *)addr;

	if (priv->type == AHCI_IMX6Q || priv->type == AHCI_IMX6QP) {
		priv->gpr = syscon_regmap_lookup_by_phandle(dev, "gpr");
		if (IS_ERR(priv->gpr)) {
			dev_err(dev,
				"failed to find fsl,imx6q-iomux-gpr regmap\n");
			return PTR_ERR(priv->gpr);
		}

		priv->phy_params =
				   IOMUXC_GPR13_SATA_PHY_7_SATA2M |
				   (3 << IOMUXC_GPR13_SATA_PHY_6_SHIFT) |
				   IOMUXC_GPR13_SATA_SPEED_3G |
				   IOMUXC_GPR13_SATA_PHY_2_TX_1P025V |
				   IOMUXC_GPR13_SATA_PHY_3_TXBOOST_3P33_DB |
				   IOMUXC_GPR13_SATA_SATA_PHY_4_ATTEN_9_16 |
				   IOMUXC_GPR13_SATA_PHY_8_RXEQ_3P0DB |
				   IOMUXC_GPR13_SATA_SATA_PHY_5_SS_DISABLED;
	} else if (priv->type == AHCI_IMX8QM) {
		ret =  imx8_sata_probe(dev, priv);
		if (ret)
			return ret;
	}

#if CONFIG_IS_ENABLED(CLK)
	ret = clk_enable(&priv->sata_clk);
	if (ret)
		return ret;

	ret = clk_enable(&priv->sata_ref_clk);
	if (ret)
		return ret;
#else
	enable_sata_clock();
#endif

	ret = imx_sata_enable(dev);
	if (ret)
		goto disable_clk;

	/*
	 * Configure the HWINIT bits of the HOST_CAP and HOST_PORTS_IMPL,
	 * and IP vendor specific register IMX_TIMER1MS.
	 * Configure CAP_SSS (support stagered spin up).
	 * Implement the port0.
	 * Get the ahb clock rate, and configure the TIMER1MS register.
	 */
	reg_val = readl(priv->mmio + HOST_CAP);
	if (!(reg_val & (1 << 27))) {
		reg_val |= (1 << 27);
		writel(reg_val, priv->mmio + HOST_CAP);
	}
	reg_val = readl(priv->mmio + HOST_PORTS_IMPL);
	if (!(reg_val & 0x1)) {
		reg_val |= 0x1;
		writel(reg_val, priv->mmio + HOST_PORTS_IMPL);
	}

#if CONFIG_IS_ENABLED(CLK)
	ret = clk_get_by_name(dev, "ahb", &priv->ahb_clk);
	if (ret) {
		dev_info(dev, "no ahb clock.\n");
	} else {
		/*
		 * AHB clock is only used to configure the vendor specified
		 * TIMER1MS register. Set it if the AHB clock is defined.
		 */
		reg_val = clk_get_rate(&priv->ahb_clk) / 1000;
		writel(reg_val, priv->mmio + IMX_TIMER1MS);
	}
#else
	reg_val = mxc_get_clock(MXC_AHB_CLK) / 1000;
	writel(reg_val, priv->mmio + IMX_TIMER1MS);
#endif

	ret = ahci_probe_scsi(dev, (ulong)priv->mmio);
	if (ret)
		goto disable_sata;

	return ret;

disable_sata:
	imx_sata_disable(dev);
disable_clk:
#if CONFIG_IS_ENABLED(CLK)
	clk_disable(&priv->sata_ref_clk);
	clk_disable(&priv->sata_clk);
#else
	disable_sata_clock();
#endif
	return ret;
}

static int imx_ahci_remove(struct udevice *dev)
{
	imx_sata_disable(dev);

#if CONFIG_IS_ENABLED(CLK)
	struct imx_ahci_priv *priv = dev_get_priv(dev);
	clk_disable(&priv->sata_ref_clk);
	clk_disable(&priv->sata_clk);
#else
	disable_sata_clock();
#endif

	return 0;
}


static const struct udevice_id imx_ahci_ids[] = {
	{ .compatible = "fsl,imx6q-ahci", .data = (ulong)AHCI_IMX6Q },
	{ .compatible = "fsl,imx6qp-ahci", .data = (ulong)AHCI_IMX6QP },
	{ .compatible = "fsl,imx8qm-ahci", .data = (ulong)AHCI_IMX8QM },
	{ }
};

U_BOOT_DRIVER(imx_ahci) = {
	.name	= "imx_ahci",
	.id	= UCLASS_AHCI,
	.of_match = imx_ahci_ids,
	.bind	= imx_ahci_bind,
	.probe	= imx_ahci_probe,
	.remove	= imx_ahci_remove,
	.priv_auto_alloc_size = sizeof(struct imx_ahci_priv),
};
