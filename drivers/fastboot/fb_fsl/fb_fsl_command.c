// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 */

#include <asm/mach-imx/sys_proto.h>
#include <fb_fsl.h>
#include <fastboot.h>
#include <fastboot-internal.h>
#include <mmc.h>
#include <android_image.h>
#include <asm/bootm.h>
#include <nand.h>
#include <part.h>
#include <sparse_format.h>
#include <image-sparse.h>
#include <image.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/arch/sys_proto.h>
#include <asm/setup.h>
#include <env.h>
#ifdef CONFIG_ANDROID_RECOVERY
#include <recovery.h>
#endif

#ifdef CONFIG_BCB_SUPPORT
#include "bcb.h"
#endif

#ifdef CONFIG_AVB_SUPPORT
#include <dt_table.h>
#include <fsl_avb.h>
#endif

#ifdef CONFIG_ANDROID_THINGS_SUPPORT
#include <asm-generic/gpio.h>
#include <asm/mach-imx/gpio.h>
#include "../lib/avb/fsl/fsl_avbkey.h"
#include "../arch/arm/include/asm/mach-imx/hab.h"
#endif

#if defined(CONFIG_FASTBOOT_LOCK)
#include "fastboot_lock_unlock.h"
#endif

#ifdef CONFIG_IMX_TRUSTY_OS
#include "u-boot/sha256.h"
#include <trusty/libtipc.h>
#endif

#include "fb_fsl_common.h"
#include "fb_fsl_virtual_ab.h"

#define EP_BUFFER_SIZE			4096

/**
 * fastboot_bytes_received - number of bytes received in the current download
 */
static u32 fastboot_bytes_received;

/**
 * fastboot_bytes_expected - number of bytes expected in the current download
 */
static u32 fastboot_bytes_expected;

/* Write the bcb with fastboot bootloader commands */
static void enable_fastboot_command(void)
{
#ifdef CONFIG_BCB_SUPPORT
	char fastboot_command[32] = {0};
	strncpy(fastboot_command, FASTBOOT_BCB_CMD, 31);
	bcb_write_command(fastboot_command);
#endif
}

#ifdef CONFIG_ANDROID_RECOVERY
/* Write the recovery options with fastboot bootloader commands */
static void enable_recovery_fastboot(void)
{
#ifdef CONFIG_BCB_SUPPORT
	char msg[32] = {0};
	strncpy(msg, RECOVERY_BCB_CMD, 31);
	bcb_write_command(msg);
	strncpy(msg, RECOVERY_FASTBOOT_ARG, 31);
	bcb_write_recovery_opt(msg);
#endif
}
#endif

/* Get the Boot mode from BCB cmd or Key pressed */
static FbBootMode fastboot_get_bootmode(void)
{
	int boot_mode = BOOTMODE_NORMAL;
#ifdef CONFIG_ANDROID_RECOVERY
	if(is_recovery_key_pressing()) {
		boot_mode = BOOTMODE_RECOVERY_KEY_PRESSED;
		return boot_mode;
	}
#endif
#ifdef CONFIG_BCB_SUPPORT
	int ret = 0;
	char command[32];
	ret = bcb_read_command(command);
	if (ret < 0) {
		printf("read command failed\n");
		return boot_mode;
	}
	if (!strcmp(command, FASTBOOT_BCB_CMD)) {
		boot_mode = BOOTMODE_FASTBOOT_BCB_CMD;
	}
#ifdef CONFIG_ANDROID_RECOVERY
	else if (!strcmp(command, RECOVERY_BCB_CMD)) {
		boot_mode = BOOTMODE_RECOVERY_BCB_CMD;
	}
#endif

	/* Clean the mode once its read out,
	   no matter what in the mode string */
	memset(command, 0, 32);
	bcb_write_command(command);
#endif
	return boot_mode;
}

/* export to lib_arm/board.c */
void fastboot_run_bootmode(void)
{
	FbBootMode boot_mode = fastboot_get_bootmode();
	switch(boot_mode){
	case BOOTMODE_FASTBOOT_BCB_CMD:
		/* Make the boot into fastboot mode*/
		puts("Fastboot: Got bootloader commands!\n");
		run_command("fastboot 0", 0);
		break;
#ifdef CONFIG_ANDROID_RECOVERY
	case BOOTMODE_RECOVERY_BCB_CMD:
	case BOOTMODE_RECOVERY_KEY_PRESSED:
		/* Make the boot into recovery mode */
		puts("Fastboot: Got Recovery key pressing or recovery commands!\n");
		board_recovery_setup();
		break;
#endif
	default:
		/* skip special mode boot*/
		puts("Fastboot: Normal\n");
		break;
	}
}



/**
 * okay() - Send bare OKAY response
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 *
 * Send a bare OKAY fastboot response. This is used where the command is
 * valid, but all the work is done after the response has been sent (e.g.
 * boot, reboot etc.)
 */
