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

#include "fb_fsl_common.h"

static lbaint_t mmc_sparse_write(struct sparse_storage *info,
		lbaint_t blk, lbaint_t blkcnt, const void *buffer)
{
#define SPARSE_FILL_BUF_SIZE (2 * 1024 * 1024)


	struct blk_desc *dev_desc = (struct blk_desc *)info->priv;
	ulong ret = 0;
	void *data;
	int fill_buf_num_blks, cnt;

	if ((unsigned long)buffer & (CONFIG_SYS_CACHELINE_SIZE - 1)) {

		fill_buf_num_blks = SPARSE_FILL_BUF_SIZE / info->blksz;

		data = memalign(CONFIG_SYS_CACHELINE_SIZE, fill_buf_num_blks * info->blksz);

		while (blkcnt) {

			if (blkcnt > fill_buf_num_blks)
				cnt = fill_buf_num_blks;
			else
				cnt = blkcnt;

			memcpy(data, buffer, cnt * info->blksz);

			ret += blk_dwrite(dev_desc, blk, cnt, data);

			blk += cnt;
			blkcnt -= cnt;
			buffer = (void *)((unsigned long)buffer + cnt * info->blksz);

		}

		free(data);
	} else {
		ret = blk_dwrite(dev_desc, blk, blkcnt, buffer);
	}

	return ret;
}

static lbaint_t mmc_sparse_reserve(struct sparse_storage *info,
		lbaint_t blk, lbaint_t blkcnt)
{
	return blkcnt;
}

int write_backup_gpt(void *download_buffer)
{
	int mmc_no = 0;
	struct mmc *mmc;
	struct blk_desc *dev_desc;

	mmc_no = fastboot_devinfo.dev_id;
	mmc = find_mmc_device(mmc_no);
	if (mmc == NULL) {
		printf("invalid mmc device\n");
		return -1;
	}
	dev_desc = blk_get_dev("mmc", mmc_no);
	if (dev_desc == NULL) {
		printf("Can't get Block device MMC %d\n",
			mmc_no);
		return -ENODEV;
	}

	/* write backup get partition */
	if (write_backup_gpt_partitions(dev_desc, download_buffer)) {
		printf("writing GPT image fail\n");
		return -1;
	}

	printf("flash backup gpt image successfully\n");
	return 0;
}

static int get_fastboot_target_dev(char *mmc_dev, struct fastboot_ptentry *ptn)
{
	int dev = 0;
	struct mmc *target_mmc;

	/* Support flash bootloader to mmc 'target_ubootdev' devices, if the
	* 'target_ubootdev' env is not set just flash bootloader to current
	* mmc device.
	*/
	if ((!strncmp(ptn->name, FASTBOOT_PARTITION_BOOTLOADER,
					sizeof(FASTBOOT_PARTITION_BOOTLOADER))) &&
					(env_get("target_ubootdev"))) {
		dev = simple_strtoul(env_get("target_ubootdev"), NULL, 10);

		/* if target_ubootdev is set, it must be that users want to change
		 * fastboot device, then fastboot environment need to be updated */
		fastboot_setup();

		target_mmc = find_mmc_device(dev);
		if ((target_mmc == NULL) || mmc_init(target_mmc)) {
			printf("MMC card init failed!\n");
			return -1;
		} else {
			printf("Flash target is mmc%d\n", dev);
			if (target_mmc->part_config != MMCPART_NOAVAILABLE)
				sprintf(mmc_dev, "mmc dev %x %x", dev, /*slot no*/
						FASTBOOT_MMC_BOOT_PARTITION_ID/*part no*/);
			else
				sprintf(mmc_dev, "mmc dev %x", dev);
			}
	} else if (ptn->partition_id != FASTBOOT_MMC_NONE_PARTITION_ID)
		sprintf(mmc_dev, "mmc dev %x %x",
				fastboot_devinfo.dev_id, /*slot no*/
				ptn->partition_id /*part no*/);
	else
		sprintf(mmc_dev, "mmc dev %x",
				fastboot_devinfo.dev_id /*slot no*/);
	return 0;
}

