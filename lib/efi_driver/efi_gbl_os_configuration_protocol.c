// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Copyright 2025 NXP
 */

#include <efi_api.h>
#include <efi_loader.h>
#include <efi.h>
#include <efi_gbl_os_configuration_protocol.h>
#include <init.h>
#include <asm/setup.h>
#include <asm/bootm.h>
#include <mmc.h>
#include "../../drivers/fastboot/fb_fsl/fastboot_lock_unlock.h"

const efi_guid_t efi_gbl_os_config_guid =
	EFI_GBL_OS_CONFIGURATION_PROTOCOL_GUID;

int get_runtime_bootconfig(char *bootconfig, int *len) {
	char args_buf[512] = {0};

	/* Generate runtime bootconfig */
#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	struct tag_serialnr serialnr;
	get_board_serial(&serialnr);

	sprintf(args_buf, "androidboot.serialno=%08x%08x", serialnr.high, serialnr.low);
	strncat(bootconfig, args_buf, *len - strlen(bootconfig));

	if (serialnr.high + serialnr.low != 0) {
		char bd_addr[16]={0};
		sprintf(bd_addr,
			"%08x%08x",
			serialnr.high,
			serialnr.low);
		sprintf(args_buf,
			" androidboot.btmacaddr=%c%c:%c%c:%c%c:%c%c:%c%c:%c%c",
			bd_addr[0],bd_addr[1],bd_addr[2],bd_addr[3],bd_addr[4],bd_addr[5],
			bd_addr[6],bd_addr[7],bd_addr[8],bd_addr[9],bd_addr[10],bd_addr[11]);
	} else {
		/* Some boards have serial number as all zeros (imx8mp),
		 * hard code the bt mac address for such case. */
		sprintf(args_buf, " androidboot.btmacaddr=22:22:67:C6:69:73");
	}

	strncat(bootconfig, args_buf, *len - strlen(bootconfig));
#endif

	/* check lock state */
	FbLockState lock_status = fastboot_get_lock_stat();
	if (lock_status == FASTBOOT_UNLOCK) {
		strncat(bootconfig, " androidboot.flash.locked=0", *len - strlen(bootconfig));
	} else {
		if (lock_status == FASTBOOT_LOCK_ERROR) {
			log_err("failed to get lock status! Setting to locked.\n");
			fastboot_set_lock_stat(FASTBOOT_LOCK);
		}

		strncat(bootconfig, " androidboot.flash.locked=1", *len - strlen(bootconfig));
	}

	/* append soc type into bootargs */
	char *soc_type = env_get("soc_type");
	if (soc_type) {
		sprintf(args_buf,
			" androidboot.soc_type=%s",
			soc_type);
		strncat(bootconfig, args_buf, *len - strlen(bootconfig));
	}
	/* append soc rev into bootargs */
	char *soc_rev = env_get("soc_rev");
	if (soc_rev) {
		sprintf(args_buf,
			" androidboot.soc_rev=%s",
			soc_rev);
		strncat(bootconfig, args_buf, *len - strlen(bootconfig));
	}

	/* boot_devices */
	char mmcblk[30];
	char *boot_device = NULL;

	sprintf(mmcblk, "boot_devices_mmcblk%d", mmc_map_to_kernel_blk(mmc_get_env_dev()));
	boot_device = env_get(mmcblk);
	if (!boot_device) {
		log_err("failed to get boot device from env!\n");
		return -1;
	} else {
		sprintf(args_buf, " androidboot.boot_devices=%s", boot_device);
		strncat(bootconfig, args_buf, *len - strlen(bootconfig));
	}

	/* boot metric variables, partitions are loaded by GBL so returns
	 * 0s here.
	 */
	sprintf(args_buf,
		" androidboot.boottime=1BLL:%d,1BLE:%d,KL:%d,KD:%d,AVB:%d,ODT:%d,SW:%d",
		0, 0, 0, 0, 0, 0, 0);
	strncat(bootconfig, args_buf, *len - strlen(bootconfig));

#if defined(CONFIG_ARCH_MX6) || defined(CONFIG_ARCH_MX7) || \
	defined(CONFIG_ARCH_MX7ULP) || defined(CONFIG_ARCH_IMX8M)
	char cause[18];

	memset(cause, '\0', sizeof(cause));
	get_reboot_reason(cause);
	if (strstr(cause, "POR"))
		sprintf(args_buf," androidboot.bootreason=cold,powerkey");
	else if (strstr(cause, "WDOG") || strstr(cause, "WDG"))
		sprintf(args_buf," androidboot.bootreason=watchdog");
	else
		sprintf(args_buf," androidboot.bootreason=reboot");
#else
	sprintf(args_buf, " androidboot.bootreason=reboot");
#endif
	strncat(bootconfig, args_buf, *len - strlen(bootconfig));

	/* keystore */
	char *keystore = env_get("keystore");
	char *bootargs_trusty;
	if ((keystore == NULL) || strncmp(keystore, "trusty", sizeof("trusty"))) {
		bootargs_trusty = " androidboot.keystore=software";
	} else {
		bootargs_trusty = " androidboot.keystore=trusty";
	}
	strncat(bootconfig, bootargs_trusty, *len - strlen(bootconfig));

#ifdef CONFIG_APPEND_BOOTARGS
	/* Add 'append_bootconfig' environment variable to hold some paramemters
	 * which need to be appended to bootconfig. Must use ":=" operator when
	 * doing variable override.
	 */
	char *append_bootconfig = env_get("append_bootconfig");
	if (append_bootconfig) {
		strncat(bootconfig, " ", *len - strlen(bootconfig));
		strncat(bootconfig, append_bootconfig, *len - strlen(bootconfig));
	}
#endif

	if (*len <= strlen(bootconfig)) {
		log_err("Bootconfig buffer overflow!\n");
		return -1;
	}

	*len = strlen(bootconfig) + 1;

	/*
	 * The parameters in bootconfig should be separated by
	 * newline escape sequence '\n' instead of space. Replace
	 * all space with "\n" here
	 */
	char *ptr = bootconfig;
	for (int i = 0; i < strlen(bootconfig); i++) {
		if (*ptr == ' ')
			*ptr = '\n';
		ptr++;
	}
	*ptr = '\n';

	return 0;
}

