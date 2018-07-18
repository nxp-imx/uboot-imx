/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <malloc.h>

#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <linux/string.h>

#include "mipi_dsi_northwest_regs.h"
#include <mipi_dsi_northwest.h>
#include <mipi_display.h>
#include <imx_mipi_dsi_bridge.h>

#define MIPI_LCD_SLEEP_MODE_DELAY	(120)
#define MIPI_FIFO_TIMEOUT		250000 /* 250ms */

enum mipi_dsi_mode {
	DSI_COMMAND_MODE,
	DSI_VIDEO_MODE
};

#define DSI_LP_MODE	0
#define DSI_HS_MODE	1

enum mipi_dsi_payload {
	DSI_PAYLOAD_CMD,
	DSI_PAYLOAD_VIDEO,
};

/*
 * mipi-dsi northwest driver information structure, holds useful data for the driver.
 */
struct mipi_dsi_northwest_info {
	u32			mmio_base;
	u32			sim_base;
	int			enabled;
	struct mipi_dsi_client_dev *dsi_panel_dev;
	struct mipi_dsi_client_driver *dsi_panel_drv;
	struct fb_videomode mode;
};

/**
 * board_mipi_panel_reset - give a reset cycle for mipi dsi panel
 *
 * Target board specific, like use gpio to reset the dsi panel
 * Machine board file overrides board_mipi_panel_reset
 *
 * Return: 0 Success
 */
int __weak board_mipi_panel_reset(void)
{
	return 0;
}

/**
 * board_mipi_panel_shutdown - Shut down the mipi dsi panel
 *
 * Target board specific, like use gpio to shut down the dsi panel
 * Machine board file overrides board_mipi_panel_shutdown
 *
 * Return: 0 Success
 */
int __weak board_mipi_panel_shutdown(void)
{
	return 0;
}

static void mipi_dsi_set_mode(struct mipi_dsi_northwest_info *mipi_dsi,
			      uint8_t mode);
static int mipi_dsi_dcs_cmd(struct mipi_dsi_northwest_info *mipi_dsi,
			    u8 cmd, const u32 *param, int num);

static void mipi_dsi_set_mode(struct mipi_dsi_northwest_info *mipi_dsi,
			      uint8_t mode)
{
	switch (mode) {
	case DSI_LP_MODE:
		writel(0x1, mipi_dsi->mmio_base + HOST_CFG_NONCONTINUOUS_CLK);
		break;
	case DSI_HS_MODE:
		writel(0x0, mipi_dsi->mmio_base + HOST_CFG_NONCONTINUOUS_CLK);
		break;
	default:
		printf("invalid dsi mode\n");
		return;
	}

	mdelay(1);
}

