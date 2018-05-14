/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/mx7ulp-pins.h>
#include <asm/arch/iomux.h>
#include <asm/gpio.h>
#include <fsl_esdhc.h>
#include <mmc.h>
#include <usb.h>

DECLARE_GLOBAL_DATA_PTR;

#define ESDHC_PAD_CTRL	(PAD_CTL_PUS_UP | PAD_CTL_DSE)
#define ESDHC_CD_GPIO_PAD_CTRL (PAD_CTL_IBE_ENABLE | PAD_CTL_PUS_UP)

#define UART_PAD_CTRL	(PAD_CTL_PUS_UP)

#define GPIO_PAD_CTRL	(PAD_CTL_OBE_ENABLE | PAD_CTL_IBE_ENABLE)

#define OTG_ID_GPIO_PAD_CTRL	(PAD_CTL_IBE_ENABLE)
#define OTG_PWR_GPIO_PAD_CTRL	(PAD_CTL_OBE_ENABLE)

#define QSPI_PAD_CTRL1	(PAD_CTL_PUS_UP | PAD_CTL_DSE)

#define QSPI_PAD_CTRL0	(PAD_CTL_PUS_UP | PAD_CTL_DSE \
	| PAD_CTL_OBE_ENABLE)


int dram_init(void)
{
    gd->ram_size = PHYS_SDRAM_SIZE;

	return 0;
}

