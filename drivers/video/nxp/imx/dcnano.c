// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021 NXP
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

#include <linux/string.h>
#include <linux/list.h>
#include <linux/fb.h>
#include <linux/bug.h>
#include <linux/delay.h>
#include "dcnano-reg.h"
#include <log.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <dm/device_compat.h>

struct dcnano_priv {
	fdt_addr_t reg_base;
	struct udevice *disp_dev;
};

static int dcnano_check_chip_info(struct dcnano_priv *dcnano)
{
	u32 val;
	int ret = 0;

	val = readl((ulong)(dcnano->reg_base + DCNANO_DCCHIPREV));
	if (val != DCCHIPREV) {
		printf("invalid chip revision(0x%08x)\n", val);
		ret = -ENODEV;
		return ret;
	}
	debug("chip revision is 0x%08x\n", val);

	val = readl((ulong)(dcnano->reg_base + DCNANO_DCCHIPDATE));
	if (val != DCCHIPDATE) {
		printf("invalid chip date(0x%08x)\n", val);
		ret = -ENODEV;
		return ret;
	}
	debug("chip date is 0x%08x\n", val);

	val = readl((ulong)(dcnano->reg_base + DCNANO_DCCHIPPATCHREV));
	if (val != DCCHIPPATCHREV) {
		printf("invalid chip patch revision(0x%08x)\n", val);
		ret = -ENODEV;
		return ret;
	}
	debug("chip patch revision is 0x%08x\n", val);

	return ret;
}

static void dcnano_set_mode(struct dcnano_priv *priv,
			struct display_timing *timing)
{
	u32 val, htotal, vtotal;

	/* select output bus, only support DPI */
	writel(DBICFG_BUS_OUTPUT_SEL_DPI, (ulong)(priv->reg_base + DCNANO_DBICONFIG));

	/* set bus format, fixed to 24 */
	writel(DPICFG_DATA_FORMAT_D24, (ulong)(priv->reg_base + DCNANO_DPICONFIG));

	htotal = timing->hactive.typ + timing->hback_porch.typ +
		timing->hfront_porch.typ + timing->hsync_len.typ;
	vtotal = timing->vactive.typ + timing->vback_porch.typ +
		timing->vfront_porch.typ + timing->vsync_len.typ;

	/* timing: active + front porch + sync + back porch = total */

	/* horizontal timing */
	val = HDISPLAY_END(timing->hactive.typ) |
	      HDISPLAY_TOTAL(htotal);
	writel(val, (ulong)(priv->reg_base + DCNANO_HDISPLAY));

	val = HSYNC_START(htotal - timing->hback_porch.typ - timing->hsync_len.typ) |
	      HSYNC_END(htotal - timing->hback_porch.typ) | HSYNC_PULSE_ENABLE;
	if (timing->flags & DISPLAY_FLAGS_HSYNC_HIGH)
		val |= HSYNC_POL_POSITIVE;
	else
		val |= HSYNC_POL_NEGATIVE;
	writel(val, (ulong)(priv->reg_base + DCNANO_HSYNC));

	/* vertical timing */
	val = VDISPLAY_END(timing->vactive.typ) |
	      VDISPLAY_TOTAL(vtotal);
	writel(val, (ulong)(priv->reg_base + DCNANO_VDISPLAY));

	val = VSYNC_START(vtotal - timing->vback_porch.typ - timing->vsync_len.typ) |
	      VSYNC_END(vtotal - timing->vback_porch.typ) | VSYNC_PULSE_ENABLE;
	if (timing->flags & DISPLAY_FLAGS_VSYNC_HIGH)
		val |= VSYNC_POL_POSITIVE;
	else
		val |= VSYNC_POL_NEGATIVE;
	writel(val, (ulong)(priv->reg_base + DCNANO_VSYNC));

	/* panel configuration */
	val = PANELCFG_DE_ENABLE | PANELCFG_DE_POL_POSITIVE |
	      PANELCFG_DATA_ENABLE | PANELCFG_DATA_POL_POSITIVE |
	      PANELCFG_CLOCK_ENABLE | PANELCFG_CLOCK_POL_POSITIVE |
	      PANELCFG_SEQUENCING_SOFTWARE;
	writel(val, (ulong)(priv->reg_base + DCNANO_PANELCONFIG));
}

static void dcnano_disable_controller(struct dcnano_priv *priv)
{
	writel(0, (ulong)(priv->reg_base + DCNANO_FRAMEBUFFERCONFIG));
}

