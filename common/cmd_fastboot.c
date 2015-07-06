/*
 * Copyright 2008 - 2009 (C) Wind River Systems, Inc.
 * Tom Rix <Tom.Rix@windriver.com>
 *
 * Copyright (C) 2010-2015 Freescale Semiconductor, Inc.
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
 *
 * Part of the rx_handler were copied from the Android project.
 * Specifically rx command parsing in the  usb_rx_data_complete
 * function of the file bootable/bootloader/legacy/usbloader/usbloader.c
 *
 * The logical naming of flash comes from the Android project
 * Thse structures and functions that look like fastboot_flash_*
 * They come from bootable/bootloader/legacy/libboot/flash.c
 *
 * This is their Copyright:
 *
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <asm/byteorder.h>
#include <common.h>
#include <command.h>
#include <nand.h>
#include <fastboot.h>
#include <environment.h>
#include <mmc.h>
#include <aboot.h>

#if defined(CONFIG_OF_LIBFDT)
#include <libfdt.h>
#include <fdt_support.h>
#endif
#include <asm/bootm.h>

#ifdef CONFIG_FASTBOOT

int do_booti(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
extern int do_bootm_linux(int flag, int argc,
	char *argv[], bootm_headers_t *images);

/* Forward decl */
static int tx_handler(void);
static int rx_handler(const unsigned char *buffer, unsigned int buffer_size);
static void reset_handler(void);

static struct cmd_fastboot_interface interface = {
	.rx_handler            = rx_handler,
	.reset_handler         = reset_handler,
	.product_name          = NULL,
	.serial_no             = NULL,
	.nand_block_size       = 0,
	.transfer_buffer       = (unsigned char *)0xffffffff,
	.transfer_buffer_size  = 0,
};

static unsigned int download_size;
static unsigned int download_bytes;
static unsigned int download_bytes_unpadded;
static unsigned int download_error;
static unsigned int continue_booting;
static unsigned int upload_size;
static unsigned int upload_bytes;
static unsigned int upload_error;

#define MMC_SATA_BLOCK_SIZE 512

#ifdef CONFIG_FASTBOOT_STORAGE_NAND
static void save_env(struct fastboot_ptentry *ptn,
		     char *var, char *val)
{
#ifdef CONFIG_CMD_NAND_LOCK_UNLOCK
	char lock[128], unlock[128];
#endif

	setenv(var, val);

#ifdef CONFIG_CMD_NAND_LOCK_UNLOCK
	sprintf(lock, "nand lock 0x%x 0x%x", ptn->start, ptn->length);
	sprintf(unlock, "nand unlock 0x%x 0x%x", ptn->start, ptn->length);

	/* This could be a problem is there is an outstanding lock */
	run_command(unlock, 0);
#endif
	saveenv();

#ifdef CONFIG_CMD_NAND_LOCK_UNLOCK
	run_command(lock, 0);
#endif
}

void save_parts_values(struct fastboot_ptentry *ptn,
			      unsigned int offset,
			      unsigned int size)
{
	char var[64], val[32];
#ifdef CONFIG_CMD_NAND_LOCK_UNLOCK
	char lock[128], unlock[128];
	struct fastboot_ptentry *env_ptn;
#endif

	printf("saving it..\n");


	sprintf(var, "%s_nand_offset", ptn->name);
	sprintf(val, "0x%x", offset);

	printf("setenv %s %s\n", var, val);

	setenv(var, val);

	sprintf(var, "%s_nand_size", ptn->name);
	sprintf(val, "0x%x", size);

	printf("setenv %s %s\n", var, val);

	setenv(var, val);

#ifdef CONFIG_CMD_NAND_LOCK_UNLOCK
	/* Warning :
	   The environment is assumed to be in a partition named 'enviroment'.
	   It is very possible that your board stores the enviroment
	   someplace else. */
	env_ptn = fastboot_flash_find_ptn("environment");

	if (env_ptn) {
		sprintf(lock, "nand lock 0x%x 0x%x",
			env_ptn->start, env_ptn->length);
		sprintf(unlock, "nand unlock 0x%x 0x%x",
			env_ptn->start, env_ptn->length);

		run_command(unlock, 0);
	}
#endif
	saveenv();

#ifdef CONFIG_CMD_NAND_LOCK_UNLOCK
	if (env_ptn)
		run_command(lock, 0);
#endif
}

int check_parts_values(struct fastboot_ptentry *ptn)
{
	char var[64];

	sprintf(var, "%s_nand_offset", ptn->name);
	if (!getenv(var))
		return 1;

	sprintf(var, "%s_nand_size", ptn->name);
	if (!getenv(var))
		return 1;

	return 0;
}

static int write_to_ptn(struct fastboot_ptentry *ptn)
{
	int ret = 1;
	char length[32];
	char write_type[32];
	int repeat, repeat_max;

#ifdef CONFIG_CMD_NAND_LOCK_UNLOCK
	char lock[128];
	char unlock[128];
#endif
	char write[128];
	char erase[128];

	printf("flashing '%s'\n", ptn->name);

	/* Which flavor of write to use */
	if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_I)
		sprintf(write_type, "write.i");
#ifdef CONFIG_CMD_NAND_TRIMFFS
	else if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_TRIMFFS)
		sprintf(write_type, "write.trimffs");
