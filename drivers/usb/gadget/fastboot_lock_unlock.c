/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#include <fsl_fastboot.h>
#ifdef CONFIG_IMX_TRUSTY_OS
#include <trusty/libtipc.h>
#include <asm/imx-common/hab.h>
#endif

#ifdef FASTBOOT_ENCRYPT_LOCK

#include <hash.h>
#include <fsl_caam.h>

//Encrypted data is 80bytes length.
#define ENDATA_LEN 80

#endif

int fastboot_flash_find_index(const char *name);

#ifdef CONFIG_IMX_TRUSTY_OS
#define HAB_TAG_IVT       0xD1
#define IVT_HDR_LEN       0x20
#define HAB_MAJ_VER       0x40
#define HAB_MAJ_MASK      0xF0

bool tos_flashed;

static bool tos_ivt_check(ulong start_addr, int ivt_offset) {
	const struct hab_ivt *ivt_initial = NULL;
	const uint8_t *start = (const uint8_t *)start_addr;

	if (start_addr & 0x3) {
		puts("Error: tos's start address is not 4 byte aligned\n");
		return false;
	}

	ivt_initial = (const struct hab_ivt *)(start + ivt_offset);

	const struct hab_hdr *ivt_hdr = &ivt_initial->hdr;

	if ((ivt_hdr->tag == HAB_TAG_IVT && \
		((ivt_hdr->len[0] << 8) + ivt_hdr->len[1]) == IVT_HDR_LEN && \
		(ivt_hdr->par & HAB_MAJ_MASK) == HAB_MAJ_VER) && \
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
#ifdef CONFIG_SECURE_BOOT
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
		strncpy((char *)bdata, "locked", strlen("locked"));
	else if (FASTBOOT_UNLOCK == lock)
		strncpy((char *)bdata, "unlocked", strlen("unlocked"));
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

static FbLockState decrypt_lock_store(unsigned char *bdata) {
	unsigned char plain_data[ENDATA_LEN];
	int p = 0, ret;

	caam_open();
	ret = caam_decap_blob((uint32_t)plain_data,
			      (uint32_t)bdata + ENDATA_LEN, ENDATA_LEN);
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

static int encrypt_lock_store(FbLockState lock, unsigned char* bdata) {
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
	ret = caam_gen_blob((uint32_t)bdata, (uint32_t)(bdata + ENDATA_LEN), ENDATA_LEN);
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
	if (!tos_flashed)
		return FASTBOOT_UNLOCK;
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
	if (!tos_flashed)
		return 0;
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

	bdata = (unsigned char *)memalign(ALIGN_BYTES, SECTOR_SIZE);
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
	status = fs_dev_desc->block_write(fs_dev_desc, fs_partition.start, 1, bdata);
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

	bdata = (unsigned char *)memalign(ALIGN_BYTES, SECTOR_SIZE);
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

	status = fs_dev_desc->block_read(fs_dev_desc, fs_partition.start, 1, bdata);
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
	status = fs_dev_desc->block_write(fs_dev_desc, target_block, 1, bdata);
	if (!status) {
		printf("%s: error in block read\n", __FUNCTION__);
		goto fail;
	}

fail:
	free(bdata);
	return;

}
FbLockEnableResult fastboot_lock_enable() {
	struct blk_desc *fs_dev_desc;
	disk_partition_t fs_partition;
	unsigned char *bdata;
	int mmc_id;
	FbLockEnableResult ret;

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
	status = fs_dev_desc->block_read(fs_dev_desc, target_block, 1, bdata);
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
	status = fs_dev_desc->block_erase(fs_dev_desc, fs_partition.start , fs_partition.size );
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
	status = fs_dev_desc->block_erase(fs_dev_desc, fs_partition.start , fs_partition.size );
	if (status != fs_partition.size ) {
		printf("erase not complete\n");
		return;
	}
	printf("fastboot wiped all.\n");
}
