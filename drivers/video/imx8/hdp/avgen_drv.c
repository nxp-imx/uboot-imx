/******************************************************************************
 *
 * Copyright (C) 2016-2017 Cadence Design Systems, Inc.
 * All rights reserved worldwide.
 *
 * Copyright 2017-2018 NXP
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT  (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. THE SOFTWARE IS PROVIDED "AS IS",
 * WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ******************************************************************************
 *
 * avgen_drv.c
 *
 ******************************************************************************
 */

#include "mhl_hdtx_top.h"
#include "address.h"
#include "avgen.h"
#include "avgen_drv.h"
#include "util.h"
#include "externs.h"

#define ADDR_AVGEN 0x80000

CDN_API_STATUS CDN_API_AVGEN_Set(VIC_MODES vicMode, CDN_PROTOCOL_TYPE protocol,
				 VIC_PXL_ENCODING_FORMAT format)
{
	/*CDN_API_STATUS ret; */
	/*GENERAL_Read_Register_response resp; */
	unsigned int pixelClockFreq = CDN_API_Get_PIXEL_FREQ_KHZ_ClosetVal
		(vic_table[vicMode][PIXEL_FREQ_KHZ], protocol);
	unsigned int v_h_polarity =
		((vic_table[vicMode][HSYNC_POL] == ACTIVE_LOW) ? 0 : 1) +
		((vic_table[vicMode][VSYNC_POL] == ACTIVE_LOW) ? 0 : 2);
	unsigned int front_porche_l = vic_table[vicMode][FRONT_PORCH] - 256 *
		((unsigned int)vic_table[vicMode][FRONT_PORCH] / 256);
	unsigned int front_porche_h = vic_table[vicMode][FRONT_PORCH] / 256;
	unsigned int back_porche_l = vic_table[vicMode][BACK_PORCH] - 256 *
		((unsigned int)vic_table[vicMode][BACK_PORCH] / 256);
	unsigned int back_porche_h = vic_table[vicMode][BACK_PORCH] / 256;
	unsigned int active_slot_l = vic_table[vicMode][H_BLANK] - 256 *
		((unsigned int)vic_table[vicMode][H_BLANK] / 256);
	unsigned int active_slot_h = vic_table[vicMode][H_BLANK] / 256;
	unsigned int frame_lines_l = vic_table[vicMode][V_TOTAL] - 256 *
		((unsigned int)vic_table[vicMode][V_TOTAL] / 256);
	unsigned int frame_lines_h = vic_table[vicMode][V_TOTAL] / 256;
	unsigned int line_width_l = vic_table[vicMode][H_TOTAL] - 256 *
		((unsigned int)vic_table[vicMode][H_TOTAL] / 256);
	unsigned int line_width_h = vic_table[vicMode][H_TOTAL] / 256;
	unsigned int vsync_lines = vic_table[vicMode][VSYNC];
	unsigned int eof_lines = vic_table[vicMode][TYPE_EOF];
	unsigned int sof_lines = vic_table[vicMode][SOF];
	unsigned int interlace_progressive =
		(vic_table[vicMode][I_P] == INTERLACED) ? 2 : 0;
	unsigned int set_vif_clock = 0;

	/*needed for HDMI /////////////////////////////// */
	/*unsigned int hblank = vic_table[vicMode][H_BLANK]; */
	/*unsigned int hactive = vic_table[vicMode][H_TOTAL]-hblank; */
	/*unsigned int vblank = vsync_lines+eof_lines+sof_lines; */
	/*unsigned int vactive = vic_table[vicMode][V_TOTAL]-vblank; */
	/*unsigned int hfront = vic_table[vicMode][FRONT_PORCH]; */
	/*unsigned int hback = vic_table[vicMode][BACK_PORCH]; */
	/*unsigned int vfront = eof_lines; */
	/*unsigned int hsync = hblank-hfront-hback; */
	/*unsigned int vsync = vsync_lines; */
	/*unsigned int vback = sof_lines; */
	unsigned int set_CLK_SEL            = 0;
	unsigned int set_REF_CLK_SEL        = 0;
	unsigned int set_pll_CLK_IN            = 0;
	unsigned int set_pll_clkfbout_l        = 0;
	unsigned int set_pll_clkfbout_h        = 0;
	unsigned int set_pll_CLKOUT5_L        = 0;
	unsigned int set_pll_CLKOUT5_H        = 0;
	unsigned int set_pll2_CLKIN            = 0;
	unsigned int set_pll2_CLKFBOUT_L    = 0;
	unsigned int set_pll2_CLKFBOUT_H    = 0;
	unsigned int set_pll2_CLKOUT5_L        = 0;
	unsigned int set_pll2_CLKOUT5_H        = 0;
	/*///////////////////////////////////////////////// */

	cdn_apb_write(0x1c00C6 << 2,
		      (int)(vic_table[vicMode][PIXEL_FREQ_KHZ] * 1000));
	cdn_apb_write(0x1c00C6 << 2, (int)(pixelClockFreq));

	if ((int)(pixelClockFreq) == 25) {
		if (protocol == CDN_HDMITX_TYPHOON) {
			set_CLK_SEL                = 4;
			set_REF_CLK_SEL            = 0;
			set_pll_CLK_IN            = 65;
			set_pll_clkfbout_l        = 4292;
			set_pll_clkfbout_h        = 128;
			set_pll_CLKOUT5_L        = 4422;
			set_pll_CLKOUT5_H        = 128;
			set_pll2_CLKIN            = 12289;
			set_pll2_CLKFBOUT_L        = 4356;
			set_pll2_CLKFBOUT_H        = 0;
			set_pll2_CLKOUT5_L        = 4552;
			set_pll2_CLKOUT5_H        = 128;
		} else {
			set_vif_clock = 0x300;
		}
	} else if ((int)pixelClockFreq == 27000) {
		if (protocol == CDN_HDMITX_TYPHOON) {
			set_CLK_SEL                = 5;
			set_REF_CLK_SEL            = 0;
			set_pll_CLK_IN            = 49217;
			set_pll_clkfbout_l        = 4226;
			set_pll_clkfbout_h        = 0;
			set_pll_CLKOUT5_L        = 4422;
			set_pll_CLKOUT5_H        = 128;
		} else {
			set_vif_clock = 0x301;
		}
	} else if ((int)pixelClockFreq == 54000) {
		if (protocol == CDN_HDMITX_TYPHOON) {
			set_CLK_SEL             = 5;
			set_REF_CLK_SEL         = 0;
			set_pll_CLK_IN          = 4096;
			set_pll_clkfbout_l      = 4226;
			set_pll_clkfbout_h      = 0;
			set_pll_CLKOUT5_L       = 4422;
			set_pll_CLKOUT5_H       = 128;
		} else {
			set_vif_clock = 0x302;
		}
	} else if (pixelClockFreq == 74250) {
		if (protocol == CDN_HDMITX_TYPHOON) {
			set_CLK_SEL          = 1;
			set_pll_CLK_IN       = 74;
		} else {
			set_vif_clock = 0x303;
		}
	} else if (pixelClockFreq == 148500) {
		if (protocol == CDN_HDMITX_TYPHOON) {
			set_CLK_SEL             = 0;
			set_pll_CLK_IN       = 148;
		} else {
			set_vif_clock = 0x304;
		}
	} else if ((int)pixelClockFreq == 108000) {
		if (protocol == CDN_HDMITX_TYPHOON) {
			set_CLK_SEL                = 5;
			set_REF_CLK_SEL            = 2;
			set_pll_CLK_IN            = 8258;
			set_pll_clkfbout_l        = 4616;
			set_pll_clkfbout_h        = 0;
			set_pll_CLKOUT5_L        = 4422;
			set_pll_CLKOUT5_H        = 128;
		} else {
			set_vif_clock = 0x305;
		}
	} else {
		if (protocol == CDN_HDMITX_TYPHOON) {
			set_CLK_SEL             = 1;
			set_pll_CLK_IN          = pixelClockFreq;
		} else {
			set_vif_clock = 0;
		}
	}
	unsigned int start_pgen = 128;
	/*unsigned int temp; */
	if (protocol == CDN_HDMITX_TYPHOON) {
		if (cdn_apb_write(0x0c0001 << 2,
				  ((0) + (2 * set_CLK_SEL) + (16 * 0) +
				   (32 * 0) + (64 * 3) + (65536 * 3) +
				   (1048576 * set_REF_CLK_SEL))))
			return CDN_ERR;
		if (cdn_apb_write(0x1c00C6 << 2, set_pll_CLK_IN))
			return CDN_ERR;
		if (cdn_apb_write(0x1c00CC << 2, set_pll_clkfbout_l))
			return CDN_ERR;
		if (cdn_apb_write(0x1c00CD << 2, set_pll_clkfbout_h))
			return CDN_ERR;
		if (cdn_apb_write(0x1c00CE << 2, set_pll_CLKOUT5_L))
			return CDN_ERR;
		if (cdn_apb_write(0x1c00CF << 2, set_pll_CLKOUT5_H))
			return CDN_ERR;
		if (cdn_apb_write(0x1c0086 << 2, set_pll2_CLKIN))
			return CDN_ERR;
		if (cdn_apb_write(0x1c008C << 2, set_pll2_CLKFBOUT_L))
			return CDN_ERR;
		if (cdn_apb_write(0x1c008D << 2, set_pll2_CLKFBOUT_H))
			return CDN_ERR;
		if (cdn_apb_write(0x1c008E << 2, set_pll2_CLKOUT5_L))
			return CDN_ERR;
		if (cdn_apb_write(0x1c008F << 2, set_pll2_CLKOUT5_H))
			return CDN_ERR;
		if (cdn_apb_write(0x0c0001 << 2,
				  ((1) + (2 * set_CLK_SEL) + (16 * 0) +
				   (32 * 0) + (64 * 3) + (65536 * 3) +
				   (1048576 * set_REF_CLK_SEL))))
			return CDN_ERR;
	}

	if (cdn_apb_write((ADDR_AVGEN + HDMIPOL) << 2, v_h_polarity))
		return CDN_ERR;
	if (cdn_apb_write((ADDR_AVGEN + HDMI_FRONT_PORCHE_L) << 2,
			  front_porche_l))
		return CDN_ERR;
	if (cdn_apb_write((ADDR_AVGEN + HDFP) << 2, front_porche_h))
		return CDN_ERR;
	if (cdn_apb_write((ADDR_AVGEN + HDBP) << 2, back_porche_l))
		return CDN_ERR;
	if (cdn_apb_write((ADDR_AVGEN + HDMI_BACK_PORCHE_H) << 2,
			  back_porche_h))
		return CDN_ERR;
	if (cdn_apb_write((ADDR_AVGEN + HDAS) << 2, active_slot_l))
		return CDN_ERR;
	if (cdn_apb_write((ADDR_AVGEN + HDMI_ACTIVE_SLOT_H) << 2,
			  active_slot_h))
		return CDN_ERR;
	if (cdn_apb_write((ADDR_AVGEN + HDFL) << 2, frame_lines_l))
		return CDN_ERR;
	if (cdn_apb_write((ADDR_AVGEN + HDMI_FRAME_LINES_H) << 2,
			  frame_lines_h))
		return CDN_ERR;
	if (cdn_apb_write((ADDR_AVGEN + HDLW) << 2, line_width_l))
		return CDN_ERR;
	if (cdn_apb_write((ADDR_AVGEN + HDMI_LINE_WIDTH_H) << 2, line_width_h))
		return CDN_ERR;
	if (cdn_apb_write((ADDR_AVGEN + HDVL) << 2, vsync_lines))
		return CDN_ERR;
	if (cdn_apb_write((ADDR_AVGEN + HDEL) << 2, eof_lines))
		return CDN_ERR;
	if (cdn_apb_write((ADDR_AVGEN + HDSL) << 2, sof_lines))
		return CDN_ERR;
	if (cdn_apb_write((ADDR_AVGEN + PTRNGENFF) << 2, interlace_progressive))
		return CDN_ERR;

	if (protocol == CDN_HDMITX_TYPHOON) {
		switch (format) {
		case PXL_RGB:

			if (cdn_apb_write((ADDR_AVGEN + PGENCTRL_H) << 2,
					  F_PIC_SEL(1) | F_PIC_YCBCR_SEL(0)))
				return CDN_ERR;
			break;

		case YCBCR_4_4_4:
			if (cdn_apb_write((ADDR_AVGEN + PGENCTRL_H) << 2,
					  F_PIC_SEL(2) | F_PIC_YCBCR_SEL(0)))
				return CDN_ERR;

			break;

		case YCBCR_4_2_2:
			if (cdn_apb_write((ADDR_AVGEN + PGENCTRL_H) << 2,
					  F_PIC_SEL(2) | F_PIC_YCBCR_SEL(1)))
				return CDN_ERR;

			break;

		case YCBCR_4_2_0:
			if (cdn_apb_write((ADDR_AVGEN + PGENCTRL_H) << 2,
					  F_PIC_SEL(2) | F_PIC_YCBCR_SEL(2)))
				return CDN_ERR;

			break;
		case Y_ONLY:
			/*not exist in hdmi */
			break;
		}
	} else {
		if (set_vif_clock != 0)
			if (cdn_apb_write(0xC0006 << 2, set_vif_clock))
				return CDN_ERR;
	}

	if (cdn_apb_write((ADDR_AVGEN + PGENCTRL) << 2, start_pgen))
		return CDN_ERR;

	return CDN_OK;
}

