// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 *
 */

#include <common.h>
#include <dm.h>
#include <video.h>
#include <video_bridge.h>
#include <asm/gpio.h>
#include <i2c.h>

struct it6263_priv {
	unsigned int addr;
};

static int it6263_i2c_reg_write(struct udevice *dev, uint addr, uint mask, uint data)
{
	uint8_t valb;
	int err;

	if (mask != 0xff) {
		err = dm_i2c_read(dev, addr, &valb, 1);
		if (err) {
			printf("%s, read err %d\n", __func__, err);
			return err;
		}

		valb &= ~mask;
		valb |= data;
	} else {
		valb = data;
	}

	err = dm_i2c_write(dev, addr, &valb, 1);
	if (err) {
		printf("%s, write err %d\n", __func__, err);
	}
	return err;
}

static int it6263_i2c_reg_read(struct udevice *dev, uint8_t addr, uint8_t *data)
{
	uint8_t valb;
	int err;

	err = dm_i2c_read(dev, addr, &valb, 1);
	if (err) {
		printf("%s, read err %d\n", __func__, err);
		return err;
	}

	*data = (int)valb;
	return 0;
}

static int it6263_enable(struct udevice *dev)
{
	uint8_t data;
	int ret;

	ret = it6263_i2c_reg_read(dev, 0x00, &data);
	if (ret) {
		printf("faill to read from it6263 revision, ret %d\n", ret);
		return ret;
	}

	/* InitIT626X(): start */
	it6263_i2c_reg_write(dev, 0x04, 0xff, 0x3d);
	it6263_i2c_reg_write(dev, 0x0f, 0xff, 0x00);
	it6263_i2c_reg_write(dev, 0x05, 0xff, 0x40);
	it6263_i2c_reg_write(dev, 0x04, 0xff, 0x15);
	it6263_i2c_reg_write(dev, 0x0f, 0xff, 0x00);
	it6263_i2c_reg_write(dev, 0x1d, 0xff, 0x66);
	it6263_i2c_reg_write(dev, 0x1e, 0xff, 0x01);

	it6263_i2c_reg_write(dev, 0x61, 0xff, 0x30);
	it6263_i2c_reg_read(dev, 0xf3, &data); /* -> 0x00 */
	it6263_i2c_reg_write(dev, 0xf3, 0xff, data & ~0x30);
	it6263_i2c_reg_read(dev, 0xf3, &data); /* -> 0x00 */
	it6263_i2c_reg_write(dev, 0xf3, 0xff, data | 0x20);

	it6263_i2c_reg_write(dev, 0x09, 0xff, 0x30);
	it6263_i2c_reg_write(dev, 0x0a, 0xff, 0xf8);
	it6263_i2c_reg_write(dev, 0x0b, 0xff, 0x37);
	it6263_i2c_reg_write(dev, 0x0f, 0xff, 0x00);
	it6263_i2c_reg_write(dev, 0xc9, 0xff, 0x00);
	it6263_i2c_reg_write(dev, 0xca, 0xff, 0x00);
	it6263_i2c_reg_write(dev, 0xcb, 0xff, 0x00);
	it6263_i2c_reg_write(dev, 0xcc, 0xff, 0x00);
	it6263_i2c_reg_write(dev, 0xcd, 0xff, 0x00);
	it6263_i2c_reg_write(dev, 0xce, 0xff, 0x00);
	it6263_i2c_reg_write(dev, 0xcf, 0xff, 0x00);
	it6263_i2c_reg_write(dev, 0xd0, 0xff, 0x00);
	it6263_i2c_reg_write(dev, 0x0f, 0xff, 0x01);

	it6263_i2c_reg_read(dev, 0x58, &data); /* -> 0x00 */
	it6263_i2c_reg_write(dev, 0x58, 0xff, data & ~(3 << 5));

	it6263_i2c_reg_write(dev, 0x0f, 0xff, 0x00);
	it6263_i2c_reg_write(dev, 0xe1, 0xff, 0x00);
	it6263_i2c_reg_write(dev, 0x0c, 0xff, 0xff);
	it6263_i2c_reg_write(dev, 0x0d, 0xff, 0xff);
	it6263_i2c_reg_read(dev, 0x0e, &data); /* -> 0x00 */
	it6263_i2c_reg_write(dev, 0x0e, 0xff, (data | 0x3));
	it6263_i2c_reg_write(dev, 0x0e, 0xff, (data & 0xfe));
	it6263_i2c_reg_write(dev, 0x0f, 0xff, 0x01);
	it6263_i2c_reg_write(dev, 0x33, 0xff, 0x00);
	it6263_i2c_reg_write(dev, 0x34, 0xff, 0x18);
	it6263_i2c_reg_write(dev, 0x35, 0xff, 0x00);
	it6263_i2c_reg_write(dev, 0x0f, 0xff, 0x00);
	it6263_i2c_reg_write(dev, 0xc4, 0xff, 0xfe);
	it6263_i2c_reg_read(dev, 0xc5, &data); /* -> 0x00 */
	it6263_i2c_reg_write(dev, 0xc5, 0xff, data | 0x30);
	/* InitIT626X  end */

	it6263_i2c_reg_write(dev, 0x0f, 0xff, 0x00);
	it6263_i2c_reg_write(dev, 0x04, 0xff, 0x3d);
	it6263_i2c_reg_write(dev, 0x04, 0xff, 0x15);
	it6263_i2c_reg_write(dev, 0x0f, 0xff, 0x00);
	it6263_i2c_reg_write(dev, 0x1d, 0xff, 0x66);
	it6263_i2c_reg_write(dev, 0x1e, 0xff, 0x01);

	it6263_i2c_reg_read(dev, 0xc1, &data); /* -> 0x00 */
	it6263_i2c_reg_write(dev, 0x61, 0xff, 0x10);

	/* SetupAFE(): */
	it6263_i2c_reg_write(dev, 0x62, 0xff, 0x88);
	it6263_i2c_reg_write(dev, 0x63, 0xff, 0x10);
	it6263_i2c_reg_write(dev, 0x64, 0xff, 0x84);
	/* SetupAFE(): end */

	it6263_i2c_reg_read(dev, 0x04, &data); /* -> 0x00 */
	it6263_i2c_reg_write(dev, 0x04, 0xff, 0x1d);

	it6263_i2c_reg_read(dev, 0x04, &data); /* -> 0x00 */
	it6263_i2c_reg_write(dev, 0x04, 0xff, 0x15);

	it6263_i2c_reg_read(dev, 0x0e, &data); /* -> 0x00 */

	/*  Wait video stable */
	it6263_i2c_reg_read(dev, 0x0e, &data); /* -> 0x00 */

	/* Reset Video */
	it6263_i2c_reg_read(dev, 0x0d, &data); /* -> 0x00 */
	it6263_i2c_reg_write(dev, 0x0d, 0xff, 0x40);
	it6263_i2c_reg_read(dev, 0x0e, &data); /* -> 0x00 */
	it6263_i2c_reg_write(dev, 0x0e, 0xff, 0x7d);
	it6263_i2c_reg_write(dev, 0x0e, 0xff, 0x7c);
	it6263_i2c_reg_write(dev, 0x0f, 0xff, 0x00);
	it6263_i2c_reg_write(dev, 0x61, 0xff, 0x00);
	it6263_i2c_reg_read(dev, 0x61, &data); /* -> 0x00 */
	it6263_i2c_reg_read(dev, 0x62, &data); /* -> 0x00 */
	it6263_i2c_reg_read(dev, 0x63, &data); /* -> 0x00 */
	it6263_i2c_reg_read(dev, 0x64, &data); /* -> 0x00 */
	it6263_i2c_reg_read(dev, 0x65, &data); /* -> 0x00 */
	it6263_i2c_reg_read(dev, 0x66, &data); /* -> 0x00 */
	it6263_i2c_reg_read(dev, 0x67, &data); /* -> 0x00 */
	it6263_i2c_reg_write(dev, 0x0f, 0xff, 0x00);
	it6263_i2c_reg_read(dev, 0xc1, &data); /* -> 0x00 */
	it6263_i2c_reg_write(dev, 0xc1, 0xff, 0x00);
	it6263_i2c_reg_write(dev, 0xc6, 0xff, 0x03);
	/* Clear AV mute */

	return 0;
}

static int it6263_attach(struct udevice *dev)
{
	return 0;
}

static int it6263_set_backlight(struct udevice *dev, int percent)
{
	debug("%s\n", __func__);

	mdelay(10);
	it6263_enable(dev);
	return 0;
}

static int it6263_probe(struct udevice *dev)
{
	struct it6263_priv *priv = dev_get_priv(dev);

	debug("%s\n", __func__);

	priv->addr  = dev_read_addr(dev);
	if (priv->addr  == 0)
		return -ENODEV;

	return 0;
}

struct video_bridge_ops it6263_ops = {
	.attach = it6263_attach,
	.set_backlight = it6263_set_backlight,
};

static const struct udevice_id it6263_ids[] = {
	{ .compatible = "ite,it6263" },
	{ }
};

U_BOOT_DRIVER(it6263_bridge) = {
	.name			  = "it6263_bridge",
	.id			  = UCLASS_VIDEO_BRIDGE,
	.of_match		  = it6263_ids,
	.ops			  = &it6263_ops,
	.bind			= dm_scan_fdt_dev,
	.probe			  = it6263_probe,
	.priv_auto_alloc_size	= sizeof(struct it6263_priv),
};
