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
static void set_dt_val(void *data, uint32_t cell_size, uint64_t val)
{
	if (cell_size == 1) {
		fdt32_t v = cpu_to_fdt32((uint32_t)val);

		memcpy(data, &v, sizeof(v));
	} else {
		fdt64_t v = cpu_to_fdt64(val);

		memcpy(data, &v, sizeof(v));
	}
}

static int add_dt_path_subnode(void *fdt, const char *path, const char *subnode)
{
	int offs;

	offs = fdt_path_offset(fdt, path);
	if (offs < 0)
		return -1;

	offs = fdt_add_subnode(fdt, offs, subnode);
	if (offs < 0)
		return -1;
	return offs;
}

static int add_res_mem_dt_node(void *fdt, const char *name, phys_addr_t pa,
			       size_t size)
{
	int offs = 0;
	int ret = 0;
	int addr_size = -1;
	int len_size = -1;
	bool found = true;
	char subnode_name[80] = { 0 };

	offs = fdt_path_offset(fdt, "/reserved-memory");

	if (offs < 0) {
		found = false;
		offs = 0;
	}

	len_size = fdt_size_cells(fdt, offs);
	if (len_size < 0)
		return -1;
	addr_size = fdt_address_cells(fdt, offs);
	if (addr_size < 0)
		return -1;

	if (!found) {
		offs = add_dt_path_subnode(fdt, "/", "reserved-memory");
		if (offs < 0)
			return -1;

		ret = fdt_setprop_cell(fdt, offs, "#address-cells", addr_size);
		if (ret < 0)
			return -1;
		ret = fdt_setprop_cell(fdt, offs, "#size-cells", len_size);
		if (ret < 0)
			return -1;
		ret = fdt_setprop(fdt, offs, "ranges", NULL, 0);
		if (ret < 0)
			return -1;
	}

	snprintf(subnode_name, sizeof(subnode_name), "%s@0x%llx", name, pa);
	offs = fdt_add_subnode(fdt, offs, subnode_name);
	if (offs >= 0) {
		u32 data[FDT_MAX_NCELLS * 2];

		set_dt_val(data, addr_size, pa);
		set_dt_val(data + addr_size, len_size, size);
		ret = fdt_setprop(fdt, offs, "reg", data,
				  sizeof(uint32_t) * (addr_size + len_size));
		if (ret < 0)
			return -1;
		ret = fdt_setprop(fdt, offs, "no-map", NULL, 0);
		if (ret < 0)
			return -1;
	} else {
		return -1;
	}
	return 0;
}

int ft_add_optee_node(void *fdt, bd_t *bd)
{
	const char *path, *subpath;
	int ret = 0;
	int offs;
	phys_addr_t optee_start;
	size_t optee_size;

	/*
	 * No TEE space allocated indicating no TEE running, so no
	 * need to add optee node in dts
	 */
	if (!rom_pointer[1])
		return 0;

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
