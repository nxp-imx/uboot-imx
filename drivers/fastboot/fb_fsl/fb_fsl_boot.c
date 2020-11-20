// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
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
#include <lz4.h>
#include "../lib/avb/fsl/utils.h"

#ifdef CONFIG_AVB_SUPPORT
#include <dt_table.h>
#include <fsl_avb.h>
#endif

#ifdef CONFIG_ANDROID_THINGS_SUPPORT
#include <asm-generic/gpio.h>
#include <asm/mach-imx/gpio.h>
#include "../lib/avb/fsl/fsl_avbkey.h"
#include "../arch/arm/include/asm/mach-imx/hab.h"
#endif

#if defined(CONFIG_FASTBOOT_LOCK)
#include "fastboot_lock_unlock.h"
#endif

#ifdef CONFIG_IMX_TRUSTY_OS
#include "u-boot/sha256.h"
#include <trusty/libtipc.h>
#include <trusty/hwcrypto.h>
#endif

#include "fb_fsl_common.h"

/* max kernel image size */
#ifdef CONFIG_ARCH_IMX8
/* imx8q has more limitation so we assign less memory here. */
#define MAX_KERNEL_LEN (60 * 1024 * 1024)
#else
#define MAX_KERNEL_LEN (64 * 1024 * 1024)
#endif

/* Offset (in u32's) of start and end fields in the zImage header. */
#define ZIMAGE_START_ADDR	10
#define ZIMAGE_END_ADDR	11

/* Boot metric variables */
boot_metric metrics = {
  .bll_1 = 0,
  .ble_1 = 0,
  .kl	 = 0,
  .kd	 = 0,
  .avb	 = 0,
  .odt	 = 0,
  .sw	 = 0
};

int read_from_partition_multi(const char* partition,
		int64_t offset, size_t num_bytes, void* buffer, size_t* out_num_read)
{
	struct fastboot_ptentry *pte;
	unsigned char *bdata;
	unsigned char *out_buf = (unsigned char *)buffer;
	unsigned char *dst, *dst64 = NULL;
	unsigned long blksz;
	unsigned long s, cnt;
	size_t num_read = 0;
	lbaint_t part_start, part_end, bs, be, bm, blk_num;
	margin_pos_t margin;
	struct blk_desc *fs_dev_desc = NULL;
	int dev_no;
	int ret;

	assert(buffer != NULL && out_num_read != NULL);

	dev_no = mmc_get_env_dev();
	if ((fs_dev_desc = blk_get_dev("mmc", dev_no)) == NULL) {
		printf("mmc device not found\n");
		return -1;
	}

	pte = fastboot_flash_find_ptn(partition);
	if (!pte) {
		printf("no %s partition\n", partition);
		fastboot_flash_dump_ptn();
		return -1;
	}

	blksz = fs_dev_desc->blksz;
	part_start = pte->start;
	part_end = pte->start + pte->length - 1;

	if (get_margin_pos((uint64_t)part_start, (uint64_t)part_end, blksz,
				&margin, offset, num_bytes, true))
		return -1;

	bs = (lbaint_t)margin.blk_start;
	be = (lbaint_t)margin.blk_end;
	s = margin.start;
	bm = margin.multi;

	/* alloc a blksz mem */
	bdata = (unsigned char *)memalign(ALIGN_BYTES, blksz);
	if (bdata == NULL) {
		printf("Failed to allocate memory!\n");
		return -1;
	}

	/* support multi blk read */
	while (bs <= be) {
		if (!s && bm > 1) {
			dst = out_buf;
			dst64 = PTR_ALIGN(out_buf, 64); /* for mmc blk read alignment */
			if (dst64 != dst) {
				dst = dst64;
				bm--;
			}
			blk_num = bm;
			cnt = bm * blksz;
			bm = 0; /* no more multi blk */
		} else {
			blk_num = 1;
			cnt = blksz - s;
			if (num_read + cnt > num_bytes)
				cnt = num_bytes - num_read;
			dst = bdata;
		}
		if (blk_dread(fs_dev_desc, bs, blk_num, dst) != blk_num) {
			ret = -1;
			goto fail;
		}

		if (dst == bdata)
			memcpy(out_buf, bdata + s, cnt);
		else if (dst == dst64)
			memcpy(out_buf, dst, cnt); /* internal copy */

		s = 0;
		bs += blk_num;
		num_read += cnt;
		out_buf += cnt;
	}
	*out_num_read = num_read;
	ret = 0;

fail:
	free(bdata);
	return ret;
}


#if defined(CONFIG_FASTBOOT_LOCK)
int do_lock_status(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) {
	FbLockState status = fastboot_get_lock_stat();
	if (status != FASTBOOT_LOCK_ERROR) {
		if (status == FASTBOOT_LOCK)
			printf("fastboot lock status: locked.\n");
		else
			printf("fastboot lock status: unlocked.\n");
	} else
		printf("fastboot lock status error!\n");

	display_lock(status, -1);

	return 0;

}

U_BOOT_CMD(
	lock_status, 2, 1, do_lock_status,
	"lock_status",
	"lock_status");
#endif

#if defined(CONFIG_FLASH_MCUFIRMWARE_SUPPORT) && defined(CONFIG_ARCH_IMX8M)
static int do_bootmcu(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret;
	size_t out_num_read;
	void *mcu_base_addr = (void *)MCU_BOOTROM_BASE_ADDR;
	char command[32];

	ret = read_from_partition_multi(FASTBOOT_MCU_FIRMWARE_PARTITION,
			0, ANDROID_MCU_FIRMWARE_SIZE, (void *)mcu_base_addr, &out_num_read);
	if ((ret != 0) || (out_num_read != ANDROID_MCU_FIRMWARE_SIZE)) {
		printf("Read MCU images failed!\n");
		return 1;
	} else {
		printf("run command: 'bootaux 0x%x'\n",(unsigned int)(ulong)mcu_base_addr);

		sprintf(command, "bootaux 0x%x", (unsigned int)(ulong)mcu_base_addr);
		ret = run_command(command, 0);
		if (ret) {
			printf("run 'bootaux' command failed!\n");
			return 1;
		}
	}
	return 0;
}

