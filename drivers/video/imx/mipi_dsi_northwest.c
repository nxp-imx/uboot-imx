/*
 * Copyright 2016-2019 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */


#include <common.h>
#include <malloc.h>
#include <dm.h>

#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <linux/err.h>
#include <asm/io.h>
#include <linux/string.h>

#include "mipi_dsi_northwest_regs.h"
#include <video_bridge.h>
#include <panel.h>
#include <dsi_host.h>
#include <div64.h>
#include <asm/arch/imx-regs.h>
#include <dm/device-internal.h>
#include <regmap.h>
#include <syscon.h>
#include <asm/arch/clock.h>

#define MIPI_LCD_SLEEP_MODE_DELAY	(120)
#define MIPI_FIFO_TIMEOUT		250000 /* 250ms */
#define	PS2KHZ(ps)	(1000000000UL / (ps))

#define DIV_ROUND_CLOSEST_ULL(x, divisor)(		\
{							\
	typeof(divisor) __d = divisor;			\
	unsigned long long _tmp = (x) + (__d) / 2;	\
	do_div(_tmp, __d);				\
	_tmp;						\
}							\
)

enum mipi_dsi_mode {
	DSI_COMMAND_MODE,
	DSI_VIDEO_MODE
};

#define DSI_LP_MODE	0
#define DSI_HS_MODE	1

enum mipi_dsi_payload {
	DSI_PAYLOAD_CMD,
	DSI_PAYLOAD_VIDEO,
};

/*
 * mipi-dsi northwest driver information structure, holds useful data for the driver.
 */
struct mipi_dsi_northwest_info {
	void __iomem *mmio_base;
	struct mipi_dsi_device *device;
	struct mipi_dsi_host dsi_host;
	struct display_timing timings;
	struct regmap *sim;

	 uint32_t max_data_lanes;
	 uint32_t max_data_rate;
	 uint32_t pll_ref;
};

struct pll_divider {
	unsigned int cm;  /* multiplier */
	unsigned int cn;  /* predivider */
	unsigned int co;  /* outdivider */
};

/**
 * 'CM' value to 'CM' reigister config value map
 * 'CM' = [16, 255];
 */
static unsigned int cm_map_table[240] = {
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,	/* 16 ~ 23 */
	0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,	/* 24 ~ 31 */

	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,	/* 32 ~ 39 */
	0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, /* 40 ~ 47 */

	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, /* 48 ~ 55 */
	0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, /* 56 ~ 63 */

	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, /* 64 ~ 71 */
	0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, /* 72 ~ 79 */

	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, /* 80 ~ 87 */
	0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, /* 88 ~ 95 */

	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, /* 96  ~ 103 */
	0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, /* 104 ~ 111 */

	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, /* 112 ~ 119 */
	0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, /* 120 ~ 127 */

	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, /* 128 ~ 135 */
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, /* 136 ~ 143 */

	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, /* 144 ~ 151 */
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, /* 152 ~ 159 */

	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, /* 160 ~ 167 */
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, /* 168 ~ 175 */

	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, /* 176 ~ 183 */
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, /* 184 ~ 191 */

	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, /* 192 ~ 199 */
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, /* 200 ~ 207 */

	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, /* 208 ~ 215 */
	0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, /* 216 ~ 223 */

	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, /* 224 ~ 231 */
	0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, /* 232 ~ 239 */

	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, /* 240 ~ 247 */
	0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f	/* 248 ~ 255 */
};

/**
 * map 'CN' value to 'CN' reigister config value
 * 'CN' = [1, 32];
 */
static unsigned int cn_map_table[32] = {
	0x1f, 0x00, 0x10, 0x18, 0x1c, 0x0e, 0x07, 0x13,	/* 1  ~ 8  */
	0x09, 0x04, 0x02, 0x11, 0x08, 0x14, 0x0a, 0x15,	/* 9  ~ 16 */
	0x1a, 0x1d, 0x1e, 0x0f, 0x17, 0x1b, 0x0d, 0x16,	/* 17 ~ 24 */
	0x0b, 0x05, 0x12, 0x19, 0x0c, 0x06, 0x03, 0x01	/* 25 ~ 32 */
};

