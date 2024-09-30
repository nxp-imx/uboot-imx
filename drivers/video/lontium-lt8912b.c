// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2024 NXP
 *
 */

#include <common.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <dm/ofnode.h>
#include <mipi_dsi.h>
#include <panel.h>
#include <asm/gpio.h>
#include <i2c.h>
#include <linux/err.h>
#include <linux/delay.h>

#define I2C_ADDR_MAIN 0x48
#define I2C_ADDR_CEC_DSI 0x49

struct lt8912_priv {
	unsigned int addr;
	unsigned int addr_cec;
	unsigned int lanes;
	enum mipi_dsi_pixel_format format;
	unsigned long mode_flags;
	struct udevice *cec_dev;
	struct gpio_desc reset;
};

struct reg_sequence {
	unsigned int reg;
	unsigned int val;
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

static int lt8912_i2c_reg_update(struct udevice *dev, uint addr, uint mask, uint data)
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

static int lt8912_i2c_reg_write(struct udevice *dev, uint addr, uint data)
{
	uint8_t valb;
	int err;

	valb = data;
	err = dm_i2c_write(dev, addr, &valb, 1);
	return err;
}

static int lt8912_write_init_config(struct udevice *dev)
{
	int i, ret = 0;
	const struct reg_sequence seq[] = {
		/* Digital clock en*/
		{0x08, 0xff},
		{0x09, 0xff},
		{0x0a, 0xff},
		{0x0b, 0x7c},
		{0x0c, 0xff},
		{0x42, 0x04},

		/*Tx Analog*/
		{0x31, 0xb1},
		{0x32, 0xb1},
		{0x33, 0x0e},
		{0x37, 0x00},
		{0x38, 0x22},
		{0x60, 0x82},

		/*Cbus Analog*/
		{0x39, 0x45},
		{0x3a, 0x00},
		{0x3b, 0x00},

		/*HDMI Pll Analog*/
		{0x44, 0x31},
		{0x55, 0x44},
		{0x57, 0x01},
		{0x5a, 0x02},

		/*MIPI Analog*/
		{0x3e, 0xd6},
		{0x3f, 0xd4},
		{0x41, 0x3c},
		{0xB2, 0x00},
	};

	for (i = 0; i < ARRAY_SIZE(seq); i++) {
		ret = lt8912_i2c_reg_write(dev, seq[i].reg, seq[i].val);
		if (ret) {
			dev_err(dev, "write i2c failed, %d\n", ret);
			break;
		}
	}

	return ret;
}

static int lt8912_write_mipi_basic_config(struct udevice *dev)
{
	int i, ret = 0;
	struct lt8912_priv *priv = dev_get_priv(dev);
	const struct reg_sequence seq[] = {
		{0x12, 0x04},
		{0x14, 0x00},
		{0x15, 0x00},
		{0x1a, 0x03},
		{0x1b, 0x03},
	};

	for (i = 0; i < ARRAY_SIZE(seq); i++) {
		ret = lt8912_i2c_reg_write(priv->cec_dev, seq[i].reg, seq[i].val);
		if (ret) {
			dev_err(dev, "write i2c failed, %d\n", ret);
			break;
		}
	}

	return ret;
};

static int lt8912_write_dds_config(struct udevice *dev)
{
	int i, ret = 0;
	struct lt8912_priv *priv = dev_get_priv(dev);
	const struct reg_sequence seq[] = {
		{0x4e, 0xff},
		{0x4f, 0x56},
		{0x50, 0x69},
		{0x51, 0x80},
		{0x1f, 0x5e},
		{0x20, 0x01},
		{0x21, 0x2c},
		{0x22, 0x01},
		{0x23, 0xfa},
		{0x24, 0x00},
		{0x25, 0xc8},
		{0x26, 0x00},
		{0x27, 0x5e},
		{0x28, 0x01},
		{0x29, 0x2c},
		{0x2a, 0x01},
		{0x2b, 0xfa},
		{0x2c, 0x00},
		{0x2d, 0xc8},
		{0x2e, 0x00},
		{0x42, 0x64},
		{0x43, 0x00},
		{0x44, 0x04},
		{0x45, 0x00},
		{0x46, 0x59},
		{0x47, 0x00},
		{0x48, 0xf2},
		{0x49, 0x06},
		{0x4a, 0x00},
		{0x4b, 0x72},
		{0x4c, 0x45},
		{0x4d, 0x00},
		{0x52, 0x08},
		{0x53, 0x00},
		{0x54, 0xb2},
		{0x55, 0x00},
		{0x56, 0xe4},
		{0x57, 0x0d},
		{0x58, 0x00},
		{0x59, 0xe4},
		{0x5a, 0x8a},
		{0x5b, 0x00},
		{0x5c, 0x34},
		{0x1e, 0x4f},
		{0x51, 0x00},
	};

	for (i = 0; i < ARRAY_SIZE(seq); i++) {
		ret = lt8912_i2c_reg_write(priv->cec_dev, seq[i].reg, seq[i].val);
		if (ret) {
			dev_err(dev, "write i2c failed, %d\n", ret);
			break;
		}
	}

	return ret;
}

static int lt8912_write_rxlogicres_config(struct udevice *dev)
{
	int ret;

	ret = lt8912_i2c_reg_write(dev, 0x03, 0x7f);
	udelay(20000);
	ret |= lt8912_i2c_reg_write(dev, 0x03, 0xff);

	return ret;
};

static int lt8912_hard_power_on(struct lt8912_priv *priv)
{
	dm_gpio_set_value(&priv->reset, 0);
	mdelay(20);

	return 0;
}

static void lt8912_hard_power_off(struct lt8912_priv *priv)
{
	dm_gpio_set_value(&priv->reset, 1);
	mdelay(20);
}

static int lt8912_video_setup(struct udevice *dev)
{
	u32 hactive, h_total, hpw, hfp, hbp;
	u32 vactive, v_total, vpw, vfp, vbp;
	u8 settle = 0x08;
	int ret, hsync_activehigh, vsync_activehigh;

	struct lt8912_priv *priv = dev_get_priv(dev);

	if (!priv)
		return -EINVAL;

	hactive = default_timing.hactive.typ;
	hfp = default_timing.hfront_porch.typ;
	hpw = default_timing.hsync_len.typ;
	hbp = default_timing.hback_porch.typ;
	h_total = hactive + hfp + hpw + hbp;
	hsync_activehigh = default_timing.flags & DISPLAY_FLAGS_HSYNC_HIGH;

	vactive = default_timing.vactive.typ;
	vfp = default_timing.vfront_porch.typ;
	vpw = default_timing.vsync_len.typ;
	vbp = default_timing.vback_porch.typ;
	v_total = vactive + vfp + vpw + vbp;
	vsync_activehigh = default_timing.flags & DISPLAY_FLAGS_VSYNC_HIGH;

	if (vactive <= 600)
		settle = 0x04;
	else if (vactive == 1080)
		settle = 0x0a;

	ret = lt8912_i2c_reg_write(priv->cec_dev, 0x10, 0x01);
	ret |= lt8912_i2c_reg_write(priv->cec_dev, 0x11, settle);
	ret |= lt8912_i2c_reg_write(priv->cec_dev, 0x18, hpw);
	ret |= lt8912_i2c_reg_write(priv->cec_dev, 0x19, vpw);
	ret |= lt8912_i2c_reg_write(priv->cec_dev, 0x1c, hactive & 0xff);
	ret |= lt8912_i2c_reg_write(priv->cec_dev, 0x1d, hactive >> 8);

	ret |= lt8912_i2c_reg_write(priv->cec_dev, 0x2f, 0x0c);

	ret |= lt8912_i2c_reg_write(priv->cec_dev, 0x34, h_total & 0xff);
	ret |= lt8912_i2c_reg_write(priv->cec_dev, 0x35, h_total >> 8);

	ret |= lt8912_i2c_reg_write(priv->cec_dev, 0x36, v_total & 0xff);
	ret |= lt8912_i2c_reg_write(priv->cec_dev, 0x37, v_total >> 8);

	ret |= lt8912_i2c_reg_write(priv->cec_dev, 0x38, vbp & 0xff);
	ret |= lt8912_i2c_reg_write(priv->cec_dev, 0x39, vbp >> 8);

	ret |= lt8912_i2c_reg_write(priv->cec_dev, 0x3a, vfp & 0xff);
	ret |= lt8912_i2c_reg_write(priv->cec_dev, 0x3b, vfp >> 8);

	ret |= lt8912_i2c_reg_write(priv->cec_dev, 0x3c, hbp & 0xff);
	ret |= lt8912_i2c_reg_write(priv->cec_dev, 0x3d, hbp >> 8);

	ret |= lt8912_i2c_reg_write(priv->cec_dev, 0x3e, hfp & 0xff);
	ret |= lt8912_i2c_reg_write(priv->cec_dev, 0x3f, hfp >> 8);

	ret |= lt8912_i2c_reg_update(dev, 0xab, BIT(0),
				  vsync_activehigh ? BIT(0) : 0);
	ret |= lt8912_i2c_reg_update(dev, 0xab, BIT(1),
				  hsync_activehigh ? BIT(1) : 0);
	ret |= lt8912_i2c_reg_update(dev, 0xb2, BIT(0), BIT(0));

	if (ret != 0) {
		dev_err(dev, "video setup failed\n");
		ret = -EIO;
	}

	return ret;
}

static int lt8912_soft_power_on(struct udevice *dev)
{
	struct lt8912_priv *priv = dev_get_priv(dev);

	lt8912_write_init_config(dev);

	lt8912_i2c_reg_write(priv->cec_dev, 0x13, priv->lanes & 3);

	lt8912_write_mipi_basic_config(dev);

	return 0;
}

static int lt8912_video_on(struct udevice *dev)
{
	int ret;

	ret = lt8912_video_setup(dev);
	if (ret < 0)
		return ret;

	ret = lt8912_write_dds_config(dev);
	if (ret < 0)
		return ret;

	ret = lt8912_write_rxlogicres_config(dev);
	if (ret < 0)
		return ret;

	return 0;
}

static int lt8912_enable(struct udevice *dev)
{
	struct lt8912_priv *priv = dev_get_priv(dev);
	lt8912_hard_power_on(priv);

	lt8912_soft_power_on(dev);

	lt8912_video_on(dev);

	return 0;
}

static int lt8912_enable_backlight(struct udevice *dev)
{
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct mipi_dsi_device *device = plat->device;
	int ret;

	ret = mipi_dsi_attach(device);
	if (ret < 0)
		return ret;

	return 0;
}

static int lt8912_get_display_timing(struct udevice *dev,
					    struct display_timing *timings)
{
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct mipi_dsi_device *device = plat->device;
	struct lt8912_priv *priv = dev_get_priv(dev);

