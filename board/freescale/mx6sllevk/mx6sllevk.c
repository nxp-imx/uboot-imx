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
#include <asm/imx-common/mxc_i2c.h>
#include <asm/io.h>
#include <common.h>
#include <fsl_esdhc.h>
#include <i2c.h>
#include <linux/sizes.h>
#include <linux/fb.h>
#include <miiphy.h>
#include <mmc.h>
#include <mxsfb.h>
#include <netdev.h>
#include <power/pmic.h>
#include <power/pfuze100_pmic.h>
#include "../common/pfuze.h"
#include <usb.h>
#include <usb/ehci-fsl.h>
#if defined(CONFIG_MXC_EPDC)
#include <lcd.h>
#include <mxc_epdc_fb.h>
#endif
#include <asm/imx-common/video.h>

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_22K_UP  | PAD_CTL_SPEED_LOW |		\
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL_WP (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_DOWN | PAD_CTL_SPEED_LOW |		\
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define I2C_PAD_CTRL    (PAD_CTL_PKE | PAD_CTL_PUE |            \
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |               \
	PAD_CTL_DSE_40ohm | PAD_CTL_HYS |			\
	PAD_CTL_ODE)

#define LCD_PAD_CTRL    (PAD_CTL_HYS | PAD_CTL_PUS_100K_UP | PAD_CTL_PUE | \
	PAD_CTL_PKE | PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm)

#define OTG_ID_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_47K_UP  | PAD_CTL_SPEED_LOW |		\
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define LCD_PAD_CTRL    (PAD_CTL_HYS | PAD_CTL_PUS_100K_UP | PAD_CTL_PUE | \
	PAD_CTL_PKE | PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm)

#define EPDC_PAD_CTRL    (PAD_CTL_PKE | PAD_CTL_SPEED_MED |	\
	PAD_CTL_DSE_40ohm | PAD_CTL_SRE_FAST)

#ifdef CONFIG_SYS_I2C_MXC
#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)
/* I2C1 for PMIC and EPD */
struct i2c_pads_info i2c_pad_info1 = {
	.scl = {
		/* conflict with usb_otg2_pwr */
		.i2c_mode = MX6_PAD_I2C1_SCL__I2C1_SCL | PC,
		.gpio_mode = MX6_PAD_I2C1_SCL__GPIO3_IO12 | PC,
		.gp = IMX_GPIO_NR(3, 12),
	},
	.sda = {
		/* conflict with usb_otg2_oc */
		.i2c_mode = MX6_PAD_I2C1_SDA__I2C1_SDA | PC,
		.gpio_mode = MX6_PAD_I2C1_SDA__GPIO3_IO13 | PC,
		.gp = IMX_GPIO_NR(3, 13),
	},
};

/* I2C2 for LCD and ADV */
struct i2c_pads_info i2c_pad_info2 = {
	.scl = {
		.i2c_mode = MX6_PAD_I2C2_SCL__I2C2_SCL | PC,
		.gpio_mode = MX6_PAD_I2C2_SCL__GPIO3_IO14 | PC,
		.gp = IMX_GPIO_NR(3, 14),
	},
	.sda = {
		.i2c_mode = MX6_PAD_I2C2_SDA__I2C2_SDA | PC,
		.gpio_mode = MX6_PAD_I2C2_SDA__GPIO3_IO15 | PC,
		.gp = IMX_GPIO_NR(3, 15),
	},
};

#endif

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_SIZE;

	return 0;
}

static iomux_v3_cfg_t const uart1_pads[] = {
	MX6_PAD_UART1_TXD__UART1_DCE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX6_PAD_UART1_RXD__UART1_DCE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const wdog_pads[] = {
	MX6_PAD_WDOG_B__WDOG1_B | MUX_PAD_CTRL(NO_PAD_CTRL),
};

/* 8bit SD1 */
static iomux_v3_cfg_t const usdhc1_pads[] = {
	MX6_PAD_SD1_CLK__SD1_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_CMD__SD1_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA0__SD1_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA1__SD1_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA2__SD1_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA3__SD1_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA4__SD1_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA5__SD1_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA6__SD1_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA7__SD1_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),

	/* CD */
	MX6_PAD_KEY_ROW7__GPIO4_IO07 | MUX_PAD_CTRL(NO_PAD_CTRL),
	/* WP */
	MX6_PAD_GPIO4_IO22__SD1_WP   | MUX_PAD_CTRL(NO_PAD_CTRL),
};

/* EMMC */
static iomux_v3_cfg_t const usdhc2_pads[] = {
	MX6_PAD_SD2_CLK__SD2_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_CMD__SD2_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DATA0__SD2_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DATA1__SD2_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DATA2__SD2_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DATA3__SD2_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DATA4__SD2_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DATA5__SD2_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DATA6__SD2_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DATA7__SD2_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),

	/* DQS */
	MX6_PAD_GPIO4_IO21__SD2_STROBE | MUX_PAD_CTRL(NO_PAD_CTRL),
	/* RST_B */
	MX6_PAD_SD2_RESET__GPIO4_IO27 | MUX_PAD_CTRL(USDHC_PAD_CTRL | PAD_CTL_LVE),
};

