// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2025 NXP
 */

#include <common.h>
#include <malloc.h>
#include <video.h>
#include <video_bridge.h>
#include <video_link.h>

#include <asm/cache.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <linux/err.h>
#include <asm/io.h>
#include <asm/arch-imx9/clock.h>

#include "../../videomodes.h"
#include <linux/string.h>
#include <linux/list.h>
#include <linux/fb.h>
#include <linux/bug.h>
#include <linux/delay.h>
#include <linux/iopoll.h>
#include "dcif-regs.h"
#include <log.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <dm/device_compat.h>
#include <clk.h>
#include <regmap.h>
#include <syscon.h>
#include <display.h>

#define	PS2KHZ(ps)	(1000000000UL / (ps))
#define HZ2PS(hz)	(1000000000UL / ((hz) / 1000))

/* registers in blk-ctrl */
#define CLOCK_CTRL			0x0
#define DSIP_CLK_SEL_MASK		0x2
#define CCM				0x0
#define LVDS_PLL_7			0x2

#define QOS_SETTING			0x1c
#define DISPLAY_PANIC_QOS_MASK		0x70
#define DISPLAY_PANIC_QOS(n)		(((n) & 0x7) << 4)
#define DISPLAY_ARQOS_MASK		0x7
#define DISPLAY_ARQOS(n)		((n) & 0x7)

struct dcif_priv {
	fdt_addr_t reg_base;
	struct udevice *disp_dev;

	struct regmap *regmap;

	u32 thres_low_mul;
	u32 thres_low_div;
	u32 thres_high_mul;
	u32 thres_high_div;

	struct clk clk_axi;
	struct clk clk_apb;
	struct clk clk_pix;
	struct clk clk_ldb;
	struct clk clk_ldb_vco;

	bool hpol;              /* horizontal pulse polarity    */
	bool vpol;              /* vertical pulse polarity      */

    bool is_crc;
};

static int dcif_video_bind(struct udevice *dev)
{
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);

	debug("%s\n", __func__);

	/* Max size supported by LCDIF, because in bind, we can't probe panel */
	plat->size = 1920 * 1080 *4;

	return 0;
}

static int dcif_of_get_timings(struct udevice *dev,
			      struct display_timing *timings)
{
	int ret = 0;
	struct dcif_priv *priv = dev_get_priv(dev);

	priv->disp_dev = video_link_get_next_device(dev);
	if (!priv->disp_dev ||
		(device_get_uclass_id(priv->disp_dev) != UCLASS_VIDEO_BRIDGE
		&& device_get_uclass_id(priv->disp_dev) != UCLASS_DISPLAY)) {

		printf("fail to find output device\n");
		return -ENODEV;
	}

	debug("disp_dev %s\n", priv->disp_dev->name);

	ret = video_link_get_display_timings(timings);
	if (ret) {
		printf("fail to get display timings\n");
		return ret;
	}

    return 0;
}

static int dcif_check_chip_info(struct dcif_priv *priv)
{
	int ret = 0;
	u32 val, vmin, vmaj;

	val = readl((ulong)(priv->reg_base + DCIF_VER));

	priv->is_crc = val & 0x2;

	vmin = DCIF_VER_GET_MINOR(val);
	vmaj = DCIF_VER_GET_MAJOR(val);
	debug("DCIF version is %d.%d\n", vmaj, vmin);

    return ret;
}

static void dcif_shadow_load_enable(struct dcif_priv *priv)
{
	u32 reg;

	reg = readl((ulong)(priv->reg_base + DCIF_CTRLDESC0(0)));
	reg |= DCIF_CTRLDESC0_SHADOW_LOAD_EN;
	writel(reg, (ulong)(priv->reg_base + DCIF_CTRLDESC0(0)));
}

static void dcif_enable_plane_panic(struct dcif_priv *priv)
{
	u32 reg;

	/* Set FIFO Panic watermarks, low 1/3, high 2/3 . */
	reg = DCIF_PANIC_THRES_LOW(1 * PANIC0_THRES_MAX / 3) |
	      DCIF_PANIC_THRES_HIGH(2 * PANIC0_THRES_MAX / 3) |
	      DCIF_PANIC_THRES_REQ_EN;
	writel(reg, (ulong)(priv->reg_base + DCIF_PANIC_THRES(0)));

	/*
	 * Enable FIFO Panic, this does not generate interrupt, but
	 * boosts NoC priority based on FIFO Panic watermarks.
	 */
	reg = readl((ulong)(priv->reg_base + DCIF_IE1(0)));
	reg |= DCIF_INT1_FIFO_PANIC0;
	writel(reg, (ulong)(priv->reg_base + DCIF_IE1(0)));
	writel(reg, (ulong)(priv->reg_base + DCIF_IE1(1)));
	writel(reg, (ulong)(priv->reg_base + DCIF_IE1(2)));
}

