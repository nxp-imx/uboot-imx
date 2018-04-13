/*
 * Copyright 2017-2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/imx-common/sci/sci.h>
#include <i2c.h>
#include <asm/arch/sys_proto.h>

#include <imxdpuv1.h>
#include <imxdpuv1_registers.h>
#include <imxdpuv1_events.h>
#include <asm/arch/imx8_lvds.h>
#include <video_fb.h>
#include <asm/arch/imx8_mipi_dsi.h>
#include <asm/arch/video_common.h>
#include <power-domain.h>

DECLARE_GLOBAL_DATA_PTR;

static struct imxdpuv1_videomode gmode;
static int8_t gdisp, gdc;
static uint32_t gpixfmt;
static GraphicDevice panel;

static int hdmi_i2c_reg_write(struct udevice *dev, uint addr, uint mask, uint data)
{
	uint8_t valb;
	int err;

	if (mask != 0xff) {
		err = dm_i2c_read(dev, addr, &valb, 1);
		if (err)
			return err;

		valb &= ~mask;
		valb |= data;
	} else {
		valb = data;
	}

	err = dm_i2c_write(dev, addr, &valb, 1);
	return err;
}

static int hdmi_i2c_reg_read(struct udevice *dev, uint8_t addr, uint8_t *data)
{
	uint8_t valb;
	int err;

	err = dm_i2c_read(dev, addr, &valb, 1);
	if (err)
		return err;

	*data = (int)valb;
	return 0;
}

/* On 8QXP ARM2, the LVDS1 signals are connected to LVDS2HDMI card's LVDS2 channel,
 *  LVDS0 signals are connected to LVDS2HDMI card's LVDS4 channel.
 *  There totally 6 channels on the cards, from 0-5.
 */
