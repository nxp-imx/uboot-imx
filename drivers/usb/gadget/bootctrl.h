/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef BOOTCTRL_H
#define BOOTCTRL_H

#include <common.h>
#include <g_dnl.h>
#include <linux/types.h>
#include <linux/stat.h>
#include "bcb.h"

#define SLOT_NUM (unsigned int)2
#define CRC_DATA_OFFSET		\
	(uint32_t)(&(((struct boot_ctl *)0)->a_slot_meta[0]))

struct slot_meta {
	u8 bootsuc:1;
	u8 tryremain:3;
	u8 priority:4;
};

struct boot_ctl {
	char magic[4]; /* "\0FSL" */
	u32 crc;
	struct slot_meta a_slot_meta[SLOT_NUM];
	u8 recovery_tryremain;
};
char *select_slot(void);
int invalid_curslot(void);
int get_slotvar(char *cmd, char *response, size_t chars_left);
void cb_set_active(struct usb_ep *ep, struct usb_request *req);
const char *get_slot_suffix(void);
#endif