/* Wifi SD */
static iomux_v3_cfg_t const usdhc3_pads[] = {
	MX6_PAD_SD3_CLK__SD3_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_CMD__SD3_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DATA0__SD3_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DATA1__SD3_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DATA2__SD3_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DATA3__SD3_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),

	/* CD */
	MX6_PAD_REF_CLK_32K__GPIO3_IO22 | MUX_PAD_CTRL(NO_PAD_CTRL),
};


static void setup_iomux_uart(void)
{
	imx_iomux_v3_setup_multiple_pads(uart1_pads, ARRAY_SIZE(uart1_pads));
}

#ifdef CONFIG_FSL_ESDHC
static struct fsl_esdhc_cfg usdhc_cfg[CONFIG_SYS_FSL_USDHC_NUM] = {
	{USDHC1_BASE_ADDR, 0, 8, 1},
	{USDHC2_BASE_ADDR, 0, 8, 0, 1}, /* fixed 1.8v IO voltage for eMMC chip */
	{USDHC3_BASE_ADDR, 0, 4},
};

#define USDHC1_CD_GPIO	IMX_GPIO_NR(4, 7)
#define USDHC2_PWR_GPIO	IMX_GPIO_NR(4, 27)
#define USDHC3_CD_GPIO	IMX_GPIO_NR(3, 22)

int board_mmc_get_env_dev(int devno)
{
	return devno;
}

int mmc_map_to_kernel_blk(int devno)
{
	return devno;
}

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;

	switch (cfg->esdhc_base) {
	case USDHC1_BASE_ADDR:
		ret = !gpio_get_value(USDHC1_CD_GPIO);
		break;
	case USDHC2_BASE_ADDR:
		ret = 1;
		break;
	case USDHC3_BASE_ADDR:
		ret = !gpio_get_value(USDHC3_CD_GPIO);
		break;
	}

	return ret;
}

int board_mmc_init(bd_t *bis)
{
	int i;

	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-boot device node)    (Physical Port)
	 * mmc0                    USDHC1
	 * mmc1                    USDHC2
	 * mmc2                    USDHC3
	 */
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			imx_iomux_v3_setup_multiple_pads(
				usdhc1_pads, ARRAY_SIZE(usdhc1_pads));
			gpio_direction_input(USDHC1_CD_GPIO);
			usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
			break;
		case 1:
			imx_iomux_v3_setup_multiple_pads(
				usdhc2_pads, ARRAY_SIZE(usdhc2_pads));
			usdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
			gpio_direction_output(USDHC2_PWR_GPIO, 1);
			break;
		case 2:
			imx_iomux_v3_setup_multiple_pads(
				usdhc3_pads, ARRAY_SIZE(usdhc3_pads));
			gpio_direction_input(USDHC3_CD_GPIO);
			usdhc_cfg[2].sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
			break;
		default:
			printf("Warning: you configured more USDHC controllers (%d) than supported by the board\n", i + 1);
			return 0;
		}

	if (fsl_esdhc_initialize(bis, &usdhc_cfg[i]))
		printf("Warning: failed to initialize mmc dev %d\n", i);
	}

	return 0;
}
#endif

