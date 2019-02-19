/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <dm.h>
#include <fdtdec.h>
#include <i2c.h>
#include <asm/mach-imx/imx_vservice.h>

DECLARE_GLOBAL_DATA_PTR;

#define MAX_SRTM_I2C_BUF_SIZE 16
#define SRTM_I2C_CATEGORY 0x09
#define SRTM_VERSION 0x0001
#define SRTM_TYPE_REQ 0x0
#define SRTM_TYPE_RESP 0x1
#define SRTM_CMD_READ 0x0
#define SRTM_CMD_WRITE 0x1

#define I2C_M_SELECT_MUX_BUS	0x010000
#define I2C_M_SRTM_STOP       0x0200

struct imx_virt_i2c_bus {
	int index;
	ulong base;
	struct imx_vservice_channel *vservice;
};

struct imx_srtm_i2c_msg {
	u8 categary;
	u8 version[2];
	u8 type;
	u8 command;
	u8 priority;
	u8 reserved[4];

	u8 i2c_bus;
	u8 return_val;
	u16 slave_addr;
	u16 flag;
	u16 data_length;
	u8 data_buf[MAX_SRTM_I2C_BUF_SIZE];
};

static void imx_virt_i2c_msg_dump(struct imx_srtm_i2c_msg *msg)
{
	u32 i = 0;
	u32 size = sizeof(struct imx_srtm_i2c_msg);
	u8 *buf = (u8 *)msg;

	for (; i < size; i++) {
		debug("%02x ", buf[i]);
		if (i % 16 == 15)
			debug("\n");
	}
}

static int imx_virt_i2c_read(struct udevice *bus, u32 chip, u8 *buf, int len, uint flag)
{
	struct imx_srtm_i2c_msg *msg;
	u32 size;
	int ret = 0;
	struct imx_virt_i2c_bus *i2c_bus = dev_get_priv(bus);

	debug("imx_virt_i2c_read, bus %d\n", i2c_bus->index);

	if (len > MAX_SRTM_I2C_BUF_SIZE) {
		printf("virt_i2c_read exceed the buf length, len=%d\n", len);
		return -EINVAL;
	}

	size = sizeof(struct imx_srtm_i2c_msg);
	msg = imx_vservice_get_buffer(i2c_bus->vservice, size);
	if (msg == NULL)
		return -ENOMEM;

	/* Fill buf with SRTM i2c format */
	msg->categary = SRTM_I2C_CATEGORY;
	msg->version[0] = SRTM_VERSION & 0xff;
	msg->version[1] = (SRTM_VERSION >> 8) & 0xff;
	msg->type = SRTM_TYPE_REQ;
	msg->command = SRTM_CMD_READ;
	msg->priority = 1;

	msg->i2c_bus = i2c_bus->index;
	msg->return_val = 0;
	msg->slave_addr = (u16)chip;
	msg->flag = (u16)flag;
	msg->data_length = len;

	imx_virt_i2c_msg_dump(msg);

	/* Send request and get return data */
	ret = imx_vservice_blocking_request(i2c_bus->vservice, (u8 *)msg, &size);
	if (ret) {
		printf("Vservice request is failed, ret %d\n", ret);
		return ret;
	}

	if (msg->type != SRTM_TYPE_RESP || msg->categary != SRTM_I2C_CATEGORY
		|| msg->command !=SRTM_CMD_READ) {
		printf("Error read response message\n");
		return -EIO;
	}

	if (msg->return_val != 0)
		return msg->return_val;

	if (len != 0)
		memcpy(buf, msg->data_buf, msg->data_length);

	return ret;
}

static int imx_virt_i2c_write(struct udevice *bus, u32 chip, u8 *buf, int len, uint flag)
{
	struct imx_srtm_i2c_msg *msg;
	u32 size;
	int ret = 0;
	struct imx_virt_i2c_bus *i2c_bus = dev_get_priv(bus);

	debug("imx_virt_i2c_write, bus %d\n", i2c_bus->index);

	if (len > MAX_SRTM_I2C_BUF_SIZE) {
		printf("virt_i2c_read exceed the buf length, len=%d\n", len);
		return -EINVAL;
	}

	size = sizeof(struct imx_srtm_i2c_msg);
	msg = imx_vservice_get_buffer(i2c_bus->vservice, size);
	if (msg == NULL)
		return -ENOMEM;

	/* Fill buf with SRTM i2c format */
	msg->categary = SRTM_I2C_CATEGORY;
	msg->version[0] = SRTM_VERSION & 0xff;
	msg->version[1] = (SRTM_VERSION >> 8) & 0xff;
	msg->type = SRTM_TYPE_REQ;
	msg->command = SRTM_CMD_WRITE;
	msg->priority = 1;

	msg->i2c_bus = i2c_bus->index;
	msg->return_val = 0;
	msg->slave_addr = (u16)chip;
	msg->flag = (u16)flag;
	msg->data_length = len;

	imx_virt_i2c_msg_dump(msg);

	if (buf) /* probe chip does not have data buffer */
		memcpy(msg->data_buf, buf, msg->data_length);

	/* Send request and get return data */
	ret = imx_vservice_blocking_request(i2c_bus->vservice,  (u8 *)msg, &size);
	if (ret) {
		printf("Vservice request is failed, ret %d\n", ret);
		return ret;
	}

	if (msg->type != SRTM_TYPE_RESP || msg->categary != SRTM_I2C_CATEGORY
		|| msg->command !=SRTM_CMD_WRITE) {
		printf("Error write response message\n");
		return -EIO;
	}

	if (msg->return_val != 0) {
		debug("Peer process message, ret %d\n", msg->return_val);
		return -EACCES;
	}

	debug("imx_vservice_blocking_request get size = %d\n", size);

	return ret;

}

