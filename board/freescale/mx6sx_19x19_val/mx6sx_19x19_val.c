/*
 * Copyright (C) 2014-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/arch/clock.h>
#include <asm/arch/iomux.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/mx6-pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/gpio.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/io.h>
#include <linux/sizes.h>
#include <common.h>
#include <fsl_esdhc_imx.h>
#include <mmc.h>
#include <miiphy.h>
#include <netdev.h>
#ifdef CONFIG_SYS_I2C_MXC
#include <i2c.h>
#include <asm/mach-imx/mxc_i2c.h>
#endif
#include <asm/arch/crm_regs.h>
#include <power/pmic.h>
#include <power/pfuze100_pmic.h>
#include "../common/pfuze.h"
#include <usb.h>
#include <usb/ehci-ci.h>
#include <asm/mach-imx/video.h>
#include <dm.h>

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_22K_UP  | PAD_CTL_SPEED_LOW |		\
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define ENET_PAD_CTRL  (PAD_CTL_PUS_100K_UP | PAD_CTL_PUE |     \
	PAD_CTL_SPEED_MED   |                                   \
	PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST)

#define ENET_RX_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |          \
	PAD_CTL_SPEED_MED   | PAD_CTL_SRE_FAST)

#define I2C_PAD_CTRL    (PAD_CTL_PKE | PAD_CTL_PUE |            \
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |               \
	PAD_CTL_DSE_40ohm | PAD_CTL_HYS |			\
	PAD_CTL_ODE)

#define LCD_PAD_CTRL    (PAD_CTL_HYS | PAD_CTL_PUS_100K_UP | PAD_CTL_PUE | \
	PAD_CTL_PKE | PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm)

#define GPMI_PAD_CTRL0 (PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP)
#define GPMI_PAD_CTRL1 (PAD_CTL_DSE_40ohm | PAD_CTL_SPEED_MED | \
			PAD_CTL_SRE_FAST)
#define GPMI_PAD_CTRL2 (GPMI_PAD_CTRL0 | GPMI_PAD_CTRL1)

#define SPI_PAD_CTRL (PAD_CTL_HYS |				\
	PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm | PAD_CTL_SRE_FAST)

#define WEIM_NOR_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST)

#define WEIM_NOR_PAD_CTRL2 (PAD_CTL_HYS | PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm)

#define OTG_ID_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_47K_UP  | PAD_CTL_SPEED_LOW |		\
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#ifdef CONFIG_SYS_I2C
#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)
/* I2C1 for PMIC */
struct i2c_pads_info i2c_pad_info1 = {
	.scl = {
		.i2c_mode = MX6_PAD_GPIO1_IO00__I2C1_SCL | PC,
		.gpio_mode = MX6_PAD_GPIO1_IO00__GPIO1_IO_0 | PC,
		.gp = IMX_GPIO_NR(1, 0),
	},
	.sda = {
		.i2c_mode = MX6_PAD_GPIO1_IO01__I2C1_SDA | PC,
		.gpio_mode = MX6_PAD_GPIO1_IO01__GPIO1_IO_1 | PC,
		.gp = IMX_GPIO_NR(1, 1),
	},
};

/* I2C2 */
struct i2c_pads_info i2c_pad_info2 = {
	.scl = {
		.i2c_mode = MX6_PAD_GPIO1_IO02__I2C2_SCL | PC,
		.gpio_mode = MX6_PAD_GPIO1_IO02__GPIO1_IO_2 | PC,
		.gp = IMX_GPIO_NR(1, 2),
	},
	.sda = {
		.i2c_mode = MX6_PAD_GPIO1_IO03__I2C2_SDA | PC,
		.gpio_mode = MX6_PAD_GPIO1_IO03__GPIO1_IO_3 | PC,
		.gp = IMX_GPIO_NR(1, 3),
	},
};
#endif

