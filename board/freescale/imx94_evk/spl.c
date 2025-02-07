// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2025 NXP
 */

#include <common.h>
#include <command.h>
#include <cpu_func.h>
#include <clk.h>
#include <hang.h>
#include <image.h>
#include <init.h>
#include <log.h>
#include <spl.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/arch/imx93_pins.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <asm/arch-mx7ulp/gpio.h>
#include <asm/mach-imx/syscounter.h>
#include <asm/mach-imx/ele_api.h>
#include <asm/sections.h>
#include <dm/uclass.h>
#include <dm/device.h>
#include <dm/uclass-internal.h>
#include <dm/device-internal.h>
#include <linux/delay.h>
#include <linux/iopoll.h>
#include <asm/arch/clock.h>
#include <asm/arch/ccm_regs.h>
#include <asm/arch/ddr.h>
#include <power/pmic.h>
#include <power/pca9450.h>
#include <asm/arch/trdc.h>
#ifdef CONFIG_SCMI_FIRMWARE
#include <scmi_agent.h>
#include <scmi_protocols.h>
#include <../dts/imx94-clock.h>
#include <../dts/imx94-power.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

static struct udevice *scmi_dev __maybe_unused;

int spl_board_boot_device(enum boot_device boot_dev_spl)
{
	switch (boot_dev_spl) {
		case SD1_BOOT:
		case MMC1_BOOT:
			return BOOT_DEVICE_MMC1;
		case SD2_BOOT:
		case MMC2_BOOT:
			return BOOT_DEVICE_MMC2;
		case USB_BOOT:
			return BOOT_DEVICE_BOARD;
		case QSPI_BOOT:
			return BOOT_DEVICE_SPI;
		default:
			return BOOT_DEVICE_RAM;
	}
}

void spl_board_init(void)
{
	int ret;
	puts("Normal Boot\n");

	ret = ele_start_rng();
	if (ret)
		printf("Fail to start RNG: %d\n", ret);
}

extern int imx9_probe_mu(void *ctx, struct event *event);

/* SCMI suport by default */
void board_init_f(ulong dummy)
{
	int ret;
	bool ddrmix_power = false;

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

#ifdef CONFIG_SPL_RECOVER_DATA_SECTION
	if (IS_ENABLED(CONFIG_SPL_BUILD)) {
		spl_save_restore_data();
	}
#endif

	timer_init();

	spl_early_init();

	ret = imx9_probe_mu(NULL, NULL);
	if (ret)
		hang();

	/* this will cause hang on 943 EMU */
	arch_cpu_init();

	board_early_init_f();

	preloader_console_init();

	printf("SOC: 0x%x\n", gd->arch.soc_rev);
	printf("LC: 0x%x\n", gd->arch.lifecycle);

	/* Currently, it is not supported on 943 EMU */
	get_reset_reason(true, false);

	/* Will set ARM freq to max rate */
	clock_init_late();

	/* Check is DDR MIX is already powered up. */
	u32 state = 0;
	ret = scmi_pwd_state_get(gd->arch.scmi_dev, IMX94_PD_DDR, &state);
	if (ret) {
		printf("scmi_pwd_state_get Failed %d for DDRMIX\n", ret);
	} else {
		if (state == BIT(30)) {
			panic("DDRMIX is powered OFF, Please initialize DDR with OEI \n");
		} else {
			printf("DDRMIX is powered UP \n");
			ddrmix_power = true;
		}
	}

	board_init_r(NULL, 0);
}

