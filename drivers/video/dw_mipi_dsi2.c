// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2025-2026 NXP
 */

#include <asm/io.h>
#include <clk.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <div64.h>
#include <dsi_host.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <mipi_dsi.h>

#define DSI2_PWR_UP			0x000c
#define RESET				0
#define POWER_UP			BIT(0)
#define CMD_TX_MODE(x)			FIELD_PREP(BIT(24), x)
#define DSI2_SOFT_RESET			0x0010
#define SYS_RSTN			BIT(2)
#define PHY_RSTN			BIT(1)
#define IPI_RSTN			BIT(0)
#define DSI2_MODE_CTRL			0x0018
#define DSI2_MODE_STATUS		0x001c
#define DSI2_CORE_STATUS		0x0020
#define PRI_RD_DATA_AVAIL		BIT(26)
#define PRI_FIFOS_NOT_EMPTY		BIT(25)
#define PRI_BUSY			BIT(24)
#define CRI_RD_DATA_AVAIL		BIT(18)
#define CRI_FIFOS_NOT_EMPTY		BIT(17)
#define CRI_BUSY			BIT(16)
#define IPI_FIFOS_NOT_EMPTY		BIT(9)
#define IPI_BUSY			BIT(8)
#define CORE_FIFOS_NOT_EMPTY		BIT(1)
#define CORE_BUSY			BIT(0)
#define MANUAL_MODE_CFG			0x0024
#define MANUAL_MODE_EN			BIT(0)

#define DSI2_PHY_MODE_CFG		0x0100
#define PPI_WIDTH(x)			FIELD_PREP(GENMASK(9, 8), x)
#define PHY_LANES(x)			FIELD_PREP(GENMASK(5, 4), (x) - 1)
#define PHY_TYPE(x)			FIELD_PREP(BIT(0), x)
#define DSI2_PHY_CLK_CFG		0X0104
#define PHY_LPTX_CLK_DIV(x)		FIELD_PREP(GENMASK(12, 8), x)
#define CLK_TYPE_MASK			BIT(0)
#define NON_CONTINUOUS_CLK		BIT(0)
#define CONTINUOUS_CLK			0
#define DSI2_PHY_LP2HS_MAN_CFG		0x010c
#define PHY_LP2HS_TIME(x)		FIELD_PREP(GENMASK(28, 0), x)
#define DSI2_PHY_HS2LP_MAN_CFG		0x0114
#define PHY_HS2LP_TIME(x)		FIELD_PREP(GENMASK(28, 0), x)

#define DSI2_PHY_IPI_RATIO_MAN_CFG	0x0134
#define PHY_IPI_RATIO(x)		FIELD_PREP(GENMASK(21, 0), x)
#define DSI2_PHY_SYS_RATIO_MAN_CFG	0x013C
#define PHY_SYS_RATIO(x)		FIELD_PREP(GENMASK(16, 0), x)

#define DSI2_DSI_GENERAL_CFG		0x0200
#define BTA_EN				BIT(1)
#define EOTP_TX_EN			BIT(0)
#define DSI2_DSI_VCID_CFG		0x0204
#define TX_VCID(x)			FIELD_PREP(GENMASK(1, 0), x)
#define DSI2_DSI_VID_TX_CFG		0x020c
#define LPDT_DISPLAY_CMD_EN		BIT(20)
#define BLK_VFP_HS_EN			BIT(14)
#define BLK_VBP_HS_EN			BIT(13)
#define BLK_VSA_HS_EN			BIT(12)
#define BLK_HFP_HS_EN			BIT(6)
#define BLK_HBP_HS_EN			BIT(5)
#define BLK_HSA_HS_EN			BIT(4)
#define VID_MODE_TYPE(x)		FIELD_PREP(GENMASK(1, 0), x)
#define DSI_TEAR_EFFECT_CFG		0x0214
#define AUTO_TEAR_BTA_DISABLE		BIT(0)
#define DSI2_CRI_TX_HDR			0x02c0
#define CMD_TX_MODE(x)			FIELD_PREP(BIT(24), x)
#define DSI2_CRI_TX_PLD			0x02c4
#define DSI2_CRI_RX_HDR			0x02c8
#define DSI2_CRI_RX_PLD			0x02cc
#define DSI2_CRI_FIFO_DEPTH_CFG		0x02e0
#define CMD_WR_PLD_FIFO_DEPTH_VALUE(x)	FIELD_PREP(GENMASK(15, 0), x)
#define CMD_RD_PLD_FIFO_DEPTH_VALUE(x)	FIELD_PREP(GENMASK(31, 16), x)