U_BOOT_CMD(
	bootmcu, 1, 0, do_bootmcu,
	"boot mcu images\n",
	"boot mcu images from 'mcu_os' partition, only support images run from TCM"
);
#endif

#ifdef CONFIG_CMD_BOOTA

/* Section for Android bootimage format support */

#if !defined(CONFIG_ANDROID_DYNAMIC_PARTITION) && defined(CONFIG_SYSTEM_RAMDISK_SUPPORT)
/* Setup booargs for taking the system parition as ramdisk */
static void fastboot_setup_system_boot_args(const char *slot, bool append_root)
{
	const char *system_part_name = NULL;
#ifdef CONFIG_ANDROID_AB_SUPPORT
	if(slot == NULL)
		return;
	if(!strncmp(slot, "_a", strlen("_a")) || !strncmp(slot, "boot_a", strlen("boot_a"))) {
		system_part_name = FASTBOOT_PARTITION_SYSTEM_A;
	}
	else if(!strncmp(slot, "_b", strlen("_b")) || !strncmp(slot, "boot_b", strlen("boot_b"))) {
		system_part_name = FASTBOOT_PARTITION_SYSTEM_B;
	} else {
		printf("slot invalid!\n");
		return;
	}
#else
	system_part_name = FASTBOOT_PARTITION_SYSTEM;
#endif

	struct fastboot_ptentry *ptentry = fastboot_flash_find_ptn(system_part_name);
	if(ptentry != NULL) {
		char bootargs_3rd[ANDR_BOOT_ARGS_SIZE] = {'\0'};
		if (append_root) {
			u32 dev_no = mmc_map_to_kernel_blk(mmc_get_env_dev());
			sprintf(bootargs_3rd, "root=/dev/mmcblk%dp%d ",
					dev_no,
					ptentry->partition_index);
		}
		strcat(bootargs_3rd, "rootwait");

		env_set("bootargs_3rd", bootargs_3rd);
	} else {
		printf("Can't find partition: %s\n", system_part_name);
		fastboot_flash_dump_ptn();
	}
}
#endif

void
bootimg_print_image_hdr(struct andr_img_hdr *hdr)
{
#ifdef DEBUG
	int i;
	printf("   Image magic:   %s\n", hdr->magic);

	printf("   kernel_size:   0x%x\n", hdr->kernel_size);
	printf("   kernel_addr:   0x%x\n", hdr->kernel_addr);

	printf("   rdisk_size:   0x%x\n", hdr->ramdisk_size);
	printf("   rdisk_addr:   0x%x\n", hdr->ramdisk_addr);

	printf("   second_size:   0x%x\n", hdr->second_size);
	printf("   second_addr:   0x%x\n", hdr->second_addr);

	printf("   tags_addr:   0x%x\n", hdr->tags_addr);
	printf("   page_size:   0x%x\n", hdr->page_size);

	printf("   name:      %s\n", hdr->name);
	printf("   cmdline:   %s\n", hdr->cmdline);

	for (i = 0; i < 8; i++)
		printf("   id[%d]:   0x%x\n", i, hdr->id[i]);
#endif
}

#if !defined(CONFIG_AVB_SUPPORT) || !defined(CONFIG_MMC)
static struct andr_img_hdr boothdr __aligned(ARCH_DMA_MINALIGN);
#endif

#ifdef CONFIG_IMX_TRUSTY_OS
#if defined(CONFIG_DUAL_BOOTLOADER) && defined(CONFIG_AVB_ATX)
static int sha256_concatenation(uint8_t *hash_buf, uint8_t *vbh, uint8_t *image_hash)
{
	if ((hash_buf == NULL) || (vbh == NULL) || (image_hash == NULL)) {
		printf("sha256_concatenation: null buffer found!\n");
		return -1;
	}

	memcpy(hash_buf, vbh, AVB_SHA256_DIGEST_SIZE);
	memcpy(hash_buf + AVB_SHA256_DIGEST_SIZE,
	       image_hash, AVB_SHA256_DIGEST_SIZE);
	sha256_csum_wd((unsigned char *)hash_buf, 2 * AVB_SHA256_DIGEST_SIZE,
		       (unsigned char *)vbh, CHUNKSZ_SHA256);

	return 0;
}

/* Since we use fit format to organize the atf, tee, u-boot and u-boot dtb,
 * so calculate the hash of fit is enough.
 */