static void process_flash_blkdev(const char *cmdbuf, void *download_buffer,
			      u32 download_bytes, char *response)
{
	if (download_bytes) {
		struct fastboot_ptentry *ptn;

		/* Next is the partition name */
		ptn = fastboot_flash_find_ptn(cmdbuf);
		if (ptn == NULL) {
			fastboot_fail("partition does not exist", response);
			fastboot_flash_dump_ptn();
		} else if ((download_bytes >
			   ptn->length * MMC_SATA_BLOCK_SIZE) &&
				!(ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_ENV)) {
			printf("Image too large for the partition\n");
			fastboot_fail("image too large for partition", response);
		} else {
			unsigned int temp;

			char blk_dev[128];
			char blk_write[128];
			int blkret;

			printf("writing to partition '%s'\n", ptn->name);
			/* Get target flash device. */
			if (get_fastboot_target_dev(blk_dev, ptn) != 0)
				return;

			if (!fastboot_parts_is_raw(ptn) &&
				is_sparse_image(download_buffer)) {
				int dev_no = 0;
				struct mmc *mmc;
				struct blk_desc *dev_desc;
				disk_partition_t info;
				struct sparse_storage sparse;
				int err;

				dev_no = fastboot_devinfo.dev_id;

				printf("sparse flash target is %s:%d\n",
				       fastboot_devinfo.type == DEV_SATA ? "scsi" : "mmc",
				       dev_no);
				if (fastboot_devinfo.type == DEV_MMC) {
					mmc = find_mmc_device(dev_no);
					if (mmc && mmc_init(mmc))
						printf("MMC card init failed!\n");
				}

				dev_desc = blk_get_dev(fastboot_devinfo.type == DEV_SATA ? "scsi" : "mmc", dev_no);
				if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
					printf("** Block device %s %d not supported\n",
					       fastboot_devinfo.type == DEV_SATA ? "scsi" : "mmc",
					       dev_no);
					return;
				}

				if( strncmp(ptn->name, FASTBOOT_PARTITION_ALL,
					    strlen(FASTBOOT_PARTITION_ALL)) == 0) {
					info.blksz = dev_desc->blksz;
					info.size = dev_desc->lba;
					info.start = 0;
				} else {

					if (part_get_info(dev_desc,
							  ptn->partition_index, &info)) {
						printf("Bad partition index:%d for partition:%s\n",
						ptn->partition_index, ptn->name);
						return;
					}
				}
				printf("writing to partition '%s' for sparse, buffer size %d\n",
						ptn->name, download_bytes);

				sparse.blksz = info.blksz;
				sparse.start = info.start;
				sparse.size = info.size;
				sparse.write = mmc_sparse_write;
				sparse.reserve = mmc_sparse_reserve;
				sparse.mssg = fastboot_fail;
				printf("Flashing sparse image at offset " LBAFU "\n",
				       sparse.start);

				sparse.priv = dev_desc;
				err = write_sparse_image(&sparse, ptn->name, download_buffer,
						   response);

				if (!err)
					fastboot_okay(NULL, response);
			} else {
				/* Will flash images in below case:
				 * 1. Is not gpt partition.
				 * 2. Is gpt partition but no overlay detected.
				 * */
				if (strncmp(ptn->name, "gpt", 3) || !bootloader_gpt_overlay()) {
					/* block count */
					if (strncmp(ptn->name, "gpt", 3) == 0) {
						temp = (ANDROID_GPT_END +
								MMC_SATA_BLOCK_SIZE - 1) /
								MMC_SATA_BLOCK_SIZE;
					} else {
						temp = (download_bytes +
								MMC_SATA_BLOCK_SIZE - 1) /
								MMC_SATA_BLOCK_SIZE;
					}

					sprintf(blk_write, "%s write 0x%x 0x%x 0x%x",
						fastboot_devinfo.type == DEV_SATA ? "scsi" : "mmc",
						(unsigned int)(uintptr_t)download_buffer, /*source*/
						ptn->start, /*dest*/
						temp /*length*/);

					printf("Initializing '%s'\n", ptn->name);

					blkret = run_command(blk_dev, 0);
					if (blkret)
						fastboot_fail("Init of BLK device failed", response);
					else
						fastboot_okay(NULL, response);

					printf("Writing '%s'\n", ptn->name);
					if (run_command(blk_write, 0)) {
						printf("Writing '%s' FAILED!\n", ptn->name);
						fastboot_fail("Write partition failed", response);
					} else {
						printf("Writing '%s' DONE!\n", ptn->name);
						fastboot_okay(NULL, response);
					}
				}
				/* Write backup gpt image */
				if (strncmp(ptn->name, "gpt", 3) == 0) {
					if (write_backup_gpt(download_buffer))
						fastboot_fail("write backup GPT image fail", response);
					else
						fastboot_okay(NULL, response);

					/* will force scan the device,
					 * so dev_desc can be re-inited
					 * with the latest data */
					run_command(blk_dev, 0);
				}
			}
		}
	} else {
		fastboot_fail("no image downloaded", response);
	}
}

