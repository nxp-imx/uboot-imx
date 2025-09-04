// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2025 NXP
 */

#include <env.h>
#include <init.h>
#include <fdt_support.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <scmi_agent.h>
#include <../dts/imx94-power.h>
#include <../dts/imx94-clock.h>
#include <asm/arch/sys_proto.h>
#include <dm/uclass.h>
#include <dm/uclass-internal.h>
#include <fuse.h>
#include <spl.h>
#include <stdlib.h>
#include <display_options.h>
#include <env_internal.h>
#include "crrm.h"

struct crrm_info_struct crrm_info = {};

/* Follow ROM LUT order for first 3 seq: read, read status, write enable, so AHB memory can read */
u32 global_xspi_lut[2][80] = {
	{
		0x0820047c, 0x1f000f08, 0, 0, 0, /* Read data bytes (Octal Output SPI)*/
		0x1c000405, 0, 0, 0, 0, /* Read status register */
		0x00000406, 0, 0, 0, 0, /* Write enable*/
		0x47664766, 0, 0, 0, 0, /* soft reset */
		0x47994799, 0, 0, 0, 0, /* soft reset */
		0x1c00049f, 0, 0, 0, 0, /* read id */
		0x0818045a, 0x1c000c08, 0, 0, 0, /* Read SFDP*/
		0x1c000470, 0, 0, 0, 0, /* Read flag status register */
		0x082004dc, 0, 0, 0, 0, /* Erase Sector */
		0x00000404, 0, 0, 0, 0, /* Write disable */
		0x08200412, 0x00002000, 0, 0, 0, /* Page program (up to 256 bytes)*/
	},
	{
		0
	}
};

/* Below is test priv key working with p256_pub key. User need to generate them and update them.
 * u8 p256_priv[32] = {
 *	 0xfe, 0x14, 0xe8, 0x11, 0x59, 0x74, 0x4a, 0x75, 0x7f, 0x34,
 *	 0x79, 0x4e, 0xd2, 0x8b, 0x53, 0x39, 0x8a, 0x57, 0xc0, 0x6a,
 *	 0x9b, 0xd7, 0xa3, 0xf4, 0x3a, 0x3f, 0xc5, 0xa4, 0x78, 0x6e,
 *	 0x4b, 0x0d
 * };
*/

static u32 snd_imgset_off(u32 fuse_val)
{

    u32 offset = 0U;
    switch(fuse_val) {
        case 0U:
            offset = 1024U * 1024U * 4U;
            break;
        case 2U:
            offset = 1024U * 1024U;
            break;
        default:
            if(fuse_val < MAX_SND_OFF_VAL){
                offset = ((u32)1U << fuse_val) * 1024U * 1024U;
            }
            else {
                /* default is 4MB */
                offset = 64U * 1024U;
            }
            break;
    }

    return offset;
}
static void *crrm_alloc(size_t size)
{
#if IS_ENABLED(CONFIG_XPL_BUILD)
	return memalign(4, size);
#else
	return (void *)(CONFIG_TEXT_BASE + SZ_2M - ALIGN(size, 4));
#endif
}

static void crrm_free(void *p)
{
#if IS_ENABLED(CONFIG_XPL_BUILD)
	free(p);
#endif
}

static inline bool crrm_validate_bm(u32 bm)
{
	if (bm == CRRM_NORMAL || bm == CRRM_RECOVERY_DOWNLOAD || bm == CRRM_RECOVERY_INSTALL)
		return true;

	return false;
}

