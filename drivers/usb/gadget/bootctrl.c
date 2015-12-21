/*
 * Copyright (C) 2015 Freescale Semiconductor, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <common.h>
#include <g_dnl.h>
#include <fsl_fastboot.h>
#include "bootctrl.h"
#include <linux/types.h>
#include <linux/stat.h>

#define SLOT_NUM (unsigned int)2

/* keep same as bootable/recovery/bootloader.h */
struct bootloader_message {
	char command[32];
	char status[32];
	char recovery[768];

	/* The 'recovery' field used to be 1024 bytes.	It has only ever
	 been used to store the recovery command line, so 768 bytes
	 should be plenty.  We carve off the last 256 bytes to store the
	 stage string (for multistage packages) and possible future
	 expansion. */
	char stage[32];
	char slot_suffix[32];
	char reserved[192];
};

/* start from bootloader_message.slot_suffix[BOOTCTRL_IDX] */
#define BOOTCTRL_IDX				0
#define BOOTCTRL_OFFSET		\
	(u32)(&(((struct bootloader_message *)0)->slot_suffix[BOOTCTRL_IDX]))
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

static unsigned int g_mmc_id;
static unsigned int g_slot_selected;
static const char *g_slot_suffix[SLOT_NUM] = {"_a", "_b"};

static int do_write(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);

static int strcmp_l1(const char *s1, const char *s2)
{
	if (!s1 || !s2)
		return -1;
	return strncmp(s1, s2, strlen(s1));
}

