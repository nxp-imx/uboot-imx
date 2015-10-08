// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 *
 */

#include <common.h>
#include <linux/libfdt.h>
#include <fdt_support.h>
#include "mp.h"

#ifdef CONFIG_MP
void ft_fixup_cpu(void *blob)
{
	int off;
	__maybe_unused u64 spin_tbl_addr = (u64)get_spin_tbl_addr();
	u64 *reg;
	u64 val;

	off = fdt_node_offset_by_prop_value(blob, -1, "device_type", "cpu", 4);
	while (off != -FDT_ERR_NOTFOUND) {
		reg = (u64 *)fdt_getprop(blob, off, "reg", 0);
		if (reg) {
			val = spin_tbl_addr;
#ifndef CONFIG_FSL_SMP_RELEASE_ALL
			val += id_to_core(fdt64_to_cpu(*reg)) * SIZE_BOOT_ENTRY;
#endif
			val = cpu_to_fdt64(val);
			fdt_setprop_string(blob, off, "enable-method",
					   "spin-table");
			fdt_setprop(blob, off, "cpu-release-addr",
				    &val, sizeof(val));
		} else {
			puts("cpu NULL\n");
		}
		off = fdt_node_offset_by_prop_value(blob, off, "device_type",
						    "cpu", 4);
	}
	/*
	 * Boot page and spin table can be reserved here if not done staticlly
	 * in device tree.
	 *
	 * fdt_add_mem_rsv(blob, bootpg,
	 *		   *((u64 *)&(__secondary_boot_page_size)));
	 * If defined CONFIG_FSL_SMP_RELEASE_ALL, the release address should
	 * also be reserved.
	 */
}
#endif

void ft_cpu_setup(void *blob, bd_t *bd)
{
#ifdef CONFIG_MP
	ft_fixup_cpu(blob);
#endif
}
