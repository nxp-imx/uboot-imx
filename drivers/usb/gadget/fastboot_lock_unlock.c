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

#include <fsl_fastboot.h>
#include "fastboot_lock_unlock.h"

#include <common.h>
#include <linux/types.h>
#include <part.h>
#include <ext_common.h>
#include <stdio_dev.h>

#ifdef FASTBOOT_ENCRYPT_LOCK

#include <hash.h>
#include <fsl_caam.h>

//Encrypted data is 80bytes length.
#define ENDATA_LEN 80

#endif


#ifndef FASTBOOT_ENCRYPT_LOCK

/*
 * This will return FASTBOOT_LOCK, FASTBOOT_UNLOCK or FASTBOOT_ERROR
 */
inline unsigned char decrypt_lock_store(unsigned char* bdata) {
	return *bdata;
}

inline void encrypt_lock_store(unsigned char lock, unsigned char* bdata) {
	*bdata  = lock;
}
#else

int sha1sum(unsigned char* data, int len, unsigned char* output) {
	struct hash_algo *algo;
	void *buf;
	if (hash_lookup_algo("sha1", &algo)) {
	printf("error in lookup sha1 algo!\n");
	return -1;
	}
	buf = map_sysmem(data, len);
	algo->hash_func_ws(buf, len, output, algo->chunk_size);
	unmap_sysmem(buf);

	return algo->digest_size;

}

int generate_salt(unsigned char* salt) {
	unsigned long time = get_timer(0);
	return sha1sum(&time, sizeof(unsigned long), salt);

}

unsigned char decrypt_lock_store(unsigned char *bdata) {
	unsigned char plain_data[ENDATA_LEN];
	int p = 0, ret;

	caam_open();
	ret = caam_decap_blob((uint32_t)plain_data, bdata + ENDATA_LEN, ENDATA_LEN);
	if (ret != 0) {
	    printf("Error during blob decap operation: 0x%x\n",ret);
	    return FASTBOOT_LOCK_ERROR;
	}
#ifdef FASTBOOT_LOCK_DEBUG
	DEBUG("Decrypt data block are:\n \t=======\t\n");
	for (p = 0; p < ENDATA_LEN; p++) {
		DEBUG("0x%2x  ", *(bdata + p));
		if (p % 16 == 0)
		DEBUG("\n");
	}
	DEBUG("\n \t========\t\n");
	for (p = ENDATA_LEN; p < (ENDATA_LEN + ENDATA_LEN + 48 ); p++) {
		DEBUG("0x%2x  ", *(bdata + p));
		if (p % 16 == 0)
		DEBUG("\n");
	}

	DEBUG("\n plain text are:\n");
	for (p = 0; p < ENDATA_LEN; p++) {
		DEBUG("0x%2x  ", plain_data[p]);
		if (p % 16 == 0)
		DEBUG("\n");
	}
	DEBUG("\n");
#endif

	for (p = 0; p < ENDATA_LEN-1; p++) {
		if (*(bdata+p) != plain_data[p]) {
		DEBUG("Verify salt in decrypt error on pointer %d\n", p);
		return FASTBOOT_LOCK_ERROR;
		}
	}

	return plain_data[ENDATA_LEN-1];
}

