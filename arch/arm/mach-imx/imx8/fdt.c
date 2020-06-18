// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 */

#include <common.h>
#include <asm/arch/sci/sci.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/optee.h>
#include <dm/ofnode.h>
#include <fdt_support.h>
#include <linux/libfdt.h>
#include <malloc.h>

DECLARE_GLOBAL_DATA_PTR;

struct edma_ch_map {
	sc_rsrc_t ch_start_rsrc;
	u32 ch_start_regs;
	u32 ch_num;
	const char* node_path;
};

static bool check_owned_resource(sc_rsrc_t rsrc_id)
{
	bool owned;

	owned = sc_rm_is_resource_owned(-1, rsrc_id);

	return owned;
}

static int disable_fdt_node(void *blob, int nodeoffset)
{
	int rc, ret;
	const char *status = "disabled";

	do {
		rc = fdt_setprop(blob, nodeoffset, "status", status,
				 strlen(status) + 1);
		if (rc) {
			if (rc == -FDT_ERR_NOSPACE) {
				ret = fdt_increase_size(blob, 512);
				if (ret)
					return ret;
			}
		}
	} while (rc == -FDT_ERR_NOSPACE);

	return rc;
}

static void fdt_edma_debug_int_array(u32 *array, int count, u32 stride)
{
#ifdef DEBUG
	int i;
	for (i = 0; i < count; i++) {
		printf("0x%x ", array[i]);
		if (i % stride == stride - 1)
			printf("\n");
	}

	printf("\n");
#endif
}

static void fdt_edma_debug_stringlist(const char *stringlist, int length)
{
#ifdef DEBUG
	int i = 0, len;
	while (i < length) {
		printf("%s\n", stringlist);

		len = strlen(stringlist) + 1;
		i += len;
		stringlist += len;
	}

	printf("\n");
#endif
}

static void fdt_edma_swap_int_array(u32 *array, int count)
{
	int i;
	for (i = 0; i < count; i++) {
		array[i] = cpu_to_fdt32(array[i]);
	}
}

static int fdt_edma_update_int_array(u32 *array, int count, u32 *new_array, u32 stride, int *remove_array, int remove_count)
{
	int i = 0, j, curr = 0, new_cnt = 0;

	do {
		if (remove_count && curr == remove_array[i]) {
			i++;
			remove_count--;
			array += stride;
		} else {
			for (j = 0; j< stride; j++) {
				*new_array = *array;
				new_array++;
				array++;
			}
			new_cnt+= j;
		}
		curr++;
	} while ((curr * stride) < count);

	return new_cnt;
}

static int fdt_edma_update_stringlist(const char *stringlist, int stringlist_count, char *newlist, int *remove_array, int remove_count)
{
	int i = 0, curr = 0, new_len = 0;
	int length;

	debug("fdt_edma_update_stringlist, remove_cnt %d\n", remove_count);

	do {
		if (remove_count && curr == remove_array[i]) {
			debug("remove %s at %d\n", stringlist, remove_array[i]);

			length = strlen(stringlist) + 1;
			stringlist += length;
			i++;
			remove_count--;
		} else {
			length = strlen(stringlist) + 1;
			strcpy(newlist, stringlist);

			debug("copy %s, %s, curr %d, len %d\n", newlist, stringlist, curr, length);

			stringlist += length;
			newlist += length;
			new_len += length;
		}
		curr++;
	} while (curr < stringlist_count);

	return new_len;
}

static int fdt_edma_get_channel_id(u32 *regs, int index, struct edma_ch_map *edma)
{
	u32 ch_reg = regs[(index << 2) + 1];
	u32 ch_reg_size = regs[(index << 2) + 3];
	int ch_id = (ch_reg - edma->ch_start_regs) / ch_reg_size;
	if (ch_id >= edma->ch_num)
		return -1;

	return ch_id;
}