static int vbh_bootloader(uint8_t *image_hash)
{
	char* slot_suffixes[2] = {"_a", "_b"};
	char partition_name[20];
	AvbABData ab_data;
	uint8_t *image_buf = NULL;
	uint32_t image_size;
	size_t image_num_read;
	int target_slot;
	int ret = 0;

	/* Load A/B metadata and decide which slot we are going to load */
	if (fsl_avb_ab_ops.read_ab_metadata(&fsl_avb_ab_ops, &ab_data) !=
					    AVB_IO_RESULT_OK) {
		ret = -1;
		goto fail ;
	}
	target_slot = get_curr_slot(&ab_data);
	sprintf(partition_name, "bootloader%s", slot_suffixes[target_slot]);

	/* Read image header to find the image size */
	image_buf = (uint8_t *)malloc(MMC_SATA_BLOCK_SIZE);
	if (fsl_avb_ops.read_from_partition(&fsl_avb_ops, partition_name,
					    0, MMC_SATA_BLOCK_SIZE,
					    image_buf, &image_num_read)) {
		printf("bootloader image load error!\n");
		ret = -1;
		goto fail;
	}
	image_size = fdt_totalsize((struct image_header *)image_buf);
	image_size = (image_size + 3) & ~3;
	free(image_buf);

	/* Load full fit image */
	image_buf = (uint8_t *)malloc(image_size);
	if (fsl_avb_ops.read_from_partition(&fsl_avb_ops, partition_name,
					    0, image_size,
					    image_buf, &image_num_read)) {
		printf("bootloader image load error!\n");
		ret = -1;
		goto fail;
	}
	/* Calculate hash */
	sha256_csum_wd((unsigned char *)image_buf, image_size,
		       (unsigned char *)image_hash, CHUNKSZ_SHA256);

fail:
	if (image_buf != NULL)
		free(image_buf);
	return ret;
}

int vbh_calculate(uint8_t *vbh, AvbSlotVerifyData *avb_out_data)
{
	uint8_t image_hash[AVB_SHA256_DIGEST_SIZE];
	uint8_t hash_buf[2 * AVB_SHA256_DIGEST_SIZE];
	uint8_t* image_buf = NULL;
	uint32_t image_size;
	size_t image_num_read;
	int ret = 0;

	if (vbh == NULL)
		return -1;

	/* Initial VBH (VBH0) should be 32 bytes 0 */
	memset(vbh, 0, AVB_SHA256_DIGEST_SIZE);
	/* Load and calculate the sha256 hash of spl.bin */
	image_size = (ANDROID_SPL_SIZE + MMC_SATA_BLOCK_SIZE -1) /
		      MMC_SATA_BLOCK_SIZE;
	image_buf = (uint8_t *)malloc(image_size);
	if (fsl_avb_ops.read_from_partition(&fsl_avb_ops,
					    FASTBOOT_PARTITION_BOOTLOADER,
					    0, image_size,
					    image_buf, &image_num_read)) {
		printf("spl image load error!\n");
		ret = -1;
		goto fail;
	}
	sha256_csum_wd((unsigned char *)image_buf, image_size,
		       (unsigned char *)image_hash, CHUNKSZ_SHA256);
	/* Calculate VBH1 */
	if (sha256_concatenation(hash_buf, vbh, image_hash)) {
		ret = -1;
		goto fail;
	}
	free(image_buf);

	/* Load and calculate hash of bootloader.img */
	if (vbh_bootloader(image_hash)) {
		ret = -1;
		goto fail;
	}

	/* Calculate VBH2 */
	if (sha256_concatenation(hash_buf, vbh, image_hash)) {
		ret = -1;
		goto fail;
	}

	/* Calculate the hash of vbmeta.img */
	avb_slot_verify_data_calculate_vbmeta_digest(avb_out_data,
						     AVB_DIGEST_TYPE_SHA256,
						     image_hash);
	/* Calculate VBH3 */
	if (sha256_concatenation(hash_buf, vbh, image_hash)) {
		ret = -1;
		goto fail;
	}

fail:
	if (image_buf != NULL)
		free(image_buf);
	return ret;
}
#endif /* CONFIG_DUAL_BOOTLOADER && CONFIG_AVB_ATX */

int trusty_setbootparameter(uint32_t os_version,
				AvbABFlowResult avb_result, AvbSlotVerifyData *avb_out_data) {
	int ret = 0;
	uint8_t vbh[AVB_SHA256_DIGEST_SIZE];
	u32 os_ver = os_version >> 11;
	u32 os_ver_km = (((os_ver >> 14) & 0x7F) * 100 + ((os_ver >> 7) & 0x7F)) * 100
	    + (os_ver & 0x7F);
	u32 os_lvl = os_version & ((1U << 11) - 1);
	u32 os_lvl_km = ((os_lvl >> 4) + 2000) * 100 + (os_lvl & 0x0F);
	keymaster_verified_boot_t vbstatus;
	FbLockState lock_status = fastboot_get_lock_stat();
	uint8_t boot_key_hash[AVB_SHA256_DIGEST_SIZE];

	bool lock = (lock_status == FASTBOOT_LOCK)? true: false;
	if ((avb_result == AVB_AB_FLOW_RESULT_OK) && lock)
		vbstatus = KM_VERIFIED_BOOT_VERIFIED;
	else
		vbstatus = KM_VERIFIED_BOOT_UNVERIFIED;

#ifdef CONFIG_AVB_ATX
	if (fsl_read_permanent_attributes_hash(&fsl_avb_atx_ops, boot_key_hash)) {
		printf("ERROR - failed to read permanent attributes hash for keymaster\n");
		memset(boot_key_hash, '\0', AVB_SHA256_DIGEST_SIZE);
	}
#else
	uint8_t public_key_buf[AVB_MAX_BUFFER_LENGTH];
	if (trusty_read_vbmeta_public_key(public_key_buf,
						AVB_MAX_BUFFER_LENGTH) != 0) {
		printf("ERROR - failed to read public key for keymaster\n");
		memset(boot_key_hash, '\0', AVB_SHA256_DIGEST_SIZE);
	} else
		sha256_csum_wd((unsigned char *)public_key_buf, AVB_SHA256_DIGEST_SIZE,
				(unsigned char *)boot_key_hash, CHUNKSZ_SHA256);
#endif

	/* All '\0' boot key should be passed if the device is unlocked. */
	if (!lock)
		memset(boot_key_hash, '\0', AVB_SHA256_DIGEST_SIZE);

	/* Calculate VBH */
#if defined(CONFIG_DUAL_BOOTLOADER) && defined(CONFIG_AVB_ATX)
	if (vbh_calculate(vbh, avb_out_data)) {
		ret = -1;
		goto fail;
	}
#else
	avb_slot_verify_data_calculate_vbmeta_digest(avb_out_data,
						     AVB_DIGEST_TYPE_SHA256,
						     vbh);
#endif
	trusty_set_boot_params(os_ver_km, os_lvl_km, vbstatus, lock,
					boot_key_hash, AVB_SHA256_DIGEST_SIZE,
					vbh, AVB_SHA256_DIGEST_SIZE);

#if defined(CONFIG_DUAL_BOOTLOADER) && defined(CONFIG_AVB_ATX)
fail:
#endif
	return ret;
}
#endif

