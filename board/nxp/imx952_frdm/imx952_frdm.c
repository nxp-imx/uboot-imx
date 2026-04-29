// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2025-2026 NXP
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
#include <asm/global_data.h>
#include <power/regulator.h>
#include <scmi_agent.h>
#include <asm/arch/sys_proto.h>
#include <i2c.h>
#include <dm/uclass.h>
#include <dm/uclass-internal.h>
#include "../arch/arm/dts/imx952-power.h"

#define PD_HSIO_TOP IMX952_PD_HSIO_TOP
#define PD_NETC IMX952_PD_NETC
#define PD_DISPLAY IMX952_PD_DISPLAY
#define PD_CAMERA IMX952_PD_CAMERA

DECLARE_GLOBAL_DATA_PTR;

extern int board_fix_fdt_fuse(void *fdt);

#if CONFIG_IS_ENABLED(EFI_HAVE_CAPSULE_SUPPORT)
#define IMX_BOOT_IMAGE_GUID \
	EFI_GUID(0x58a661f3, 0xe7c7, 0x4173, 0x80, 0x21, \
		0xa3, 0x1b, 0x95, 0xc8, 0x6e, 0x9b)

struct efi_fw_image fw_images[] = {
	{
		.image_type_id = IMX_BOOT_IMAGE_GUID,
		.fw_name = u"IMX952-EVK-RAW",
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
struct tcpc_port portpd;
struct tcpc_port port;
struct tcpc_port_config portpd_config = {
	.i2c_bus = 2, /* i2c3 */
	.addr = 0x52,
	.port_type = TYPEC_PORT_UFP,
	.disable_pd = false,
	.max_snk_mv = 20000,
	.max_snk_ma = 5000,
	.max_snk_mw = 100000,
	.op_snk_mv = 9000,
};
struct tcpc_port_config port_config = {
	.i2c_bus = 3, /* i2c4 */
	.addr = 0x50,
	.port_type = TYPEC_PORT_DRP,
	.disable_pd = true,
};

static int setup_typec(void)
{
	int ret;

	ret = tcpc_init(&portpd, portpd_config, NULL);
	if (ret) {
		printf("%s: tcpc portpd init failed, err=%d\n",
		       __func__, ret);
	} else if (tcpc_pd_sink_check_charging(&portpd))
		printf("Power supply on USB PD\n");

	debug("tcpc_init port 1\n");
	ret = tcpc_init(&port, port_config, NULL);
	if (ret) {
		printf("%s: tcpc port init failed, err=%d\n",
		       __func__, ret);
	}

	return ret;
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
		ret = imx9_scmi_power_domain_enable(PD_HSIO_TOP, true);
		if (ret) {
			printf("SCMI_POWWER_STATE_SET Failed for USB\n");
			return ret;
		}

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

int board_ehci_usb_phy_mode(struct udevice *dev)
{
	int ret = 0;
	enum typec_cc_polarity pol;
	enum typec_cc_state state;
	struct tcpc_port *port_ptr;

	debug("%s %d\n", __func__, dev_seq(dev));

	if (dev_seq(dev) == 0)
		port_ptr = &port;
	else
		return USB_INIT_HOST;

	tcpc_setup_ufp_mode(port_ptr);

	ret = tcpc_get_cc_status(port_ptr, &pol, &state);

	tcpc_print_log(port_ptr);
	if (!ret) {
		if (state == TYPEC_STATE_SRC_RD_RA || state == TYPEC_STATE_SRC_RD)
			return USB_INIT_HOST;
	}

	return USB_INIT_DEVICE;
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

static bool is_netc_cfg(void)
{
	char cfgname[SCMI_MISC_MAX_CFGNAME];
	u32 msel;
	int ret;
	const char *netcfg = "mx952evknetc";

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

	ret = imx9_scmi_power_domain_enable(PD_NETC, false);
	udelay(10000);

	/* Power up the NETC MIX. */
	ret = imx9_scmi_power_domain_enable(PD_NETC, true);
	if (ret) {
		printf("SCMI_POWWER_STATE_SET Failed for NETC MIX\n");
		return;
	}

	netc_phy_rst("i2c2_io@20_0", "ENET1_RST_B");
	netc_phy_rst("i2c2_io@20_1", "ENET2_RST_B");
}

void lvds_backlight_on(void)
{
	/* None */
}

int board_init(void)
{
	int ret;

	ret = imx9_scmi_power_domain_enable(PD_HSIO_TOP, true);
	if (ret) {
		printf("SCMI_POWWER_STATE_SET Failed for USB\n");
		return ret;
	}

	imx9_scmi_power_domain_enable(PD_DISPLAY, false);
	imx9_scmi_power_domain_enable(PD_CAMERA, false);

#if defined(CONFIG_USB_TCPC)
	setup_typec();
#endif

	netc_init();

	power_on_m7("mx952evkrpmsg");

	lvds_backlight_on();

	return 0;
}

int board_late_init(void)
{
	char jh_root_mem[64];

	if (IS_ENABLED(CONFIG_ENV_IS_IN_MMC))
		board_late_mmc_env_init();

	env_set("sec_boot", "no");
#ifdef CONFIG_AHAB_BOOT
	env_set("sec_boot", "yes");
#endif
	/*
	 * jailhouse inmate cell uses the address [0x100000000, 0x180000000)
	 * imx952_evk is to support two boards with different DRAM size, so
	 * runtime cut off them from jh_root_mem.imx952_frdm resuse the code.
	 */
	snprintf(jh_root_mem, sizeof(jh_root_mem), "0x58000000@0x90000000,0x%llx@0x180000000",
		 gd->bd->bi_dram[1].size - SZ_2G);

	env_set("jailhouse_root_mem", jh_root_mem);

	return 0;
}

#ifdef CONFIG_OF_BOARD_SETUP
static int jh_mem_fdt_setup(void *blob)
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

int ft_board_setup(void *blob, struct bd_info *bd)
{
	int ret;

	ret = jh_mem_fdt_setup(blob);
	if (ret) {
		printf("jailhouse memory process fail.\n");
		return ret;
	}

	return 0;
}
#endif

void board_quiesce_devices(void)
{
	int ret;
	struct uclass *uc_dev;

	ret = imx9_scmi_power_domain_enable(PD_HSIO_TOP, false);
	if (ret) {
		printf("%s: Failed for HSIO MIX: %d\n", __func__, ret);
		return;
	}

	ret = imx9_scmi_power_domain_enable(PD_NETC, false);
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

static void disable_fdt_resources(void *fdt)
{
	int i = 0;
	int nodeoff, ret;
	const char *status = "disabled";
	static const char * const dsi_nodes[] = {
		"/soc/bus@42000000/i2c@422e0000",
		"/soc/netc-blk-ctrl@4cd20000"
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
	/* Remove nodes based on fuses. */
	board_fix_fdt_fuse(fdt);

	if (is_netc_cfg())
		disable_fdt_resources(fdt);

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