static int crrm_get_lock_region(u32 *start, u32 *size, bool install)
{
	rom_passover_t pass_over;
	int ret;
	u32 val;

	if (!start || !size)
		return -EINVAL;

	ret = scmi_get_rom_data(&pass_over);
	if (ret) {
		printf("Fail to get ROM passover\n");
		return -EPERM;
	}

	/* Check secondary fuse */
	ret = fuse_read(4, 0, &val);
	if (ret) {
		printf("Fail to read fuse\n");
		return -EPERM;
	}

	val = snd_imgset_off((val >> 8) & 0xff);

	/* Can't protect entire flash, must have one region for open,
	 * 0x10000 is the min size, we reserve 0x20000 for env
	 */
	if (CONFIG_IMX_CRRM_PROTECT_SIZE <= val &&
		val + CONFIG_IMX_CRRM_PROTECT_SIZE <= SZ_64M - 0x20000) {

		*start = XSPI1_AHB_ADDR + pass_over.img_ofs;
		*start &= 0xffff0000;

		if (!install) {
			/* protect both two containers */
			*size = val + CONFIG_IMX_CRRM_PROTECT_SIZE;
		} else {
			/* protect active container */
			*size = CONFIG_IMX_CRRM_PROTECT_SIZE;
		}
	} else {
		if (!install) {
			/* Only one container is avaliable due to flash size */
			*start = XSPI1_AHB_ADDR;
			*size = CONFIG_IMX_CRRM_PROTECT_SIZE;
		} else {
			*start = 0;
			*size = 0;
		}
	}

	debug("Protect from 0x%x, size 0x%x, container set %u\n",
		*start, *size, pass_over.img_set_sel);

	return 0;
}

int crrm_get_status(u8 timer_id, bool *timer_running)
{
	void *status_buf;
	u32 status_size = sizeof(struct crrm_status) + 0x20;
	u32 resp;
	int ret;
	struct crrm_status *p;

	status_buf = crrm_alloc(status_size);
	if (!status_buf) {
		printf("Fail to allocate status buffer size 0x%x\n", status_size);
		return -ENOMEM;
	}

	flush_dcache_range((ulong)status_buf, (ulong)status_buf + status_size);
	ret = ele_crrm_get_status(timer_id, (ulong)status_buf, &status_size, &resp);
	if (ret) {
		printf("ELE get status failed %d, resp 0x%x\n", ret, resp);
		crrm_free(status_buf);
		return ret;
	} else {
		printf("ELE get status successfully\n");
	}

	p = (struct crrm_status *)status_buf;

	if (timer_running)
		*timer_running = p->running;

	printf("initialized %u, running %u, bbsm_running %u\n", p->initialized, p->running, p->bbsm_running);
	printf("curr_timer_val %u, safety_period %u, max_timer_val %u, bbsm_timer_val %u\n",
		p->curr_timer_val, p->safety_period, p->max_timer_val, p->bbsm_timer_val);
	printf("timer_info_len %u\n", p->timer_info_len);

	crrm_free(status_buf);

	return ret;
}

int xspi_install_lut_seqid(u32 *install_lut, u32 lut_num)
{
	int i, j;
	u8 table_index = 0;
	u32 *p = (u32 *)&global_xspi_lut;
	int ret;
	u32 resp;

	if (lut_num < 1 || lut_num > 5)
		return -EINVAL;

	for (i = 0; i < 160; i += 5) {
		for (j = 0; j < lut_num; j++) {
			if (p[i + j] != install_lut[j])
				break;
		}

		if (j == lut_num) {
			/* Find the lut */
			table_index = i / 80;
			i = i % 80;

			/* if not in current table, switch table */
			if (table_index != crrm_info.luts_index) {
				printf("change to lut index %u\n", table_index);
				ret = ele_crrm_change_lut(1, table_index, &resp);
				if (ret) {
					printf("Change LUT failed 0x%x\n", resp);
					return ret;
				}

				crrm_info.luts_index = table_index;
			}

			return i / 5; /* return seq id */
		}
	}

	return -ENOENT;
}

u32 xspi_adjust_cmd_sfar(u32 sfar_addr)
{

	if (sfar_addr >= crrm_info.protect_start
		&& sfar_addr < crrm_info.protect_start + crrm_info.protect_size) {
		/* adjust sfar with non protected area */
		if (crrm_info.protect_start > XSPI1_AHB_ADDR)
			sfar_addr = XSPI1_AHB_ADDR;
		else
			sfar_addr = crrm_info.protect_start + crrm_info.protect_size;
	}

	return sfar_addr;
}

#if IS_ENABLED(CONFIG_XPL_BUILD)
u8 p256_pub[] = {
	0x98, 0x49, 0x46, 0x6d, 0x79, 0x91, 0xca, 0x40, 0x8d, 0x47, 0x1a, 0xfb, 0x3c, 0x9b,
	0x65, 0xa9, 0xf9, 0x0d, 0x02, 0xa1, 0x2a, 0x82, 0xf6, 0x3e, 0x2c, 0x97, 0x9a, 0x84,
	0x59, 0x6c, 0x0c, 0xdf, 0x95, 0xde, 0x99, 0x51, 0x15, 0xcf, 0xe7, 0x4b, 0x29, 0x21,
	0x79, 0xe7, 0xfc, 0xf9, 0x7c, 0x5c, 0x3e, 0x3c, 0x26, 0x19, 0x53, 0x46, 0x7a, 0xc2,
	0x35, 0x95, 0x76, 0xc6, 0x8c, 0x0b, 0x18, 0x97
};

