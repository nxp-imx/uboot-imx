// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019-2020 NXP
 */

#include <asm/mach-imx/sys_proto.h>
#include <fb_fsl.h>
#include <fastboot.h>
#include <mmc.h>
#include <android_image.h>
#include <asm/bootm.h>
#include <nand.h>
#include <part.h>
#include <sparse_format.h>
#include <image-sparse.h>
#include <image.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/arch/sys_proto.h>
#include <asm/setup.h>
#include <env.h>
#ifdef CONFIG_DM_SCSI
#include <scsi.h>
#endif

#if defined(CONFIG_FASTBOOT_LOCK)
#include "fastboot_lock_unlock.h"
#endif

#ifdef CONFIG_IMX_TRUSTY_OS
#include "u-boot/sha256.h"
#include <trusty/libtipc.h>
#endif


#ifndef TRUSTY_OS_MMC_BLKS
#define TRUSTY_OS_MMC_BLKS 0x7FF
#endif

#define MEK_8QM_EMMC 0

enum {
	PTN_GPT_INDEX = 0,
	PTN_TEE_INDEX,
#ifdef CONFIG_FLASH_MCUFIRMWARE_SUPPORT
	PTN_MCU_OS_INDEX,
#endif
	PTN_ALL_INDEX,
	PTN_BOOTLOADER_INDEX,
};

struct fastboot_ptentry g_ptable[MAX_PTN];
unsigned int g_pcount;

static ulong bootloader_mmc_offset(void)
{
	if (is_imx8mq() || is_imx8mm() || ((is_imx8qm() || is_imx8qxp()) && is_soc_rev(CHIP_REV_A)))
		return 0x8400;
	else if (is_imx8qm() || (is_imx8qxp() && !is_soc_rev(CHIP_REV_B))) {
		if (MEK_8QM_EMMC == fastboot_devinfo.dev_id)
		/* target device is eMMC boot0 partition, bootloader offset is 0x0 */
			return 0x0;
		else
		/* target device is SD card, bootloader offset is 0x8000 */
			return 0x8000;
	} else if (is_imx8mn() || is_imx8mp() || is_imx8dxl()) {
		/* target device is eMMC boot0 partition, bootloader offset is 0x0 */
		if (env_get_ulong("emmc_dev", 10, 2) == fastboot_devinfo.dev_id)
			return 0;
		else
			return 0x8000;
	}
	else if (is_imx8())
		return 0x8000;
	else
		return 0x400;
}

bool bootloader_gpt_overlay(void)
{
	return (g_ptable[PTN_GPT_INDEX].partition_id  == g_ptable[PTN_BOOTLOADER_INDEX].partition_id  &&
		bootloader_mmc_offset() < ANDROID_GPT_END);
}

/**
   @mmc_dos_partition_index: the partition index in mbr.
   @mmc_partition_index: the boot partition or user partition index,
   not related to the partition table.
 */
static int _fastboot_parts_add_ptable_entry(int ptable_index,
				      int mmc_dos_partition_index,
				      int mmc_partition_index,
				      const char *name,
				      const char *fstype,
				      struct blk_desc *dev_desc,
				      struct fastboot_ptentry *ptable)
{
	disk_partition_t info;

