// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 */

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

#ifdef CONFIG_IMX_TRUSTY_OS
#include "u-boot/sha256.h"
#include <trusty/libtipc.h>

extern int armv7_init_nonsec(void);
extern void trusty_os_init(void);
#endif

#include "fb_fsl_common.h"

#include <serial.h>
#include <stdio_dev.h>

#if defined(CONFIG_AVB_SUPPORT) && defined(CONFIG_MMC)
AvbABOps fsl_avb_ab_ops = {
	.read_ab_metadata = fsl_avb_ab_data_read,
	.write_ab_metadata = fsl_avb_ab_data_write,
	.ops = NULL
};
#ifdef CONFIG_AVB_ATX
AvbAtxOps fsl_avb_atx_ops = {
	.ops = NULL,
	.read_permanent_attributes = fsl_read_permanent_attributes,
	.read_permanent_attributes_hash = fsl_read_permanent_attributes_hash,
#ifdef CONFIG_IMX_TRUSTY_OS
	.set_key_version = fsl_write_rollback_index_rpmb,
#else
	.set_key_version = fsl_set_key_version,
#endif
	.get_random = fsl_get_random
};
#endif
AvbOps fsl_avb_ops = {
	.ab_ops = &fsl_avb_ab_ops,
#ifdef CONFIG_AVB_ATX
	.atx_ops = &fsl_avb_atx_ops,
#endif
	.read_from_partition = fsl_read_from_partition_multi,
	.write_to_partition = fsl_write_to_partition,
#ifdef CONFIG_AVB_ATX
	.validate_vbmeta_public_key = avb_atx_validate_vbmeta_public_key,
#else
	.validate_vbmeta_public_key = fsl_validate_vbmeta_public_key_rpmb,
#endif
	.read_rollback_index = fsl_read_rollback_index_rpmb,
        .write_rollback_index = fsl_write_rollback_index_rpmb,
	.read_is_device_unlocked = fsl_read_is_device_unlocked,
	.get_unique_guid_for_partition = fsl_get_unique_guid_for_partition,
	.get_size_of_partition = fsl_get_size_of_partition
};
#endif

int get_block_size(void) {
        int dev_no = 0;
        struct blk_desc *dev_desc;

        dev_no = fastboot_devinfo.dev_id;
        dev_desc = blk_get_dev(fastboot_devinfo.type == DEV_SATA ? "scsi" : "mmc", dev_no);
        if (NULL == dev_desc) {
                printf("** Block device %s %d not supported\n",
                       fastboot_devinfo.type == DEV_SATA ? "scsi" : "mmc",
                       dev_no);
                return 0;
        }
        return dev_desc->blksz;
}

struct fastboot_device_info fastboot_devinfo = {0xff, 0xff};

#ifdef CONFIG_FLASH_MCUFIRMWARE_SUPPORT
struct fastboot_device_info fastboot_firmwareinfo;
#endif

/**
 * fastboot_none() - Skip the common write operation, nothing output.
 *
 * @response: Pointer to fastboot response buffer
 */
void fastboot_none_resp(char *response)
{
	*response = 0;
}