static int crrm_lock_region(bool install)
{
	int ret;
	u32 resp;

	ret = crrm_get_lock_region(&crrm_info.protect_start,
		&crrm_info.protect_size, install);
	if (ret) {
		printf("fail to get lock region %d\n", ret);
		return ret;
	}

	/* No region to protect */
	if (crrm_info.protect_start == 0 && crrm_info.protect_size == 0)
		return 0;

	ret = ele_crrm_protect_image(1, crrm_info.protect_start,
		crrm_info.protect_size, &resp);
	if (ret) {
		printf("Fail to protect image, start 0x%x, length 0x%x, resp 0x%x\n",
			crrm_info.protect_start, crrm_info.protect_size, resp);
		return -EPERM;
	}

	return 0;
}

static int crrm_init_xspi_lut(void)
{
	u32 resp;
	u32 *p = (u32 *)&global_xspi_lut;
	int ret;

	flush_dcache_range((ulong)p, (ulong)p + 80 * sizeof(u32));
	ret = ele_crrm_set_luts(1, 2, (ulong)p, 80 * sizeof(u32), &resp);
	if (ret) {
		printf("Set LUTs failed 0x%x\n", resp);
		return ret;
	}

	/* Select first LUT table */
	ret = ele_crrm_change_lut(1, 0, &resp);
	if (ret) {
		printf("Change LUT failed 0x%x\n", resp);
		return ret;
	}

	crrm_info.luts_index = 0;

	return 0;
}

static int crrm_init_awdt(u8 timer_id, u8 op, u8 *pubkey, u32 pubkey_size, u8 *timerinfo, u32 timerinfo_size)
{
	struct crrm_awdt_init_conf conf;
	struct crrm_awdt_keylist keylist;
	void *conf_buf;
	u8 *p;
	u32 conf_buf_size, resp;
	int ret;

	conf.timer_val = CONFIG_IMX_CRRM_AWDT_TIMEOUT;
#if IS_ENABLED(CONFIG_IMX_CRRM_SELFTEST)
	conf.timer_val = 600000; /* 600s */
#endif
	conf.safety_period = 1000; /* 1s */
	conf.verif_key_cnt = 1;
	conf.max_timer_val = 4000000; /* 4000 s */

	if (!timerinfo || !timerinfo_size)
		conf.timer_info_len = 0;
	else
		conf.timer_info_len = timerinfo_size;

	keylist.key_size = pubkey_size;
	keylist.key_type = 0x4112;

	conf_buf_size = sizeof(struct crrm_awdt_init_conf) +
		sizeof(struct crrm_awdt_keylist) +
		pubkey_size +
		timerinfo_size;

	conf_buf = memalign(4, conf_buf_size);
	if (!conf_buf) {
		printf("Fail to allocate configuration buffer size 0x%x\n", conf_buf_size);
		return -ENOMEM;
	}

	p = conf_buf;
	memcpy(p, &conf, sizeof(struct crrm_awdt_init_conf));
	p += sizeof(struct crrm_awdt_init_conf);

	memcpy(p, &keylist, sizeof(struct crrm_awdt_keylist));
	p += sizeof(struct crrm_awdt_keylist);

	memcpy(p, pubkey, pubkey_size);
	p += pubkey_size;

	if (timerinfo)
		memcpy(p, timerinfo, timerinfo_size);

#ifdef DEBUG
	printf("conf_buf 0x%lx, size 0x%x\n", (ulong)conf_buf, conf_buf_size);
	print_buffer(0, conf_buf, 4, conf_buf_size / 4 + 1, 4);
#endif

	flush_dcache_range((ulong)conf_buf, (ulong)conf_buf + conf_buf_size);

	/* Initialize configuration done, call ELE API */
	ret = ele_crrm_init_awdt(timer_id, op, (ulong)conf_buf, conf_buf_size, &resp);
	if (ret) {
		printf("ELE init AWDT failed %d, resp 0x%x\n", ret, resp);
	} else {
		printf("ELE init AWDT successfully\n");
	}

	free(conf_buf);

	return ret;
}

