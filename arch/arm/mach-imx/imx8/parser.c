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
#include <asm/arch/image.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/sci/sci.h>
#include <asm/mach-imx/boot_mode.h>

#define MMC_DEV		0
#define QSPI_DEV	1

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

static int read(int start, int len, void *load_addr)
{
	int ret = -ENODEV;

	if (!device) {
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

	return ret;
}

static int authenticate_image(struct boot_img_t *img, int image_index)
{
	sc_ipc_t ipcHndl = gd->arch.ipc_channel_handle;
	sc_faddr_t start, end;
	sc_rm_mr_t mr;
	sc_err_t err;
	int ret = 0;

	debug("img %d, dst 0x%llx, src 0x%x, size 0x%x\n",
	      image_index, img->dst, img->offset, img->size);

	/* Find the memreg and set permission for seco pt */
	err = sc_rm_find_memreg(ipcHndl, &mr,
				img->dst & ~(CONFIG_SYS_CACHELINE_SIZE - 1),
				ALIGN(img->dst + img->size, CONFIG_SYS_CACHELINE_SIZE));

	if (err) {
		printf("can't find memreg for image load address %d, error %d\n",
		       image_index, err);
		return -ENOMEM;
	}

	err = sc_rm_get_memreg_info(ipcHndl, mr, &start, &end);
	if (!err)
		debug("memreg %u 0x%llx -- 0x%llx\n", mr, start, end);

	err = sc_rm_set_memreg_permissions(ipcHndl, mr,
					   SECO_PT, SC_RM_PERM_FULL);
	if (err) {
		printf("set permission failed for img %d, error %d\n",
		       image_index, err);
		return -EPERM;
	}

	err = sc_misc_seco_authenticate(ipcHndl, SC_MISC_VERIFY_IMAGE,
					1 << image_index);
	if (err) {
		printf("authenticate img %d failed, return %d\n",
		       image_index, err);
		ret = -EIO;
	}

	err = sc_rm_set_memreg_permissions(ipcHndl, mr,
					   SECO_PT, SC_RM_PERM_NONE);
	if (err) {
		printf("remove permission failed for img %d, error %d\n",
		       image_index, err);
		ret = -EPERM;
	}

	return ret;
}

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

	if (authenticate_image(&images[image_index], image_index)) {
		printf("Failed to authenticate image %d\n", image_index);
		return NULL;
	}

	return &images[image_index];
}

static int read_auth_container(struct spl_image_info *spl_image)
{
	sc_ipc_t ipcHndl = gd->arch.ipc_channel_handle;
	struct container_hdr *container = NULL;
	uint16_t length;
	int ret;
	int i;

	container = malloc(sizeof(struct container_hdr));
	if (!container)
		return -ENOMEM;

	ret = read(start_offset, CONTAINER_HDR_ALIGNMENT, (void *)container);
	if (ret)
		return ret;

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
	memcpy((void *)SEC_SECURE_RAM_BASE, (const void *)container,
	       ALIGN(length, CONFIG_SYS_CACHELINE_SIZE));

	ret = sc_misc_seco_authenticate(ipcHndl, SC_MISC_AUTH_CONTAINER,
					SECO_LOCAL_SEC_SEC_SECURE_RAM_BASE);
	if (ret) {
		printf("authenticate container hdr failed, return %d\n", ret);
		ret = -EFAULT;
		goto out;
	}

	for (i = 0; i < container->num_images; i++) {
		struct boot_img_t *image = read_auth_image(container, i);

		if (!image) {
			ret = -EINVAL;
			goto out;
		}

		if (i == 0) {
			spl_image->load_addr = image->dst;
			spl_image->entry_point = image->entry;
		}
	}

	sc_misc_seco_authenticate(ipcHndl, SC_MISC_REL_CONTAINER, 0);

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
		/* Images loaded, now check the rpmb keyblob for Trusty OS. */
#if defined(CONFIG_IMX_TRUSTY_OS)
		ret = check_rpmb_blob(mmc);
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
