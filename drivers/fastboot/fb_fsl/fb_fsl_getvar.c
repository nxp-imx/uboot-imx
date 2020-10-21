// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 */

#include <common.h>
#include <asm/mach-imx/sys_proto.h>
#include <fb_fsl.h>
#include <fastboot.h>
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
#include <version.h>

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

#include "fb_fsl_common.h"

#ifdef CONFIG_IMX_TRUSTY_OS
#include "u-boot/sha256.h"
#include <trusty/libtipc.h>

#define ATAP_UUID_SIZE 32
#define ATAP_UUID_STR_SIZE ((ATAP_UUID_SIZE*2) + 1)
#endif

#ifdef CONFIG_VIRTUAL_AB_SUPPORT
#include "fb_fsl_virtual_ab.h"
#endif

#if defined(CONFIG_ANDROID_THINGS_SUPPORT) && defined(CONFIG_ARCH_IMX8M)
#define FASTBOOT_COMMON_VAR_NUM 15
#else
#define FASTBOOT_COMMON_VAR_NUM 14
#endif

#define FASTBOOT_VAR_YES    "yes"
#define FASTBOOT_VAR_NO     "no"

/* common variables of fastboot getvar command */
char *fastboot_common_var[FASTBOOT_COMMON_VAR_NUM] = {
	"version",
	"version-bootloader",
	"version-baseband",
	"product",
	"secure",
	"max-download-size",
	"erase-block-size",
	"logical-block-size",
	"unlocked",
	"off-mode-charge",
	"battery-voltage",
	"variant",
	"battery-soc-ok",
	"is-userspace",
#if defined(CONFIG_ANDROID_THINGS_SUPPORT) && defined(CONFIG_ARCH_IMX8M)
	"baseboard_id"
#endif
};

/* at-vboot-state variable list */
#ifdef CONFIG_AVB_ATX
#define AT_VBOOT_STATE_VAR_NUM 6
extern struct imx_sec_config_fuse_t const imx_sec_config_fuse;
extern int fuse_read(u32 bank, u32 word, u32 *val);

char *fastboot_at_vboot_state_var[AT_VBOOT_STATE_VAR_NUM] = {
	"bootloader-locked",
	"bootloader-min-versions",
	"avb-perm-attr-set",
	"avb-locked",
	"avb-unlock-disabled",
	"avb-min-versions"
};
#endif

static int strcmp_l1(const char *s1, const char *s2)
{
	if (!s1 || !s2)
		return -1;
	return strncmp(s1, s2, strlen(s1));
}

static bool is_slotvar(char *cmd)
{
	assert(cmd != NULL);
	if (!strcmp_l1("has-slot:", cmd) ||
		!strcmp_l1("slot-successful:", cmd) ||
		!strcmp_l1("slot-count", cmd) ||
		!strcmp_l1("slot-suffixes", cmd) ||
		!strcmp_l1("current-slot", cmd) ||
		!strcmp_l1("slot-unbootable:", cmd) ||
		!strcmp_l1("slot-retry-count:", cmd))
			return true;
	return false;
}

static char serial[IMX_SERIAL_LEN];

char *get_serial(void)
{
#ifdef CONFIG_SERIAL_TAG
	struct tag_serialnr serialnr;
	memset(serial, 0, IMX_SERIAL_LEN);

	get_board_serial(&serialnr);
	sprintf(serial, "%08x%08x", serialnr.high, serialnr.low);
	return serial;
#else
	return NULL;
#endif
}

#if !defined(PRODUCT_NAME)
#define PRODUCT_NAME "NXP i.MX"
#endif

#if !defined(VARIANT_NAME)
#define VARIANT_NAME "NXP i.MX"
#endif

#ifdef CONFIG_IMX_TRUSTY_OS
static void uuid_hex2string(uint8_t *uuid, char* buf, uint32_t uuid_len, uint32_t uuid_strlen) {
	uint32_t i;
	if (!uuid || !buf)
		return;
	char *cp = buf;
	char *buf_end = buf + uuid_strlen;
	for (i = 0; i < uuid_len; i++) {
		cp += snprintf(cp, buf_end - cp, "%02x", uuid[i]);
	}
}
#endif

#if defined(CONFIG_ANDROID_THINGS_SUPPORT) && defined(CONFIG_ARCH_IMX8M)
int get_imx8m_baseboard_id(void);
#endif