static void okay(char *cmd_parameter, char *response)
{
	fastboot_okay(NULL, response);
}

/**
 * getvar() - Read a config/version variable
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 */
static void getvar(char *cmd_parameter, char *response)
{
	fastboot_getvar(cmd_parameter, response);
}

/**
 * reboot_bootloader() - Sets reboot bootloader flag.
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 */
static void reboot_bootloader(char *cmd_parameter, char *response)
{
	enable_fastboot_command();

	if (fastboot_set_reboot_flag())
		fastboot_fail("Cannot set reboot flag", response);
	else
		fastboot_okay(NULL, response);
}

#ifdef CONFIG_ANDROID_RECOVERY
/**
 * reboot_fastboot() - Sets reboot fastboot flag.
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 */
static void reboot_fastboot(char *cmd_parameter, char *response)
{
	enable_recovery_fastboot();

	if (fastboot_set_reboot_flag())
		fastboot_fail("Cannot set reboot flag", response);
	else
		fastboot_okay(NULL, response);
}
#endif

static void upload(char *cmd_parameter, char *response)
{
	if (!fastboot_bytes_received || fastboot_bytes_received > (EP_BUFFER_SIZE * 32)) {
		fastboot_fail("", response);
		return;
	}

	printf("Will upload %d bytes.\n", fastboot_bytes_received);
	snprintf(response, FASTBOOT_RESPONSE_LEN, "DATA%08x", fastboot_bytes_received);
	fastboot_tx_write_more(response);

	fastboot_tx_write((const char *)(fastboot_buf_addr), fastboot_bytes_received);

	snprintf(response,FASTBOOT_RESPONSE_LEN, "OKAY");
	fastboot_tx_write_more(response);

	fastboot_none_resp(response);
}

/**
 * fastboot_download() - Start a download transfer from the client
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 */
static void download(char *cmd_parameter, char *response)
{
	char *tmp;

	if (!cmd_parameter) {
		fastboot_fail("Expected command parameter", response);
		return;
	}
	fastboot_bytes_received = 0;
	fastboot_bytes_expected = simple_strtoul(cmd_parameter, &tmp, 16);
	if (fastboot_bytes_expected == 0) {
		fastboot_fail("Expected nonzero image size", response);
		return;
	}
	/*
	 * Nothing to download yet. Response is of the form:
	 * [DATA|FAIL]$cmd_parameter
	 *
	 * where cmd_parameter is an 8 digit hexadecimal number
	 */
	if (fastboot_bytes_expected > fastboot_buf_size) {
		fastboot_fail(cmd_parameter, response);
	} else {
		printf("Starting download of %d bytes\n",
		       fastboot_bytes_expected);
		fastboot_response("DATA", response, "%s", cmd_parameter);
	}
}

/**
 * fastboot_data_remaining() - return bytes remaining in current transfer
 *
 * Return: Number of bytes left in the current download
 */
u32 fastboot_data_remaining(void)
{
	if (fastboot_bytes_received >= fastboot_bytes_expected)
		return 0;

	return fastboot_bytes_expected - fastboot_bytes_received;
}

/**
 * fastboot_data_download() - Copy image data to fastboot_buf_addr.
 *
 * @fastboot_data: Pointer to received fastboot data
 * @fastboot_data_len: Length of received fastboot data
 * @response: Pointer to fastboot response buffer
 *
 * Copies image data from fastboot_data to fastboot_buf_addr. Writes to
 * response. fastboot_bytes_received is updated to indicate the number
 * of bytes that have been transferred.
 *
 * On completion sets image_size and ${filesize} to the total size of the
 * downloaded image.
 */
void fastboot_data_download(const void *fastboot_data,
			    unsigned int fastboot_data_len,
			    char *response)
{
#define BYTES_PER_DOT	0x20000
	u32 pre_dot_num, now_dot_num;

	if (fastboot_data_len == 0 ||
	    (fastboot_bytes_received + fastboot_data_len) >
	    fastboot_bytes_expected) {
		fastboot_fail("Received invalid data length",
			      response);
		return;
	}
	/* Download data to fastboot_buf_addr */
	memcpy(fastboot_buf_addr + fastboot_bytes_received,
	       fastboot_data, fastboot_data_len);

	pre_dot_num = fastboot_bytes_received / BYTES_PER_DOT;
	fastboot_bytes_received += fastboot_data_len;
	now_dot_num = fastboot_bytes_received / BYTES_PER_DOT;

	if (pre_dot_num != now_dot_num) {
		putc('.');
		if (!(now_dot_num % 74))
			putc('\n');
	}
	*response = '\0';
}

