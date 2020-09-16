/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2014-2016 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 */

#include <common.h>
#include <malloc.h>
#include <video_fb.h>

#include <asm/arch/imx-regs.h>
#include <asm/arch/sys_proto.h>
#include <linux/errno.h>
#include <asm/io.h>

#include <linux/string.h>
#include <linux/list.h>
#include <linux/fb.h>
#include <gis.h>
#include <mxsfb.h>

#include "mxc_gis.h"
#include "mxc_csi.h"
#include "mxc_pxp.h"
#include "mxc_vadc.h"

#define CHANNEL_OFFSET 36
#define COMMAND_OFFSET 8
#define REG_OFFSET 4
#define COMMAND_OPCODE_SHIFT 8

enum {
	CMD_SET_ACC = 0,
	CMD_WR_DATA,
	CMD_WR_ACC,
	CMD_WR_ALU,
	CMD_MOV_ACC,
	CMD_RD_DATA,
	CMD_RD_ALU,
	CMD_WR_FB_CSI,
	CMD_WR_FB_PXP_IN,
	CMD_WR_FB_PXP_OUT,
	CMD_WR_FB_LCDIF,
};

enum {
	ALU_AND = 0,
	ALU_OR,
	ALU_XOR,
	ALU_ADD,
	ALU_SUB,
};

enum {
	CH_MAPPING_CSI_ISR = 0,
	CH_MAPPING_CSI_FB_UPDATE,
	CH_MAPPING_PXP_ISR,
	CH_MAPPING_LCDIF_FB_UPDATE,
	CH_MAPPING_PXP_KICK,
	CH_MAPPING_CHANNEL_UNUSED = 0xf,
};

enum {
	LCDIF1_SEL = 0x10,
	LCDIF0_SEL = 0x8,
	PXP_SEL    = 0x4,
	CSI1_SEL   = 0x2,
	CSI0_SEL   = 0x1,
};

struct command_opcode {
	unsigned opcode:4;
	unsigned alu:3;
	unsigned acc_neg:1;
};

struct command_param {
	union {
		struct command_opcode cmd_bits;
		u8  cmd_opc;
	};
	u32 addr;
	u32 data;
};

struct channel_param {
	u32 ch_num;
	u32 ch_map;
	u32 cmd_num;
	struct command_param cmd_data[4];
};

static void *csibuf0, *csibuf1, *fb0, *fb1;
static struct mxs_gis_regs *gis_regs;
static struct mxs_pxp_regs *pxp_regs;
static struct mxs_csi_regs *csi_regs;
static struct mxs_lcdif_regs *lcdif_regs;
static u32 lcdif_sel;
static bool gis_running;

