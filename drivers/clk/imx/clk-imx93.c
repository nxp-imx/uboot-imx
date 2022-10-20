// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2022 NXP
 *
 * Author: Alice Guo <alice.guo@nxp.com>
 */

#include <asm/io.h>
#include <common.h>
#include <clk-uclass.h>
#include <dm.h>
#include <dt-bindings/clock/imx93-clock.h>
#include <linux/delay.h>

#include "clk.h"

/* Low-speed clocks operating at <=133Mhz */
static const char *const low_speed_sels[] = { "clock-osc-24m", "sys_pll_pfd0_div2", "sys_pll_pfd1_div2", "video_pll" };
/* Non-IO clocks operating at 133-399MHz */
static const char *const non_io_sels[] = { "clock-osc-24m", "sys_pll_pfd0_div2", "sys_pll_pfd1_div2", "sys_pll_pfd2_div2" };
/* Clocks for IP with max frequency in the range 400-1000MHz */
static const char *const fast_speed_sels[] = { "clock-osc-24m", "sys_pll_pfd0", "sys_pll_pfd1", "sys_pll_pfd2" };
/* Audio interface-related clocks, such as SAI, MQS and SPDIF */
static const char *const audio_sels[] = { "clock-osc-24m", "audio_pll", "video_pll", "clk_ext" };
/* Video interface-related clocks */
static const char *const video_sels[] = { "clock-osc-24m", "audio_pll", "video_pll", "sys_pll_pfd0" };
/* Special case CKO1 and CKO2 clocks */
static const char *const cko1_sels[] = { "clock-osc-24m", "sys_pll_pfd0", "sys_pll_pfd1", "audio_pll" };
static const char *const cko2_sels[] = { "clock-osc-24m", "sys_pll_pfd0", "sys_pll_pfd1", "video_pll" };
/* Special case TPM clocks */
static const char *const tpm_sels[] = { "clock-osc-24m", "sys_pll_pfd0", "audio_pll", "clk_ext" };
static const char *const misc_sels[] = { "clock-osc-24m", "audio_pll", "video_pll", "sys_pll_pfd2" };

struct imx93_clk_root {
	u32 clk_id;
	char *name;
	const char * const *parent_names;
	u32 off;
	unsigned long flags;
};

