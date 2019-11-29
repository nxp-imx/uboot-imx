/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <asm/mach-imx/video.h>
#include <asm/arch/video_common.h>
#include <imx8_hdmi.h>

int do_hdp(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	if (argc < 2)
		return 0;

	if (strncmp(argv[1], "colorbar", 8) == 0) {
		GraphicDevice *gdev;
		struct video_mode_settings *vm;

		gdev = imx8m_get_gd();
		vm = imx8m_get_gmode();
		imx8m_show_gmode();

		imx8m_create_color_bar(
			(void *)((uint64_t)gdev->frameAdrs),
			vm);
		printf("colorbar test\n");
	} else if (strncmp(argv[1], "stop", 4) == 0) {
		imx8_hdmi_disable();
		printf("stopping hdmi\n");
	} else {
		printf("test error argc %d\n", argc);
	}

	return 0;
}
/***************************************************/

U_BOOT_CMD(
	hdp,  CONFIG_SYS_MAXARGS, 1, do_hdp,
	"hdmi/dp display test commands",
	"[<command>] ...\n"
	"colorbar - display a colorbar pattern\n"
	);