static int mipi_dsi_dphy_init(struct mipi_dsi_northwest_info *mipi_dsi)
{
	uint32_t time_out = 100;
	uint32_t CN, CM, CO;
	uint32_t lock;

	setbits_le32(mipi_dsi->sim_base + SIM_SOPT1, MIPI_ISO_DISABLE);

	/* According to the RM, the dpi_pclk and clk_byte frequencies are related by the following formula:
	 * clk_byte_freq >= dpi_pclk_freq * DPI_pixel_size / ( 8 * (cfg_num_lanes + 1))
	 */

	/* PLL out clock = refclk * CM / (CN * CO)
	 * refclock = 24MHz
	 * pll vco = 24 * 40 / (3 * 1) = 320MHz
	 */
	CN = 0x10; /* 3  */
	CM = 0xc8; /* 40 */
	CO = 0x0;  /* 1  */

	writel(CN, mipi_dsi->mmio_base + DPHY_CN);
	writel(CM, mipi_dsi->mmio_base + DPHY_CM);
	writel(CO, mipi_dsi->mmio_base + DPHY_CO);

	writel(0x25, mipi_dsi->mmio_base + DPHY_TST);
	writel(0x0, mipi_dsi->mmio_base + DPHY_PD_PLL);

	while (!(lock = readl(mipi_dsi->mmio_base + DPHY_LOCK))) {
		udelay(10);
		time_out--;
		if (time_out == 0) {
			printf("cannot get the dphy lock = 0x%x\n", lock);
			return -EINVAL;
		}
	}
	debug("%s: dphy lock = 0x%x\n", __func__, lock);

	writel(0x0, mipi_dsi->mmio_base + DPHY_LOCK_BYP);
	writel(0x1, mipi_dsi->mmio_base + DPHY_RTERM_SEL);
	writel(0x0, mipi_dsi->mmio_base + DPHY_AUTO_PD_EN);
	writel(0x1, mipi_dsi->mmio_base + DPHY_RXLPRP);
	writel(0x1, mipi_dsi->mmio_base + DPHY_RXCDRP);
	writel(0x0, mipi_dsi->mmio_base + DPHY_M_PRG_HS_PREPARE);
	writel(0x0, mipi_dsi->mmio_base + DPHY_MC_PRG_HS_PREPARE);
	writel(0x9, mipi_dsi->mmio_base + DPHY_M_PRG_HS_ZERO);
	writel(0x20, mipi_dsi->mmio_base + DPHY_MC_PRG_HS_ZERO);
	writel(0x5, mipi_dsi->mmio_base + DPHY_M_PRG_HS_TRAIL);
	writel(0x5, mipi_dsi->mmio_base + DPHY_MC_PRG_HS_TRAIL);
	writel(0x0, mipi_dsi->mmio_base + DPHY_PD_DPHY);

	setbits_le32(mipi_dsi->sim_base + SIM_SOPT1CFG, DSI_PLL_EN);
	return 0;
}

static int mipi_dsi_host_init(struct mipi_dsi_northwest_info *mipi_dsi)
{
	uint32_t lane_num;

	switch (mipi_dsi->dsi_panel_dev->lanes) {
	case 1:
		lane_num = 0x0;
		break;
	case 2:
		lane_num = 0x1;
		break;
	default:
		/* Invalid lane num */
		return -EINVAL;
	}

	writel(lane_num, mipi_dsi->mmio_base + HOST_CFG_NUM_LANES);
	writel(0x1, mipi_dsi->mmio_base + HOST_CFG_NONCONTINUOUS_CLK);
	writel(0x1, mipi_dsi->mmio_base + HOST_CFG_T_PRE);
	writel(52, mipi_dsi->mmio_base + HOST_CFG_T_POST);
	writel(13, mipi_dsi->mmio_base + HOST_CFG_TX_GAP);
	writel(0x1, mipi_dsi->mmio_base + HOST_CFG_AUTOINSERT_EOTP);
	writel(0x0, mipi_dsi->mmio_base + HOST_CFG_EXTRA_CMDS_AFTER_EOTP);
	writel(0x0, mipi_dsi->mmio_base + HOST_CFG_HTX_TO_COUNT);
	writel(0x0, mipi_dsi->mmio_base + HOST_CFG_LRX_H_TO_COUNT);
	writel(0x0, mipi_dsi->mmio_base + HOST_CFG_BTA_H_TO_COUNT);
	writel(0x3A98, mipi_dsi->mmio_base + HOST_CFG_TWAKEUP);

	return 0;
}