int encrypt_lock_store(unsigned char lock, unsigned char* bdata) {
	unsigned int p = 0;
	int ret;
	int salt_len = generate_salt(bdata);
	if (salt_len < 0)
		return;

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
	DEBUG("encrypt plain_text:\n");
	for (i = 0; i < ENDATA_LEN; i++) {
		DEBUG("0x%2x\t", *(bdata+i));
		if (i % 16 == 0)
		printf("\n");
	}
	printf("\nto:\n");
	for (i=0; i < ENDATA_LEN + 48; i++) {
		DEBUG("0x%2x\t", *(bdata + ENDATA_LEN + i));
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
char* get_mmc_part(int part) {
	u32 dev_no = mmc_get_env_devno();
	sprintf(mmc_dev_part,"%x:%x",dev_no, part);
	return mmc_dev_part;
}

/*
 * The enabling value is stored in the last byte of target partition.
 */
inline unsigned char lock_enable_parse(unsigned char* bdata) {
	DEBUG("lock_enable_parse: 0x%x\n", *(bdata + SECTOR_SIZE -1));
	return *(bdata + SECTOR_SIZE -1);
}

/*
 * Set status of the lock&unlock to FSL_FASTBOOT_FB_PART
 * Currently use the very first Byte of FSL_FASTBOOT_FB_PART
 * to store the fastboot lock&unlock status
 */
int fastboot_set_lock_stat(unsigned char lock) {
	block_dev_desc_t *fs_dev_desc;
	disk_partition_t fs_partition;
	unsigned char *bdata;

	bdata = (unsigned char *)memalign(ALIGN_BYTES, SECTOR_SIZE);
	memset(bdata, 0, SECTOR_SIZE);

	int status;
	status = get_device_and_partition(FSL_FASTBOOT_FB_DEV,
		get_mmc_part(FSL_FASTBOOT_FB_PART_NUM),
		&fs_dev_desc, &fs_partition, 1);
	if (!status) {
		printf("%s:error in getdevice partition.\n", __FUNCTION__);
		return -1;
	}
	DEBUG("%s %s partition.start=%d, size=%d\n",FSL_FASTBOOT_FB_DEV,
		get_mmc_part(FSL_FASTBOOT_FB_PART_NUM), fs_partition.start, fs_partition.size);

	status = encrypt_lock_store(lock, bdata);
	if (status < 0)
		return -1;
	status = fs_dev_desc->block_write(fs_dev_desc->dev, fs_partition.start, 1, bdata);
	if (!status) {
		printf("%s:error in block write.\n", __FUNCTION__);
		return -1;
	}



	return 0;
}

unsigned char fastboot_get_lock_stat() {

	block_dev_desc_t *fs_dev_desc;
	disk_partition_t fs_partition;
	unsigned char *bdata;

	bdata = (unsigned char *)memalign(ALIGN_BYTES, SECTOR_SIZE);

	int status;
	status = get_device_and_partition(FSL_FASTBOOT_FB_DEV,
		get_mmc_part(FSL_FASTBOOT_FB_PART_NUM),
		&fs_dev_desc, &fs_partition, 1);

	if (!status) {
		printf("%s:error in getdevice partition.\n", __FUNCTION__);
		return FASTBOOT_LOCK_ERROR;
	}
	DEBUG("%s %s partition.start=%d, size=%d\n",FSL_FASTBOOT_FB_DEV,
		get_mmc_part(FSL_FASTBOOT_FB_PART_NUM), fs_partition.start, fs_partition.size);

	status = fs_dev_desc->block_read(fs_dev_desc->dev, fs_partition.start, 1, bdata);
	if (!status) {
		printf("%s:error in block read.\n", __FUNCTION__);
		return FASTBOOT_LOCK_ERROR;
	}

	return decrypt_lock_store(bdata);
}


/* Return the last byte of of FSL_FASTBOOT_PR_DATA
 * which is managed by PresistDataService
 */

#ifdef CONFIG_BRILLO_SUPPORT
//Brillo has no presist data partition
unsigned char fastboot_lock_enable() {
	return FASTBOOT_UL_ENABLE;
}
#else
unsigned char fastboot_lock_enable() {
	block_dev_desc_t *fs_dev_desc;
	disk_partition_t fs_partition;
	unsigned char *bdata;

	bdata = (unsigned char *)memalign(ALIGN_BYTES, SECTOR_SIZE);

	int status;
	status = get_device_and_partition(FSL_FASTBOOT_FB_DEV,
		get_mmc_part(FSL_FASTBOOT_PR_DATA_PART_NUM),
		&fs_dev_desc, &fs_partition, 1);
	if (!status) {
		printf("%s:error in getdevice partition.\n", __FUNCTION__);
		return FASTBOOT_UL_ERROR;
	}

    //The data is stored in the last blcok of this partition.
	lbaint_t target_block = fs_partition.start + fs_partition.size - 1;
	DEBUG("target_block.start=%d, size=%d target_block=%d\n", fs_partition.start, fs_partition.size, target_block);
	status = fs_dev_desc->block_read(fs_dev_desc->dev, target_block, 1, bdata);
	if (!status) {
		printf("%s: error in block read\n", __FUNCTION__);
		return FASTBOOT_UL_ERROR;
	}
	int i = 0;
	DEBUG("\n PRIST last sector is:\n");
	for (i = 0; i < SECTOR_SIZE; i++) {
		DEBUG("0x%x  ", *(bdata + i));
		if (i % 32 == 0)
			DEBUG("\n");
	}
	DEBUG("\n");
	return lock_enable_parse(bdata);

}
#endif

int display_lock(int lock, int verify) {
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
	}
	else
	printf("not found VGA disp console.\n");

	return -1;

}

int fastboot_wipe_data_partition() {
	block_dev_desc_t *fs_dev_desc;
	disk_partition_t fs_partition;
	int status;
	status = get_device_and_partition(FSL_FASTBOOT_FB_DEV,
		get_mmc_part(FSL_FASTBOOT_DATA_PART_NUM), &fs_dev_desc, &fs_partition, 1);
	if (!status) {
		printf("error in get device partition for wipe /data\n");
		return -1;
	}
	DEBUG("fs->start=%x, size=%d\n", fs_partition.start, fs_partition.size);
	status = fs_dev_desc->block_erase(fs_dev_desc->dev, fs_partition.start , fs_partition.size );
	if (status != fs_partition.size ) {
		printf("erase not complete\n");
		return -1;
	}
	mdelay(2000);

	return 0;
}