#endif
	else
		sprintf(write_type, "write");

	/* Some flashing requires writing the same data in multiple,
	   consecutive flash partitions */
	repeat_max = 1;
	if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_REPEAT_MASK) {
		if (ptn->flags &
		    FASTBOOT_PTENTRY_FLAGS_WRITE_CONTIGUOUS_BLOCK) {
			printf("Warning can not do both 'contiguous block' "
				"and 'repeat' writes for for partition '%s'\n", ptn->name);
			printf("Ignoring repeat flag\n");
		} else {
			repeat_max = ptn->flags &
				FASTBOOT_PTENTRY_FLAGS_REPEAT_MASK;
		}
	}

	/* Unlock the whole partition instead of trying to
	   manage special cases */
	sprintf(length, "0x%x", ptn->length * repeat_max);

	for (repeat = 0; repeat < repeat_max; repeat++) {
#ifdef CONFIG_CMD_NAND_LOCK_UNLOCK
		sprintf(lock, "nand lock 0x%x %s",
			ptn->start + (repeat * ptn->length), length);
		sprintf(unlock, "nand unlock 0x%x %s",
			ptn->start + (repeat * ptn->length), length);
#endif
		sprintf(erase, "nand erase 0x%x %s",
			ptn->start + (repeat * ptn->length), length);

#ifdef CONFIG_CMD_NAND_LOCK_UNLOCK
		run_command(unlock, 0);
#endif
		run_command(erase, 0);

		if ((ptn->flags &
		     FASTBOOT_PTENTRY_FLAGS_WRITE_NEXT_GOOD_BLOCK) &&
		    (ptn->flags &
		     FASTBOOT_PTENTRY_FLAGS_WRITE_CONTIGUOUS_BLOCK)) {
			/* Both can not be true */
			printf("Warning can not do 'next good block' and \
				'contiguous block' for partition '%s'\n",
				ptn->name);
			printf("Ignoring these flags\n");
		} else if (ptn->flags &
			   FASTBOOT_PTENTRY_FLAGS_WRITE_NEXT_GOOD_BLOCK) {
			/* Keep writing until you get a good block
			   transfer_buffer should already be aligned */
			if (interface.nand_block_size) {
				unsigned int blocks = download_bytes /
					interface.nand_block_size;
				unsigned int i = 0;
				unsigned int offset = 0;

				while (i < blocks) {
					/* Check for overflow */
					if (offset >= ptn->length)
						break;

					/* download's address only advance
					   if last write was successful */

					/* nand's address always advances */
					sprintf(write, "nand %s 0x%p 0x%x 0x%x", write_type,
						interface.transfer_buffer +
						(i * interface.nand_block_size),
						ptn->start + (repeat * ptn->length) + offset,
						interface.nand_block_size);

					ret = run_command(write, 0);
					if (ret)
						break;
					else
						i++;

					/* Go to next nand block */
					offset += interface.nand_block_size;
				}
			} else {
				printf("Warning nand block size can not be 0 \
					when using 'next good block' for \
					partition '%s'\n", ptn->name);
				printf("Ignoring write request\n");
			}
		} else if (ptn->flags &
			 FASTBOOT_PTENTRY_FLAGS_WRITE_CONTIGUOUS_BLOCK) {
			/* Keep writing until you get a good block
			   transfer_buffer should already be aligned */
			if (interface.nand_block_size) {
				if (0 == nand_curr_device) {
					nand_info_t *nand;
					unsigned long off;
					unsigned int ok_start;

					nand = &nand_info[nand_curr_device];

					printf("\nDevice %d bad blocks:\n",
					       nand_curr_device);

					/* Initialize the ok_start to the
					   start of the partition
					   Then try to find a block large
					   enough for the download */
					ok_start = ptn->start;

					/* It is assumed that the start and
					   length are multiples of block size */
					for (off = ptn->start;
					     off < ptn->start + ptn->length;
					     off += nand->erasesize) {
						if (nand_block_isbad(nand, off)) {
							/* Reset the ok_start
							   to the next block */
							ok_start = off +
								nand->erasesize;
						}

						/* Check if we have enough
						   blocks */
						if ((ok_start - off) >=
						    download_bytes)
							break;
					}

					/* Check if there is enough space */
					if (ok_start + download_bytes <=
					    ptn->start + ptn->length) {

						sprintf(write, "nand %s 0x%p 0x%x 0x%x", write_type,
							interface.transfer_buffer,
							ok_start,
							download_bytes);

						ret = run_command(write, 0);

						/* Save the results into an
						   environment variable on the
						   format
						   ptn_name + 'offset'
						   ptn_name + 'size'  */
						if (ret) {
							/* failed */
							save_parts_values(ptn, ptn->start, 0);
						} else {
							/* success */
							save_parts_values(ptn, ok_start, download_bytes);
						}
					} else {
						printf("Error could not find enough contiguous space "
							"in partition '%s'\n", ptn->name);
						printf("Ignoring write request\n");
					}
				} else {
					/* TBD : Generalize flash handling */
					printf("Error only handling 1 NAND per board");
					printf("Ignoring write request\n");
				}
			} else {
				printf("Warning nand block size can not be 0 \
					when using 'continuous block' for \
					partition '%s'\n", ptn->name);
				printf("Ignoring write request\n");
			}
		} else {
			/* Normal case */
			sprintf(write, "nand %s 0x%p 0x%x 0x%x", write_type,
							interface.transfer_buffer,
							ptn->start + (repeat * ptn->length),
							download_bytes);
#ifdef CONFIG_CMD_NAND_TRIMFFS
			if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_TRIMFFS) {
				sprintf(write, "nand %s 0x%p 0x%x 0x%x", write_type,
							interface.transfer_buffer,
							ptn->start + (repeat * ptn->length),
							download_bytes_unpadded);
			}
#endif

			ret = run_command(write, 0);

			if (0 == repeat) {
				if (ret) /* failed */
					save_parts_values(ptn, ptn->start, 0);
				else     /* success */
					save_parts_values(ptn, ptn->start,
							  download_bytes);
			}
		}

#ifdef CONFIG_CMD_NAND_LOCK_UNLOCK
		run_command(lock, 0);
#endif

		if (ret)
			break;
	}

	return ret;
}
#else
static void save_env(struct fastboot_ptentry *ptn,
		     char *var, char *val)
{
	setenv(var, val);
	saveenv();
}
#endif

/* When save = 0, just parse.  The input is unchanged
   When save = 1, parse and do the save.  The input is changed */
static int parse_env(void *ptn, char *err_string, int save, int debug)
{
	int ret = 1;
	unsigned int sets = 0;
	unsigned int comment_start = 0;
	char *var = NULL;
	char *var_end = NULL;
	char *val = NULL;
	char *val_end = NULL;
	unsigned int i;

	char *buff = (char *)interface.transfer_buffer;
	unsigned int size = download_bytes_unpadded;

	/* The input does not have to be null terminated.
	   This will cause a problem in the corner case
	   where the last line does not have a new line.
	   Put a null after the end of the input.

	   WARNING : Input buffer is assumed to be bigger
	   than the size of the input */
	if (save)
		buff[size] = 0;

	for (i = 0; i < size; i++) {

		if (NULL == var) {

			/*
			 * Check for comments, comment ok only on
			 * mostly empty lines
			 */
			if (buff[i] == '#')
				comment_start = 1;

			if (comment_start) {
				if  ((buff[i] == '\r') ||
				     (buff[i] == '\n')) {
					comment_start = 0;
				}
			} else {
				if (!((buff[i] == ' ') ||
				      (buff[i] == '\t') ||
				      (buff[i] == '\r') ||
				      (buff[i] == '\n'))) {
					/*
					 * Normal whitespace before the
					 * variable
					 */
					var = &buff[i];
				}
			}

		} else if (((NULL == var_end) || (NULL == val)) &&
			   ((buff[i] == '\r') || (buff[i] == '\n'))) {

			/* This is the case when a variable
			   is unset. */

			if (save) {
				/* Set the var end to null so the
				   normal string routines will work

				   WARNING : This changes the input */
				buff[i] = '\0';

				save_env(ptn, var, val);

				if (debug)
					printf("Unsetting %s\n", var);
			}

			/* Clear the variable so state is parse is back
			   to initial. */
			var = NULL;
			var_end = NULL;
			sets++;
		} else if (NULL == var_end) {
			if ((buff[i] == ' ') ||
			    (buff[i] == '\t'))
				var_end = &buff[i];
		} else if (NULL == val) {
			if (!((buff[i] == ' ') ||
			      (buff[i] == '\t')))
				val = &buff[i];
		} else if (NULL == val_end) {
			if ((buff[i] == '\r') ||
			    (buff[i] == '\n')) {
				/* look for escaped cr or ln */
				if ('\\' == buff[i - 1]) {
					/* check for dos */
					if ((buff[i] == '\r') &&
					    (buff[i+1] == '\n'))
						buff[i + 1] = ' ';
					buff[i - 1] = buff[i] = ' ';
				} else {
					val_end = &buff[i];
				}
			}
		} else {
			sprintf(err_string, "Internal Error");

			if (debug)
				printf("Internal error at %s %d\n",
				       __FILE__, __LINE__);
			return 1;
		}
		/* Check if a var / val pair is ready */
		if (NULL != val_end) {
			if (save) {
				/* Set the end's with nulls so
				   normal string routines will
				   work.

				   WARNING : This changes the input */
				*var_end = '\0';
				*val_end = '\0';

				save_env(ptn, var, val);

				if (debug)
					printf("Setting %s %s\n", var, val);
			}

			/* Clear the variable so state is parse is back
			   to initial. */
			var = NULL;
			var_end = NULL;
			val = NULL;
			val_end = NULL;

			sets++;
		}
	}

	/* Corner case
	   Check for the case that no newline at end of the input */
	if ((NULL != var) &&
	    (NULL == val_end)) {
		if (save) {
			/* case of val / val pair */
			if (var_end)
				*var_end = '\0';
			/* else case handled by setting 0 past
			   the end of buffer.
			   Similar for val_end being null */
			save_env(ptn, var, val);

			if (debug) {
				if (var_end)
					printf("Trailing Setting %s %s\n", var, val);
				else
					printf("Trailing Unsetting %s\n", var);
			}
		}
		sets++;
	}
	/* Did we set anything ? */
	if (0 == sets)
		sprintf(err_string, "No variables set");
	else
		ret = 0;

	return ret;
}