static __maybe_unused void update_fdt_edma_nodes(void *blob)
{
	struct edma_ch_map edma_qm[] = {
		{ SC_R_DMA_0_CH0, 0x5a200000, 32, "/dma-controller@5a1f0000"},
		{ SC_R_DMA_1_CH0, 0x5aa00000, 32, "/dma-controller@5a9f0000"},
		{ SC_R_DMA_2_CH0, 0x59200000, 5, "/dma-controller@591F0000"},
		{ SC_R_DMA_2_CH5, 0x59250000, 27, "/dma-controller@591F0000"},
		{ SC_R_DMA_3_CH0, 0x59a00000, 32, "/dma-controller@599F0000"},
	};

	struct edma_ch_map edma_qxp[] = {
		{ SC_R_DMA_0_CH0, 0x59200000, 32, "/dma-controller@591F0000"},
		{ SC_R_DMA_1_CH0, 0x59a00000, 32, "/dma-controller@599F0000"},
		{ SC_R_DMA_2_CH0, 0x5a200000, 5, "/dma-controller@5a1f0000"},
		{ SC_R_DMA_2_CH5, 0x5a250000, 27, "/dma-controller@5a1f0000"},
		{ SC_R_DMA_3_CH0, 0x5aa00000, 32, "/dma-controller@5a9f0000"},
	};

	u32 i, j, edma_size;
	int nodeoff, ret;
	struct edma_ch_map *edma_array;

	if (is_imx8qm()) {
		edma_array = edma_qm;
		edma_size = ARRAY_SIZE(edma_qm);
	} else {
		edma_array = edma_qxp;
		edma_size = ARRAY_SIZE(edma_qxp);
	}

	for (i = 0; i < edma_size; i++, edma_array++) {
		u32 regs[128];
		u32 interrupts[96];
		u32 dma_channels;
		int regs_count, interrupts_count, int_names_count;

		const char *list;
		int list_len, newlist_len;
		int remove[32];
		int remove_cnt = 0;
		char * newlist;

		nodeoff = fdt_path_offset(blob, edma_array->node_path);
		if (nodeoff < 0)
			continue; /* Not found, skip it */

		printf("%s, %d\n", edma_array->node_path, nodeoff);

		regs_count = fdtdec_get_int_array_count(blob, nodeoff, "reg", regs, 128);
		debug("regs_count %d\n", regs_count);
		if (regs_count < 0)
			continue;

		interrupts_count = fdtdec_get_int_array_count(blob, nodeoff, "interrupts", interrupts, 96);
		debug("interrupts_count %d\n", interrupts_count);
		if (interrupts_count < 0)
			continue;

		dma_channels = fdtdec_get_uint(blob, nodeoff, "dma-channels", 0);
		if (dma_channels == 0)
			continue;

		list = fdt_getprop(blob, nodeoff, "interrupt-names", &list_len);
		if (!list)
			continue;

		int_names_count = fdt_stringlist_count(blob, nodeoff, "interrupt-names");

		fdt_edma_debug_int_array(regs, regs_count, 4);
		fdt_edma_debug_int_array(interrupts, interrupts_count, 3);
		fdt_edma_debug_stringlist(list, list_len);

		for (j = 0; j < (regs_count >> 2); j++) {
			int ch_id = fdt_edma_get_channel_id(regs, j, edma_array);
			if (ch_id < 0)
				continue;

			if (!check_owned_resource(edma_array->ch_start_rsrc + ch_id)) {
				printf("remove edma items %d\n", j);

				dma_channels--;

				remove[remove_cnt] = j;
				remove_cnt++;
			}
		}

		if (remove_cnt > 0) {
			u32 new_regs[128];
			u32 new_interrupts[96];

			regs_count = fdt_edma_update_int_array(regs, regs_count, new_regs, 4, remove, remove_cnt);
			interrupts_count = fdt_edma_update_int_array(interrupts, interrupts_count, new_interrupts, 3, remove, remove_cnt);

			fdt_edma_debug_int_array(new_regs, regs_count, 4);
			fdt_edma_debug_int_array(new_interrupts, interrupts_count, 3);

			fdt_edma_swap_int_array(new_regs, regs_count);
			fdt_edma_swap_int_array(new_interrupts, interrupts_count);

			/* malloc a new string list */
			newlist = (char *)malloc(list_len);
			if (!newlist) {
				printf("malloc new string list failed, len=%d\n", list_len);
				continue;
			}

			newlist_len = fdt_edma_update_stringlist(list, int_names_count, newlist, remove, remove_cnt);
			fdt_edma_debug_stringlist(newlist, newlist_len);

			ret = fdt_setprop(blob, nodeoff, "reg", new_regs, regs_count * sizeof(u32));
			if (ret)
				printf("fdt_setprop regs error %d\n", ret);

			ret = fdt_setprop(blob, nodeoff, "interrupts", new_interrupts, interrupts_count * sizeof(u32));
			if (ret)
				printf("fdt_setprop interrupts error %d\n", ret);

			ret = fdt_setprop_u32(blob, nodeoff, "dma-channels", dma_channels);
			if (ret)
				printf("fdt_setprop_u32 dma-channels error %d\n", ret);

			ret = fdt_setprop(blob, nodeoff, "interrupt-names", newlist, newlist_len);
			if (ret)
				printf("fdt_setprop interrupt-names error %d\n", ret);

			free(newlist);
		}
	}
}