/**
 * map 'CO' value to 'CO' reigister config value
 * 'CO' = { 1, 2, 4, 8 };
 */
static unsigned int co_map_table[4] = {
	0x0, 0x1, 0x2, 0x3
};

unsigned long gcd(unsigned long a, unsigned long b)
{
	unsigned long r = a | b;

	if (!a || !b)
		return r;

	/* Isolate lsbit of r */
	r &= -r;

	while (!(b & r))
		b >>= 1;
	if (b == r)
		return r;

	for (;;) {
		while (!(a & r))
			a >>= 1;
		if (a == r)
			return r;
		if (a == b)
			return a;

		if (a < b)
			swap(a, b);
		a -= b;
		a >>= 1;
		if (a & r)
			a += b;
		a >>= 1;
	}
}

static void mipi_dsi_set_mode(struct mipi_dsi_northwest_info *mipi_dsi,
			      uint8_t mode);
static int mipi_dsi_dcs_cmd(struct mipi_dsi_northwest_info *mipi_dsi,
			    u8 cmd, const u32 *param, int num);

static void mipi_dsi_set_mode(struct mipi_dsi_northwest_info *mipi_dsi,
			      uint8_t mode)
{
	switch (mode) {
	case DSI_LP_MODE:
		writel(0x1, mipi_dsi->mmio_base + HOST_CFG_NONCONTINUOUS_CLK);
		break;
	case DSI_HS_MODE:
		writel(0x0, mipi_dsi->mmio_base + HOST_CFG_NONCONTINUOUS_CLK);
		break;
	default:
		printf("invalid dsi mode\n");
		return;
	}

	mdelay(1);
}