/**
 * fastboot_data_complete() - Mark current transfer complete
 *
 * @response: Pointer to fastboot response buffer
 *
 * Set image_size and ${filesize} to the total size of the downloaded image.
 */
void fastboot_data_complete(char *response)
{
	/* Download complete. Respond with "OKAY" */
	fastboot_okay(NULL, response);
	printf("\ndownloading of %d bytes finished\n", fastboot_bytes_received);
	env_set_hex("filesize", fastboot_bytes_received);
	env_set_hex("fastboot_bytes", fastboot_bytes_received);
	fastboot_bytes_expected = 0;
}

#if defined(CONFIG_FASTBOOT_LOCK)
static int partition_table_valid(void)
{
	int status, mmc_no;
	struct blk_desc *dev_desc;
#if defined(CONFIG_IMX_TRUSTY_OS) && !defined(CONFIG_ARM64)
	/* Prevent other partition accessing when no TOS flashed. */
	if (!tos_flashed)
		return 0;
#endif
	disk_partition_t info;
	mmc_no = fastboot_devinfo.dev_id;
	dev_desc = blk_get_dev("mmc", mmc_no);
	if (dev_desc)
		status = part_get_info(dev_desc, 1, &info);
	else
		status = -1;
	return (status == 0);
}

static void wipe_all_userdata(void)
{
	char response[FASTBOOT_RESPONSE_LEN];

	/* Erase all user data */
	printf("Start userdata wipe process....\n");
	/* Erase /data partition */
	fastboot_wipe_data_partition();

#if defined (CONFIG_ANDROID_SUPPORT) || defined (CONFIG_ANDROID_AUTO_SUPPORT)
	/* Erase the misc partition. */
	process_erase_mmc(FASTBOOT_PARTITION_MISC, response);
#endif

#ifndef CONFIG_ANDROID_AB_SUPPORT
	/* Erase the cache partition for legacy imx6/7 */
	process_erase_mmc(FASTBOOT_PARTITION_CACHE, response);
#endif

#if defined(AVB_RPMB) && !defined(CONFIG_IMX_TRUSTY_OS)
	printf("Start stored_rollback_index wipe process....\n");
	rbkidx_erase();
	printf("Wipe stored_rollback_index completed.\n");
#endif
	process_erase_mmc(FASTBOOT_PARTITION_METADATA, response);
	printf("Wipe userdata completed.\n");
}

static FbLockState do_fastboot_unlock(bool force)
{
	int status;

	if (fastboot_get_lock_stat() == FASTBOOT_UNLOCK) {
		printf("The device is already unlocked\n");
		return FASTBOOT_UNLOCK;
	}
	if ((fastboot_lock_enable() == FASTBOOT_UL_ENABLE) || force) {
		printf("It is able to unlock device. %d\n",fastboot_lock_enable());

#if defined(CONFIG_SECURE_UNLOCK) && defined(CONFIG_IMX_TRUSTY_OS)
		if ((fastboot_bytes_received == 0) || !hab_is_enabled()) {
			printf("No unlock credential found or hab is not closed!\n");
			return FASTBOOT_LOCK_ERROR;
		} else {
			char *serial = get_serial();
			status = trusty_verify_secure_unlock(fastboot_buf_addr,
								fastboot_bytes_received,
								(uint8_t *)serial, 16);
			if (status < 0) {
				printf("verify secure unlock credential fail due Trusty return %d\n", status);
				return FASTBOOT_LOCK_ERROR;
			}
		}
#endif

#ifdef CONFIG_VIRTUAL_AB_SUPPORT
		if (virtual_ab_update_is_merging() ||
			(virtual_ab_update_is_snapshoted() && !virtual_ab_slot_match())) {
			printf("Can not erase userdata while a snapshot update is in progress!\n");
			return FASTBOOT_LOCK_ERROR;
		}
#endif

		wipe_all_userdata();
		status = fastboot_set_lock_stat(FASTBOOT_UNLOCK);
		if (status < 0)
			return FASTBOOT_LOCK_ERROR;
	} else {
		printf("It is not able to unlock device.");
		return FASTBOOT_LOCK_ERROR;
	}

	return FASTBOOT_UNLOCK;
}

static FbLockState do_fastboot_lock(void)
{
	int status;

	if (fastboot_get_lock_stat() == FASTBOOT_LOCK) {
		printf("The device is already locked\n");
		return FASTBOOT_LOCK;
	}

#ifdef CONFIG_VIRTUAL_AB_SUPPORT
		if (virtual_ab_update_is_merging() ||
			(virtual_ab_update_is_snapshoted() && !virtual_ab_slot_match())) {
			printf("Can not erase userdata while a snapshot update is in progress!\n");
			return FASTBOOT_LOCK_ERROR;
		}
#endif

	wipe_all_userdata();
	status = fastboot_set_lock_stat(FASTBOOT_LOCK);
	if (status < 0)
		return FASTBOOT_LOCK_ERROR;

	return FASTBOOT_LOCK;
}

