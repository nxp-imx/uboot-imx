/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/arch/clock.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/iomux.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/mx6-pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/gpio.h>
#include <asm/imx-common/iomux-v3.h>
#include <asm/imx-common/boot_mode.h>
#include <asm/io.h>
#include <common.h>
#include <linux/sizes.h>
#include <mmc.h>
#include <mxsfb.h>
#include <power/pmic.h>
#include <power/pfuze100_pmic.h>
#include "../common/pfuze.h"
#include <asm/imx-common/video.h>

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define LCD_PAD_CTRL    (PAD_CTL_HYS | PAD_CTL_PUS_100K_UP | PAD_CTL_PUE | \
	PAD_CTL_PKE | PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm)

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

	ret = pmic_get("pfuze100", &dev);
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

#ifdef CONFIG_VIDEO_MXS
static iomux_v3_cfg_t const lcd_pads[] = {
	MX6_PAD_LCD_CLK__LCD_CLK | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_ENABLE__LCD_ENABLE | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_HSYNC__LCD_HSYNC | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_VSYNC__LCD_VSYNC | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA00__LCD_DATA00 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA01__LCD_DATA01 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA02__LCD_DATA02 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA03__LCD_DATA03 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA04__LCD_DATA04 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA05__LCD_DATA05 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA06__LCD_DATA06 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA07__LCD_DATA07 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA08__LCD_DATA08 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA09__LCD_DATA09 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA10__LCD_DATA10 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA11__LCD_DATA11 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA12__LCD_DATA12 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA13__LCD_DATA13 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA14__LCD_DATA14 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA15__LCD_DATA15 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA16__LCD_DATA16 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA17__LCD_DATA17 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA18__LCD_DATA18 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA19__LCD_DATA19 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA20__LCD_DATA20 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA21__LCD_DATA21 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA22__LCD_DATA22 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA23__LCD_DATA23 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_KEY_ROW5__GPIO4_IO03 | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX6_PAD_LCD_RESET__GPIO2_IO19 | MUX_PAD_CTRL(NO_PAD_CTRL),

	/* Use GPIO for Brightness adjustment, duty cycle = period */
	MX6_PAD_PWM1__GPIO3_IO23 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

void do_enable_parallel_lcd(struct display_info_t const *dev)

{
	int ret;

	ret = enable_lcdif_clock(dev->bus, 1);
	if (ret) {
		printf("Enable LCDIF clock failed, %d\n", ret);
		return;
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
}

struct display_info_t const displays[] = {{
	.bus = MX6SLL_LCDIF_BASE_ADDR,
	.addr = 0,
	.pixfmt = 24,
	.detect = NULL,
	.enable	= do_enable_parallel_lcd,
	.mode	= {
		.name		= "MCIMX28LCD",
		.xres           = 800,
		.yres           = 480,
		.pixclock       = 29850,
		.left_margin    = 89,
		.right_margin   = 164,
		.upper_margin   = 23,
		.lower_margin   = 10,
		.hsync_len      = 10,
		.vsync_len      = 10,
		.sync           = 0,
		.vmode          = FB_VMODE_NONINTERLACED
} } };
size_t display_count = ARRAY_SIZE(displays);
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

	return 0;
}

int board_late_init(void)
{
	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));

	return 0;
}

int checkboard(void)
{
	puts("Board: MX6SLL EVK\n");

	return 0;
}

int board_mmc_get_env_dev(int devno)
{
	return devno;
}

int mmc_map_to_kernel_blk(int devno)
{
	return devno;
}