static int imx_virt_i2c_probe_chip(struct udevice *bus, u32 chip,
				u32 chip_flags)
{
	debug("imx_virt_i2c_probe_chip\n");

	return imx_virt_i2c_write(bus, chip, NULL, 0, I2C_M_SRTM_STOP);
}

static int imx_virt_i2c_xfer(struct udevice *bus, struct i2c_msg *msg, int nmsgs)
{
	int ret = 0;
	uint flag = 0;

	for (; nmsgs > 0; nmsgs--, msg++) {
		debug("virt_i2c_xfer: chip=0x%x, len=0x%x, buf=0x%08x\n", msg->addr, msg->len, *msg->buf);

		flag = msg->flags;
		if (nmsgs == 1)
			flag |= I2C_M_SRTM_STOP;

		if (flag & I2C_M_RD)
			ret = imx_virt_i2c_read(bus, msg->addr, msg->buf, msg->len, flag);
		else {
			ret = imx_virt_i2c_write(bus, msg->addr, msg->buf,
					    msg->len, flag);
			if (ret)
				break;
		}
	}

	if (ret)
		printf("i2c_xfer: error %d\n", ret);

	return ret;
}

static int imx_virt_i2c_probe(struct udevice *bus)
{
	struct imx_virt_i2c_bus *i2c_bus = dev_get_priv(bus);
	fdt_addr_t addr;

	addr = devfdt_get_addr(bus);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	i2c_bus->base = addr;
	i2c_bus->index = bus->seq;

	debug("virt_i2c : controller bus %d at 0x%lx,  bus udev 0x%lx\n",
	      bus->seq, i2c_bus->base, (ulong)bus);

	i2c_bus->vservice = imx_vservice_setup(bus);
	if (i2c_bus->vservice == NULL) {
		printf("virt_i2c: Faild to setup vservice\n");
		return -ENODEV;
	}

	return 0;
}

static int imx_virt_i2c_set_flags(struct udevice *child_dev, uint flags)
{
#ifdef CONFIG_I2C_MUX_IMX_VIRT
	if (child_dev->uclass->uc_drv->id == UCLASS_I2C_MUX) {
		struct udevice *bus = child_dev->parent;
		struct imx_virt_i2c_bus *i2c_bus = dev_get_priv(bus);

		if (flags == 0) {
			i2c_bus->index = bus->seq;
		} else if (flags & I2C_M_SELECT_MUX_BUS) {
			i2c_bus->index = (flags >> 24) & 0xff;
		}

		debug("virt_i2c_set_flags bus %d\n", i2c_bus->index);
	}
#endif
	return 0;
}

int __weak board_imx_virt_i2c_bind(struct udevice *dev)
{
	return 0;
}

static int imx_virt_i2c_bind(struct udevice *dev)
{
	debug("imx_virt_i2c_bind, %s, seq %d\n", dev->name, dev->req_seq);

	return board_imx_virt_i2c_bind(dev);
}

static int imx_virt_i2c_child_post_bind(struct udevice *child_dev)
{
#ifdef CONFIG_I2C_MUX_IMX_VIRT
	if (child_dev->uclass->uc_drv->id == UCLASS_I2C_MUX) {
		if (!strcmp(child_dev->driver->name, "imx_virt_i2c_mux"))
			return 0;
		else
			return -ENODEV;
	}
#endif

	return 0;
}

static const struct dm_i2c_ops imx_virt_i2c_ops = {
	.xfer		= imx_virt_i2c_xfer,
	.probe_chip	= imx_virt_i2c_probe_chip,
	.set_flags = imx_virt_i2c_set_flags,
};

static const struct udevice_id imx_virt_i2c_ids[] = {
	{ .compatible = "fsl,imx-virt-i2c", },
	{}
};

U_BOOT_DRIVER(imx_virt_i2c) = {
	.name = "imx_virt_i2c",
	.id = UCLASS_I2C,
	.of_match = imx_virt_i2c_ids,
	.bind = imx_virt_i2c_bind,
	.probe = imx_virt_i2c_probe,
	.child_post_bind = imx_virt_i2c_child_post_bind,
	.priv_auto_alloc_size = sizeof(struct imx_virt_i2c_bus),
	.ops = &imx_virt_i2c_ops,
	.flags	= DM_FLAG_DEFAULT_PD_CTRL_OFF | DM_FLAG_IGNORE_DEFAULT_CLKS,
};
