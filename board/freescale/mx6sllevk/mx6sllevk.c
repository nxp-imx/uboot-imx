// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 * Copyright 2017-2018 NXP
 */

#include <init.h>
#include <asm/arch/clock.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/iomux.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/mx6-pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/gpio.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/io.h>
#include <common.h>
#include <linux/sizes.h>
#include <mmc.h>
#include <mxsfb.h>
#include <power/pmic.h>
#include <power/pfuze100_pmic.h>
#include "../common/pfuze.h"

#if defined(CONFIG_MXC_EPDC)
#include <lcd.h>
#include <mxc_epdc_fb.h>
#endif
#include <asm/mach-imx/video.h>

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define LCD_PAD_CTRL    (PAD_CTL_HYS | PAD_CTL_PUS_100K_UP | PAD_CTL_PUE | \
	PAD_CTL_PKE | PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm)

#define EPDC_PAD_CTRL    (PAD_CTL_PKE | PAD_CTL_SPEED_MED |	\
	PAD_CTL_DSE_40ohm | PAD_CTL_SRE_FAST)

int dram_init(void)
{
	gd->ram_size = imx_ddr_size();

	return 0;
}

static iomux_v3_cfg_t const uart1_pads[] = {
	MX6_PAD_UART1_TXD__UART1_DCE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX6_PAD_UART1_RXD__UART1_DCE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const wdog_pads[] = {
	MX6_PAD_WDOG_B__WDOG1_B | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static void setup_iomux_uart(void)
{
	imx_iomux_v3_setup_multiple_pads(uart1_pads, ARRAY_SIZE(uart1_pads));
}

#ifdef CONFIG_DM_PMIC_PFUZE100
int power_init_board(void)
{
	struct udevice *dev;
	int ret;
	u32 dev_id, rev_id, i;
	u32 switch_num = 6;
	u32 offset = PFUZE100_SW1CMODE;

	ret = pmic_get("pfuze100@8", &dev);
	if (ret == -ENODEV)
		return 0;

	if (ret != 0)
		return ret;

	dev_id = pmic_reg_read(dev, PFUZE100_DEVICEID);
	rev_id = pmic_reg_read(dev, PFUZE100_REVID);
	printf("PMIC: PFUZE100! DEV_ID=0x%x REV_ID=0x%x\n", dev_id, rev_id);


	/* Init mode to APS_PFM */
	pmic_reg_write(dev, PFUZE100_SW1ABMODE, APS_PFM);

	for (i = 0; i < switch_num - 1; i++)
		pmic_reg_write(dev, offset + i * SWITCH_SIZE, APS_PFM);

	/* set SW1AB staby volatage 0.975V */
	pmic_clrsetbits(dev, PFUZE100_SW1ABSTBY, 0x3f, 0x1b);

	/* set SW1AB/VDDARM step ramp up time from 16us to 4us/25mV */
	pmic_clrsetbits(dev, PFUZE100_SW1ABCONF, 0xc0, 0x40);

	/* set SW1C staby volatage 0.975V */
	pmic_clrsetbits(dev, PFUZE100_SW1CSTBY, 0x3f, 0x1b);

	/* set SW1C/VDDSOC step ramp up time to from 16us to 4us/25mV */
	pmic_clrsetbits(dev, PFUZE100_SW1CCONF, 0xc0, 0x40);

	return 0;
}
#endif

#ifdef CONFIG_DM_VIDEO
static iomux_v3_cfg_t const lcd_pads[] = {
	MX6_PAD_KEY_ROW5__GPIO4_IO03 | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX6_PAD_LCD_RESET__GPIO2_IO19 | MUX_PAD_CTRL(NO_PAD_CTRL),

	/* Use GPIO for Brightness adjustment, duty cycle = period */
	MX6_PAD_PWM1__GPIO3_IO23 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static int setup_lcd(void)
{
	int ret;

	ret = enable_lcdif_clock(MX6SLL_LCDIF_BASE_ADDR, 1);
	if (ret) {
		printf("Enable LCDIF clock failed, %d\n", ret);
		return -EPERM;
	}

	imx_iomux_v3_setup_multiple_pads(lcd_pads, ARRAY_SIZE(lcd_pads));

	/* Reset the LCD */
	gpio_request(IMX_GPIO_NR(2, 19), "lcd reset");
	gpio_direction_output(IMX_GPIO_NR(2, 19) , 0);
	udelay(500);
	gpio_direction_output(IMX_GPIO_NR(2, 19) , 1);

	gpio_request(IMX_GPIO_NR(4, 3), "lcd pwr en");
	gpio_direction_output(IMX_GPIO_NR(4, 3) , 1);

	/* Set Brightness to high */
	gpio_request(IMX_GPIO_NR(3, 23), "backlight");
	gpio_direction_output(IMX_GPIO_NR(3, 23) , 1);

	return 0;
}
#else
static inline int setup_lcd(void) { return 0; }
#endif

#ifdef CONFIG_MXC_EPDC
static iomux_v3_cfg_t const epdc_enable_pads[] = {
	MX6_PAD_EPDC_DATA00__EPDC_DATA00	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_EPDC_DATA01__EPDC_DATA01	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_EPDC_DATA02__EPDC_DATA02	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_EPDC_DATA03__EPDC_DATA03	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_EPDC_DATA04__EPDC_DATA04	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_EPDC_DATA05__EPDC_DATA05	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_EPDC_DATA06__EPDC_DATA06	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_EPDC_DATA07__EPDC_DATA07	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_EPDC_DATA08__EPDC_DATA08	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_EPDC_DATA09__EPDC_DATA09	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_EPDC_DATA10__EPDC_DATA10	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_EPDC_DATA11__EPDC_DATA11	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_EPDC_DATA12__EPDC_DATA12	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_EPDC_DATA13__EPDC_DATA13	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_EPDC_DATA14__EPDC_DATA14	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_EPDC_DATA15__EPDC_DATA15	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_EPDC_SDCLK__EPDC_SDCLK_P	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_EPDC_SDLE__EPDC_SDLE		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_EPDC_SDOE__EPDC_SDOE		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_EPDC_SDSHR__EPDC_SDSHR		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_EPDC_SDCE0__EPDC_SDCE0		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_EPDC_GDCLK__EPDC_GDCLK		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_EPDC_GDOE__EPDC_GDOE		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_EPDC_GDRL__EPDC_GDRL		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_EPDC_GDSP__EPDC_GDSP		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
};

static iomux_v3_cfg_t const epdc_disable_pads[] = {
	MX6_PAD_EPDC_DATA01__GPIO1_IO08,
	MX6_PAD_EPDC_DATA02__GPIO1_IO09,
	MX6_PAD_EPDC_DATA03__GPIO1_IO10,
	MX6_PAD_EPDC_DATA04__GPIO1_IO11,
	MX6_PAD_EPDC_DATA05__GPIO1_IO12,
	MX6_PAD_EPDC_DATA06__GPIO1_IO13,
	MX6_PAD_EPDC_DATA07__GPIO1_IO14,
	MX6_PAD_EPDC_DATA08__GPIO1_IO15,
	MX6_PAD_EPDC_DATA09__GPIO1_IO16,
	MX6_PAD_EPDC_DATA10__GPIO1_IO17,
	MX6_PAD_EPDC_DATA11__GPIO1_IO18,
	MX6_PAD_EPDC_DATA12__GPIO1_IO19,
	MX6_PAD_EPDC_DATA13__GPIO1_IO20,
	MX6_PAD_EPDC_DATA14__GPIO1_IO21,
	MX6_PAD_EPDC_DATA15__GPIO1_IO22,
	MX6_PAD_EPDC_SDCLK__GPIO1_IO23,
	MX6_PAD_EPDC_SDLE__GPIO1_IO24,
	MX6_PAD_EPDC_SDOE__GPIO1_IO25,
	MX6_PAD_EPDC_SDSHR__GPIO1_IO26,
	MX6_PAD_EPDC_SDCE0__GPIO1_IO27,
	MX6_PAD_EPDC_GDCLK__GPIO1_IO31,
	MX6_PAD_EPDC_GDOE__GPIO2_IO00,
	MX6_PAD_EPDC_GDRL__GPIO2_IO01,
	MX6_PAD_EPDC_GDSP__GPIO2_IO02,
};

vidinfo_t panel_info = {
	.vl_refresh = 85,
	.vl_col = 1024,
	.vl_row = 758,
	.vl_pixclock = 40000000,
	.vl_left_margin = 12,
	.vl_right_margin = 76,
	.vl_upper_margin = 4,
	.vl_lower_margin = 5,
	.vl_hsync = 12,
	.vl_vsync = 2,
	.vl_sync = 0,
	.vl_mode = 0,
	.vl_flag = 0,
	.vl_bpix = 3,
	.cmap = 0,
};

struct epdc_timing_params panel_timings = {
	.vscan_holdoff = 4,
	.sdoed_width = 10,
	.sdoed_delay = 20,
	.sdoez_width = 10,
	.sdoez_delay = 20,
	.gdclk_hp_offs = 524,
	.gdsp_offs = 327,
	.gdoe_offs = 0,
	.gdclk_offs = 19,
	.num_ce = 1,
};

static void setup_epdc_power(void)
{
	/* Setup epdc voltage */

	/* EPDC_PWRSTAT - GPIO2[13] for PWR_GOOD status */
	imx_iomux_v3_setup_pad(MX6_PAD_EPDC_PWR_STAT__GPIO2_IO13 |
				MUX_PAD_CTRL(EPDC_PAD_CTRL));
	gpio_request(IMX_GPIO_NR(2, 13), "epdc_pwrstat");
	gpio_direction_input(IMX_GPIO_NR(2, 13));

	/* EPDC_VCOM0 - GPIO2[03] for VCOM control */
	imx_iomux_v3_setup_pad(MX6_PAD_EPDC_VCOM0__GPIO2_IO03 |
				MUX_PAD_CTRL(EPDC_PAD_CTRL));

	/* Set as output */
	gpio_request(IMX_GPIO_NR(2, 3), "epdc_vcom0");
	gpio_direction_output(IMX_GPIO_NR(2, 3), 1);

	/* EPDC_PWRWAKEUP - GPIO2[14] for EPD PMIC WAKEUP */
	imx_iomux_v3_setup_pad(MX6_PAD_EPDC_PWR_WAKE__GPIO2_IO14 |
				MUX_PAD_CTRL(EPDC_PAD_CTRL));
	/* Set as output */
	gpio_request(IMX_GPIO_NR(2, 14), "epdc_pwr_wake");
	gpio_direction_output(IMX_GPIO_NR(2, 14), 1);

	/* EPDC_PWRCTRL0 - GPIO2[07] for EPD PWR CTL0 */
	imx_iomux_v3_setup_pad(MX6_PAD_EPDC_PWR_CTRL0__GPIO2_IO07 |
				MUX_PAD_CTRL(EPDC_PAD_CTRL));
	/* Set as output */
	gpio_request(IMX_GPIO_NR(2, 7), "epdc_pwr_ctrl0");
	gpio_direction_output(IMX_GPIO_NR(2, 7), 1);
}

static void epdc_enable_pins(void)
{
	/* epdc iomux settings */
	imx_iomux_v3_setup_multiple_pads(epdc_enable_pads,
				ARRAY_SIZE(epdc_enable_pads));
}

static void epdc_disable_pins(void)
{
	/* Configure MUX settings for EPDC pins to GPIO  and drive to 0 */
	imx_iomux_v3_setup_multiple_pads(epdc_disable_pads,
				ARRAY_SIZE(epdc_disable_pads));
}

static void setup_epdc(void)
{
	/*** epdc Maxim PMIC settings ***/

	/* EPDC_PWRSTAT - GPIO2[13] for PWR_GOOD status */
	imx_iomux_v3_setup_pad(MX6_PAD_EPDC_PWR_STAT__GPIO2_IO13 |
				MUX_PAD_CTRL(EPDC_PAD_CTRL));

	/* EPDC_VCOM0 - GPIO2[03] for VCOM control */
	imx_iomux_v3_setup_pad(MX6_PAD_EPDC_VCOM0__GPIO2_IO03 |
				MUX_PAD_CTRL(EPDC_PAD_CTRL));

	/* EPDC_PWRWAKEUP - GPIO2[14] for EPD PMIC WAKEUP */
	imx_iomux_v3_setup_pad(MX6_PAD_EPDC_PWR_WAKE__GPIO2_IO14 |
				MUX_PAD_CTRL(EPDC_PAD_CTRL));

	/* EPDC_PWRCTRL0 - GPIO2[07] for EPD PWR CTL0 */
	imx_iomux_v3_setup_pad(MX6_PAD_EPDC_PWR_CTRL0__GPIO2_IO07 |
				MUX_PAD_CTRL(EPDC_PAD_CTRL));

	/* Set pixel clock rates for EPDC in clock.c */

	panel_info.epdc_data.wv_modes.mode_init = 0;
	panel_info.epdc_data.wv_modes.mode_du = 1;
	panel_info.epdc_data.wv_modes.mode_gc4 = 3;
	panel_info.epdc_data.wv_modes.mode_gc8 = 2;
	panel_info.epdc_data.wv_modes.mode_gc16 = 2;
	panel_info.epdc_data.wv_modes.mode_gc32 = 2;

	panel_info.epdc_data.epdc_timings = panel_timings;

	setup_epdc_power();
}

void epdc_power_on(void)
{
	unsigned int reg;
	struct gpio_regs *gpio_regs = (struct gpio_regs *)GPIO2_BASE_ADDR;

	/* Set EPD_PWR_CTL0 to high - enable EINK_VDD (3.15) */
	gpio_set_value(IMX_GPIO_NR(2, 7), 1);
	udelay(1000);

	/* Enable epdc signal pin */
	epdc_enable_pins();

	/* Set PMIC Wakeup to high - enable Display power */
	gpio_set_value(IMX_GPIO_NR(2, 14), 1);

	/* Wait for PWRGOOD == 1 */
	while (1) {
		reg = readl(&gpio_regs->gpio_psr);
		if (!(reg & (1 << 13)))
			break;

		udelay(100);
	}

	/* Enable VCOM */
	gpio_set_value(IMX_GPIO_NR(2, 3), 1);

	udelay(500);
}

void epdc_power_off(void)
{
	/* Set PMIC Wakeup to low - disable Display power */
	gpio_set_value(IMX_GPIO_NR(2, 14), 0);

	/* Disable VCOM */
	gpio_set_value(IMX_GPIO_NR(2, 3), 0);

	epdc_disable_pins();

	/* Set EPD_PWR_CTL0 to low - disable EINK_VDD (3.15) */
	gpio_set_value(IMX_GPIO_NR(2, 7), 0);
}
#endif

int board_early_init_f(void)
{
	setup_iomux_uart();

	return 0;
}

int board_init(void)
{
	/* Address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

#ifdef	CONFIG_MXC_EPDC
	enable_epdc_clock();
	setup_epdc();
#endif

	return 0;
}

int board_late_init(void)
{

	env_set("tee", "no");
#ifdef CONFIG_IMX_OPTEE
	env_set("tee", "yes");
#endif

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

	setup_lcd();

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));

	set_wdog_reset((struct wdog_regs *)WDOG1_BASE_ADDR);

	return 0;
}

int checkboard(void)
{
	puts("Board: MX6SLL EVK\n");

	return 0;
}
