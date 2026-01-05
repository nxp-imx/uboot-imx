
// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright 2025 NXP
 */

#include <efi.h>
#include <efi_gbl_boot_memory_protocol.h>
#include <efi_loader.h>
#include <inttypes.h>
#include <linux/sizes.h>
#include <stdint.h>
#include <stdlib.h>
#include <android_image.h>
#include <fb_fsl.h>

#define KERNEL_ALIGNMENT (2 * 1024 * 1024)
#define RAMDISK_ALIGNMENT (4 * 1024)

//TODO check the size
// 64MB for kernel
#define KERNEL_SIZE (64 * 1024 * 1024)
// 32MB for ramdisk
#define RAMDISK_SIZE (32 * 1024 * 1024)
// 4MB for fdt
#define FDT_SIZE (4 * 1024 * 1024)
#define GENERAL_LOAD_SIZE (32 * 1024 * 1024)

static bool addr_fixed = false;
static int loaded_slot = 0;
static char slot_suffix[2] = {'a', 'b'};

const efi_guid_t efi_gbl_boot_memory_protocol_guid = EFI_GBL_BOOT_MEMORY_GUID;

typedef struct image_buffer {
	size_t buffer_size;
	uintptr_t buffer;
	GBL_EFI_BOOT_BUFFER_TYPE name;
} image_buffer;

static image_buffer image_buffers[] = {
	{
		.buffer_size = KERNEL_SIZE,
		.buffer = 0,
		.name = KERNEL,
	},
	{
		.buffer_size = FDT_SIZE,
		.buffer = 0,
		.name = FDT,
	},
	{
		.buffer_size = RAMDISK_SIZE,
		.buffer = 0,
		.name = RAMDISK,
	},
	{
		.buffer_size = CONFIG_FASTBOOT_BUF_SIZE,
		.buffer = CONFIG_FASTBOOT_BUF_ADDR,
		.name = FASTBOOT_DOWNLOAD,
	},
	{
		.buffer_size = GENERAL_LOAD_SIZE,
		.buffer = 0,
		.name = GENERAL_LOAD,
	},
};

extern int current_slot(void);
static int init_load_addr(void) {
	struct vendor_boot_img_hdr_v4 vendor_boot_hdr = {};
	char vendor_boot_name[16] = {0};
	ulong kernel_addr;
	ulong ramdisk_addr;
	size_t number_read = 0;
	int slot = 0;
	int ret = 0;

	slot = current_slot();
	if (ret == -1) {
		log_err("Failed to get current slot!\n");
		return ret;
	}

	if (slot == loaded_slot && addr_fixed) {
		/* Address has been figured out, no need update */
		return 0;
	}

	snprintf(vendor_boot_name, sizeof(vendor_boot_name),
		 "vendor_boot_%c", slot_suffix[slot]);
	ret = read_from_partition_multi(vendor_boot_name, 0, sizeof(vendor_boot_hdr),
					&vendor_boot_hdr, &number_read);
	if (ret != 0 || number_read != sizeof(vendor_boot_hdr)) {
		log_err("Failed to read vendor_boot header from %s!\n", vendor_boot_name);
	}

	kernel_addr = vendor_boot_hdr.kernel_addr;
	ramdisk_addr = vendor_boot_hdr.ramdisk_addr;

	if (kernel_addr % (KERNEL_ALIGNMENT)) {
		kernel_addr = (kernel_addr + KERNEL_ALIGNMENT - 1) & ~(KERNEL_ALIGNMENT - 1);
		log_err("Kernel address 0x%x is not aligned, moving to 0x%lx\n",
				vendor_boot_hdr.kernel_addr, kernel_addr);
	}

	image_buffers[0].buffer = kernel_addr;
	// Put the fdt at the ramdisk address
	image_buffers[1].buffer = ramdisk_addr;
	// Put ramdisk right after fdt
	image_buffers[2].buffer = ramdisk_addr + FDT_SIZE;

	addr_fixed = true;
	loaded_slot = slot;
	return 0;
}

static efi_status_t EFIAPI get_partition_buffer(struct efi_gbl_boot_memory_protocol *this,
						const char *base_name,
						size_t *size,
						void **addr,
						GBL_EFI_PARTITION_BUFFER_FLAG *flag) {
	EFI_ENTRY("%p %p %p %p %p", this, base_name, size, addr, flag);

	return EFI_EXIT(EFI_NOT_FOUND);
}

efi_status_t EFIAPI sync_partition_buffer(struct efi_gbl_boot_memory_protocol *this,
					  bool sync_preloaded) {
	EFI_ENTRY("%p %d", this, sync_preloaded);

	return EFI_EXIT(EFI_SUCCESS);
}

efi_status_t EFIAPI get_boot_buffer(struct efi_gbl_boot_memory_protocol *this,
				    GBL_EFI_BOOT_BUFFER_TYPE buf_type,
				    size_t* size,
				    void** addr) {
	EFI_ENTRY("%p %d %p %p", this, buf_type, size, addr);

	if (init_load_addr()) {
		return EFI_EXIT(EFI_OUT_OF_RESOURCES);
	}

	for (size_t i = 0; i < ARRAY_SIZE(image_buffers); i++) {
		image_buffer *pbuf = &image_buffers[i];

		if (buf_type == pbuf->name) {
			*addr = (void *)pbuf->buffer;
			*size = pbuf->buffer_size;
			return EFI_EXIT(EFI_SUCCESS);
		}
	}

	*addr = NULL;
	*size = 0;
	return EFI_EXIT(EFI_SUCCESS);
}

static efi_gbl_boot_memory_protocol efi_gbl_boot_memory_proto = {
	.revision = GBL_EFI_BOOT_MEMORY_PROTOCOL_REVISION,
	.get_partition_buffer = get_partition_buffer,
	.sync_partition_buffer = sync_partition_buffer,
	.get_boot_buffer = get_boot_buffer,
};

efi_status_t efi_gbl_boot_memory_register(void)
{
	efi_status_t ret =
		efi_add_protocol(efi_root, &efi_gbl_boot_memory_protocol_guid,
				 &efi_gbl_boot_memory_proto);
	if (ret != EFI_SUCCESS) {
		log_err("Failed to install EFI_GBL_BOOT_MEMORY_PROTOCOL: 0x%lx\n",
			ret);
	}

	return ret;
}
