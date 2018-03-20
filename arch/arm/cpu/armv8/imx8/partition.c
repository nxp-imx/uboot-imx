/*
 * Copyright 2018 NXP.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <asm/imx-common/sci/sci.h>
#include <asm/imx-common/boot_mode.h>
#include <malloc.h>
#include <command.h>
#include <asm/arch-imx/cpu.h>
#include <asm/arch/sys_proto.h>

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

static int do_part_alloc(int argc, char * const argv[])
{
	sc_rm_pt_t parent_part, os_part;
	sc_ipc_t ipc_handle;
	sc_err_t err;
	int i;
	bool restricted = false, isolated = false, grant = false;

	for (i = 0; i < SC_MAX_PARTS; i++) {
		if (!rm_part_data[i].used)
			break;
	}

	if (i == SC_MAX_PARTS) {
		puts("No empty slots\n");
		return CMD_RET_FAILURE;
	}

	ipc_handle = gd->arch.ipc_channel_handle;

	err = sc_rm_get_partition(ipc_handle, &parent_part);
	if (err != SC_ERR_NONE) {
		puts("sc_rm_get_partition failure\n");
		return CMD_RET_FAILURE;
	}

	isolated = simple_strtoul(argv[0], NULL, 10);
	restricted = simple_strtoul(argv[1], NULL, 10);
	grant = simple_strtoul(argv[2], NULL, 10);
	/* Refine here */
	err = sc_rm_partition_alloc(ipc_handle, &os_part, false, isolated,
				    restricted, grant, false);
	if (err != SC_ERR_NONE) {
		printf("sc_rm_partition_alloc failure %d\n", err);
		return CMD_RET_FAILURE;
	}

	err = sc_rm_set_parent(ipc_handle, os_part, parent_part);
	if (err != SC_ERR_NONE) {
		sc_rm_partition_free(ipc_handle, os_part);
		return CMD_RET_FAILURE;
	}


	rm_part_data[i].self = os_part;
	rm_part_data[i].parent = parent_part;
	rm_part_data[i].used = true;
	rm_part_data[i].restricted = restricted;
	rm_part_data[i].isolated = isolated;
	rm_part_data[i].grant = grant;

	printf("%s: os_part, %d: parent_part, %d\n", __func__, os_part,
	       parent_part);

	return CMD_RET_SUCCESS;
}

static int do_part_free(int argc, char * const argv[])
{
	sc_rm_pt_t os_part;
	sc_ipc_t ipc_handle;
	sc_err_t err;
	ipc_handle = gd->arch.ipc_channel_handle;

	if (argc == 0)
		return CMD_RET_FAILURE;

	os_part = simple_strtoul(argv[0], NULL, 10);

	err = sc_rm_partition_free(ipc_handle, os_part);
	if (err != SC_ERR_NONE) {
		printf("free partiiton %d err %d\n", os_part, err);
		return CMD_RET_FAILURE;
	}

	rm_part_data[os_part].used = false;

	return CMD_RET_SUCCESS;
}

static int do_resource_assign(int argc, char * const argv[])
{
	sc_rm_pt_t os_part;
	sc_ipc_t ipc_handle;
	sc_err_t err;
	sc_rsrc_t resource;
	sc_pad_t pad;
	int i, flag;

	ipc_handle = gd->arch.ipc_channel_handle;

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
		err = sc_rm_assign_pad(ipc_handle, os_part, pad);
	else
		err = sc_rm_assign_resource(ipc_handle, os_part, resource);
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

static int do_scu_rm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc < 2)
		return CMD_RET_USAGE;

	if (!strcmp(argv[1], "alloc"))
		return do_part_alloc(argc - 2, argv + 2);
	if (!strcmp(argv[1], "free"))
		return do_part_free(argc - 2, argv + 2);
	else if (!strcmp(argv[1], "assign"))
		return do_resource_assign(argc - 2, argv + 2);
	else if (!strcmp(argv[1], "print"))
		return do_part_list(argc - 2, argv + 2);

	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	scu_rm, CONFIG_SYS_MAXARGS, 1, do_scu_rm,
	"scu partition function",
	"\n"
	"scu_rm alloc [isolated] [restricted] [grant]\n"
	"scu_rm free pt\n"
	"scu_rm assign pt 0 resource\n"
	"scu_rm assign pt 1 pad\n"
	"scu_rm print\n"
);