static void config_channel(struct channel_param *ch)
{
	u32 val, i;
	u32 reg_offset;

	if (ch->cmd_num > 3 || ch->ch_num > 5) {
		printf("Error val cmd_num=%d, ch_num=%d\n , \n", ch->cmd_num, ch->ch_num);
		return;
	}

	/* Config channel map and command */
	switch (ch->ch_num) {
	case 0:
		val = readl(&gis_regs->hw_gis_config0);
		val &= ~(GIS_CONFIG0_CH0_MAPPING_MASK | GIS_CONFIG0_CH0_NUM_MASK);
		val |= ch->ch_map << GIS_CONFIG0_CH0_MAPPING_SHIFT;
		val |= ch->cmd_num << GIS_CONFIG0_CH0_NUM_SHIFT;
		writel(val, &gis_regs->hw_gis_config0);
		break;
	case 1:
		val = readl(&gis_regs->hw_gis_config0);
		val &= ~(GIS_CONFIG0_CH1_MAPPING_MASK | GIS_CONFIG0_CH1_NUM_MASK);
		val |= ch->ch_map << GIS_CONFIG0_CH1_MAPPING_SHIFT;
		val |= ch->cmd_num << GIS_CONFIG0_CH1_NUM_SHIFT;
		writel(val, &gis_regs->hw_gis_config0);
		break;
	case 2:
		val = readl(&gis_regs->hw_gis_config0);
		val &= ~(GIS_CONFIG0_CH2_MAPPING_MASK | GIS_CONFIG0_CH2_NUM_MASK);
		val |= ch->ch_map << GIS_CONFIG0_CH2_MAPPING_SHIFT;
		val |= ch->cmd_num << GIS_CONFIG0_CH2_NUM_SHIFT;
		writel(val, &gis_regs->hw_gis_config0);
		break;
	case 3:
		val = readl(&gis_regs->hw_gis_config0);
		val &= ~(GIS_CONFIG0_CH3_MAPPING_MASK | GIS_CONFIG0_CH3_NUM_MASK);
		val |= ch->ch_map << GIS_CONFIG0_CH3_MAPPING_SHIFT;
		val |= ch->cmd_num << GIS_CONFIG0_CH3_NUM_SHIFT;
		writel(val, &gis_regs->hw_gis_config0);
		break;
	case 4:
		val = readl(&gis_regs->hw_gis_config1);
		val &= ~(GIS_CONFIG1_CH4_MAPPING_MASK | GIS_CONFIG1_CH4_NUM_MASK);
		val |= ch->ch_map << GIS_CONFIG1_CH4_MAPPING_SHIFT;
		val |= ch->cmd_num << GIS_CONFIG1_CH4_NUM_SHIFT;
		writel(val, &gis_regs->hw_gis_config1);
		break;
	case 5:
		val = readl(&gis_regs->hw_gis_config1);
		val &= ~(GIS_CONFIG1_CH5_MAPPING_MASK | GIS_CONFIG1_CH5_NUM_MASK);
		val |= ch->ch_map << GIS_CONFIG1_CH5_MAPPING_SHIFT;
		val |= ch->cmd_num << GIS_CONFIG1_CH5_NUM_SHIFT;
		writel(val, &gis_regs->hw_gis_config1);
		break;
	default:
		printf("Error channel num\n");
	}

	/* Config command  */
	for (i = 0; i < ch->cmd_num; i++) {
		val = readl(&gis_regs->hw_gis_ch0_ctrl + ch->ch_num * CHANNEL_OFFSET);
		val &= ~(0xFF << (COMMAND_OPCODE_SHIFT * i));
		val |= ch->cmd_data[i].cmd_opc << (COMMAND_OPCODE_SHIFT * i);
		writel(val, &gis_regs->hw_gis_ch0_ctrl + ch->ch_num * CHANNEL_OFFSET);

		reg_offset = ch->ch_num * CHANNEL_OFFSET + i * COMMAND_OFFSET;
		writel(ch->cmd_data[i].addr, &gis_regs->hw_gis_ch0_addr0 + reg_offset);
		writel(ch->cmd_data[i].data, &gis_regs->hw_gis_ch0_data0 + reg_offset);
	}
}

