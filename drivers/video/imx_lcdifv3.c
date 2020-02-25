// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 */

#include <common.h>
#include <malloc.h>
#include <video_fb.h>

#include <asm/arch/imx-regs.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <linux/errno.h>
#include <asm/io.h>

#include "videomodes.h"
#include <linux/string.h>
#include <linux/list.h>
#include <linux/fb.h>
#include "lcdifv3-regs.h"
#include <imx_lcdifv3.h>

#ifdef CONFIG_IMX_MIPI_DSI_BRIDGE
#include <imx_mipi_dsi_bridge.h>
#endif

#define	PS2KHZ(ps)	(1000000000UL / (ps))

static GraphicDevice panel;
static int setup;
static struct fb_videomode fbmode;
static int depth;

int lcdifv3_panel_setup(struct fb_videomode mode, int bpp,
	uint32_t base_addr)
{
	fbmode = mode;
	depth  = bpp;
	panel.isaBase  = base_addr;

	setup = 1;

	return 0;
}

void lcdifv3_get_panel(struct display_panel *dispanel)
{
	dispanel->width = fbmode.xres;
	dispanel->height = fbmode.yres;
	dispanel->reg_base = panel.isaBase;
	dispanel->gdfindex = panel.gdfIndex;
	dispanel->gdfbytespp = panel.gdfBytesPP;
}

static int lcdifv3_set_pix_fmt(GraphicDevice *panel, unsigned int format)
{
	uint32_t ctrldescl0_5 = 0;

	ctrldescl0_5 = readl((ulong)(panel->isaBase + LCDIFV3_CTRLDESCL0_5));

	WARN_ON(ctrldescl0_5 & CTRLDESCL0_5_SHADOW_LOAD_EN);

	ctrldescl0_5 &= ~(CTRLDESCL0_5_BPP(0xf) | CTRLDESCL0_5_YUV_FORMAT(0x3));

	switch (format) {
	case GDF_16BIT_565RGB:
		ctrldescl0_5 |= CTRLDESCL0_5_BPP(BPP16_RGB565);
		break;
	case GDF_32BIT_X888RGB:
		ctrldescl0_5 |= CTRLDESCL0_5_BPP(BPP32_ARGB8888);
		break;
	default:
		printf("unsupported pixel format: %u\n", format);
		return -EINVAL;
	}

	writel(ctrldescl0_5,  (ulong)(panel->isaBase + LCDIFV3_CTRLDESCL0_5));

	return 0;
}


static void lcdifv3_set_mode(GraphicDevice *panel,
			struct ctfb_res_modes *mode)
{
	u32 disp_size, hsyn_para, vsyn_para, vsyn_hsyn_width, ctrldescl0_1;

	/* config display timings */
	disp_size = DISP_SIZE_DELTA_Y(mode->yres) |
		    DISP_SIZE_DELTA_X(mode->xres);
	writel(disp_size, (ulong)(panel->isaBase + LCDIFV3_DISP_SIZE));

	hsyn_para = HSYN_PARA_BP_H(mode->left_margin) |
		    HSYN_PARA_FP_H(mode->right_margin);
	writel(hsyn_para, (ulong)(panel->isaBase + LCDIFV3_HSYN_PARA));

	vsyn_para = VSYN_PARA_BP_V(mode->upper_margin) |
		    VSYN_PARA_FP_V(mode->lower_margin);
	writel(vsyn_para, (ulong)(panel->isaBase + LCDIFV3_VSYN_PARA));

	vsyn_hsyn_width = VSYN_HSYN_WIDTH_PW_V(mode->vsync_len) |
			  VSYN_HSYN_WIDTH_PW_H(mode->hsync_len);
	writel(vsyn_hsyn_width, (ulong)(panel->isaBase + LCDIFV3_VSYN_HSYN_WIDTH));

	/* config layer size */
	/* TODO: 32bits alignment for width */
	ctrldescl0_1 = CTRLDESCL0_1_HEIGHT(mode->yres) |
		       CTRLDESCL0_1_WIDTH(mode->xres);
	writel(ctrldescl0_1, (ulong)(panel->isaBase + LCDIFV3_CTRLDESCL0_1));

	/* Polarities */
	writel(CTRL_INV_HS, (ulong)(panel->isaBase + LCDIFV3_CTRL_CLR));
	writel(CTRL_INV_VS, (ulong)(panel->isaBase + LCDIFV3_CTRL_CLR));
#ifdef CONFIG_IMX_SEC_MIPI_DSI /* SEC MIPI DSI specific */
	writel(CTRL_INV_PXCK, (ulong)(panel->isaBase + LCDIFV3_CTRL_CLR));
	writel(CTRL_INV_DE, (ulong)(panel->isaBase + LCDIFV3_CTRL_CLR));
#endif
}

static void lcdifv3_set_bus_fmt(GraphicDevice *panel)
{
	uint32_t disp_para = 0;

	disp_para = readl((ulong)(panel->isaBase + LCDIFV3_DISP_PARA));
	disp_para &= DISP_PARA_LINE_PATTERN(0xf);

	/* Fixed to 24 bits output */
	disp_para |= DISP_PARA_LINE_PATTERN(LP_RGB888_OR_YUV444);

	/* config display mode: default is normal mode */
	disp_para &= DISP_PARA_DISP_MODE(3);
	disp_para |= DISP_PARA_DISP_MODE(0);
	writel(disp_para, (ulong)(panel->isaBase + LCDIFV3_DISP_PARA));
}