static void process_erase_blkdev(const char *cmdbuf, char *response)
{
	int mmc_no = 0;
	char blk_dev[128];
	lbaint_t blks, blks_start, blks_size, grp_size;
	struct mmc *mmc;
	struct blk_desc *dev_desc;
	struct fastboot_ptentry *ptn;
	disk_partition_t info;

	ptn = fastboot_flash_find_ptn(cmdbuf);
	if ((ptn == NULL) || (ptn->flags & FASTBOOT_PTENTRY_FLAGS_UNERASEABLE)) {
		fastboot_fail("partition does not exist or uneraseable", response);
		fastboot_flash_dump_ptn();
		return;
	}

	if (fastboot_devinfo.type == DEV_SATA) {
		printf("Not support erase on SATA\n");
		return;
	}

	mmc_no = fastboot_devinfo.dev_id;
	printf("erase target is MMC:%d\n", mmc_no);

	mmc = find_mmc_device(mmc_no);
	if ((mmc == NULL) || mmc_init(mmc)) {
		printf("MMC card init failed!\n");
		return;
	}

	dev_desc = blk_get_dev("mmc", mmc_no);
	if (NULL == dev_desc) {
		printf("Block device MMC %d not supported\n",
			mmc_no);
		fastboot_fail("not valid MMC card", response);
		return;
	}

	/* Get and switch target flash device. */
	if (get_fastboot_target_dev(blk_dev, ptn) != 0) {
		printf("failed to get target dev!\n");
		return;
	} else if (run_command(blk_dev, 0)) {
		printf("Init of BLK device failed\n");
		return;
	}

	if (part_get_info(dev_desc,
				ptn->partition_index, &info)) {
		printf("Bad partition index:%d for partition:%s\n",
		ptn->partition_index, ptn->name);
		fastboot_fail("erasing of MMC card", response);
		return;
	}

	/* Align blocks to erase group size to avoid erasing other partitions */
	grp_size = mmc->erase_grp_size;
	blks_start = (info.start + grp_size - 1) & ~(grp_size - 1);
	if (info.size >= grp_size)
		blks_size = (info.size - (blks_start - info.start)) &
				(~(grp_size - 1));
	else
		blks_size = 0;

	printf("Erasing blocks " LBAFU " to " LBAFU " due to alignment\n",
	       blks_start, blks_start + blks_size);

	blks = blk_derase(dev_desc, blks_start, blks_size);
	if (blks != blks_size) {
		printf("failed erasing from device %d", dev_desc->devnum);
		fastboot_fail("erasing of MMC card", response);
		return;
	}

	printf("........ erased " LBAFU " bytes from '%s'\n",
	       blks_size * info.blksz, cmdbuf);
	fastboot_okay(NULL, response);

    return;
}

#ifdef CONFIG_FLASH_MCUFIRMWARE_SUPPORT
static void process_flash_sf(const char *cmdbuf, void *download_buffer,
			      u32 download_bytes, char *response)
{
	int blksz = 0;
	blksz = get_block_size();

	if (download_bytes) {
		struct fastboot_ptentry *ptn;
		ptn = fastboot_flash_find_ptn(cmdbuf);
		if (ptn == 0) {
			fastboot_fail("partition does not exist", response);
			fastboot_flash_dump_ptn();
		} else if ((download_bytes > ptn->length * blksz)) {
			fastboot_fail("image too large for partition", response);
		/* TODO : Improve check for yaffs write */
		} else {
			int ret;
			char sf_command[128];
			/* Normal case */
			/* Probe device */
			sprintf(sf_command, "sf probe");
			ret = run_command(sf_command, 0);
			if (ret){
				fastboot_fail("Probe sf failed", response);
				return;
			}
			/* Erase */
			sprintf(sf_command, "sf erase 0x%x 0x%lx", ptn->start * blksz, /*start*/
			ptn->length * blksz /*size*/);
			ret = run_command(sf_command, 0);
			if (ret) {
				fastboot_fail("Erasing sf failed", response);
				return;
			}
			/* Write image */
			sprintf(sf_command, "sf write 0x%x 0x%x 0x%x",
					(unsigned int)(ulong)download_buffer, /* source */
					ptn->start * blksz, /* start */
					download_bytes /*size*/);
			printf("sf write '%s'\n", ptn->name);
			ret = run_command(sf_command, 0);
			if (ret){
				fastboot_fail("Writing sf failed", response);
				return;
			}
			printf("sf write finished '%s'\n", ptn->name);
			fastboot_okay(NULL, response);
		}
	} else {
		fastboot_fail("no image downloaded", response);
	}
}