static int saveenv_to_ptn(struct fastboot_ptentry *ptn, char *err_string)
{
	int ret = 1;
	int save = 0;
	int debug = 0;

	/* err_string is only 32 bytes
	   Initialize with a generic error message. */
	sprintf(err_string, "%s", "Unknown Error");

	/* Parse the input twice.
	   Only save to the enviroment if the entire input if correct */
	save = 0;
	if (0 == parse_env(ptn, err_string, save, debug)) {
		save = 1;
		ret = parse_env(ptn, err_string, save, debug);
	}
	return ret;
}


#if defined(CONFIG_FASTBOOT_STORAGE_NAND)

static void process_flash_nand(const char *cmdbuf, char *response)
{
	if (download_bytes) {
		struct fastboot_ptentry *ptn;

		ptn = fastboot_flash_find_ptn(cmdbuf + 6);
		if (ptn == 0) {
			sprintf(response, "FAILpartition does not exist");
		} else if ((download_bytes > ptn->length) &&
			   !(ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_ENV)) {
			sprintf(response, "FAILimage too large for partition");
			/* TODO : Improve check for yaffs write */
		} else {
			/* Check if this is not really a flash write
			   but rather a saveenv */
			if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_ENV) {
				/* Since the response can only be 64 bytes,
				   there is no point in having a large error message. */
				char err_string[32];
				if (saveenv_to_ptn(ptn, &err_string[0])) {
					printf("savenv '%s' failed : %s\n",
						ptn->name, err_string);
					sprintf(response, "FAIL%s", err_string);
				} else {
					printf("partition '%s' saveenv-ed\n", ptn->name);
					sprintf(response, "OKAY");
				}
			} else {
				/* Normal case */
				if (write_to_ptn(ptn)) {
					printf("flashing '%s' failed\n", ptn->name);
					sprintf(response, "FAILfailed to flash partition");
				} else {
					printf("partition '%s' flashed\n", ptn->name);
					sprintf(response, "OKAY");
				}
			}
		}
	} else {
		sprintf(response, "FAILno image downloaded");
	}

}
#endif

#if defined(CONFIG_FASTBOOT_STORAGE_SATA)
static void process_flash_sata(const char *cmdbuf, char *response)
{
	if (download_bytes) {
		struct fastboot_ptentry *ptn;

		/* Next is the partition name */
		ptn = fastboot_flash_find_ptn(cmdbuf + 6);
		if (ptn == 0) {
			printf("Partition:'%s' does not exist\n", ptn->name);
			sprintf(response, "FAILpartition does not exist");
		} else if ((download_bytes >
			   ptn->length * MMC_SATA_BLOCK_SIZE) &&
				!(ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_ENV)) {
			printf("Image too large for the partition\n");
			sprintf(response, "FAILimage too large for partition");
		} else if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_ENV) {
			/* Since the response can only be 64 bytes,
			   there is no point in having a large error message. */
			char err_string[32];
			if (saveenv_to_ptn(ptn, &err_string[0])) {
				printf("savenv '%s' failed : %s\n", ptn->name, err_string);
				sprintf(response, "FAIL%s", err_string);
			} else {
				printf("partition '%s' saveenv-ed\n", ptn->name);
				sprintf(response, "OKAY");
			}
		} else {
			unsigned int temp;
			char sata_write[128];

			/* block count */
			temp = (download_bytes +
				MMC_SATA_BLOCK_SIZE - 1) /
					MMC_SATA_BLOCK_SIZE;

			sprintf(sata_write, "sata write 0x%x 0x%x 0x%x",
				(unsigned int)interface.transfer_buffer,
				ptn->start,
				temp)

			if (run_command(sata_write, 0)) {
				printf("Writing '%s' FAILED!\n",
					 ptn->name);
				sprintf(response,
				       "FAIL: Write partition");
			} else {
				printf("Writing '%s' DONE!\n",
					ptn->name);
				sprintf(response, "OKAY");
			}
		}
	} else {
		sprintf(response, "FAILno image downloaded");
	}

}
#endif

#if defined(CONFIG_FASTBOOT_STORAGE_MMC)
static int is_sparse_partition(struct fastboot_ptentry *ptn)
{
	if (ptn && (!strncmp(ptn->name,
		FASTBOOT_PARTITION_SYSTEM, strlen(FASTBOOT_PARTITION_SYSTEM))
		||  !strncmp(ptn->name,
		FASTBOOT_PARTITION_DATA, strlen(FASTBOOT_PARTITION_DATA)))) {
		printf("support sparse flash partition for %s\n", ptn->name);
		return 1;
	 } else
		 return 0;
}

