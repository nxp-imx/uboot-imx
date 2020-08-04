/*
 * Copyright 2018 NXP.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <asm/arch/sci/sci.h>
#include <asm/mach-imx/boot_mode.h>
#include <malloc.h>
#include <command.h>
#include <asm/arch-imx/cpu.h>
#include <asm/arch/sys_proto.h>
#include <fdt_support.h>
#include <fdtdec.h>
#include <linux/libfdt.h>
#include <linux/io.h>
#include <linux/compat.h>

DECLARE_GLOBAL_DATA_PTR;

#define SC_MAX_PARTS	32

struct scu_rm_part_data {
	bool used;
	bool isolated;
	bool restricted;
	bool grant;
	sc_rm_did_t did;
	sc_rm_pt_t self;
	sc_rm_pt_t parent;
	char *name;
};

static struct scu_rm_part_data rm_part_data[SC_MAX_PARTS];

static int partition_alloc(bool isolated, bool restricted, bool grant, sc_rm_pt_t *pt)
{
	sc_rm_pt_t parent_part, os_part;
	int err;
	int i;

	for (i = 0; i < SC_MAX_PARTS; i++) {
		if (!rm_part_data[i].used)
			break;
	}

	if (i == SC_MAX_PARTS) {
		puts("No empty slots\n");
		return -EINVAL;
	}

	err = sc_rm_get_partition(-1, &parent_part);
	if (err != SC_ERR_NONE) {
		puts("sc_rm_get_partition failure\n");
		return -EINVAL;
	}

	debug("isolated %d, restricted %d, grant %d\n", isolated, restricted, grant);
	err = sc_rm_partition_alloc(-1, &os_part, false, isolated,
				    restricted, grant, false);
	if (err != SC_ERR_NONE) {
		printf("sc_rm_partition_alloc failure %d\n", err);
		return -EINVAL;
	}

	err = sc_rm_set_parent(-1, os_part, parent_part);
	if (err != SC_ERR_NONE) {
		sc_rm_partition_free(-1, os_part);
		return -EINVAL;
	}


	rm_part_data[i].self = os_part;
	rm_part_data[i].parent = parent_part;
	rm_part_data[i].used = true;
	rm_part_data[i].restricted = restricted;
	rm_part_data[i].isolated = isolated;
	rm_part_data[i].grant = grant;

	if (pt)
		*pt = os_part;

	printf("%s: os_part, %d: parent_part, %d\n", __func__, os_part,
	       parent_part);

	return 0;
}

static int do_part_alloc(int argc, char * const argv[])
{
	bool restricted = false, isolated = false, grant = false;
	int ret;

	if (argv[0])
		isolated = simple_strtoul(argv[0], NULL, 10);
	if (argv[1])
		restricted = simple_strtoul(argv[1], NULL, 10);
	if (argv[2])
		grant = simple_strtoul(argv[2], NULL, 10);

	ret = partition_alloc(isolated, restricted, grant, NULL);
	if (ret)
		return CMD_RET_FAILURE;

	return CMD_RET_SUCCESS;
}

static int do_part_dtb(int argc, char * const argv[])
{
	int err;
	sc_rm_pt_t pt;
	char *pathp = "/domu";
	int nodeoffset, subnode;
	int rsrc_size = 0, pad_size = 0;
	int i, ret;
	u32 *rsrc_data = NULL, *pad_data = NULL;
	const struct fdt_property *prop;
	bool init_ignore_domu_power = false;
	char *tmp;
	void *fdt;

	tmp = env_get("domu-init-ignore-poweroff");
	if (tmp && !strncmp(tmp, "yes", 3)) {
		init_ignore_domu_power = true;
		printf("ignore init power off domu power\n");
	}

	if (argc)
		fdt = (void *)simple_strtoul(argv[0], NULL, 16);
	else
		fdt = working_fdt;
	printf("fdt addr %p\n", fdt);
	nodeoffset = fdt_path_offset(fdt, pathp);
	debug("%s %s %p\n", __func__, fdt_get_name(fdt, nodeoffset, NULL), fdt);
	fdt_for_each_subnode(subnode, fdt, nodeoffset) {
		if (!fdtdec_get_is_enabled(fdt, subnode))
			continue;
		if (!fdt_node_check_compatible(fdt, subnode, "xen,domu")) {
			u32 temp;
			prop = fdt_getprop(fdt, subnode, "rsrcs", &rsrc_size);
			if (!prop)
				debug("No rsrcs %s\n", fdt_get_name(fdt, subnode, NULL));
			if (rsrc_size > 0) {
				rsrc_data = kmalloc(rsrc_size, __GFP_ZERO);
				if (!rsrc_data) {
					debug("No mem\n");
					return CMD_RET_FAILURE;
				}
				if (fdtdec_get_int_array(fdt, subnode, "rsrcs",
							 rsrc_data, rsrc_size >> 2)) {
					debug("Error reading rsrcs\n");
					kfree(rsrc_data);
					return CMD_RET_FAILURE;
				}
			}

			prop = fdt_getprop(fdt, subnode, "pads", &pad_size);
			if (!prop)
				debug("No pads %s %d\n", fdt_get_name(fdt, subnode, NULL), pad_size);
			if (pad_size > 0) {
				pad_data = kmalloc(pad_size, __GFP_ZERO);
				if (!pad_data) {
					debug("No mem\n");
					if (rsrc_data != NULL)
						kfree(rsrc_data);
					return CMD_RET_FAILURE;
				}
				if (fdtdec_get_int_array(fdt, subnode, "pads",
							 pad_data, pad_size >> 2)) {
					debug("Error reading pad\n");
					kfree(pad_data);
					kfree(rsrc_data);
					return CMD_RET_FAILURE;
				}
			}

			if ((rsrc_size <= 0) && (pad_size <= 0))
				continue;

			ret = partition_alloc(false, false, true, &pt);
			if (ret)
				goto free_data;

			temp = cpu_to_fdt32(pt);
			ret = fdt_setprop(fdt, subnode, "reg", &temp,
					  sizeof(u32));
			if (ret) {
				printf("Could not set reg property %d\n", ret);
				sc_rm_partition_free(-1, pt);
				goto free_data;
			}

			if (rsrc_size > 0) {
				for (i = 0; i < rsrc_size >> 2; i++) {
					switch (rsrc_data[i]) {
					case SC_R_MU_2A:
					case SC_R_MU_3A:
					case SC_R_MU_4A:
						err = sc_pm_set_resource_power_mode(-1, rsrc_data[i], SC_PM_PW_MODE_ON);
						if (err)
							debug("power on resource %d, err %d\n", rsrc_data[i], err);
						break;
					default:
						if (init_ignore_domu_power)
							break;
						err = sc_pm_set_resource_power_mode(-1, rsrc_data[i], SC_PM_PW_MODE_OFF);
						if (err)
							debug("power off resource %d, err %d\n", rsrc_data[i], err);
						break;
					}
					if (sc_rm_is_resource_owned(-1, rsrc_data[i])) {
						err = sc_rm_assign_resource(-1, pt, rsrc_data[i]);
						debug("pt %d, resource %d, err %d\n", pt, rsrc_data[i], err);
					}
				}
			}

			if (pad_size > 0) {
				for (i = 0; i < pad_size >> 2; i++) {
					if (sc_rm_is_pad_owned(-1, pad_data[i])) {
						err = sc_rm_assign_pad(-1, pt, pad_data[i]);
						debug("pt %d, pad %d, err %d\n", pt, pad_data[i], err);
					}
				}
			}

			free_data:
				if (pad_size > 0)
					kfree(pad_data);
				if (rsrc_size > 0) {
					kfree(rsrc_data);
					rsrc_data = NULL;
				}
		}

	}

	return 0;
}

static int do_part_free(int argc, char * const argv[])
{
	sc_rm_pt_t os_part;
	int err;
	int i;

	if (argc == 0)
		return CMD_RET_FAILURE;

	os_part = simple_strtoul(argv[0], NULL, 10);

	err = sc_rm_partition_free(-1, os_part);
	if (err != SC_ERR_NONE) {
		printf("free partiiton %d err %d\n", os_part, err);
		return CMD_RET_FAILURE;
	}

	for (i = 0; i < SC_MAX_PARTS; i++) {
		if ((rm_part_data[i].self == os_part) && rm_part_data[i].used) {
			rm_part_data[i].used = false;
			break;
		}
	}

	return CMD_RET_SUCCESS;
}

static int do_resource_assign(int argc, char * const argv[])
{
	sc_rm_pt_t os_part;
	int err;
	sc_rsrc_t resource;
	sc_pad_t pad;
	int i, flag;


	if (argc < 3)
		return CMD_RET_FAILURE;

	os_part = simple_strtoul(argv[0], NULL, 10);
	flag = simple_strtoul(argv[1], NULL, 10);
	if (flag)
		pad = simple_strtoul(argv[2], NULL, 10);
	else
		resource = simple_strtoul(argv[2], NULL, 10);

	for (i = 0; i < SC_MAX_PARTS; i++) {
		if ((rm_part_data[i].self == os_part) && rm_part_data[i].used)
			break;
	}

	if (i == SC_MAX_PARTS) {
		puts("Not valid partition\n");
		return CMD_RET_FAILURE;
	}

	if (flag)
		err = sc_rm_assign_pad(-1, os_part, pad);
	else
		err = sc_rm_assign_resource(-1, os_part, resource);
	if (err != SC_ERR_NONE) {
		printf("assign resource/pad error %d\n", err);
		return CMD_RET_FAILURE;
	}

	printf("%s: os_part, %d, %d\n", __func__, os_part,
	       flag ? pad : resource);

	return CMD_RET_SUCCESS;
}

static int do_part_list(int argc, char * const argv[])
{
	int i;

	for (i = 0; i < SC_MAX_PARTS; i++) {
		if (rm_part_data[i].used)
			printf("part id: %d %d\n", rm_part_data[i].self,
			       rm_part_data[i].parent);
	}

	return CMD_RET_SUCCESS;
}

static int do_part_test(int argc, char * const argv[])
{
	sc_err_t err;
	sc_rsrc_t resource;

	if (argc < 1)
		return CMD_RET_FAILURE;

	resource = simple_strtoul(argv[0], NULL, 10);

	err = sc_pm_set_resource_power_mode(-1, resource, SC_PM_PW_MODE_ON);
	if (err == SC_ERR_NOACCESS)
		puts("NO ACCESS\n");

	return CMD_RET_SUCCESS;
}

static int do_scu_rm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc < 2)
		return CMD_RET_USAGE;

	if (!strcmp(argv[1], "alloc"))
		return do_part_alloc(argc - 2, argv + 2);
	else if (!strcmp(argv[1], "dtb"))
		return do_part_dtb(argc - 2, argv + 2);
	else if (!strcmp(argv[1], "free"))
		return do_part_free(argc - 2, argv + 2);
	else if (!strcmp(argv[1], "assign"))
		return do_resource_assign(argc - 2, argv + 2);
	else if (!strcmp(argv[1], "test"))
		return do_part_test(argc - 2, argv + 2);
	else if (!strcmp(argv[1], "print"))
		return do_part_list(argc - 2, argv + 2);

	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	scu_rm, CONFIG_SYS_MAXARGS, 1, do_scu_rm,
	"scu partition function",
	"\n"
	"scu_rm alloc [isolated] [restricted] [grant]\n"
	"scu_rm dtb [fdt]\n"
	"scu_rm free pt\n"
	"scu_rm assign pt 0 resource\n"
	"scu_rm assign pt 1 pad\n"
	"scu_rm test resource\n"
	"scu_rm print\n"
);
