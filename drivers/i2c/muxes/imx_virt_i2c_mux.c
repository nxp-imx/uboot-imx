/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <i2c.h>

#include <asm-generic/gpio.h>

DECLARE_GLOBAL_DATA_PTR;

#define I2C_M_SELECT_MUX_BUS	0x010000

struct imx_virt_i2c_mux_priv {
	u32 addr; /* I2C mux address */
	u32 i2c_bus_alias_off;
};

static int imx_virt_i2c_mux_deselect(struct udevice *mux, struct udevice *bus,
			    uint channel)
{
	return i2c_set_chip_flags(mux, 0);
}

static int imx_virt_i2c_mux_select(struct udevice *mux, struct udevice *bus,
			  uint channel)
{
	struct imx_virt_i2c_mux_priv *priv = dev_get_priv(mux);
	uint flags = I2C_M_SELECT_MUX_BUS;

	flags |= ((priv->i2c_bus_alias_off + channel) << 24);

	return i2c_set_chip_flags(mux, flags);
}

static const struct i2c_mux_ops imx_virt_i2c_mux_ops = {
	.select = imx_virt_i2c_mux_select,
	.deselect = imx_virt_i2c_mux_deselect,
};

static const struct udevice_id imx_virt_i2c_mux_ids[] = {
	{ .compatible = "fsl,imx-virt-i2c-mux", },
	{ }
};

static int imx_virt_i2c_mux_probe(struct udevice *dev)
{
	struct imx_virt_i2c_mux_priv *priv = dev_get_priv(dev);

	priv->addr = fdtdec_get_int(gd->fdt_blob, dev_of_offset(dev), "reg", 0);
	if (!priv->addr) {
		debug("MUX not found\n");
		return -ENODEV;
	}

	priv->i2c_bus_alias_off = fdtdec_get_int(gd->fdt_blob, dev_of_offset(dev), "virtual-bus-seq", 0);

	debug("Device %s at 0x%x with i2c_bus_alias_off %d\n",
	      dev->name, priv->addr, priv->i2c_bus_alias_off);
	return 0;
}

U_BOOT_DRIVER(imx_virt_i2c_mux) = {
	.name = "imx_virt_i2c_mux",
	.id = UCLASS_I2C_MUX,
	.of_match = imx_virt_i2c_mux_ids,
	.probe = imx_virt_i2c_mux_probe,
	.ops = &imx_virt_i2c_mux_ops,
	.priv_auto_alloc_size = sizeof(struct imx_virt_i2c_mux_priv),
};
