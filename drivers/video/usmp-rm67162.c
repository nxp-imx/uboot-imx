// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2022 NXP
 *
 */

#include <common.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <mipi_dsi.h>
#include <panel.h>
#include <asm/gpio.h>
#include <linux/err.h>
#include <linux/delay.h>

#define CMD_TABLE_LEN 2
typedef u8 cmd_set_table[CMD_TABLE_LEN];

/* Write Manufacture Command Set Control */
#define WRMAUCCTR 0xFE

struct rm67162_panel_priv {
	struct gpio_desc reset;
	unsigned int lanes;
	enum mipi_dsi_pixel_format format;
	unsigned long mode_flags;
};

static const cmd_set_table mcs_rm67162_400x400[] = {
    /* Page 3:GOA */
    {0xFE, 0x04},
    /* GOA SETTING */
    {0x00, 0xDC},
    {0x01, 0x00},
    {0x02, 0x02},
    {0x03, 0x00},
    {0x04, 0x00},
    {0x05, 0x03},
    {0x06, 0x16},
    {0x07, 0x13},
    {0x08, 0x08},
    {0x09, 0xDC},
    {0x0A, 0x00},
    {0x0B, 0x02},
    {0x0C, 0x00},
    {0x0D, 0x00},
    {0x0E, 0x02},
    {0x0F, 0x16},
    {0x10, 0x18},
    {0x11, 0x08},
    {0x12, 0x92},
    {0x13, 0x00},
    {0x14, 0x02},
    {0x15, 0x05},
    {0x16, 0x40},
    {0x17, 0x03},
    {0x18, 0x16},
    {0x19, 0xD7},
    {0x1A, 0x01},
    {0x1B, 0xDC},
    {0x1C, 0x00},
    {0x1D, 0x04},
    {0x1E, 0x00},
    {0x1F, 0x00},
    {0x20, 0x03},
    {0x21, 0x16},
    {0x22, 0x18},
    {0x23, 0x08},
    {0x24, 0xDC},
    {0x25, 0x00},
    {0x26, 0x04},
    {0x27, 0x00},
    {0x28, 0x00},
    {0x29, 0x01},
    {0x2A, 0x16},
    {0x2B, 0x18},
    {0x2D, 0x08},
    {0x4C, 0x99},
    {0x4D, 0x00},
    {0x4E, 0x00},
    {0x4F, 0x00},
    {0x50, 0x01},
    {0x51, 0x0A},
    {0x52, 0x00},
    {0x5A, 0xE4},
    {0x5E, 0x77},
    {0x5F, 0x77},
    {0x60, 0x34},
    {0x61, 0x02},
    {0x62, 0x81},

    /* Page 6 */
    {0xFE, 0x07},
    {0x07, 0x4F},

    /* Page 0 */
    {0xFE, 0x01},
    /* Display Resolution Panel Option */
    {0x05, 0x15},
    /* DDVDH Charge Pump Control Normal Mode */
    {0x0E, 0x8B},
    /* DDVDH Charge Pump Control ldle Mode */
    {0x0F, 0x8B},
    /* DDVDH/VCL Regulator Enable */
    {0x10, 0x11},
    /* VCL Charge Pump Control Normal Mode */
    {0x11, 0xA2},
    /* VCL Charge Pump Control Idle Mode */
    {0x12, 0xA0},
    /* VGH Charge Pump Control ldle Mode */
    {0x14, 0xA1},
    /* VGL Charge Pump Control Normal Mode */
    {0x15, 0x82},
    /* VGHR Control */
    {0x18, 0x47},
    /* VGLR Control */
    {0x19, 0x36},
    /* VREFPN5 REGULATOR ENABLE */
    {0x1A, 0x10},
    /* VREFPN5 */
    {0x1C, 0x57},
    /* SWITCH EQ Control */
    {0x1D, 0x02},
    /* VGMP Control */
    {0x21, 0xF8},
    /* VGSP Control */
    {0x22, 0x90},
    /* VGMP / VGSP control */
    {0x23, 0x00},
    /* Low Frame Rate Control Normal Mode */
    {0x25, 0x03},
    {0x26, 0x4a},
    /* Low Frame Rate Control Idle Mode */
    {0x2A, 0x03},
    {0x2B, 0x4A},
    {0x2D, 0x12},
    {0x2F, 0x12},

    {0x30, 0x45},

    /* Source Control */
    {0x37, 0x0C},
    /* Switch Timing Control */
    {0x3A, 0x00},
    {0x3B, 0x20},
    {0x3D, 0x0B},
    {0x3F, 0x38},
    {0x40, 0x0B},
    {0x41, 0x0B},

    /* Switch Output Selection */
    {0x42, 0x33},
    {0x43, 0x66},
    {0x44, 0x11},
    {0x45, 0x44},
    {0x46, 0x22},
    {0x47, 0x55},
    {0x4C, 0x33},
    {0x4D, 0x66},
    {0x4E, 0x11},
    {0x4f, 0x44},
    {0x50, 0x22},
    {0x51, 0x55},

    /* Source Data Output Selection */
    {0x56, 0x11},
    {0x58, 0x44},
    {0x59, 0x22},
    {0x5A, 0x55},
    {0x5B, 0x33},
    {0x5C, 0x66},
    {0x61, 0x11},
    {0x62, 0x44},
    {0x63, 0x22},
    {0x64, 0x55},
    {0x65, 0x33},
    {0x66, 0x66},

    {0x6D, 0x90},
    {0x6E, 0x40},

    /* Source Sequence 2 */
    {0x70, 0xA5},

    /* OVDD control */
    {0x72, 0x04},

    /* OVSS control */
    {0x73, 0x15},

    /* Page 9 */
    {0xFE, 0x0A},
    {0x29, 0x10},

    /* Page 4 */
    {0xFE, 0x05},
    /* ELVSS -2.4V(RT4723). 0x15: RT4723. 0x01: RT4723B. 0x17: STAM1332. */
    {0x05, 0x15},

    {0xFE, 0x00},
    /* enable TE. */
    {0x35, 0x00},
};

