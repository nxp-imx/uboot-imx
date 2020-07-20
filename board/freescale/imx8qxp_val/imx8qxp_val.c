/*
 * Copyright 2017-2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <netdev.h>
#include <fsl_ifc.h>
#include <fdt_support.h>
#include <linux/libfdt.h>
#include <cpu_func.h>
#include <env.h>
#include <fsl_esdhc.h>
#include <i2c.h>
#include "pca953x.h"

#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/clock.h>
#include <asm/arch/sci/sci.h>
#include <asm/arch/imx8-pins.h>
#include <asm/arch/snvs_security_sc.h>
#include <dm.h>
#include <imx8_hsio.h>
#include <usb.h>
#include <asm/arch/iomux.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/dma.h>
#include <power-domain.h>
#include <asm/arch/lpcg.h>
#include <bootm.h>

DECLARE_GLOBAL_DATA_PTR;

#define ESDHC_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define GPMI_NAND_PAD_CTRL	 ((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) \
				  | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define ESDHC_CLK_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))


#define ENET_INPUT_PAD_CTRL	((SC_PAD_CONFIG_OD_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_18V_10MA << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define ENET_NORMAL_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_18V_10MA << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))


#define GPIO_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define I2C_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_LOW << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define UART_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#ifdef CONFIG_SPL_BUILD
#ifdef CONFIG_NAND_MXS
static iomux_cfg_t gpmi_nand_pads[] = {
	SC_P_EMMC0_CLK | MUX_MODE_ALT(1) | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_EMMC0_DATA0 | MUX_MODE_ALT(1) | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_EMMC0_DATA1 | MUX_MODE_ALT(1) | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_EMMC0_DATA2 | MUX_MODE_ALT(1) | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_EMMC0_DATA3 | MUX_MODE_ALT(1) | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_EMMC0_DATA4 | MUX_MODE_ALT(1) | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_EMMC0_DATA5 | MUX_MODE_ALT(1) | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_EMMC0_DATA6 | MUX_MODE_ALT(1) | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_EMMC0_DATA7 | MUX_MODE_ALT(1) | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_EMMC0_STROBE | MUX_MODE_ALT(1) | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_EMMC0_RESET_B | MUX_MODE_ALT(1) | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_USDHC1_CMD | MUX_MODE_ALT(1) | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_USDHC1_DATA2 | MUX_MODE_ALT(1) | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_USDHC1_DATA3 | MUX_MODE_ALT(1) | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_USDHC1_DATA0 | MUX_MODE_ALT(1) | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),

	/* i.MX8QXP NAND use nand_re_dqs_pins */
	SC_P_USDHC1_CD_B | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),
	SC_P_USDHC1_VSELECT | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPMI_NAND_PAD_CTRL),

};

static void setup_iomux_gpmi_nand(void)
{
	imx8_iomux_setup_multiple_pads(gpmi_nand_pads, ARRAY_SIZE(gpmi_nand_pads));
}

static void imx8qxp_gpmi_nand_initialize(void)
{
	int ret;

	ret = sc_pm_set_resource_power_mode(-1, SC_R_NAND, SC_PM_PW_MODE_ON);
	if (ret != SC_ERR_NONE)
		return;

	init_clk_gpmi_nand();
	setup_iomux_gpmi_nand();
}
#endif
#endif

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

#ifdef CONFIG_SPL_BUILD
#ifdef CONFIG_NAND_MXS
	imx8qxp_gpmi_nand_initialize();
#endif
#endif

	return 0;
}

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
	struct gpio_desc desc_enet0;
	struct gpio_desc desc_enet1;
	int ret;

	ret = dm_gpio_lookup_name("gpio@18_1", &desc_enet0);
	if (ret)
		return;

	ret = dm_gpio_request(&desc_enet0, "enet0_reset");
	if (ret)
		return;

	ret = dm_gpio_lookup_name("gpio@18_4", &desc_enet1);
	if (ret)
		return;

	ret = dm_gpio_request(&desc_enet1, "enet1_reset");
	if (ret)
		return;

	dm_gpio_set_dir_flags(&desc_enet0, GPIOD_IS_OUT);
	dm_gpio_set_value(&desc_enet0, 0);
	udelay(50);
	dm_gpio_set_value(&desc_enet0, 1);

	dm_gpio_set_dir_flags(&desc_enet1, GPIOD_IS_OUT);
	dm_gpio_set_value(&desc_enet1, 0);
	udelay(50);
	dm_gpio_set_value(&desc_enet1, 1);

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

