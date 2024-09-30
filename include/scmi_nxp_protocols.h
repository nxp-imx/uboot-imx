/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright 2023 NXP
 */

#ifndef _SCMI_NXP_PROTOCOLS_H
#define _SCMI_NXP_PROTOCOLS_H

#include <linux/bitops.h>
#include <asm/types.h>

enum scmi_imx_protocol {
	SCMI_IMX_PROTOCOL_ID_MISC = 0x84,
};

#define SCMI_PAYLOAD_LEN	100

#define SCMI_ARRAY(X, Y)	((SCMI_PAYLOAD_LEN - (X)) / sizeof(Y))

#define SCMI_IMX_MISC_BUILD_INFO	0x6
#define SCMI_IMX_MISC_RESET_REASON	0xA
#define SCMI_IMX_MISC_CFG_INFO		0xC

struct scmi_imx_misc_reset_reason_in {
#define MISC_REASON_FLAG_SYSTEM		BIT(0)
	u32 flags;
};

struct scmi_imx_misc_reset_reason_out {
	s32 status;
	/* Boot reason flags */
#define MISC_BOOT_FLAG_VLD	BIT(31)
#define MISC_BOOT_FLAG_ORG_VLD	BIT(28)
#define MISC_BOOT_FLAG_ORIGIN	GENMASK(27, 24)
#define MISC_BOOT_FLAG_O_SHIFT	24
#define MISC_BOOT_FLAG_ERR_VLD	BIT(23)
#define MISC_BOOT_FLAG_ERR_ID	GENMASK(22, 8)
#define MISC_BOOT_FLAG_E_SHIFT	8
#define MISC_BOOT_FLAG_REASON	GENMASK(7, 0)
	u32 bootflags;
	/* Shutdown reason flags */
#define MISC_SHUTDOWN_FLAG_VLD		BIT(31)
#define MISC_SHUTDOWN_FLAG_EXT_LEN	GENMASK(30, 29)
#define MISC_SHUTDOWN_FLAG_ORG_VLD	BIT(28)
#define MISC_SHUTDOWN_FLAG_ORIGIN	GENMASK(27, 24)
#define MISC_SHUTDOWN_FLAG_O_SHIFT	24
#define MISC_SHUTDOWN_FLAG_ERR_VLD	BIT(23)
#define MISC_SHUTDOWN_FLAG_ERR_ID	GENMASK(22, 8)
#define MISC_SHUTDOWN_FLAG_E_SHIFT	8
#define MISC_SHUTDOWN_FLAG_REASON	GENMASK(7, 0)
	u32 shutdownflags;
	/* Array of extended info words */
#define MISC_MAX_EXTINFO	SCMI_ARRAY(16, u32)
	u32 extInfo[MISC_MAX_EXTINFO];
};

struct scmi_imx_misc_cfg_info_out {
	s32 status;
	/* Mode selector value */
	u32 msel;
#define MISC_MAX_CFGNAME	16
	/* Config (cfg) file basename */
	char cfgname[MISC_MAX_CFGNAME];
};

struct scmi_imx_misc_build_info_out {
	/* Return status */
	s32 status;
	/* Build number */
	u32 buildnum;
	/* Most significant 32 bits of the git commit hash */
	u32 buildcommit;
#define MISC_MAX_BUILDDATE	16
	/* Date of build */
	char builddate[MISC_MAX_BUILDDATE];
#define MISC_MAX_BUILDTIME	16
	/* Time of build */
	char buildtime[MISC_MAX_BUILDTIME];
};
#endif