#if defined(CONFIG_AVB_SUPPORT) && defined(CONFIG_MMC)
/* we can use avb to verify Trusty if we want */
const char *requested_partitions_boot[] = {"boot", "dtbo", "vendor_boot", NULL};
const char *requested_partitions_recovery[] = {"recovery", NULL};

static bool gki_is_enabled(void)
{
	size_t size;
	struct andr_img_hdr hdr;
	char partition_name[20];

#ifdef CONFIG_ANDROID_AB_SUPPORT
	int target_slot;
	struct bootloader_control ab_data;
	char* slot_suffixes[2] = {"_a", "_b"};

	if (fsl_avb_ab_ops.read_ab_metadata(&fsl_avb_ab_ops, &ab_data) !=
					    AVB_IO_RESULT_OK) {
		printf("Read A/B metadata fail!\n");
		return false;
	}
	target_slot = get_curr_slot(&ab_data);
	sprintf(partition_name, "boot%s", slot_suffixes[target_slot]);
#else
	sprintf(partition_name, "boot");
#endif

	/* Read boot header to find the version */
	if (fsl_avb_ops.read_from_partition(&fsl_avb_ops, partition_name,
					    0, sizeof(struct andr_img_hdr),
					    (void *)&hdr, &size)) {
		printf("%s load error!\n", partition_name);
		return false;
	}

	if (hdr.header_version >= 3)
		return true;
	else
		return false;
}

static int find_partition_data_by_name(char* part_name,
		AvbSlotVerifyData* avb_out_data, AvbPartitionData** avb_loadpart)
{
	int num = 0;
	AvbPartitionData* loadpart = NULL;

	for (num = 0; num < avb_out_data->num_loaded_partitions; num++) {
		loadpart = &(avb_out_data->loaded_partitions[num]);
		if (!(strncmp(loadpart->partition_name,
			part_name, strlen(part_name)))) {
			*avb_loadpart = loadpart;
			break;
		}
	}
	if (num == avb_out_data->num_loaded_partitions) {
		printf("Error! Can't find %s partition from avb partition data!\n",
				part_name);
		return -1;
	}
	else
		return 0;
}

bool __weak is_power_key_pressed(void) {
	return false;
}

