// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2025 NXP
 */

#include <env.h>
#include <efi_loader.h>
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
#include <linux/delay.h>
#include <adc.h>

#define UART_PAD_CTRL	(PAD_CTL_DSE(6) | PAD_CTL_FSEL2)
#define LCDIF_GPIO_PAD_CTRL	(PAD_CTL_DSE(0xf) | PAD_CTL_FSEL2 | PAD_CTL_PUE)

static const iomux_v3_cfg_t uart_pads[] = {
	MX91_PAD_UART1_RXD__LPUART1_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX91_PAD_UART1_TXD__LPUART1_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static const iomux_v3_cfg_t lcdif_gpio_pads[] = {
	MX91_PAD_GPIO_IO00__GPIO2_IO0 | MUX_PAD_CTRL(LCDIF_GPIO_PAD_CTRL),
	MX91_PAD_GPIO_IO01__GPIO2_IO1 | MUX_PAD_CTRL(LCDIF_GPIO_PAD_CTRL),
	MX91_PAD_GPIO_IO02__GPIO2_IO2 | MUX_PAD_CTRL(LCDIF_GPIO_PAD_CTRL),
	MX91_PAD_GPIO_IO03__GPIO2_IO3 | MUX_PAD_CTRL(LCDIF_GPIO_PAD_CTRL),
};

#if CONFIG_IS_ENABLED(EFI_HAVE_CAPSULE_SUPPORT)
#define IMX_BOOT_IMAGE_GUID \
	EFI_GUID(0xbc550d86, 0xda26, 0x4b70, 0xac, 0x05, \
		 0x2a, 0x44, 0x8e, 0xda, 0x6f, 0x21)

struct efi_fw_image fw_images[] = {
	{
		.image_type_id = IMX_BOOT_IMAGE_GUID,
		.fw_name = u"IMX91-11X11-FRDM-IMX91S-RAW",
		.image_index = 1,
	},
};

struct efi_capsule_update_info update_info = {
	.dfu_string = "mmc 0=flash-bin raw 0 0x2000 mmcpart 1",
	.num_images = ARRAY_SIZE(fw_images),
	.images = fw_images,
};

#endif /* EFI_HAVE_CAPSULE_SUPPORT */

int board_early_init_f(void)
{
	imx_iomux_v3_setup_multiple_pads(uart_pads, ARRAY_SIZE(uart_pads));
	imx_iomux_v3_setup_multiple_pads(lcdif_gpio_pads, ARRAY_SIZE(lcdif_gpio_pads));

	/* Workaround LCD panel leakage, output low of CLK/DE/VSYNC/HSYNC as early as possible */
	struct gpio_regs *gpio2 = (struct gpio_regs *)(GPIO2_BASE_ADDR + 0x40);

	setbits_le32(&gpio2->gpio_pcor, 0xf);
	setbits_le32(&gpio2->gpio_pddr, 0xf);
	/* Set GPIO2_26 to output high to disable panel backlight at default */
	setbits_le32(&gpio2->gpio_psor, BIT(26));
	setbits_le32(&gpio2->gpio_pddr, BIT(26));

	init_uart_clk(LPUART1_CLK_ROOT);

	return 0;
}

#ifdef CONFIG_USB_TCPC
struct tcpc_port port1;

struct tcpc_port_config port1_config = {
	.i2c_bus = 1, /*i2c2*/
	.addr = 0x52,
	.port_type = TYPEC_PORT_UFP,
	.max_snk_mv = 5000,
	.max_snk_ma = 3000,
	.max_snk_mw = 40000,
	.op_snk_mv = 9000,
	.disable_pd = true,
};

static int setup_typec(void)
{
	int ret;

	debug("tcpc_init port 1\n");
	ret = tcpc_init(&port1, port1_config, NULL);
	if (ret) {
		printf("%s: tcpc port1 init failed, err=%d\n",
		       __func__, ret);
	}

	return ret;
}

int board_usb_init(int index, enum usb_init_type init)
{
	int ret = 0;
	struct tcpc_port *port_ptr;

	debug("board_usb_init %d, type %d\n", index, init);

	if (index == 0) {
		port_ptr = &port1;
	} else {
		debug("only 1 usb on FRDM-IMX91S");
		return 0;
	}

	if (init == USB_INIT_HOST)
		tcpc_setup_dfp_mode(port_ptr);
	else
		tcpc_setup_ufp_mode(port_ptr);

	return ret;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	int ret = 0;

	debug("%s %d, type %d\n", __func__, index, init);

	if (init == USB_INIT_HOST) {
		if (index == 0) {
			ret = tcpc_disable_src_vbus(&port1);
		} else {
			debug("only 1 usb on FRDM-IMX91S");
			return 0;
		}
	}

	return ret;
}

int board_ehci_usb_phy_mode(struct udevice *dev)
{
	int ret = 0;
	enum typec_cc_polarity pol;
	enum typec_cc_state state;
	struct tcpc_port *port_ptr;

	debug("%s %d\n", __func__, dev_seq(dev));

	if (dev_seq(dev) == 0) {
		port_ptr = &port1;
	} else {
		debug("only 1 usb on FRDM-IMX91S");
		return 0;
	}

	tcpc_setup_ufp_mode(port_ptr);

	ret = tcpc_get_cc_status(port_ptr, &pol, &state);

	tcpc_print_log(port_ptr);
	if (!ret) {
		if (state == TYPEC_STATE_SRC_RD_RA || state == TYPEC_STATE_SRC_RD)
			return USB_INIT_HOST;
	}

	return USB_INIT_DEVICE;
}
#endif

struct gpio_desc ext_pwren_desc;

static void board_gpio_init(void)
{
	struct gpio_desc desc;
	int ret;

	/* Reset ENET_QOS PHY */
	ret = dm_gpio_lookup_name("GPIO3_17", &desc);
	if (ret)
		return;
	ret = dm_gpio_request(&desc, "eth0_rst");
	if (ret)
		return;
	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT);
	dm_gpio_set_value(&desc, 1);
	udelay(10000);
	dm_gpio_set_value(&desc, 0);

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

	/* Enable EXT_PWREN for vEXP 5V */
	ret = dm_gpio_lookup_name("gpio@22_8", &ext_pwren_desc);
	if (ret)
		return;

	ret = dm_gpio_request(&ext_pwren_desc, "EXT_PWREN");
	if (ret)
		return;

	dm_gpio_set_dir_flags(&ext_pwren_desc, GPIOD_IS_OUT);
	dm_gpio_set_value(&ext_pwren_desc, 1);
}

