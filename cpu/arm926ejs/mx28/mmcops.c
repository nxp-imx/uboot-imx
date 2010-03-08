/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <command.h>
#include <exports.h>
#include <mmc.h>

#ifdef CONFIG_GENERIC_MMC
#define MMCOPS_DEBUG

#define MBR_SIGNATURE		0xaa55
#define MBR_SECTOR_COUNT	(CONFIG_ENV_OFFSET >> 9)
/* 1. RSV partition (env and uImage) */
#define RSVPART_SECTOR_OFFSET	(MBR_SECTOR_COUNT)
#define RSVPART_SECTOR_COUNT	0x3ffe	/* 8 MB */
/* 2. FAT partition */
#define FATPART_FILESYS		0xb
#define FATPART_SECTOR_OFFSET	(RSVPART_SECTOR_OFFSET + RSVPART_SECTOR_COUNT)
#define FATPART_SECTOR_COUNT	0x10000	/* 32 MB (minimal) */
/* 3. EXT partition */
#define EXTPART_FILESYS		0x83
#define EXTPART_SECTOR_COUNT	0xc0000	/* 384 MB */
/* 4. SB partition (uboot.sb or linux.sb) */
#define SBPART_FILESYS		'S'
#define SBPART_SECTOR_COUNT	0x4000	/* 8 MB */
#define CB_SIGNATURE		0x00112233
#define CB_SECTOR_COUNT		2

#define MAX_CYLINDERS		1024
#define MAX_HEADS		256
#define MAX_SECTORS		63

struct partition {
	u8 boot_flag;
	u8 start_head;
	u8 start_sector;
	u8 start_cylinder;
	u8 file_system;
	u8 end_head;
	u8 end_sector;
	u8 end_cylinder;
	u32 sector_offset;
	u32 sector_count;
} __attribute__ ((__packed__));

struct partition_table {
	u8 reserved[446];
	struct partition partitions[4];
	u16 signature;
};

struct drive_info {
	u32 chip_num;
	u32 drive_type;
	u32 drive_tag;
	u32 sector_offset;
	u32 sector_count;
};

struct config_block {
	u32 signature;
	u32 primary_tag;
	u32 secondary_tag;
	u32 num_copies;
	struct drive_info drive_info[2];
};