int do_boota(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) {

	ulong addr = 0;
	u32 avb_metric;
	bool check_image_arm64 =  false;
	bool is_recovery_mode = false;
	bool gki_is_supported = false;

	/* 'hdr' should point to boot.img */
	struct andr_img_hdr *hdr = NULL;
	struct boot_img_hdr_v3 *hdr_v3 = NULL;
	struct vendor_boot_img_hdr_v3 *vendor_boot_hdr = NULL;

	AvbABFlowResult avb_result;
	AvbSlotVerifyData *avb_out_data = NULL;
	AvbPartitionData *avb_loadpart = NULL;
	AvbPartitionData *avb_vendorboot = NULL;

	/* get bootmode, default to boot "boot" */
	if (argc > 1) {
		is_recovery_mode =
			(strncmp(argv[1], "recovery", sizeof("recovery")) != 0) ? false: true;
		if (is_recovery_mode)
			printf("Will boot from recovery!\n");
	}

	/* check lock state */
	FbLockState lock_status = fastboot_get_lock_stat();
	if (lock_status == FASTBOOT_LOCK_ERROR) {
		printf("In boota get fastboot lock status error. Set lock status\n");
		fastboot_set_lock_stat(FASTBOOT_LOCK);
		lock_status = FASTBOOT_LOCK;
	}

	bool allow_fail = (lock_status == FASTBOOT_UNLOCK ? true : false);
	avb_metric = get_timer(0);

	if (gki_is_enabled())
		/* set flag when GKI is enabled, vendor_boot partition will be supported. */
		gki_is_supported = true;
	else {
		requested_partitions_boot[2] = NULL;
	}

	/* For imx6 on Android, we don't have a/b slot and we want to verify boot/recovery with AVB.
	 * For imx8 and Android Things we don't have recovery and support a/b slot for boot */
#ifdef CONFIG_DUAL_BOOTLOADER
	/* We will only verify single one slot which has been selected in SPL */
	avb_result = avb_flow_dual_uboot(&fsl_avb_ab_ops, requested_partitions_boot, allow_fail,
			AVB_HASHTREE_ERROR_MODE_RESTART_AND_INVALIDATE, &avb_out_data);

	/* Reboot if current slot is not bootable. */
	if (avb_result == AVB_AB_FLOW_RESULT_ERROR_NO_BOOTABLE_SLOTS) {
		printf("boota: slot verify fail!\n");
		do_reset(NULL, 0, 0, NULL);
	}
#else /* CONFIG_DUAL_BOOTLOADER */
#ifdef CONFIG_ANDROID_AB_SUPPORT
	/* we can use avb to verify Trusty if we want */
	avb_result = avb_ab_flow_fast(&fsl_avb_ab_ops, requested_partitions_boot, allow_fail,
			AVB_HASHTREE_ERROR_MODE_RESTART_AND_INVALIDATE, &avb_out_data);
#else /* CONFIG_ANDROID_AB_SUPPORT */
	/* For imx6/7 devices. */
	if (is_recovery_mode) {
		avb_result = avb_single_flow(&fsl_avb_ab_ops, requested_partitions_recovery, allow_fail,
				AVB_HASHTREE_ERROR_MODE_RESTART, &avb_out_data);
	} else {
		avb_result = avb_single_flow(&fsl_avb_ab_ops, requested_partitions_boot, allow_fail,
				AVB_HASHTREE_ERROR_MODE_RESTART, &avb_out_data);
	}
#endif /* CONFIG_ANDROID_AB_SUPPORT */
#endif /* CONFIG_DUAL_BOOTLOADER */

	/* get the duration of avb */
	metrics.avb = get_timer(avb_metric);

	/* Parse the avb data */
	if ((avb_result == AVB_AB_FLOW_RESULT_OK) ||
			(avb_result == AVB_AB_FLOW_RESULT_OK_WITH_VERIFICATION_ERROR)) {
		if (avb_out_data == NULL)
			goto fail;
		/* We may have more than one partition loaded by AVB, find the boot partition first.*/
#ifdef CONFIG_SYSTEM_RAMDISK_SUPPORT
		if (find_partition_data_by_name("boot", avb_out_data, &avb_loadpart))
			goto fail;
		if (gki_is_supported && find_partition_data_by_name("vendor_boot", avb_out_data, &avb_vendorboot))
			goto fail;
#else
		if (is_recovery_mode) {
			if (find_partition_data_by_name("recovery", avb_out_data, &avb_loadpart))
				goto fail;
		} else {
			if (find_partition_data_by_name("boot", avb_out_data, &avb_loadpart))
				goto fail;
		}
#endif

		assert(avb_loadpart != NULL);

		/* boot image is already read by avb */
		if (gki_is_supported) {
			assert(avb_vendorboot != NULL);
			hdr_v3 = (struct boot_img_hdr_v3 *)avb_loadpart->data;
			vendor_boot_hdr = (struct vendor_boot_img_hdr_v3 *)avb_vendorboot->data;
			if (android_image_check_header_v3(hdr_v3, vendor_boot_hdr)) {
				printf("boota: bad boot/vendor_boot image magic\n");
				goto fail;
			}
		} else {
			hdr = (struct andr_img_hdr *)avb_loadpart->data;
			if (android_image_check_header(hdr)) {
				printf("boota: bad boot image magic\n");
				goto fail;
			}
		}

		if (avb_result == AVB_AB_FLOW_RESULT_OK)
			printf(" verify OK, boot '%s%s'\n",
					avb_loadpart->partition_name, avb_out_data->ab_suffix);
		else {
			printf(" verify FAIL, state: UNLOCK\n");
			printf(" boot '%s%s' still\n",
					avb_loadpart->partition_name, avb_out_data->ab_suffix);
		}
		char bootargs_sec[ANDR_BOOT_EXTRA_ARGS_SIZE];
		if (lock_status == FASTBOOT_LOCK) {
			snprintf(bootargs_sec, sizeof(bootargs_sec),
					"androidboot.verifiedbootstate=green androidboot.flash.locked=1 androidboot.slot_suffix=%s ",
					avb_out_data->ab_suffix);
		} else {
			snprintf(bootargs_sec, sizeof(bootargs_sec),
					"androidboot.verifiedbootstate=orange androidboot.flash.locked=0 androidboot.slot_suffix=%s ",
					avb_out_data->ab_suffix);
		}
		if (avb_out_data->cmdline != NULL)
			strcat(bootargs_sec, avb_out_data->cmdline);
#if defined(CONFIG_ANDROID_DYNAMIC_PARTITION) && defined(CONFIG_SYSTEM_RAMDISK_SUPPORT)
		/* for the condition dynamic partition is used , recovery ramdisk is used to boot
		 * up Android, in this condition, "androidboot.force_normal_boot=1" is needed */
		if(!is_recovery_mode) {
			strcat(bootargs_sec, " androidboot.force_normal_boot=1");
		}
#endif
		env_set("bootargs_sec", bootargs_sec);
#if !defined(CONFIG_ANDROID_DYNAMIC_PARTITION) && defined(CONFIG_SYSTEM_RAMDISK_SUPPORT)
		if(!is_recovery_mode) {
			if(avb_out_data->cmdline != NULL && strstr(avb_out_data->cmdline, "root="))
				fastboot_setup_system_boot_args(avb_out_data->ab_suffix, false);
			else
				fastboot_setup_system_boot_args(avb_out_data->ab_suffix, true);
		}
#endif /* CONFIG_ANDROID_AUTO_SUPPORT */
	} else {
		/* Fall into fastboot mode if get unacceptable error from avb
		 * or verify fail in lock state.
		 */
		if (lock_status == FASTBOOT_LOCK)
			printf(" verify FAIL, state: LOCK\n");

		goto fail;
	}

	/*
	 * Start decompress & load kernel image. If we are using uncompressed kernel image,
	 * copy it directly to physical dram address. If we are using compressed lz4 kernel
	 * image, we need to decompress the kernel image first.
	 */
	if (gki_is_supported) {
		if (image_arm64((void *)((ulong)hdr_v3 + 4096))) {
			memcpy((void *)(long)vendor_boot_hdr->kernel_addr,
					(void *)((ulong)hdr_v3 + 4096), hdr_v3->kernel_size);
		} else if (IS_ENABLED(CONFIG_LZ4)) {
			size_t lz4_len = MAX_KERNEL_LEN;
			if (ulz4fn((void *)((ulong)hdr_v3 + 4096),
						hdr_v3->kernel_size, (void *)(ulong)vendor_boot_hdr->kernel_addr, &lz4_len) != 0) {
				printf("Decompress kernel fail!\n");
				goto fail;
			}
		} else {
			printf("Wrong kernel image! Please check if you need to enable 'CONFIG_LZ4'\n");
			goto fail;
		}
	} else {
#if defined (CONFIG_ARCH_IMX8) || defined (CONFIG_ARCH_IMX8M)
		if (image_arm64((void *)((ulong)hdr + hdr->page_size))) {
			memcpy((void *)(long)hdr->kernel_addr,
					(void *)((ulong)hdr + hdr->page_size), hdr->kernel_size);
		} else if (IS_ENABLED(CONFIG_LZ4)) {
			size_t lz4_len = MAX_KERNEL_LEN;
			if (ulz4fn((void *)((ulong)hdr + hdr->page_size),
						hdr->kernel_size, (void *)(ulong)hdr->kernel_addr, &lz4_len) != 0) {
				printf("Decompress kernel fail!\n");
				goto fail;
			}
		} else {
			printf("Wrong kernel image! Please check if you need to enable 'CONFIG_LZ4'\n");
			goto fail;
		}
#else /* CONFIG_ARCH_IMX8 || CONFIG_ARCH_IMX8M */
		/* copy kernel image and boot header to hdr->kernel_addr - hdr->page_size */
		memcpy((void *)(ulong)(hdr->kernel_addr - hdr->page_size), (void *)hdr,
				hdr->page_size + ALIGN(hdr->kernel_size, hdr->page_size));
#endif /* CONFIG_ARCH_IMX8 || CONFIG_ARCH_IMX8M */
	}

	/*
	 * Start loading ramdisk. */
	/* Load ramdisk except for Android Auto which doesn't support dynamic partition, it will only
	 * load ramdisk in recovery mode.
	 */
	if (gki_is_supported) {
		/* Need to concatenate vendor_boot ramdisk and boot ramdisk, check
		 * "include/android_image.h" for boot/vendor_boot image overlay.
		 */
		memcpy((void *)(ulong)vendor_boot_hdr->ramdisk_addr,
				(void *)(ulong)vendor_boot_hdr + ALIGN(sizeof(struct vendor_boot_img_hdr_v3), vendor_boot_hdr->page_size),
				 vendor_boot_hdr->vendor_ramdisk_size);
		memcpy((void *)(ulong)vendor_boot_hdr->ramdisk_addr + vendor_boot_hdr->vendor_ramdisk_size,
				(void *)(ulong)hdr_v3 + 4096 + ALIGN(hdr_v3->kernel_size, 4096), hdr_v3->ramdisk_size);
	} else {
#if !defined(CONFIG_SYSTEM_RAMDISK_SUPPORT) || defined(CONFIG_ANDROID_DYNAMIC_PARTITION)
		memcpy((void *)(ulong)hdr->ramdisk_addr, (void *)(ulong)hdr + hdr->page_size
				+ ALIGN(hdr->kernel_size, hdr->page_size), hdr->ramdisk_size);
#else
		if (is_recovery_mode)
			memcpy((void *)(ulong)hdr->ramdisk_addr, (void *)(ulong)hdr + hdr->page_size
					+ ALIGN(hdr->kernel_size, hdr->page_size), hdr->ramdisk_size);
#endif
	}

	/* Start loading the dtb file */
	u32 fdt_addr = 0;
	u32 fdt_size = 0;
	struct dt_table_header *dt_img = NULL;

	if (gki_is_supported)
		fdt_addr = (ulong)((ulong)(vendor_boot_hdr->kernel_addr) + MAX_KERNEL_LEN);
	else
		fdt_addr = (ulong)((ulong)(hdr->kernel_addr) + MAX_KERNEL_LEN);

#ifdef CONFIG_SYSTEM_RAMDISK_SUPPORT
	/* It means boot.img(recovery) do not include dtb, it need load dtb from partition */
	if (find_partition_data_by_name("dtbo",
				avb_out_data, &avb_loadpart)) {
		goto fail;
	} else
		dt_img = (struct dt_table_header *)avb_loadpart->data;
#else
	/* recovery.img include dts while boot.img use dtbo */
	if (is_recovery_mode) {
		if (hdr->header_version != 1) {
			printf("boota: boot image header version error!\n");
			goto fail;
		}

		dt_img = (struct dt_table_header *)((void *)(ulong)hdr +
					hdr->page_size +
					ALIGN(hdr->kernel_size, hdr->page_size) +
					ALIGN(hdr->ramdisk_size, hdr->page_size) +
					ALIGN(hdr->second_size, hdr->page_size));
	} else if (find_partition_data_by_name("dtbo",
					avb_out_data, &avb_loadpart)) {
		goto fail;
	} else
		dt_img = (struct dt_table_header *)avb_loadpart->data;
#endif

	if (be32_to_cpu(dt_img->magic) != DT_TABLE_MAGIC) {
		printf("boota: bad dt table magic %08x\n",
				be32_to_cpu(dt_img->magic));
		goto fail;
	} else if (!be32_to_cpu(dt_img->dt_entry_count)) {
		printf("boota: no dt entries\n");
		goto fail;
	}

	struct dt_table_entry *dt_entry;
	dt_entry = (struct dt_table_entry *)((ulong)dt_img +
			be32_to_cpu(dt_img->dt_entries_offset));
	fdt_size = be32_to_cpu(dt_entry->dt_size);
	memcpy((void *)(ulong)fdt_addr, (void *)((ulong)dt_img +
			be32_to_cpu(dt_entry->dt_offset)), fdt_size);

	/* Combine cmdline and Print image info  */
	if (gki_is_supported) {
		check_image_arm64  = image_arm64((void *)(ulong)vendor_boot_hdr->kernel_addr);
		android_image_get_kernel_v3(hdr_v3, vendor_boot_hdr);
		addr = vendor_boot_hdr->kernel_addr;
		printf("kernel   @ %08x (%d)\n", vendor_boot_hdr->kernel_addr, hdr_v3->kernel_size);
		printf("ramdisk  @ %08x (%d)\n", vendor_boot_hdr->ramdisk_addr,
						vendor_boot_hdr->vendor_ramdisk_size + hdr_v3->ramdisk_size);
	} else {
		check_image_arm64  = image_arm64((void *)(ulong)hdr->kernel_addr);
		if (check_image_arm64) {
			android_image_get_kernel(hdr, 0, NULL, NULL);
			addr = hdr->kernel_addr;
		} else {
			addr = (ulong)(hdr->kernel_addr - hdr->page_size);
		}
		printf("kernel   @ %08x (%d)\n", hdr->kernel_addr, hdr->kernel_size);
		printf("ramdisk  @ %08x (%d)\n", hdr->ramdisk_addr, hdr->ramdisk_size);
	}
	if (fdt_size)
		printf("fdt      @ %08x (%d)\n", fdt_addr, fdt_size);

	/* Set boot parameters */
	char boot_addr_start[12];
	char ramdisk_addr[25];
	char fdt_addr_start[12];

	char *boot_args[] = { NULL, boot_addr_start, ramdisk_addr, fdt_addr_start};
	if (check_image_arm64)
		boot_args[0] = "booti";
	else
		boot_args[0] = "bootm";

	sprintf(boot_addr_start, "0x%lx", addr);
	if (gki_is_supported) {
		sprintf(ramdisk_addr, "0x%x:0x%x", vendor_boot_hdr->ramdisk_addr,
						   vendor_boot_hdr->vendor_ramdisk_size + hdr_v3->ramdisk_size);
	} else {
		sprintf(ramdisk_addr, "0x%x:0x%x", hdr->ramdisk_addr, hdr->ramdisk_size);
	}
	sprintf(fdt_addr_start, "0x%x", fdt_addr);

	/* Don't pass ramdisk addr for Android Auto if we are not booting from recovery */
#if !defined(CONFIG_ANDROID_DYNAMIC_PARTITION) && defined(CONFIG_SYSTEM_RAMDISK_SUPPORT)
	if (!is_recovery_mode)
		boot_args[2] = NULL;
#endif

	/* Show orange warning for unlocked device, press power button to skip. */
#ifdef CONFIG_AVB_WARNING_LOGO
	if (fastboot_get_lock_stat() == FASTBOOT_UNLOCK) {
		int count = 0;

		printf("Device is unlocked, press power key to skip warning logo... \n");
		if (display_unlock_warning())
			printf("can't show unlock warning.\n");
		while ( (count < 10 * CONFIG_AVB_WARNING_TIME_LAST) && !is_power_key_pressed()) {
			mdelay(100);
			count++;
		}
	}
#endif

	/* Trusty related operations */
#ifdef CONFIG_IMX_TRUSTY_OS
	/* Trusty keymaster needs some parameters before it work */
	uint32_t os_version;
	if (gki_is_supported)
		os_version = hdr_v3->os_version;
	else
		os_version = hdr->os_version;
	if (trusty_setbootparameter(os_version, avb_result, avb_out_data))
		goto fail;

	/* lock the boot status and rollback_idx preventing Linux modify it */
	trusty_lock_boot_state();
	/* lock the boot state so linux can't use some hwcrypto commands. */
	hwcrypto_lock_boot_state();
	/* put ql-tipc to release resource for Linux */
	trusty_ipc_shutdown();
#endif

	/* Free AVB data */
	if (avb_out_data != NULL)
		avb_slot_verify_data_free(avb_out_data);

	/* Images are loaded, start to boot. */
	if (check_image_arm64) {
#ifdef CONFIG_CMD_BOOTI
		do_booti(NULL, 0, 4, boot_args);
#else
		debug("please enable CONFIG_CMD_BOOTI when kernel are Image");
#endif
	} else {
		do_bootm(NULL, 0, 4, boot_args);
	}

	/* This only happens if image is somehow faulty so we start over */
	do_reset(NULL, 0, 0, NULL);

	return 1;

fail:
	/* avb has no recovery */
	if (avb_out_data != NULL)
		avb_slot_verify_data_free(avb_out_data);

	return run_command("fastboot 0", 0);
}