static void process_flash_mmc(const char *cmdbuf, char *response)
{
	if (download_bytes) {
		struct fastboot_ptentry *ptn;

		/* Next is the partition name */
		ptn = fastboot_flash_find_ptn(cmdbuf + 6);
		if (ptn == 0) {
			printf("Partition:'%s' does not exist\n", ptn->name);
			sprintf(response, "FAILpartition does not exist");
		} else if ((download_bytes >
			   ptn->length * MMC_SATA_BLOCK_SIZE) &&
				!(ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_ENV)) {
			printf("Image too large for the partition\n");
			sprintf(response, "FAILimage too large for partition");
		} else if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_ENV) {
			/* Since the response can only be 64 bytes,
			   there is no point in having a large error message. */
			char err_string[32];
			if (saveenv_to_ptn(ptn, &err_string[0])) {
				printf("savenv '%s' failed : %s\n", ptn->name, err_string);
				sprintf(response, "FAIL%s", err_string);
			} else {
				printf("partition '%s' saveenv-ed\n", ptn->name);
				sprintf(response, "OKAY");
			}
		} else {
			unsigned int temp;

			char mmc_dev[128];
			char mmc_write[128];
			int mmcret;

			printf("writing to partition '%s'\n", ptn->name);

			if (ptn->partition_id != FASTBOOT_MMC_NONE_PARTITION_ID)
				sprintf(mmc_dev, "mmc dev %x %x",
					fastboot_devinfo.dev_id, /*slot no*/
					ptn->partition_id /*part no*/);
			else
				sprintf(mmc_dev, "mmc dev %x",
					fastboot_devinfo.dev_id /*slot no*/);

			if (is_sparse_partition(ptn) &&
				is_sparse_image(interface.transfer_buffer)) {
				int mmc_no = 0;
				struct mmc *mmc;
				block_dev_desc_t *dev_desc;
				disk_partition_t info;
				mmc_no = fastboot_devinfo.dev_id;

				printf("sparse flash target is MMC:%d\n", mmc_no);
				mmc = find_mmc_device(mmc_no);
				if (mmc && mmc_init(mmc))
					printf("MMC card init failed!\n");

				dev_desc = get_dev("mmc", mmc_no);
				if (NULL == dev_desc) {
					printf("** Block device MMC %d not supported\n",
						mmc_no);
					return;
				}

				if (get_partition_info(dev_desc,
			       ptn->partition_index, &info)) {
					printf("Bad partition index:%d for partition:%s\n",
					ptn->partition_index, ptn->name);
					return;
				}

				printf("writing to partition '%s' for sparse, buffer size %d\n",
						ptn->name, download_bytes);
				mmcret = write_sparse_image(dev_desc, &info, ptn->name,
						interface.transfer_buffer, download_bytes);
				if (mmcret)
					sprintf(response, "FAIL: Write partition");
				else
					sprintf(response, "OKAY");
			} else {
				/* block count */
				temp = (download_bytes +
					    MMC_SATA_BLOCK_SIZE - 1) /
					    MMC_SATA_BLOCK_SIZE;

				sprintf(mmc_write, "mmc write 0x%x 0x%x 0x%x",
						(unsigned int)interface.transfer_buffer, /*source*/
						ptn->start, /*dest*/
						temp /*length*/);

				printf("Initializing '%s'\n", ptn->name);

				mmcret = run_command(mmc_dev, 0);
				if (mmcret)
					sprintf(response, "FAIL:Init of MMC card");
				else
					sprintf(response, "OKAY");

				printf("Writing '%s'\n", ptn->name);
				if (run_command(mmc_write, 0)) {
					printf("Writing '%s' FAILED!\n", ptn->name);
					sprintf(response, "FAIL: Write partition");
				} else {
					printf("Writing '%s' DONE!\n", ptn->name);
					sprintf(response, "OKAY");
				}
			}
		}
	} else {
		sprintf(response, "FAILno image downloaded");
	}
}

#endif

static void reset_handler ()
{
	/* If there was a download going on, bail */
	download_size = 0;
	download_bytes = 0;
	download_bytes_unpadded = 0;
	download_error = 0;
	continue_booting = 0;
	upload_size = 0;
	upload_bytes = 0;
	upload_error = 0;
}

static void rx_process_getvar(const char *cmdbuf, char *response)
{
	int temp_len = 0;

	strcpy(response, "OKAY");

	temp_len = strlen("getvar:");
	if (!strcmp(cmdbuf + temp_len, "version")) {
		strcpy(response + 4, FASTBOOT_VERSION);
	} else if (!strcmp(cmdbuf + temp_len,
			     "product")) {
		if (interface.product_name)
			strcpy(response + 4, interface.product_name);

	} else if (!strcmp(cmdbuf + temp_len,
			     "serialno")) {
		if (interface.serial_no)
			strcpy(response + 4, interface.serial_no);

	} else if (!strcmp(cmdbuf + temp_len,
			    "downloadsize")) {
		if (interface.transfer_buffer_size)
			sprintf(response + 4, "0x%x",
				interface.transfer_buffer_size);
	} else {
		fastboot_getvar(cmdbuf + 7, response + 4);
	}
}

static void rx_process_reboot(const char *cmdbuf, char *response)
{
	sprintf(response, "OKAY");
	fastboot_tx_status(response, strlen(response));
	udelay(1000000); /* 1 sec */

	do_reset(NULL, 0, 0, NULL);
}

static int rx_process_erase(const char *cmdbuf, char *response)
{
#if defined(CONFIG_FASTBOOT_STORAGE_NAND)
	struct fastboot_ptentry *ptn;

	ptn = fastboot_flash_find_ptn(cmdbuf + 6);
	if (ptn == 0) {
		sprintf(response, "FAILpartition does not exist");
	} else {
		int status, repeat, repeat_max;

		printf("erasing '%s'\n", ptn->name);

#ifdef CONFIG_CMD_NAND_LOCK_UNLOCK
		char lock[128];
		char unlock[128];
#endif
		char erase[128];

		repeat_max = 1;
		if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_REPEAT_MASK)
			repeat_max = ptn->flags & FASTBOOT_PTENTRY_FLAGS_REPEAT_MASK;

		for (repeat = 0; repeat < repeat_max;
			repeat++) {
#ifdef CONFIG_CMD_NAND_LOCK_UNLOCK
			sprintf(lock, "nand lock 0x%x 0x%x",
				ptn->start + (repeat * ptn->length),
				ptn->length);
			sprintf(unlock, "nand unlock 0x%x 0x%x",
				ptn->start + (repeat * ptn->length),
				ptn->length);
#endif
			sprintf(erase, "nand erase 0x%x 0x%x",
				ptn->start + (repeat * ptn->length),
				ptn->length);

#ifdef CONFIG_CMD_NAND_LOCK_UNLOCK
			run_command(unlock, 0);
#endif
			status = run_command(erase, 0);
#ifdef CONFIG_CMD_NAND_LOCK_UNLOCK
			run_command(lock, 0);
#endif

			if (status)
				break;
		}

		if (status) {
			sprintf(response,
				   "FAILfailed to erase partition");
		} else {
			printf("partition '%s' erased\n", ptn->name);
			sprintf(response, "OKAY");
		}
	}
	return 0;
#else
	printf("Not support erase command for EMMC\n");
	return -1;
#endif

}

static void rx_process_flash(const char *cmdbuf, char *response)
{
	switch (fastboot_devinfo.type) {
#if defined(CONFIG_FASTBOOT_STORAGE_SATA)
	case DEV_SATA:
		process_flash_sata(cmdbuf, response);
		break;
#endif
#if defined(CONFIG_FASTBOOT_STORAGE_MMC)
	case DEV_MMC:
		process_flash_mmc(cmdbuf, response);
		break;
#endif
#if defined(CONFIG_FASTBOOT_STORAGE_NAND)
	case DEV_NAND:
		process_flash_nand(cmdbuf, response);
		break;
#endif
	default:
		printf("Not support flash command for current device %d\n",
			fastboot_devinfo.type);
		sprintf(response,
			   "FAILfailed to flash device");
		break;
	}
}

