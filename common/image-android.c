// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2011 Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 *
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 */

#include <common.h>
#include <image.h>
#include <android_image.h>
#include <malloc.h>
#include <errno.h>
#include <asm/bootm.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/arch/sys_proto.h>
#include <fb_fsl.h>
#include <asm/setup.h>
#include <dm.h>
#include <mmc.h>

#define ANDROID_IMAGE_DEFAULT_KERNEL_ADDR	0x10008000

static char andr_tmp_str[ANDR_BOOT_ARGS_SIZE + 1];

static ulong android_image_get_kernel_addr(const struct andr_img_hdr *hdr)
{
	/*
	 * All the Android tools that generate a boot.img use this
	 * address as the default.
	 *
	 * Even though it doesn't really make a lot of sense, and it
	 * might be valid on some platforms, we treat that adress as
	 * the default value for this field, and try to execute the
	 * kernel in place in such a case.
	 *
	 * Otherwise, we will return the actual value set by the user.
	 */
	if (hdr->kernel_addr == ANDROID_IMAGE_DEFAULT_KERNEL_ADDR)
		return (ulong)hdr + hdr->page_size;

	return hdr->kernel_addr;
}

/**
 * android_image_get_kernel() - processes kernel part of Android boot images
 * @hdr:	Pointer to image header, which is at the start
 *			of the image.
 * @verify:	Checksum verification flag. Currently unimplemented.
 * @os_data:	Pointer to a ulong variable, will hold os data start
 *			address.
 * @os_len:	Pointer to a ulong variable, will hold os data length.
 *
 * This function returns the os image's start address and length. Also,
 * it appends the kernel command line to the bootargs env variable.
 *
 * Return: Zero, os start address and length on success,
 *		otherwise on failure.
 */