#define DSI2_IPI_COLOR_MAN_CFG		0x0300
#define IPI_DEPTH(x)			FIELD_PREP(GENMASK(7, 4), x)
#define IPI_DEPTH_5_6_5_BITS		0x02
#define IPI_DEPTH_6_BITS		0x03
#define IPI_DEPTH_8_BITS		0x05
#define IPI_DEPTH_10_BITS		0x06
#define IPI_FORMAT(x)			FIELD_PREP(GENMASK(3, 0), x)
#define IPI_FORMAT_RGB			0x0
#define IPI_FORMAT_DSC			0x0b
#define DSI2_IPI_VID_HSA_MAN_CFG	0x0304
#define VID_HSA_TIME(x)			FIELD_PREP(GENMASK(29, 0), x)
#define DSI2_IPI_VID_HBP_MAN_CFG	0x030c
#define VID_HBP_TIME(x)			FIELD_PREP(GENMASK(29, 0), x)
#define DSI2_IPI_VID_HACT_MAN_CFG	0x0314
#define VID_HACT_TIME(x)		FIELD_PREP(GENMASK(29, 0), x)
#define DSI2_IPI_VID_HLINE_MAN_CFG	0x031c
#define VID_HLINE_TIME(x)		FIELD_PREP(GENMASK(29, 0), x)
#define DSI2_IPI_VID_VSA_MAN_CFG	0x0324
#define VID_VSA_LINES(x)		FIELD_PREP(GENMASK(9, 0), x)
#define DSI2_IPI_VID_VBP_MAN_CFG	0X032C
#define VID_VBP_LINES(x)		FIELD_PREP(GENMASK(9, 0), x)
#define DSI2_IPI_VID_VACT_MAN_CFG	0X0334
#define VID_VACT_LINES(x)		FIELD_PREP(GENMASK(13, 0), x)
#define DSI2_IPI_VID_VFP_MAN_CFG	0X033C
#define VID_VFP_LINES(x)		FIELD_PREP(GENMASK(9, 0), x)
#define DSI2_IPI_PIX_PKT_CFG		0x0344
#define MAX_PIX_PKT(x)			FIELD_PREP(GENMASK(15, 0), x)
#define LANES_MAN_CFG			0x034c
#define IPI_LANES(x)			FIELD_PREP(GENMASK(1, 0), x)
#define FIFO_DEPTH_CFG			0x03c0
#define IPI_FIFO_DEPTH_VALUE(x)		FIELD_PREP(GENMASK(15, 0), x)
#define IPI_MAPPING_CFG			0x03c4
#define IPI_MAPPING(x)			FIELD_PREP(GENMASK(1, 0), x)

#define MODE_STATUS_TIMEOUT_US		10000
#define CMD_PKT_STATUS_TIMEOUT_US	20000

#define usleep_range(a, b) udelay((b))

/*
 * Configures the video mode transmission type.
 */
enum vid_mode_type {
	VID_MODE_TYPE_NON_BURST_SYNC_PULSES,
	VID_MODE_TYPE_NON_BURST_SYNC_EVENTS,
	VID_MODE_TYPE_BURST,
};

/* The DSI2_V2_HOST controller has the following operating modes:
 * Idle Mode
 * Auto-Calculation Mode
 * Video Mode
 * Command Mode
 */
enum mode_ctrl {
	IDLE_MODE,
	AUTOCALC_MODE,
	COMMAND_MODE,
	VIDEO_MODE,
	DATA_STREAM_MODE,
	VIDEO_TEST_MODE,
	DATA_STREAM_TEST_MODE,
};

enum ppi_width {
	PPI_WIDTH_8_BITS,
	PPI_WIDTH_16_BITS,
	PPI_WIDTH_32_BITS,
};

enum ipi_lanes {
	IPI_LANES_N_4,
	IPI_LANES_N_1,
	IPI_LANES_N_2,
};

struct cmd_header {
	u8 cmd_type;
	u8 delay;
	u8 payload_length;
};

struct dw_mipi_dsi2 {
	struct mipi_dsi_host dsi_host;
	struct mipi_dsi_device *device;
	void __iomem *base;
	struct clk *pclk;
	struct clk *sys_clk;

	unsigned int lane_mbps; /* per lane */
	u32 channel;
	unsigned int max_data_lanes;
	u32 lanes;
	u32 format;
	unsigned long mode_flags;
	int ppi_width;

	const struct mipi_dsi_phy_ops *phy_ops;
	const struct dw_mipi_dsi2_plat_data *plat_data;
};

