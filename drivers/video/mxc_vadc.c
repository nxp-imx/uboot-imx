/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2014 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 */

#include <common.h>

#include <asm/arch/imx-regs.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/clock.h>
#include <linux/errno.h>
#include <asm/io.h>

#include <linux/string.h>
#include <linux/list.h>
#include <gis.h>

#include "mxc_vadc.h"

#define reg32_write(addr, val) __raw_writel(val, addr)
#define reg32_read(addr)       __raw_readl(addr)
#define reg32setbit(addr, bitpos) \
	reg32_write((addr), (reg32_read((addr)) | (1<<(bitpos))))

#define reg32clrbit(addr, bitpos) \
	reg32_write((addr), (reg32_read((addr)) & (0xFFFFFFFF ^ (1<<(bitpos)))))

void __iomem *vafe_regbase;
void __iomem *vdec_regbase;

enum {
	STD_NTSC = 0,
	STD_PAL,
};

/* Video format structure. */
struct video_fmt_t{
	int v4l2_id;		/* Video for linux ID. */
	char name[16];		/* Name (e.g., "NTSC", "PAL", etc.) */
	u16 active_width;	/* Active width. */
	u16 active_height;	/* Active height. */
};

/* Description of video formats supported.
 *
 *  PAL: active=720x576.
 *  NTSC:active=720x480.
 */
static struct video_fmt_t video_fmts[] = {
	/* NTSC */
	{
	 .v4l2_id = STD_NTSC,
	 .name = "NTSC",
	 .active_width = 720,
	 .active_height = 480,
	 },
	/* (B, G, H, I, N) PAL */
	{
	 .v4l2_id = STD_PAL,
	 .name = "PAL",
	 .active_width = 720,
	 .active_height = 576,
	 },
};

static void afe_voltage_clampingmode(void)
{
	reg32_write(AFE_CLAMP, 0x07);
	reg32_write(AFE_CLMPAMP, 0x60);
	reg32_write(AFE_CLMPDAT, 0xF0);
}

static void afe_alwayson_clampingmode(void)
{
	reg32_write(AFE_CLAMP, 0x15);
	reg32_write(AFE_CLMPDAT, 0x08);
	reg32_write(AFE_CLMPAMP, 0x00);
}

static void afe_init(void)
{
	reg32_write(AFE_PDBUF, 0x1f);
	reg32_write(AFE_PDADC, 0x0f);
	reg32_write(AFE_PDSARH, 0x01);
	reg32_write(AFE_PDSARL, 0xff);
	reg32_write(AFE_PDADCRFH, 0x01);
	reg32_write(AFE_PDADCRFL, 0xff);
	reg32_write(AFE_ICTRL, 0x3a);
	reg32_write(AFE_ICTLSTG, 0x1e);

	reg32_write(AFE_RCTRLSTG, 0x1e);
	reg32_write(AFE_INPBUF, 0x035);
	reg32_write(AFE_INPFLT, 0x02);
	reg32_write(AFE_ADCDGN, 0x40);
	reg32_write(AFE_TSTSEL, 0x10);

	reg32_write(AFE_ACCTST, 0x07);

	reg32_write(AFE_BGREG, 0x08);

	reg32_write(AFE_ADCGN, 0x09);

	/* set current controlled clamping
	* always on, low current */
	reg32_write(AFE_CLAMP, 0x11);
	reg32_write(AFE_CLMPAMP, 0x08);
}

static void vdec_mode_timing_init(u32 std)
{
	if (std == STD_NTSC) {
		/* NTSC 720x480 */
		printf("NTSC\n");
		reg32_write(VDEC_HACTS, 0x66);
		reg32_write(VDEC_HACTE, 0x24);

		reg32_write(VDEC_VACTS, 0x29);
		reg32_write(VDEC_VACTE, 0x04);

		/* set V Position */
		reg32_write(VDEC_VRTPOS, 0x2);
	} else if (std == STD_PAL) {
		/* PAL 720x576 */
		printf("PAL\n");
		reg32_write(VDEC_HACTS, 0x66);
		reg32_write(VDEC_HACTE, 0x24);

		reg32_write(VDEC_VACTS, 0x29);
		reg32_write(VDEC_VACTE, 0x04);

		/* set V Position */
		reg32_write(VDEC_VRTPOS, 0x6);
	} else
		printf("Error not support video mode\n");

	/* set H Position */
	reg32_write(VDEC_HZPOS, 0x60);

	/* set H ignore start */
	reg32_write(VDEC_HSIGS, 0xf8);

	/* set H ignore end */
	reg32_write(VDEC_HSIGE, 0x18);
}