static void dcif_set_formats(struct dcif_priv *priv, unsigned int format)
{
	u32 reg;

	reg = readl((ulong)(priv->reg_base + DCIF_DPI_CTRL));
	reg &= ~DCIF_DPI_CTRL_DATA_PATTERN_MASK;
	/* fixed 24 bits output */
	reg |= DCIF_DPI_CTRL_DATA_PATTERN(PATTERN_RGB888);
	writel(reg, (ulong)(priv->reg_base + DCIF_DPI_CTRL));

	if (format == VIDEO_BPP32) {
		/* fixed ARGB888 */
		reg = readl((ulong)(priv->reg_base + DCIF_CTRLDESC0(0)));
		reg &= ~(DCIF_CTRLDESC0_FORMAT_MASK | DCIF_CTRLDESC0_YUV_FORMAT_MASK);
		reg |= DCIF_CTRLDESC0_FORMAT(CTRLDESCL0_FORMAT_ARGB8888);
		reg |= DCIF_CTRLDESC0_GLOBAL_ALPHA(0xff);
		reg |= ALPHA_GLOBAL;
		writel(reg, (ulong)(priv->reg_base + DCIF_CTRLDESC0(0)));
	} else if (format == VIDEO_BPP16) {
		reg = readl((ulong)(priv->reg_base + DCIF_CTRLDESC0(0)));
		reg &= ~(DCIF_CTRLDESC0_FORMAT_MASK | DCIF_CTRLDESC0_YUV_FORMAT_MASK);
		reg |= DCIF_CTRLDESC0_FORMAT(CTRLDESCL0_FORMAT_RGB565);
		writel(reg, (ulong)(priv->reg_base + DCIF_CTRLDESC0(0)));
	} else {
		printf("Error source format, not support\n");
	}
	writel(0, (ulong)(priv->reg_base + DCIF_CSC_CTRL_L0));

}

static void dcif_set_mode(struct dcif_priv *priv, struct ctfb_res_modes *mode)
{
	u32 val;

	val = readl((ulong)(priv->reg_base + DCIF_DPI_CTRL));
	val &= ~DCIF_DPI_CTRL_HVSYNC_POL_MASK;

	if (!(mode->sync & FB_SYNC_VERT_HIGH_ACT))
		val |= DCIF_DPI_CTRL_VSYNC_POL_LOW;
	if (!(mode->sync & FB_SYNC_HOR_HIGH_ACT))
		val |= DCIF_DPI_CTRL_HSYNC_POL_LOW;

	writel(val, (ulong)(priv->reg_base + DCIF_DPI_CTRL));

	/* config display timings */
	val = DCIF_DISP_SIZE_DISP_WIDTH(mode->xres) |
	      DCIF_DISP_SIZE_DISP_HEIGHT(mode->yres);
	writel(val, (ulong)(priv->reg_base + DCIF_DISP_SIZE));

	val = DCIF_DPI_HSYN_PAR_BP_H(mode->left_margin) |
	      DCIF_DPI_HSYN_PAR_FP_H(mode->right_margin);
	writel(val, (ulong)(priv->reg_base + DCIF_DPI_HSYN_PAR));

	val = DCIF_DPI_VSYN_PAR_BP_V(mode->upper_margin) |
	      DCIF_DPI_VSYN_PAR_FP_V(mode->lower_margin);
	writel(val, (ulong)(priv->reg_base + DCIF_DPI_VSYN_PAR));

	val = DCIF_DPI_VSYN_HSYN_WIDTH_PW_V(mode->vsync_len) |
	      DCIF_DPI_VSYN_HSYN_WIDTH_PW_H(mode->hsync_len);
	writel(val, (ulong)(priv->reg_base + DCIF_DPI_VSYN_HSYN_WIDTH));

	/* Layer 0 frame size */
	val = DCIF_CTRLDESC2_HEIGHT(mode->yres) |
	      DCIF_CTRLDESC2_WIDTH(mode->xres);
	writel(val, (ulong)(priv->reg_base + DCIF_CTRLDESC2(0)));

	/* config P_SIZE, T_SIZE and pitch
	 * 1. P_SIZE and T_SIZE should never
	 *    be less than AXI bus width.
	 * 2. P_SIZE should never be less than T_SIZE.
	 */
	val = DCIF_CTRLDESC3_P_SIZE(2) | DCIF_CTRLDESC3_T_SIZE(2) |
	      DCIF_CTRLDESC3_PITCH(mode->xres * 4);
	writel(val, (ulong)(priv->reg_base + DCIF_CTRLDESC3(0)));
}

