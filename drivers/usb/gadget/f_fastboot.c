/*
 * (C) Copyright 2008 - 2009
 * Windriver, <www.windriver.com>
 * Tom Rix <Tom.Rix@windriver.com>
 *
 * Copyright 2011 Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 *
 * Copyright 2014 Linaro, Ltd.
 * Rob Herring <robh@kernel.org>
 *
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <config.h>
#include <common.h>
#include <errno.h>
#include <stdlib.h>
#include <fastboot.h>
#include <malloc.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/composite.h>
#include <linux/compiler.h>
#include <version.h>
#include <g_dnl.h>
#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
#include <fb_mmc.h>
#endif
#ifdef CONFIG_FASTBOOT_FLASH_NAND_DEV
#include <fb_nand.h>
#endif

#ifdef CONFIG_FSL_FASTBOOT
#include <asm/imx-common/sys_proto.h>
#include <fsl_fastboot.h>
#include <mmc.h>
#include <android_image.h>
#include <asm/bootm.h>
#include <nand.h>
#include <part.h>
#include <sparse_format.h>
#include <image-sparse.h>
#include <image.h>
#include <asm/imx-common/boot_mode.h>
#ifdef CONFIG_ANDROID_RECOVERY
#include <recovery.h>
#endif
#endif

#ifdef CONFIG_BCB_SUPPORT
#include "bcb.h"
#endif

#ifdef CONFIG_AVB_SUPPORT
#include <fsl_avb.h>
#endif

#define FASTBOOT_VERSION		"0.4"

#ifdef CONFIG_FASTBOOT_LOCK
#include "fastboot_lock_unlock.h"
#endif
#define FASTBOOT_COMMON_VAR_NUM	13
#define FASTBOOT_VAR_YES    "yes"
#define FASTBOOT_VAR_NO     "no"

#define ANDROID_GPT_OFFSET         0
#define ANDROID_GPT_SIZE           0x100000
#define ANDROID_GPT_END	           0x4400
#define FASTBOOT_INTERFACE_CLASS	0xff
#define FASTBOOT_INTERFACE_SUB_CLASS	0x42
#define FASTBOOT_INTERFACE_PROTOCOL	0x03

#define RX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0  (0x0200)
#define RX_ENDPOINT_MAXIMUM_PACKET_SIZE_1_1  (0x0040)
#define TX_ENDPOINT_MAXIMUM_PACKET_SIZE      (0x0040)

#define EP_BUFFER_SIZE			4096
/*
 * EP_BUFFER_SIZE must always be an integral multiple of maxpacket size
 * (64 or 512 or 1024), else we break on certain controllers like DWC3
 * that expect bulk OUT requests to be divisible by maxpacket size.
 */

/* common variables of fastboot getvar command */
char *fastboot_common_var[FASTBOOT_COMMON_VAR_NUM] = {
	"version",
	"version-bootloader",
	"version-baseband",
	"product",
	"secure",
	"max-download-size",
	"erase-block-size",
	"logical-block-size",
	"unlocked",
	"off-mode-charge",
	"battery-voltage",
	"variant",
	"battery-soc-ok"
};


#define MAX_REQ_NUM 100

struct f_fastboot {
	struct usb_function usb_function;

	/* IN/OUT EP's and corresponding requests */
	struct usb_ep *in_ep, *out_ep;
	struct usb_request *in_req, *in_req_more[MAX_REQ_NUM], *out_req;
	int next_req;
};

static inline struct f_fastboot *func_to_fastboot(struct usb_function *f)
{
	return container_of(f, struct f_fastboot, usb_function);
}

static struct f_fastboot *fastboot_func;
static unsigned int download_size;
static unsigned int download_bytes;
#ifdef CONFIG_SYSTEM_RAMDISK_SUPPORT
static bool is_recovery_mode;
#endif

static int strcmp_l1(const char *s1, const char *s2);

static struct usb_endpoint_descriptor fs_ep_in = {
	.bLength            = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType    = USB_DT_ENDPOINT,
	.bEndpointAddress   = USB_DIR_IN,
	.bmAttributes       = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize     = cpu_to_le16(64),
};

static struct usb_endpoint_descriptor fs_ep_out = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= cpu_to_le16(64),
};

static struct usb_endpoint_descriptor hs_ep_in = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= cpu_to_le16(512),
};

static struct usb_endpoint_descriptor hs_ep_out = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= cpu_to_le16(512),
};

static struct usb_interface_descriptor interface_desc = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= 0x00,
	.bAlternateSetting	= 0x00,
	.bNumEndpoints		= 0x02,
	.bInterfaceClass	= FASTBOOT_INTERFACE_CLASS,
	.bInterfaceSubClass	= FASTBOOT_INTERFACE_SUB_CLASS,
	.bInterfaceProtocol	= FASTBOOT_INTERFACE_PROTOCOL,
};

static struct usb_descriptor_header *fb_fs_function[] = {
	(struct usb_descriptor_header *)&interface_desc,
	(struct usb_descriptor_header *)&fs_ep_in,
	(struct usb_descriptor_header *)&fs_ep_out,
};

static struct usb_descriptor_header *fb_hs_function[] = {
	(struct usb_descriptor_header *)&interface_desc,
	(struct usb_descriptor_header *)&hs_ep_in,
	(struct usb_descriptor_header *)&hs_ep_out,
	NULL,
};

static struct usb_endpoint_descriptor *
fb_ep_desc(struct usb_gadget *g, struct usb_endpoint_descriptor *fs,
	    struct usb_endpoint_descriptor *hs)
{
	if (gadget_is_dualspeed(g) && g->speed == USB_SPEED_HIGH)
		return hs;
	return fs;
}

/*
 * static strings, in UTF-8
 */
static const char fastboot_name[] = "Android Fastboot";

static struct usb_string fastboot_string_defs[] = {
	[0].s = fastboot_name,
	{  }			/* end of list */
};

static struct usb_gadget_strings stringtab_fastboot = {
	.language	= 0x0409,	/* en-us */
	.strings	= fastboot_string_defs,
};

static struct usb_gadget_strings *fastboot_strings[] = {
	&stringtab_fastboot,
	NULL,
};

#ifdef CONFIG_FSL_FASTBOOT


#define ANDROID_MBR_OFFSET	    0
#define ANDROID_MBR_SIZE	    0x200
#ifdef  CONFIG_BOOTLOADER_OFFSET_33K
#define ANDROID_BOOTLOADER_OFFSET   0x8400
#else
#define ANDROID_BOOTLOADER_OFFSET   0x400
#endif
#define ANDROID_BOOTLOADER_SIZE	    0xFFC00
#define ANDROID_KERNEL_OFFSET	    0x100000
#define ANDROID_KERNEL_SIZE	    0x500000
#define ANDROID_URAMDISK_OFFSET	    0x600000
#define ANDROID_URAMDISK_SIZE	    0x100000



#define MMC_SATA_BLOCK_SIZE 512
#define FASTBOOT_FBPARTS_ENV_MAX_LEN 1024
/* To support the Android-style naming of flash */
#define MAX_PTN		    32
static struct fastboot_ptentry g_ptable[MAX_PTN];
static unsigned int g_pcount;
struct fastboot_device_info fastboot_devinfo;


enum {
	PTN_GPT_INDEX = 0,
	PTN_BOOTLOADER_INDEX
};
static unsigned int download_bytes_unpadded;

static struct cmd_fastboot_interface interface = {
	.rx_handler            = NULL,
	.reset_handler         = NULL,
	.product_name          = NULL,
	.serial_no             = NULL,
	.nand_block_size       = 0,
	.transfer_buffer       = (unsigned char *)0xffffffff,
	.transfer_buffer_size  = 0,
};


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
					struct mtd_info *nand;
					unsigned long off;
					unsigned int ok_start;

					nand = nand_info[nand_curr_device];

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

static void process_flash_nand(const char *cmdbuf)
{
	if (download_bytes) {
		struct fastboot_ptentry *ptn;

		ptn = fastboot_flash_find_ptn(cmdbuf);
		if (ptn == 0) {
			fastboot_fail("partition does not exist");
		} else if ((download_bytes > ptn->length) &&
			   !(ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_ENV)) {
			fastboot_fail("image too large for partition");
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
					fastboot_fail(err_string);
				} else {
					printf("partition '%s' saveenv-ed\n", ptn->name);
					fastboot_okay("");
				}
			} else {
				/* Normal case */
				if (write_to_ptn(ptn)) {
					printf("flashing '%s' failed\n", ptn->name);
					fastboot_fail("failed to flash partition");
				} else {
					printf("partition '%s' flashed\n", ptn->name);
					fastboot_okay("");
				}
			}
		}
	} else {
		fastboot_fail("no image downloaded");
	}

}
#endif

#if defined(CONFIG_FASTBOOT_STORAGE_SATA)
static void process_flash_sata(const char *cmdbuf)
{
	if (download_bytes) {
		struct fastboot_ptentry *ptn;

		/* Next is the partition name */
		ptn = fastboot_flash_find_ptn(cmdbuf);
		if (ptn == NULL) {
			fastboot_fail("partition does not exist");
		} else if ((download_bytes >
			   ptn->length * MMC_SATA_BLOCK_SIZE) &&
				!(ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_ENV)) {
			printf("Image too large for the partition\n");
			fastboot_fail("image too large for partition");
		} else if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_ENV) {
			/* Since the response can only be 64 bytes,
			   there is no point in having a large error message. */
			char err_string[32];
			if (saveenv_to_ptn(ptn, &err_string[0])) {
				printf("savenv '%s' failed : %s\n", ptn->name, err_string);
				fastboot_fail(err_string);
			} else {
				printf("partition '%s' saveenv-ed\n", ptn->name);
				fastboot_okay("");
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
				temp);

			if (run_command(sata_write, 0)) {
				printf("Writing '%s' FAILED!\n",
					 ptn->name);
				fastboot_fail("Write partition failed");
			} else {
				printf("Writing '%s' DONE!\n",
					ptn->name);
				fastboot_okay("");
			}
		}
	} else {
		fastboot_fail("no image downloaded");
	}

}
#endif

#if defined(CONFIG_FASTBOOT_STORAGE_MMC)
static int is_raw_partition(struct fastboot_ptentry *ptn)
{
#ifdef CONFIG_ANDROID_AB_SUPPORT
	if (ptn && (!strncmp(ptn->name, FASTBOOT_PARTITION_BOOTLOADER,
			strlen(FASTBOOT_PARTITION_BOOTLOADER)) ||
			!strncmp(ptn->name, FASTBOOT_PARTITION_GPT,
			strlen(FASTBOOT_PARTITION_GPT)) ||
			!strncmp(ptn->name, FASTBOOT_PARTITION_BOOT_A,
			strlen(FASTBOOT_PARTITION_BOOT_A)) ||
			!strncmp(ptn->name, FASTBOOT_PARTITION_BOOT_B,
			strlen(FASTBOOT_PARTITION_BOOT_B)) ||
#ifdef CONFIG_FASTBOOT_LOCK
			!strncmp(ptn->name, FASTBOOT_PARTITION_FBMISC,
			strlen(FASTBOOT_PARTITION_FBMISC)) ||
#endif
			!strncmp(ptn->name, FASTBOOT_PARTITION_MISC,
			strlen(FASTBOOT_PARTITION_MISC)))) {
#else
	 if (ptn && (!strncmp(ptn->name, FASTBOOT_PARTITION_BOOTLOADER,
		strlen(FASTBOOT_PARTITION_BOOTLOADER)) ||
		!strncmp(ptn->name, FASTBOOT_PARTITION_BOOT,
		strlen(FASTBOOT_PARTITION_BOOT)) ||
#ifdef CONFIG_FASTBOOT_LOCK
		!strncmp(ptn->name, FASTBOOT_PARTITION_FBMISC,
		strlen(FASTBOOT_PARTITION_FBMISC)) ||
#endif
		!strncmp(ptn->name, FASTBOOT_PARTITION_MISC,
		strlen(FASTBOOT_PARTITION_MISC)))) {
#endif
		printf("support sparse flash partition for %s\n", ptn->name);
		return 1;
	 } else
		return 0;
}

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

/*judge wether the gpt image and bootloader image are overlay*/
bool bootloader_gpt_overlay(void)
{
	return (g_ptable[PTN_GPT_INDEX].partition_id  == g_ptable[PTN_BOOTLOADER_INDEX].partition_id  &&
		ANDROID_BOOTLOADER_OFFSET < ANDROID_GPT_END);
}

int write_backup_gpt(void)
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

	/* write backup get partition */
	if (write_backup_gpt_partitions(dev_desc, interface.transfer_buffer)) {
		printf("writing GPT image fail\n");
		return -1;
	}

	printf("flash backup gpt image successfully\n");
	return 0;
}

