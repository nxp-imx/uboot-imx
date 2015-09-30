// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2013-2016 Freescale Semiconductor, Inc.
 */

#include <common.h>
#include <init.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/siul.h>
#include <asm/arch/clock.h>
#include <fdt_support.h>
#include <linux/libfdt.h>
#include <miiphy.h>
#include <netdev.h>
#include <i2c.h>

DECLARE_GLOBAL_DATA_PTR;

static void setup_iomux_uart(void)
{
	/* Muxing for linflex0 and linflex1 */

	/* set TXD - MSCR[12] PA12 */
	writel(SIUL2_UART_TXD, SIUL2_MSCRn(SIUL2_UART0_TXD_PAD));

	/* set RXD - MSCR[11] - PA11 */
	writel(SIUL2_UART_MSCR_RXD, SIUL2_MSCRn(SIUL2_UART0_MSCR_RXD_PAD));

	/* set RXD - IMCR[200] - 200 */
	writel(SIUL2_UART_IMCR_RXD, SIUL2_IMCRn(SIUL2_UART0_IMCR_RXD_PAD));

	/* set TXD - MSCR[14] PA14 */
	writel(SIUL2_UART_TXD, SIUL2_MSCRn(SIUL2_UART1_TXD_PAD));

	/* set RXD - MSCR[13] - PA13*/
	writel(SIUL2_UART_MSCR_RXD, SIUL2_MSCRn(SIUL2_UART1_MSCR_RXD_PAD));

	/* set RXD - IMCR[202] - 202 */
	writel(SIUL2_UART_IMCR_RXD, SIUL2_IMCRn(SIUL2_UART1_IMCR_RXD_PAD));
}
static void setup_iomux_enet(void)
{
}

static void setup_iomux_i2c(void)
{
}

#ifdef CONFIG_SYS_USE_NAND
void setup_iomux_nfc(void)
{
}
#endif

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

int board_early_init_f(void)
{
// start_secondary_cores();
	clock_init();
	mscm_init();

	setup_iomux_uart();
	setup_iomux_enet();
	setup_iomux_i2c();
#ifdef CONFIG_SYS_USE_NAND
	setup_iomux_nfc();
#endif
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
	puts("Board: s32v234evb\n");

	return 0;
}

#if defined(CONFIG_OF_FDT) && defined(CONFIG_OF_BOARD_SETUP)
int ft_board_setup(void *blob, bd_t *bd)
{
	ft_cpu_setup(blob, bd);
	return 0;
}
#endif /* defined(CONFIG_OF_FDT) && defined(CONFIG_OF_BOARD_SETUP) */
