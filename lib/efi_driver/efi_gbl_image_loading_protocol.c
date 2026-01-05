// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (C) 2024 The Android Open Source Project
 * Copyright 2025 NXP
 */

#include <efi.h>
#include <efi_gbl_image_loading_protocol.h>
#include <efi_loader.h>
#include <inttypes.h>
#include <linux/sizes.h>
#include <stdint.h>
#include <stdlib.h>

const efi_guid_t efi_gbl_image_loading_protocol_guid = \
	EFI_GBL_IMAGE_LOADING_PROTOCOL_GUID;

static efi_status_t EFIAPI get_buffer(struct efi_image_loading_protocol *this,
				      const gbl_image_info *gbl_info,
				      gbl_image_buffer *buffer)
{
	EFI_ENTRY("%p %p %p", this, gbl_info, buffer);

	if (!this || !gbl_info || !buffer)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	log_info("Image loading buffer for %ls\n", gbl_info->image_type);

	/* Set fastboot buffer */
	if (!memcmp(gbl_info->image_type, GBL_IMAGE_TYPE_FASTBOOT,
			sizeof(GBL_IMAGE_TYPE_FASTBOOT))) {
		buffer->memory = (void *)CONFIG_FASTBOOT_BUF_ADDR;
		buffer->size_bytes = CONFIG_FASTBOOT_BUF_SIZE;
		return EFI_EXIT(EFI_SUCCESS);
	}

	buffer->memory = NULL;
	buffer->size_bytes = 0;
	return EFI_EXIT(EFI_SUCCESS);
}

static efi_image_loading_protocol efi_gbl_image_loading_proto = {
	.revision = EFI_GBL_IMAGE_LOADING_PROTOCOL_REVISION,
	.get_buffer = get_buffer,
};

efi_status_t efi_gbl_image_loading_register(void)
{
	efi_status_t ret =
		efi_add_protocol(efi_root, &efi_gbl_image_loading_protocol_guid,
				 &efi_gbl_image_loading_proto);
	if (ret != EFI_SUCCESS) {
		log_err("Failed to install EFI_GBL_IMAGE_LOADING_PROTOCOL: 0x%lx\n",
			ret);
	}

	return ret;
}