static void process_flash_mmc(const char *cmdbuf)
{
	if (download_bytes) {
		struct fastboot_ptentry *ptn;
#ifdef CONFIG_AVB_SUPPORT
		if (!strcmp_l1(FASTBOOT_PARTITION_AVBKEY, cmdbuf)) {
			printf("pubkey len %d\n", download_bytes);
			if (avbkey_init(interface.transfer_buffer, download_bytes) != 0) {
				fastboot_fail("fail to Write partition");
			} else {
				printf("init 'avbkey' DONE!\n");
				fastboot_okay("OKAY");
			}
			return;
		}
#endif

		/* Next is the partition name */
		ptn = fastboot_flash_find_ptn(cmdbuf);
		if (ptn == NULL) {
			fastboot_fail("partition does not exist");
		} else if ((download_bytes >
			   ptn->length * MMC_SATA_BLOCK_SIZE) &&
				!(ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_ENV)) {
			printf("Image too large for the partition\n");
			fastboot_fail("image too large for partition");
		} else if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_ENV) {
			/* Since the response can only be 64 bytes,
			   there is no point in having a large error message. */
			char err_string[32];
			if (saveenv_to_ptn(ptn, &err_string[0])) {
				printf("savenv '%s' failed : %s\n", ptn->name, err_string);
				fastboot_fail(err_string);
			} else {
				printf("partition '%s' saveenv-ed\n", ptn->name);
				fastboot_okay("");
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

			if (!is_raw_partition(ptn) &&
				is_sparse_image(interface.transfer_buffer)) {
				int mmc_no = 0;
				struct mmc *mmc;
				struct blk_desc *dev_desc;
				disk_partition_t info;
				struct sparse_storage sparse;
				
				mmc_no = fastboot_devinfo.dev_id;

				printf("sparse flash target is MMC:%d\n", mmc_no);
				mmc = find_mmc_device(mmc_no);
				if (mmc && mmc_init(mmc))
					printf("MMC card init failed!\n");

				dev_desc = blk_get_dev("mmc", mmc_no);
				if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
					printf("** Block device MMC %d not supported\n",
						mmc_no);
					return;
				}

				if (part_get_info(dev_desc,
			       ptn->partition_index, &info)) {
					printf("Bad partition index:%d for partition:%s\n",
					ptn->partition_index, ptn->name);
					return;
				}

				printf("writing to partition '%s' for sparse, buffer size %d\n",
						ptn->name, download_bytes);

				sparse.blksz = info.blksz;
				sparse.start = info.start;
				sparse.size = info.size;
				sparse.write = mmc_sparse_write;
				sparse.reserve = mmc_sparse_reserve;
				printf("Flashing sparse image at offset " LBAFU "\n",
				       sparse.start);

				sparse.priv = dev_desc;
				write_sparse_image(&sparse, ptn->name, interface.transfer_buffer,
						   download_bytes);

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

					sprintf(mmc_write, "mmc write 0x%x 0x%x 0x%x",
							(unsigned int)(uintptr_t)interface.transfer_buffer, /*source*/
							ptn->start, /*dest*/
							temp /*length*/);

					printf("Initializing '%s'\n", ptn->name);

					mmcret = run_command(mmc_dev, 0);
					if (mmcret)
						fastboot_fail("Init of MMC card failed");
					else
						fastboot_okay("");

					printf("Writing '%s'\n", ptn->name);
					if (run_command(mmc_write, 0)) {
						printf("Writing '%s' FAILED!\n", ptn->name);
						fastboot_fail("Write partition failed");
					} else {
						printf("Writing '%s' DONE!\n", ptn->name);
						fastboot_okay("");
					}
				}
				/* Write backup gpt image */
				if (strncmp(ptn->name, "gpt", 3) == 0) {
					if (write_backup_gpt())
						fastboot_fail("write backup GPT image fail");
					else
						fastboot_okay("");

					/* will force scan the device,
					 * so dev_desc can be re-inited
					 * with the latest data */
					run_command(mmc_dev, 0);
				}
			}
		}
	} else {
		fastboot_fail("no image downloaded");
	}
}

#endif

#if defined(CONFIG_FASTBOOT_STORAGE_MMC)
static void process_erase_mmc(const char *cmdbuf, char *response)
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

	blks = dev_desc->block_erase(dev_desc, blks_start, blks_size);
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
#endif

#if defined(CONFIG_FASTBOOT_STORAGE_SATA)
static void process_erase_sata(const char *cmdbuf, char *response)
{
    return;
}
#endif
#if defined(CONFIG_FASTBOOT_STORAGE_NAND)
static int process_erase_nand(const char *cmdbuf, char *response)
{
	struct fastboot_ptentry *ptn;

	ptn = fastboot_flash_find_ptn(cmdbuf);
	if (ptn == NULL ||  (ptn->flags & FASTBOOT_PTENTRY_FLAGS_UNERASEABLE)) {
		fastboot_fail("partition does not exist");
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
			fastboot_fail("failed to erase partition");
		} else {
			printf("partition '%s' erased\n", ptn->name);
			fastboot_okay("");
		}
	}
	return;
}
#endif

