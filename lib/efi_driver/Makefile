# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2017 Heinrich Schuchardt

# This file only gets included with CONFIG_EFI_LOADER set, so all
# object inclusion implicitly depends on it

obj-y += efi_uclass.o
ifeq ($(CONFIG_PARTITIONS),y)
obj-y += efi_block_device.o
endif
obj-$(CONFIG_SYSRESET_SBI) += efi_reset_riscv.o

obj-$(CONFIG_EFI_GBL_AVB) += efi_gbl_avb_protocol.o
obj-$(CONFIG_EFI_GBL_OS_CONFIGURATION) += efi_gbl_os_configuration_protocol.o
obj-$(CONFIG_EFI_GBL_BOOT_CONTROL) += efi_gbl_boot_control_protocol.o
obj-$(CONFIG_EFI_GBL_BOOT_MEMORY) += efi_gbl_boot_memory_protocol.o
obj-$(CONFIG_EFI_GBL_FASTBOOT_TRANSPORT) += efi_gbl_fastboot_transport_protocol.o
obj-$(CONFIG_EFI_GBL_FASTBOOT) += efi_gbl_fastboot_protocol.o
obj-$(CONFIG_EFI_GBL_VENDOR_PARTITION) += efi_gbl_vendor_partition.o
obj-$(CONFIG_EFI_GBL_TIMESTAMP) += efi_gbl_timestamp_protocol.o