static bool check_owned_resources_in_pd_tree(void *blob, int nodeoff,
	unsigned int *unowned_rsrc)
{
	unsigned int rsrc_id;
	int phplen;
	const fdt32_t *php;

	/* Search the ancestors nodes in current SS power-domain tree,
	*   if all ancestors' resources are owned,  we can enable the node,
	*   otherwise any ancestor is not owned, we should disable the node.
	*/

	do {
		php = fdt_getprop(blob, nodeoff, "power-domains", &phplen);
		if (!php) {
			debug("   - ignoring no power-domains\n");
			break;
		}
		if (phplen != 4) {
			printf("ignoring %s power-domains of unexpected length %d\n",
					fdt_get_name(blob, nodeoff, NULL), phplen);
			break;
		}
		nodeoff = fdt_node_offset_by_phandle(blob, fdt32_to_cpu(*php));

		rsrc_id = fdtdec_get_uint(blob, nodeoff, "reg", 0);
		if (rsrc_id == SC_R_NONE) {
			debug("%s's power domain use SC_R_NONE\n",
				fdt_get_name(blob, nodeoff, NULL));
			break;
		}

		debug("power-domains node 0x%x, resource id %u\n", nodeoff, rsrc_id);

		if (!check_owned_resource(rsrc_id)) {
			if (unowned_rsrc != NULL)
				*unowned_rsrc = rsrc_id;
			return false;
		}
	} while (fdt_node_check_compatible(blob, nodeoff, "nxp,imx8-pd"));

	return true;
}

