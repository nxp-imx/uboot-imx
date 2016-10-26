// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 *
 */

#include <common.h>
#include <linux/libfdt.h>
#include <fdt_support.h>
#include <asm/io.h>
#include <asm/arch/siul.h>
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

void ft_fixup_soc_revision(void *blob)
{
	const u32 socmask_info = readl(SIUL2_MIDR1) &
		(SIUL2_MIDR1_MINOR_MASK | SIUL2_MIDR1_MAJOR_MASK);
	const char *path = "/chosen";
	int ret;

	/* The booting guest may implement its own fixups based on the chip
	 * revision. One such example is PCIe erratum ERR009852, which can be
	 * safely ignored iff the chip is newer than revision 0.
	 * So pass this piece of info along in the FDT.
	 */
	ret = fdt_find_and_setprop(blob, path, "soc_revision", &socmask_info,
				   sizeof(u32), 1);
	if (ret)
		printf("WARNING: Could not fix up the S32V234 device-tree, err=%s\n",
		       fdt_strerror(ret));
}

void ft_cpu_setup(void *blob, bd_t *bd)
{
#ifdef CONFIG_MP
	ft_fixup_cpu(blob);
#endif
	ft_fixup_soc_revision(blob);
}