static struct imx93_clk_root clk_roots[] = {
	{ IMX93_CLK_A55_PERIPH, "a55_periph_root", fast_speed_sels, 0x0000, CLK_IS_CRITICAL },
	{ IMX93_CLK_A55_MTR_BUS, "a55_mtr_bus_root", low_speed_sels, 0x0080, CLK_IS_CRITICAL },
	{ IMX93_CLK_A55, "a55_root", fast_speed_sels, 0x0100, CLK_IS_CRITICAL },
	{ IMX93_CLK_M33, "m33_root", low_speed_sels, 0x0180, CLK_IS_CRITICAL },
	{ IMX93_CLK_BUS_WAKEUP, "bus_wakeup_root", low_speed_sels, 0x0280, CLK_IS_CRITICAL },
	{ IMX93_CLK_BUS_AON, "bus_aon_root", low_speed_sels, 0x0300, CLK_IS_CRITICAL },
	{ IMX93_CLK_WAKEUP_AXI, "wakeup_axi_root", fast_speed_sels, 0x0380, CLK_IS_CRITICAL },
	{ IMX93_CLK_SWO_TRACE, "swo_trace_root", low_speed_sels, 0x0400, },
	{ IMX93_CLK_M33_SYSTICK, "m33_systick_root", low_speed_sels, 0x0480, },
	{ IMX93_CLK_FLEXIO1, "flexio1_root", low_speed_sels, 0x0500, },
	{ IMX93_CLK_FLEXIO2, "flexio2_root", low_speed_sels, 0x0580, },
	{ IMX93_CLK_LPTMR1, "lptmr1_root", low_speed_sels, 0x0700, },
	{ IMX93_CLK_LPTMR2, "lptmr2_root", low_speed_sels, 0x0780, },
	{ IMX93_CLK_TPM2, "tpm2_root", tpm_sels, 0x0880, },
	{ IMX93_CLK_TPM4, "tpm4_root", tpm_sels, 0x0980, },
	{ IMX93_CLK_TPM5, "tpm5_root", tpm_sels, 0x0a00, },
	{ IMX93_CLK_TPM6, "tpm6_root", tpm_sels, 0x0a80, },
	{ IMX93_CLK_FLEXSPI1, "flexspi1_root", fast_speed_sels, 0x0b00, },
	{ IMX93_CLK_CAN1, "can1_root", low_speed_sels, 0x0b80, },
	{ IMX93_CLK_CAN2, "can2_root", low_speed_sels, 0x0c00, },
	{ IMX93_CLK_LPUART1, "lpuart1_root", low_speed_sels, 0x0c80, },
	{ IMX93_CLK_LPUART2, "lpuart2_root", low_speed_sels, 0x0d00, },
	{ IMX93_CLK_LPUART3, "lpuart3_root", low_speed_sels, 0x0d80, },
	{ IMX93_CLK_LPUART4, "lpuart4_root", low_speed_sels, 0x0e00, },
	{ IMX93_CLK_LPUART5, "lpuart5_root", low_speed_sels, 0x0e80, },
	{ IMX93_CLK_LPUART6, "lpuart6_root", low_speed_sels, 0x0f00, },
	{ IMX93_CLK_LPUART7, "lpuart7_root", low_speed_sels, 0x0f80, },
	{ IMX93_CLK_LPUART8, "lpuart8_root", low_speed_sels, 0x1000, },
	{ IMX93_CLK_LPI2C1, "lpi2c1_root", low_speed_sels, 0x1080, },
	{ IMX93_CLK_LPI2C2, "lpi2c2_root", low_speed_sels, 0x1100, },
	{ IMX93_CLK_LPI2C3, "lpi2c3_root", low_speed_sels, 0x1180, },
	{ IMX93_CLK_LPI2C4, "lpi2c4_root", low_speed_sels, 0x1200, },
	{ IMX93_CLK_LPI2C5, "lpi2c5_root", low_speed_sels, 0x1280, },
	{ IMX93_CLK_LPI2C6, "lpi2c6_root", low_speed_sels, 0x1300, },
	{ IMX93_CLK_LPI2C7, "lpi2c7_root", low_speed_sels, 0x1380, },
	{ IMX93_CLK_LPI2C8, "lpi2c8_root", low_speed_sels, 0x1400, },
	{ IMX93_CLK_LPSPI1, "lpspi1_root", low_speed_sels, 0x1480, },
	{ IMX93_CLK_LPSPI2, "lpspi2_root", low_speed_sels, 0x1500, },
	{ IMX93_CLK_LPSPI3, "lpspi3_root", low_speed_sels, 0x1580, },
	{ IMX93_CLK_LPSPI4, "lpspi4_root", low_speed_sels, 0x1600, },
	{ IMX93_CLK_LPSPI5, "lpspi5_root", low_speed_sels, 0x1680, },
	{ IMX93_CLK_LPSPI6, "lpspi6_root", low_speed_sels, 0x1700, },
	{ IMX93_CLK_LPSPI7, "lpspi7_root", low_speed_sels, 0x1780, },
	{ IMX93_CLK_LPSPI8, "lpspi8_root", low_speed_sels, 0x1800, },
	{ IMX93_CLK_I3C1, "i3c1_root", low_speed_sels, 0x1880, },
	{ IMX93_CLK_I3C2, "i3c2_root", low_speed_sels, 0x1900, },
	{ IMX93_CLK_USDHC1, "usdhc1_root", fast_speed_sels, 0x1980, },
	{ IMX93_CLK_USDHC2, "usdhc2_root", fast_speed_sels, 0x1a00, },
	{ IMX93_CLK_USDHC3, "usdhc3_root", fast_speed_sels, 0x1a80, },
	{ IMX93_CLK_SAI1, "sai1_root", audio_sels, 0x1b00, },
	{ IMX93_CLK_SAI2, "sai2_root", audio_sels, 0x1b80, },
	{ IMX93_CLK_SAI3, "sai3_root", audio_sels, 0x1c00, },
	{ IMX93_CLK_CCM_CKO1, "ccm_cko1_root", cko1_sels, 0x1c80, },
	{ IMX93_CLK_CCM_CKO2, "ccm_cko2_root", cko2_sels, 0x1d00, },
	{ IMX93_CLK_CCM_CKO3, "ccm_cko3_root", cko1_sels, 0x1d80, },
	{ IMX93_CLK_CCM_CKO4, "ccm_cko4_root", cko2_sels, 0x1e00, },
	{ IMX93_CLK_HSIO, "hsio_root", low_speed_sels, 0x1e80, },
	{ IMX93_CLK_HSIO_USB_TEST_60M, "hsio_usb_test_60m_root", low_speed_sels, 0x1f00, },
	{ IMX93_CLK_HSIO_ACSCAN_80M, "hsio_acscan_80m_root", low_speed_sels, 0x1f80, },
	{ IMX93_CLK_HSIO_ACSCAN_480M, "hsio_acscan_480m_root", misc_sels, 0x2000, },
	{ IMX93_CLK_ML_APB, "ml_apb_root", low_speed_sels, 0x2180, },
	{ IMX93_CLK_ML, "ml_root", fast_speed_sels, 0x2200, },
	{ IMX93_CLK_MEDIA_AXI, "media_axi_root", fast_speed_sels, 0x2280, },
	{ IMX93_CLK_MEDIA_APB, "media_apb_root", low_speed_sels, 0x2300, },
	{ IMX93_CLK_MEDIA_LDB, "media_ldb_root", video_sels, 0x2380, },
	{ IMX93_CLK_MEDIA_DISP_PIX, "media_disp_pix_root", video_sels, 0x2400, },
	{ IMX93_CLK_CAM_PIX, "cam_pix_root", video_sels, 0x2480, },
	{ IMX93_CLK_MIPI_TEST_BYTE, "mipi_test_byte_root", video_sels, 0x2500, },
	{ IMX93_CLK_MIPI_PHY_CFG, "mipi_phy_cfg_root", video_sels, 0x2580, },
	{ IMX93_CLK_ADC, "adc_root", low_speed_sels, 0x2700, },
	{ IMX93_CLK_PDM, "pdm_root", audio_sels, 0x2780, },
	{ IMX93_CLK_TSTMR1, "tstmr1_root", low_speed_sels, 0x2800, },
	{ IMX93_CLK_TSTMR2, "tstmr2_root", low_speed_sels, 0x2880, },
	{ IMX93_CLK_MQS1, "mqs1_root", audio_sels, 0x2900, },
	{ IMX93_CLK_MQS2, "mqs2_root", audio_sels, 0x2980, },
	{ IMX93_CLK_AUDIO_XCVR, "audio_xcvr_root", non_io_sels, 0x2a00, },
	{ IMX93_CLK_SPDIF, "spdif_root", audio_sels, 0x2a80, },
	{ IMX93_CLK_ENET, "enet_root", non_io_sels, 0x2b00, },
	{ IMX93_CLK_ENET_TIMER1, "enet_timer1_root", low_speed_sels, 0x2b80, },
	{ IMX93_CLK_ENET_TIMER2, "enet_timer2_root", low_speed_sels, 0x2c00, },
	{ IMX93_CLK_ENET_REF, "enet_ref_root", non_io_sels, 0x2c80, },
	{ IMX93_CLK_ENET_REF_PHY, "enet_ref_phy_root", low_speed_sels, 0x2d00, },
	{ IMX93_CLK_I3C1_SLOW, "i3c1_slow_root", low_speed_sels, 0x2d80, },
	{ IMX93_CLK_I3C2_SLOW, "i3c2_slow_root", low_speed_sels, 0x2e00, },
	{ IMX93_CLK_USB_PHY_BURUNIN, "usb_phy_root", low_speed_sels, 0x2e80, },
	{ IMX93_CLK_PAL_CAME_SCAN, "pal_came_scan_root", misc_sels, 0x2f00, }
};

