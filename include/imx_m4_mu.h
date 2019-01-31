// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2019 NXP
 *
 */

#ifndef __IMX_M4_MU_H__
#define __IMX_M4_MU_H__

enum imx_m4_msg_type {
	MU_MSG_REQ		= 0x1, /* request message sent from A side */
	MU_MSG_RESP		= 0x2, /* response message from B side for request */
	MU_MSG_READY_A	= 0x3, /* A side notifies ready */
	MU_MSG_READY_B	= 0x4, /* B side notifies ready */
};

union imx_m4_msg {
	struct {
		u32 seq;
		u32 type;
		u32 buffer;
		u32 size;
	} format;
	u32 data[4];
};
#endif