static iomux_cfg_t const lpuart4_pads[] = {
    MX7ULP_PAD_PTC3__LPUART4_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
    MX7ULP_PAD_PTC2__LPUART4_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

/* PTF11 and PTF10 also can mux to LPUART6 on 10x10 ARM2, depends on rework*/
static iomux_cfg_t const lpuart6_pads[] = {
    MX7ULP_PAD_PTE11__LPUART6_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
    MX7ULP_PAD_PTE10__LPUART6_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static void setup_iomux_uart(void)
{
#ifdef CONFIG_TARGET_MX7ULP_10X10_ARM2
	mx7ulp_iomux_setup_multiple_pads(lpuart6_pads, ARRAY_SIZE(lpuart4_pads));
#else
    mx7ulp_iomux_setup_multiple_pads(lpuart4_pads, ARRAY_SIZE(lpuart4_pads));
#endif
}

#ifdef CONFIG_USB_EHCI_MX7

static iomux_cfg_t const usb_otg1_pads[] = {

#ifdef CONFIG_TARGET_MX7ULP_10X10_ARM2
	MX7ULP_PAD_PTC0__PTC0 | MUX_PAD_CTRL(OTG_ID_GPIO_PAD_CTRL),  /* gpio for otgid */
	MX7ULP_PAD_PTC1__PTC1 | MUX_PAD_CTRL(OTG_PWR_GPIO_PAD_CTRL),  /* gpio for power en */
#else
	/*Need rework for ID and PWR_EN pins on 14x14 ARM2*/
	MX7ULP_PAD_PTC18__PTC18 | MUX_PAD_CTRL(OTG_ID_GPIO_PAD_CTRL),  /* gpio for otgid */
	MX7ULP_PAD_PTA31__PTA31 | MUX_PAD_CTRL(OTG_PWR_GPIO_PAD_CTRL),  /* gpio for power en */
#endif
};

#ifdef CONFIG_TARGET_MX7ULP_10X10_ARM2
#define OTG0_ID_GPIO	IMX_GPIO_NR(3, 0)
#define OTG0_PWR_EN		IMX_GPIO_NR(3, 1)
#else
#define OTG0_ID_GPIO	IMX_GPIO_NR(3, 18)
#define OTG0_PWR_EN		IMX_GPIO_NR(1, 31)
#endif
static void setup_usb(void)
{
	mx7ulp_iomux_setup_multiple_pads(usb_otg1_pads,
						 ARRAY_SIZE(usb_otg1_pads));

	gpio_request(OTG0_ID_GPIO, "otg_id");
	gpio_direction_input(OTG0_ID_GPIO);
}

/*Needs to override the ehci power if controlled by GPIO */
int board_ehci_power(int port, int on)
{
	switch (port) {
	case 0:
		if (on)
			gpio_direction_output(OTG0_PWR_EN, 1);
		else
			gpio_direction_output(OTG0_PWR_EN, 0);
		break;
	default:
		printf("MXC USB port %d not yet supported\n", port);
		return -EINVAL;
	}

	return 0;
}

int board_usb_phy_mode(int port)
{
	int ret = 0;

	if (port == 0) {
		ret = gpio_get_value(OTG0_ID_GPIO);

		if (ret)
			return USB_INIT_DEVICE;
		else
			return USB_INIT_HOST;
	}

	return USB_INIT_HOST;
}

#endif


int board_early_init_f(void)
{
	setup_iomux_uart();

	return 0;
}

#ifdef CONFIG_FSL_QSPI
#ifdef CONFIG_TARGET_MX7ULP_10X10_ARM2
static iomux_cfg_t const quadspi_pads[] = {
	MX7ULP_PAD_PTB8__QSPIA_SS0_B | MUX_PAD_CTRL(QSPI_PAD_CTRL0),
	MX7ULP_PAD_PTB14__QSPIA_SS1_B | MUX_PAD_CTRL(QSPI_PAD_CTRL0),
	MX7ULP_PAD_PTB15__QSPIA_SCLK  | MUX_PAD_CTRL(QSPI_PAD_CTRL0),
	MX7ULP_PAD_PTB16__QSPIA_DATA3 | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX7ULP_PAD_PTB17__QSPIA_DATA2 | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX7ULP_PAD_PTB18__QSPIA_DATA1 | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX7ULP_PAD_PTB19__QSPIA_DATA0 | MUX_PAD_CTRL(QSPI_PAD_CTRL1),

	MX7ULP_PAD_PTB5__PTB5 | MUX_PAD_CTRL(GPIO_PAD_CTRL),
};

#define QSPI_RST_GPIO	IMX_GPIO_NR(2, 5)
#else
/* MT35XU512ABA supports 8 bits I/O, since our driver only support 4, so mux 4 data pins*/
static iomux_cfg_t const quadspi_pads[] = {
	MX7ULP_PAD_PTB8__QSPIA_SS0_B | MUX_PAD_CTRL(QSPI_PAD_CTRL0),
	MX7ULP_PAD_PTB9__QSPIA_DQS   | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX7ULP_PAD_PTB15__QSPIA_SCLK  | MUX_PAD_CTRL(QSPI_PAD_CTRL0),
	MX7ULP_PAD_PTB16__QSPIA_DATA3 | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX7ULP_PAD_PTB17__QSPIA_DATA2 | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX7ULP_PAD_PTB18__QSPIA_DATA1 | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX7ULP_PAD_PTB19__QSPIA_DATA0 | MUX_PAD_CTRL(QSPI_PAD_CTRL1),

	MX7ULP_PAD_PTB12__PTB12 | MUX_PAD_CTRL(GPIO_PAD_CTRL),
};

#define QSPI_RST_GPIO	IMX_GPIO_NR(2, 12)

#endif
int board_qspi_init(void)
{
	u32 val;
	mx7ulp_iomux_setup_multiple_pads(quadspi_pads, ARRAY_SIZE(quadspi_pads));
	/* enable clock */
	val = readl(PCC1_RBASE + 0x94);

	if (!(val & 0x20000000)) {
		writel(0x03000003, (PCC1_RBASE + 0x94));
		writel(0x43000003, (PCC1_RBASE + 0x94));
	}

	/* Enable QSPI as a wakeup source on B0 */
	if (soc_rev() >= CHIP_REV_2_0)
		setbits_le32(SIM0_RBASE + WKPU_WAKEUP_EN, WKPU_QSPI_CHANNEL);

	gpio_request(QSPI_RST_GPIO, "qspi_reset");
	gpio_direction_output(QSPI_RST_GPIO, 0);
	mdelay(10);
	gpio_direction_output(QSPI_RST_GPIO, 1);
	return 0;
}
#endif

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

#ifdef CONFIG_USB_EHCI_MX7
	setup_usb();
#endif

#ifdef CONFIG_FSL_QSPI
	board_qspi_init();
#endif

	return 0;
}

#ifndef CONFIG_DM_MMC
static struct fsl_esdhc_cfg usdhc_cfg[2] = {
	{USDHC0_RBASE, 0, 8},
	{USDHC1_RBASE, 0},
};

static iomux_cfg_t const usdhc0_emmc_pads[] = {
	MX7ULP_PAD_PTD0__SDHC0_RESET_b | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
    MX7ULP_PAD_PTD1__SDHC0_CMD | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
    MX7ULP_PAD_PTD2__SDHC0_CLK | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
    MX7ULP_PAD_PTD3__SDHC0_D7 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
    MX7ULP_PAD_PTD4__SDHC0_D6 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
    MX7ULP_PAD_PTD5__SDHC0_D5 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
    MX7ULP_PAD_PTD6__SDHC0_D4 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
    MX7ULP_PAD_PTD7__SDHC0_D3 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
    MX7ULP_PAD_PTD8__SDHC0_D2 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
    MX7ULP_PAD_PTD9__SDHC0_D1 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
    MX7ULP_PAD_PTD10__SDHC0_D0 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
    MX7ULP_PAD_PTD11__SDHC0_DQS | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
};

static iomux_cfg_t const usdhc1_pads[] = {
	MX7ULP_PAD_PTE11__SDHC1_RESET_b | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
    MX7ULP_PAD_PTE3__SDHC1_CMD | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
    MX7ULP_PAD_PTE2__SDHC1_CLK | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
    MX7ULP_PAD_PTE9__SDHC1_D7 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
    MX7ULP_PAD_PTE8__SDHC1_D6 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
    MX7ULP_PAD_PTE7__SDHC1_D5 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
    MX7ULP_PAD_PTE6__SDHC1_D4 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
    MX7ULP_PAD_PTE4__SDHC1_D3 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
    MX7ULP_PAD_PTE5__SDHC1_D2 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
    MX7ULP_PAD_PTE0__SDHC1_D1 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
    MX7ULP_PAD_PTE1__SDHC1_D0 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
    MX7ULP_PAD_PTE10__SDHC1_DQS | MUX_PAD_CTRL(ESDHC_PAD_CTRL),

	MX7ULP_PAD_PTE13__PTE13 | MUX_PAD_CTRL(ESDHC_CD_GPIO_PAD_CTRL),  /*CD*/
};

#define USDHC0_CD_GPIO	IMX_GPIO_NR(5, 13)

int board_mmc_init(bd_t *bis)
{
	int i, ret;
	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-Boot device node)    (Physical Port)
	 * mmc0                    USDHC0
	 * mmc1                    USDHC1
	 */
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			mx7ulp_iomux_setup_multiple_pads(usdhc0_emmc_pads, ARRAY_SIZE(usdhc0_emmc_pads));
			init_clk_usdhc(0);
			usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);

			break;
		case 1:
			mx7ulp_iomux_setup_multiple_pads(usdhc1_pads, ARRAY_SIZE(usdhc1_pads));
			init_clk_usdhc(1);
			usdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);

			gpio_request(USDHC0_CD_GPIO, "usdhc1_cd");
			gpio_direction_input(USDHC0_CD_GPIO);
			break;
		default:
			printf("Warning: you configured more USDHC controllers"
				"(%d) than supported by the board\n", i + 1);
			return -EINVAL;
			}

			ret = fsl_esdhc_initialize(bis, &usdhc_cfg[i]);
			if (ret)
				return ret;
	}

	return 0;
}

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;

	switch (cfg->esdhc_base) {
	case USDHC0_RBASE:
		ret = 1;
		break;
	case USDHC1_RBASE:
		ret = !gpio_get_value(USDHC0_CD_GPIO);
		break;
	}
	return ret;
}
#endif


int board_late_init(void)
{
    return 0;
}

int checkboard(void)
{
#ifdef CONFIG_TARGET_MX7ULP_10X10_ARM2
	printf("Board: i.MX7ULP 10x10 ARM2 board\n");
#else
	printf("Board: i.MX7ULP 14x14 ARM2 board\n");
#endif
	return 0;
}