static inline void dsi2_write(struct dw_mipi_dsi2 *dsi2, u32 reg, u32 val)
{
	writel(val, dsi2->base + reg);
}

static inline u32 dsi2_read(struct dw_mipi_dsi2 *dsi2, u32 reg)
{
	return readl(dsi2->base + reg);
}

static inline struct dw_mipi_dsi2 *host_to_dsi2(struct mipi_dsi_host *host)
{
	return container_of(host, struct dw_mipi_dsi2, dsi_host);
}

static int cri_fifos_wait_avail(struct dw_mipi_dsi2 *dsi2)
{
	u32 sts, mask;
	int ret;

	mask = CRI_BUSY | CRI_FIFOS_NOT_EMPTY;
	ret = readl_poll_timeout(dsi2->base + DSI2_CORE_STATUS, sts,
				 !(sts & mask), CMD_PKT_STATUS_TIMEOUT_US);
	if (ret < 0) {
		dev_err(dsi2->dsi_host.dev, "command interface is busy\n");
		return ret;
	}

	return 0;
}

static void dw_mipi_dsi2_set_vid_mode(struct dw_mipi_dsi2 *dsi2)
{
	u32 val = 0, mode;
	int ret;

	if (dsi2->mode_flags & MIPI_DSI_MODE_VIDEO_HFP)
		val |= BLK_HFP_HS_EN;

	if (dsi2->mode_flags & MIPI_DSI_MODE_VIDEO_HBP)
		val |= BLK_HBP_HS_EN;

	if (dsi2->mode_flags & MIPI_DSI_MODE_VIDEO_HSA)
		val |= BLK_HSA_HS_EN;

	if (dsi2->mode_flags & MIPI_DSI_MODE_VIDEO_BURST)
		val |= VID_MODE_TYPE_BURST;
	else if (dsi2->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE)
		val |= VID_MODE_TYPE_NON_BURST_SYNC_PULSES;
	else
		val |= VID_MODE_TYPE_NON_BURST_SYNC_EVENTS;

	dsi2_write(dsi2, DSI2_DSI_VID_TX_CFG, val);

	dsi2_write(dsi2, DSI2_MODE_CTRL, VIDEO_MODE);
	ret = readl_poll_timeout(dsi2->base + DSI2_MODE_STATUS,
				 mode, mode & VIDEO_MODE, MODE_STATUS_TIMEOUT_US);
	if (ret < 0)
		dev_err(dsi2->dsi_host.dev, "failed to enter video mode\n");
}

static void dw_mipi_dsi2_set_data_stream_mode(struct dw_mipi_dsi2 *dsi2)
{
	u32 mode;
	int ret;

	dsi2_write(dsi2, DSI2_MODE_CTRL, DATA_STREAM_MODE);
	ret = readl_poll_timeout(dsi2->base + DSI2_MODE_STATUS,
				 mode, mode & DATA_STREAM_MODE, MODE_STATUS_TIMEOUT_US);
	if (ret < 0)
		dev_err(dsi2->dsi_host.dev, "failed to enter data stream mode\n");
}

static void dw_mipi_dsi2_set_cmd_mode(struct dw_mipi_dsi2 *dsi2)
{
	u32 mode;
	int ret;

	dsi2_write(dsi2, DSI2_MODE_CTRL, COMMAND_MODE);
	ret = readl_poll_timeout(dsi2->base + DSI2_MODE_STATUS,
				 mode, mode & COMMAND_MODE, MODE_STATUS_TIMEOUT_US);
	if (ret < 0)
		dev_err(dsi2->dsi_host.dev, "failed to enter data stream mode\n");
}

static void dw_mipi_dsi2_host_softrst(struct dw_mipi_dsi2 *dsi2)
{
	dsi2_write(dsi2, DSI2_SOFT_RESET, 0x0);
	usleep_range(50, 100);
	dsi2_write(dsi2, DSI2_SOFT_RESET, SYS_RSTN | PHY_RSTN | IPI_RSTN);
}

