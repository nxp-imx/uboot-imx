// SPDX-License-Identifier: GPL-2.0+
/*
 * Freescale i.MX23/i.MX28 LCDIF driver
 *
 * Copyright (C) 2011-2013 Marek Vasut <marex@denx.de>
 * Copyright (C) 2014-2016 Freescale Semiconductor, Inc.
 *
 */
#include <common.h>
#include <dm.h>
#include <env.h>
#include <dm/device_compat.h>
#include <linux/errno.h>
#include <malloc.h>
#include <video.h>
#include <video_fb.h>
#if CONFIG_IS_ENABLED(CLK) && IS_ENABLED(CONFIG_IMX8)
#include <clk.h>
#else
#include <asm/arch/clock.h>
#endif
#include <asm/arch/imx-regs.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/dma.h>
#include <asm/io.h>
#include <reset.h>
#include <panel.h>
#include <video_bridge.h>
#include <video_link.h>

#include "videomodes.h"
#include <linux/string.h>
#include <linux/list.h>
#include <linux/fb.h>
#include <mxsfb.h>
#include <dm/device-internal.h>

#ifdef CONFIG_VIDEO_GIS
#include <gis.h>
#endif

#define	PS2KHZ(ps)	(1000000000UL / (ps))
#define HZ2PS(hz)	(1000000000UL / ((hz) / 1000))

#define BITS_PP		18
#define BYTES_PP	4

struct mxs_dma_desc desc;

/**
 * mxsfb_system_setup() - Fine-tune LCDIF configuration
 *
 * This function is used to adjust the LCDIF configuration. This is usually
 * needed when driving the controller in System-Mode to operate an 8080 or
 * 6800 connected SmartLCD.
 */
__weak void mxsfb_system_setup(void)
{
}

/*
 * ARIES M28EVK:
 * setenv videomode
 * video=ctfb:x:800,y:480,depth:18,mode:0,pclk:30066,
 *       le:0,ri:256,up:0,lo:45,hs:1,vs:1,sync:100663296,vmode:0
 *
 * Freescale mx23evk/mx28evk with a Seiko 4.3'' WVGA panel:
 * setenv videomode
 * video=ctfb:x:800,y:480,depth:24,mode:0,pclk:29851,
 * 	 le:89,ri:164,up:23,lo:10,hs:10,vs:10,sync:0,vmode:0
 */

