// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2015 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 */

#include <init.h>
#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/mx7-pins.h>
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
#include <power/pmic.h>
#include <power/pfuze3000_pmic.h>
#include "../common/pfuze.h"
#include <i2c.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <asm/arch/crm_regs.h>
#if defined(CONFIG_MXC_EPDC)
#include <lcd.h>
#include <mxc_epdc_fb.h>
#endif
#include <asm/mach-imx/video.h>

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL  (PAD_CTL_DSE_3P3V_49OHM | \
	PAD_CTL_PUS_PU100KOHM | PAD_CTL_HYS)

#define LCD_PAD_CTRL    (PAD_CTL_HYS | PAD_CTL_PUS_PU100KOHM | \
	PAD_CTL_DSE_3P3V_49OHM)

#define NAND_PAD_CTRL (PAD_CTL_DSE_3P3V_49OHM | PAD_CTL_SRE_SLOW | PAD_CTL_HYS)

#define SPI_PAD_CTRL \
  (PAD_CTL_HYS | PAD_CTL_DSE_3P3V_49OHM | PAD_CTL_SRE_FAST)

#define NAND_PAD_READY0_CTRL (PAD_CTL_DSE_3P3V_49OHM | PAD_CTL_PUS_PU5KOHM)

#define EPDC_PAD_CTRL	0x0

#ifdef CONFIG_MXC_SPI
static iomux_v3_cfg_t const ecspi3_pads[] = {
    MX7D_PAD_SAI2_RX_DATA__ECSPI3_SCLK | MUX_PAD_CTRL(SPI_PAD_CTRL),
    MX7D_PAD_SAI2_TX_SYNC__ECSPI3_MISO | MUX_PAD_CTRL(SPI_PAD_CTRL),
    MX7D_PAD_SAI2_TX_BCLK__ECSPI3_MOSI | MUX_PAD_CTRL(SPI_PAD_CTRL),
    MX7D_PAD_SAI2_TX_DATA__GPIO6_IO22 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

int board_spi_cs_gpio(unsigned bus, unsigned cs)
{
         return (bus == 2 && cs == 0) ? (IMX_GPIO_NR(6, 22)) : -1;
}

static void setup_spi(void)
{
         imx_iomux_v3_setup_multiple_pads(ecspi3_pads, ARRAY_SIZE(ecspi3_pads));
}
#endif

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_SIZE;

	return 0;
}