static void gis_channel_init(void)
{
	struct channel_param ch;
	int ret;
	u32 addr0, data0, addr1, data1;
	u32 val;

	/* Restart the GIS block */
	ret = mxs_reset_block(&gis_regs->hw_gis_ctrl_reg);
	if (ret) {
		debug("MXS GIS: Block reset timeout\n");
		return;
	}

	writel((u32)csibuf0, &gis_regs->hw_gis_fb0);
	writel((u32)csibuf1, &gis_regs->hw_gis_fb1);
	writel((u32)fb0, &gis_regs->hw_gis_pxp_fb0);
	writel((u32)fb1, &gis_regs->hw_gis_pxp_fb1);

	/* Config channel 0 -- CSI clean interrupt  */
	addr0 = (u32)&csi_regs->csi_csisr;
	data0 = BIT_DMA_TSF_DONE_FB1 | BIT_DMA_TSF_DONE_FB2 | BIT_SOF_INT;
	ch.ch_num = 0;
	ch.ch_map = CH_MAPPING_CSI_ISR;
	ch.cmd_num = 1;
	ch.cmd_data[0].cmd_bits.opcode = CMD_WR_DATA;
	ch.cmd_data[0].cmd_bits.alu = ALU_AND;
	ch.cmd_data[0].cmd_bits.acc_neg = GIS_CH_CTRL_CMD_ACC_NO_NEGATE;
	ch.cmd_data[0].addr = CSI0_SEL << GIS_CH_ADDR_SEL_SHIFT | addr0;
	ch.cmd_data[0].data = data0;
	config_channel(&ch);

	/* Config channel 1 -- CSI set next framebuffer addr  */
	addr0 = (u32)&csi_regs->csi_csidmasa_fb1;
	data0 = (u32)&csi_regs->csi_csidmasa_fb2;
	ch.ch_num = 1;
	ch.ch_map = CH_MAPPING_CSI_FB_UPDATE;
	ch.cmd_num = 1;
	ch.cmd_data[0].cmd_bits.opcode = CMD_WR_FB_CSI;
	ch.cmd_data[0].cmd_bits.alu = ALU_AND;
	ch.cmd_data[0].cmd_bits.acc_neg = GIS_CH_CTRL_CMD_ACC_NO_NEGATE;
	ch.cmd_data[0].addr = CSI0_SEL << GIS_CH_ADDR_SEL_SHIFT | addr0;
	ch.cmd_data[0].data = data0;
	config_channel(&ch);

	/* Config channel 2 -- PXP clear interrupt and set framebuffer */
	addr0 = (u32)&pxp_regs->pxp_stat_clr;
	data0 = BM_PXP_STAT_IRQ;
	addr1 = (u32)&pxp_regs->pxp_out_buf;
	data1 = 0;
	ch.ch_num = 2;
	ch.ch_map = CH_MAPPING_PXP_ISR;
	ch.cmd_num = 2;
	ch.cmd_data[0].cmd_bits.opcode = CMD_WR_DATA;
	ch.cmd_data[0].cmd_bits.alu = ALU_AND;
	ch.cmd_data[0].cmd_bits.acc_neg = GIS_CH_CTRL_CMD_ACC_NO_NEGATE;
	ch.cmd_data[0].addr = PXP_SEL << GIS_CH_ADDR_SEL_SHIFT | addr0;
	ch.cmd_data[0].data = data0;
	ch.cmd_data[1].cmd_bits.opcode = CMD_WR_FB_PXP_OUT;
	ch.cmd_data[1].cmd_bits.alu = ALU_AND;
	ch.cmd_data[1].cmd_bits.acc_neg = GIS_CH_CTRL_CMD_ACC_NO_NEGATE;
	ch.cmd_data[1].addr = PXP_SEL << GIS_CH_ADDR_SEL_SHIFT | addr1;
	ch.cmd_data[1].data = data1;
	config_channel(&ch);

	/* Config channel 3 -- LCDIF set framebuffer to display */
	addr0 = (u32)&lcdif_regs->hw_lcdif_next_buf;
	data0 = 0;
	ch.ch_num = 3;
	ch.ch_map = CH_MAPPING_LCDIF_FB_UPDATE;
	ch.cmd_num = 1;
	ch.cmd_data[0].cmd_bits.opcode = CMD_WR_FB_LCDIF;
	ch.cmd_data[0].cmd_bits.alu = ALU_AND;
	ch.cmd_data[0].cmd_bits.acc_neg = GIS_CH_CTRL_CMD_ACC_NO_NEGATE;
	ch.cmd_data[0].addr = ((lcdif_sel == 0) ? LCDIF0_SEL : LCDIF1_SEL) << GIS_CH_ADDR_SEL_SHIFT | addr0;
	ch.cmd_data[0].data = data0;
	config_channel(&ch);

	/* Config channel 4 -- PXP kick to process next framebuffer */
	addr0 = (u32)&pxp_regs->pxp_ps_buf;
	data0 = 0;
	addr1 = (u32)&pxp_regs->pxp_ctrl;
	data1 = BM_PXP_CTRL_IRQ_ENABLE | BM_PXP_CTRL_ENABLE;
	ch.ch_num = 4;
	ch.ch_map = CH_MAPPING_PXP_KICK;
	ch.cmd_num = 2;
	ch.cmd_data[0].cmd_bits.opcode = CMD_WR_FB_PXP_IN;
	ch.cmd_data[0].cmd_bits.alu = ALU_AND;
	ch.cmd_data[0].cmd_bits.acc_neg = GIS_CH_CTRL_CMD_ACC_NO_NEGATE;
	ch.cmd_data[0].addr = PXP_SEL << GIS_CH_ADDR_SEL_SHIFT | addr0;
	ch.cmd_data[0].data = data0;
	ch.cmd_data[1].cmd_bits.opcode = CMD_WR_DATA;
	ch.cmd_data[1].cmd_bits.alu = ALU_AND;
	ch.cmd_data[1].cmd_bits.acc_neg = GIS_CH_CTRL_CMD_ACC_NO_NEGATE;
	ch.cmd_data[1].addr = PXP_SEL << GIS_CH_ADDR_SEL_SHIFT | addr1;
	ch.cmd_data[1].data = data1;
	config_channel(&ch);

	/* start gis  */
	val = readl(&gis_regs->hw_gis_ctrl);
	if (lcdif_sel == 1)
		val |= GIS_CTRL_ENABLE_SET | GIS_CTRL_LCDIF_SEL_LCDIF1;
	else
		val |= GIS_CTRL_ENABLE_SET | GIS_CTRL_LCDIF_SEL_LCDIF0;
	writel(val, &gis_regs->hw_gis_ctrl);
}

