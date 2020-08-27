// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2018 NXP
 */

#include <common.h>
#include <cpu_func.h>
#include <env.h>
#include <errno.h>
#include <init.h>
#include <linux/libfdt.h>
#include <fsl_esdhc_imx.h>
#include <fdt_support.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/clock.h>
#include <asm/arch/sci/sci.h>
#include <asm/arch/imx8-pins.h>
#include <asm/arch/snvs_security_sc.h>
#include <asm/arch/iomux.h>
#include <asm/arch/sys_proto.h>
#include <usb.h>
#include "../common/tcpc.h"

DECLARE_GLOBAL_DATA_PTR;

#define ENET_INPUT_PAD_CTRL	((SC_PAD_CONFIG_OD_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_18V_10MA << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define ENET_NORMAL_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_18V_10MA << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define GPIO_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | \
			 (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) | \
			 (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | \
			 (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define UART_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | \
			 (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) | \
			 (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | \
			 (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

static iomux_cfg_t uart0_pads[] = {
	SC_P_UART0_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	SC_P_UART0_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static void setup_iomux_uart(void)
{
	imx8_iomux_setup_multiple_pads(uart0_pads, ARRAY_SIZE(uart0_pads));
}

int board_early_init_f(void)
{
	sc_pm_clock_rate_t rate = SC_80MHZ;
	int ret;

	/* Set UART0 clock root to 80 MHz */
	ret = sc_pm_setup_uart(SC_R_UART_0, rate);
	if (ret)
		return ret;

	setup_iomux_uart();

/* Dual bootloader feature will require CAAM access, but JR0 and JR1 will be
 * assigned to seco for imx8, use JR3 instead.
 */
#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_DUAL_BOOTLOADER)
	sc_pm_set_resource_power_mode(-1, SC_R_CAAM_JR3, SC_PM_PW_MODE_ON);
	sc_pm_set_resource_power_mode(-1, SC_R_CAAM_JR3_OUT, SC_PM_PW_MODE_ON);
#endif

	return 0;
}

#if CONFIG_IS_ENABLED(DM_GPIO)
static void board_gpio_init(void)
{
	struct gpio_desc desc;
	int ret;

	ret = dm_gpio_lookup_name("gpio@1a_3", &desc);
	if (ret)
		return;

	ret = dm_gpio_request(&desc, "bb_per_rst_b");
	if (ret)
		return;

	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT);
	dm_gpio_set_value(&desc, 0);
	udelay(50);
	dm_gpio_set_value(&desc, 1);
}
#else
static inline void board_gpio_init(void) {}
#endif

#if IS_ENABLED(CONFIG_FEC_MXC)
#include <miiphy.h>

#ifndef CONFIG_DM_ETH
static iomux_cfg_t pad_enet1[] = {
	SC_P_SPDIF0_TX | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_SPDIF0_RX | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ESAI0_TX3_RX2 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ESAI0_TX2_RX3 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ESAI0_TX1 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ESAI0_TX0 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ESAI0_SCKR | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ESAI0_TX4_RX1 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ESAI0_TX5_RX0 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ESAI0_FST  | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ESAI0_SCKT | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ESAI0_FSR  | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),

	/* Shared MDIO */
	SC_P_ENET0_MDC | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_MDIO | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
};

static iomux_cfg_t pad_enet0[] = {
	SC_P_ENET0_RGMII_RX_CTL | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET0_RGMII_RXD0 | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET0_RGMII_RXD1 | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET0_RGMII_RXD2 | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET0_RGMII_RXD3 | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET0_RGMII_RXC | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET0_RGMII_TX_CTL | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_RGMII_TXD0 | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_RGMII_TXD1 | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_RGMII_TXD2 | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_RGMII_TXD3 | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_RGMII_TXC | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),

	/* Shared MDIO */
	SC_P_ENET0_MDC | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_MDIO | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
};

static void setup_iomux_fec(void)
{
	if (0 == CONFIG_FEC_ENET_DEV)
		imx8_iomux_setup_multiple_pads(pad_enet0, ARRAY_SIZE(pad_enet0));
	else
		imx8_iomux_setup_multiple_pads(pad_enet1, ARRAY_SIZE(pad_enet1));
}

static void enet_device_phy_reset(void)
{
	struct gpio_desc desc;
	int ret;

	/* The BB_PER_RST_B will reset the ENET1 PHY */
	if (0 == CONFIG_FEC_ENET_DEV) {
		ret = dm_gpio_lookup_name("gpio@1a_4", &desc);
		if (ret)
			return;

		ret = dm_gpio_request(&desc, "enet0_reset");
		if (ret)
			return;

		dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT);
		dm_gpio_set_value(&desc, 0);
		udelay(50);
		dm_gpio_set_value(&desc, 1);
	}

	/* The board has a long delay for this reset to become stable */
	mdelay(200);
}

int board_eth_init(bd_t *bis)
{
	int ret;
	struct power_domain pd;

	printf("[%s] %d\n", __func__, __LINE__);

	/* Reset ENET PHY */
	enet_device_phy_reset();

	if (CONFIG_FEC_ENET_DEV) {
		if (!power_domain_lookup_name("conn_enet1", &pd))
			power_domain_on(&pd);
	} else {
		if (!power_domain_lookup_name("conn_enet0", &pd))
			power_domain_on(&pd);
	}

	setup_iomux_fec();

	ret = fecmxc_initialize_multi(bis, CONFIG_FEC_ENET_DEV,
		CONFIG_FEC_MXC_PHYADDR, IMX_FEC_BASE);
	if (ret)
		printf("FEC1 MXC: %s:failed\n", __func__);

	return ret;
}
#endif

int board_phy_config(struct phy_device *phydev)
{
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x1f);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x8);

	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x00);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x82ee);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x100);

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}
#endif