static int mipi_dsi_dphy_init(struct mipi_dsi_northwest_info *mipi_dsi)
{
	uint32_t time_out = 100;
	uint32_t lock;
	uint32_t req_bit_clk;
	uint32_t bpp;

	int i, best_div = -1;
	int64_t delta;
	uint64_t least_delta = ~0U;
	uint64_t limit, div_result;
	uint64_t denominator, numerator, divisor;
	uint64_t norm_denom, norm_num, split_denom;
	struct pll_divider div = { 0 };

	regmap_update_bits(mipi_dsi->sim, SIM_SOPT1, MIPI_ISO_DISABLE, MIPI_ISO_DISABLE);

	bpp = mipi_dsi_pixel_format_to_bpp(mipi_dsi->device->format);

	/* req_bit_clk is PLL out, clk_byte is 1/8th of the req_bit_clk
	*  We need meet clk_byte_freq >= dpi_pclk_freq * DPI_pixel_size / ( 8 * (cfg_num_lanes + 1))
	*/

	req_bit_clk = mipi_dsi->timings.pixelclock.typ;
	req_bit_clk = req_bit_clk * bpp;

	switch (mipi_dsi->device->lanes) {
	case 1:
		break;
	case 2:
		req_bit_clk = req_bit_clk >> 1;
		break;
	case 4:
		req_bit_clk = req_bit_clk >> 2;
		break;
	default:
		printf("requested data lane num is invalid\n");
		return -EINVAL;
	}

	debug("req_bit_clk %u\n", req_bit_clk);

	/* The max rate for PLL out is 800Mhz */
	if (req_bit_clk > mipi_dsi->max_data_rate)
		return -EINVAL;

	/* calc CM, CN and CO according to PHY PLL formula:
	 *
	 * 'PLL out bitclk = refclk * CM / (CN * CO);'
	 *
	 * Let:
	 * 'numerator   = bitclk / divisor';
	 * 'denominator = refclk / divisor';
	 * Then:
	 * 'numerator / denominator = CM / (CN * CO)';
	 *
	 * CM is in [16, 255]
	 * CN is in [1, 32]
	 * CO is in { 1, 2, 4, 8 };
	 */
	divisor = gcd(mipi_dsi->pll_ref, req_bit_clk);
	WARN_ON(divisor == 1);

	div_result = req_bit_clk;
	do_div(div_result, divisor);
	numerator = div_result;

	div_result = mipi_dsi->pll_ref;
	do_div(div_result, divisor);
	denominator = div_result;

	/* denominator & numerator out of range check */
	if (DIV_ROUND_CLOSEST_ULL(numerator, denominator) > 255 ||
	    DIV_ROUND_CLOSEST_ULL(denominator, numerator) > 32 * 8)
		return -EINVAL;

	/* Normalization: reduce or increase
	 * numerator	to [16, 255]
	 * denominator	to [1, 32 * 8]
	 * Reduce normalization result is 'approximiate'
	 * Increase nomralization result is 'precise'
	 */
	if (numerator > 255 || denominator > 32 * 8) {
		/* approximate */
		if (likely(numerator > denominator)) {
			/* 'numerator > 255';
			 * 'limit' should meet below conditions:
			 *  a. '(numerator   / limit) >= 16'
			 *  b. '(denominator / limit) >= 1'
			 */
			limit = min(denominator,
				    DIV_ROUND_CLOSEST_ULL(numerator, 16));

			/* Let:
			 * norm_num   = numerator   / i;
			 * norm_denom = denominator / i;
			 *
			 * So:
			 * delta = numerator * norm_denom -
			 * 	   denominator * norm_num
			 */
			for (i = 2; i <= limit; i++) {
				norm_num = DIV_ROUND_CLOSEST_ULL(numerator, i);
				if (norm_num > 255)
					continue;

				norm_denom = DIV_ROUND_CLOSEST_ULL(denominator, i);

				/* 'norm_num <= 255' && 'norm_num > norm_denom'
				 * so, 'norm_denom < 256'
				 */
				delta = numerator * norm_denom -
					denominator * norm_num;
				delta = abs(delta);
				if (delta < least_delta) {
					least_delta = delta;
					best_div = i;
				} else if (delta == least_delta) {
					/* choose better one IF:
					 * 'norm_denom' derived from last 'best_div'
					 * needs later split, i.e, 'norm_denom > 32'.
					 */
					if (DIV_ROUND_CLOSEST_ULL(denominator, best_div) > 32) {
						least_delta = delta;
						best_div = i;
					}
				}
			}
		} else {
			/* 'denominator > 32 * 8';
			 * 'limit' should meet below conditions:
			 *  a. '(numerator   / limit >= 16'
			 *  b. '(denominator / limit >= 1': obviously.
			 */
			limit = DIV_ROUND_CLOSEST_ULL(numerator, 16);
			if (!limit ||
			    DIV_ROUND_CLOSEST_ULL(denominator, limit) > 32 * 8)
				return -EINVAL;

			for (i = 2; i <= limit; i++) {
				norm_denom = DIV_ROUND_CLOSEST_ULL(denominator, i);
				if (norm_denom > 32 * 8)
					continue;

				norm_num = DIV_ROUND_CLOSEST_ULL(numerator, i);

				/* 'norm_denom <= 256' && 'norm_num < norm_denom'
				 * so, 'norm_num <= 255'
				 */
				delta = numerator * norm_denom -
					denominator * norm_num;
				delta = abs(delta);
				if (delta < least_delta) {
					least_delta = delta;
					best_div = i;
				} else if (delta == least_delta) {
					if (DIV_ROUND_CLOSEST_ULL(denominator, best_div) > 32) {
						least_delta = delta;
						best_div = i;
					}
				}
			}
		}

		numerator   = DIV_ROUND_CLOSEST_ULL(numerator, best_div);
		denominator = DIV_ROUND_CLOSEST_ULL(denominator, best_div);
	} else if (numerator < 16) {
		/* precise */

		/* 'limit' should meet below conditions:
		 *  a. 'denominator * limit <= 32 * 8'
		 *  b. '16 <= numerator * limit <= 255'
		 *  Choose 'limit' to be the least value
		 *  which makes 'numerator * limit' to be
		 *  in [16, 255].
		 */
		limit = min(256 / (uint32_t)denominator,
			    255 / (uint32_t)numerator);
		if (limit == 1 || limit < DIV_ROUND_UP_ULL(16, numerator))
			return -EINVAL;

		/* choose the least available value for 'limit' */
		limit = DIV_ROUND_UP_ULL(16, numerator);
		numerator   = numerator * limit;
		denominator = denominator * limit;

		WARN_ON(numerator < 16 || denominator > 32 * 8);
	}

	div.cm = cm_map_table[numerator - 16];

	/* split 'denominator' to 'CN' and 'CO' */
	if (denominator > 32) {
		/* traverse four possible values of 'CO'
		 * there must be some value of 'CO' can be used
		 */
		least_delta = ~0U;
		for (i = 0; i < 4; i++) {
			split_denom = DIV_ROUND_CLOSEST_ULL(denominator, 1 << i);
			if (split_denom > 32)
				continue;

			/* calc deviation to choose the best one */
			delta = denominator - split_denom * (1 << i);
			delta = abs(delta);
			if (delta < least_delta) {
				least_delta = delta;
				div.co = co_map_table[i];
				div.cn = cn_map_table[split_denom - 1];
			}
		}
	} else {
		div.co = co_map_table[1 >> 1];
		div.cn = cn_map_table[denominator - 1];
	}

	debug("cn 0x%x, cm 0x%x, co 0x%x\n", div.cn, div.cm, div.co);

	writel(div.cn, mipi_dsi->mmio_base + DPHY_CN);
	writel(div.cm, mipi_dsi->mmio_base + DPHY_CM);
	writel(div.co, mipi_dsi->mmio_base + DPHY_CO);

	writel(0x25, mipi_dsi->mmio_base + DPHY_TST);
	writel(0x0, mipi_dsi->mmio_base + DPHY_PD_PLL);

	while (!(lock = readl(mipi_dsi->mmio_base + DPHY_LOCK))) {
		udelay(10);
		time_out--;
		if (time_out == 0) {
			printf("cannot get the dphy lock = 0x%x\n", lock);
			return -EINVAL;
		}
	}
	debug("%s: dphy lock = 0x%x\n", __func__, lock);

	writel(0x0, mipi_dsi->mmio_base + DPHY_LOCK_BYP);
	writel(0x1, mipi_dsi->mmio_base + DPHY_RTERM_SEL);
	writel(0x0, mipi_dsi->mmio_base + DPHY_AUTO_PD_EN);
	writel(0x1, mipi_dsi->mmio_base + DPHY_RXLPRP);
	writel(0x1, mipi_dsi->mmio_base + DPHY_RXCDRP);
	writel(0x0, mipi_dsi->mmio_base + DPHY_M_PRG_HS_PREPARE);
	writel(0x0, mipi_dsi->mmio_base + DPHY_MC_PRG_HS_PREPARE);
	writel(0x9, mipi_dsi->mmio_base + DPHY_M_PRG_HS_ZERO);
	writel(0x20, mipi_dsi->mmio_base + DPHY_MC_PRG_HS_ZERO);
	writel(0x5, mipi_dsi->mmio_base + DPHY_M_PRG_HS_TRAIL);
	writel(0x5, mipi_dsi->mmio_base + DPHY_MC_PRG_HS_TRAIL);
	writel(0x0, mipi_dsi->mmio_base + DPHY_PD_DPHY);

	regmap_update_bits(mipi_dsi->sim, SIM_SOPT1CFG, DSI_PLL_EN, DSI_PLL_EN);
	return 0;
}

