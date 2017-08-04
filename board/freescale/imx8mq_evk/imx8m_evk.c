/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <asm/io.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/imx-common/iomux-v3.h>
#include <asm-generic/gpio.h>
#include <fsl_esdhc.h>
#include <mmc.h>
#include <asm/arch/imx8mq_pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/imx-common/gpio.h>
#include <asm/imx-common/mxc_i2c.h>
#include <asm/arch/clock.h>
#include <spl.h>
#include <power/pmic.h>
#include <power/pfuze100_pmic.h>
#include "../common/pfuze.h"

DECLARE_GLOBAL_DATA_PTR;

#define QSPI_PAD_CTRL	(PAD_CTL_DSE2 | PAD_CTL_HYS)

#define UART_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL1)

#define ENET_PAD_CTRL		(PAD_CTL_DSE7 | PAD_CTL_FSEL3)
#define ENET_PAD_CTRL_MII_MDIO	(PAD_CTL_ODE | PAD_CTL_DSE3 | PAD_CTL_FSEL0)
#define ENET_PAD_CTRL_MII_MDC	(PAD_CTL_DSE3 | PAD_CTL_FSEL0)
#define ENET_RX_PAD_CTRL	(PAD_CTL_DSE7 | PAD_CTL_FSEL3)

#define WDOG_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_HYS | PAD_CTL_PUE)

static iomux_v3_cfg_t const wdog_pads[] = {
	IMX8MQ_PAD_GPIO1_IO02__WDOG1_WDOG_B | MUX_PAD_CTRL(WDOG_PAD_CTRL),
};

#ifdef CONFIG_FSL_QSPI
static iomux_v3_cfg_t const qspi_pads[] = {
	IMX8MQ_PAD_NAND_ALE__QSPI_A_SCLK | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	IMX8MQ_PAD_NAND_CE0_B__QSPI_A_SS0_B | MUX_PAD_CTRL(QSPI_PAD_CTRL),

	IMX8MQ_PAD_NAND_DATA00__QSPI_A_DATA0 | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	IMX8MQ_PAD_NAND_DATA01__QSPI_A_DATA1 | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	IMX8MQ_PAD_NAND_DATA02__QSPI_A_DATA2 | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	IMX8MQ_PAD_NAND_DATA03__QSPI_A_DATA3 | MUX_PAD_CTRL(QSPI_PAD_CTRL),
};

int board_qspi_init(void)
{
	imx_iomux_v3_setup_multiple_pads(qspi_pads, ARRAY_SIZE(qspi_pads));

	set_clk_qspi();

	return 0;
}
#endif

static iomux_v3_cfg_t const uart_pads[] = {
	IMX8MQ_PAD_UART1_RXD__UART1_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MQ_PAD_UART1_TXD__UART1_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

int board_early_init_f(void)
{
	imx_iomux_v3_setup_multiple_pads(uart_pads, ARRAY_SIZE(uart_pads));

	return 0;
}

#ifdef CONFIG_BOARD_POSTCLK_INIT
int board_postclk_init(void)
{
	/* TODO */
	return 0;
}
#endif

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_SIZE;

	return 0;
}

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, bd_t *bd)
{
	return 0;
}
#endif

#ifdef CONFIG_FEC_MXC
#define FEC_RST_PAD IMX_GPIO_NR(1, 9)
static iomux_v3_cfg_t const fec1_rst_pads[] = {
	IMX8MQ_PAD_GPIO1_IO09__GPIO1_IO9 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static iomux_v3_cfg_t const fec1_pads[] = {
	IMX8MQ_PAD_ENET_MDC__ENET_MDC | MUX_PAD_CTRL(ENET_PAD_CTRL_MII_MDC),
	IMX8MQ_PAD_ENET_MDIO__ENET_MDIO | MUX_PAD_CTRL(ENET_PAD_CTRL_MII_MDIO),
	IMX8MQ_PAD_ENET_TD3__ENET_RGMII_TD3 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MQ_PAD_ENET_TD2__ENET_RGMII_TD2 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MQ_PAD_ENET_TD1__ENET_RGMII_TD1 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MQ_PAD_ENET_TD0__ENET_RGMII_TD0 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MQ_PAD_ENET_TX_CTL__ENET_RGMII_TX_CTL | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MQ_PAD_ENET_TXC__ENET_RGMII_TXC | MUX_PAD_CTRL(ENET_PAD_CTRL),
	IMX8MQ_PAD_ENET_RX_CTL__ENET_RGMII_RX_CTL | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	IMX8MQ_PAD_ENET_RXC__ENET_RGMII_RXC | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	IMX8MQ_PAD_ENET_RD3__ENET_RGMII_RD3 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	IMX8MQ_PAD_ENET_RD2__ENET_RGMII_RD2 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	IMX8MQ_PAD_ENET_RD1__ENET_RGMII_RD1 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	IMX8MQ_PAD_ENET_RD0__ENET_RGMII_RD0 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
};

static void setup_iomux_fec(void)
{
	imx_iomux_v3_setup_multiple_pads(fec1_rst_pads, ARRAY_SIZE(fec1_rst_pads));

	gpio_request(IMX_GPIO_NR(1, 9), "fec1_rst");
	gpio_direction_output(IMX_GPIO_NR(1, 9), 0);
	udelay(500);
	gpio_direction_output(IMX_GPIO_NR(1, 9), 1);

	imx_iomux_v3_setup_multiple_pads(fec1_pads, ARRAY_SIZE(fec1_pads));
}

int board_eth_init(bd_t *bis)
{
	int ret;

	setup_iomux_fec();

	ret = fecmxc_initialize_multi(bis, 0, CONFIG_FEC_MXC_PHYADDR, IMX_FEC_BASE);
	if (ret)
		printf("FEC1 MXC: %s:failed\n", __func__);

	return ret;
}

static int setup_fec(void)
{
	/* Use 125M anatop REF_CLK1 for ENET1, not from external */
	clrsetbits_le32(IOMUXC_GPR1,
			BIT(13) | BIT(17), 0);
	return set_clk_enet(ENET_125MHz);
}


int board_phy_config(struct phy_device *phydev)
{
	/* enable rgmii rxc skew and phy mode select to RGMII copper */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x1f);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x8);

	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x100);

	if (phydev->drv->config)
		phydev->drv->config(phydev);
	return 0;
}
#endif

int board_init(void)
{
	board_qspi_init();

#ifdef CONFIG_FEC_MXC
	setup_fec();
#endif

	return 0;
}

int board_mmc_get_env_dev(int devno)
{
	return devno;
}

int board_late_init(void)
{
	struct wdog_regs *wdog = (struct wdog_regs *)WDOG1_BASE_ADDR;

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));

	set_wdog_reset(wdog);

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

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
