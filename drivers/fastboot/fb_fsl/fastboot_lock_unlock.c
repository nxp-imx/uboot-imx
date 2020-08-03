// SPDX-License-Identifier:     GPL-2.0+
/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 */
#include <common.h>
#include <mapmem.h>
#include <linux/types.h>
#include <part.h>
#include <mmc.h>
#include <ext_common.h>
#include <stdio_dev.h>
#include <stdlib.h>
#include "fastboot_lock_unlock.h"
#include <fb_fsl.h>
#include <memalign.h>
#include <asm/mach-imx/sys_proto.h>
#ifdef CONFIG_IMX_TRUSTY_OS
#include <trusty/libtipc.h>
#include <asm/mach-imx/hab.h>
#endif

#include <fsl_avb.h>

#ifdef FASTBOOT_ENCRYPT_LOCK

#include <hash.h>
#include <fsl_caam.h>

//Encrypted data is 80bytes length.
#define ENDATA_LEN 80

#endif

#ifdef CONFIG_AVB_WARNING_LOGO
#include "lcd.h"
#include "video.h"
#include "dm/uclass.h"
#include "fsl_avb_logo.h"
#include "video_link.h"
#include "video_console.h"
#include "video_font_data.h"
#endif

int fastboot_flash_find_index(const char *name);

#if defined(CONFIG_IMX_TRUSTY_OS) && !defined(CONFIG_ARM64)
#define IVT_HEADER_MAGIC       0xD1
#define IVT_HDR_LEN       0x20
#define HAB_MAJ_VER       0x40
#define HAB_MAJ_MASK      0xF0

bool tos_flashed;

static bool tos_ivt_check(ulong start_addr, int ivt_offset) {
	const struct ivt *ivt_initial = NULL;
	const uint8_t *start = (const uint8_t *)start_addr;

	if (start_addr & 0x3) {
		puts("Error: tos's start address is not 4 byte aligned\n");
		return false;
	}

	ivt_initial = (const struct ivt *)(start + ivt_offset);

	const struct ivt_header *ivt_hdr = &ivt_initial->hdr;

	if ((ivt_hdr->magic == IVT_HEADER_MAGIC && \
		(be16_to_cpu(ivt_hdr->length) == IVT_HDR_LEN) && \
		(ivt_hdr->version & HAB_MAJ_MASK) == HAB_MAJ_VER) && \
		(ivt_initial->entry != 0x0) && \
		(ivt_initial->reserved1 == 0x0) && \
		(ivt_initial->self == (uint32_t)ivt_initial) && \
		(ivt_initial->csf != 0x0) && \
		(ivt_initial->reserved2 == 0x0)) {
		if (ivt_initial->dcd != 0x0)
			return false;
		else
			return true;
	}

	return false;
}

bool valid_tos() {
	/*
	 * If enabled SECURE_BOOT then use HAB to verify tos.
	 * Or check the IVT only.
	 */
	bool valid = false;
#ifdef CONFIG_IMX_HAB
	if (is_hab_enabled()) {
		valid = authenticate_image(TRUSTY_OS_ENTRY, TRUSTY_OS_PADDED_SZ);
	} else
#endif
	valid = tos_ivt_check(TRUSTY_OS_ENTRY, TRUSTY_OS_PADDED_SZ);

	if (valid) {
		tos_flashed = true;
		return true;
	} else {
		tos_flashed = false;
		return false;
	}
}

#endif

#if !defined(FASTBOOT_ENCRYPT_LOCK) || defined(NON_SECURE_FASTBOOT)

/*
 * This will return FASTBOOT_LOCK, FASTBOOT_UNLOCK or FASTBOOT_ERROR
 */
#ifndef CONFIG_IMX_TRUSTY_OS
static FbLockState decrypt_lock_store(unsigned char* bdata) {
	if (!strncmp((const char *)bdata, "locked", strlen("locked")))
		return FASTBOOT_LOCK;
	else if (!strncmp((const char *)bdata, "unlocked", strlen("unlocked")))
		return FASTBOOT_UNLOCK;
	else
		return FASTBOOT_LOCK_ERROR;
}
static inline int encrypt_lock_store(FbLockState lock, unsigned char* bdata) {
	if (FASTBOOT_LOCK == lock)
		strncpy((char *)bdata, "locked", strlen("locked") + 1);
	else if (FASTBOOT_UNLOCK == lock)
		strncpy((char *)bdata, "unlocked", strlen("unlocked") + 1);
	else
		return -1;
	return 0;
}
#endif
#else

