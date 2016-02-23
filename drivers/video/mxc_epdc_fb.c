/*
 * Copyright (C) 2010-2016 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
/*
 * Based on STMP378X LCDIF
 * Copyright 2008 Embedded Alley Solutions, Inc All Rights Reserved.
 */

#include <common.h>
#include <lcd.h>
#include <linux/list.h>
#include <linux/err.h>
#include <linux/types.h>
#include <malloc.h>

#include <mxc_epdc_fb.h>

DECLARE_GLOBAL_DATA_PTR;

void *lcd_base;			/* Start of framebuffer memory	*/
void *lcd_console_address;	/* Start of console buffer	*/

int lcd_color_fg;
int lcd_color_bg;

short console_col;
short console_row;

int rev;

void lcd_initcolregs(void)
{
}

void lcd_setcolreg(ushort regno, ushort red, ushort green, ushort blue)
{
}

#define TEMP_USE_DEFAULT 8

#define UPDATE_MODE_PARTIAL			0x0
#define UPDATE_MODE_FULL			0x1

#define TRUE 1
#define FALSE 0

#define msleep(a)	udelay(a * 1000)


/********************************************************
 * Start Low-Level EPDC Functions
 ********************************************************/

static inline void epdc_set_screen_res(u32 width, u32 height)
{
	u32 val = (height << EPDC_RES_VERTICAL_OFFSET) | width;

	REG_WR(EPDC_BASE, EPDC_RES, val);
}

static inline void epdc_set_update_coord(u32 x, u32 y)
{
	u32 val = (y << EPDC_UPD_CORD_YCORD_OFFSET) | x;

	REG_WR(EPDC_BASE, EPDC_UPD_CORD, val);
}

static inline void epdc_set_update_dimensions(u32 width, u32 height)
{
	u32 val = (height << EPDC_UPD_SIZE_HEIGHT_OFFSET) | width;

	REG_WR(EPDC_BASE, EPDC_UPD_SIZE, val);
}

static void epdc_submit_update(u32 lut_num, u32 waveform_mode, u32 update_mode,
			       int use_test_mode, u32 np_val)
{
	u32 reg_val = 0;

	if (use_test_mode) {
		reg_val |=
			((np_val << EPDC_UPD_FIXED_FIXNP_OFFSET) &
			EPDC_UPD_FIXED_FIXNP_MASK) | EPDC_UPD_FIXED_FIXNP_EN;

		REG_WR(EPDC_BASE, EPDC_UPD_FIXED, reg_val);

		reg_val = EPDC_UPD_CTRL_USE_FIXED;
	} else {
		REG_WR(EPDC_BASE, EPDC_UPD_FIXED, reg_val);
	}

	reg_val |=
		((lut_num << EPDC_UPD_CTRL_LUT_SEL_OFFSET) &
		EPDC_UPD_CTRL_LUT_SEL_MASK) |
		((waveform_mode << EPDC_UPD_CTRL_WAVEFORM_MODE_OFFSET) &
		EPDC_UPD_CTRL_WAVEFORM_MODE_MASK) |
		update_mode;

	REG_WR(EPDC_BASE, EPDC_UPD_CTRL, reg_val);
}

static inline int epdc_is_lut_active(u32 lut_num)
{
	u32 val = REG_RD(EPDC_BASE, EPDC_STATUS_LUTS);
	int is_active = val & (1 << lut_num) ? TRUE : FALSE;

	return is_active;
}

