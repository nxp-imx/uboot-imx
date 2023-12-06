// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 NXP
 */

#include <common.h>
#include <fdtdec.h>
#include <errno.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <i2c.h>
#include <linux/err.h>
#include <log.h>
#include <asm/global_data.h>
#include <asm-generic/gpio.h>
#include <power/pmic.h>
#include <power/regulator.h>
#include <power/pf0900.h>

DECLARE_GLOBAL_DATA_PTR;

static const struct pmic_child_info pmic_children_info[] = {
	/* VAON */
	{ .prefix = "V", .driver = PF0900_REGULATOR_DRIVER},
	/* sw */
	{ .prefix = "S", .driver = PF0900_REGULATOR_DRIVER},
	/* ldo */
	{ .prefix = "L", .driver = PF0900_REGULATOR_DRIVER},
	{ },
};

static int pf0900_reg_count(struct udevice *dev)
{
	return PF0900_MAX_REGISTER;
}

static u8 crc8_j1850(u8 *data, u8 length)
{
	u8 t_crc;
	u8 i, j;

	t_crc = 0xFF;
	for (i = 0; i < length; i++) {
		t_crc ^= data[i];
		for (j = 0; j < 8; j++) {
			if ((t_crc & 0x80) != 0) {
				t_crc <<= 1;
				t_crc ^= 0x1D;
			} else {
				t_crc <<= 1;
			}
		}
	}
	return t_crc;
}

static int pf0900_read(struct udevice *dev, uint reg, u8 *buff,
		       int len)
{
	u8 crcBuf[3];
	u8 data[2], crc;
	int ret;
	struct pf0900_priv *priv = dev_get_priv(dev);

	if (reg < PF0900_MAX_REGISTER) {
		ret = dm_i2c_read(dev, reg, data,
				  priv->crc_en ? 2U : 1U);
		if (ret)
			return ret;
		buff[0] = data[0];
		if (priv->crc_en) {
			/* Get CRC */
			crcBuf[0] = priv->addr << 1U | 0x1U;
			crcBuf[1] = reg;
			crcBuf[2] = data[0];
			crc = crc8_j1850(crcBuf, 3U);
			if (crc != data[1])
				return -EINVAL;
		}
	} else {
		return -EINVAL;
	}
	return ret;
}

static int pf0900_write(struct udevice *dev, uint reg, const u8 *buff,
			int len)
{
	u8 crcBuf[3];
	u8 data[2];
	int ret;
	struct pf0900_priv *priv = dev_get_priv(dev);

	if (reg < PF0900_MAX_REGISTER) {
		data[0] = buff[0];
		if (priv->crc_en) {
			/* Get CRC */
			crcBuf[0] = priv->addr << 1U;
			crcBuf[1] = reg;
			crcBuf[2] = data[0];
			data[1] = crc8_j1850(crcBuf, 3U);
		}
		/* Write data */
		ret = dm_i2c_write(dev, reg, data,
				   priv->crc_en ? 2U : 1U);
		if (ret)
			return ret;
	}
	return ret;
}

static int pf0900_bind(struct udevice *dev)
{
	int children;
	ofnode regulators_node;

	regulators_node = dev_read_subnode(dev, "regulators");
	if (!ofnode_valid(regulators_node)) {
		debug("%s: %s regulators subnode not found!", __func__,
		      dev->name);
		return -ENXIO;
	}

	children = pmic_bind_children(dev, regulators_node,
				      pmic_children_info);
	if (!children)
		debug("%s: %s - no child found\n", __func__, dev->name);

	/* Always return success for this device */
	return 0;
}

static int pf0900_probe(struct udevice *dev)
{
	struct pf0900_priv *priv = dev_get_priv(dev);
	unsigned int reg;
	u8 dev_id;
	int ret = 0;

	ret = ofnode_read_u32(dev_ofnode(dev), "reg", &reg);
	if (ret)
		return ret;
	priv->addr = reg;

	if (ofnode_read_bool(dev_ofnode(dev), "i2c-crc-enable"))
		priv->crc_en = true;
	ret = pf0900_read(dev, PF0900_REG_DEV_ID, &dev_id, 1);
	if (ret)
		return ret;
	if ((dev_id & 0x1F) == 0)
		return ret;
	else
		return -EINVAL;
}

static struct dm_pmic_ops pf0900_ops = {
	.reg_count = pf0900_reg_count,
	.read = pf0900_read,
	.write = pf0900_write,
};

static const struct udevice_id pf0900_ids[] = {
	{ .compatible = "nxp,pf0900", .data = PF0900_TYPE_PF0900, },
	{ }
};

U_BOOT_DRIVER(pmic_pf0900) = {
	.name = "pf0900 pmic",
	.id = UCLASS_PMIC,
	.of_match = pf0900_ids,
	.bind = pf0900_bind,
	.probe = pf0900_probe,
	.ops = &pf0900_ops,
	.priv_auto = sizeof(struct pf0900_priv),
};