static int mmc_format(int dev)
{
	int rc = 0;
	u8 *buf = 0;
	u32 i, cnt, total_sectors;
	u32 offset[4];
	struct config_block *cb;
	struct partition_table *mbr;
	struct mmc *mmc = find_mmc_device(dev);

	/* Warning */
	printf("WARN: Data on card will get lost with format.\n"
		"Continue? (y/n)");
	char ch = getc();
	printf("\n");
	if (ch != 'y') {
		rc = -1;
		goto out;
	}

	/* Allocate sector buffer */
	buf = malloc(mmc->read_bl_len);
	if (!buf) {
		printf("%s[%d]: malloc error\n", __func__, __LINE__);
		rc = -1;
		goto out;
	}
	memset(buf, 0, mmc->read_bl_len);

	/* Erase the first sector of each partition */
	cnt = mmc->block_dev.block_read(dev, 0, 1, buf);
	if (cnt != 1) {
		printf("%s[%d]: read mmc error\n", __func__, __LINE__);
		rc = -1;
		goto out;
	}
	mbr = (struct partition_table *)buf;
	if (mbr->signature == MBR_SIGNATURE) {
		/* Get sector offset of each partition */
		for (i = 0; i < 4; i++)
			offset[i] = mbr->partitions[i].sector_offset;
		/* Erase */
		memset(buf, 0, mmc->read_bl_len);
		for (i = 0; i < 4; i++) {
			if (offset[i] > 0) {
				cnt = mmc->block_dev.block_write(dev,
					offset[i], 1, buf);
				if (cnt != 1) {
					printf("%s[%d]: write mmc error\n",
						__func__, __LINE__);
					rc = -1;
					goto out;
				}
			}
		}
	}

	/* Get total sectors */
	total_sectors = mmc->capacity >> 9;
	if (RSVPART_SECTOR_COUNT + SBPART_SECTOR_COUNT > total_sectors) {
		printf("Card capacity is too low to format\n");
		rc = -1;
		goto out;
	}

	/* Write config block */
	cb = (struct config_block *)buf;
	cb->signature = CB_SIGNATURE;
	cb->num_copies = 2;
	cb->primary_tag = 0x1;
	cb->secondary_tag = 0x2;
	cb->drive_info[0].chip_num = 0;
	cb->drive_info[0].drive_type = 0;
	cb->drive_info[0].drive_tag = 0x1;
	cb->drive_info[0].sector_count = SBPART_SECTOR_COUNT - CB_SECTOR_COUNT;
	cb->drive_info[0].sector_offset =
		total_sectors - cb->drive_info[0].sector_count;
	cb->drive_info[1].chip_num = 0;
	cb->drive_info[1].drive_type = 0;
	cb->drive_info[1].drive_tag = 0x2;
	cb->drive_info[1].sector_count = SBPART_SECTOR_COUNT - CB_SECTOR_COUNT;
	cb->drive_info[1].sector_offset =
		total_sectors - cb->drive_info[1].sector_count;

	cnt = mmc->block_dev.block_write(dev,
		total_sectors - SBPART_SECTOR_COUNT, 1, (void *)cb);
	if (cnt != 1) {
		printf("%s[%d]: write mmc error\n", __func__, __LINE__);
		rc = -1;
		goto out;
	}

	/* Prepare for MBR */
	memset(buf, 0, mmc->read_bl_len);
	mbr = (struct partition_table *)buf;

	/* RSV partition */
	mbr->partitions[0].sector_offset = RSVPART_SECTOR_OFFSET;
	mbr->partitions[0].sector_count = RSVPART_SECTOR_COUNT;

	/* SB partition */
	mbr->partitions[3].file_system = SBPART_FILESYS;
	mbr->partitions[3].sector_offset = total_sectors - SBPART_SECTOR_COUNT;
	mbr->partitions[3].sector_count = SBPART_SECTOR_COUNT;

	/* EXT partition */
	if (EXTPART_SECTOR_COUNT + SBPART_SECTOR_COUNT +
		RSVPART_SECTOR_COUNT > total_sectors) {
#ifdef MMCOPS_DEBUG
		printf("No room for EXT partition\n");
#endif
	} else {
		mbr->partitions[2].file_system = EXTPART_FILESYS;
		mbr->partitions[2].sector_offset = total_sectors -
			SBPART_SECTOR_COUNT - EXTPART_SECTOR_COUNT;
		mbr->partitions[2].sector_count = EXTPART_SECTOR_COUNT;
	}

	/* FAT partition */
	if (FATPART_SECTOR_COUNT + MBR_SECTOR_COUNT +
		mbr->partitions[0].sector_count +
		mbr->partitions[2].sector_count +
		mbr->partitions[3].sector_count > total_sectors) {
#ifdef MMCOPS_DEBUG
		printf("No room for FAT partition\n");
#endif
		goto out;
	}
	mbr->partitions[1].file_system = FATPART_FILESYS;
	mbr->partitions[1].sector_offset = FATPART_SECTOR_OFFSET;
	mbr->partitions[1].sector_count = total_sectors - MBR_SECTOR_COUNT -
		mbr->partitions[0].sector_count -
		mbr->partitions[2].sector_count -
		mbr->partitions[3].sector_count;

out:
	if (rc == 0) {
		/* Write MBR */
		mbr->signature = MBR_SIGNATURE;
		cnt = mmc->block_dev.block_write(dev, 0, 1, (void *)mbr);
		if (cnt != 1) {
			printf("%s[%d]: write mmc error\n", __func__, __LINE__);
			rc = -1;
		} else
			printf("Done.\n");
	}

	if (!buf)
		free(buf);
	return rc;
}

