// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2020 NXP
 */

#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <miiphy.h>
#include <netdev.h>
#include <fsl_esdhc.h>
#include <mmc.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <power/regulator.h>
#if defined (CONFIG_TARGET_IMX8MM_AB2) || defined(CONFIG_TARGET_IMX8MM_DDR4_AB2)
#include <asm/arch/imx8mm_pins.h>
#else
#include <asm/arch/imx8mn_pins.h>
#endif
#include <asm/arch/sys_proto.h>
#include <asm-generic/gpio.h>
#include <asm/mach-imx/dma.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <spl.h>

DECLARE_GLOBAL_DATA_PTR;

#define PWR_EN_5V0	IMX_GPIO_NR(1, 7)
#define PWR_EN_ANA	IMX_GPIO_NR(1, 10)
#define UART_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL1)
#define WDOG_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_ODE | PAD_CTL_PUE | PAD_CTL_PE)

#if defined (CONFIG_TARGET_IMX8MM_AB2) || defined(CONFIG_TARGET_IMX8MM_DDR4_AB2)
static iomux_v3_cfg_t const uart_pads[] = {
	IMX8MM_PAD_UART2_RXD_UART2_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MM_PAD_UART2_TXD_UART2_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const wdog_pads[] = {
	IMX8MM_PAD_GPIO1_IO02_WDOG1_WDOG_B | MUX_PAD_CTRL(WDOG_PAD_CTRL),
};

static iomux_v3_cfg_t const pwr_en_5v0[] = {
	IMX8MM_PAD_GPIO1_IO07_GPIO1_IO7 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static iomux_v3_cfg_t const pwr_en_ana[] = {
	IMX8MM_PAD_GPIO1_IO10_GPIO1_IO10 | MUX_PAD_CTRL(NO_PAD_CTRL),
};
#endif

#if defined(CONFIG_TARGET_IMX8MN_AB2) || defined(CONFIG_TARGET_IMX8MN_DDR4_AB2)
static iomux_v3_cfg_t const uart_pads[] = {
	IMX8MN_PAD_UART2_RXD__UART2_DCE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MN_PAD_UART2_TXD__UART2_DCE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const wdog_pads[] = {
	IMX8MN_PAD_GPIO1_IO02__WDOG1_WDOG_B | MUX_PAD_CTRL(WDOG_PAD_CTRL),
};

static iomux_v3_cfg_t const pwr_en_5v0[] = {
	IMX8MN_PAD_GPIO1_IO07__GPIO1_IO7 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static iomux_v3_cfg_t const pwr_en_ana[] = {
	IMX8MN_PAD_GPIO1_IO10__GPIO1_IO10 | MUX_PAD_CTRL(NO_PAD_CTRL),
};
#endif

#ifdef CONFIG_NAND_MXS
static void setup_gpmi_nand(void)
{
	init_nand_clk();
}
#endif

int board_early_init_f(void)
{
	struct wdog_regs *wdog = (struct wdog_regs *)WDOG1_BASE_ADDR;

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));

	set_wdog_reset(wdog);

	imx_iomux_v3_setup_multiple_pads(uart_pads, ARRAY_SIZE(uart_pads));

	init_uart_clk(1);

	imx_iomux_v3_setup_multiple_pads(pwr_en_5v0, ARRAY_SIZE(pwr_en_5v0));
	gpio_request(PWR_EN_5V0, "pwr_en_5v0");
	gpio_direction_output(PWR_EN_5V0, 1);

	imx_iomux_v3_setup_multiple_pads(pwr_en_ana, ARRAY_SIZE(pwr_en_ana));
	gpio_request(PWR_EN_ANA, "pwr_en_ana");
	gpio_direction_output(PWR_EN_ANA, 0);

	return 0;
}

static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *gpr =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	/* Use 125M anatop REF_CLK1 for ENET1, not from external */
	clrsetbits_le32(&gpr->gpr[1],
			IOMUXC_GPR_GPR1_GPR_ENET1_TX_CLK_SEL_MASK, 0);

	return set_clk_enet(ENET_125MHZ);
}

int board_phy_config(struct phy_device *phydev)
{
	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}

int board_init(void)
{
#ifdef CONFIG_DM_REGULATOR
	regulators_enable_boot_on(false);
#endif

#ifdef CONFIG_FEC_MXC
	setup_fec();
#endif

#ifdef CONFIG_NAND_MXS
	setup_gpmi_nand();
#endif
	return 0;
}

int board_late_init(void)
{
#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif
#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	env_set("board_name", "AB2");
#if defined (CONFIG_TARGET_IMX8MM_AB2) || defined(CONFIG_TARGET_IMX8MM_DDR4_AB2)
	env_set("board_rev", "iMX8MM");
#else
	env_set("board_rev", "iMX8MN");
#endif
#endif
	return 0;
}
