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
#include <i2c.h>
#include <linux/err.h>

struct adv7535_priv {
	unsigned int addr;
	unsigned int addr_cec;
	unsigned int lanes;
	enum mipi_dsi_pixel_format format;
	unsigned long mode_flags;
	struct udevice *cec_dev;
};

static const struct display_timing default_timing = {
	.pixelclock.typ		= 148500000,
	.hactive.typ		= 1920,
	.hfront_porch.typ	= 88,
	.hback_porch.typ	= 148,
	.hsync_len.typ		= 44,
	.vactive.typ		= 1080,
	.vfront_porch.typ	= 4,
	.vback_porch.typ	= 36,
	.vsync_len.typ		= 5,
};

static int adv7535_i2c_reg_write(struct udevice *dev, uint addr, uint mask, uint data)
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

static int adv7535_i2c_reg_read(struct udevice *dev, uint8_t addr, uint8_t *data)
{
	uint8_t valb;
	int err;

	err = dm_i2c_read(dev, addr, &valb, 1);
	if (err)
		return err;

	*data = (int)valb;
	return 0;
}

static int adv7535_enable(struct udevice *dev)
{
	struct adv7535_priv *priv = dev_get_priv(dev);
	uint8_t val;

	adv7535_i2c_reg_read(dev, 0x00, &val);
	debug("Chip revision: 0x%x (expected: 0x14)\n", val);
	adv7535_i2c_reg_read(priv->cec_dev, 0x00, &val);
	debug("Chip ID MSB: 0x%x (expected: 0x75)\n", val);
	adv7535_i2c_reg_read(priv->cec_dev, 0x01, &val);
	debug("Chip ID LSB: 0x%x (expected: 0x33)\n", val);

	/* Power */
	adv7535_i2c_reg_write(dev, 0x41, 0xff, 0x10);
	/* Initialisation (Fixed) Registers */
	adv7535_i2c_reg_write(dev, 0x16, 0xff, 0x20);
	adv7535_i2c_reg_write(dev, 0x9A, 0xff, 0xE0);
	adv7535_i2c_reg_write(dev, 0xBA, 0xff, 0x70);
	adv7535_i2c_reg_write(dev, 0xDE, 0xff, 0x82);
	adv7535_i2c_reg_write(dev, 0xE4, 0xff, 0x40);
	adv7535_i2c_reg_write(dev, 0xE5, 0xff, 0x80);
	adv7535_i2c_reg_write(priv->cec_dev, 0x15, 0xff, 0xD0);
	adv7535_i2c_reg_write(priv->cec_dev, 0x17, 0xff, 0xD0);
	adv7535_i2c_reg_write(priv->cec_dev, 0x24, 0xff, 0x20);
	adv7535_i2c_reg_write(priv->cec_dev, 0x57, 0xff, 0x11);
	adv7535_i2c_reg_write(priv->cec_dev, 0x05, 0xff, 0xc8);

	/* 4 x DSI Lanes */
	adv7535_i2c_reg_write(priv->cec_dev, 0x1C, 0xff, 0x40);

	/* DSI Pixel Clock Divider */
	//adv7535_i2c_reg_write(priv->cec_dev, 0x16, 0xff, 0x0);
	adv7535_i2c_reg_write(priv->cec_dev, 0x16, 0xff, 0x18);

	/* Enable Internal Timing Generator */
	adv7535_i2c_reg_write(priv->cec_dev, 0x27, 0xff, 0xCB);
	/* 1920 x 1080p 60Hz */
	adv7535_i2c_reg_write(priv->cec_dev, 0x28, 0xff, 0x89); /* total width */
	adv7535_i2c_reg_write(priv->cec_dev, 0x29, 0xff, 0x80); /* total width */
	adv7535_i2c_reg_write(priv->cec_dev, 0x2A, 0xff, 0x02); /* hsync */
	adv7535_i2c_reg_write(priv->cec_dev, 0x2B, 0xff, 0xC0); /* hsync */
	adv7535_i2c_reg_write(priv->cec_dev, 0x2C, 0xff, 0x05); /* hfp */
	adv7535_i2c_reg_write(priv->cec_dev, 0x2D, 0xff, 0x80); /* hfp */
	adv7535_i2c_reg_write(priv->cec_dev, 0x2E, 0xff, 0x09); /* hbp */
	adv7535_i2c_reg_write(priv->cec_dev, 0x2F, 0xff, 0x40); /* hbp */

	adv7535_i2c_reg_write(priv->cec_dev, 0x30, 0xff, 0x46); /* total height */
	adv7535_i2c_reg_write(priv->cec_dev, 0x31, 0xff, 0x50); /* total height */
	adv7535_i2c_reg_write(priv->cec_dev, 0x32, 0xff, 0x00); /* vsync */
	adv7535_i2c_reg_write(priv->cec_dev, 0x33, 0xff, 0x50); /* vsync */
	adv7535_i2c_reg_write(priv->cec_dev, 0x34, 0xff, 0x00); /* vfp */
	adv7535_i2c_reg_write(priv->cec_dev, 0x35, 0xff, 0x40); /* vfp */
	adv7535_i2c_reg_write(priv->cec_dev, 0x36, 0xff, 0x02); /* vbp */
	adv7535_i2c_reg_write(priv->cec_dev, 0x37, 0xff, 0x40); /* vbp */

	/* Reset Internal Timing Generator */
	adv7535_i2c_reg_write(priv->cec_dev, 0x27, 0xff, 0xCB);
	adv7535_i2c_reg_write(priv->cec_dev, 0x27, 0xff, 0x8B);
	adv7535_i2c_reg_write(priv->cec_dev, 0x27, 0xff, 0xCB);

	/* HDMI Output */
	adv7535_i2c_reg_write(dev, 0xAF, 0xff, 0x16);
	/* AVI Infoframe - RGB - 16-9 Aspect Ratio */
	adv7535_i2c_reg_write(dev, 0x55, 0xff, 0x10);
	//adv7535_i2c_reg_write(dev, 0x55, 0xff, 0x02);
	adv7535_i2c_reg_write(dev, 0x56, 0xff, 0x28);
	//adv7535_i2c_reg_write(dev, 0x56, 0xff, 0x0);

	/*  GC Packet Enable */
	adv7535_i2c_reg_write(dev, 0x40, 0xff, 0x80);
	//adv7535_i2c_reg_write(dev, 0x40, 0xff, 0x0);
	/*  GC Colour Depth - 24 Bit */
	adv7535_i2c_reg_write(dev, 0x4C, 0xff, 0x04);
	//adv7535_i2c_reg_write(dev, 0x4C, 0xff, 0x0);
	/*  Down Dither Output Colour Depth - 8 Bit (default) */
	adv7535_i2c_reg_write(dev, 0x49, 0xff, 0x00);

	/* set low refresh 1080p30 */
	adv7535_i2c_reg_write(dev, 0x4A, 0xff, 0x80); /*should be 0x80 for 1080p60 and 0x8c for 1080p30*/

	/* HDMI Output Enable */
	//adv7535_i2c_reg_write(priv->cec_dev, 0xbe, 0xff, 0x3c);
	adv7535_i2c_reg_write(priv->cec_dev, 0xbe, 0xff, 0x3d);
	adv7535_i2c_reg_write(priv->cec_dev, 0x03, 0xff, 0x89);

	return 0;
}

