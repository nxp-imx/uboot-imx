/******************************************************************************
 *
 * Copyright (C) 2017 Cadence Design Systems, Inc.
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
 * This file was auto-generated. Do not edit it manually.
 *
 ******************************************************************************
 *
 * avgen.h
 *
 ******************************************************************************
 */

#ifndef AVGEN_H_
# define AVGEN_H_


/* register HDMIPOL */
# define HDMIPOL 0
# define F_HDMI_V_H_POLARITY(x) (((x) & ((1 << 2) - 1)) << 0)
# define F_HDMI_V_H_POLARITY_RD(x) (((x) & (((1 << 2) - 1) << 0)) >> 0)
# define F_HDMI_BITWIDTH(x) (((x) & ((1 << 2) - 1)) << 2)
# define F_HDMI_BITWIDTH_RD(x) (((x) & (((1 << 2) - 1) << 2)) >> 2)

/* register HDMI_FRONT_PORCHE_L */
# define HDMI_FRONT_PORCHE_L 1
# define F_HDMI_FRONT_PORCHE_L(x) (((x) & ((1 << 8) - 1)) << 0)
# define F_HDMI_FRONT_PORCHE_L_RD(x) (((x) & (((1 << 8) - 1) << 0)) >> 0)

/* register HDFP */
# define HDFP 2
# define F_HDMI_FRONT_PORCHE_H(x) (((x) & ((1 << 8) - 1)) << 0)
# define F_HDMI_FRONT_PORCHE_H_RD(x) (((x) & (((1 << 8) - 1) << 0)) >> 0)

/* register HDBP */
# define HDBP 3
# define F_HDMI_BACK_PORCHE_L(x) (((x) & ((1 << 8) - 1)) << 0)
# define F_HDMI_BACK_PORCHE_L_RD(x) (((x) & (((1 << 8) - 1) << 0)) >> 0)

/* register HDMI_BACK_PORCHE_H */
# define HDMI_BACK_PORCHE_H 4
# define F_HDMI_BACK_PORCHE_H(x) (((x) & ((1 << 8) - 1)) << 0)
# define F_HDMI_BACK_PORCHE_H_RD(x) (((x) & (((1 << 8) - 1) << 0)) >> 0)

/* register HDAS */
# define HDAS 5
# define F_HDMI_ACTIVE_SLOT_L(x) (((x) & ((1 << 8) - 1)) << 0)
# define F_HDMI_ACTIVE_SLOT_L_RD(x) (((x) & (((1 << 8) - 1) << 0)) >> 0)

/* register HDMI_ACTIVE_SLOT_H */
# define HDMI_ACTIVE_SLOT_H 6
# define F_HDMI_ACTIVE_SLOT_H(x) (((x) & ((1 << 8) - 1)) << 0)
# define F_HDMI_ACTIVE_SLOT_H_RD(x) (((x) & (((1 << 8) - 1) << 0)) >> 0)

/* register HDFL */
# define HDFL 7
# define F_HDMI_FRAME_LINES_L(x) (((x) & ((1 << 8) - 1)) << 0)
# define F_HDMI_FRAME_LINES_L_RD(x) (((x) & (((1 << 8) - 1) << 0)) >> 0)

/* register HDMI_FRAME_LINES_H */
# define HDMI_FRAME_LINES_H 8
# define F_HDMI_FRAME_LINES_H(x) (((x) & ((1 << 8) - 1)) << 0)
# define F_HDMI_FRAME_LINES_H_RD(x) (((x) & (((1 << 8) - 1) << 0)) >> 0)

/* register HDLW */
# define HDLW 9
# define F_HDMI_LINE_WIDTH_L(x) (((x) & ((1 << 8) - 1)) << 0)
# define F_HDMI_LINE_WIDTH_L_RD(x) (((x) & (((1 << 8) - 1) << 0)) >> 0)

/* register HDMI_LINE_WIDTH_H */
# define HDMI_LINE_WIDTH_H 10
# define F_HDMI_LINE_WIDTH_H(x) (((x) & ((1 << 8) - 1)) << 0)
# define F_HDMI_LINE_WIDTH_H_RD(x) (((x) & (((1 << 8) - 1) << 0)) >> 0)

/* register HDVL */
# define HDVL 11
# define F_HDMI_VSYNC_LINES(x) (((x) & ((1 << 7) - 1)) << 0)
# define F_HDMI_VSYNC_LINES_RD(x) (((x) & (((1 << 7) - 1) << 0)) >> 0)

