/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/mx7ulp-pins.h>
#include <asm/arch/iomux.h>
#include <asm/gpio.h>
#include <fsl_esdhc.h>
#include <mmc.h>
#include <usb.h>
#include <asm/imx-common/video.h>

#ifdef CONFIG_FSL_FASTBOOT
#include <fastboot.h>
#include <asm/imx-common/boot_mode.h>
#ifdef CONFIG_ANDROID_RECOVERY
#include <recovery.h>
#endif /*CONFIG_ANDROID_RECOVERY*/
#endif /*CONFIG_FASTBOOT*/

DECLARE_GLOBAL_DATA_PTR;

#define ESDHC_PAD_CTRL	(PAD_CTL_PUS_UP | PAD_CTL_DSE)
#define ESDHC_CD_GPIO_PAD_CTRL (PAD_CTL_IBE_ENABLE | PAD_CTL_PUS_UP)

#define UART_PAD_CTRL	(PAD_CTL_PUS_UP)

#define I2C_PAD_CTRL	(PAD_CTL_PUS_UP | PAD_CTL_ODE)

#define OTG_ID_GPIO_PAD_CTRL	(PAD_CTL_IBE_ENABLE)
#define OTG_PWR_GPIO_PAD_CTRL	(PAD_CTL_OBE_ENABLE)

#define QSPI_PAD_CTRL1	(PAD_CTL_PUS_UP | PAD_CTL_DSE)

#define QSPI_PAD_CTRL0	(PAD_CTL_PUS_UP | PAD_CTL_DSE \
	| PAD_CTL_OBE_ENABLE)

#define MIPI_GPIO_PAD_CTRL	(PAD_CTL_OBE_ENABLE)

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_SIZE;

	return 0;
}

static int mx7ulp_board_rev(void)
{
	return 0x41;
}

u32 get_board_rev(void)
{
	int rev = mx7ulp_board_rev();

	return (get_cpu_rev() & ~(0xF << 8)) | rev;
}