int crrm_spl_init(void)
{
	int ret;
	u32 resp, gpr_bm;
	u8 action, timer_id, operation;
	bool run;

	/* Initialize CRRM */
	ret = ele_crrm_init(&action, &resp);
	if (ret) {
		ret = scmi_get_bbnsm_gpr(4, &gpr_bm);
		if (ret) {
			printf("Get BBM GPR failed %d\n", ret);
			return ret;
		}

		if (!crrm_validate_bm(gpr_bm)) {
			printf("CRRM Init failed 0x%x, Please check fuse\n", resp);
			return -EPERM;
		}

		crrm_info.bootmode = gpr_bm;

		printf("CRRM: Bootmode 0x%x\n", crrm_info.bootmode);

		ret = crrm_get_lock_region(&crrm_info.protect_start,
			&crrm_info.protect_size, crrm_info.bootmode == CRRM_RECOVERY_INSTALL);
		if (ret) {
			printf("fail to get lock region %d\n", ret);
			return ret;
		}

		/* If CRRM init failed and GPR has valid BM, treat it as warm domain reset, call get status */
		ret = crrm_get_status(0, NULL);
		if (ret) {
			printf("CRRM get status failed %d\n", ret);
			return ret;
		}

		return 0;
	}

	if (action == 0xa5) {
		/* No action, get boot mode */
		printf("CRRM: BBSM Ready\n");

	} else if (action == 0xc3) {
		/* BBSM reinited */
		printf("CRRM: BBSM Initialized\n");
	}

	ret = ele_crrm_get_boot_mode(&crrm_info.bootmode, &timer_id, &resp);
	if (ret) {
		printf("CRRM get boot mode failed 0x%x\n", resp);
		return ret;
	}

	printf("CRRM: Bootmode 0x%x\n", crrm_info.bootmode);
	if (timer_id == 0xff)
		printf("CRRM: No timer expired\n");
	else
		printf("CRRM: Expired timer id %u\n", timer_id);

	/* pass boot mode to u-boot by calling SM BBNSM GPR set, 4-7 are avaliable */
	ret = scmi_set_bbnsm_gpr(4, crrm_info.bootmode);
	if (ret) {
		printf("Set BBM GPR failed %d\n", ret);
		return ret;
	}

	/* Lock region */
	ret = crrm_lock_region(crrm_info.bootmode == CRRM_RECOVERY_INSTALL);
	if (ret) {
		printf("Lock CRRM Image region failed %d\n", ret);
		return ret;
	}

	/* Set LUT table */
	ret = crrm_init_xspi_lut();
	if (ret) {
		printf("Configure LUT failed %d\n", ret);
		return ret;
	}

	if (crrm_info.bootmode == CRRM_NORMAL) {
		operation = 1; /* init */
	} else {
		operation = 3; /* stop */
		ret = crrm_get_status(0, &run);
		if (ret) {
			printf("CRRM get status failed %d\n", ret);
			return ret;
		}

		/* if timer is already stopped, nothing to do */
		if (!run)
			return 0;
	}

	/* Configure AWDT */
	char *test_info = "AWDT timer";
	ret = crrm_init_awdt(0, operation, p256_pub, ARRAY_SIZE(p256_pub), (u8 *)test_info, strlen(test_info) + 1);
	if (ret) {
		printf("CRRM init awdt failed %d\n", ret);
		return ret;
	}

#if IS_ENABLED(CONFIG_IMX_CRRM_SELFTEST)
	int i = 2;
	while (i > 0) {
		ret = crrm_get_status(0, NULL);
		if (ret)
			break;

		mdelay(1000);
		i--;
	}
#endif

	return 0;
}

void arch_spl_load_configure(struct spl_image_info *spl_image)
{
	if (crrm_info.bootmode != CRRM_NORMAL)
			spl_image->recovery = true;

	return;
}

#else
extern int do_board_reboot(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[]);