static int mipi_dsi_dpi_init(struct mipi_dsi_northwest_info *mipi_dsi)
{
	uint32_t bpp, color_coding, pixel_fmt;
	struct fb_videomode *mode = &(mipi_dsi->mode);

	bpp = mipi_dsi_pixel_format_to_bpp(mipi_dsi->dsi_panel_dev->format);
	if (bpp < 0)
		return -EINVAL;

	writel(mode->xres, mipi_dsi->mmio_base + DPI_PIXEL_PAYLOAD_SIZE);
	writel(mode->xres, mipi_dsi->mmio_base + DPI_PIXEL_FIFO_SEND_LEVEL);

	switch (bpp) {
	case 24:
		color_coding = 5;
		pixel_fmt = 3;
		break;
	case 16:
	case 18:
	default:
		/* Not supported */
		return -EINVAL;
	}
	writel(color_coding, mipi_dsi->mmio_base + DPI_INTERFACE_COLOR_CODING);
	writel(pixel_fmt, mipi_dsi->mmio_base + DPI_PIXEL_FORMAT);
	writel(0x0, mipi_dsi->mmio_base + DPI_VSYNC_POLARITY);
	writel(0x0, mipi_dsi->mmio_base + DPI_HSYNC_POLARITY);
	writel(0x2, mipi_dsi->mmio_base + DPI_VIDEO_MODE);

	writel(mode->right_margin * (bpp >> 3), mipi_dsi->mmio_base + DPI_HFP);
	writel(mode->left_margin * (bpp >> 3), mipi_dsi->mmio_base + DPI_HBP);
	writel(mode->hsync_len * (bpp >> 3), mipi_dsi->mmio_base + DPI_HSA);
	writel(0x0, mipi_dsi->mmio_base + DPI_ENABLE_MULT_PKTS);

	writel(mode->upper_margin, mipi_dsi->mmio_base + DPI_VBP);
	writel(mode->lower_margin, mipi_dsi->mmio_base + DPI_VFP);
	writel(0x1, mipi_dsi->mmio_base + DPI_BLLP_MODE);
	writel(0x0, mipi_dsi->mmio_base + DPI_USE_NULL_PKT_BLLP);

	writel(mode->yres - 1, mipi_dsi->mmio_base + DPI_VACTIVE);

	writel(0x0, mipi_dsi->mmio_base + DPI_VC);

	return 0;
}

static void mipi_dsi_init_interrupt(struct mipi_dsi_northwest_info *mipi_dsi)
{
	/* disable all the irqs */
	writel(0xffffffff, mipi_dsi->mmio_base + HOST_IRQ_MASK);
	writel(0x7, mipi_dsi->mmio_base + HOST_IRQ_MASK2);
}

static int mipi_display_enter_sleep(struct mipi_dsi_northwest_info *mipi_dsi)
{
	int err;

	err = mipi_dsi_dcs_cmd(mipi_dsi, MIPI_DCS_SET_DISPLAY_OFF,
			       NULL, 0);
	if (err)
		return -EINVAL;
	mdelay(50);

	err = mipi_dsi_dcs_cmd(mipi_dsi, MIPI_DCS_ENTER_SLEEP_MODE,
			NULL, 0);
	if (err)
		printf("MIPI DSI DCS Command sleep in error!\n");

	mdelay(MIPI_LCD_SLEEP_MODE_DELAY);

	return err;
}

static int mipi_dsi_enable(struct mipi_dsi_northwest_info *mipi_dsi)
{
	int ret;

	/* Assert resets */
	/* escape domain */
	clrbits_le32(mipi_dsi->sim_base + SIM_SOPT1CFG, DSI_RST_ESC_N);

	/* byte domain */
	clrbits_le32(mipi_dsi->sim_base + SIM_SOPT1CFG, DSI_RST_BYTE_N);

	/* dpi domain */
	clrbits_le32(mipi_dsi->sim_base + SIM_SOPT1CFG, DSI_RST_DPI_N);

	/* Enable mipi relevant clocks */
	enable_mipi_dsi_clk(1);

	ret = mipi_dsi_dphy_init(mipi_dsi);
	if (ret < 0)
		return ret;

	ret = mipi_dsi_host_init(mipi_dsi);
	if (ret < 0)
		return ret;

	ret = mipi_dsi_dpi_init(mipi_dsi);
	if (ret < 0)
		return ret;

	/* Deassert resets */
	/* escape domain */
	setbits_le32(mipi_dsi->sim_base + SIM_SOPT1CFG, DSI_RST_ESC_N);

	/* byte domain */
	setbits_le32(mipi_dsi->sim_base + SIM_SOPT1CFG, DSI_RST_BYTE_N);

	/* dpi domain */
	setbits_le32(mipi_dsi->sim_base + SIM_SOPT1CFG, DSI_RST_DPI_N);

	/* display_en */
	clrbits_le32(mipi_dsi->sim_base + SIM_SOPT1CFG, DSI_SD);

	/* normal cm */
	clrbits_le32(mipi_dsi->sim_base + SIM_SOPT1CFG, DSI_CM);
	mdelay(20);

	/* Reset mipi panel */
	board_mipi_panel_reset();
	mdelay(60);

	/* Disable all interrupts, since we use polling */
	mipi_dsi_init_interrupt(mipi_dsi);

	/* Call panel driver's setup */
	if (mipi_dsi->dsi_panel_drv->dsi_client_setup) {
		ret = mipi_dsi->dsi_panel_drv->dsi_client_setup(mipi_dsi->dsi_panel_dev);
		if (ret < 0) {
			printf("failed to init mipi lcd.\n");
			return ret;
		}
	}

	/* Enter the HS mode for video stream */
	mipi_dsi_set_mode(mipi_dsi, DSI_HS_MODE);

	return 0;
}