int android_image_get_kernel(const struct andr_img_hdr *hdr, int verify,
			     ulong *os_data, ulong *os_len)
{
	extern boot_metric metrics;
	u32 kernel_addr = android_image_get_kernel_addr(hdr);

	/*
	 * Not all Android tools use the id field for signing the image with
	 * sha1 (or anything) so we don't check it. It is not obvious that the
	 * string is null terminated so we take care of this.
	 */
	strncpy(andr_tmp_str, hdr->name, ANDR_BOOT_NAME_SIZE);
	andr_tmp_str[ANDR_BOOT_NAME_SIZE] = '\0';
	if (strlen(andr_tmp_str))
		printf("Android's image name: %s\n", andr_tmp_str);

	printf("Kernel load addr 0x%08x size %u KiB\n",
	       kernel_addr, DIV_ROUND_UP(hdr->kernel_size, 1024));

	char newbootargs[512] = {0};
	char commandline[2048] = {0};
	int offset;
	char *bootargs = env_get("bootargs");

	if (bootargs) {
		if (strlen(bootargs) + 1 > sizeof(commandline)) {
			printf("bootargs is too long!\n");
			return -1;
		}
		else
			strncpy(commandline, bootargs, sizeof(commandline) - 1);
	} else {
		offset = fdt_path_offset(gd->fdt_blob, "/chosen");
		if (offset > 0) {
			bootargs = (char *)fdt_getprop(gd->fdt_blob, offset,
							"bootargs", NULL);
			if (bootargs)
				sprintf(commandline, "%s ", bootargs);
		}

		if (*hdr->cmdline) {
			if (strlen(hdr->cmdline) + 1 >
				sizeof(commandline) - strlen(commandline)) {
				printf("cmdline in bootimg is too long!\n");
				return -1;
			}
			else
				strncat(commandline, hdr->cmdline, sizeof(commandline) - strlen(commandline));
		}
	}

	/* Add 'bootargs_ram_capacity' to hold the parameters based on different ram capacity */
	char *bootargs_ram_capacity = env_get("bootargs_ram_capacity");
	if (bootargs_ram_capacity) {
		strncat(commandline, " ", sizeof(commandline) - strlen(commandline));
		strncat(commandline, bootargs_ram_capacity,
			sizeof(commandline) - strlen(commandline));
	}

#ifdef CONFIG_SERIAL_TAG
	struct tag_serialnr serialnr;
	get_board_serial(&serialnr);

	sprintf(newbootargs,
					" androidboot.serialno=%08x%08x",
					serialnr.high,
					serialnr.low);
	strncat(commandline, newbootargs, sizeof(commandline) - strlen(commandline));

	if (serialnr.high + serialnr.low != 0) {
		char bd_addr[16]={0};
		sprintf(bd_addr,
			"%08x%08x",
			serialnr.high,
			serialnr.low);
		sprintf(newbootargs,
			" androidboot.btmacaddr=%c%c:%c%c:%c%c:%c%c:%c%c:%c%c",
			bd_addr[0],bd_addr[1],bd_addr[2],bd_addr[3],bd_addr[4],bd_addr[5],
			bd_addr[6],bd_addr[7],bd_addr[8],bd_addr[9],bd_addr[10],bd_addr[11]);
		strncat(commandline, newbootargs, sizeof(commandline) - strlen(commandline));
	}
#endif

	/* append soc type into bootargs */
	char *soc_type = env_get("soc_type");
	if (soc_type) {
		sprintf(newbootargs,
			" androidboot.soc_type=%s",
			soc_type);
		strncat(commandline, newbootargs, sizeof(commandline) - strlen(commandline));
	}

	sprintf(newbootargs,
			" androidboot.boot_device_root=mmcblk%d", mmc_map_to_kernel_blk(mmc_get_env_dev()));
	strncat(commandline, newbootargs, sizeof(commandline) - strlen(commandline));

	/* boot metric variables */
	metrics.ble_1 = get_timer(0);
	sprintf(newbootargs,
		" androidboot.boottime=1BLL:%d,1BLE:%d,KL:%d,KD:%d,AVB:%d,ODT:%d,SW:%d",
		metrics.bll_1, metrics.ble_1, metrics.kl, metrics.kd, metrics.avb,
		metrics.odt, metrics.sw);
	strncat(commandline, newbootargs, sizeof(commandline) - strlen(commandline));

#if defined(CONFIG_ARCH_MX6) || defined(CONFIG_ARCH_MX7) || \
	defined(CONFIG_ARCH_MX7ULP) || defined(CONFIG_ARCH_IMX8M)
	char cause[18];

	memset(cause, '\0', sizeof(cause));
	get_reboot_reason(cause);
	if (strstr(cause, "POR"))
		sprintf(newbootargs," androidboot.bootreason=cold,powerkey");
	else if (strstr(cause, "WDOG") || strstr(cause, "WDG"))
		sprintf(newbootargs," androidboot.bootreason=watchdog");
	else
		sprintf(newbootargs," androidboot.bootreason=reboot");
#else
	sprintf(newbootargs," androidboot.bootreason=reboot");
#endif
	strncat(commandline, newbootargs, sizeof(commandline) - strlen(commandline));

#ifdef CONFIG_AVB_SUPPORT
	/* secondary cmdline added by avb */
	char *bootargs_sec = env_get("bootargs_sec");
	if (bootargs_sec) {
		strncat(commandline, " ", sizeof(commandline) - strlen(commandline));
		strncat(commandline, bootargs_sec, sizeof(commandline) - strlen(commandline));
	}
#endif
#ifdef CONFIG_SYSTEM_RAMDISK_SUPPORT
	/* Normal boot:
	 * cmdline to bypass ramdisk in boot.img, but use the system.img
	 * Recovery boot:
	 * Use the ramdisk in boot.img
	 */
	char *bootargs_3rd = env_get("bootargs_3rd");
	if (bootargs_3rd) {
		strncat(commandline, " ", sizeof(commandline) - strlen(commandline));
		strncat(commandline, bootargs_3rd, sizeof(commandline) - strlen(commandline));
	}
#endif

	/* VTS need this commandline to verify fdt overlay. Pass the
	 * dtb index as "0" here since we only have one dtb in dtbo
	 * partition and haven't enabled the dtb overlay.
	 */
#if defined(CONFIG_ANDROID_SUPPORT) || defined(CONFIG_ANDROID_AUTO_SUPPORT)
	sprintf(newbootargs," androidboot.dtbo_idx=0");
	strncat(commandline, newbootargs, sizeof(commandline) - strlen(commandline));
#endif

	char *keystore = env_get("keystore");
	if ((keystore == NULL) || strncmp(keystore, "trusty", sizeof("trusty"))) {
		char *bootargs_trusty = "androidboot.keystore=software";
		strncat(commandline, " ", sizeof(commandline) - strlen(commandline));
		strncat(commandline, bootargs_trusty, sizeof(commandline) - strlen(commandline));
	} else {
		char *bootargs_trusty = "androidboot.keystore=trusty";
		strncat(commandline, " ", sizeof(commandline) - strlen(commandline));
		strncat(commandline, bootargs_trusty, sizeof(commandline) - strlen(commandline));
	}

#ifdef CONFIG_APPEND_BOOTARGS
	/* Add 'append_bootargs' to hold some paramemters which need to be appended
	 * to bootargs */
	char *append_bootargs = env_get("append_bootargs");
	if (append_bootargs) {
		if (strlen(append_bootargs) + 2 >
				(sizeof(commandline) - strlen(commandline))) {
			printf("The 'append_bootargs' is too long to be appended to bootargs\n");
		} else {
			strncat(commandline, " ", sizeof(commandline) - strlen(commandline));
			strncat(commandline, append_bootargs, sizeof(commandline) - strlen(commandline));
		}
	}
#endif

	debug("Kernel command line: %s\n", commandline);
	env_set("bootargs", commandline);

	if (os_data) {
		*os_data = (ulong)hdr;
		*os_data += hdr->page_size;
	}
	if (os_len)
		*os_len = hdr->kernel_size;
	return 0;
}

