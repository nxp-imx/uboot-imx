// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2025 NXP
 */

#include <env.h>
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
#include <../dts/imx94-power.h>
#include <../dts/imx94-clock.h>
#include <asm/arch/sys_proto.h>
#include <i2c.h>
#include <dm/uclass.h>
#include <dm/uclass-internal.h>

int board_early_init_f(void)
{
	init_uart_clk(0);
	return 0;
}

ulong tca_base;
struct udevice *usb3_tcpc_dev = NULL;

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

#ifdef CONFIG_XPL_BUILD
static int typec_config(enum usb_init_type init)
{
	return 0;
}
#else
static int typec_config(enum usb_init_type init)
{
	uint8_t val, port;

	int ret;

	if (usb3_tcpc_dev == NULL)
		return -EINVAL;

	ret = dm_i2c_read(usb3_tcpc_dev, 0x4, (uint8_t *)&val, 1);
	if (ret) {
		printf("%s dm_i2c_read failed, err %d\n", __func__, ret);
		return -EIO;
	}

	/* Default port pin status is DRP, check port attachment status */
	port = (val & GENMASK(4, 2)) >> 2;
	if ((init == USB_INIT_HOST && port != 0x2) ||
		(init == USB_INIT_DEVICE && port != 0x1))
		return -EPERM;

	val = val & 0x3;
	if (val == 0x1)
		tca_mux_select(TYPEC_POLARITY_CC1);
	else if (val == 0x2)
		tca_mux_select(TYPEC_POLARITY_CC2);
	else
		return -EPERM;

	return 0;
}
#endif

static int setup_usb3_typec(void)
{
	int ret;
	struct udevice *bus;

	tca_base = USB1_BASE_ADDR + 0xfc000;

	ret = uclass_get_device_by_seq(UCLASS_I2C, 2, &bus); /*lpi2c3*/
	if (ret) {
		printf("%s: Can't find bus\n", __func__);
		return -EINVAL;
	}

	ret = dm_i2c_probe(bus, 0x1d, 0, &usb3_tcpc_dev);
	if (ret) {
		printf("%s: Can't find typec device id=0x1d\n",
			__func__);
		return -ENODEV;
	}

	return 0;
}

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
		ret = imx9_scmi_power_domain_enable(IMX94_PD_HSIO_TOP, true);
		if (ret) {
			printf("SCMI_POWWER_STATE_SET Failed for USB\n");
			return ret;
		}

#ifdef CONFIG_USB_DWC3
		dwc3_nxp_usb_phy_init(&dwc3_device_data);
#endif
		ret = typec_config(init);
		if (ret)
			return ret;

#ifdef CONFIG_USB_DWC3
		return dwc3_uboot_init(&dwc3_device_data);
#endif
	} else if (index == 0 && init == USB_INIT_HOST) {
		ret = typec_config(init);
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
	}

	return ret;
}

static void netc_regulator_enable(const char *devname)
{
	int ret;
	struct udevice *dev;

	ret = regulator_get_by_devname(devname, &dev);
	if (ret) {
		printf("Get %s regulator failed %d\n", devname, ret);
		return;
	}

	ret = regulator_set_enable_if_allowed(dev, true);
	if (ret) {
		printf("Enable %s regulator %d\n", devname, ret);
		return;
	}
}

static bool is_netc_cfg(void)
{
	char cfgname[SCMI_MISC_MAX_CFGNAME];
	u32 msel;
	int ret;
	const char *netcfg = "mx94evknetc";

	ret = scmi_misc_cfginfo(&msel, cfgname);
	if (!ret) {
		debug("SM: %s\n", cfgname);
		if (!strcmp(netcfg, cfgname))
			return true;
	}

	return false;
}

void netc_init(void)
{
	int ret;

	if (is_netc_cfg())
		return;

	/* Power up the NETC MIX. */
	ret = imx9_scmi_power_domain_enable(IMX94_PD_NETC, true);
	if (ret) {
		printf("SCMI_POWWER_STATE_SET Failed for NETC MIX\n");
		return;
	}

	set_clk_netc(ENET_125MHZ);

	pci_init();
}

int board_init(void)
{
	int ret;
	ret = imx9_scmi_power_domain_enable(IMX94_PD_HSIO_TOP, true);
	if (ret) {
		printf("SCMI_POWWER_STATE_SET Failed for USB\n");
		return ret;
	}

	imx9_scmi_power_domain_enable(IMX94_PD_DISPLAY, false);

	setup_usb3_typec();

	netc_regulator_enable("regulator-m2-pwr");

	netc_init();

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

#if IS_ENABLED(CONFIG_OF_BOARD_FIXUP)
static void disable_fdt_resources(void *fdt)
{
	int i = 0;
	int nodeoff, ret;
	const char *status = "disabled";
	static const char * const dsi_nodes[] = {
		"/soc/bus@42000000/i2c@42530000",
		"/soc/bus@42000000/i2c@426c0000",
		"/soc/system-controller@4ceb0000"
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

int board_fix_fdt(void *fdt)
{
	if (is_netc_cfg())
		disable_fdt_resources(fdt);

	return 0;
}
#endif

void board_quiesce_devices(void)
{
	int ret;
	struct uclass *uc_dev;

	ret = imx9_scmi_power_domain_enable(IMX94_PD_HSIO_TOP, false);
	if (ret) {
		printf("%s: Failed for HSIO MIX: %d\n", __func__, ret);
		return;
	}

	ret = imx9_scmi_power_domain_enable(IMX94_PD_NETC, false);
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

#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_ANDROID_RECOVERY
int is_recovery_key_pressing(void)
{
	return 0;
}
#endif /*CONFIG_ANDROID_RECOVERY*/
#endif /*CONFIG_FSL_FASTBOOT*/