static void mipi_dsi_wr_tx_header(struct mipi_dsi_northwest_info *mipi_dsi,
				  u8 di, u8 data0, u8 data1, u8 mode, u8 need_bta)
{
	uint32_t pkt_control = 0;
	uint16_t word_count = 0;

	word_count = data0 | (data1 << 8);
	pkt_control = HOST_PKT_CONTROL_WC(word_count) |
		      HOST_PKT_CONTROL_VC(0)	      |
		      HOST_PKT_CONTROL_DT(di)	      |
		      HOST_PKT_CONTROL_HS_SEL(mode)   |
		      HOST_PKT_CONTROL_BTA_TX(need_bta);

	debug("pkt_control = %x\n", pkt_control);
	writel(pkt_control, mipi_dsi->mmio_base + HOST_PKT_CONTROL);
}

static void mipi_dsi_wr_tx_data(struct mipi_dsi_northwest_info *mipi_dsi,
				uint32_t tx_data)
{
	writel(tx_data, mipi_dsi->mmio_base + HOST_TX_PAYLOAD);
}

static void mipi_dsi_long_data_wr(struct mipi_dsi_northwest_info *mipi_dsi,
			const uint8_t *data0, uint32_t data_size)
{
	uint32_t data_cnt = 0, payload = 0;

	/* in case that data count is more than 4 */
	for (data_cnt = 0; data_cnt < data_size; data_cnt += 4) {
		/*
		 * after sending 4bytes per one time,
		 * send remainder data less then 4.
		 */
		if ((data_size - data_cnt) < 4) {
			if ((data_size - data_cnt) == 3) {
				payload = data0[data_cnt] |
					  (data0[data_cnt + 1] << 8) |
					  (data0[data_cnt + 2] << 16);
				debug("count = 3 payload = %x, %x %x %x\n",
				      payload, data0[data_cnt], data0[data_cnt + 1], data0[data_cnt + 2]);
			} else if ((data_size - data_cnt) == 2) {
				payload = data0[data_cnt] |
					  (data0[data_cnt + 1] << 8);
				debug("count = 2 payload = %x, %x %x\n",
				      payload, data0[data_cnt], data0[data_cnt + 1]);
			} else if ((data_size - data_cnt) == 1) {
				payload = data0[data_cnt];
				debug("count = 1 payload = %x, %x\n",
				      payload, data0[data_cnt]);
			}

			mipi_dsi_wr_tx_data(mipi_dsi, payload);
		} else {
			payload = data0[data_cnt] |
				  (data0[data_cnt + 1] << 8) |
				  (data0[data_cnt + 2] << 16) |
				  (data0[data_cnt + 3] << 24);

			debug("count = 4 payload = %x, %x %x %x %x\n",
			      payload, *(u8 *)(data0 + data_cnt),
			      data0[data_cnt + 1],
			      data0[data_cnt + 2],
			      data0[data_cnt + 3]);

			mipi_dsi_wr_tx_data(mipi_dsi, payload);
		}
	}
}

static int wait_for_pkt_done(struct mipi_dsi_northwest_info *mipi_dsi, unsigned long timeout)
{
	uint32_t irq_status;

	do {
		irq_status = readl(mipi_dsi->mmio_base + HOST_PKT_STATUS);
		if (irq_status & HOST_IRQ_STATUS_TX_PKT_DONE)
			return timeout;

		udelay(1);
	} while (--timeout);

	return 0;
}

