/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __IMX8QM_MEK_TRUSTY_XEN_H
#define __IMX8QM_MEK_TRUSTY_XEN_H

#ifdef CONFIG_SPL_BUILD

#undef CONFIG_BLK
#define CONFIG_AVB_SUPPORT
#define AVB_RPMB
#define KEYSLOT_HWPARTITION_ID   2
#define KEYSLOT_BLKS             0x3FFF
#define AVB_AB_I_UNDERSTAND_LIBAVB_AB_IS_DEPRECATED

#define CONFIG_SUPPORT_EMMC_RPMB

#endif

#endif
