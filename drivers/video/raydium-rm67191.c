// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 *
 */

#include <common.h>
#include <dm.h>
#include <mipi_dsi.h>
#include <panel.h>
#include <asm/gpio.h>
#include <linux/err.h>

#define CMD_TABLE_LEN 2
typedef u8 cmd_set_table[CMD_TABLE_LEN];

/* Write Manufacture Command Set Control */
#define WRMAUCCTR 0xFE

struct rm67191_panel_priv {
	struct gpio_desc reset;
	unsigned int lanes;
	enum mipi_dsi_pixel_format format;
	unsigned long mode_flags;
};

/* Manufacturer Command Set pages (CMD2) */
static const cmd_set_table manufacturer_cmd_set[] = {
	{0xFE, 0x0B},
	{0x28, 0x40},
	{0x29, 0x4F},
	{0xFE, 0x0E},
	{0x4B, 0x00},
	{0x4C, 0x0F},
	{0x4D, 0x20},
	{0x4E, 0x40},
	{0x4F, 0x60},
	{0x50, 0xA0},
	{0x51, 0xC0},
	{0x52, 0xE0},
	{0x53, 0xFF},
	{0xFE, 0x0D},
	{0x18, 0x08},
	{0x42, 0x00},
	{0x08, 0x41},
	{0x46, 0x02},
	{0x72, 0x09},
	{0xFE, 0x0A},
	{0x24, 0x17},
	{0x04, 0x07},
	{0x1A, 0x0C},
	{0x0F, 0x44},
	{0xFE, 0x04},
	{0x00, 0x0C},
	{0x05, 0x08},
	{0x06, 0x08},
	{0x08, 0x08},
	{0x09, 0x08},
	{0x0A, 0xE6},
	{0x0B, 0x8C},
	{0x1A, 0x12},
	{0x1E, 0xE0},
	{0x29, 0x93},
	{0x2A, 0x93},
	{0x2F, 0x02},
	{0x31, 0x02},
	{0x33, 0x05},
	{0x37, 0x2D},
	{0x38, 0x2D},
	{0x3A, 0x1E},
	{0x3B, 0x1E},
	{0x3D, 0x27},
	{0x3F, 0x80},
	{0x40, 0x40},
	{0x41, 0xE0},
	{0x4F, 0x2F},
	{0x50, 0x1E},
	{0xFE, 0x06},
	{0x00, 0xCC},
	{0x05, 0x05},
	{0x07, 0xA2},
	{0x08, 0xCC},
	{0x0D, 0x03},
	{0x0F, 0xA2},
	{0x32, 0xCC},
	{0x37, 0x05},
	{0x39, 0x83},
	{0x3A, 0xCC},
	{0x41, 0x04},
	{0x43, 0x83},
	{0x44, 0xCC},
	{0x49, 0x05},
	{0x4B, 0xA2},
	{0x4C, 0xCC},
	{0x51, 0x03},
	{0x53, 0xA2},
	{0x75, 0xCC},
	{0x7A, 0x03},
	{0x7C, 0x83},
	{0x7D, 0xCC},
	{0x82, 0x02},
	{0x84, 0x83},
	{0x85, 0xEC},
	{0x86, 0x0F},
	{0x87, 0xFF},
	{0x88, 0x00},
	{0x8A, 0x02},
	{0x8C, 0xA2},
	{0x8D, 0xEA},
	{0x8E, 0x01},
	{0x8F, 0xE8},
	{0xFE, 0x06},
	{0x90, 0x0A},
	{0x92, 0x06},
	{0x93, 0xA0},
	{0x94, 0xA8},
	{0x95, 0xEC},
	{0x96, 0x0F},
	{0x97, 0xFF},
	{0x98, 0x00},
	{0x9A, 0x02},
	{0x9C, 0xA2},
	{0xAC, 0x04},
	{0xFE, 0x06},
	{0xB1, 0x12},
	{0xB2, 0x17},
	{0xB3, 0x17},
	{0xB4, 0x17},
	{0xB5, 0x17},
	{0xB6, 0x11},
	{0xB7, 0x08},
	{0xB8, 0x09},
	{0xB9, 0x06},
	{0xBA, 0x07},
	{0xBB, 0x17},
	{0xBC, 0x17},
	{0xBD, 0x17},
	{0xBE, 0x17},
	{0xBF, 0x17},
	{0xC0, 0x17},
	{0xC1, 0x17},
	{0xC2, 0x17},
	{0xC3, 0x17},
	{0xC4, 0x0F},
	{0xC5, 0x0E},
	{0xC6, 0x00},
	{0xC7, 0x01},
	{0xC8, 0x10},
	{0xFE, 0x06},
	{0x95, 0xEC},
	{0x8D, 0xEE},
	{0x44, 0xEC},
	{0x4C, 0xEC},
	{0x32, 0xEC},
	{0x3A, 0xEC},
	{0x7D, 0xEC},
	{0x75, 0xEC},
	{0x00, 0xEC},
	{0x08, 0xEC},
	{0x85, 0xEC},
	{0xA6, 0x21},
	{0xA7, 0x05},
	{0xA9, 0x06},
	{0x82, 0x06},
	{0x41, 0x06},
	{0x7A, 0x07},
	{0x37, 0x07},
	{0x05, 0x06},
	{0x49, 0x06},
	{0x0D, 0x04},
	{0x51, 0x04},
};