static void mxs_lcd_init(phys_addr_t reg_base, u32 fb_addr, struct ctfb_res_modes *mode, int bpp, bool bridge, bool enable_pol)
{
	struct mxs_lcdif_regs *regs = (struct mxs_lcdif_regs *)(reg_base);
	uint32_t word_len = 0, bus_width = 0;
	uint8_t valid_data = 0;

	/* Kick in the LCDIF clock */
#if !(CONFIG_IS_ENABLED(CLK) && IS_ENABLED(CONFIG_IMX8))
	mxs_set_lcdclk((u32)reg_base, PS2KHZ(mode->pixclock));
#endif
	/* Restart the LCDIF block */
	mxs_reset_block(&regs->hw_lcdif_ctrl_reg);

	switch (bpp) {
	case 24:
		word_len = LCDIF_CTRL_WORD_LENGTH_24BIT;
		bus_width = LCDIF_CTRL_LCD_DATABUS_WIDTH_24BIT;
		valid_data = 0x7;
		break;
	case 18:
		word_len = LCDIF_CTRL_WORD_LENGTH_24BIT;
		bus_width = LCDIF_CTRL_LCD_DATABUS_WIDTH_18BIT;
		valid_data = 0x7;
		break;
	case 16:
		word_len = LCDIF_CTRL_WORD_LENGTH_16BIT;
		bus_width = LCDIF_CTRL_LCD_DATABUS_WIDTH_16BIT;
		valid_data = 0xf;
		break;
	case 8:
		word_len = LCDIF_CTRL_WORD_LENGTH_8BIT;
		bus_width = LCDIF_CTRL_LCD_DATABUS_WIDTH_8BIT;
		valid_data = 0xf;
		break;
	}

	writel(bus_width | word_len | LCDIF_CTRL_DOTCLK_MODE |
		LCDIF_CTRL_BYPASS_COUNT | LCDIF_CTRL_LCDIF_MASTER,
		&regs->hw_lcdif_ctrl);

	writel(valid_data << LCDIF_CTRL1_BYTE_PACKING_FORMAT_OFFSET,
		&regs->hw_lcdif_ctrl1);

	if (bridge)
		writel(LCDIF_CTRL2_OUTSTANDING_REQS_REQ_16, &regs->hw_lcdif_ctrl2);

	mxsfb_system_setup();

	writel((mode->yres << LCDIF_TRANSFER_COUNT_V_COUNT_OFFSET) | mode->xres,
		&regs->hw_lcdif_transfer_count);

	if (!enable_pol)
		writel(LCDIF_VDCTRL0_ENABLE_PRESENT |
			LCDIF_VDCTRL0_VSYNC_PERIOD_UNIT |
			LCDIF_VDCTRL0_VSYNC_PULSE_WIDTH_UNIT |
			mode->vsync_len, &regs->hw_lcdif_vdctrl0);
	else
		writel(LCDIF_VDCTRL0_ENABLE_PRESENT | LCDIF_VDCTRL0_ENABLE_POL |
			LCDIF_VDCTRL0_VSYNC_PERIOD_UNIT |
			LCDIF_VDCTRL0_VSYNC_PULSE_WIDTH_UNIT |
			mode->vsync_len, &regs->hw_lcdif_vdctrl0);

	writel(mode->upper_margin + mode->lower_margin +
		mode->vsync_len + mode->yres,
		&regs->hw_lcdif_vdctrl1);
	writel((mode->hsync_len << LCDIF_VDCTRL2_HSYNC_PULSE_WIDTH_OFFSET) |
		(mode->left_margin + mode->right_margin +
		mode->hsync_len + mode->xres),
		&regs->hw_lcdif_vdctrl2);
	writel(((mode->left_margin + mode->hsync_len) <<
		LCDIF_VDCTRL3_HORIZONTAL_WAIT_CNT_OFFSET) |
		(mode->upper_margin + mode->vsync_len),
		&regs->hw_lcdif_vdctrl3);
	writel((0 << LCDIF_VDCTRL4_DOTCLK_DLY_SEL_OFFSET) | mode->xres,
		&regs->hw_lcdif_vdctrl4);

	writel(fb_addr, &regs->hw_lcdif_cur_buf);
	writel(fb_addr, &regs->hw_lcdif_next_buf);

	/* Flush FIFO first */
	writel(LCDIF_CTRL1_FIFO_CLEAR, &regs->hw_lcdif_ctrl1_set);

#ifndef CONFIG_VIDEO_MXS_MODE_SYSTEM
	/* Sync signals ON */
	setbits_le32(&regs->hw_lcdif_vdctrl4, LCDIF_VDCTRL4_SYNC_SIGNALS_ON);
#endif

	/* FIFO cleared */
	writel(LCDIF_CTRL1_FIFO_CLEAR, &regs->hw_lcdif_ctrl1_clr);

	/* RUN! */
	writel(LCDIF_CTRL_RUN, &regs->hw_lcdif_ctrl_set);
}