static void dcnano_init(struct udevice *dev,
			struct display_timing *timing, unsigned int format)
{
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);
	struct dcnano_priv *priv = dev_get_priv(dev);
	u32 primary_fb_fmt = 0, val = 0;

	/* Kick in the LCDIF clock */
	mxs_set_lcdclk(priv->reg_base, timing->pixelclock.typ / 1000);

	dcnano_check_chip_info(priv);

	dcnano_set_mode(priv, timing);

	/* Set fb address */
	writel(plat->base, (ulong)(priv->reg_base + DCNANO_FRAMEBUFFERADDRESS));

	switch (format) {
	case VIDEO_BPP16:
		/* 16 bpp */
		writel(ALIGN(timing->hactive.typ * 2, 128),
			(ulong)(priv->reg_base + DCNANO_FRAMEBUFFERSTRIDE));
		primary_fb_fmt = FBCFG_FORMAT_R5G6B5;
		break;
	case VIDEO_BPP32:
		/* 32 bpp */
		writel(ALIGN(timing->hactive.typ * 4, 128),
			(ulong)(priv->reg_base + DCNANO_FRAMEBUFFERSTRIDE));
		primary_fb_fmt = FBCFG_FORMAT_R8G8B8;
		break;
	default:
		printf("unsupported pixel format: %u\n", format);
		return;
	}

	/* Disable interrupts */
	writel(0, (ulong)(priv->reg_base + DCNANO_DISPLAYINTRENABLE));

	/* Enable controller */
	val = FBCFG_OUTPUT_ENABLE | FBCFG_RESET_ENABLE | primary_fb_fmt;
	writel(val, (ulong)(priv->reg_base + DCNANO_FRAMEBUFFERCONFIG));
}

void dcnano_power_down(struct dcnano_priv *priv)
{
	dcnano_disable_controller(priv);
}

static int dcnano_of_get_timings(struct udevice *dev,
			      struct display_timing *timings)
{
	int ret = 0;
	struct dcnano_priv *priv = dev_get_priv(dev);

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

	return ret;
}

static int dcnano_video_probe(struct udevice *dev)
{
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	struct dcnano_priv *priv = dev_get_priv(dev);

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

	ret = dcnano_of_get_timings(dev, &timings);
	if (ret)
		return ret;

	reset_lcdclk();

	if (priv->disp_dev) {
#if IS_ENABLED(CONFIG_VIDEO_BRIDGE)
		if (device_get_uclass_id(priv->disp_dev) == UCLASS_VIDEO_BRIDGE) {
			ret = video_bridge_attach(priv->disp_dev);
			if (ret) {
				dev_err(dev, "fail to attach bridge\n");
				return ret;
			}

			ret = video_bridge_set_backlight(priv->disp_dev, 80);
			if (ret) {
				dev_err(dev, "fail to set backlight\n");
				return ret;
			}
		}
#endif
	}

	dcnano_init(dev, &timings, VIDEO_BPP32);

	uc_priv->bpix = VIDEO_BPP32; /* only support 32 BPP now */
	uc_priv->xsize = timings.hactive.typ;
	uc_priv->ysize = timings.vactive.typ;

	/* Enable dcache for the frame buffer */
	fb_start = plat->base & ~(MMU_SECTION_SIZE - 1);
	fb_end = plat->base + plat->size;
	fb_end = ALIGN(fb_end, 1 << MMU_SECTION_SHIFT);
	mmu_set_region_dcache_behaviour(fb_start, fb_end - fb_start,
					DCACHE_WRITEBACK);
	video_set_flush_dcache(dev, true);
	gd->fb_base = plat->base;

	return ret;
}

static int dcnano_video_bind(struct udevice *dev)
{
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);

	/* Max size supported by LCDIF, because in bind, we can't probe panel */
	plat->size = 1920 * 1080 *4 * 2;

	return 0;
}

static int dcnano_video_remove(struct udevice *dev)
{
	struct dcnano_priv *priv = dev_get_priv(dev);

	debug("%s\n", __func__);

	if (priv->disp_dev)
		device_remove(priv->disp_dev, DM_REMOVE_NORMAL);

	dcnano_power_down(priv);

	return 0;
}

static const struct udevice_id dcnano_video_ids[] = {
	{ .compatible = "nxp,imx8ulp-dcnano" },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(dcnano_video) = {
	.name	= "dcnano_video",
	.id	= UCLASS_VIDEO,
	.of_match = dcnano_video_ids,
	.bind	= dcnano_video_bind,
	.probe	= dcnano_video_probe,
	.remove = dcnano_video_remove,
	.flags	= DM_FLAG_PRE_RELOC | DM_FLAG_OS_PREPARE,
	.priv_auto   = sizeof(struct dcnano_priv),
};
