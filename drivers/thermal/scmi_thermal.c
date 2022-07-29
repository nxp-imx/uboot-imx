// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2022 NXP
 */

#include <common.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <thermal.h>
#include <scmi_agent.h>
#include <scmi_protocols.h>
#include <asm/types.h>
#include <dm/device-internal.h>
#include <dm/device.h>

struct scmi_thermal_priv {
	s16 num_sensors;
	s16 thermal_id;
	struct scmi_sensor_descrition_get_p2a *desc_buf;
	size_t desc_buf_size;
};

static int scmi_thermal_get_temp(struct udevice *dev, int *temp)
{
	struct scmi_thermal_priv *priv = dev_get_priv(dev);
	struct scmi_sensor_reading_get_a2p in = { priv->thermal_id };
	struct scmi_sensor_reading_get_p2a out = { 0 };
	struct scmi_msg msg = {
		.protocol_id = SCMI_PROTOCOL_ID_SENSOR,
		.message_id = SCMI_SENSOR_READING_GET,
		.in_msg = (u8 *)&in,
		.in_msg_sz = sizeof(in),
		.out_msg = (u8 *)&out,
		.out_msg_sz = sizeof(out),
	};
	int ret;

	ret = devm_scmi_process_msg(dev->parent, &msg);
	if (ret)
		return ret;

	ret = scmi_to_linux_errno(out.status);
	if (ret < 0)
		return ret;

	*temp = out.val.value_low;

	return ret;
}

static int scmi_sensor_attributes_get(struct udevice *dev)
{
	struct scmi_protocol_attributes_p2a_sensor out = { 0 };
	struct scmi_msg msg = {
		.protocol_id = SCMI_PROTOCOL_ID_SENSOR,
		.message_id = SCMI_PROTOCOL_ATTRIBUTES,
		.in_msg = NULL,
		.in_msg_sz = 0,
		.out_msg = (u8 *)&out,
		.out_msg_sz = sizeof(out),
	};
	int ret;
	struct scmi_thermal_priv *priv = dev_get_priv(dev);

	ret = devm_scmi_process_msg(dev->parent, &msg);
	if (ret)
		return ret;

	ret = scmi_to_linux_errno(out.status);
	if (ret < 0)
		return ret;

	priv->num_sensors = out.num_sensors;

	dev_dbg(dev,"num_sensors %d\n", priv->num_sensors);

	return 0;
}

static int scmi_sensor_description_get(struct udevice *dev, u32 start_ind,
	struct scmi_sensor_descrition_get_p2a *desc_buf, size_t desc_buf_size)
{
	struct scmi_sensor_description_get_a2p in = { start_ind };
	struct scmi_msg msg = {
		.protocol_id = SCMI_PROTOCOL_ID_SENSOR,
		.message_id = SCMI_SENSOR_DESCRIPTION_GET,
		.in_msg = (u8 *)&in,
		.in_msg_sz = sizeof(in),
		.out_msg = (u8 *)desc_buf,
		.out_msg_sz = desc_buf_size,
	};
	int ret;

	ret = devm_scmi_process_msg(dev->parent, &msg);
	if (ret)
		return ret;

	ret = scmi_to_linux_errno(desc_buf->status);
	if (ret < 0)
		return ret;

	return ret;
}

static const struct dm_thermal_ops scmi_thermal_ops = {
	.get_temp	= scmi_thermal_get_temp,
};

static int scmi_thermal_probe(struct udevice *dev)
{
	int ret;
	u16 num_returned, num_remaining, cnt;
	u32 index = 0;
	bool find = false;
	struct scmi_sensor_desc *desc;
	struct scmi_thermal_priv *priv = dev_get_priv(dev);

	ret = scmi_sensor_attributes_get(dev);
	if (ret){
		dev_err(dev, "scmi_sensor_attributes_get failure %d\n", ret);
		return ret;
	}

	priv->desc_buf_size = sizeof(struct scmi_sensor_descrition_get_p2a) +
		(priv->num_sensors - 1) * sizeof(struct scmi_sensor_desc);
	priv->desc_buf = (struct scmi_sensor_descrition_get_p2a *)calloc(1, priv->desc_buf_size);
	if (!priv->desc_buf) {
		dev_err(dev, "allocate desc_buffer failure\n");
		return -ENOMEM;
	}

	do {
		ret = scmi_sensor_description_get(dev, index, priv->desc_buf, priv->desc_buf_size);
		if (ret){
			dev_err(dev, "scmi_sensor_description_get failure %d\n", ret);
			return ret;
		}

		num_returned = priv->desc_buf->num_sensor_flags & 0xffff;
		num_remaining = (priv->desc_buf->num_sensor_flags >> 16) & 0xffff;

		if (index + num_returned > priv->num_sensors) {
			dev_err(dev, "Num of sensors can't exceed %d",
				priv->num_sensors);
			return -EINVAL;
		}

		for (cnt = 0; cnt < num_returned; cnt++) {
			desc = &priv->desc_buf->desc[cnt];
			if ((desc->attr_high & 0xff) == 0x2) {
				priv->thermal_id = desc->id; /* Only get one thermal sensor */
				dev_dbg(dev, "thermal id %u\n", priv->thermal_id);
				find = true;
				break;
			}
		}

		index += num_returned;
	} while (num_returned && num_remaining && !find);

	if (!find) {
		dev_err(dev, "Can't find thermal sensor device\n");
		return -ENODEV;
	}

	return 0;
}

U_BOOT_DRIVER(scmi_thermal) = {
	.name = "scmi_thermal",
	.id = UCLASS_THERMAL,
	.ops = &scmi_thermal_ops,
	.probe = scmi_thermal_probe,
	.flags = DM_FLAG_PRE_RELOC,
	.priv_auto	= sizeof(struct scmi_thermal_priv),
};