static int mxs_probe_common(phys_addr_t reg_base, struct ctfb_res_modes *mode, int bpp, u32 fb, bool bridge, bool enable_pol)
{
	/* Start framebuffer */
	mxs_lcd_init(reg_base, fb, mode, bpp, bridge, enable_pol);

#ifdef CONFIG_VIDEO_MXS_MODE_SYSTEM
	/*
	 * If the LCD runs in system mode, the LCD refresh has to be triggered
	 * manually by setting the RUN bit in HW_LCDIF_CTRL register. To avoid
	 * having to set this bit manually after every single change in the
	 * framebuffer memory, we set up specially crafted circular DMA, which
	 * sets the RUN bit, then waits until it gets cleared and repeats this
	 * infinitelly. This way, we get smooth continuous updates of the LCD.
	 */
	struct mxs_lcdif_regs *regs = (struct mxs_lcdif_regs *)reg_base;

	memset(&desc, 0, sizeof(struct mxs_dma_desc));
	desc.address = (dma_addr_t)&desc;
	desc.cmd.data = MXS_DMA_DESC_COMMAND_NO_DMAXFER | MXS_DMA_DESC_CHAIN |
			MXS_DMA_DESC_WAIT4END |
			(1 << MXS_DMA_DESC_PIO_WORDS_OFFSET);
	desc.cmd.pio_words[0] = readl(&regs->hw_lcdif_ctrl) | LCDIF_CTRL_RUN;
	desc.cmd.next = (uint32_t)&desc.cmd;

	/* Execute the DMA chain. */
	mxs_dma_circ_start(MXS_DMA_CHANNEL_AHB_APBH_LCDIF, &desc);
#endif

	return 0;
}

static int mxs_remove_common(phys_addr_t reg_base, u32 fb)
{
	struct mxs_lcdif_regs *regs = (struct mxs_lcdif_regs *)(reg_base);
	int timeout = 1000000;

#ifdef CONFIG_MX6
	if (check_module_fused(MX6_MODULE_LCDIF))
		return -ENODEV;
#endif

	if (!fb)
		return -EINVAL;

	writel(fb, &regs->hw_lcdif_cur_buf_reg);
	writel(fb, &regs->hw_lcdif_next_buf_reg);
	writel(LCDIF_CTRL1_VSYNC_EDGE_IRQ, &regs->hw_lcdif_ctrl1_clr);
	while (--timeout) {
		if (readl(&regs->hw_lcdif_ctrl1_reg) &
		    LCDIF_CTRL1_VSYNC_EDGE_IRQ)
			break;
		udelay(1);
	}
	mxs_reset_block((struct mxs_register_32 *)&regs->hw_lcdif_ctrl_reg);

	return 0;
}

#ifndef CONFIG_DM_VIDEO

static GraphicDevice panel;
static int setup;
static struct fb_videomode fbmode;
static int depth;

int mxs_lcd_panel_setup(struct fb_videomode mode, int bpp,
	uint32_t base_addr)
{
	fbmode = mode;
	depth  = bpp;
	panel.isaBase  = base_addr;

	setup = 1;

	return 0;
}

void mxs_lcd_get_panel(struct display_panel *dispanel)
{
	dispanel->width = fbmode.xres;
	dispanel->height = fbmode.yres;
	dispanel->reg_base = panel.isaBase;
	dispanel->gdfindex = panel.gdfIndex;
	dispanel->gdfbytespp = panel.gdfBytesPP;
}

void lcdif_power_down(void)
{
	mxs_remove_common(panel.isaBase, panel.frameAdrs);
}