static void update_fdt_with_owned_resources_legacy(void *blob)
{
	/* Traverses the fdt nodes,
	  * check its power domain and use the resource id in the power domain
	  * for checking whether it is owned by current partition
	  */

	int offset = 0, next_off;
	int depth = 0, next_depth;
	unsigned int rsrc_id;
	int rc;

	for (offset = fdt_next_node(blob, offset, &depth); offset > 0;
		 offset = fdt_next_node(blob, offset, &depth)) {

		debug("Node name: %s, depth %d\n", fdt_get_name(blob, offset, NULL), depth);

		if (!fdtdec_get_is_enabled(blob, offset)) {
			debug("   - ignoring disabled device\n");
			continue;
		}

		if (!fdt_node_check_compatible(blob, offset, "nxp,imx8-pd")) {
			/* Skip to next depth=1 node*/
			next_off = offset;
			next_depth = depth;
			do {
				offset = next_off;
				depth = next_depth;
				next_off = fdt_next_node(blob, offset, &next_depth);
				if (next_off < 0 || next_depth < 1)
					break;

				debug("PD name: %s, offset %d, depth %d\n",
					fdt_get_name(blob, next_off, NULL), next_off, next_depth);
			} while (next_depth > 1);

			continue;
		}

		if (!check_owned_resources_in_pd_tree(blob, offset, &rsrc_id)) {
			/* If the resource is not owned, disable it in FDT */
			rc = disable_fdt_node(blob, offset);
			if (!rc)
				printf("Disable %s, resource id %u not owned\n",
					fdt_get_name(blob, offset, NULL), rsrc_id);
			else
				printf("Unable to disable %s, err=%s\n",
					fdt_get_name(blob, offset, NULL), fdt_strerror(rc));
		}

	}
}

static __maybe_unused void update_fdt_with_owned_resources(void *blob)
{
	/*
	 * Traverses the fdt nodes, check its power domain and use
	 * the resource id in the power domain for checking whether
	 * it is owned by current partition
	 */
	struct fdtdec_phandle_args args;
	int offset = 0, depth = 0;
	u32 rsrc_id;
	int rc, i, count;

	/* Check the new PD, if not find, continue with old PD tree */
	count = fdt_node_offset_by_compatible(blob, -1, "fsl,scu-pd");
	if (count < 0)
		return update_fdt_with_owned_resources_legacy(blob);

	for (offset = fdt_next_node(blob, offset, &depth); offset > 0;
	     offset = fdt_next_node(blob, offset, &depth)) {
		debug("Node name: %s, depth %d\n",
		      fdt_get_name(blob, offset, NULL), depth);

		if (!fdt_get_property(blob, offset, "power-domains", NULL)) {
			debug("   - ignoring node %s\n",
			      fdt_get_name(blob, offset, NULL));
			continue;
		}

		if (!fdtdec_get_is_enabled(blob, offset)) {
			debug("   - ignoring node %s\n",
			      fdt_get_name(blob, offset, NULL));
			continue;
		}

		i = 0;
		while (true) {
			rc = fdtdec_parse_phandle_with_args(blob, offset,
							    "power-domains",
							    "#power-domain-cells",
							    0, i++, &args);
			if (rc == -ENOENT) {
				break;
			} else if (rc) {
				printf("Parse power-domains of %s wrong: %d\n",
				       fdt_get_name(blob, offset, NULL), rc);
				continue;
			}

			rsrc_id = args.args[0];

			if (!check_owned_resource(rsrc_id)) {
				rc = disable_fdt_node(blob, offset);
				if (!rc) {
					printf("Disable %s rsrc %u not owned\n",
					       fdt_get_name(blob, offset, NULL),
					       rsrc_id);
				} else {
					printf("Unable to disable %s, err=%s\n",
					       fdt_get_name(blob, offset, NULL),
					       fdt_strerror(rc));
				}
			}
		}
	}
}

static int config_smmu_resource_sid(int rsrc, int sid)
{
	int err;

	err = sc_rm_set_master_sid(-1, rsrc, sid);
	debug("set_master_sid rsrc=%d sid=0x%x err=%d\n", rsrc, sid, err);
	if (err != SC_ERR_NONE) {
		if (!check_owned_resource(rsrc)) {
			printf("%s rsrc[%d] not owned\n", __func__, rsrc);
			return -1;
		}
		pr_err("fail set_master_sid rsrc=%d sid=0x%x err=%d\n", rsrc, sid, err);
		return -EINVAL;
	}

	return 0;
}

