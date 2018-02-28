/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <asm/imx-common/video.h>
#include <asm/arch/video_common.h>
#include <imx8_hdmi.h>

#include "API_General.h"
#include "vic_table.h"
#include "API_HDMITX.h"
#include "apb_cfg.h"
#include "externs.h"
#include "API_AVI.h"
#include "address.h"
#include "source_car.h"
#include "source_phy.h"
#include "API_AFE.h"
#include "source_vif.h"
#include "general_handler.h"
#include "mhl_hdtx_top.h"


#ifdef CONFIG_IMX8QM
#include "API_AFE_mcu1_dp.h"
#include "API_AFE_ss28fdsoi_kiran_hdmitx.h"
#endif

#ifdef CONFIG_IMX8M
#include "API_AFE_t28hpc_hdmitx.h"
#endif

DECLARE_GLOBAL_DATA_PTR;

#define ON  1
#define OFF 0

unsigned long g_encoding = 1;    /* 1 RGB, 2 YUV 444, 4 YUV 422, 8 YUV 420 */
unsigned long g_color_depth = 8; /* 8 pits per color */

static int imx8_hdmi_set_vic_mode(int vic,
				  struct video_mode_settings *vms)
{
	/*struct video_mode_settings *vms = &vm_settings[VM_USER]; */
	uint32_t pixel_clock_kHz;
	uint32_t frame_rate_Hz;
	uint32_t frame_rate_frac_Hz;
	uint32_t cea_vic;
	char iflag;

	if (vic >= VIC_MODE_COUNT) {
		debug("%s(): unsupported VIC\n", __func__);
		return -1;
	}


	vms->hfp   = vic_table[vic][FRONT_PORCH];
	vms->hbp   = vic_table[vic][BACK_PORCH];
	vms->hsync = vic_table[vic][HSYNC];
	vms->vfp   = vic_table[vic][TYPE_EOF];
	vms->vbp   = vic_table[vic][SOF];
	vms->vsync = vic_table[vic][VSYNC];
	vms->xres  = vic_table[vic][H_ACTIVE];
	vms->yres  = vic_table[vic][V_ACTIVE];

	vms->hpol = vic_table[vic][HSYNC_POL] != 0;
	vms->vpol = vic_table[vic][VSYNC_POL] != 0;

	cea_vic = vic_table[vic][VIC];
	if (vic_table[vic][I_P] != 0)
		iflag = 'i';
	else
		iflag = 'p';
	pixel_clock_kHz    = vic_table[vic][PIXEL_FREQ_KHZ];
	frame_rate_Hz      = vic_table[vic][V_FREQ_HZ] * 1000;
	frame_rate_frac_Hz = frame_rate_Hz % 1000;
	frame_rate_Hz      /= 1000;

	vms->pixelclock = pixel_clock_kHz;

	debug("Cadence VIC %3d, CEA VIC %3d: %4d x %4d %c @ %3d.%03d [%6d kHz] Vpol=%d Hpol=%d\n",
	      vic, cea_vic, vms->xres, vms->yres, iflag, frame_rate_Hz,
	      frame_rate_frac_Hz, pixel_clock_kHz, vms->vpol, vms->hpol);

	debug(" mode timing fp sync bp h:%3d %3d %3d v:%3d %3d %3d\n",
	      vms->hfp, vms->hsync, vms->hbp, vms->vfp, vms->vsync, vms->vbp);

	return 0;
	/*debug("leaving %s() ...\n", __func__); */
}

