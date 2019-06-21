/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <image.h>
#include <linux/libfdt.h>
#include <spl.h>

DECLARE_GLOBAL_DATA_PTR;

#define QUERY_ROM_VER		1
#define QUERY_BT_DEV		2
#define QUERY_PAGE_SZ		3
#define QUERY_IVT_OFF		4
#define QUERY_BT_STAGE		5
#define QUERY_IMG_OFF		6

#define BOOT_FROM_SD		1
#define BOOT_FROM_MMC		2
#define BOOT_FROM_NAND		3
#define BOOT_FROM_SPINOR	4
#define BOOT_FROM_USB		0xE

#define ROM_API_OKAY		0xF0

struct rom_api
{
	uint16_t ver;
	uint16_t tag;
	uint32_t reserved1;
	uint32_t (*download_image)(uint8_t * dest, uint32_t offset, uint32_t size,  uint32_t xor);
	uint32_t (*query_boot_infor)(uint32_t info_type, uint32_t *info, uint32_t xor);
} * g_rom_api = (struct rom_api*)0x980;



static int is_boot_from_stream_device(uint32_t boot)
{
	uint32_t interface;
	interface = boot >> 16;

	if(interface >= BOOT_FROM_USB)
		return 1;

	if(interface == BOOT_FROM_MMC && (boot&1))
		return 1;

	return 0;
}

static ulong spl_romapi_read_seekable(struct spl_load_info *load, ulong sector, ulong count, void *buf)
{
	uint32_t pagesize = *(uint32_t*) load->priv;
	volatile gd_t *pgd = gd;
	int ret;
	uint32_t offset;
	ulong byte = count * pagesize;

	offset = sector * pagesize;

	debug("ROM API load from 0x%x, size 0x%x\n", offset, (uint32_t)byte);

	ret = g_rom_api->download_image(buf, offset, byte,
			((uintptr_t) buf) ^ offset ^ byte);
	gd = pgd;

	if(ret == ROM_API_OKAY)
		return count;

	printf("ROM API Failure when load 0x%x\n", offset);
	return 0;
}

static int spl_romapi_load_image_seekable(struct spl_image_info *spl_image,
					  struct spl_boot_device *bootdev, u32 rom_bt_dev)
{
	int ret;
	volatile gd_t *pgd = gd;
	uint32_t offset;
	uint32_t pagesize, size;
	struct image_header *header;
	uint32_t image_offset;

	ret = g_rom_api->query_boot_infor(QUERY_IVT_OFF, &offset, ((uintptr_t) &offset)^ QUERY_IVT_OFF);
	ret |= g_rom_api->query_boot_infor(QUERY_PAGE_SZ, &pagesize, ((uintptr_t) &pagesize)^ QUERY_PAGE_SZ);
        ret |= g_rom_api->query_boot_infor(QUERY_IMG_OFF, &image_offset, ((uintptr_t) &image_offset)^ QUERY_IMG_OFF);

	gd = pgd;

	if (ret != ROM_API_OKAY) {
		puts("ROMAPI: Failure query boot infor pagesize/offset\n");
		return -1;
	}

	header = (struct image_header *)(CONFIG_SPL_IMX_ROMAPI_LOADADDR);

	printf("image offset 0x%x, pagesize 0x%x, ivt offset 0x%x\n", image_offset, pagesize, offset);

	if (((rom_bt_dev >> 16) & 0xff) ==  BOOT_FROM_SPINOR)
		offset = CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512;
	else
		offset = image_offset + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512 - 0x8000;

	size = ALIGN(sizeof(struct image_header), pagesize);
	ret = g_rom_api->download_image((uint8_t*)header, offset, size, ((uintptr_t) header) ^ offset ^ size);
	gd = pgd;

	if (ret != ROM_API_OKAY) {
		printf("ROMAPI: download failure offset 0x%x size 0x%x\n", offset, size);
		return -1;
	}

	if (IS_ENABLED(CONFIG_SPL_LOAD_FIT) &&
		image_get_magic(header) == FDT_MAGIC) {

		struct spl_load_info load;

		memset(&load, 0, sizeof(load));
		load.bl_len = pagesize;
		load.read = spl_romapi_read_seekable;
		load.priv = &pagesize;
		spl_load_simple_fit(spl_image, &load, offset / pagesize, header);

	} else {
		/* TODO */
		puts("Can't support legacy image\n");
		return -1;
	}

	return 0;
}

static ulong spl_ram_load_read(struct spl_load_info *load, ulong sector,
				ulong count, void *buf)
{
	memcpy(buf, (void *)(sector), count);

	if (load->priv) {
		ulong *p = (ulong*) load->priv;
		ulong total = sector + count;
		if( total > *p)
			*p = total;
	}
	return count;
}


