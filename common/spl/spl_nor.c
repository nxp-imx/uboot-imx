/*
 * Copyright (C) 2012 Stefan Roese <sr@denx.de>
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <spl.h>

static ulong spl_nor_fit_read(struct spl_load_info *load, ulong sector,
			      ulong count, void *buf)
{
	memcpy(buf, (void *)sector, count);

	return count;
}

unsigned long  __weak spl_nor_get_uboot_base(void)
{
	return CONFIG_SYS_UBOOT_BASE;
}

#ifdef CONFIG_PARSE_CONTAINER
int __weak nor_load_image_parse_container(struct spl_image_info *spl_image,
					  unsigned long offset)
{
	return -EINVAL;
}
#else
static int nor_load_legacy(struct spl_image_info *spl_image)
{
	int ret;

	/*
	 * Load real U-Boot from its location in NOR flash to its
	 * defined location in SDRAM
	 */
	ret = spl_parse_image_header(spl_image,
			(const struct image_header *)spl_nor_get_uboot_base());
	if (ret)
		return ret;

	memcpy((void *)(unsigned long)spl_image->load_addr,
	       (void *)(spl_nor_get_uboot_base() + sizeof(struct image_header)),
	       spl_image->size);

	return 0;
}
#endif

static int spl_nor_load_image(struct spl_image_info *spl_image,
			      struct spl_boot_device *bootdev)
{
	const struct image_header *header;

	int ret;
	/*
	 * Loading of the payload to SDRAM is done with skipping of
	 * the mkimage header in this SPL NOR driver
	 */
	spl_image->flags |= SPL_COPY_PAYLOAD_ONLY;

#ifdef CONFIG_SPL_OS_BOOT
	if (!spl_start_uboot()) {

		/*
		 * Load Linux from its location in NOR flash to its defined
		 * location in SDRAM
		 */
		header = (const struct image_header *)CONFIG_SYS_OS_BASE;

		if (image_get_os(header) == IH_OS_LINUX) {
			/* happy - was a Linux */

			ret = spl_parse_image_header(spl_image, header);
			if (ret)
				return ret;

			memcpy((void *)spl_image->load_addr,
			       (void *)(CONFIG_SYS_OS_BASE +
					sizeof(struct image_header)),
			       spl_image->size);

			spl_image->arg = (void *)CONFIG_SYS_FDT_BASE;

			return 0;
		} else {
			puts("The Expected Linux image was not found.\n"
			     "Please check your NOR configuration.\n"
			     "Trying to start u-boot now...\n");
		}
	}
#endif

	header = (struct image_header *)(CONFIG_SYS_TEXT_BASE -
					 sizeof(struct image_header));

	memcpy((void *)header, (void *)spl_nor_get_uboot_base(), 0x40);

	if (IS_ENABLED(CONFIG_SPL_LOAD_FIT) &&
	    image_get_magic(header) == FDT_MAGIC) {
		struct spl_load_info load;

		debug("Found FIT\n");
		load.dev = NULL;
		load.priv = NULL;
		load.filename = NULL;
		load.bl_len = 1;
		load.read = spl_nor_fit_read;
		ret = spl_load_simple_fit(spl_image, &load,
					  spl_nor_get_uboot_base(),
					  (void *)header);
	} else {
#ifdef CONFIG_PARSE_CONTAINER
		ret = nor_load_image_parse_container(spl_image,
							     spl_nor_get_uboot_base());
#else
		ret = nor_load_legacy(spl_image);
#endif
	}

	return ret;
}
SPL_LOAD_IMAGE_METHOD("NOR", 0, BOOT_DEVICE_NOR, spl_nor_load_image);