static int mipi_dsi_host_init(struct mipi_dsi_northwest_info *mipi_dsi)
{
	uint32_t lane_num;

	switch (mipi_dsi->device->lanes) {
	case 1:
		lane_num = 0x0;
		break;
	case 2:
		lane_num = 0x1;
		break;
	default:
		/* Invalid lane num */
		return -EINVAL;
	}

	writel(lane_num, mipi_dsi->mmio_base + HOST_CFG_NUM_LANES);
	writel(0x1, mipi_dsi->mmio_base + HOST_CFG_NONCONTINUOUS_CLK);
	writel(0x1, mipi_dsi->mmio_base + HOST_CFG_T_PRE);
	writel(52, mipi_dsi->mmio_base + HOST_CFG_T_POST);
	writel(13, mipi_dsi->mmio_base + HOST_CFG_TX_GAP);
	writel(0x1, mipi_dsi->mmio_base + HOST_CFG_AUTOINSERT_EOTP);
	writel(0x0, mipi_dsi->mmio_base + HOST_CFG_EXTRA_CMDS_AFTER_EOTP);
	writel(0x0, mipi_dsi->mmio_base + HOST_CFG_HTX_TO_COUNT);
	writel(0x0, mipi_dsi->mmio_base + HOST_CFG_LRX_H_TO_COUNT);
	writel(0x0, mipi_dsi->mmio_base + HOST_CFG_BTA_H_TO_COUNT);
	writel(0x3A98, mipi_dsi->mmio_base + HOST_CFG_TWAKEUP);

	return 0;
}

