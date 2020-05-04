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
#include <usb.h>

DECLARE_GLOBAL_DATA_PTR;

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

#ifdef CONFIG_TARGET_MX7ULP_10X10_VAL
/* PTF11 and PTF10 also can mux to LPUART6 on 10x10 validation, depends on rework*/
static iomux_cfg_t const lpuart6_pads[] = {
    MX7ULP_PAD_PTE11__LPUART6_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
    MX7ULP_PAD_PTE10__LPUART6_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};
#else
static iomux_cfg_t const lpuart4_pads[] = {
    MX7ULP_PAD_PTC3__LPUART4_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
    MX7ULP_PAD_PTC2__LPUART4_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};
#endif

static void setup_iomux_uart(void)
{
#ifdef CONFIG_TARGET_MX7ULP_10X10_VAL
	mx7ulp_iomux_setup_multiple_pads(lpuart6_pads, ARRAY_SIZE(lpuart6_pads));
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

#ifdef CONFIG_TARGET_MX7ULP_10X10_VAL
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
#ifdef CONFIG_TARGET_MX7ULP_10X10_VAL
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

int board_late_init(void)
{
    return 0;
}

int checkboard(void)
{
#ifdef CONFIG_TARGET_MX7ULP_10X10_VAL
	printf("Board: i.MX7ULP 10x10 Validation board\n");
#else
	printf("Board: i.MX7ULP 14x14 Validation board\n");
#endif
	return 0;
}