#define MAX7322_I2C_ADDR		0x68
#define MAX7322_I2C_BUS		0 /* I2C1 */
int board_phy_config(struct phy_device *phydev)
{
	if (phydev->addr == 1) {
		/* This is needed to drive the pads to 1.8V instead of 1.5V */
		uint8_t value;
		struct udevice *bus;
		struct udevice *i2c_dev = NULL;
		int ret;

		ret = uclass_get_device_by_seq(UCLASS_I2C, MAX7322_I2C_BUS, &bus);
		if (ret) {
			printf("%s: Can't find bus\n", __func__);
			return -EINVAL;
		}

		ret = dm_i2c_probe(bus, MAX7322_I2C_ADDR, 0, &i2c_dev);
		if (ret) {
			printf("%s: Can't find device id=0x%x\n",
				__func__, MAX7322_I2C_ADDR);
			return -ENODEV;
		}

		i2c_set_chip_offset_len(i2c_dev, 0);

		value = 0x1;

		ret = dm_i2c_write(i2c_dev, 0x0, (const uint8_t *)&value, 1);
		if (ret) {
			printf("%s dm_i2c_write failed, err %d\n", __func__, ret);
			return -EIO;
		}

		mdelay(1);
	}

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

#define DEBUG_LED   IMX_GPIO_NR(3, 23)
#define IOEXP_RESET IMX_GPIO_NR(0, 19)
#define BB_PWR_EN IMX_GPIO_NR(5, 9)

static iomux_cfg_t board_gpios[] = {
	SC_P_QSPI0B_SS0_B | MUX_MODE_ALT(4) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
	SC_P_MCLK_IN0 | MUX_MODE_ALT(4) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
	SC_P_ENET0_REFCLK_125M_25M |  MUX_MODE_ALT(4) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
};

static void board_gpio_init(void)
{
	int ret;
	struct gpio_desc desc;
	struct udevice *dev;

	imx8_iomux_setup_multiple_pads(board_gpios, ARRAY_SIZE(board_gpios));
	
	/* enable i2c port expander assert reset line first */
	/* we can't use dm_gpio_lookup_name for GPIO1_12, because the func will probe the
	 * uclass list until find the device. The expander device is at begin of the list due to
	 * I2c nodes is prior than gpio in the DTS. So if the func goes through the uclass list,
	 * probe to expander will fail, and exit the dm_gpio_lookup_name func. Thus, we always
	 * fail to get the device
	*/
	ret = uclass_get_device_by_seq(UCLASS_GPIO, 0, &dev);
	if (ret) {
		printf("%s failed to find GPIO1 device, ret = %d\n", __func__, ret);
		return;
	}

	desc.dev = dev;
	desc.offset = 19;

	ret = dm_gpio_request(&desc, "ioexp_rst");
	if (ret) {
		printf("%s request ioexp_rst failed ret = %d\n", __func__, ret);
		return;
	}

	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);
	
	ret = dm_gpio_lookup_name("GPIO3_23", &desc);
	if (ret) {
		printf("%s lookup GPIO@3_23 failed ret = %d\n", __func__, ret);
		return;
	}

	ret = dm_gpio_request(&desc, "debug_led");
	if (ret) {
		printf("%s request debug_led failed ret = %d\n", __func__, ret);
		return;
	}

	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);
	
	ret = dm_gpio_lookup_name("GPIO5_9", &desc);
	if (ret) {
		printf("%s lookup GPIO@5_9 failed ret = %d\n", __func__, ret);
		return;
	}

	ret = dm_gpio_request(&desc, "bb_pwr_en");
	if (ret) {
		printf("%s request bb_pwr_en failed ret = %d\n", __func__, ret);
		return;
	}

	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);
}

int checkboard(void)
{
#if defined(CONFIG_TARGET_IMX8QXP_DDR3_VAL)
	puts("Board: iMX8QXP DDR3 VAL\n");
#elif defined(CONFIG_TARGET_IMX8X_17X17_VAL)
	puts("Board: iMX8X(QXP/DX) 17x17 Validation Board\n");
#else
	puts("Board: iMX8QXP LPDDR4 VAL\n");
#endif

	print_bootinfo();

	return 0;
}

