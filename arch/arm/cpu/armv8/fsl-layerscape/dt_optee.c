// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021-2022 NXP
 */
#include <common.h>
#include <errno.h>
#include <fdt_support.h>
#include <linux/sizes.h>
#include "dt_optee.h"

int ft_add_optee_overlay(void *fdt, struct bd_info *bd)
{
	int ret = 0;

	/*
	 * No BL32_BASE passed means no TEE running, so no
	 * need to add optee node in dts
	 */
	if (!rom_pointer[0]) {
		debug("No BL32_BASE passed means no TEE running\n");
		return ret;
	}

	if (rom_pointer[2]) {
		debug("OP-TEE: applying overlay on 0x%lx\n", rom_pointer[2]);
		ret = fdt_check_header((void *)rom_pointer[2]);
		if (ret == 0) {
			/* Copy the fdt overlay to next 1M and use copied overlay */
			memcpy((void *)(rom_pointer[2] + SZ_1M), (void *)rom_pointer[2],
			       fdt_totalsize((void *)rom_pointer[2]));
			ret = fdt_overlay_apply_verbose(fdt, (void *)(rom_pointer[2] + SZ_1M));
			if (ret == 0) {
				debug("Overlay applied with success");
				fdt_pack(fdt);
			}
		} else {
			printf("DTB overlay not present, exiting without applying\n");
			ret = 0;
		}
	}
	return ret;
}