static void epdc_set_horizontal_timing(u32 horiz_start, u32 horiz_end,
				       u32 hsync_width, u32 hsync_line_length)
{
	u32 reg_val =
		((hsync_width << EPDC_TCE_HSCAN1_LINE_SYNC_WIDTH_OFFSET) &
		EPDC_TCE_HSCAN1_LINE_SYNC_WIDTH_MASK)
		| ((hsync_line_length << EPDC_TCE_HSCAN1_LINE_SYNC_OFFSET) &
		EPDC_TCE_HSCAN1_LINE_SYNC_MASK);
	REG_WR(EPDC_BASE, EPDC_TCE_HSCAN1, reg_val);

	reg_val =
		((horiz_start << EPDC_TCE_HSCAN2_LINE_BEGIN_OFFSET) &
		EPDC_TCE_HSCAN2_LINE_BEGIN_MASK)
		| ((horiz_end << EPDC_TCE_HSCAN2_LINE_END_OFFSET) &
		EPDC_TCE_HSCAN2_LINE_END_MASK);
	REG_WR(EPDC_BASE, EPDC_TCE_HSCAN2, reg_val);
}

static void epdc_set_vertical_timing(u32 vert_start, u32 vert_end,
					u32 vsync_width)
{
	u32 reg_val =
		((vert_start << EPDC_TCE_VSCAN_FRAME_BEGIN_OFFSET) &
		EPDC_TCE_VSCAN_FRAME_BEGIN_MASK)
		| ((vert_end << EPDC_TCE_VSCAN_FRAME_END_OFFSET) &
		EPDC_TCE_VSCAN_FRAME_END_MASK)
		| ((vsync_width << EPDC_TCE_VSCAN_FRAME_SYNC_OFFSET) &
		EPDC_TCE_VSCAN_FRAME_SYNC_MASK);
	REG_WR(EPDC_BASE, EPDC_TCE_VSCAN, reg_val);
}

