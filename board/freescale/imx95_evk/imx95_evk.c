// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2025 NXP
 */

#include <env.h>
#include <efi_loader.h>
#include <init.h>
#include <fdt_support.h>
#include <asm/arch/clock.h>
#include <usb.h>
#include "../common/tcpc.h"
#include <dwc3-uboot.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <asm/gpio.h>
#include <power/regulator.h>
#include <scmi_agent.h>
#include "../dts/upstream/src/arm64/freescale/imx95-power.h"
#include <asm/arch/sys_proto.h>
#include <i2c.h>
#include <dm/uclass.h>
#include <dm/uclass-internal.h>

extern int board_fix_fdt_fuse(void *fdt);

#if CONFIG_IS_ENABLED(EFI_HAVE_CAPSULE_SUPPORT)
#define IMX_BOOT_IMAGE_GUID \
	EFI_GUID(0x2c4db6b3, 0x0b15, 0x4a36, 0xbe, 0xae, \
		 0x1e, 0xa1, 0x35, 0x46, 0x4f, 0x5b)

struct efi_fw_image fw_images[] = {
	{
		.image_type_id = IMX_BOOT_IMAGE_GUID,
		.fw_name = u"IMX95-EVK-RAW",
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
	/* UART1: A55, UART2: M33, UART3: M7 */
	init_uart_clk(0);