static void rx_process_reboot_bootloader(const char *cmdbuf, char *response)
{
	sprintf(response, "OKAY");
	fastboot_tx_status(response, strlen(response));
	udelay(1000000);
	fastboot_enable_flag();
	do_reset(NULL, 0, 0, NULL);
}

static void rx_process_boot(const char *cmdbuf, char *response)
{
	if ((download_bytes) &&
		(CFG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE <
			download_bytes)) {
		char start[32];
		char *booti_args[4] = {"booti",  NULL, "boot", NULL};

		/*
		 * Use this later to determine if a command line was passed
		 * for the kernel.
		 */
		/* struct fastboot_boot_img_hdr *fb_hdr = */
		/*	(struct fastboot_boot_img_hdr *) interface.transfer_buffer; */

		/* Skip the mkbootimage header */
		/* image_header_t *hdr = */
		/*	(image_header_t *) */
		/*	&interface.transfer_buffer[CFG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE]; */

		booti_args[1] = start;
		sprintf(start, "0x%x", (unsigned int)interface.transfer_buffer);

		/* Execution should jump to kernel so send the response
		   now and wait a bit.	*/
		sprintf(response, "OKAY");
		fastboot_tx_status(response, strlen(response));

		printf("Booting kernel...\n");


		/* Reserve for further use, this can
		 * be more convient for developer. */
		/* if (strlen ((char *) &fb_hdr->cmdline[0])) */
		/*	set_env("bootargs", (char *) &fb_hdr->cmdline[0]); */

		/* boot the boot.img */
		do_booti(NULL, 0, 3, booti_args);


	}
	sprintf(response, "FAILinvalid boot image");
}

static void rx_process_upload(const char *cmdbuf, char *response)
{
#if defined(CONFIG_FASTBOOT_STORAGE_NAND)
	unsigned int adv, delim_index, len;
	struct fastboot_ptentry *ptn;
	unsigned int is_raw = 0;

	/* Is this a raw read ? */
	if (memcmp(cmdbuf, "uploadraw:", 10) == 0) {
		is_raw = 1;
		adv = 10;
	} else {
		adv = 7;
	}

	/* Scan to the next ':' to find when the size starts */
	len = strlen(cmdbuf);
	for (delim_index = adv;
		 delim_index < len; delim_index++) {
		if (cmdbuf[delim_index] == ':') {
			/* WARNING, cmdbuf is being modified. */
			*((char *) &cmdbuf[delim_index]) = 0;
			break;
		}
	}

	ptn = fastboot_flash_find_ptn(cmdbuf + adv);
	if (ptn == 0) {
		sprintf(response,
			"FAILpartition does not exist");
	} else {
		/* This is how much the user is expecting */
		unsigned int user_size;
		/*
		 * This is the maximum size needed for
		 * this partition
		 */
		unsigned int size;
		/* This is the length of the data */
		unsigned int length;
		/*
		 * Used to check previous write of
		 * the parition
		 */
		char env_ptn_length_var[128];
		char *env_ptn_length_val;

		user_size = 0;
		if (delim_index < len)
			user_size =
				simple_strtoul(cmdbuf + delim_index + 1,
					NULL, 16);

		/* Make sure output is padded to block size */
		length = ptn->length;
		sprintf(env_ptn_length_var,
			"%s_nand_size", ptn->name);
		env_ptn_length_val = getenv(env_ptn_length_var);
		if (env_ptn_length_val) {
			length =
				simple_strtoul(env_ptn_length_val,
					NULL, 16);
			/* Catch possible problems */
			if (!length)
				length = ptn->length;
		}

		size = length / interface.nand_block_size;
		size *= interface.nand_block_size;
		if (length % interface.nand_block_size)
			size += interface.nand_block_size;

		if (is_raw)
			size += (size /
				 interface.nand_block_size) *
				interface.nand_oob_size;

		if (size > interface.transfer_buffer_size) {

			sprintf(response, "FAILdata too large");

		} else if (user_size == 0) {

			/* Send the data response */
			sprintf(response, "DATA%08x", size);

		} else if (user_size != size) {
			/* This is the wrong size */
			sprintf(response, "FAIL");
		} else {
			/*
			 * This is where the transfer
			 * buffer is populated
			 */
			unsigned char *buf =
			  interface.transfer_buffer;
			char read[128];

			/*
			 * Setting upload_size causes
			 * transfer to happen in main loop
			 */
			upload_size = size;
			upload_bytes = 0;
			upload_error = 0;

			/*
			 * Poison the transfer buffer, 0xff
			 * is erase value of nand
			 */
			memset(buf, 0xff, upload_size);

			/* Which flavor of read to use */
			if (is_raw)
				sprintf(read, "nand read.raw 0x%x 0x%x 0x%x",
					(unsigned int)(interface.transfer_buffer),
					ptn->start,
					upload_size);
			else
				sprintf(read, "nand read.i 0x%x 0x%x 0x%x",
					(unsigned int)(interface.transfer_buffer),
					ptn->start,
					upload_size);

			run_command(read, 0);

			/* Send the data response */
			sprintf(response, "DATA%08x", size);
		}
	}
#endif

}

static int tx_handler(void)
{
	if (upload_size) {

		int bytes_written;
		bytes_written = fastboot_tx(interface.transfer_buffer +
					    upload_bytes, upload_size -
					    upload_bytes);
		if (bytes_written > 0) {

			upload_bytes += bytes_written;
			/* Check if this is the last */
			if (upload_bytes == upload_size) {

				/* Reset upload */
				upload_size = 0;
				upload_bytes = 0;
				upload_error = 0;
			}
		}
	}
	return upload_error;
}