static int sha1sum(unsigned char* data, int len, unsigned char* output) {
	struct hash_algo *algo;
	void *buf;
	if (hash_lookup_algo("sha1", &algo)) {
		printf("error in lookup sha1 algo!\n");
		return -1;
	}
	buf = map_sysmem((ulong)data, len);
	algo->hash_func_ws(buf, len, output, algo->chunk_size);
	unmap_sysmem(buf);

	return algo->digest_size;

}

static int generate_salt(unsigned char* salt) {
	unsigned long time = get_timer(0);
	return sha1sum((unsigned char *)&time, sizeof(unsigned long), salt);

}

static __maybe_unused FbLockState decrypt_lock_store(unsigned char *bdata) {
	int p = 0, ret;
	ALLOC_CACHE_ALIGN_BUFFER(uint8_t, plain_data, ENDATA_LEN);

	caam_open();
	ret = caam_decap_blob((uint32_t)(ulong)plain_data,
			      (uint32_t)(ulong)bdata + ROUND(ENDATA_LEN, ARCH_DMA_MINALIGN),
			      ENDATA_LEN);
	if (ret != 0) {
		printf("Error during blob decap operation: 0x%x\n",ret);
		return FASTBOOT_LOCK_ERROR;
	}
#ifdef FASTBOOT_LOCK_DEBUG
	FB_DEBUG("Decrypt data block are:\n \t=======\t\n");
	for (p = 0; p < ENDATA_LEN; p++) {
		FB_DEBUG("0x%2x  ", *(bdata + p));
		if (p % 16 == 0)
			FB_DEBUG("\n");
	}
	FB_DEBUG("\n \t========\t\n");
	for (p = ENDATA_LEN; p < (ENDATA_LEN + ENDATA_LEN + 48 ); p++) {
		FB_DEBUG("0x%2x  ", *(bdata + p));
		if (p % 16 == 0)
			FB_DEBUG("\n");
	}

	FB_DEBUG("\n plain text are:\n");
	for (p = 0; p < ENDATA_LEN; p++) {
		FB_DEBUG("0x%2x  ", plain_data[p]);
		if (p % 16 == 0)
			FB_DEBUG("\n");
	}
	FB_DEBUG("\n");
#endif

	for (p = 0; p < ENDATA_LEN-1; p++) {
		if (*(bdata+p) != plain_data[p]) {
			FB_DEBUG("Verify salt in decrypt error on pointer %d\n", p);
			return FASTBOOT_LOCK_ERROR;
		}
	}

	if (plain_data[ENDATA_LEN - 1] >= FASTBOOT_LOCK_NUM)
		return FASTBOOT_LOCK_ERROR;
	else
		return plain_data[ENDATA_LEN-1];
}

static __maybe_unused int encrypt_lock_store(FbLockState lock, unsigned char* bdata) {
	unsigned int p = 0;
	int ret;
	int salt_len = generate_salt(bdata);
	if (salt_len < 0)
		return -1;

    //salt_len cannot be longer than endata block size.
	if (salt_len >= ENDATA_LEN)
		salt_len = ENDATA_LEN - 1;

	p = ENDATA_LEN - 1;

	//Set lock value
	*(bdata + p) = lock;

	caam_open();
	ret = caam_gen_blob((uint32_t)(ulong)bdata,
			(uint32_t)(ulong)bdata + ROUND(ENDATA_LEN, ARCH_DMA_MINALIGN),
			ENDATA_LEN);
	if (ret != 0) {
		printf("error in caam_gen_blob:0x%x\n", ret);
		return -1;
	}


#ifdef FASTBOOT_LOCK_DEBUG
	int i = 0;
	FB_DEBUG("encrypt plain_text:\n");
	for (i = 0; i < ENDATA_LEN; i++) {
		FB_DEBUG("0x%2x\t", *(bdata+i));
		if (i % 16 == 0)
			printf("\n");
	}
	printf("\nto:\n");
	for (i=0; i < ENDATA_LEN + 48; i++) {
		FB_DEBUG("0x%2x\t", *(bdata + ENDATA_LEN + i));
		if (i % 16 == 0)
			printf("\n");
	}
	printf("\n");

#endif
	//protect value
	*(bdata + p) = 0xff;
	return 0;
}