	if (part_get_info(dev_desc,
			       mmc_dos_partition_index, &info)) {
		debug("Bad partition index:%d for partition:%s\n",
		       mmc_dos_partition_index, name);
		return -1;
	}
	ptable[ptable_index].start = info.start;
	ptable[ptable_index].length = info.size;
	ptable[ptable_index].partition_id = mmc_partition_index;
	ptable[ptable_index].partition_index = mmc_dos_partition_index;
	strncpy(ptable[ptable_index].name, (const char *)info.name,
			sizeof(ptable[ptable_index].name) - 1);

#ifdef CONFIG_PARTITION_UUIDS
	strcpy(ptable[ptable_index].uuid, (const char *)info.uuid);
#endif
#ifdef CONFIG_ANDROID_AB_SUPPORT
	if (!strcmp((const char *)info.name, FASTBOOT_PARTITION_SYSTEM_A) ||
	    !strcmp((const char *)info.name, FASTBOOT_PARTITION_SYSTEM_B) ||
	    !strcmp((const char *)info.name, FASTBOOT_PARTITION_OEM_A) ||
	    !strcmp((const char *)info.name, FASTBOOT_PARTITION_VENDOR_A) ||
	    !strcmp((const char *)info.name, FASTBOOT_PARTITION_OEM_B) ||
	    !strcmp((const char *)info.name, FASTBOOT_PARTITION_VENDOR_B) ||
	    !strcmp((const char *)info.name, FASTBOOT_PARTITION_DATA) ||
#else
	if (!strcmp((const char *)info.name, FASTBOOT_PARTITION_SYSTEM) ||
	    !strcmp((const char *)info.name, FASTBOOT_PARTITION_DATA) ||
	    !strcmp((const char *)info.name, FASTBOOT_PARTITION_DEVICE) ||
	    !strcmp((const char *)info.name, FASTBOOT_PARTITION_CACHE) ||
#endif
	    !strcmp((const char *)info.name, FASTBOOT_PARTITION_METADATA))
		strcpy(ptable[ptable_index].fstype, "ext4");
	else
		strcpy(ptable[ptable_index].fstype, "raw");
	return 0;
}

static int _fastboot_parts_load_from_ptable(void)
{
	int i;

	/* mmc boot partition: -1 means no partition, 0 user part., 1 boot part.
	 * default is no partition, for emmc default user part, except emmc*/
	int boot_partition = FASTBOOT_MMC_NONE_PARTITION_ID;
	int user_partition = FASTBOOT_MMC_NONE_PARTITION_ID;

	struct mmc *mmc;
	struct blk_desc *dev_desc;
	struct fastboot_ptentry ptable[MAX_PTN];

	/* sata case in env */
	if (fastboot_devinfo.type == DEV_SATA) {
#ifdef CONFIG_DM_SCSI
		int sata_device_no = fastboot_devinfo.dev_id;
		puts("flash target is SATA\n");
		scsi_scan(false);
		dev_desc = blk_get_dev("scsi", sata_device_no);
#else /*! CONFIG_SATA*/
		puts("SATA isn't buildin\n");
		return -1;
#endif /*! CONFIG_SATA*/
	} else if (fastboot_devinfo.type == DEV_MMC) {
		int mmc_no = fastboot_devinfo.dev_id;

		printf("flash target is MMC:%d\n", mmc_no);
		mmc = find_mmc_device(mmc_no);

		if (mmc == NULL) {
			printf("invalid mmc device %d\n", mmc_no);
			return -1;
		}

		/* Force to init mmc */
		mmc->has_init = 0;
		if (mmc_init(mmc))
			printf("MMC card init failed!\n");

		dev_desc = blk_get_dev("mmc", mmc_no);
		if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
			printf("** Block device MMC %d not supported\n",
				mmc_no);
			return -1;
		}

		/* multiple boot paritions for eMMC 4.3 later */
		if (mmc->part_config != MMCPART_NOAVAILABLE) {
			boot_partition = FASTBOOT_MMC_BOOT_PARTITION_ID;
			user_partition = FASTBOOT_MMC_USER_PARTITION_ID;
		}
	} else {
		printf("Can't setup partition table on this device %d\n",
			fastboot_devinfo.type);
		return -1;
	}