static const struct display_timing default_timing = {
	.pixelclock.typ		= 132000000,
	.hactive.typ		= 1080,
	.hfront_porch.typ	= 20,
	.hback_porch.typ	= 34,
	.hsync_len.typ		= 2,
	.vactive.typ		= 1920,
	.vfront_porch.typ	= 10,
	.vback_porch.typ	= 4,
	.vsync_len.typ		= 2,
	.flags = DISPLAY_FLAGS_HSYNC_LOW |
		 DISPLAY_FLAGS_VSYNC_LOW |
		 DISPLAY_FLAGS_DE_LOW |
		 DISPLAY_FLAGS_PIXDATA_NEGEDGE,
};


static u8 color_format_from_dsi_format(enum mipi_dsi_pixel_format format)
{
	switch (format) {
	case MIPI_DSI_FMT_RGB565:
		return 0x55;
	case MIPI_DSI_FMT_RGB666:
	case MIPI_DSI_FMT_RGB666_PACKED:
		return 0x66;
	case MIPI_DSI_FMT_RGB888:
		return 0x77;
	default:
		return 0x77; /* for backward compatibility */
	}
};

static int rad_panel_push_cmd_list(struct mipi_dsi_device *device)
{
	size_t i;
	const u8 *cmd;
	size_t count = sizeof(manufacturer_cmd_set) / CMD_TABLE_LEN;
	int ret = 0;

	for (i = 0; i < count ; i++) {
		cmd = manufacturer_cmd_set[i];
		ret = mipi_dsi_generic_write(device, cmd, CMD_TABLE_LEN);
		if (ret < 0)
			return ret;
	}

	return ret;
};

static int rm67191_enable(struct udevice *dev)
{
	struct rm67191_panel_priv *priv = dev_get_priv(dev);
	struct mipi_dsi_panel_plat *plat = dev_get_platdata(dev);
	struct mipi_dsi_device *dsi = plat->device;
	u8 color_format = color_format_from_dsi_format(priv->format);
	u16 brightness;
	int ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	ret = rad_panel_push_cmd_list(dsi);
	if (ret < 0) {
		printf("Failed to send MCS (%d)\n", ret);
		return -EIO;
	}

	/* Select User Command Set table (CMD1) */
	ret = mipi_dsi_generic_write(dsi, (u8[]){ WRMAUCCTR, 0x00 }, 2);
	if (ret < 0)
		return -EIO;

	/* Software reset */
	ret = mipi_dsi_dcs_soft_reset(dsi);
	if (ret < 0) {
		printf("Failed to do Software Reset (%d)\n", ret);
		return -EIO;
	}

	/* Wait 80ms for panel out of reset */
	mdelay(80);

	/* Set DSI mode */
	ret = mipi_dsi_generic_write(dsi, (u8[]){ 0xC2, 0x0B }, 2);
	if (ret < 0) {
		printf("Failed to set DSI mode (%d)\n", ret);
		return -EIO;
	}

	/* Set tear ON */
	ret = mipi_dsi_dcs_set_tear_on(dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	if (ret < 0) {
		printf("Failed to set tear ON (%d)\n", ret);
		return -EIO;
	}

	/* Set tear scanline */
	ret = mipi_dsi_dcs_set_tear_scanline(dsi, 0x380);
	if (ret < 0) {
		printf("Failed to set tear scanline (%d)\n", ret);
		return -EIO;
	}

	/* Set pixel format */
	ret = mipi_dsi_dcs_set_pixel_format(dsi, color_format);
	if (ret < 0) {
		printf("Failed to set pixel format (%d)\n", ret);
		return -EIO;
	}


	/* Set display brightness */
	brightness = 255; /* Max brightness */
	ret = mipi_dsi_dcs_write(dsi, MIPI_DCS_SET_DISPLAY_BRIGHTNESS, &brightness, 2);
	if (ret < 0) {
		printf("Failed to set display brightness (%d)\n",
				  ret);
		return -EIO;
	}

	/* Exit sleep mode */
	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		printf("Failed to exit sleep mode (%d)\n", ret);
		return -EIO;
	}

	mdelay(5);

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0) {
		printf("Failed to set display ON (%d)\n", ret);
		return -EIO;
	}

	return 0;
}

