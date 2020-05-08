/*
 * Copyright 2015-2017 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _IMX8_MIPI_DSI_H_
#define _IMX8_MIPI_DSI_H_

#define MIPI_CSR_OFFSET 0x1000 /* Subsystem Control Status Registers (CSR) */
#define MIPI_CSR_TX_ULPS  0x0
#define MIPIv2_CSR_TX_ULPS  0x30
#define MIPI_CSR_TX_ULPS_VALUE  0x1F

#define MIPI_CSR_PXL2DPI         0x4
#define MIPIv2_CSR_PXL2DPI         0x40

#define MIPI_CSR_PXL2DPI_16_BIT_PACKED       0x0
#define MIPI_CSR_PXL2DPI_16_BIT_565_ALIGNED  0x1
#define MIPI_CSR_PXL2DPI_16_BIT_565_SHIFTED  0x2
#define MIPI_CSR_PXL2DPI_18_BIT_PACKED       0x3
#define MIPI_CSR_PXL2DPI_18_BIT_ALIGNED      0x4
#define MIPI_CSR_PXL2DPI_24_BIT              0x5

#define	DSI_CMD_BUF_MAXSIZE         (128)

#define MIPI_DSI_OFFSET 0x8000 /* MIPI DSI Controller */

/* DPI interface pixel color coding map */
enum mipi_dsi_dpi_fmt {
	MIPI_RGB565_PACKED = 0,
	MIPI_RGB565_LOOSELY,
	MIPI_RGB565_CONFIG3,
	MIPI_RGB666_PACKED,
	MIPI_RGB666_LOOSELY,
	MIPI_RGB888,
};

struct mipi_dsi_context {
	char *NAME;
	uint32_t REGS_BASE;
	uint32_t CSR_REGS_BASE;
};

struct dsi_cfg_csr_object {
	uint32_t dsi_host_cfg_num_lanes;
	uint32_t dsi_host_cfg_noncont_clk;
	uint32_t dsi_host_cfg_t_pre;
	uint32_t dsi_host_cfg_t_post;
	uint32_t dsi_host_cfg_tx_gap;
	uint32_t dsi_host_cfg_autoinsert_eotp;
	uint32_t dsi_host_cfg_extrcmd_after_eotp;
	uint32_t dsi_host_cfg_htx_to_cnt;
	uint32_t dsi_host_cfg_lrx_h_to_cnt;
	uint32_t dsi_host_cfg_bta_h_to_cnt;
	uint32_t dsi_host_cfg_twakeup;
};

struct dsi_cfg_dpi_object {
	uint32_t dsi_host_cfg_dpi_pxl_payld_size;
	uint32_t dsi_host_cfg_dpi_pxl_fifo_send_lev;
	uint32_t dsi_host_cfg_dpi_if_color_coding;
	uint32_t dsi_host_cfg_dpi_pxl_format;
	uint32_t dsi_host_cfg_dpi_vsync_pol;
	uint32_t dsi_host_cfg_dpi_hsync_pol;
	uint32_t dsi_host_cfg_dpi_video_mode;
	uint32_t dsi_host_cfg_dpi_hfp;
	uint32_t dsi_host_cfg_dpi_hbp;
	uint32_t dsi_host_cfg_dpi_hsa;
	uint32_t dsi_host_cfg_dpi_en_mult_pkt;
	uint32_t dsi_host_cfg_dpi_vbp;
	uint32_t dsi_host_cfg_dpi_vfp;
	uint32_t dsi_host_cfg_dpi_bllp_mode;
	uint32_t dsi_host_cfg_dpi_null_pkt_bllp;
	uint32_t dsi_host_cfg_dpi_vactive;
	uint32_t dsi_host_cfg_dpi_vc;
};

struct dsi_cfg_pkt_object {
	uint32_t dsi_host_pkt_ctrl;
	uint32_t dsi_host_send_pkt;
	uint32_t dsi_host_irq_mask;
	uint32_t dsi_host_irq_mask2;
};

struct dsi_cfg_dphy_object {
	uint32_t dphy_pd_tx;
	uint32_t dphy_m_prg_hs_prepare;
	uint32_t dphy_mc_prg_hs_prepare;
	uint32_t dphy_m_prg_hs_zero;
	uint32_t dphy_mc_prg_hs_zero;
	uint32_t dphy_m_prg_hs_trial;
	uint32_t dphy_mc_prg_hs_trial;
	uint32_t dphy_pd_pll;
	uint32_t dphy_tst;
	uint32_t dphy_cn;
	uint32_t dphy_cm;
	uint32_t dphy_co;
	uint32_t dphy_lock;
	uint32_t dphy_lock_byp;
	uint32_t dphy_tx_rcal;
	uint32_t dphy_auto_pd_en;
	uint32_t dphy_rxlprp;
	uint32_t dphy_rxcdrp;
};