static iomux_cfg_t const lpuart4_pads[] = {
	MX7ULP_PAD_PTC3__LPUART4_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX7ULP_PAD_PTC2__LPUART4_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static void setup_iomux_uart(void)
{
	mx7ulp_iomux_setup_multiple_pads(lpuart4_pads, ARRAY_SIZE(lpuart4_pads));
}

#ifdef CONFIG_SYS_I2C_IMX
static iomux_cfg_t const i2c5_pads[] = {
	MX7ULP_PAD_PTC4__LPI2C5_SCL | MUX_PAD_CTRL(I2C_PAD_CTRL),
	MX7ULP_PAD_PTC5__LPI2C5_SDA | MUX_PAD_CTRL(I2C_PAD_CTRL),
};

static iomux_cfg_t const i2c7_pads[] = {
	MX7ULP_PAD_PTF12__LPI2C7_SCL | MUX_PAD_CTRL(I2C_PAD_CTRL),
	MX7ULP_PAD_PTF13__LPI2C7_SDA | MUX_PAD_CTRL(I2C_PAD_CTRL),
};

void i2c_init_board(void)
{
	mx7ulp_iomux_setup_multiple_pads(i2c5_pads, ARRAY_SIZE(i2c5_pads));
	mx7ulp_iomux_setup_multiple_pads(i2c7_pads, ARRAY_SIZE(i2c7_pads));
}
#endif

#ifdef CONFIG_USB_EHCI_MX7
/*Need rework for ID and PWR_EN pins*/
static iomux_cfg_t const usb_otg1_pads[] = {
	MX7ULP_PAD_PTC0__PTC0 | MUX_PAD_CTRL(OTG_PWR_GPIO_PAD_CTRL),  /* gpio for power en */
	MX7ULP_PAD_PTC8__PTC8 | MUX_PAD_CTRL(OTG_ID_GPIO_PAD_CTRL),  /* gpio for OTG ID*/
};

static void setup_usb(void)
{
	mx7ulp_iomux_setup_multiple_pads(usb_otg1_pads,
						 ARRAY_SIZE(usb_otg1_pads));

	gpio_request(IMX_GPIO_NR(3, 8), "otg_id");
	gpio_direction_input(IMX_GPIO_NR(3, 8));

	gpio_request(IMX_GPIO_NR(3, 0), "otg_pwr");
}

/*Needs to override the ehci power if controlled by GPIO */
int board_ehci_power(int port, int on)
{
	switch (port) {
	case 0:
		if (on)
			gpio_direction_output(IMX_GPIO_NR(3, 0), 1);
		else
			gpio_direction_output(IMX_GPIO_NR(3, 0), 0);
		break;
	default:
		printf("MXC USB port %d not yet supported\n", port);
		return -EINVAL;
	}

	return 0;
}

int board_usb_phy_mode(int port)
{
	int ret = 0;

	if (port == 0) {
		ret = gpio_get_value(IMX_GPIO_NR(3, 8));

		if (ret)
			return USB_INIT_DEVICE;
		else
			return USB_INIT_HOST;
	}

	return USB_INIT_HOST;
}

#endif

int board_early_init_f(void)
{
	setup_iomux_uart();

	return 0;
}

#ifdef CONFIG_FSL_QSPI
static iomux_cfg_t const quadspi_pads[] = {
	MX7ULP_PAD_PTB8__QSPIA_SS0_B | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX7ULP_PAD_PTB15__QSPIA_SCLK  | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX7ULP_PAD_PTB16__QSPIA_DATA3 | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX7ULP_PAD_PTB17__QSPIA_DATA2 | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX7ULP_PAD_PTB18__QSPIA_DATA1 | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX7ULP_PAD_PTB19__QSPIA_DATA0 | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
};

int board_qspi_init(void)
{
	u32 val;

	mx7ulp_iomux_setup_multiple_pads(quadspi_pads, ARRAY_SIZE(quadspi_pads));

	/* enable clock */
	val = readl(PCC1_RBASE + 0x94);

	if (!(val & 0x20000000)) {
		writel(0x03000003, (PCC1_RBASE + 0x94));
		writel(0x43000003, (PCC1_RBASE + 0x94));
	}
	return 0;
}
#endif

#ifdef CONFIG_VIDEO_MXS

#define MIPI_RESET_GPIO	IMX_GPIO_NR(3, 19)
#define LED_PWM_EN_GPIO	IMX_GPIO_NR(6, 2)

static iomux_cfg_t const mipi_reset_pad[] = {
	MX7ULP_PAD_PTC19__PTC19 | MUX_PAD_CTRL(MIPI_GPIO_PAD_CTRL),
};

static iomux_cfg_t const led_pwm_en_pad[] = {
	MX7ULP_PAD_PTF2__PTF2 | MUX_PAD_CTRL(MIPI_GPIO_PAD_CTRL),
};

int board_mipi_panel_reset(void)
{
	gpio_direction_output(MIPI_RESET_GPIO, 0);
	udelay(1000);
	gpio_direction_output(MIPI_RESET_GPIO, 1);
	return 0;
}

int board_mipi_panel_shutdown(void)
{
	gpio_direction_output(MIPI_RESET_GPIO, 0);
	gpio_direction_output(LED_PWM_EN_GPIO, 0);
	return 0;
}

void setup_mipi_reset(void)
{
	mx7ulp_iomux_setup_multiple_pads(mipi_reset_pad, ARRAY_SIZE(mipi_reset_pad));
	gpio_request(MIPI_RESET_GPIO, "mipi_panel_reset");
}

void do_enable_mipi_dsi(struct display_info_t const *dev)
{
	setup_mipi_reset();

	/* Enable backlight */
	mx7ulp_iomux_setup_multiple_pads(led_pwm_en_pad, ARRAY_SIZE(mipi_reset_pad));
	gpio_request(LED_PWM_EN_GPIO, "led_pwm_en");
	gpio_direction_output(LED_PWM_EN_GPIO, 1);
}

struct display_info_t const displays[] = {{
	.bus = LCDIF_RBASE,
	.addr = 0,
	.pixfmt = 24,
	.detect = NULL,
	.enable	= do_enable_mipi_dsi,
	.mode	= {
		.name			= "HX8363_WVGA",
		.xres           = 480,
		.yres           = 854,
		.pixclock       = 41042,
		.left_margin    = 40,
		.right_margin   = 60,
		.upper_margin   = 3,
		.lower_margin   = 3,
		.hsync_len      = 8,
		.vsync_len      = 4,
		.sync           = 0,
		.vmode          = FB_VMODE_NONINTERLACED
} } };
size_t display_count = ARRAY_SIZE(displays);
#endif

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

#ifdef CONFIG_USB_EHCI_MX7
	setup_usb();
#endif

#ifdef CONFIG_FSL_QSPI
	board_qspi_init();
#endif

	return 0;
}

#ifdef CONFIG_FSL_ESDHC
static struct fsl_esdhc_cfg usdhc_cfg[1] = {
	{USDHC0_RBASE, 0, 4},
};