static int print_board_version(void)
{
	int i, ret;
	struct udevice *dev;
	unsigned int rev[2];
	unsigned int data[2];

	ret = uclass_first_device_check(UCLASS_ADC, &dev);

	if (dev) {
		ret = adc_channel_single_shot(dev->name, 2, &data[0]);
		if (ret) {
			printf("BOARD: unknown\n");
			return 0;
		}
		ret = adc_channel_single_shot(dev->name, 3, &data[1]);
		if (ret) {
			printf("BOARD: unknown\n");
			return 0;
		}

		for (i = 0; i < 2; i++) {
			if (data[i] < 500)
				rev[i] = 0;
			else if (data[i] < 700)
				rev[i] = 1;
			else if (data[i] < 1500)
				rev[i] = 2;
			else if (data[i] < 2300)
				rev[i] = 3;
			else if (data[i] < 3000)
				rev[i] = 4;
			else if (data[i] < 3600)
				rev[i] = 5;
			else
				rev[i] = 6;
		}
		printf("BOARD: V%d.%d(ADC2:%d,ADC3:%d)\n", rev[0], rev[1], data[0], data[1]);
	} else {
		printf("BOARD: unknown\n");
	}

	return 0;
}

int board_init(void)
{
	print_board_version();

#ifdef CONFIG_USB_TCPC
	setup_typec();
#endif

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
	env_set("board_name", "11X11_FRDM_IMX91S");
	env_set("board_rev", "iMX93");
#endif
	return 0;
}

void board_quiesce_devices(void)
{
	/* Turn off 5V for backlight */
	dm_gpio_set_value(&ext_pwren_desc, 0);
}