#ifdef CONFIG_POWER
#define I2C_PMIC 0
int power_init_board(void)
{
	struct pmic *pfuze;
	unsigned int reg;
	int ret;

	pfuze = pfuze_common_init(I2C_PMIC);
	if (!pfuze)
		return -ENODEV;

	ret = pfuze_mode_init(pfuze, APS_PFM);
	if (ret < 0)
		return ret;

	/* set SW1AB staby volatage 0.975V */
	pmic_reg_read(pfuze, PFUZE100_SW1ABSTBY, &reg);
	reg &= ~0x3f;
	reg |= PFUZE100_SW1ABC_SETP(9750);
	pmic_reg_write(pfuze, PFUZE100_SW1ABSTBY, reg);

	/* set SW1AB/VDDARM step ramp up time from 16us to 4us/25mV */
	pmic_reg_read(pfuze, PFUZE100_SW1ABCONF, &reg);
	reg &= ~0xc0;
	reg |= 0x40;
	pmic_reg_write(pfuze, PFUZE100_SW1ABCONF, reg);

	/* set SW1C staby volatage 0.975V */
	pmic_reg_read(pfuze, PFUZE100_SW1CSTBY, &reg);
	reg &= ~0x3f;
	reg |= PFUZE100_SW1ABC_SETP(9750);
	pmic_reg_write(pfuze, PFUZE100_SW1CSTBY, reg);

	/* set SW1C/VDDSOC step ramp up time to from 16us to 4us/25mV */
	pmic_reg_read(pfuze, PFUZE100_SW1CCONF, &reg);
	reg &= ~0xc0;
	reg |= 0x40;
	pmic_reg_write(pfuze, PFUZE100_SW1CCONF, reg);

	return 0;
}

#ifdef CONFIG_LDO_BYPASS_CHECK
void ldo_mode_set(int ldo_bypass)
{
	unsigned int value;
	int is_400M;
	u32 vddarm;
	struct pmic *p = pmic_get("PFUZE100");

	if (!p) {
		printf("No PMIC found!\n");
		return;
	}

	/* switch to ldo_bypass mode */
	if (ldo_bypass) {
		prep_anatop_bypass();
		/* decrease VDDARM to 1.275V */
		pmic_reg_read(p, PFUZE100_SW1ABVOL, &value);
		value &= ~0x3f;
		value |= PFUZE100_SW1ABC_SETP(12750);
		pmic_reg_write(p, PFUZE100_SW1ABVOL, value);

		/* decrease VDDSOC to 1.3V */
		pmic_reg_read(p, PFUZE100_SW1CVOL, &value);
		value &= ~0x3f;
		value |= PFUZE100_SW1ABC_SETP(13000);
		pmic_reg_write(p, PFUZE100_SW1CVOL, value);

		is_400M = set_anatop_bypass(1);
		if (is_400M)
			vddarm = PFUZE100_SW1ABC_SETP(10750);
		else
			vddarm = PFUZE100_SW1ABC_SETP(11750);

		pmic_reg_read(p, PFUZE100_SW1ABVOL, &value);
		value &= ~0x3f;
		value |= vddarm;
		pmic_reg_write(p, PFUZE100_SW1ABVOL, value);

		pmic_reg_read(p, PFUZE100_SW1CVOL, &value);
		value &= ~0x3f;
		value |= PFUZE100_SW1ABC_SETP(11750);
		pmic_reg_write(p, PFUZE100_SW1CVOL, value);

		finish_anatop_bypass();
		printf("switch to ldo_bypass mode!\n");
	}

}
#endif

#elif defined(CONFIG_DM_PMIC_PFUZE100)
int power_init_board(void)
{
	struct udevice *dev;
	int ret;

	dev = pfuze_common_init();
	if (!dev)
		return -ENODEV;

	ret = pfuze_mode_init(dev, APS_PFM);
	if (ret < 0)
		return ret;

	return 0;
}

