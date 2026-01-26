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
#include "../dts/upstream/src/arm64/freescale/imx95-power.h"
#include <asm/arch/sys_proto.h>
#include <i2c.h>
#include <adc.h>
#include <dm/uclass.h>
#include <dm/uclass-internal.h>

extern int board_fix_fdt_fuse(void *fdt);
static int get_board_version(int *rev, int *data);

int board_early_init_f(void)
{
	/* UART1: A55, UART2: M33, UART3: M7 */
	init_uart_clk(0);

	return 0;
}

#ifdef CONFIG_USB_TCPC
struct tcpc_port port;
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
	.max_snk_ma = 5000,
	.max_snk_mw = 100000,
	.op_snk_mv = 9000,
};

ulong tca_base;

void tca_mux_select(enum typec_cc_polarity pol)
{
	u32 val;

	if (!tca_base)
		return;

	/* Set OP mode to System configure Mode */
	clrbits_le32(tca_base + 0x10, 0x3);

	val = readl(tca_base + 0x30);

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
#ifdef CONFIG_TARGET_IMX95_19X19_FRDM_PRO
	struct gpio_desc dcdc2_5v_desc;
	struct gpio_desc dcdc_3_3v_desc;
	struct gpio_desc ext_12v_desc;
	struct gpio_desc ext_5v_desc;
	struct gpio_desc ext_3_3v_desc;
	struct gpio_desc ext_1_8v_desc;
#else
	struct gpio_desc ext_pwr_desc;
	unsigned int rev[2];
	unsigned int data[2];
#endif

	ret = tcpc_init(&portpd, portpd_config, NULL);
	if (ret) {
		printf("%s: tcpc portpd init failed, err=%d\n",
		       __func__, ret);
	} else if (tcpc_pd_sink_check_charging(&portpd)) {
		printf("Power supply on USB PD\n");
#ifdef CONFIG_TARGET_IMX95_19X19_FRDM_PRO
		/* Enable dcdc2_5v */
		ret = dm_gpio_lookup_name("gpio@22_1", &dcdc2_5v_desc);
		if (ret) {
			printf("%s lookup gpio@22_1 failed ret = %d\n", __func__, ret);
			return;
		}
		ret = dm_gpio_request(&dcdc2_5v_desc, "dcdc2_5v_en");
		if (ret) {
			printf("%s request dcdc2_5v_en failed ret = %d\n", __func__, ret);
			return;
		}
		/* Enable DCDC2_5V regulator */
		dm_gpio_set_dir_flags(&dcdc2_5v_desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);

		/* Enable dcdc_3_3v */
		ret = dm_gpio_lookup_name("gpio@22_18", &dcdc_3_3v_desc);
		if (ret) {
			printf("%s lookup gpio@22_18 failed ret = %d\n", __func__, ret);
			return;
		}
		ret = dm_gpio_request(&dcdc_3_3v_desc, "dcdc_3_3v_en");
		if (ret) {
			printf("%s request dcdc_3_3v_en failed ret = %d\n", __func__, ret);
			return;
		}
		/* Enable DCDC_3_3V regulator */
		dm_gpio_set_dir_flags(&dcdc_3_3v_desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);

		/* Enable EXT 12V */
		ret = dm_gpio_lookup_name("gpio@22_17", &ext_12v_desc);
		if (ret) {
			printf("%s lookup gpio@22_17 failed ret = %d\n", __func__, ret);
			return;
		}
		ret = dm_gpio_request(&ext_12v_desc, "ext_12v_en");
		if (ret) {
			printf("%s request ext_12v_en failed ret = %d\n", __func__, ret);
			return;
		}
		/* Enable Ext 12V regulator */
		dm_gpio_set_dir_flags(&ext_12v_desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);

		/* Enable EXT 5V */
		ret = dm_gpio_lookup_name("gpio@22_5", &ext_5v_desc);
		if (ret) {
			printf("%s lookup gpio@22_5 failed ret = %d\n", __func__, ret);
			return;
		}
		ret = dm_gpio_request(&ext_5v_desc, "ext_5v_en");
		if (ret) {
			printf("%s request ext_5v_en failed ret = %d\n", __func__, ret);
			return;
		}
		/* Enable Ext 5V regulator */
		dm_gpio_set_dir_flags(&ext_5v_desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);

		/* Enable EXT 3.3V */
		ret = dm_gpio_lookup_name("gpio@22_6", &ext_3_3v_desc);
		if (ret) {
			printf("%s lookup gpio@22_6 failed ret = %d\n", __func__, ret);
			return;
		}
		ret = dm_gpio_request(&ext_3_3v_desc, "ext_3_3v_en");
		if (ret) {
			printf("%s request ext_3_3v_en failed ret = %d\n", __func__, ret);
			return;
		}
		/* Enable Ext 3.3V regulator */
		dm_gpio_set_dir_flags(&ext_3_3v_desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);

		/* Enable EXT 1.8V */
		ret = dm_gpio_lookup_name("gpio@22_10", &ext_1_8v_desc);
		if (ret) {
			printf("%s lookup gpio@22_10 failed ret = %d\n", __func__, ret);
			return;
		}
		ret = dm_gpio_request(&ext_1_8v_desc, "ext_1_8v_en");
		if (ret) {
			printf("%s request ext_1_8v_en failed ret = %d\n", __func__, ret);
			return;
		}
		/* Enable Ext 1.8V regulator */
		dm_gpio_set_dir_flags(&ext_1_8v_desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);
#else
		/* Enable EXT PWR */
		ret = get_board_version(rev, data);
		if (ret == 0) {
			if (rev[0] < 1) {
				ret = dm_gpio_lookup_name("GPIO5_9", &ext_pwr_desc);
				if (ret) {
					printf("%s lookup GPIO5_9 failed ret = %d\n",
					       __func__, ret);
					return;
				}
			} else {
				ret = dm_gpio_lookup_name("gpio@22_12", &ext_pwr_desc);
				if (ret) {
					printf("%s lookup gpio@22_12 failed ret = %d\n",
					       __func__, ret);
					return;
				}
			}
		}

		ret = dm_gpio_request(&ext_pwr_desc, "ext_pwr_en");
		if (ret) {
			printf("%s request ext_pwr_en failed ret = %d\n", __func__, ret);
			return;
		}

		/* Enable EXT PWR */
		dm_gpio_set_dir_flags(&ext_pwr_desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);
#endif
	}

	ret = tcpc_init(&port, port_config, &tca_mux_select);
	if (ret) {
		printf("%s: tcpc init failed, err=%d\n", __func__, ret);
		return;
	}
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
#ifdef CONFIG_USB_TCPC
		ret = tcpc_setup_ufp_mode(&port);
		if (ret)
			return ret;
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

	if (index == 0 && init == USB_INIT_HOST) {
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
#ifdef CONFIG_TARGET_IMX95_19X19_FRDM_PRO
	netc_phy_rst("gpio@20_1", "ENET1_RST_B");
	netc_phy_rst("gpio@20_2", "ENET2_RST_B");
#else
	netc_phy_rst("gpio@22_0", "ENET1_RST_B");
	netc_phy_rst("gpio@22_1", "ENET2_RST_B");
#endif

	pci_init();
}

#ifdef CONFIG_TARGET_IMX95_15X15_FRDM
void lvds_backlight_on(void)
{
	struct udevice *dev;
	int ret;
	u8 reg;

	ret = i2c_get_chip_for_busnum(3, 0x62, 1, &dev);
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
#endif

static int get_board_version(int *rev, int *data)
{
	int i, ret;
	struct udevice *dev;

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
		return 0;
	} else {
		return -1;
	}
}

int board_init(void)
{
	int ret;
	unsigned int rev[2];
	unsigned int data[2];

	ret = get_board_version(rev, data);
	if (ret == 0)
		printf("BOARD: V%d.%d(ADC2:%d,ADC3:%d)\n", rev[0], rev[1], data[0], data[1]);
	else
		printf("BOARD: Unable to determine board version\n");

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

	power_on_m7("mx95evkrpmsg");
#ifdef CONFIG_TARGET_IMX95_15X15_FRDM
	lvds_backlight_on();
#endif

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
#ifdef CONFIG_TARGET_IMX95_15X15_FRDM
int board_fix_fdt_version(void *blob)
{
	int ret, nodeoffset;
	int gpio5_offset;
	u32 gpio5_phandle;
	u32 gpio_phandle_data[3];

	static const struct {
		const char *path;
		int gpio_pin;  // -1 means delete the node
	} configs[] = {
		{"/regulator-ext-3v3", -1},    // Delete node
		{"/regulator-ext-5v", 9},      // GPIO pin 9
		{"/regulator-m2-pwr", 11},     // GPIO pin 11
		{"/regulator-m2-mkey-pwr", 10}, // GPIO pin 10
	};

	gpio5_offset = fdt_path_offset(blob, "/soc/gpio@43850000");
	if (gpio5_offset < 0) {
		printf("Failed to find gpio5 node\n");
		return gpio5_offset;
	}

	gpio5_phandle = fdt_get_phandle(blob, gpio5_offset);
	if (!gpio5_phandle) {
		printf("Failed to get gpio5 phandle\n");
		return gpio5_phandle;
	}

	for (int i = 0; i < ARRAY_SIZE(configs); i++) {
		printf("Modify node: %s\n", configs[i].path);
		nodeoffset = fdt_path_offset(blob, configs[i].path);
		if (nodeoffset < 0)
			return nodeoffset;

		if (configs[i].gpio_pin < 0) {
			/* Delete node */
			ret = fdt_del_node(blob, nodeoffset);
			if (ret < 0) {
				printf("Unable to delete node %s, err=%s\n",
				       configs[i].path, fdt_strerror(ret));
			} else {
				printf("Delete node %s\n", configs[i].path);
			}
		} else {
			/* Set GPIO property */
			gpio_phandle_data[0] = cpu_to_fdt32(gpio5_phandle);
			gpio_phandle_data[1] = cpu_to_fdt32(configs[i].gpio_pin);
			gpio_phandle_data[2] = cpu_to_fdt32(1);

			ret = fdt_setprop(blob, nodeoffset, "gpio", gpio_phandle_data,
					  sizeof(gpio_phandle_data));
			if (ret < 0) {
				printf("Failed to set gpio property for %s: %s\n",
				       configs[i].path, fdt_strerror(ret));
				return ret;
			}
		}
	}

	return 0;
}
#endif
int ft_board_setup(void *blob, struct bd_info *bd)
{
	char *p, *b, *s;
	char *token = NULL;
	int i, ret = 0;
	u64 base[CONFIG_NR_DRAM_BANKS] = {0};
	u64 size[CONFIG_NR_DRAM_BANKS] = {0};
#ifdef CONFIG_TARGET_IMX95_15X15_FRDM
	unsigned int rev[2];
	unsigned int data[2];

	ret = get_board_version(rev, data);
	if (ret == 0) {
		if (rev[0] < 1) {
			/* For RevA and RevB, potentially remove or modify nodes */
			board_fix_fdt_version(blob);
		}
	}
#endif
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

int board_fix_fdt(void *fdt)
{
	/* Remove nodes based on fuses. */
	board_fix_fdt_fuse(fdt);
	return 0;
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
