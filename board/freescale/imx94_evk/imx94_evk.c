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
#include "crrm.h"

int board_early_init_f(void)
{
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

ulong tca_base;

void tca_mux_select(enum typec_cc_polarity pol)
{
	u32 val;

	if (!tca_base)
		return;

	printf("tca pol %u\n", pol);

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

	/* TODO */
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
#endif /* CONFIG_USB_TCPC */
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

	netc_regulator_enable("regulator-gpy-stby");

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

#if defined(CONFIG_USB_TCPC)
	setup_typec();
#endif

	netc_regulator_enable("regulator-m2-pwr");

	netc_init();

	power_on_m7("mx94evkrpmsg");

#if IS_ENABLED(CONFIG_IMX_CRRM)
	crrm_uboot_init();
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

#if IS_ENABLED(CONFIG_IMX_CRRM)
	crrm_uboot_late_init();
#endif

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

	/* Disable XSPI1 node for CRRM */
#if IS_ENABLED(CONFIG_IMX_CRRM)
	int nodeoff;
	const char *status = "disabled";

	nodeoff = fdt_path_offset(blob, "/soc/bus@42800000/spi@42b90000");
	if (nodeoff > 0) {
		ret = fdt_increase_size(blob, 256);
		if (ret) {
			printf("Unable to increase fdt size, err=%s\n", fdt_strerror(ret));
			return ret;
		}

		ret = fdt_setprop(blob, nodeoff, "status", status,
				  strlen(status) + 1);
		if (ret) {
			printf("Unable to disable XSPI1, err=%s\n", fdt_strerror(ret));
			return ret;
		}
	}
#endif

	return 0;
}
#endif

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

extern int board_fix_fdt_fuse(void *fdt);

int board_fix_fdt(void *fdt)
{
	/* Remove nodes based on fuses. */
	board_fix_fdt_fuse(fdt);

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