int lvds2hdmi_setup(int i2c_bus)
{
	struct udevice *bus, *dev;
	uint8_t chip = 0x4c;
	uint8_t data;
	int ret;

	ret = uclass_get_device_by_seq(UCLASS_I2C, i2c_bus, &bus);
	if (ret) {
		printf("%s: No bus %d\n", __func__, i2c_bus);
		return ret;
	}

	ret = dm_i2c_probe(bus, chip, 0, &dev);
	if (ret) {
		printf("%s: Can't find device id=0x%x, on bus %d\n",
			__func__, chip, i2c_bus);
		return ret;
	}

	/* InitIT626X(): start */
	hdmi_i2c_reg_write(dev, 0x04, 0xff, 0x3d);
	hdmi_i2c_reg_write(dev, 0x0f, 0xff, 0x00);
	hdmi_i2c_reg_write(dev, 0x05, 0xff, 0x40);
	hdmi_i2c_reg_write(dev, 0x04, 0xff, 0x15);
	hdmi_i2c_reg_write(dev, 0x0f, 0xff, 0x00);
	hdmi_i2c_reg_write(dev, 0x1d, 0xff, 0x66);
	hdmi_i2c_reg_write(dev, 0x1e, 0xff, 0x01);

	hdmi_i2c_reg_write(dev, 0x61, 0xff, 0x30);
	hdmi_i2c_reg_read(dev, 0xf3, &data); /* -> 0x00 */
	hdmi_i2c_reg_write(dev, 0xf3, 0xff, data & ~0x30);
	hdmi_i2c_reg_read(dev, 0xf3, &data); /* -> 0x00 */
	hdmi_i2c_reg_write(dev, 0xf3, 0xff, data | 0x20);

	hdmi_i2c_reg_write(dev, 0x09, 0xff, 0x30);
	hdmi_i2c_reg_write(dev, 0x0a, 0xff, 0xf8);
	hdmi_i2c_reg_write(dev, 0x0b, 0xff, 0x37);
	hdmi_i2c_reg_write(dev, 0x0f, 0xff, 0x00);
	hdmi_i2c_reg_write(dev, 0xc9, 0xff, 0x00);
	hdmi_i2c_reg_write(dev, 0xca, 0xff, 0x00);
	hdmi_i2c_reg_write(dev, 0xcb, 0xff, 0x00);
	hdmi_i2c_reg_write(dev, 0xcc, 0xff, 0x00);
	hdmi_i2c_reg_write(dev, 0xcd, 0xff, 0x00);
	hdmi_i2c_reg_write(dev, 0xce, 0xff, 0x00);
	hdmi_i2c_reg_write(dev, 0xcf, 0xff, 0x00);
	hdmi_i2c_reg_write(dev, 0xd0, 0xff, 0x00);
	hdmi_i2c_reg_write(dev, 0x0f, 0xff, 0x01);

	hdmi_i2c_reg_read(dev, 0x58, &data); /* -> 0x00 */
	hdmi_i2c_reg_write(dev, 0x58, 0xff, data & ~(3 << 5));

	hdmi_i2c_reg_write(dev, 0x0f, 0xff, 0x00);
	hdmi_i2c_reg_write(dev, 0xe1, 0xff, 0x00);
	hdmi_i2c_reg_write(dev, 0x0c, 0xff, 0xff);
	hdmi_i2c_reg_write(dev, 0x0d, 0xff, 0xff);
	hdmi_i2c_reg_read(dev, 0x0e, &data); /* -> 0x00 */
	hdmi_i2c_reg_write(dev, 0x0e, 0xff, (data | 0x3));
	hdmi_i2c_reg_write(dev, 0x0e, 0xff, (data & 0xfe));
	hdmi_i2c_reg_write(dev, 0x0f, 0xff, 0x01);
	hdmi_i2c_reg_write(dev, 0x33, 0xff, 0x00);
	hdmi_i2c_reg_write(dev, 0x34, 0xff, 0x18);
	hdmi_i2c_reg_write(dev, 0x35, 0xff, 0x00);
	hdmi_i2c_reg_write(dev, 0x0f, 0xff, 0x00);
	hdmi_i2c_reg_write(dev, 0xc4, 0xff, 0xfe);
	hdmi_i2c_reg_read(dev, 0xc5, &data); /* -> 0x00 */
	hdmi_i2c_reg_write(dev, 0xc5, 0xff, data | 0x30);
	/* InitIT626X  end */

	hdmi_i2c_reg_write(dev, 0x0f, 0xff, 0x00);
	hdmi_i2c_reg_write(dev, 0x04, 0xff, 0x3d);
	hdmi_i2c_reg_write(dev, 0x04, 0xff, 0x15);
	hdmi_i2c_reg_write(dev, 0x0f, 0xff, 0x00);
	hdmi_i2c_reg_write(dev, 0x1d, 0xff, 0x66);
	hdmi_i2c_reg_write(dev, 0x1e, 0xff, 0x01);

	hdmi_i2c_reg_read(dev, 0xc1, &data); /* -> 0x00 */
	hdmi_i2c_reg_write(dev, 0x61, 0xff, 0x10);

	/* SetupAFE(): */
	hdmi_i2c_reg_write(dev, 0x62, 0xff, 0x88);
	hdmi_i2c_reg_write(dev, 0x63, 0xff, 0x10);
	hdmi_i2c_reg_write(dev, 0x64, 0xff, 0x84);
	/* SetupAFE(): end */

	hdmi_i2c_reg_read(dev, 0x04, &data); /* -> 0x00 */
	hdmi_i2c_reg_write(dev, 0x04, 0xff, 0x1d);

	hdmi_i2c_reg_read(dev, 0x04, &data); /* -> 0x00 */
	hdmi_i2c_reg_write(dev, 0x04, 0xff, 0x15);

	hdmi_i2c_reg_read(dev, 0x0e, &data); /* -> 0x00 */

	/*  Wait video stable */
	hdmi_i2c_reg_read(dev, 0x0e, &data); /* -> 0x00 */

	/* Reset Video */
	hdmi_i2c_reg_read(dev, 0x0d, &data); /* -> 0x00 */
	hdmi_i2c_reg_write(dev, 0x0d, 0xff, 0x40);
	hdmi_i2c_reg_read(dev, 0x0e, &data); /* -> 0x00 */
	hdmi_i2c_reg_write(dev, 0x0e, 0xff, 0x7d);
	hdmi_i2c_reg_write(dev, 0x0e, 0xff, 0x7c);
	hdmi_i2c_reg_write(dev, 0x0f, 0xff, 0x00);
	hdmi_i2c_reg_write(dev, 0x61, 0xff, 0x00);
	hdmi_i2c_reg_read(dev, 0x61, &data); /* -> 0x00 */
	hdmi_i2c_reg_read(dev, 0x62, &data); /* -> 0x00 */
	hdmi_i2c_reg_read(dev, 0x63, &data); /* -> 0x00 */
	hdmi_i2c_reg_read(dev, 0x64, &data); /* -> 0x00 */
	hdmi_i2c_reg_read(dev, 0x65, &data); /* -> 0x00 */
	hdmi_i2c_reg_read(dev, 0x66, &data); /* -> 0x00 */
	hdmi_i2c_reg_read(dev, 0x67, &data); /* -> 0x00 */
	hdmi_i2c_reg_write(dev, 0x0f, 0xff, 0x00);
	hdmi_i2c_reg_read(dev, 0xc1, &data); /* -> 0x00 */
	hdmi_i2c_reg_write(dev, 0xc1, 0xff, 0x00);
	hdmi_i2c_reg_write(dev, 0xc6, 0xff, 0x03);
	/* Clear AV mute */

	return 0;
}