#ifdef CONFIG_ARCH_IMX8M
/* Check if the mcu image is built for running from TCM */
static bool is_tcm_image(unsigned char *image_addr)
{
	u32 stack;

	stack = *(u32 *)image_addr;

	if ((stack != (u32)ANDROID_MCU_FIRMWARE_HEADER_STACK)) {
		printf("Please flash mcu firmware images for running from TCM\n");
		return false;
	} else
		return true;
}
#endif
#endif

void fastboot_process_erase(const char *cmdbuf, char *response)
{
	switch (fastboot_devinfo.type) {
	case DEV_SATA:
	case DEV_MMC:
		process_erase_blkdev(cmdbuf, response);
		break;
	default:
		printf("Not support flash command for current device %d\n",
			fastboot_devinfo.type);
		fastboot_fail("failed to flash device", response);
		break;
	}
}

void fastboot_process_flash(const char *cmdbuf, void *download_buffer,
			      u32 download_bytes, char *response)
{
/* Check if we need to flash mcu firmware */
#ifdef CONFIG_FLASH_MCUFIRMWARE_SUPPORT
	if (!strncmp(cmdbuf, FASTBOOT_MCU_FIRMWARE_PARTITION,
				sizeof(FASTBOOT_MCU_FIRMWARE_PARTITION))) {
		switch (fastboot_firmwareinfo.type) {
		case DEV_SF:
			process_flash_sf(cmdbuf, download_buffer,
				download_bytes, response);
			break;
#ifdef CONFIG_ARCH_IMX8M
		case DEV_MMC:
			if (is_tcm_image(download_buffer))
				process_flash_blkdev(cmdbuf, download_buffer,
					download_bytes, response);
			break;
#endif
		default:
			printf("Don't support flash firmware\n");
		}
		return;
	}
#endif
	/* Normal case */
	switch (fastboot_devinfo.type) {
	case DEV_SATA:
	case DEV_MMC:
		process_flash_blkdev(cmdbuf, download_buffer,
			download_bytes, response);
		break;
	default:
		printf("Not support flash command for current device %d\n",
			fastboot_devinfo.type);
		fastboot_fail("failed to flash device", response);
		break;
	}
}

/* erase a partition on mmc */
void process_erase_mmc(const char *cmdbuf, char *response)
{
	int mmc_no = 0;
	lbaint_t blks, blks_start, blks_size, grp_size;
	struct mmc *mmc;
	struct blk_desc *dev_desc;
	struct fastboot_ptentry *ptn;
	disk_partition_t info;

	ptn = fastboot_flash_find_ptn(cmdbuf);
	if ((ptn == NULL) || (ptn->flags & FASTBOOT_PTENTRY_FLAGS_UNERASEABLE)) {
		sprintf(response, "FAILpartition does not exist or uneraseable");
		fastboot_flash_dump_ptn();
		return;
	}

	mmc_no = fastboot_devinfo.dev_id;
	printf("erase target is MMC:%d\n", mmc_no);

	mmc = find_mmc_device(mmc_no);
	if ((mmc == NULL) || mmc_init(mmc)) {
		printf("MMC card init failed!\n");
		return;
	}

	dev_desc = blk_get_dev("mmc", mmc_no);
	if (NULL == dev_desc) {
		printf("Block device MMC %d not supported\n",
			mmc_no);
		sprintf(response, "FAILnot valid MMC card");
		return;
	}

	if (part_get_info(dev_desc,
				ptn->partition_index, &info)) {
		printf("Bad partition index:%d for partition:%s\n",
		ptn->partition_index, ptn->name);
		sprintf(response, "FAILerasing of MMC card");
		return;
	}

	/* Align blocks to erase group size to avoid erasing other partitions */
	grp_size = mmc->erase_grp_size;
	blks_start = (info.start + grp_size - 1) & ~(grp_size - 1);
	if (info.size >= grp_size)
		blks_size = (info.size - (blks_start - info.start)) &
				(~(grp_size - 1));
	else
		blks_size = 0;

	printf("Erasing blocks " LBAFU " to " LBAFU " due to alignment\n",
	       blks_start, blks_start + blks_size);

	blks = blk_derase(dev_desc, blks_start, blks_size);
	if (blks != blks_size) {
		printf("failed erasing from device %d", dev_desc->devnum);
		sprintf(response, "FAILerasing of MMC card");
		return;
	}

	printf("........ erased " LBAFU " bytes from '%s'\n",
	       blks_size * info.blksz, cmdbuf);
	sprintf(response, "OKAY");

    return;
}