static int install_sbimage(int dev, void *addr, u32 size)
{
	int rc = 0;
	u8 *buf = 0;
	u32 cnt, offset, cb_offset, sectors, not_format = 0;
	struct config_block *cb;
	struct partition_table *mbr;
	struct mmc *mmc = find_mmc_device(dev);

	/* Allocate sector buffer */
	buf = malloc(mmc->read_bl_len);
	if (!buf) {
		printf("%s[%d]: malloc error\n", __func__, __LINE__);
		rc = -1;
		goto out;
	}

	/* Check partition */
	offset = 0;
	cnt = mmc->block_dev.block_read(dev, offset, 1, buf);
	if (cnt != 1) {
		printf("%s[%d]: read mmc error\n", __func__, __LINE__);
		rc = -1;
		goto out;
	}
	mbr = (struct partition_table *)buf;
	if ((mbr->signature != MBR_SIGNATURE) ||
		(mbr->partitions[3].file_system != SBPART_FILESYS))
		not_format = 1;

	/* Check config block */
	offset = mbr->partitions[3].sector_offset;
	cb_offset = offset; /* Save for later use */
	cnt = mmc->block_dev.block_read(dev, offset, 1, buf);
	if (cnt != 1) {
		printf("%s[%d]: read mmc error\n", __func__, __LINE__);
		rc = -1;
		goto out;
	}
	cb = (struct config_block *)buf;
	if (cb->signature != CB_SIGNATURE)
		not_format = 1;

	/* Not formatted */
	if (not_format) {
		printf("Card is not formatted yet\n");
		rc = -1;
		goto out;
	}

	/* Calculate sectors of image */
	sectors = size / mmc->read_bl_len;
	if (size % mmc->read_bl_len)
		sectors++;

	/* Write image */
	offset = cb->drive_info[0].sector_offset;
	cnt = mmc->block_dev.block_write(dev, offset, sectors, addr);
	if (cnt != sectors) {
		printf("%s[%d]: write mmc error\n", __func__, __LINE__);
		rc = -1;
		goto out;
	}
	/* Verify */
	cnt = mmc->block_dev.block_read(dev, offset, sectors,
		addr + sectors * mmc->read_bl_len);
	if (cnt != sectors) {
		printf("%s[%d]: read mmc error\n", __func__, __LINE__);
		rc = -1;
		goto out;
	}
	if (memcmp(addr, addr + sectors * mmc->read_bl_len,
		sectors * mmc->read_bl_len)) {
		printf("Verifying sbImage write fails\n");
		rc = -1;
		goto out;
	}

	/* Redundant one */
	offset += sectors;
	cnt = mmc->block_dev.block_write(dev, offset, sectors, addr);
	if (cnt != sectors) {
		printf("%s[%d]: write mmc error\n", __func__, __LINE__);
		rc = -1;
		goto out;
	}
	/* Verify */
	cnt = mmc->block_dev.block_read(dev, offset, sectors,
		addr + sectors * mmc->read_bl_len);
	if (cnt != sectors) {
		printf("%s[%d]: read mmc error\n", __func__, __LINE__);
		rc = -1;
		goto out;
	}
	if (memcmp(addr, addr + sectors * mmc->read_bl_len,
		sectors * mmc->read_bl_len)) {
		printf("Verifying redundant sbImage write fails");
		rc = -1;
		goto out;
	}

	/* Update config block */
	cb->drive_info[0].sector_count = sectors;
	cb->drive_info[1].sector_count = sectors;
	cb->drive_info[1].sector_offset = offset;
	cnt = mmc->block_dev.block_write(dev, cb_offset, 1, (void *)cb);
	if (cnt != 1) {
		printf("%s[%d]: write mmc error\n", __func__, __LINE__);
		rc = -1;
		goto out;
	}

	/* Done */
	printf("Done: %d (%x hex) sectors written at %d (%x hex)\n",
		sectors, sectors, offset - sectors, offset - sectors);

out:
	if (!buf)
		free(buf);
	return rc;
}

static int install_uimage(int dev, void *addr, u32 size)
{
	int rc = 0;
	u8 *buf = 0;
	u32 cnt, offset, sectors;
	struct partition_table *mbr;
	struct mmc *mmc = find_mmc_device(dev);

	/* Calculate sectors of uImage */
	sectors = size / mmc->read_bl_len;
	if (size % mmc->read_bl_len)
		sectors++;

	/* Allocate sector buffer */
	buf = malloc(mmc->read_bl_len);
	if (!buf) {
		printf("%s[%d]: malloc error\n", __func__, __LINE__);
		rc = -1;
		goto out;
	}

	/* Check partition */
	offset = 0;
	cnt = mmc->block_dev.block_read(dev, offset, 1, buf);
	if (cnt != 1) {
		printf("%s[%d]: read mmc error\n", __func__, __LINE__);
		rc = -1;
		goto out;
	}
	mbr = (struct partition_table *)buf;
	if (mbr->signature != MBR_SIGNATURE) {
		printf("No valid partition table\n");
		rc = -1;
		goto out;
	}
	if (mbr->partitions[0].sector_count < sectors) {
		printf("No enough uImage partition room\n");
		rc = -1;
		goto out;
	}

	/* Write uImage */
	offset = mbr->partitions[0].sector_offset + (CONFIG_ENV_SIZE >> 9);
	cnt = mmc->block_dev.block_write(dev, offset, sectors, addr);
	if (cnt != sectors) {
		printf("%s[%d]: write mmc error\n", __func__, __LINE__);
		rc = -1;
		goto out;
	}
	/* Verify */
	cnt = mmc->block_dev.block_read(dev, offset, sectors,
		addr + sectors * mmc->read_bl_len);
	if (cnt != sectors) {
		printf("%s[%d]: read mmc error\n", __func__, __LINE__);
		rc = -1;
		goto out;
	}
	if (memcmp(addr, addr + sectors * mmc->read_bl_len,
		sectors * mmc->read_bl_len)) {
		printf("Verifying uImage write fails");
		rc = -1;
		goto out;
	}

	/* Done */
	printf("Done: %d (%x hex) sectors written at %d (%x hex)\n",
		sectors, sectors, offset, offset);

out:
	if (!buf)
		free(buf);
	return rc;
}