int lvds_soc_setup(int lvds_id, sc_pm_clock_rate_t pixel_clock)
{
	sc_err_t err;
	sc_rsrc_t lvds_rsrc, mipi_rsrc;
	const char *pd_name;
	sc_ipc_t ipcHndl = gd->arch.ipc_channel_handle;

	struct power_domain pd;
	int ret;

	if (lvds_id == 0) {
		lvds_rsrc = SC_R_LVDS_0;
		mipi_rsrc = SC_R_MIPI_0;
		pd_name = "lvds0_power_domain";
	} else {
		lvds_rsrc = SC_R_LVDS_1;
		mipi_rsrc = SC_R_MIPI_1;
		pd_name = "lvds1_power_domain";
	}
	/* Power up LVDS */
	if (!power_domain_lookup_name(pd_name, &pd)) {
		ret = power_domain_on(&pd);
		if (ret) {
			printf("%s Power up failed! (error = %d)\n", pd_name, ret);
			return -EIO;
		}
	} else {
		printf("%s lookup failed!\n", pd_name);
		return -EIO;
	}

	/* Setup clocks */
	err = sc_pm_set_clock_rate(ipcHndl, lvds_rsrc, SC_PM_CLK_BYPASS, &pixel_clock);
	if (err != SC_ERR_NONE) {
		printf("LVDS set rate SC_PM_CLK_BYPASS failed! (error = %d)\n", err);
		return -EIO;
	}

	err = sc_pm_set_clock_rate(ipcHndl, lvds_rsrc, SC_PM_CLK_PER, &pixel_clock);
	if (err != SC_ERR_NONE) {
		printf("LVDS set rate SC_PM_CLK_BYPASS failed! (error = %d)\n", err);
		return -EIO;
	}

	err = sc_pm_set_clock_rate(ipcHndl, lvds_rsrc, SC_PM_CLK_PHY, &pixel_clock);
	if (err != SC_ERR_NONE) {
		printf("LVDS set rate SC_PM_CLK_BYPASS failed! (error = %d)\n", err);
		return -EIO;
	}

	if (is_imx8qxp()) {
		/* For QXP, there is only one DC, and two pixel links to each LVDS with a mux provided.
		  * We connect LVDS0 to pixel link 0, lVDS1 to pixel link 1 from DC
		  */

		/* Configure to LVDS mode not MIPI DSI */
		err = sc_misc_set_control(ipcHndl, mipi_rsrc, SC_C_MODE, 1);
		if (err != SC_ERR_NONE) {
			printf("LVDS sc_misc_set_control SC_C_MODE failed! (error = %d)\n", err);
			return -EIO;
		}

		/* Configure to LVDS mode with single channel */
		err = sc_misc_set_control(ipcHndl, mipi_rsrc, SC_C_DUAL_MODE, 0);
		if (err != SC_ERR_NONE) {
			printf("LVDS sc_misc_set_control SC_C_DUAL_MODE failed! (error = %d)\n", err);
			return -EIO;
		}

		err = sc_misc_set_control(ipcHndl, mipi_rsrc, SC_C_PXL_LINK_SEL, lvds_id);
		if (err != SC_ERR_NONE) {
			printf("LVDS sc_misc_set_control SC_C_PXL_LINK_SEL failed! (error = %d)\n", err);
			return -EIO;
		}
	}

	err = sc_pm_clock_enable(ipcHndl, lvds_rsrc, SC_PM_CLK_BYPASS, true, false);
	if (err != SC_ERR_NONE) {
		printf("LVDS enable clock SC_PM_CLK_BYPASS failed! (error = %d)\n", err);
		return -EIO;
	}

	err = sc_pm_clock_enable(ipcHndl, lvds_rsrc, SC_PM_CLK_PER, true, false);
	if (err != SC_ERR_NONE) {
		printf("LVDS enable clock SC_PM_CLK_PER failed! (error = %d)\n", err);
		return -EIO;
	}

	err = sc_pm_clock_enable(ipcHndl, lvds_rsrc, SC_PM_CLK_PHY, true, false);
	if (err != SC_ERR_NONE) {
		printf("LVDS enable clock SC_PM_CLK_PHY failed! (error = %d)\n", err);
		return -EIO;
	}

	return 0;
}

