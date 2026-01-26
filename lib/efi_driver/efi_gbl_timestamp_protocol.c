// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright 2025 NXP
 */

#include <efi.h>
#include <efi_loader.h>
#include <efi_gbl_timestamp_protocol.h>

const efi_guid_t efi_gbl_timestamp_guid = EFI_GBL_TIMESTAMP_GUID;

extern unsigned long timer_read_counter(void);
static uint64_t get_timestamp(void) {
	EFI_ENTRY();

	/* arch timer */
	return timer_read_counter();
}

extern unsigned long notrace get_tbclk(void);
static efi_status_t EFIAPI get_properties(efi_timestamp_properties_t *properties) {
	EFI_ENTRY("%p", properties);

	if (properties == NULL) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	properties->frequency = get_tbclk();
	properties->end_value = (0xFFFFFFFFFFFFFFFF);

	return EFI_EXIT(EFI_SUCCESS);
}

static struct efi_gbl_timestamp_protocol efi_gbl_timestamp = {
	.get_timestamp = get_timestamp,
	.get_properties = get_properties,
};

efi_status_t efi_gbl_timestamp_register(void)
{
	efi_status_t ret = efi_add_protocol(efi_root, &efi_gbl_timestamp_guid,
					    &efi_gbl_timestamp);
	if (ret != EFI_SUCCESS) {
		log_err("Failed to install EFI_GBL_TIMESTAMP_PROTOCOL: 0x%lx\n", ret);
	}

	return ret;
}
