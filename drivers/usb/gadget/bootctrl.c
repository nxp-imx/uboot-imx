/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <g_dnl.h>
#include <fsl_fastboot.h>
#include "bootctrl.h"
#include <linux/types.h>
#include <linux/stat.h>
static unsigned int g_slot_selected;
static const char *g_slot_suffix[SLOT_NUM] = {"_a", "_b"};
static int init_slotmeta(struct boot_ctl *ptbootctl);

static int strcmp_l1(const char *s1, const char *s2)
{
	if (!s1 || !s2)
		return -1;
	return strncmp(s1, s2, strlen(s1));
}


static void dump_slotmeta(struct boot_ctl *ptbootctl)
{
	int i;

	if (ptbootctl == NULL)
		return;

	printf("RecoveryTryRemain %d, crc %u\n",
		   ptbootctl->recovery_tryremain, ptbootctl->crc);

	for (i = 0; i < SLOT_NUM; i++) {
		printf("slot %d: pri %d, try %d, suc %d\n", i,
			   ptbootctl->a_slot_meta[i].priority,
				ptbootctl->a_slot_meta[i].tryremain,
				ptbootctl->a_slot_meta[i].bootsuc);
	}

	return;
}

const char *get_slot_suffix(void)
{
	return g_slot_suffix[g_slot_selected];
}

static unsigned int slot_find(struct boot_ctl *ptbootctl)
{
	unsigned int max_pri = 0;
	unsigned int slot = -1;
	int i;

	for (i	= 0; i < SLOT_NUM; i++) {
		struct slot_meta *pslot_meta = &(ptbootctl->a_slot_meta[i]);
		if ((pslot_meta->priority > max_pri) &&
			((pslot_meta->bootsuc > 0) ||
			(pslot_meta->tryremain > 0))) {
			max_pri = pslot_meta->priority;
			slot = i;
			printf("select_slot slot %d\n", slot);
		}
	}

	return slot;
}



static int read_bootctl(struct boot_ctl *ptbootctl)
{
	int ret = 0;
	unsigned int crc = 0;
	char *p_block = NULL;
	uint offset_in_block = 0;
	uint blk_size = 0;
	char *pmagic = NULL;

	if (ptbootctl == NULL)
		return -1;
	ret = bcb_rw_block(true, &p_block, &blk_size, NULL,
			BOOTCTRL_OFFSET, sizeof(struct boot_ctl));
	if (ret) {
		printf("read_bootctl, bcb_rw_block read failed\n");
		return -1;
	}

	offset_in_block = BOOTCTRL_OFFSET%blk_size;
	memcpy(ptbootctl, p_block + offset_in_block, sizeof(struct boot_ctl));

	pmagic = ptbootctl->magic;
	if (!((pmagic[0] == '\0') && (pmagic[1] == 'F') &&
	      (pmagic[2] == 'S') && (pmagic[3] == 'L'))) {
		printf("magic error, 0x%x 0x%x 0x%x 0x%x\n",
		       pmagic[0], pmagic[1], pmagic[2], pmagic[3]);

		ret = init_slotmeta(ptbootctl);
		if (ret) {
			printf("init_slotmeta failed, ret %d\n", ret);
			free(p_block);
			return -1;
		}
	}

	/* check crc */
	crc = crc32(0, (unsigned char *)ptbootctl + CRC_DATA_OFFSET,
		sizeof(struct boot_ctl) - CRC_DATA_OFFSET);
	if (crc != ptbootctl->crc) {
		printf("crc check failed, caculated %d, read %d\n",
		       crc, ptbootctl->crc);

		free(p_block);
		return -1;
	}

	free(p_block);
	return 0;
}

static int write_bootctl(struct boot_ctl *ptbootctl)
{
	int ret = 0;
	char *p_block = NULL;
	uint offset_in_block = 0;
	uint blk_size = 0;

	if (ptbootctl == NULL)
		return -1;

	ptbootctl->crc = crc32(0, (unsigned char *)ptbootctl + CRC_DATA_OFFSET,
		sizeof(struct boot_ctl) - CRC_DATA_OFFSET);

	ret = bcb_rw_block(true, &p_block, &blk_size, NULL,
				BOOTCTRL_OFFSET, sizeof(struct boot_ctl));
	if (ret) {
		printf("write_bootctl, bcb_rw_block read failed\n");
		return -1;
	}
	offset_in_block = BOOTCTRL_OFFSET%blk_size;
	memcpy(p_block + offset_in_block, ptbootctl, sizeof(struct boot_ctl));

	ret = bcb_rw_block(false, NULL, NULL, p_block,
			BOOTCTRL_OFFSET, sizeof(struct boot_ctl));
	if (ret) {
		free(p_block);
		printf("write_bootctl, bcb_rw_block write failed\n");
		return -1;
	}
	free(p_block);
	return 0;
}

