// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2018 NXP
 */

#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <dm.h>
#include <mmc.h>
#include <asm/arch/image.h>

static int get_container_size(ulong addr)
{
	struct container_hdr *phdr;
	struct boot_img_t *img_entry;
	struct signature_block_hdr *sign_hdr;
	uint8_t i = 0;
	uint32_t max_offset = 0, img_end;

	phdr = (struct container_hdr *)addr;
	if (phdr->tag != 0x87 && phdr->version != 0x0) {
		debug("Wrong container header\n");
		return -EFAULT;
	}

	max_offset = sizeof(struct container_hdr);

	img_entry = (struct boot_img_t *)(addr + sizeof(struct container_hdr));
	for (i=0; i< phdr->num_images; i++) {
		img_end = img_entry->offset + img_entry->size;
		if (img_end > max_offset)
			max_offset = img_end;

		debug("img[%u], end = 0x%x\n", i, img_end);

		img_entry++;
	}

	if (phdr->sig_blk_offset != 0) {
		sign_hdr = (struct signature_block_hdr *)(addr + phdr->sig_blk_offset);
		uint16_t len = sign_hdr->length_lsb + (sign_hdr->length_msb << 8);

		if (phdr->sig_blk_offset + len > max_offset)
			max_offset = phdr->sig_blk_offset + len;

		debug("sigblk, end = 0x%x\n", phdr->sig_blk_offset + len);
	}

	return max_offset;
}

static int mmc_get_imageset_end(struct mmc *mmc)
{
	int value_container[2];
	unsigned long count;
	uint8_t *buf = malloc(CONTAINER_HDR_ALIGNMENT);
	if (!buf) {
		printf("Malloc buffer failed\n");
		return -ENOMEM;
	}

	count = blk_dread(mmc_get_blk_desc(mmc), CONTAINER_HDR_MMCSD_OFFSET/mmc->read_bl_len, CONTAINER_HDR_ALIGNMENT/mmc->read_bl_len, buf);
	if (count == 0) {
		printf("Read container image from MMC/SD failed\n");
		return -EIO;
	}

	value_container[0] = get_container_size((ulong)buf);
	if (value_container[0] < 0) {
		printf("Parse seco container failed %d\n", value_container[0]);
		return value_container[0];
	}

	debug("seco container size 0x%x\n", value_container[0]);

	count = blk_dread(mmc_get_blk_desc(mmc), (CONTAINER_HDR_ALIGNMENT + CONTAINER_HDR_MMCSD_OFFSET)/mmc->read_bl_len,
		CONTAINER_HDR_ALIGNMENT/mmc->read_bl_len, buf);
	if (count == 0) {
		printf("Read container image from MMC/SD failed\n");
		return -EIO;
	}

	value_container[1] = get_container_size((ulong)buf);
	if (value_container[1] < 0) {
		debug("Parse scu container image failed %d, only seco container\n", value_container[1]);
		return value_container[0] + CONTAINER_HDR_MMCSD_OFFSET; /* return seco container total size */
	}

	debug("scu container size 0x%x\n", value_container[1]);

	return value_container[1] + (CONTAINER_HDR_ALIGNMENT + CONTAINER_HDR_MMCSD_OFFSET);
}

unsigned long spl_mmc_get_uboot_raw_sector(struct mmc *mmc)
{
	int end;

	end = mmc_get_imageset_end(mmc);
	end = ROUND(end, SZ_1K);

	printf("Load image from MMC/SD 0x%x\n", end);

	return end/mmc->read_bl_len;
}