void lvds_configure(int lvds_id)
{
	void __iomem *lvds_base;
	void __iomem *mipi_base;
	uint32_t phy_setting;
	uint32_t mode;

	if (lvds_id == 0) {
		lvds_base = (void __iomem *)LVDS0_PHYCTRL_BASE;
		mipi_base = (void __iomem *)MIPI0_SS_BASE;
	} else {
		lvds_base = (void __iomem *)LVDS1_PHYCTRL_BASE;
		mipi_base = (void __iomem *)MIPI1_SS_BASE;
	}

	if (is_imx8qm()) {
		mode =
			IMX_LVDS_SET_FIELD(LVDS_CTRL_CH0_MODE, LVDS_CTRL_CH0_MODE__DI0) |
			IMX_LVDS_SET_FIELD(LVDS_CTRL_CH0_DATA_WIDTH, LVDS_CTRL_CH0_DATA_WIDTH__24BIT) |
			IMX_LVDS_SET_FIELD(LVDS_CTRL_CH0_BIT_MAP, LVDS_CTRL_CH0_BIT_MAP__JEIDA) |
			IMX_LVDS_SET_FIELD(LVDS_CTRL_CH0_10BIT_ENABLE, LVDS_CTRL_CH0_10BIT_ENABLE__10BIT) |
			IMX_LVDS_SET_FIELD(LVDS_CTRL_DI0_DATA_WIDTH, LVDS_CTRL_DI0_DATA_WIDTH__USE_30BIT);

		writel(mode, lvds_base + LVDS_CTRL);

		phy_setting =
			LVDS_PHY_CTRL_RFB_MASK |
			LVDS_PHY_CTRL_CH0_EN_MASK |
			(0 << LVDS_PHY_CTRL_M_SHIFT) |
			(0x04 << LVDS_PHY_CTRL_CCM_SHIFT) |
			(0x04 << LVDS_PHY_CTRL_CA_SHIFT);
		writel(phy_setting, lvds_base + LVDS_PHY_CTRL);
	} else if (is_imx8qxp()) {
		mode =
			IMX_LVDS_SET_FIELD(LVDS_CTRL_CH0_MODE, LVDS_CTRL_CH0_MODE__DI0) |
			IMX_LVDS_SET_FIELD(LVDS_CTRL_CH0_DATA_WIDTH, LVDS_CTRL_CH0_DATA_WIDTH__24BIT) |
			IMX_LVDS_SET_FIELD(LVDS_CTRL_CH0_BIT_MAP, LVDS_CTRL_CH0_BIT_MAP__JEIDA);

		phy_setting = 0x4 << 5 | 0x4 << 2 | 1 << 1 | 0x1;
		writel(phy_setting, lvds_base + 0 /* PHY_CTRL*/);
		writel(mode, lvds_base + LVDS_CTRL);
		writel(0, lvds_base +  MIPIv2_CSR_TX_ULPS);
		writel(MIPI_CSR_PXL2DPI_24_BIT, lvds_base + MIPIv2_CSR_PXL2DPI);

		/* Power up PLL in MIPI DSI PHY */
		writel(0, mipi_base + MIPI_DSI_OFFSET + DPHY_PD_PLL);
		writel(0, mipi_base + MIPI_DSI_OFFSET + DPHY_PD_TX);
	}
}