void *video_hw_init(void)
{
	int bpp = -1;
	int ret = 0;
	char *penv;
	void *fb = NULL;
	struct ctfb_res_modes mode;

	puts("Video: ");

	if (!setup) {

		/* Suck display configuration from "videomode" variable */
		penv = env_get("videomode");
		if (!penv) {
			printf("MXSFB: 'videomode' variable not set!\n");
			return NULL;
		}

		bpp = video_get_params(&mode, penv);
		panel.isaBase  = MXS_LCDIF_BASE;
	} else {
		mode.xres = fbmode.xres;
		mode.yres = fbmode.yres;
		mode.pixclock = fbmode.pixclock;
		mode.left_margin = fbmode.left_margin;
		mode.right_margin = fbmode.right_margin;
		mode.upper_margin = fbmode.upper_margin;
		mode.lower_margin = fbmode.lower_margin;
		mode.hsync_len = fbmode.hsync_len;
		mode.vsync_len = fbmode.vsync_len;
		mode.sync = fbmode.sync;
		mode.vmode = fbmode.vmode;
		bpp = depth;
	}

#ifdef CONFIG_MX6
	if (check_module_fused(MX6_MODULE_LCDIF)) {
		printf("LCDIF@0x%x is fused, disable it\n", MXS_LCDIF_BASE);
		return NULL;
	}
#endif
	/* fill in Graphic device struct */
	sprintf(panel.modeIdent, "%dx%dx%d", mode.xres, mode.yres, bpp);


	panel.winSizeX = mode.xres;
	panel.winSizeY = mode.yres;
	panel.plnSizeX = mode.xres;
	panel.plnSizeY = mode.yres;

	switch (bpp) {
	case 24:
	case 18:
		panel.gdfBytesPP = 4;
		panel.gdfIndex = GDF_32BIT_X888RGB;
		break;
	case 16:
		panel.gdfBytesPP = 2;
		panel.gdfIndex = GDF_16BIT_565RGB;
		break;
	case 8:
		panel.gdfBytesPP = 1;
		panel.gdfIndex = GDF__8BIT_INDEX;
		break;
	default:
		printf("MXSFB: Invalid BPP specified! (bpp = %i)\n", bpp);
		return NULL;
	}

	panel.memSize = mode.xres * mode.yres * panel.gdfBytesPP;


	/* Allocate framebuffer */
	fb = memalign(ARCH_DMA_MINALIGN,
		      roundup(panel.memSize, ARCH_DMA_MINALIGN));
	if (!fb) {
		printf("MXSFB: Error allocating framebuffer!\n");
		return NULL;
	}

	/* Wipe framebuffer */
	memset(fb, 0, panel.memSize);

	panel.frameAdrs = (u32)fb;

	printf("%s\n", panel.modeIdent);

	ret = mxs_probe_common(panel.isaBase, &mode, bpp, (u32)fb, false, true);
	if (ret)
		goto dealloc_fb;

#ifdef CONFIG_VIDEO_GIS
	/* Entry for GIS */
	mxc_enable_gis();
#endif

	return (void *)&panel;

dealloc_fb:
	free(fb);

	return NULL;
}
#else /* ifndef CONFIG_DM_VIDEO */

struct mxsfb_priv {
	fdt_addr_t reg_base;
	struct udevice *disp_dev;

#if IS_ENABLED(CONFIG_DM_RESET)
	struct reset_ctl_bulk soft_resetn;
	struct reset_ctl_bulk clk_enable;
#endif

#if CONFIG_IS_ENABLED(CLK) && IS_ENABLED(CONFIG_IMX8)
	struct clk			lcdif_pix;
	struct clk			lcdif_disp_axi;
	struct clk			lcdif_axi;
#endif
};

#if IS_ENABLED(CONFIG_DM_RESET)
static int lcdif_rstc_reset(struct reset_ctl_bulk *rstc, bool assert)
{
	int ret;

	if (!rstc)
		return 0;

	ret = assert ? reset_assert_bulk(rstc)	:
		       reset_deassert_bulk(rstc);

	return ret;
}

static int lcdif_of_parse_resets(struct udevice *dev)
{
	int ret;
	ofnode parent, child;
	struct ofnode_phandle_args args;
	struct reset_ctl_bulk rstc;
	const char *compat;
	uint32_t rstc_num = 0;

	struct mxsfb_priv *priv = dev_get_priv(dev);

	ret = dev_read_phandle_with_args(dev, "resets", "#reset-cells", 0,
					 0, &args);
	if (ret)
		return ret;

	parent = args.node;
	ofnode_for_each_subnode(child, parent) {
		compat = ofnode_get_property(child, "compatible", NULL);
		if (!compat)
			continue;

		ret = reset_get_bulk_nodev(child, &rstc);
		if (ret)
			continue;

		if (!of_compat_cmp("lcdif,soft-resetn", compat, 0)) {
			priv->soft_resetn = rstc;
			rstc_num++;
		} else if (!of_compat_cmp("lcdif,clk-enable", compat, 0)) {
			priv->clk_enable = rstc;
			rstc_num++;
		}
		else
			dev_warn(dev, "invalid lcdif reset node: %s\n", compat);
	}

	if (!rstc_num) {
		dev_err(dev, "no invalid reset control exists\n");
		return -EINVAL;
	}

	return 0;
}
#endif