	memset((char *)ptable, 0,
		    sizeof(struct fastboot_ptentry) * (MAX_PTN));
	/* GPT */
	strcpy(ptable[PTN_GPT_INDEX].name, FASTBOOT_PARTITION_GPT);
	ptable[PTN_GPT_INDEX].start = ANDROID_GPT_OFFSET / dev_desc->blksz;
	ptable[PTN_GPT_INDEX].length = ANDROID_GPT_SIZE  / dev_desc->blksz;
	ptable[PTN_GPT_INDEX].partition_id = user_partition;
	ptable[PTN_GPT_INDEX].flags = FASTBOOT_PTENTRY_FLAGS_UNERASEABLE;
	strcpy(ptable[PTN_GPT_INDEX].fstype, "raw");

#ifndef CONFIG_ARM64
	/* Trusty OS */
	strcpy(ptable[PTN_TEE_INDEX].name, FASTBOOT_PARTITION_TEE);
	ptable[PTN_TEE_INDEX].start = 0;
	ptable[PTN_TEE_INDEX].length = TRUSTY_OS_MMC_BLKS;
	ptable[PTN_TEE_INDEX].partition_id = TEE_HWPARTITION_ID;
	strcpy(ptable[PTN_TEE_INDEX].fstype, "raw");
#endif

	/* Add mcu_os partition if we support mcu firmware image flash */
#ifdef CONFIG_FLASH_MCUFIRMWARE_SUPPORT
	strcpy(ptable[PTN_MCU_OS_INDEX].name, FASTBOOT_MCU_FIRMWARE_PARTITION);
	ptable[PTN_MCU_OS_INDEX].start = ANDROID_MCU_FIRMWARE_START / dev_desc->blksz;
	ptable[PTN_MCU_OS_INDEX].length = ANDROID_MCU_FIRMWARE_SIZE / dev_desc->blksz;
	ptable[PTN_MCU_OS_INDEX].flags = FASTBOOT_PTENTRY_FLAGS_UNERASEABLE;
	ptable[PTN_MCU_OS_INDEX].partition_id = user_partition;
	strcpy(ptable[PTN_MCU_OS_INDEX].fstype, "raw");
#endif

	strcpy(ptable[PTN_ALL_INDEX].name, FASTBOOT_PARTITION_ALL);
	ptable[PTN_ALL_INDEX].start = 0;
	ptable[PTN_ALL_INDEX].length = dev_desc->lba;
	ptable[PTN_ALL_INDEX].partition_id = user_partition;
	strcpy(ptable[PTN_ALL_INDEX].fstype, "device");

	/* Bootloader */
	strcpy(ptable[PTN_BOOTLOADER_INDEX].name, FASTBOOT_PARTITION_BOOTLOADER);
	ptable[PTN_BOOTLOADER_INDEX].start =
				bootloader_mmc_offset() / dev_desc->blksz;
	ptable[PTN_BOOTLOADER_INDEX].length =
				 ANDROID_BOOTLOADER_SIZE / dev_desc->blksz;
	ptable[PTN_BOOTLOADER_INDEX].partition_id = boot_partition;
	ptable[PTN_BOOTLOADER_INDEX].flags = FASTBOOT_PTENTRY_FLAGS_UNERASEABLE;
	strcpy(ptable[PTN_BOOTLOADER_INDEX].fstype, "raw");

	int tbl_idx;
	int part_idx = 1;
	int ret;
	for (tbl_idx = PTN_BOOTLOADER_INDEX + 1; tbl_idx < MAX_PTN; tbl_idx++) {
		ret = _fastboot_parts_add_ptable_entry(tbl_idx,
				part_idx++,
				user_partition,
				NULL,
				NULL,
				dev_desc, ptable);
		if (ret)
			break;
	}
	for (i = 0; i < tbl_idx; i++)
		fastboot_flash_add_ptn(&ptable[i]);

	return 0;
}

void fastboot_load_partitions(void)
{
	g_pcount = 0;
	_fastboot_parts_load_from_ptable();
}

/*
 * Android style flash utilties */
void fastboot_flash_add_ptn(struct fastboot_ptentry *ptn)
{
	if (g_pcount < MAX_PTN) {
		memcpy(g_ptable + g_pcount, ptn, sizeof(struct fastboot_ptentry));
		g_pcount++;
	}
}

void fastboot_flash_dump_ptn(void)
{
	unsigned int n;
	for (n = 0; n < g_pcount; n++) {
		struct fastboot_ptentry *ptn = g_ptable + n;
		printf("idx %d, ptn %d name='%s' start=%d len=%ld\n",
			n, ptn->partition_index, ptn->name, ptn->start, ptn->length);
	}
}