#ifdef CONFIG_POWER
#define I2C_PMIC	0
static struct pmic *pfuze;
int power_init_board(void)
{
	int ret;
	u32 rev_id, value;

	ret = power_pfuze100_init(I2C_PMIC);
	if (ret)
		return ret;

	pfuze = pmic_get("PFUZE100");
	if (!pfuze)
		return -ENODEV;

	ret = pmic_probe(pfuze);
	if (ret)
		return ret;

	ret = pfuze_mode_init(pfuze, APS_PFM);
	if (ret < 0)
		return ret;

	pmic_reg_read(pfuze, PFUZE100_DEVICEID, &value);
	pmic_reg_read(pfuze, PFUZE100_REVID, &rev_id);
	printf("PMIC: PFUZE100! DEV_ID=0x%x REV_ID=0x%x\n", value, rev_id);

	/* set SW1AB staby volatage 0.975V */
	pmic_reg_read(pfuze, PFUZE100_SW1ABSTBY, &value);
	value &= ~0x3f;
	value |= 0x1b;
	pmic_reg_write(pfuze, PFUZE100_SW1ABSTBY, value);

	/* set SW1AB/VDDARM step ramp up time from 16us to 4us/25mV */
	pmic_reg_read(pfuze, PFUZE100_SW1ABCONF, &value);
	value &= ~0xc0;
	value |= 0x40;
	pmic_reg_write(pfuze, PFUZE100_SW1ABCONF, value);

	/* set SW1C staby volatage 0.975V */
	pmic_reg_read(pfuze, PFUZE100_SW1CSTBY, &value);
	value &= ~0x3f;
	value |= 0x1b;
	pmic_reg_write(pfuze, PFUZE100_SW1CSTBY, value);

	/* set SW1C/VDDSOC step ramp up time to from 16us to 4us/25mV */
	pmic_reg_read(pfuze, PFUZE100_SW1CCONF, &value);
	value &= ~0xc0;
	value |= 0x40;
	pmic_reg_write(pfuze, PFUZE100_SW1CCONF, value);

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

	ret = enable_lcdif_clock(dev->bus);
	if (ret) {
		printf("Enable LCDIF clock failed, %d\n", ret);
		return;
	}

	imx_iomux_v3_setup_multiple_pads(lcd_pads, ARRAY_SIZE(lcd_pads));

	/* Reset the LCD */
	gpio_direction_output(IMX_GPIO_NR(2, 19) , 0);
	udelay(500);
	gpio_direction_output(IMX_GPIO_NR(2, 19) , 1);

	gpio_direction_output(IMX_GPIO_NR(4, 3) , 1);

	/* Set Brightness to high */
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
	gpio_direction_input(IMX_GPIO_NR(2, 13));

	/* EPDC_VCOM0 - GPIO2[03] for VCOM control */
	imx_iomux_v3_setup_pad(MX6_PAD_EPDC_VCOM0__GPIO2_IO03 |
				MUX_PAD_CTRL(EPDC_PAD_CTRL));

	/* Set as output */
	gpio_direction_output(IMX_GPIO_NR(2, 3), 1);

	/* EPDC_PWRWAKEUP - GPIO2[14] for EPD PMIC WAKEUP */
	imx_iomux_v3_setup_pad(MX6_PAD_EPDC_PWR_WAKE__GPIO2_IO14 |
				MUX_PAD_CTRL(EPDC_PAD_CTRL));
	/* Set as output */
	gpio_direction_output(IMX_GPIO_NR(2, 14), 1);

	/* EPDC_PWRCTRL0 - GPIO2[07] for EPD PWR CTL0 */
	imx_iomux_v3_setup_pad(MX6_PAD_EPDC_PWR_CTRL0__GPIO2_IO07 |
				MUX_PAD_CTRL(EPDC_PAD_CTRL));
	/* Set as output */
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


#ifdef CONFIG_USB_EHCI_MX6
#define USB_OTHERREGS_OFFSET	0x800
#define UCTRL_PWR_POL		(1 << 9)
iomux_v3_cfg_t const usb_otg1_pads[] = {
	MX6_PAD_KEY_COL4__USB_OTG1_PWR | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX6_PAD_KEY_ROW4__USB_OTG1_OC  | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX6_PAD_EPDC_PWR_COM__USB_OTG1_ID | MUX_PAD_CTRL(OTG_ID_PAD_CTRL),
};

iomux_v3_cfg_t const usb_otg2_pads[] = {
	MX6_PAD_KEY_COL5__USB_OTG2_PWR   | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX6_PAD_ECSPI2_SCLK__USB_OTG2_OC | MUX_PAD_CTRL(NO_PAD_CTRL),
};

int board_usb_phy_mode(int port)
{
	if (port == 1)
		return USB_INIT_HOST;
	else
		return usb_phy_mode(port);
}

int board_ehci_hcd_init(int port)
{
	u32 *usbnc_usb_ctrl;

	if (port > 1)
		return -EINVAL;

	switch (port) {
	case 0:
		imx_iomux_v3_setup_multiple_pads(usb_otg1_pads,
						 ARRAY_SIZE(usb_otg1_pads));
		break;
	case 1:
		imx_iomux_v3_setup_multiple_pads(usb_otg2_pads,
						 ARRAY_SIZE(usb_otg2_pads));
		break;
	default:
		printf("MXC USB port %d not yet supported\n", port);
		return 1;
	}

	usbnc_usb_ctrl = (u32 *)(USB_BASE_ADDR + USB_OTHERREGS_OFFSET +
				 port * 4);

	/* Set Power polarity */
	setbits_le32(usbnc_usb_ctrl, UCTRL_PWR_POL);

	return 0;
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

#ifdef CONFIG_SYS_I2C_MXC
	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info1);
	setup_i2c(1, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info2);
#endif

#ifdef	CONFIG_MXC_EPDC
	enable_epdc_clock();
	setup_epdc();
#endif

	return 0;
}

int board_late_init(void)
{
#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));

	set_wdog_reset((struct wdog_regs *)WDOG1_BASE_ADDR);

	return 0;
}

u32 get_board_rev(void)
{
	return get_cpu_rev();
}

int checkboard(void)
{
	puts("Board: MX6SLL EVK\n");

	return 0;
}