static bool endswith(char* s, char* subs) {
	if (!s || !subs)
		return false;
	uint32_t len = strlen(s);
	uint32_t sublen = strlen(subs);
	if (len < sublen) {
		return false;
	}
	if (strncmp(s + len - sublen, subs, sublen)) {
		return false;
	}
	return true;
}

static void flashing(char *cmd, char *response)
{
	FbLockState status;
	FbLockEnableResult result;
	if (endswith(cmd, "lock_critical")) {
		strcpy(response, "OKAY");
	}
#ifdef CONFIG_AVB_ATX
	else if (endswith(cmd, FASTBOOT_AVB_AT_PERM_ATTR)) {
		if (avb_atx_fuse_perm_attr(fastboot_buf_addr, fastboot_bytes_received))
			strcpy(response, "FAILInternal error!");
		else
			strcpy(response, "OKAY");
	} else if (endswith(cmd, FASTBOOT_AT_GET_UNLOCK_CHALLENGE)) {
		if (avb_atx_get_unlock_challenge(fsl_avb_ops.atx_ops,
						fastboot_buf_addr, &fastboot_bytes_received))
			strcpy(response, "FAILInternal error!");
		else
			strcpy(response, "OKAY");
	} else if (endswith(cmd, FASTBOOT_AT_UNLOCK_VBOOT)) {
		if (at_unlock_vboot_is_disabled()) {
			printf("unlock vboot already disabled, can't unlock the device!\n");
			strcpy(response, "FAILunlock vboot already disabled!.");
		} else {
#ifdef CONFIG_AT_AUTHENTICATE_UNLOCK
			if (avb_atx_verify_unlock_credential(fsl_avb_ops.atx_ops,
								fastboot_buf_addr))
				strcpy(response, "FAILIncorrect unlock credential!");
			else {
#endif
				status = do_fastboot_unlock(true);
				if (status != FASTBOOT_LOCK_ERROR)
					strcpy(response, "OKAY");
				else
					strcpy(response, "FAILunlock device failed.");
#ifdef CONFIG_AT_AUTHENTICATE_UNLOCK
			}
#endif
		}
	} else if (endswith(cmd, FASTBOOT_AT_LOCK_VBOOT)) {
		if (perm_attr_are_fused()) {
			status = do_fastboot_lock();
			if (status != FASTBOOT_LOCK_ERROR)
				strcpy(response, "OKAY");
			else
				strcpy(response, "FAILlock device failed.");
		} else
			strcpy(response, "FAILpermanent attributes not fused!");
	} else if (endswith(cmd, FASTBOOT_AT_DISABLE_UNLOCK_VBOOT)) {
		/* This command can only be called after 'oem at-lock-vboot' */
		status = fastboot_get_lock_stat();
		if (status == FASTBOOT_LOCK) {
			if (at_unlock_vboot_is_disabled()) {
				printf("unlock vboot already disabled!\n");
				strcpy(response, "OKAY");
			}
			else {
				if (!at_disable_vboot_unlock())
					strcpy(response, "OKAY");
				else
					strcpy(response, "FAILdisable unlock vboot fail!");
			}
		} else
			strcpy(response, "FAILplease lock the device first!");
	}
#endif /* CONFIG_AVB_ATX */
#ifdef CONFIG_ANDROID_THINGS_SUPPORT
	else if (endswith(cmd, FASTBOOT_BOOTLOADER_VBOOT_KEY)) {
		strcpy(response, "OKAY");
	}
#endif /* CONFIG_ANDROID_THINGS_SUPPORT */
#ifdef CONFIG_IMX_TRUSTY_OS
	else if (endswith(cmd, FASTBOOT_GET_CA_REQ)) {
		uint8_t *ca_output;
		uint32_t ca_length, cp_length;
		if (trusty_atap_get_ca_request(fastboot_buf_addr, fastboot_bytes_received,
						&(ca_output), &ca_length)) {
			printf("ERROR get_ca_request failed!\n");
			strcpy(response, "FAILInternal error!");
		} else {
			cp_length = min((uint32_t)CONFIG_FASTBOOT_BUF_SIZE, ca_length);
			memcpy(fastboot_buf_addr, ca_output, cp_length);
			fastboot_bytes_received = ca_length;
			strcpy(response, "OKAY");
		}

	} else if (endswith(cmd, FASTBOOT_SET_CA_RESP)) {
		if (trusty_atap_set_ca_response(fastboot_buf_addr, fastboot_bytes_received)) {
			printf("ERROR set_ca_response failed!\n");
			strcpy(response, "FAILInternal error!");
		} else
			strcpy(response, "OKAY");
	} else if (endswith(cmd, FASTBOOT_SET_RSA_ATTESTATION_KEY_ENC)) {
		if (trusty_set_attestation_key_enc(fastboot_buf_addr,
							fastboot_bytes_received,
							KM_ALGORITHM_RSA)) {
			printf("ERROR set rsa attestation key failed!\n");
			strcpy(response, "FAILInternal error!");
		} else {
			printf("Set rsa attestation key successfully!\n");
			strcpy(response, "OKAY");
		}
	} else if (endswith(cmd, FASTBOOT_SET_EC_ATTESTATION_KEY_ENC)) {
		if (trusty_set_attestation_key_enc(fastboot_buf_addr,
							fastboot_bytes_received,
							KM_ALGORITHM_EC)) {
			printf("ERROR set ec attestation key failed!\n");
			strcpy(response, "FAILInternal error!");
		} else {
			printf("Set ec attestation key successfully!\n");
			strcpy(response, "OKAY");
		}
	} else if (endswith(cmd, FASTBOOT_APPEND_RSA_ATTESTATION_CERT_ENC)) {
		if (trusty_append_attestation_cert_chain_enc(fastboot_buf_addr,
								fastboot_bytes_received,
								KM_ALGORITHM_RSA)) {
			printf("ERROR append rsa attestation cert chain failed!\n");
			strcpy(response, "FAILInternal error!");
		} else {
			printf("Append rsa attestation key successfully!\n");
			strcpy(response, "OKAY");
		}
	}  else if (endswith(cmd, FASTBOOT_APPEND_EC_ATTESTATION_CERT_ENC)) {
		if (trusty_append_attestation_cert_chain_enc(fastboot_buf_addr,
								fastboot_bytes_received,
								KM_ALGORITHM_EC)) {
			printf("ERROR append ec attestation cert chain failed!\n");
			strcpy(response, "FAILInternal error!");
		} else {
			printf("Append ec attestation key successfully!\n");
			strcpy(response, "OKAY");
		}
	} else if (endswith(cmd, FASTBOOT_SET_RSA_ATTESTATION_KEY)) {
		if (trusty_set_attestation_key(fastboot_buf_addr,
						fastboot_bytes_received,
						KM_ALGORITHM_RSA)) {
			printf("ERROR set rsa attestation key failed!\n");
			strcpy(response, "FAILInternal error!");
		} else {
			printf("Set rsa attestation key successfully!\n");
			strcpy(response, "OKAY");
		}
	} else if (endswith(cmd, FASTBOOT_SET_EC_ATTESTATION_KEY)) {
		if (trusty_set_attestation_key(fastboot_buf_addr,
						fastboot_bytes_received,
						KM_ALGORITHM_EC)) {
			printf("ERROR set ec attestation key failed!\n");
			strcpy(response, "FAILInternal error!");
		} else {
			printf("Set ec attestation key successfully!\n");
			strcpy(response, "OKAY");
		}
	} else if (endswith(cmd, FASTBOOT_APPEND_RSA_ATTESTATION_CERT)) {
		if (trusty_append_attestation_cert_chain(fastboot_buf_addr,
							fastboot_bytes_received,
							KM_ALGORITHM_RSA)) {
			printf("ERROR append rsa attestation cert chain failed!\n");
			strcpy(response, "FAILInternal error!");
		} else {
			printf("Append rsa attestation key successfully!\n");
			strcpy(response, "OKAY");
		}
	}  else if (endswith(cmd, FASTBOOT_APPEND_EC_ATTESTATION_CERT)) {
		if (trusty_append_attestation_cert_chain(fastboot_buf_addr,
							fastboot_bytes_received,
							KM_ALGORITHM_EC)) {
			printf("ERROR append ec attestation cert chain failed!\n");
			strcpy(response, "FAILInternal error!");
		} else {
			printf("Append ec attestation key successfully!\n");
			strcpy(response, "OKAY");
		}
	}  else if (endswith(cmd, FASTBOOT_GET_MPPUBK)) {
		if (fastboot_get_mppubk(fastboot_buf_addr, &fastboot_bytes_received)) {
			printf("ERROR Generate mppubk failed!\n");
			strcpy(response, "FAILGenerate mppubk failed!");
		} else {
			printf("mppubk generated!\n");
			strcpy(response, "OKAY");
		}
	}  else if (endswith(cmd, FASTBOOT_GET_SERIAL_NUMBER)) {
		char *serial = get_serial();

		if (!serial)
			strcpy(response, "FAILSerial number not support!");
		else {
			/* Serial number will not exceed 16 bytes.*/
			strncpy(fastboot_buf_addr, serial, 16);
			fastboot_bytes_received = 16;
			printf("Serial number generated!\n");
			strcpy(response, "OKAY");
		}
	}
#ifndef CONFIG_AVB_ATX
	else if (endswith(cmd, FASTBOOT_SET_RPMB_KEY)) {
		if (fastboot_set_rpmb_key(fastboot_buf_addr, fastboot_bytes_received)) {
			printf("ERROR set rpmb key failed!\n");
			strcpy(response, "FAILset rpmb key failed!");
		} else
			strcpy(response, "OKAY");
	} else if (endswith(cmd, FASTBOOT_SET_RPMB_RANDOM_KEY)) {
		if (fastboot_set_rpmb_random_key()) {
			printf("ERROR set rpmb random key failed!\n");
			strcpy(response, "FAILset rpmb random key failed!");
		} else
			strcpy(response, "OKAY");
	} else if (endswith(cmd, FASTBOOT_SET_VBMETA_PUBLIC_KEY)) {
		if (avb_set_public_key(fastboot_buf_addr,
					fastboot_bytes_received))
			strcpy(response, "FAILcan't set public key!");
		else
			strcpy(response, "OKAY");
	}
#endif /* !CONFIG_AVB_ATX */
#endif /* CONFIG_IMX_TRUSTY_OS */
	else if (endswith(cmd, "unlock_critical")) {
		strcpy(response, "OKAY");
	} else if (endswith(cmd, "unlock")) {
		printf("flashing unlock.\n");
#ifdef CONFIG_AVB_ATX
		/* We should do nothing here For Android Things which
		 * enables the authenticated unlock feature.
		 */
		strcpy(response, "OKAY");
#else
		status = do_fastboot_unlock(false);
		if (status != FASTBOOT_LOCK_ERROR)
			strcpy(response, "OKAY");
		else
			strcpy(response, "FAILunlock device failed.");
#endif
	} else if (endswith(cmd, "lock")) {
#ifdef CONFIG_AVB_ATX
		/* We should do nothing here For Android Things which
		 * enables the at-lock-vboot feature.
		 */
		strcpy(response, "OKAY");
#else
		printf("flashing lock.\n");
		status = do_fastboot_lock();
		if (status != FASTBOOT_LOCK_ERROR)
			strcpy(response, "OKAY");
		else
			strcpy(response, "FAILlock device failed.");
#endif
	} else if (endswith(cmd, "get_unlock_ability")) {
		result = fastboot_lock_enable();
		if (result == FASTBOOT_UL_ENABLE) {
			fastboot_tx_write_more("INFO1");
			strcpy(response, "OKAY");
		} else if (result == FASTBOOT_UL_DISABLE) {
			fastboot_tx_write_more("INFO0");
			strcpy(response, "OKAY");
		} else {
			printf("flashing get_unlock_ability fail!\n");
			strcpy(response, "FAILget unlock ability failed.");
		}
	} else {
		printf("Unknown flashing command:%s\n", cmd);
		strcpy(response, "FAILcommand not defined");
	}
	fastboot_tx_write_more(response);

	/* Must call fastboot_none_resp before returning from the dispatch function
	 *  which uses fastboot_tx_write_more
	 */
	fastboot_none_resp(response);
}
#endif /* CONFIG_FASTBOOT_LOCK */

