/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef BOOTCTRL_H
#define BOOTCTRL_H

void set_mmc_id(unsigned int id);
char *select_slot(void);
bool is_sotvar(char *cmd);
void get_slotvar(char *cmd, char *response, size_t chars_left);
void cb_set_active(struct usb_ep *ep, struct usb_request *req);
const char *get_slot_suffix(void);

#endif
