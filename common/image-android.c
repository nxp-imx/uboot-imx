// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2011 Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 *
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 */

#include <common.h>
#include <env.h>
#include <image.h>
#include <image-android-dt.h>
#include <android_image.h>
#include <malloc.h>
#include <errno.h>
#include <asm/unaligned.h>
#include <mapmem.h>
#include <asm/bootm.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/arch/sys_proto.h>
#include <fb_fsl.h>
#include <asm/setup.h>
#include <dm.h>
#include <mmc.h>

#define ANDROID_IMAGE_DEFAULT_KERNEL_ADDR	0x10008000
#define COMMANDLINE_LENGTH			2048

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

static void append_kernel_cmdline(char *commandline)
{
	char newbootargs[512] = {0};
	extern boot_metric metrics;

	/* Add 'bootargs_ram_capacity' to hold the parameters based on different ram capacity */
	char *bootargs_ram_capacity = env_get("bootargs_ram_capacity");
	if (bootargs_ram_capacity) {
		strncat(commandline, " ", COMMANDLINE_LENGTH - strlen(commandline));
		strncat(commandline, bootargs_ram_capacity,
			COMMANDLINE_LENGTH - strlen(commandline));
	}

#ifdef CONFIG_SERIAL_TAG
	struct tag_serialnr serialnr;
	get_board_serial(&serialnr);

	sprintf(newbootargs,
					" androidboot.serialno=%08x%08x",
					serialnr.high,
					serialnr.low);
	strncat(commandline, newbootargs, COMMANDLINE_LENGTH - strlen(commandline));

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
	} else {
		/* Some boards have serial number as all zeros (imx8mp),
		 * hard code the bt mac address for such case. */
		sprintf(newbootargs, " androidboot.btmacaddr=22:22:67:C6:69:73");
	}

	strncat(commandline, newbootargs, COMMANDLINE_LENGTH - strlen(commandline));
#endif

	/* append soc type into bootargs */
	char *soc_type = env_get("soc_type");
	if (soc_type) {
		sprintf(newbootargs,
			" androidboot.soc_type=%s",
			soc_type);
		strncat(commandline, newbootargs, COMMANDLINE_LENGTH - strlen(commandline));
	}

	sprintf(newbootargs,
			" androidboot.boot_device_root=mmcblk%d", mmc_map_to_kernel_blk(mmc_get_env_dev()));
	strncat(commandline, newbootargs, COMMANDLINE_LENGTH - strlen(commandline));

	/* boot metric variables */
	metrics.ble_1 = get_timer(0);
	sprintf(newbootargs,
		" androidboot.boottime=1BLL:%d,1BLE:%d,KL:%d,KD:%d,AVB:%d,ODT:%d,SW:%d",
		metrics.bll_1, metrics.ble_1, metrics.kl, metrics.kd, metrics.avb,
		metrics.odt, metrics.sw);
	strncat(commandline, newbootargs, COMMANDLINE_LENGTH - strlen(commandline));

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
	strncat(commandline, newbootargs, COMMANDLINE_LENGTH - strlen(commandline));

#ifdef CONFIG_AVB_SUPPORT
	/* secondary cmdline added by avb */
	char *bootargs_sec = env_get("bootargs_sec");
	if (bootargs_sec) {
		strncat(commandline, " ", COMMANDLINE_LENGTH - strlen(commandline));
		strncat(commandline, bootargs_sec, COMMANDLINE_LENGTH - strlen(commandline));
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
		strncat(commandline, " ", COMMANDLINE_LENGTH - strlen(commandline));
		strncat(commandline, bootargs_3rd, COMMANDLINE_LENGTH - strlen(commandline));
	}
#endif

	/* VTS need this commandline to verify fdt overlay. Pass the
	 * dtb index as "0" here since we only have one dtb in dtbo
	 * partition and haven't enabled the dtb overlay.
	 */
#if defined(CONFIG_ANDROID_SUPPORT) || defined(CONFIG_ANDROID_AUTO_SUPPORT)
	sprintf(newbootargs," androidboot.dtbo_idx=0");
	strncat(commandline, newbootargs, COMMANDLINE_LENGTH - strlen(commandline));
#endif

	char *keystore = env_get("keystore");
	if ((keystore == NULL) || strncmp(keystore, "trusty", sizeof("trusty"))) {
		char *bootargs_trusty = "androidboot.keystore=software";
		strncat(commandline, " ", COMMANDLINE_LENGTH - strlen(commandline));
		strncat(commandline, bootargs_trusty, COMMANDLINE_LENGTH - strlen(commandline));
	} else {
		char *bootargs_trusty = "androidboot.keystore=trusty";
		strncat(commandline, " ", COMMANDLINE_LENGTH - strlen(commandline));
		strncat(commandline, bootargs_trusty, COMMANDLINE_LENGTH - strlen(commandline));
	}