static int mipi_dsi_dpi_init(struct mipi_dsi_northwest_info *mipi_dsi)
{
	uint32_t color_coding, pixel_fmt;
	int bpp;
	struct display_timing *timings = &(mipi_dsi->timings);

	bpp = mipi_dsi_pixel_format_to_bpp(mipi_dsi->device->format);
	if (bpp < 0)
		return -EINVAL;

	writel(timings->hactive.typ, mipi_dsi->mmio_base + DPI_PIXEL_PAYLOAD_SIZE);
	writel(timings->hactive.typ, mipi_dsi->mmio_base + DPI_PIXEL_FIFO_SEND_LEVEL);

	switch (bpp) {
	case 24:
		color_coding = 5;
		pixel_fmt = 3;
		break;
	case 16:
	case 18:
	default:
		/* Not supported */
		return -EINVAL;
	}
	writel(color_coding, mipi_dsi->mmio_base + DPI_INTERFACE_COLOR_CODING);
	writel(pixel_fmt, mipi_dsi->mmio_base + DPI_PIXEL_FORMAT);
	writel(0x0, mipi_dsi->mmio_base + DPI_VSYNC_POLARITY);
	writel(0x0, mipi_dsi->mmio_base + DPI_HSYNC_POLARITY);
	writel(0x2, mipi_dsi->mmio_base + DPI_VIDEO_MODE);

	writel(timings->hfront_porch.typ * (bpp >> 3), mipi_dsi->mmio_base + DPI_HFP);
	writel(timings->hback_porch.typ * (bpp >> 3), mipi_dsi->mmio_base + DPI_HBP);
	writel(timings->hsync_len.typ * (bpp >> 3), mipi_dsi->mmio_base + DPI_HSA);
	writel(0x0, mipi_dsi->mmio_base + DPI_ENABLE_MULT_PKTS);

	writel(timings->vback_porch.typ, mipi_dsi->mmio_base + DPI_VBP);
	writel(timings->vfront_porch.typ, mipi_dsi->mmio_base + DPI_VFP);
	writel(0x1, mipi_dsi->mmio_base + DPI_BLLP_MODE);
	writel(0x0, mipi_dsi->mmio_base + DPI_USE_NULL_PKT_BLLP);

	writel(timings->vactive.typ - 1, mipi_dsi->mmio_base + DPI_VACTIVE);

	writel(0x0, mipi_dsi->mmio_base + DPI_VC);

	return 0;
}

static void mipi_dsi_init_interrupt(struct mipi_dsi_northwest_info *mipi_dsi)
{
	/* disable all the irqs */
	writel(0xffffffff, mipi_dsi->mmio_base + HOST_IRQ_MASK);
	writel(0x7, mipi_dsi->mmio_base + HOST_IRQ_MASK2);
}

static int mipi_display_enter_sleep(struct mipi_dsi_northwest_info *mipi_dsi)
{
	int err;

	err = mipi_dsi_dcs_cmd(mipi_dsi, MIPI_DCS_SET_DISPLAY_OFF,
			       NULL, 0);
	if (err)
		return -EINVAL;
	mdelay(50);

	err = mipi_dsi_dcs_cmd(mipi_dsi, MIPI_DCS_ENTER_SLEEP_MODE,
			NULL, 0);
	if (err)
		printf("MIPI DSI DCS Command sleep in error!\n");

	mdelay(MIPI_LCD_SLEEP_MODE_DELAY);

	return err;
}