struct fastboot_ptentry *fastboot_flash_find_ptn(const char *name)
{
	unsigned int n;

	for (n = 0; n < g_pcount; n++) {
		/* Make sure a substring is not accepted */
		if (strlen(name) == strlen(g_ptable[n].name)) {
			if (0 == strcmp(g_ptable[n].name, name))
				return g_ptable + n;
		}
	}

	return 0;
}

int fastboot_flash_find_index(const char *name)
{
	struct fastboot_ptentry *ptentry = fastboot_flash_find_ptn(name);
	if (ptentry == NULL) {
		printf("cannot get the partion info for %s\n",name);
		fastboot_flash_dump_ptn();
		return -1;
	}
	return ptentry->partition_index;
}

struct fastboot_ptentry *fastboot_flash_get_ptn(unsigned int n)
{
	if (n < g_pcount)
		return g_ptable + n;
	else
		return 0;
}

unsigned int fastboot_flash_get_ptn_count(void)
{
	return g_pcount;
}

bool fastboot_parts_is_raw(struct fastboot_ptentry *ptn)
{
	if (ptn) {
		if (!strncmp(ptn->name, FASTBOOT_PARTITION_BOOTLOADER,
			strlen(FASTBOOT_PARTITION_BOOTLOADER)))
			return true;
#ifdef CONFIG_ANDROID_AB_SUPPORT
		else if (!strncmp(ptn->name, FASTBOOT_PARTITION_GPT,
			strlen(FASTBOOT_PARTITION_GPT)) ||
			!strncmp(ptn->name, FASTBOOT_PARTITION_BOOT_A,
			strlen(FASTBOOT_PARTITION_BOOT_A)) ||
			!strncmp(ptn->name, FASTBOOT_PARTITION_BOOT_B,
			strlen(FASTBOOT_PARTITION_BOOT_B)))
			return true;
#else
		else if (!strncmp(ptn->name, FASTBOOT_PARTITION_BOOT,
			strlen(FASTBOOT_PARTITION_BOOT)))
			return true;
#endif
#if defined(CONFIG_FASTBOOT_LOCK)
		else if (!strncmp(ptn->name, FASTBOOT_PARTITION_FBMISC,
			strlen(FASTBOOT_PARTITION_FBMISC)))
			return true;
#endif
		else if (!strncmp(ptn->name, FASTBOOT_PARTITION_MISC,
			strlen(FASTBOOT_PARTITION_MISC)))
			return true;
	}

	 return false;
}

static bool is_exist(char (*partition_base_name)[20], char *buffer, int count)
{
	int n;

	for (n = 0; n < count; n++) {
		if (!strcmp(partition_base_name[n],buffer))
			return true;
	}
	return false;
}

/*get partition base name from gpt without "_a/_b"*/
int fastboot_parts_get_name(char (*partition_base_name)[20])
{
	int n = 0;
	int count = 0;
	char *ptr1, *ptr2;
	char buffer[20];

	for (n = 0; n < g_pcount; n++) {
		strcpy(buffer,g_ptable[n].name);
		ptr1 = strstr(buffer, "_a");
		ptr2 = strstr(buffer, "_b");
		if (ptr1 != NULL) {
			*ptr1 = '\0';
			if (!is_exist(partition_base_name,buffer,count)) {
				strcpy(partition_base_name[count++],buffer);
			}
		} else if (ptr2 != NULL) {
			*ptr2 = '\0';
			if (!is_exist(partition_base_name,buffer,count)) {
				strcpy(partition_base_name[count++],buffer);
			}
		} else {
			strcpy(partition_base_name[count++],buffer);
		}
	}
	return count;
}

bool fastboot_parts_is_slot(void)
{
	char slot_suffix[2][5] = {"_a","_b"};
	int n;

	for (n = 0; n < g_pcount; n++) {
		if (strstr(g_ptable[n].name, slot_suffix[0]) ||
		strstr(g_ptable[n].name, slot_suffix[1]))
			return true;
	}
	return false;
}