static int install_rootfs(int dev, void *addr, u32 size)
{
	int rc = 0;
	u8 *buf = 0;
	u32 cnt, offset, sectors;
	struct partition_table *mbr;
	struct mmc *mmc = find_mmc_device(dev);

	/* Calculate sectors of rootfs */
	sectors = size / mmc->read_bl_len;
	if (size % mmc->read_bl_len)
		sectors++;

	/* Allocate sector buffer */
	buf = malloc(mmc->read_bl_len);
	if (!buf) {
		printf("%s[%d]: malloc error\n", __func__, __LINE__);
		rc = -1;
		goto out;
	}

	/* Check partition */
	offset = 0;
	cnt = mmc->block_dev.block_read(dev, offset, 1, buf);
	if (cnt != 1) {
		printf("%s[%d]: read mmc error\n", __func__, __LINE__);
		rc = -1;
		goto out;
	}
	mbr = (struct partition_table *)buf;
	if ((mbr->signature != MBR_SIGNATURE) ||
		(mbr->partitions[2].file_system != EXTPART_FILESYS)) {
		printf("No rootfs partition\n");
		rc = -1;
		goto out;
	}
	if (mbr->partitions[2].sector_count < sectors) {
		printf("No enough rootfs partition room\n");
		rc = -1;
		goto out;
	}

	/* Write rootfs */
	offset = mbr->partitions[2].sector_offset;
	cnt = mmc->block_dev.block_write(dev, offset, sectors, addr);
	if (cnt != sectors) {
		printf("%s[%d]: write mmc error\n", __func__, __LINE__);
		rc = -1;
		goto out;
	}

	/* Done */
	printf("Done: %d (%x hex) sectors written at %d (%x hex)\n",
		sectors, sectors, offset, offset);

out:
	if (!buf)
		free(buf);
	return rc;
}

int do_mxs_mmcops(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int dev = 0;
	struct mmc *mmc;

	if (argc < 2)
		goto err_out;

	if (strcmp(argv[1], "format") &&
		strcmp(argv[1], "install"))
		goto err_out;

	if (argc == 2) { /* list */
		print_mmc_devices('\n');
		return 0;
	}

	/* Find and init mmc */
	dev = simple_strtoul(argv[2], NULL, 10);
	mmc = find_mmc_device(dev);
	if (!mmc) {
		printf("%s[%d]: find mmc error\n", __func__, __LINE__);
		return -1;
	}
	if (mmc_init(mmc)) {
		printf("%s[%d]: init mmc error\n", __func__, __LINE__);
		return -1;
	}

	if (!strcmp(argv[1], "format"))
		mmc_format(dev);
	if (argc == 3) /* rescan (mmc_init) */
		return 0;

	if (argc != 6)
		goto err_out;

	if (!strcmp(argv[1], "install")) {
		void *addr = (void *)simple_strtoul(argv[3], NULL, 16);
		u32 size = simple_strtoul(argv[4], NULL, 16);

		if (!strcmp(argv[5], "sbImage"))
			return install_sbimage(dev, addr, size);
		else if (!strcmp(argv[5], "uImage"))
			return install_uimage(dev, addr, size);
		else if (!strcmp(argv[5], "rootfs"))
			return install_rootfs(dev, addr, size);
	}

err_out:
	printf("Usage:\n%s\n", cmdtp->usage);
	return -1;
}

U_BOOT_CMD(
	mxs_mmc, 6, 1, do_mxs_mmcops,
	"MXS specific MMC sub system",
	"mxs_mmc format <device num>\n"
	"mxs_mmc install <device num> addr size sbImage/uImage/rootfs\n");
#endif /* CONFIG_GENERIC_MMC */