static int get_single_var(char *cmd, char *response)
{
	char *str = cmd;
	int chars_left;
	const char *s;
	struct mmc *mmc;
	int mmc_dev_no;
	int blksz;

	chars_left = FASTBOOT_RESPONSE_LEN - strlen(response) - 1;

	if ((str = strstr(cmd, "partition-size:"))) {
		str +=strlen("partition-size:");
		struct fastboot_ptentry* fb_part;
		fb_part = fastboot_flash_find_ptn(str);
		if (!fb_part) {
			strncat(response, "Wrong partition name.", chars_left);
			fastboot_flash_dump_ptn();
			return -1;
		} else {
			snprintf(response + strlen(response), chars_left,
				 "0x%llx",
				 (uint64_t)fb_part->length * get_block_size());
		}
	} else if ((str = strstr(cmd, "partition-type:"))) {
		str +=strlen("partition-type:");
		struct fastboot_ptentry* fb_part;
		fb_part = fastboot_flash_find_ptn(str);
		if (!fb_part) {
			strncat(response, "Wrong partition name.", chars_left);
			fastboot_flash_dump_ptn();
			return -1;
		} else {
			strncat(response, fb_part->fstype, chars_left);
		}
	} else if ((str = strstr(cmd, "is-logical:"))) {
		str +=strlen("is-logical:");
		struct fastboot_ptentry* fb_part;
		fb_part = fastboot_flash_find_ptn(str);
		if (!fb_part) {
			return -1;
		} else {
			snprintf(response + strlen(response), chars_left, "no");
		}
	} else if (!strcmp_l1("version-baseband", cmd)) {
		strncat(response, "N/A", chars_left);
	} else if (!strcmp_l1("version-bootloader", cmd) ||
		!strcmp_l1("bootloader-version", cmd)) {
		strncat(response, U_BOOT_VERSION, chars_left);
	} else if (!strcmp_l1("version", cmd)) {
		strncat(response, FASTBOOT_VERSION, chars_left);
	} else if (!strcmp_l1("battery-voltage", cmd)) {
		strncat(response, "0mV", chars_left);
	} else if (!strcmp_l1("battery-soc-ok", cmd)) {
		strncat(response, "yes", chars_left);
	} else if (!strcmp_l1("variant", cmd)) {
		strncat(response, VARIANT_NAME, chars_left);
	} else if (!strcmp_l1("off-mode-charge", cmd)) {
		strncat(response, "1", chars_left);
	} else if (!strcmp_l1("is-userspace", cmd)) {
		strncat(response, FASTBOOT_VAR_NO, chars_left);
	} else if (!strcmp_l1("downloadsize", cmd) ||
		!strcmp_l1("max-download-size", cmd)) {

		snprintf(response + strlen(response), chars_left, "0x%x", CONFIG_FASTBOOT_BUF_SIZE);
	} else if (!strcmp_l1("erase-block-size", cmd)) {
		mmc_dev_no = mmc_get_env_dev();
		mmc = find_mmc_device(mmc_dev_no);
		if (!mmc) {
			strncat(response, "FAILCannot get dev", chars_left);
			return -1;
		}
		blksz = get_block_size();
		snprintf(response + strlen(response), chars_left, "0x%x",
				(blksz * mmc->erase_grp_size));
	} else if (!strcmp_l1("logical-block-size", cmd)) {
		blksz = get_block_size();
		snprintf(response + strlen(response), chars_left, "0x%x", blksz);
	} else if (!strcmp_l1("serialno", cmd)) {
		s = get_serial();
		if (s)
			strncat(response, s, chars_left);
		else {
			strncat(response, "FAILValue not set", chars_left);
			return -1;
		}
	} else if (!strcmp_l1("product", cmd)) {
		strncat(response, PRODUCT_NAME, chars_left);
	}
#ifdef CONFIG_IMX_TRUSTY_OS
        else if(!strcmp_l1("at-attest-uuid", cmd)) {
		char *uuid;
		char uuid_str[ATAP_UUID_STR_SIZE];
		if (trusty_atap_read_uuid_str(&uuid)) {
			printf("ERROR read uuid failed!\n");
			strncat(response, "FAILCannot get uuid!", chars_left);
			return -1;
		} else {
			uuid_hex2string((uint8_t*)uuid, uuid_str,ATAP_UUID_SIZE, ATAP_UUID_STR_SIZE);
			strncat(response, uuid_str, chars_left);
			trusty_free(uuid);
		}
	}
	else if(!strcmp_l1("at-attest-dh", cmd)) {
		strncat(response, "1:P256,2:curve25519", chars_left);
	}
#endif
#if defined(CONFIG_FASTBOOT_LOCK)
	else if (!strcmp_l1("secure", cmd)) {
		strncat(response, FASTBOOT_VAR_YES, chars_left);
	} else if (!strcmp_l1("unlocked",cmd)){
		int status = fastboot_get_lock_stat();
		if (status == FASTBOOT_UNLOCK) {
			strncat(response, FASTBOOT_VAR_YES, chars_left);
		} else {
			strncat(response, FASTBOOT_VAR_NO, chars_left);
		}
	}
#else
	else if (!strcmp_l1("secure", cmd)) {
		strncat(response, FASTBOOT_VAR_NO, chars_left);
	} else if (!strcmp_l1("unlocked",cmd)) {
		strncat(response, FASTBOOT_VAR_NO, chars_left);
	}
#endif
	else if (is_slotvar(cmd)) {
#ifdef CONFIG_AVB_SUPPORT
		if (get_slotvar_avb(&fsl_avb_ab_ops, cmd,
				response + strlen(response), chars_left + 1) < 0)
			return -1;
#else
		strncat(response, FASTBOOT_VAR_NO, chars_left);
#endif
	}
#if defined(CONFIG_ANDROID_THINGS_SUPPORT) && defined(CONFIG_ARCH_IMX8M)
	else if (!strcmp_l1("baseboard_id", cmd)) {
		int baseboard_id;

		baseboard_id = get_imx8m_baseboard_id();
		if (baseboard_id < 0) {
			printf("Get baseboard id failed!\n");
			strncat(response, "Get baseboard id failed!", chars_left);
			return -1;
		} else
			snprintf(response + strlen(response), chars_left, "0x%x", baseboard_id);
	}
#endif
#ifdef CONFIG_AVB_ATX
	else if (!strcmp_l1("bootloader-locked", cmd)) {

		/* Below is basically copied from is_hab_enabled() */
		struct imx_sec_config_fuse_t *fuse =
			(struct imx_sec_config_fuse_t *)&imx_sec_config_fuse;
		uint32_t reg;
		int ret;

		/* Read the secure boot status from fuse. */
		ret = fuse_read(fuse->bank, fuse->word, &reg);
		if (ret) {
			printf("\nSecure boot fuse read error!\n");
			strncat(response, "Secure boot fuse read error!", chars_left);
			return -1;
		}
		/* Check if the secure boot bit is enabled */
		if ((reg & 0x2000000) == 0x2000000)
			strncat(response, "1", chars_left);
		else
			strncat(response, "0", chars_left);
	} else if (!strcmp_l1("bootloader-min-versions", cmd)) {
#ifndef CONFIG_ARM64
		/* We don't support bootloader rbindex protection for
		 * ARM32(like imx7d) and the format is: "bootloader,tee". */
		strncat(response, "-1,-1", chars_left);

#elif defined(CONFIG_DUAL_BOOTLOADER)
		/* Rbindex protection for bootloader is supported only when the
		 * 'dual bootloader' feature is enabled. U-boot will get the rbindx
		 * from RAM which is passed by spl because we can only get the rbindex
		 * at spl stage. The format in this case is: "spl,atf,tee,u-boot".
		 */
		struct bl_rbindex_package *bl_rbindex;
		uint32_t rbindex;

		bl_rbindex = (struct bl_rbindex_package *)BL_RBINDEX_LOAD_ADDR;
		if (!strncmp(bl_rbindex->magic, BL_RBINDEX_MAGIC,
				BL_RBINDEX_MAGIC_LEN)) {
			rbindex = bl_rbindex->rbindex;
			snprintf(response + strlen(response), chars_left,
					"-1,%d,%d,%d",rbindex, rbindex, rbindex);
		} else {
			printf("Error bootloader rbindex magic!\n");
			strncat(response, "Get bootloader rbindex fail!", chars_left);
			return -1;
		}
#else
		/* Return -1 for all partition if 'dual bootloader' feature
		 * is not enabled */
		strncat(response, "-1,-1,-1,-1", chars_left);
#endif
	} else if (!strcmp_l1("avb-perm-attr-set", cmd)) {
		if (perm_attr_are_fused())
			strncat(response, "1", chars_left);
		else
			strncat(response, "0", chars_left);
	} else if (!strcmp_l1("avb-locked", cmd)) {
		FbLockState status;

		status = fastboot_get_lock_stat();
		if (status == FASTBOOT_LOCK)
			strncat(response, "1", chars_left);
		else if (status == FASTBOOT_UNLOCK)
			strncat(response, "0", chars_left);
		else {
			printf("Get lock state error!\n");
			strncat(response, "Get lock state failed!", chars_left);
			return -1;
		}
	} else if (!strcmp_l1("avb-unlock-disabled", cmd)) {
		if (at_unlock_vboot_is_disabled())
			strncat(response, "1", chars_left);
		else
			strncat(response, "0", chars_left);
	} else if (!strcmp_l1("avb-min-versions", cmd)) {
		int i = 0;
		/* rbindex location/value can be very large
		 * number so we reserve enough space here.
		 */
		char buffer[35];
		uint32_t rbindex_location[AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS + 2];
		uint32_t location;
		uint64_t rbindex;

		memset(buffer, '\0', sizeof(buffer));

		/* Set rbindex locations. */
		for (i = 0; i < AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS; i++)
			rbindex_location[i] = i;

		/* Set Android Things key version rbindex locations */
		rbindex_location[AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS]
						= AVB_ATX_PIK_VERSION_LOCATION;
		rbindex_location[AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS + 1]
						= AVB_ATX_PSK_VERSION_LOCATION;

		/* Read rollback index and set the reponse*/
		for (i = 0; i < AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS + 2; i++) {
			location = rbindex_location[i];
			if (fsl_avb_ops.read_rollback_index(&fsl_avb_ops,
								location, &rbindex)
								!= AVB_IO_RESULT_OK) {
				printf("Read rollback index error!\n");
				snprintf(response, FASTBOOT_RESPONSE_LEN,
					"INFOread rollback index error when get avb-min-versions");
				return -1;
			}
			/* Generate the "location:value" pair */
			snprintf(buffer, sizeof(buffer), "%d:%lld", location, rbindex);
			if (i != AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS + 1)
				strncat(buffer, ",", strlen(","));

			if ((chars_left - (int)strlen(buffer)) >= 0) {
				strncat(response, buffer, strlen(buffer));
				chars_left -= strlen(buffer);
			} else {
				strncat(response, buffer, chars_left);
				/* reponse buffer is full, send it first */
				fastboot_tx_write_more(response);
				/* reset the reponse buffer for next round */
				memset(response, '\0', FASTBOOT_RESPONSE_LEN);
				strncpy(response, "INFO", 5);
				/* Copy left strings from 'buffer' to 'response' */
				strncat(response, buffer + chars_left, strlen(buffer));
				chars_left = FASTBOOT_RESPONSE_LEN -
						strlen(response) - 1;
			}
		}

	}
#endif
#ifdef CONFIG_VIRTUAL_AB_SUPPORT
	else if (!strcmp_l1("snapshot-update-status", cmd)) {
		if (virtual_ab_update_is_merging())
			strncat(response, "merging", chars_left);
		else if (virtual_ab_update_is_snapshoted())
			strncat(response, "snapshotted", chars_left);
		else
			strncat(response, "none", chars_left);
	}
#endif
	else {
		char envstr[32];

		snprintf(envstr, sizeof(envstr) - 1, "fastboot.%s", cmd);
		s = env_get(envstr);
		if (s) {
			strncat(response, s, chars_left);
		} else {
			snprintf(response, chars_left, "FAILunknown variable:%s",cmd);
			printf("WARNING: unknown variable: %s\n", cmd);
			return -1;
		}
	}
	return 0;
}