void board_fastboot_setup(void)
{
	static char boot_dev_part[32];
	u32 dev_no;

	switch (get_boot_device()) {
	case SD1_BOOT:
	case SD2_BOOT:
	case SD3_BOOT:
	case SD4_BOOT:
	case MMC1_BOOT:
	case MMC2_BOOT:
	case MMC3_BOOT:
	case MMC4_BOOT:
		dev_no = mmc_get_env_dev();
		sprintf(boot_dev_part,"mmc%d",dev_no);
		if (!env_get("fastboot_dev"))
			env_set("fastboot_dev", boot_dev_part);
		sprintf(boot_dev_part, "boota mmc%d", dev_no);
		if (!env_get("bootcmd"))
			env_set("bootcmd", boot_dev_part);
		break;
	case USB_BOOT:
		printf("Detect USB boot. Will enter fastboot mode!\n");
		if (!env_get("bootcmd"))
			env_set("bootcmd", "fastboot 0");
		break;
	default:
		if (!env_get("bootcmd"))
			printf("unsupported boot devices\n");
		break;
	}

	/* add soc type into bootargs */
	if (is_mx6dqp()) {
		if (!env_get("soc_type"))
			env_set("soc_type", "imx6qp");
	} else if (is_mx6dq()) {
		if (!env_get("soc_type"))
			env_set("soc_type", "imx6q");
	} else if (is_mx6sdl()) {
		if (!env_get("soc_type"))
			env_set("soc_type", "imx6dl");
	} else if (is_mx6sx()) {
		if (!env_get("soc_type"))
			env_set("soc_type", "imx6sx");
	} else if (is_mx6sl()) {
		if (!env_get("soc_type"))
			env_set("soc_type", "imx6sl");
	} else if (is_mx6ul()) {
		if (!env_get("soc_type"))
			env_set("soc_type", "imx6ul");
	} else if (is_mx7()) {
		if (!env_get("soc_type"))
			env_set("soc_type", "imx7d");
	} else if (is_mx7ulp()) {
		if (!env_get("soc_type"))
			env_set("soc_type", "imx7ulp");
	} else if (is_imx8qm()) {
		if (!env_get("soc_type"))
			env_set("soc_type", "imx8qm");
	} else if (is_imx8qxp()) {
		if (!env_get("soc_type"))
			env_set("soc_type", "imx8qxp");
	} else if (is_imx8mq()) {
		if (!env_get("soc_type"))
			env_set("soc_type", "imx8mq");
	} else if (is_imx8mm()) {
		if (!env_get("soc_type"))
			env_set("soc_type", "imx8mm");
	} else if (is_imx8mn()) {
		if (!env_get("soc_type"))
			env_set("soc_type", "imx8mn");
	} else if (is_imx8mp()) {
		if (!env_get("soc_type"))
			env_set("soc_type", "imx8mp");
	}
}

#ifdef CONFIG_ANDROID_RECOVERY
void board_recovery_setup(void)
{
/* boot from current mmc with avb verify */
#ifdef CONFIG_AVB_SUPPORT
	if (!env_get("bootcmd_android_recovery"))
		env_set("bootcmd_android_recovery", "boota recovery");
#else
	static char boot_dev_part[32];
	u32 dev_no;

	int bootdev = get_boot_device();
	switch (bootdev) {
	case SD1_BOOT:
	case SD2_BOOT:
	case SD3_BOOT:
	case SD4_BOOT:
	case MMC1_BOOT:
	case MMC2_BOOT:
	case MMC3_BOOT:
	case MMC4_BOOT:
		dev_no = mmc_get_env_dev();
		sprintf(boot_dev_part,"boota mmc%d recovery",dev_no);
		if (!env_get("bootcmd_android_recovery"))
			env_set("bootcmd_android_recovery", boot_dev_part);
		break;
	default:
		printf("Unsupported bootup device for recovery: dev: %d\n",
			bootdev);
		return;
	}
#endif /* CONFIG_AVB_SUPPORT */
	printf("setup env for recovery..\n");
	env_set("bootcmd", env_get("bootcmd_android_recovery"));
}
#endif /*CONFIG_ANDROID_RECOVERY*/

#ifdef CONFIG_IMX_TRUSTY_OS
#ifdef CONFIG_ARM64
void tee_setup(void)
{
	trusty_ipc_init();
}

#else
extern bool tos_flashed;

void tee_setup(void)
{
	/* load tee from boot1 of eMMC. */
	int mmcc = mmc_get_env_dev();
	struct blk_desc *dev_desc = NULL;

	struct mmc *mmc;
	mmc = find_mmc_device(mmcc);
	if (!mmc) {
	            printf("boota: cannot find '%d' mmc device\n", mmcc);
		            goto fail;
	}

	dev_desc = blk_get_dev("mmc", mmcc);
	if (NULL == dev_desc) {
	            printf("** Block device MMC %d not supported\n", mmcc);
		            goto fail;
	}

	/* below was i.MX mmc operation code */
	if (mmc_init(mmc)) {
	            printf("mmc%d init failed\n", mmcc);
		            goto fail;
	}

	struct fastboot_ptentry *tee_pte;
	char *tee_ptn = FASTBOOT_PARTITION_TEE;
	tee_pte = fastboot_flash_find_ptn(tee_ptn);
	mmc_switch_part(mmc, TEE_HWPARTITION_ID);
	if (!tee_pte) {
		printf("boota: cannot find tee partition!\n");
		fastboot_flash_dump_ptn();
	}

	if (blk_dread(dev_desc, tee_pte->start,
		    tee_pte->length, (void *)TRUSTY_OS_ENTRY) < 0) {
		printf("Failed to load tee.");
	}
	mmc_switch_part(mmc, FASTBOOT_MMC_USER_PARTITION_ID);

	tos_flashed = false;
	if(!valid_tos()) {
		printf("TOS not flashed! Will enter TOS recovery mode. Everything will be wiped!\n");
		fastboot_wipe_all();
		run_command("fastboot 0", 0);
		goto fail;
	}
#ifdef NON_SECURE_FASTBOOT
	armv7_init_nonsec();
	trusty_os_init();
	trusty_ipc_init();
#endif

fail:
	return;

}
#endif /* CONFIG_ARM64 */
#endif /* CONFIG_IMX_TRUSTY_OS */

