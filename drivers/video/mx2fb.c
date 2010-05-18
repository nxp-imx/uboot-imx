/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the MX53-EVK Freescale board.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <lcd.h>
#include <asm/arch/mx25-regs.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <asm/errno.h>
#include <mx2fb.h>

DECLARE_GLOBAL_DATA_PTR;

void *lcd_base;			/* Start of framebuffer memory	*/
void *lcd_console_address;	/* Start of console buffer	*/

int lcd_line_length;
int lcd_color_fg;
int lcd_color_bg;

short console_col;
short console_row;


void lcd_initcolregs(void)
{
}

void lcd_setcolreg(ushort regno, ushort red, ushort green, ushort blue)
{
}

void lcd_enable(void)
{
}

void lcd_disable(void)
{
}

void lcd_panel_disable(void)
{
}

void lcd_ctrl_init(void *lcdbase)
{
	u32 ccm_ipg_cg, pcr;

	/* LSSAR */
	writel(gd->fb_base, LCDC_BASE + 0x00);
	/* LSR = x << 20 + y */
	writel(((panel_info.vl_col >> 4) << 20) + panel_info.vl_row,
		LCDC_BASE + 0x04);
	/* LVPWR = x_res * 2 / 2 */
	writel(panel_info.vl_col / 2, LCDC_BASE + 0x08);
	/* LPCR =  To be fixed using Linux BSP Value for now */
	switch (panel_info.vl_bpix) {
	/* bpix = 4 (16bpp) */
	case 4:
		pcr = LCDC_LPCR | (0x5 << 25);
		break;
	default:
		pcr = LCDC_LPCR;
		break;
	}

	pcr |= (panel_info.vl_sync & FB_SYNC_CLK_LAT_FALL) ? 0x00200000 : 0;
	pcr |= (panel_info.vl_sync & FB_SYNC_DATA_INVERT) ? 0x01000000 : 0;
	pcr |= (panel_info.vl_sync & FB_SYNC_SHARP_MODE) ? 0x00000040 : 0;
	pcr |= (panel_info.vl_sync & FB_SYNC_OE_LOW_ACT) ? 0x00100000 : 0;

	pcr |= LCDC_LPCR_PCD;

	writel(pcr, LCDC_BASE + 0x18);
	/* LHCR = H Pulse width, Right and Left Margins */
	writel(((panel_info.vl_hsync - 1) << 26) + \
		((panel_info.vl_right_margin - 1) << 8) + \
		(panel_info.vl_left_margin - 3),
		LCDC_BASE + 0x1c);
	/* LVCR = V Pulse width, lower and upper margins */
	writel((panel_info.vl_vsync << 26) + \
		(panel_info.vl_lower_margin << 8) + \
		(panel_info.vl_upper_margin),
		LCDC_BASE + 0x20);
	/* LSCR */
	writel(LCDC_LSCR, LCDC_BASE + 0x28);
	/* LRMCR */
	writel(LCDC_LRMCR, LCDC_BASE + 0x34);
	/* LDCR	*/
	writel(LCDC_LDCR, LCDC_BASE + 0x30);
	/* LPCCR = PWM */
	writel(LCDC_LPCCR, LCDC_BASE + 0x2c);

	/* On and off clock gating */
	ccm_ipg_cg = readl(CCM_BASE + 0x10);

	writel(ccm_ipg_cg&0xDFFFFFFF, CCM_BASE + 0x10);
	writel(ccm_ipg_cg|0x20000000, CCM_BASE + 0x10);
}

ulong calc_fbsize(void)
{
	return panel_info.vl_row * panel_info.vl_col * 2 \
		* NBITS(panel_info.vl_bpix) / 8;
}