#ifdef CONFIG_AVB_SUPPORT
static void set_active_avb(char *cmd, char *response)
{
	AvbIOResult ret;
	int slot = 0;

	if (!cmd) {
		pr_err("missing slot suffix\n");
		fastboot_fail("missing slot suffix", response);
		return;
	}

#ifdef CONFIG_VIRTUAL_AB_SUPPORT
	if (virtual_ab_update_is_merging()) {
		printf("Can not switch slot while snapshot merge is in progress!\n");
		fastboot_fail("Snapshot merge is in progress!", response);
		return;
	}

	/* Only output a warning when the image is snapshoted. */
	if (virtual_ab_update_is_snapshoted())
		printf("Warning: changing the active slot with a snapshot applied may cancel the update!\n");
	else
		printf("Warning: Virtual A/B is enabled, switch slot may make the system fail to boot. \n");
#endif

	slot = slotidx_from_suffix(cmd);

	if (slot < 0) {
		fastboot_fail("err slot suffix", response);
		return;
	}

	ret = fsl_avb_ab_mark_slot_active(&fsl_avb_ab_ops, slot);
	if (ret != AVB_IO_RESULT_OK)
		fastboot_fail("avb IO error", response);
	else
		fastboot_okay(NULL, response);

	return;
}
#endif /*CONFIG_AVB_SUPPORT*/

