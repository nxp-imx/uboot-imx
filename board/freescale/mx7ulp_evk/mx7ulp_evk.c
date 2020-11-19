// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 */

#include <common.h>
#include <fdt_support.h>
#include <asm/io.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/mx7ulp-pins.h>
#include <asm/arch/iomux.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/gpio.h>
#include <usb.h>
#include <dm.h>

#ifdef CONFIG_BOOTLOADER_MENU
#include "video.h"
#include "dm/uclass.h"
#include "video_font_data.h"
#include "video_console.h"
#include "recovery.h"
#endif

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL	(PAD_CTL_PUS_UP)
#define QSPI_PAD_CTRL1	(PAD_CTL_PUS_UP | PAD_CTL_DSE)
#define OTG_ID_GPIO_PAD_CTRL	(PAD_CTL_IBE_ENABLE)

int dram_init(void)
{
	gd->ram_size = imx_ddr_size();

	return 0;
}

ulong board_get_usable_ram_top(ulong total_size)
{
	/* Reserve top 1M memory used by M core vring/buffer */
	return gd->ram_top - SZ_1M;
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

int board_early_init_f(void)
{
	setup_iomux_uart();

	return 0;
}

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

#ifdef CONFIG_FSL_QSPI
	board_qspi_init();
#endif

	return 0;
}

#if IS_ENABLED(CONFIG_OF_BOARD_SETUP)
int ft_board_setup(void *blob, bd_t *bd)
{
	const char *path;
	int rc, nodeoff;

	if (get_boot_device() == USB_BOOT) {
		path = fdt_get_alias(blob, "mmc0");
		if (!path) {
			puts("Not found mmc0\n");
			return 0;
		}

		nodeoff = fdt_path_offset(blob, path);
		if (nodeoff < 0)
			return 0;

		printf("Found usdhc0 node\n");
		if (fdt_get_property(blob, nodeoff, "vqmmc-supply",
		    NULL) != NULL) {
			rc = fdt_delprop(blob, nodeoff, "vqmmc-supply");
			if (!rc) {
				puts("Removed vqmmc-supply property\n");
add:
				rc = fdt_setprop(blob, nodeoff,
						 "no-1-8-v", NULL, 0);
				if (rc == -FDT_ERR_NOSPACE) {
					rc = fdt_increase_size(blob, 32);
					if (!rc)
						goto add;
				} else if (rc) {
					printf("Failed to add no-1-8-v property, %d\n", rc);
				} else {
					puts("Added no-1-8-v property\n");
				}
			} else {
				printf("Failed to remove vqmmc-supply property, %d\n", rc);
			}
		}
	}

	return 0;
}
#endif

#ifdef CONFIG_BOOTLOADER_MENU
static iomux_cfg_t const vol_pad[] = {
	MX7ULP_PAD_PTA3__PTA3 | MUX_PAD_CTRL(PAD_CTL_IBE_ENABLE),
};
#define VOLP_GPIO	IMX_GPIO_NR(1, 3)
bool is_vol_key_pressed(void);
int show_bootloader_menu(void);
#endif

int board_late_init(void)
{
	env_set("tee", "no");
#ifdef CONFIG_IMX_OPTEE
	env_set("tee", "yes");
#endif

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

#ifdef CONFIG_BOOTLOADER_MENU
	mx7ulp_iomux_setup_multiple_pads(vol_pad, ARRAY_SIZE(vol_pad));
	if (gpio_request(VOLP_GPIO, "volp"))
		printf("request error\n");
	gpio_direction_input(VOLP_GPIO);

	if (is_vol_key_pressed())
		show_bootloader_menu();
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

#ifdef CONFIG_ANDROID_SUPPORT
bool is_power_key_pressed(void) {
	/* the onoff button is 'pressed' by default on evk board */
	return (bool)(!(readl(SNVS_HPSR_REVB) & (0x1 << 6)));
}

#ifdef CONFIG_BOOTLOADER_MENU
char bootloader_menu[4][40] = {
	"   * Power off the device\n",
	"   * Start the device normally\n",
	"   * Restart the bootloader\n",
	"   * Boot into recovery mode\n"
};

bool is_vol_key_pressed(void) {
	int ret = 0;
	ret = gpio_get_value(VOLP_GPIO);
	return (bool)(!!ret);
}

int show_bootloader_menu(void) {
	struct udevice *dev, *dev_console;
	uint32_t focus = 0, i;
	bool stop_menu = false;

	/* clear screen first */
	if (uclass_first_device_err(UCLASS_VIDEO, &dev)) {
		printf("no video device found!\n");
		return -1;
	}
	video_clear(dev);

	if (uclass_first_device_err(UCLASS_VIDEO_CONSOLE, &dev_console)) {
		printf("no text console device found!\n");
		return -1;
	}

	vidconsole_position_cursor(dev_console, 0, 1);
	vidconsole_put_string(dev_console, "Press 'vol+' to choose an item, press\n");
	vidconsole_put_string(dev_console, "power key to confirm:\n");
	while (!stop_menu) {
		/* reset the cursor position. */
		vidconsole_position_cursor(dev_console, 0, 4);
		/* show menu */
		for (i = 0; i < 4; i++) {
			/* reverse color for the 'focus' line. */
			if (i == focus)
				vidconsole_put_string(dev_console, "\x1b[7m");
			/* show text */
			vidconsole_put_string(dev_console, bootloader_menu[i]);
			/* reset color back for the 'next' line. */
			if (i == focus)
				vidconsole_put_string(dev_console, "\x1b[0m");
		}
		/* check button status */
		while (1) {
			if (is_power_key_pressed()) {
				switch (focus) {
					case 0: /*TODO*/
					case 1:
						break;
					case 2:
						do_reset(NULL, 0, 0, NULL);
						break;
					case 3:
						board_recovery_setup();
						break;
					default:
						break;
				}
				stop_menu = true;
				break;
			} else if (is_vol_key_pressed()) {
				focus++;
				if (focus > 3)
					focus = 0;
				mdelay(400);
				break;
			}
		}
	}

	/* clear screen before exit */
	video_clear(dev);
	return 0;
}
#endif /* CONFIG_BOOTLOADER_MENU */
#endif /* CONFIG_ANDROID_SUPPORT*/