void mxc_disable_gis(void)
{
	u32 val;

	if (!gis_running)
		return;

	/* Stop gis */
	val = GIS_CTRL_SFTRST_SET | GIS_CTRL_CLK_GATE_SET;
	writel(val, &gis_regs->hw_gis_ctrl);

	/* Stop pxp */
	mxs_reset_block(&pxp_regs->pxp_ctrl_reg);
	val = BM_PXP_CTRL_SFTRST | BM_PXP_CTRL_CLKGATE;
	writel(val , &pxp_regs->pxp_ctrl);

	csi_disable();

	vadc_power_down();
}

void mxc_enable_gis(void)
{
	struct sensor_data sensor;
	struct csi_conf_param csi_conf;
	struct pxp_config_data pxp_conf;
	struct display_panel panel;
	u32 csimemsize, pxpmemsize;
	char const *gis_input = env_get("gis");

#ifdef CONFIG_MX6
	if (check_module_fused(MX6_MODULE_CSI)) {
		printf("CSI@0x%x is fused, disable it\n", CSI1_BASE_ADDR);
		return;
	}
#endif

#ifdef CONFIG_MX6
	if (check_module_fused(MX6_MODULE_PXP)) {
		printf("PXP@0x%x is fused, disable it\n", PXP_BASE_ADDR);
		return;
	}
#endif

	gis_regs = (struct mxs_gis_regs *)GIS_BASE_ADDR;
	pxp_regs = (struct mxs_pxp_regs *)PXP_BASE_ADDR;
	csi_regs = (struct mxs_csi_regs *)CSI1_BASE_ADDR;

	gis_running = false;

	if (gis_input != NULL && !strcmp(gis_input, "vadc")) {
		printf("gis input --- vadc\n");
		/* vadc_in 0 */
		vadc_config(0);

		/* Get vadc mode */
		vadc_get_std(&sensor);
	} else {
		printf("gis input --- No input\n");
		return;
	}

	/* Get display mode */
	mxs_lcd_get_panel(&panel);

	lcdif_regs = (struct mxs_lcdif_regs *)panel.reg_base;
	if (panel.reg_base == LCDIF2_BASE_ADDR)
		lcdif_sel = 1;
	else
		lcdif_sel = 0;

	/* Allocate csi buffer */
	if (sensor.pixel_fmt == FMT_YUV444) {
		csimemsize = sensor.width * sensor.height * 4;
		csi_conf.bpp = 32;
	} else {
		csimemsize = sensor.width * sensor.height * 2;
		csi_conf.bpp = 16;
	}

	pxpmemsize = panel.width * panel.height * panel.gdfbytespp;
	csibuf0 = malloc(csimemsize);
	csibuf1 = malloc(csimemsize);
	fb0 = malloc(pxpmemsize);
	fb1 = malloc(pxpmemsize);
	if (!csibuf0 || !csibuf1 || !fb0 || !fb1) {
		printf("MXSGIS: Error allocating csibuffer!\n");
		return;
	}
	/* Wipe framebuffer */
	memset(csibuf0, 0, csimemsize);
	memset(csibuf1, 0, csimemsize);
	memset(fb0, 0, pxpmemsize);
	memset(fb1, 0, pxpmemsize);

	/*config csi  */
	csi_conf.width = sensor.width;
	csi_conf.height = sensor.height;
	csi_conf.btvmode = true;
	csi_conf.std = sensor.std_id;
	csi_conf.fb0addr = csibuf0;
	csi_conf.fb1addr = csibuf1;
	csi_config(&csi_conf);

	/* config pxp */
	pxp_conf.s0_param.pixel_fmt = sensor.pixel_fmt;
	pxp_conf.s0_param.width = sensor.width;
	pxp_conf.s0_param.height = sensor.height;
	pxp_conf.s0_param.stride = sensor.width * csi_conf.bpp/8;
	pxp_conf.s0_param.paddr = csibuf0;

	switch (panel.gdfindex) {
	case GDF_32BIT_X888RGB:
		pxp_conf.out_param.pixel_fmt = FMT_RGB888;
		break;
	case GDF_16BIT_565RGB:
		pxp_conf.out_param.pixel_fmt = FMT_RGB565;
		break;
	default:
		printf("GIS unsupported format!");
	}

	pxp_conf.out_param.width = panel.width;
	pxp_conf.out_param.height = panel.height;
	pxp_conf.out_param.stride = pxp_conf.out_param.width * panel.gdfbytespp;
	pxp_conf.out_param.paddr = fb0;
	pxp_config(&pxp_conf);

	gis_running = true;

	/* Config gis */
	gis_channel_init();
}
