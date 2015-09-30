// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2013-2016 Freescale Semiconductor, Inc.
 * (C) Copyright 2017,2020 NXP
 *
 */

#include <common.h>
#include <fdt_support.h>
#include <i2c.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/io.h>
#include <asm/arch/soc.h>
#include <asm/arch/siul.h>
#include <asm/arch/xrdc.h>
#include <linux/libfdt.h>

DECLARE_GLOBAL_DATA_PTR;

static void setup_iomux_uart(void)
{
	/* Muxing for linflex1 */

	/* set PA14 - MSCR[14] - for UART1 TXD*/
	writel(SIUL2_MSCR_PORT_CTRL_UART_TXD, SIUL2_MSCRn(SIUL2_MSCR_PA14));

	/* set PA13 - MSCR[13] - for UART1 RXD */
	writel(SIUL2_MSCR_PORT_CTRL_UART_RXD, SIUL2_MSCRn(SIUL2_MSCR_PA13));
	/* set UART1 RXD - IMCR[202] - to link to PA13 */
	writel(SIUL2_IMCR_UART_RXD_to_pad, SIUL2_IMCRn(SIUL2_IMCR_UART1_RXD));
}

void setup_iomux_enet(void)
{
#ifndef CONFIG_PHY_RGMII_DIRECT_CONNECTED
	/* set PC13 - MSCR[45] - for MDC */
	writel(SIUL2_MSCR_ENET_MDC, SIUL2_MSCRn(SIUL2_MSCR_PC13));

	/* set PC14 - MSCR[46] - for MDIO */
	writel(SIUL2_MSCR_ENET_MDIO, SIUL2_MSCRn(SIUL2_MSCR_PC14));
	writel(SIUL2_MSCR_ENET_MDIO_IN, SIUL2_MSCRn(SIUL2_MSCR_PC14_IN));
#endif

	/* set PC15 - MSCR[47] - for TX CLK SWITCH */
	writel(SIUL2_MSCR_ENET_TX_CLK_SWITCH,
	       SIUL2_MSCRn(SIUL2_MSCR_PC15_SWITCH));
	writel(SIUL2_MSCR_ENET_TX_CLK_IN, SIUL2_MSCRn(SIUL2_MSCR_PC15_IN));

	/* set PD0 - MSCR[48] - for RX_CLK */
	writel(SIUL2_MSCR_ENET_RX_CLK, SIUL2_MSCRn(SIUL2_MSCR_PD0));
	writel(SIUL2_MSCR_ENET_RX_CLK_IN, SIUL2_MSCRn(SIUL2_MSCR_PD0_IN));

	/* set PD1 - MSCR[49] - for RX_D0 */
	writel(SIUL2_MSCR_ENET_RX_D0, SIUL2_MSCRn(SIUL2_MSCR_PD1));
	writel(SIUL2_MSCR_ENET_RX_D0_IN, SIUL2_MSCRn(SIUL2_MSCR_PD1_IN));

	/* set PD2 - MSCR[50] - for RX_D1 */
	writel(SIUL2_MSCR_ENET_RX_D1, SIUL2_MSCRn(SIUL2_MSCR_PD2));
	writel(SIUL2_MSCR_ENET_RX_D1_IN, SIUL2_MSCRn(SIUL2_MSCR_PD2_IN));

	/* set PD3 - MSCR[51] - for RX_D2 */
	writel(SIUL2_MSCR_ENET_RX_D2, SIUL2_MSCRn(SIUL2_MSCR_PD3));
	writel(SIUL2_MSCR_ENET_RX_D2_IN, SIUL2_MSCRn(SIUL2_MSCR_PD3_IN));

	/* set PD4 - MSCR[52] - for RX_D3 */
	writel(SIUL2_MSCR_ENET_RX_D3, SIUL2_MSCRn(SIUL2_MSCR_PD4));
	writel(SIUL2_MSCR_ENET_RX_D3_IN, SIUL2_MSCRn(SIUL2_MSCR_PD4_IN));

	/* set PD5 - MSCR[53] - for RX_DV */
	writel(SIUL2_MSCR_ENET_RX_DV, SIUL2_MSCRn(SIUL2_MSCR_PD5));
	writel(SIUL2_MSCR_ENET_RX_DV_IN, SIUL2_MSCRn(SIUL2_MSCR_PD5_IN));

	/* set PD7 - MSCR[55] - for TX_D0 */
	writel(SIUL2_MSCR_ENET_TX_D0, SIUL2_MSCRn(SIUL2_MSCR_PD7));
	/* set PD8 - MSCR[56] - for TX_D1 */
	writel(SIUL2_MSCR_ENET_TX_D1, SIUL2_MSCRn(SIUL2_MSCR_PD8));
	/* set PD9 - MSCR[57] - for TX_D2 */
	writel(SIUL2_MSCR_ENET_TX_D2, SIUL2_MSCRn(SIUL2_MSCR_PD9));
	/* set PD10 - MSCR[58] - for TX_D3 */
	writel(SIUL2_MSCR_ENET_TX_D3, SIUL2_MSCRn(SIUL2_MSCR_PD10));
	/* set PD11 - MSCR[59] - for TX_EN */
	writel(SIUL2_MSCR_ENET_TX_EN, SIUL2_MSCRn(SIUL2_MSCR_PD11));
}