	return 0;
}

#ifdef CONFIG_USB_TCPC
struct tcpc_port port;
#ifdef CONFIG_TARGET_IMX95_15X15_EVK
struct tcpc_port portpd;
struct tcpc_port_config port_config = {
	.i2c_bus = 2, /* i2c3 */
	.addr = 0x50,
	.port_type = TYPEC_PORT_DRP,
	.disable_pd = true,
};

struct tcpc_port_config portpd_config = {
	.i2c_bus = 2, /*i2c3*/
	.addr = 0x52,
	.port_type = TYPEC_PORT_UFP,
	.max_snk_mv = 20000,
	.max_snk_ma = 3000,
	.max_snk_mw = 15000,
	.op_snk_mv = 9000,
};
#else
struct tcpc_port_config port_config = {
	.i2c_bus = 6, /* i2c7 */
	.addr = 0x50,
	.port_type = TYPEC_PORT_DRP,
	.disable_pd = true,
};
#endif

ulong tca_base;

void tca_mux_select(enum typec_cc_polarity pol)
{
	u32 val;

	if (!tca_base)
		return;

	/* reset XBar block */
	setbits_le32(tca_base, BIT(9));

	/* Set OP mode to System configure Mode */
	clrbits_le32(tca_base + 0x10, 0x3);

	val = readl(tca_base + 0x30);

	WARN_ON((val & GENMASK(1, 0)) != 0x3);
	WARN_ON((val & BIT(2)) != 0);
	WARN_ON((val & BIT(3)) != 0);
	WARN_ON((val & BIT(4)) != 0);

	printf("tca pstate 0x%x\n", val);

	setbits_le32(tca_base + 0x18, BIT(3));
	udelay(1);

	if (pol == TYPEC_POLARITY_CC1)
		clrbits_le32(tca_base + 0x18, BIT(2));
	else
		setbits_le32(tca_base + 0x18, BIT(2));

	udelay(1);

	clrbits_le32(tca_base + 0x18, BIT(3));
}

static void setup_typec(void)
{
	int ret;

	tca_base = USB1_BASE_ADDR + 0xfc000;

#ifdef CONFIG_TARGET_IMX95_15X15_EVK
	struct gpio_desc ext_12v_desc;

	ret = tcpc_init(&portpd, portpd_config, NULL);
	if (ret) {
		printf("%s: tcpc portpd init failed, err=%d\n",
		       __func__, ret);
	} else if (tcpc_pd_sink_check_charging(&portpd)) {
		printf("Power supply on USB PD\n");

		/* Enable EXT 12V */
		ret = dm_gpio_lookup_name("gpio@22_1", &ext_12v_desc);
		if (ret) {
			printf("%s lookup gpio@22_1 failed ret = %d\n", __func__, ret);
			return;
		}

		ret = dm_gpio_request(&ext_12v_desc, "ext_12v_en");
		if (ret) {
			printf("%s request ext_12v_en failed ret = %d\n", __func__, ret);
			return;
		}

		/* Enable PER 12V regulator */
		dm_gpio_set_dir_flags(&ext_12v_desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);
	}
#endif

	ret = tcpc_init(&port, port_config, &tca_mux_select);
	if (ret) {
		printf("%s: tcpc init failed, err=%d\n", __func__, ret);
		return;
	}
}
#endif

#ifdef CONFIG_USB_DWC3

#define PHY_CTRL0			0xF0040
#define PHY_CTRL0_REF_SSP_EN		BIT(2)
#define PHY_CTRL0_FSEL_MASK		GENMASK(10, 5)
#define PHY_CTRL0_FSEL_24M		0x2a
#define PHY_CTRL0_FSEL_100M		0x27
#define PHY_CTRL0_SSC_RANGE_MASK	GENMASK(23, 21)
#define PHY_CTRL0_SSC_RANGE_4003PPM	(0x2 << 21)

#define PHY_CTRL1			0xF0044
#define PHY_CTRL1_RESET			BIT(0)
#define PHY_CTRL1_COMMONONN		BIT(1)
#define PHY_CTRL1_ATERESET		BIT(3)
#define PHY_CTRL1_DCDENB		BIT(17)
#define PHY_CTRL1_CHRGSEL		BIT(18)
#define PHY_CTRL1_VDATSRCENB0		BIT(19)
#define PHY_CTRL1_VDATDETENB0		BIT(20)

#define PHY_CTRL2			0xF0048
#define PHY_CTRL2_TXENABLEN0		BIT(8)
#define PHY_CTRL2_OTG_DISABLE		BIT(9)

#define PHY_CTRL6			0xF0058
#define PHY_CTRL6_RXTERM_OVERRIDE_SEL	BIT(29)
#define PHY_CTRL6_ALT_CLK_EN		BIT(1)
#define PHY_CTRL6_ALT_CLK_SEL		BIT(0)

static struct dwc3_device dwc3_device_data = {
#ifdef CONFIG_XPL_BUILD
	.maximum_speed = USB_SPEED_HIGH,
#else
	.maximum_speed = USB_SPEED_SUPER,
#endif
	.base = USB1_BASE_ADDR,
	.dr_mode = USB_DR_MODE_PERIPHERAL,
	.index = 0,
	.power_down_scale = 2,
};

static void dwc3_nxp_usb_phy_init(struct dwc3_device *dwc3)
{
	u32 value;

	/* USB3.0 PHY signal fsel for 24M ref */
	value = readl(dwc3->base + PHY_CTRL0);
	value &= ~PHY_CTRL0_FSEL_MASK;
	value |= FIELD_PREP(PHY_CTRL0_FSEL_MASK, PHY_CTRL0_FSEL_24M);
	writel(value, dwc3->base + PHY_CTRL0);

	/* Disable alt_clk_en and use internal MPLL clocks */
	value = readl(dwc3->base + PHY_CTRL6);
	value &= ~(PHY_CTRL6_ALT_CLK_SEL | PHY_CTRL6_ALT_CLK_EN);
	writel(value, dwc3->base + PHY_CTRL6);

	value = readl(dwc3->base + PHY_CTRL1);
	value &= ~(PHY_CTRL1_VDATSRCENB0 | PHY_CTRL1_VDATDETENB0);
	value |= PHY_CTRL1_RESET | PHY_CTRL1_ATERESET;
	writel(value, dwc3->base + PHY_CTRL1);

	value = readl(dwc3->base + PHY_CTRL0);
	value |= PHY_CTRL0_REF_SSP_EN;
	writel(value, dwc3->base + PHY_CTRL0);

	value = readl(dwc3->base + PHY_CTRL2);
	value |= PHY_CTRL2_TXENABLEN0 | PHY_CTRL2_OTG_DISABLE;
	writel(value, dwc3->base + PHY_CTRL2);

	udelay(10);

	value = readl(dwc3->base + PHY_CTRL1);
	value &= ~(PHY_CTRL1_RESET | PHY_CTRL1_ATERESET);
	writel(value, dwc3->base + PHY_CTRL1);
}
#endif

static int imx9_scmi_power_domain_enable(u32 domain, bool enable)
{
	struct udevice *dev;
	int ret;

	ret = uclass_get_device_by_name(UCLASS_CLK, "protocol@14", &dev);
	if (ret)
		return ret;

	return scmi_pwd_state_set(dev, 0, domain, enable ? 0 : BIT(30));
}

int board_usb_init(int index, enum usb_init_type init)
{
	int ret = 0;

	if (index == 0 && init == USB_INIT_DEVICE) {
		ret = imx9_scmi_power_domain_enable(IMX95_PD_HSIO_TOP, true);
		if (ret) {
			printf("SCMI_POWWER_STATE_SET Failed for USB\n");
			return ret;
		}

#ifdef CONFIG_USB_DWC3
		dwc3_nxp_usb_phy_init(&dwc3_device_data);
#endif
#ifdef CONFIG_USB_TCPC
		ret = tcpc_setup_ufp_mode(&port);
		if (ret)
			return ret;
#endif
#ifdef CONFIG_USB_DWC3
		return dwc3_uboot_init(&dwc3_device_data);
#endif
	} else if (index == 0 && init == USB_INIT_HOST) {
#ifdef CONFIG_USB_TCPC
		ret = tcpc_setup_dfp_mode(&port);
#endif
		return ret;
	}

	return 0;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	int ret = 0;
	if (index == 0 && init == USB_INIT_DEVICE) {
#ifdef CONFIG_USB_DWC3
		dwc3_uboot_exit(index);
#endif
	} else if (index == 0 && init == USB_INIT_HOST) {
#ifdef CONFIG_USB_TCPC
		ret = tcpc_disable_src_vbus(&port);
#endif
	}

	return ret;
}

static void netc_phy_rst(const char *gpio_name, const char *label)
{
	int ret;
	struct gpio_desc desc;

	/* ENET_RST_B */
	ret = dm_gpio_lookup_name(gpio_name, &desc);
	if (ret) {
		printf("%s lookup %s failed ret = %d\n", __func__, gpio_name, ret);
		return;
	}

	ret = dm_gpio_request(&desc, label);
	if (ret) {
		printf("%s request %s failed ret = %d\n", __func__, label, ret);
		return;
	}

	/* assert the ENET_RST_B */
	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE | GPIOD_ACTIVE_LOW);
	udelay(10000);
	dm_gpio_set_value(&desc, 0); /* deassert the ENET_RST_B */
	udelay(80000);

}