static void epdc_init_settings(void)
{
	u32 reg_val;
	int num_ce;

	/* EPDC_CTRL */
	reg_val = REG_RD(EPDC_BASE, EPDC_CTRL);
	reg_val &= ~EPDC_CTRL_UPD_DATA_SWIZZLE_MASK;
	reg_val |= EPDC_CTRL_UPD_DATA_SWIZZLE_NO_SWAP;
	reg_val &= ~EPDC_CTRL_LUT_DATA_SWIZZLE_MASK;
	reg_val |= EPDC_CTRL_LUT_DATA_SWIZZLE_NO_SWAP;
	REG_SET(EPDC_BASE, EPDC_CTRL, reg_val);

	/* EPDC_FORMAT - 2bit TFT and 4bit Buf pixel format */
	reg_val = EPDC_FORMAT_TFT_PIXEL_FORMAT_2BIT
		| EPDC_FORMAT_BUF_PIXEL_FORMAT_P4N
		| ((0x0 << EPDC_FORMAT_DEFAULT_TFT_PIXEL_OFFSET) &
		EPDC_FORMAT_DEFAULT_TFT_PIXEL_MASK);
	REG_WR(EPDC_BASE, EPDC_FORMAT, reg_val);

	/* EPDC_FIFOCTRL (disabled) */
	reg_val =
		((100 << EPDC_FIFOCTRL_FIFO_INIT_LEVEL_OFFSET) &
		EPDC_FIFOCTRL_FIFO_INIT_LEVEL_MASK)
		| ((200 << EPDC_FIFOCTRL_FIFO_H_LEVEL_OFFSET) &
		EPDC_FIFOCTRL_FIFO_H_LEVEL_MASK)
		| ((100 << EPDC_FIFOCTRL_FIFO_L_LEVEL_OFFSET) &
		EPDC_FIFOCTRL_FIFO_L_LEVEL_MASK);
	REG_WR(EPDC_BASE, EPDC_FIFOCTRL, reg_val);

	/* EPDC_TEMP - Use default temperature */
	REG_WR(EPDC_BASE, EPDC_TEMP, TEMP_USE_DEFAULT);

	/* EPDC_RES */
	epdc_set_screen_res(panel_info.vl_col, panel_info.vl_row);

	/*
	 * EPDC_TCE_CTRL
	 * VSCAN_HOLDOFF = 4
	 * VCOM_MODE = MANUAL
	 * VCOM_VAL = 0
	 * DDR_MODE = DISABLED
	 * LVDS_MODE_CE = DISABLED
	 * LVDS_MODE = DISABLED
	 * DUAL_SCAN = DISABLED
	 * SDDO_WIDTH = 8bit
	 * PIXELS_PER_SDCLK = 4
	 */
	reg_val =
		((panel_info.epdc_data.epdc_timings.vscan_holdoff <<
			EPDC_TCE_CTRL_VSCAN_HOLDOFF_OFFSET) &
			EPDC_TCE_CTRL_VSCAN_HOLDOFF_MASK)
		| EPDC_TCE_CTRL_PIXELS_PER_SDCLK_4;
	REG_WR(EPDC_BASE, EPDC_TCE_CTRL, reg_val);

	/* EPDC_TCE_HSCAN */
	epdc_set_horizontal_timing(panel_info.vl_left_margin,
				panel_info.vl_right_margin,
				panel_info.vl_hsync,
				panel_info.vl_hsync);

	/* EPDC_TCE_VSCAN */
	epdc_set_vertical_timing(panel_info.vl_upper_margin,
				 panel_info.vl_lower_margin,
				 panel_info.vl_vsync);

	/* EPDC_TCE_OE */
	reg_val =
		((panel_info.epdc_data.epdc_timings.sdoed_width <<
			EPDC_TCE_OE_SDOED_WIDTH_OFFSET) &
			EPDC_TCE_OE_SDOED_WIDTH_MASK)
		| ((panel_info.epdc_data.epdc_timings.sdoed_delay <<
			EPDC_TCE_OE_SDOED_DLY_OFFSET) &
			EPDC_TCE_OE_SDOED_DLY_MASK)
		| ((panel_info.epdc_data.epdc_timings.sdoez_width <<
			EPDC_TCE_OE_SDOEZ_WIDTH_OFFSET) &
			EPDC_TCE_OE_SDOEZ_WIDTH_MASK)
		| ((panel_info.epdc_data.epdc_timings.sdoez_delay <<
			EPDC_TCE_OE_SDOEZ_DLY_OFFSET) &
			EPDC_TCE_OE_SDOEZ_DLY_MASK);
	REG_WR(EPDC_BASE, EPDC_TCE_OE, reg_val);

	/* EPDC_TCE_TIMING1 */
	REG_WR(EPDC_BASE, EPDC_TCE_TIMING1, 0x0);

	/* EPDC_TCE_TIMING2 */
	reg_val =
		((panel_info.epdc_data.epdc_timings.gdclk_hp_offs <<
			EPDC_TCE_TIMING2_GDCLK_HP_OFFSET) &
			EPDC_TCE_TIMING2_GDCLK_HP_MASK)
		| ((panel_info.epdc_data.epdc_timings.gdsp_offs <<
			EPDC_TCE_TIMING2_GDSP_OFFSET_OFFSET) &
			EPDC_TCE_TIMING2_GDSP_OFFSET_MASK);
	REG_WR(EPDC_BASE, EPDC_TCE_TIMING2, reg_val);

	/* EPDC_TCE_TIMING3 */
	reg_val =
		((panel_info.epdc_data.epdc_timings.gdoe_offs <<
			EPDC_TCE_TIMING3_GDOE_OFFSET_OFFSET) &
			EPDC_TCE_TIMING3_GDOE_OFFSET_MASK)
		| ((panel_info.epdc_data.epdc_timings.gdclk_offs <<
			EPDC_TCE_TIMING3_GDCLK_OFFSET_OFFSET) &
			EPDC_TCE_TIMING3_GDCLK_OFFSET_MASK);
	REG_WR(EPDC_BASE, EPDC_TCE_TIMING3, reg_val);

	/*
	 * EPDC_TCE_SDCFG
	 * SDCLK_HOLD = 1
	 * SDSHR = 1
	 * NUM_CE = 1
	 * SDDO_REFORMAT = FLIP_PIXELS
	 * SDDO_INVERT = DISABLED
	 * PIXELS_PER_CE = display horizontal resolution
	 */
	num_ce = panel_info.epdc_data.epdc_timings.num_ce;
	if (num_ce == 0)
		num_ce = 1;
	reg_val = EPDC_TCE_SDCFG_SDCLK_HOLD | EPDC_TCE_SDCFG_SDSHR
		| ((num_ce << EPDC_TCE_SDCFG_NUM_CE_OFFSET) & EPDC_TCE_SDCFG_NUM_CE_MASK)
		| EPDC_TCE_SDCFG_SDDO_REFORMAT_FLIP_PIXELS
		| ((panel_info.vl_col << EPDC_TCE_SDCFG_PIXELS_PER_CE_OFFSET) &
		EPDC_TCE_SDCFG_PIXELS_PER_CE_MASK);
	REG_WR(EPDC_BASE, EPDC_TCE_SDCFG, reg_val);

	/*
	 * EPDC_TCE_GDCFG
	 * GDRL = 1
	 * GDOE_MODE = 0;
	 * GDSP_MODE = 0;
	 */
	reg_val = EPDC_TCE_SDCFG_GDRL;
	REG_WR(EPDC_BASE, EPDC_TCE_GDCFG, reg_val);

	/*
	 * EPDC_TCE_POLARITY
	 * SDCE_POL = ACTIVE LOW
	 * SDLE_POL = ACTIVE HIGH
	 * SDOE_POL = ACTIVE HIGH
	 * GDOE_POL = ACTIVE HIGH
	 * GDSP_POL = ACTIVE LOW
	 */
	reg_val = EPDC_TCE_POLARITY_SDLE_POL_ACTIVE_HIGH
		| EPDC_TCE_POLARITY_SDOE_POL_ACTIVE_HIGH
		| EPDC_TCE_POLARITY_GDOE_POL_ACTIVE_HIGH;
	REG_WR(EPDC_BASE, EPDC_TCE_POLARITY, reg_val);

	/* EPDC_IRQ_MASK */
	REG_WR(EPDC_BASE, EPDC_IRQ_MASK,
		EPDC_IRQ_TCE_UNDERRUN_IRQ);

	/*
	 * EPDC_GPIO
	 * PWRCOM = ?
	 * PWRCTRL = ?
	 * BDR = ?
	 */
	reg_val = ((0 << EPDC_GPIO_PWRCTRL_OFFSET) & EPDC_GPIO_PWRCTRL_MASK)
		| ((0 << EPDC_GPIO_BDR_OFFSET) & EPDC_GPIO_BDR_MASK);
	REG_WR(EPDC_BASE, EPDC_GPIO, reg_val);
}