#ifdef CONFIG_LDO_BYPASS_CHECK
void ldo_mode_set(int ldo_bypass)
{
	struct udevice *dev;
	int ret;
	int is_400M;
	u32 vddarm;

	ret = pmic_get("pfuze100@8", &dev);
	if (ret == -ENODEV) {
		printf("No PMIC found!\n");
		return;
	}

	/* switch to ldo_bypass mode , boot on 800Mhz */
	if (ldo_bypass) {
		prep_anatop_bypass();

		/* decrease VDDARM for 400Mhz DQ:1.1V, DL:1.275V */
		pmic_clrsetbits(dev, PFUZE100_SW1ABVOL, 0x3f, PFUZE100_SW1ABC_SETP(12750));

		/* increase VDDSOC to 1.3V */
		pmic_clrsetbits(dev, PFUZE100_SW1CVOL, 0x3f, PFUZE100_SW1ABC_SETP(13000));

		is_400M = set_anatop_bypass(1);
		if (is_400M)
			vddarm = PFUZE100_SW1ABC_SETP(10750);
		else
			vddarm = PFUZE100_SW1ABC_SETP(11750);

		pmic_clrsetbits(dev, PFUZE100_SW1ABVOL, 0x3f, vddarm);

		/* decrease VDDSOC to 1.175V */
		pmic_clrsetbits(dev, PFUZE100_SW1CVOL, 0x3f, PFUZE100_SW1ABC_SETP(11750));

		finish_anatop_bypass();
		printf("switch to ldo_bypass mode!\n");
	}
}
#endif
#endif

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_SIZE;

	return 0;
}