static int mipi_dsi_pkt_write(struct mipi_dsi_northwest_info *mipi_dsi,
			u8 data_type, const u8 *buf, int len)
{
	int ret = 0;
	const uint8_t *data = (const uint8_t *)buf;

	debug("mipi_dsi_pkt_write data_type 0x%x, buf 0x%x, len %u\n", data_type, (u32)buf, len);

	if (len == 0)
		/* handle generic long write command */
		mipi_dsi_wr_tx_header(mipi_dsi, data_type, data[0], data[1], DSI_LP_MODE, 0);
	else {
		/* handle generic long write command */
		mipi_dsi_long_data_wr(mipi_dsi, data, len);
		mipi_dsi_wr_tx_header(mipi_dsi, data_type, len & 0xff,
				      (len & 0xff00) >> 8, DSI_LP_MODE, 0);
	}

	/* send packet */
	writel(0x1, mipi_dsi->mmio_base + HOST_SEND_PACKET);
	ret = wait_for_pkt_done(mipi_dsi, MIPI_FIFO_TIMEOUT);

	if (!ret) {
		printf("wait tx done timeout!\n");
		return -ETIMEDOUT;
	}
	mdelay(10);

	return 0;
}

static int mipi_dsi_dcs_cmd(struct mipi_dsi_northwest_info *mipi_dsi,
			    u8 cmd, const u32 *param, int num)
{
	int err = 0;
	u32 buf[DSI_CMD_BUF_MAXSIZE];

	switch (cmd) {
	case MIPI_DCS_EXIT_SLEEP_MODE:
	case MIPI_DCS_ENTER_SLEEP_MODE:
	case MIPI_DCS_SET_DISPLAY_ON:
	case MIPI_DCS_SET_DISPLAY_OFF:
		buf[0] = cmd;
		buf[1] = 0x0;
		err = mipi_dsi_pkt_write(mipi_dsi,
				MIPI_DSI_DCS_SHORT_WRITE, (u8 *)buf, 0);
		break;

	default:
		printf("MIPI DSI DCS Command:0x%x Not supported!\n", cmd);
		break;
	}

	return err;
}

static void mipi_dsi_shutdown(struct mipi_dsi_northwest_info *mipi_dsi)
{
	mipi_display_enter_sleep(mipi_dsi);

	writel(0x1, mipi_dsi->mmio_base + DPHY_PD_PLL);
	writel(0x1, mipi_dsi->mmio_base + DPHY_PD_DPHY);

	enable_mipi_dsi_clk(0);

	/* Assert resets */
	/* escape domain */
	clrbits_le32(mipi_dsi->sim_base + SIM_SOPT1CFG, DSI_RST_ESC_N);

	/* byte domain */
	clrbits_le32(mipi_dsi->sim_base + SIM_SOPT1CFG, DSI_RST_BYTE_N);

	/* dpi domain */
	clrbits_le32(mipi_dsi->sim_base + SIM_SOPT1CFG, DSI_RST_DPI_N);
}

/* Attach a LCD panel device */
int mipi_dsi_northwest_bridge_attach(struct mipi_dsi_bridge_driver *bridge_driver, struct mipi_dsi_client_dev *panel_dev)
{
	struct mipi_dsi_northwest_info *dsi_info = (struct mipi_dsi_northwest_info *)bridge_driver->driver_private;

	if (!panel_dev) {
		printf("mipi_dsi_northwest_panel_device is NULL.\n");
		return -EFAULT;
	}

	if (!panel_dev->name) {
		printf("mipi_dsi_northwest_panel_device name is NULL.\n");
		return -EFAULT;
	}

	if (dsi_info->dsi_panel_drv) {
		if (strcmp(panel_dev->name, dsi_info->dsi_panel_drv->name)) {
			printf("The panel device name %s is not for LCD driver %s\n",
			       panel_dev->name, dsi_info->dsi_panel_drv->name);
			return -EFAULT;
		}
	}

	dsi_info->dsi_panel_dev = panel_dev;

	return 0;
}