/* register HDEL */
# define HDEL 12
# define F_HDMI_EOF_LINES(x) (((x) & ((1 << 7) - 1)) << 0)
# define F_HDMI_EOF_LINES_RD(x) (((x) & (((1 << 7) - 1) << 0)) >> 0)

/* register HDSL */
# define HDSL 13
# define F_HDMI_SOF_LINES(x) (((x) & ((1 << 7) - 1)) << 0)
# define F_HDMI_SOF_LINES_RD(x) (((x) & (((1 << 7) - 1) << 0)) >> 0)

/* register HDCFUPDT */
# define HDCFUPDT 14
# define F_HDMI_CODE_FORMAT_UPDT(x) (((x) & ((1 << 6) - 1)) << 0)
# define F_HDMI_CODE_FORMAT_UPDT_RD(x) (((x) & (((1 << 6) - 1) << 0)) >> 0)

/* register HDCF */
# define HDCF 15
# define F_HDMI_CODE_FORMAT(x) (((x) & ((1 << 6) - 1)) << 0)
# define F_HDMI_CODE_FORMAT_RD(x) (((x) & (((1 << 6) - 1) << 0)) >> 0)

/* register HDASPACE */
# define HDASPACE 16
# define F_HDASPACE(x) (((x) & ((1 << 8) - 1)) << 0)
# define F_HDASPACE_RD(x) (((x) & (((1 << 8) - 1) << 0)) >> 0)

/* register HDMI_3D_MODE */
# define HDMI_3D_MODE 17
# define F_HDMI_3D_MODE(x) (((x) & ((1 << 3) - 1)) << 0)
# define F_HDMI_3D_MODE_RD(x) (((x) & (((1 << 3) - 1) << 0)) >> 0)

/* register PTRNGENR */
# define PTRNGENR 18
# define F_PTRNGENR_L(x) (((x) & ((1 << 8) - 1)) << 0)
# define F_PTRNGENR_L_RD(x) (((x) & (((1 << 8) - 1) << 0)) >> 0)

/* register PTRNGENR_H */
# define PTRNGENR_H 19
# define F_PTRNGENR_H(x) (((x) & ((1 << 8) - 1)) << 0)
# define F_PTRNGENR_H_RD(x) (((x) & (((1 << 8) - 1) << 0)) >> 0)

/* register PTRNGENG */
# define PTRNGENG 20
# define F_PTRNGENG_L(x) (((x) & ((1 << 8) - 1)) << 0)
# define F_PTRNGENG_L_RD(x) (((x) & (((1 << 8) - 1) << 0)) >> 0)

/* register PTRNEGENG_H */
# define PTRNEGENG_H 21
# define F_PTRNGENG_H(x) (((x) & ((1 << 8) - 1)) << 0)
# define F_PTRNGENG_H_RD(x) (((x) & (((1 << 8) - 1) << 0)) >> 0)

/* register PTRNGENB */
# define PTRNGENB 22
# define F_PTRNGENB_L(x) (((x) & ((1 << 8) - 1)) << 0)
# define F_PTRNGENB_L_RD(x) (((x) & (((1 << 8) - 1) << 0)) >> 0)

/* register PTRGENB */
# define PTRGENB 23
# define F_PTRNGENB_H(x) (((x) & ((1 << 8) - 1)) << 0)
# define F_PTRNGENB_H_RD(x) (((x) & (((1 << 8) - 1) << 0)) >> 0)

/* register PTRNGENFF */
# define PTRNGENFF 30
# define F_PTRNGENIP(x) (((x) & ((1 << 1) - 1)) << 1)
# define F_PTRNGENIP_RD(x) (((x) & (((1 << 1) - 1) << 1)) >> 1)

/* register PGENCTRL */
# define PGENCTRL 32
# define F_PGENCF(x) (((x) & ((1 << 6) - 1)) << 1)
# define F_PGENCF_RD(x) (((x) & (((1 << 6) - 1) << 1)) >> 1)
# define F_PTRNGENSTRT(x) (((x) & ((1 << 1) - 1)) << 7)
# define F_PTRNGENSTRT_RD(x) (((x) & (((1 << 1) - 1) << 7)) >> 7)

/* register PGENCTRL_H */
# define PGENCTRL_H 33
# define F_PTRNGENRST(x) (((x) & ((1 << 1) - 1)) << 0)
# define F_PTRNGENRST_RD(x) (((x) & (((1 << 1) - 1) << 0)) >> 0)
# define F_PIC_SEL(x) (((x) & ((1 << 3) - 1)) << 1)
# define F_PIC_SEL_RD(x) (((x) & (((1 << 3) - 1) << 1)) >> 1)
# define F_PIC_YCBCR_SEL(x) (((x) & ((1 << 2) - 1)) << 4)
# define F_PIC_YCBCR_SEL_RD(x) (((x) & (((1 << 2) - 1) << 4)) >> 4)