U_BOOT_CMD(
	boota,	2,	1,	do_boota,
	"boota   - boot android bootimg \n",
	"boot from current mmc with avb verify\n"
);

#else /* CONFIG_AVB_SUPPORT */
/* boota <addr> [ mmc0 | mmc1 [ <partition> ] ] */
int do_boota(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong addr = 0;
	char *ptn = "boot";
	int mmcc = -1;
	struct andr_img_hdr *hdr = &boothdr;
	ulong image_size;
	bool check_image_arm64 =  false;
	int i = 0;

	for (i = 0; i < argc; i++)
		printf("%s ", argv[i]);
	printf("\n");

	if (argc < 2)
		return -1;

	mmcc = simple_strtoul(argv[1]+3, NULL, 10);

	if (argc > 2)
		ptn = argv[2];

	if (mmcc != -1) {
#ifdef CONFIG_MMC
		struct fastboot_ptentry *pte;
		struct mmc *mmc;
		disk_partition_t info;
		struct blk_desc *dev_desc = NULL;
		unsigned bootimg_sectors;

		memset((void *)&info, 0 , sizeof(disk_partition_t));
		/* i.MX use MBR as partition table, so this will have
		   to find the start block and length for the
		   partition name and register the fastboot pte we
		   define the partition number of each partition in
		   config file
		 */
		mmc = find_mmc_device(mmcc);
		if (!mmc) {
			printf("boota: cannot find '%d' mmc device\n", mmcc);
			goto fail;
		}
		dev_desc = blk_get_dev("mmc", mmcc);
		if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
			printf("** Block device MMC %d not supported\n", mmcc);
			goto fail;
		}

		/* below was i.MX mmc operation code */
		if (mmc_init(mmc)) {
			printf("mmc%d init failed\n", mmcc);
			goto fail;
		}

		pte = fastboot_flash_find_ptn(ptn);
		if (!pte) {
			printf("boota: cannot find '%s' partition\n", ptn);
			fastboot_flash_dump_ptn();
			goto fail;
		}

		if (blk_dread(dev_desc, pte->start,
					      1, (void *)hdr) < 0) {
			printf("boota: mmc failed to read bootimg header\n");
			goto fail;
		}

		if (android_image_check_header(hdr)) {
			printf("boota: bad boot image magic\n");
			goto fail;
		}

		image_size = android_image_get_end(hdr) - (ulong)hdr;
		bootimg_sectors = image_size/512;

		if (blk_dread(dev_desc,	pte->start,
					bootimg_sectors,
					(void *)(hdr->kernel_addr - hdr->page_size)) < 0) {
			printf("boota: mmc failed to read bootimage\n");
			goto fail;
		}
		check_image_arm64  = image_arm64((void *)hdr->kernel_addr);
#if defined(CONFIG_FASTBOOT_LOCK)
		int verifyresult = -1;
#endif

#if defined(CONFIG_FASTBOOT_LOCK)
		int lock_status = fastboot_get_lock_stat();
		if (lock_status == FASTBOOT_LOCK_ERROR) {
			printf("In boota get fastboot lock status error. Set lock status\n");
			fastboot_set_lock_stat(FASTBOOT_LOCK);
		}
		display_lock(fastboot_get_lock_stat(), verifyresult);
#endif
		/* load the ramdisk file */
		memcpy((void *)hdr->ramdisk_addr, (void *)hdr->kernel_addr
			+ ALIGN(hdr->kernel_size, hdr->page_size), hdr->ramdisk_size);

#ifdef CONFIG_OF_LIBFDT
		u32 fdt_size = 0;
		/* load the dtb file */
		if (hdr->second_addr) {
			u32 zimage_size = ((u32 *)hdrload->kernel_addr)[ZIMAGE_END_ADDR]
					- ((u32 *)hdrload->kernel_addr)[ZIMAGE_START_ADDR];
			fdt_size = hdrload->kernel_size - zimage_size;
			memcpy((void *)(ulong)hdrload->second_addr,
					(void*)(ulong)hdrload->kernel_addr + zimage_size, fdt_size);
		}
#endif /*CONFIG_OF_LIBFDT*/

#else /*! CONFIG_MMC*/
		return -1;
#endif /*! CONFIG_MMC*/
	} else {
		printf("boota: parameters is invalid. only support mmcX device\n");
		return -1;
	}

	printf("kernel   @ %08x (%d)\n", hdr->kernel_addr, hdr->kernel_size);
	printf("ramdisk  @ %08x (%d)\n", hdr->ramdisk_addr, hdr->ramdisk_size);