static iomux_v3_cfg_t const uart1_pads[] = {
	MX6_PAD_GPIO1_IO04__UART1_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX6_PAD_GPIO1_IO05__UART1_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const usdhc1_pads[] = {
	MX6_PAD_SD1_CLK__USDHC1_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_CMD__USDHC1_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA0__USDHC1_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA1__USDHC1_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA2__USDHC1_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA3__USDHC1_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

#ifdef CONFIG_VIDEO_MXS
static iomux_v3_cfg_t const lvds_ctrl_pads[] = {
	/* CABC enable */
	MX6_PAD_KEY_ROW1__GPIO2_IO_16 | MUX_PAD_CTRL(NO_PAD_CTRL),

	/* Use GPIO for Brightness adjustment, duty cycle = period */
	MX6_PAD_GPIO1_IO12__GPIO1_IO_12 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static iomux_v3_cfg_t const lcd_pads[] = {
	MX6_PAD_LCD1_CLK__LCDIF1_CLK | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_ENABLE__LCDIF1_ENABLE | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_HSYNC__LCDIF1_HSYNC | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_VSYNC__LCDIF1_VSYNC | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA00__LCDIF1_DATA_0 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA01__LCDIF1_DATA_1 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA02__LCDIF1_DATA_2 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA03__LCDIF1_DATA_3 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA04__LCDIF1_DATA_4 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA05__LCDIF1_DATA_5 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA06__LCDIF1_DATA_6 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA07__LCDIF1_DATA_7 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA08__LCDIF1_DATA_8 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA09__LCDIF1_DATA_9 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA10__LCDIF1_DATA_10 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA11__LCDIF1_DATA_11 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA12__LCDIF1_DATA_12 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA13__LCDIF1_DATA_13 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA14__LCDIF1_DATA_14 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA15__LCDIF1_DATA_15 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA16__LCDIF1_DATA_16 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA17__LCDIF1_DATA_17 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA18__LCDIF1_DATA_18 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA19__LCDIF1_DATA_19 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA20__LCDIF1_DATA_20 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA21__LCDIF1_DATA_21 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA22__LCDIF1_DATA_22 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA23__LCDIF1_DATA_23 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_RESET__GPIO3_IO_27 | MUX_PAD_CTRL(NO_PAD_CTRL),

	/* Use GPIO for Brightness adjustment, duty cycle = period */
	MX6_PAD_GPIO1_IO12__GPIO1_IO_12 | MUX_PAD_CTRL(NO_PAD_CTRL),
};


struct lcd_panel_info_t {
	unsigned int lcdif_base_addr;
	int depth;
	void	(*enable)(struct lcd_panel_info_t const *dev);
	struct fb_videomode mode;
};

void do_enable_lvds(struct display_info_t const *dev)
{
	enable_lcdif_clock(dev->bus, 1);
	enable_lvds_bridge(dev->bus);

	SETUP_IOMUX_PADS(lvds_ctrl_pads);

	/* Enable CABC */
	gpio_request(IMX_GPIO_NR(2, 16), "cabc enable");
	gpio_direction_output(IMX_GPIO_NR(2, 16) , 1);

	/* Set Brightness to high */
	gpio_request(IMX_GPIO_NR(1, 12), "backlight");
	gpio_direction_output(IMX_GPIO_NR(1, 12) , 1);
}

void do_enable_parallel_lcd(struct display_info_t const *dev)
{
	enable_lcdif_clock(dev->bus, 1);

	SETUP_IOMUX_PADS(lcd_pads);

	/* Power up the LCD */
	gpio_request(IMX_GPIO_NR(3, 27), "lcd pwr");
	gpio_direction_output(IMX_GPIO_NR(3, 27) , 1);

	/* Set Brightness to high */
		gpio_request(IMX_GPIO_NR(1, 12), "backlight");
	gpio_direction_output(IMX_GPIO_NR(1, 12) , 1);
}

struct display_info_t const displays[] = {{
	.bus = LCDIF2_BASE_ADDR,
	.addr = 0,
	.pixfmt = 18,
	.enable	= do_enable_lvds,
	.detect = NULL,
	.mode	= {
		.name			= "Hannstar-XGA",
		.xres           = 1024,
		.yres           = 768,
		.pixclock       = 15385,
		.left_margin    = 220,
		.right_margin   = 40,
		.upper_margin   = 21,
		.lower_margin   = 7,
		.hsync_len      = 60,
		.vsync_len      = 10,
		.sync           = 0,
		.vmode          = FB_VMODE_NONINTERLACED
} }, {
	.bus = MX6SX_LCDIF1_BASE_ADDR,
	.pixfmt = 24,
	.addr = 0,
	.detect = NULL,
	.enable	= do_enable_parallel_lcd,
	.mode	= {
		.name			= "MCIMX28LCD",
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

#ifdef CONFIG_FEC_MXC
static iomux_v3_cfg_t const fec1_pads[] = {
	MX6_PAD_ENET1_MDC__ENET1_MDC | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_MDIO__ENET1_MDIO | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII1_RX_CTL__ENET1_RX_EN | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6_PAD_RGMII1_RD0__ENET1_RX_DATA_0 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6_PAD_RGMII1_RD1__ENET1_RX_DATA_1 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6_PAD_RGMII1_RD2__ENET1_RX_DATA_2 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6_PAD_RGMII1_RD3__ENET1_RX_DATA_3 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6_PAD_RGMII1_RXC__ENET1_RX_CLK | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6_PAD_RGMII1_TX_CTL__ENET1_TX_EN | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII1_TD0__ENET1_TX_DATA_0 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII1_TD1__ENET1_TX_DATA_1 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII1_TD2__ENET1_TX_DATA_2 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII1_TD3__ENET1_TX_DATA_3 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII1_TXC__ENET1_RGMII_TXC | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_TX_CLK__ENET1_TX_CLK | MUX_PAD_CTRL(ENET_PAD_CTRL),

	/* AR8031 PHY Reset. For arm2 board, silder the resistance */
	MX6_PAD_SD4_DATA4__GPIO6_IO_18 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static void setup_iomux_fec1(void)
{
	SETUP_IOMUX_PADS(fec1_pads);
}
#endif

static void setup_iomux_uart(void)
{
	SETUP_IOMUX_PADS(uart1_pads);
}

#ifdef CONFIG_FSL_QSPI
#ifndef CONFIG_DM_SPI
#define QSPI_PAD_CTRL1	\
		(PAD_CTL_SRE_FAST | PAD_CTL_SPEED_MED | \
		PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_47K_UP | PAD_CTL_DSE_60ohm)

#define QSPI_PAD_CTRL2 (QSPI_PAD_CTRL1 | PAD_CTL_DSE_34ohm)

static iomux_v3_cfg_t const quadspi_pads[] = {
	MX6_PAD_NAND_WP_B__QSPI2_A_DATA_0 | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6_PAD_NAND_READY_B__QSPI2_A_DATA_1 | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6_PAD_NAND_CE0_B__QSPI2_A_DATA_2 | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6_PAD_NAND_CE1_B__QSPI2_A_DATA_3 | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6_PAD_NAND_CLE__QSPI2_A_SCLK | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6_PAD_NAND_ALE__QSPI2_A_SS0_B | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6_PAD_NAND_DATA01__QSPI2_B_DATA_0	| MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6_PAD_NAND_DATA00__QSPI2_B_DATA_1	| MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6_PAD_NAND_WE_B__QSPI2_B_DATA_2		| MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6_PAD_NAND_RE_B__QSPI2_B_DATA_3		| MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6_PAD_NAND_DATA03__QSPI2_B_SS0_B	| MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6_PAD_NAND_DATA02__QSPI2_B_SCLK		| MUX_PAD_CTRL(QSPI_PAD_CTRL1),

};
#endif

int board_qspi_init(void)
{
#ifndef CONFIG_DM_SPI
	/* Set the iomux */
	SETUP_IOMUX_PADS(quadspi_pads);
#endif

	/* Set the clock */
	enable_qspi_clk(1);

	return 0;
}
#endif

#ifdef CONFIG_FSL_ESDHC_IMX
static struct fsl_esdhc_cfg usdhc_cfg[1] = {
	{USDHC1_BASE_ADDR, 0, 4},
};

int board_mmc_getcd(struct mmc *mmc)
{
	return 1;	/* Assume boot SD always present */
}
int board_mmc_init(bd_t *bis)
{
	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-boot device node)    (Physical Port)
	 * mmc0                    USDHC1 (SDA)
	 */
	SETUP_IOMUX_PADS(usdhc1_pads);

	usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
	return fsl_esdhc_initialize(bis, &usdhc_cfg[0]);
}
#endif

#ifdef CONFIG_MTD_NOR_FLASH
iomux_v3_cfg_t eimnor_pads[] = {
	MX6_PAD_QSPI1A_SCLK__WEIM_DATA_0   | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL2),
	MX6_PAD_QSPI1A_SS0_B__WEIM_DATA_1  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL2),
	MX6_PAD_QSPI1A_SS1_B__WEIM_DATA_2  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL2),
	MX6_PAD_QSPI1A_DATA3__WEIM_DATA_3  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL2),
	MX6_PAD_QSPI1A_DATA2__WEIM_DATA_4  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL2),
	MX6_PAD_QSPI1A_DATA1__WEIM_DATA_5  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL2),
	MX6_PAD_QSPI1A_DATA0__WEIM_DATA_6  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL2),
	MX6_PAD_QSPI1A_DQS__WEIM_DATA_7    | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL2),
	MX6_PAD_QSPI1B_SCLK__WEIM_DATA_8   | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL2),
	MX6_PAD_QSPI1B_SS0_B__WEIM_DATA_9  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL2),
	MX6_PAD_QSPI1B_SS1_B__WEIM_DATA_10 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL2),
	MX6_PAD_QSPI1B_DATA3__WEIM_DATA_11 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL2),
	MX6_PAD_QSPI1B_DATA2__WEIM_DATA_12 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL2),
	MX6_PAD_QSPI1B_DATA1__WEIM_DATA_13 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL2),
	MX6_PAD_QSPI1B_DATA0__WEIM_DATA_14 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL2),
	MX6_PAD_QSPI1B_DQS__WEIM_DATA_15   | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL2),

	MX6_PAD_NAND_DATA00__WEIM_AD_0     | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_NAND_DATA01__WEIM_AD_1     | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_NAND_DATA02__WEIM_AD_2     | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_NAND_DATA03__WEIM_AD_3     | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_NAND_DATA04__WEIM_AD_4     | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_NAND_DATA05__WEIM_AD_5     | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_NAND_DATA06__WEIM_AD_6     | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_NAND_DATA07__WEIM_AD_7     | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_LCD1_DATA08__WEIM_AD_8     | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_LCD1_DATA09__WEIM_AD_9     | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_LCD1_DATA10__WEIM_AD_10    | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_LCD1_DATA11__WEIM_AD_11    | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL) ,
	MX6_PAD_LCD1_DATA12__WEIM_AD_12    | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_LCD1_DATA13__WEIM_AD_13    | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_LCD1_DATA14__WEIM_AD_14    | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_LCD1_DATA15__WEIM_AD_15    | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_LCD1_DATA16__WEIM_ADDR_16  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_LCD1_DATA17__WEIM_ADDR_17  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_LCD1_DATA18__WEIM_ADDR_18  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_LCD1_DATA19__WEIM_ADDR_19  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_LCD1_DATA20__WEIM_ADDR_20  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_LCD1_DATA21__WEIM_ADDR_21  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_LCD1_DATA22__WEIM_ADDR_22  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_LCD1_DATA23__WEIM_ADDR_23  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_LCD1_DATA03__WEIM_ADDR_24  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_LCD1_DATA04__WEIM_ADDR_25  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_LCD1_DATA05__WEIM_ADDR_26  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),

	MX6_PAD_NAND_CE1_B__WEIM_OE        | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_NAND_RE_B__WEIM_RW         | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_NAND_WE_B__WEIM_WAIT       | MUX_PAD_CTRL(NO_PAD_CTRL),

	MX6_PAD_NAND_ALE__WEIM_CS0_B       | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
};
static void eimnor_cs_setup(void)
{
	writel(0x00000120, WEIM_BASE_ADDR + 0x090);
	writel(0x00010181, WEIM_BASE_ADDR + 0x000);
	writel(0x00000001, WEIM_BASE_ADDR + 0x004);
	writel(0x0a020000, WEIM_BASE_ADDR + 0x008);
	writel(0x0000c000, WEIM_BASE_ADDR + 0x00c);
	writel(0x0804a240, WEIM_BASE_ADDR + 0x010);
}

