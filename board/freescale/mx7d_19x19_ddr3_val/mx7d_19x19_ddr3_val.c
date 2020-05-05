/*
 * Copyright (C) 2014-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

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
#include <netdev.h>
#include <power/pmic.h>
#include <power/pfuze3000_pmic.h>
#include "../common/pfuze.h"
#ifdef CONFIG_SYS_I2C_MXC
#include <i2c.h>
#include <asm/mach-imx/mxc_i2c.h>
#endif
#include <asm/arch/crm_regs.h>
#include <asm/mach-imx/video.h>

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL  (PAD_CTL_DSE_3P3V_49OHM | \
	PAD_CTL_PUS_PU100KOHM | PAD_CTL_HYS)

#define USDHC_PAD_CTRL (PAD_CTL_DSE_3P3V_32OHM | PAD_CTL_SRE_SLOW | \
	PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PUS_PU47KOHM)

#define ENET_PAD_CTRL  (PAD_CTL_PUS_PU100KOHM | PAD_CTL_DSE_3P3V_98OHM)
#define ENET_PAD_CTRL_MII  (PAD_CTL_PUS_PU100KOHM | PAD_CTL_DSE_3P3V_98OHM)

#define ENET_RX_PAD_CTRL  (PAD_CTL_PUS_PU100KOHM | PAD_CTL_DSE_3P3V_98OHM)

#define I2C_PAD_CTRL    (PAD_CTL_DSE_3P3V_32OHM | PAD_CTL_SRE_SLOW | \
	PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PUS_PU100KOHM)

#define LCD_PAD_CTRL    (PAD_CTL_HYS | PAD_CTL_PUS_PU100KOHM | \
	PAD_CTL_DSE_3P3V_49OHM)

#define QSPI_PAD_CTRL	\
		(PAD_CTL_DSE_3P3V_49OHM | PAD_CTL_PUE | PAD_CTL_PUS_PU47KOHM)

#define SPI_PAD_CTRL (PAD_CTL_DSE_3P3V_49OHM | PAD_CTL_SRE_SLOW | PAD_CTL_HYS)

#ifdef CONFIG_SYS_I2C
#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)
/* I2C1 for PMIC */
struct i2c_pads_info i2c_pad_info1 = {
	.scl = {
		.i2c_mode = MX7D_PAD_I2C1_SCL__I2C1_SCL | PC,
		.gpio_mode = MX7D_PAD_I2C1_SCL__GPIO4_IO8 | PC,
		.gp = IMX_GPIO_NR(4, 8),
	},
	.sda = {
		.i2c_mode = MX7D_PAD_I2C1_SDA__I2C1_SDA | PC,
		.gpio_mode = MX7D_PAD_I2C1_SDA__GPIO4_IO9 | PC,
		.gp = IMX_GPIO_NR(4, 9),
	},
};

/* I2C2 */
struct i2c_pads_info i2c_pad_info2 = {
	.scl = {
		.i2c_mode = MX7D_PAD_I2C2_SCL__I2C2_SCL | PC,
		.gpio_mode = MX7D_PAD_I2C2_SCL__GPIO4_IO10 | PC,
		.gp = IMX_GPIO_NR(4, 10),
	},
	.sda = {
		.i2c_mode = MX7D_PAD_I2C2_SDA__I2C2_SDA | PC,
		.gpio_mode = MX7D_PAD_I2C2_SDA__GPIO4_IO11 | PC,
		.gp = IMX_GPIO_NR(4, 11),
	},
};
#endif

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_SIZE;

	return 0;
}