static void __maybe_unused netc_regulator_enable(const char *devname, bool enable)
{
	int ret;
	struct udevice *dev;

	ret = regulator_get_by_devname(devname, &dev);
	if (ret) {
		printf("Get %s regulator failed %d\n", devname, ret);
		return;
	}

	ret = regulator_set_enable_if_allowed(dev, enable);
	if (ret) {
		printf("%s %s regulator %d\n",
			enable ? "Enable": "Disable", devname, ret);
		return;
	}
}

void netc_init(void)
{
	int ret;

	ret = imx9_scmi_power_domain_enable(IMX95_PD_NETC, false);
	udelay(10000);

	/* Power up the NETC MIX. */
	ret = imx9_scmi_power_domain_enable(IMX95_PD_NETC, true);
	if (ret) {
		printf("SCMI_POWWER_STATE_SET Failed for NETC MIX\n");
		return;
	}

#ifdef CONFIG_TARGET_IMX95_15X15_EVK
	netc_phy_rst("gpio@22_4", "ENET1_RST_B");
	netc_phy_rst("gpio@22_5", "ENET2_RST_B");
#else
	netc_phy_rst("i2c5_io@21_2", "ENET1_RST_B");

	/* Enable in SW count */
	netc_regulator_enable("regulator-aqr-stby", true);
	netc_regulator_enable("regulator-mac-stby", true);
	netc_regulator_enable("regulator-aqr-en", true);
	netc_regulator_enable("regulator-mac-en", true);

	/* Disable regulator to have explicit reset to AQR PHY and clock generator */
	udelay(10000);
	netc_regulator_enable("regulator-aqr-stby", false);
	netc_regulator_enable("regulator-mac-stby", false);
	netc_regulator_enable("regulator-aqr-en", false);
	netc_regulator_enable("regulator-mac-en", false);

	udelay(10000);
	netc_regulator_enable("regulator-aqr-stby", true);
	netc_regulator_enable("regulator-mac-stby", true);

#endif
	pci_init();
}

