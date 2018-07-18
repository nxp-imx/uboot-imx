/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/mx7ulp-pins.h>
#include <asm/arch/iomux.h>
#include <asm/gpio.h>
#include <usb.h>
#include <dm.h>
#include <asm/imx-common/video.h>
#include <mipi_dsi_northwest.h>
#include <imx_mipi_dsi_bridge.h>
#include <mipi_dsi_panel.h>

#ifdef CONFIG_FSL_FASTBOOT
#include <fastboot.h>
#include <asm/imx-common/boot_mode.h>
#ifdef CONFIG_ANDROID_RECOVERY
#include <recovery.h>
#endif /*CONFIG_ANDROID_RECOVERY*/
#endif /*CONFIG_FSL_FASTBOOT*/

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL	(PAD_CTL_PUS_UP)
#define QSPI_PAD_CTRL1	(PAD_CTL_PUS_UP | PAD_CTL_DSE)
#define OTG_ID_GPIO_PAD_CTRL	(PAD_CTL_IBE_ENABLE)

#define MIPI_GPIO_PAD_CTRL	(PAD_CTL_OBE_ENABLE)

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_SIZE;

	return 0;
}

static iomux_cfg_t const lpuart4_pads[] = {
	MX7ULP_PAD_PTC3__LPUART4_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX7ULP_PAD_PTC2__LPUART4_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static void setup_iomux_uart(void)
{
	mx7ulp_iomux_setup_multiple_pads(lpuart4_pads,
					 ARRAY_SIZE(lpuart4_pads));
}

#ifdef CONFIG_FSL_QSPI
#ifndef CONFIG_DM_SPI
static iomux_cfg_t const quadspi_pads[] = {
	MX7ULP_PAD_PTB8__QSPIA_SS0_B | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX7ULP_PAD_PTB15__QSPIA_SCLK  | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX7ULP_PAD_PTB16__QSPIA_DATA3 | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX7ULP_PAD_PTB17__QSPIA_DATA2 | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX7ULP_PAD_PTB18__QSPIA_DATA1 | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX7ULP_PAD_PTB19__QSPIA_DATA0 | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
};
#endif

int board_qspi_init(void)
{
	u32 val;
#ifndef CONFIG_DM_SPI
	mx7ulp_iomux_setup_multiple_pads(quadspi_pads, ARRAY_SIZE(quadspi_pads));
#endif

	/* enable clock */
	val = readl(PCC1_RBASE + 0x94);

	if (!(val & 0x20000000)) {
		writel(0x03000003, (PCC1_RBASE + 0x94));
		writel(0x43000003, (PCC1_RBASE + 0x94));
	}

	/* Enable QSPI as a wakeup source on B0 */
	if (soc_rev() >= CHIP_REV_2_0)
		setbits_le32(SIM0_RBASE + WKPU_WAKEUP_EN, WKPU_QSPI_CHANNEL);
	return 0;
}
#endif

#ifdef CONFIG_DM_USB
static iomux_cfg_t const usb_otg1_pads[] = {
	MX7ULP_PAD_PTC8__PTC8 | MUX_PAD_CTRL(OTG_ID_GPIO_PAD_CTRL),  /* gpio for OTG ID*/
};

static void setup_usb(void)
{
	mx7ulp_iomux_setup_multiple_pads(usb_otg1_pads,
						 ARRAY_SIZE(usb_otg1_pads));

	gpio_request(IMX_GPIO_NR(3, 8), "otg_id");
	gpio_direction_input(IMX_GPIO_NR(3, 8));
}

int board_ehci_usb_phy_mode(struct udevice *dev)
{
	int ret = 0;

	if (dev_get_addr(dev) == USBOTG0_RBASE) {
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

#ifdef CONFIG_VIDEO_MXS

#define MIPI_RESET_GPIO	IMX_GPIO_NR(3, 19)
#define LED_PWM_EN_GPIO	IMX_GPIO_NR(6, 2)

static iomux_cfg_t const mipi_reset_pad[] = {
	MX7ULP_PAD_PTC19__PTC19 | MUX_PAD_CTRL(MIPI_GPIO_PAD_CTRL),
};

static iomux_cfg_t const led_pwm_en_pad[] = {
	MX7ULP_PAD_PTF2__PTF2 | MUX_PAD_CTRL(MIPI_GPIO_PAD_CTRL),
};

struct mipi_dsi_client_dev hx8363_dev = {
	.channel	= 0,
	.lanes = 2,
	.format  = MIPI_DSI_FMT_RGB888,
	.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE |
			  MIPI_DSI_MODE_EOT_PACKET | MIPI_DSI_MODE_VIDEO_HSE,
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

	/* Setup DSI host driver */
	mipi_dsi_northwest_setup(DSI_RBASE, SIM0_RBASE);

	/* Init hx8363 driver, must after dsi host driver setup */
	hx8363_init();
	hx8363_dev.name = displays[0].mode.name;
	imx_mipi_dsi_bridge_attach(&hx8363_dev); /* attach hx8363 device */

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

#ifdef CONFIG_FSL_QSPI
	board_qspi_init();
#endif

#ifdef CONFIG_DM_USB
	setup_usb();
#endif

	return 0;
}

int board_mmc_get_env_dev(int devno)
{
	return devno;
}

int board_late_init(void)
{
	setenv("tee", "no");
#ifdef CONFIG_IMX_OPTEE
	setenv("tee", "yes");
#endif

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

	return 0;
}

#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_ANDROID_RECOVERY
int is_recovery_key_pressing(void)
{
	/* TODO: uboot can get the key event from M4 core*/
	return 0;
}

#endif /*CONFIG_ANDROID_RECOVERY*/
#endif /*CONFIG_FSL_FASTBOOT*/