static void dw_mipi_dsi2_phy_clk_mode_cfg(struct dw_mipi_dsi2 *dsi2)
{
	const struct mipi_dsi_phy_ops *phy_ops = dsi2->phy_ops;
	unsigned int esc_rate = 20000000; /* Default to 20MHz */
	u32 sys_clk, esc_clk_div;
	u32 val = 0;

	/*
	 * clk_type should be NON_CONTINUOUS_CLK before
	 * initial deskew calibration be sent.
	 */
	val |= NON_CONTINUOUS_CLK;

	/* The maximum value of the escape clock frequency is 20MHz */
	if (phy_ops->get_esc_clk_rate)
		phy_ops->get_esc_clk_rate(dsi2->device, &esc_rate);

	sys_clk = clk_get_rate(dsi2->sys_clk);
	esc_clk_div = DIV_ROUND_UP(sys_clk, esc_rate * 2);
	val |= PHY_LPTX_CLK_DIV(esc_clk_div);

	dsi2_write(dsi2, DSI2_PHY_CLK_CFG, val);
}

static void dw_mipi_dsi2_phy_ratio_cfg(struct dw_mipi_dsi2 *dsi2,
				       struct display_timing *timings)
{
	u64 sys_clk = clk_get_rate(dsi2->sys_clk);
	u64 pixel_clk, ipi_clk, phy_hsclk;
	u64 tmp;

	/*
	 * in DPHY mode, the phy_hstx_clk is exactly 1/ppi_width the Lane
	 * high-speed data rate; In CPHY mode, the phy_hstx_clk is exactly 1/7
	 * the trio high speed symbol rate.
	 */
	phy_hsclk = DIV_ROUND_CLOSEST_ULL(dsi2->lane_mbps * USEC_PER_SEC, dsi2->ppi_width);

	/* IPI_RATIO_MAN_CFG = PHY_HSTX_CLK / IPI_CLK */
	pixel_clk = timings->pixelclock.typ;
	ipi_clk = pixel_clk / 4;

	tmp = DIV_ROUND_CLOSEST_ULL(phy_hsclk << 16, ipi_clk);
	dsi2_write(dsi2, DSI2_PHY_IPI_RATIO_MAN_CFG, PHY_IPI_RATIO(tmp));

	/*
	 * SYS_RATIO_MAN_CFG = MIPI_DCPHY_HSCLK_Freq / MIPI_DCPHY_HSCLK_Freq
	 */
	tmp = DIV_ROUND_CLOSEST_ULL(phy_hsclk << 16, sys_clk);
	dsi2_write(dsi2, DSI2_PHY_SYS_RATIO_MAN_CFG, PHY_SYS_RATIO(tmp));
}

static void dw_mipi_dsi2_lp2hs_or_hs2lp_cfg(struct dw_mipi_dsi2 *dsi2)
{
	const struct mipi_dsi_phy_ops *phy_ops = dsi2->phy_ops;
	struct mipi_dsi_phy_timing timing;
	int ret;

	ret = phy_ops->get_timing(dsi2->device, dsi2->lane_mbps, &timing);
	if (ret)
		dev_err(dsi2->dsi_host.dev, "Retrieving phy timings failed\n");

	dsi2_write(dsi2, DSI2_PHY_LP2HS_MAN_CFG, PHY_LP2HS_TIME(timing.data_lp2hs));
	dsi2_write(dsi2, DSI2_PHY_HS2LP_MAN_CFG, PHY_HS2LP_TIME(timing.data_hs2lp));
}

static void dw_mipi_dsi2_phy_init(struct dw_mipi_dsi2 *dsi2,
				  struct display_timing *timings)
{
	const struct mipi_dsi_phy_ops *phy_ops = dsi2->phy_ops;
	struct dw_mipi_dsi2_phy_iface iface;
	u32 val = 0;
	int ret;

	ret = phy_ops->init(dsi2->device);
	if (ret)
		dev_err(dsi2->dsi_host.dev, "Phy init() failed\n");

	phy_ops->get_interface(dsi2->device, &iface);
	dsi2->ppi_width = iface.ppi_width;

	switch (iface.ppi_width) {
	case 8:
		val |= PPI_WIDTH(PPI_WIDTH_8_BITS);
		break;
	case 16:
		val |= PPI_WIDTH(PPI_WIDTH_16_BITS);
		break;
	case 32:
		val |= PPI_WIDTH(PPI_WIDTH_32_BITS);
		break;
	default:
		/* Caught in probe */
		break;
	}

	val |= PHY_LANES(dsi2->device->lanes);
	val |= PHY_TYPE(DW_MIPI_DSI2_DPHY);
	dsi2_write(dsi2, DSI2_PHY_MODE_CFG, val);

	dw_mipi_dsi2_phy_clk_mode_cfg(dsi2);
	dw_mipi_dsi2_phy_ratio_cfg(dsi2, timings);
	dw_mipi_dsi2_lp2hs_or_hs2lp_cfg(dsi2);

	/* phy configuration 8 - 10 */
}