static void flexspi_nor_steup(void)
{
	struct gpio_desc desc;
	int ret;

	if (!IS_ENABLED(CONFIG_TARGET_IMX95_15X15_EVK))
		return;

	/* Power cycle the M2 3V3 */
	ret = dm_gpio_lookup_name("gpio@22_10", &desc);
	if (ret)
		return;

	ret = dm_gpio_request(&desc, "M2_PWREN");
	if (ret)
		return;

	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT);
	dm_gpio_set_value(&desc, 0);
	udelay(100000);
	dm_gpio_set_value(&desc, 1);

	/* Enable 1.8V LDO */
	ret = dm_gpio_lookup_name("gpio@22_11", &desc);
	if (ret)
		return;

	ret = dm_gpio_request(&desc, "M2_DIS1");
	if (ret)
		return;

	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT);
	dm_gpio_set_value(&desc, 1);

	/* Deassert M2_SD3_nRST */
	ret = dm_gpio_lookup_name("GPIO5_9", &desc);
	if (ret)
		return;

	ret = dm_gpio_request(&desc, "M2_SD3_nRST");
	if (ret)
		return;

	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT);
	dm_gpio_set_value(&desc, 1);
}

void lvds_backlight_on(void)
{
	struct udevice *dev;
	int ret;
	u8 reg;

	if (!IS_ENABLED(CONFIG_TARGET_IMX95_15X15_EVK))
		return;

	ret = i2c_get_chip_for_busnum(2, 0x62, 1, &dev);
	if (ret) {
		printf("%s: Cannot find pca9632 led dev\n",
		       __func__);
		return;
	}

	reg = 1;
	dm_i2c_write(dev, 0x1, &reg, 1);

	reg = 5;
	dm_i2c_write(dev, 0x8, &reg, 1);
}

int board_init(void)
{
	int ret;
	ret = imx9_scmi_power_domain_enable(IMX95_PD_HSIO_TOP, true);
	if (ret) {
		printf("SCMI_POWWER_STATE_SET Failed for USB\n");
		return ret;
	}

	imx9_scmi_power_domain_enable(IMX95_PD_DISPLAY, false);
	imx9_scmi_power_domain_enable(IMX95_PD_CAMERA, false);

#if defined(CONFIG_USB_TCPC)
	setup_typec();
#endif

	netc_init();

	flexspi_nor_steup();

	power_on_m7("mx95evkrpmsg");

	lvds_backlight_on();

	return 0;
}

int board_late_init(void)
{
	if (IS_ENABLED(CONFIG_ENV_IS_IN_MMC))
		board_late_mmc_env_init();

	env_set("sec_boot", "no");
#ifdef CONFIG_AHAB_BOOT
	env_set("sec_boot", "yes");
#endif

	return 0;
}

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, struct bd_info *bd)
{
	char *p, *b, *s;
	char *token = NULL;
	int i, ret = 0;
	u64 base[CONFIG_NR_DRAM_BANKS] = {0};
	u64 size[CONFIG_NR_DRAM_BANKS] = {0};

	p = env_get("jh_root_mem");
	if (!p)
		return 0;

	i = 0;
	token = strtok(p, ",");
	while (token) {
		if (i >= CONFIG_NR_DRAM_BANKS) {
			printf("Error: The number of size@base exceeds CONFIG_NR_DRAM_BANKS.\n");
			return -EINVAL;
		}

		b = token;
		s = strsep(&b, "@");
		if (!s) {
			printf("The format of jh_root_mem is size@base[,size@base...].\n");
			return -EINVAL;
		}
		base[i] = simple_strtoull(b, NULL, 16);
		size[i] = simple_strtoull(s, NULL, 16);
		token = strtok(NULL, ",");
		i++;
	}

	ret = fdt_fixup_memory_banks(blob, base, size, CONFIG_NR_DRAM_BANKS);
	if (ret)
		return ret;

	return 0;
}
#endif

