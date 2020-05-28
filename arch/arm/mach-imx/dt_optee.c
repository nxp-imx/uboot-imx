// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2020 NXP
 */
#include <common.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/optee.h>
#include <errno.h>
#include <fdt_support.h>

#ifdef CONFIG_OF_SYSTEM_SETUP
int ft_add_optee_node(void *fdt, bd_t *bd)
{
	const char *path, *subpath;
	int ret = 0;
	int offs;
	phys_addr_t optee_start;
	size_t optee_size;

	/* Not let uboot create the node */
	if (CONFIG_IS_ENABLED(XEN))
		return 0;
	/*
	 * No TEE space allocated indicating no TEE running, so no
	 * need to add optee node in dts
	 */
	if (!rom_pointer[1])
		return 0;

#ifdef CONFIG_OF_LIBFDT_OVERLAY
	if (rom_pointer[2]) {
		debug("OP-TEE: applying overlay on 0x%lx\n",rom_pointer[2]);
		ret = fdt_overlay_apply_verbose(fdt, (void*)rom_pointer[2]);
		if (ret == 0) {
			debug("Overlay applied with success");
			fdt_pack(fdt);
			return 0;
		}
	}
	/* Fallback to previous implementation */
#endif

	optee_start = (phys_addr_t)rom_pointer[0];
	optee_size = rom_pointer[1] - OPTEE_SHM_SIZE;

	offs = fdt_increase_size(fdt, 512);
	if (offs) {
		printf("No Space for dtb\n");
		return -1;
	}

	path = "/firmware";
	offs = fdt_path_offset(fdt, path);
	if (offs < 0) {
		offs = add_dt_path_subnode(fdt, "/", "firmware");
		if (offs < 0)
			return -1;
	}

	subpath = "optee";
	offs = fdt_add_subnode(fdt, offs, subpath);
	if (offs < 0) {
		printf("Could not create %s node.\n", subpath);
		return -1;
	}

	fdt_setprop_string(fdt, offs, "compatible", "linaro,optee-tz");
	fdt_setprop_string(fdt, offs, "method", "smc");

	ret = add_res_mem_dt_node(fdt, "optee_core", optee_start, optee_size);
	if (ret < 0) {
		printf("Could not create optee_core node.\n");
		return -1;
	}

	ret = add_res_mem_dt_node(fdt, "optee_shm", optee_start + optee_size,
				  OPTEE_SHM_SIZE);
	if (ret < 0) {
		printf("Could not create optee_shm node.\n");
		return -1;
	}
	return ret;
}
#endif