static iomux_v3_cfg_t const wdog_pads[] = {
	MX7D_PAD_GPIO1_IO00__WDOG1_WDOG_B | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static iomux_v3_cfg_t const uart1_pads[] = {
	MX7D_PAD_UART1_TX_DATA__UART1_DCE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX7D_PAD_UART1_RX_DATA__UART1_DCE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

#define BOARD_REV_C  0x300
#define BOARD_REV_B  0x200
#define BOARD_REV_A  0x100

static int mx7sabre_rev(void)
{
	/*
	 * Get Board ID information from OCOTP_GP1[15:8]
	 * i.MX7D SDB RevA: 0x41
	 * i.MX7D SDB RevB: 0x42
	 */
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	struct fuse_bank *bank = &ocotp->bank[14];
	int reg = readl(&bank->fuse_regs[0]);
	int ret;

	if (reg != 0) {
		switch (reg >> 8 & 0x0F) {
		case 0x3:
			ret = BOARD_REV_C;
			break;
		case 0x02:
			ret = BOARD_REV_B;
			break;
		case 0x01:
		default:
			ret = BOARD_REV_A;
			break;
		}
	} else {
		/* If the gp1 fuse is not burn, we have to use TO rev for the board rev */
		if (is_soc_rev(CHIP_REV_1_0))
			ret = BOARD_REV_A;
		else if (is_soc_rev(CHIP_REV_1_1))
			ret = BOARD_REV_B;
		else
			ret = BOARD_REV_C;
	}

	return ret;
}

u32 get_board_rev(void)
{
	int rev = mx7sabre_rev();

	return (get_cpu_rev() & ~(0xF << 8)) | rev;
}

#ifdef CONFIG_NAND_MXS
static iomux_v3_cfg_t const gpmi_pads[] = {
	MX7D_PAD_SD3_DATA0__NAND_DATA00 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA1__NAND_DATA01 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA2__NAND_DATA02 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA3__NAND_DATA03 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA4__NAND_DATA04 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA5__NAND_DATA05 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA6__NAND_DATA06 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA7__NAND_DATA07 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_CLK__NAND_CLE	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_CMD__NAND_ALE	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_STROBE__NAND_RE_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_RESET_B__NAND_WE_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_MCLK__NAND_WP_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_RX_BCLK__NAND_CE3_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_RX_SYNC__NAND_CE2_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_RX_DATA__NAND_CE1_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_TX_BCLK__NAND_CE0_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_TX_SYNC__NAND_DQS	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_TX_DATA__NAND_READY_B	| MUX_PAD_CTRL(NAND_PAD_READY0_CTRL),
};

static void setup_gpmi_nand(void)
{
	imx_iomux_v3_setup_multiple_pads(gpmi_pads, ARRAY_SIZE(gpmi_pads));

	/* NAND_USDHC_BUS_CLK is set in rom */
	set_clk_nand();
}
#endif

#ifdef CONFIG_DM_VIDEO
static iomux_v3_cfg_t const lcd_pads[] = {
	MX7D_PAD_LCD_RESET__GPIO3_IO4	| MUX_PAD_CTRL(LCD_PAD_CTRL),
};

static iomux_v3_cfg_t const pwm_pads[] = {
	/* Use GPIO for Brightness adjustment, duty cycle = period */
	MX7D_PAD_GPIO1_IO01__GPIO1_IO1 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static int setup_lcd(void)
{
	int ret;
	struct gpio_desc desc;

	imx_iomux_v3_setup_multiple_pads(lcd_pads, ARRAY_SIZE(lcd_pads));

	imx_iomux_v3_setup_multiple_pads(pwm_pads, ARRAY_SIZE(pwm_pads));

	/* Reset LCD */
	ret = dm_gpio_lookup_name("GPIO3_4", &desc);
	if (ret) {
		printf("%s lookup GPIO3_4 failed ret = %d\n", __func__, ret);
		return -ENODEV;
	}

	ret = dm_gpio_request(&desc, "lcd reset");
	if (ret) {
		printf("%s request lcd reset failed ret = %d\n", __func__, ret);
		return -ENODEV;
	}

	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT);
	dm_gpio_set_value(&desc, 0);
	udelay(500);
	dm_gpio_set_value(&desc, 1);

	/* Set Brightness to high */
	ret = dm_gpio_lookup_name("GPIO1_1", &desc);
	if (ret) {
		printf("%s lookup GPIO1_1 failed ret = %d\n", __func__, ret);
		return -ENODEV;
	}

	ret = dm_gpio_request(&desc, "lcd backlight");
	if (ret) {
		printf("%s request lcd backlight failed ret = %d\n", __func__, ret);
		return -ENODEV;
	}

	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);

	return 0;
}
#else
static inline int setup_lcd(void) { return 0; }
#endif

static void setup_iomux_uart(void)
{
	imx_iomux_v3_setup_multiple_pads(uart1_pads, ARRAY_SIZE(uart1_pads));
}

#ifdef CONFIG_IMX_BOOTAUX
ulong board_get_usable_ram_top(ulong total_size)
{
	/* Reserve top 1M memory used by M core vring/buffer */
	return gd->ram_top - SZ_1M;
}
#endif

#ifdef CONFIG_FEC_MXC
static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *const iomuxc_gpr_regs
		= (struct iomuxc_gpr_base_regs *) IOMUXC_GPR_BASE_ADDR;

	/* Use 125M anatop REF_CLK1 for ENET1, clear gpr1[13], gpr1[17]*/
	clrsetbits_le32(&iomuxc_gpr_regs->gpr[1],
		(IOMUXC_GPR_GPR1_GPR_ENET1_TX_CLK_SEL_MASK |
		 IOMUXC_GPR_GPR1_GPR_ENET1_CLK_DIR_MASK), 0);

	clrsetbits_le32(&iomuxc_gpr_regs->gpr[1],
			(IOMUXC_GPR_GPR1_GPR_ENET2_TX_CLK_SEL_MASK |
			 IOMUXC_GPR_GPR1_GPR_ENET2_CLK_DIR_MASK), 0);

	return set_clk_enet(ENET_125MHZ);
}