static void dw_mipi_dsi2_tx_option_set(struct dw_mipi_dsi2 *dsi2)
{
	struct mipi_dsi_device *device = dsi2->device;
	u32 val;

	val = BTA_EN | EOTP_TX_EN;

	if (device->mode_flags & MIPI_DSI_MODE_EOT_PACKET)
		val &= ~EOTP_TX_EN;

	dsi2_write(dsi2, DSI2_DSI_GENERAL_CFG, val);
	dsi2_write(dsi2, DSI2_DSI_VCID_CFG, TX_VCID(device->channel));
}

static void dw_mipi_dsi2_cri_set(struct dw_mipi_dsi2 *dsi2)
{
	const struct dw_mipi_dsi2_plat_data *pdata = dsi2->plat_data;
	u32 val;

	if (pdata->cri_cmd_wr_pld_fifo_depth == 0 &&
	    pdata->cri_cmd_rd_pld_fifo_depth == 0)
		return;

	val = CMD_WR_PLD_FIFO_DEPTH_VALUE(pdata->cri_cmd_wr_pld_fifo_depth) |
	      CMD_RD_PLD_FIFO_DEPTH_VALUE(pdata->cri_cmd_rd_pld_fifo_depth);

	dsi2_write(dsi2, DSI2_CRI_FIFO_DEPTH_CFG, val);
}

static void dw_mipi_dsi2_ipi_color_coding_cfg(struct dw_mipi_dsi2 *dsi2)
{
	u32 val, color_depth;

	switch (dsi2->device->format) {
	case MIPI_DSI_FMT_RGB666:
	case MIPI_DSI_FMT_RGB666_PACKED:
		color_depth = IPI_DEPTH_6_BITS;
		break;
	case MIPI_DSI_FMT_RGB565:
		color_depth = IPI_DEPTH_5_6_5_BITS;
		break;
	case MIPI_DSI_FMT_RGB888:
	default:
		color_depth = IPI_DEPTH_8_BITS;
		break;
	}

	val = IPI_DEPTH(color_depth) |
	      IPI_FORMAT(IPI_FORMAT_RGB);
	dsi2_write(dsi2, DSI2_IPI_COLOR_MAN_CFG, val);
}

static void dw_mipi_dsi2_vertical_timing_config(struct dw_mipi_dsi2 *dsi2,
						struct display_timing *timings)
{
	u32 vactive, vsa, vfp, vbp;

	vactive = timings->vactive.typ;
	vsa = timings->vsync_len.typ;
	vfp = timings->vfront_porch.typ;
	vbp = timings->vback_porch.typ;

	dsi2_write(dsi2, DSI2_IPI_VID_VSA_MAN_CFG, VID_VSA_LINES(vsa));
	dsi2_write(dsi2, DSI2_IPI_VID_VBP_MAN_CFG, VID_VBP_LINES(vbp));
	dsi2_write(dsi2, DSI2_IPI_VID_VACT_MAN_CFG, VID_VACT_LINES(vactive));
	dsi2_write(dsi2, DSI2_IPI_VID_VFP_MAN_CFG, VID_VFP_LINES(vfp));
}

static void dw_mipi_dsi2_ipi_set(struct dw_mipi_dsi2 *dsi2,
				 struct display_timing *timings)
{
	const struct dw_mipi_dsi2_plat_data *pdata = dsi2->plat_data;
	u32 hline, hsa, hbp, hact;
	u64 hline_time, hsa_time, hbp_time, hact_time, tmp;
	u64 pixel_clk, phy_hs_clk;
	u16 val;

	val = timings->hactive.typ;

	dsi2_write(dsi2, DSI2_IPI_PIX_PKT_CFG, MAX_PIX_PKT(val));

	dw_mipi_dsi2_ipi_color_coding_cfg(dsi2);

	/*
	 * if the controller is intended to operate in data stream mode,
	 * no more steps are required.
	 */
	if (!(dsi2->device->mode_flags & MIPI_DSI_MODE_VIDEO))
		return;

	hact = timings->hactive.typ;
	hsa = timings->hsync_len.typ;
	hbp = timings->hback_porch.typ;
	hline = timings->hactive.typ + timings->hfront_porch.typ +
		timings->hback_porch.typ + timings->hsync_len.typ;

	pixel_clk = timings->pixelclock.typ;

	phy_hs_clk = DIV_ROUND_CLOSEST_ULL(dsi2->lane_mbps * USEC_PER_SEC, dsi2->ppi_width);

