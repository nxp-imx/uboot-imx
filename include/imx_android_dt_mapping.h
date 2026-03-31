/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __IMX_ANDROID_DT_MAPPING_H__
#define __IMX_ANDROID_DT_MAPPING_H__

/*
 * Default dtb name if the "fdt_name" variable is not set.
 */
#ifdef CONFIG_TARGET_IMX95_19X19_EVK
#define IMX_ANDROID_DEFAULT_FDT_NAME "imx95"
#elif defined(CONFIG_TARGET_IMX95_15X15_EVK)
#define IMX_ANDROID_DEFAULT_FDT_NAME "imx95-15x15"
#elif defined(CONFIG_TARGET_IMX95_15X15_FRDM)
#define IMX_ANDROID_DEFAULT_FDT_NAME "imx95-15x15-frdm"
#elif defined(CONFIG_TARGET_VERDIN_IMX95_19X19)
#define IMX_ANDROID_DEFAULT_FDT_NAME "imx95-verdin"
#elif defined(CONFIG_IMX94)
#define IMX_ANDROID_DEFAULT_FDT_NAME "imx943"
#elif defined(CONFIG_IMX93)
#define IMX_ANDROID_DEFAULT_FDT_NAME "imx93"
#elif defined(CONFIG_IMX8QM)
#define IMX_ANDROID_DEFAULT_FDT_NAME "imx8qm"
#elif defined(CONFIG_IMX8QXP)
#define IMX_ANDROID_DEFAULT_FDT_NAME "imx8qxp"
#elif defined(CONFIG_IMX8ULP)
#ifdef CONFIG_TARGET_IMX8ULP_9X9_EVK
#define IMX_ANDROID_DEFAULT_FDT_NAME "imx8ulp-9x9"
#else
#define IMX_ANDROID_DEFAULT_FDT_NAME "imx8ulp"
#endif
#elif defined(CONFIG_IMX8MM)
#define IMX_ANDROID_DEFAULT_FDT_NAME "imx8mm"
#elif defined(CONFIG_IMX8MN)
#define IMX_ANDROID_DEFAULT_FDT_NAME "imx8mn"
#elif defined(CONFIG_IMX8MP)
#define IMX_ANDROID_DEFAULT_FDT_NAME "imx8mp"
#elif defined(CONFIG_IMX8MQ)
#define IMX_ANDROID_DEFAULT_FDT_NAME "imx8mq"
#else
#define IMX_ANDROID_DEFAULT_FDT_NAME ""
#endif
static const char imx_android_default_fdt_name[] = IMX_ANDROID_DEFAULT_FDT_NAME;

#endif //__IMX_ANDROID_DT_MAPPING_H__
