/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __IMX_ANDROID_DT_MAPPING_H__
#define __IMX_ANDROID_DT_MAPPING_H__

/* This mapping covers the dtb files listed in config "TARGET_BOARD_DTS_CONFIG" which is
 * defined in "{ANDROID_ROOT}/device/nxp/{SOC}/{BOARD}/BoardConfig.mk".
 *
 * The sequence should be kept same or the "id <--> fdt_name" mapping would be wrong.
 * */
static char *imx_android_dt_mapping[] = {
#ifdef CONFIG_IMX95
	/* For i.MX 95 */
	"imx95",
	"imx95-ox03c10",
	"imx95-ap1302",
	"imx95-mipi-lvds1",
	"imx95-mipi-panel",
	"imx95-lvds0",
	"imx95-lvds0-dual-os08a20",
	"imx95-lvds-dualdisp",
	"imx95-lvds-panel",
	"imx95-cs42888",
	"imx95-rpmsg",
	"imx95-mipi4k",
	"imx95-dsi-serdes",
	"imx95-verdin",
	"imx95-verdin-ox03c10",
	"imx95-verdin-ap1302",
	"imx95-verdin-lt8912",
	"imx95-verdin-10inch-panel-lvds",
	"imx95-verdin-10inch-panel-dsi",
	"imx95-verdin-mipi-panel",
	"imx95-verdin-mipi4k",
	"imx95-15x15",
	"imx95-15x15-ox03c10",
	"imx95-15x15-ap1302",
	"imx95-15x15-mipi-panel",
	"imx95-15x15-aud-hat",
	"imx95-15x15-mqs",
	"imx95-15x15-mipi4k",
	"imx95-15x15-boe-panel-lvds1",
	"imx95-15x15-frdm",
	"imx95-15x15-frdm-dual-os08a20",
	"imx95-15x15-frdm-boe-panel",
#elif defined(CONFIG_IMX94)
	/* For i.MX 943 */
	"imx943",
	"imx943-sdwifi",
#elif defined(CONFIG_IMX93)
	/* For i.MX 93 */
	"imx93",
	"imx93-iw612",
	"imx93-frdm-iw612",
	"imx93-frdm-iw612-tianma-wvga",
#elif defined(CONFIG_ANDROID_SUPPORT) && (defined(CONFIG_IMX8QM) || defined(CONFIG_IMX8QXP))
	/* For i.MX 8QM/QXP Standard Android */
	"imx8qm",
	"imx8qm-ov5640-csi0",
	"imx8qm-ov5640-csi1",
	"imx8qm-mipi-panel",
	"imx8qm-mipi-panel-rm67191",
	"imx8qm-hdmi",
	"imx8qm-hdmi-rx",
	"imx8qm-lvds1-panel",
	"imx8qxp",
	"imx8qxp-ov5640-csi",
	"imx8qxp-ov5640-parallel",
	"imx8dx",
	"imx8qxp-mipi-panel",
	"imx8qxp-mipi-panel-rm67191",
	"imx8qxp-lvds0-panel",
	"imx8qm-sof",
	"imx8qxp-sof",
	"imx8qm-revd",
	"imx8qm-ov5640-csi0-revd",
	"imx8qm-ov5640-csi1-revd",
	"imx8qm-mipi-panel-revd",
	"imx8qm-mipi-panel-rm67191-revd",
	"imx8qm-hdmi-revd",
	"imx8qm-hdmi-rx-revd",
	"imx8qm-lvds1-panel-revd",
	"imx8qm-sof-revd",
#elif defined(CONFIG_IMX8ULP)
	/* For i.MX 8ULP */
#ifdef CONFIG_TARGET_IMX8ULP_WATCH
	/* For i.MX 8ULP WATCH */
	"imx8ulp",
#else
	/* For i.MX 8ULP EVK and i.MX 8ULP 9x9 EVK */
	"imx8ulp",
	"imx8ulp-hdmi",
	"imx8ulp-epdc",
	"imx8ulp-9x9",
	"imx8ulp-9x9-hdmi",
	"imx8ulp-sof",
	"imx8ulp-lpa",
#endif
#elif defined(CONFIG_IMX8MM)
	/* For i.MX 8MM */
	"imx8mm-ddr4",
	"imx8mm",
	"imx8mm-mipi-panel",
	"imx8mm-mipi-panel-rm67191",
	"imx8mm-m4",
	"imx8mm-iw612",
#elif defined(CONFIG_IMX8MN)
	/* For i.MX 8MN */
	"imx8mn-ddr4",
	"imx8mn",
	"imx8mn-ddr4-mipi-panel",
	"imx8mn-ddr4-mipi-panel-rm67191",
	"imx8mn-ddr4-rpmsg",
	"imx8mn-mipi-panel",
	"imx8mn-mipi-panel-rm67191",
	"imx8mn-rpmsg",
#elif defined(CONFIG_IMX8MP)
	/* For i.MX 8MP */
	"imx8mp",
	"imx8mp-os08a20-ov5640",
	"imx8mp-os08a20",
	"imx8mp-dual-basler",
	"imx8mp-basler-ov5640",
	"imx8mp-basler",
	"imx8mp-ov5640",
#ifdef CONFIG_IMX8M_LPDDR4_FREQ0_2400MTS
	/* With powersave setting */
	"imx8mp-rpmsg",
#else
	/* Without powersave setting */
	"imx8mp-rpmsg",
	"imx8mp-rpmsg-revb4",
#endif
	"imx8mp-lvds",
	"imx8mp-lvds-panel",
	"imx8mp-mipi-panel",
	"imx8mp-mipi-panel-rm67191",
	"imx8mp-sof",
	"imx8mp-sof-revb4",
#ifdef CONFIG_IMX8M_LPDDR4_FREQ0_2400MTS
	/* With powersave setting */
	"imx8mp-powersave",
	"imx8mp-powersave-revb4",
#else
	/* Without powersave setting */
	"imx8mp-powersave-non-rpmsg",
	"imx8mp-powersave-non-rpmsg-revb4",
#endif
	"imx8mp-revb4",
	"imx8mp-os08a20-ov5640-revb4",
	"imx8mp-os08a20-revb4",
	"imx8mp-dual-basler-revb4",
	"imx8mp-basler-ov5640-revb4",
	"imx8mp-basler-revb4",
	"imx8mp-ov5640-revb4",
	"imx8mp-lvds-revb4",
	"imx8mp-lvds-panel-revb4",
	"imx8mp-mipi-panel-revb4",
	"imx8mp-mipi-panel-rm67191-revb4",
	"imx8mp-frdm",
	"imx8mp-frdm-lvds0-panel",
#elif defined(CONFIG_IMX8MQ)
	/* For i.MX 8MQ */
	"imx8mq",
	"imx8mq-sdio",
	"imx8mq-wevk",
	"imx8mq-mipi",
	"imx8mq-dual",
	"imx8mq-mipi-panel",
	"imx8mq-mipi-panel-rm67191",
#endif
	NULL,
};

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