static void setup_iomux_i2c(void)
{
	/* I2C0 - Serial Data Input */
	writel(SIUL2_PAD_CTRL_I2C0_MSCR_SDA_AC15,
	       SIUL2_MSCRn(SIUL2_MSCR_PG3));
	writel(SIUL2_PAD_CTRL_I2C0_IMCR_SDA_AC15,
	       SIUL2_IMCRn(SIUL2_IMCR_I2C0_DATA));

	/* I2C0 - Serial Clock Input */
	writel(SIUL2_PAD_CTRL_I2C0_MSCR_SCLK_AE15,
	       SIUL2_MSCRn(SIUL2_MSCR_PG4));
	writel(SIUL2_PAD_CTRL_I2C0_IMCR_SCLK_AE15,
	       SIUL2_IMCRn(SIUL2_IMCR_I2C0_CLK));

	/* I2C1 - Serial Data Input */
	writel(SIUL2_PAD_CTRL_I2C1_MSCR_SDA,
	       SIUL2_MSCRn(SIUL2_MSCR_PG5));
	writel(SIUL2_PAD_CTRL_I2C1_IMCR_SDA,
	       SIUL2_IMCRn(SIUL2_IMCR_I2C1_DATA));

	/* I2C1 - Serial Clock Input */
	writel(SIUL2_PAD_CTRL_I2C1_MSCR_SCLK,
	       SIUL2_MSCRn(SIUL2_MSCR_PG6));
	writel(SIUL2_PAD_CTRL_I2C1_IMCR_SCLK,
	       SIUL2_IMCRn(SIUL2_IMCR_I2C1_CLK));

	/* I2C2 - Serial Data Input */
	writel(SIUL2_PAD_CTRL_I2C2_MSCR_SDA,
	       SIUL2_MSCRn(SIUL2_MSCR_PB3));
	writel(SIUL2_PAD_CTRL_I2C2_IMCR_SDA,
	       SIUL2_IMCRn(SIUL2_IMCR_I2C2_DATA));

	/* I2C2 - Serial Clock Input */
	writel(SIUL2_PAD_CTRL_I2C2_MSCR_SCLK,
	       SIUL2_MSCRn(SIUL2_MSCR_PB4));
	writel(SIUL2_PAD_CTRL_I2C2_IMCR_SCLK,
	       SIUL2_IMCRn(SIUL2_IMCR_I2C2_CLK));
}

static void mscm_init(void)
{
	struct mscm_ir *mscmir = (struct mscm_ir *)MSCM_BASE_ADDR;
	int i;

	for (i = 0; i < MSCM_IRSPRC_NUM; i++)
		writew(MSCM_IRSPRC_CPn_EN, &mscmir->irsprc[i]);
}

int board_phy_config(struct phy_device *phydev)
{
	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}

void setup_xrdc(void)
{
	writel(XRDC_ADDR_MIN, XRDC_MRGD_W0_16);
	writel(XRDC_ADDR_MAX, XRDC_MRGD_W1_16);
	writel(XRDC_VALID, XRDC_MRGD_W3_16);

	writel(XRDC_ADDR_MIN, XRDC_MRGD_W0_17);
	writel(XRDC_ADDR_MAX, XRDC_MRGD_W1_17);
	writel(XRDC_VALID, XRDC_MRGD_W3_17);

	writel(XRDC_ADDR_MIN, XRDC_MRGD_W0_18);
	writel(XRDC_ADDR_MAX, XRDC_MRGD_W1_18);
	writel(XRDC_VALID, XRDC_MRGD_W3_18);

	writel(XRDC_ADDR_MIN, XRDC_MRGD_W0_19);
	writel(XRDC_ADDR_MAX, XRDC_MRGD_W1_19);
	writel(XRDC_VALID, XRDC_MRGD_W3_19);
}

int board_early_init_f(void)
{
	clock_init();
	mscm_init();

	setup_iomux_uart();
	setup_iomux_enet();
	setup_iomux_i2c();

#ifdef CONFIG_FSL_DCU_FB
	setup_iomux_dcu();
#endif
#ifdef CONFIG_DCU_QOS_FIX
	board_dcu_qos();
#endif
	setup_xrdc();

	return 0;
}

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

	return 0;
}

int checkboard(void)
{
	puts("Board: s32v234pcie\n");

	return 0;
}

#if defined(CONFIG_OF_FDT) && defined(CONFIG_OF_BOARD_SETUP)
int ft_board_setup(void *blob, bd_t *bd)
{
	ft_cpu_setup(blob, bd);
	return 0;
}
#endif /* defined(CONFIG_OF_FDT) && defined(CONFIG_OF_BOARD_SETUP) */