static int rx_handler (const unsigned char *buffer, unsigned int buffer_size)
{
	int ret = 1;

	/*response buffer, Use 65 instead of 64
	   null gets dropped. strcpy's need the extra byte */
	char response[FASTBOOT_RESPONSE_SIZE];

	if (download_size) {
		/* Something to download */

		if (buffer_size) {
			/* Handle possible overflow */
			unsigned int transfer_size =
				download_size - download_bytes;

			if (buffer_size < transfer_size)
				transfer_size = buffer_size;

			/* Save the data to the transfer buffer */
			memcpy(interface.transfer_buffer + download_bytes,
				buffer, transfer_size);

			download_bytes += transfer_size;

			/* Check if transfer is done */
			if (download_bytes >= download_size) {
				/* Reset global transfer variable,
				   Keep download_bytes because it will be
				   used in the next possible flashing command */
				download_size = 0;

				if (download_error) {
					/* There was an earlier error */
					sprintf(response, "ERROR");
				} else {
					/* Everything has transferred,
					   send the OK response */
					sprintf(response, "OKAY");
				}
				fastboot_tx_status(response, strlen(response));

				printf("\ndownloading of %d bytes finished\n",
					download_bytes);

#if defined(CONFIG_FASTBOOT_STORAGE_NAND)
				/* Pad to block length
				   In most cases, padding the download to be
				   block aligned is correct. The exception is
				   when the following flash writes to the oob
				   area.  This happens when the image is a
				   YAFFS image.  Since we do not know what
				   the download is until it is flashed,
				   go ahead and pad it, but save the true
				   size in case if should have
				   been unpadded */
				download_bytes_unpadded = download_bytes;
				if (interface.nand_block_size) {
					if (download_bytes %
					    interface.nand_block_size) {
						unsigned int pad =
							interface.nand_block_size -
							(download_bytes % interface.nand_block_size);
						unsigned int i;

						for (i = 0; i < pad; i++) {
							if (download_bytes >=
								interface.transfer_buffer_size)
								break;

							interface.transfer_buffer[download_bytes] = 0;
							download_bytes++;
						}
					}
				}
#endif
			}

			/* Provide some feedback */
			if (download_bytes &&
			    0 == (download_bytes %
				  (16 * interface.nand_block_size))) {
				/* Some feeback that the
				   download is happening */
				if (download_error)
					printf("X");
				else
					printf(".");
				if (0 == (download_bytes %
					  (80 * 16 *
					   interface.nand_block_size)))
					printf("\n");

			}
		} else {
			/* Ignore empty buffers */
			printf("Warning empty download buffer\n");
			printf("Ignoring\n");
		}
		ret = 0;
	} else {
		/* A command */

		/* Cast to make compiler happy with string functions */
		const char *cmdbuf = (char *) buffer;
		printf("cmdbuf: %s\n", cmdbuf);

		/* Generic failed response */
		sprintf(response, "FAIL");

		/*reboot to bootloader mode*/
		if (memcmp(cmdbuf, "reboot-bootloader", 17) == 0) {
			rx_process_reboot_bootloader(cmdbuf, response);
			return 0;
		}


		/* reboot
		   Reboot the board. */
		if (memcmp(cmdbuf, "reboot", 6) == 0) {
			rx_process_reboot(cmdbuf, response);
			/* This code is unreachable,
			   leave it to make the compiler happy */
			return 0;
		}
		/* getvar
		   Get common fastboot variables
		   Board has a chance to handle other variables */
		if (memcmp(cmdbuf, "getvar:", 7) == 0) {
			rx_process_getvar(cmdbuf, response);
			ret = 0;
		}

		/* erase
		   Erase a register flash partition
		   Board has to set up flash partitions */
		if (memcmp(cmdbuf, "erase:", 6) == 0)
			ret = rx_process_erase(cmdbuf, response);

		/* download
		   download something ..
		   What happens to it depends on the next command after data */

		if (memcmp(cmdbuf, "download:", 9) == 0) {

			/* save the size */
			download_size = simple_strtoul(cmdbuf + 9, NULL, 16);
			/* Reset the bytes count, now it is safe */
			download_bytes = 0;
			/* Reset error */
			download_error = 0;

			printf("Starting download of %d bytes\n",
				download_size);

			if (0 == download_size) {
				/* bad user input */
				sprintf(response, "FAILdata invalid size");
			} else if (download_size >
				    interface.transfer_buffer_size) {
				/* set download_size to 0 because this is an error */
				download_size = 0;
				sprintf(response, "FAILdata too large");
			} else {
				/* The default case, the transfer fits
				   completely in the interface buffer */
				sprintf(response, "DATA%08x", download_size);
			}
			ret = 0;
		}

		/* boot
		   boot what was downloaded

		   WARNING WARNING WARNING

		   This is not what you expect.
		   The fastboot client does its own packaging of the
		   kernel.  The layout is defined in the android header
		   file bootimage.h.  This layeout is copiedlooks like this,

		   **
		   ** +-----------------+
		   ** | boot header     | 1 page
		   ** +-----------------+
		   ** | kernel          | n pages
		   ** +-----------------+
		   ** | ramdisk         | m pages
		   ** +-----------------+
		   ** | second stage    | o pages
		   ** +-----------------+
		   **

		   We only care about the kernel.
		   So we have to jump past a page.

		   What is a page size ?
		   The fastboot client uses 2048

		   The is the default value of

		   CFG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE

		*/

		if (memcmp(cmdbuf, "boot", 4) == 0) {
			rx_process_boot(cmdbuf, response);
			ret = 0;
		}

		/* flash
		   Flash what was downloaded */
		if (memcmp(cmdbuf, "flash:", 6) == 0) {
			rx_process_flash(cmdbuf, response);
			ret = 0;
		}

		/* continue
		   Stop doing fastboot */
		if (memcmp(cmdbuf, "continue", 8) == 0) {
			sprintf(response, "OKAY");
			continue_booting = 1;
			ret = 0;
		}

		/* upload
		   Upload just the data in a partition */
		if ((memcmp(cmdbuf, "upload:", 7) == 0) ||
		    (memcmp(cmdbuf, "uploadraw:", 10) == 0)) {
			rx_process_upload(cmdbuf, response);
			ret = 0;
		}

		fastboot_tx_status(response, strlen(response));

	} /* End of command */

	return ret;
}

int do_fastboot(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = 1;
	int check_timeout = 0;
	uint64_t timeout_endtime = 0;
	uint64_t timeout_ticks = 1000;
	int continue_from_disconnect = 0;

	do {
		continue_from_disconnect = 0;

		/* Initialize the board specific support */
		if (0 == fastboot_init(&interface)) {

			int poll_status;

			/* If we got this far, we are a success */
			ret = 0;
			printf("fastboot initialized\n");

			timeout_endtime = get_timer(0);
			timeout_endtime += timeout_ticks;

			while (1) {
				uint64_t current_time = 0;
				poll_status = fastboot_poll();

				if (1 == check_timeout)
					current_time = get_timer(0);

				if (FASTBOOT_ERROR == poll_status) {
					/* Error */
					break;
				} else if (FASTBOOT_DISCONNECT == poll_status) {
					/* beak, cleanup and re-init */
					printf("Fastboot disconnect detected\n");
					continue_from_disconnect = 1;
					break;
				} else if ((1 == check_timeout) &&
					   (FASTBOOT_INACTIVE == poll_status)) {

					/* No activity */
					if (current_time >= timeout_endtime) {
						printf("Fastboot inactivity detected\n");
						break;
					}
				} else {
					/* Something happened */
					if (1 == check_timeout) {
						/* Update the timeout endtime */
						timeout_endtime = current_time;
						timeout_endtime += timeout_ticks;
					}
				}

				/* Check if the user wanted to terminate with ^C */
				if ((FASTBOOT_INACTIVE == poll_status) &&
				    (ctrlc())) {
					printf("Fastboot ended by user\n");
					break;
				}

				/*
				 * Check if the fastboot client wanted to
				 * continue booting
				 */
				if (continue_booting) {
					printf("Fastboot ended by client\n");
					break;
				}

				/* Check if there is something to upload */
				tx_handler();
			}
		}

		/* Reset the board specific support */
		fastboot_shutdown();

		/* restart the loop if a disconnect was detected */
	} while (continue_from_disconnect);

	return ret;
}

U_BOOT_CMD(
	fastboot,	2,	1,	do_fastboot,
	"fastboot- use USB Fastboot protocol\n",
	"[inactive timeout]\n"
	"    - Run as a fastboot usb device.\n"
	"    - The optional inactive timeout is the decimal seconds before\n"
	"    - the normal console resumes\n"
);