static void mipi_dsi_wr_tx_header(struct mipi_dsi_northwest_info *mipi_dsi,
				  u8 di, u8 data0, u8 data1, u8 mode, u8 need_bta)
{
	uint32_t pkt_control = 0;
	uint16_t word_count = 0;

	word_count = data0 | (data1 << 8);
	pkt_control = HOST_PKT_CONTROL_WC(word_count) |
		      HOST_PKT_CONTROL_VC(0)	      |
		      HOST_PKT_CONTROL_DT(di)	      |
		      HOST_PKT_CONTROL_HS_SEL(mode)   |
		      HOST_PKT_CONTROL_BTA_TX(need_bta);

	debug("pkt_control = %x\n", pkt_control);
	writel(pkt_control, mipi_dsi->mmio_base + HOST_PKT_CONTROL);
}

static void mipi_dsi_wr_tx_data(struct mipi_dsi_northwest_info *mipi_dsi,
				uint32_t tx_data)
{
	writel(tx_data, mipi_dsi->mmio_base + HOST_TX_PAYLOAD);
}

static void mipi_dsi_long_data_wr(struct mipi_dsi_northwest_info *mipi_dsi,
			const uint8_t *data0, uint32_t data_size)
{
	uint32_t data_cnt = 0, payload = 0;

	/* in case that data count is more than 4 */
	for (data_cnt = 0; data_cnt < data_size; data_cnt += 4) {
		/*
		 * after sending 4bytes per one time,
		 * send remainder data less then 4.
		 */
		if ((data_size - data_cnt) < 4) {
			if ((data_size - data_cnt) == 3) {
				payload = data0[data_cnt] |
					  (data0[data_cnt + 1] << 8) |
					  (data0[data_cnt + 2] << 16);
				debug("count = 3 payload = %x, %x %x %x\n",
				      payload, data0[data_cnt], data0[data_cnt + 1], data0[data_cnt + 2]);
			} else if ((data_size - data_cnt) == 2) {
				payload = data0[data_cnt] |
					  (data0[data_cnt + 1] << 8);
				debug("count = 2 payload = %x, %x %x\n",
				      payload, data0[data_cnt], data0[data_cnt + 1]);
			} else if ((data_size - data_cnt) == 1) {
				payload = data0[data_cnt];
				debug("count = 1 payload = %x, %x\n",
				      payload, data0[data_cnt]);
			}

			mipi_dsi_wr_tx_data(mipi_dsi, payload);
		} else {
			payload = data0[data_cnt] |
				  (data0[data_cnt + 1] << 8) |
				  (data0[data_cnt + 2] << 16) |
				  (data0[data_cnt + 3] << 24);

			debug("count = 4 payload = %x, %x %x %x %x\n",
			      payload, *(u8 *)(data0 + data_cnt),
			      data0[data_cnt + 1],
			      data0[data_cnt + 2],
			      data0[data_cnt + 3]);

			mipi_dsi_wr_tx_data(mipi_dsi, payload);
		}
	}
}

static int wait_for_pkt_done(struct mipi_dsi_northwest_info *mipi_dsi, unsigned long timeout)
{
	uint32_t irq_status;

	do {
		irq_status = readl(mipi_dsi->mmio_base + HOST_IRQ_STATUS);
		if (irq_status & HOST_IRQ_STATUS_TX_PKT_DONE)
			return timeout;

		udelay(1);
	} while (--timeout);

	return 0;
}

static int mipi_dsi_pkt_write(struct mipi_dsi_northwest_info *mipi_dsi,
			u8 data_type, const u8 *buf, int len)
{
	int ret = 0;
	const uint8_t *data = (const uint8_t *)buf;

	debug("mipi_dsi_pkt_write data_type 0x%x, buf 0x%x, len %u\n", data_type, (u32)buf, len);

	if (len == 0)
		/* handle generic long write command */
		mipi_dsi_wr_tx_header(mipi_dsi, data_type, data[0], data[1], DSI_LP_MODE, 0);
	else {
		/* handle generic long write command */
		mipi_dsi_long_data_wr(mipi_dsi, data, len);
		mipi_dsi_wr_tx_header(mipi_dsi, data_type, len & 0xff,
				      (len & 0xff00) >> 8, DSI_LP_MODE, 0);
	}

	/* send packet */
	writel(0x1, mipi_dsi->mmio_base + HOST_SEND_PACKET);
	ret = wait_for_pkt_done(mipi_dsi, MIPI_FIFO_TIMEOUT);

	if (!ret) {
		printf("wait tx done timeout!\n");
		return -ETIMEDOUT;
	}

	return 0;
}