/* dphy */
#define DPHY_PD_TX			0x300
#define DPHY_M_PRG_HS_PREPARE		0x304
#define DPHY_MC_PRG_HS_PREPARE		0x308
#define DPHY_M_PRG_HS_ZERO		0x30c
#define DPHY_MC_PRG_HS_ZERO		0x310
#define DPHY_M_PRG_HS_TRAIL		0x314
#define DPHY_MC_PRG_HS_TRAIL		0x318
#define DPHY_PD_PLL			0x31c
#define DPHY_TST			0x320
#define DPHY_CN				0x324
#define DPHY_CM				0x328
#define DPHY_CO				0x32c
#define DPHY_LOCK			0x330
#define DPHY_LOCK_BYP			0x334
#define DPHY_RTERM_SEL			0x338
#define DPHY_AUTO_PD_EN			0x33c
#define DPHY_RXLPRP			0x340
#define DPHY_RXCDRP			0x344

/* host */
#define HOST_CFG_NUM_LANES		0x0
#define HOST_CFG_NONCONTINUOUS_CLK	0x4
#define HOST_CFG_T_PRE			0x8
#define HOST_CFG_T_POST			0xc
#define HOST_CFG_TX_GAP			0x10
#define HOST_CFG_AUTOINSERT_EOTP	0x14
#define HOST_CFG_EXTRA_CMDS_AFTER_EOTP	0x18
#define HOST_CFG_HTX_TO_COUNT		0x1c
#define HOST_CFG_LRX_H_TO_COUNT		0x20
#define HOST_CFG_BTA_H_TO_COUNT		0x24
#define HOST_CFG_TWAKEUP		0x28
#define HOST_CFG_STATUS_OUT		0x2c
#define HOST_RX_ERROR_STATUS		0x30

/* dpi */
#define DPI_PIXEL_PAYLOAD_SIZE		0x200
#define DPI_PIXEL_FIFO_SEND_LEVEL	0x204
#define DPI_INTERFACE_COLOR_CODING	0x208
#define DPI_PIXEL_FORMAT		0x20c
#define DPI_VSYNC_POLARITY		0x210
#define DPI_HSYNC_POLARITY		0x214
#define DPI_VIDEO_MODE			0x218
#define DPI_HFP				0x21c
#define DPI_HBP				0x220
#define DPI_HSA				0x224
#define DPI_ENABLE_MULT_PKTS		0x228
#define DPI_VBP				0x22c
#define DPI_VFP				0x230
#define DPI_BLLP_MODE			0x234
#define DPI_USE_NULL_PKT_BLLP		0x238
#define DPI_VACTIVE			0x23c
#define DPI_VC				0x240

/* apb pkt */
#define HOST_TX_PAYLOAD			0x280

#define HOST_PKT_CONTROL		0x284
#define HOST_PKT_CONTROL_WC(x)		(((x) & 0xffff) << 0)
#define HOST_PKT_CONTROL_VC(x)		(((x) & 0x3) << 16)
#define HOST_PKT_CONTROL_DT(x)		(((x) & 0x3f) << 18)
#define HOST_PKT_CONTROL_HS_SEL(x)	(((x) & 0x1) << 24)
#define HOST_PKT_CONTROL_BTA_TX(x)	(((x) & 0x1) << 25)
#define HOST_PKT_CONTROL_BTA_NO_TX(x)	(((x) & 0x1) << 26)

#define HOST_SEND_PACKET		0x288
#define HOST_PKT_STATUS			0x28c
#define HOST_PKT_FIFO_WR_LEVEL		0x290
#define HOST_PKT_FIFO_RD_LEVEL		0x294
#define HOST_PKT_RX_PAYLOAD		0x298

#define HOST_PKT_RX_PKT_HEADER		0x29c
#define HOST_PKT_RX_PKT_HEADER_WC(x)	(((x) & 0xffff) << 0)
#define HOST_PKT_RX_PKT_HEADER_DT(x)	(((x) & 0x3f) << 16)
#define HOST_PKT_RX_PKT_HEADER_VC(x)	(((x) & 0x3) << 22)