#endif

static char mmc_dev_part[16];
static char* get_mmc_part(int part) {
	u32 dev_no = mmc_get_env_dev();
	sprintf(mmc_dev_part,"%x:%x",dev_no, part);
	return mmc_dev_part;
}

static inline void set_lock_disable_data(unsigned char* bdata) {
	*(bdata + SECTOR_SIZE -1) = 0;
}

/*
 * The enabling value is stored in the last byte of target partition.
 */
static inline unsigned char lock_enable_parse(unsigned char* bdata) {
	FB_DEBUG("lock_enable_parse: 0x%x\n", *(bdata + SECTOR_SIZE -1));
	if (*(bdata + SECTOR_SIZE -1) >= FASTBOOT_UL_NUM)
		return FASTBOOT_UL_ERROR;
	else
		return *(bdata + SECTOR_SIZE -1);
}

static FbLockState g_lockstat = FASTBOOT_UNLOCK;

#ifdef CONFIG_IMX_TRUSTY_OS
FbLockState fastboot_get_lock_stat(void) {
	uint8_t l_status;
	int ret;
	/*
	 * If Trusty OS not flashed, then must return
	 * unlock status to make device been able
	 * to flash Trusty OS binary.
	 */
#ifndef CONFIG_ARM64
	if (!tos_flashed)
		return FASTBOOT_UNLOCK;
#endif
	ret = trusty_read_lock_state(&l_status);
	if (ret < 0)
		return g_lockstat;
	else
		return l_status;

}

int fastboot_set_lock_stat(FbLockState lock) {
	int ret;
	/*
	 * If Trusty OS not flashed, we must prevent set lock
	 * status. Due the Trusty IPC won't work here.
	 */
#ifndef CONFIG_ARM64
	if (!tos_flashed)
		return 0;
#endif
	ret = trusty_write_lock_state(lock);
	if (ret < 0) {
		printf("cannot set lock status due Trusty return %d\n", ret);
		return ret;
	}
	return 0;
}
#else

/*
 * Set status of the lock&unlock to FSL_FASTBOOT_FB_PART
 * Currently use the very first Byte of FSL_FASTBOOT_FB_PART
 * to store the fastboot lock&unlock status
 */
int fastboot_set_lock_stat(FbLockState lock) {
	struct blk_desc *fs_dev_desc;
	disk_partition_t fs_partition;
	unsigned char *bdata;
	int mmc_id;
	int status, ret;

	bdata = (unsigned char *)memalign(ARCH_DMA_MINALIGN, SECTOR_SIZE);
	if (bdata == NULL)
		goto fail2;
	memset(bdata, 0, SECTOR_SIZE);

	mmc_id = fastboot_flash_find_index(FASTBOOT_PARTITION_FBMISC);
	if (mmc_id < 0) {
		printf("%s: error in get mmc part\n", __FUNCTION__);
		ret = -1;
		goto fail;
	}
	status = blk_get_device_part_str(FSL_FASTBOOT_FB_DEV,
		get_mmc_part(mmc_id),
		&fs_dev_desc, &fs_partition, 1);
	if (status < 0) {
		printf("%s:error in getdevice partition.\n", __FUNCTION__);
		ret = -1;
		goto fail;
	}

	status = encrypt_lock_store(lock, bdata);
	if (status < 0) {
		ret = -1;
		goto fail;
	}
	status = blk_dwrite(fs_dev_desc, fs_partition.start, 1, bdata);
	if (!status) {
		printf("%s:error in block write.\n", __FUNCTION__);
		ret = -1;
		goto fail;
	}
	ret = 0;
fail:
	free(bdata);
	return ret;
fail2:
	g_lockstat = lock;
	return 0;
}