static int config_smmu_fdt_device_sid(void *blob, int device_offset, int sid)
{
	const char *name = fdt_get_name(blob, device_offset, NULL);
	struct fdtdec_phandle_args args;
	int rsrc, ret;
	int proplen;
	const fdt32_t *prop;
	int i;

	prop = fdt_getprop(blob, device_offset, "fsl,sc_rsrc_id", &proplen);
	if (prop) {
		int i;

		debug("configure node %s sid 0x%x for %d resources\n",
		      name, sid, (int)(proplen / sizeof(fdt32_t)));
		for (i = 0; i < proplen / sizeof(fdt32_t); ++i) {
			ret = config_smmu_resource_sid(fdt32_to_cpu(prop[i]),
						       sid);
			if (ret)
				return ret;
		}

		return 0;
	}

	i = 0;
	while (true) {
		ret = fdtdec_parse_phandle_with_args(blob, device_offset,
						     "power-domains",
						     "#power-domain-cells",
						     0, i++, &args);
		if (ret == -ENOENT) {
			return 0;
		} else if (ret) {
			printf("Parse power-domains of node %s wrong: %d\n",
			       fdt_get_name(blob, device_offset, NULL), ret);
			continue;
		}

		rsrc = args.args[0];
		debug("configure node %s sid 0x%x rsrc=%d\n",
		      name, sid, rsrc);

		ret = config_smmu_resource_sid(rsrc, sid);
		if (ret)
			break;
	}

	return ret;
}

static int config_smmu_fdt(void *blob)
{
	int offset, proplen, i, ret;
	const fdt32_t *prop;
	const char *name;

	/* Legacy smmu bindings, still used by xen. */
	offset = fdt_node_offset_by_compatible(blob, 0, "arm,mmu-500");
	prop = fdt_getprop(blob, offset, "mmu-masters", &proplen);
	if (offset > 0 && prop) {
		debug("found legacy mmu-masters property\n");

		for (i = 0; i < proplen / 8; ++i) {
			u32 phandle = fdt32_to_cpu(prop[2 * i]);
			int sid = fdt32_to_cpu(prop[2 * i + 1]);
			int device_offset;

			device_offset = fdt_node_offset_by_phandle(blob,
								   phandle);
			if (device_offset < 0) {
				pr_err("Not find device from mmu_masters: %d",
				       device_offset);
				continue;
			}
			ret = config_smmu_fdt_device_sid(blob, device_offset,
							 sid);
			if (ret)
				return ret;
		}

		/* Ignore new bindings if old bindings found, just like linux. */
		return 0;
	}

	/* Generic smmu bindings */
	offset = 0;
	while ((offset = fdt_next_node(blob, offset, NULL)) > 0) {
		name = fdt_get_name(blob, offset, NULL);
		prop = fdt_getprop(blob, offset, "iommus", &proplen);
		if (!prop)
			continue;
		debug("node %s iommus proplen %d\n", name, proplen);

		if (proplen == 12) {
			int sid = fdt32_to_cpu(prop[1]);

			config_smmu_fdt_device_sid(blob, offset, sid);
		} else if (proplen != 4) {
			debug("node %s ignore unexpected iommus proplen=%d\n",
			      name, proplen);
		}
	}

	return 0;
}

int ft_system_setup(void *blob, bd_t *bd)
{
	int ret;

#if (CONFIG_BOOTAUX_RESERVED_MEM_SIZE != 0x00)
	int off;
	off = fdt_add_mem_rsv(blob, CONFIG_BOOTAUX_RESERVED_MEM_BASE,
				      CONFIG_BOOTAUX_RESERVED_MEM_SIZE);
	if (off < 0)
		printf("Failed	to reserve memory for bootaux: %s\n",
			   fdt_strerror(off));
#endif

#ifndef CONFIG_SKIP_RESOURCE_CHECKING
	update_fdt_with_owned_resources(blob);
	update_fdt_edma_nodes(blob);
#endif

	if (is_imx8qm()) {
		ret = config_smmu_fdt(blob);
		if (ret)
			return ret;
	}

	return ft_add_optee_node(blob, bd);
}
