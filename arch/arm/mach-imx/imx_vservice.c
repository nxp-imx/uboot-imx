// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2019 NXP
 *
 */

#include <common.h>
#include <asm/io.h>
#include <dm.h>
#include <dm/lists.h>
#include <dm/root.h>
#include <dm/device-internal.h>
#include <dm/uclass-internal.h>
#include <misc.h>
#include <asm/mach-imx/imx_vservice.h>
#include <imx_m4_mu.h>
#include <malloc.h>

static LIST_HEAD(vservice_channels);

void * __weak board_imx_vservice_get_buffer(struct imx_vservice_channel *node, u32 size)
{
	if (size <= CONFIG_IMX_VSERVICE_SHARED_BUFFER_SIZE)
		return (void * )CONFIG_IMX_VSERVICE_SHARED_BUFFER;

	return NULL;
}

void * imx_vservice_get_buffer(struct imx_vservice_channel *node, u32 size)
{
	return board_imx_vservice_get_buffer(node, size);
}

int imx_vservice_blocking_request(struct imx_vservice_channel *node, u8 *buf, u32* size)
{
	int ret = 0;
	union imx_m4_msg msg;

	msg.format.seq = node->msg_seq;
	msg.format.type = MU_MSG_REQ;
	msg.format.buffer = (u32)(ulong)buf;
	msg.format.size = *size;

	ret = misc_call(node->mu_dev, 1000000, &msg, 4, &msg, 4);
	if (ret) {
		printf("%s: Send request MU message failed, ret %d\n", __func__, ret);
		goto MU_ERR;
	}

	if (msg.format.type != MU_MSG_RESP|| msg.format.seq != node->msg_seq) {
		printf("%s: wrong msg response: type %d, seq %d, expect seq %d\n",
			__func__, msg.format.type, msg.format.seq, node->msg_seq);
		ret = -EIO;
		goto MU_ERR;
	}

	*size = msg.format.size;

MU_ERR:
	node->msg_seq++;

	return ret;
}

static int imx_vservice_connect(struct imx_vservice_channel *node)
{
	int ret = 0;
	union imx_m4_msg msg;

	unsigned long timeout = timer_get_us() + 2000000; /* 2s timeout */

	for (;;) {
		msg.format.seq = 0;
		msg.format.type = MU_MSG_READY_A;
		msg.format.buffer = 0;
		msg.format.size = 0;

		ret = misc_call(node->mu_dev, 100000, &msg, 4, &msg, 4);
		if (!ret && msg.format.type == MU_MSG_READY_B)
			return 0;

		if (time_after(timer_get_us(), timeout)) {
			printf("%s: Timeout to connect peer, %d\n", __func__, ret);
			return -ETIMEDOUT;
		}
	}

	return -EIO;
}

struct udevice * __weak board_imx_vservice_find_mu(struct udevice *virt_dev)
{
	int ret;
	struct ofnode_phandle_args args;
	struct udevice *mu_dev;

	/* Default get mu from "fsl,vservice-mu" property*/
	ret = dev_read_phandle_with_args(virt_dev, "fsl,vservice-mu",
					 NULL, 0, 0, &args);
	if (ret) {
		printf("Can't find \"fsl,vservice-mu\" property\n");
		return NULL;
	}

	ret = uclass_find_device_by_ofnode(UCLASS_MISC, args.node, &mu_dev);
	if (ret) {
		printf("Can't find MU device, err %d\n", ret);
		return NULL;
	}

	return mu_dev;
}

static struct udevice * imx_vservice_find_mu(struct udevice *virt_dev)
{
	return board_imx_vservice_find_mu(virt_dev);
}

struct imx_vservice_channel * imx_vservice_setup(struct udevice *virt_dev)
{
	int ret;
	 struct udevice *mu_dev;
	 struct imx_vservice_channel *channel;

	mu_dev = imx_vservice_find_mu(virt_dev);
	if (mu_dev == NULL) {
		printf("No MU device for virtual service %s connection\n", virt_dev->name);
		return NULL;
	}

	ret = device_probe(mu_dev);
	if (ret) {
		printf("Probe MU device failed\n");
		return NULL;
	}

	list_for_each_entry(channel, &vservice_channels, channel_head) {
		if (channel->mu_dev == mu_dev)
			return channel;
	}

	channel = malloc(sizeof(struct imx_vservice_channel));
	if (!channel) {
		printf("Malloc vservice channel is failed\n");
		return NULL;
	}

	channel->msg_seq = 0;
	channel->mu_dev = mu_dev;
	INIT_LIST_HEAD(&channel->channel_head);

	ret = imx_vservice_connect(channel);
	if (ret) {
		printf("VService: Connection is failed, ret %d\n", ret);
		free(channel);
		return NULL;
	}

	list_add_tail(&channel->channel_head, &vservice_channels);

	printf("VService: Connection is ok on MU %s\n", mu_dev->name);

	return channel;
}