/*
* vdec_init()
* Initialises the VDEC registers
* Returns: nothing
*/
static void vdec_init(struct sensor_data *vadc)
{
	/* Get work mode PAL or NTSC
	 * delay 500ms wait vdec detect input format*/
	udelay(500*1000);
	vadc_get_std(vadc);

	vdec_mode_timing_init(vadc->std_id);

	/* vcr detect threshold high, automatic detections */
	reg32_write(VDEC_VSCON2, 0);

	reg32_write(VDEC_BASE + 0x110, 0x01);

	/* set the noramp mode on the Hloop PLL. */
	reg32_write(VDEC_BASE+(0x14*4), 0x10);

	/* set the YC relative delay.*/
	reg32_write(VDEC_YCDEL, 0x90);

	/* setup the Hpll */
	reg32_write(VDEC_BASE+(0x13*4), 0x13);

	/* setup the 2d comb */
	/* set the gain of the Hdetail output to 3
	 * set the notch alpha gain to 1 */
	reg32_write(VDEC_CFC2, 0x34);

	/* setup various 2d comb bits.*/
	reg32_write(VDEC_BASE+(0x02*4), 0x01);
	reg32_write(VDEC_BASE+(0x03*4), 0x18);
	reg32_write(VDEC_BASE+(0x04*4), 0x34);

	/* set the start of the burst gate */
	reg32_write(VDEC_BRSTGT, 0x30);

	/* set 1f motion gain */
	reg32_write(VDEC_BASE+(0x0f*4), 0x20);

	/* set the 1F chroma motion detector thresh for colour reverse detection */
	reg32_write(VDEC_THSH1, 0x02);
	reg32_write(VDEC_BASE+(0x4a*4), 0x20);
	reg32_write(VDEC_BASE+(0x4b*4), 0x08);

	reg32_write(VDEC_BASE+(0x4c*4), 0x08);

	/* set the threshold for the narrow/wide adaptive chroma BW */
	reg32_write(VDEC_BASE+(0x20*4), 0x20);

	/* turn up the colour with the new colour gain reg */
	/* hue: */
	reg32_write(VDEC_HUE, 0x00);

	/* cbgain: 22 B4 */
	reg32_write(VDEC_CBGN, 0xb4);
	/* cr gain 80 */
	reg32_write(VDEC_CRGN, 0x80);
	/* luma gain (contrast) */
	reg32_write(VDEC_CNTR, 0x80);

	/* setup the signed black level register, brightness */
	reg32_write(VDEC_BRT, 0x00);

	/* filter the standard detection
	 * enable the comb for the ntsc443 */
	reg32_write(VDEC_STDDBG, 0x23);

	/* setup chroma kill thresh for no chroma */
	reg32_write(VDEC_CHBTH, 0x0);

	/* set chroma loop to wider BW
	 * no set it to normal BW. i fixed the bw problem.*/
	reg32_write(VDEC_YCDEL, 0x00);

	/* set the compensation in the chroma loop for the Hloop
	 * set the ratio for the nonarithmetic 3d comb modes.*/
	reg32_write(VDEC_BASE + (0x1d*4), 0x90);

	/* set the threshold for the nonarithmetic mode for the 2d comb
	 * the higher the value the more Fc Fh offset we will tolerate before turning off the comb. */
	reg32_write(VDEC_BASE + (0x33*4), 0xa0);

	/* setup the bluescreen output colour */
	reg32_write(VDEC_BASE + (0x3d*4), 35);
	reg32_write(VDEC_BLSCRCR, 114);
	reg32_write(VDEC_BLSCRCB, 212);

	/* disable the active blanking */
	reg32_write(VDEC_BASE + (0x15*4), 0x02);

	/* setup the luma agc for automatic gain. */
	reg32_write(VDEC_LMAGC2, 0x5e);
	reg32_write(VDEC_BASE + (0x40*4), 0x81);

	/* setup chroma agc */
	reg32_write(VDEC_CHAGC2, 0xa0);
	reg32_write(VDEC_CHAGC1, 0x01);

	/* setup the MV thresh lower nibble
	 * setup the sync top cap, upper nibble */
	reg32_write(VDEC_BASE + (0x3a*4), 0x80);
	reg32_write(VDEC_SHPIMP, 0x00);

	/* setup the vsync block */
	reg32_write(VDEC_VSCON1, 0x87);

	/* set the nosignal threshold
	 * set the vsync threshold */
	reg32_write(VDEC_VSSGTH, 0x35);

	/* set length for min hphase filter (or saturate limit if saturate is chosen) */
	reg32_write(VDEC_BASE + (0x45*4), 0x40);

	/* enable the internal resampler,
	 * select min filter not saturate for hphase noise filter for vcr detect.
	 * enable vcr pause mode different field lengths */
	reg32_write(VDEC_BASE + (0x46*4), 0x90);

	/* disable VCR detection, lock to the Hsync rather than the Vsync */
	reg32_write(VDEC_VSCON2, 0x04);

	/* set tiplevel goal for dc clamp. */
	reg32_write(VDEC_BASE + (0x3c*4), 0xB0);

	/* override SECAM detection and force SECAM off */
	reg32_write(VDEC_BASE + (0x2f*4), 0x20);

	/* Set r3d_hardblend in 3D control2 reg */
	reg32_write(VDEC_BASE + (0x0c*4), 0x04);
}