void fastboot_getvar(char *cmd, char *response)
{
	int n = 0;
	int status = 0;
	int count = 0;
	char var_name[FASTBOOT_RESPONSE_LEN];
	char partition_base_name[MAX_PTN][20];
	char slot_suffix[2][5] = {"a","b"};

	if (!cmd) {
		pr_err("missing variable");
		fastboot_fail("missing var", response);
		return;
	}

	if (!strcmp_l1("all", cmd)) {

		memset(response, '\0', FASTBOOT_RESPONSE_LEN);


		/* get common variables */
		for (n = 0; n < FASTBOOT_COMMON_VAR_NUM; n++) {
			snprintf(response, FASTBOOT_RESPONSE_LEN, "INFO%s:", fastboot_common_var[n]);
			get_single_var(fastboot_common_var[n], response);
			fastboot_tx_write_more(response);
		}

		/* get at-vboot-state variables */
#ifdef CONFIG_AVB_ATX
		for (n = 0; n < AT_VBOOT_STATE_VAR_NUM; n++) {
			snprintf(response, FASTBOOT_RESPONSE_LEN, "INFO%s:", fastboot_at_vboot_state_var[n]);
			get_single_var(fastboot_at_vboot_state_var[n], response);
			fastboot_tx_write_more(response);
		}
#endif
		/* get partition type */
		for (n = 0; n < g_pcount; n++) {
			snprintf(response, FASTBOOT_RESPONSE_LEN, "INFOpartition-type:%s:", g_ptable[n].name);
			snprintf(var_name, sizeof(var_name), "partition-type:%s", g_ptable[n].name);
			get_single_var(var_name, response);
			fastboot_tx_write_more(response);
		}
		/* get partition size */
		for (n = 0; n < g_pcount; n++) {
			snprintf(response, FASTBOOT_RESPONSE_LEN, "INFOpartition-size:%s:", g_ptable[n].name);
			snprintf(var_name, sizeof(var_name), "partition-size:%s", g_ptable[n].name);
			get_single_var(var_name,response);
			fastboot_tx_write_more(response);
		}
		/* slot related variables */
		if (fastboot_parts_is_slot()) {
			/* get has-slot variables */
			count = fastboot_parts_get_name(partition_base_name);
			for (n = 0; n < count; n++) {
				snprintf(response, FASTBOOT_RESPONSE_LEN, "INFOhas-slot:%s:", partition_base_name[n]);
				snprintf(var_name, sizeof(var_name), "has-slot:%s", partition_base_name[n]);
				get_single_var(var_name,response);
				fastboot_tx_write_more(response);
			}
			/* get current slot */
			strncpy(response, "INFOcurrent-slot:", FASTBOOT_RESPONSE_LEN);
			get_single_var("current-slot", response);
			fastboot_tx_write_more(response);
			/* get slot count */
			strncpy(response, "INFOslot-count:", FASTBOOT_RESPONSE_LEN);
			get_single_var("slot-count", response);
			fastboot_tx_write_more(response);
			/* get slot-successful variable */
			for (n = 0; n < 2; n++) {
				snprintf(response, FASTBOOT_RESPONSE_LEN, "INFOslot-successful:%s:", slot_suffix[n]);
				snprintf(var_name, sizeof(var_name), "slot-successful:%s", slot_suffix[n]);
				get_single_var(var_name, response);
				fastboot_tx_write_more(response);
			}
			/*get slot-unbootable variable*/
			for (n = 0; n < 2; n++) {
				snprintf(response, FASTBOOT_RESPONSE_LEN, "INFOslot-unbootable:%s:", slot_suffix[n]);
				snprintf(var_name, sizeof(var_name), "slot-unbootable:%s", slot_suffix[n]);
				get_single_var(var_name, response);
				fastboot_tx_write_more(response);
			}
			/*get slot-retry-count variable*/
			for (n = 0; n < 2; n++) {
				snprintf(response, FASTBOOT_RESPONSE_LEN, "INFOslot-retry-count:%s:", slot_suffix[n]);
				snprintf(var_name, sizeof(var_name), "slot-retry-count:%s", slot_suffix[n]);
				get_single_var(var_name, response);
				fastboot_tx_write_more(response);
			}
		}

#ifdef CONFIG_VIRTUAL_AB_SUPPORT
		strncpy(response, "INFOsnapshot-update-status:", FASTBOOT_RESPONSE_LEN);
		get_single_var("snapshot-update-status", response);
		fastboot_tx_write_more(response);
#endif

		strncpy(response, "OKAYDone!", 10);
		fastboot_tx_write_more(response);
		fastboot_none_resp(response);

		return;
	}
#ifdef CONFIG_AVB_ATX
	else if (!strcmp_l1("at-vboot-state", cmd)) {
			/* get at-vboot-state variables */
		for (n = 0; n < AT_VBOOT_STATE_VAR_NUM; n++) {
			snprintf(response, FASTBOOT_RESPONSE_LEN, "INFO%s:", fastboot_at_vboot_state_var[n]);
			get_single_var(fastboot_at_vboot_state_var[n], response);
			fastboot_tx_write_more(response);
		}

		strncpy(response, "OKAY", 5);
		fastboot_tx_write_more(response);
		fastboot_none_resp(response);

		return;
	} else if ((!strcmp_l1("bootloader-locked", cmd)) ||
			(!strcmp_l1("bootloader-min-versions", cmd)) ||
			(!strcmp_l1("avb-perm-attr-set", cmd)) ||
			(!strcmp_l1("avb-locked", cmd)) ||
			(!strcmp_l1("avb-unlock-disabled", cmd)) ||
			(!strcmp_l1("avb-min-versions", cmd))) {

		printf("Can't get this variable alone, get 'at-vboot-state' instead!\n");
		fastboot_fail("Can't get this variable alone, get 'at-vboot-state' instead.", response);
		return;
	}
#endif
	else {
		char reason[FASTBOOT_RESPONSE_LEN];
		memset(reason, '\0', FASTBOOT_RESPONSE_LEN);

		status = get_single_var(cmd, reason);
		if (status != 0)
			fastboot_fail(reason, response);
		else
			fastboot_okay(reason, response);

		return;
	}
}
