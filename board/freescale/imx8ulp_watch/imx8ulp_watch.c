// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2022 NXP
 */

#include <common.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/arch/imx8ulp-pins.h>
#include <asm/arch/clock.h>
#include <asm/arch/pcc.h>
#include <asm/arch/sys_proto.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/gpio.h>
#include <i2c.h>
#include <power-domain.h>
#include <dt-bindings/power/imx8ulp-power.h>
#include <linux/delay.h>

DECLARE_GLOBAL_DATA_PTR;

#define IMX8ULP_GPIOA_BASE_ADDR		(0x28800000 + 0x40)
#define IMX8ULP_GPIOB_BASE_ADDR		(0x28810000 + 0x40)
#define IMX8ULP_GPIOC_BASE_ADDR		(0x28820000 + 0x40)

#define PANEL_RESET_GPIO_ADDR		IMX8ULP_GPIOC_BASE_ADDR
#define PANEL_RESET_PIN			10


static iomux_cfg_t const panel_reset_pads[] = {
	IMX8ULP_PAD_PTC10__PTC10,
};

static void imx8ulp_gpio_set_value(struct gpio_regs *regs, int offset, int value)
{
	if (value)
		writel((1 << offset), &regs->gpio_psor);
	else
		writel((1 << offset), &regs->gpio_pcor);
}

void mipi_dsi_mux_panel(void)
{
	struct gpio_regs *panel_rst = (struct gpio_regs *)PANEL_RESET_GPIO_ADDR;

	/* It is temp solution to directly access gpio in RTD, need change to rpmsg later */

	imx8ulp_iomux_setup_multiple_pads(panel_reset_pads, ARRAY_SIZE(panel_reset_pads));
	panel_rst->gpio_pddr |= (1 << PANEL_RESET_PIN); // set panel reset pin as output direction
}

void mipi_dsi_panel_reset(void)
{
	/* It is temp solution to directly access gpio in RTD, need change to rpmsg later */
	struct gpio_regs *panel_rst = (struct gpio_regs *)PANEL_RESET_GPIO_ADDR;
	imx8ulp_gpio_set_value(panel_rst, PANEL_RESET_PIN, 0);
	mdelay(1);
	imx8ulp_gpio_set_value(panel_rst, PANEL_RESET_PIN, 1);
	mdelay(12);
}

int board_init(void)
{
	/* When sync with M33 is failed, use local driver to set for video */
	if (!is_m33_handshake_necessary() && IS_ENABLED(CONFIG_VIDEO)) {
		mipi_dsi_mux_panel();
		mipi_dsi_panel_reset();
	}

	return 0;
}

int board_early_init_f(void)
{
	return 0;
}

int board_late_init(void)
{
#if CONFIG_IS_ENABLED(ENV_IS_IN_MMC)
	board_late_mmc_env_init();
#endif

	env_set("sec_boot", "no");
#ifdef CONFIG_AHAB_BOOT
	env_set("sec_boot", "yes");
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


void board_quiesce_devices(void)
{
	/* Disable the power domains may used in u-boot before entering kernel */
#if CONFIG_IS_ENABLED(POWER_DOMAIN)
	struct udevice *scmi_devpd;
	int ret, i;
	struct power_domain pd;
	ulong ids[] = {
		IMX8ULP_PD_FLEXSPI2, IMX8ULP_PD_USB0, IMX8ULP_PD_USDHC0,
		IMX8ULP_PD_USDHC1, IMX8ULP_PD_USDHC2_USB1, IMX8ULP_PD_DCNANO,
		IMX8ULP_PD_MIPI_DSI};

	ret = uclass_get_device(UCLASS_POWER_DOMAIN, 0, &scmi_devpd);
	if (ret) {
		printf("Cannot get scmi devpd: err=%d\n", ret);
		return;
	}

	pd.dev = scmi_devpd;

	for (i = 0; i < ARRAY_SIZE(ids); i++) {
		pd.id = ids[i];
		ret = power_domain_off(&pd);
		if (ret)
			printf("power_domain_off %lu failed: err=%d\n", ids[i], ret);
	}
#endif
}
