/*
 * Freescale i.MX23/i.MX28 LCDIF driver
 *
 * Copyright (C) 2011-2013 Marek Vasut <marex@denx.de>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <malloc.h>
#include <video_fb.h>

#include <asm/arch/imx-regs.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/errno.h>
#include <asm/io.h>

#include <asm/imx-common/dma.h>

#include "videomodes.h"
#include <linux/string.h>
#include <linux/list.h>
#include <linux/fb.h>
#include <mxsfb.h>

#ifdef CONFIG_VIDEO_GIS
#include <gis.h>
#endif

#define	PS2KHZ(ps)	(1000000000UL / (ps))
#define	WAIT_FOR_VSYNC_TIMEOUT	1000000

static GraphicDevice panel;
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

/*
 * DENX M28EVK:
 * setenv videomode
 * video=ctfb:x:800,y:480,depth:18,mode:0,pclk:30066,
 *       le:0,ri:256,up:0,lo:45,hs:1,vs:1,sync:100663296,vmode:0
 *
 * Freescale mx23evk/mx28evk with a Seiko 4.3'' WVGA panel:
 * setenv videomode
 * video=ctfb:x:800,y:480,depth:24,mode:0,pclk:29851,
 * 	 le:89,ri:164,up:23,lo:10,hs:10,vs:10,sync:0,vmode:0
 */

static void mxs_lcd_init(GraphicDevice *panel,
			struct ctfb_res_modes *mode, int bpp)
{
	struct mxs_lcdif_regs *regs = (struct mxs_lcdif_regs *)(panel->isaBase);
	uint32_t word_len = 0, bus_width = 0;
	uint8_t valid_data = 0;

	/* Kick in the LCDIF clock */
	mxs_set_lcdclk(panel->isaBase, PS2KHZ(mode->pixclock));

	/* Restart the LCDIF block */
	mxs_reset_block((struct mxs_register_32 *)&regs->hw_lcdif_ctrl);

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

	mxsfb_system_setup();

	writel((mode->yres << LCDIF_TRANSFER_COUNT_V_COUNT_OFFSET) | mode->xres,
		&regs->hw_lcdif_transfer_count);

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

	writel(panel->frameAdrs, &regs->hw_lcdif_cur_buf);
	writel(panel->frameAdrs, &regs->hw_lcdif_next_buf);

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

void lcdif_power_down()
{
	struct mxs_lcdif_regs *regs = (struct mxs_lcdif_regs *)(panel.isaBase);
	int timeout = WAIT_FOR_VSYNC_TIMEOUT;

	writel(panel.frameAdrs, &regs->hw_lcdif_cur_buf);
	writel(panel.frameAdrs, &regs->hw_lcdif_next_buf);
	writel(LCDIF_CTRL1_VSYNC_EDGE_IRQ, &regs->hw_lcdif_ctrl1_clr);
	while (--timeout) {
		if (readl(&regs->hw_lcdif_ctrl1) & LCDIF_CTRL1_VSYNC_EDGE_IRQ)
			break;
		udelay(1);
	}
	mxs_reset_block((struct mxs_register_32 *)&regs->hw_lcdif_ctrl);
}

void *video_hw_init(void)
{
	int bpp = -1;
	char *penv;
	void *fb;
	struct ctfb_res_modes mode;

	puts("Video: ");

	if (!setup) {

		/* Suck display configuration from "videomode" variable */
		penv = getenv("videomode");
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

	/* fill in Graphic device struct */
	sprintf(panel.modeIdent, "%dx%dx%d",
			mode.xres, mode.yres, bpp);


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

	/* Start framebuffer */
	mxs_lcd_init(&panel, &mode, bpp);

#ifdef CONFIG_VIDEO_MXS_MODE_SYSTEM
	/*
	 * If the LCD runs in system mode, the LCD refresh has to be triggered
	 * manually by setting the RUN bit in HW_LCDIF_CTRL register. To avoid
	 * having to set this bit manually after every single change in the
	 * framebuffer memory, we set up specially crafted circular DMA, which
	 * sets the RUN bit, then waits until it gets cleared and repeats this
	 * infinitelly. This way, we get smooth continuous updates of the LCD.
	 */
	struct mxs_lcdif_regs *regs = (struct mxs_lcdif_regs *)MXS_LCDIF_BASE;

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

#ifdef CONFIG_VIDEO_GIS
	/* Entry for GIS */
	mxc_enable_gis();
#endif

	return (void *)&panel;
}