int android_image_check_header(const struct andr_img_hdr *hdr)
{
	return memcmp(ANDR_BOOT_MAGIC, hdr->magic, ANDR_BOOT_MAGIC_SIZE);
}

ulong android_image_get_end(const struct andr_img_hdr *hdr)
{
	ulong end;
	/*
	 * The header takes a full page, the remaining components are aligned
	 * on page boundary
	 */
	end = (ulong)hdr;
	end += hdr->page_size;
	end += ALIGN(hdr->kernel_size, hdr->page_size);
	end += ALIGN(hdr->ramdisk_size, hdr->page_size);
	end += ALIGN(hdr->second_size, hdr->page_size);

	return end;
}

ulong android_image_get_kload(const struct andr_img_hdr *hdr)
{
	return android_image_get_kernel_addr(hdr);
}

int android_image_get_ramdisk(const struct andr_img_hdr *hdr,
			      ulong *rd_data, ulong *rd_len)
{
	if (!hdr->ramdisk_size) {
		*rd_data = *rd_len = 0;
		return -1;
	}

	printf("RAM disk load addr 0x%08x size %u KiB\n",
	       hdr->ramdisk_addr, DIV_ROUND_UP(hdr->ramdisk_size, 1024));

	*rd_data = (unsigned long)hdr;
	*rd_data += hdr->page_size;
	*rd_data += ALIGN(hdr->kernel_size, hdr->page_size);

	*rd_len = hdr->ramdisk_size;
	return 0;
}

int android_image_get_second(const struct andr_img_hdr *hdr,
			      ulong *second_data, ulong *second_len)
{
	if (!hdr->second_size) {
		*second_data = *second_len = 0;
		return -1;
	}

	*second_data = (unsigned long)hdr;
	*second_data += hdr->page_size;
	*second_data += ALIGN(hdr->kernel_size, hdr->page_size);
	*second_data += ALIGN(hdr->ramdisk_size, hdr->page_size);

	printf("second address is 0x%lx\n",*second_data);

	*second_len = hdr->second_size;
	return 0;
}

#if !defined(CONFIG_SPL_BUILD)
/**
 * android_print_contents - prints out the contents of the Android format image
 * @hdr: pointer to the Android format image header
 *
 * android_print_contents() formats a multi line Android image contents
 * description.
 * The routine prints out Android image properties
 *
 * returns:
 *     no returned results
 */
void android_print_contents(const struct andr_img_hdr *hdr)
{
	const char * const p = IMAGE_INDENT_STRING;
	/* os_version = ver << 11 | lvl */
	u32 os_ver = hdr->os_version >> 11;
	u32 os_lvl = hdr->os_version & ((1U << 11) - 1);

	printf("%skernel size:      %x\n", p, hdr->kernel_size);
	printf("%skernel address:   %x\n", p, hdr->kernel_addr);
	printf("%sramdisk size:     %x\n", p, hdr->ramdisk_size);
	printf("%sramdisk addrress: %x\n", p, hdr->ramdisk_addr);
	printf("%ssecond size:      %x\n", p, hdr->second_size);
	printf("%ssecond address:   %x\n", p, hdr->second_addr);
	printf("%stags address:     %x\n", p, hdr->tags_addr);
	printf("%spage size:        %x\n", p, hdr->page_size);
	/* ver = A << 14 | B << 7 | C         (7 bits for each of A, B, C)
	 * lvl = ((Y - 2000) & 127) << 4 | M  (7 bits for Y, 4 bits for M) */
	printf("%sos_version:       %x (ver: %u.%u.%u, level: %u.%u)\n",
	       p, hdr->os_version,
	       (os_ver >> 7) & 0x7F, (os_ver >> 14) & 0x7F, os_ver & 0x7F,
	       (os_lvl >> 4) + 2000, os_lvl & 0x0F);
	printf("%sname:             %s\n", p, hdr->name);
	printf("%scmdline:          %s\n", p, hdr->cmdline);
}
#endif

#define ARM64_IMAGE_MAGIC	0x644d5241
bool image_arm64(void *images)
{
	struct header_image *ih;

	ih = (struct header_image *)images;
	debug("image magic: %x\n", ih->magic);
	if (ih->magic == le32_to_cpu(ARM64_IMAGE_MAGIC))
		return true;
	return false;
}