static int rm67191_panel_enable_backlight(struct udevice *dev)
{
	struct mipi_dsi_panel_plat *plat = dev_get_platdata(dev);
	struct mipi_dsi_device *device = plat->device;
	int ret;

	ret = mipi_dsi_attach(device);
	if (ret < 0)
		return ret;

	return rm67191_enable(dev);
}

static int rm67191_panel_get_display_timing(struct udevice *dev,
					    struct display_timing *timings)
{
	struct mipi_dsi_panel_plat *plat = dev_get_platdata(dev);
	struct mipi_dsi_device *device = plat->device;
	struct rm67191_panel_priv *priv = dev_get_priv(dev);

	memcpy(timings, &default_timing, sizeof(*timings));

	/* fill characteristics of DSI data link */
	if (device) {
		device->lanes = priv->lanes;
		device->format = priv->format;
		device->mode_flags = priv->mode_flags;
	}

	return 0;
}

static int rm67191_panel_probe(struct udevice *dev)
{
	struct rm67191_panel_priv *priv = dev_get_priv(dev);
	int ret;
	u32 video_mode;

	priv->format = MIPI_DSI_FMT_RGB888;
	priv->mode_flags = MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_VIDEO;

	ret = dev_read_u32(dev, "video-mode", &video_mode);
	if (!ret) {
		switch (video_mode) {
		case 0:
			/* burst mode */
			priv->mode_flags |= MIPI_DSI_MODE_VIDEO_BURST;
			break;
		case 1:
			/* non-burst mode with sync event */
			break;
		case 2:
			/* non-burst mode with sync pulse */
			priv->mode_flags |= MIPI_DSI_MODE_VIDEO_SYNC_PULSE;
			break;
		default:
			dev_warn(dev, "invalid video mode %d\n", video_mode);
			break;
		}
	}

	ret = dev_read_u32(dev, "dsi-lanes", &priv->lanes);
	if (ret) {
		printf("Failed to get dsi-lanes property (%d)\n", ret);
		return ret;
	}

	ret = gpio_request_by_name(dev, "reset-gpio", 0, &priv->reset,
				   GPIOD_IS_OUT);
	if (ret) {
		printf("Warning: cannot get reset GPIO\n");
		if (ret != -ENOENT)
			return ret;
	}

	/* reset panel */
	ret = dm_gpio_set_value(&priv->reset, true);
	if (ret)
		printf("reset gpio fails to set true\n");
	mdelay(100);
	ret = dm_gpio_set_value(&priv->reset, false);
	if (ret)
		printf("reset gpio fails to set true\n");
	mdelay(100);

	return 0;
}

static int rm67191_panel_disable(struct udevice *dev)
{
	struct rm67191_panel_priv *priv = dev_get_priv(dev);

	dm_gpio_set_value(&priv->reset, true);

	return 0;
}

static const struct panel_ops rm67191_panel_ops = {
	.enable_backlight = rm67191_panel_enable_backlight,
	.get_display_timing = rm67191_panel_get_display_timing,
};

static const struct udevice_id rm67191_panel_ids[] = {
	{ .compatible = "raydium,rm67191" },
	{ }
};

U_BOOT_DRIVER(rm67191_panel) = {
	.name			  = "rm67191_panel",
	.id			  = UCLASS_PANEL,
	.of_match		  = rm67191_panel_ids,
	.ops			  = &rm67191_panel_ops,
	.probe			  = rm67191_panel_probe,
	.remove			  = rm67191_panel_disable,
	.platdata_auto_alloc_size = sizeof(struct mipi_dsi_panel_plat),
	.priv_auto_alloc_size	= sizeof(struct rm67191_panel_priv),
};