#define HOST_IRQ_STATUS			0x2a0
#define HOST_IRQ_STATUS_SM_NOT_IDLE			(1 << 0)
#define HOST_IRQ_STATUS_TX_PKT_DONE			(1 << 1)
#define HOST_IRQ_STATUS_DPHY_DIRECTION			(1 << 2)
#define HOST_IRQ_STATUS_TX_FIFO_OVFLW			(1 << 3)
#define HOST_IRQ_STATUS_TX_FIFO_UDFLW			(1 << 4)
#define HOST_IRQ_STATUS_RX_FIFO_OVFLW			(1 << 5)
#define HOST_IRQ_STATUS_RX_FIFO_UDFLW			(1 << 6)
#define HOST_IRQ_STATUS_RX_PKT_HDR_RCVD			(1 << 7)
#define HOST_IRQ_STATUS_RX_PKT_PAYLOAD_DATA_RCVD	(1 << 8)
#define HOST_IRQ_STATUS_HOST_BTA_TIMEOUT		(1 << 29)
#define HOST_IRQ_STATUS_LP_RX_TIMEOUT			(1 << 30)
#define HOST_IRQ_STATUS_HS_TX_TIMEOUT			(1 << 31)

#define HOST_IRQ_STATUS2		0x2a4
#define HOST_IRQ_STATUS2_SINGLE_BIT_ECC_ERR		(1 << 0)
#define HOST_IRQ_STATUS2_MULTI_BIT_ECC_ERR		(1 << 1)
#define HOST_IRQ_STATUS2_CRC_ERR			(1 << 2)

#define HOST_IRQ_MASK			0x2a8
#define HOST_IRQ_MASK_SM_NOT_IDLE_MASK			(1 << 0)
#define HOST_IRQ_MASK_TX_PKT_DONE_MASK			(1 << 1)
#define HOST_IRQ_MASK_DPHY_DIRECTION_MASK		(1 << 2)
#define HOST_IRQ_MASK_TX_FIFO_OVFLW_MASK		(1 << 3)
#define HOST_IRQ_MASK_TX_FIFO_UDFLW_MASK		(1 << 4)
#define HOST_IRQ_MASK_RX_FIFO_OVFLW_MASK		(1 << 5)
#define HOST_IRQ_MASK_RX_FIFO_UDFLW_MASK		(1 << 6)
#define HOST_IRQ_MASK_RX_PKT_HDR_RCVD_MASK		(1 << 7)
#define HOST_IRQ_MASK_RX_PKT_PAYLOAD_DATA_RCVD_MASK	(1 << 8)
#define HOST_IRQ_MASK_HOST_BTA_TIMEOUT_MASK		(1 << 29)
#define HOST_IRQ_MASK_LP_RX_TIMEOUT_MASK		(1 << 30)
#define HOST_IRQ_MASK_HS_TX_TIMEOUT_MASK		(1 << 31)

#define HOST_IRQ_MASK2			0x2ac
#define HOST_IRQ_MASK2_SINGLE_BIT_ECC_ERR_MASK		(1 << 0)
#define HOST_IRQ_MASK2_MULTI_BIT_ECC_ERR_MASK		(1 << 1)
#define HOST_IRQ_MASK2_CRC_ERR_MASK			(1 << 2)

/* ------------------------------------- end -------------------------------- */
#define BITSLICE(x, a, b) (((x) >> (b)) & ((1 << ((a)-(b)+1)) - 1))

#ifdef DEBUG
#define W32(reg, val) \
do {printf("%s():%d reg 0x%p  val 0x%08x\n",\
		   __func__, __LINE__, reg, val);\
		__raw_writel(val, reg); } while (0)
#else
#define W32(reg, val) __raw_writel(val, reg)
#endif

#define R32(reg) __raw_readl(reg)

