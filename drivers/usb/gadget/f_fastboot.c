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
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <config.h>
#include <common.h>
#include <errno.h>
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

#ifdef CONFIG_FSL_FASTBOOT
#include <fsl_fastboot.h>
#include <mmc.h>
#include <android_image.h>
#include <asm/bootm.h>
#include <nand.h>
#include <aboot.h>
#ifdef CONFIG_ANDROID_RECOVERY
#include <recovery.h>
#endif
#endif

#define FASTBOOT_VERSION		"0.4"

#define FASTBOOT_INTERFACE_CLASS	0xff
#define FASTBOOT_INTERFACE_SUB_CLASS	0x42
#define FASTBOOT_INTERFACE_PROTOCOL	0x03

#define RX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0  (0x0200)
#define RX_ENDPOINT_MAXIMUM_PACKET_SIZE_1_1  (0x0040)
#define TX_ENDPOINT_MAXIMUM_PACKET_SIZE      (0x0040)

/* The 64 defined bytes plus \0 */
#define RESPONSE_LEN	(64 + 1)

#define EP_BUFFER_SIZE			4096

struct f_fastboot {
	struct usb_function usb_function;

	/* IN/OUT EP's and corresponding requests */
	struct usb_ep *in_ep, *out_ep;
	struct usb_request *in_req, *out_req;
};

static inline struct f_fastboot *func_to_fastboot(struct usb_function *f)
{
	return container_of(f, struct f_fastboot, usb_function);
}

static struct f_fastboot *fastboot_func;
static unsigned int download_size;
static unsigned int download_bytes;
static bool is_high_speed;

static struct usb_endpoint_descriptor fs_ep_in = {
	.bLength            = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType    = USB_DT_ENDPOINT,
	.bEndpointAddress   = USB_DIR_IN,
	.bmAttributes       = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize     = TX_ENDPOINT_MAXIMUM_PACKET_SIZE,
	.bInterval          = 0x00,
};

static struct usb_endpoint_descriptor fs_ep_out = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= RX_ENDPOINT_MAXIMUM_PACKET_SIZE_1_1,
	.bInterval		= 0x00,
};

static struct usb_endpoint_descriptor hs_ep_out = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= RX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0,
	.bInterval		= 0x00,
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

static struct usb_descriptor_header *fb_runtime_descs[] = {
	(struct usb_descriptor_header *)&interface_desc,
	(struct usb_descriptor_header *)&fs_ep_in,
	(struct usb_descriptor_header *)&hs_ep_out,
	NULL,
};

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
#define ANDROID_BOOTLOADER_OFFSET   0x400
#define ANDROID_BOOTLOADER_SIZE	    0xFFC00
#define ANDROID_KERNEL_OFFSET	    0x100000
#define ANDROID_KERNEL_SIZE	    0x500000
#define ANDROID_URAMDISK_OFFSET	    0x600000
#define ANDROID_URAMDISK_SIZE	    0x100000



#define MMC_SATA_BLOCK_SIZE 512
#define FASTBOOT_FBPARTS_ENV_MAX_LEN 1024
/* To support the Android-style naming of flash */
#define MAX_PTN		    16