/* set Input selector & input pull-downs */
static void vadc_select_input(int vadc_in)
{
	switch (vadc_in) {
	case 0:
		reg32_write(AFE_INPFLT, 0x02);
		reg32_write(AFE_OFFDRV, 0x00);
		reg32_write(AFE_INPCONFIG, 0x1e);
		break;
	case 1:
		reg32_write(AFE_INPFLT, 0x02);
		reg32_write(AFE_OFFDRV, 0x00);
		reg32_write(AFE_INPCONFIG, 0x2d);
		break;
	case 2:
		reg32_write(AFE_INPFLT, 0x02);
		reg32_write(AFE_OFFDRV, 0x00);
		reg32_write(AFE_INPCONFIG, 0x4b);
		break;
	case 3:
		reg32_write(AFE_INPFLT, 0x02);
		reg32_write(AFE_OFFDRV, 0x00);
		reg32_write(AFE_INPCONFIG, 0x87);
		break;
	default:
		printf("error video input %d\n", vadc_in);
	}
}

/*!
 * Return attributes of current video standard.
 * Since this device autodetects the current standard, this function also
 * sets the values that need to be changed if the standard changes.
 * There is no set std equivalent function.
 *
 *  @return		None.
 */
void vadc_get_std(struct sensor_data *vadc)
{
	int tmp;
	int idx;

	/* Read PAL mode detected result */
	tmp = reg32_read(VDEC_VIDMOD);
	tmp &= (VDEC_VIDMOD_PAL_MASK | VDEC_VIDMOD_M625_MASK);

	if (tmp)
		idx = STD_PAL;
	else
		idx = STD_NTSC;

	vadc->std_id = idx;
	vadc->pixel_fmt = FMT_YUV444;
	vadc->width = video_fmts[idx].active_width;
	vadc->height = video_fmts[idx].active_height;
}

void vadc_config(u32 vadc_in)
{
	struct sensor_data vadc;

	/* map vafe,vdec,gpr,gpc address  */
	vafe_regbase = (u32 *)VADC_BASE_ADDR;
	vdec_regbase = (u32 *)VDEC_BASE_ADDR;

	vadc_power_up();

	/* clock config for vadc */
	reg32_write(VDEC_BASE + 0x320, 0xe3);
	reg32_write(VDEC_BASE + 0x324, 0x38);
	reg32_write(VDEC_BASE + 0x328, 0x8e);
	reg32_write(VDEC_BASE + 0x32c, 0x23);
	mxs_set_vadcclk();

	afe_init();

	/* select Video Input 0-3 */
	vadc_select_input(vadc_in);

	afe_voltage_clampingmode();

	vdec_init(&vadc);

	/*
	* current control loop will move sinewave input off below
	* the bottom of the signal range visible when the testbus is viewed as magnitude,
	* so have to break before this point while capturing ENOB data:
	*/
	afe_alwayson_clampingmode();
}

