// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2024 NXP
 */

#include <common.h>
#include <env.h>
#include <init.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/global_data.h>
#include <asm/arch-imx9/ccm_regs.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch-imx9/imx91_pins.h>
#include <asm/arch/clock.h>
#include <power/pmic.h>
#include "../common/tcpc.h"
#include <dm/device.h>
#include <dm/uclass.h>
#include <usb.h>
#include <dwc3-uboot.h>
#include <asm/gpio.h>

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL	(PAD_CTL_DSE(6) | PAD_CTL_FSEL2)
#define LCDIF_GPIO_PAD_CTRL	(PAD_CTL_DSE(0xf) | PAD_CTL_FSEL2 | PAD_CTL_PUE)

static iomux_v3_cfg_t const uart_pads[] = {
	MX91_PAD_UART1_RXD__LPUART1_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX91_PAD_UART1_TXD__LPUART1_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const lcdif_gpio_pads[] = {
	MX91_PAD_GPIO_IO00__GPIO2_IO0| MUX_PAD_CTRL(LCDIF_GPIO_PAD_CTRL),
	MX91_PAD_GPIO_IO01__GPIO2_IO1 | MUX_PAD_CTRL(LCDIF_GPIO_PAD_CTRL),
	MX91_PAD_GPIO_IO02__GPIO2_IO2 | MUX_PAD_CTRL(LCDIF_GPIO_PAD_CTRL),
	MX91_PAD_GPIO_IO03__GPIO2_IO3 | MUX_PAD_CTRL(LCDIF_GPIO_PAD_CTRL),
};

int board_early_init_f(void)
{
	imx_iomux_v3_setup_multiple_pads(uart_pads, ARRAY_SIZE(uart_pads));
	imx_iomux_v3_setup_multiple_pads(lcdif_gpio_pads, ARRAY_SIZE(lcdif_gpio_pads));

	/* Workaround LCD panel leakage, output low of CLK/DE/VSYNC/HSYNC as early as possible */
	struct gpio_regs *gpio2 = (struct gpio_regs *)(GPIO2_BASE_ADDR + 0x40);
	setbits_le32(&gpio2->gpio_pcor, 0xf);
	setbits_le32(&gpio2->gpio_pddr, 0xf);

	/* Set GPIO1_7 to output high to disable panel backlight at default */
	struct gpio_regs *gpio1 = (struct gpio_regs *)(GPIO1_BASE_ADDR + 0x40);
	setbits_le32(&gpio1->gpio_psor, BIT(7));
	setbits_le32(&gpio1->gpio_pddr, BIT(7));

	init_uart_clk(LPUART1_CLK_ROOT);

	return 0;
}

#ifdef CONFIG_USB_TCPC
struct tcpc_port port1;
struct tcpc_port portpd;

static int setup_pd_switch(uint8_t i2c_bus, uint8_t addr)
{
	struct udevice *bus;
	struct udevice *i2c_dev = NULL;
	int ret;
	uint8_t valb;

	ret = uclass_get_device_by_seq(UCLASS_I2C, i2c_bus, &bus);
	if (ret) {
		printf("%s: Can't find bus\n", __func__);
		return -EINVAL;
	}

	ret = dm_i2c_probe(bus, addr, 0, &i2c_dev);
	if (ret) {
		printf("%s: Can't find device id=0x%x\n",
			__func__, addr);
		return -ENODEV;
	}

	ret = dm_i2c_read(i2c_dev, 0xB, &valb, 1);
	if (ret) {
		printf("%s dm_i2c_read failed, err %d\n", __func__, ret);
		return -EIO;
	}
	valb |= 0x4; /* Set DB_EXIT to exit dead battery mode */
	ret = dm_i2c_write(i2c_dev, 0xB, (const uint8_t *)&valb, 1);
	if (ret) {
		printf("%s dm_i2c_write failed, err %d\n", __func__, ret);
		return -EIO;
	}

	/* Set OVP threshold to 23V */
	valb = 0x6;
	ret = dm_i2c_write(i2c_dev, 0x8, (const uint8_t *)&valb, 1);
	if (ret) {
		printf("%s dm_i2c_write failed, err %d\n", __func__, ret);
		return -EIO;
	}

	return 0;
}

int pd_switch_snk_enable(struct tcpc_port *port)
{
	if (port == &port1) {
		debug("Setup pd switch on port 1\n");
		return setup_pd_switch(0, 0x71);
	} else
		return -EINVAL;
}

struct tcpc_port_config portpd_config = {
	.i2c_bus = 0, /*i2c1*/
	.addr = 0x52,
	.port_type = TYPEC_PORT_UFP,
	.max_snk_mv = 20000,
	.max_snk_ma = 3000,
	.max_snk_mw = 15000,
	.op_snk_mv = 9000,
};

struct tcpc_port_config port1_config = {
	.i2c_bus = 0, /*i2c1*/
	.addr = 0x50,
	.port_type = TYPEC_PORT_UFP,
	.max_snk_mv = 5000,
	.max_snk_ma = 3000,
	.max_snk_mw = 40000,
	.op_snk_mv = 9000,
	.switch_setup_func = &pd_switch_snk_enable,
	.disable_pd = true,
};

static int setup_typec(void)
{
	int ret;

	debug("tcpc_init port pd\n");
	ret = tcpc_init(&portpd, portpd_config, NULL);
	if (ret) {
		printf("%s: tcpc portpd init failed, err=%d\n",
		       __func__, ret);
	}

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

	port_ptr = &port1;

	if (init == USB_INIT_HOST)
		tcpc_setup_dfp_mode(port_ptr);
	else
		tcpc_setup_ufp_mode(port_ptr);

	return ret;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	int ret = 0;

	debug("board_usb_cleanup %d, type %d\n", index, init);

	if (init == USB_INIT_HOST)
		ret = tcpc_disable_src_vbus(&port1);

	return ret;
}

int board_ehci_usb_phy_mode(struct udevice *dev)
{
	int ret = 0;
	enum typec_cc_polarity pol;
	enum typec_cc_state state;
	struct tcpc_port *port_ptr;

	debug("%s %d\n", __func__, dev_seq(dev));

	port_ptr = &port1;

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

int board_phy_config(struct phy_device *phydev)
{
	if (phydev->drv->config)
		phydev->drv->config(phydev);

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
	env_set("board_name", "(9X9_QSB");
	env_set("board_rev", "iMX91");
#endif
	return 0;
}

#ifndef CONFIG_SPL_BUILD

struct gpio_desc ext5v_en_desc, sai3_en_desc;

static int last_stage_init(void)
{
	int ret;
	struct gpio_desc desc;

	/* Turn on the backlight later than video starting */
	/* SAI3_TX/RX_SEL to L */
	ret = dm_gpio_lookup_name("gpio@22_14", &desc);
	if (ret)
		return -ENODEV;

	ret = dm_gpio_request(&desc, "SAI3_TXRX_SEL");
	if (ret)
		return -ENODEV;

	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT);
	dm_gpio_set_value(&desc, 0);

	/* Enable SAI3_EN for mux */
	ret = dm_gpio_lookup_name("gpio@22_23", &sai3_en_desc);
	if (ret)
		return -ENODEV;

	ret = dm_gpio_request(&sai3_en_desc, "SAI3_EN");
	if (ret)
		return -ENODEV;

	dm_gpio_set_dir_flags(&sai3_en_desc, GPIOD_IS_OUT);
	dm_gpio_set_value(&sai3_en_desc, 0);

	/* Enable EXT5V_EN for vRPi 5V */
	ret = dm_gpio_lookup_name("gpio@22_4", &ext5v_en_desc);
	if (ret)
		return -ENODEV;

	ret = dm_gpio_request(&ext5v_en_desc, "EXT5V_EN");
	if (ret)
		return -ENODEV;

	dm_gpio_set_dir_flags(&ext5v_en_desc, GPIOD_IS_OUT);
	dm_gpio_set_value(&ext5v_en_desc, 1);

	return 0;
}
EVENT_SPY_SIMPLE(EVT_LAST_STAGE_INIT, last_stage_init);

void board_quiesce_devices(void)
{
	/* Turn off 5V for backlight */
	dm_gpio_set_value(&ext5v_en_desc, 0);

	/* Turn off MUX for SAI3_TX_SYNC used by backlight */
	dm_gpio_set_value(&sai3_en_desc, 1);
}
#endif