/* register PGEN_COLOR_BAR_CTRL */
# define PGEN_COLOR_BAR_CTRL 34
# define F_PGEN_NUM_BAR(x) (((x) & ((1 << 3) - 1)) << 0)
# define F_PGEN_NUM_BAR_RD(x) (((x) & (((1 << 3) - 1) << 0)) >> 0)

/* register PGEN_COLOR_BAR_CONTROL_H */
# define PGEN_COLOR_BAR_CONTROL_H 35
# define F_PGEN_COLOR_UPDT(x) (((x) & ((1 << 6) - 1)) << 0)
# define F_PGEN_COLOR_UPDT_RD(x) (((x) & (((1 << 6) - 1) << 0)) >> 0)

/* register GEN_AUDIO_CONTROL */
# define GEN_AUDIO_CONTROL 36
# define F_AUDIO_START(x) (((x) & ((1 << 1) - 1)) << 1)
# define F_AUDIO_START_RD(x) (((x) & (((1 << 1) - 1) << 1)) >> 1)
# define F_AUDIO_RESET(x) (((x) & ((1 << 1) - 1)) << 2)
# define F_AUDIO_RESET_RD(x) (((x) & (((1 << 1) - 1) << 2)) >> 2)

/* register SPDIF_CTRL_A */
# define SPDIF_CTRL_A 37
# define F_SPDIF_SOURCE_NUM(x) (((x) & ((1 << 4) - 1)) << 0)
# define F_SPDIF_SOURCE_NUM_RD(x) (((x) & (((1 << 4) - 1) << 0)) >> 0)
# define F_SPDIF_CH_NUM(x) (((x) & ((1 << 4) - 1)) << 4)
# define F_SPDIF_CH_NUM_RD(x) (((x) & (((1 << 4) - 1) << 4)) >> 4)

/* register SPDIF_CTRL_A_H */
# define SPDIF_CTRL_A_H 38
# define F_SPDIF_SMP_FREQ(x) (((x) & ((1 << 4) - 1)) << 0)
# define F_SPDIF_SMP_FREQ_RD(x) (((x) & (((1 << 4) - 1) << 0)) >> 0)
# define F_SPDIF_CLK_ACCUR(x) (((x) & ((1 << 2) - 1)) << 4)
# define F_SPDIF_CLK_ACCUR_RD(x) (((x) & (((1 << 2) - 1) << 4)) >> 4)
# define F_SPDIF_VALID(x) (((x) & ((1 << 1) - 1)) << 6)
# define F_SPDIF_VALID_RD(x) (((x) & (((1 << 1) - 1) << 6)) >> 6)

/* register SPDIF_CTRL_B */
# define SPDIF_CTRL_B 39
# define F_SPDIF_WORD_LENGTH(x) (((x) & ((1 << 4) - 1)) << 0)
# define F_SPDIF_WORD_LENGTH_RD(x) (((x) & (((1 << 4) - 1) << 0)) >> 0)
# define F_SPDIF_ORG_SMP_FREQ(x) (((x) & ((1 << 4) - 1)) << 4)
# define F_SPDIF_ORG_SMP_FREQ_RD(x) (((x) & (((1 << 4) - 1) << 4)) >> 4)

/* register SPDIF_CTRL_B_H */
# define SPDIF_CTRL_B_H 40
# define F_CATEGORY_MODE(x) (((x) & ((1 << 8) - 1)) << 0)
# define F_CATEGORY_MODE_RD(x) (((x) & (((1 << 8) - 1) << 0)) >> 0)

/* register AUDIO_DIV_EN */
# define AUDIO_DIV_EN 45
# define F_AGEN_60958_I2S(x) (((x) & ((1 << 1) - 1)) << 1)
# define F_AGEN_60958_I2S_RD(x) (((x) & (((1 << 1) - 1) << 1)) >> 1)
# define F_AGEN_PRL_SUBFRAME(x) (((x) & ((1 << 1) - 1)) << 2)
# define F_AGEN_PRL_SUBFRAME_RD(x) (((x) & (((1 << 1) - 1) << 2)) >> 2)
# define F_AGEN_SAMPLES_DATA(x) (((x) & ((1 << 1) - 1)) << 3)
# define F_AGEN_SAMPLES_DATA_RD(x) (((x) & (((1 << 1) - 1) << 3)) >> 3)

#endif /*AVGEN */