	memcpy(timings, &default_timing, sizeof(*timings));

	/* fill characteristics of DSI data link */
	if (device) {
		device->lanes = priv->lanes;
		device->format = priv->format;
		device->mode_flags = priv->mode_flags;
	}

	return 0;
}

ofnode ofnode_graph_get_endpoint_by_regs(
	const ofnode parent, int port_reg, int reg);

static int lt8912_probe(struct udevice *dev)
{
	struct lt8912_priv *priv = dev_get_priv(dev);
	int ret, len;
	ofnode ep;

	priv->format = MIPI_DSI_FMT_RGB888;
	priv->mode_flags = MIPI_DSI_MODE_VIDEO;

	priv->addr  = dev_read_addr(dev);
	if (priv->addr  == 0)
		return -ENODEV;

	ret = gpio_request_by_name(dev, "reset-gpio", 0, &priv->reset,
				   GPIOD_IS_OUT);
	if (ret) {
		dev_err(dev, "Cannot get reset GPIO: ret=%d\n", ret);
		return ret;
	}

	ep = ofnode_graph_get_endpoint_by_regs(dev_ofnode(dev), 0, -1);
	if (!ofnode_valid(ep)) {
		dev_err(dev, "Failed to get endpoint on port 0\n");
		return -ENODEV;
	}

	len = ofnode_read_size(ep, "data-lanes");
	if (len < 0) {
		dev_err(dev, "%s: Error while reading data-lanes size (ret = %d)\n",
		      ofnode_get_name(ep), len);
		return len;
	}
	len /= sizeof(fdt32_t);
	if (len < 1 || len > 4) {
		dev_err(dev, "%s: Invalid data in data-lanes property\n",
		      ofnode_get_name(ep));
		return -EINVAL;
	}

	priv->lanes = len;

	priv->addr_cec = I2C_ADDR_CEC_DSI;

	ret = dm_i2c_probe(dev_get_parent(dev), priv->addr_cec, 0, &priv->cec_dev);
	if (ret) {
		dev_err(dev, "Can't find cec device id=0x%x\n", priv->addr_cec);
		return -ENODEV;
	}

	lt8912_enable(dev);

	return 0;
}

static int lt8912_remove(struct udevice *dev)
{
	struct lt8912_priv *priv = dev_get_priv(dev);
	lt8912_hard_power_off(priv);

	return 0;
}

static const struct panel_ops lt8912_ops = {
	.enable_backlight = lt8912_enable_backlight,
	.get_display_timing = lt8912_get_display_timing,
};

static const struct udevice_id lt8912_ids[] = {
	{ .compatible = "lontium,lt8912b" },
	{ }
};

U_BOOT_DRIVER(lt8912_mipi2hdmi) = {
	.name			  = "lt8912_mipi2hdmi",
	.id			  = UCLASS_PANEL,
	.of_match		  = lt8912_ids,
	.ops			  = &lt8912_ops,
	.probe			  = lt8912_probe,
	.plat_auto = sizeof(struct mipi_dsi_panel_plat),
	.priv_auto	= sizeof(struct lt8912_priv),
	.remove		= lt8912_remove,
};