void set_mmc_id(unsigned int id)
{
	g_mmc_id = id;
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

static ulong get_block_size(char *ifname, int dev, int part)
{
	block_dev_desc_t *dev_desc = NULL;
	disk_partition_t part_info;

	dev_desc = get_dev(ifname, dev);
	if (dev_desc == NULL) {
		printf("Block device %s %d not supported\n", ifname, dev);
		return 0;
	}

	if (get_partition_info(dev_desc, part, &part_info)) {
		printf("Cannot find partition %d\n", part);
		return 0;
	}

	return part_info.blksz;
}

#define ALIGN_BYTES 64 /*armv7 cache line need 64 bytes aligned */
static int rw_block(bool bread, char **ppblock,
					uint *pblksize, char *pblock_write)
{
	int ret;
	char *argv[6];
	char addr_str[20];
	char cnt_str[8];
	char devpart_str[8];
	char block_begin_str[8];
	ulong blk_size = 0;
	uint blk_begin = 0;
	uint blk_end = 0;
	uint block_cnt = 0;
	char *p_block = NULL;

	if (bread && ((ppblock == NULL) || (pblksize == NULL)))
		return -1;

	if (!bread && (pblock_write == NULL))
		return -1;

	blk_size = get_block_size("mmc", g_mmc_id,
			CONFIG_ANDROID_MISC_PARTITION_MMC);
	if (blk_size == 0) {
		printf("rw_block, get_block_size return 0\n");
		return -1;
	}

	blk_begin = BOOTCTRL_OFFSET/blk_size;
	blk_end = (BOOTCTRL_OFFSET + sizeof(struct boot_ctl) - 1)/blk_size;
	block_cnt = 1 + (blk_end - blk_begin);

	sprintf(devpart_str, "0x%x:0x%x", g_mmc_id,
		CONFIG_ANDROID_MISC_PARTITION_MMC);
	sprintf(block_begin_str, "0x%x", blk_begin);
	sprintf(cnt_str, "0x%x", block_cnt);

	argv[0] = "rw"; /* not care */
	argv[1] = "mmc";
	argv[2] = devpart_str;
	argv[3] = addr_str;
	argv[4] = block_begin_str;
	argv[5] = cnt_str;

	if (bread) {
		p_block = (char *)memalign(ALIGN_BYTES, blk_size * block_cnt);
		if (NULL == p_block) {
			printf("rw_block, memalign %d bytes failed\n",
			       (int)(blk_size * block_cnt));
			return -1;
		}
		sprintf(addr_str, "0x%x", (unsigned int)p_block);
		ret = do_read(NULL, 0, 6, argv);
		if (ret) {
			free(p_block);
			printf("do_read failed, ret %d\n", ret);
			return -1;
		}

		*ppblock = p_block;
		*pblksize = (uint)blk_size;
	} else {
		sprintf(addr_str, "0x%x", (unsigned int)pblock_write);
		ret = do_write(NULL, 0, 6, argv);
		if (ret) {
			printf("do_write failed, ret %d\n", ret);
			return -1;
		}
	}

	return 0;
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

	ret = rw_block(true, &p_block, &blk_size, NULL);
	if (ret) {
		printf("read_bootctl, rw_block read failed\n");
		return -1;
	}

	offset_in_block = BOOTCTRL_OFFSET%blk_size;
	memcpy(ptbootctl, p_block + offset_in_block, sizeof(struct boot_ctl));

	pmagic = ptbootctl->magic;
	if (!((pmagic[0] == '\0') && (pmagic[1] == 'F') &&
	      (pmagic[2] == 'S') && (pmagic[3] == 'L'))) {
		printf("magic error, %c %c %c %c\n",
		       pmagic[0], pmagic[1], pmagic[2], pmagic[3]);

		free(p_block);
		return -1;
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

	ret = rw_block(true, &p_block, &blk_size, NULL);
	if (ret) {
		printf("write_bootctl, rw_block read failed\n");
		return -1;
	}

	offset_in_block = BOOTCTRL_OFFSET%blk_size;
	memcpy(p_block + offset_in_block, ptbootctl, sizeof(struct boot_ctl));

	ret = rw_block(false, NULL, NULL, p_block);
	if (ret) {
		free(p_block);
		printf("write_bootctl, rw_block write failed\n");
		return -1;
	}

	free(p_block);
	return 0;
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

static unsigned int slotidx_from_suffix(char *suffix)
{
	unsigned int slot = -1;

	if (!strcmp(suffix, "_a"))
		slot = 0;
	else if (!strcmp(suffix, "_b"))
		slot = 1;

	return slot;
}

bool is_sotvar(char *cmd)
{
	if (!strcmp_l1("has-slot:", cmd) ||
		!strcmp_l1("slot-successful:", cmd) ||
		!strcmp_l1("slot-suffixes", cmd) ||
		!strcmp_l1("current-slot", cmd) ||
		!strcmp_l1("slot-unbootable:", cmd) ||
		!strcmp_l1("slot-retry-count:", cmd)) {
		return true;
	}

	return false;
}

void get_slotvar(char *cmd, char *response, size_t chars_left)
{
	int ret;
	struct boot_ctl t_bootctl;
	memset(&t_bootctl, 0, sizeof(t_bootctl));

	ret = read_bootctl(&t_bootctl);
	if (ret) {
		error("get_slotvar, read_bootctl failed\n");
		strcpy(response, "get_slotvar read_bootctl failed");
		return;
	}

	if (!strcmp_l1("has-slot:", cmd)) {
		char *ptnname = NULL;
		ptnname = strchr(cmd, ':') + 1;
		if (!strcmp(ptnname, "system") || !strcmp(ptnname, "boot"))
			strncat(response, "yes", chars_left);
		else
			strncat(response, "no", chars_left);
	} else if (!strcmp_l1("current-slot", cmd)) {
		unsigned int slot = slot_find(&t_bootctl);
		if (slot < SLOT_NUM)
			strncat(response, g_slot_suffix[slot], chars_left);
		else
			strncat(response, "no valid slot", chars_left);
	} else if (!strcmp_l1("slot-suffixes", cmd)) {
		strncat(response, "_a,_b", chars_left);
	} else if (!strcmp_l1("slot-successful:", cmd)) {
		char *suffix = strchr(cmd, ':') + 1;
		unsigned int slot = slotidx_from_suffix(suffix);
		if (slot >= SLOT_NUM) {
			strncat(response, "no such slot", chars_left);
		} else {
			bool suc = t_bootctl.a_slot_meta[slot].bootsuc;
			strncat(response, suc ? "yes" : "no", chars_left);
		}
	} else if (!strcmp_l1("slot-unbootable:", cmd)) {
		char *suffix = strchr(cmd, ':') + 1;
		unsigned int slot = slotidx_from_suffix(suffix);
		if (slot >= SLOT_NUM) {
			strncat(response, "no such slot", chars_left);
		} else {
			unsigned int pri = t_bootctl.a_slot_meta[slot].priority;
			strncat(response, pri ? "no" : "yes", chars_left);
		}
	} else if (!strcmp_l1("slot-retry-count:", cmd)) {
		char *suffix = strchr(cmd, ':') + 1;
		unsigned int slot = slotidx_from_suffix(suffix);
		if (slot >= SLOT_NUM)
			strncat(response, "no such slot", chars_left);
		else
			sprintf(response, "OKAY%d",
				t_bootctl.a_slot_meta[slot].tryremain);
	} else {
		strncat(response, "no such slot command", chars_left);
	}

	return;
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

static int do_write(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *ep;
	block_dev_desc_t *dev_desc = NULL;
	int dev;
	int part = 0;
	disk_partition_t part_info;
	ulong offset = 0u;
	ulong limit = 0u;
	void *addr;
	uint blk;
	uint cnt;

	if (argc != 6) {
		cmd_usage(cmdtp);
		return 1;
	}

	dev = (int)simple_strtoul(argv[2], &ep, 16);
	if (*ep) {
		if (*ep != ':') {
			printf("Invalid block device %s\n", argv[2]);
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	dev_desc = get_dev(argv[1], dev);
	if (dev_desc == NULL) {
		printf("Block device %s %d not supported\n", argv[1], dev);
		return 1;
	}

	addr = (void *)simple_strtoul(argv[3], NULL, 16);
	blk = simple_strtoul(argv[4], NULL, 16);
	cnt = simple_strtoul(argv[5], NULL, 16);

	if (part != 0) {
		if (get_partition_info(dev_desc, part, &part_info)) {
			printf("Cannot find partition %d\n", part);
			return 1;
		}
		offset = part_info.start;
		limit = part_info.size;
	} else {
		/* Largest address not available in block_dev_desc_t. */
		limit = ~0;
	}

	if (cnt + blk > limit) {
		printf("Write out of range\n");
		return 1;
	}

	if (dev_desc->block_write(dev, offset + blk, cnt, addr) < 0) {
		printf("Error writing blocks\n");
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
	write,	6,	0,	do_write,
	"write binary data to a partition",
	"<interface> <dev[:part]> addr blk# cnt"
);