static int crrm_get_install_region(u32 *start, u32 *size)
{
	rom_passover_t pass_over;
	int ret;
	u32 val;

	if (!start || !size)
		return -EINVAL;

	ret = scmi_get_rom_data(&pass_over);
	if (ret) {
		printf("Fail to get ROM passover\n");
		return -EPERM;
	}

	/* Check secondary fuse */
	ret = fuse_read(4, 0, &val);
	if (ret) {
		printf("Fail to read fuse\n");
		return -EPERM;
	}

	val = snd_imgset_off((val >> 8) & 0xff);

	if (pass_over.img_ofs > CONFIG_IMX_CRRM_PROTECT_SIZE || val < CONFIG_IMX_CRRM_PROTECT_SIZE)
		*start = 0;
	else
		*start = val;

	*size = CONFIG_IMX_CRRM_PROTECT_SIZE;

	debug("Install from 0x%x, size 0x%x, container set %u\n",
		*start, *size, pass_over.img_set_sel);

	return 0;
}

int crrm_uboot_init(void)
{
	int ret;
	bool install = false;
	ret = scmi_get_bbnsm_gpr(4, &crrm_info.bootmode);
	if (ret) {
		printf("Get BBM GPR failed %d\n", ret);
		return ret;
	}

	if (crrm_info.bootmode == CRRM_RECOVERY_INSTALL)
		install = true;

	ret = crrm_get_lock_region(&crrm_info.protect_start,
		&crrm_info.protect_size, install);
	if (ret) {
		printf("Fail to get lock region, %d\n", ret);
		return ret;
	}

	if (install) {
		ret = crrm_get_install_region(&crrm_info.install_off,
			&crrm_info.install_size);
		if (ret) {
			printf("Fail to get install region, %d\n", ret);
			return ret;
		}
	}

	printf("CRRM:  Bootmode ");
	if (crrm_info.bootmode == CRRM_NORMAL) {
		printf("Normal\n");
	} else if (crrm_info.bootmode == CRRM_RECOVERY_DOWNLOAD) {
		printf("Recovery Download\n");
	} else if (crrm_info.bootmode == CRRM_RECOVERY_INSTALL) {
		printf("Recovery Install\n");
	} else {
		printf("Invalid (0x%x)\n", crrm_info.bootmode);
	}

	return 0;
}

int crrm_uboot_late_init(void)
{
	if (crrm_info.bootmode == CRRM_RECOVERY_DOWNLOAD) {
		/* Update env for download mode */
		if (IS_ENABLED(CONFIG_IMX_CRRM_SELFTEST)) {
			env_set("bootcmd", "run crrm_download; crrm_next");
		} else {
			env_set("bootcmd", "run crrm_recovery_args; booti ${loadaddr} ${initrd_addr} ${fdt_addr_r}");
		}
	} else if (crrm_info.bootmode == CRRM_RECOVERY_INSTALL) {
		u32 size, resp;
		char install_cmd[128];
		int ret;
		enum CRRM_BOOT_MODE next_mode = CRRM_RECOVERY_DOWNLOAD;

		/* Execute installing flash_install.bin from eMMC FAT partition */
		sprintf(install_cmd, "mmc dev ${mmcdev}");
		ret = run_command(install_cmd, 0);
		if (ret){
			printf("select mmc dev failed ret %d\n", ret);
			goto set_mode;
		}

		sprintf(install_cmd, "fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${crrm_file}");
		ret = run_command(install_cmd, 0);
		if (ret){
			printf("Fatload image failed ret %d\n", ret);
			goto set_mode;
		}

		size = env_get_hex("filesize", 0);
		if (size > crrm_info.install_size) {
			printf("Fail to install the image, the size 0x%x has exceeded install size 0x%x\n",
				size, crrm_info.install_size);
			ret = -EPERM;
			goto set_mode;
		}

		if (size  == 0) {
			printf("Fail to load the image, size is 0\n");
			ret = -ENODATA;
			goto set_mode;
		}

		sprintf(install_cmd, "sf probe");
		ret = run_command(install_cmd, 0);
		if (ret){
			printf("Probe sf failed ret %d\n", ret);
			goto set_mode;
		}
		/* Erase */
		sprintf(install_cmd, "sf erase 0x%x +${filesize}", crrm_info.install_off);
		ret = run_command(install_cmd, 0);
		if (ret) {
			printf("Erasing sf failed ret %d\n", ret);
			goto set_mode;
		}

		/* Write image */
		sprintf(install_cmd, "sf write ${loadaddr} 0x%x ${filesize}",
				crrm_info.install_off);
		ret = run_command(install_cmd, 0);
		if (ret){
			printf("Writing sf failed ret %d\n", ret);
			goto set_mode;
		}

		printf("sf write finished, Recovery Install done, reset to Normal\n");
		next_mode = CRRM_NORMAL;

set_mode:
		ret = ele_crrm_set_boot_mode(&next_mode, &resp);
		if (ret) {
			printf("CRRM set boot mode failed 0x%x\n", resp);
			return ret;
		}

		/* reboot*/
		printf("reboot...\n");
		do_board_reboot(NULL, 0, 0, NULL);
	}

	return 0;
}