#ifdef CONFIG_FSL_HSIO

#define PCIE_PAD_CTRL	((SC_PAD_CONFIG_OD_IN << PADRING_CONFIG_SHIFT))
static iomux_cfg_t board_pcie_pins[] = {
	SC_P_PCIE_CTRL0_CLKREQ_B | MUX_MODE_ALT(0) | MUX_PAD_CTRL(PCIE_PAD_CTRL),
	SC_P_PCIE_CTRL0_WAKE_B | MUX_MODE_ALT(0) | MUX_PAD_CTRL(PCIE_PAD_CTRL),
	SC_P_PCIE_CTRL0_PERST_B | MUX_MODE_ALT(0) | MUX_PAD_CTRL(PCIE_PAD_CTRL),
};

static void imx8qxp_hsio_initialize(void)
{
	struct power_domain pd;
	int ret;

	if (!power_domain_lookup_name("hsio_pcie1", &pd)) {
		ret = power_domain_on(&pd);
		if (ret)
			printf("hsio_pcie1 Power up failed! (error = %d)\n", ret);
	}
        if (!power_domain_lookup_name("hsio_gpio", &pd)) {
		ret = power_domain_on(&pd);
		if (ret)
			printf("hsio_gpio Power up failed! (error = %d)\n", ret);
	}

	lpcg_all_clock_on(HSIO_PCIE_X1_LPCG);
	lpcg_all_clock_on(HSIO_PHY_X1_LPCG);
	lpcg_all_clock_on(HSIO_PHY_X1_CRR1_LPCG);
	lpcg_all_clock_on(HSIO_PCIE_X1_CRR3_LPCG);
	lpcg_all_clock_on(HSIO_MISC_LPCG);
	lpcg_all_clock_on(HSIO_GPIO_LPCG);

	imx8_iomux_setup_multiple_pads(board_pcie_pins, ARRAY_SIZE(board_pcie_pins));
}

void pci_init_board(void)
{
	imx8qxp_hsio_initialize();

	/* test the 1 lane mode of the PCIe A controller */
	mx8qxp_pcie_init();
}

#endif

int board_usb_init(int index, enum usb_init_type init)
{
	int ret = 0;

	if (index == 0) {
		if (init == USB_INIT_DEVICE) {
#if !CONFIG_IS_ENABLED(DM_USB_GADGET) && !CONFIG_IS_ENABLED(DM_USB)
			ret = sc_pm_set_resource_power_mode(-1, SC_R_USB_0, SC_PM_PW_MODE_ON);
			if (ret != SC_ERR_NONE)
				printf("conn_usb0 Power up failed! (error = %d)\n", ret);

			ret = sc_pm_set_resource_power_mode(-1, SC_R_USB_0_PHY, SC_PM_PW_MODE_ON);
			if (ret != SC_ERR_NONE)
				printf("conn_usb0_phy Power up failed! (error = %d)\n", ret);
#endif
		}
	}
	return ret;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	int ret = 0;

	if (index == 0) {
		if (init == USB_INIT_DEVICE) {
#if !CONFIG_IS_ENABLED(DM_USB_GADGET) && !CONFIG_IS_ENABLED(DM_USB)
			ret = sc_pm_set_resource_power_mode(-1, SC_R_USB_0, SC_PM_PW_MODE_OFF);
			if (ret != SC_ERR_NONE)
				printf("conn_usb0 Power down failed! (error = %d)\n", ret);

			ret = sc_pm_set_resource_power_mode(-1, SC_R_USB_0_PHY, SC_PM_PW_MODE_OFF);
			if (ret != SC_ERR_NONE)
				printf("conn_usb0_phy Power down failed! (error = %d)\n", ret);
#endif
		}
	}
	return ret;
}

int board_init(void)
{
	board_gpio_init();

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
	/* TODO */
}

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, bd_t *bd)
{
	return 0;
}
#endif


int board_late_init(void)
{
	build_info();

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	env_set("board_name", "VAL");
	env_set("board_rev", "iMX8QXP");
#endif

	env_set("sec_boot", "no");
#ifdef CONFIG_AHAB_BOOT
	env_set("sec_boot", "yes");
#endif

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

	return 0;
}