static ulong get_fit_image_size(void *fit)
{
	struct spl_image_info spl_image;
        struct spl_load_info spl_load_info;

        ulong last = (ulong)fit;
        memset(&spl_load_info, 0, sizeof(spl_load_info));
        spl_load_info.bl_len = 1;
        spl_load_info.read = spl_ram_load_read;
        spl_load_info.priv = &last;

        /* We call load_simple_fit is just to get total size, the image is not downloaded,
         * so should bypass authentication
         */
        spl_image.flags = SPL_FIT_BYPASS_POST_LOAD;
        spl_load_simple_fit(&spl_image, &spl_load_info, (uintptr_t)fit, fit);
        return last - (ulong)fit;
}

uint8_t * search_fit_header(uint8_t *p, int size)
{
	int i = 0;
	for (i = 0; i < size; i += 4)
                if (genimg_get_format(p+i) == IMAGE_FORMAT_FIT)
                        return p + i;

        return NULL;
}

static int spl_romapi_load_image_stream(struct spl_image_info *spl_image,
					struct spl_boot_device *bootdev)
{
	volatile gd_t *pgd = gd;
	uint32_t pagesize, pg;
	int ret;
	int i = 0;
	uint8_t *p = (uint8_t *)CONFIG_SPL_IMX_ROMAPI_LOADADDR;
	uint8_t *pfit = NULL;
	int imagesize;
	int total;

	ret = g_rom_api->query_boot_infor(QUERY_PAGE_SZ, &pagesize,
				((uintptr_t) &pagesize)^ QUERY_PAGE_SZ);
	gd = pgd;

	if (ret != ROM_API_OKAY) {
		puts("failure at query_boot_info\n");
	}

	pg = pagesize;
	if(pg < 1024)
		pg = 1024;

	for (i = 0; i < 640; i++) {

		ret = g_rom_api->download_image(p, 0, pg,  ((uintptr_t)p)^pg);
		gd = pgd;

		if (ret != ROM_API_OKAY) {
			puts("Steam(USB) download failure\n");
			return -1;
		}

		pfit = search_fit_header(p, pg);
		p += pg;

		if (pfit)
			break;
	}

	if (pfit == NULL) {
		puts("Can't found uboot FIT image in 640K range \n");
		return -1;
	}

	if (p - pfit < sizeof(struct fdt_header)) {
		ret = g_rom_api->download_image(p, 0, pg,  ((uintptr_t)p)^pg);
		gd = pgd;

		if (ret != ROM_API_OKAY) {
			puts("Steam(USB) download failure\n");
			return -1;
		}

		p += pg;
	}

	imagesize = fit_get_size(pfit);
	printf("Find FIT header 0x&%p, size %d\n", pfit, imagesize);

	if (p - pfit < imagesize) {
		imagesize -= p - pfit;

		imagesize += pg -1; /*need pagesize hear after ROM fix USB problme*/
		imagesize /= pg;
		imagesize *= pg;

		printf("Need continue download %d\n", imagesize);

		ret = g_rom_api->download_image(p, 0, imagesize, ((uintptr_t)p)^imagesize);
		gd = pgd;

		p += imagesize;

		if (ret != ROM_API_OKAY) {
			printf("Failure download %d\n", imagesize);
			return -1;
		}
	}

	total = get_fit_image_size(pfit);
	total += 3;
	total &= ~0x3;

	imagesize = total - (p - pfit);

	imagesize += pagesize -1;
	imagesize /= pagesize;
	imagesize *= pagesize;

	printf("Download %d, total fit %d\n", imagesize, total);

	ret = g_rom_api->download_image(p, 0, imagesize,  ((uintptr_t)p)^imagesize);
	if (ret != ROM_API_OKAY) {
		printf("ROM download failure %d\n", imagesize);
	}

	struct spl_load_info load;
	memset(&load, 0, sizeof(load));
	load.bl_len = 1;
	load.read = spl_ram_load_read;
	spl_load_simple_fit(spl_image, &load, (ulong)pfit, pfit);

	return 0;
}


static int spl_romapi_load_image(struct spl_image_info *spl_image,
				 struct spl_boot_device *bootdev)
{
	volatile gd_t *pgd = gd;
	int ret;
	uint32_t boot;

	ret = g_rom_api->query_boot_infor(QUERY_BT_DEV, &boot, ((uintptr_t) &boot)^ QUERY_BT_DEV);
        gd =  pgd;

	if (ret != ROM_API_OKAY) {
		puts("ROMAPI: failure at query_boot_info\n");
		return -1;
	}

	if (is_boot_from_stream_device(boot))
		return spl_romapi_load_image_stream(spl_image, bootdev);

	return spl_romapi_load_image_seekable(spl_image, bootdev, boot);
}

SPL_LOAD_IMAGE_METHOD("ROMAPI", 0, BOOT_DEVICE_IMX_ROMAPI, spl_romapi_load_image);