static iomux_v3_cfg_t const uart1_pads[] = {
	MX7D_PAD_UART1_TX_DATA__UART1_DCE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX7D_PAD_UART1_RX_DATA__UART1_DCE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const usdhc1_emmc_pads[] = {
	MX7D_PAD_SD1_CLK__SD1_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_CMD__SD1_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_DATA0__SD1_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_DATA1__SD1_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_DATA2__SD1_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_DATA3__SD1_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_ECSPI2_SCLK__SD1_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_ECSPI2_MOSI__SD1_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_ECSPI2_MISO__SD1_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_ECSPI2_SS0__SD1_DATA7  | MUX_PAD_CTRL(USDHC_PAD_CTRL),

	MX7D_PAD_SD1_CD_B__GPIO5_IO0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_RESET_B__GPIO5_IO2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

static iomux_v3_cfg_t const usdhc2_pads[] = {
	MX7D_PAD_SD2_CLK__SD2_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD2_CMD__SD2_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD2_DATA0__SD2_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD2_DATA1__SD2_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD2_DATA2__SD2_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD2_DATA3__SD2_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),

	MX7D_PAD_GPIO1_IO12__SD2_VSELECT | MUX_PAD_CTRL(USDHC_PAD_CTRL),

	MX7D_PAD_SD2_CD_B__GPIO5_IO9 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD2_RESET_B__GPIO5_IO11 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

static iomux_v3_cfg_t const usdhc3_pads[] = {
	MX7D_PAD_SD3_CLK__SD3_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_CMD__SD3_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA0__SD3_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA1__SD3_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA2__SD3_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA3__SD3_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA4__SD3_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA5__SD3_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA6__SD3_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA7__SD3_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),

	MX7D_PAD_GPIO1_IO13__SD3_VSELECT | MUX_PAD_CTRL(USDHC_PAD_CTRL),

	MX7D_PAD_GPIO1_IO14__GPIO1_IO14  | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_RESET_B__GPIO6_IO11 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};


#ifdef CONFIG_VIDEO_MXS
static iomux_v3_cfg_t const lcd_pads[] = {
	MX7D_PAD_LCD_CLK__LCD_CLK | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_ENABLE__LCD_ENABLE | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_HSYNC__LCD_HSYNC | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_VSYNC__LCD_VSYNC | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA00__LCD_DATA0 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA01__LCD_DATA1 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA02__LCD_DATA2 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA03__LCD_DATA3 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA04__LCD_DATA4 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA05__LCD_DATA5 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA06__LCD_DATA6 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA07__LCD_DATA7 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA08__LCD_DATA8 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA09__LCD_DATA9 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA10__LCD_DATA10 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA11__LCD_DATA11 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA12__LCD_DATA12 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA13__LCD_DATA13 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA14__LCD_DATA14 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA15__LCD_DATA15 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA16__LCD_DATA16 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA17__LCD_DATA17 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA18__LCD_DATA18 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA19__LCD_DATA19 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA20__LCD_DATA20 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA21__LCD_DATA21 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA22__LCD_DATA22 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA23__LCD_DATA23 | MUX_PAD_CTRL(LCD_PAD_CTRL),

	MX7D_PAD_LCD_RESET__GPIO3_IO4 | MUX_PAD_CTRL(LCD_PAD_CTRL),
};

static iomux_v3_cfg_t const pwm_pads[] = {
	/* Use GPIO for Brightness adjustment, duty cycle = period */
	MX7D_PAD_GPIO1_IO01__GPIO1_IO1 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

void do_enable_parallel_lcd(struct display_info_t const *dev)
{
	imx_iomux_v3_setup_multiple_pads(lcd_pads, ARRAY_SIZE(lcd_pads));

	imx_iomux_v3_setup_multiple_pads(pwm_pads, ARRAY_SIZE(pwm_pads));

	/* Power up the LCD */
	gpio_request(IMX_GPIO_NR(3, 4), "lcd_pwr");
	gpio_direction_output(IMX_GPIO_NR(3, 4) , 1);

	/* Set Brightness to high */
	gpio_request(IMX_GPIO_NR(1, 1), "lcd_backlight");
	gpio_direction_output(IMX_GPIO_NR(1, 1) , 1);
}

struct display_info_t const displays[] = {{
	.bus = ELCDIF1_IPS_BASE_ADDR,
	.addr = 0,
	.pixfmt = 24,
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

static iomux_v3_cfg_t const per_rst_pads[] = {
	MX7D_PAD_GPIO1_IO03__GPIO1_IO3 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static iomux_v3_cfg_t const wdog_pads[] = {
	MX7D_PAD_GPIO1_IO00__WDOG1_WDOG_B | MUX_PAD_CTRL(NO_PAD_CTRL),
};

#ifdef CONFIG_FEC_MXC
static iomux_v3_cfg_t const fec2_pads[] = {
	MX7D_PAD_GPIO1_IO11__ENET1_MDC  | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_GPIO1_IO10__ENET1_MDIO | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_EPDC_SDCE0__ENET2_RGMII_RX_CTL | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_EPDC_SDCLK__ENET2_RGMII_RD0 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_EPDC_SDLE__ENET2_RGMII_RD1  | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_EPDC_SDOE__ENET2_RGMII_RD2  | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_EPDC_SDSHR__ENET2_RGMII_RD3 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_EPDC_SDCE1__ENET2_RGMII_RXC | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_EPDC_GDRL__ENET2_RGMII_TX_CTL | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_EPDC_SDCE2__ENET2_RGMII_TD0 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_EPDC_SDCE3__ENET2_RGMII_TD1 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_EPDC_GDCLK__ENET2_RGMII_TD2 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_EPDC_GDOE__ENET2_RGMII_TD3  | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_EPDC_GDSP__ENET2_RGMII_TXC  | MUX_PAD_CTRL(ENET_PAD_CTRL),
};

static void setup_iomux_fec2(void)
{
	imx_iomux_v3_setup_multiple_pads(fec2_pads, ARRAY_SIZE(fec2_pads));
}
#endif

static void setup_iomux_uart(void)
{
	imx_iomux_v3_setup_multiple_pads(uart1_pads, ARRAY_SIZE(uart1_pads));
}

#ifdef CONFIG_FSL_QSPI
#ifndef CONFIG_DM_SPI

static iomux_v3_cfg_t const quadspi_pads[] = {
	MX7D_PAD_EPDC_DATA00__QSPI_A_DATA0 | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA01__QSPI_A_DATA1 | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA02__QSPI_A_DATA2 | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA03__QSPI_A_DATA3 | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA04__QSPI_A_DQS   | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA05__QSPI_A_SCLK  | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA06__QSPI_A_SS0_B | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA07__QSPI_A_SS1_B | MUX_PAD_CTRL(QSPI_PAD_CTRL),

	MX7D_PAD_EPDC_DATA08__QSPI_B_DATA0 | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA09__QSPI_B_DATA1 | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA10__QSPI_B_DATA2 | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA11__QSPI_B_DATA3 | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA12__QSPI_B_DQS   | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA13__QSPI_B_SCLK  | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA14__QSPI_B_SS0_B | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA15__QSPI_B_SS1_B | MUX_PAD_CTRL(QSPI_PAD_CTRL),

};
#endif

int board_qspi_init(void)
{
#ifndef CONFIG_DM_SPI
	/* Set the iomux */
	imx_iomux_v3_setup_multiple_pads(quadspi_pads, ARRAY_SIZE(quadspi_pads));
#endif

	/* Set the clock */
	set_clk_qspi();

	return 0;
}
#endif

#ifdef CONFIG_FSL_ESDHC_IMX

#define USDHC2_CD_GPIO	IMX_GPIO_NR(5, 9)
#define USDHC3_CD_GPIO	IMX_GPIO_NR(1, 14)

#define USDHC1_PWR_GPIO	IMX_GPIO_NR(5, 2)
#define USDHC2_PWR_GPIO	IMX_GPIO_NR(5, 11)
#define USDHC3_PWR_GPIO	IMX_GPIO_NR(6, 11)


static struct fsl_esdhc_cfg usdhc_cfg[3] = {
	{USDHC1_BASE_ADDR},
	{USDHC2_BASE_ADDR, 0, 4},
	{USDHC3_BASE_ADDR},
};

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;

	switch (cfg->esdhc_base) {
	case USDHC1_BASE_ADDR:
		ret = 1; /* Assume uSDHC1 emmc is always present */
		break;
	case USDHC2_BASE_ADDR:
		ret = !gpio_get_value(USDHC2_CD_GPIO);
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
	 * mmc0                    USDHC1 (eMMC)
	 * mmc1                    USDHC2
	 * mmc2                    USDHC3
	 */
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			imx_iomux_v3_setup_multiple_pads(
				usdhc1_emmc_pads, ARRAY_SIZE(usdhc1_emmc_pads));
			gpio_request(USDHC1_PWR_GPIO, "usdhc1_pwr");
			gpio_direction_output(USDHC1_PWR_GPIO, 1);
			usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
			break;
		case 1:
			imx_iomux_v3_setup_multiple_pads(
				usdhc2_pads, ARRAY_SIZE(usdhc2_pads));
			gpio_request(USDHC2_PWR_GPIO, "usdhc2_pwr");
			gpio_request(USDHC2_CD_GPIO, "usdhc2_cd");
			gpio_direction_input(USDHC2_CD_GPIO);
			gpio_direction_output(USDHC2_PWR_GPIO, 1);
			usdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
			break;
		case 2:
			imx_iomux_v3_setup_multiple_pads(
				usdhc3_pads, ARRAY_SIZE(usdhc3_pads));
			gpio_request(USDHC3_PWR_GPIO, "usdhc3_pwr");
			gpio_request(USDHC3_CD_GPIO, "usdhc3_cd");
			gpio_direction_input(USDHC3_CD_GPIO);
			gpio_direction_output(USDHC3_PWR_GPIO, 1);
			usdhc_cfg[2].sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
			break;
		default:
			printf("Warning: you configured more USDHC controllers"
				"(%d) than supported by the board\n", i + 1);
			return 0;
			}

			if (fsl_esdhc_initialize(bis, &usdhc_cfg[i]))
				printf("Warning: failed to initialize mmc dev %d\n", i);
	}

	return 0;
}
#endif

#ifdef CONFIG_FEC_MXC
int board_eth_init(bd_t *bis)
{
	int ret;

	setup_iomux_fec2();

	ret = fecmxc_initialize_multi(bis, 0,
		CONFIG_FEC_MXC_PHYADDR, IMX_FEC_BASE);
	if (ret)
		printf("FEC1 MXC: %s:failed\n", __func__);

	return 0;
}

static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *const iomuxc_gpr_regs
		= (struct iomuxc_gpr_base_regs *) IOMUXC_GPR_BASE_ADDR;
	int ret;

	/* Use 125M anatop REF_CLK for ENET2, clear gpr1[14], gpr1[18]*/
	clrsetbits_le32(&iomuxc_gpr_regs->gpr[1],
		(IOMUXC_GPR_GPR1_GPR_ENET2_TX_CLK_SEL_MASK |
		 IOMUXC_GPR_GPR1_GPR_ENET2_CLK_DIR_MASK), 0);

	ret = set_clk_enet(ENET_125MHZ);
	if (ret)
		return ret;

	return 0;
}


int board_phy_config(struct phy_device *phydev)
{
	/* Enable 1.8V(SEL_1P5_1P8_POS_REG) on
	   Phy control debug reg 0 */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x1f);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x8);

	/* rgmii tx clock delay enable */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x100);

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}
#endif

