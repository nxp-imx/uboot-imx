// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 *
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <display.h>
#include <video.h>
#include <asm/io.h>
#include <dm/device-internal.h>
#include <linux/iopoll.h>
#include <clk.h>
#include <video_link.h>

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
#include "API_AFE_t28hpc_hdmitx.h"

struct imx8m_hdmi_priv {
	fdt_addr_t base;
	struct display_timing timings;
	int vic;
	bool hpol;
	bool vpol;
};

static int imx8m_hdmi_set_vic_mode(int vic,
				  struct imx8m_hdmi_priv *priv)
{
	uint32_t pixel_clock_kHz;
	uint32_t frame_rate_Hz;
	uint32_t frame_rate_frac_Hz;
	uint32_t cea_vic;
	char iflag;

	if (vic >= VIC_MODE_COUNT) {
		debug("%s(): unsupported VIC\n", __func__);
		return -1;
	}

	priv->timings.hfront_porch.typ = vic_table[vic][FRONT_PORCH];
	priv->timings.hback_porch.typ = vic_table[vic][BACK_PORCH];
	priv->timings.hsync_len.typ = vic_table[vic][HSYNC];
	priv->timings.vfront_porch.typ = vic_table[vic][TYPE_EOF];
	priv->timings.vback_porch.typ = vic_table[vic][SOF];
	priv->timings.vsync_len.typ = vic_table[vic][VSYNC];
	priv->timings.hactive.typ = vic_table[vic][H_ACTIVE];
	priv->timings.vactive.typ = vic_table[vic][V_ACTIVE];

	priv->hpol = vic_table[vic][HSYNC_POL] != 0;
	priv->vpol = vic_table[vic][VSYNC_POL] != 0;

	cea_vic = vic_table[vic][VIC];
	if (vic_table[vic][I_P] != 0)
		iflag = 'i';
	else
		iflag = 'p';
	pixel_clock_kHz    = vic_table[vic][PIXEL_FREQ_KHZ];
	frame_rate_Hz      = vic_table[vic][V_FREQ_HZ] * 1000;
	frame_rate_frac_Hz = frame_rate_Hz % 1000;
	frame_rate_Hz      /= 1000;

	priv->timings.pixelclock.typ = pixel_clock_kHz * 1000;

	debug("Cadence VIC %3d, CEA VIC %3d: %4d x %4d %c @ %3d.%03d [%6d kHz] Vpol=%d Hpol=%d\n",
	      vic, cea_vic, priv->timings.hactive.typ, priv->timings.vactive.typ, iflag, frame_rate_Hz,
	      frame_rate_frac_Hz, pixel_clock_kHz, priv->vpol, priv->hpol);

	debug(" mode timing fp sync bp h:%3d %3d %3d v:%3d %3d %3d\n",
	      priv->timings.hfront_porch.typ, priv->timings.hsync_len.typ, priv->timings.hback_porch.typ,
	      priv->timings.vfront_porch.typ, priv->timings.vsync_len.typ, priv->timings.vback_porch.typ);

	return 0;
}

static int imx8m_hdmi_init(int vic,
			  int encoding,
			  int color_depth,
			  bool pixel_clk_from_phy)
{
	int ret;
	uint32_t character_freq_khz;

	uint8_t echo_msg[] = "echo test";
	uint8_t echo_resp[sizeof(echo_msg) + 1];

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

	hdmi_tx_t28hpc_power_config_seq(4);

	/* Set the lane swapping */
	ret = cdn_api_general_write_register_blocking
		(ADDR_SOURCD_PHY + (LANES_CONFIG << 2),
		 F_SOURCE_PHY_LANE0_SWAP(0) | F_SOURCE_PHY_LANE1_SWAP(1) |
		 F_SOURCE_PHY_LANE2_SWAP(2) | F_SOURCE_PHY_LANE3_SWAP(3) |
		 F_SOURCE_PHY_COMB_BYPASS(0) | F_SOURCE_PHY_20_10(1));

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


	udelay(20000);

	return 0;
}