struct imx93_clk_ccgr {
	u32 clk_id;
	char *name;
	char *parent_names;
	u32 off;
	unsigned long flags;
};

static struct imx93_clk_ccgr clk_ccgrs[] = {
	{ IMX93_CLK_A55_GATE, "a55", "a55_root", 0x8000, CLK_IS_CRITICAL },
	{ IMX93_CLK_CM33_GATE, "cm33", "m33_root", 0x8040, CLK_IS_CRITICAL },
	{ IMX93_CLK_ADC1_GATE, "adc1", "clock-osc-24m", 0x82c0, },
	{ IMX93_CLK_WDOG1_GATE, "wdog1", "clock-osc-24m", 0x8300, },
	{ IMX93_CLK_WDOG2_GATE, "wdog2", "clock-osc-24m", 0x8340, },
	{ IMX93_CLK_WDOG3_GATE, "wdog3", "clock-osc-24m", 0x8380, },
	{ IMX93_CLK_WDOG4_GATE, "wdog4", "clock-osc-24m", 0x83c0, },
	{ IMX93_CLK_WDOG5_GATE, "wdog5", "clock-osc-24m", 0x8400, },
	{ IMX93_CLK_SEMA1_GATE, "sema1", "bus_aon_root", 0x8440, },
	{ IMX93_CLK_SEMA2_GATE, "sema2", "bus_wakeup_root", 0x8480, },
	{ IMX93_CLK_MU_A_GATE, "mu_a", "bus_aon_root", 0x84c0, },
	{ IMX93_CLK_MU_B_GATE, "mu_b", "bus_aon_root", 0x8500, },
	{ IMX93_CLK_EDMA1_GATE, "edma1", "wakeup_axi_root", 0x8540, },
	{ IMX93_CLK_EDMA2_GATE, "edma2", "wakeup_axi_root", 0x8580, },
	{ IMX93_CLK_FLEXSPI1_GATE, "flexspi1", "flexspi1_root", 0x8640, },
	{ IMX93_CLK_GPIO1_GATE, "gpio1", "m33_root", 0x8880, },
	{ IMX93_CLK_GPIO2_GATE, "gpio2", "bus_wakeup_root", 0x88c0, },
	{ IMX93_CLK_GPIO3_GATE, "gpio3", "bus_wakeup_root", 0x8900, },
	{ IMX93_CLK_GPIO4_GATE, "gpio4", "bus_wakeup_root", 0x8940, },
	{ IMX93_CLK_FLEXIO1_GATE, "flexio1", "flexio1_root", 0x8980, },
	{ IMX93_CLK_FLEXIO2_GATE, "flexio2", "flexio2_root", 0x89c0, },
	{ IMX93_CLK_LPIT1_GATE, "lpit1", "bus_aon_root", 0x8a00, },
	{ IMX93_CLK_LPIT2_GATE, "lpit2", "bus_wakeup_root", 0x8a40, },
	{ IMX93_CLK_LPTMR1_GATE, "lptmr1", "lptmr1_root", 0x8a80, },
	{ IMX93_CLK_LPTMR2_GATE, "lptmr2", "lptmr2_root", 0x8ac0, },
	{ IMX93_CLK_TPM1_GATE, "tpm1", "bus_aon_root", 0x8b00, },
	{ IMX93_CLK_TPM2_GATE, "tpm2", "tpm2_root", 0x8b40, },
	{ IMX93_CLK_TPM3_GATE, "tpm3", "bus_wakeup_root", 0x8b80, },
	{ IMX93_CLK_TPM4_GATE, "tpm4", "tpm4_root", 0x8bc0, },
	{ IMX93_CLK_TPM5_GATE, "tpm5", "tpm5_root", 0x8c00, },
	{ IMX93_CLK_TPM6_GATE, "tpm6", "tpm6_root", 0x8c40, },
	{ IMX93_CLK_CAN1_GATE, "can1", "can1_root", 0x8c80, },
	{ IMX93_CLK_CAN2_GATE, "can2", "can2_root", 0x8cc0, },
	{ IMX93_CLK_LPUART1_GATE, "lpuart1", "lpuart1_root", 0x8d00, },
	{ IMX93_CLK_LPUART2_GATE, "lpuart2", "lpuart2_root", 0x8d40, },
	{ IMX93_CLK_LPUART3_GATE, "lpuart3", "lpuart3_root", 0x8d80, },
	{ IMX93_CLK_LPUART4_GATE, "lpuart4", "lpuart4_root", 0x8dc0, },
	{ IMX93_CLK_LPUART5_GATE, "lpuart5", "lpuart5_root", 0x8e00, },
	{ IMX93_CLK_LPUART6_GATE, "lpuart6", "lpuart6_root", 0x8e40, },
	{ IMX93_CLK_LPUART7_GATE, "lpuart7", "lpuart7_root", 0x8e80, },
	{ IMX93_CLK_LPUART8_GATE, "lpuart8", "lpuart8_root", 0x8ec0, },
	{ IMX93_CLK_LPI2C1_GATE, "lpi2c1", "lpi2c1_root", 0x8f00, },
	{ IMX93_CLK_LPI2C2_GATE, "lpi2c2", "lpi2c2_root", 0x8f40, },
	{ IMX93_CLK_LPI2C3_GATE, "lpi2c3", "lpi2c3_root", 0x8f80, },
	{ IMX93_CLK_LPI2C4_GATE, "lpi2c4", "lpi2c4_root", 0x8fc0, },
	{ IMX93_CLK_LPI2C5_GATE, "lpi2c5", "lpi2c5_root", 0x9000, },
	{ IMX93_CLK_LPI2C6_GATE, "lpi2c6", "lpi2c6_root", 0x9040, },
	{ IMX93_CLK_LPI2C7_GATE, "lpi2c7", "lpi2c7_root", 0x9080, },
	{ IMX93_CLK_LPI2C8_GATE, "lpi2c8", "lpi2c8_root", 0x90c0, },
	{ IMX93_CLK_LPSPI1_GATE, "lpspi1", "lpspi1_root", 0x9100, },
	{ IMX93_CLK_LPSPI2_GATE, "lpspi2", "lpspi2_root", 0x9140, },
	{ IMX93_CLK_LPSPI3_GATE, "lpspi3", "lpspi3_root", 0x9180, },
	{ IMX93_CLK_LPSPI4_GATE, "lpspi4", "lpspi4_root", 0x91c0, },
	{ IMX93_CLK_LPSPI5_GATE, "lpspi5", "lpspi5_root", 0x9200, },
	{ IMX93_CLK_LPSPI6_GATE, "lpspi6", "lpspi6_root", 0x9240, },
	{ IMX93_CLK_LPSPI7_GATE, "lpspi7", "lpspi7_root", 0x9280, },
	{ IMX93_CLK_LPSPI8_GATE, "lpspi8", "lpspi8_root", 0x92c0, },
	{ IMX93_CLK_I3C1_GATE, "i3c1", "i3c1_root", 0x9300, },
	{ IMX93_CLK_I3C2_GATE, "i3c2", "i3c2_root", 0x9340, },
	{ IMX93_CLK_USDHC1_GATE, "usdhc1", "usdhc1_root", 0x9380, },
	{ IMX93_CLK_USDHC2_GATE, "usdhc2", "usdhc2_root", 0x93c0, },
	{ IMX93_CLK_USDHC3_GATE, "usdhc3", "usdhc3_root", 0x9400, },
	{ IMX93_CLK_SAI1_GATE, "sai1", "sai1_root", 0x9440, },
	{ IMX93_CLK_SAI2_GATE, "sai2", "sai2_root", 0x9480, },
	{ IMX93_CLK_SAI3_GATE, "sai3", "sai3_root", 0x94c0, },
	{ IMX93_CLK_MIPI_CSI_GATE, "mipi_csi", "media_apb_root", 0x9580, },
	{ IMX93_CLK_MIPI_DSI_GATE, "mipi_dsi", "media_apb_root", 0x95c0, },
	{ IMX93_CLK_LVDS_GATE, "lvds", "media_ldb_root", 0x9600, },
	{ IMX93_CLK_LCDIF_GATE, "lcdif", "media_apb_root", 0x9640, },
	{ IMX93_CLK_PXP_GATE, "pxp", "media_apb_root", 0x9680, },
	{ IMX93_CLK_ISI_GATE, "isi", "media_apb_root", 0x96c0, },
	{ IMX93_CLK_NIC_MEDIA_GATE, "nic_media", "media_apb_root", 0x9700, },
	{ IMX93_CLK_USB_CONTROLLER_GATE, "usb_controller", "hsio_root", 0x9a00, },
	{ IMX93_CLK_USB_TEST_60M_GATE, "usb_test_60m", "hsio_usb_test_60m_root", 0x9a40, },
	{ IMX93_CLK_HSIO_TROUT_24M_GATE, "hsio_trout_24m", "clock-osc-24m", 0x9a80, },
	{ IMX93_CLK_PDM_GATE, "pdm", "pdm_root", 0x9ac0, },
	{ IMX93_CLK_MQS1_GATE, "mqs1", "sai1_root", 0x9b00, },
	{ IMX93_CLK_MQS2_GATE, "mqs2", "sai3_root", 0x9b40, },
	{ IMX93_CLK_AUD_XCVR_GATE, "aud_xcvr", "audio_xcvr_root", 0x9b80, },
	{ IMX93_CLK_SPDIF_GATE, "spdif", "spdif_root", 0x9c00, },
	{ IMX93_CLK_HSIO_32K_GATE, "hsio_32k", "clock-osc-32k", 0x9dc0, },
	{ IMX93_CLK_ENET1_GATE, "enet1", "enet_root", 0x9e00, },
	{ IMX93_CLK_ENET_QOS_GATE, "enet_qos", "wakeup_axi_root", 0x9e40, },
	{ IMX93_CLK_SYS_CNT_GATE, "sys_cnt", "clock-osc-24m", 0x9e80, },
	{ IMX93_CLK_TSTMR1_GATE, "tstmr1", "bus_aon_root", 0x9ec0, },
	{ IMX93_CLK_TSTMR2_GATE, "tstmr2", "bus_wakeup_root", 0x9f00, },
	{ IMX93_CLK_TMC_GATE, "tmc", "clock-osc-24m", 0x9f40, },
	{ IMX93_CLK_PMRO_GATE, "pmro", "clock-osc-24m", 0x9f80, }
};