static int init_slotmeta(struct boot_ctl *ptbootctl)
{
	int ret = 0;
	if (ptbootctl == NULL)
		return -1;
	memset(ptbootctl, 0, sizeof(struct boot_ctl));
	ptbootctl->recovery_tryremain = 7;
	ptbootctl->a_slot_meta[0].priority = 8;
	ptbootctl->a_slot_meta[0].tryremain = 7;
	ptbootctl->a_slot_meta[0].bootsuc = 0;
	ptbootctl->a_slot_meta[1].priority = 6;
	ptbootctl->a_slot_meta[1].tryremain = 7;
	ptbootctl->a_slot_meta[1].bootsuc = 0;

	ptbootctl->magic[0] = '\0';
	ptbootctl->magic[1] = 'F';
	ptbootctl->magic[2] = 'S';
	ptbootctl->magic[3] = 'L';

	ptbootctl->crc = crc32(0, (uint8_t *)ptbootctl + CRC_DATA_OFFSET,
			sizeof(struct boot_ctl) - CRC_DATA_OFFSET);
	ret = write_bootctl(ptbootctl);
	return ret;
}

char *select_slot(void)
{
	int i = 0;
	int ret = 0;
	unsigned int slot;
	struct boot_ctl t_bootctl;
	bool b_need_write = false;

	ret = read_bootctl(&t_bootctl);
	if (ret) {
		printf("read_bootctl failed, ret %d\n", ret);
		return NULL;
	}

	dump_slotmeta(&t_bootctl);

	slot = slot_find(&t_bootctl);
	if (slot >= SLOT_NUM) {
		printf("!!! select_slot, no valid slot\n");
		return NULL;
	}

	/* invalid slot, set priority to 0 */
	for (i	= 0; i < SLOT_NUM; i++) {
		struct slot_meta *pslot_meta = &(t_bootctl.a_slot_meta[i]);
		if ((pslot_meta->bootsuc == 0) &&
			(pslot_meta->tryremain == 0) &&
			(pslot_meta->priority > 0)) {
			pslot_meta->priority = 0;
			b_need_write = true;
		}
	}

	if (t_bootctl.recovery_tryremain != 7) {
		b_need_write = true;
		t_bootctl.recovery_tryremain = 7;
	}

	if ((t_bootctl.a_slot_meta[slot].bootsuc == 0) &&
		(t_bootctl.a_slot_meta[slot].tryremain > 0)) {
		b_need_write = true;
		t_bootctl.a_slot_meta[slot].tryremain--;
	}

	if (b_need_write) {
		ret = write_bootctl(&t_bootctl);
		if (ret)
			printf("!!! write_bootctl failed, ret %d\n", ret);
	}

	g_slot_selected = slot;

	if (slot == 0)
		return FASTBOOT_PARTITION_BOOT_A;
	else
		return FASTBOOT_PARTITION_BOOT_B;
}


int invalid_curslot(void)
{
	int ret = 0;
	struct boot_ctl t_bootctl;
	unsigned int slot = g_slot_selected;

	printf("invalid_curslot %d\n", slot);

	if (slot >= SLOT_NUM)
		return -1;

	ret = read_bootctl(&t_bootctl);
	if (ret) {
		printf("invalid_slot failed, ret %d\n", ret);
		return -1;
	}

	t_bootctl.a_slot_meta[slot].priority = 0;
	ret = write_bootctl(&t_bootctl);
	if (ret) {
		printf("!!! write_bootctl failed, ret %d\n", ret);
		return -1;
	}

	return 0;
}


static unsigned int slotidx_from_suffix(char *suffix)
{
	unsigned int slot = -1;

	if (!strcmp(suffix, "_a") ||
			!strcmp(suffix, "a"))
		slot = 0;
	else if (!strcmp(suffix, "_b") ||
			!strcmp(suffix, "b"))
		slot = 1;

	return slot;
}