#if CONFIG_IS_ENABLED(FASTBOOT_FLASH)
static void flash(char *cmd, char *response)
{
	if (!cmd) {
		pr_err("missing partition name");
		fastboot_fail("missing partition name", response);
		return;
	}

	/* Always enable image flash for Android Things. */
#if defined(CONFIG_FASTBOOT_LOCK) && !defined(CONFIG_AVB_ATX)
	int status;
	status = fastboot_get_lock_stat();

	if (status == FASTBOOT_LOCK) {
		pr_err("device is LOCKed!\n");
		fastboot_fail("device is locked.", response);
		return;

	} else if (status == FASTBOOT_LOCK_ERROR) {
		pr_err("write lock status into device!\n");
		fastboot_set_lock_stat(FASTBOOT_LOCK);
		fastboot_fail("device is locked.", response);
		return;
	}
#endif

#ifdef CONFIG_VIRTUAL_AB_SUPPORT
	if (partition_is_protected_during_merge(cmd)) {
		printf("Can not flash partition %s while a snapshot update is in progress!\n", cmd);
		fastboot_fail("Snapshot update is in progress", response);
		return;
	}
#endif

	fastboot_process_flash(cmd, fastboot_buf_addr,
		fastboot_bytes_received, response);

#ifdef CONFIG_VIRTUAL_AB_SUPPORT
	/* Cancel virtual AB update after image flash */
	if (virtual_ab_update_is_merging() || virtual_ab_update_is_snapshoted())
		virtual_ab_cancel_update();
#endif

#if defined(CONFIG_FASTBOOT_LOCK)
	if (strncmp(cmd, "gpt", 3) == 0) {
		int gpt_valid = 0;
		gpt_valid = partition_table_valid();
		/* If gpt is valid, load partitons table into memory.
		   So if the next command is "fastboot reboot bootloader",
		   it can find the "misc" partition to r/w. */
		if(gpt_valid) {
			fastboot_load_partitions();
			/* Unlock device if the gpt is valid */
			do_fastboot_unlock(true);
		}
	}

#endif
}

