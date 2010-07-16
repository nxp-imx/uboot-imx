/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
 * Terry Lv <r65388@freescale.com>
 *
 * (C) Copyright 2003
 * Kyle Harris, kharris@nexus-tech.net
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
#include <mmc.h>

#ifndef CONFIG_GENERIC_MMC
static int curr_device = -1;

int do_mmc (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int dev;

	if (argc < 2) {
		cmd_usage(cmdtp);
		return 1;
	}

	if (strcmp(argv[1], "init") == 0) {
		if (argc == 2) {
			if (curr_device < 0)
				dev = 1;
			else
				dev = curr_device;
		} else if (argc == 3) {
			dev = (int)simple_strtoul(argv[2], NULL, 10);
		} else {
			cmd_usage(cmdtp);
			return 1;
		}

		if (mmc_legacy_init(dev) != 0) {
			puts("No MMC card found\n");
			return 1;
		}

		curr_device = dev;
		printf("mmc%d is available\n", curr_device);
	} else if (strcmp(argv[1], "device") == 0) {
		if (argc == 2) {
			if (curr_device < 0) {
				puts("No MMC device available\n");
				return 1;
			}
		} else if (argc == 3) {
			dev = (int)simple_strtoul(argv[2], NULL, 10);

#ifdef CONFIG_SYS_MMC_SET_DEV
			if (mmc_set_dev(dev) != 0)
				return 1;
#endif
			curr_device = dev;
		} else {
			cmd_usage(cmdtp);
			return 1;
		}

		printf("mmc%d is current device\n", curr_device);
	} else {
		cmd_usage(cmdtp);
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
	mmc, 3, 1, do_mmc,
	"MMC sub-system",
	"init [dev] - init MMC sub system\n"
	"mmc device [dev] - show or set current device"
);
#else /* !CONFIG_GENERIC_MMC */

#ifdef CONFIG_BOOT_PARTITION_ACCESS
#define MMC_PARTITION_SWITCH(mmc, part, enable_boot) \
	do { \
		if (IS_SD(mmc)) {	\
			if (part > 1)	{\
				printf( \
				"\nError: SD partition can only be 0 or 1\n");\
				return 1;	\
			}	\
			if (sd_switch_partition(mmc, part) < 0) {	\
				if (part > 0) { \
					printf("\nError: Unable to switch SD "\
					"partition\n");\
					return 1;	\
				}	\
			}	\
		} else {	\
			if (mmc_switch_partition(mmc, part, enable_boot) \
				< 0) {	\
				printf("Error: Fail to switch "	\
					"partition to %d\n", part);	\
				return 1;	\
			}	\
		} \
	} while (0)
#endif

static void print_mmcinfo(struct mmc *mmc)
{
	printf("Device: %s\n", mmc->name);
	printf("Manufacturer ID: %x\n", mmc->cid[0] >> 24);
	printf("OEM: %x\n", (mmc->cid[0] >> 8) & 0xffff);
	printf("Name: %c%c%c%c%c \n", mmc->cid[0] & 0xff,
			(mmc->cid[1] >> 24), (mmc->cid[1] >> 16) & 0xff,
			(mmc->cid[1] >> 8) & 0xff, mmc->cid[1] & 0xff);

	printf("Tran Speed: %d\n", mmc->tran_speed);
	printf("Rd Block Len: %d\n", mmc->read_bl_len);

	printf("%s version %d.%d\n", IS_SD(mmc) ? "SD" : "MMC",
			(mmc->version >> 4) & 0xf, mmc->version & 0xf);

	printf("High Capacity: %s\n", mmc->high_capacity ? "Yes" : "No");
	printf("Capacity: %lld\n", mmc->capacity);

#ifdef CONFIG_EMMC_DDR_MODE
	if (mmc->bus_width == EMMC_MODE_4BIT_DDR ||
		mmc->bus_width == EMMC_MODE_8BIT_DDR)
		printf("Bus Width: %d-bit DDR\n", (mmc->bus_width >> 8));
	else
#endif
	printf("Bus Width: %d-bit\n", mmc->bus_width);
#ifdef CONFIG_BOOT_PARTITION_ACCESS
	if (mmc->boot_size_mult == 0) {
		printf("Boot Partition Size: %s\n", "No boot partition available");
	} else {
		printf("Boot Partition Size: %5dKB\n", mmc->boot_size_mult * 128);

		printf("Current Partition for boot: ");
		switch (mmc->boot_config & EXT_CSD_BOOT_PARTITION_ENABLE_MASK) {
		case EXT_CSD_BOOT_PARTITION_DISABLE:
			printf("Not bootable\n");
			break;
		case EXT_CSD_BOOT_PARTITION_PART1:
			printf("Boot partition 1\n");
			break;
		case EXT_CSD_BOOT_PARTITION_PART2:
			printf("Boot partition 2\n");
			break;
		case EXT_CSD_BOOT_PARTITION_USER:
			printf("User area\n");
			break;
		default:
			printf("Unknown\n");
			break;
		}
	}
#endif
}

int do_mmcinfo (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	struct mmc *mmc;
	int dev_num;

	if (argc < 2)
		dev_num = 0;
	else
		dev_num = simple_strtoul(argv[1], NULL, 0);

	mmc = find_mmc_device(dev_num);

	if (mmc) {
		if (mmc_init(mmc))
			puts("MMC card init failed!\n");
		else
			print_mmcinfo(mmc);
	}

	return 0;
}

U_BOOT_CMD(mmcinfo, 2, 0, do_mmcinfo,
	"mmcinfo <dev num>-- display MMC info",
	""
);

int do_mmcops(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int rc = 0;
#ifdef CONFIG_BOOT_PARTITION_ACCESS
	u32 part = 0;
#endif

	switch (argc) {
	case 3:
		if (strcmp(argv[1], "rescan") == 0) {
			int dev = simple_strtoul(argv[2], NULL, 10);
			struct mmc *mmc = find_mmc_device(dev);

			if (!mmc)
				return 1;

			mmc_init(mmc);

			return 0;
		}

	case 0:
	case 1:
	case 4:
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;

	case 2:
		if (!strcmp(argv[1], "list")) {
			print_mmc_devices('\n');
			return 0;
		}
		return 1;
#ifdef CONFIG_BOOT_PARTITION_ACCESS
	case 7: /* Fall through */
		part = simple_strtoul(argv[6], NULL, 10);
#endif
	default: /* at least 5 args */
		if (strcmp(argv[1], "read") == 0) {
			int dev = simple_strtoul(argv[2], NULL, 10);
			void *addr = (void *)simple_strtoul(argv[3], NULL, 16);
			u32 cnt = simple_strtoul(argv[5], NULL, 16);
			u32 n;
			u32 blk = simple_strtoul(argv[4], NULL, 16);

			struct mmc *mmc = find_mmc_device(dev);

			if (!mmc)
				return 1;

#ifdef CONFIG_BOOT_PARTITION_ACCESS
			printf("\nMMC read: dev # %d, block # %d, "
				"count %d partition # %d ... \n",
				dev, blk, cnt, part);
#else
			printf("\nMMC read: dev # %d, block # %d,"
				"count %d ... \n", dev, blk, cnt);
#endif

			mmc_init(mmc);

#ifdef CONFIG_BOOT_PARTITION_ACCESS
			if (((mmc->boot_config &
				EXT_CSD_BOOT_PARTITION_ACCESS_MASK) != part)
				|| IS_SD(mmc)) {
				/*
				 * After mmc_init, we now know whether
				 * this is a eSD/eMMC which support boot
				 * partition
				 */
				MMC_PARTITION_SWITCH(mmc, part, 0);
			}
#endif

			n = mmc->block_dev.block_read(dev, blk, cnt, addr);

			/* flush cache after read */
			flush_cache((ulong)addr, cnt * 512); /* FIXME */

			printf("%d blocks read: %s\n",
				n, (n==cnt) ? "OK" : "ERROR");
			return (n == cnt) ? 0 : 1;
		} else if (strcmp(argv[1], "write") == 0) {
			int dev = simple_strtoul(argv[2], NULL, 10);
			void *addr = (void *)simple_strtoul(argv[3], NULL, 16);
			u32 cnt = simple_strtoul(argv[5], NULL, 16);
			u32 n;

			struct mmc *mmc = find_mmc_device(dev);

			int blk = simple_strtoul(argv[4], NULL, 16);

			if (!mmc)
				return 1;

#ifdef CONFIG_BOOT_PARTITION_ACCESS
			printf("\nMMC write: dev # %d, block # %d, "
				"count %d, partition # %d ... \n",
				dev, blk, cnt, part);
#else
			printf("\nMMC write: dev # %d, block # %d, "
				"count %d ... \n",
				dev, blk, cnt);
#endif

			mmc_init(mmc);

#ifdef CONFIG_BOOT_PARTITION_ACCESS
			if (((mmc->boot_config &
				EXT_CSD_BOOT_PARTITION_ACCESS_MASK) != part)
				|| IS_SD(mmc)) {
				/*
				 * After mmc_init, we now know whether this is a
				 * eSD/eMMC which support boot partition
				 */
				MMC_PARTITION_SWITCH(mmc, part, 1);
			}
#endif

			n = mmc->block_dev.block_write(dev, blk, cnt, addr);

			printf("%d blocks written: %s\n",
				n, (n == cnt) ? "OK" : "ERROR");
			return (n == cnt) ? 0 : 1;
		} else {
			printf("Usage:\n%s\n", cmdtp->usage);
			rc = 1;
		}

		return rc;
	}
}

#ifndef CONFIG_BOOT_PARTITION_ACCESS
U_BOOT_CMD(
	mmc, 6, 1, do_mmcops,
	"MMC sub system",
	"mmc read <device num> addr blk# cnt\n"
	"mmc write <device num> addr blk# cnt\n"
	"mmc rescan <device num>\n"
	"mmc list - lists available devices");
#else
U_BOOT_CMD(
	mmc, 7, 1, do_mmcops,
	"MMC sub system",
	"mmc read <device num> addr blk# cnt [partition]\n"
	"mmc write <device num> addr blk# cnt [partition]\n"
	"mmc rescan <device num>\n"
	"mmc list - lists available devices");
#endif
#endif