static int imx8m_hdmi_update_timings(struct udevice *dev)
{
	struct imx8m_hdmi_priv *priv = dev_get_priv(dev);

	/* map the resolution to a VIC index in the vic table*/
	if ((priv->timings.hactive.typ == 1280) && (priv->timings.vactive.typ == 720))
		priv->vic = 1; /* 720p60 */
	else if ((priv->timings.hactive.typ == 1920) && (priv->timings.vactive.typ == 1080))
		priv->vic = 2; /* 1080p60 */
	else if ((priv->timings.hactive.typ == 3840) && (priv->timings.vactive.typ == 2160))
		priv->vic = 3; /* 2160p60 */
	else
		priv->vic = 0; /* 480p60 */

	return imx8m_hdmi_set_vic_mode(priv->vic, priv);
}

static void imx8m_hdmi_disable(void)
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
	}

	resp.val &= ~F_DATA_EN(1); /* disable HDMI */

	ret = cdn_api_general_write_register_blocking(ADDR_SOURCE_MHL_HD +
						      (HDTX_CONTROLLER << 2),
						      resp.val);
	if (ret != CDN_OK) {
		printf("%s(): dn_api_general_write_register_blocking failed\n",
		       __func__);
		return;
	}
}

static int imx8m_hdmi_read_timing(struct udevice *dev, struct display_timing *timing)
{
	struct imx8m_hdmi_priv *priv = dev_get_priv(dev);

	if (timing) {
		memcpy(timing, &priv->timings, sizeof(struct display_timing));

		if (priv->hpol)
			timing->flags |= DISPLAY_FLAGS_HSYNC_HIGH;

		if (priv->vpol)
			timing->flags |= DISPLAY_FLAGS_VSYNC_HIGH;

		return 0;
	}

	return -EINVAL;
}

static int imx8m_hdmi_enable(struct udevice *dev, int panel_bpp,
		      const struct display_timing *timing)
{
	struct imx8m_hdmi_priv *priv = dev_get_priv(dev);
	int ret;

	ret = imx8m_hdmi_init(priv->vic, 1, 8, true);
	if (ret) {
		printf("HDMI enable failed, ret %d!\n", ret);
		return ret;
	}

	return 0;
}

static int imx8m_hdmi_probe(struct udevice *dev)
{
	struct imx8m_hdmi_priv *priv = dev_get_priv(dev);
	int ret;

	printf("%s\n", __func__);

	priv->base = dev_read_addr(dev);
	if (priv->base == FDT_ADDR_T_NONE)
		return -EINVAL;

	ret = video_link_get_display_timings(&priv->timings);
	if (ret) {
		printf("decode display timing error %d\n", ret);
		return ret;
	}

	imx8m_hdmi_update_timings(dev);

	return 0;
}

static int imx8m_hdmi_remove(struct udevice *dev)
{
	imx8m_hdmi_disable();

	return 0;
}

struct dm_display_ops imx8m_hdmi_ops = {
	.read_timing =  imx8m_hdmi_read_timing,
	.enable =  imx8m_hdmi_enable,
};

static const struct udevice_id  imx8m_hdmi_ids[] = {
	{ .compatible = "fsl,imx8mq-hdmi" },
	{ }
};

U_BOOT_DRIVER( imx8m_hdmi) = {
	.name				= " imx8m_hdmi",
	.id				= UCLASS_DISPLAY,
	.of_match			=  imx8m_hdmi_ids,
	.bind				= dm_scan_fdt_dev,
	.probe				=  imx8m_hdmi_probe,
	.remove				= imx8m_hdmi_remove,
	.ops				= & imx8m_hdmi_ops,
	.priv_auto_alloc_size		= sizeof(struct  imx8m_hdmi_priv),
};