#define	DSI_CMD_BUF_MAXSIZE         (128)

static int mipi_dsi_dcs_cmd(struct mipi_dsi_northwest_info *mipi_dsi,
			    u8 cmd, const u32 *param, int num)
{
	int err = 0;
	u32 buf[DSI_CMD_BUF_MAXSIZE];

	switch (cmd) {
	case MIPI_DCS_EXIT_SLEEP_MODE:
	case MIPI_DCS_ENTER_SLEEP_MODE:
	case MIPI_DCS_SET_DISPLAY_ON:
	case MIPI_DCS_SET_DISPLAY_OFF:
		buf[0] = cmd;
		buf[1] = 0x0;
		err = mipi_dsi_pkt_write(mipi_dsi,
				MIPI_DSI_DCS_SHORT_WRITE, (u8 *)buf, 0);
		break;

	default:
		printf("MIPI DSI DCS Command:0x%x Not supported!\n", cmd);
		break;
	}

	return err;
}

static void reset_dsi_domains(struct mipi_dsi_northwest_info *mipi_dsi, bool reset)
{
	/* escape domain */
	regmap_update_bits(mipi_dsi->sim, SIM_SOPT1CFG,
			DSI_RST_ESC_N, (reset ? 0 : DSI_RST_ESC_N));
	/* byte domain */
	regmap_update_bits(mipi_dsi->sim, SIM_SOPT1CFG,
			DSI_RST_BYTE_N, (reset ? 0 : DSI_RST_BYTE_N));

	/* dpi domain */
	regmap_update_bits(mipi_dsi->sim, SIM_SOPT1CFG,
			DSI_RST_DPI_N, (reset ? 0 : DSI_RST_DPI_N));
}

static void mipi_dsi_shutdown(struct mipi_dsi_northwest_info *mipi_dsi)
{
	mipi_display_enter_sleep(mipi_dsi);

	writel(0x1, mipi_dsi->mmio_base + DPHY_PD_PLL);
	writel(0x1, mipi_dsi->mmio_base + DPHY_PD_DPHY);

	enable_mipi_dsi_clk(false);

	reset_dsi_domains(mipi_dsi, true);
}

static inline struct mipi_dsi_northwest_info *host_to_dsi(struct mipi_dsi_host *host)
{
	return container_of(host, struct mipi_dsi_northwest_info, dsi_host);
}

static int mipi_dsi_northwest_host_attach(struct mipi_dsi_host *host,
				   struct mipi_dsi_device *device)
{
	struct mipi_dsi_northwest_info *mipi_dsi = host_to_dsi(host);
	int ret;

	/* Assert resets */
	reset_dsi_domains(mipi_dsi, true);

	/* Enable mipi relevant clocks */
	enable_mipi_dsi_clk(true);

	ret = mipi_dsi_dphy_init(mipi_dsi);
	if (ret < 0)
		return ret;

	ret = mipi_dsi_host_init(mipi_dsi);
	if (ret < 0)
		return ret;

	ret = mipi_dsi_dpi_init(mipi_dsi);
	if (ret < 0)
		return ret;

	/* Deassert resets */
	reset_dsi_domains(mipi_dsi, false);

	/* display_en */
	regmap_update_bits(mipi_dsi->sim, SIM_SOPT1CFG, DSI_SD, 0);

	/* normal cm */
	regmap_update_bits(mipi_dsi->sim, SIM_SOPT1CFG, DSI_CM, 0);
	mdelay(20);

	/* Disable all interrupts, since we use polling */
	mipi_dsi_init_interrupt(mipi_dsi);

	return 0;
}