enum env_location env_get_location(enum env_operation op, int prio)
{
	int ret;
	u32 bm;

	if (prio)
		return ENVL_UNKNOWN;

	ret = scmi_get_bbnsm_gpr(4, &bm);
	if (!ret) {
		if (bm == CRRM_RECOVERY_DOWNLOAD || bm == CRRM_RECOVERY_INSTALL)
			return ENVL_NOWHERE;
	}

	return arch_env_get_location(op, prio);
}

static int do_crrm_status(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	int ret;

	printf("Boot mode: 0x%x\n", crrm_info.bootmode);
	printf("Protect region: 0x%x, 0x%x\n", crrm_info.protect_start, crrm_info.protect_size);
	printf("Install offset: 0x%x, 0x%x\n", crrm_info.install_off, crrm_info.install_size);
	printf("LUT select: 0x%x\n", crrm_info.luts_index);
	printf("\n");

	ret = crrm_get_status(0, NULL);
	if (ret) {
		printf("CRRM get status failed %d\n", ret);
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(crrm_status, CONFIG_SYS_MAXARGS, 1, do_crrm_status,
	   "display CRRM status",
	   ""
);

#if IS_ENABLED(CONFIG_IMX_CRRM_SELFTEST)
u32 p256_sign[] = {
	0x21024530, 0x7c5dd600, 0x7be88677, 0x781fe47e,
	0x65e535a1, 0xdc0a9ba4, 0xf360e0a6, 0x2b94a210,
	0x5024ca19, 0x302002e5, 0x82bde80f, 0x599ae9f1,
	0x2ffa7814, 0xfe2565be, 0x618a2019, 0x03970cb9,
	0x862b5413, 0x00137060
};

/* Fill a fake signature for test purpose */
static void *p256_gen_signature(void *data_buf, u32 data_length, u32 *signature_length)
{
	void *buf;

	buf = memalign(4, sizeof(p256_sign) - 1);
	if (!buf) {
		printf("Failed to allocate memory\n");
		return NULL;
	}

	memcpy(buf, p256_sign, sizeof(p256_sign) - 1);
	*signature_length = 64; /* length is fixed to 64 */

	return buf;
}

static int crrm_awdt_refresh(u8 timer_id, u8 pub_key_index, u32 new_timer)
{
	void *buf, *signature;
	u32 size = 4 * 2 + 0x20 + 0x20; /* signed data: pubkey+timer_id, new_timer, nonce, timerinfo */
	u32 nonce_size = size - 8;
	u32 nonce, resp, signature_len;
	int ret;
	char *p;

	buf = crrm_alloc(size);
	if (!buf) {
		printf("Fail to allocate status buffer size 0x%x\n", size);
		return -ENOMEM;
	}

	flush_dcache_range((ulong)buf, (ulong)buf + size);
	ret = ele_crrm_get_nonce(timer_id, (ulong)buf + 8, &nonce_size, &resp);
	if (ret) {
		printf("ELE get nonce failed %d, resp 0x%x\n", ret, resp);
		crrm_free(buf);
		return ret;
	} else {
		printf("ELE get nonce successfully, nonce_size %u\n", nonce_size);
	}

	nonce = *(u32 *)(buf + 8);
	p = buf + 8 + 0x20;

	*(u32 *)buf = pub_key_index | timer_id;
	*(u32 *)(buf + 4) = new_timer;

	/* generate signature */
	signature = p256_gen_signature(buf, nonce_size + 8, &signature_len);
	crrm_free(buf);
	if (!signature) {
		printf("Fail to generate signature\n");
		return -ENODATA;
	}

	/* setup signature buffer for refresh */
	size = 6 + signature_len;
	buf = crrm_alloc(size);
	if (!buf) {
		free(signature);
		printf("Fail to allocate signature buffer size 0x%x\n", size);
		return -ENOMEM;
	}

	struct crrm_awdt_refresh *temp = buf;
	temp->new_timer_val = new_timer;
	temp->signature_length = signature_len;

	memcpy(buf + 6, signature, signature_len);
#ifdef DEBUG
	printf("buf 0x%lx len 0x%x\n", (ulong)buf, size);
	print_buffer(0, buf, 4, size / 4 + 1, 4);
#endif

	flush_dcache_range((ulong)buf, (ulong)buf + size);
	ret = ele_crrm_refresh_awdt(timer_id, pub_key_index, (ulong)buf, size, &resp);
	if (ret) {
		printf("refresh awdt failed resp 0x%x\n", resp);
	} else {
		printf("refresh awdt successfully, resp 0x%x\n", resp);
	}

	crrm_free(buf);
	free(signature);

	return ret;
}

static int do_crrm_refresh(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	int ret;
	ulong new_timeout;

	if (argc < 2)
		return CMD_RET_USAGE;

	new_timeout = simple_strtoul(argv[1], NULL, 10);

	ret = crrm_awdt_refresh(0, 0, new_timeout);
	if (ret) {
		printf("CRRM refresh AWDT timer failed %d\n", ret);
		return CMD_RET_FAILURE;
	}

	printf("Refresh AWDT timer with %lu\n", new_timeout);

	return CMD_RET_SUCCESS;
}

static int do_crrm_nextmode(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	int ret;
	u32 resp;
	enum CRRM_BOOT_MODE next_mode;

	if (argc > 2)
		return CMD_RET_USAGE;

	if (argc == 2) {
		if (!strcmp(argv[1], "install"))
			next_mode = CRRM_RECOVERY_INSTALL;
		else if (!strcmp(argv[1], "download"))
			next_mode = CRRM_RECOVERY_DOWNLOAD;
		else if (!strcmp(argv[1], "normal"))
			next_mode = CRRM_NORMAL;
		else
			return CMD_RET_USAGE;
	}

	if (crrm_info.bootmode == CRRM_NORMAL) {
		printf("Current boot mode is Normal, can't move to any\n");
		return CMD_RET_FAILURE;
	} else if (crrm_info.bootmode == CRRM_RECOVERY_DOWNLOAD) {
		printf("Current boot mode is Recovery Download,");
		if (argc == 2 && next_mode != CRRM_RECOVERY_INSTALL) {
			printf("can't move to %s\n", argv[1]);
			return CMD_RET_USAGE;
		}

		printf("move to Install\n");
		next_mode = CRRM_RECOVERY_INSTALL;
	} else {
		printf("Current boot mode is Recovery Install");
		if (argc == 2) {
			if (next_mode != CRRM_NORMAL && next_mode != CRRM_RECOVERY_DOWNLOAD) {
				printf("can't move to %s\n", argv[1]);
				return CMD_RET_USAGE;
			}
			printf("move to %s\n", argv[1]);
		} else {
			printf("move to Normal\n");
			next_mode = CRRM_NORMAL;
		}
	}

	ret = ele_crrm_set_boot_mode(&next_mode, &resp);
	if (ret) {
		printf("CRRM set boot mode failed 0x%x\n", resp);
		return CMD_RET_FAILURE;
	}

	/* reboot*/
	printf("reboot...\n");
	do_board_reboot(NULL, 0, 0, NULL);
	while (1) {}

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(crrm_refresh, CONFIG_SYS_MAXARGS, 1, do_crrm_refresh,
	   "refresh CRRM AWDT timer",
		"<timeout>\n"
		"   - new timeout value used by refresh\n"
);

U_BOOT_CMD(crrm_next, CONFIG_SYS_MAXARGS, 1, do_crrm_nextmode,
		"Move CRRM boot mode to next mode",
		"[<next_mode>]\n"
		"   - optional, set CRRM mode to next_mode: normal, download, install\n"
		"     When the next_mode is not set, CRRM move to next automatically\n"
);

#endif
#endif