void board_quiesce_devices(void)
{
	int ret;
	struct uclass *uc_dev;

	ret = imx9_scmi_power_domain_enable(IMX95_PD_HSIO_TOP, false);
	if (ret) {
		printf("%s: Failed for HSIO MIX: %d\n", __func__, ret);
		return;
	}

	ret = imx9_scmi_power_domain_enable(IMX95_PD_NETC, false);
	if (ret) {
		printf("%s: Failed for NETC MIX: %d\n", __func__, ret);
		return;
	}

	ret = uclass_get(UCLASS_SPI_FLASH, &uc_dev);
	if (uc_dev)
		ret = uclass_destroy(uc_dev);
	if (ret)
		printf("couldn't remove SPI FLASH devices\n");
}

#if IS_ENABLED(CONFIG_OF_BOARD_FIXUP)

#if IS_ENABLED(CONFIG_TARGET_IMX95_15X15_EVK)
static void change_fdt_mido_pins(void *fdt)
{
	int nodeoff, ret;
	u32 enet1_pins[12] = { 0x00B8, 0x02BC, 0x0424, 0x00, 0x00, 0x57e,
		0x00BC, 0x02C0, 0x0428, 0x00, 0x00, 0x97e};

	nodeoff = fdt_path_offset(fdt, "/firmware/scmi/protocol@19/emdiogrp");
	if (nodeoff > 0) {

		int i;
		for (i = 0; i < 12; i++) {
			enet1_pins[i] = cpu_to_fdt32(enet1_pins[i]);
		}

		ret = fdt_setprop(fdt, nodeoff, "fsl,pins", enet1_pins, 12 * sizeof(u32));
		if (ret)
			printf("fdt_setprop fsl,pins error %d\n", ret);
		else
			debug("Update MDIO pins ok\n");
	}
}

static int board_fix_15x15_evk(void *fdt)
{
	int ret;
	struct udevice *bus;
	struct udevice *i2c_dev = NULL;

	ret = uclass_get_device_by_seq(UCLASS_I2C, 2, &bus);
	if (ret) {
		printf("%s: Can't find I2C bus 2\n", __func__);
		return 0;
	}

	ret = dm_i2c_probe(bus, 0x50, 0, &i2c_dev);
	if (ret) {
		ret = dm_i2c_probe(bus, 0x20, 0, &i2c_dev);
		if (!ret) {
			debug("Find Audio board\n");
			change_fdt_mido_pins(fdt);
		}
	}

	return 0;
}

#else
static void disable_fdt_resources(void *fdt)
{
	int i = 0;
	int nodeoff, ret;
	const char *status = "disabled";
	static const char * const dsi_nodes[] = {
		"/soc/bus@42000000/i2c@426b0000",
		"/soc/bus@42000000/i2c@426d0000",
		"/soc/netc-blk-ctrl@4cde0000"
	};

	for (i = 0; i < ARRAY_SIZE(dsi_nodes); i++) {
		nodeoff = fdt_path_offset(fdt, dsi_nodes[i]);
		if (nodeoff > 0) {
set_status:
			ret = fdt_setprop(fdt, nodeoff, "status", status,
					  strlen(status) + 1);
			if (ret == -FDT_ERR_NOSPACE) {
				ret = fdt_increase_size(fdt, 512);
				if (!ret)
					goto set_status;
			}
		}
	}
}

static int board_fix_19x19_evk(void *fdt)
{
	char cfgname[SCMI_MISC_MAX_CFGNAME];
	u32 msel;
	int ret;
	const char *netcfg = "mx95netc";

	ret = scmi_misc_cfginfo(&msel, cfgname);
	if (!ret) {
		debug("SM: %s\n", cfgname);
		if (!strcmp(netcfg, cfgname))
			disable_fdt_resources(fdt);
	}

	return 0;
}
#endif

int board_fix_fdt(void *fdt)
{
	/* Remove nodes based on fuses. */
	board_fix_fdt_fuse(fdt);

#if IS_ENABLED(CONFIG_TARGET_IMX95_15X15_EVK)
	return board_fix_15x15_evk(fdt);
#else
	return board_fix_19x19_evk(fdt);
#endif
}
#endif
#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_ANDROID_RECOVERY
int is_recovery_key_pressing(void)
{
	return 0;
}
#endif /*CONFIG_ANDROID_RECOVERY*/
#endif /*CONFIG_FSL_FASTBOOT*/