static ssize_t mipi_dsi_northwest_host_transfer(struct mipi_dsi_host *host,
					 const struct mipi_dsi_msg *msg)
{
	struct mipi_dsi_northwest_info *dsi = host_to_dsi(host);

	if (!msg)
		return -EINVAL;

	/* do some minimum sanity checking */
	if (!mipi_dsi_packet_format_is_short(msg->type) &&
	    !mipi_dsi_packet_format_is_long(msg->type))
		return -EINVAL;

#ifdef DEBUG
	int i = 0;
	u8 *p = msg->tx_buf;

	printf("sec_mipi_dsi_host_transfer\n");
	for (i; i < msg->tx_len; i++) {
		printf("0x%.2x ", *(u8 *)p);
		p++;
	}
	printf("\n");
#endif

	if (mipi_dsi_packet_format_is_long(msg->type)) {
		return mipi_dsi_pkt_write(dsi, msg->type, msg->tx_buf, msg->tx_len);
	} else {
		return mipi_dsi_pkt_write(dsi, msg->type, msg->tx_buf, 0);
	}
}


static const struct mipi_dsi_host_ops mipi_dsi_northwest_host_ops = {
	.attach = mipi_dsi_northwest_host_attach,
	.transfer = mipi_dsi_northwest_host_transfer,
};

static int mipi_dsi_northwest_init(struct udevice *dev,
			    struct mipi_dsi_device *device,
			    struct display_timing *timings,
			    unsigned int max_data_lanes,
			    const struct mipi_dsi_phy_ops *phy_ops)
{
	struct mipi_dsi_northwest_info *dsi = dev_get_priv(dev);
	int ret;

	dsi->max_data_lanes = max_data_lanes;
	dsi->device = device;
	dsi->dsi_host.ops = &mipi_dsi_northwest_host_ops;
	device->host = &dsi->dsi_host;

	dsi->timings = *timings;
	dsi->mmio_base = (void *)dev_read_addr(device->dev);
	if ((fdt_addr_t)dsi->mmio_base == FDT_ADDR_T_NONE) {
		dev_err(device->dev, "dsi dt register address error\n");
		return -EINVAL;
	}

	ret = dev_read_u32(device->dev, "max-data-rate", &dsi->max_data_rate);
	if (ret) {
		dev_err(device->dev, "fail to get max-data-rate\n");
		return -EINVAL;
	}

	ret = dev_read_u32(device->dev, "phy-ref-clkfreq", &dsi->pll_ref);
	if (ret) {
		dev_err(device->dev, "fail to get phy-ref-clkfreq\n");
		return -EINVAL;
	}

	dsi->sim = syscon_regmap_lookup_by_phandle(device->dev, "sim");
	if (IS_ERR(dsi->sim)) {
		dev_err(device->dev, "fail to get sim regmap\n");
		return PTR_ERR(dsi->sim);
	}

	return 0;
}

static int mipi_dsi_northwest_enable(struct udevice *dev)
{
	struct mipi_dsi_northwest_info *mipi_dsi = dev_get_priv(dev);

	/* Enter the HS mode for video stream */
	mipi_dsi_set_mode(mipi_dsi, DSI_HS_MODE);

	return 0;
}

static int mipi_dsi_northwest_disable(struct udevice *dev)
{
	struct mipi_dsi_northwest_info *mipi_dsi = dev_get_priv(dev);

	mipi_dsi_shutdown(mipi_dsi);
	return 0;
}

struct dsi_host_ops mipi_dsi_northwest_ops = {
	.init = mipi_dsi_northwest_init,
	.enable = mipi_dsi_northwest_enable,
	.disable = mipi_dsi_northwest_disable,
};

static int mipi_dsi_northwest_probe(struct udevice *dev)
{
	return 0;
}

static const struct udevice_id mipi_dsi_northwest_ids[] = {
	{ .compatible = "northwest,mipi-dsi" },
	{ }
};

U_BOOT_DRIVER(mipi_dsi_northwest) = {
	.name			= "mipi_dsi_northwest",
	.id			= UCLASS_DSI_HOST,
	.of_match		= mipi_dsi_northwest_ids,
	.probe			= mipi_dsi_northwest_probe,
	.remove 		= mipi_dsi_northwest_disable,
	.ops			= &mipi_dsi_northwest_ops,
	.priv_auto_alloc_size	= sizeof(struct mipi_dsi_northwest_info),
};
