// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 NXP
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
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/syscounter.h>
#include <asm/mach-imx/ele_api.h>
#include <dm/uclass.h>
#include <dm/device.h>
#include <dm/uclass-internal.h>
#include <dm/device-internal.h>
#include <linux/delay.h>
#include <linux/iopoll.h>
#include <asm/arch/clock.h>
#include <asm/arch/ccm_regs.h>
#ifdef CONFIG_SCMI_FIRMWARE
#include <scmi_agent.h>
#include <scmi_protocols.h>
#include <dt-bindings/clock/fsl,imx95-clock.h>
#include <dt-bindings/power/fsl,imx95-power.h>
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
		return BOOT_DEVICE_NONE;
	}
}

void spl_board_init(void)
{
	puts("Normal Boot\n");
}

extern int imx9_probe_mu(void *ctx, struct event *event);

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

	/* Need dm_init() to run before any SCMI calls can be made. */
	spl_early_init();

	/* Need enable SCMI drivers and ELE driver before enabling console */
	ret = imx9_probe_mu(NULL, NULL);
	if (ret)
		hang(); /* if MU not probed, nothing can output, just hang here */

	arch_cpu_init();

	board_early_init_f();

	preloader_console_init();

	printf("SOC: 0x%x\n", gd->arch.soc_rev);
	printf("LC: 0x%x\n", gd->arch.lifecycle);

	/* Will set ARM freq to max rate */
	clock_init_late();

	/* Check is DDR MIX is already powered up. */
	struct scmi_power_get_state in = {
		.domain = IMX95_PD_DDR,
	};
	struct scmi_power_get_state_out out;
	struct scmi_msg msg = SCMI_MSG_IN(SCMI_PROTOCOL_ID_POWER_DOMAIN,
					  SCMI_POWER_STATE_GET,
					  in, out);

	devm_scmi_process_msg(gd->arch.scmi_dev, gd->arch.scmi_channel, &msg);

	if (!out.status) {
		if (out.state) {
			panic("DDRMIX is powered OFF, Please initialize DDR with OEI \n");
        } else {
			printf("DDRMIX is powered UP \n");
			ddrmix_power = true;
		}
	} else {
		printf("SCMI_POWER_STATE_GET Failed DDRMIX\n");
	}

	board_init_r(NULL, 0);
}