static ulong imx93_clk_set_rate(struct clk *clk, ulong rate)
{
	struct clk *c;
	int err = clk_get_by_id(clk->id, &c);

	if (err)
		return err;
	return clk_set_rate(c, rate);
}

static ulong imx93_clk_get_rate(struct clk *clk)
{
	struct clk *c;
	int err = clk_get_by_id(clk->id, &c);

	if (err)
		return err;
	return clk_get_rate(c);
}

static int imx93_clk_enable(struct clk *clk)
{
	struct clk *c;
	int err = clk_get_by_id(clk->id, &c);

	if (err)
		return err;
	return clk_enable(c);
}

static int imx93_clk_disable(struct clk *clk)
{
	struct clk *c;
	int err = clk_get_by_id(clk->id, &c);

	if (err)
		return err;
	return clk_disable(c);
}

static int imx93_clk_set_parent(struct clk *clk, struct clk *parent)
{
	struct clk *c, *p;
	int err = clk_get_by_id(clk->id, &c);

	if (err)
		return err;

	err = clk_get_by_id(parent->id, &p);
	if (err)
		return err;

	return clk_set_parent(c, p);
}

static struct clk_ops imx93_clk_ops = {
	.set_rate = imx93_clk_set_rate,
	.get_rate = imx93_clk_get_rate,
	.enable = imx93_clk_enable,
	.disable = imx93_clk_disable,
	.set_parent = imx93_clk_set_parent,
};