int display_controller_setup(sc_pm_clock_rate_t pixel_clock)
{
	sc_err_t err;
	sc_rsrc_t dc_rsrc, pll0_rsrc, pll1_rsrc;
	sc_pm_clock_rate_t pll_clk;
	const char *pll1_pd_name;
	sc_ipc_t ipcHndl = gd->arch.ipc_channel_handle;

	int dc_id = gdc;

	struct power_domain pd;
	int ret;

	if (dc_id == 0) {
		dc_rsrc = SC_R_DC_0;
		pll0_rsrc = SC_R_DC_0_PLL_0;
		pll1_rsrc = SC_R_DC_0_PLL_1;
		pll1_pd_name = "dc0_pll1";
	} else {
		dc_rsrc = SC_R_DC_1;
		pll0_rsrc = SC_R_DC_1_PLL_0;
		pll1_rsrc = SC_R_DC_1_PLL_1;
		pll1_pd_name = "dc1_pll1";
	}

	if (!power_domain_lookup_name(pll1_pd_name, &pd)) {
		ret = power_domain_on(&pd);
		if (ret) {
			printf("%s Power up failed! (error = %d)\n", pll1_pd_name, ret);
			return -EIO;
		}
	} else {
		printf("%s lookup failed!\n", pll1_pd_name);
		return -EIO;
	}

	/* Setup the pll1/2 and DISP0/1 clock */
	if (pixel_clock >= 40000000)
		pll_clk = 1188000000;
	else
		pll_clk = 675000000;

	err = sc_pm_set_clock_rate(ipcHndl, pll0_rsrc, SC_PM_CLK_PLL, &pll_clk);
	if (err != SC_ERR_NONE) {
		printf("PLL0 set clock rate failed! (error = %d)\n", err);
		return -EIO;
	}

	err = sc_pm_set_clock_rate(ipcHndl, pll1_rsrc, SC_PM_CLK_PLL, &pll_clk);
	if (err != SC_ERR_NONE) {
		printf("PLL1 set clock rate failed! (error = %d)\n", err);
		return -EIO;
	}

	err = sc_pm_set_clock_rate(ipcHndl, dc_rsrc, SC_PM_CLK_MISC0, &pixel_clock);
	if (err != SC_ERR_NONE) {
		printf("DISP0 set clock rate failed! (error = %d)\n", err);
		return -EIO;
	}

	err = sc_pm_set_clock_rate(ipcHndl, dc_rsrc, SC_PM_CLK_MISC1, &pixel_clock);
	if (err != SC_ERR_NONE) {
		printf("DISP1 set clock rate failed! (error = %d)\n", err);
		return -EIO;
	}

	err = sc_pm_clock_enable(ipcHndl, pll0_rsrc, SC_PM_CLK_PLL, true, false);
	if (err != SC_ERR_NONE) {
		printf("PLL0 clock enable failed! (error = %d)\n", err);
		return -EIO;
	}

	err = sc_pm_clock_enable(ipcHndl, pll1_rsrc, SC_PM_CLK_PLL, true, false);
	if (err != SC_ERR_NONE) {
		printf("PLL1 clock enable failed! (error = %d)\n", err);
		return -EIO;
	}

	err = sc_pm_clock_enable(ipcHndl, dc_rsrc, SC_PM_CLK_MISC0, true, false);
	if (err != SC_ERR_NONE) {
		printf("DISP0 clock enable failed! (error = %d)\n", err);
		return -EIO;
	}

	err = sc_pm_clock_enable(ipcHndl, dc_rsrc, SC_PM_CLK_MISC1, true, false);
	if (err != SC_ERR_NONE) {
		printf("DISP1 clock enable failed! (error = %d)\n", err);
		return -EIO;
	}

	err = sc_misc_set_control(ipcHndl, dc_rsrc, SC_C_PXL_LINK_MST1_ADDR, 0);
	if (err != SC_ERR_NONE) {
		printf("DC Set control fSC_C_PXL_LINK_MST1_ADDR ailed! (error = %d)\n", err);
		return -EIO;
	}

	err = sc_misc_set_control(ipcHndl, dc_rsrc, SC_C_PXL_LINK_MST1_ENB, 1);
	if (err != SC_ERR_NONE) {
		printf("DC Set control SC_C_PXL_LINK_MST1_ENB failed! (error = %d)\n", err);
		return -EIO;
	}

	err = sc_misc_set_control(ipcHndl, dc_rsrc, SC_C_PXL_LINK_MST1_VLD, 1);
	if (err != SC_ERR_NONE) {
		printf("DC Set control SC_C_PXL_LINK_MST1_VLD failed! (error = %d)\n", err);
		return -EIO;
	}

	err = sc_misc_set_control(ipcHndl, dc_rsrc, SC_C_PXL_LINK_MST2_ADDR, 0);
	if (err != SC_ERR_NONE) {
		printf("DC Set control SC_C_PXL_LINK_MST2_ADDR ailed! (error = %d)\n", err);
		return -EIO;
	}

	err = sc_misc_set_control(ipcHndl, dc_rsrc, SC_C_PXL_LINK_MST2_ENB, 1);
	if (err != SC_ERR_NONE) {
		printf("DC Set control SC_C_PXL_LINK_MST2_ENB failed! (error = %d)\n", err);
		return -EIO;
	}

	err = sc_misc_set_control(ipcHndl, dc_rsrc, SC_C_PXL_LINK_MST2_VLD, 1);
	if (err != SC_ERR_NONE) {
		printf("DC Set control SC_C_PXL_LINK_MST2_VLD failed! (error = %d)\n", err);
		return -EIO;
	}

	err = sc_misc_set_control(ipcHndl, dc_rsrc, SC_C_SYNC_CTRL0, 1);
	if (err != SC_ERR_NONE) {
		printf("DC Set control SC_C_SYNC_CTRL0 failed! (error = %d)\n", err);
		return -EIO;
	}

	err = sc_misc_set_control(ipcHndl, dc_rsrc, SC_C_SYNC_CTRL1, 1);
	if (err != SC_ERR_NONE) {
		printf("DC Set control SC_C_SYNC_CTRL1 failed! (error = %d)\n", err);
		return -EIO;
	}

	return 0;
}