static void erase(char *cmd, char *response)
{
	if (!cmd) {
		pr_err("missing partition name");
		fastboot_fail("missing partition name", response);
		return;
	}

#if defined(CONFIG_FASTBOOT_LOCK) && !defined(CONFIG_AVB_ATX)
	FbLockState status;
	status = fastboot_get_lock_stat();
	if (status == FASTBOOT_LOCK) {
		pr_err("device is LOCKed!\n");
		fastboot_fail("device is locked.", response);
		return;
	} else if (status == FASTBOOT_LOCK_ERROR) {
		pr_err("write lock status into device!\n");
		fastboot_set_lock_stat(FASTBOOT_LOCK);
		fastboot_fail("device is locked.", response);
		return;
	}
#endif

#ifdef CONFIG_VIRTUAL_AB_SUPPORT
	if (partition_is_protected_during_merge(cmd)) {
		printf("Can not erase partition %s while a snapshot update is in progress!", cmd);
		fastboot_fail("Snapshot update is in progress", response);
		return;
	}
#endif

	fastboot_process_erase(cmd, response);
}
#endif

/**
 * fastboot_set_reboot_flag() - Set flag to indicate reboot-bootloader
 *
 * This is a redefinition, since BSP dose not need the function of
 * "reboot into bootloader", and with BCB support, the flag can be
 * set with another way. Redefine this function to override the weak
 * definition to avoid error return value.
 */
int fastboot_set_reboot_flag(void)
{
	return 0;
}

#if CONFIG_IS_ENABLED(FASTBOOT_UUU_SUPPORT)
/**
 * run_ucmd() - Execute the UCmd command
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 */
static void run_ucmd(char *cmd_parameter, char *response)
{
	if (!cmd_parameter) {
		pr_err("missing slot suffix\n");
		fastboot_fail("missing command", response);
		return;
	}
	if(run_command(cmd_parameter, 0)) {
		fastboot_fail("", response);
	} else {
		fastboot_okay(NULL, response);
		/* cmd may impact fastboot related environment*/
		fastboot_setup();
	}
}

static char g_a_cmd_buff[64];

void fastboot_acmd_complete(void)
{
	run_command(g_a_cmd_buff, 0);
}

/**
 * run_acmd() - Execute the ACmd command
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 */
static void run_acmd(char *cmd_parameter, char *response)
{
	if (!cmd_parameter) {
		pr_err("missing slot suffix\n");
		fastboot_fail("missing command", response);
		return;
	}

	if (strlen(cmd_parameter) >= sizeof(g_a_cmd_buff)) {
		pr_err("input acmd is too long\n");
		fastboot_fail("too long command", response);
		return;
	}

	strcpy(g_a_cmd_buff, cmd_parameter);
	fastboot_okay(NULL, response);
}
#endif