	tmp = hsa * phy_hs_clk;
	hsa_time = DIV_ROUND_CLOSEST_ULL(tmp << 16, pixel_clk);
	dsi2_write(dsi2, DSI2_IPI_VID_HSA_MAN_CFG, VID_HSA_TIME(hsa_time));

	tmp = hbp * phy_hs_clk;
	hbp_time = DIV_ROUND_CLOSEST_ULL(tmp << 16, pixel_clk);
	dsi2_write(dsi2, DSI2_IPI_VID_HBP_MAN_CFG, VID_HBP_TIME(hbp_time));

	tmp = hact * phy_hs_clk;
	hact_time = DIV_ROUND_CLOSEST_ULL(tmp << 16, pixel_clk);
	dsi2_write(dsi2, DSI2_IPI_VID_HACT_MAN_CFG, VID_HACT_TIME(hact_time));

	tmp = hline * phy_hs_clk;
	hline_time = DIV_ROUND_CLOSEST_ULL(tmp << 16, pixel_clk);
	dsi2_write(dsi2, DSI2_IPI_VID_HLINE_MAN_CFG, VID_HLINE_TIME(hline_time));

	dw_mipi_dsi2_vertical_timing_config(dsi2, timings);

	switch (pdata->ipi_lanes) {
	case 1:
		dsi2_write(dsi2, LANES_MAN_CFG, IPI_LANES(IPI_LANES_N_1));
		break;
	case 2:
		dsi2_write(dsi2, LANES_MAN_CFG, IPI_LANES(IPI_LANES_N_2));
		break;
	case 4:
		dsi2_write(dsi2, LANES_MAN_CFG, IPI_LANES(IPI_LANES_N_4));
		break;
	}

	if (pdata->ipi_fifo_depth)
		dsi2_write(dsi2, FIFO_DEPTH_CFG, IPI_FIFO_DEPTH_VALUE(960));

	if (pdata->ipi_mapping != DW_MIPI_DSI2_IPI_MAPPING_NA)
		dsi2_write(dsi2, IPI_MAPPING_CFG, IPI_MAPPING(1));
}

static void
dw_mipi_dsi2_work_mode(struct dw_mipi_dsi2 *dsi2, u32 mode)
{
	/*
	 * select controller work in Manual mode
	 * Manual: MANUAL_MODE_EN
	 * Automatic: 0
	 */
	dsi2_write(dsi2, MANUAL_MODE_CFG, mode);
}

static int dw_mipi_dsi2_host_attach(struct mipi_dsi_host *host,
				    struct mipi_dsi_device *device)
{
	struct dw_mipi_dsi2 *dsi2 = host_to_dsi2(host);
	const struct dw_mipi_dsi2_plat_data *pdata = dsi2->plat_data;
	int ret;

	if (device->lanes > dsi2->plat_data->max_data_lanes) {
		dev_err(dsi2->dsi_host.dev, "the number of data lanes(%u) is too many\n",
			device->lanes);
		return -EINVAL;
	}

	dsi2->lanes = device->lanes;
	dsi2->channel = device->channel;
	dsi2->format = device->format;
	dsi2->mode_flags = device->mode_flags;