static const struct display_timing default_timing = {
	.pixelclock.typ		= 12000000,
	.hactive.typ		= 400,
	.hfront_porch.typ	= 20,
	.hback_porch.typ	= 20,
	.hsync_len.typ		= 40,
	.vactive.typ		= 400,
	.vfront_porch.typ	= 20,
	.vback_porch.typ	= 4,
	.vsync_len.typ		= 12,
};

static u8 color_format_from_dsi_format(enum mipi_dsi_pixel_format format)
{
	switch (format) {
	case MIPI_DSI_FMT_RGB565:
		return 0x75;
	case MIPI_DSI_FMT_RGB666:
	case MIPI_DSI_FMT_RGB666_PACKED:
		return 0x76;
	case MIPI_DSI_FMT_RGB888:
		return 0x77;
	default:
		return 0x77; /* for backward compatibility */
	}
};

static int usmp_panel_push_cmd_list(struct mipi_dsi_device *device,
				   const cmd_set_table *cmd_set,
				   size_t count)
{
	size_t i;
	const cmd_set_table *cmd;
	int ret = 0;

	for (i = 0; i < count; i++) {
		cmd = cmd_set++;
		ret = mipi_dsi_generic_write(device, cmd, CMD_TABLE_LEN);
		if (ret < 0)
			return ret;
	}

	return ret;
};

static int rm67162_enable(struct udevice *dev)
{
	struct rm67162_panel_priv *priv = dev_get_priv(dev);
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct mipi_dsi_device *dsi = plat->device;
	u8 color_format = color_format_from_dsi_format(priv->format);
	u16 brightness;
	u8 dsi_mode;
	int ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	ret = usmp_panel_push_cmd_list(dsi, &mcs_rm67162_400x400[0],
		sizeof(mcs_rm67162_400x400) / CMD_TABLE_LEN);
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

	/* Wait 16ms for panel out of reset */
	mdelay(16);

	/* Set DSI mode */
	dsi_mode = (dsi->mode_flags & MIPI_DSI_MODE_VIDEO) ? 0x0B : 0x00;
	ret = mipi_dsi_generic_write(dsi, (u8[]){ 0xC2, dsi_mode }, 2);
	if (ret < 0) {
		dev_err(dev, "Failed to set DSI mode (%d)\n", ret);
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

	/* Exit sleep mode */
	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to exit sleep mode (%d)\n", ret);
		return -EIO;
	}

	mdelay(121);

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0) {
		printf("Failed to set display ON (%d)\n", ret);
		return -EIO;
	}

	//mdelay(50);
	/* Set display brightness */
	brightness = 255; /* Max brightness */
	ret = mipi_dsi_dcs_write(dsi, MIPI_DCS_SET_DISPLAY_BRIGHTNESS, &brightness, 2);
	if (ret < 0) {
		printf("Failed to set display brightness (%d)\n", ret);
		return -EIO;
	}

	return 0;
}

static int rm67162_panel_enable_backlight(struct udevice *dev)
{
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct mipi_dsi_device *device = plat->device;
	int ret;

	ret = mipi_dsi_attach(device);
	if (ret < 0)
		return ret;

	return rm67162_enable(dev);
}

static int rm67162_panel_get_display_timing(struct udevice *dev,
					    struct display_timing *timings)
{
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct mipi_dsi_device *device = plat->device;
	struct rm67162_panel_priv *priv = dev_get_priv(dev);

	memcpy(timings, &default_timing, sizeof(*timings));

	/* fill characteristics of DSI data link */
	if (device) {
		device->lanes = priv->lanes;
		device->format = priv->format;
		device->mode_flags = priv->mode_flags;
	}

	return 0;
}

static int rm67162_panel_probe(struct udevice *dev)
{
	struct rm67162_panel_priv *priv = dev_get_priv(dev);
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

	if (dev_read_bool(dev, "reset,otherway")) {
		return 0;
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
	mdelay(1);
	ret = dm_gpio_set_value(&priv->reset, false);
	if (ret)
		printf("reset gpio fails to set true\n");
	mdelay(12);

	return 0;
}

static int rm67162_panel_disable(struct udevice *dev)
{
	struct rm67162_panel_priv *priv = dev_get_priv(dev);

	if (&priv->reset != NULL)
		dm_gpio_set_value(&priv->reset, true);

	return 0;
}

static const struct panel_ops rm67162_panel_ops = {
	.enable_backlight = rm67162_panel_enable_backlight,
	.get_display_timing = rm67162_panel_get_display_timing,
};

static const struct udevice_id rm67162_panel_ids[] = {
	{ .compatible = "usmp,rm67162" },
	{ }
};

U_BOOT_DRIVER(rm67162_panel) = {
	.name			  = "rm67162_panel",
	.id			  = UCLASS_PANEL,
	.of_match		  = rm67162_panel_ids,
	.ops			  = &rm67162_panel_ops,
	.probe			  = rm67162_panel_probe,
	.remove			  = rm67162_panel_disable,
	.plat_auto = sizeof(struct mipi_dsi_panel_plat),
	.priv_auto = sizeof(struct rm67162_panel_priv),
};