static void rx_process_erase(const char *cmdbuf, char *response)
{
	switch (fastboot_devinfo.type) {
#if defined(CONFIG_FASTBOOT_STORAGE_SATA)
	case DEV_SATA:
		process_erase_sata(cmdbuf, response);
		break;
#endif
#if defined(CONFIG_FASTBOOT_STORAGE_MMC)
	case DEV_MMC:
		process_erase_mmc(cmdbuf, response);
		break;
#endif
#if defined(CONFIG_FASTBOOT_STORAGE_NAND)
	case DEV_NAND:
		process_erase_nand(cmdbuf, response);
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

static void rx_process_flash(const char *cmdbuf)
{
	switch (fastboot_devinfo.type) {
#if defined(CONFIG_FASTBOOT_STORAGE_SATA)
	case DEV_SATA:
		process_flash_sata(cmdbuf);
		break;
#endif
#if defined(CONFIG_FASTBOOT_STORAGE_MMC)
	case DEV_MMC:
		process_flash_mmc(cmdbuf);
		break;
#endif
#if defined(CONFIG_FASTBOOT_STORAGE_NAND)
	case DEV_NAND:
		process_flash_nand(cmdbuf);
		break;
#endif
	default:
		printf("Not support flash command for current device %d\n",
			fastboot_devinfo.type);
		fastboot_fail("failed to flash device");
		break;
	}
}


static void parameters_setup(void)
{
	interface.nand_block_size = 0;
#if defined(CONFIG_FASTBOOT_STORAGE_NAND)
	struct mtd_info *nand = nand_info[0];
	if (nand)
		interface.nand_block_size = nand->writesize;
#endif
	interface.transfer_buffer =
				(unsigned char *)CONFIG_FASTBOOT_BUF_ADDR;
	interface.transfer_buffer_size =
				CONFIG_FASTBOOT_BUF_SIZE;
}

static int _fastboot_setup_dev(void)
{
	char *fastboot_env;
	fastboot_env = getenv("fastboot_dev");

	if (fastboot_env) {
		if (!strcmp(fastboot_env, "sata")) {
			fastboot_devinfo.type = DEV_SATA;
			fastboot_devinfo.dev_id = 0;
		} else if (!strcmp(fastboot_env, "nand")) {
			fastboot_devinfo.type = DEV_NAND;
			fastboot_devinfo.dev_id = 0;
#if defined(CONFIG_FASTBOOT_STORAGE_MMC)
		} else if (!strncmp(fastboot_env, "mmc", 3)) {
			fastboot_devinfo.type = DEV_MMC;
			fastboot_devinfo.dev_id = mmc_get_env_dev();
#endif
		}
	} else {
		return 1;
	}

	return 0;
}

#if defined(CONFIG_FASTBOOT_STORAGE_SATA) \
	|| defined(CONFIG_FASTBOOT_STORAGE_MMC)
/**
   @mmc_dos_partition_index: the partition index in mbr.
   @mmc_partition_index: the boot partition or user partition index,
   not related to the partition table.
 */
static int _fastboot_parts_add_ptable_entry(int ptable_index,
				      int mmc_dos_partition_index,
				      int mmc_partition_index,
				      const char *name,
				      const char *fstype,
				      struct blk_desc *dev_desc,
				      struct fastboot_ptentry *ptable)
{
	disk_partition_t info;

	if (part_get_info(dev_desc,
			       mmc_dos_partition_index, &info)) {
		debug("Bad partition index:%d for partition:%s\n",
		       mmc_dos_partition_index, name);
		return -1;
	}
	ptable[ptable_index].start = info.start;
	ptable[ptable_index].length = info.size;
	ptable[ptable_index].partition_id = mmc_partition_index;
	ptable[ptable_index].partition_index = mmc_dos_partition_index;
	strcpy(ptable[ptable_index].name, (const char *)info.name);

#ifdef CONFIG_PARTITION_UUIDS
	strcpy(ptable[ptable_index].uuid, (const char *)info.uuid);
#endif
#ifdef CONFIG_ANDROID_AB_SUPPORT
	if (!strcmp((const char *)info.name, FASTBOOT_PARTITION_SYSTEM_A) ||
		!strcmp((const char *)info.name, FASTBOOT_PARTITION_SYSTEM_B) ||
		!strcmp((const char *)info.name, FASTBOOT_PARTITION_DATA))
#else
	if (!strcmp((const char *)info.name, FASTBOOT_PARTITION_SYSTEM) ||
		!strcmp((const char *)info.name, FASTBOOT_PARTITION_DATA) ||
		!strcmp((const char *)info.name, FASTBOOT_PARTITION_DEVICE) ||
		!strcmp((const char *)info.name, FASTBOOT_PARTITION_CACHE))
#endif
		strcpy(ptable[ptable_index].fstype, "ext4");
	else
		strcpy(ptable[ptable_index].fstype, "raw");
	return 0;
}

static int _fastboot_parts_load_from_ptable(void)
{
	int i;
#ifdef CONFIG_CMD_SATA
	int sata_device_no;
#endif

	/* mmc boot partition: -1 means no partition, 0 user part., 1 boot part.
	 * default is no partition, for emmc default user part, except emmc*/
	int boot_partition = FASTBOOT_MMC_NONE_PARTITION_ID;
    int user_partition = FASTBOOT_MMC_NONE_PARTITION_ID;

	struct mmc *mmc;
	struct blk_desc *dev_desc;
	struct fastboot_ptentry ptable[MAX_PTN];

	/* sata case in env */
	if (fastboot_devinfo.type == DEV_SATA) {
#ifdef CONFIG_CMD_SATA
		puts("flash target is SATA\n");
		if (sata_initialize())
			return -1;
		sata_device_no = CONFIG_FASTBOOT_SATA_NO;
		if (sata_device_no >= CONFIG_SYS_SATA_MAX_DEVICE) {
			printf("Unknown SATA(%d) device for fastboot\n",
				sata_device_no);
			return -1;
		}
		dev_desc = sata_get_dev(sata_device_no);
#else /*! CONFIG_CMD_SATA*/
		puts("SATA isn't buildin\n");
		return -1;
#endif /*! CONFIG_CMD_SATA*/
	} else if (fastboot_devinfo.type == DEV_MMC) {
		int mmc_no = 0;
		mmc_no = fastboot_devinfo.dev_id;

		printf("flash target is MMC:%d\n", mmc_no);
		mmc = find_mmc_device(mmc_no);
		if (mmc && mmc_init(mmc))
			printf("MMC card init failed!\n");

		dev_desc = blk_get_dev("mmc", mmc_no);
		if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
			printf("** Block device MMC %d not supported\n",
				mmc_no);
			return -1;
		}

		/* multiple boot paritions for eMMC 4.3 later */
		if (mmc->part_config != MMCPART_NOAVAILABLE) {
			boot_partition = FASTBOOT_MMC_BOOT_PARTITION_ID;
			user_partition = FASTBOOT_MMC_USER_PARTITION_ID;
		}
	} else {
		printf("Can't setup partition table on this device %d\n",
			fastboot_devinfo.type);
		return -1;
	}

	memset((char *)ptable, 0,
		    sizeof(struct fastboot_ptentry) * (MAX_PTN));
	/* GPT */
	strcpy(ptable[PTN_GPT_INDEX].name, FASTBOOT_PARTITION_GPT);
	ptable[PTN_GPT_INDEX].start = ANDROID_GPT_OFFSET / dev_desc->blksz;
	ptable[PTN_GPT_INDEX].length = ANDROID_GPT_SIZE  / dev_desc->blksz;
	ptable[PTN_GPT_INDEX].partition_id = user_partition;
	ptable[PTN_GPT_INDEX].flags = FASTBOOT_PTENTRY_FLAGS_UNERASEABLE;
	strcpy(ptable[PTN_GPT_INDEX].fstype, "raw");
	/* Bootloader */
	strcpy(ptable[PTN_BOOTLOADER_INDEX].name, FASTBOOT_PARTITION_BOOTLOADER);
	ptable[PTN_BOOTLOADER_INDEX].start =
				ANDROID_BOOTLOADER_OFFSET / dev_desc->blksz;
	ptable[PTN_BOOTLOADER_INDEX].length =
				 ANDROID_BOOTLOADER_SIZE / dev_desc->blksz;
	ptable[PTN_BOOTLOADER_INDEX].partition_id = boot_partition;
	ptable[PTN_BOOTLOADER_INDEX].flags = FASTBOOT_PTENTRY_FLAGS_UNERASEABLE;
	strcpy(ptable[PTN_BOOTLOADER_INDEX].fstype, "raw");

	int tbl_idx;
	int part_idx = 1;
	int ret;
	for (tbl_idx = 2; tbl_idx < MAX_PTN; tbl_idx++) {
		ret = _fastboot_parts_add_ptable_entry(tbl_idx,
				part_idx++,
				user_partition,
				NULL,
				NULL,
				dev_desc, ptable);
		if (ret)
			break;
	}
	for (i = 0; i < part_idx; i++)
		fastboot_flash_add_ptn(&ptable[i]);

	return 0;
}
#endif /*CONFIG_FASTBOOT_STORAGE_SATA || CONFIG_FASTBOOT_STORAGE_MMC*/

#if defined(CONFIG_FASTBOOT_STORAGE_NAND)
static unsigned long long _memparse(char *ptr, char **retptr)
{
	char *endptr;	/* local pointer to end of parsed string */

	unsigned long ret = simple_strtoul(ptr, &endptr, 0);

	switch (*endptr) {
	case 'M':
	case 'm':
		ret <<= 10;
	case 'K':
	case 'k':
		ret <<= 10;
		endptr++;
	default:
		break;
	}

	if (retptr)
		*retptr = endptr;

	return ret;
}

static int _fastboot_parts_add_env_entry(char *s, char **retptr)
{
	unsigned long size;
	unsigned long offset = 0;
	char *name;
	int name_len;
	int delim;
	unsigned int flags;
	struct fastboot_ptentry part;

	size = _memparse(s, &s);
	if (0 == size) {
		printf("Error:FASTBOOT size of parition is 0\n");
		return 1;
	}

	/* fetch partition name and flags */
	flags = 0; /* this is going to be a regular partition */
	delim = 0;
	/* check for offset */
	if (*s == '@') {
		s++;
		offset = _memparse(s, &s);
	} else {
		printf("Error:FASTBOOT offset of parition is not given\n");
		return 1;
	}

	/* now look for name */
	if (*s == '(')
		delim = ')';

	if (delim) {
		char *p;

		name = ++s;
		p = strchr((const char *)name, delim);
		if (!p) {
			printf("Error:FASTBOOT no closing %c found in partition name\n",
				delim);
			return 1;
		}
		name_len = p - name;
		s = p + 1;
	} else {
		printf("Error:FASTBOOT no partition name for \'%s\'\n", s);
		return 1;
	}

	/* check for options */
	while (1) {
		if (strncmp(s, "i", 1) == 0) {
			flags |= FASTBOOT_PTENTRY_FLAGS_WRITE_I;
			s += 1;
		} else if (strncmp(s, "ubifs", 5) == 0) {
			/* ubifs */
			flags |= FASTBOOT_PTENTRY_FLAGS_WRITE_TRIMFFS;
			s += 5;
		} else {
			break;
		}
		if (strncmp(s, "|", 1) == 0)
			s += 1;
	}

	/* enter this partition (offset will be calculated later if it is zero at this point) */
	part.length = size;
	part.start = offset;
	part.flags = flags;

	if (name) {
		if (name_len >= sizeof(part.name)) {
			printf("Error:FASTBOOT partition name is too long\n");
			return 1;
		}
		strncpy(&part.name[0], name, name_len);
		/* name is not null terminated */
		part.name[name_len] = '\0';
	} else {
		printf("Error:FASTBOOT no name\n");
		return 1;
	}

	fastboot_flash_add_ptn(&part);

	/*if the nand partitions envs are not initialized, try to init them*/
	if (check_parts_values(&part))
		save_parts_values(&part, part.start, part.length);

	/* return (updated) pointer command line string */
	*retptr = s;

	/* return partition table */
	return 0;
}

static int _fastboot_parts_load_from_env(void)
{
	char fbparts[FASTBOOT_FBPARTS_ENV_MAX_LEN], *env;

	env = getenv("fbparts");
	if (env) {
		unsigned int len;
		len = strlen(env);
		if (len && len < FASTBOOT_FBPARTS_ENV_MAX_LEN) {
			char *s, *e;

			memcpy(&fbparts[0], env, len + 1);
			printf("Fastboot: Adding partitions from environment\n");
			s = &fbparts[0];
			e = s + len;
			while (s < e) {
				if (_fastboot_parts_add_env_entry(s, &s)) {
					printf("Error:Fastboot: Abort adding partitions\n");
					g_pcount = 0;
					return 1;
				}
				/* Skip a bunch of delimiters */
				while (s < e) {
					if ((' ' == *s) ||
					    ('\t' == *s) ||
					    ('\n' == *s) ||
					    ('\r' == *s) ||
					    (',' == *s)) {
						s++;
					} else {
						break;
					}
				}
			}
		}
	}

	return 0;
}
#endif /*CONFIG_FASTBOOT_STORAGE_NAND*/

static void _fastboot_load_partitions(void)
{
	g_pcount = 0;
#if defined(CONFIG_FASTBOOT_STORAGE_NAND)
	_fastboot_parts_load_from_env();
#elif defined(CONFIG_FASTBOOT_STORAGE_SATA) \
	|| defined(CONFIG_FASTBOOT_STORAGE_MMC)
	_fastboot_parts_load_from_ptable();
#endif
}

/*
 * Android style flash utilties */
void fastboot_flash_add_ptn(struct fastboot_ptentry *ptn)
{
	if (g_pcount < MAX_PTN) {
		memcpy(g_ptable + g_pcount, ptn, sizeof(struct fastboot_ptentry));
		g_pcount++;
	}
}

void fastboot_flash_dump_ptn(void)
{
	unsigned int n;
	for (n = 0; n < g_pcount; n++) {
		struct fastboot_ptentry *ptn = g_ptable + n;
		printf("idx %d, ptn %d name='%s' start=%d len=%d\n",
			n, ptn->partition_index, ptn->name, ptn->start, ptn->length);
	}
}


struct fastboot_ptentry *fastboot_flash_find_ptn(const char *name)
{
	unsigned int n;

	for (n = 0; n < g_pcount; n++) {
		/* Make sure a substring is not accepted */
		if (strlen(name) == strlen(g_ptable[n].name)) {
			if (0 == strcmp(g_ptable[n].name, name))
				return g_ptable + n;
		}
	}

	printf("can't find partition: %s, dump the partition table\n", name);
	fastboot_flash_dump_ptn();
	return 0;
}

int fastboot_flash_find_index(const char *name)
{
	struct fastboot_ptentry *ptentry = fastboot_flash_find_ptn(name);
	if (ptentry == NULL) {
		printf("cannot get the partion info for %s\n",name);
		return -1;
	}
	return ptentry->partition_index;
}

struct fastboot_ptentry *fastboot_flash_get_ptn(unsigned int n)
{
	if (n < g_pcount)
		return g_ptable + n;
	else
		return 0;
}

unsigned int fastboot_flash_get_ptn_count(void)
{
	return g_pcount;
}

#ifdef CONFIG_FSL_FASTBOOT
void board_fastboot_setup(void)
{
#if defined(CONFIG_FASTBOOT_STORAGE_MMC)
	static char boot_dev_part[32];
	u32 dev_no;
#endif
	switch (get_boot_device()) {
#if defined(CONFIG_FASTBOOT_STORAGE_MMC)
	case SD1_BOOT:
	case SD2_BOOT:
	case SD3_BOOT:
	case SD4_BOOT:
	case MMC1_BOOT:
	case MMC2_BOOT:
	case MMC3_BOOT:
	case MMC4_BOOT:
		dev_no = mmc_get_env_dev();
		sprintf(boot_dev_part,"mmc%d",dev_no);
		if (!getenv("fastboot_dev"))
			setenv("fastboot_dev", boot_dev_part);
		sprintf(boot_dev_part, "boota mmc%d", dev_no);
		if (!getenv("bootcmd"))
			setenv("bootcmd", boot_dev_part);
		break;
#endif /*CONFIG_FASTBOOT_STORAGE_MMC*/
#if defined(CONFIG_FASTBOOT_STORAGE_NAND)
	case NAND_BOOT:
		if (!getenv("fastboot_dev"))
			setenv("fastboot_dev", "nand");
		if (!getenv("fbparts"))
			setenv("fbparts", ANDROID_FASTBOOT_NAND_PARTS);
		if (!getenv("bootcmd"))
			setenv("bootcmd",
				"nand read ${loadaddr} ${boot_nand_offset} "
				"${boot_nand_size};boota ${loadaddr}");
		break;
#endif /*CONFIG_FASTBOOT_STORAGE_NAND*/
	default:
		printf("unsupported boot devices\n");
		break;
	}

	/* add soc type into bootargs */
	if (is_mx6dqp()) {
		if (!getenv("soc_type"))
			setenv("soc_type", "imx6qp");
	} else if (is_mx6dq()) {
		if (!getenv("soc_type"))
			setenv("soc_type", "imx6q");
	} else if (is_mx6sdl()) {
		if (!getenv("soc_type"))
			setenv("soc_type", "imx6dl");
	} else if (is_mx6sx()) {
		if (!getenv("soc_type"))
			setenv("soc_type", "imx6sx");
	} else if (is_mx6sl()) {
		if (!getenv("soc_type"))
			setenv("soc_type", "imx6sl");
	} else if (is_mx6ul()) {
		if (!getenv("soc_type"))
			setenv("soc_type", "imx6ul");
	} else if (is_mx7()) {
		if (!getenv("soc_type"))
			setenv("soc_type", "imx7d");
	} else if (is_mx7ulp()) {
		if (!getenv("soc_type"))
			setenv("soc_type", "imx7ulp");
	}
}

#ifdef CONFIG_ANDROID_RECOVERY
void board_recovery_setup(void)
{
#if defined(CONFIG_FASTBOOT_STORAGE_MMC)
	static char boot_dev_part[32];
	u32 dev_no;
#endif
	int bootdev = get_boot_device();
	switch (bootdev) {
#if defined(CONFIG_FASTBOOT_STORAGE_MMC)
	case SD1_BOOT:
	case SD2_BOOT:
	case SD3_BOOT:
	case SD4_BOOT:
	case MMC1_BOOT:
	case MMC2_BOOT:
	case MMC3_BOOT:
	case MMC4_BOOT:
		dev_no = mmc_get_env_dev();
		sprintf(boot_dev_part,"boota mmc%d recovery",dev_no);
		if (!getenv("bootcmd_android_recovery"))
			setenv("bootcmd_android_recovery", boot_dev_part);
		break;
#endif /*CONFIG_FASTBOOT_STORAGE_MMC*/
#if defined(CONFIG_FASTBOOT_STORAGE_NAND)
	case NAND_BOOT:
		if (!getenv("bootcmd_android_recovery"))
			setenv("bootcmd_android_recovery",
				"nand read ${loadaddr} ${recovery_nand_offset} "
				"${recovery_nand_size};boota ${loadaddr}");
		break;
#endif /*CONFIG_FASTBOOT_STORAGE_NAND*/
	default:
		printf("Unsupported bootup device for recovery: dev: %d\n",
			bootdev);
		return;
	}

	printf("setup env for recovery..\n");
	setenv("bootcmd", "run bootcmd_android_recovery");
}
#endif /*CONFIG_ANDROID_RECOVERY*/
#endif /*CONFIG_FSL_FASTBOOT*/

#if defined(CONFIG_AVB_SUPPORT) && defined(CONFIG_MMC)
static AvbABOps fsl_avb_ab_ops = {
	.read_ab_metadata = fsl_read_ab_metadata,
	.write_ab_metadata = fsl_write_ab_metadata,
	.ops = NULL
};

static AvbOps fsl_avb_ops = {
	.ab_ops = &fsl_avb_ab_ops,
	.read_from_partition = fsl_read_from_partition_multi,
	.write_to_partition = fsl_write_to_partition,
	.validate_vbmeta_public_key = fsl_validate_vbmeta_public_key_rpmb,
	.read_rollback_index = fsl_read_rollback_index_rpmb,
	.write_rollback_index = fsl_write_rollback_index_rpmb,
	.read_is_device_unlocked = fsl_read_is_device_unlocked,
	.get_unique_guid_for_partition = fsl_get_unique_guid_for_partition
};
#endif

void fastboot_setup(void)
{
	struct tag_serialnr serialnr;
	char serial[17];

	get_board_serial(&serialnr);
	sprintf(serial, "%08x%08x", serialnr.high, serialnr.low);
	g_dnl_set_serialnumber(serial);

	/*execute board relevant initilizations for preparing fastboot */
	board_fastboot_setup();

	/*get the fastboot dev*/
	_fastboot_setup_dev();


	/*load partitions information for the fastboot dev*/
	_fastboot_load_partitions();

	parameters_setup();
#ifdef CONFIG_AVB_SUPPORT
	fsl_avb_ab_ops.ops = &fsl_avb_ops;
#endif
}

/* Write the bcb with fastboot bootloader commands */
static void enable_fastboot_command(void)
{
	char fastboot_command[32] = {0};
	strncpy(fastboot_command, FASTBOOT_BCB_CMD, 31);
	bcb_write_command(fastboot_command);
}

/* Get the Boot mode from BCB cmd or Key pressed */
static FbBootMode fastboot_get_bootmode(void)
{
	int ret = 0;
	int boot_mode = BOOTMODE_NORMAL;
	char command[32];
#ifdef CONFIG_ANDROID_RECOVERY
	if(is_recovery_key_pressing()) {
		boot_mode = BOOTMODE_RECOVERY_KEY_PRESSED;
		return boot_mode;
	}
#endif
	ret = bcb_read_command(command);
	if (ret < 0) {
		printf("read command failed\n");
		return boot_mode;
	}
	if (!strcmp(command, FASTBOOT_BCB_CMD)) {
		boot_mode = BOOTMODE_FASTBOOT_BCB_CMD;
	}
#ifdef CONFIG_ANDROID_RECOVERY
	else if (!strcmp(command, RECOVERY_BCB_CMD)) {
		boot_mode = BOOTMODE_RECOVERY_BCB_CMD;
	}
#endif

	/* Clean the mode once its read out,
	   no matter what in the mode string */
	memset(command, 0, 32);
	bcb_write_command(command);
	return boot_mode;
}

#ifdef CONFIG_SYSTEM_RAMDISK_SUPPORT
/* Setup booargs for taking the system parition as ramdisk */
static void fastboot_setup_system_boot_args(const char *slot)
{
	const char *system_part_name = NULL;
	if(slot == NULL)
		return;
	if(!strncmp(slot, "_a", strlen("_a")) || !strncmp(slot, "boot_a", strlen("boot_a"))) {
		system_part_name = FASTBOOT_PARTITION_SYSTEM_A;
	}
	else if(!strncmp(slot, "_b", strlen("_b")) || !strncmp(slot, "boot_b", strlen("boot_b"))) {
		system_part_name = FASTBOOT_PARTITION_SYSTEM_B;
	}
	struct fastboot_ptentry *ptentry = fastboot_flash_find_ptn(system_part_name);
	if(ptentry != NULL) {
		char bootargs_3rd[ANDR_BOOT_ARGS_SIZE];
#if defined(CONFIG_FASTBOOT_STORAGE_MMC)
		u32 dev_no = mmc_map_to_kernel_blk(mmc_get_env_dev());
		sprintf(bootargs_3rd, "skip_initramfs root=/dev/mmcblk%dp%d",
				dev_no,
				ptentry->partition_index);
		setenv("bootargs_3rd", bootargs_3rd);
#endif
		//TBD to support NAND ubifs system parition boot directly
	}
}
#endif
/* export to lib_arm/board.c */
void fastboot_run_bootmode(void)
{
	FbBootMode boot_mode = fastboot_get_bootmode();
	switch(boot_mode){
	case BOOTMODE_FASTBOOT_BCB_CMD:
		/* Make the boot into fastboot mode*/
		puts("Fastboot: Got bootloader commands!\n");
#ifdef CONFIG_SYSTEM_RAMDISK_SUPPORT
		is_recovery_mode = false;
#endif
		run_command("fastboot 0", 0);
		break;
#ifdef CONFIG_ANDROID_RECOVERY
	case BOOTMODE_RECOVERY_BCB_CMD:
	case BOOTMODE_RECOVERY_KEY_PRESSED:
		/* Make the boot into recovery mode */
		puts("Fastboot: Got Recovery key pressing or recovery commands!\n");
#ifdef CONFIG_SYSTEM_RAMDISK_SUPPORT
		is_recovery_mode = true;
#else
		board_recovery_setup();
#endif
		break;
#endif
	default:
		/* skip special mode boot*/
#ifdef CONFIG_SYSTEM_RAMDISK_SUPPORT
		is_recovery_mode = false;
#endif
		puts("Fastboot: Normal\n");
		break;
	}
}

#ifdef CONFIG_CMD_BOOTA
  /* Section for Android bootimage format support
   * Refer:
   * http://android.git.kernel.org/?p=platform/system/core.git;a=blob;
   * f=mkbootimg/bootimg.h
   */

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

static struct andr_img_hdr boothdr __aligned(ARCH_DMA_MINALIGN);

#if defined(CONFIG_AVB_SUPPORT) && defined(CONFIG_MMC)
/* we can use avb to verify Trusty if we want */
const char *requested_partitions[] = {"boot", 0};

int do_boota(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) {

	ulong addr = 0;
	struct andr_img_hdr *hdr = NULL;
	struct andr_img_hdr *hdrload;
	ulong image_size;

	AvbABFlowResult avb_result;
	AvbSlotVerifyData *avb_out_data;
	AvbPartitionData *avb_loadpart;

#ifdef CONFIG_FASTBOOT_LOCK
	/* check lock state */
	FbLockState lock_status = fastboot_get_lock_stat();
	if (lock_status == FASTBOOT_LOCK_ERROR) {
		printf("In boota get fastboot lock status error. Set lock status\n");
		fastboot_set_lock_stat(FASTBOOT_LOCK);
		lock_status = FASTBOOT_LOCK;
	}
	bool allow_fail = (lock_status == FASTBOOT_UNLOCK ? true : false);
	/* if in lock state, do avb verify */
	avb_result = avb_ab_flow(&fsl_avb_ab_ops, requested_partitions, allow_fail, &avb_out_data);
	if (avb_result == AVB_AB_FLOW_RESULT_OK) {
		assert(avb_out_data != NULL);
		/* load the first partition */
		avb_loadpart = avb_out_data->loaded_partitions;
		assert(avb_loadpart != NULL);
		/* we should use avb_part_data->data as boot image */
		/* boot image is already read by avb */
		hdr = (struct andr_img_hdr *)avb_loadpart->data;
		if (android_image_check_header(hdr)) {
			printf("boota: bad boot image magic\n");
			goto fail;
		}
		printf(" verify OK, boot '%s%s'\n", avb_loadpart->partition_name, avb_out_data->ab_suffix);
		char bootargs_sec[2048];
		sprintf(bootargs_sec, "androidboot.slot_suffix=%s %s", avb_out_data->ab_suffix, avb_out_data->cmdline);
		setenv("bootargs_sec", bootargs_sec);
#ifdef CONFIG_SYSTEM_RAMDISK_SUPPORT
		if(!is_recovery_mode)
			fastboot_setup_system_boot_args(avb_out_data->ab_suffix);
#endif
		image_size = avb_loadpart->data_size;
		memcpy((void *)load_addr, (void *)hdr, image_size);
	} else if (lock_status == FASTBOOT_LOCK) { /* && verify fail */
		/* if in lock state, verify enforce fail */
		printf(" verify FAIL, state: LOCK\n");
		goto fail;
	} else { /* lock_status == FASTBOOT_UNLOCK && verify fail */
		/* if in unlock state, log the verify state */
		printf(" verify FAIL, state: UNLOCK\n");
#endif
		/* if lock/unlock not enabled or verify fail
		 * in unlock state, will try boot */
		size_t num_read;
		hdr = &boothdr;

		char bootimg[8];
		char *slot = select_slot(&fsl_avb_ab_ops);
		if (slot == NULL) {
			printf("boota: no bootable slot\n");
			goto fail;
		}
		sprintf(bootimg, "boot%s", slot);
		printf(" boot '%s' still\n", bootimg);
		/* maybe we should use bootctl to select a/b
		 * but libavb doesn't export a/b select */
		if (fsl_avb_ops.read_from_partition(&fsl_avb_ops, bootimg,
					0, sizeof(boothdr), hdr, &num_read) != AVB_IO_RESULT_OK &&
				num_read != sizeof(boothdr)) {
			printf("boota: read bootimage head error\n");
			goto fail;
		}
		if (android_image_check_header(hdr)) {
			printf("boota: bad boot image magic\n");
			goto fail;
		}
		image_size = android_image_get_end(hdr) - (ulong)hdr;
		if (fsl_avb_ops.read_from_partition(&fsl_avb_ops, bootimg,
					0, image_size, (void *)load_addr, &num_read) != AVB_IO_RESULT_OK &&
				num_read != image_size) {
			printf("boota: read boot image error\n");
			goto fail;
		}
		char bootargs_sec[ANDR_BOOT_ARGS_SIZE];
		sprintf(bootargs_sec, "androidboot.slot_suffix=%s", slot);
		setenv("bootargs_sec", bootargs_sec);
#ifdef CONFIG_SYSTEM_RAMDISK_SUPPORT
		if(!is_recovery_mode)
			fastboot_setup_system_boot_args(slot);
#endif
#ifdef CONFIG_FASTBOOT_LOCK
	}
#endif

	flush_cache((ulong)load_addr, image_size);
	addr = load_addr;
	hdrload = (struct andr_img_hdr *)load_addr;

	ulong img_ramdisk, img_ramdisk_len;
	if(android_image_get_ramdisk(hdrload, &img_ramdisk, &img_ramdisk_len) != 0) {
		printf("boota: mmc failed to read ramdisk\n");
		goto fail;
	}
	memcpy((void *)hdrload->ramdisk_addr, (void *)img_ramdisk, img_ramdisk_len);
	/* flush cache after read */
	flush_cache((ulong)hdrload->ramdisk_addr, img_ramdisk_len); /* FIXME */

#ifdef CONFIG_OF_LIBFDT
	/* load the dtb file */
	if (hdrload->second_size && hdrload->second_addr) {
		ulong img_fdt, img_fdt_len;
		if (android_image_get_fdt(hdrload, &img_fdt, &img_fdt_len) != 0) {
			printf("boota: mmc failed to dtb\n");
			goto fail;
		}
		memcpy((void *)hdrload->second_addr, (void *)img_fdt, img_fdt_len);
		/* flush cache after read */
		flush_cache((ulong)hdrload->second_addr, img_fdt_len); /* FIXME */
	}
#endif /*CONFIG_OF_LIBFDT*/
	printf("kernel   @ %08x (%d)\n", hdrload->kernel_addr, hdrload->kernel_size);
	printf("ramdisk  @ %08x (%d)\n", hdrload->ramdisk_addr, hdrload->ramdisk_size);
#ifdef CONFIG_OF_LIBFDT
	if (hdrload->second_size)
		printf("fdt      @ %08x (%d)\n", hdrload->second_addr, hdrload->second_size);
#endif /*CONFIG_OF_LIBFDT*/

	char boot_addr_start[12];
	char ramdisk_addr[25];
	char fdt_addr[12];

	char *bootm_args[] = { "bootm", boot_addr_start, ramdisk_addr, fdt_addr};

	sprintf(boot_addr_start, "0x%lx", addr);
	sprintf(ramdisk_addr, "0x%x:0x%x", hdrload->ramdisk_addr, hdrload->ramdisk_size);
	sprintf(fdt_addr, "0x%x", hdrload->second_addr);

	if (avb_out_data != NULL)
		avb_slot_verify_data_free(avb_out_data);

	do_bootm(NULL, 0, 4, bootm_args);

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
#ifdef CONFIG_SECURE_BOOT
#define IVT_SIZE 0x20
#define CSF_PAD_SIZE CONFIG_CSF_SIZE
/* Max of bootimage size to be 16MB */
#define MAX_ANDROID_BOOT_AUTH_SIZE 0x1000000
/* Size appended to boot.img with boot_signer */
#define BOOTIMAGE_SIGNATURE_SIZE 0x100
#endif
	int i = 0;

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
		struct blk_desc *dev_desc = NULL;
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
			goto fail;
		}

		if (mmc->block_dev.block_read(dev_desc, pte->start,
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

#ifdef CONFIG_SECURE_BOOT
		/* Default boot.img should be padded to 0x1000
		   before appended with IVT&CSF data. Set the threshold of
		   boot image for athendication as 16MB
		*/
		image_size += BOOTIMAGE_SIGNATURE_SIZE;
		image_size = ALIGN(image_size, 0x1000);
		if (image_size > MAX_ANDROID_BOOT_AUTH_SIZE) {
			printf("The image size is too large for athenticated boot!\n");
			return 1;
		}
		/* Make sure all data boot.img + IVT + CSF been read to memory */
		bootimg_sectors = image_size/512 +
			ALIGN(IVT_SIZE + CSF_PAD_SIZE, 512)/512;
#endif

		if (mmc->block_dev.block_read(dev_desc, pte->start,
					bootimg_sectors,
					(void *)load_addr) < 0) {
			printf("boota: mmc failed to read kernel\n");
			goto fail;
		}
		/* flush cache after read */
		flush_cache((ulong)load_addr, bootimg_sectors * 512); /* FIXME */
		check_image_arm64  = image_arm64((void *)(load_addr + hdr->page_size));
		addr = load_addr;
#ifdef CONFIG_FASTBOOT_LOCK
		int verifyresult = -1;
#endif

#ifdef CONFIG_SECURE_BOOT
		extern uint32_t authenticate_image(uint32_t ddr_start,
				uint32_t image_size);

		if (authenticate_image(load_addr, image_size)) {
			printf("Authenticate OK\n");
#ifdef CONFIG_FASTBOOT_LOCK
			verifyresult = 0;
#endif
		} else {
			printf("Authenticate image Fail, Please check\n\n");
			/* For Android if the verify not passed we continue the boot process */
#ifdef CONFIG_FASTBOOT_LOCK
#ifndef CONFIG_ANDROID_SUPPORT
			return 1;
#endif
			verifyresult = 1;
#endif
		}
#endif /*CONFIG_SECURE_BOOT*/
#ifdef CONFIG_FASTBOOT_LOCK
		int lock_status = fastboot_get_lock_stat();
		if (lock_status == FASTBOOT_LOCK_ERROR) {
			printf("In boota get fastboot lock status error. Set lock status\n");
			fastboot_set_lock_stat(FASTBOOT_LOCK);
		}
		display_lock(fastboot_get_lock_stat(), verifyresult);
#endif

		sector = pte->start + (hdr->page_size / 512);
		if (check_image_arm64) {
			if (mmc->block_dev.block_read(dev_desc, sector,
								(hdr->kernel_size / 512) + 1,
								(void *)hdr->kernel_addr) < 0) {
				printf("boota: mmc failed to read kernel\n");
				goto fail;
			}
			flush_cache((ulong)hdr->kernel_addr, hdr->kernel_size);
			android_image_get_kernel(hdr, 0, NULL, NULL);
			addr = hdr->kernel_addr;
		}
		sector += ALIGN(hdr->kernel_size, hdr->page_size) / 512;
		if (mmc->block_dev.block_read(dev_desc, sector,
						(hdr->ramdisk_size / 512) + 1,
						(void *)hdr->ramdisk_addr) < 0) {
			printf("boota: mmc failed to read ramdisk\n");
			goto fail;
		}
		/* flush cache after read */
		flush_cache((ulong)hdr->ramdisk_addr, ((hdr->ramdisk_size / 512) + 1) * 512); /* FIXME */

#ifdef CONFIG_OF_LIBFDT
		/* load the dtb file */
		if (hdr->second_size && hdr->second_addr) {
			sector += ALIGN(hdr->ramdisk_size, hdr->page_size) / 512;
			if (mmc->block_dev.block_read(dev_desc, sector,
						(hdr->second_size / 512) + 1,
						(void *)hdr->second_addr) < 0) {
				printf("boota: mmc failed to dtb\n");
				goto fail;
			}
			/* flush cache after read */
			flush_cache((ulong)hdr->second_addr, ((hdr->second_size / 512) + 1) * 512); /* FIXME */
		}
#endif /*CONFIG_OF_LIBFDT*/

#else /*! CONFIG_MMC*/
		return -1;
#endif /*! CONFIG_MMC*/
	} else {
		unsigned raddr, end;
#ifdef CONFIG_OF_LIBFDT
		unsigned fdtaddr = 0;
#endif

		/* set this aside somewhere safe */
		memcpy(hdr, (void *)addr, sizeof(*hdr));

		if (android_image_check_header(hdr)) {
			printf("boota: bad boot image magic\n");
			return 1;
		}

		bootimg_print_image_hdr(hdr);

		image_size = hdr->page_size +
			ALIGN(hdr->kernel_size, hdr->page_size) +
			ALIGN(hdr->ramdisk_size, hdr->page_size) +
			ALIGN(hdr->second_size, hdr->page_size);

#ifdef CONFIG_SECURE_BOOT
		image_size = image_size + BOOTIMAGE_SIGNATURE_SIZE;
		if (image_size > MAX_ANDROID_BOOT_AUTH_SIZE) {
			printf("The image size is too large for athenticated boot!\n");
			return 1;
		}
#endif /*CONFIG_SECURE_BOOT*/

#ifdef CONFIG_SECURE_BOOT
		extern uint32_t authenticate_image(uint32_t ddr_start,
				uint32_t image_size);

		if (authenticate_image(addr, image_size)) {
			printf("Authenticate OK\n");
		} else {
			printf("Authenticate image Fail, Please check\n\n");
			return 1;
		}
#endif

		raddr = addr + hdr->page_size;
		raddr += ALIGN(hdr->kernel_size, hdr->page_size);
		end = raddr + hdr->ramdisk_size;
#ifdef CONFIG_OF_LIBFDT
		if (hdr->second_size) {
			fdtaddr = raddr + ALIGN(hdr->ramdisk_size, hdr->page_size);
			end = fdtaddr + hdr->second_size;
		}
#endif /*CONFIG_OF_LIBFDT*/

		if (raddr != hdr->ramdisk_addr) {
			/*check overlap*/
			if (((hdr->ramdisk_addr >= addr) &&
					(hdr->ramdisk_addr <= end)) ||
				((addr >= hdr->ramdisk_addr) &&
					(addr <= hdr->ramdisk_addr + hdr->ramdisk_size))) {
				printf("Fail: boota address overlap with ramdisk address\n");
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
				printf("Fail: boota address overlap with FDT address\n");
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


	char boot_addr_start[12];
	char ramdisk_addr[25];
	char fdt_addr[12];
	char *boot_args[] = { NULL, boot_addr_start, ramdisk_addr, fdt_addr};
	if (check_image_arm64)
		boot_args[0] = "booti";
	else
		boot_args[0] = "bootm";

	sprintf(boot_addr_start, "0x%lx", addr);
	sprintf(ramdisk_addr, "0x%x:0x%x", hdr->ramdisk_addr, hdr->ramdisk_size);
	sprintf(fdt_addr, "0x%x", hdr->second_addr);
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
#endif

static void rx_handler_command(struct usb_ep *ep, struct usb_request *req);


static char *fb_response_str;

void fastboot_fail(const char *reason)
{
	strncpy(fb_response_str, "FAIL\0", 5);
	strncat(fb_response_str, reason, FASTBOOT_RESPONSE_LEN - 4 - 1);
}

void fastboot_okay(const char *reason)
{
	strncpy(fb_response_str, "OKAY\0", 5);
	strncat(fb_response_str, reason, FASTBOOT_RESPONSE_LEN - 4 - 1);
}

static void fastboot_complete(struct usb_ep *ep, struct usb_request *req)
{
	int status = req->status;
	if (!status)
		return;
	printf("status: %d ep '%s' trans: %d\n", status, ep->name, req->actual);
}

static int fastboot_bind(struct usb_configuration *c, struct usb_function *f)
{
	int id;
	struct usb_gadget *gadget = c->cdev->gadget;
	struct f_fastboot *f_fb = func_to_fastboot(f);
	const char *s;

	/* DYNAMIC interface numbers assignments */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	interface_desc.bInterfaceNumber = id;

	id = usb_string_id(c->cdev);
	if (id < 0)
		return id;
	fastboot_string_defs[0].id = id;
	interface_desc.iInterface = id;

	f_fb->in_ep = usb_ep_autoconfig(gadget, &fs_ep_in);
	if (!f_fb->in_ep)
		return -ENODEV;
	f_fb->in_ep->driver_data = c->cdev;

	f_fb->out_ep = usb_ep_autoconfig(gadget, &fs_ep_out);
	if (!f_fb->out_ep)
		return -ENODEV;
	f_fb->out_ep->driver_data = c->cdev;

	f->descriptors = fb_fs_function;

	if (gadget_is_dualspeed(gadget)) {
		/* Assume endpoint addresses are the same for both speeds */
		hs_ep_in.bEndpointAddress = fs_ep_in.bEndpointAddress;
		hs_ep_out.bEndpointAddress = fs_ep_out.bEndpointAddress;
		/* copy HS descriptors */
		f->hs_descriptors = fb_hs_function;
	}

	s = getenv("serial#");
	if (s)
		g_dnl_set_serialnumber((char *)s);

	return 0;
}

static void fastboot_unbind(struct usb_configuration *c, struct usb_function *f)
{
	memset(fastboot_func, 0, sizeof(*fastboot_func));
}

static void fastboot_disable(struct usb_function *f)
{
	int i = 0;
	struct f_fastboot *f_fb = func_to_fastboot(f);

	usb_ep_disable(f_fb->out_ep);
	usb_ep_disable(f_fb->in_ep);

	if (f_fb->out_req) {
		free(f_fb->out_req->buf);
		usb_ep_free_request(f_fb->out_ep, f_fb->out_req);
		f_fb->out_req = NULL;
	}
	if (f_fb->in_req) {
		free(f_fb->in_req->buf);
		usb_ep_free_request(f_fb->in_ep, f_fb->in_req);
		f_fb->in_req = NULL;
	}
	for (i = 0; i < MAX_REQ_NUM; i++) {
		if (f_fb->in_req_more[i]) {
			usb_ep_free_request(f_fb->in_ep, f_fb->in_req_more[i]);
		}
	}
}

static struct usb_request *fastboot_start_ep(struct usb_ep *ep)
{
	struct usb_request *req;

	req = usb_ep_alloc_request(ep, 0);
	if (!req)
		return NULL;

	req->length = EP_BUFFER_SIZE;
	req->buf = memalign(CONFIG_SYS_CACHELINE_SIZE, EP_BUFFER_SIZE);
	if (!req->buf) {
		usb_ep_free_request(ep, req);
		return NULL;
	}

	memset(req->buf, 0, req->length);
	return req;
}

static int fastboot_set_alt(struct usb_function *f,
			    unsigned interface, unsigned alt)
{
	int i, ret;
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_gadget *gadget = cdev->gadget;
	struct f_fastboot *f_fb = func_to_fastboot(f);
	const struct usb_endpoint_descriptor *d;

	debug("%s: func: %s intf: %d alt: %d\n",
	      __func__, f->name, interface, alt);

	d = fb_ep_desc(gadget, &fs_ep_out, &hs_ep_out);
	ret = usb_ep_enable(f_fb->out_ep, d);
	if (ret) {
		puts("failed to enable out ep\n");
		return ret;
	}

	f_fb->out_req = fastboot_start_ep(f_fb->out_ep);
	if (!f_fb->out_req) {
		puts("failed to alloc out req\n");
		ret = -EINVAL;
		goto err;
	}
	f_fb->out_req->complete = rx_handler_command;

	d = fb_ep_desc(gadget, &fs_ep_in, &hs_ep_in);
	ret = usb_ep_enable(f_fb->in_ep, d);
	if (ret) {
		puts("failed to enable in ep\n");
		goto err;
	}

	f_fb->in_req = fastboot_start_ep(f_fb->in_ep);
	if (!f_fb->in_req) {
		puts("failed alloc req in\n");
		ret = -EINVAL;
		goto err;
	}
	f_fb->in_req->complete = fastboot_complete;

	for (i = 0; i < MAX_REQ_NUM; i++) {
		f_fb->in_req_more[i] = fastboot_start_ep(f_fb->in_ep);
		if (!f_fb->in_req_more[i]) {
			puts("failed alloc req in\n");
			ret = -EINVAL;
			goto err;
		}
		f_fb->in_req_more[i]->complete = fastboot_complete;
	}

	ret = usb_ep_queue(f_fb->out_ep, f_fb->out_req, 0);
	if (ret)
		goto err;

	return 0;
err:
	fastboot_disable(f);
	return ret;
}

static int fastboot_add(struct usb_configuration *c)
{
	struct f_fastboot *f_fb = fastboot_func;
	int status;

	debug("%s: cdev: 0x%p\n", __func__, c->cdev);

	if (!f_fb) {
		f_fb = memalign(CONFIG_SYS_CACHELINE_SIZE, sizeof(*f_fb));
		if (!f_fb)
			return -ENOMEM;

		fastboot_func = f_fb;
		memset(f_fb, 0, sizeof(*f_fb));
	}

	f_fb->usb_function.name = "f_fastboot";
	f_fb->usb_function.bind = fastboot_bind;
	f_fb->usb_function.unbind = fastboot_unbind;
	f_fb->usb_function.set_alt = fastboot_set_alt;
	f_fb->usb_function.disable = fastboot_disable;
	f_fb->usb_function.strings = fastboot_strings;

	status = usb_add_function(c, &f_fb->usb_function);
	if (status) {
		free(f_fb);
		fastboot_func = f_fb;
	}

	return status;
}
DECLARE_GADGET_BIND_CALLBACK(usb_dnl_fastboot, fastboot_add);

static int fastboot_tx_write_more(const char *buffer)
{
	int next_req = fastboot_func->next_req;
	struct usb_request *in_req = fastboot_func->in_req_more[next_req];
	unsigned int buffer_size = strlen(buffer);
	int ret = 0;

	memcpy(in_req->buf, buffer, buffer_size);
	in_req->length = buffer_size;

	ret = usb_ep_queue(fastboot_func->in_ep, in_req, 0);
	if (ret)
		printf("Error %d on queue\n", ret);

	if (in_req == fastboot_func->in_req_more[MAX_REQ_NUM-1]) {
		fastboot_func->next_req = 0;
		return -EINVAL;
	} else {
		fastboot_func->next_req++;
	}

	return ret;
}

static int fastboot_tx_write(const char *buffer, unsigned int buffer_size)
{
	struct usb_request *in_req = fastboot_func->in_req;
	int ret;

	memcpy(in_req->buf, buffer, buffer_size);
	in_req->length = buffer_size;

	usb_ep_dequeue(fastboot_func->in_ep, in_req);

	ret = usb_ep_queue(fastboot_func->in_ep, in_req, 0);
	if (ret)
		printf("Error %d on queue\n", ret);
	return 0;
}

int fastboot_tx_write_str(const char *buffer)
{
	return fastboot_tx_write(buffer, strlen(buffer));
}

static void compl_do_reset(struct usb_ep *ep, struct usb_request *req)
{
	do_reset(NULL, 0, 0, NULL);
}

int __weak fb_set_reboot_flag(void)
{
	return -ENOSYS;
}

static void cb_reboot(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	if (!strcmp_l1("reboot-bootloader", cmd)) {
		if (fb_set_reboot_flag()) {
			fastboot_tx_write_str("FAILCannot set reboot flag");
			return;
		}
	}
	fastboot_func->in_req->complete = compl_do_reset;
	fastboot_tx_write_str("OKAY");
}

static int strcmp_l1(const char *s1, const char *s2)
{
	if (!s1 || !s2)
		return -1;
	return strncmp(s1, s2, strlen(s1));
}

bool is_slotvar(char *cmd)
{
	assert(cmd != NULL);
	if (!strcmp_l1("has-slot:", cmd) ||
		!strcmp_l1("slot-successful:", cmd) ||
		!strcmp_l1("slot-count", cmd) ||
		!strcmp_l1("slot-suffixes", cmd) ||
		!strcmp_l1("current-slot", cmd) ||
		!strcmp_l1("slot-unbootable:", cmd) ||
		!strcmp_l1("slot-retry-count:", cmd))
			return true;
	return false;
}

static char *get_serial(void)
{
#ifdef CONFIG_SERIAL_TAG
	struct tag_serialnr serialnr;
	static char serial[32];
	get_board_serial(&serialnr);
	sprintf(serial, "%08x%08x", serialnr.high,      serialnr.low);
	return serial;
#else
	return NULL;
#endif
}

#if !defined(PRODUCT_NAME)
#define PRODUCT_NAME "NXP i.MX"
#endif

#if !defined(VARIANT_NAME)
#define VARIANT_NAME "NXP i.MX"
#endif

static int get_block_size(void) {
	int mmc_no = 0;
	struct blk_desc *dev_desc;
	mmc_no = fastboot_devinfo.dev_id;
	dev_desc = blk_get_dev("mmc", mmc_no);
	if (NULL == dev_desc) {
		printf("** Block device MMC %d not supported\n",
			mmc_no);
		return 0;
	}
	return dev_desc->blksz;
}

static bool is_exist(char (*partition_base_name)[16], char *buffer, int count)
{
	int n;

	for (n = 0; n < count; n++) {
		if (!strcmp(partition_base_name[n],buffer))
			return true;
	}
	return false;
}
/*get partition base name from gpt without "_a/_b"*/
static int get_partition_base_name(char (*partition_base_name)[16])
{
	int n = 0;
	int count = 0;
	char *ptr1, *ptr2;
	char buffer[16];

	for (n = 0; n < g_pcount; n++) {
		strcpy(buffer,g_ptable[n].name);
		ptr1 = strstr(buffer, "_a");
		ptr2 = strstr(buffer, "_b");
		if (ptr1 != NULL) {
			*ptr1 = '\0';
			if (!is_exist(partition_base_name,buffer,count)) {
				strcpy(partition_base_name[count++],buffer);
			}
		} else if (ptr2 != NULL) {
			*ptr2 = '\0';
			if (!is_exist(partition_base_name,buffer,count)) {
				strcpy(partition_base_name[count++],buffer);
			}
		} else {
			strcpy(partition_base_name[count++],buffer);
		}
	}
	return count;
}

static bool is_slot(void)
{
	char slot_suffix[2][5] = {"_a","_b"};
	int n;

	for (n = 0; n < g_pcount; n++) {
		if (strstr(g_ptable[n].name, slot_suffix[0]) ||
		strstr(g_ptable[n].name, slot_suffix[1]))
			return true;
	}
	return false;
}

static int get_single_var(char *cmd, char *response)
{
	char *str = cmd;
	size_t chars_left;
	const char *s;
	int mmc_no = 0;
	struct blk_desc *dev_desc;

	chars_left = FASTBOOT_RESPONSE_LEN - strlen(response) - 1;

	if ((str = strstr(cmd, "partition-size:"))) {
		str +=strlen("partition-size:");
		struct fastboot_ptentry* fb_part;
		fb_part = fastboot_flash_find_ptn(str);
		if (!fb_part) {
			strncat(response, "Wrong partition name.", chars_left);
			return -1;
		} else {
			snprintf(response + strlen(response), chars_left, "0x%x", fb_part->length * get_block_size());
		}
	} else if ((str = strstr(cmd, "partition-type:"))) {
		str +=strlen("partition-type:");
		struct fastboot_ptentry* fb_part;
		fb_part = fastboot_flash_find_ptn(str);
		if (!fb_part) {
			strncat(response, "Wrong partition name.", chars_left);
			return -1;
		} else {
			strncat(response, fb_part->fstype, chars_left);
		}
	} else if (!strcmp_l1("version-baseband", cmd)) {
		strncat(response, "N/A", chars_left);
	} else if (!strcmp_l1("version-bootloader", cmd) ||
		!strcmp_l1("bootloader-version", cmd)) {
		strncat(response, U_BOOT_VERSION, chars_left);
	} else if (!strcmp_l1("version", cmd)) {
		strncat(response, FASTBOOT_VERSION, chars_left);
	} else if (!strcmp_l1("battery-voltage", cmd)) {
		strncat(response, "0mV", chars_left);
	} else if (!strcmp_l1("battery-soc-ok", cmd)) {
		strncat(response, "yes", chars_left);
	} else if (!strcmp_l1("variant", cmd)) {
		strncat(response, VARIANT_NAME, chars_left);
	} else if (!strcmp_l1("off-mode-charge", cmd)) {
		strncat(response, "1", chars_left);
	} else if (!strcmp_l1("downloadsize", cmd) ||
		!strcmp_l1("max-download-size", cmd)) {

		snprintf(response + strlen(response), chars_left, "0x%x", CONFIG_FASTBOOT_BUF_SIZE);
	} else if (!strcmp_l1("erase-block-size", cmd)) {
		mmc_no = fastboot_devinfo.dev_id;
		dev_desc = blk_get_dev("mmc", mmc_no);
		snprintf(response + strlen(response), chars_left, "0x%x", (unsigned int)dev_desc->blksz);
	} else if (!strcmp_l1("logical-block-size", cmd)) {
		mmc_no = fastboot_devinfo.dev_id;
		dev_desc = blk_get_dev("mmc", mmc_no);
		snprintf(response + strlen(response), chars_left, "0x%x", (unsigned int)dev_desc->blksz);
	} else if (!strcmp_l1("serialno", cmd)) {
		s = get_serial();
		if (s)
			strncat(response, s, chars_left);
		else {
			strncat(response, "FAILValue not set", chars_left);
			return -1;
		}
	} else if (!strcmp_l1("product", cmd)) {
		strncat(response, PRODUCT_NAME, chars_left);
	}
#ifdef CONFIG_FASTBOOT_LOCK
	else if (!strcmp_l1("secure", cmd)) {
		strncat(response, FASTBOOT_VAR_YES, chars_left);
	} else if (!strcmp_l1("unlocked",cmd)){
		int status = fastboot_get_lock_stat();
		if (status == FASTBOOT_UNLOCK) {
			strncat(response, FASTBOOT_VAR_YES, chars_left);
		} else {
			strncat(response, FASTBOOT_VAR_NO, chars_left);
		}
	}
#else
	else if (!strcmp_l1("secure", cmd)) {
		strncat(response, FASTBOOT_VAR_NO, chars_left);
	} else if (!strcmp_l1("unlocked",cmd)) {
		strncat(response, FASTBOOT_VAR_NO, chars_left);
	}
#endif
	else if (is_slotvar(cmd)) {
#ifdef CONFIG_AVB_SUPPORT
		if (get_slotvar_avb(&fsl_avb_ab_ops, cmd,
				response + strlen(response), chars_left + 1) < 0)
			return -1;
#else
		strncat(response, FASTBOOT_VAR_NO, chars_left);
#endif
	}
	else {
		char envstr[32];

		snprintf(envstr, sizeof(envstr) - 1, "fastboot.%s", cmd);
		s = getenv(envstr);
		if (s) {
			strncat(response, s, chars_left);
		} else {
			sprintf(response,"FAILunknow variable:%s",cmd);
			printf("WARNING: unknown variable: %s\n", cmd);
			return -1;
		}
	}
	return 0;
}

static void cb_getvar(struct usb_ep *ep, struct usb_request *req)
{
	int n = 0;
	int status = 0;
	int count = 0;
	char *cmd = req->buf;
	char var_name[FASTBOOT_RESPONSE_LEN - 1];
	char partition_base_name[MAX_PTN][16];
	char slot_suffix[2][5] = {"_a","_b"};
	char response[FASTBOOT_RESPONSE_LEN - 1];

	strsep(&cmd, ":");
	if (!cmd) {
		error("missing variable");
		fastboot_tx_write_str("FAILmissing var");
	}

	if (!strcmp_l1("all", cmd)) {

		memset(response, '\0', FASTBOOT_RESPONSE_LEN - 1);


		/* get common variables */
		for (n = 0; n < FASTBOOT_COMMON_VAR_NUM; n++) {
			snprintf(response, sizeof(response), "INFO%s:", fastboot_common_var[n]);
			get_single_var(fastboot_common_var[n], response);
			fastboot_tx_write_more(response);
		}

		/* get partition type */
		for (n = 0; n < g_pcount; n++) {
			snprintf(response, sizeof(response), "INFOpartition-type:%s:", g_ptable[n].name);
			snprintf(var_name, sizeof(var_name), "partition-type:%s", g_ptable[n].name);
			get_single_var(var_name, response);
			fastboot_tx_write_more(response);
		}
		/* get partition size */
		for (n = 0; n < g_pcount; n++) {
			snprintf(response, sizeof(response), "INFOpartition-size:%s:", g_ptable[n].name);
			snprintf(var_name, sizeof(var_name), "partition-size:%s", g_ptable[n].name);
			get_single_var(var_name,response);
			fastboot_tx_write_more(response);
		}
		/* slot related variables */
		if (is_slot()) {
			/* get has-slot variables */
			count = get_partition_base_name(partition_base_name);
			for (n = 0; n < count; n++) {
				snprintf(response, sizeof(response), "INFOhas-slot:%s:", partition_base_name[n]);
				snprintf(var_name, sizeof(var_name), "has-slot:%s", partition_base_name[n]);
				get_single_var(var_name,response);
				fastboot_tx_write_more(response);
			}
			/* get current slot */
			strncpy(response, "INFOcurrent-slot:", sizeof(response));
			get_single_var("current-slot", response);
			fastboot_tx_write_more(response);
			/* get slot count */
			strncpy(response, "INFOslot-count:", sizeof(response));
			get_single_var("slot-count", response);
			fastboot_tx_write_more(response);
			/* get slot-successful variable */
			for (n = 0; n < 2; n++) {
				snprintf(response, sizeof(response), "INFOslot-successful:%s:", slot_suffix[n]);
				snprintf(var_name, sizeof(var_name), "slot-successful:%s", slot_suffix[n]);
				get_single_var(var_name, response);
				fastboot_tx_write_more(response);
			}
			/*get slot-unbootable variable*/
			for (n = 0; n < 2; n++) {
				snprintf(response, sizeof(response), "INFOslot-unbootable:%s:", slot_suffix[n]);
				snprintf(var_name, sizeof(var_name), "slot-unbootable:%s", slot_suffix[n]);
				get_single_var(var_name, response);
				fastboot_tx_write_more(response);
			}
			/*get slot-retry-count variable*/
			for (n = 0; n < 2; n++) {
				snprintf(response, sizeof(response), "INFOslot-retry-count:%s:", slot_suffix[n]);
				snprintf(var_name, sizeof(var_name), "slot-retry-count:%s", slot_suffix[n]);
				get_single_var(var_name, response);
				fastboot_tx_write_more(response);
			}
		}

		strncpy(response, "OKAYDone!", 10);
		fastboot_tx_write_more(response);

		return;
	} else {

		strncpy(response, "OKAY", 5);
		status = get_single_var(cmd, response);
		if (status != 0) {
			strncpy(response, "FAIL", 5);
		}
		fastboot_tx_write_str(response);
		return;
	}
}

static unsigned int rx_bytes_expected(struct usb_ep *ep)
{
	int rx_remain = download_size - download_bytes;
	unsigned int rem;
	unsigned int maxpacket = ep->maxpacket;

	if (rx_remain <= 0)
		return 0;
	else if (rx_remain > EP_BUFFER_SIZE)
		return EP_BUFFER_SIZE;

	/*
	 * Some controllers e.g. DWC3 don't like OUT transfers to be
	 * not ending in maxpacket boundary. So just make them happy by
	 * always requesting for integral multiple of maxpackets.
	 * This shouldn't bother controllers that don't care about it.
	 */
	rem = rx_remain % maxpacket;
	if (rem > 0)
		rx_remain = rx_remain + (maxpacket - rem);

	return rx_remain;
}

#define BYTES_PER_DOT	0x20000
static void rx_handler_dl_image(struct usb_ep *ep, struct usb_request *req)
{
	char response[FASTBOOT_RESPONSE_LEN];
	unsigned int transfer_size = download_size - download_bytes;
	const unsigned char *buffer = req->buf;
	unsigned int buffer_size = req->actual;
	unsigned int pre_dot_num, now_dot_num;

	if (req->status != 0) {
		printf("Bad status: %d\n", req->status);
		return;
	}

	if (buffer_size < transfer_size)
		transfer_size = buffer_size;

	memcpy((void *)CONFIG_FASTBOOT_BUF_ADDR + download_bytes,
	       buffer, transfer_size);

	pre_dot_num = download_bytes / BYTES_PER_DOT;
	download_bytes += transfer_size;
	now_dot_num = download_bytes / BYTES_PER_DOT;

	if (pre_dot_num != now_dot_num) {
		putc('.');
		if (!(now_dot_num % 74))
			putc('\n');
	}

	/* Check if transfer is done */
	if (download_bytes >= download_size) {
		/*
		 * Reset global transfer variable, keep download_bytes because
		 * it will be used in the next possible flashing command
		 */
		download_size = 0;
		req->complete = rx_handler_command;
		req->length = EP_BUFFER_SIZE;

		strcpy(response, "OKAY");
		fastboot_tx_write_str(response);

		printf("\ndownloading of %d bytes finished\n", download_bytes);

#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_FASTBOOT_STORAGE_NAND
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
#endif
	} else {
		req->length = rx_bytes_expected(ep);
	}

	req->actual = 0;
	usb_ep_queue(ep, req, 0);
}

static void cb_download(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char response[FASTBOOT_RESPONSE_LEN];

	strsep(&cmd, ":");
	download_size = simple_strtoul(cmd, NULL, 16);
	download_bytes = 0;

	printf("Starting download of %d bytes\n", download_size);

	if (0 == download_size) {
		strcpy(response, "FAILdata invalid size");
	} else if (download_size > CONFIG_FASTBOOT_BUF_SIZE) {
		download_size = 0;
		strcpy(response, "FAILdata too large");
	} else {
		sprintf(response, "DATA%08x", download_size);
		req->complete = rx_handler_dl_image;
		req->length = rx_bytes_expected(ep);
	}
	fastboot_tx_write_str(response);
}

static void do_bootm_on_complete(struct usb_ep *ep, struct usb_request *req)
{
	char boot_addr_start[12];
#ifdef CONFIG_FSL_FASTBOOT
	char *bootm_args[] = { "boota", boot_addr_start, NULL };
#else
	char *bootm_args[] = { "bootm", boot_addr_start, NULL };
#endif

	puts("Booting kernel..\n");

	sprintf(boot_addr_start, "0x%lx", load_addr);
	do_bootm(NULL, 0, 2, bootm_args);

	/* This only happens if image is somehow faulty so we start over */
	do_reset(NULL, 0, 0, NULL);
}

static void cb_boot(struct usb_ep *ep, struct usb_request *req)
{
	fastboot_func->in_req->complete = do_bootm_on_complete;
	fastboot_tx_write_str("OKAY");
}

static void do_exit_on_complete(struct usb_ep *ep, struct usb_request *req)
{
	g_dnl_trigger_detach();
}

static void cb_continue(struct usb_ep *ep, struct usb_request *req)
{
	fastboot_func->in_req->complete = do_exit_on_complete;
	fastboot_tx_write_str("OKAY");
}


#ifdef CONFIG_FASTBOOT_LOCK

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

static FbLockState do_fastboot_unlock(bool force)
{
	int status;
	if (force)
		set_fastboot_lock_disable();
	if ((fastboot_lock_enable() == FASTBOOT_UL_ENABLE) || force) {
		printf("It is able to unlock device. %d\n",fastboot_lock_enable());
		if (fastboot_get_lock_stat() == FASTBOOT_UNLOCK) {
			printf("The device is already unlocked\n");
			return FASTBOOT_UNLOCK;
		}
		status = fastboot_set_lock_stat(FASTBOOT_UNLOCK);
		if (status < 0)
			return FASTBOOT_LOCK_ERROR;

		printf("Start /data wipe process....\n");
		fastboot_wipe_data_partition();
		printf("Wipe /data completed.\n");

#ifdef CONFIG_AVB_SUPPORT
		printf("Start stored_rollback_index wipe process....\n");
		rbkidx_erase();
		printf("Wipe stored_rollback_index completed.\n");
#endif
	} else {
		printf("It is not able to unlock device.");
		return FASTBOOT_LOCK_ERROR;
	}

	return FASTBOOT_UNLOCK;
}

static FbLockState do_fastboot_lock(void)
{
	int status;
	if (fastboot_get_lock_stat() == FASTBOOT_LOCK) {
		printf("The device is already locked\n");
		return FASTBOOT_LOCK;
	}
	status = fastboot_set_lock_stat(FASTBOOT_LOCK);
	if (status < 0)
		return FASTBOOT_LOCK_ERROR;

	printf("Start /data wipe process....\n");
	fastboot_wipe_data_partition();
	printf("Wipe /data completed.\n");

	return FASTBOOT_LOCK;
}

static void cb_flashing(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char response[FASTBOOT_RESPONSE_LEN];
	unsigned char len = strlen(cmd);
	FbLockState status;
	if (!strncmp(cmd + len - 15, "unlock_critical", 15)) {
		strcpy(response, "OKAY");
	} else if (!strncmp(cmd + len - 13, "lock_critical", 13)) {
		strcpy(response, "OKAY");
	} else if (!strncmp(cmd + len - 6, "unlock", 6)) {
		printf("flashing unlock.\n");
		status = do_fastboot_unlock(false);
		if (status != FASTBOOT_LOCK_ERROR)
			strcpy(response, "OKAY");
		else
			strcpy(response, "FAIL unlock device failed.");
	} else if (!strncmp(cmd + len - 4, "lock", 4)) {
		printf("flashing lock.\n");
		status = do_fastboot_lock();
		if (status != FASTBOOT_LOCK_ERROR)
			strcpy(response, "OKAY");
		else
			strcpy(response, "FAIL lock device failed.");
	} else {
		printf("Unknown flashing command:%s\n", cmd);
		strcpy(response, "FAIL command not defined");
	}
	fastboot_tx_write_str(response);
}

#endif

static int partition_table_valid(void)
{
	int status, mmc_no;
	struct blk_desc *dev_desc;
	disk_partition_t info;
	mmc_no = fastboot_devinfo.dev_id;
	dev_desc = blk_get_dev("mmc", mmc_no);
	if (dev_desc)
		status = part_get_info(dev_desc, 1, &info);
	else
		status = -1;
	return (status == 0);
}

#ifdef CONFIG_FASTBOOT_FLASH
static void cb_flash(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char response[FASTBOOT_RESPONSE_LEN];

	strsep(&cmd, ":");
	if (!cmd) {
		error("missing partition name");
		fastboot_tx_write_str("FAILmissing partition name");
		return;
	}

	/* initialize the response buffer */
	fb_response_str = response;

#ifdef CONFIG_FASTBOOT_LOCK
	int status;
	status = fastboot_get_lock_stat();

	if (status == FASTBOOT_LOCK) {
		error("device is LOCKed!\n");
		strcpy(response, "FAIL device is locked.");
		fastboot_tx_write_str(response);
		return;

	} else if (status == FASTBOOT_LOCK_ERROR) {
		error("write lock status into device!\n");
		fastboot_set_lock_stat(FASTBOOT_LOCK);
		strcpy(response, "FAIL device is locked.");
		fastboot_tx_write_str(response);
		return;
	}
#endif
	fastboot_fail("no flash device defined");

#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_FASTBOOT_LOCK
	int gpt_valid_pre = 0;
	int gpt_valid_pst = 0;
	if (strncmp(cmd, "gpt", 3) == 0)
		gpt_valid_pre = partition_table_valid();
#endif
	rx_process_flash(cmd);
#ifdef CONFIG_FASTBOOT_LOCK
	if (strncmp(cmd, "gpt", 3) == 0) {
		gpt_valid_pst = partition_table_valid();
		/* If gpt is valid, load partitons table into memory.
		   So if the next command is "fastboot reboot bootloader",
		   it can find the "misc" partition to r/w. */
		if(gpt_valid_pst)
			_fastboot_load_partitions();
		/* If gpt invalid -> valid, write unlock status, also wipe data. */
		if ((gpt_valid_pre == 0) && (gpt_valid_pst == 1))
			do_fastboot_unlock(true);
	}

#endif
#ifdef CONFIG_FASTBOOT_FLASH_NAND_DEV
	fb_nand_flash_write(cmd,
			    (void *)CONFIG_FASTBOOT_BUF_ADDR,
			    download_bytes);
#endif
#endif
	fastboot_tx_write_str(response);
}
#endif

#ifdef CONFIG_FASTBOOT_FLASH
static void cb_erase(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char response[FASTBOOT_RESPONSE_LEN];

	strsep(&cmd, ":");
	if (!cmd) {
		error("missing partition name");
		fastboot_tx_write_str("FAILmissing partition name");
		return;
	}

	/* initialize the response buffer */
	fb_response_str = response;

#ifdef CONFIG_FASTBOOT_LOCK
	FbLockState status;
	status = fastboot_get_lock_stat();
	if (status == FASTBOOT_LOCK) {
		error("device is LOCKed!\n");
		strcpy(response, "FAIL device is locked.");
		fastboot_tx_write_str(response);
		return;
	} else if (status == FASTBOOT_LOCK_ERROR) {
		error("write lock status into device!\n");
		fastboot_set_lock_stat(FASTBOOT_LOCK);
		strcpy(response, "FAIL device is locked.");
		fastboot_tx_write_str(response);
		return;
	}
#endif
	rx_process_erase(cmd, response);
	fastboot_tx_write_str(response);
}
#endif

#ifdef CONFIG_AVB_SUPPORT
static void cb_set_active_avb(struct usb_ep *ep, struct usb_request *req)
{
	AvbIOResult ret;
	int slot = 0;
	char *cmd = req->buf;

	strsep(&cmd, ":");
	if (!cmd) {
		error("missing slot suffix\n");
		fastboot_tx_write_str("FAILmissing slot suffix");
		return;
	}

	slot = slotidx_from_suffix(cmd);

	if (slot < 0) {
		fastboot_tx_write_str("FAILerr slot suffix");
		return;
	}

	ret = avb_ab_mark_slot_active(&fsl_avb_ab_ops, slot);
	if (ret != AVB_IO_RESULT_OK)
		fastboot_tx_write_str("avb IO error");
	else
		fastboot_tx_write_str("OKAY");

	return;
}
#endif /*CONFIG_AVB_SUPPORT*/

#ifdef CONFIG_FSL_FASTBOOT
static void cb_reboot_bootloader(struct usb_ep *ep, struct usb_request *req)
{
	fastboot_tx_write_str("OKAY");

	udelay(1000000);
	enable_fastboot_command();
	do_reset(NULL, 0, 0, NULL);
}
#endif

struct cmd_dispatch_info {
	char *cmd;
	void (*cb)(struct usb_ep *ep, struct usb_request *req);
};

static const struct cmd_dispatch_info cmd_dispatch_info[] = {
#ifdef CONFIG_FSL_FASTBOOT
	{
		.cmd = "reboot-bootloader",
		.cb = cb_reboot_bootloader,
	},
#endif
	{
		.cmd = "reboot",
		.cb = cb_reboot,
	}, {
		.cmd = "getvar:",
		.cb = cb_getvar,
	}, {
		.cmd = "download:",
		.cb = cb_download,
	}, {
		.cmd = "boot",
		.cb = cb_boot,
	}, {
		.cmd = "continue",
		.cb = cb_continue,
	},
#ifdef CONFIG_FASTBOOT_LOCK
	{
		.cmd = "flashing",
		.cb = cb_flashing,
	},
#endif
#ifdef CONFIG_FASTBOOT_FLASH
	{
		.cmd = "flash",
		.cb = cb_flash,
	}, {
		.cmd = "erase",
		.cb = cb_erase,
	},
#endif
#ifdef CONFIG_FASTBOOT_LOCK
	{
		.cmd = "oem",
		.cb = cb_flashing,
	},
#endif
#ifdef CONFIG_AVB_SUPPORT
	{
		.cmd = "set_active",
		.cb = cb_set_active_avb,
	},
#endif
};

static void rx_handler_command(struct usb_ep *ep, struct usb_request *req)
{
	char *cmdbuf = req->buf;
	void (*func_cb)(struct usb_ep *ep, struct usb_request *req) = NULL;
	int i;
	/*init the next_req if need muti USB request*/
	fastboot_func->next_req = 0;

	if (req->status != 0 || req->length == 0)
		return;

	for (i = 0; i < ARRAY_SIZE(cmd_dispatch_info); i++) {
		if (!strcmp_l1(cmd_dispatch_info[i].cmd, cmdbuf)) {
			func_cb = cmd_dispatch_info[i].cb;
			break;
		}
	}

	if (!func_cb) {
		error("unknown command: %s", cmdbuf);
		fastboot_tx_write_str("FAILunknown command");
	} else {
		if (req->actual < req->length) {
			u8 *buf = (u8 *)req->buf;
			buf[req->actual] = 0;
			func_cb(ep, req);
		} else {
			error("buffer overflow");
			fastboot_tx_write_str("FAILbuffer overflow");
		}
	}

	*cmdbuf = '\0';
	req->actual = 0;
	usb_ep_queue(ep, req, 0);
}