static void setup_eimnor(void)
{
	SETUP_IOMUX_PADS(eimnor_pads);

	eimnor_cs_setup();
}
#endif

#ifdef CONFIG_NAND_MXS
iomux_v3_cfg_t gpmi_pads[] = {
	MX6_PAD_NAND_CLE__RAWNAND_CLE		| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_ALE__RAWNAND_ALE		| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_WP_B__RAWNAND_WP_B	| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_READY_B__RAWNAND_READY_B	| MUX_PAD_CTRL(GPMI_PAD_CTRL0),
	MX6_PAD_NAND_CE0_B__RAWNAND_CE0_B		| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_RE_B__RAWNAND_RE_B		| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_WE_B__RAWNAND_WE_B		| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_DATA00__RAWNAND_DATA00	| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_DATA01__RAWNAND_DATA01	| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_DATA02__RAWNAND_DATA02	| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_DATA03__RAWNAND_DATA03	| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_DATA04__RAWNAND_DATA04	| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_DATA05__RAWNAND_DATA05	| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_DATA06__RAWNAND_DATA06	| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_DATA07__RAWNAND_DATA07	| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
};

static void setup_gpmi_nand(void)
{
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;

	/* config gpmi nand iomux */
	SETUP_IOMUX_PADS(gpmi_pads);

	setup_gpmi_io_clk((MXC_CCM_CS2CDR_QSPI2_CLK_PODF(0) |
			MXC_CCM_CS2CDR_QSPI2_CLK_PRED(3) |
			MXC_CCM_CS2CDR_QSPI2_CLK_SEL(3)));

	/* enable apbh clock gating */
	setbits_le32(&mxc_ccm->CCGR0, MXC_CCM_CCGR0_APBHDMA_MASK);
}
#endif