FbLockState fastboot_get_lock_stat(void) {
	struct blk_desc *fs_dev_desc;
	disk_partition_t fs_partition;
	unsigned char *bdata;
	int mmc_id;
	FbLockState ret;
	/* uboot used by uuu will boot from USB, always return UNLOCK state */
	if (is_boot_from_usb())
		return g_lockstat;

	bdata = (unsigned char *)memalign(ARCH_DMA_MINALIGN, SECTOR_SIZE);
	if (bdata == NULL)
		return g_lockstat;

	int status;
	mmc_id = fastboot_flash_find_index(FASTBOOT_PARTITION_FBMISC);
	if (mmc_id < 0) {
		printf("%s: error in get mmc part\n", __FUNCTION__);
		ret = g_lockstat;
		goto fail;
	}
	status = blk_get_device_part_str(FSL_FASTBOOT_FB_DEV,
		get_mmc_part(mmc_id),
		&fs_dev_desc, &fs_partition, 1);

	if (status < 0) {
		printf("%s:error in getdevice partition.\n", __FUNCTION__);
		ret = g_lockstat;
		goto fail;
	}

	status = blk_dread(fs_dev_desc, fs_partition.start, 1, bdata);
	if (!status) {
		printf("%s:error in block read.\n", __FUNCTION__);
		ret = FASTBOOT_LOCK_ERROR;
		goto fail;
	}

	ret = decrypt_lock_store(bdata);
fail:
	free(bdata);
	return ret;
}
#endif


/* Return the last byte of of FSL_FASTBOOT_PR_DATA
 * which is managed by PresistDataService
 */

#ifdef CONFIG_ENABLE_LOCKSTATUS_SUPPORT
//Brillo has no presist data partition
FbLockEnableResult fastboot_lock_enable(void) {
	return FASTBOOT_UL_ENABLE;
}
void set_fastboot_lock_disable(void) {
}
#else
void set_fastboot_lock_disable(void) {
	struct blk_desc *fs_dev_desc;
	disk_partition_t fs_partition;
	unsigned char *bdata;
	int mmc_id;

	bdata = (unsigned char *)memalign(ALIGN_BYTES, SECTOR_SIZE);
	if (bdata == NULL)
		return;
	set_lock_disable_data(bdata);
	int status;
	mmc_id = fastboot_flash_find_index(FASTBOOT_PARTITION_PRDATA);
	if (mmc_id < 0) {
		printf("%s: error in get mmc part\n", __FUNCTION__);
		goto fail;
	}
	status = blk_get_device_part_str(FSL_FASTBOOT_FB_DEV,
		get_mmc_part(mmc_id),
		&fs_dev_desc, &fs_partition, 1);
	if (status < 0) {
		printf("%s:error in getdevice partition.\n", __FUNCTION__);
		goto fail;
	}

	lbaint_t target_block = fs_partition.start + fs_partition.size - 1;
	status = blk_dwrite(fs_dev_desc, target_block, 1, bdata);
	if (!status) {
		printf("%s: error in block read\n", __FUNCTION__);
		goto fail;
	}

fail:
	free(bdata);
	return;

}
FbLockEnableResult fastboot_lock_enable() {
#ifdef CONFIG_DUAL_BOOTLOADER
	/* Always allow unlock device in spl recovery mode. */
	if (is_spl_recovery())
		return FASTBOOT_UL_ENABLE;
#endif

#if defined(CONFIG_IMX_TRUSTY_OS) || defined(CONFIG_TRUSTY_UNLOCK_PERMISSION)
	int ret;
	uint8_t oem_device_unlock;

	ret = trusty_read_oem_unlock_device_permission(&oem_device_unlock);
	if (ret < 0)
		return FASTBOOT_UL_ERROR;
	else
		return oem_device_unlock;
#else /* CONFIG_IMX_TRUSTY_OS */
	FbLockEnableResult ret;
	struct blk_desc *fs_dev_desc;
	disk_partition_t fs_partition;
	unsigned char *bdata;
	int mmc_id;

	bdata = (unsigned char *)memalign(ALIGN_BYTES, SECTOR_SIZE);
	if (bdata == NULL)
		return FASTBOOT_UL_ERROR;
	int status;
	mmc_id = fastboot_flash_find_index(FASTBOOT_PARTITION_PRDATA);
	if (mmc_id < 0) {
		printf("%s: error in get mmc part\n", __FUNCTION__);
		ret = FASTBOOT_UL_ERROR;
		goto fail;
	}
	status = blk_get_device_part_str(FSL_FASTBOOT_FB_DEV,
		get_mmc_part(mmc_id),
		&fs_dev_desc, &fs_partition, 1);
	if (status < 0) {
		printf("%s:error in getdevice partition.\n", __FUNCTION__);
		ret = FASTBOOT_UL_ERROR;
		goto fail;
	}

    //The data is stored in the last blcok of this partition.
	lbaint_t target_block = fs_partition.start + fs_partition.size - 1;
	status = blk_dread(fs_dev_desc, target_block, 1, bdata);
	if (!status) {
		printf("%s: error in block read\n", __FUNCTION__);
		ret = FASTBOOT_UL_ERROR;
		goto fail;
	}
	int i = 0;
	FB_DEBUG("\n PRIST last sector is:\n");
	for (i = 0; i < SECTOR_SIZE; i++) {
		FB_DEBUG("0x%x  ", *(bdata + i));
		if (i % 32 == 0)
			FB_DEBUG("\n");
	}
	FB_DEBUG("\n");
	ret = lock_enable_parse(bdata);
fail:
	free(bdata);
	return ret;
#endif /* CONFIG_IMX_TRUSTY_OS */

}
#endif