static iomux_cfg_t const usdhc0_pads[] = {
	MX7ULP_PAD_PTD0__SDHC0_RESET_b | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	MX7ULP_PAD_PTD1__SDHC0_CMD | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	MX7ULP_PAD_PTD2__SDHC0_CLK | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	MX7ULP_PAD_PTD7__SDHC0_D3 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	MX7ULP_PAD_PTD8__SDHC0_D2 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	MX7ULP_PAD_PTD9__SDHC0_D1 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	MX7ULP_PAD_PTD10__SDHC0_D0 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),

#ifdef CONFIG_MX7ULP_EVK_EMMC
	MX7ULP_PAD_PTD11__SDHC0_DQS | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
#else
	/* CD */
	MX7ULP_PAD_PTC10__PTC10 | MUX_PAD_CTRL(ESDHC_CD_GPIO_PAD_CTRL),
#endif
};

#define USDHC0_CD_GPIO	IMX_GPIO_NR(3, 10)

int board_mmc_init(bd_t *bis)
{
	int i, ret;
	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-Boot device node)    (Physical Port)
	 * mmc0                    USDHC0
	 */
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			mx7ulp_iomux_setup_multiple_pads(usdhc0_pads, ARRAY_SIZE(usdhc0_pads));
			init_clk_usdhc(0);
			usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);

#ifndef CONFIG_MX7ULP_EVK_EMMC
			gpio_request(USDHC0_CD_GPIO, "usdhc0_cd");
			gpio_direction_input(USDHC0_CD_GPIO);
#endif
			break;
		default:
			printf("Warning: you configured more USDHC controllers"
				"(%d) than supported by the board\n", i + 1);
			return -EINVAL;
			}

			ret = fsl_esdhc_initialize(bis, &usdhc_cfg[i]);
			if (ret)
				return ret;
	}

	return 0;
}

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;

	switch (cfg->esdhc_base) {
	case USDHC0_RBASE:
#ifdef CONFIG_MX7ULP_EVK_EMMC
		ret = 1;
#else
		ret = !gpio_get_value(USDHC0_CD_GPIO);
#endif
		break;
	}

	return ret;
}

int board_mmc_get_env_dev(int devno)
{
	return devno;
}
#endif

int board_late_init(void)
{
#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

	return 0;
}

int checkboard(void)
{
	printf("Board: i.MX7ULP EVK board\n");

	return 0;
}

#ifdef CONFIG_FSL_FASTBOOT
void board_fastboot_setup(void)
{
	switch (get_boot_device()) {
#if defined(CONFIG_FASTBOOT_STORAGE_MMC)
	case SD1_BOOT:
	case MMC1_BOOT:
		if (!getenv("fastboot_dev"))
			setenv("fastboot_dev", "mmc0");
		if (!getenv("bootcmd"))
			setenv("bootcmd", "boota mmc0");
		break;
	case SD3_BOOT:
	case MMC3_BOOT:
		if (!getenv("fastboot_dev"))
			setenv("fastboot_dev", "mmc1");
		if (!getenv("bootcmd"))
			setenv("bootcmd", "boota mmc1");
		break;
#endif /*CONFIG_FASTBOOT_STORAGE_MMC*/
	default:
		printf("unsupported boot devices\n");
		break;
	}
}

#ifdef CONFIG_ANDROID_RECOVERY
int check_recovery_cmd_file(void)
{
	int recovery_mode = 0;
	recovery_mode = recovery_check_and_clean_flag();
	return recovery_mode;
}

void board_recovery_setup(void)
{
	int bootdev = get_boot_device();
	switch (bootdev) {
#if defined(CONFIG_FASTBOOT_STORAGE_MMC)
	case SD1_BOOT:
	case MMC1_BOOT:
		if (!getenv("bootcmd_android_recovery"))
			setenv("bootcmd_android_recovery", "boota mmc0 recovery");
		break;
	case SD2_BOOT:
	case MMC2_BOOT:
		if (!getenv("bootcmd_android_recovery"))
			setenv("bootcmd_android_recovery", "boota mmc1 recovery");
		break;
#endif /*CONFIG_FASTBOOT_STORAGE_MMC*/
	default:
		printf("Unsupported bootup device for recovery: dev: %d\n",
			bootdev);
		return;
	}

	printf("setup env for recovery..\n");
	setenv("bootcmd", "run bootcmd_android_recovery");
}
#endif /*CONFIG_ANDROID_RECOVERY*/
#endif /*CONFIG_FSL_FASTBOOT*/