/*pentry index internally*/
enum {
    PTN_MBR_INDEX = 0,
    PTN_BOOTLOADER_INDEX,
    PTN_KERNEL_INDEX,
    PTN_URAMDISK_INDEX,
    PTN_SYSTEM_INDEX,
    PTN_RECOVERY_INDEX,
    PTN_DATA_INDEX
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

static char result_str[RESPONSE_LEN];

void fastboot_fail(const char *s)
{
	strncpy(result_str, "FAIL\0", 5);
	strncat(result_str, s, RESPONSE_LEN - 4 - 1);
}

void fastboot_okay(const char *s)
{
	strncpy(result_str, "OKAY\0", 5);
	strncat(result_str, s, RESPONSE_LEN - 4 - 1);
}

#if defined(CONFIG_FASTBOOT_STORAGE_NAND)

static void process_flash_nand(const char *cmdbuf, char *response)
{
	if (download_bytes) {
		struct fastboot_ptentry *ptn;

		ptn = fastboot_flash_find_ptn(cmdbuf);
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
		ptn = fastboot_flash_find_ptn(cmdbuf);
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
				 || !strncmp(ptn->name,
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
		ptn = fastboot_flash_find_ptn(cmdbuf);
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
				write_sparse_image(dev_desc, &info, ptn->name,
						interface.transfer_buffer, download_bytes);
				strcpy(response, result_str);
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


static int rx_process_erase(const char *cmdbuf, char *response)
{
#if defined(CONFIG_FASTBOOT_STORAGE_NAND)
	struct fastboot_ptentry *ptn;

	ptn = fastboot_flash_find_ptn(cmdbuf);
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


static void parameters_setup(void)
{
	interface.nand_block_size = 0;
#if defined(CONFIG_FASTBOOT_STORAGE_NAND)
	nand_info_t *nand = &nand_info[0];
	if (nand)
		interface.nand_block_size = nand->writesize;
#endif
	interface.transfer_buffer =
				(unsigned char *)CONFIG_USB_FASTBOOT_BUF_ADDR;
	interface.transfer_buffer_size =
				CONFIG_USB_FASTBOOT_BUF_SIZE;
}

static struct fastboot_ptentry ptable[MAX_PTN];
static unsigned int pcount;
struct fastboot_device_info fastboot_devinfo;

/*
   Get mmc control number from passed string, eg, "mmc1" mean device 1. Only
   support "mmc0" to "mmc9" currently. It will be treated as device 0 for
   other string.
*/
static int _fastboot_get_mmc_no(char *env_str)
{
	int digit = 0;
	unsigned char a;

	if (env_str && (strlen(env_str) >= 4) &&
	    !strncmp(env_str, "mmc", 3)) {
		a = env_str[3];
		if (a >= '0' && a <= '9')
			digit = a - '0';
	}

	return digit;
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
		} else if (!strncmp(fastboot_env, "mmc", 3)) {
			fastboot_devinfo.type = DEV_MMC;
			fastboot_devinfo.dev_id = _fastboot_get_mmc_no(fastboot_env);
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
				      block_dev_desc_t *dev_desc,
				      struct fastboot_ptentry *ptable)
{
	disk_partition_t info;
	strcpy(ptable[ptable_index].name, name);

	if (get_partition_info(dev_desc,
			       mmc_dos_partition_index, &info)) {
		printf("Bad partition index:%d for partition:%s\n",
		       mmc_dos_partition_index, name);
		return -1;
	} else {
		ptable[ptable_index].start = info.start;
		ptable[ptable_index].length = info.size;
		ptable[ptable_index].partition_id = mmc_partition_index;
		ptable[ptable_index].partition_index = mmc_dos_partition_index;
	}
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
	block_dev_desc_t *dev_desc;
	struct fastboot_ptentry ptable[PTN_DATA_INDEX + 1];

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

		dev_desc = get_dev("mmc", mmc_no);
		if (NULL == dev_desc) {
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
		    sizeof(struct fastboot_ptentry) * (PTN_DATA_INDEX + 1));
	/* MBR */
	strcpy(ptable[PTN_MBR_INDEX].name, "mbr");
	ptable[PTN_MBR_INDEX].start = ANDROID_MBR_OFFSET / dev_desc->blksz;
	ptable[PTN_MBR_INDEX].length = ANDROID_MBR_SIZE / dev_desc->blksz;
	ptable[PTN_MBR_INDEX].partition_id = user_partition;
	/* Bootloader */
	strcpy(ptable[PTN_BOOTLOADER_INDEX].name, FASTBOOT_PARTITION_BOOTLOADER);
	ptable[PTN_BOOTLOADER_INDEX].start =
				ANDROID_BOOTLOADER_OFFSET / dev_desc->blksz;
	ptable[PTN_BOOTLOADER_INDEX].length =
				 ANDROID_BOOTLOADER_SIZE / dev_desc->blksz;
	ptable[PTN_BOOTLOADER_INDEX].partition_id = boot_partition;

	_fastboot_parts_add_ptable_entry(PTN_KERNEL_INDEX,
				   CONFIG_ANDROID_BOOT_PARTITION_MMC,
				   user_partition,
				   FASTBOOT_PARTITION_BOOT , dev_desc, ptable);
	_fastboot_parts_add_ptable_entry(PTN_RECOVERY_INDEX,
				   CONFIG_ANDROID_RECOVERY_PARTITION_MMC,
				   user_partition,
				   FASTBOOT_PARTITION_RECOVERY, dev_desc, ptable);
	_fastboot_parts_add_ptable_entry(PTN_SYSTEM_INDEX,
				   CONFIG_ANDROID_SYSTEM_PARTITION_MMC,
				   user_partition,
				   FASTBOOT_PARTITION_SYSTEM, dev_desc, ptable);
	_fastboot_parts_add_ptable_entry(PTN_DATA_INDEX,
				   CONFIG_ANDROID_DATA_PARTITION_MMC,
				   user_partition,
				   FASTBOOT_PARTITION_DATA, dev_desc, ptable);

	for (i = 0; i <= PTN_DATA_INDEX; i++)
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
					pcount = 0;
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
	pcount = 0;
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
	if (pcount < MAX_PTN) {
		memcpy(ptable + pcount, ptn, sizeof(struct fastboot_ptentry));
		pcount++;
	}
}

void fastboot_flash_dump_ptn(void)
{
	unsigned int n;
	for (n = 0; n < pcount; n++) {
		struct fastboot_ptentry *ptn = ptable + n;
		printf("ptn %d name='%s' start=%d len=%d\n",
			n, ptn->name, ptn->start, ptn->length);
	}
}


struct fastboot_ptentry *fastboot_flash_find_ptn(const char *name)
{
	unsigned int n;

	for (n = 0; n < pcount; n++) {
		/* Make sure a substring is not accepted */
		if (strlen(name) == strlen(ptable[n].name)) {
			if (0 == strcmp(ptable[n].name, name))
				return ptable + n;
		}
	}

	printf("can't find partition: %s, dump the partition table\n", name);
	fastboot_flash_dump_ptn();
	return 0;
}

struct fastboot_ptentry *fastboot_flash_get_ptn(unsigned int n)
{
	if (n < pcount)
		return ptable + n;
	else
		return 0;
}

unsigned int fastboot_flash_get_ptn_count(void)
{
	return pcount;
}

/*
 * CPU and board-specific fastboot initializations.  Aliased function
 * signals caller to move on
 */
static void __def_fastboot_setup(void)
{
	/*do nothing here*/
}
void board_fastboot_setup(void) \
	__attribute__((weak, alias("__def_fastboot_setup")));


void fastboot_setup(void)
{
	struct tag_serialnr serialnr;
	char serial[17];

	get_board_serial(&serialnr);
	sprintf(serial, "%u%u", serialnr.high, serialnr.low);
	g_dnl_set_serialnumber(serial);

	/*execute board relevant initilizations for preparing fastboot */
	board_fastboot_setup();

	/*get the fastboot dev*/
	_fastboot_setup_dev();

	/*check if we need to setup recovery*/
#ifdef CONFIG_ANDROID_RECOVERY
    check_recovery_mode();
#endif

	/*load partitions information for the fastboot dev*/
	_fastboot_load_partitions();

	parameters_setup();
}

/* export to lib_arm/board.c */
void check_fastboot(void)
{
	if (fastboot_check_and_clean_flag())
		run_command("fastboot", 0);
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
	printf("   cmdline:   %s%x\n", hdr->cmdline);

	for (i = 0; i < 8; i++)
		printf("   id[%d]:   0x%x\n", i, hdr->id[i]);
#endif
}

static struct andr_img_hdr boothdr __aligned(ARCH_DMA_MINALIGN);

/* boota <addr> [ mmc0 | mmc1 [ <partition> ] ] */
int do_boota(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong addr = 0;
	char *ptn = "boot";
	int mmcc = -1;
	struct andr_img_hdr *hdr = &boothdr;
    ulong image_size;
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
			printf("boota: cannot find '%d' mmc device\n", mmcc);
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
			printf("boota: cannot find '%s' partition\n", ptn);
			goto fail;
		}

		if (mmc->block_dev.block_read(mmcc, pte->start,
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

		if (mmc->block_dev.block_read(mmcc, pte->start,
					bootimg_sectors,
					(void *)load_addr) < 0) {
			printf("boota: mmc failed to read kernel\n");
			goto fail;
		}
		/* flush cache after read */
		flush_cache((ulong)load_addr, bootimg_sectors * 512); /* FIXME */

		addr = load_addr;

#ifdef CONFIG_SECURE_BOOT
		extern uint32_t authenticate_image(uint32_t ddr_start,
				uint32_t image_size);

		if (authenticate_image(load_addr, image_size)) {
			printf("Authenticate OK\n");
		} else {
			printf("Authenticate image Fail, Please check\n\n");
			return 1;
		}
#endif /*CONFIG_SECURE_BOOT*/

		sector = pte->start + (hdr->page_size / 512);
		sector += ALIGN(hdr->kernel_size, hdr->page_size) / 512;
		if (mmc->block_dev.block_read(mmcc, sector,
						(hdr->ramdisk_size / 512) + 1,
						(void *)hdr->ramdisk_addr) < 0) {
			printf("boota: mmc failed to read ramdisk\n");
			goto fail;
		}
		/* flush cache after read */
		flush_cache((ulong)hdr->ramdisk_addr, hdr->ramdisk_size); /* FIXME */

#ifdef CONFIG_OF_LIBFDT
		/* load the dtb file */
		if (hdr->second_size && hdr->second_addr) {
			sector += ALIGN(hdr->ramdisk_size, hdr->page_size) / 512;
			if (mmc->block_dev.block_read(mmcc, sector,
						(hdr->second_size / 512) + 1,
						(void *)hdr->second_addr) < 0) {
				printf("boota: mmc failed to dtb\n");
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

	char *bootm_args[] = { "bootm", boot_addr_start, ramdisk_addr, fdt_addr};

	sprintf(boot_addr_start, "0x%lx", addr);
	sprintf(ramdisk_addr, "0x%x:0x%x", hdr->ramdisk_addr, hdr->ramdisk_size);
	sprintf(fdt_addr, "0x%x", hdr->second_addr);

	do_bootm(NULL, 0, 4, bootm_args);

	/* This only happens if image is somehow faulty so we start over */
	do_reset(NULL, 0, 0, NULL);

	return 1;

fail:
#ifdef CONFIG_FSL_FASTBOOT
	return run_command("fastboot", 0);
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
#endif	/* CONFIG_CMD_BOOTA */
#endif

static void rx_handler_command(struct usb_ep *ep, struct usb_request *req);

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

	hs_ep_out.bEndpointAddress = fs_ep_out.bEndpointAddress;

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
	int ret;
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_gadget *gadget = cdev->gadget;
	struct f_fastboot *f_fb = func_to_fastboot(f);

	debug("%s: func: %s intf: %d alt: %d\n",
	      __func__, f->name, interface, alt);

	/* make sure we don't enable the ep twice */
	if (gadget->speed == USB_SPEED_HIGH) {
		ret = usb_ep_enable(f_fb->out_ep, &hs_ep_out);
		is_high_speed = true;
	} else {
		ret = usb_ep_enable(f_fb->out_ep, &fs_ep_out);
		is_high_speed = false;
	}
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

	ret = usb_ep_enable(f_fb->in_ep, &fs_ep_in);
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
	f_fb->usb_function.hs_descriptors = fb_runtime_descs;
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

static int fastboot_tx_write(const char *buffer, unsigned int buffer_size)
{
	struct usb_request *in_req = fastboot_func->in_req;
	int ret;

	memcpy(in_req->buf, buffer, buffer_size);
	in_req->length = buffer_size;
	ret = usb_ep_queue(fastboot_func->in_ep, in_req, 0);
	if (ret)
		printf("Error %d on queue\n", ret);
	return 0;
}

static int fastboot_tx_write_str(const char *buffer)
{
	return fastboot_tx_write(buffer, strlen(buffer));
}

static void compl_do_reset(struct usb_ep *ep, struct usb_request *req)
{
	do_reset(NULL, 0, 0, NULL);
}

static void cb_reboot(struct usb_ep *ep, struct usb_request *req)
{
	fastboot_func->in_req->complete = compl_do_reset;
	fastboot_tx_write_str("OKAY");
}

static int strcmp_l1(const char *s1, const char *s2)
{
	if (!s1 || !s2)
		return -1;
	return strncmp(s1, s2, strlen(s1));
}

static void cb_getvar(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char response[RESPONSE_LEN];
	const char *s;
	size_t chars_left;

	strcpy(response, "OKAY");
	chars_left = sizeof(response) - strlen(response) - 1;

	strsep(&cmd, ":");
	if (!cmd) {
		error("missing variable\n");
		fastboot_tx_write_str("FAILmissing var");
		return;
	}

	if (!strcmp_l1("version", cmd)) {
		strncat(response, FASTBOOT_VERSION, chars_left);
	} else if (!strcmp_l1("bootloader-version", cmd)) {
		strncat(response, U_BOOT_VERSION, chars_left);
	} else if (!strcmp_l1("downloadsize", cmd) ||
		!strcmp_l1("max-download-size", cmd)) {
		char str_num[12];

		sprintf(str_num, "0x%08x", CONFIG_USB_FASTBOOT_BUF_SIZE);
		strncat(response, str_num, chars_left);
	} else if (!strcmp_l1("serialno", cmd)) {
		s = getenv("serial#");
		if (s)
			strncat(response, s, chars_left);
		else
			strcpy(response, "FAILValue not set");
	} else if (!strcmp_l1("partition-type", cmd)) {
		strcpy(response, "FAILVariable not implemented");
	} else {
		error("unknown variable: %s\n", cmd);
		strcpy(response, "FAILVariable not implemented");
	}
	fastboot_tx_write_str(response);
}

static unsigned int rx_bytes_expected(unsigned int maxpacket)
{
	int rx_remain = download_size - download_bytes;
	int rem = 0;
	if (rx_remain < 0)
		return 0;
	if (rx_remain > EP_BUFFER_SIZE)
		return EP_BUFFER_SIZE;
	if (rx_remain < maxpacket) {
		rx_remain = maxpacket;
	} else if (rx_remain % maxpacket != 0) {
		rem = rx_remain % maxpacket;
		rx_remain = rx_remain + (maxpacket - rem);
	}
	return rx_remain;
}

#define BYTES_PER_DOT	0x20000
static void rx_handler_dl_image(struct usb_ep *ep, struct usb_request *req)
{
	char response[RESPONSE_LEN];
	unsigned int transfer_size = download_size - download_bytes;
	const unsigned char *buffer = req->buf;
	unsigned int buffer_size = req->actual;
	unsigned int pre_dot_num, now_dot_num;
	unsigned int max;

	if (req->status != 0) {
		printf("Bad status: %d\n", req->status);
		return;
	}

	if (buffer_size < transfer_size)
		transfer_size = buffer_size;

	memcpy((void *)CONFIG_USB_FASTBOOT_BUF_ADDR + download_bytes,
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

		sprintf(response, "OKAY");
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
		max = is_high_speed ? hs_ep_out.wMaxPacketSize :
				fs_ep_out.wMaxPacketSize;
		req->length = rx_bytes_expected(max);
		if (req->length < ep->maxpacket)
			req->length = ep->maxpacket;
	}

	req->actual = 0;
	usb_ep_queue(ep, req, 0);
}

static void cb_download(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char response[RESPONSE_LEN];
	unsigned int max;

	strsep(&cmd, ":");
	download_size = simple_strtoul(cmd, NULL, 16);
	download_bytes = 0;

	printf("Starting download of %d bytes\n", download_size);

	if (0 == download_size) {
		sprintf(response, "FAILdata invalid size");
	} else if (download_size > CONFIG_USB_FASTBOOT_BUF_SIZE) {
		download_size = 0;
		sprintf(response, "FAILdata too large");
	} else {
		sprintf(response, "DATA%08x", download_size);
		req->complete = rx_handler_dl_image;
		max = is_high_speed ? hs_ep_out.wMaxPacketSize :
			fs_ep_out.wMaxPacketSize;
		req->length = rx_bytes_expected(max);
		if (req->length < ep->maxpacket)
			req->length = ep->maxpacket;
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

#ifdef CONFIG_FASTBOOT_FLASH
static void cb_flash(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char response[RESPONSE_LEN];

	strsep(&cmd, ":");
	if (!cmd) {
		error("missing partition name\n");
		fastboot_tx_write_str("FAILmissing partition name");
		return;
	}

	strcpy(response, "FAILno flash device defined");

#ifdef CONFIG_FSL_FASTBOOT
	rx_process_flash(cmd, response);
#else
#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
	fb_mmc_flash_write(cmd, (void *)CONFIG_USB_FASTBOOT_BUF_ADDR,
			   download_bytes, response);
#endif
#endif
	fastboot_tx_write_str(response);
}
#endif

static void cb_oem(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
#if defined(CONFIG_FASTBOOT_FLASH) && defined(CONFIG_FASTBOOT_FLASH_MMC_DEV)
	if (strncmp("format", cmd + 4, 6) == 0) {
		char cmdbuf[32];
                sprintf(cmdbuf, "gpt write mmc %x $partitions",
			CONFIG_FASTBOOT_FLASH_MMC_DEV);
                if (run_command(cmdbuf, 0))
			fastboot_tx_write_str("FAIL");
                else
			fastboot_tx_write_str("OKAY");
	} else
#endif
	if (strncmp("unlock", cmd + 4, 8) == 0) {
		fastboot_tx_write_str("FAILnot implemented");
	}
	else {
		fastboot_tx_write_str("FAILunknown oem command");
	}
}

#ifdef CONFIG_FASTBOOT_FLASH
static void cb_erase(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char response[RESPONSE_LEN];

	strsep(&cmd, ":");
	if (!cmd) {
		error("missing partition name");
		fastboot_tx_write_str("FAILmissing partition name");
		return;
	}

	strcpy(response, "FAILno flash device defined");

#ifdef CONFIG_FSL_FASTBOOT
	rx_process_erase(cmd, response);
#else
#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
	fb_mmc_erase(cmd, response);
#endif
#endif
	fastboot_tx_write_str(response);
}
#endif

#ifdef CONFIG_FSL_FASTBOOT
static void cb_reboot_bootloader(struct usb_ep *ep, struct usb_request *req)
{
	fastboot_tx_write_str("OKAY");

	udelay(1000000);
	fastboot_enable_flag();
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
#ifdef CONFIG_FASTBOOT_FLASH
	{
		.cmd = "flash",
		.cb = cb_flash,
	}, {
		.cmd = "erase",
		.cb = cb_erase,
	},
#endif
	{
		.cmd = "oem",
		.cb = cb_oem,
	},
};

static void rx_handler_command(struct usb_ep *ep, struct usb_request *req)
{
	char *cmdbuf = req->buf;
	void (*func_cb)(struct usb_ep *ep, struct usb_request *req) = NULL;
	int i;

	for (i = 0; i < ARRAY_SIZE(cmd_dispatch_info); i++) {
		if (!strcmp_l1(cmd_dispatch_info[i].cmd, cmdbuf)) {
			func_cb = cmd_dispatch_info[i].cb;
			break;
		}
	}

	if (!func_cb) {
		error("unknown command: %s\n", cmdbuf);
		fastboot_tx_write_str("FAILunknown command");
	} else {
		if (req->actual < req->length) {
			u8 *buf = (u8 *)req->buf;
			buf[req->actual] = 0;
			func_cb(ep, req);
		} else {
			error("buffer overflow\n");
			fastboot_tx_write_str("FAILbuffer overflow");
		}
	}

	if (req->status == 0) {
		*cmdbuf = '\0';
		req->actual = 0;
		usb_ep_queue(ep, req, 0);
	}
}