#ifdef CONFIG_FEC_MXC

#define MAX7322_I2C_ADDR		0x68
#define MAX7322_I2C_BUS		1

static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *const iomuxc_gpr_regs
		= (struct iomuxc_gpr_base_regs *) IOMUXC_GPR_BASE_ADDR;
	int ret;
	unsigned char value = 1;

	/* clear gpr1[13], gpr1[17] to select anatop clock */
	clrsetbits_le32(&iomuxc_gpr_regs->gpr[1], IOMUX_GPR1_FEC1_MASK, 0);

	ret = enable_fec_anatop_clock(0, ENET_125MHZ);
	if (ret)
		return ret;

	/* Reset AR8031 PHY */
	gpio_request(IMX_GPIO_NR(6, 18), "ar8031 reset");
	gpio_direction_output(IMX_GPIO_NR(6, 18) , 0);
	udelay(500);
	gpio_set_value(IMX_GPIO_NR(6, 18), 1);

#ifdef CONFIG_DM_I2C
	struct udevice *bus, *dev;
	ret = uclass_get_device_by_seq(UCLASS_I2C, MAX7322_I2C_BUS - 1, &bus);
	if (ret) {
		printf("Get i2c bus %u failed, ret = %d\n", MAX7322_I2C_BUS - 1, ret);
		return ret;
	}

	ret = dm_i2c_probe(bus, MAX7322_I2C_ADDR, 0, &dev);
	if (ret) {
		printf("MAX7322 Not found, ret = %d\n", ret);
		return ret;
	}

	/* Write 0x1 to enable O0 output, this device has no addr */
	/* hence addr length is 0 */
	value = 0x1;
	ret = dm_i2c_write(dev, 0, &value, 1);
	if (ret) {
		printf("MAX7322 write failed, ret = %d\n", ret);
		return ret;
	}
