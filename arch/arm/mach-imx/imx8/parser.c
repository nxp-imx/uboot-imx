// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2018 NXP
 */
#include <common.h>
#include <spl.h>
#include <errno.h>
#include <asm/io.h>
#include <dm.h>
#include <mmc.h>
#include <spi_flash.h>
#include <nand.h>
#include <asm/arch/image.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/sci/sci.h>
#include <asm/mach-imx/boot_mode.h>

#define MMC_DEV		0
#define QSPI_DEV	1
#define NAND_DEV	2
#define RAM_DEV		3

#define SEC_SECURE_RAM_BASE			(0x31800000UL)
#define SEC_SECURE_RAM_END_BASE			(SEC_SECURE_RAM_BASE + 0xFFFFUL)
#define SECO_LOCAL_SEC_SEC_SECURE_RAM_BASE	(0x60000000UL)

#define SECO_PT         2U

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_IMX_TRUSTY_OS)
/* Pre-declaration of check_rpmb_blob. */
int check_rpmb_blob(struct mmc *mmc);
#endif

static int current_dev_type = MMC_DEV;
static int start_offset;
static void *device;

static int read(u32 start, u32 len, void *load_addr)
{
	int ret = -ENODEV;

	if (current_dev_type != NAND_DEV && current_dev_type != RAM_DEV
		&& !device) {
		debug("No device selected\n");
		return ret;
	}

#ifdef CONFIG_SPL_MMC_SUPPORT
	if (current_dev_type == MMC_DEV) {
		struct mmc *mmc = (struct mmc *)device;
		unsigned long count;

		ret = 0;

		count = blk_dread(mmc_get_blk_desc(mmc),
				  start / mmc->read_bl_len,
				  len / mmc->read_bl_len,
				  load_addr);
		if (count == 0) {
			debug("Read container image failed\n");
			return -EIO;
		}
	}
#endif
#ifdef CONFIG_SPL_SPI_LOAD
	if (current_dev_type == QSPI_DEV) {
		struct spi_flash *flash = (struct spi_flash *)device;

		ret = spi_flash_read(flash, start,
				     len, load_addr);
		if (ret != 0) {
			debug("Read container image from QSPI failed\n");
			return -EIO;
		}
	}
#endif
#ifdef CONFIG_SPL_NAND_SUPPORT
	if (current_dev_type == NAND_DEV) {
		ret = nand_spl_load_image(start, len, load_addr);
		if (ret != 0) {
			debug("Read container image from NAND failed\n");
			return -EIO;
		}
	}
#endif

	if (current_dev_type == RAM_DEV) {
		memcpy(load_addr, (const void *)(ulong)start, len);
		ret = 0;
	}

	return ret;
}

#ifdef CONFIG_AHAB_BOOT
static int authenticate_image(struct boot_img_t *img, int image_index)
{
	sc_faddr_t start, end;
	sc_rm_mr_t mr;
	int err;
	int ret = 0;

	debug("img %d, dst 0x%llx, src 0x%x, size 0x%x\n",
	      image_index, img->dst, img->offset, img->size);

	/* Find the memreg and set permission for seco pt */
	err = sc_rm_find_memreg(-1, &mr,
				img->dst & ~(CONFIG_SYS_CACHELINE_SIZE - 1),
				ALIGN(img->dst + img->size, CONFIG_SYS_CACHELINE_SIZE));

	if (err) {
		printf("can't find memreg for image load address %d, error %d\n",
		       image_index, err);
		return -ENOMEM;
	}

	err = sc_rm_get_memreg_info(-1, mr, &start, &end);
	if (!err)
		debug("memreg %u 0x%llx -- 0x%llx\n", mr, start, end);

	err = sc_rm_set_memreg_permissions(-1, mr,
					   SECO_PT, SC_RM_PERM_FULL);
	if (err) {
		printf("set permission failed for img %d, error %d\n",
		       image_index, err);
		return -EPERM;
	}

	err = sc_seco_authenticate(-1, SC_SECO_VERIFY_IMAGE,
					1 << image_index);
	if (err) {
		printf("authenticate img %d failed, return %d\n",
		       image_index, err);
		ret = -EIO;
	}

	err = sc_rm_set_memreg_permissions(-1, mr,
					   SECO_PT, SC_RM_PERM_NONE);
	if (err) {
		printf("remove permission failed for img %d, error %d\n",
		       image_index, err);
		ret = -EPERM;
	}

	return ret;
}
#endif

static struct boot_img_t *read_auth_image(struct container_hdr *container,
					  int image_index)
{
	struct boot_img_t *images;

	if (image_index > container->num_images) {
		debug("Invalid image number\n");
		return NULL;
	}

	images = (struct boot_img_t *)
			((uint8_t *)container + sizeof(struct container_hdr));

	if (read(images[image_index].offset + start_offset,
			images[image_index].size,
			(void *)images[image_index].entry) < 0) {
		return NULL;
	}

#ifdef CONFIG_AHAB_BOOT
	if (authenticate_image(&images[image_index], image_index)) {
		printf("Failed to authenticate image %d\n", image_index);
		return NULL;
	}
#endif

	return &images[image_index];
}