#ifdef CONFIG_APPEND_BOOTARGS
	/* Add 'append_bootargs' to hold some paramemters which need to be appended
	 * to bootargs */
	char *append_bootargs = env_get("append_bootargs");
	if (append_bootargs) {
		if (strlen(append_bootargs) + 2 >
				(COMMANDLINE_LENGTH - strlen(commandline))) {
			printf("The 'append_bootargs' is too long to be appended to bootargs\n");
		} else {
			strncat(commandline, " ", COMMANDLINE_LENGTH - strlen(commandline));
			strncat(commandline, append_bootargs, COMMANDLINE_LENGTH - strlen(commandline));
		}
	}
#endif
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
	u32 kernel_addr = android_image_get_kernel_addr(hdr);
	const struct image_header *ihdr = (const struct image_header *)
		((uintptr_t)hdr + hdr->page_size);

	/*
	 * Not all Android tools use the id field for signing the image with
	 * sha1 (or anything) so we don't check it. It is not obvious that the
	 * string is null terminated so we take care of this.
	 */
	strncpy(andr_tmp_str, hdr->name, ANDR_BOOT_NAME_SIZE);
	andr_tmp_str[ANDR_BOOT_NAME_SIZE] = '\0';
	if (strlen(andr_tmp_str))
		printf("Android's image name: %s\n", andr_tmp_str);

	debug("Kernel load addr 0x%08x size %u KiB\n",
	       kernel_addr, DIV_ROUND_UP(hdr->kernel_size, 1024));

	char commandline[COMMANDLINE_LENGTH] = {0};
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
				COMMANDLINE_LENGTH - strlen(commandline)) {
				printf("cmdline in bootimg is too long!\n");
				return -1;
			}
			else
				strncat(commandline, hdr->cmdline, COMMANDLINE_LENGTH - strlen(commandline));
		}
	}

	append_kernel_cmdline(commandline);

	debug("Kernel command line: %s\n", commandline);
	env_set("bootargs", commandline);

	if (os_data) {
		if (image_get_magic(ihdr) == IH_MAGIC) {
			*os_data = image_get_data(ihdr);
		} else {
			*os_data = (ulong)hdr;
			*os_data += hdr->page_size;
		}
	}
	if (os_len) {
		if (image_get_magic(ihdr) == IH_MAGIC)
			*os_len = image_get_data_size(ihdr);
		else
			*os_len = hdr->kernel_size;
	}

	return 0;
}

/**
 * android_image_get_kernel() - processes kernel part of Android boot images
 * @hdr:	Pointer to boot image header, which is at the start
 *			of the image.
 * @vendor_hdr:	Pointer to vendor_boot image header, which is at the start
 *			of the image.
 * This function appends the kernel command line to the bootargs env variable.
 *
 * Return: Zero on success, otherwise on failure.
 */
int android_image_get_kernel_v3(const struct boot_img_hdr_v3 *hdr,
				const struct vendor_boot_img_hdr_v3 *vendor_hdr)
{
	u32 kernel_addr = vendor_hdr->kernel_addr;

	/*
	 * Not all Android tools use the id field for signing the image with
	 * sha1 (or anything) so we don't check it. It is not obvious that the
	 * string is null terminated so we take care of this.
	 */
	strncpy(andr_tmp_str, (char *)(vendor_hdr->name), ANDR_VENDOR_BOOT_NAME_SIZE);
	andr_tmp_str[ANDR_VENDOR_BOOT_NAME_SIZE] = '\0';
	if (strlen(andr_tmp_str))
		printf("Android's image name: %s\n", andr_tmp_str);

	printf("Kernel load addr 0x%08x size %u KiB\n",
	       kernel_addr, DIV_ROUND_UP(hdr->kernel_size, 1024));

	char commandline[COMMANDLINE_LENGTH] = {0};
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

		if (*vendor_hdr->cmdline) {
			if (strlen((char *)vendor_hdr->cmdline) + 1 >
				COMMANDLINE_LENGTH - strlen(commandline)) {
				printf("cmdline in bootimg is too long!\n");
				return -1;
			}
			else
				strncat(commandline, (char *)(vendor_hdr->cmdline), COMMANDLINE_LENGTH - strlen(commandline));
		}

		if (*hdr->cmdline) {
			if (strlen((char *)hdr->cmdline) + 1 >
				COMMANDLINE_LENGTH - strlen(commandline)) {
				printf("cmdline in bootimg is too long!\n");
				return -1;
			}
			else
				strncat(commandline, (char *)hdr->cmdline, COMMANDLINE_LENGTH - strlen(commandline));
		}
	}

	append_kernel_cmdline(commandline);

	debug("Kernel command line: %s\n", commandline);
	env_set("bootargs", commandline);

	return 0;
}

