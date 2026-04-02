//SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2025 NXP
 */

#include <efi.h>
#include <efi_loader.h>
#include <efi_gbl_avb_protocol.h>
#include <linux/delay.h>
#include <trusty/avb.h>
#include <u-boot/sha256.h>
#include <trusty/libtipc.h>
#include <trusty/hwcrypto.h>
#include <fsl_avb.h>
#include <fastboot.h>
#include "../lib/avb/fsl/fsl_avbkey.h"
#include "../../../drivers/fastboot/fb_fsl/fastboot_lock_unlock.h"

#ifndef CONFIG_LOAD_KEY_FROM_RPMB
#include "../lib/avb/fsl/fsl_public_key.h"
#endif

#define AVB_MAX_SLOT_NUMBER (32)

const efi_guid_t efi_gbl_avb_protocol_guid = EFI_GBL_AVB_PROTOCOL_UUID;
#ifdef CONFIG_IMX_TRUSTY_OS
const static char *boot_security_patch_string = "com.android.build.boot.security_patch";
#endif

static efi_status_t EFIAPI read_partitions_to_verify(struct efi_gbl_avb_protocol *this,
						     size_t *num_partitions,
						     efi_gbl_avb_partition *partitions) {
	EFI_ENTRY("%p %p %p", this, num_partitions, partitions);

	*num_partitions = 0;

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI read_device_status(struct efi_gbl_avb_protocol *this,
					      efi_gbl_avb_device_status *status_flags) {
	EFI_ENTRY("%p %p", this, status_flags);

	FbLockState status;

	/* Check parameters. */
	if (!status_flags) {
		log_err("Invalid avb device lock status parameter.\n");
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	status = fastboot_get_lock_stat();
	if (status == FASTBOOT_UNLOCK) {
		*status_flags = EFI_GBL_AVB_STATUS_UNLOCKED;
	} else {
		if (status == FASTBOOT_LOCK_ERROR) {
			log_err("Failed to get device lock status! Setting to locked.\n");
			fastboot_set_lock_stat(FASTBOOT_LOCK);
		}
		*status_flags = 0;
	}

	if (fastboot_lock_enable() == FASTBOOT_UL_ENABLE)
		*status_flags |= GBL_EFI_AVB_DEVICE_STATUS_UNLOCKABLE;

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI validate_vbmeta_public_key(struct efi_gbl_avb_protocol *this,
						      size_t public_key_length,
						      const uint8_t *public_key_data,
						      size_t public_key_metadata_length,
						      const uint8_t *public_key_metadata,
						      efi_gbl_avb_key_validation_status *validation_status) {
	EFI_ENTRY("%p %ld %p %ld %p %p", this,
			public_key_length,
			public_key_data,
			public_key_metadata_length,
			public_key_metadata,
			validation_status);

	/* Check the parameters */
	if (!validation_status || !public_key_data || !public_key_length) {
		log_err("Invalid avb public key parameters\n");
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	/* Set the initial value to INVALID */
	*validation_status = EFI_GBL_AVB_KEY_VALIDATION_STATUS_INVALID;

	/* Load and verify the public key */
#ifdef CONFIG_LOAD_KEY_FROM_RPMB
	uint8_t public_key_buf[AVB_MAX_BUFFER_LENGTH];
	uint32_t public_key_sz = sizeof(public_key_buf);
	if (trusty_read_vbmeta_public_key(public_key_buf,
						&public_key_sz) != 0) {
		log_err("Failed to load avb public key from secure storage.\n");

		if (!rpmbkey_is_set()) {
			/* Skip verification if RPMB key is not set. */
			return EFI_EXIT(EFI_SUCCESS);
		}
		return EFI_EXIT(EFI_DEVICE_ERROR);
	}

	if (public_key_sz != public_key_length || \
		memcmp(public_key_buf, public_key_data, public_key_length)) {
#else
	/* The public key was hard-coded in image */
	if (sizeof(fsl_public_key) != public_key_length || \
		memcmp(fsl_public_key, public_key_data, public_key_length)) {
#endif
		log_err("AVB public key validate failed!\n");
		return EFI_EXIT(EFI_SUCCESS);
	}

	log_info("AVB public key is valid.\n");
	*validation_status = EFI_GBL_AVB_KEY_VALIDATION_STATUS_VALID;
	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI read_rollback_index(struct efi_gbl_avb_protocol *this,
					       size_t index_location,
					       uint64_t *rollback_index) {
	EFI_ENTRY("%p %ld %p", this, index_location, rollback_index);

	/* Check parameters. */
	if (!rollback_index || index_location > AVB_MAX_SLOT_NUMBER) {
		log_err("Invalid avb rollback index parameters.\n");
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

#ifdef CONFIG_IMX_TRUSTY_OS
	if (trusty_read_rollback_index(index_location, rollback_index)) {
		log_err("Failed to read avb rollback index from location: %ld.\n", index_location);

		/* Return rollback_index as 0 if RPMB key is not programed. */
		if (!rpmbkey_is_set()) {
			*rollback_index = 0;
			return EFI_EXIT(EFI_SUCCESS);
		}

		return EFI_EXIT(EFI_DEVICE_ERROR);
	} else {
		return EFI_EXIT(EFI_SUCCESS);
	}
#else
	/* Return rollback index as 0 when Trusty OS is missing. */
	*rollback_index = 0;
	return EFI_EXIT(EFI_SUCCESS);
#endif
}

static efi_status_t EFIAPI write_rollback_index(struct efi_gbl_avb_protocol *this,
						size_t index_location,
						uint64_t rollback_index) {
	EFI_ENTRY("%p %ld %lld", this, index_location, rollback_index);

	/* Check parameters. */
	if (index_location > AVB_MAX_SLOT_NUMBER) {
		log_err("Invalid avb rollback index parameters.\n");
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

#ifdef CONFIG_IMX_TRUSTY_OS
	if (trusty_write_rollback_index(index_location, rollback_index)) {
		log_err("Failed to write avb rollback index to location: %ld.\n", index_location);

		/* Return success if RPMB key is not programed. */
		if (!rpmbkey_is_set()) {
			return EFI_EXIT(EFI_SUCCESS);
		}

		return EFI_EXIT(EFI_DEVICE_ERROR);
	} else {
		return EFI_EXIT(EFI_SUCCESS);
	}
#else
	/* Return success when Trusty OS is missing. */
	return EFI_EXIT(EFI_SUCCESS);
#endif
}

static efi_status_t EFIAPI read_persistent_value(struct efi_gbl_avb_protocol *this,
						 const uint8_t* name,
						 size_t* value_size,
						 uint8_t* value) {
	EFI_ENTRY("%p %p %p %p", this, name, value, value_size);

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI write_persistent_value(struct efi_gbl_avb_protocol *this,
						  const uint8_t* name,
						  size_t value_size,
						  const uint8_t* value) {
	EFI_ENTRY("%p %p %p %ld", this, name, value, value_size);

	return EFI_EXIT(EFI_SUCCESS);
}

#ifdef CONFIG_IMX_TRUSTY_OS
static int calculate_public_key_hash(uint8_t *public_key_hash, uint32_t len) {

	/* Only sha256 is supported */
	if (len != AVB_SHA256_DIGEST_SIZE) {
		log_err("Only sha256 digest is supported.\n");
		return -1;
	}

#ifdef CONFIG_LOAD_KEY_FROM_RPMB
	uint8_t public_key_buf[AVB_MAX_BUFFER_LENGTH];
	uint32_t public_key_sz = sizeof(public_key_buf);
	if (trusty_read_vbmeta_public_key(public_key_buf,
						&public_key_sz) != 0) {
		log_err("Failed to load avb public key from secure storage.\n");

		/* Return 0s if RPMB key is not set */
		if (!rpmbkey_is_set()) {
			memset(public_key_hash, 0, len);
			return 0;
		}
		return -1;
	}

	sha256_csum_wd(public_key_buf, public_key_sz,
			public_key_hash, CHUNKSZ_SHA256);
#else
	/* The public key was hard-coded in image */
	sha256_csum_wd(fsl_public_key, sizeof(fsl_public_key),
			public_key_hash, CHUNKSZ_SHA256);
#endif

	return 0;
}

/* Format the security patch level which is YYYY-MM-DD */
static int parse_boot_patch_level(uint8_t *boot_patch_level, uint32_t *trimmed_patch_level) {
	uint32_t year = 0, month = 0, day = 0;
	char *start, *end;
	char date_buf[10] = {0};

	/* Year */
	start = (char *)boot_patch_level;
	end = strchr(boot_patch_level, '-');
	if (!end) {
		log_err("Failed to parse boot security patch level!\n");
		return -1;
	}
	memcpy(date_buf, start, end - start);
	year = simple_strtoul(date_buf, NULL, 10);
	if (year < 1970) {
		log_err("Invalid year in security patch level! Year: %d\n", year);
		return -1;
	}

	/* Month */
	start = end + 1;
	end = strchr(start, '-');
	if (!end) {
		log_err("Failed to parse boot security patch level!\n");
		return -1;
	}
	memset(date_buf, 0, sizeof(date_buf));
	memcpy(date_buf, start, end - start);
	month = simple_strtoul(date_buf, NULL, 10);
	if ((month < 1) || (month > 12)) {
		log_err("Invalid month in security patch level! Month: %d\n", month);
		return -1;
	}

	/* Day */
	start = end + 1;
	memset(date_buf, 0, sizeof(date_buf));
	memcpy(date_buf, start, strlen(start));
	day = simple_strtoul(date_buf, NULL, 10);
	if ((day < 1) || (day > 31)) {
		log_err("Invalid day in security patch level! Day: %d\n", day);
		return -1;
	}

	*trimmed_patch_level = year * 10000 + month * 100 + day;

	return 0;
}
#endif

extern bool is_power_key_pressed(void);
static efi_status_t EFIAPI handle_verification_result(struct efi_gbl_avb_protocol *this,
						      const efi_gbl_avb_verification_result *result) {
	EFI_ENTRY("%p %p", this, result);

	/* Check parameters. */
	if (!result || result->digest == NULL || \
		!result->num_properties || result->properties == NULL) {
		log_err("Invalid avb verification result parameters.\n");
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

#ifdef CONFIG_IMX_TRUSTY_OS
	keymaster_verified_boot_t vbstatus;
	uint8_t public_key_hash[AVB_SHA256_DIGEST_SIZE];
	bool lock = false;
	int ret;

	/* Set KM boot parameters */
	lock = (fastboot_get_lock_stat() == FASTBOOT_UNLOCK)? false: true;
	if (result->color == EFI_GBL_AVB_BOOT_COLOR_GREEN && lock) {
		vbstatus = KM_VERIFIED_BOOT_VERIFIED;
	} else {
		vbstatus = KM_VERIFIED_BOOT_UNVERIFIED;
	}

	if (lock) {
		if (calculate_public_key_hash(public_key_hash,
						sizeof(public_key_hash))) {
			log_err("Failed to calculate public key hash!\n");
			return EFI_EXIT(EFI_DEVICE_ERROR);
		}
	} else {
		/* Pass 0s public key hash in unlocked state */
		memset(public_key_hash, 0, sizeof(public_key_hash));
	}

	ret = trusty_set_boot_params(0/* os_version, no need from bootloader */,
				     0 /* os_patchlevel, no need from bootloader */,
				     vbstatus,
				     lock,
				     public_key_hash,
				     sizeof(public_key_hash),
				     result->digest,
				     strlen(result->digest));
	if (ret != TRUSTY_ERR_NONE) {
		log_err("Failed to set KM boot parameters!\n");
		return EFI_EXIT(EFI_DEVICE_ERROR);
	}

	/* Get boot security patch level property */
	efi_gbl_avb_property *boot_security_patch = NULL;
	for (int i = 0; i < result->num_properties; i++) {
		efi_gbl_avb_property *property;
		property = (efi_gbl_avb_property *)&result->properties[i];
		if (!strncmp(property->base_partition_name, "boot", strlen("boot")) && \
		    !strncmp(property->key, boot_security_patch_string, strlen(boot_security_patch_string))) {
			boot_security_patch = property;
			break;
		}
	}
	if (boot_security_patch == NULL) {
		log_err("Failed to find property: %s!\n", boot_security_patch_string);
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	/* Set boot patch level */
	uint32_t boot_patch_level = 0;
	if (parse_boot_patch_level((uint8_t *)boot_security_patch->value,
					&boot_patch_level)) {
		log_err("Failed to parse boot security patch level!\n");
		return EFI_EXIT(EFI_DEVICE_ERROR);
	} else {
		/* Set the patch level to secure world */
		ret = trusty_set_boot_patch_level(boot_patch_level);
		if (ret != TRUSTY_ERR_NONE) {
			log_err("Failed to set boot patch level!\n");
			return EFI_EXIT(EFI_DEVICE_ERROR);
		}
		log_info("boot security patch level set to %d.\n", boot_patch_level);
	}

	/* lock the boot status and rollback_idx preventing Linux modify it */
	ret = trusty_lock_boot_state();
	if (ret != TRUSTY_ERR_NONE) {
		log_err("Failed to lock avb boot state!\n");
		return EFI_EXIT(EFI_DEVICE_ERROR);
	}

	/* set deprivilege state to stop NS access */
	ret = hwbcc_ns_deprivilege();
	if (ret != TRUSTY_ERR_NONE) {
		log_err("Failed to set hwbcc deprivilege!\n");
		return EFI_EXIT(EFI_DEVICE_ERROR);
	}

#ifdef CONFIG_IMX_SUPPORT_SRM
	char *keystore = env_get("keystore");
	if ((keystore != NULL) && (!strcmp(keystore, "trusty"))) {
		ret = hwcrypto_load_srm();
		if (ret != TRUSTY_ERR_NONE) {
			log_err("Failed to load SRM keys!\n");
			return EFI_EXIT(EFI_DEVICE_ERROR);
		}
	}
#endif
#endif

	/* Show orange warning for unlocked device, press power button to skip. */
#ifdef CONFIG_AVB_WARNING_LOGO
	if (fastboot_get_lock_stat() == FASTBOOT_UNLOCK) {
		int count = 0;

		printf("Device is unlocked, press power key to skip warning logo... \n");
		if (display_unlock_warning())
			printf("can't show unlock warning.\n");
		while ( (count < 10 * CONFIG_AVB_WARNING_TIME_LAST) && !is_power_key_pressed()) {
			mdelay(100);
			count++;
		}
	}
#endif

#if defined(CONFIG_IMX_HAB) && defined(CONFIG_CMD_PRIBLOB)
	/*
	* prevent the dek blob usable to decrypt an encrypted image after
	* encrypted boot stage has passed.
	*/
	if(run_command("set_priblob_bitfield", 0)){
		log_err("set priblob bitfield failed!\n");
	}
#endif

	return EFI_EXIT(EFI_SUCCESS);
}

extern void flashing(char *cmd, char *response);
static efi_status_t EFIAPI write_lock_state(struct efi_gbl_avb_protocol *this,
					    efi_gbl_avb_lock_type type,
					    efi_gbl_avb_lock_state state) {
	uint8_t response[FASTBOOT_RESPONSE_LEN] = {0};

	EFI_ENTRY("%p %d %d", this, type, state);

	if (type == EFI_GBL_AVB_LOCK_TYPE_CRITICAL) {
		log_err("Lock type (%d) is not supported.\n", type);
		return EFI_EXIT(EFI_UNSUPPORTED);
	} else if (type != EFI_GBL_AVB_LOCK_TYPE_DEVICE) {
		log_err("Invalid avb lock type :%d.\n", type);
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	if (state != EFI_GBL_AVB_LOCK_STATE_UNLOCKED &&
		state != EFI_GBL_AVB_LOCK_STATE_LOCKED) {
		log_err("Invalid avb lock state: %d.\n", state);
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	if (state == EFI_GBL_AVB_LOCK_STATE_LOCKED) {
		/* Lock the device */
		flashing("lock", response);
	} else {
		/* Unlock the device */
		flashing("unlock", response);
	}

	if (strncmp(response, "OKAY", 4) == 0) {
		/* Successfully locked/unlocked */
		return EFI_EXIT(EFI_SUCCESS);
	} else {
		/* Failed to lock/unlock */
		return EFI_EXIT(EFI_ACCESS_DENIED);
	}
}

static efi_gbl_avb_protocol efi_gbl_avb_proto = {
	.revision = EFI_GBL_AVB_PROTOCOL_REVISION,
	.read_partitions_to_verify = read_partitions_to_verify,
	.read_device_status = read_device_status,
	.validate_vbmeta_public_key = validate_vbmeta_public_key,
	.read_rollback_index = read_rollback_index,
	.write_rollback_index = write_rollback_index,
	.read_persistent_value = read_persistent_value,
	.write_persistent_value = write_persistent_value,
	.handle_verification_result = handle_verification_result,
	.write_lock_state = write_lock_state,
};

efi_status_t efi_gbl_avb_register(void) {
	efi_status_t ret = efi_add_protocol(efi_root,
						&efi_gbl_avb_protocol_guid,
						&efi_gbl_avb_proto);
	if (ret != EFI_SUCCESS) {
		log_err("Failed to install EFI_GBL_AVB_PROTOCOL: 0x%lx\n", ret);
	}

	return ret;
}