static void lcdifv3_enable_controller(GraphicDevice *panel)
{
	u32 disp_para, ctrldescl0_5;

	disp_para = readl((ulong)(panel->isaBase + LCDIFV3_DISP_PARA));
	ctrldescl0_5 = readl((ulong)(panel->isaBase + LCDIFV3_CTRLDESCL0_5));

	/* disp on */
	disp_para |= DISP_PARA_DISP_ON;
	writel(disp_para, (ulong)(panel->isaBase + LCDIFV3_DISP_PARA));

	/* enable shadow load */
	ctrldescl0_5 |= CTRLDESCL0_5_SHADOW_LOAD_EN;
	writel(ctrldescl0_5, (ulong)(panel->isaBase + LCDIFV3_CTRLDESCL0_5));

	/* enable layer dma */
	ctrldescl0_5 |= CTRLDESCL0_5_EN;
	writel(ctrldescl0_5, (ulong)(panel->isaBase + LCDIFV3_CTRLDESCL0_5));
}

static void lcdifv3_disable_controller(GraphicDevice *panel)
{
	u32 disp_para, ctrldescl0_5;

	disp_para = readl((ulong)(panel->isaBase + LCDIFV3_DISP_PARA));
	ctrldescl0_5 = readl((ulong)(panel->isaBase + LCDIFV3_CTRLDESCL0_5));

	/* dma off */
	ctrldescl0_5 &= ~CTRLDESCL0_5_EN;
	writel(ctrldescl0_5, (ulong)(panel->isaBase + LCDIFV3_CTRLDESCL0_5));

	/* disp off */
	disp_para &= ~DISP_PARA_DISP_ON;
	writel(disp_para, (ulong)(panel->isaBase + LCDIFV3_DISP_PARA));
}

static void lcdifv3_init(GraphicDevice *panel,
			struct ctfb_res_modes *mode, unsigned int format)
{
	int ret;

	/* Kick in the LCDIF clock */
	mxs_set_lcdclk(panel->isaBase, PS2KHZ(mode->pixclock));

	writel(CTRL_SW_RESET, (ulong)(panel->isaBase + LCDIFV3_CTRL_CLR));

	lcdifv3_set_mode(panel, mode);

	lcdifv3_set_bus_fmt(panel);

	ret = lcdifv3_set_pix_fmt(panel, format);
	if (ret) {
		printf("Fail to init lcdifv3, wrong format %u\n", format);
		return;
	}

	/* Set fb address to primary layer */
	writel(panel->frameAdrs, (ulong)(panel->isaBase + LCDIFV3_CTRLDESCL_LOW0_4));

	writel(CTRLDESCL0_3_P_SIZE(1) |CTRLDESCL0_3_T_SIZE(1) | CTRLDESCL0_3_PITCH(mode->xres * 4),
		(ulong)(panel->isaBase + LCDIFV3_CTRLDESCL0_3));

	lcdifv3_enable_controller(panel);
}

void lcdifv3_power_down(void)
{
	int timeout = 1000000;

	if (!panel.frameAdrs)
		return;

#ifdef CONFIG_IMX_MIPI_DSI_BRIDGE
	imx_mipi_dsi_bridge_disable();
#endif

	/* Disable LCDIF during VBLANK */
	writel(INT_STATUS_D0_VS_BLANK,
		(ulong)(panel.isaBase + LCDIFV3_INT_STATUS_D0));
	while (--timeout) {
		if (readl((ulong)(panel.isaBase + LCDIFV3_INT_STATUS_D0)) &
		    INT_STATUS_D0_VS_BLANK)
			break;
		udelay(1);
	}

	lcdifv3_disable_controller(&panel);
}

void *video_hw_init(void)
{
	int bpp = -1;
	void *fb;
	struct ctfb_res_modes mode;

	puts("Video: ");

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

	/* fill in Graphic device struct */
	sprintf(panel.modeIdent, "%dx%dx%d",
			mode.xres, mode.yres, bpp);

	panel.winSizeX = mode.xres;
	panel.winSizeY = mode.yres;
	panel.plnSizeX = mode.xres;
	panel.plnSizeY = mode.yres;

	switch (bpp) {
	case 24:
		panel.gdfBytesPP = 4;
		panel.gdfIndex = GDF_32BIT_X888RGB;
		break;
	case 16:
		panel.gdfBytesPP = 2;
		panel.gdfIndex = GDF_16BIT_565RGB;
		break;
	default:
		printf("LCDIFV3: Invalid BPP specified! (bpp = %i)\n", bpp);
		return NULL;
	}

	panel.memSize = mode.xres * mode.yres * panel.gdfBytesPP;

	/* Allocate framebuffer */
	fb = memalign(ARCH_DMA_MINALIGN,
		      roundup(panel.memSize, ARCH_DMA_MINALIGN));
	if (!fb) {
		printf("LCDIFV3: Error allocating framebuffer!\n");
		return NULL;
	}

	/* Wipe framebuffer */
	memset(fb, 0, panel.memSize);

	panel.frameAdrs = (ulong)fb;

	printf("%s\n", panel.modeIdent);

#ifdef CONFIG_IMX_MIPI_DSI_BRIDGE
	int dsi_ret;
	imx_mipi_dsi_bridge_mode_set(&fbmode);
	dsi_ret = imx_mipi_dsi_bridge_enable();
	if (dsi_ret) {
		printf("Enable DSI bridge failed, err %d\n", dsi_ret);
		return NULL;
	}
#endif

	/* Start framebuffer */
	lcdifv3_init(&panel, &mode, panel.gdfIndex);

	return (void *)&panel;
}