int get_slotvar(char *cmd, char *response, size_t chars_left)
{
	int ret;
	struct boot_ctl t_bootctl;
	memset(&t_bootctl, 0, sizeof(t_bootctl));

	if (!strcmp_l1("has-slot:", cmd)) {
		char *ptnname = NULL;
		ptnname = strchr(cmd, ':') + 1;
		if (!strcmp(ptnname, "system") || !strcmp(ptnname, "boot"))
			strlcpy(response, "yes", chars_left);
		else
			strlcpy(response, "no", chars_left);
		return 0;
	} else if (!strcmp_l1("slot-suffixes", cmd)) {
		strlcpy(response, "_a,_b", chars_left);
		return 0;
	}

	ret = read_bootctl(&t_bootctl);
	if (ret) {
		error("get_slotvar, read_bootctl failed\n");
		strcpy(response, "get_slotvar read_bootctl failed");
		return -1;
	}
	if (!strcmp_l1("current-slot", cmd)) {
		unsigned int slot = slot_find(&t_bootctl);
		if (slot < SLOT_NUM)
			strlcpy(response, g_slot_suffix[slot], chars_left);
		else {
			strlcpy(response, "no valid slot", chars_left);
			return -1;
		}
	} else if (!strcmp_l1("slot-successful:", cmd)) {
		char *suffix = strchr(cmd, ':') + 1;
		unsigned int slot = slotidx_from_suffix(suffix);
		if (slot >= SLOT_NUM) {
			strlcpy(response, "no such slot", chars_left);
			return -1;
		} else {
			bool suc = t_bootctl.a_slot_meta[slot].bootsuc;
			strlcpy(response, suc ? "yes" : "no", chars_left);
		}
	} else if (!strcmp_l1("slot-unbootable:", cmd)) {
		char *suffix = strchr(cmd, ':') + 1;
		unsigned int slot = slotidx_from_suffix(suffix);
		if (slot >= SLOT_NUM) {
			strlcpy(response, "no such slot", chars_left);
			return -1;
		} else {
			unsigned int pri = t_bootctl.a_slot_meta[slot].priority;
			strlcpy(response, pri ? "no" : "yes", chars_left);
		}
	} else if (!strcmp_l1("slot-retry-count:", cmd)) {
		char *suffix = strchr(cmd, ':') + 1;
		unsigned int slot = slotidx_from_suffix(suffix);
		if (slot >= SLOT_NUM) {
			strlcpy(response, "no such slot", chars_left);
			return -1;
		} else {
			char str_num[7];
			sprintf(str_num, "%d",
				t_bootctl.a_slot_meta[slot].tryremain);
			strlcpy(response, str_num, chars_left);
		}
	} else {
		strlcpy(response, "no such slot command", chars_left);
		return -1;
	}

	return 0;
}


void cb_set_active(struct usb_ep *ep, struct usb_request *req)
{
	int ret;
	int i;
	unsigned int slot = 0;
	char *cmd = req->buf;
	struct boot_ctl t_bootctl;

	memset(&t_bootctl, 0, sizeof(t_bootctl));

	strsep(&cmd, ":");
	if (!cmd) {
		error("missing slot suffix\n");
		fastboot_tx_write_str("FAILmissing slot suffix");
		return;
	}

	slot = slotidx_from_suffix(cmd);
	if (slot >= SLOT_NUM) {
		fastboot_tx_write_str("FAILerr slot suffix");
		return;
	}

	ret = read_bootctl(&t_bootctl);
	if (ret)
		fastboot_tx_write_str("FAILReadBootCtl failed");

	t_bootctl.a_slot_meta[slot].bootsuc = 0;
	t_bootctl.a_slot_meta[slot].priority = 15;
	t_bootctl.a_slot_meta[slot].tryremain = 7;

	/* low other slot priority */
	for (i = 0; i < SLOT_NUM; i++) {
		if (i == slot)
			continue;

		if (t_bootctl.a_slot_meta[i].priority >= 15)
			t_bootctl.a_slot_meta[i].priority = 14;
	}

	ret = write_bootctl(&t_bootctl);
	if (ret)
		fastboot_tx_write_str("write_bootctl failed");
	else
		fastboot_tx_write_str("OKAY");

	return;
}