#ifdef CONFIG_MXC_SPI
#ifndef CONFIG_DM_SPI
iomux_v3_cfg_t const ecspi1_pads[] = {
	MX7D_PAD_ECSPI1_SCLK__ECSPI1_SCLK | MUX_PAD_CTRL(SPI_PAD_CTRL),
	MX7D_PAD_ECSPI1_MOSI__ECSPI1_MOSI | MUX_PAD_CTRL(SPI_PAD_CTRL),
	MX7D_PAD_ECSPI1_MISO__ECSPI1_MISO | MUX_PAD_CTRL(SPI_PAD_CTRL),

	/* CS0 */
	MX7D_PAD_ECSPI1_SS0__GPIO4_IO19 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

void setup_spinor(void)
{
	imx_iomux_v3_setup_multiple_pads(ecspi1_pads,
					 ARRAY_SIZE(ecspi1_pads));
	gpio_request(IMX_GPIO_NR(4, 19), "ecspi1_cs");
	gpio_direction_output(IMX_GPIO_NR(4, 19), 0);
}

int board_spi_cs_gpio(unsigned bus, unsigned cs)
{
	return (bus == 0 && cs == 0) ? (IMX_GPIO_NR(4, 19)) : -1;
}
#endif
#endif

#ifdef CONFIG_USB_EHCI_MX7
#ifndef CONFIG_DM_USB
iomux_v3_cfg_t const usb_otg1_pads[] = {
	MX7D_PAD_GPIO1_IO05__USB_OTG1_PWR | MUX_PAD_CTRL(NO_PAD_CTRL),
};

iomux_v3_cfg_t const usb_otg2_pads[] = {
	MX7D_PAD_GPIO1_IO07__USB_OTG2_PWR | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static void setup_usb(void)
{
	imx_iomux_v3_setup_multiple_pads(usb_otg1_pads, ARRAY_SIZE(usb_otg1_pads));
	imx_iomux_v3_setup_multiple_pads(usb_otg2_pads, ARRAY_SIZE(usb_otg2_pads));
}
#endif
#endif

int board_early_init_f(void)
{
	setup_iomux_uart();

#ifdef CONFIG_SYS_I2C
	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info1);
	setup_i2c(1, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info2);
#endif

#ifdef CONFIG_USB_EHCI_MX7
#ifndef CONFIG_DM_USB
	setup_usb();
#endif
#endif

	return 0;
}

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

	/* Reset peripherals */
	imx_iomux_v3_setup_multiple_pads(per_rst_pads, ARRAY_SIZE(per_rst_pads));

	gpio_request(IMX_GPIO_NR(1, 3), "per_rst");
	gpio_direction_output(IMX_GPIO_NR(1, 3), 0);
	udelay(500);
	gpio_set_value(IMX_GPIO_NR(1, 3), 1);

#ifdef CONFIG_MXC_SPI
#ifndef CONFIG_DM_SPI
	setup_spinor();
#endif
#endif

#ifdef CONFIG_FEC_MXC
	setup_fec();
#endif

#ifdef CONFIG_FSL_QSPI
	board_qspi_init();
#endif

	return 0;
}