	if (pdata->host_ops && pdata->host_ops->attach) {
		ret = pdata->host_ops->attach(pdata->priv_data, device);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int dw_mipi_dsi2_host_detach(struct mipi_dsi_host *host,
				    struct mipi_dsi_device *device)
{
	return 0;
}

static int dw_mipi_dsi2_gen_pkt_hdr_write(struct dw_mipi_dsi2 *dsi2,
					  u32 hdr_val, bool lpm)
{
	int ret;

	dsi2_write(dsi2, DSI2_CRI_TX_HDR, hdr_val | CMD_TX_MODE(lpm));

	ret = cri_fifos_wait_avail(dsi2);
	if (ret) {
		dev_err(dsi2->dsi_host.dev, "failed to write command header\n");
		return ret;
	}

	return 0;
}

static int dw_mipi_dsi2_write(struct dw_mipi_dsi2 *dsi2,
			      const struct mipi_dsi_packet *packet, bool lpm)
{
	const u8 *tx_buf = packet->payload;
	int len = packet->payload_length, pld_data_bytes = sizeof(u32);
	__le32 word;

	/* Send payload */
	while (len) {
		if (len < pld_data_bytes) {
			word = 0;
			memcpy(&word, tx_buf, len);
			dsi2_write(dsi2, DSI2_CRI_TX_PLD, le32_to_cpu(word));
			len = 0;
		} else {
			memcpy(&word, tx_buf, pld_data_bytes);
			dsi2_write(dsi2, DSI2_CRI_TX_PLD, le32_to_cpu(word));
			tx_buf += pld_data_bytes;
			len -= pld_data_bytes;
		}
	}

	word = 0;
	memcpy(&word, packet->header, sizeof(packet->header));
	return dw_mipi_dsi2_gen_pkt_hdr_write(dsi2, le32_to_cpu(word), lpm);
}

static int dw_mipi_dsi2_read(struct dw_mipi_dsi2 *dsi2,
			     const struct mipi_dsi_msg *msg)
{
	u8 *payload = msg->rx_buf;
	int i, j, ret, len = msg->rx_len;
	u8 data_type;
	u16 wc;
	u32 val;

	ret = readl_poll_timeout(dsi2->base + DSI2_CORE_STATUS,
				 val, val & CRI_RD_DATA_AVAIL, CMD_PKT_STATUS_TIMEOUT_US);
	if (ret) {
		dev_err(dsi2->dsi_host.dev, "CRI has no available read data\n");
		return ret;
	}

	val = dsi2_read(dsi2, DSI2_CRI_RX_HDR);
	data_type = val & 0x3f;

	if (mipi_dsi_packet_format_is_short(data_type)) {
		for (i = 0; i < len && i < 2; i++)
			payload[i] = (val >> (8 * (i + 1))) & 0xff;

		return 0;
	}

	wc = (val >> 8) & 0xffff;
	/* Receive payload */
	for (i = 0; i < len && i < wc; i += 4) {
		val = dsi2_read(dsi2, DSI2_CRI_RX_PLD);
		for (j = 0; j < 4 && j + i < len && j + i < wc; j++)
			payload[i + j] = val >> (8 * j);
	}

	return 0;
}

static ssize_t dw_mipi_dsi2_host_transfer(struct mipi_dsi_host *host,
					  const struct mipi_dsi_msg *msg)
{
	struct dw_mipi_dsi2 *dsi2 = host_to_dsi2(host);
	bool lpm = msg->flags & MIPI_DSI_MSG_USE_LPM;
	struct mipi_dsi_packet packet;
	int ret, nb_bytes;

	dsi2_write(dsi2, DSI2_DSI_VID_TX_CFG,
		   LPDT_DISPLAY_CMD_EN & (lpm ? LPDT_DISPLAY_CMD_EN : 0));

	dsi2_write(dsi2, DSI_TEAR_EFFECT_CFG, AUTO_TEAR_BTA_DISABLE);
	/* create a packet to the DSI protocol */
	ret = mipi_dsi_create_packet(&packet, msg);
	if (ret) {
		dev_err(dsi2->dsi_host.dev, "failed to create packet: %d\n", ret);
		return ret;
	}

	ret = cri_fifos_wait_avail(dsi2);
	if (ret)
		return ret;

	ret = dw_mipi_dsi2_write(dsi2, &packet, lpm);
	if (ret)
		return ret;

	if (msg->rx_buf && msg->rx_len) {
		ret = dw_mipi_dsi2_read(dsi2, msg);
		if (ret < 0)
			return ret;
		nb_bytes = msg->rx_len;
	} else {
		nb_bytes = packet.size;
	}

	return nb_bytes;
}

static const struct mipi_dsi_host_ops dw_mipi_dsi2_host_ops = {
	.attach = dw_mipi_dsi2_host_attach,
	.detach = dw_mipi_dsi2_host_detach,
	.transfer = dw_mipi_dsi2_host_transfer,
};

static void dw_mipi_dsi2_bridge_set(struct dw_mipi_dsi2 *dsi2,
				    struct display_timing *timings)
{
	const struct mipi_dsi_phy_ops *phy_ops = dsi2->phy_ops;
	struct mipi_dsi_device *device = dsi2->device;
	int ret;
	u32 tmp;

	clk_prepare_enable(dsi2->pclk);
	clk_prepare_enable(dsi2->sys_clk);

	ret = phy_ops->get_lane_mbps(dsi2->device, timings, device->lanes,
				     device->format, &dsi2->lane_mbps);
	if (ret)
		dev_warn(dsi2->dsi_host.dev, "Phy get_lane_mbps() failed\n");

	dw_mipi_dsi2_host_softrst(dsi2);
	dsi2_write(dsi2, DSI2_PWR_UP, RESET);

	dw_mipi_dsi2_work_mode(dsi2, MANUAL_MODE_EN);
	dw_mipi_dsi2_phy_init(dsi2, timings);

	if (phy_ops->power_on)
		phy_ops->power_on(dsi2->device);

	dw_mipi_dsi2_tx_option_set(dsi2);
	dw_mipi_dsi2_cri_set(dsi2);

	/*
	 * initial deskew calibration is send after phy_power_on,
	 * then we can configure clk_type.
	 */

	tmp = dsi2_read(dsi2, DSI2_PHY_CLK_CFG) & ~CLK_TYPE_MASK;
	tmp |= (device->mode_flags & MIPI_DSI_CLOCK_NON_CONTINUOUS ? NON_CONTINUOUS_CLK :
	       CONTINUOUS_CLK) & CLK_TYPE_MASK;
	dsi2_write(dsi2, DSI2_PHY_CLK_CFG, tmp);

	dsi2_write(dsi2, DSI2_PWR_UP, POWER_UP);
	dw_mipi_dsi2_set_cmd_mode(dsi2);

	dw_mipi_dsi2_ipi_set(dsi2, timings);
}

static int dw_mipi_dsi2_init(struct udevice *dev,
			     struct mipi_dsi_device *device,
			     struct display_timing *timings,
			     unsigned int max_data_lanes,
			     const struct mipi_dsi_phy_ops *phy_ops)
{
	struct dw_mipi_dsi2 *dsi2 = dev_get_priv(dev);

	dsi2->plat_data = dev_get_plat(device->dev);

	if (!phy_ops->init || !phy_ops->get_lane_mbps ||
	    !phy_ops->get_timing) {
		dev_err(device->dev, "Phy not properly configured\n");
		return -ENODEV;
	}

	dsi2->base = dev_read_addr_ptr(device->dev);
	if (!dsi2->base) {
		dev_err(device->dev, "dsi2 dt register address error\n");
		return -EINVAL;
	}

	dsi2->phy_ops = phy_ops;
	dsi2->max_data_lanes = max_data_lanes;
	dsi2->device = device;
	dsi2->dsi_host.dev = (struct device *)dev;
	dsi2->dsi_host.ops = &dw_mipi_dsi2_host_ops;
	device->host = &dsi2->dsi_host;

	dsi2->pclk = devm_clk_get(device->dev, "pclk");
	if (IS_ERR(dsi2->pclk)) {
		dev_err(device->dev, "Unable to get pclk\n");
		return PTR_ERR(dsi2->pclk);
	}

	dsi2->sys_clk = devm_clk_get(device->dev, "sys");
	if (IS_ERR(dsi2->sys_clk)) {
		dev_err(device->dev, "Unable to get sys_clk\n");
		return PTR_ERR(dsi2->sys_clk);
	}
	clk_set_rate(dsi2->sys_clk, timings->pixelclock.typ);

	dw_mipi_dsi2_bridge_set(dsi2, timings);

	return 0;
}

static int dw_mipi_dsi2_enable(struct udevice *dev)
{
	struct dw_mipi_dsi2 *dsi2 = dev_get_priv(dev);

	/* Switch to video mode for panel-bridge enable & panel enable */
	if (dsi2->mode_flags & MIPI_DSI_MODE_VIDEO)
		dw_mipi_dsi2_set_vid_mode(dsi2);
	else
		dw_mipi_dsi2_set_data_stream_mode(dsi2);

	return 0;
}

struct dsi_host_ops dw_mipi_dsi2_ops = {
	.init = dw_mipi_dsi2_init,
	.enable = dw_mipi_dsi2_enable,
};

static int dw_mipi_dsi2_probe(struct udevice *dev)
{
	return 0;
}

#if (IS_ENABLED(CONFIG_VIDEO_IMX_DW_DSI) || IS_ENABLED(CONFIG_VIDEO_IMX952_MIPI_DSI2))
static const struct udevice_id dw_mipi_dsi2_ids[] = {
	{ .compatible = "synopsys,dw-mipi-dsi2" },
	{ }
};
#endif

U_BOOT_DRIVER(dw_mipi_dsi2) = {
	.name			= "dw_mipi_dsi2",
	.id			= UCLASS_DSI_HOST,
#if (IS_ENABLED(CONFIG_VIDEO_IMX_DW_DSI) || IS_ENABLED(CONFIG_VIDEO_IMX952_MIPI_DSI2))
	.of_match		= dw_mipi_dsi2_ids,
#endif
	.probe			= dw_mipi_dsi2_probe,
	.ops			= &dw_mipi_dsi2_ops,
	.priv_auto		= sizeof(struct dw_mipi_dsi2),
};

MODULE_DESCRIPTION("DW MIPI DSI2 host controller driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:dw-mipi-dsi2");