/* helper functions */
inline void dsi_host_ctrl_csr_setup(void __iomem *base,
	struct dsi_cfg_csr_object *dsi_config,
	uint16_t csr_setup_mask)
{
	if (BITSLICE(csr_setup_mask, 0, 0))
		W32(base + HOST_CFG_NUM_LANES,
			dsi_config->dsi_host_cfg_num_lanes);
	if (BITSLICE(csr_setup_mask, 1, 1))
		W32(base + HOST_CFG_NONCONTINUOUS_CLK,
			dsi_config->dsi_host_cfg_noncont_clk);
	if (BITSLICE(csr_setup_mask, 2, 2))
		W32(base + HOST_CFG_T_PRE, dsi_config->dsi_host_cfg_t_pre);
	if (BITSLICE(csr_setup_mask, 3, 3))
		W32(base + HOST_CFG_T_POST,
			dsi_config->dsi_host_cfg_t_post);
	if (BITSLICE(csr_setup_mask, 4, 4))
		W32(base + HOST_CFG_TX_GAP,
			dsi_config->dsi_host_cfg_tx_gap);
	if (BITSLICE(csr_setup_mask, 5, 5))
		W32(base + HOST_CFG_AUTOINSERT_EOTP,
			dsi_config->dsi_host_cfg_autoinsert_eotp);
	if (BITSLICE(csr_setup_mask, 6, 6))
		W32(base + HOST_CFG_EXTRA_CMDS_AFTER_EOTP,
			dsi_config->dsi_host_cfg_extrcmd_after_eotp);
	if (BITSLICE(csr_setup_mask, 7, 7))
		W32(base + HOST_CFG_HTX_TO_COUNT,
			dsi_config->dsi_host_cfg_htx_to_cnt);
	if (BITSLICE(csr_setup_mask, 8, 8))
		W32(base + HOST_CFG_LRX_H_TO_COUNT,
			dsi_config->dsi_host_cfg_lrx_h_to_cnt);
	if (BITSLICE(csr_setup_mask, 9, 9))
		W32(base + HOST_CFG_BTA_H_TO_COUNT,
			dsi_config->dsi_host_cfg_bta_h_to_cnt);
	if (BITSLICE(csr_setup_mask, 10, 10))
		W32(base + HOST_CFG_TWAKEUP,
			dsi_config->dsi_host_cfg_twakeup);
}

inline void dsi_host_ctrl_dpi_setup(void __iomem *base,
	struct dsi_cfg_dpi_object *dsi_config,
	uint32_t dpi_setup_mask)
{
	if (BITSLICE(dpi_setup_mask, 0, 0))
		W32(base + DPI_PIXEL_PAYLOAD_SIZE,
			dsi_config->dsi_host_cfg_dpi_pxl_payld_size);
	if (BITSLICE(dpi_setup_mask, 1, 1))
		W32(base + DPI_PIXEL_FIFO_SEND_LEVEL,
			dsi_config->dsi_host_cfg_dpi_pxl_fifo_send_lev);
	if (BITSLICE(dpi_setup_mask, 2, 2))
		W32(base + DPI_INTERFACE_COLOR_CODING,
			dsi_config->dsi_host_cfg_dpi_if_color_coding);
	if (BITSLICE(dpi_setup_mask, 3, 3))
		W32(base + DPI_PIXEL_FORMAT,
			dsi_config->dsi_host_cfg_dpi_pxl_format);
	if (BITSLICE(dpi_setup_mask, 4, 4))
		W32(base + DPI_VSYNC_POLARITY,
			dsi_config->dsi_host_cfg_dpi_vsync_pol);
	if (BITSLICE(dpi_setup_mask, 5, 5))
		W32(base + DPI_HSYNC_POLARITY,
			dsi_config->dsi_host_cfg_dpi_hsync_pol);
	if (BITSLICE(dpi_setup_mask, 6, 6))
		W32(base + DPI_VIDEO_MODE,
			dsi_config->dsi_host_cfg_dpi_video_mode);
	if (BITSLICE(dpi_setup_mask, 7, 7))
		W32(base + DPI_HFP, dsi_config->dsi_host_cfg_dpi_hfp);
	if (BITSLICE(dpi_setup_mask, 8, 8))
		W32(base + DPI_HBP, dsi_config->dsi_host_cfg_dpi_hbp);
	if (BITSLICE(dpi_setup_mask, 9, 9))
		W32(base + DPI_HSA, dsi_config->dsi_host_cfg_dpi_hsa);
	if (BITSLICE(dpi_setup_mask, 10, 10))
		W32(base + DPI_ENABLE_MULT_PKTS,
			dsi_config->dsi_host_cfg_dpi_en_mult_pkt);
	if (BITSLICE(dpi_setup_mask, 11, 11))
		W32(base + DPI_VBP, dsi_config->dsi_host_cfg_dpi_vbp);
	if (BITSLICE(dpi_setup_mask, 12, 12))
		W32(base + DPI_VFP, dsi_config->dsi_host_cfg_dpi_vfp);
	if (BITSLICE(dpi_setup_mask, 13, 13))
		W32(base + DPI_BLLP_MODE,
			dsi_config->dsi_host_cfg_dpi_bllp_mode);
	if (BITSLICE(dpi_setup_mask, 14, 14))
		W32(base + DPI_USE_NULL_PKT_BLLP,
			dsi_config->dsi_host_cfg_dpi_null_pkt_bllp);
	if (BITSLICE(dpi_setup_mask, 15, 15))
		W32(base + DPI_VACTIVE,
			dsi_config->dsi_host_cfg_dpi_vactive);
	if (BITSLICE(dpi_setup_mask, 16, 16))
		W32(base + DPI_VC, dsi_config->dsi_host_cfg_dpi_vc);
}