static void draw_mode0(void)
{
	int i;

	/* Program EPDC update to process buffer */
	epdc_set_update_coord(0, 0);
	epdc_set_update_dimensions(panel_info.vl_col, panel_info.vl_row);
	epdc_submit_update(0, panel_info.epdc_data.wv_modes.mode_init,
				UPDATE_MODE_FULL, FALSE, 0);

	debug("Mode0 update - Waiting for LUT to complete...\n");

	/* Will timeout after ~4-5 seconds */

	for (i = 0; i < 40; i++) {
		if (!epdc_is_lut_active(0)) {
			debug("Mode0 init complete\n");
			return;
		}
		msleep(100);
	}

	debug("Mode0 init failed!\n");

}

static void draw_splash_screen(void)
{
	int i;
	int lut_num = 0;

	/* Program EPDC update to process buffer */
	epdc_set_update_coord(0, 0);
	epdc_set_update_dimensions(panel_info.vl_col, panel_info.vl_row);
	epdc_submit_update(lut_num, panel_info.epdc_data.wv_modes.mode_gc16,
		UPDATE_MODE_FULL, FALSE, 0);

	for (i = 0; i < 40; i++) {
		if (!epdc_is_lut_active(lut_num)) {
			debug("Splash screen update complete\n");
			return;
		}
		msleep(100);
	}
	debug("Splash screen update failed!\n");
}