int display_lock(FbLockState lock, int verify) {
	struct stdio_dev *disp;
	disp = stdio_get_by_name("vga");
	if (disp != NULL) {
		if (lock == FASTBOOT_UNLOCK) {
			disp->puts(disp, "\n============= NOTICE ============\n");
			disp->puts(disp,   "|                               |\n");
			disp->puts(disp,   "|   Your device is NOT locked.  |\n");
			disp->puts(disp,   "|                               |\n");
			disp->puts(disp,   "=================================\n");
		} else {
			if (verify == -1) {
				disp->puts(disp, "\n============= NOTICE ============\n");
				disp->puts(disp,   "|                               |\n");
				disp->puts(disp,   "| Your device is NOT protected. |\n");
				disp->puts(disp,   "|                               |\n");
				disp->puts(disp,   "=================================\n");
			} else if (verify == 1) {
				disp->puts(disp, "\n============= NOTICE ============\n");
				disp->puts(disp,   "|                               |\n");
				disp->puts(disp,   "|       Boot verify failed!     |\n");
				disp->puts(disp,   "|                               |\n");
				disp->puts(disp,   "=================================\n");
			}
		}
		return 0;
	} else
		printf("not found VGA disp console.\n");

	return -1;

}

#ifdef CONFIG_AVB_WARNING_LOGO
int display_unlock_warning(void) {
	int ret;
	struct udevice *dev;

	ret = uclass_first_device_err(UCLASS_VIDEO, &dev);
	if (!ret) {
		/* clear screen first */
		video_clear(dev);
		/* Draw the orange warning bmp logo */
		ret = bmp_display((ulong)orange_warning_bmp_bitmap,
					CONFIG_AVB_WARNING_LOGO_COLS, CONFIG_AVB_WARNING_LOGO_ROWS);

		/* Show warning text. */
		if (uclass_first_device_err(UCLASS_VIDEO_CONSOLE, &dev)) {
			printf("no text console device found!\n");
			return -1;
		}
		/* Adjust the cursor postion, the (x, y) are hard-coded here. */
		vidconsole_position_cursor(dev, CONFIG_AVB_WARNING_LOGO_COLS/VIDEO_FONT_WIDTH,
						CONFIG_AVB_WARNING_LOGO_ROWS/VIDEO_FONT_HEIGHT + 6);
		vidconsole_put_string(dev, "The bootloader is unlocked and software");
		vidconsole_position_cursor(dev, CONFIG_AVB_WARNING_LOGO_COLS/VIDEO_FONT_WIDTH,
						CONFIG_AVB_WARNING_LOGO_ROWS/VIDEO_FONT_HEIGHT + 7);
		vidconsole_put_string(dev, "integrity cannot be guaranteed. Any data");
		vidconsole_position_cursor(dev, CONFIG_AVB_WARNING_LOGO_COLS/VIDEO_FONT_WIDTH,
						CONFIG_AVB_WARNING_LOGO_ROWS/VIDEO_FONT_HEIGHT + 8);
		vidconsole_put_string(dev, "stored on the device may be available to");
		vidconsole_position_cursor(dev, CONFIG_AVB_WARNING_LOGO_COLS/VIDEO_FONT_WIDTH,
						CONFIG_AVB_WARNING_LOGO_ROWS/VIDEO_FONT_HEIGHT + 9);
		vidconsole_put_string(dev, "attackers. Do not store any sensitive data");
		vidconsole_position_cursor(dev, CONFIG_AVB_WARNING_LOGO_COLS/VIDEO_FONT_WIDTH,
						CONFIG_AVB_WARNING_LOGO_ROWS/VIDEO_FONT_HEIGHT + 10);
		vidconsole_put_string(dev, "on the device.");
		/* Jump one line to show the link */
		vidconsole_position_cursor(dev, CONFIG_AVB_WARNING_LOGO_COLS/VIDEO_FONT_WIDTH,
						CONFIG_AVB_WARNING_LOGO_ROWS/VIDEO_FONT_HEIGHT + 13);
		vidconsole_put_string(dev, "Visit this link on another device:");
		vidconsole_position_cursor(dev, CONFIG_AVB_WARNING_LOGO_COLS/VIDEO_FONT_WIDTH,
						CONFIG_AVB_WARNING_LOGO_ROWS/VIDEO_FONT_HEIGHT + 14);
		vidconsole_put_string(dev, "g.co/ABH");

		vidconsole_position_cursor(dev, CONFIG_AVB_WARNING_LOGO_COLS/VIDEO_FONT_WIDTH,
						CONFIG_AVB_WARNING_LOGO_ROWS/VIDEO_FONT_HEIGHT + 20);
		vidconsole_put_string(dev, "PRESS POWER BUTTON TO CONTINUE...");
		/* sync frame buffer */
		video_sync_all();

		return 0;
	} else {
		printf("no video device found!\n");
		return -1;
	}
}
#endif