static int mxs_of_get_timings(struct udevice *dev,
			      struct display_timing *timings,
			      u32 *bpp)
{
	int ret = 0;
	u32 display_phandle;
	ofnode display_node;
	struct mxsfb_priv *priv = dev_get_priv(dev);

	ret = ofnode_read_u32(dev_ofnode(dev), "display", &display_phandle);
	if (ret) {
		dev_err(dev, "required display property isn't provided\n");
		return -EINVAL;
	}

	display_node = ofnode_get_by_phandle(display_phandle);
	if (!ofnode_valid(display_node)) {
		dev_err(dev, "failed to find display subnode\n");
		return -EINVAL;
	}

	ret = ofnode_read_u32(display_node, "bits-per-pixel", bpp);
	if (ret) {
		dev_err(dev,
			"required bits-per-pixel property isn't provided\n");
		return -EINVAL;
	}

	priv->disp_dev = video_link_get_next_device(dev);
	if (priv->disp_dev) {
		ret = video_link_get_display_timings(timings);
		if (ret) {
			dev_err(dev, "failed to get any video link display timings\n");
			return -EINVAL;
		}
	} else {
		ret = ofnode_decode_display_timing(display_node, 0, timings);
		if (ret) {
			dev_err(dev, "failed to get any display timings\n");
			return -EINVAL;
		}
	}

	return ret;
}

static int mxs_video_probe(struct udevice *dev)
{
	struct video_uc_platdata *plat = dev_get_uclass_platdata(dev);
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	struct mxsfb_priv *priv = dev_get_priv(dev);

	struct ctfb_res_modes mode;
	struct display_timing timings;
	u32 bpp = 0;
	u32 fb_start, fb_end;
	int ret;
	bool enable_pol = true, enable_bridge = false;

	debug("%s() plat: base 0x%lx, size 0x%x\n",
	       __func__, plat->base, plat->size);

	priv->reg_base = dev_read_addr(dev);
	if (priv->reg_base == FDT_ADDR_T_NONE) {
		dev_err(dev, "lcdif base address is not found\n");
		return -EINVAL;
	}

	ret = mxs_of_get_timings(dev, &timings, &bpp);
	if (ret)
		return ret;

#if CONFIG_IS_ENABLED(CLK) && IS_ENABLED(CONFIG_IMX8)
	ret = clk_get_by_name(dev, "pix", &priv->lcdif_pix);
	if (ret) {
		printf("Failed to get pix clk\n");
		return ret;
	}

	ret = clk_get_by_name(dev, "disp_axi", &priv->lcdif_disp_axi);
	if (ret) {
		printf("Failed to get disp_axi clk\n");
		return ret;
	}

	ret = clk_get_by_name(dev, "axi", &priv->lcdif_axi);
	if (ret) {
		printf("Failed to get axi clk\n");
		return ret;
	}

	ret = clk_enable(&priv->lcdif_axi);
	if (ret) {
		printf("unable to enable lcdif_axi clock\n");
		return ret;
	}

	ret = clk_enable(&priv->lcdif_disp_axi);
	if (ret) {
		printf("unable to enable lcdif_disp_axi clock\n");
		return ret;
	}
#endif

#if IS_ENABLED(CONFIG_DM_RESET)
	ret = lcdif_of_parse_resets(dev);
	if (!ret) {
		ret = lcdif_rstc_reset(&priv->soft_resetn, false);
		if (ret) {
			dev_err(dev, "deassert soft_resetn failed\n");
			return ret;
		}

		ret = lcdif_rstc_reset(&priv->clk_enable, true);
		if (ret) {
			dev_err(dev, "assert clk_enable failed\n");
			return ret;
		}
	}
#endif

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

			enable_bridge = true;

			/* sec dsim needs enable ploarity at low, default we set to high */
			if (dev_read_bool(dev, "enable_polarity_low"))
				enable_pol = false;

		}
