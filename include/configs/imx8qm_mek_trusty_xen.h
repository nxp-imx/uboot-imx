/*
 * Copyright 2020 NXP
 *
 */

#ifndef __IMX8QM_MEK_XEN_TRUSTY_H__
#define __IMX8QM_MEK_XEN_TRUSTY_H__

#ifdef CONFIG_SPL_BUILD
#define CONFIG_AVB_SUPPORT
#define AVB_RPMB
#define CONFIG_SHA256
#define KEYSLOT_HWPARTITION_ID   2
#define KEYSLOT_BLKS             0x3FFF
#define CONFIG_SUPPORT_EMMC_RPMB
#endif

#endif