static int adv7535_enable_backlight(struct udevice *dev)
{
	struct mipi_dsi_panel_plat *plat = dev_get_platdata(dev);
	struct mipi_dsi_device *device = plat->device;
	int ret;

	ret = mipi_dsi_attach(device);
	if (ret < 0)
		return ret;

	return 0;
}

static int adv7535_get_display_timing(struct udevice *dev,
					    struct display_timing *timings)
{
	struct mipi_dsi_panel_plat *plat = dev_get_platdata(dev);
	struct mipi_dsi_device *device = plat->device;
	struct adv7535_priv *priv = dev_get_priv(dev);

	memcpy(timings, &default_timing, sizeof(*timings));

	/* fill characteristics of DSI data link */
	if (device) {
		device->lanes = priv->lanes;
		device->format = priv->format;
		device->mode_flags = priv->mode_flags;
	}

	return 0;
}

static int adv7535_probe(struct udevice *dev)
{
	struct adv7535_priv *priv = dev_get_priv(dev);
	int ret;

	debug("%s\n", __func__);

	priv->format = MIPI_DSI_FMT_RGB888;
	priv->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE |
			  MIPI_DSI_MODE_EOT_PACKET | MIPI_DSI_MODE_VIDEO_HSE,

	priv->addr  = dev_read_addr(dev);
	if (priv->addr  == 0)
		return -ENODEV;

	ret = dev_read_u32(dev, "adi,dsi-lanes", &priv->lanes);
	if (ret) {
		dev_err(dev, "Failed to get dsi-lanes property (%d)\n", ret);
		return ret;
	}

	if (priv->lanes < 1 || priv->lanes > 4) {
		dev_err(dev, "Invalid dsi-lanes: %d\n", priv->lanes);
		return -EINVAL;
	}

	ret = dev_read_u32(dev, "adi,addr-cec", &priv->addr_cec);
	if (ret) {
		dev_err(dev, "Failed to get addr-cec property (%d)\n", ret);
		return -EINVAL;
	}

	ret = dm_i2c_probe(dev_get_parent(dev), priv->addr_cec, 0, &priv->cec_dev);
	if (ret) {
		dev_err(dev, "Can't find cec device id=0x%x\n", priv->addr_cec);
		return -ENODEV;
	}

	adv7535_enable(dev);

	return 0;
}

static const struct panel_ops adv7535_ops = {
	.enable_backlight = adv7535_enable_backlight,
	.get_display_timing = adv7535_get_display_timing,
};

static const struct udevice_id adv7535_ids[] = {
	{ .compatible = "adi,adv7533" },
	{ }
};

U_BOOT_DRIVER(adv7535_mipi2hdmi) = {
	.name			  = "adv7535_mipi2hdmi",
	.id			  = UCLASS_PANEL,
	.of_match		  = adv7535_ids,
	.ops			  = &adv7535_ops,
	.probe			  = adv7535_probe,
	.platdata_auto_alloc_size = sizeof(struct mipi_dsi_panel_plat),
	.priv_auto_alloc_size	= sizeof(struct adv7535_priv),
};