#endif

		if (device_get_uclass_id(priv->disp_dev) == UCLASS_PANEL) {
			ret = panel_enable_backlight(priv->disp_dev);
			if (ret) {
				dev_err(dev, "panel %s enable backlight error %d\n",
					priv->disp_dev->name, ret);
				return ret;
			}
		}
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

#if CONFIG_IS_ENABLED(CLK) && IS_ENABLED(CONFIG_IMX8)
	ret = clk_set_rate(&priv->lcdif_pix, timings.pixelclock.typ);
	if (ret < 0) {
		printf("Failed to set pix clk rate\n");
		return ret;
	}

	ret = clk_enable(&priv->lcdif_pix);
	if (ret) {
		printf("unable to enable lcdif_pix clock\n");
		return ret;
	}
#endif

	ret = mxs_probe_common(priv->reg_base, &mode, bpp, plat->base, enable_bridge, enable_pol);
	if (ret)
		return ret;

	switch (bpp) {
	case 32:
	case 24:
	case 18:
		uc_priv->bpix = VIDEO_BPP32;
		break;
	case 16:
		uc_priv->bpix = VIDEO_BPP16;
		break;
	case 8:
		uc_priv->bpix = VIDEO_BPP8;
		break;
	default:
		dev_err(dev, "invalid bpp specified (bpp = %i)\n", bpp);
		return -EINVAL;
	}

	uc_priv->xsize = mode.xres;
	uc_priv->ysize = mode.yres;

	/* Enable dcache for the frame buffer */
	fb_start = plat->base;
	fb_end = plat->base + plat->size;

	mmu_set_region_dcache_behaviour(fb_start, fb_end - fb_start,
					DCACHE_WRITEBACK);
	video_set_flush_dcache(dev, true);
	gd->fb_base = plat->base;

	return ret;
}

static int mxs_video_bind(struct udevice *dev)
{
	struct video_uc_platdata *plat = dev_get_uclass_platdata(dev);

	/* Max size supported by LCDIF, because in bind, we can't probe panel */
	plat->size = ALIGN(1920 * 1080 *4 * 2, MMU_SECTION_SIZE);
	plat->align = MMU_SECTION_SIZE;

	return 0;
}

static int mxs_video_remove(struct udevice *dev)
{
	struct video_uc_platdata *plat = dev_get_uclass_platdata(dev);
	struct mxsfb_priv *priv = dev_get_priv(dev);

	debug("%s\n", __func__);

	if (priv->disp_dev)
		device_remove(priv->disp_dev, DM_REMOVE_NORMAL);

	mxs_remove_common(priv->reg_base, plat->base);

	return 0;
}

static const struct udevice_id mxs_video_ids[] = {
	{ .compatible = "fsl,imx23-lcdif" },
	{ .compatible = "fsl,imx28-lcdif" },
	{ .compatible = "fsl,imx7ulp-lcdif" },
	{ .compatible = "fsl,imx8mm-lcdif" },
	{ .compatible = "fsl,imx8mn-lcdif" },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(mxs_video) = {
	.name	= "mxs_video",
	.id	= UCLASS_VIDEO,
	.of_match = mxs_video_ids,
	.bind	= mxs_video_bind,
	.probe	= mxs_video_probe,
	.remove = mxs_video_remove,
	.flags	= DM_FLAG_PRE_RELOC | DM_FLAG_OS_PREPARE,
	.priv_auto_alloc_size   = sizeof(struct mxsfb_priv),
};
#endif /* ifndef CONFIG_DM_VIDEO */
