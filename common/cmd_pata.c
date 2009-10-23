/*
 * Copyright (C) 2000-2005, DENX Software Engineering
 *		Wolfgang Denk <wd@denx.de>
 * Copyright (C) Procsys. All rights reserved.
 *		Mushtaq Khan <mushtaq_k@procsys.com>
 *			<mushtaqk_921@yahoo.co.in>
 * (C) Copyright 2008-2009 Freescale Semiconductor, Inc.
 *		Terry Lv <r65388@freescale.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <part.h>
#include <ata.h>
#include <pata.h>

int pata_curr_device = -1;
block_dev_desc_t pata_dev_desc[CONFIG_SYS_ATA_MAX_DEVICE];

int pata_initialize(void)
{
	int rc;
	int i;

	for (i = 0; i < CONFIG_SYS_ATA_MAX_DEVICE; i++) {
		memset(&pata_dev_desc[i], 0, sizeof(struct block_dev_desc));
		pata_dev_desc[i].if_type = IF_TYPE_ATAPI;
		pata_dev_desc[i].dev = i;
		pata_dev_desc[i].part_type = PART_TYPE_UNKNOWN;
		pata_dev_desc[i].type = DEV_TYPE_HARDDISK;
		pata_dev_desc[i].lba = 0;
		pata_dev_desc[i].blksz = 512;
		pata_dev_desc[i].block_read = pata_read;
		pata_dev_desc[i].block_write = pata_write;

		rc = init_pata(i);
		rc = scan_pata(i);
		if ((pata_dev_desc[i].lba > 0) && (pata_dev_desc[i].blksz > 0))
			init_part(&pata_dev_desc[i]);
	}
	pata_curr_device = 0;
	return rc;
}

block_dev_desc_t *pata_get_dev(int dev)
{
	return (dev < CONFIG_SYS_ATA_MAX_DEVICE) ? &pata_dev_desc[dev] : NULL;
}

int do_pata(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int rc = 0;

	if (argc == 2 && strcmp(argv[1], "init") == 0)
		return pata_initialize();

	/* If the user has not yet run `pata init`, do it now */
	if (pata_curr_device == -1)
		if (pata_initialize())
			return 1;

	switch (argc) {
	case 0:
	case 1:
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;
	case 2:
		if (strncmp(argv[1], "inf", 3) == 0) {
			int i;
			putc('\n');
			for (i = 0; i < CONFIG_SYS_ATA_MAX_DEVICE; ++i) {
				if (pata_dev_desc[i].type == DEV_TYPE_UNKNOWN)
					continue;
				printf("PATA device %d: ", i);
				dev_print(&pata_dev_desc[i]);
			}
			return 0;
		} else if (strncmp(argv[1], "dev", 3) == 0) {
			if ((pata_curr_device < 0) || \
					(pata_curr_device >= CONFIG_SYS_ATA_MAX_DEVICE)) {
				puts("\nno PATA devices available\n");
				return 1;
			}
			printf("\nPATA device %d: ", pata_curr_device);
			dev_print(&pata_dev_desc[pata_curr_device]);
			return 0;
		} else if (strncmp(argv[1], "part", 4) == 0) {
			int dev, ok;

			for (ok = 0, dev = 0; dev < CONFIG_SYS_ATA_MAX_DEVICE; ++dev) {
				if (pata_dev_desc[dev].part_type != PART_TYPE_UNKNOWN) {
					++ok;
					if (dev)
						putc('\n');
					print_part(&pata_dev_desc[dev]);
				}
			}
			if (!ok) {
				puts("\nno PATA devices available\n");
				rc++;
			}
			return rc;
		}
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;
	case 3:
		if (strncmp(argv[1], "dev", 3) == 0) {
			int dev = (int)simple_strtoul(argv[2], NULL, 10);

			printf("\nPATA device %d: ", dev);
			if (dev >= CONFIG_SYS_ATA_MAX_DEVICE) {
				puts("unknown device\n");
				return 1;
			}
			dev_print(&pata_dev_desc[dev]);

			if (pata_dev_desc[dev].type == DEV_TYPE_UNKNOWN)
				return 1;

			pata_curr_device = dev;

			puts("... is now current device\n");

			return 0;
		} else if (strncmp(argv[1], "part", 4) == 0) {
			int dev = (int)simple_strtoul(argv[2], NULL, 10);

			if (pata_dev_desc[dev].part_type != PART_TYPE_UNKNOWN) {
				print_part(&pata_dev_desc[dev]);
			} else {
				printf("\nPATA device %d not available\n", dev);
				rc = 1;
			}
			return rc;
		}
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;

	default: /* at least 4 args */
		if (strcmp(argv[1], "read") == 0) {
			ulong addr = simple_strtoul(argv[2], NULL, 16);
			ulong cnt = simple_strtoul(argv[4], NULL, 16);
			ulong n;
			lbaint_t blk = simple_strtoul(argv[3], NULL, 16);

			printf("\nPATA read: device %d block # %ld, count %ld ... ",
				pata_curr_device, blk, cnt);

			n = pata_read(pata_curr_device, blk, cnt, (u32 *)addr);

			/* flush cache after read */
			flush_cache(addr, cnt * pata_dev_desc[pata_curr_device].blksz);

			printf("%ld blocks read: %s\n",
				n, (n == cnt) ? "OK" : "ERROR");
			return (n == cnt) ? 0 : 1;
		} else if (strcmp(argv[1], "write") == 0) {
			ulong addr = simple_strtoul(argv[2], NULL, 16);
			ulong cnt = simple_strtoul(argv[4], NULL, 16);
			ulong n;

			lbaint_t blk = simple_strtoul(argv[3], NULL, 16);

			printf("\nPATA write: device %d block # %ld, count %ld ... ",
				pata_curr_device, blk, cnt);

			n = pata_write(pata_curr_device, blk, cnt, (u32 *)addr);

			printf("%ld blocks written: %s\n",
				n, (n == cnt) ? "OK" : "ERROR");
			return (n == cnt) ? 0 : 1;
		} else {
			printf("Usage:\n%s\n", cmdtp->usage);
			rc = 1;
		}

		return rc;
	}
}

U_BOOT_CMD(
	pata, 5, 1, do_pata,
	"pata	- PATA sub system\n",
	"pata info - show available PATA devices\n"
	"pata device [dev] - show or set current device\n"
	"pata part [dev] - print partition table\n"
	"pata read addr blk# cnt\n"
	"pata write addr blk# cnt\n");
