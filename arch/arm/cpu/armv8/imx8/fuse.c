/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <fuse.h>
#include <asm/imx-common/sci/sci.h>

DECLARE_GLOBAL_DATA_PTR;

int fuse_read(u32 bank, u32 word, u32 *val)
{
	return fuse_sense(bank, word, val);
}

int fuse_sense(u32 bank, u32 word, u32 *val)
{
	sc_err_t err;
	sc_ipc_t ipc;

	if (bank != 0) {
		printf("Invalid bank argument, ONLY bank 0 is supported\n");
		return -EINVAL;
	}

	ipc = gd->arch.ipc_channel_handle;

	err = sc_misc_otp_fuse_read(ipc, word, val);
	if (err != SC_ERR_NONE) {
		printf("fuse read error: %d\n", err);
		return -EIO;
	}

	return 0;
}

int fuse_prog(u32 bank, u32 word, u32 val)
{
	printf("Program fuse to i.MX8 in u-boot is forbidden\n");
	return -EPERM;
}

int fuse_override(u32 bank, u32 word, u32 val)
{
	printf("Override fuse to i.MX8 in u-boot is forbidden\n");
	return -EPERM;
}