void *video_hw_init(void)
{
	imxdpuv1_channel_params_t channel;
	imxdpuv1_layer_t layer;
	void *fb;

	int8_t imxdpuv1_id = gdc;

	if (imxdpuv1_id != 0 || (imxdpuv1_id == 1 && !is_imx8qm())) {
		printf("%s(): invalid imxdpuv1_id %d", __func__, imxdpuv1_id);
		return NULL;
	}

	panel.winSizeX = gmode.hlen;
	panel.winSizeY = gmode.vlen;
	panel.plnSizeX = gmode.hlen;
	panel.plnSizeY = gmode.vlen;

	panel.gdfBytesPP = 4;
	panel.gdfIndex = GDF_32BIT_X888RGB;

	panel.memSize = gmode.hlen * gmode.vlen * panel.gdfBytesPP;

	/* Allocate framebuffer */
	fb = memalign(0x1000,
		      roundup(panel.memSize, 0x1000));
	if (!fb) {
		printf("IMXDPUv1: Error allocating framebuffer!\n");
		return NULL;
	}

	/* Wipe framebuffer */
	memset(fb, 0, panel.memSize);

	panel.frameAdrs = (ulong)fb;

	imxdpuv1_init(imxdpuv1_id);
	imxdpuv1_disp_enable_frame_gen(imxdpuv1_id, 0, IMXDPUV1_FALSE);
	imxdpuv1_disp_enable_frame_gen(imxdpuv1_id, 1, IMXDPUV1_FALSE);

	imxdpuv1_disp_setup_frame_gen(imxdpuv1_id, gdisp,
		(const struct imxdpuv1_videomode *)&gmode,
		0x3ff, 0, 0, 1, IMXDPUV1_DISABLE);
	imxdpuv1_disp_init(imxdpuv1_id, gdisp);
	imxdpuv1_disp_setup_constframe(imxdpuv1_id,
		gdisp, 0, 0, 0xff, 0); /* blue */

	if (gdisp == 0)
		channel.common.chan = IMXDPUV1_CHAN_VIDEO_0;
	else
		channel.common.chan = IMXDPUV1_CHAN_VIDEO_1;
	channel.common.src_pixel_fmt = gpixfmt;
	channel.common.dest_pixel_fmt = gpixfmt;
	channel.common.src_width = gmode.hlen;
	channel.common.src_height = gmode.vlen;

	channel.common.clip_width = 0;
	channel.common.clip_height = 0;
	channel.common.clip_top = 0;
	channel.common.clip_left = 0;

	channel.common.dest_width = gmode.hlen;
	channel.common.dest_height = gmode.vlen;
	channel.common.dest_top = 0;
	channel.common.dest_left = 0;
	channel.common.stride =
		gmode.hlen * imxdpuv1_bytes_per_pixel(IMXDPUV1_PIX_FMT_BGRA32);
	channel.common.disp_id = gdisp;
	channel.common.const_color = 0;
	channel.common.use_global_alpha = 0;
	channel.common.use_local_alpha = 0;
	imxdpuv1_init_channel(imxdpuv1_id, &channel);

	imxdpuv1_init_channel_buffer(imxdpuv1_id,
		channel.common.chan,
		gmode.hlen * imxdpuv1_bytes_per_pixel(IMXDPUV1_PIX_FMT_RGB32),
		IMXDPUV1_ROTATE_NONE,
		(dma_addr_t)fb,
		0,
		0);

	layer.enable    = IMXDPUV1_TRUE;
	layer.secondary = get_channel_blk(channel.common.chan);

	if (gdisp == 0) {
		layer.stream    = IMXDPUV1_DISPLAY_STREAM_0;
		layer.primary   = IMXDPUV1_ID_CONSTFRAME0;
	} else {
		layer.stream    = IMXDPUV1_DISPLAY_STREAM_1;
		layer.primary   = IMXDPUV1_ID_CONSTFRAME1;
	}

	imxdpuv1_disp_setup_layer(
		imxdpuv1_id, &layer, IMXDPUV1_LAYER_0, 1);
	imxdpuv1_disp_set_layer_global_alpha(
		imxdpuv1_id, IMXDPUV1_LAYER_0, 0xff);

	imxdpuv1_disp_set_layer_position(
		imxdpuv1_id, IMXDPUV1_LAYER_0, 0, 0);
	imxdpuv1_disp_set_chan_position(
		imxdpuv1_id, channel.common.chan, 0, 0);

	imxdpuv1_disp_enable_frame_gen(imxdpuv1_id, gdisp, IMXDPUV1_ENABLE);

	debug("IMXDPU display start ...\n");

	return &panel;
}