int android_image_check_header(const struct andr_img_hdr *hdr)
{
	return memcmp(ANDR_BOOT_MAGIC, hdr->magic, ANDR_BOOT_MAGIC_SIZE);
}

int android_image_check_header_v3(const struct boot_img_hdr_v3 *hdr,
				  const struct vendor_boot_img_hdr_v3 *vendor_hdr)
{
	return memcmp(ANDR_BOOT_MAGIC, hdr->magic, ANDR_BOOT_MAGIC_SIZE) ||
		memcmp(ANDR_VENDOR_BOOT_MAGIC, vendor_hdr->magic, ANDR_VENDOR_BOOT_MAGIC_SIZE);
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

	if (hdr->header_version >= 1)
		end += ALIGN(hdr->recovery_dtbo_size, hdr->page_size);

	if (hdr->header_version >= 2)
		end += ALIGN(hdr->dtb_size, hdr->page_size);

	return end;
}

ulong android_image_get_kload(const struct andr_img_hdr *hdr)
{
	return android_image_get_kernel_addr(hdr);
}

ulong android_image_get_kcomp(const struct andr_img_hdr *hdr)
{
	const void *p = (void *)((uintptr_t)hdr + hdr->page_size);

	if (image_get_magic((image_header_t *)p) == IH_MAGIC)
		return image_get_comp((image_header_t *)p);
	else if (get_unaligned_le32(p) == LZ4F_MAGIC)
		return IH_COMP_LZ4;
	else
		return IH_COMP_NONE;
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

/**
 * android_image_get_dtbo() - Get address and size of recovery DTBO image.
 * @hdr_addr: Boot image header address
 * @addr: If not NULL, will contain address of recovery DTBO image
 * @size: If not NULL, will contain size of recovery DTBO image
 *
 * Get the address and size of DTBO image in "Recovery DTBO" area of Android
 * Boot Image in RAM. The format of this image is Android DTBO (see
 * corresponding "DTB/DTBO Partitions" AOSP documentation for details). Once
 * the address is obtained from this function, one can use 'adtimg' U-Boot
 * command or android_dt_*() functions to extract desired DTBO blob.
 *
 * This DTBO (included in boot image) is only needed for non-A/B devices, and it
 * only can be found in recovery image. On A/B devices we can always rely on
 * "dtbo" partition. See "Including DTBO in Recovery for Non-A/B Devices" in
 * AOSP documentation for details.
 *
 * Return: true on success or false on error.
 */
bool android_image_get_dtbo(ulong hdr_addr, ulong *addr, u32 *size)
{
	const struct andr_img_hdr *hdr;
	ulong dtbo_img_addr;
	bool ret = true;

	hdr = map_sysmem(hdr_addr, sizeof(*hdr));
	if (android_image_check_header(hdr)) {
		printf("Error: Boot Image header is incorrect\n");
		ret = false;
		goto exit;
	}

	if (hdr->header_version < 1) {
		printf("Error: header_version must be >= 1 to get dtbo\n");
		ret = false;
		goto exit;
	}

	if (hdr->recovery_dtbo_size == 0) {
		printf("Error: recovery_dtbo_size is 0\n");
		ret = false;
		goto exit;
	}

	/* Calculate the address of DTB area in boot image */
	dtbo_img_addr = hdr_addr;
	dtbo_img_addr += hdr->page_size;
	dtbo_img_addr += ALIGN(hdr->kernel_size, hdr->page_size);
	dtbo_img_addr += ALIGN(hdr->ramdisk_size, hdr->page_size);
	dtbo_img_addr += ALIGN(hdr->second_size, hdr->page_size);

	if (addr)
		*addr = dtbo_img_addr;
	if (size)
		*size = hdr->recovery_dtbo_size;

exit:
	unmap_sysmem(hdr);
	return ret;
}

/**
 * android_image_get_dtb_img_addr() - Get the address of DTB area in boot image.
 * @hdr_addr: Boot image header address
 * @addr: Will contain the address of DTB area in boot image
 *
 * Return: true on success or false on fail.
 */
static bool android_image_get_dtb_img_addr(ulong hdr_addr, ulong *addr)
{
	const struct andr_img_hdr *hdr;
	ulong dtb_img_addr;
	bool ret = true;

	hdr = map_sysmem(hdr_addr, sizeof(*hdr));
	if (android_image_check_header(hdr)) {
		printf("Error: Boot Image header is incorrect\n");
		ret = false;
		goto exit;
	}

	if (hdr->header_version < 2) {
		printf("Error: header_version must be >= 2 to get dtb\n");
		ret = false;
		goto exit;
	}

	if (hdr->dtb_size == 0) {
		printf("Error: dtb_size is 0\n");
		ret = false;
		goto exit;
	}

	/* Calculate the address of DTB area in boot image */
	dtb_img_addr = hdr_addr;
	dtb_img_addr += hdr->page_size;
	dtb_img_addr += ALIGN(hdr->kernel_size, hdr->page_size);
	dtb_img_addr += ALIGN(hdr->ramdisk_size, hdr->page_size);
	dtb_img_addr += ALIGN(hdr->second_size, hdr->page_size);
	dtb_img_addr += ALIGN(hdr->recovery_dtbo_size, hdr->page_size);

	*addr = dtb_img_addr;

exit:
	unmap_sysmem(hdr);
	return ret;
}

/**
 * android_image_get_dtb_by_index() - Get address and size of blob in DTB area.
 * @hdr_addr: Boot image header address
 * @index: Index of desired DTB in DTB area (starting from 0)
 * @addr: If not NULL, will contain address to specified DTB
 * @size: If not NULL, will contain size of specified DTB
 *
 * Get the address and size of DTB blob by its index in DTB area of Android
 * Boot Image in RAM.
 *
 * Return: true on success or false on error.
 */
bool android_image_get_dtb_by_index(ulong hdr_addr, u32 index, ulong *addr,
				    u32 *size)
{
	const struct andr_img_hdr *hdr;
	bool res;
	ulong dtb_img_addr;	/* address of DTB part in boot image */
	u32 dtb_img_size;	/* size of DTB payload in boot image */
	ulong dtb_addr;		/* address of DTB blob with specified index  */
	u32 i;			/* index iterator */

	res = android_image_get_dtb_img_addr(hdr_addr, &dtb_img_addr);
	if (!res)
		return false;

	/* Check if DTB area of boot image is in DTBO format */
	if (android_dt_check_header(dtb_img_addr)) {
		return android_dt_get_fdt_by_index(dtb_img_addr, index, addr,
						   size);
	}

	/* Find out the address of DTB with specified index in concat blobs */
	hdr = map_sysmem(hdr_addr, sizeof(*hdr));
	dtb_img_size = hdr->dtb_size;
	unmap_sysmem(hdr);
	i = 0;
	dtb_addr = dtb_img_addr;
	while (dtb_addr < dtb_img_addr + dtb_img_size) {
		const struct fdt_header *fdt;
		u32 dtb_size;

		fdt = map_sysmem(dtb_addr, sizeof(*fdt));
		if (fdt_check_header(fdt) != 0) {
			unmap_sysmem(fdt);
			printf("Error: Invalid FDT header for index %u\n", i);
			return false;
		}

		dtb_size = fdt_totalsize(fdt);
		unmap_sysmem(fdt);

		if (i == index) {
			if (size)
				*size = dtb_size;
			if (addr)
				*addr = dtb_addr;
			return true;
		}

		dtb_addr += dtb_size;
		++i;
	}

	printf("Error: Index is out of bounds (%u/%u)\n", index, i);
	return false;
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

	printf("%skernel size:          %x\n", p, hdr->kernel_size);
	printf("%skernel address:       %x\n", p, hdr->kernel_addr);
	printf("%sramdisk size:         %x\n", p, hdr->ramdisk_size);
	printf("%sramdisk address:      %x\n", p, hdr->ramdisk_addr);
	printf("%ssecond size:          %x\n", p, hdr->second_size);
	printf("%ssecond address:       %x\n", p, hdr->second_addr);
	printf("%stags address:         %x\n", p, hdr->tags_addr);
	printf("%spage size:            %x\n", p, hdr->page_size);
	/* ver = A << 14 | B << 7 | C         (7 bits for each of A, B, C)
	 * lvl = ((Y - 2000) & 127) << 4 | M  (7 bits for Y, 4 bits for M) */
	printf("%sos_version:           %x (ver: %u.%u.%u, level: %u.%u)\n",
	       p, hdr->os_version,
	       (os_ver >> 7) & 0x7F, (os_ver >> 14) & 0x7F, os_ver & 0x7F,
	       (os_lvl >> 4) + 2000, os_lvl & 0x0F);
	printf("%sname:                 %s\n", p, hdr->name);
	printf("%scmdline:              %s\n", p, hdr->cmdline);
	printf("%sheader_version:       %d\n", p, hdr->header_version);

	if (hdr->header_version >= 1) {
		printf("%srecovery dtbo size:   %x\n", p,
		       hdr->recovery_dtbo_size);
		printf("%srecovery dtbo offset: %llx\n", p,
		       hdr->recovery_dtbo_offset);
		printf("%sheader size:          %x\n", p,
		       hdr->header_size);
	}

	if (hdr->header_version >= 2) {
		printf("%sdtb size:             %x\n", p, hdr->dtb_size);
		printf("%sdtb addr:             %llx\n", p, hdr->dtb_addr);
	}
}

/**
 * android_image_print_dtb_info - Print info for one DTB blob in DTB area.
 * @fdt: DTB header
 * @index: Number of DTB blob in DTB area.
 *
 * Return: true on success or false on error.
 */
static bool android_image_print_dtb_info(const struct fdt_header *fdt,
					 u32 index)
{
	int root_node_off;
	u32 fdt_size;
	const char *model;
	const char *compatible;

	root_node_off = fdt_path_offset(fdt, "/");
	if (root_node_off < 0) {
		printf("Error: Root node not found\n");
		return false;
	}

	fdt_size = fdt_totalsize(fdt);
	compatible = fdt_getprop(fdt, root_node_off, "compatible",
				 NULL);
	model = fdt_getprop(fdt, root_node_off, "model", NULL);

	printf(" - DTB #%u:\n", index);
	printf("           (DTB)size = %d\n", fdt_size);
	printf("          (DTB)model = %s\n", model ? model : "(unknown)");
	printf("     (DTB)compatible = %s\n",
	       compatible ? compatible : "(unknown)");

	return true;
}

/**
 * android_image_print_dtb_contents() - Print info for DTB blobs in DTB area.
 * @hdr_addr: Boot image header address
 *
 * DTB payload in Android Boot Image v2+ can be in one of following formats:
 *   1. Concatenated DTB blobs
 *   2. Android DTBO format (see CONFIG_CMD_ADTIMG for details)
 *
 * This function does next:
 *   1. Prints out the format used in DTB area
 *   2. Iterates over all DTB blobs in DTB area and prints out the info for
 *      each blob.
 *
 * Return: true on success or false on error.
 */
bool android_image_print_dtb_contents(ulong hdr_addr)
{
	const struct andr_img_hdr *hdr;
	bool res;
	ulong dtb_img_addr;	/* address of DTB part in boot image */
	u32 dtb_img_size;	/* size of DTB payload in boot image */
	ulong dtb_addr;		/* address of DTB blob with specified index  */
	u32 i;			/* index iterator */

	res = android_image_get_dtb_img_addr(hdr_addr, &dtb_img_addr);
	if (!res)
		return false;

	/* Check if DTB area of boot image is in DTBO format */
	if (android_dt_check_header(dtb_img_addr)) {
		printf("## DTB area contents (DTBO format):\n");
		android_dt_print_contents(dtb_img_addr);
		return true;
	}

	printf("## DTB area contents (concat format):\n");

	/* Iterate over concatenated DTB blobs */
	hdr = map_sysmem(hdr_addr, sizeof(*hdr));
	dtb_img_size = hdr->dtb_size;
	unmap_sysmem(hdr);
	i = 0;
	dtb_addr = dtb_img_addr;
	while (dtb_addr < dtb_img_addr + dtb_img_size) {
		const struct fdt_header *fdt;
		u32 dtb_size;

		fdt = map_sysmem(dtb_addr, sizeof(*fdt));
		if (fdt_check_header(fdt) != 0) {
			unmap_sysmem(fdt);
			printf("Error: Invalid FDT header for index %u\n", i);
			return false;
		}

		res = android_image_print_dtb_info(fdt, i);
		if (!res) {
			unmap_sysmem(fdt);
			return false;
		}

		dtb_size = fdt_totalsize(fdt);
		unmap_sysmem(fdt);
		dtb_addr += dtb_size;
		++i;
	}

	return true;
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
