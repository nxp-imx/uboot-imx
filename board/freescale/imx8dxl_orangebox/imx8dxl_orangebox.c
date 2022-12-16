// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2022 NXP
 */

#include <common.h>
#include <cpu_func.h>
#include <env.h>
#include <errno.h>
#include <init.h>
#include <asm/global_data.h>
#include <linux/delay.h>
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

#define ENET_INPUT_PAD_CTRL ((SC_PAD_CONFIG_OD_IN << PADRING_CONFIG_SHIFT) | \
			     (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) | \
			     (SC_PAD_28FDSOI_DSE_18V_10MA << PADRING_DSE_SHIFT) | \
			     (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define ENET_NORMAL_PAD_CTRL ((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | \
			      (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) | \
			      (SC_PAD_28FDSOI_DSE_18V_10MA << PADRING_DSE_SHIFT) | \
			      (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

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

	return 0;
}

static inline void board_gpio_init(void) {}

int checkboard(void)
{
	puts("Board: iMX8DXL Orange Box\n");

	print_bootinfo();

	return 0;
}

#ifdef CONFIG_DWC_ETH_QOS
static int setup_eqos(void)
{
	int err;

	/* set GPR14:12 to b'001: RGMII mode */
	err = sc_misc_set_control(-1, SC_R_ENET_1, SC_C_INTF_SEL, 0x1);
	if (err)
		printf("SC_R_ENET_1 INTF_SEL failed! (error = %d)\n", err);

	/* enable GPR11: CLK_GEN_EN */
	err = sc_misc_set_control(-1, SC_R_ENET_1, SC_C_CLK_GEN_EN, 1);
	if (err)
		printf("SC_R_ENET_1 CLK_GEN_EN failed! (error = %d)\n", err);

	return 0;
}
#endif

int board_init(void)
{
	board_gpio_init();
#ifdef CONFIG_DWC_ETH_QOS
	/* clock, phy interface mode */
	setup_eqos();
#endif

#ifdef CONFIG_IMX_SNVS_SEC_SC_AUTO
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
	};

	imx8_power_off_pd_devices(power_on_devices, ARRAY_SIZE(power_on_devices));
}

/*
 * Board specific reset that is system reset.
 */
void reset_cpu(void)
{
	/* TODO */
}

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, struct bd_info *bd)
{
	return 0;
}
#endif

int board_late_init(void)
{
	char *fdt_file;
	bool __maybe_unused m4_booted;

	build_info();

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	env_set("board_name", "OrangeBox");
	env_set("board_rev", "iMX8DXL");
#endif

	env_set("sec_boot", "no");
#ifdef CONFIG_AHAB_BOOT
	env_set("sec_boot", "yes");
#endif

	fdt_file = env_get("fdt_file");
	m4_booted = m4_parts_booted();

	if (fdt_file && !strcmp(fdt_file, "undefined"))
		env_set("fdt_file", "imx8dxl-orangebox.dtb");

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

	return 0;
}