static void dcif_reset_block(struct dcif_priv *priv)
{
	u32 reg;

	reg = readl((ulong)(priv->reg_base + DCIF_DISP_CTRL));
	reg |= DCIF_DISP_CTRL_SW_RST;
	writel(reg, (ulong)(priv->reg_base + DCIF_DISP_CTRL));

	reg = readl((ulong)(priv->reg_base + DCIF_DISP_CTRL));
	reg &= ~DCIF_DISP_CTRL_SW_RST;
	writel(reg, (ulong)(priv->reg_base + DCIF_DISP_CTRL));
}

static void dcif_enable_controller(struct dcif_priv *priv)
{
	u32 reg;

	reg = readl((ulong)(priv->reg_base + DCIF_DISP_CTRL));
	reg |= DCIF_DISP_CTRL_DISP_ON;
	writel(reg, (ulong)(priv->reg_base + DCIF_DISP_CTRL));

	reg = readl((ulong)(priv->reg_base + DCIF_CTRLDESC0(0)));
	reg |= DCIF_CTRLDESC0_EN;
	writel(reg, (ulong)(priv->reg_base + DCIF_CTRLDESC0(0)));
}

static void dcif_disable_controller(struct dcif_priv *priv)
{
	u32 reg;
	int ret;

	reg = readl((ulong)(priv->reg_base + DCIF_CTRLDESC0(0)));
	reg &= ~DCIF_CTRLDESC0_EN;
	writel(reg, (ulong)(priv->reg_base + DCIF_CTRLDESC0(0)));

	ret = readl_poll_timeout(priv->reg_base + DCIF_CTRLDESC0(0),
			 reg, !(reg & DCIF_CTRLDESC0_EN),
			 36000);	/* Wait ~2 frame times max */
	if (ret)
		printf("Failed to disable controller!\n");

	reg = readl((ulong)(priv->reg_base + DCIF_DISP_CTRL));
	reg &= ~DCIF_DISP_CTRL_DISP_ON;
	writel(reg, (ulong)(priv->reg_base + DCIF_DISP_CTRL));
}

static int dcif_set_qos(struct dcif_priv *priv)
{
	int ret;

	ret = regmap_update_bits(priv->regmap, QOS_SETTING,
				 DISPLAY_PANIC_QOS_MASK | DISPLAY_ARQOS_MASK,
				 DISPLAY_PANIC_QOS(0x3) | DISPLAY_ARQOS(0x3));
	if (ret < 0)
		printf("failed to set QoS: %d\n", ret);
	return ret;
}

static void dcif_init(struct udevice *dev,
			struct ctfb_res_modes *mode, unsigned int format)
{
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);
	struct dcif_priv *priv = dev_get_priv(dev);

	dcif_check_chip_info(priv);

	dcif_reset_block(priv);

	dcif_set_formats(priv, format);

	dcif_set_mode(priv, mode);

	/* Set fb address to primary layer */
	writel(plat->base, (ulong)(priv->reg_base + DCIF_CTRLDESC4(0)));

	clk_prepare_enable(&priv->clk_ldb_vco);
	clk_prepare_enable(&priv->clk_ldb);

	/* enable plane FIFO panic */
	dcif_enable_plane_panic(priv);

	dcif_enable_controller(priv);

	dcif_shadow_load_enable(priv);

}