static int imx8_hdmi_init(int vic,
			  int encoding,
			  int color_depth,
			  bool pixel_clk_from_phy)
{
	int ret;
#ifdef CONFIG_IMX8QM
	sc_ipc_t ipcHndl = gd->arch.ipc_channel_handle;
	void __iomem *hdmi_csr_base = (void __iomem *)0x56261000;
#endif
	/*GENERAL_Read_Register_response regresp; */
	/*uint8_t sts; */
	uint32_t character_freq_khz;

	uint8_t echo_msg[] = "echo test";
	uint8_t echo_resp[sizeof(echo_msg) + 1];
	/*uint8_t response; */
	/*uint8_t dpcd_resp; */
	/*uint8_t hdcp_resp; */
	/*uint8_t capb_resp; */
	/*uint32_t temp; */

	/*================================================================== */
	/* Parameterization: */
	/*================================================================== */

	/* VIC Mode - index from vic_table (see API_SRC/vic_table.c) */
	VIC_MODES vic_mode = vic;

	/* Pixel Encodeing Format */
	/*   PXL_RGB = 0x1, */
	/*   YCBCR_4_4_4 = 0x2, */
	/*   YCBCR_4_2_2 = 0x4, */
	/*   YCBCR_4_2_0 = 0x8, */
	/*   Y_ONLY = 0x10, */
	VIC_PXL_ENCODING_FORMAT format = encoding;
	/*VIC_PXL_ENCODING_FORMAT format = 1; */

	/* B/W Balance Type: 0 no data, 1 IT601, 2 ITU709 */
	BT_TYPE bw_type = 0;

	/* bpp (bits per subpixel) - 8 24bpp, 10 30bpp, 12 36bpp, 16 48bpp */
	uint8_t bps = color_depth;

	/* Set HDMI TX Mode */
	/* Mode = 0 - DVI, 1 - HDMI1.4, 2 HDMI 2.0 */
	HDMI_TX_MAIL_HANDLER_PROTOCOL_TYPE ptype = 1;

	if (vic_mode == VIC_MODE_97_60Hz)
		ptype = 2;

	/*================================================================== */
	/* Parameterization done */
	/*================================================================== */
#ifdef CONFIG_IMX8QM
	/* set the pixel link mode and pixel type */
	SC_MISC_SET_CONTROL(ipcHndl, SC_R_HDMI, SC_C_PHY_RESET, 0);
#if 1
	SC_MISC_SET_CONTROL(ipcHndl, SC_R_DC_0, SC_C_PXL_LINK_MST1_ADDR, 1);
	/*SC_MISC_SET_CONTROL(ipcHndl, SC_R_DC_0, SC_C_PXL_LINK_MST1_ADDR, 0);*/
	if (g_clock_mode == CLOCK_MODES_HDMI_DUAL) {
		SC_MISC_SET_CONTROL(ipcHndl, SC_R_DC_0,
				    SC_C_PXL_LINK_MST2_ADDR, 2);
		/*SC_MISC_SET_CONTROL(ipcHndl, SC_R_DC_0,
		  SC_C_PXL_LINK_MST2_ADDR, 0); */
		__raw_writel(0x6, hdmi_csr_base);
	} else
#endif
		__raw_writel(0x34, hdmi_csr_base);
#endif
	cdn_api_init();
	debug("CDN_API_Init completed\n");

	ret = cdn_api_checkalive();
	debug("CDN_API_CheckAlive returned ret = %d\n", ret);

	if (ret)
		return -EPERM;

	ret = cdn_api_general_test_echo_ext_blocking(echo_msg,
						     echo_resp,
						     sizeof(echo_msg),
						     CDN_BUS_TYPE_APB);
	debug("_General_Test_Echo_Ext_blocking - (ret = %d echo_resp = %s)\n",
	      ret, echo_resp);

	/* Configure PHY */
	character_freq_khz = phy_cfg_t28hpc(4, vic_mode, bps,
					    format, pixel_clk_from_phy);
	debug("phy_cfg_t28hpc (character_freq_mhz = %d)\n",
	      character_freq_khz);

	/*phy_reset(1); */

#ifdef CONFIG_IMX8QM
	SC_MISC_SET_CONTROL(ipcHndl, SC_R_HDMI, SC_C_PHY_RESET, 1);
#endif
	hdmi_tx_t28hpc_power_config_seq(4);
#ifdef CONFIG_IMX8QM
	/* Set the lane swapping */
	ret = cdn_api_general_write_register_blocking
		(ADDR_SOURCD_PHY + (LANES_CONFIG << 2),
		 F_SOURCE_PHY_LANE0_SWAP(3) | F_SOURCE_PHY_LANE1_SWAP(0) |
		 F_SOURCE_PHY_LANE2_SWAP(1) | F_SOURCE_PHY_LANE3_SWAP(2) |
		 F_SOURCE_PHY_COMB_BYPASS(0) | F_SOURCE_PHY_20_10(1));
#else
	/* Set the lane swapping */
	ret = cdn_api_general_write_register_blocking
		(ADDR_SOURCD_PHY + (LANES_CONFIG << 2),
		 F_SOURCE_PHY_LANE0_SWAP(0) | F_SOURCE_PHY_LANE1_SWAP(1) |
		 F_SOURCE_PHY_LANE2_SWAP(2) | F_SOURCE_PHY_LANE3_SWAP(3) |
		 F_SOURCE_PHY_COMB_BYPASS(0) | F_SOURCE_PHY_20_10(1));
#endif
	debug("_General_Write_Register_blocking LANES_CONFIG ret = %d\n", ret);

	ret = CDN_API_HDMITX_Init_blocking();
	debug("CDN_API_STATUS CDN_API_HDMITX_Init_blocking  ret = %d\n", ret);

	ret = CDN_API_HDMITX_Init_blocking();
	debug("CDN_API_STATUS CDN_API_HDMITX_Init_blocking  ret = %d\n", ret);

	ret = CDN_API_HDMITX_Set_Mode_blocking(ptype, character_freq_khz);
	debug("CDN_API_HDMITX_Set_Mode_blocking ret = %d\n", ret);

	ret = cdn_api_set_avi(vic_mode, format, bw_type);
	debug("cdn_api_set_avi  ret = %d\n", ret);

	ret =  CDN_API_HDMITX_SetVic_blocking(vic_mode, bps, format);
	debug("CDN_API_HDMITX_SetVic_blocking ret = %d\n", ret);

#ifdef CONFIG_IMX8QM
	{
		GENERAL_Read_Register_response regresp;
		/* adjust the vsync/hsync polarity */
		cdn_api_general_read_register_blocking(ADDR_SOURCE_VIF +
						       (HSYNC2VSYNC_POL_CTRL
							<< 2),
						       &regresp);
		debug("Initial HSYNC2VSYNC_POL_CTRL: 0x%x\n", regresp.val);
		if ((regresp.val & 0x3) != 0)
			__raw_writel(0x4, hdmi_csr_base);
	}
#endif
	/*regresp.val &= ~0x03; // clear HSP and VSP bits */
	/*debug("Final HSYNC2VSYNC_POL_CTRL: 0x%x\n",regresp.val); */
	/*CDN_API_General_Write_Register_blocking(ADDR_DPTX_FRAMER +
						(DP_FRAMER_SP << 2),
						regresp.val); */

	udelay(20000);

	return 0;
}