int checkboard(void)
{
#ifdef CONFIG_TARGET_IMX8DX_MEK
	puts("Board: iMX8DX MEK\n");
#else
	puts("Board: iMX8QXP MEK\n");
#endif

	print_bootinfo();

	return 0;
}

#ifdef CONFIG_USB

#ifdef CONFIG_USB_TCPC
struct gpio_desc type_sel_desc;
static iomux_cfg_t ss_mux_gpio[] = {
	SC_P_ENET0_REFCLK_125M_25M | MUX_MODE_ALT(4) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
};

struct tcpc_port port;
struct tcpc_port_config port_config = {
	.i2c_bus = 1,
	.addr = 0x50,
	.port_type = TYPEC_PORT_DFP,
};

void ss_mux_select(enum typec_cc_polarity pol)
{
	if (pol == TYPEC_POLARITY_CC1)
		dm_gpio_set_value(&type_sel_desc, 0);
	else
		dm_gpio_set_value(&type_sel_desc, 1);
}

static void setup_typec(void)
{
	int ret;
	struct gpio_desc typec_en_desc;

	imx8_iomux_setup_multiple_pads(ss_mux_gpio, ARRAY_SIZE(ss_mux_gpio));
	ret = dm_gpio_lookup_name("GPIO5_9", &type_sel_desc);
	if (ret) {
		printf("%s lookup GPIO5_9 failed ret = %d\n", __func__, ret);
		return;
	}

	ret = dm_gpio_request(&type_sel_desc, "typec_sel");
	if (ret) {
		printf("%s request typec_sel failed ret = %d\n", __func__, ret);
		return;
	}

	dm_gpio_set_dir_flags(&type_sel_desc, GPIOD_IS_OUT);

	ret = dm_gpio_lookup_name("gpio@1a_7", &typec_en_desc);
	if (ret) {
		printf("%s lookup gpio@1a_7 failed ret = %d\n", __func__, ret);
		return;
	}

	ret = dm_gpio_request(&typec_en_desc, "typec_en");
	if (ret) {
		printf("%s request typec_en failed ret = %d\n", __func__, ret);
		return;
	}

	/* Enable SS MUX */
	dm_gpio_set_dir_flags(&typec_en_desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);

	tcpc_init(&port, port_config, &ss_mux_select);
}
#endif

int board_usb_init(int index, enum usb_init_type init)
{
	int ret = 0;

	if (index == 1) {
		if (init == USB_INIT_HOST) {
#ifdef CONFIG_USB_TCPC
			ret = tcpc_setup_dfp_mode(&port);
#endif
#ifdef CONFIG_USB_CDNS3_GADGET
		} else {
#ifdef CONFIG_USB_TCPC
			ret = tcpc_setup_ufp_mode(&port);
			printf("%d setufp mode %d\n", index, ret);
#endif
#endif
		}
	}

	return ret;

}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	int ret = 0;

	if (index == 1) {
		if (init == USB_INIT_HOST) {
#ifdef CONFIG_USB_TCPC
			ret = tcpc_disable_src_vbus(&port);
#endif
		}
	}

	return ret;
}
#endif

int board_init(void)
{
	board_gpio_init();

#if defined(CONFIG_USB) && defined(CONFIG_USB_TCPC)
	setup_typec();
#endif

#ifdef CONFIG_SNVS_SEC_SC_AUTO
	{
		int ret = snvs_security_sc_init();

		if (ret)
			return ret;
	}
#endif

	return 0;
}

void board_quiesce_devices(void)
{
	const char *power_on_devices[] = {
		"dma_lpuart0",

		/* HIFI DSP boot */
		"audio_sai0",
		"audio_ocram",
	};

	power_off_pd_devices(power_on_devices, ARRAY_SIZE(power_on_devices));
}

/*
 * Board specific reset that is system reset.
 */
void reset_cpu(ulong addr)
{
	sc_pm_reboot(-1, SC_PM_RESET_TYPE_COLD);
	while(1);

}

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, bd_t *bd)
{
	return 0;
}
#endif

int board_late_init(void)
{
	char *fdt_file;
	bool m4_boot;

#ifndef CONFIG_ANDROID_AUTO_SUPPORT
	build_info();
#endif

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	env_set("board_name", "MEK");
#ifdef CONFIG_TARGET_IMX8DX_MEK
	env_set("board_rev", "iMX8DX");
#else
	env_set("board_rev", "iMX8QXP");
#endif
#endif

	env_set("sec_boot", "no");
#ifdef CONFIG_AHAB_BOOT
	env_set("sec_boot", "yes");
#endif

	fdt_file = env_get("fdt_file");
	m4_boot = check_m4_parts_boot();

	if (fdt_file && !strcmp(fdt_file, "undefined")) {
#ifdef CONFIG_TARGET_IMX8DX_MEK
		if (m4_boot)
			env_set("fdt_file", "imx8dx-mek-rpmsg.dtb");
		else
			env_set("fdt_file", "imx8dx-mek.dtb");
#else
		if (m4_boot)
			env_set("fdt_file", "imx8qxp-mek-rpmsg.dtb");
		else
			env_set("fdt_file", "imx8qxp-mek.dtb");
#endif
	}

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

#ifdef CONFIG_ANDROID_SUPPORT
bool is_power_key_pressed(void) {
	sc_bool_t status = SC_FALSE;

	sc_misc_get_button_status(-1, &status);
	return (bool)status;
}
#endif