void imxdpuv1_fb_disable(void)
{
	/* Disable video only when video init is done */
	if (panel.frameAdrs)
		imxdpuv1_disp_enable_frame_gen(gdc, gdisp, IMXDPUV1_DISABLE);
}

int imxdpuv1_fb_init(struct fb_videomode const *mode,
		  uint8_t disp, uint32_t pixfmt)
{
	if (disp > 1) {
		printf("Invalid disp parameter %d for imxdpuv1_fb_init\n", disp);
		return -EINVAL;
	}

	memset(&gmode, 0, sizeof(struct imxdpuv1_videomode));
	gmode.pixelclock = PS2KHZ(mode->pixclock) * 1000;
	gmode.hlen = mode->xres;
	gmode.hbp = mode->left_margin;
	gmode.hfp = mode->right_margin;

	gmode.vlen = mode->yres;
	gmode.vbp = mode->upper_margin;
	gmode.vfp = mode->lower_margin;

	gmode.hsync = mode->hsync_len;
	gmode.vsync = mode->vsync_len;
	gmode.flags = IMXDPUV1_MODE_FLAGS_HSYNC_POL | IMXDPUV1_MODE_FLAGS_VSYNC_POL | IMXDPUV1_MODE_FLAGS_DE_POL;

	if (is_imx8qm()) { /* QM has two DCs each contains one LVDS as secondary display output */
		gdisp = 1;
		gdc = disp;
	} else if (is_imx8qxp()) { /* QXP has one DC which contains 2 LVDS/MIPI_DSI combo */
		gdisp = disp;
		gdc = 0;
	} else {
		printf("Unsupported SOC for imxdpuv1_fb_init\n");
		return -EPERM;
	}

	gpixfmt = pixfmt;

	debug("imxdpuv1_fb_init, dc=%d, disp=%d\n", gdc, gdisp);

	return 0;
}