#ifdef CONFIG_OF_LIBFDT
	if (fdt_size)
		printf("fdt      @ %08x (%d)\n", hdr->second_addr, fdt_size);
#endif /*CONFIG_OF_LIBFDT*/


	char boot_addr_start[12];
	char ramdisk_addr[25];
	char fdt_addr[12];
	char *boot_args[] = { NULL, boot_addr_start, ramdisk_addr, fdt_addr};
	if (check_image_arm64 ) {
		addr = hdr->kernel_addr;
		boot_args[0] = "booti";
	} else {
		addr = hdr->kernel_addr - hdr->page_size;
		boot_args[0] = "bootm";
	}

	sprintf(boot_addr_start, "0x%lx", addr);
	sprintf(ramdisk_addr, "0x%x:0x%x", hdr->ramdisk_addr, hdr->ramdisk_size);
	sprintf(fdt_addr, "0x%x", hdr->second_addr);
	if (check_image_arm64) {
		android_image_get_kernel(hdr, 0, NULL, NULL);
#ifdef CONFIG_CMD_BOOTI
		do_booti(NULL, 0, 4, boot_args);
#else
		debug("please enable CONFIG_CMD_BOOTI when kernel are Image");
#endif
	} else {
		do_bootm(NULL, 0, 4, boot_args);
	}
	/* This only happens if image is somehow faulty so we start over */
	do_reset(NULL, 0, 0, NULL);

	return 1;

fail:
#if defined(CONFIG_FSL_FASTBOOT)
	return run_command("fastboot 0", 0);
#else /*! CONFIG_FSL_FASTBOOT*/
	return -1;
#endif /*! CONFIG_FSL_FASTBOOT*/
}

U_BOOT_CMD(
	boota,	3,	1,	do_boota,
	"boota   - boot android bootimg from memory\n",
	"[<addr> | mmc0 | mmc1 | mmc2 | mmcX] [<partition>]\n    "
	"- boot application image stored in memory or mmc\n"
	"\t'addr' should be the address of boot image "
	"which is zImage+ramdisk.img\n"
	"\t'mmcX' is the mmc device you store your boot.img, "
	"which will read the boot.img from 1M offset('/boot' partition)\n"
	"\t 'partition' (optional) is the partition id of your device, "
	"if no partition give, will going to 'boot' partition\n"
);
#endif /* CONFIG_AVB_SUPPORT */
#endif	/* CONFIG_CMD_BOOTA */