#ifdef CONFIG_CMD_BMODE
static const struct boot_mode board_boot_modes[] = {
	/* 4 bit bus width */
	{"emmc", MAKE_CFGVAL(0x10, 0x22, 0x00, 0x00)},
	{"sd2", MAKE_CFGVAL(0x10, 0x16, 0x00, 0x00)},
	{"sd3", MAKE_CFGVAL(0x10, 0x1a, 0x00, 0x00)},
	{"qspi", MAKE_CFGVAL(0x00, 0x40, 0x00, 0x00)},
	{NULL,   0},
};
#endif

#ifdef CONFIG_DM_PMIC
int power_init_board(void)
{
	struct udevice *dev;
	int ret, dev_id, rev_id, reg;

	ret = pmic_get("pfuze3000@8", &dev);
	if (ret == -ENODEV)
		return 0;
	if (ret != 0)
		return ret;

	dev_id = pmic_reg_read(dev, PFUZE3000_DEVICEID);
	rev_id = pmic_reg_read(dev, PFUZE3000_REVID);
	printf("PMIC: PFUZE3000 DEV_ID=0x%x REV_ID=0x%x\n", dev_id, rev_id);

	/* disable Low Power Mode during standby mode */
	reg = pmic_reg_read(dev, PFUZE3000_LDOGCTL);
	reg |= 0x1;
	pmic_reg_write(dev, PFUZE3000_LDOGCTL, reg);

	/* SW1A/1B mode set to APS/APS */
	reg = 0x8;
	pmic_reg_write(dev, PFUZE3000_SW1AMODE, reg);
	pmic_reg_write(dev, PFUZE3000_SW1BMODE, reg);

	/* SW1A/1B standby voltage set to 0.975V */
	reg = 0xb;
	pmic_reg_write(dev, PFUZE3000_SW1ASTBY, reg);
	pmic_reg_write(dev, PFUZE3000_SW1BSTBY, reg);

	/* set SW1B normal voltage to 0.975V */
	reg = pmic_reg_read(dev, PFUZE3000_SW1BVOLT);
	reg &= ~0x1f;
	reg |= PFUZE3000_SW1AB_SETP(9750);
	pmic_reg_write(dev, PFUZE3000_SW1BVOLT, reg);

	return 0;
}
#endif

int board_late_init(void)
{
#ifdef CONFIG_CMD_BMODE
	add_board_boot_modes(board_boot_modes);
#endif

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
	puts("Board: MX7D 19x19 DDR3 VAL\n");

	return 0;
}