struct clk *imx93_clk_composite(const char *name, const char * const *parent_names,
				int num_parents, void __iomem *reg, unsigned long flags)
{
	struct clk *clk = ERR_PTR(-ENOMEM);
	struct clk_divider *div = NULL;
	struct clk_gate *gate = NULL;
	struct clk_mux *mux = NULL;

	mux = kzalloc(sizeof(*mux), GFP_KERNEL);
	if (!mux)
		goto fail;

	mux->reg = reg;
	mux->shift = 8;
	mux->mask = 3;
	mux->num_parents = num_parents;
	mux->flags = flags;
	mux->parent_names = parent_names;

	div = kzalloc(sizeof(*div), GFP_KERNEL);
	if (!div)
		goto fail;

	div->reg = reg;
	div->shift = 0;
	div->width = 8;
	div->flags = CLK_DIVIDER_ROUND_CLOSEST | flags;

	gate = kzalloc(sizeof(*gate), GFP_KERNEL);
	if (!gate)
		goto fail;

	gate->reg = reg;
	gate->bit_idx = 24;
	gate->flags = CLK_GATE_SET_TO_DISABLE | flags;

	clk = clk_register_composite(NULL, name, parent_names, num_parents,
				     &mux->clk, &clk_mux_ops,
				     &div->clk, &clk_divider_ops,
				     &gate->clk, &clk_gate_ops, flags);
	if (IS_ERR(clk))
		goto fail;

	return clk;

fail:
	kfree(gate);
	kfree(div);
	kfree(mux);
	return ERR_CAST(clk);
}