int imx8_hdmi_enable(int encoding,
		      struct video_mode_settings *vms)
{
	int vic = 0;
	const int use_phy_pixel_clk = 1;

	/* map the resolution to a VIC index in the vic table*/
	if ((vms->xres == 1280) && (vms->yres == 720))
		vic = 1; /* 720p60 */
	else if ((vms->xres == 1920) && (vms->yres == 1080))
		vic = 2; /* 1080p60 */
	else if ((vms->xres == 3840) && (vms->yres == 2160))
		vic = 3; /* 2160p60 */
	else /* if  ((vms->xres == 720) && (vms->yres == 480)) */
		vic = 0; /* 480p60 */

	imx8_hdmi_set_vic_mode(vic, vms);
	return imx8_hdmi_init(vic, encoding, g_color_depth, use_phy_pixel_clk);
}

void imx8_hdmi_disable(void)
{
	int ret;
	GENERAL_READ_REGISTER_RESPONSE resp;

	resp.val = 0;
	ret = cdn_api_general_read_register_blocking(ADDR_SOURCE_MHL_HD +
						     (HDTX_CONTROLLER << 2),
						     &resp);
	if (ret != CDN_OK) {
		printf("%s(): dn_api_general_read_register_blocking failed\n",
		       __func__);
		/*return;*/
	}

	resp.val &= ~F_DATA_EN(1); /* disable HDMI */
	/*resp.val |= F_SET_AVMUTE( 1);*/

	ret = cdn_api_general_write_register_blocking(ADDR_SOURCE_MHL_HD +
						      (HDTX_CONTROLLER << 2),
						      resp.val);
	if (ret != CDN_OK) {
		printf("%s(): dn_api_general_write_register_blocking failed\n",
		       __func__);
		return;
	}
}
