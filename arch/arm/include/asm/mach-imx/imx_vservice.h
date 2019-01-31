// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2019 NXP
 *
 */

#ifndef __IMX_VSERVICE_H__
#define __IMX_VSERVICE_H__

#include <common.h>
#include <dm.h>
#include <linux/list.h>

struct imx_vservice_channel
{
	u32 msg_seq;
	struct udevice *mu_dev;
	struct list_head channel_head;
};

void * imx_vservice_get_buffer(struct imx_vservice_channel *node, u32 size);
int imx_vservice_blocking_request(struct imx_vservice_channel *node, u8 *buf, u32* size);
struct imx_vservice_channel * imx_vservice_setup(struct udevice *virt_dev);

#endif
