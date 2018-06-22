/*
 * Copyright (C) 2014 Gateworks Corporation
 * Copyright (C) 2011-2012 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 *
 * Author: Tim Harvey <tharvey@gateworks.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <asm/spl.h>
#include <spl.h>
#include <asm/imx-common/boot_mode.h>
#include <asm/imx-common/hab.h>
#include <g_dnl.h>

#if defined(CONFIG_MX6)
/* determine boot device from SRC_SBMR1 (BOOT_CFG[4:1]) or SRC_GPR9 register */
u32 spl_boot_device(void)
{
	struct src *psrc = (struct src *)SRC_BASE_ADDR;
	unsigned int gpr10_boot = readl(&psrc->gpr10) & (1 << 28);
	unsigned reg = gpr10_boot ? readl(&psrc->gpr9) : readl(&psrc->sbmr1);
	unsigned int bmode = readl(&psrc->sbmr2);

	/*
	 * Check for BMODE if serial downloader is enabled
	 * BOOT_MODE - see IMX6DQRM Table 8-1
	 */
	if (((bmode >> 24) & 0x03) == 0x01) /* Serial Downloader */
		return BOOT_DEVICE_UART;
	/* BOOT_CFG1[7:4] - see IMX6DQRM Table 8-8 */
	switch ((reg & 0x000000FF) >> 4) {
	 /* EIM: See 8.5.1, Table 8-9 */
	case 0x0:
		/* BOOT_CFG1[3]: NOR/OneNAND Selection */
		if ((reg & 0x00000008) >> 3)
			return BOOT_DEVICE_ONENAND;
		else
			return BOOT_DEVICE_NOR;
		break;
	/* Reserved: Used to force Serial Downloader */
	case 0x1:
		return BOOT_DEVICE_UART;
	/* SATA: See 8.5.4, Table 8-20 */
	case 0x2:
		return BOOT_DEVICE_SATA;
	/* Serial ROM: See 8.5.5.1, Table 8-22 */
	case 0x3:
		/* BOOT_CFG4[2:0] */
		switch ((reg & 0x07000000) >> 24) {
		case 0x0 ... 0x4:
			return BOOT_DEVICE_SPI;
		case 0x5 ... 0x7:
			return BOOT_DEVICE_I2C;
		}
		break;
	/* SD/eSD: 8.5.3, Table 8-15  */
	case 0x4:
	case 0x5:
		return BOOT_DEVICE_MMC1;
	/* MMC/eMMC: 8.5.3 */
	case 0x6:
	case 0x7:
		return BOOT_DEVICE_MMC1;
	/* NAND Flash: 8.5.2 */
	case 0x8 ... 0xf:
		return BOOT_DEVICE_NAND;
	}
	return BOOT_DEVICE_NONE;
}

#if defined(CONFIG_SPL_MMC_SUPPORT)
/* called from spl_mmc to see type of boot mode for storage (RAW or FAT) */
u32 spl_boot_mode(const u32 boot_device)
{
	switch (spl_boot_device()) {
	/* for MMC return either RAW or FAT mode */
	case BOOT_DEVICE_MMC1:
	case BOOT_DEVICE_MMC2:
#if defined(CONFIG_SPL_FAT_SUPPORT)
		return MMCSD_MODE_FS;
#elif defined(CONFIG_SUPPORT_EMMC_BOOT)
		return MMCSD_MODE_EMMCBOOT;
#else
		return MMCSD_MODE_RAW;
#endif
		break;
	default:
		puts("spl: ERROR:  unsupported device\n");
		hang();
	}
}
#endif

#elif defined(CONFIG_IMX8M)
u32 spl_boot_device(void)
{
	switch (get_boot_device()) {
	case SD1_BOOT:
	case MMC1_BOOT:
		return BOOT_DEVICE_MMC1;
#if defined(CONFIG_IMX8MM)
	case SD2_BOOT:
	case MMC2_BOOT:
		return BOOT_DEVICE_MMC1;
	case SD3_BOOT:
	case MMC3_BOOT:
		return BOOT_DEVICE_MMC2;
#else
	case SD2_BOOT:
	case MMC2_BOOT:
		return BOOT_DEVICE_MMC2;
#endif
	case NAND_BOOT:
		return BOOT_DEVICE_NAND;
	case USB_BOOT:
		return BOOT_DEVICE_BOARD;
	case SPI_NOR_BOOT:
		return BOOT_DEVICE_SPI;
	case QSPI_BOOT:
		return BOOT_DEVICE_NOR;
	default:
		return BOOT_DEVICE_NONE;
	}
}

#if defined(CONFIG_SPL_MMC_SUPPORT)
/* called from spl_mmc to see type of boot mode for storage (RAW or FAT) */
u32 spl_boot_mode(const u32 boot_device)
{
	switch (get_boot_device()) {
	/* for MMC return either RAW or FAT mode */
	case SD1_BOOT:
	case SD2_BOOT:
	case SD3_BOOT:
#if defined(CONFIG_SPL_FAT_SUPPORT)
		return MMCSD_MODE_FS;
#else
		return MMCSD_MODE_RAW;
#endif
		break;

	case MMC1_BOOT:
	case MMC2_BOOT:
	case MMC3_BOOT:
#if defined(CONFIG_SPL_FAT_SUPPORT)
		return MMCSD_MODE_FS;
#elif defined(CONFIG_SUPPORT_EMMC_BOOT)
		return MMCSD_MODE_EMMCBOOT;
#else
		return MMCSD_MODE_RAW;
#endif
		break;
	default:
		puts("spl: ERROR:  unsupported device\n");
		hang();
	}
}
#endif

#endif

#ifdef CONFIG_SPL_USB_GADGET_SUPPORT
int g_dnl_bind_fixup(struct usb_device_descriptor *dev, const char *name)
{
	put_unaligned(CONFIG_G_DNL_PRODUCT_NUM + 0xfff, &dev->idProduct);

	return 0;
}
#endif

#if defined(CONFIG_SECURE_BOOT)

__weak void __noreturn jump_to_image_no_args(struct spl_image_info *spl_image)
{
	typedef void __noreturn (*image_entry_noargs_t)(void);

	image_entry_noargs_t image_entry =
		(image_entry_noargs_t)(unsigned long)spl_image->entry_point;

	debug("image entry point: 0x%lX\n", spl_image->entry_point);

	if (spl_image->flags & SPL_FIT_FOUND) {
		image_entry();
	} else {
		/* HAB looks for the CSF at the end of the authenticated data therefore,
		 * we need to subtract the size of the CSF from the actual filesize */
		if (authenticate_image(spl_image->load_addr,
				       spl_image->size - CONFIG_CSF_SIZE)) {
			image_entry();
		} else {
			puts("spl: ERROR:  image authentication unsuccessful\n");
			hang();
		}
	}
}

ulong board_spl_fit_size_align(ulong size)
{
	/* HAB authenticate_image requests the IVT offset is aligned to 0x1000 */
#define ALIGN_SIZE		0x1000

	size = ALIGN(size, ALIGN_SIZE);
	size += CONFIG_CSF_SIZE;

	return size;
}

void board_spl_fit_post_load(ulong load_addr, size_t length)
{
	if (!authenticate_image(load_addr, length - CONFIG_CSF_SIZE)) {
		puts("spl: ERROR:  image authentication unsuccessful\n");
		hang();
	}
}
#endif
