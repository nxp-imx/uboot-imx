// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2024 NXP
 */

#include <env.h>
#include <init.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/arch-imx9/ccm_regs.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch-imx9/imx91_pins.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/clock.h>
#include <i2c.h>
#include "../common/tcpc.h"
#include <usb.h>
#include <dwc3-uboot.h>
#include <asm/gpio.h>

#define UART_PAD_CTRL	(PAD_CTL_DSE(6) | PAD_CTL_FSEL2)

static iomux_v3_cfg_t const uart_pads[] = {
	MX91_PAD_UART1_RXD__LPUART1_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX91_PAD_UART1_TXD__LPUART1_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

int board_early_init_f(void)
{
	imx_iomux_v3_setup_multiple_pads(uart_pads, ARRAY_SIZE(uart_pads));

	init_uart_clk(LPUART1_CLK_ROOT);

	return 0;
}


static void board_gpio_init(void)
{
	struct gpio_desc desc;
	int ret;

	/* Enable EXT1_PWREN for PCIE_3.3V */
	ret = dm_gpio_lookup_name("gpio@22_13", &desc);
	if (ret)
		return;

	ret = dm_gpio_request(&desc, "EXT1_PWREN");
	if (ret)
		return;

	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT);
	dm_gpio_set_value(&desc, 1);

	/* Deassert SD3_nRST */
	ret = dm_gpio_lookup_name("gpio@22_12", &desc);
	if (ret)
		return;

	ret = dm_gpio_request(&desc, "SD3_nRST");
	if (ret)
		return;

	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT);
	dm_gpio_set_value(&desc, 1);
}

int board_init(void)
{
	board_gpio_init();

	return 0;
}

int board_late_init(void)
{
#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

	env_set("sec_boot", "no");
#ifdef CONFIG_AHAB_BOOT
	env_set("sec_boot", "yes");
#endif

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	env_set("board_name", "9X9_flux");
	env_set("board_rev", "iMX91");
#endif
	return 0;
}


/*
 * USB: ChipIdea controller, USB 2.0 device mode via USB-A port.
 * No Type-C/TCPC - fixed peripheral mode.
 */
int board_usb_init(int index, enum usb_init_type init)
{
	debug("board_usb_init %d, type %d (ChipIdea)\n", index, init);
	
	/* ChipIdea USB controller is initialized by driver/DT */
	/* Add any board-specific USB power/PHY setup here if needed */
	
	return 0;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	debug("board_usb_cleanup %d, type %d\n", index, init);
	return 0;
}

int board_ehci_usb_phy_mode(struct udevice *dev)
{
	/* Fixed device mode for USB-A gadget port */
	return USB_INIT_DEVICE;
}