static int _fastboot_setup_dev(int *switched)
{
	char *fastboot_env;
	struct fastboot_device_info devinfo;;
	fastboot_env = env_get("fastboot_dev");

	if (fastboot_env) {
		if (!strcmp(fastboot_env, "sata")) {
			devinfo.type = DEV_SATA;
			devinfo.dev_id = 0;
		} else if (!strncmp(fastboot_env, "mmc", 3)) {
			devinfo.type = DEV_MMC;
			if(env_get("target_ubootdev"))
				devinfo.dev_id = simple_strtoul(env_get("target_ubootdev"), NULL, 10);
			else
				devinfo.dev_id = mmc_get_env_dev();
		} else {
			return 1;
		}
	} else {
		return 1;
	}
#ifdef CONFIG_FLASH_MCUFIRMWARE_SUPPORT
	/* For imx7ulp, flash m4 images directly to spi nor-flash, M4 will
	 * run automatically after powered on. For imx8mq, flash m4 images to
	 * physical partition 'mcu_os', m4 will be kicked off by A core. */
	fastboot_firmwareinfo.type = ANDROID_MCU_FRIMWARE_DEV_TYPE;
#endif

	if (switched) {
		if (devinfo.type != fastboot_devinfo.type || devinfo.dev_id != fastboot_devinfo.dev_id)
			*switched = 1;
		else
			*switched = 0;
	}

	fastboot_devinfo.type	 = devinfo.type;
	fastboot_devinfo.dev_id = devinfo.dev_id;

	return 0;
}

void fastboot_setup(void)
{
	int sw, ret;
	struct tag_serialnr serialnr;
	char serial[17];

	get_board_serial(&serialnr);
	sprintf(serial, "%08x%08x", serialnr.high, serialnr.low);
	env_set("serial#", serial);

	/*execute board relevant initilizations for preparing fastboot */
	board_fastboot_setup();

	/*get the fastboot dev*/
	ret = _fastboot_setup_dev(&sw);

	/*load partitions information for the fastboot dev*/
	if (!ret && sw)
		fastboot_load_partitions();

	fastboot_init(NULL, 0);
#ifdef CONFIG_AVB_SUPPORT
	fsl_avb_ab_ops.ops = &fsl_avb_ops;
#ifdef CONFIG_AVB_ATX
	fsl_avb_atx_ops.ops = &fsl_avb_ops;
#endif
#endif
}

static void fastboot_putc(struct stdio_dev *dev, const char c)
{
	char buff[6] = "INFO";
	buff[4] = c;
	buff[5] = 0;
	fastboot_tx_write_more(buff);
}

#define FASTBOOT_MAX_LEN 64

static void fastboot_puts(struct stdio_dev *dev, const char *s)
{
	char buff[FASTBOOT_MAX_LEN + 1] = "INFO";
	int len = strlen(s);
	int i, left;

	for (i = 0; i < len; i += FASTBOOT_MAX_LEN - 4) {
		left = len - i;
		if (left > FASTBOOT_MAX_LEN - 4)
			left = FASTBOOT_MAX_LEN - 4;

		memcpy(buff + 4, s + i, left);
		buff[left + 4] = 0;
		fastboot_tx_write_more(buff);
	}
}

struct stdio_dev g_fastboot_stdio = {
	.name = "fastboot",
	.flags = DEV_FLAGS_OUTPUT,
	.putc = fastboot_putc,
	.puts = fastboot_puts,
};