int board_phy_config(struct phy_device *phydev)
{
	/* enable rgmii rxc skew and phy mode select to RGMII copper */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x21);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1f, 0x7ea8);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x2f);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1f, 0x71b7);

	if (phydev->drv->config)
		phydev->drv->config(phydev);
	return 0;
}
#endif

#ifdef CONFIG_FSL_QSPI
int board_qspi_init(void)
{
	/* Set the clock */
	set_clk_qspi();

	return 0;
}
#endif

#ifdef CONFIG_MXC_EPDC
iomux_v3_cfg_t const epdc_en_pads[] = {
	MX7D_PAD_GPIO1_IO04__GPIO1_IO4 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static iomux_v3_cfg_t const epdc_enable_pads[] = {
	MX7D_PAD_EPDC_DATA00__EPDC_DATA0	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX7D_PAD_EPDC_DATA01__EPDC_DATA1	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX7D_PAD_EPDC_DATA02__EPDC_DATA2	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX7D_PAD_EPDC_DATA03__EPDC_DATA3	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX7D_PAD_EPDC_DATA04__EPDC_DATA4	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX7D_PAD_EPDC_DATA05__EPDC_DATA5	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX7D_PAD_EPDC_DATA06__EPDC_DATA6	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX7D_PAD_EPDC_DATA07__EPDC_DATA7	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX7D_PAD_EPDC_SDCLK__EPDC_SDCLK		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX7D_PAD_EPDC_SDLE__EPDC_SDLE		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX7D_PAD_EPDC_SDOE__EPDC_SDOE		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX7D_PAD_EPDC_SDSHR__EPDC_SDSHR		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX7D_PAD_EPDC_SDCE0__EPDC_SDCE0		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX7D_PAD_EPDC_SDCE1__EPDC_SDCE1		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX7D_PAD_EPDC_GDCLK__EPDC_GDCLK		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX7D_PAD_EPDC_GDOE__EPDC_GDOE		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX7D_PAD_EPDC_GDRL__EPDC_GDRL		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX7D_PAD_EPDC_GDSP__EPDC_GDSP		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX7D_PAD_EPDC_BDR0__EPDC_BDR0		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX7D_PAD_EPDC_BDR1__EPDC_BDR1		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
};

static iomux_v3_cfg_t const epdc_disable_pads[] = {
	MX7D_PAD_EPDC_DATA00__GPIO2_IO0,
	MX7D_PAD_EPDC_DATA01__GPIO2_IO1,
	MX7D_PAD_EPDC_DATA02__GPIO2_IO2,
	MX7D_PAD_EPDC_DATA03__GPIO2_IO3,
	MX7D_PAD_EPDC_DATA04__GPIO2_IO4,
	MX7D_PAD_EPDC_DATA05__GPIO2_IO5,
	MX7D_PAD_EPDC_DATA06__GPIO2_IO6,
	MX7D_PAD_EPDC_DATA07__GPIO2_IO7,
	MX7D_PAD_EPDC_SDCLK__GPIO2_IO16,
	MX7D_PAD_EPDC_SDLE__GPIO2_IO17,
	MX7D_PAD_EPDC_SDOE__GPIO2_IO18,
	MX7D_PAD_EPDC_SDSHR__GPIO2_IO19,
	MX7D_PAD_EPDC_SDCE0__GPIO2_IO20,
	MX7D_PAD_EPDC_SDCE1__GPIO2_IO21,
	MX7D_PAD_EPDC_GDCLK__GPIO2_IO24,
	MX7D_PAD_EPDC_GDOE__GPIO2_IO25,
	MX7D_PAD_EPDC_GDRL__GPIO2_IO26,
	MX7D_PAD_EPDC_GDSP__GPIO2_IO27,
	MX7D_PAD_EPDC_BDR0__GPIO2_IO28,
	MX7D_PAD_EPDC_BDR1__GPIO2_IO29,
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

struct gpio_desc epd_pwrstat_desc;
struct gpio_desc epd_vcom_desc;
struct gpio_desc epd_wakeup_desc;
struct gpio_desc epd_pwr_ctl0_desc;

static void setup_epdc_power(void)
{
	int ret;

	/* IOMUX_GPR1: bit30: Disable On-chip RAM EPDC Function */
	struct iomuxc_gpr_base_regs *const iomuxc_gpr_regs
		= (struct iomuxc_gpr_base_regs *) IOMUXC_GPR_BASE_ADDR;

	clrsetbits_le32(&iomuxc_gpr_regs->gpr[1],
		IOMUXC_GPR_GPR1_GPR_ENABLE_OCRAM_EPDC_MASK, 0);

	/* Setup epdc voltage */

	/* EPDC_PWRSTAT - GPIO2[31] for PWR_GOOD status */
	ret = dm_gpio_lookup_name("GPIO2_31", &epd_pwrstat_desc);
	if (ret) {
		printf("%s lookup GPIO2_31 failed ret = %d\n", __func__, ret);
		return;
	}

	ret = dm_gpio_request(&epd_pwrstat_desc, "epdc_pwrstat");
	if (ret) {
		printf("%s request epdc_pwrstat failed ret = %d\n", __func__, ret);
		return;
	}

	dm_gpio_set_dir_flags(&epd_pwrstat_desc, GPIOD_IS_IN);

	/* EPDC_VCOM0 - GPIO4[14] for VCOM control */
	/* Set as output */
	ret = dm_gpio_lookup_name("GPIO4_14", &epd_vcom_desc);
	if (ret) {
		printf("%s lookup GPIO4_14 failed ret = %d\n", __func__, ret);
		return;
	}

	ret = dm_gpio_request(&epd_vcom_desc, "epdc_vcom");
	if (ret) {
		printf("%s request epdc_vcom failed ret = %d\n", __func__, ret);
		return;
	}

	dm_gpio_set_dir_flags(&epd_vcom_desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);

	/* EPDC_PWRWAKEUP - GPIO2[23] for EPD PMIC WAKEUP */
	/* Set as output */
	ret = dm_gpio_lookup_name("GPIO2_23", &epd_wakeup_desc);
	if (ret) {
		printf("%s lookup GPIO2_23 failed ret = %d\n", __func__, ret);
		return;
	}

	ret = dm_gpio_request(&epd_wakeup_desc, "epdc_pmic");
	if (ret) {
		printf("%s request epdc_pmic failed ret = %d\n", __func__, ret);
		return;
	}

	dm_gpio_set_dir_flags(&epd_wakeup_desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);

	/* EPDC_PWRCTRL0 - GPIO2[30] for EPD PWR CTL0 */
	/* Set as output */
	ret = dm_gpio_lookup_name("GPIO2_30", &epd_pwr_ctl0_desc);
	if (ret) {
		printf("%s lookup GPIO2_30 failed ret = %d\n", __func__, ret);
		return;
	}

	ret = dm_gpio_request(&epd_pwr_ctl0_desc, "epdc_pwr_ctl0");
	if (ret) {
		printf("%s request epdc_pwr_ctl0 failed ret = %d\n", __func__, ret);
		return;
	}

	dm_gpio_set_dir_flags(&epd_pwr_ctl0_desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);
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

	/* EPDC_PWRSTAT - GPIO2[31] for PWR_GOOD status */
	imx_iomux_v3_setup_pad(MX7D_PAD_EPDC_PWR_STAT__GPIO2_IO31 |
				MUX_PAD_CTRL(EPDC_PAD_CTRL));

	/* EPDC_VCOM0 - GPIO4[14] for VCOM control */
	imx_iomux_v3_setup_pad(MX7D_PAD_I2C4_SCL__GPIO4_IO14 |
				MUX_PAD_CTRL(EPDC_PAD_CTRL));

	/* EPDC_PWRWAKEUP - GPIO4[23] for EPD PMIC WAKEUP */
	imx_iomux_v3_setup_pad(MX7D_PAD_EPDC_SDCE3__GPIO2_IO23 |
				MUX_PAD_CTRL(EPDC_PAD_CTRL));

	/* EPDC_PWRCTRL0 - GPIO4[20] for EPD PWR CTL0 */
	imx_iomux_v3_setup_pad(MX7D_PAD_EPDC_PWR_COM__GPIO2_IO30 |
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
	dm_gpio_set_value(&epd_pwr_ctl0_desc, 1);
	udelay(1000);

	/* Enable epdc signal pin */
	epdc_enable_pins();

	/* Set PMIC Wakeup to high - enable Display power */
	dm_gpio_set_value(&epd_wakeup_desc, 1);

	/* Wait for PWRGOOD == 1 */
	while (1) {
		reg = readl(&gpio_regs->gpio_psr);
		if (!(reg & (1 << 31)))
			break;

		udelay(100);
	}

	/* Enable VCOM */
	dm_gpio_set_value(&epd_vcom_desc, 1);

	udelay(500);
}

void epdc_power_off(void)
{
	/* Set PMIC Wakeup to low - disable Display power */
	dm_gpio_set_value(&epd_wakeup_desc, 0);

	/* Disable VCOM */
	dm_gpio_set_value(&epd_vcom_desc, 0);

	epdc_disable_pins();

	/* Set EPD_PWR_CTL0 to low - disable EINK_VDD (3.15) */
	dm_gpio_set_value(&epd_pwr_ctl0_desc, 0);
}
#endif

int board_early_init_f(void)
{
	setup_iomux_uart();

	return 0;
}

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

#ifdef CONFIG_FEC_MXC
	setup_fec();
#endif

#ifdef CONFIG_NAND_MXS
	setup_gpmi_nand();
#endif

#ifdef CONFIG_FSL_QSPI
	board_qspi_init();
#endif

#ifdef CONFIG_MXC_EPDC
	if (mx7sabre_rev() >= BOARD_REV_B) {
		int ret;
		struct gpio_desc desc;
		/*
		 * From RevB, GPIO1_IO04 is used for ENET2 EN,
		 * so set its output to high to isolate the
		 * ENET2 signals for EPDC
		 */
		imx_iomux_v3_setup_multiple_pads(epdc_en_pads,
			ARRAY_SIZE(epdc_en_pads));

		ret = dm_gpio_lookup_name("GPIO1_4", &desc);
		if (ret) {
			printf("%s lookup GPIO1_4 failed ret = %d\n", __func__, ret);
			return -ENODEV;
		}

		ret = dm_gpio_request(&desc, "epdc_en");
		if (ret) {
			printf("%s request epdc_en failed ret = %d\n", __func__, ret);
			return -ENODEV;
		}

		dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);
	}
	setup_epdc();
#endif

#ifdef CONFIG_MXC_SPI
       setup_spi();
#endif

	return 0;
}


#ifdef CONFIG_DM_PMIC
int power_init_board(void)
{
	struct udevice *dev;
	int ret, dev_id, rev_id;
	u32 sw3mode;

	ret = pmic_get("pfuze3000@8", &dev);
	if (ret == -ENODEV)
		return 0;
	if (ret != 0)
		return ret;

	dev_id = pmic_reg_read(dev, PFUZE3000_DEVICEID);
	rev_id = pmic_reg_read(dev, PFUZE3000_REVID);
	printf("PMIC: PFUZE3000 DEV_ID=0x%x REV_ID=0x%x\n", dev_id, rev_id);

	pmic_clrsetbits(dev, PFUZE3000_LDOGCTL, 0, 1);

	/*
	 * Set the voltage of VLDO4 output to 2.8V which feeds
	 * the MIPI DSI and MIPI CSI inputs.
	 */
	pmic_clrsetbits(dev, PFUZE3000_VLD4CTL, 0xF, 0xA);

	/* change sw3 mode to avoid DDR power off */
	sw3mode = pmic_reg_read(dev, PFUZE3000_SW3MODE);
	ret = pmic_reg_write(dev, PFUZE3000_SW3MODE, sw3mode | 0x20);
	if (ret < 0)
		printf("PMIC: PFUZE3000 change sw3 mode failed\n");

	return 0;
}
#endif

int board_late_init(void)
{
	struct wdog_regs *wdog = (struct wdog_regs *)WDOG1_BASE_ADDR;

	env_set("tee", "no");
#ifdef CONFIG_IMX_OPTEE
	env_set("tee", "yes");
#endif

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

	setup_lcd();

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));

	set_wdog_reset(wdog);

	return 0;
}

int checkboard(void)
{
	int rev = mx7sabre_rev();
	char *mode;
	char *revname;

	if (IS_ENABLED(CONFIG_ARMV7_BOOT_SEC_DEFAULT))
		mode = "secure";
	else
		mode = "non-secure";

	switch (rev) {
	case BOARD_REV_C:
		revname = "C";
		break;
	case BOARD_REV_B:
		revname = "B";
		break;
	case BOARD_REV_A:
	default:
		revname = "A";
		break;
	}

	printf("Board: i.MX7D SABRESD Rev%s in %s mode\n", revname, mode);

	return 0;
}