inline void dsi_host_ctrl_pkt_setup(void __iomem *base,
	struct dsi_cfg_pkt_object *dsi_config,
	uint8_t pkt_setup_mask)
{
	if (BITSLICE(pkt_setup_mask, 0, 0))
		W32(base + HOST_PKT_CONTROL,
			dsi_config->dsi_host_pkt_ctrl);
	if (BITSLICE(pkt_setup_mask, 2, 2))
		W32(base + HOST_IRQ_MASK, dsi_config->dsi_host_irq_mask);
	if (BITSLICE(pkt_setup_mask, 3, 3))
		W32(base + HOST_IRQ_MASK2, dsi_config->dsi_host_irq_mask2);
	if (BITSLICE(pkt_setup_mask, 1, 1))
		W32(base + HOST_SEND_PACKET,
			dsi_config->dsi_host_send_pkt);
}

inline void dsi_host_ctrl_dphy_setup(void __iomem *base,
	struct dsi_cfg_dphy_object *dsi_config,
	uint32_t dphy_setup_mask)
{
	int i;

	if (BITSLICE(dphy_setup_mask, 8, 8))
		W32(base + DPHY_TST, dsi_config->dphy_tst);
	if (BITSLICE(dphy_setup_mask, 9, 9))
		W32(base + DPHY_CN, dsi_config->dphy_cn);
	if (BITSLICE(dphy_setup_mask, 10, 10))
		W32(base + DPHY_CM, dsi_config->dphy_cm);
	if (BITSLICE(dphy_setup_mask, 11, 11))
		W32(base + DPHY_CO, dsi_config->dphy_co);
	if (BITSLICE(dphy_setup_mask, 7, 7))
		W32(base + DPHY_PD_PLL, dsi_config->dphy_pd_pll);
	/* todo: disable on zebu */
	/*Polling of DPHY Lock status / wait for PLL lock */
	for (i = 0; i < 100; i++) {
		u32 lock;
		udelay(10);
		/*todo: zebu abort when reading DPHY LOCK */
		lock = R32(DPHY_LOCK);
		printf("DPHY PLL Lock = 0x%08x\n", lock);
	}
	/*todo: Need to wait for lock here */

	if (BITSLICE(dphy_setup_mask, 1, 1))
		W32(base + DPHY_M_PRG_HS_PREPARE,
			dsi_config->dphy_m_prg_hs_prepare);
	if (BITSLICE(dphy_setup_mask, 2, 2))
		W32(base + DPHY_MC_PRG_HS_PREPARE,
			dsi_config->dphy_mc_prg_hs_prepare);
	if (BITSLICE(dphy_setup_mask, 3, 3))
		W32(base + DPHY_M_PRG_HS_ZERO,
			dsi_config->dphy_m_prg_hs_zero);
	if (BITSLICE(dphy_setup_mask, 4, 4))
		W32(base + DPHY_MC_PRG_HS_ZERO,
			dsi_config->dphy_mc_prg_hs_zero);
	if (BITSLICE(dphy_setup_mask, 5, 5))
		W32(base + DPHY_M_PRG_HS_TRAIL,
			dsi_config->dphy_m_prg_hs_trial);
	if (BITSLICE(dphy_setup_mask, 6, 6))
		W32(base + DPHY_MC_PRG_HS_TRAIL,
			dsi_config->dphy_mc_prg_hs_trial);
	if (BITSLICE(dphy_setup_mask, 0, 0))
		W32(base + DPHY_PD_TX, dsi_config->dphy_pd_tx);
	if (BITSLICE(dphy_setup_mask, 12, 12))
		W32(base + DPHY_LOCK, dsi_config->dphy_lock);
	if (BITSLICE(dphy_setup_mask, 13, 13))
		W32(base + DPHY_LOCK_BYP, dsi_config->dphy_lock_byp);
}
#endif /* _IMX8_MIPI_DSI_H_ */