static int read_auth_container(struct spl_image_info *spl_image)
{
	struct container_hdr *container = NULL;
	uint16_t length;
	int ret;
	int i;

	container = malloc(CONTAINER_HDR_ALIGNMENT);
	if (!container)
		return -ENOMEM;

	ret = read(start_offset, CONTAINER_HDR_ALIGNMENT, (void *)container);
	if (ret) {
		printf("Error in read container %d\n", ret);
		goto out;
	}

	if (container->tag != 0x87 && container->version != 0x0) {
		printf("Wrong container header\n");
		ret = -EFAULT;
		goto out;
	}

	if (!container->num_images) {
		printf("Wrong container, no image found\n");
		ret = -EFAULT;
		goto out;
	}

	length = container->length_lsb + (container->length_msb << 8);

	debug("container length %u\n", length);

	if (length > CONTAINER_HDR_ALIGNMENT) {
		length =  ALIGN(length, CONTAINER_HDR_ALIGNMENT);

		free(container);
		container = malloc(length);
		if (!container)
			return -ENOMEM;

		ret = read(start_offset, length, (void *)container);
		if (ret) {
			printf("Error in read full container %d\n", ret);
			goto out;
		}
	}

#ifdef CONFIG_AHAB_BOOT
	memcpy((void *)SEC_SECURE_RAM_BASE, (const void *)container,
	       ALIGN(length, CONFIG_SYS_CACHELINE_SIZE));

	ret = sc_seco_authenticate(-1, SC_SECO_AUTH_CONTAINER,
					SECO_LOCAL_SEC_SEC_SECURE_RAM_BASE);
	if (ret) {
		printf("authenticate container hdr failed, return %d\n", ret);
		ret = -EFAULT;
		goto out;
	}
#endif

	for (i = 0; i < container->num_images; i++) {
		struct boot_img_t *image = read_auth_image(container, i);

		if (!image) {
			ret = -EINVAL;
			goto end_auth;
		}

		if (i == 0) {
			spl_image->load_addr = image->dst;
			spl_image->entry_point = image->entry;
		}
	}

#if defined(CONFIG_SPL_BUILD) && \
	defined(CONFIG_DUAL_BOOTLOADER) && defined(CONFIG_IMX_TRUSTY_OS)
	/* Everything checks out, get the sw_version now. */
	spl_image->rbindex = (uint64_t)container->sw_version;
#endif

end_auth:
#ifdef CONFIG_AHAB_BOOT
	if (sc_seco_authenticate(-1, SC_SECO_REL_CONTAINER, 0) != SC_ERR_NONE)
		printf("Error: release container failed!\n");
#endif
out:
	free(container);

	return ret;
}

int mmc_load_image_parse_container(struct spl_image_info *spl_image,
				     struct mmc *mmc, unsigned long sector)
{
	int ret = 0;

	current_dev_type = MMC_DEV;
	device = mmc;

	start_offset = sector * mmc->read_bl_len;

	ret = read_auth_container(spl_image);

	if (!ret)
	{
		/* Images loaded, now check the rpmb keyblob for Trusty OS.
		 * Skip this step when the dual bootloader feature is enabled
		 * since the blob should be checked earlier.
		 */
#if defined(CONFIG_IMX_TRUSTY_OS) && !defined(CONFIG_DUAL_BOOTLOADER)
		ret = check_rpmb_blob(mmc);
#endif
#if defined(CONFIG_IMX8_TRUSTY_XEN)
	struct mmc *rpmb_mmc;

	rpmb_mmc = find_mmc_device(0);
	if (ret = mmc_init(rpmb_mmc))
		printf("mmc init failed %s\n", __func__);
	else
	ret = check_rpmb_blob(rpmb_mmc);
#endif
	}
	return ret;
}

int spi_load_image_parse_container(struct spl_image_info *spl_image,
				   struct spi_flash *flash,
				   unsigned long offset)
{
	int ret = 0;

	current_dev_type = QSPI_DEV;
	device = flash;

	start_offset = offset;

	ret = read_auth_container(spl_image);

	return ret;
}

int nand_load_image_parse_container(struct spl_image_info *spl_image,
				   unsigned long offset)
{
	int ret = 0;

	current_dev_type = NAND_DEV;
	device = NULL;

	start_offset = offset;

	ret = read_auth_container(spl_image);

	return ret;
}

int sdp_load_image_parse_container(struct spl_image_info *spl_image,
				   unsigned long offset)
{
	int ret = 0;

	current_dev_type = RAM_DEV;
	device = NULL;

	start_offset = offset;

	ret = read_auth_container(spl_image);

	return ret;
}

int __weak nor_load_image_parse_container(struct spl_image_info *spl_image,
					  unsigned long offset)
{
	int ret = 0;

	current_dev_type = RAM_DEV;
	device = NULL;

	start_offset = offset;

	ret = read_auth_container(spl_image);

	return ret;
}