void lcd_enable(void)
{
	if (board_setup_logo_file(lcd_base)) {
		debug("Load logo failed!\n");
		return;
	}

	epdc_power_on();

	flush_cache((ulong)lcd_base, panel_info.vl_col * panel_info.vl_row);

	/* Draw data to display */
	draw_mode0();

	draw_splash_screen();
}

void lcd_disable(void)
{
	debug("lcd_disable\n");

	/* Disable clocks to EPDC */
	REG_SET(EPDC_BASE, EPDC_CTRL, EPDC_CTRL_CLKGATE);
}

void lcd_panel_disable(void)
{
	epdc_power_off();
}

void lcd_ctrl_init(void *lcdbase)
{
	unsigned int val;

	/*
	 * We rely on lcdbase being a physical address, i.e., either MMU off,
	 * or 1-to-1 mapping. Might want to add some virt2phys here.
	 */
	if (!lcdbase)
		return;

	panel_info.epdc_data.working_buf_addr = (u_long)memalign(ARCH_DMA_MINALIGN,
		panel_info.vl_col * panel_info.vl_row * 2);

	if (!panel_info.epdc_data.working_buf_addr) {
		printf("EPDC: Error allocating working buffer!\n");
		return;
	}

	panel_info.epdc_data.waveform_buf_addr = (u_long)memalign(ARCH_DMA_MINALIGN,
		CONFIG_WAVEFORM_BUF_SIZE);

	if (!panel_info.epdc_data.waveform_buf_addr) {
		printf("EPDC: Error allocating waveform buffer!\n");
		return;
	}

	lcd_color_fg = 0xFF;
	lcd_color_bg = 0xFF;

	/* Reset */
	REG_SET(EPDC_BASE, EPDC_CTRL, EPDC_CTRL_SFTRST);
	while (!(REG_RD(EPDC_BASE, EPDC_CTRL) & EPDC_CTRL_CLKGATE))
		;
	REG_CLR(EPDC_BASE, EPDC_CTRL, EPDC_CTRL_SFTRST);

	/* Enable clock gating (clear to enable) */
	REG_CLR(EPDC_BASE, EPDC_CTRL, EPDC_CTRL_CLKGATE);
	while (REG_RD(EPDC_BASE, EPDC_CTRL) &
	       (EPDC_CTRL_SFTRST | EPDC_CTRL_CLKGATE))
		;

	debug("resolution %dx%d, bpp %d\n", (int)panel_info.vl_col,
		(int)panel_info.vl_row, NBITS(panel_info.vl_bpix));

	/* Get EPDC version */
	val = REG_RD(EPDC_BASE, EPDC_VERSION);
	rev = ((val & EPDC_VERSION_MAJOR_MASK) >>
				EPDC_VERSION_MAJOR_OFFSET) * 10
			+ ((val & EPDC_VERSION_MINOR_MASK) >>
				EPDC_VERSION_MINOR_OFFSET);

	/* Set framebuffer pointer */
	REG_WR(EPDC_BASE, EPDC_UPD_ADDR, (u32)lcdbase);

	/* Set Working Buffer pointer */
	REG_WR(EPDC_BASE, EPDC_WB_ADDR, panel_info.epdc_data.working_buf_addr);
	if (rev > 20)
		REG_WR(EPDC_BASE, EPDC_WB_ADDR_TCE, panel_info.epdc_data.working_buf_addr);

	/* Get waveform data address and offset */
	if (board_setup_waveform_file(panel_info.epdc_data.waveform_buf_addr)) {
		printf("Can't load waveform data!\n");
		return;
	}

	/* Set Waveform Buffer pointer */
	REG_WR(EPDC_BASE, EPDC_WVADDR,
		panel_info.epdc_data.waveform_buf_addr);

	/* Initialize EPDC, passing pointer to EPDC registers */
	epdc_init_settings();

	lcd_base = lcdbase;

	return;
}

ulong calc_fbsize(void)
{
	return panel_info.vl_row * panel_info.vl_col * 2 \
		* NBITS(panel_info.vl_bpix) / 8;
}

