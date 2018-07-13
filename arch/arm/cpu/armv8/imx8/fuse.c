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
#include <asm/arch/sys_proto.h>

DECLARE_GLOBAL_DATA_PTR;

#define FSL_SIP_OTP_READ             0xc200000A
#define FSL_SIP_OTP_WRITE            0xc200000B

int fuse_read(u32 bank, u32 word, u32 *val)
{
	return fuse_sense(bank, word, val);
}

int fuse_sense(u32 bank, u32 word, u32 *val)
{
	if (bank != 0) {
		printf("Invalid bank argument, ONLY bank 0 is supported\n");
		return -EINVAL;
	}
#if defined(CONFIG_SMC_FUSE)
	unsigned long ret, value;
	ret = call_imx_sip_ret2(FSL_SIP_OTP_READ, (unsigned long)word,
		&value, 0, 0);
	*val = (u32)value;
	return ret;
#else
	sc_err_t err;
	sc_ipc_t ipc;

	ipc = gd->arch.ipc_channel_handle;

	err = sc_misc_otp_fuse_read(ipc, word, val);
	if (err != SC_ERR_NONE) {
		printf("fuse read error: %d\n", err);
		return -EIO;
	}

	return 0;
#endif
}

int fuse_prog(u32 bank, u32 word, u32 val)
{
	if (bank != 0) {
		printf("Invalid bank argument, ONLY bank 0 is supported\n");
		return -EINVAL;
	}

#if defined(CONFIG_SMC_FUSE)
	return call_imx_sip(FSL_SIP_OTP_WRITE, (unsigned long)word,\
		(unsigned long)val, 0, 0);
#else
	printf("Program fuse to i.MX8 in u-boot is forbidden\n");
	return -EPERM;
#endif
}

int fuse_override(u32 bank, u32 word, u32 val)
{
	printf("Override fuse to i.MX8 in u-boot is forbidden\n");
	return -EPERM;
}
