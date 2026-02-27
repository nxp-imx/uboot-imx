// SPDX-License-Identifier: BSD-2-Clause
/* Copyright 2025 NXP
 *
 */

#include <stdlib.h>
#include <efi.h>
#include <efi_gbl_fastboot_protocol.h>
#include <efi_loader.h>
#include <fastboot.h>
#include <fb_fsl.h>
#include "../../drivers/fastboot/fb_fsl/fastboot_lock_unlock.h"

static const efi_guid_t efi_gbl_fastboot_protocol_guid = EFI_GBL_FASTBOOT_GUID;

int get_single_var(char *cmd, char *response);
/* common variables of fastboot getvar command */
extern char *fastboot_common_var[];
/**
 * fastboot_buf_addr - base address of the fastboot download buffer
 */
extern void *fastboot_buf_addr;
/**
 * fastboot_bytes_received - number of bytes received in the current download
 */
extern u32 fastboot_bytes_received;

static efi_status_t EFIAPI get_var(struct efi_gbl_fastboot_protocol* this,
				   const char* const* args, size_t num_args,
				   uint8_t* out, size_t* out_size) {
	int ret = 0;
	char buf[FASTBOOT_RESPONSE_LEN] = {0};
	char cmd[FASTBOOT_RESPONSE_LEN] = {0};

	EFI_ENTRY("%p %p %ld %p %p", this, args, num_args, out, out_size);

	if (!args || !num_args || !out || !out_size) {
		EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	/*
	 * Concatenates multiple command arguments into a single command string.
	 */
	for (int i = 0; i < num_args; i++) {
		if (i > 0) {
			strncat(cmd, ":", FASTBOOT_RESPONSE_LEN - strlen(cmd) - 1);
		}
		strncat(cmd, args[i], FASTBOOT_RESPONSE_LEN - strlen(cmd) - 1);
	}

	ret = get_single_var(cmd, buf);
	if (ret != 0) {
		log_err("get_var variable not found or get failed!\n");
		return EFI_EXIT(EFI_NOT_FOUND);
	} else {
		if (strlen(buf) > *out_size) {
			log_err("Buffer is too small!\n");
			return EFI_EXIT(EFI_BUFFER_TOO_SMALL);
		}

		memcpy(out, buf, strlen(buf));
		*out_size = strlen(buf);
	}

	return EFI_EXIT(EFI_SUCCESS);
}

static int check_and_send_single_var(char *var_name, char *buf, size_t buf_len,
				     void* ctx, get_var_all_callback cb) {
	int ret = 0;

	memset(buf, '\0', buf_len);
	ret = get_single_var(var_name, buf);
	if (ret) {
		log_err("Failed to get variable:%s\n", var_name);
		return -1;
	} else {
		/* We only have 1 argument */
		cb(ctx, (const char* const*)(&var_name), 1, buf);
	}

	return 0;
}

static efi_status_t EFIAPI get_var_all(struct efi_gbl_fastboot_protocol* this,
				       void* ctx, get_var_all_callback cb) {
	int n = 0;
	char buf[FASTBOOT_RESPONSE_LEN];
	char var_name[FASTBOOT_RESPONSE_LEN];

	EFI_ENTRY("%p %p %p", this, ctx, cb);

	if (!cb) {
		EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	/* Get common variables */
	for (n = 0; fastboot_common_var[n] != NULL; n++) {
		if (check_and_send_single_var(fastboot_common_var[n],
					      buf, sizeof(buf), ctx, cb))
			return EFI_EXIT(EFI_DEVICE_ERROR);
	}

#ifdef CONFIG_VIRTUAL_AB_SUPPORT
	strncpy(var_name, "snapshot-update-status:", FASTBOOT_RESPONSE_LEN);
	if (check_and_send_single_var(var_name, buf, sizeof(buf), ctx, cb))
		return EFI_EXIT(EFI_DEVICE_ERROR);
#endif

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI get_staged(struct efi_gbl_fastboot_protocol* this, uint8_t* out,
				      size_t* out_size, size_t* out_remain) {
	size_t len = 0;
	static size_t remaining = 0;

	EFI_ENTRY("%p %p %p %p", this, out, out_size, out_remain);

	if (!this || (*out_size != 0 && !out) || !out_size || !out_remain) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	/* Return fastboot_bytes_received to let the caller know the total size. */
	if (*out_size == 0) {
		*out_remain = fastboot_bytes_received;
		return EFI_EXIT(EFI_SUCCESS);
	}

	/* If remaining is 0, it means we are starting a new transfer */
	if (remaining == 0) {
		remaining = fastboot_bytes_received;
	}

	/* On each call, copy up to *out_size bytes from the download buffer,
	 * updating 'remaining' to track how much data is left to send.
	 */
	if (*out_size < remaining) {
		len = *out_size;
		*out_remain = remaining - len;
	} else {
		len = remaining;
		*out_remain = 0;
	}
	memcpy(out, fastboot_buf_addr + (fastboot_bytes_received - remaining), len);
	*out_size = len;
	remaining -= len;

	return EFI_EXIT(EFI_SUCCESS);
}

/* Defined in fb_fsl_command.c */
extern void erase(char *cmd, char *response);
static efi_status_t EFIAPI vendor_erase(struct efi_gbl_fastboot_protocol* this,
					const char* part_name, efi_gbl_fastboot_erase_action* action) {
	char response[FASTBOOT_RESPONSE_LEN] = {0};
	char cmd[FASTBOOT_RESPONSE_LEN] = {0};

	EFI_ENTRY("%p %p %p", this, part_name, action);

	if (!this || !part_name || !action) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	strncat(cmd, part_name, strlen(part_name));
	erase(cmd, response);

	if (strncmp(response, "OKAY", 4) == 0) {
		/* Successfully erased */
		*action = EFI_GBL_FASTBOOT_ERASE_ACTION_NOOP;
		return EFI_EXIT(EFI_SUCCESS);
	} else {
		/* Failed to erase */
		log_err("Failed to erase partition: %s\n", part_name);
		return EFI_EXIT(EFI_DEVICE_ERROR);
	}
}

static char *gbl_covered_oem_commands[] = {
	"gbl-set-default-block",
	"gbl-unset-default-block",
	NULL
};

/* Defined in fb_fsl_command.c */
extern void flashing(char *cmd, char *response);
static efi_status_t EFIAPI command_exec(struct efi_gbl_fastboot_protocol* this,
				        size_t num_args, const char* const* args,
				        size_t download_data_used_len,
				        uint8_t *download_data,
				        size_t download_data_full_size,
				        efi_gbl_fastboot_cmd_exec_result *implementation,
				        fastboot_message_sender sender, void *ctx) {
	efi_status_t status = EFI_SUCCESS;
	uint8_t response[FASTBOOT_RESPONSE_LEN] = {0};
	efi_gbl_fastboot_message_type msg_type = EFI_GBL_FASTBOOT_MESSAGE_TYPE_OKAY;

	EFI_ENTRY("%p %ld %p %ld %p %ld %p %p %p", this, num_args, args, download_data_used_len,
				download_data, download_data_full_size, implementation, sender, ctx);

	if (!this || !args || !implementation || !sender) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	/* Hijack the global fastboot_buf_addr and fastboot_bytes_received
	 * in case we need to handle downloaded fastboot data.
	 */
	fastboot_buf_addr = download_data;
	fastboot_bytes_received = download_data_used_len;

	/* Reject flash operations when the device is locked. */
	if (!strncmp(args[0], "flash", strlen(args[0]))) {
		FbLockState lock_state = fastboot_get_lock_stat();

		if (lock_state == FASTBOOT_LOCK) {
			log_err("Can't flash images when device is in locked state!\n");
			*implementation = EFI_GBL_FASTBOOT_COMMAND_EXEC_RESULT_PROHIBITED;
		} else if (lock_state == FASTBOOT_UNLOCK) {
			/* Use default "flash" implementation in GBL */
			*implementation = EFI_GBL_FASTBOOT_COMMAND_EXEC_RESULT_DEFAULT_IMPL;
		} else {
			log_err("Failed to get lock state!\n");
			status = EFI_DEVICE_ERROR;
		}

		goto exit;
	}

	/* Delegate "oem" command to firmware */
	if (!strncmp(args[0], "oem", strlen("oem"))) {
		/* Some oem commands should be executed by GBL, filter them out */
		for (int i = 0; gbl_covered_oem_commands[i] != NULL; i++) {
			if (strstr(args[0], gbl_covered_oem_commands[i])) {
				*implementation = EFI_GBL_FASTBOOT_COMMAND_EXEC_RESULT_DEFAULT_IMPL;
				goto exit;
			}
		}

		/* Execute oem command, the result would be in response.
		 * Need to take care of the response because it already
		 * includes the "OKAY" or "FAIL" prefix.
		 */
		flashing((char *)args[0], response);

		if (strncmp(response, "OKAY", 4) == 0) {
			msg_type = EFI_GBL_FASTBOOT_MESSAGE_TYPE_OKAY;
		} else if (strncmp(response, "FAIL", 4) == 0) {
			msg_type = EFI_GBL_FASTBOOT_MESSAGE_TYPE_FAIL;
		} else {
			log_err("Invalid response from oem command: %s\n", response);
			status = EFI_DEVICE_ERROR;
			goto exit;
		}

		/* Send the response back to the sender, need to skip
		 * the prefix at the beginning of the response.
		 */
		status = sender(ctx, msg_type, (const char*)(response + 4),
				strlen((const char*)response) - 4);

		*implementation = EFI_GBL_FASTBOOT_COMMAND_EXEC_RESULT_CUSTOM_IMPL;
	}

exit:
	return EFI_EXIT(status);
}

static efi_status_t EFIAPI get_partition_type(struct efi_gbl_fastboot_protocol* this,
					      const uint8_t* part_name, uint8_t* part_type,
					      size_t* part_type_len) {
	EFI_ENTRY("%p %p %p %p", this, part_name, part_type, part_type_len);

	if (!this || !part_name || !part_type || !part_type_len) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	if (!strcmp(part_name, FASTBOOT_PARTITION_DATA) ||
			!strcmp(part_name, FASTBOOT_PARTITION_METADATA)) {
		strcpy(part_type, "f2fs");
		*part_type_len = strlen("f2fs");

		return EFI_EXIT(EFI_SUCCESS);
	} else {
		return EFI_EXIT(EFI_UNSUPPORTED);
	}
}

static efi_gbl_fastboot_protocol efi_gbl_fastboot_proto = {
  .revision = EFI_GBL_FASTBOOT_PROTOCOL_REVISION,
  .get_var = get_var,
  .get_var_all = get_var_all,
  .get_staged = get_staged,
  .vendor_erase = vendor_erase,
  .command_exec = command_exec,
  .get_partition_type = get_partition_type,
};

efi_status_t efi_gbl_fastboot_register(void)
{
	const char *s;
	efi_status_t ret = EFI_SUCCESS;

	memset(efi_gbl_fastboot_proto.serial_number,
		'\0', sizeof(efi_gbl_fastboot_proto.serial_number));
	s = env_get("serial#");
	if (s) {
		strncpy((char *)efi_gbl_fastboot_proto.serial_number,
			s, sizeof(efi_gbl_fastboot_proto.serial_number));
	} else {
		log_err("Failed to get serial number!\n");
		return EFI_DEVICE_ERROR;
	}

	ret = efi_add_protocol(efi_root, &efi_gbl_fastboot_protocol_guid,
			       &efi_gbl_fastboot_proto);
	if (ret != EFI_SUCCESS) {
		log_err("Failed to install EFI_GBL_FASTBOOT_PROTOCOL: 0x%lx\n",
			ret);
	}

	return ret;
}