#ifdef CONFIG_CMD_BOOTI
  /* Section for Android bootimage format support
   * Refer:
   * http://android.git.kernel.org/?p=platform/system/core.git;a=blob;
   * f=mkbootimg/bootimg.h
   */

void
bootimg_print_image_hdr(struct fastboot_boot_img_hdr *hdr)
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
	printf("   cmdline:   %s%x\n", hdr->cmdline);

	for (i = 0; i < 8; i++)
		printf("   id[%d]:   0x%x\n", i, hdr->id[i]);
#endif
}

static struct fastboot_boot_img_hdr boothdr __aligned(ARCH_DMA_MINALIGN);

#define ALIGN_SECTOR(n, pagesz) ((n + (pagesz - 1)) & (~(pagesz - 1)))

#ifdef CONFIG_LMB
static void boot_start_lmb(bootm_headers_t *images)
{
	ulong		mem_start;
	phys_size_t	mem_size;

	lmb_init(&images->lmb);

	mem_start = getenv_bootm_low();
	mem_size = getenv_bootm_size();

	lmb_add(&images->lmb, (phys_addr_t)mem_start, mem_size);

	arch_lmb_reserve(&images->lmb);
	board_lmb_reserve(&images->lmb);
}
#else
#define lmb_reserve(lmb, base, size)
static inline void boot_start_lmb(bootm_headers_t *images) { }
#endif

/* Allow for arch specific config before we boot */
static void __arch_preboot_os(void)
{
	/* please define platform specific arch_preboot_os() */
}
void arch_preboot_os(void) __attribute__((weak, alias("__arch_preboot_os")));

/* booti <addr> [ mmc0 | mmc1 [ <partition> ] ] */
int do_booti(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned addr = 0;
	char *ptn = "boot";
	int mmcc = -1;
	struct fastboot_boot_img_hdr *hdr = &boothdr;
    uint32_t image_size;
    unsigned bootimg_addr;
#ifdef CONFIG_SECURE_BOOT
#define IVT_SIZE 0x20
#define CSF_PAD_SIZE 0x2000
/* Max of bootimage size to be 16MB */
#define MAX_ANDROID_BOOT_AUTH_SIZE 0x1000000
/* Size appended to boot.img with boot_signer */
#define BOOTIMAGE_SIGNATURE_SIZE 0x100
#endif
	int i = 0;
	bootm_headers_t images;

	for (i = 0; i < argc; i++)
		printf("%s ", argv[i]);
	printf("\n");

	if (argc < 2)
		return -1;

	if (!strncmp(argv[1], "mmc", 3))
		mmcc = simple_strtoul(argv[1]+3, NULL, 10);
	else
		addr = simple_strtoul(argv[1], NULL, 16);

	if (argc > 2)
		ptn = argv[2];

	if (mmcc != -1) {
#ifdef CONFIG_MMC
		struct fastboot_ptentry *pte;
		struct mmc *mmc;
		disk_partition_t info;
		block_dev_desc_t *dev_desc = NULL;
		unsigned sector;
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
			printf("booti: cannot find '%d' mmc device\n", mmcc);
			goto fail;
		}
		dev_desc = get_dev("mmc", mmcc);
		if (NULL == dev_desc) {
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
			printf("booti: cannot find '%s' partition\n", ptn);
			goto fail;
		}

		if (mmc->block_dev.block_read(mmcc, pte->start,
					      1, (void *)hdr) < 0) {
			printf("booti: mmc failed to read bootimg header\n");
			goto fail;
		}

		if (memcmp(hdr->magic, FASTBOOT_BOOT_MAGIC, 8)) {
			printf("booti: bad boot image magic\n");
			goto fail;
		}

		image_size = hdr->page_size +
			ALIGN_SECTOR(hdr->kernel_size, hdr->page_size) +
			ALIGN_SECTOR(hdr->ramdisk_size, hdr->page_size) +
			ALIGN_SECTOR(hdr->second_size, hdr->page_size);
		bootimg_sectors = image_size/512;
		bootimg_addr = hdr->kernel_addr - hdr->page_size;

#ifdef CONFIG_SECURE_BOOT
		/* Default boot.img should be padded to 0x1000
		   before appended with IVT&CSF data. Set the threshold of
		   boot image for athendication as 16MB
		*/
		image_size += BOOTIMAGE_SIGNATURE_SIZE;
		image_size = ALIGN_SECTOR(image_size, 0x1000);
		if (image_size > MAX_ANDROID_BOOT_AUTH_SIZE) {
			printf("The image size is too large for athenticated boot!\n");
			return 1;
		}
		/* Make sure all data boot.img + IVT + CSF been read to memory */
		bootimg_sectors = image_size/512 +
			ALIGN_SECTOR(IVT_SIZE + CSF_PAD_SIZE, 512)/512;
#endif

		if (mmc->block_dev.block_read(mmcc, pte->start,
					bootimg_sectors,
					(void *)bootimg_addr) < 0) {
			printf("booti: mmc failed to read kernel\n");
			goto fail;
		}
		/* flush cache after read */
		flush_cache((ulong)bootimg_addr, bootimg_sectors * 512); /* FIXME */

#ifdef CONFIG_SECURE_BOOT
		extern uint32_t authenticate_image(uint32_t ddr_start,
				uint32_t image_size);

		if (authenticate_image(bootimg_addr, image_size)) {
			printf("Authenticate OK\n");
		} else {
			printf("Authenticate image Fail, Please check\n\n");
			return 1;
		}
#endif /*CONFIG_SECURE_BOOT*/

		sector = pte->start + (hdr->page_size / 512);
		sector += ALIGN_SECTOR(hdr->kernel_size, hdr->page_size) / 512;
		if (mmc->block_dev.block_read(mmcc, sector,
						(hdr->ramdisk_size / 512) + 1,
						(void *)hdr->ramdisk_addr) < 0) {
			printf("booti: mmc failed to read ramdisk\n");
			goto fail;
		}
		/* flush cache after read */
		flush_cache((ulong)hdr->ramdisk_addr, hdr->ramdisk_size); /* FIXME */

#ifdef CONFIG_OF_LIBFDT
		/* load the dtb file */
		if (hdr->second_size && hdr->second_addr) {
			sector += ALIGN_SECTOR(hdr->ramdisk_size, hdr->page_size) / 512;
			if (mmc->block_dev.block_read(mmcc, sector,
						(hdr->second_size / 512) + 1,
						(void *)hdr->second_addr) < 0) {
				printf("booti: mmc failed to dtb\n");
				goto fail;
			}
			/* flush cache after read */
			flush_cache((ulong)hdr->second_addr, hdr->second_size); /* FIXME */
		}
#endif /*CONFIG_OF_LIBFDT*/

#else /*! CONFIG_MMC*/
		return -1;
#endif /*! CONFIG_MMC*/
	} else {
		unsigned kaddr, raddr, end;
#ifdef CONFIG_OF_LIBFDT
		unsigned fdtaddr = 0;
#endif

		/* set this aside somewhere safe */
		memcpy(hdr, (void *) addr, sizeof(*hdr));

		if (memcmp(hdr->magic, FASTBOOT_BOOT_MAGIC, 8)) {
			printf("booti: bad boot image magic\n");
			return 1;
		}

		bootimg_print_image_hdr(hdr);

		kaddr = addr + hdr->page_size;
		raddr = kaddr + ALIGN_SECTOR(hdr->kernel_size, hdr->page_size);
		end = raddr + hdr->ramdisk_size;
#ifdef CONFIG_OF_LIBFDT
		if (hdr->second_size) {
			fdtaddr = raddr + ALIGN_SECTOR(hdr->ramdisk_size, hdr->page_size);
			end = fdtaddr + hdr->second_size;
		}
#endif /*CONFIG_OF_LIBFDT*/

#ifdef CONFIG_SECURE_BOOT
		image_size = hdr->page_size +
			ALIGN_SECTOR(hdr->kernel_size, hdr->page_size) +
			ALIGN_SECTOR(hdr->ramdisk_size, hdr->page_size) +
			ALIGN_SECTOR(hdr->second_size, hdr->page_size) + BOOTIMAGE_SIGNATURE_SIZE;
		if (image_size > MAX_ANDROID_BOOT_AUTH_SIZE) {
			printf("The image size is too large for athenticated boot!\n");
			return 1;
		}
		bootimg_addr = addr;
		extern uint32_t authenticate_image(uint32_t ddr_start,
				uint32_t image_size);

		if (authenticate_image(bootimg_addr, image_size)) {
			printf("Authenticate OK\n");
		} else {
			printf("Authenticate image Fail, Please check\n\n");
			return 1;
		}
#endif /*CONFIG_SECURE_BOOT*/

		if (kaddr != hdr->kernel_addr) {
			/*check overlap*/
			if (((hdr->kernel_addr >= addr) &&
					(hdr->kernel_addr <= end)) ||
				((addr >= hdr->kernel_addr) &&
					(addr <= hdr->kernel_addr + hdr->kernel_size))) {
				printf("Fail: booti address overlap with kernel address\n");
				return 1;
			}
			memmove((void *) hdr->kernel_addr,
				(void *)kaddr, hdr->kernel_size);
		}
		if (raddr != hdr->ramdisk_addr) {
			/*check overlap*/
			if (((hdr->ramdisk_addr >= addr) &&
					(hdr->ramdisk_addr <= end)) ||
				((addr >= hdr->ramdisk_addr) &&
					(addr <= hdr->ramdisk_addr + hdr->ramdisk_size))) {
				printf("Fail: booti address overlap with ramdisk address\n");
				return 1;
			}
			memmove((void *) hdr->ramdisk_addr,
				(void *)raddr, hdr->ramdisk_size);
		}

#ifdef CONFIG_OF_LIBFDT
		if (hdr->second_size && fdtaddr != hdr->second_addr) {
			/*check overlap*/
			if (((hdr->second_addr >= addr) &&
					(hdr->second_addr <= end)) ||
				((addr >= hdr->second_addr) &&
					(addr <= hdr->second_addr + hdr->second_size))) {
				printf("Fail: booti address overlap with FDT address\n");
				return 1;
			}
			memmove((void *) hdr->second_addr,
				(void *)fdtaddr, hdr->second_size);
		}
#endif /*CONFIG_OF_LIBFDT*/
	}

	printf("kernel   @ %08x (%d)\n", hdr->kernel_addr, hdr->kernel_size);
	printf("ramdisk  @ %08x (%d)\n", hdr->ramdisk_addr, hdr->ramdisk_size);