static int imx93_clk_probe(struct udevice *dev)
{
	void __iomem *ccm_base;
	struct imx93_clk_root *root;
	struct imx93_clk_ccgr *ccgr;
	struct clk *clk;
	struct clk fixed_clock;
	int ret;

	clk_dm(IMX93_CLK_DUMMY, clk_register_fixed_rate(NULL, "dummy", 0UL));

	ret = clk_get_by_name(dev, "osc_24m", &fixed_clock);
	if (ret)
		return ret;
	clk_dm(IMX93_CLK_24M, dev_get_clk_ptr(fixed_clock.dev));

	ret = clk_get_by_name(dev, "clk_ext1", &fixed_clock);
	if (ret)
		return ret;
	clk_dm(IMX93_CLK_EXT1, dev_get_clk_ptr(fixed_clock.dev));

	ret = clk_get_by_name(dev, "osc_32k", &fixed_clock);
	if (ret)
		return ret;
	clk_dm(IMX93_CLK_32K, dev_get_clk_ptr(fixed_clock.dev));

	clk_dm(IMX93_CLK_SYS_PLL_PFD0,
	       clk_register_fixed_rate(NULL, "sys_pll_pfd0", 1000000000UL));
	clk_dm(IMX93_CLK_SYS_PLL_PFD0_DIV2,
	       imx_clk_fixed_factor("sys_pll_pfd0_div2", "sys_pll_pfd0", 1, 2));

	clk_dm(IMX93_CLK_SYS_PLL_PFD1,
	       clk_register_fixed_rate(NULL, "sys_pll_pfd1", 800000000UL));
	clk_dm(IMX93_CLK_SYS_PLL_PFD1_DIV2,
	       imx_clk_fixed_factor("sys_pll_pfd1_div2", "sys_pll_pfd1", 1, 2));

	clk_dm(IMX93_CLK_SYS_PLL_PFD2,
	       clk_register_fixed_rate(NULL, "sys_pll_pfd2", 625000000UL));
	clk_dm(IMX93_CLK_SYS_PLL_PFD2_DIV2,
	       imx_clk_fixed_factor("sys_pll_pfd2_div2", "sys_pll_pfd2", 1, 2));

#ifndef CONFIG_SPL_BUILD
	clk_dm(IMX93_CLK_VIDEO_PLL,
	       clk_register_imx93_pll("video_pll", "clock-osc-24m", (void __iomem *)0x44481400));
#endif

	ccm_base = dev_read_addr_ptr(dev);
	if (ccm_base == (void *)FDT_ADDR_T_NONE) {
		debug("%s: No CCM register base address\n", __func__);
		return -EINVAL;
	}

	for (int i = 0; i < ARRAY_SIZE(clk_roots); i++) {
		root = &clk_roots[i];
		clk = imx93_clk_composite(root->name, root->parent_names, 4,
					  ccm_base + root->off, root->flags);
		clk_dm(root->clk_id, clk);
	}

	for (int i = 0; i < ARRAY_SIZE(clk_ccgrs); i++) {
		ccgr = &clk_ccgrs[i];
		clk = clk_register_imx93_clk_gate(ccgr->name, ccgr->parent_names,
						  ccm_base + ccgr->off, 0, ccgr->flags);
		clk_dm(ccgr->clk_id, clk);
	}

	return 0;
}

static const struct udevice_id imx93_clk_ids[] = {
	{ .compatible = "fsl,imx93-ccm" },
	{ },
};

U_BOOT_DRIVER(imx93_clk) = {
	.name = "imx93_clk",
	.id = UCLASS_CLK,
	.of_match = imx93_clk_ids,
	.ops = &imx93_clk_ops,
	.probe = imx93_clk_probe,
	.flags = DM_FLAG_PRE_RELOC,
};