int fastboot_wipe_data_partition(void)
{
	struct blk_desc *fs_dev_desc;
	disk_partition_t fs_partition;
	int status;
	int mmc_id;
	mmc_id = fastboot_flash_find_index(FASTBOOT_PARTITION_DATA);
	if (mmc_id < 0) {
		printf("%s: error in get mmc part\n", __FUNCTION__);
		return -1;
	}
	status = blk_get_device_part_str(FSL_FASTBOOT_FB_DEV,
		get_mmc_part(mmc_id), &fs_dev_desc, &fs_partition, 1);
	if (status < 0) {
		printf("error in get device partition for wipe /data\n");
		return -1;
	}
	status = blk_derase(fs_dev_desc, fs_partition.start , fs_partition.size );
	if (status != fs_partition.size ) {
		printf("erase not complete\n");
		return -1;
	}
	mdelay(2000);

	return 0;
}

void fastboot_wipe_all(void) {
	struct blk_desc *fs_dev_desc;
	disk_partition_t fs_partition;
	int status;
	int mmc_id;
	mmc_id = fastboot_flash_find_index(FASTBOOT_PARTITION_GPT);
	if (mmc_id < 0) {
		printf("%s: error in get mmc part\n", __FUNCTION__);
		return;
	}
	status = blk_get_device_part_str(FSL_FASTBOOT_FB_DEV,
		get_mmc_part(mmc_id), &fs_dev_desc, &fs_partition, 1);
	if (status < 0) {
		printf("error in get device partition for wipe user partition\n");
		return;
	}
	status = blk_derase(fs_dev_desc, fs_partition.start , fs_partition.size );
	if (status != fs_partition.size ) {
		printf("erase not complete\n");
		return;
	}
	printf("fastboot wiped all.\n");
}