#ifdef CONFIG_OF_LIBFDT
	if (hdr->second_size)
		printf("fdt      @ %08x (%d)\n", hdr->second_addr, hdr->second_size);
#endif /*CONFIG_OF_LIBFDT*/

#ifdef CONFIG_CMDLINE_TAG
#ifndef CONFIG_SECURE_BOOT
    /* not allow to change bootargs in cmd line */
	char *commandline = getenv("bootargs");
#else
	char *commandline = NULL;
#endif

	/* If no bootargs env, just use hdr command line */
	if (!commandline) {
		commandline = (char *)hdr->cmdline;
#ifdef CONFIG_SERIAL_TAG
		char appended_cmd_line[FASTBOOT_BOOT_ARGS_SIZE];
		struct tag_serialnr serialnr;
		get_board_serial(&serialnr);
		if (strlen((char *)hdr->cmdline) +
			strlen("androidboot.serialno") + 17 < FASTBOOT_BOOT_ARGS_SIZE) {
			sprintf(appended_cmd_line,
							"%s androidboot.serialno=%08x%08x",
							(char *)hdr->cmdline,
							serialnr.high,
							serialnr.low);
			commandline = appended_cmd_line;
		} else {
			printf("Cannot append androidboot.serialno\n");
		}

		setenv("bootargs", commandline);
#endif
	}

	/* XXX: in production, you should always use boot.img 's cmdline !!! */
	printf("kernel cmdline:\n");
	printf("\tuse boot.img ");
	printf("command line:\n\t%s\n", commandline);
#endif /*CONFIG_CMDLINE_TAG*/

	memset(&images, 0, sizeof(images));

	/*Setup lmb for memory reserve*/
	boot_start_lmb(&images);

	images.ep = hdr->kernel_addr;
	images.rd_start = hdr->ramdisk_addr;
	images.rd_end = hdr->ramdisk_addr + hdr->ramdisk_size;

	/*Reserve memory for kernel image*/
	lmb_reserve(&images.lmb, images.ep, hdr->kernel_size);

#ifdef CONFIG_OF_LIBFDT
	/*use secondary fields for fdt, second_size= fdt size, second_addr= fdt addr*/
	images.ft_addr = (char *)(hdr->second_addr);
	images.ft_len = (ulong)(hdr->second_size);
	set_working_fdt_addr((ulong)(images.ft_addr));
#endif /*CONFIG_OF_LIBFDT*/

	arch_preboot_os();

	do_bootm_linux(0, 0, NULL, &images);

	puts("booti: Control returned to monitor - resetting...\n");
	do_reset(cmdtp, flag, argc, argv);
	return 1;

fail:
#ifdef CONFIG_FASTBOOT
	return do_fastboot(NULL, 0, 0, NULL);
#else /*! CONFIG_FASTBOOT*/
	return -1;
#endif /*! CONFIG_FASTBOOT*/
}

U_BOOT_CMD(
	booti,	3,	1,	do_booti,
	"booti   - boot android bootimg from memory\n",
	"[<addr> | mmc0 | mmc1 | mmc2 | mmcX] [<partition>]\n    "
	"- boot application image stored in memory or mmc\n"
	"\t'addr' should be the address of boot image "
	"which is zImage+ramdisk.img\n"
	"\t'mmcX' is the mmc device you store your boot.img, "
	"which will read the boot.img from 1M offset('/boot' partition)\n"
	"\t 'partition' (optional) is the partition id of your device, "
	"if no partition give, will going to 'boot' partition\n"
);
#endif	/* CONFIG_CMD_BOOTI */

#endif	/* CONFIG_FASTBOOT */