#ifdef CONFIG_VIRTUAL_AB_SUPPORT
static void snapshot_update(char *cmd_parameter, char *response)
{
	if (endswith(cmd_parameter, "cancel")) {
		FbLockState status;
		status = fastboot_get_lock_stat();
		if ((status == FASTBOOT_LOCK) || (status == FASTBOOT_LOCK_ERROR)) {
			printf("Can not cancel snapshot update when the device is locked!\n");
			fastboot_fail("device is locked!", response);
		} else if (virtual_ab_update_is_merging() || virtual_ab_update_is_snapshoted()) {
			if (virtual_ab_cancel_update() != -1)
				fastboot_okay(NULL, response);
			else
				fastboot_fail("Can't cancel snapshot update!", response);
		} else {
			printf("Device is not in 'merging' or 'snapshotted' state, do nothing...\n");
			fastboot_okay(NULL, response);
		}

		return;
	} else {
		printf("Error! Only 'cancel' is supported!");
		strcpy(response, "FAILInternal error!");
	}

	return;
}
#endif

static const struct {
	const char *command;
	void (*dispatch)(char *cmd_parameter, char *response);
} commands[FASTBOOT_COMMAND_COUNT] = {
		[FASTBOOT_COMMAND_REBOOT_BOOTLOADER] = {
			.command = "reboot-bootloader",
			.dispatch = reboot_bootloader,
		},
		[FASTBOOT_COMMAND_UPLOAD] = {
			.command = "upload",
			.dispatch = upload,
		},
		[FASTBOOT_COMMAND_GETSTAGED] = {
			.command = "get_staged",
			.dispatch = upload,
		},
#if defined(CONFIG_FASTBOOT_LOCK)
		[FASTBOOT_COMMAND_FLASHING] = {
			.command = "flashing",
			.dispatch = flashing,
		},
		[FASTBOOT_COMMAND_OEM] = {
			.command = "oem",
			.dispatch = flashing,
		},
#endif
#ifdef CONFIG_AVB_SUPPORT
		[FASTBOOT_COMMAND_SETACTIVE] = {
			.command = "set_active",
			.dispatch = set_active_avb,
		},
#endif
#if CONFIG_IS_ENABLED(FASTBOOT_UUU_SUPPORT)
		[FASTBOOT_COMMAND_UCMD] = {
			.command = "UCmd",
			.dispatch = run_ucmd,
		},
		[FASTBOOT_COMMAND_ACMD] = {
			.command ="ACmd",
			.dispatch = run_acmd,
		},
#endif
		[FASTBOOT_COMMAND_REBOOT] = {
			.command = "reboot",
			.dispatch = okay,
		},
		[FASTBOOT_COMMAND_GETVAR] = {
			.command = "getvar",
			.dispatch = getvar,
		},
		[FASTBOOT_COMMAND_DOWNLOAD] = {
			.command = "download",
			.dispatch = download,
		},
		[FASTBOOT_COMMAND_BOOT] = {
			.command = "boot",
			.dispatch = okay,
		},
		[FASTBOOT_COMMAND_CONTINUE] = {
			.command = "continue",
			.dispatch = okay,
		},
#ifdef CONFIG_FASTBOOT_FLASH
		[FASTBOOT_COMMAND_FLASH] = {
			.command = "flash",
			.dispatch = flash,
		},
		[FASTBOOT_COMMAND_ERASE] = {
			.command = "erase",
			.dispatch = erase,
		},
#endif
#ifdef CONFIG_AVB_ATX
		[FASTBOOT_COMMAND_STAGE] = {
			.command = "stage",
			.dispatch = download,
		},
#endif
#ifdef CONFIG_ANDROID_RECOVERY
		[FASTBOOT_COMMAND_RECOVERY_FASTBOOT] = {
			.command = "reboot-fastboot",
			.dispatch = reboot_fastboot,
		},
#endif
#ifdef CONFIG_VIRTUAL_AB_SUPPORT
		[FASTBOOT_COMMAND_SNAPSHOT_UPDATE] = {
			.command = "snapshot-update",
			.dispatch = snapshot_update,
		},
#endif
};

/**
 * fastboot_handle_command - Handle fastboot command
 *
 * @cmd_string: Pointer to command string
 * @response: Pointer to fastboot response buffer
 *
 * Return: Executed command, or -1 if not recognized
 */
int fastboot_handle_command(char *cmd_string, char *response)
{
	int i;
	char *cmd_parameter;

	cmd_parameter = cmd_string;
	strsep(&cmd_parameter, ":");
	/* separate cmdstring for "fastboot oem/flashing" with a blank */
	if(cmd_parameter == NULL)
	{
		cmd_parameter = cmd_string;
		strsep(&cmd_parameter, " ");
	}

	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (commands[i].command != NULL &&
			!strcmp(commands[i].command, cmd_string)) {
			if (commands[i].dispatch) {
				commands[i].dispatch(cmd_parameter,
							response);
				return i;
			} else {
				break;
			}
		}
	}

	pr_err("command %s not recognized.\n", cmd_string);
	fastboot_fail("unrecognized command", response);
	return -1;
}