#else
	/* This is needed to drive the pads to 1.8V instead of 1.5V */
	i2c_set_bus_num(MAX7322_I2C_BUS);

	if (!i2c_probe(MAX7322_I2C_ADDR)) {
		/* Write 0x1 to enable O0 output, this device has no addr */
		/* hence addr length is 0 */
		value = 0x1;
		if (i2c_write(MAX7322_I2C_ADDR, 0, 0, &value, 1))
			printf("MAX7322 write failed\n");
	} else {
		printf("MAX7322 Not found\n");
	}
#endif

	return 0;
}

int board_eth_init(bd_t *bis)
{
	int ret;

	setup_iomux_fec1();

	ret = fecmxc_initialize_multi(bis, 0,
		CONFIG_FEC_MXC_PHYADDR, IMX_FEC_BASE);
	if (ret)
		printf("FEC1 MXC: %s:failed\n", __func__);

	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
#ifdef CONFIG_FEC_ENABLE_MAX7322
	/* Enable 1.8V(SEL_1P5_1P8_POS_REG) on
	   Phy control debug reg 0 */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x1f);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x8);
#endif

	/* rgmii tx clock delay enable */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x100);

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}
#endif

int board_early_init_f(void)
{
	setup_iomux_uart();
	return 0;
}

#ifdef CONFIG_USB_EHCI_MX6
#ifndef CONFIG_DM_USB

#define USB_OTHERREGS_OFFSET	0x800
#define UCTRL_PWR_POL		(1 << 9)

iomux_v3_cfg_t const usb_otg_pads[] = {
	MX6_PAD_GPIO1_IO09__USB_OTG1_PWR | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX6_PAD_GPIO1_IO10__ANATOP_OTG1_ID | MUX_PAD_CTRL(OTG_ID_PAD_CTRL),
	MX6_PAD_GPIO1_IO12__USB_OTG2_PWR | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static void setup_usb(void)
{
	SETUP_IOMUX_PADS(usb_otg_pads);
}

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

	if (port >= 1)
		return -EINVAL;

	usbnc_usb_ctrl = (u32 *)(USB_BASE_ADDR + USB_OTHERREGS_OFFSET +
				 port * 4);

	/* Set Power polarity */
	setbits_le32(usbnc_usb_ctrl, UCTRL_PWR_POL);

	return 0;
}
#endif
#endif

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

#ifdef CONFIG_SYS_I2C
	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info1);
	setup_i2c(1, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info2);
#endif

#ifdef CONFIG_FEC_MXC
	setup_fec();
#endif

#ifdef CONFIG_MTD_NOR_FLASH
	setup_eimnor();
#endif

#ifdef CONFIG_NAND_MXS
	setup_gpmi_nand();
#endif

#ifdef CONFIG_USB_EHCI_MX6
#ifndef CONFIG_DM_USB
	setup_usb();
#endif
#endif

#ifdef CONFIG_FSL_QSPI
	board_qspi_init();
#endif

	return 0;
}

#ifdef CONFIG_CMD_BMODE
static const struct boot_mode board_boot_modes[] = {
	/* 4 bit bus width */
	{"sd1", MAKE_CFGVAL(0x40, 0x20, 0x00, 0x00)},
	{"qspi2", MAKE_CFGVAL(0x18, 0x00, 0x00, 0x00)},
	{"spinor", MAKE_CFGVAL(0x30, 0x00, 0x00, 0x0B)},
	{"eimnor", MAKE_CFGVAL(0x00, 0x80, 0x00, 0x00)},
	{NULL,   0},
};
#endif

int board_late_init(void)
{
#ifdef CONFIG_CMD_BMODE
	add_board_boot_modes(board_boot_modes);
#endif

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

	return 0;
}

u32 get_board_rev(void)
{
	return get_cpu_rev();
}

int checkboard(void)
{
	puts("Board: MX6SX 19x19 ARM2\n");

	return 0;
}