/* Add a LCD panel driver, will search the panel device to bind with them */
int mipi_dsi_northwest_bridge_add_client_driver(struct mipi_dsi_bridge_driver *bridge_driver,
	struct mipi_dsi_client_driver *panel_drv)
{
	struct mipi_dsi_northwest_info *dsi_info = (struct mipi_dsi_northwest_info *)bridge_driver->driver_private;

	if (!panel_drv) {
		printf("mipi_dsi_northwest_panel_driver is NULL.\n");
		return -EFAULT;
	}

	if (!panel_drv->name) {
		printf("mipi_dsi_northwest_panel_driver name is NULL.\n");
		return -EFAULT;
	}

	if (dsi_info->dsi_panel_dev) {
		if (strcmp(panel_drv->name, dsi_info->dsi_panel_dev->name)) {
			printf("The panel driver name %s is not for LCD device %s\n",
			       panel_drv->name, dsi_info->dsi_panel_dev->name);
			return -EFAULT;
		}
	}

	dsi_info->dsi_panel_drv = panel_drv;

	return 0;
}

/* Enable the mipi dsi display */
static int mipi_dsi_northwest_bridge_enable(struct mipi_dsi_bridge_driver *bridge_driver)
{
	struct mipi_dsi_northwest_info *dsi_info = (struct mipi_dsi_northwest_info *)bridge_driver->driver_private;

	if (!dsi_info->dsi_panel_dev || !dsi_info->dsi_panel_drv)
		return -ENODEV;

	mipi_dsi_enable(dsi_info);

	dsi_info->enabled = 1;

	return 0;
}

/* Disable and shutdown the mipi dsi display */
static int mipi_dsi_northwest_bridge_disable(struct mipi_dsi_bridge_driver *bridge_driver)
{
	struct mipi_dsi_northwest_info *dsi_info = (struct mipi_dsi_northwest_info *)bridge_driver->driver_private;

	if (!dsi_info->enabled)
		return 0;

	mipi_dsi_shutdown(dsi_info);
	board_mipi_panel_shutdown();

	dsi_info->enabled = 0;

	return 0;
}

static int mipi_dsi_northwest_bridge_mode_set(struct mipi_dsi_bridge_driver *bridge_driver,
	struct fb_videomode *fbmode)
{
	struct mipi_dsi_northwest_info *dsi_info = (struct mipi_dsi_northwest_info *)bridge_driver->driver_private;

	dsi_info->mode = *fbmode;

	return 0;
}

static int mipi_dsi_northwest_bridge_pkt_write(struct mipi_dsi_bridge_driver *bridge_driver,
			u8 data_type, const u8 *buf, int len)
{
	struct mipi_dsi_northwest_info *mipi_dsi = (struct mipi_dsi_northwest_info *)bridge_driver->driver_private;

	return mipi_dsi_pkt_write(mipi_dsi, data_type, buf, len);
}

struct mipi_dsi_bridge_driver imx_northwest_dsi_driver = {
	.attach    = mipi_dsi_northwest_bridge_attach,
	.enable   = mipi_dsi_northwest_bridge_enable,
	.disable   = mipi_dsi_northwest_bridge_disable,
	.mode_set   = mipi_dsi_northwest_bridge_mode_set,
	.pkt_write = mipi_dsi_northwest_bridge_pkt_write,
	.add_client_driver = mipi_dsi_northwest_bridge_add_client_driver,
	.name = "imx_northwest_mipi_dsi",
};

int mipi_dsi_northwest_setup(u32 base_addr, u32 sim_addr)
{
	struct mipi_dsi_northwest_info *dsi_info;

	dsi_info = (struct mipi_dsi_northwest_info *)malloc(sizeof(struct mipi_dsi_northwest_info));
	if (!dsi_info) {
		printf("failed to allocate mipi_dsi_northwest_info object.\n");
		return -ENOMEM;
	}

	dsi_info->mmio_base = base_addr;
	dsi_info->sim_base = sim_addr;
	dsi_info->dsi_panel_dev = NULL;
	dsi_info->dsi_panel_drv = NULL;
	dsi_info->enabled = 0;

	imx_northwest_dsi_driver.driver_private = dsi_info;
	return imx_mipi_dsi_bridge_register_driver(&imx_northwest_dsi_driver);
}