static efi_status_t EFIAPI fixup_bootconfig(
	struct efi_gbl_os_configuration_protocol *this, size_t size,
	const char *bootconfig, size_t *fixup_buffer_size, char *fixup)
{
	char bootconfig_buf[2048] = {0};
	uint32_t len = sizeof(bootconfig_buf);

	EFI_ENTRY("%p, %p, %zu, %p, %p", this, bootconfig, size, fixup,
		  fixup_buffer_size);

	if (!this || !bootconfig || !fixup || !fixup_buffer_size)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	if (get_runtime_bootconfig(bootconfig_buf, &len) != 0) {
		log_err("Failed to construct runtime bootconfig!\n");
		return EFI_EXIT(EFI_DEVICE_ERROR);
	} else if (len > *fixup_buffer_size) {
		log_err("Bootconfig fixup buffer size too small!\n");
		return EFI_EXIT(EFI_BUFFER_TOO_SMALL);
	} else {
		memcpy(fixup, bootconfig_buf, len);
		*fixup_buffer_size = len;
	}

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI
select_device_trees(struct efi_gbl_os_configuration_protocol *this,
		    size_t num_device_trees,
		    struct efi_gbl_verified_device_tree *device_trees)
{
	int fdt_id = 0;

	EFI_ENTRY("%p, %p, %zu", this, device_trees, num_device_trees);

	if (!this || !device_trees || !num_device_trees) {
		log_err("Invalid gbl os configuration device tree parameters!\n");
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	/* The first fdt from vendor_boot contains the Id<-->Name mapping, parse expected
	 * dt id from it.
	 */
	for (int i = 0; i < num_device_trees; i++) {
		if (device_trees[i].metadata.source == VENDOR_BOOT && \
			device_trees[i].metadata.id == 0) {

			fdt_id = get_imx_android_fdt_id((void *)device_trees[i].device_tree);
			if (fdt_id < 0) {
				log_err("Failed to select device tree!\n");
				return EFI_EXIT(EFI_INVALID_PARAMETER);
			}

			break;
		}
	}

	/* Check the selected id */
	if (fdt_id <= 0) {
		log_err("Failed to get device tree id!\n");
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	/*
	 * Select the device tree
	 */
	for (int i = 0; i < num_device_trees; i++) {
		/* Select the device tree from vendor_boot. */
		if (device_trees[i].metadata.source == VENDOR_BOOT && \
			device_trees[i].metadata.id == fdt_id) {
			log_info("Selected dts source: %d, type: %d, id: %d, dtb size:%d.\n",
					device_trees[i].metadata.source,
					device_trees[i].metadata.type,
					device_trees[i].metadata.id,
					fdt_totalsize(device_trees[i].device_tree));
			device_trees[i].selected = 1;

			/* Currently we only select one dt, so we break here */
			break;
		}
	}

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI
select_fit_configuration(struct efi_gbl_os_configuration_protocol *this,
			 size_t fit_size, const uint8_t* fit,
			 size_t metadata_size, const uint8_t* metadata,
			 size_t* selected_configuration_offset) {
	EFI_ENTRY("%p, %zu, %p, %zu, %p, %p", this, fit_size, fit,
			metadata_size, metadata, selected_configuration_offset);

	return EFI_EXIT(EFI_UNSUPPORTED);
}

static struct efi_gbl_os_configuration_protocol efi_gbl_os_config_proto = {
	.revision = EFI_GBL_OS_CONFIGURATION_PROTOCOL_REVISION,
	.fixup_bootconfig = fixup_bootconfig,
	.select_device_trees = select_device_trees,
	.select_fit_configuration = select_fit_configuration,
};

efi_status_t efi_gbl_os_config_register(void)
{
	efi_status_t ret = efi_add_protocol(efi_root, &efi_gbl_os_config_guid,
					    &efi_gbl_os_config_proto);
	if (ret != EFI_SUCCESS)
		log_err("Failed to install EFI_GBL_OS_CONFIGURATION_PROTOCOL: 0x%lx\n",
			ret);

	return ret;
}