static int dcif_video_probe(struct udevice *dev)
{
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	struct dcif_priv *priv = dev_get_priv(dev);

	struct ctfb_res_modes mode;
	struct display_timing timings;

	u32 fb_start, fb_end;
	int ret;

	debug("%s() plat: base 0x%lx, size 0x%x\n",
	       __func__, plat->base, plat->size);

	priv->reg_base = dev_read_addr(dev);
	if (priv->reg_base == FDT_ADDR_T_NONE) {
		dev_err(dev, "lcdif base address is not found\n");
		return -EINVAL;
	}

	priv->regmap = syscon_regmap_lookup_by_phandle(dev, "nxp,blk-ctrl");
	if (IS_ERR(priv->regmap)) {
		dev_err(dev, "failed to get blk-ctrl regmap\n");
		return IS_ERR(priv->regmap);
	}

	ret = clk_get_by_name(dev, "pix", &priv->clk_pix);
	if (ret) {
		dev_err(dev, "failed to get pix clock\n");
		return ret;
	}

	ret = clk_get_by_name(dev, "apb", &priv->clk_apb);
	if (ret) {
		dev_err(dev, "failed to get apb clock\n");
		return ret;
	}

	ret = clk_get_by_name(dev, "axi", &priv->clk_axi);
	if (ret) {
		dev_err(dev, "failed to get axi clock\n");
		return ret;
	}

	ret = clk_get_by_name(dev, "ldb", &priv->clk_ldb);
	if (ret) {
		dev_err(dev, "failed to get ldb clock\n");
		return ret;
	}

	ret = clk_get_by_name(dev, "ldb_vco", &priv->clk_ldb_vco);
	if (ret) {
		dev_err(dev, "failed to get ldb_vco clock\n");
		return ret;
	}

	clk_prepare_enable(&priv->clk_axi);
	clk_prepare_enable(&priv->clk_apb);
	clk_prepare_enable(&priv->clk_pix);

	ret = dcif_set_qos(priv);
	if (ret) {
		clk_disable_unprepare(&priv->clk_pix);
		clk_disable_unprepare(&priv->clk_apb);
		clk_disable_unprepare(&priv->clk_axi);
		return ret;
	}

	ret = dcif_of_get_timings(dev, &timings);
	if (ret)
		return ret;

	if (priv->disp_dev) {
#if IS_ENABLED(CONFIG_VIDEO_BRIDGE)
		if (device_get_uclass_id(priv->disp_dev) == UCLASS_VIDEO_BRIDGE) {
			ret = video_bridge_attach(priv->disp_dev);
			if (ret) {
				dev_err(dev, "fail to attach bridge\n");
				return ret;
			}

			ret = video_bridge_check_timing(priv->disp_dev, &timings);
			if (ret) {
				dev_err(dev, "fail to check timing\n");
				return ret;
			}

			ret = video_bridge_set_backlight(priv->disp_dev, 80);
			if (ret) {
				dev_err(dev, "fail to set backlight\n");
				return ret;
			}
		}
#endif
#if IS_ENABLED(CONFIG_DISPLAY)
		if (device_get_uclass_id(priv->disp_dev) == UCLASS_DISPLAY) {
			ret = display_enable(priv->disp_dev, 24, NULL);
			if (ret) {
				dev_err(dev, "fail to enable display\n");
				return ret;
			}
		}
#endif

	}

	mode.xres = timings.hactive.typ;
	mode.yres = timings.vactive.typ;
	mode.left_margin = timings.hback_porch.typ;
	mode.right_margin = timings.hfront_porch.typ;
	mode.upper_margin = timings.vback_porch.typ;
	mode.lower_margin = timings.vfront_porch.typ;
	mode.hsync_len = timings.hsync_len.typ;
	mode.vsync_len = timings.vsync_len.typ;
	mode.pixclock = HZ2PS(timings.pixelclock.typ);
	mode.sync = FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT;

	if (timings.flags & DISPLAY_FLAGS_HSYNC_LOW )
		mode.sync &= ~FB_SYNC_HOR_HIGH_ACT;

	if (timings.flags & DISPLAY_FLAGS_VSYNC_LOW )
		mode.sync &= ~FB_SYNC_VERT_HIGH_ACT;

	dcif_init(dev, &mode, VIDEO_BPP32);

	uc_priv->bpix = VIDEO_BPP32; /* only support 32 BPP now */
	uc_priv->xsize = mode.xres;
	uc_priv->ysize = mode.yres;

	/* Enable dcache for the frame buffer */
	fb_start = plat->base & ~(MMU_SECTION_SIZE - 1);
	fb_end = plat->base + plat->size;
	fb_end = ALIGN(fb_end, 1 << MMU_SECTION_SHIFT);

	mmu_set_region_dcache_behaviour(fb_start, fb_end - fb_start,
					DCACHE_WRITEBACK);
	video_set_flush_dcache(dev, true);
	gd->fb_base = plat->base;

	return 0;
}

static int dcif_video_remove(struct udevice *dev)
{
	struct dcif_priv *priv = dev_get_priv(dev);

	dcif_disable_controller(priv);
	clk_disable_unprepare(&priv->clk_ldb);
        clk_disable_unprepare(&priv->clk_ldb_vco);

	return 0;
}

static const struct udevice_id dcif_video_ids[] = {
	{ .compatible = "nxp,imx943-dcif" },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(dcif_video) = {
	.name	= "dcif_video",
	.id	= UCLASS_VIDEO,
	.of_match = dcif_video_ids,
	.bind	= dcif_video_bind,
	.probe	= dcif_video_probe,
	.remove = dcif_video_remove,
	.flags	= DM_FLAG_PRE_RELOC | DM_FLAG_OS_PREPARE,
	.priv_auto   = sizeof(struct dcif_priv),
};
