// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2020 NXP
 */

#include <common.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/arch/imx8ulp-pins.h>
#include <asm/arch/clock.h>
#include <asm/arch/pcc.h>
#include <asm/arch/sys_proto.h>
#include <miiphy.h>
#include <netdev.h>

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_NXP_FSPI) || defined(CONFIG_FSL_FSPI_NAND)
#define FSPI_PAD_CTRL	(PAD_CTL_PUS_UP | PAD_CTL_DSE)
static iomux_cfg_t const flexspi2_pads[] = {
	IMX8ULP_PAD_PTD12__FLEXSPI2_A_SS0_B | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTD13__FLEXSPI2_A_SCLK | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTD17__FLEXSPI2_A_DATA0 | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTD16__FLEXSPI2_A_DATA1 | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTD15__FLEXSPI2_A_DATA2 | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTD14__FLEXSPI2_A_DATA3 | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTD22__FLEXSPI2_A_DATA4 | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTD21__FLEXSPI2_A_DATA5 | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTD20__FLEXSPI2_A_DATA6 | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTD19__FLEXSPI2_A_DATA7 | MUX_PAD_CTRL(FSPI_PAD_CTRL),
};

static iomux_cfg_t const flexspi0_pads[] = {
	IMX8ULP_PAD_PTC5__FLEXSPI0_A_SS0_b | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTC6__FLEXSPI0_A_SCLK | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTC10__FLEXSPI0_A_DATA0 | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTC9__FLEXSPI0_A_DATA1 | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTC8__FLEXSPI0_A_DATA2 | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTC7__FLEXSPI0_A_DATA3 | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTC4__FLEXSPI0_A_DATA4 | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTC3__FLEXSPI0_A_DATA5 | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTC2__FLEXSPI0_A_DATA6 | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTC1__FLEXSPI0_A_DATA7 | MUX_PAD_CTRL(FSPI_PAD_CTRL),
};

static void setup_flexspi(void)
{
	imx8ulp_iomux_setup_multiple_pads(flexspi2_pads, ARRAY_SIZE(flexspi2_pads));

	init_clk_fspi(0);
}

static void setup_rtd_flexspi0(void)
{
	imx8ulp_iomux_setup_multiple_pads(flexspi0_pads, ARRAY_SIZE(flexspi0_pads));

	/* Set PCC of flexspi0, 192Mhz % 4 = 48Mhz */
	writel(0xD6000003, 0x280300e4);
}

#endif

#if IS_ENABLED(CONFIG_FEC_MXC)
#define ENET_CLK_PAD_CTRL	(PAD_CTL_PUS_UP | PAD_CTL_DSE | PAD_CTL_IBE_ENABLE)
static iomux_cfg_t const enet_clk_pads[] = {
	IMX8ULP_PAD_PTE19__ENET0_REFCLK | MUX_PAD_CTRL(ENET_CLK_PAD_CTRL),
	IMX8ULP_PAD_PTF10__ENET0_1588_CLKIN | MUX_PAD_CTRL(ENET_CLK_PAD_CTRL),
};

static int setup_fec(void)
{
	/*
	 * Since ref clock and timestamp clock are from external,
	 * set the iomux prior the clock enablement
	 */
	imx8ulp_iomux_setup_multiple_pads(enet_clk_pads, ARRAY_SIZE(enet_clk_pads));

	/* Select enet time stamp clock: 001 - External Timestamp Clock */
	cgc1_enet_stamp_sel(1);

	/* enable FEC PCC */
	pcc_clock_enable(4, ENET_PCC4_SLOT, true);
	pcc_reset_peripheral(4, ENET_PCC4_SLOT, false);

	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
	if (phydev->drv->config)
		phydev->drv->config(phydev);
	return 0;
}
#endif

int board_init(void)
{
#if defined(CONFIG_NXP_FSPI) || defined(CONFIG_FSL_FSPI_NAND)
	setup_flexspi();

	if (get_boot_mode() == SINGLE_BOOT) {
		setup_rtd_flexspi0();
	}
#endif

	if (IS_ENABLED(CONFIG_FEC_MXC))
		setup_fec();

	return 0;
}

int board_early_init_f(void)
{
	return 0;
}

int board_late_init(void)
{
	return 0;
}

#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_ANDROID_RECOVERY
int is_recovery_key_pressing(void)
{
	return 0; /*TODO*/
}
#endif /*CONFIG_ANDROID_RECOVERY*/
#endif /*CONFIG_FSL_FASTBOOT*/
