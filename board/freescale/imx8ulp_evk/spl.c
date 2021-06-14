// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2020 NXP
 */

#include <common.h>
#include <init.h>
#include <spl.h>
#include <asm/io.h>
#include <errno.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/clock.h>
#include <asm/arch/imx8ulp-pins.h>
#include <dm/uclass.h>
#include <dm/device.h>
#include <dm/uclass-internal.h>
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <asm/arch/ddr.h>
#include <asm/arch/upower.h>
#include <asm/arch/rdc.h>
#include <asm/mach-imx/boot_mode.h>

DECLARE_GLOBAL_DATA_PTR;

void spl_dram_init(void)
{
	init_clk_ddr();
	ddr_init(&dram_timing);
}

u32 spl_boot_device(void)
{
#ifdef CONFIG_SPL_BOOTROM_SUPPORT
	return BOOT_DEVICE_BOOTROM;
#else
	enum boot_device boot_device_spl = get_boot_device();

	switch (boot_device_spl) {
	case SD1_BOOT:
	case MMC1_BOOT:
	case SD2_BOOT:
	case MMC2_BOOT:
		return BOOT_DEVICE_MMC1;
	case SD3_BOOT:
	case MMC3_BOOT:
		return BOOT_DEVICE_MMC2;
	case QSPI_BOOT:
		return BOOT_DEVICE_NOR;
	case NAND_BOOT:
		return BOOT_DEVICE_NAND;
	case USB_BOOT:
	case USB2_BOOT:
		return BOOT_DEVICE_BOARD;
	default:
		return BOOT_DEVICE_NONE;
	}
#endif
}

#define PMIC_I2C_PAD_CTRL	(PAD_CTL_PUS_UP | PAD_CTL_SRE_SLOW | PAD_CTL_ODE)
#define PMIC_MODE_PAD_CTRL	(PAD_CTL_PUS_UP)

static iomux_cfg_t const pmic_pads[] = {
	IMX8ULP_PAD_PTB7__PMIC0_MODE2 | MUX_PAD_CTRL(PMIC_MODE_PAD_CTRL),
	IMX8ULP_PAD_PTB8__PMIC0_MODE1 | MUX_PAD_CTRL(PMIC_MODE_PAD_CTRL),
	IMX8ULP_PAD_PTB9__PMIC0_MODE0 | MUX_PAD_CTRL(PMIC_MODE_PAD_CTRL),
	IMX8ULP_PAD_PTB10__PMIC0_SDA | MUX_PAD_CTRL(PMIC_I2C_PAD_CTRL),
	IMX8ULP_PAD_PTB11__PMIC0_SCL | MUX_PAD_CTRL(PMIC_I2C_PAD_CTRL),
};

void setup_iomux_pmic(void)
{
	imx8ulp_iomux_setup_multiple_pads(pmic_pads, ARRAY_SIZE(pmic_pads));
}

int power_init_board(void)
{
	u32 pmic_reg;

	/* PMIC set bucks1-4 to PWM mode */
	upower_pmic_i2c_read(0x10, &pmic_reg);
	upower_pmic_i2c_read(0x14, &pmic_reg);
	upower_pmic_i2c_read(0x21, &pmic_reg);
	upower_pmic_i2c_read(0x2e, &pmic_reg);

	upower_pmic_i2c_write(0x10, 0x3d);
	upower_pmic_i2c_write(0x14, 0x7d);
	upower_pmic_i2c_write(0x21, 0x7d);
	upower_pmic_i2c_write(0x2e, 0x3d);

	upower_pmic_i2c_read(0x10, &pmic_reg);
	upower_pmic_i2c_read(0x14, &pmic_reg);
	upower_pmic_i2c_read(0x21, &pmic_reg);
	upower_pmic_i2c_read(0x2e, &pmic_reg);

	/* Set buck3 to 1.1v OD */
	upower_pmic_i2c_write(0x22, 0x28);
	return 0;
}

void spl_board_init(void)
{
	struct udevice *dev;
	u32 res;
	int ret;

	uclass_find_first_device(UCLASS_MISC, &dev);

	for (; dev; uclass_find_next_device(&dev)) {
		if (device_probe(dev))
			continue;
	}

	board_early_init_f();

	preloader_console_init();

	puts("Normal Boot\n");

	/* After AP set iomuxc0, the i2c can't work, Need M33 to set it now */
	/*
	if (get_boot_mode() == SINGLE_BOOT)
		setup_iomux_pmic();
	*/

	upower_init();

	power_init_board();

	/* DDR initialization */
	spl_dram_init();

	/* This must place after upower init, so access to MDA and MRC are valid */
	/* Init XRDC MDA  */
	xrdc_init_mda();

	/* Init XRDC MRC for VIDEO, DSP domains */
	xrdc_init_mrc();

	/* Enable A35 access to the CAAM */
	ret = ahab_release_caam(0x7, &res);
	if (ret)
		printf("ahab release caam failed %d, 0x%x\n", ret, res);
}

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	/* Just empty function now - can't decide what to choose */
	debug("%s: %s\n", __func__, name);

	return 0;
}
#endif

void board_init_f(ulong dummy)
{
	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	timer_init();

	arch_cpu_init();

	board_init_r(NULL, 0);
}
