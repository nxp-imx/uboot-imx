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
#include "lib/avb/fsl/utils.h"
#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
#include <fb_mmc.h>
#endif

#ifdef CONFIG_IMX_TRUSTY_OS
#define ATAP_UUID_SIZE 32
#define ATAP_UUID_STR_SIZE ((ATAP_UUID_SIZE*2) + 1)

extern int armv7_init_nonsec(void);
extern void trusty_os_init(void);
#include <trusty/libtipc.h>
extern bool tos_flashed;
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
#include <dt_table.h>
#include <fsl_avb.h>
#endif

#ifdef CONFIG_ANDROID_THINGS_SUPPORT
#include <asm-generic/gpio.h>
#include <asm/imx-common/gpio.h>
#endif

#define FASTBOOT_VERSION		"0.4"

#ifdef CONFIG_FASTBOOT_LOCK
#include "fastboot_lock_unlock.h"
#endif

#if defined(CONFIG_ANDROID_THINGS_SUPPORT) && defined(CONFIG_ARCH_IMX8M)
#define FASTBOOT_COMMON_VAR_NUM 14
#else
#define FASTBOOT_COMMON_VAR_NUM 13
#endif

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

#ifdef CONFIG_FLASH_MCUFIRMWARE_SUPPORT
struct fastboot_device_info fastboot_firmwareinfo;
#endif

#if defined (CONFIG_ARCH_IMX8) || defined (CONFIG_ARCH_IMX8M)
#define DST_DECOMPRESS_LEN 1024*1024*32
#endif

/*
 * EP_BUFFER_SIZE must always be an integral multiple of maxpacket size
 * (64 or 512 or 1024), else we break on certain controllers like DWC3
 * that expect bulk OUT requests to be divisible by maxpacket size.
 */
static unsigned int download_size;
static unsigned int download_bytes;

#define AT_OEM_BL_PART_NAME_BASE	"oem_bootloader"
#define AT_OEM_BL_PART_SIZE	(sizeof(AT_OEM_BL_PART_NAME_BASE) + \
		sizeof("_a") - 1)

/* Offset (in u32's) of start and end fields in the zImage header. */
#define ZIMAGE_START_ADDR	10
#define ZIMAGE_END_ADDR	11

#if defined(CONFIG_ANDROID_THINGS_SUPPORT) && defined(CONFIG_ARCH_IMX8M)
#define IMX8MQ_GPIO3_IO25 IMX_GPIO_NR(3, 25)  //board_id[6]:6
#define IMX8MQ_GPIO3_IO19 IMX_GPIO_NR(3, 19)  //board_id[6]:5
#define IMX8MQ_GPIO3_IO20 IMX_GPIO_NR(3, 20)  //board_id[6]:4
#define IMX8MQ_GPIO3_IO24 IMX_GPIO_NR(3, 24)  //board_id[6]:3
#define IMX8MQ_GPIO3_IO23 IMX_GPIO_NR(3, 23)  //board_id[6]:2
#define IMX8MQ_GPIO3_IO22 IMX_GPIO_NR(3, 22)  //board_id[6]:1
#define IMX8MQ_GPIO3_IO21 IMX_GPIO_NR(3, 21)  //board_id[6]:0
#endif

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
	"battery-soc-ok",
#if defined(CONFIG_ANDROID_THINGS_SUPPORT) && defined(CONFIG_ARCH_IMX8M)
	"baseboard_id"
#endif
};

/* Boot metric variables */
boot_metric metrics = {
	.bll_1 = 0,
	.ble_1 = 0,
	.kl    = 0,
	.kd    = 0,
	.avb   = 0,
	.odt   = 0,
	.sw    = 0
};

#ifdef CONFIG_USB_GADGET
typedef struct usb_req usb_req;
struct usb_req {
	struct usb_request *in_req;
	usb_req *next;
};

struct f_fastboot {
	struct usb_function usb_function;

	/* IN/OUT EP's and corresponding requests */
	struct usb_ep *in_ep, *out_ep;
	struct usb_request *in_req, *out_req;
	usb_req *front, *rear;
};

static char fb_ext_prop_name[] = "DeviceInterfaceGUID";
static char fb_ext_prop_data[] = "{4866319A-F4D6-4374-93B9-DC2DEB361BA9}";

static struct usb_os_desc_ext_prop fb_ext_prop = {
	.type = 1,		/* NUL-terminated Unicode String (REG_SZ) */
	.name = fb_ext_prop_name,
	.data = fb_ext_prop_data,
};

/* 16 bytes of "Compatible ID" and "Subcompatible ID" */
static char fb_cid[16] = {'W', 'I', 'N', 'U', 'S', 'B'};
static struct usb_os_desc fb_os_desc = {
	.ext_compat_id = fb_cid,
};

static struct usb_os_desc_table fb_os_desc_table = {
	.os_desc = &fb_os_desc,
};

static inline struct f_fastboot *func_to_fastboot(struct usb_function *f)
{
	return container_of(f, struct f_fastboot, usb_function);
}
static int strcmp_l1(const char *s1, const char *s2);

static struct f_fastboot *fastboot_func;
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
#endif

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

#ifndef TRUSTY_OS_MMC_BLKS
#define TRUSTY_OS_MMC_BLKS 0x7FF
#endif
#ifndef TEE_HWPARTITION_ID
#define TEE_HWPARTITION_ID 2
#endif

#define FASTBOOT_PARTITION_ALL "all"

#define ANDROID_MBR_OFFSET	    0
#define ANDROID_MBR_SIZE	    0x200
#define ANDROID_BOOTLOADER_SIZE	    0x1FFC00

#define MMC_SATA_BLOCK_SIZE 512
#define FASTBOOT_FBPARTS_ENV_MAX_LEN 1024
/* To support the Android-style naming of flash */
#define MAX_PTN		    32
struct fastboot_ptentry g_ptable[MAX_PTN];
unsigned int g_pcount;
struct fastboot_device_info fastboot_devinfo = {0xff, 0xff};


enum {
	PTN_GPT_INDEX = 0,
	PTN_TEE_INDEX,
#ifdef CONFIG_FLASH_MCUFIRMWARE_SUPPORT
	PTN_M4_OS_INDEX,
#endif
	PTN_ALL_INDEX,
	PTN_BOOTLOADER_INDEX,
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
		if (!fs_dev_desc->block_read(fs_dev_desc, bs, blk_num, dst)) {
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

static void save_env(struct fastboot_ptentry *ptn,
		     char *var, char *val)
{
	setenv(var, val);
	saveenv();
}

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

static int get_block_size(void);
#ifdef CONFIG_FLASH_MCUFIRMWARE_SUPPORT
static void process_flash_sf(const char *cmdbuf)
{
	int blksz = 0;
	blksz = get_block_size();

	if (download_bytes) {
		struct fastboot_ptentry *ptn;
		ptn = fastboot_flash_find_ptn(cmdbuf);
		if (ptn == 0) {
			fastboot_fail("partition does not exist");
		} else if ((download_bytes > ptn->length * blksz)) {
			fastboot_fail("image too large for partition");
		/* TODO : Improve check for yaffs write */
		} else {
			int ret;
			char sf_command[128];
			/* Normal case */
			/* Probe device */
			sprintf(sf_command, "sf probe");
			ret = run_command(sf_command, 0);
			if (ret){
				fastboot_fail("Probe sf failed");
				return;
			}
			/* Erase */
			sprintf(sf_command, "sf erase 0x%x 0x%x", ptn->start * blksz, /*start*/
			ptn->length * blksz /*size*/);
			ret = run_command(sf_command, 0);
			if (ret) {
				fastboot_fail("Erasing sf failed");
				return;
			}
			/* Write image */
			sprintf(sf_command, "sf write 0x%x 0x%x 0x%x",
					(unsigned int)(ulong)interface.transfer_buffer, /* source */
					ptn->start * blksz, /* start */
					download_bytes /*size*/);
			printf("sf write '%s'\n", ptn->name);
			ret = run_command(sf_command, 0);
			if (ret){
				fastboot_fail("Writing sf failed");
				return;
			}
			printf("sf write finished '%s'\n", ptn->name);
			fastboot_okay("");
		}
	} else {
		fastboot_fail("no image downloaded");
	}
}

#ifdef CONFIG_ARCH_IMX8M
/* Check if the mcu image is built for running from TCM */
static bool is_tcm_image(char *image_addr)
{
	u32 stack, pc;

	stack = *(u32 *)image_addr;

	if ((stack != (u32)ANDROID_MCU_FIRMWARE_HEADER_STACK)) {
		printf("Please flash mcu firmware images for running from TCM\n");
		return false;
	} else
		return true;
}

static int do_bootmcu(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret;
	size_t out_num_read;
	void *m4_base_addr = (void *)M4_BOOTROM_BASE_ADDR;
	char command[32];

	ret = read_from_partition_multi(FASTBOOT_MCU_FIRMWARE_PARTITION,
			0, ANDROID_MCU_FIRMWARE_SIZE, (void *)m4_base_addr, &out_num_read);
	if ((ret != 0) || (out_num_read != ANDROID_MCU_FIRMWARE_SIZE)) {
		printf("Read M4 images failed!\n");
		return 1;
	} else {
		printf("run command: 'bootaux 0x%x'\n",(unsigned int)(ulong)m4_base_addr);

		sprintf(command, "bootaux 0x%x", (unsigned int)(ulong)m4_base_addr);
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
	"boot mcu images from 'm4_os' partition, only support images run from TCM"
);
#endif
#endif /* CONFIG_FLASH_MCUFIRMWARE_SUPPORT */

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

static ulong bootloader_mmc_offset(void)
{
	if (is_imx8m() || (is_imx8() && is_soc_rev(CHIP_REV_A)))
		return 0x8400;
	else if (is_imx8())
		return 0x8000;
	else
		return 0x400;
}

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
		bootloader_mmc_offset() < ANDROID_GPT_END);
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
					(getenv("target_ubootdev"))) {
		dev = simple_strtoul(getenv("target_ubootdev"), NULL, 10);
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
			/* Get target flash device. */
			if (get_fastboot_target_dev(mmc_dev, ptn) != 0)
				return;

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
/* Check if we need to flash mcu firmware */
#ifdef CONFIG_FLASH_MCUFIRMWARE_SUPPORT
	if (!strncmp(cmdbuf, FASTBOOT_MCU_FIRMWARE_PARTITION,
				sizeof(FASTBOOT_MCU_FIRMWARE_PARTITION))) {
		switch (fastboot_firmwareinfo.type) {
		case DEV_SF:
			process_flash_sf(cmdbuf);
			break;
#ifdef CONFIG_ARCH_IMX8M
		case DEV_MMC:
			if (is_tcm_image(interface.transfer_buffer))
				process_flash_mmc(cmdbuf);
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
	interface.transfer_buffer =
				(unsigned char *)getenv_ulong("fastboot_buffer", 16, CONFIG_FASTBOOT_BUF_ADDR);
	interface.transfer_buffer_size =
				CONFIG_FASTBOOT_BUF_SIZE;
}

static int _fastboot_setup_dev(int *switched)
{
	char *fastboot_env;
	struct fastboot_device_info devinfo;;

	fastboot_env = getenv("fastboot_dev");

	if (fastboot_env) {
		if (!strcmp(fastboot_env, "sata")) {
			devinfo.type = DEV_SATA;
			devinfo.dev_id = 0;
#if defined(CONFIG_FASTBOOT_STORAGE_MMC)
		} else if (!strncmp(fastboot_env, "mmc", 3)) {
			devinfo.type = DEV_MMC;
			devinfo.dev_id = mmc_get_env_dev();
#endif
		} else {
			return 1;
		}
	} else {
		return 1;
	}
#ifdef CONFIG_FLASH_MCUFIRMWARE_SUPPORT
	/* For imx7ulp, flash m4 images directly to spi nor-flash, M4 will
	 * run automatically after powered on. For imx8mq, flash m4 images to
	 * physical partition 'm4_os', m4 will be kicked off by A core. */
	fastboot_firmwareinfo.type = ANDROID_MCU_FRIMWARE_DEV_TYPE;
#endif

	if (switched) {
		if (devinfo.type != fastboot_devinfo.type || devinfo.dev_id != fastboot_devinfo.dev_id)
			*switched = 1;
		else
			*switched = 0;
	}

	fastboot_devinfo.type	 = devinfo.type;
	fastboot_devinfo.dev_id = devinfo.dev_id;

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
	strncpy(ptable[ptable_index].name, (const char *)info.name,
			sizeof(ptable[ptable_index].name) - 1);

#ifdef CONFIG_PARTITION_UUIDS
	strcpy(ptable[ptable_index].uuid, (const char *)info.uuid);
#endif
#ifdef CONFIG_ANDROID_AB_SUPPORT
	if (!strcmp((const char *)info.name, FASTBOOT_PARTITION_SYSTEM_A) ||
	    !strcmp((const char *)info.name, FASTBOOT_PARTITION_SYSTEM_B) ||
	    !strcmp((const char *)info.name, FASTBOOT_PARTITION_OEM_A) ||
	    !strcmp((const char *)info.name, FASTBOOT_PARTITION_VENDOR_A) ||
	    !strcmp((const char *)info.name, FASTBOOT_PARTITION_OEM_B) ||
	    !strcmp((const char *)info.name, FASTBOOT_PARTITION_VENDOR_B) ||
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

		/* Force to init mmc */
		mmc->has_init = 0;
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

	/* Trusty OS */
	strcpy(ptable[PTN_TEE_INDEX].name, FASTBOOT_PARTITION_TEE);
	ptable[PTN_TEE_INDEX].start = 0;
	ptable[PTN_TEE_INDEX].length = TRUSTY_OS_MMC_BLKS;
	ptable[PTN_TEE_INDEX].partition_id = TEE_HWPARTITION_ID;
	strcpy(ptable[PTN_TEE_INDEX].fstype, "raw");

	/* Add m4_os partition if we support mcu firmware image flash */
#ifdef CONFIG_FLASH_MCUFIRMWARE_SUPPORT
	strcpy(ptable[PTN_M4_OS_INDEX].name, FASTBOOT_MCU_FIRMWARE_PARTITION);
	ptable[PTN_M4_OS_INDEX].start = ANDROID_MCU_FIRMWARE_START / dev_desc->blksz;
	ptable[PTN_M4_OS_INDEX].length = ANDROID_MCU_FIRMWARE_SIZE / dev_desc->blksz;
	ptable[PTN_M4_OS_INDEX].flags = FASTBOOT_PTENTRY_FLAGS_UNERASEABLE;
	ptable[PTN_M4_OS_INDEX].partition_id = user_partition;
	strcpy(ptable[PTN_M4_OS_INDEX].fstype, "raw");
#endif

	strcpy(ptable[PTN_ALL_INDEX].name, FASTBOOT_PARTITION_ALL);
	ptable[PTN_ALL_INDEX].start = 0;
	ptable[PTN_ALL_INDEX].length = dev_desc->lba;
	ptable[PTN_ALL_INDEX].partition_id = user_partition;
	strcpy(ptable[PTN_ALL_INDEX].fstype, "device");

	/* Bootloader */
	strcpy(ptable[PTN_BOOTLOADER_INDEX].name, FASTBOOT_PARTITION_BOOTLOADER);
	ptable[PTN_BOOTLOADER_INDEX].start =
				bootloader_mmc_offset() / dev_desc->blksz;
	ptable[PTN_BOOTLOADER_INDEX].length =
				 ANDROID_BOOTLOADER_SIZE / dev_desc->blksz;
	ptable[PTN_BOOTLOADER_INDEX].partition_id = boot_partition;
	ptable[PTN_BOOTLOADER_INDEX].flags = FASTBOOT_PTENTRY_FLAGS_UNERASEABLE;
	strcpy(ptable[PTN_BOOTLOADER_INDEX].fstype, "raw");

	int tbl_idx;
	int part_idx = 1;
	int ret;
	for (tbl_idx = PTN_BOOTLOADER_INDEX + 1; tbl_idx < MAX_PTN; tbl_idx++) {
		ret = _fastboot_parts_add_ptable_entry(tbl_idx,
				part_idx++,
				user_partition,
				NULL,
				NULL,
				dev_desc, ptable);
		if (ret)
			break;
	}
	for (i = 0; i < tbl_idx; i++)
		fastboot_flash_add_ptn(&ptable[i]);

	return 0;
}
#endif /*CONFIG_FASTBOOT_STORAGE_SATA || CONFIG_FASTBOOT_STORAGE_MMC*/

static void _fastboot_load_partitions(void)
{
	g_pcount = 0;
#if defined(CONFIG_FASTBOOT_STORAGE_SATA) \
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
	case USB_BOOT:
		printf("Detect USB boot. Will enter fastboot mode!\n");
		if (!getenv("bootcmd"))
			setenv("bootcmd", "fastboot 0");
#endif /*CONFIG_FASTBOOT_STORAGE_MMC*/
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
	} else if (is_imx8qm()) {
		if (!getenv("soc_type"))
			setenv("soc_type", "imx8qm");
	} else if (is_imx8qxp()) {
		if (!getenv("soc_type"))
			setenv("soc_type", "imx8qxp");
	} else if (is_imx8mq()) {
		if (!getenv("soc_type"))
			setenv("soc_type", "imx8mq");
	} else if (is_imx8mm()) {
		if (!getenv("soc_type"))
			setenv("soc_type", "imx8mm");
	}
}

#ifdef CONFIG_ANDROID_RECOVERY
void board_recovery_setup(void)
{
/* boot from current mmc with avb verify */
#ifdef CONFIG_AVB_SUPPORT
	if (!getenv("bootcmd_android_recovery"))
		setenv("bootcmd_android_recovery", "boota recovery");
#else
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
	default:
		printf("Unsupported bootup device for recovery: dev: %d\n",
			bootdev);
		return;
	}
#endif /* CONFIG_AVB_SUPPORT */
	printf("setup env for recovery..\n");
	setenv("bootcmd", getenv("bootcmd_android_recovery"));
}
#endif /*CONFIG_ANDROID_RECOVERY*/
#endif /*CONFIG_FSL_FASTBOOT*/

#if defined(CONFIG_AVB_SUPPORT) && defined(CONFIG_MMC)
static AvbABOps fsl_avb_ab_ops = {
	.read_ab_metadata = fsl_read_ab_metadata,
	.write_ab_metadata = fsl_write_ab_metadata,
	.ops = NULL
};
#ifdef CONFIG_AVB_ATX
static AvbAtxOps fsl_avb_atx_ops = {
	.ops = NULL,
	.read_permanent_attributes = fsl_read_permanent_attributes,
	.read_permanent_attributes_hash = fsl_read_permanent_attributes_hash,
#ifdef CONFIG_IMX_TRUSTY_OS
	.set_key_version = fsl_write_rollback_index_rpmb
#else
	.set_key_version = fsl_set_key_version
#endif
};
#endif
static AvbOps fsl_avb_ops = {
	.ab_ops = &fsl_avb_ab_ops,
#ifdef CONFIG_AVB_ATX
	.atx_ops = &fsl_avb_atx_ops,
#endif
	.read_from_partition = fsl_read_from_partition_multi,
	.write_to_partition = fsl_write_to_partition,
#ifdef CONFIG_AVB_ATX
	.validate_vbmeta_public_key = avb_atx_validate_vbmeta_public_key,
#else
	.validate_vbmeta_public_key = fsl_validate_vbmeta_public_key_rpmb,
#endif
	.read_rollback_index = fsl_read_rollback_index_rpmb,
	.write_rollback_index = fsl_write_rollback_index_rpmb,
	.read_is_device_unlocked = fsl_read_is_device_unlocked,
	.get_unique_guid_for_partition = fsl_get_unique_guid_for_partition,
	.get_size_of_partition = fsl_get_size_of_partition
};
#endif

#ifdef CONFIG_IMX_TRUSTY_OS
void tee_setup(void)
{
	/* load tee from boot1 of eMMC. */
	int mmcc = mmc_get_env_dev();
	struct blk_desc *dev_desc = NULL;

	struct mmc *mmc;
	mmc = find_mmc_device(mmcc);
	if (!mmc) {
	            printf("boota: cannot find '%d' mmc device\n", mmcc);
		            goto fail;
	}

	dev_desc = blk_get_dev("mmc", mmcc);
	if (NULL == dev_desc) {
	            printf("** Block device MMC %d not supported\n", mmcc);
		            goto fail;
	}

	/* below was i.MX mmc operation code */
	if (mmc_init(mmc)) {
	            printf("mmc%d init failed\n", mmcc);
		            goto fail;
	}

	struct fastboot_ptentry *tee_pte;
	char *tee_ptn = FASTBOOT_PARTITION_TEE;
	tee_pte = fastboot_flash_find_ptn(tee_ptn);
	mmc_switch_part(mmc, TEE_HWPARTITION_ID);
	if (!tee_pte) {
		printf("boota: cannot find tee partition!\n");
	}

	if (mmc->block_dev.block_read(dev_desc, tee_pte->start,
		    tee_pte->length, (void *)TRUSTY_OS_ENTRY) < 0) {
		printf("Failed to load tee.");
	}
	mmc_switch_part(mmc, FASTBOOT_MMC_USER_PARTITION_ID);

	tos_flashed = false;
	if(!valid_tos()) {
		printf("TOS not flashed! Will enter TOS recovery mode. Everything will be wiped!\n");
		fastboot_wipe_all();
		run_command("fastboot 0", 0);
		goto fail;
	}
#ifdef NON_SECURE_FASTBOOT
	armv7_init_nonsec();
	trusty_os_init();
	trusty_ipc_init();
#endif

fail:
	return;

}
#endif

void fastboot_setup(void)
{
	int sw, ret;
#ifdef CONFIG_USB_GADGET
	struct tag_serialnr serialnr;
	char serial[17];

	get_board_serial(&serialnr);
	sprintf(serial, "%08x%08x", serialnr.high, serialnr.low);
	g_dnl_set_serialnumber(serial);
#endif
	/*execute board relevant initilizations for preparing fastboot */
	board_fastboot_setup();

	/*get the fastboot dev*/
	ret = _fastboot_setup_dev(&sw);

	/*load partitions information for the fastboot dev*/
	if (!ret && sw)
		_fastboot_load_partitions();

	parameters_setup();
#ifdef CONFIG_AVB_SUPPORT
	fsl_avb_ab_ops.ops = &fsl_avb_ops;
#ifdef CONFIG_AVB_ATX
	fsl_avb_atx_ops.ops = &fsl_avb_ops;
#endif
#endif
}

/* Write the bcb with fastboot bootloader commands */
static void enable_fastboot_command(void)
{
#ifdef CONFIG_BCB_SUPPORT
	char fastboot_command[32] = {0};
	strncpy(fastboot_command, FASTBOOT_BCB_CMD, 31);
	bcb_write_command(fastboot_command);
#endif
}

/* Get the Boot mode from BCB cmd or Key pressed */
static FbBootMode fastboot_get_bootmode(void)
{
	int boot_mode = BOOTMODE_NORMAL;
#ifdef CONFIG_ANDROID_RECOVERY
	if(is_recovery_key_pressing()) {
		boot_mode = BOOTMODE_RECOVERY_KEY_PRESSED;
		return boot_mode;
	}
#endif
#ifdef CONFIG_BCB_SUPPORT
	int ret = 0;
	char command[32];
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
#endif
	return boot_mode;
}

#ifdef CONFIG_SYSTEM_RAMDISK_SUPPORT
/* Setup booargs for taking the system parition as ramdisk */
static void fastboot_setup_system_boot_args(const char *slot, bool append_root)
{
	const char *system_part_name = NULL;
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
	struct fastboot_ptentry *ptentry = fastboot_flash_find_ptn(system_part_name);
	if(ptentry != NULL) {
		char bootargs_3rd[ANDR_BOOT_ARGS_SIZE];
#if defined(CONFIG_FASTBOOT_STORAGE_MMC)
		if (append_root) {
			u32 dev_no = mmc_map_to_kernel_blk(mmc_get_env_dev());
			sprintf(bootargs_3rd, "skip_initramfs root=/dev/mmcblk%dp%d",
					dev_no,
					ptentry->partition_index);
		} else {
			sprintf(bootargs_3rd, "skip_initramfs");
		}
		strcat(bootargs_3rd, " rootwait");
		setenv("bootargs_3rd", bootargs_3rd);
#endif
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
		run_command("fastboot 0", 0);
		break;
#ifdef CONFIG_ANDROID_RECOVERY
	case BOOTMODE_RECOVERY_BCB_CMD:
	case BOOTMODE_RECOVERY_KEY_PRESSED:
		/* Make the boot into recovery mode */
		puts("Fastboot: Got Recovery key pressing or recovery commands!\n");
		board_recovery_setup();
		break;
#endif
	default:
		/* skip special mode boot*/
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

#if !defined(CONFIG_AVB_SUPPORT) || !defined(CONFIG_MMC)
static struct andr_img_hdr boothdr __aligned(ARCH_DMA_MINALIGN);
#endif

#ifdef CONFIG_IMX_TRUSTY_OS
void trusty_setbootparameter(struct andr_img_hdr *hdr, AvbABFlowResult avb_result) {
	u32 os_ver = hdr->os_version >> 11;
	u32 os_ver_km = (((os_ver >> 14) & 0x7F) * 100 + ((os_ver >> 7) & 0x7F)) * 100
	    + (os_ver & 0x7F);
	u32 os_lvl = hdr->os_version & ((1U << 11) - 1);
	u32 os_lvl_km = ((os_lvl >> 4) + 2000) * 100 + (os_lvl & 0x0F);
	keymaster_verified_boot_t vbstatus;
	FbLockState lock_status = fastboot_get_lock_stat();

	uint8_t permanent_attributes_hash[AVB_SHA256_DIGEST_SIZE];
	if (fsl_read_permanent_attributes_hash(&fsl_avb_atx_ops, permanent_attributes_hash)) {
		printf("ERROR - failed to read permanent attributes hash for keymaster\n");
		memset(permanent_attributes_hash, 0, AVB_SHA256_DIGEST_SIZE);
	}

	bool lock = (lock_status == FASTBOOT_LOCK)? true: false;
	if (avb_result == AVB_AB_FLOW_RESULT_OK)
		vbstatus = KM_VERIFIED_BOOT_VERIFIED;
	else
		vbstatus = KM_VERIFIED_BOOT_FAILED;

	/* TODO: Need VBH added in the last parameter */
	trusty_set_boot_params(os_ver_km, os_lvl_km, vbstatus, lock,
		permanent_attributes_hash, AVB_SHA256_DIGEST_SIZE,
		NULL, 0);
}
#endif

#if defined(CONFIG_AVB_SUPPORT) && defined(CONFIG_MMC)

int do_boota(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) {

	ulong addr = 0;
	struct andr_img_hdr *hdr = NULL;
	void *boot_buf = NULL;
	ulong image_size;
	u32 avb_metric;
	bool check_image_arm64 =  false;
	bool is_recovery_mode = false;
	char *slot = NULL;

	AvbABFlowResult avb_result;
	AvbSlotVerifyData *avb_out_data;
	AvbPartitionData *avb_loadpart;

	/* get bootmode, default to boot "boot" */
	if (argc > 1) {
		is_recovery_mode =
			(strncmp(argv[1], "recovery", sizeof("recovery")) != 0) ? false: true;
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
	/* if in lock state, do avb verify */
#ifndef CONFIG_DUAL_BOOTLOADER
	/* For imx6 on Android, we don't have a/b slot and we want to verify
	 * boot/recovery with AVB. For imx8 and Android Things we don't have
	 * recovery and support a/b slot for boot */
#ifdef CONFIG_ANDROID_AB_SUPPORT
	/* we can use avb to verify Trusty if we want */
	const char *requested_partitions[] = {"boot", 0};
	avb_result = avb_ab_flow_fast(&fsl_avb_ab_ops, requested_partitions, allow_fail,
			AVB_HASHTREE_ERROR_MODE_RESTART_AND_INVALIDATE, &avb_out_data);
#else
	if (!is_recovery_mode) {
		const char *requested_partitions[] = {"boot", 0};
		avb_result = avb_single_flow(&fsl_avb_ab_ops, requested_partitions, allow_fail,
				AVB_HASHTREE_ERROR_MODE_RESTART, &avb_out_data);
	} else {
		const char *requested_partitions[] = {"recovery", 0};
		avb_result = avb_single_flow(&fsl_avb_ab_ops, requested_partitions, allow_fail,
				AVB_HASHTREE_ERROR_MODE_RESTART, &avb_out_data);
	}
#endif
#else /* !CONFIG_DUAL_BOOTLOADER */
	/* We will only verify single one slot which has been selected in SPL */
	const char *requested_partitions[] = {"boot", 0};
	avb_result = avb_flow_dual_uboot(&fsl_avb_ab_ops, requested_partitions, allow_fail,
			AVB_HASHTREE_ERROR_MODE_RESTART_AND_INVALIDATE, &avb_out_data);

	/* Goto fail early if current slot is not bootable. */
	if (avb_result == AVB_AB_FLOW_RESULT_ERROR_NO_BOOTABLE_SLOTS) {
		printf("boota: slot verify fail!\n");
		goto fail;
	}
#endif /* !CONFIG_DUAL_BOOTLOADER */

	/* get the duration of avb */
	metrics.avb = get_timer(avb_metric);

	if ((avb_result == AVB_AB_FLOW_RESULT_OK) ||
			(avb_result == AVB_AB_FLOW_RESULT_OK_WITH_VERIFICATION_ERROR)) {
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
		slot = avb_out_data->ab_suffix;
		if (avb_result == AVB_AB_FLOW_RESULT_OK)
			printf(" verify OK, boot '%s%s'\n",
					avb_loadpart->partition_name, avb_out_data->ab_suffix);
		else {
			printf(" verify FAIL, state: UNLOCK\n");
			printf(" boot '%s%s' still\n",
					avb_loadpart->partition_name, avb_out_data->ab_suffix);
		}
		char bootargs_sec[ANDR_BOOT_ARGS_SIZE];
		if (lock_status == FASTBOOT_LOCK) {
			sprintf(bootargs_sec,
					"androidboot.verifiedbootstate=green androidboot.slot_suffix=%s %s",
					avb_out_data->ab_suffix, avb_out_data->cmdline);
		} else {
			sprintf(bootargs_sec,
					"androidboot.verifiedbootstate=orange androidboot.slot_suffix=%s %s",
					avb_out_data->ab_suffix, avb_out_data->cmdline);
		}
		setenv("bootargs_sec", bootargs_sec);
#ifdef CONFIG_SYSTEM_RAMDISK_SUPPORT
		if(!is_recovery_mode) {
			if(avb_out_data->cmdline != NULL && strstr(avb_out_data->cmdline, "root="))
				fastboot_setup_system_boot_args(avb_out_data->ab_suffix, false);
			else
				fastboot_setup_system_boot_args(avb_out_data->ab_suffix, true);
		}
#endif /* CONFIG_SYSTEM_RAMDISK_SUPPORT */
		image_size = avb_loadpart->data_size;
#if defined (CONFIG_ARCH_IMX8) || defined (CONFIG_ARCH_IMX8M)
		/* If we are using uncompressed kernel image, copy it directly to
		 * hdr->kernel_addr, if we are using compressed lz4 kernel image,
		 * we need to decompress the kernel image first. */
		if (image_arm64((void *)((ulong)hdr + hdr->page_size))) {
			memcpy((void *)(long)hdr->kernel_addr,
					(void *)((ulong)hdr + hdr->page_size), hdr->kernel_size);
		} else {
#ifdef CONFIG_LZ4
			size_t lz4_len = DST_DECOMPRESS_LEN;
			if (ulz4fn((void *)((ulong)hdr + hdr->page_size),
						hdr->kernel_size, (void *)(ulong)hdr->kernel_addr, &lz4_len) != 0) {
				printf("Decompress kernel fail!\n");
				goto fail;
			}
#else /* CONFIG_LZ4 */
			printf("please enable CONFIG_LZ4 if we're using compressed lz4 kernel image!\n");
			goto fail;
#endif /* CONFIG_LZ4 */
		}
#else /* CONFIG_ARCH_IMX8 || CONFIG_ARCH_IMX8M */
		/* copy kernel image and boot header to hdr->kernel_addr - hdr->page_size */
		memcpy((void *)(ulong)(hdr->kernel_addr - hdr->page_size), (void *)hdr,
				hdr->page_size + ALIGN(hdr->kernel_size, hdr->page_size));
#endif /* CONFIG_ARCH_IMX8 || CONFIG_ARCH_IMX8M */
	} else {
		/* Fall into fastboot mode if get unacceptable error from avb
		 * or verify fail in lock state.
		 */
		if (lock_status == FASTBOOT_LOCK)
			printf(" verify FAIL, state: LOCK\n");

		goto fail;
	}

	flush_cache((ulong)load_addr, image_size);
	check_image_arm64  = image_arm64((void *)(ulong)hdr->kernel_addr);
#ifdef CONFIG_SYSTEM_RAMDISK_SUPPORT
	if (is_recovery_mode)
		memcpy((void *)(ulong)hdr->ramdisk_addr, (void *)(ulong)hdr + hdr->page_size
				+ ALIGN(hdr->kernel_size, hdr->page_size), hdr->ramdisk_size);
#else
	memcpy((void *)(ulong)hdr->ramdisk_addr, (void *)(ulong)hdr + hdr->page_size
			+ ALIGN(hdr->kernel_size, hdr->page_size), hdr->ramdisk_size);
#endif
#ifdef CONFIG_OF_LIBFDT
	/* load the dtb file */
#ifdef CONFIG_LOAD_FDT_FROM_PART
	u32 fdt_size = 0;
	char oemimage[AT_OEM_BL_PART_SIZE];
	snprintf(oemimage, sizeof(oemimage), "%s%s",
			AT_OEM_BL_PART_NAME_BASE, slot);

	struct dt_table_header dt_img;
	size_t num_read;
	if (fsl_avb_ops.read_from_partition(&fsl_avb_ops, oemimage, 0,
			sizeof(dt_img), &dt_img, &num_read) !=
			AVB_IO_RESULT_OK &&
			num_read != sizeof(dt_img)) {
		printf("boota: read dt table header error\n");
		goto dt_read_done;
	}

	if (be32_to_cpu(dt_img.magic) != DT_TABLE_MAGIC) {
		printf("boota: bad dt table magic %08x\n",
				be32_to_cpu(dt_img.magic));
		goto dt_read_done;
	} else if (!be32_to_cpu(dt_img.dt_entry_count)) {
		printf("boota: no dt entries\n");
		goto dt_read_done;
	}

	struct dt_table_entry dt_entry;
	assert(be32_to_cpu(dt_img.dt_entry_size) == sizeof(dt_entry));
	if (fsl_avb_ops.read_from_partition(&fsl_avb_ops, oemimage,
			be32_to_cpu(dt_img.dt_entries_offset),
			be32_to_cpu(dt_img.dt_entry_size), &dt_entry,
			&num_read) != AVB_IO_RESULT_OK &&
			num_read != sizeof(dt_entry)) {
		printf("boota: read dt entry error\n");
		goto dt_read_done;
	}

	/* Read the fdt from oem_bootloader into hdr->second_addr. */
	fdt_size = be32_to_cpu(dt_entry.dt_size);
	if (fsl_avb_ops.read_from_partition(&fsl_avb_ops, oemimage,
			be32_to_cpu(dt_entry.dt_offset), fdt_size,
			(void *)(ulong)hdr->second_addr, &num_read) !=
			AVB_IO_RESULT_OK && num_read != fdt_size) {
		printf("boota: read fdt error\n");
	}

dt_read_done:
	;
#else /* CONFIG_LOAD_FDT_FROM_PART */
	if (hdr->second_size && hdr->second_addr) {
		memcpy((void *)(ulong)hdr->second_addr, (void *)(ulong)hdr + hdr->page_size
			+ ALIGN(hdr->kernel_size, hdr->page_size)
			+ ALIGN(hdr->ramdisk_size, hdr->page_size), hdr->second_size);
	}
#endif /* CONFIG_LOAD_FDT_FROM_PART */
#endif /*CONFIG_OF_LIBFDT*/
	if (check_image_arm64) {
		android_image_get_kernel(hdr, 0, NULL, NULL);
		addr = hdr->kernel_addr;
	} else {
		addr = (ulong)(hdr->kernel_addr - hdr->page_size);
	}
	printf("kernel   @ %08x (%d)\n", hdr->kernel_addr, hdr->kernel_size);
	printf("ramdisk  @ %08x (%d)\n", hdr->ramdisk_addr, hdr->ramdisk_size);
#ifdef CONFIG_OF_LIBFDT
#ifdef CONFIG_LOAD_FDT_FROM_PART
	if (fdt_size)
		printf("fdt      @ %08x (%d)\n", hdr->second_addr, fdt_size);
#else
	if (hdr->second_size)
		printf("fdt      @ %08x (%d)\n", hdr->second_addr, hdr->second_size);
#endif
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

/* no need to pass ramdisk addr for normal boot mode when enable CONFIG_SYSTEM_RAMDISK_SUPPORT*/
#ifdef CONFIG_SYSTEM_RAMDISK_SUPPORT
	if (!is_recovery_mode)
		boot_args[2] = NULL;
#endif
	if (avb_out_data != NULL)
		avb_slot_verify_data_free(avb_out_data);
	if (boot_buf != NULL)
		free(boot_buf);

#ifdef CONFIG_IMX_TRUSTY_OS
	/* Trusty keymaster needs some parameters before it work */
	trusty_setbootparameter(hdr, avb_result);
	/* lock the boot status and rollback_idx preventing Linux modify it */
	trusty_lock_boot_state();
	/* put ql-tipc to release resource for Linux */
	trusty_ipc_shutdown();
#endif
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

		if (mmc->block_dev.block_read(dev_desc,	pte->start,
					bootimg_sectors,
					(void *)(hdr->kernel_addr - hdr->page_size)) < 0) {
			printf("boota: mmc failed to read bootimage\n");
			goto fail;
		}
		check_image_arm64  = image_arm64((void *)hdr->kernel_addr);
#ifdef CONFIG_FASTBOOT_LOCK
		int verifyresult = -1;
#endif

#ifdef CONFIG_FASTBOOT_LOCK
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
#endif

#ifdef CONFIG_USB_GADGET
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

static void fastboot_fifo_complete(struct usb_ep *ep, struct usb_request *req)
{
	int status = req->status;
	usb_req *request;

	if (!status) {
		if (fastboot_func->front != NULL) {
			request = fastboot_func->front;
			fastboot_func->front = fastboot_func->front->next;
			usb_ep_free_request(ep, request->in_req);
			free(request);
		} else {
			printf("fail free request\n");
		}
		return;
	}
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

	/* Enable OS and Extended Properties Feature Descriptor */
	c->cdev->use_os_string = 1;
	f->os_desc_table = &fb_os_desc_table;
	f->os_desc_n = 1;
	f->os_desc_table->if_id = id;
	INIT_LIST_HEAD(&fb_os_desc.ext_prop);
	fb_ext_prop.name_len = strlen(fb_ext_prop.name) * 2 + 2;
	fb_os_desc.ext_prop_len = 14 + fb_ext_prop.name_len;
	fb_os_desc.ext_prop_count = 1;
	fb_ext_prop.data_len = strlen(fb_ext_prop.data);
	fb_os_desc.ext_prop_len += fb_ext_prop.data_len;
	list_add_tail(&fb_ext_prop.entry, &fb_os_desc.ext_prop);

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
	f->os_desc_table = NULL;
	list_del(&fb_os_desc.ext_prop);
	memset(fastboot_func, 0, sizeof(*fastboot_func));
}

static void fastboot_disable(struct usb_function *f)
{
	usb_req *req;
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

		/* disable usb request FIFO */
		while(f_fb->front != NULL) {
			req = f_fb->front;
			f_fb->front = f_fb->front->next;
			free(req->in_req->buf);
			usb_ep_free_request(f_fb->in_ep, req->in_req);
			free(req);
		}

		f_fb->rear = NULL;
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
#ifdef CONFIG_ANDROID_THINGS_SUPPORT
	/*
	 * fastboot host end implement to get data in one bulk package so need
	 * large buffer for the "fastboot upload" and "fastboot get_staged".
	 */
	if (f_fb->in_req->buf)
		free(f_fb->in_req->buf);
	f_fb->in_req->buf = memalign(CONFIG_SYS_CACHELINE_SIZE, EP_BUFFER_SIZE * 32);
#endif
	f_fb->in_req->complete = fastboot_complete;

	f_fb->front = f_fb->rear = NULL;

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
		fastboot_func = NULL;
	}

	return status;
}
DECLARE_GADGET_BIND_CALLBACK(usb_dnl_fastboot, fastboot_add);

static int fastboot_tx_write_more(const char *buffer)
{
	int ret = 0;

	/* alloc usb request FIFO node */
	usb_req *req = (usb_req *)malloc(sizeof(usb_req));
	if (!req) {
		printf("failed alloc usb req!\n");
		return -ENOMEM;
	}

	/* usb request node FIFO enquene */
	if ((fastboot_func->front == NULL) && (fastboot_func->rear == NULL)) {
		fastboot_func->front = fastboot_func->rear = req;
		req->next = NULL;
	} else {
		fastboot_func->rear->next = req;
		fastboot_func->rear = req;
		req->next = NULL;
	}

	/* alloc in request for current node */
	req->in_req = fastboot_start_ep(fastboot_func->in_ep);
	if (!req->in_req) {
		printf("failed alloc req in\n");
		fastboot_disable(&(fastboot_func->usb_function));
		return  -EINVAL;
	}
	req->in_req->complete = fastboot_fifo_complete;

	memcpy(req->in_req->buf, buffer, strlen(buffer));
	req->in_req->length = strlen(buffer);

	ret = usb_ep_queue(fastboot_func->in_ep, req->in_req, 0);
	if (ret) {
		printf("Error %d on queue\n", ret);
		return -EINVAL;
	}

	ret = 0;
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
#ifdef CONFIG_IMX_TRUSTY_OS
static void uuid_hex2string(uint8_t *uuid, char* buf, uint32_t uuid_len, uint32_t uuid_strlen) {
	uint32_t i;
	if (!uuid || !buf)
		return;
	char *cp = buf;
	char *buf_end = buf + uuid_strlen;
	for (i = 0; i < uuid_len; i++) {
		cp += snprintf(cp, buf_end - cp, "%02x", uuid[i]);
	}
}
#endif

#if defined(CONFIG_ANDROID_THINGS_SUPPORT) && defined(CONFIG_ARCH_IMX8M)
static int get_imx8m_baseboard_id(void)
{
	int  i = 0, value = 0;
	int baseboard_id;
	int pin[7];

	/* initialize the pin array */
	pin[0] = IMX8MQ_GPIO3_IO21;
	pin[1] = IMX8MQ_GPIO3_IO22;
	pin[2] = IMX8MQ_GPIO3_IO23;
	pin[3] = IMX8MQ_GPIO3_IO24;
	pin[4] = IMX8MQ_GPIO3_IO20;
	pin[5] = IMX8MQ_GPIO3_IO19;
	pin[6] = IMX8MQ_GPIO3_IO25;

	/* request gpio */
	gpio_request(IMX8MQ_GPIO3_IO21, "GPIO3_IO21");
	gpio_request(IMX8MQ_GPIO3_IO22, "GPIO3_IO22");
	gpio_request(IMX8MQ_GPIO3_IO23, "GPIO3_IO23");
	gpio_request(IMX8MQ_GPIO3_IO24, "GPIO3_IO24");
	gpio_request(IMX8MQ_GPIO3_IO20, "GPIO3_IO20");
	gpio_request(IMX8MQ_GPIO3_IO19, "GPIO3_IO19");
	gpio_request(IMX8MQ_GPIO3_IO25, "GPIO3_IO25");

	/* Set gpio direction as input and get the input value */
	baseboard_id = 0;
	for (i = 0; i < 7; i++) {
		gpio_direction_input(pin[i]);
		if ((value = gpio_get_value(pin[i])) < 0) {
			printf("Error! Read gpio port: %d failed!\n", pin[i]);
			return -1;
		}
		else
			baseboard_id |= ((value & 0x01) << i);
	}

	return baseboard_id;
}
#endif

static int get_single_var(char *cmd, char *response)
{
	char *str = cmd;
	size_t chars_left;
	const char *s;
	struct mmc *mmc;
	int mmc_dev_no;
	int blksz;

	chars_left = FASTBOOT_RESPONSE_LEN - strlen(response) - 1;

	if ((str = strstr(cmd, "partition-size:"))) {
		str +=strlen("partition-size:");
		struct fastboot_ptentry* fb_part;
		fb_part = fastboot_flash_find_ptn(str);
		if (!fb_part) {
			strncat(response, "Wrong partition name.", chars_left);
			return -1;
		} else {
			snprintf(response + strlen(response), chars_left,
				 "0x%lx",
				 (ulong)fb_part->length * get_block_size());
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
		mmc_dev_no = mmc_get_env_dev();
		mmc = find_mmc_device(mmc_dev_no);
		blksz = get_block_size();
		snprintf(response + strlen(response), chars_left, "0x%x",
				(blksz * mmc->erase_grp_size));
	} else if (!strcmp_l1("logical-block-size", cmd)) {
		blksz = get_block_size();
		snprintf(response + strlen(response), chars_left, "0x%x", blksz);
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
#ifdef CONFIG_IMX_TRUSTY_OS
        else if(!strcmp_l1("at-attest-uuid", cmd)) {
		char *uuid;
		char uuid_str[ATAP_UUID_STR_SIZE];
		if (trusty_atap_read_uuid_str(&uuid)) {
			printf("ERROR read uuid failed!\n");
			strncat(response, "FAILCannot get uuid!", chars_left);
			return -1;
		} else {
			uuid_hex2string((uint8_t*)uuid, uuid_str,ATAP_UUID_SIZE, ATAP_UUID_STR_SIZE);
			strncat(response, uuid_str, chars_left);
			trusty_free(uuid);
		}
	}
#endif
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
#if defined(CONFIG_ANDROID_THINGS_SUPPORT) && defined(CONFIG_ARCH_IMX8M)
	else if (!strcmp_l1("baseboard_id", cmd)) {
		int baseboard_id;

		baseboard_id = get_imx8m_baseboard_id();
		if (baseboard_id < 0) {
			printf("Get baseboard id failed!\n");
			strncat(response, "Get baseboard id failed!", chars_left);
			return -1;
		} else
			snprintf(response + strlen(response), chars_left, "0x%x", baseboard_id);
	}
#endif
	else {
		char envstr[32];

		snprintf(envstr, sizeof(envstr) - 1, "fastboot.%s", cmd);
		s = getenv(envstr);
		if (s) {
			strncat(response, s, chars_left);
		} else {
			snprintf(response, chars_left, "FAILunknown variable:%s",cmd);
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
	char slot_suffix[2][5] = {"a","b"};
	char response[FASTBOOT_RESPONSE_LEN - 1];

	strsep(&cmd, ":");
	if (!cmd) {
		error("missing variable");
		fastboot_tx_write_str("FAILmissing var");
		return;
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
	void * base_addr = (void*)getenv_ulong("fastboot_buffer", 16, CONFIG_FASTBOOT_BUF_ADDR);

	if (req->status != 0) {
		printf("Bad status: %d\n", req->status);
		return;
	}

	if (buffer_size < transfer_size)
		transfer_size = buffer_size;

	memcpy(base_addr + download_bytes,
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
		setenv_hex("fastboot_bytes", download_bytes);

		printf("\ndownloading of %d bytes finished\n", download_bytes);

	} else {
		req->length = rx_bytes_expected(ep);
	}

	req->actual = 0;
	usb_ep_queue(ep, req, 0);
}

static void cb_upload(struct usb_ep *ep, struct usb_request *req)
{
	char response[FASTBOOT_RESPONSE_LEN];

	if (!download_bytes || download_bytes > (EP_BUFFER_SIZE * 32)) {
		sprintf(response, "FAIL");
		fastboot_tx_write_str(response);
		return;
	}

	printf("Will upload %d bytes.\n", download_bytes);
	snprintf(response, FASTBOOT_RESPONSE_LEN, "DATA%08x", download_bytes);
	fastboot_tx_write_more(response);

	fastboot_tx_write((const char *)(interface.transfer_buffer), download_bytes);

	snprintf(response,FASTBOOT_RESPONSE_LEN, "OKAY");
	fastboot_tx_write_more(response);
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

#if defined(CONFIG_AVB_SUPPORT) && !defined(CONFIG_IMX_TRUSTY_OS)
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

static bool endswith(char* s, char* subs) {
	if (!s || !subs)
		return false;
	uint32_t len = strlen(s);
	uint32_t sublen = strlen(subs);
	if (len < sublen) {
		return false;
	}
	if (strncmp(s + len - sublen, subs, sublen)) {
		return false;
	}
	return true;
}

static void cb_flashing(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char response[FASTBOOT_RESPONSE_LEN];
	FbLockState status;
	FbLockEnableResult result;
	if (endswith(cmd, "lock_critical")) {
		strcpy(response, "OKAY");
	}
#ifdef CONFIG_AVB_ATX
	else if (endswith(cmd, FASTBOOT_AVB_AT_PERM_ATTR)) {
		if (avb_atx_fuse_perm_attr(interface.transfer_buffer, download_bytes))
			strcpy(response, "FAILInternal error!");
		else
			strcpy(response, "OKAY");
	}
#endif /* CONFIG_AVB_ATX */
#ifdef CONFIG_ANDROID_THINGS_SUPPORT
	else if (endswith(cmd, FASTBOOT_BOOTLOADER_VBOOT_KEY)) {
		strcpy(response, "OKAY");
	}
#endif /* CONFIG_ANDROID_THINGS_SUPPORT */
#ifdef CONFIG_IMX_TRUSTY_OS
	else if (endswith(cmd, FASTBOOT_GET_CA_REQ)) {
		uint8_t *ca_output;
		uint32_t ca_length, cp_length;
		if (trusty_atap_get_ca_request(interface.transfer_buffer, download_bytes,
						&(ca_output), &ca_length)) {
			printf("ERROR get_ca_request failed!\n");
			strcpy(response, "FAILInternal error!");
		} else {
			cp_length = min((uint32_t)CONFIG_FASTBOOT_BUF_SIZE, ca_length);
			memcpy(interface.transfer_buffer, ca_output, cp_length);
			download_bytes = ca_length;
			strcpy(response, "OKAY");
		}

	}
	else if (endswith(cmd, FASTBOOT_SET_CA_RESP)) {
		if (trusty_atap_set_ca_response(interface.transfer_buffer,download_bytes)) {
			printf("ERROR set_ca_response failed!\n");
			strcpy(response, "FAILInternal error!");
		} else
			strcpy(response, "OKAY");
	}
#endif /* CONFIG_IMX_TRUSTY_OS */
	else if (endswith(cmd, "unlock_critical")) {
		strcpy(response, "OKAY");
	} else if (endswith(cmd, "unlock")) {
		printf("flashing unlock.\n");
		status = do_fastboot_unlock(false);
		if (status != FASTBOOT_LOCK_ERROR)
			strcpy(response, "OKAY");
		else
			strcpy(response, "FAILunlock device failed.");
	} else if (endswith(cmd, "lock")) {
		printf("flashing lock.\n");
		status = do_fastboot_lock();
		if (status != FASTBOOT_LOCK_ERROR)
			strcpy(response, "OKAY");
		else
			strcpy(response, "FAILlock device failed.");
	} else if (endswith(cmd, "get_unlock_ability")) {
		result = fastboot_lock_enable();
		if (result == FASTBOOT_UL_ENABLE) {
			fastboot_tx_write_more("INFO1");
			strcpy(response, "OKAY");
		} else if (result == FASTBOOT_UL_DISABLE) {
			fastboot_tx_write_more("INFO0");
			strcpy(response, "OKAY");
		} else {
			printf("flashing get_unlock_ability fail!\n");
			strcpy(response, "FAILget unlock ability failed.");
		}
	} else {
		printf("Unknown flashing command:%s\n", cmd);
		strcpy(response, "FAILcommand not defined");
	}
	fastboot_tx_write_more(response);
}

#endif /* CONFIG_FASTBOOT_LOCK */

#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_FASTBOOT_LOCK
static int partition_table_valid(void)
{
	int status, mmc_no;
	struct blk_desc *dev_desc;
#ifdef CONFIG_IMX_TRUSTY_OS
	//Prevent other partition accessing when no TOS flashed.
	if (!tos_flashed)
		return 0;
#endif
	disk_partition_t info;
	mmc_no = fastboot_devinfo.dev_id;
	dev_desc = blk_get_dev("mmc", mmc_no);
	if (dev_desc)
		status = part_get_info(dev_desc, 1, &info);
	else
		status = -1;
	return (status == 0);
}
#endif
#endif

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
		strcpy(response, "FAILdevice is locked.");
		fastboot_tx_write_str(response);
		return;

	} else if (status == FASTBOOT_LOCK_ERROR) {
		error("write lock status into device!\n");
		fastboot_set_lock_stat(FASTBOOT_LOCK);
		strcpy(response, "FAILdevice is locked.");
		fastboot_tx_write_str(response);
		return;
	}
#endif
	fastboot_fail("no flash device defined");

#ifdef CONFIG_FSL_FASTBOOT
	rx_process_flash(cmd);
#ifdef CONFIG_FASTBOOT_LOCK
	if (strncmp(cmd, "gpt", 3) == 0) {
		int gpt_valid = 0;
		gpt_valid = partition_table_valid();
		/* If gpt is valid, load partitons table into memory.
		   So if the next command is "fastboot reboot bootloader",
		   it can find the "misc" partition to r/w. */
		if(gpt_valid) {
			_fastboot_load_partitions();
			/* Unlock device if the gpt is valid */
			do_fastboot_unlock(true);
		}
	}

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
		strcpy(response, "FAILdevice is locked.");
		fastboot_tx_write_str(response);
		return;
	} else if (status == FASTBOOT_LOCK_ERROR) {
		error("write lock status into device!\n");
		fastboot_set_lock_stat(FASTBOOT_LOCK);
		strcpy(response, "FAILdevice is locked.");
		fastboot_tx_write_str(response);
		return;
	}
#endif
	rx_process_erase(cmd, response);
	fastboot_tx_write_str(response);
}
#endif

#ifdef CONFIG_FSL_FASTBOOT
static void cb_run_uboot_cmd(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	strsep(&cmd, ":");
	if (!cmd) {
		error("missing slot suffix\n");
		fastboot_tx_write_str("FAILmissing command");
		return;
	}
	if(run_command(cmd, 0)) {
		fastboot_tx_write_str("FAIL");
	} else {
		fastboot_tx_write_str("OKAY");
		/* cmd may impact fastboot related environment*/
		fastboot_setup();
	}
	return ;
}

static char g_a_cmd_buff[64];
static void do_acmd_complete(struct usb_ep *ep, struct usb_request *req)
{
	/* When usb dequeue complete will be called
	 * Meed status value before call run_command.
	 * otherwise, host can't get last message.
	 */
	if(req->status == 0)
		run_command(g_a_cmd_buff, 0);
}

static void cb_run_uboot_acmd(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
        strsep(&cmd, ":");
        if (!cmd) {
                error("missing slot suffix\n");
                fastboot_tx_write_str("FAILmissing command");
                return;
        }
	strcpy(g_a_cmd_buff, cmd);
	fastboot_func->in_req->complete = do_acmd_complete;
	fastboot_tx_write_str("OKAY");
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
	enable_fastboot_command();
        fastboot_func->in_req->complete = compl_do_reset;
        fastboot_tx_write_str("OKAY");
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
		.cmd = "upload",
		.cb = cb_upload,
	}, {
		.cmd = "get_staged",
		.cb = cb_upload,
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
#ifdef CONFIG_FSL_FASTBOOT
	{
		.cmd = "UCmd:",
		.cb = cb_run_uboot_cmd,
	},
	{	.cmd ="ACmd:",
		.cb = cb_run_uboot_acmd,
	},
#endif
#ifdef CONFIG_AVB_ATX
	{
		.cmd = "stage",
		.cb = cb_download,
	},
#endif
};

static void rx_handler_command(struct usb_ep *ep, struct usb_request *req)
{
	char *cmdbuf = req->buf;
	void (*func_cb)(struct usb_ep *ep, struct usb_request *req) = NULL;
	int i;

	/* init in request FIFO pointer */
	fastboot_func->front = NULL;
	fastboot_func->rear  = NULL;

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
#endif
