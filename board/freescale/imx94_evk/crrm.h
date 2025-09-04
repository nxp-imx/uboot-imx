// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2025 NXP
 */

#include <init.h>
#include <asm/mach-imx/ele_api.h>

#if CONFIG_IMX_CRRM_PROTECT_SIZE > SZ_64M
#error "Can't protect entire NOR flash, re-configure CONFIG_IMX_CRRM_PROTECT_SIZE"
#endif

#define MAX_SND_OFF_VAL 10U
#define XSPI1_AHB_ADDR 0x28000000

struct crrm_info_struct {
	u32 protect_start;
	u32 protect_size;
	u32 install_off;
	u32 install_size;
	enum CRRM_BOOT_MODE bootmode;
	u8 luts_index;
};

struct crrm_awdt_refresh {
	u32 new_timer_val;
	u16 signature_length;
};

struct crrm_awdt_keylist {
	u16 key_type;
	u16 key_size;
};

struct crrm_awdt_init_conf {
	u32 timer_val;
	u32 safety_period;
	u32 max_timer_val;
	u16 verif_key_cnt;
	u16 timer_info_len;
};

struct crrm_status {
	u8 initialized;
	u8 running;
	u16 bbsm_running;
	u32 curr_timer_val;
	u32 safety_period;
	u32 max_timer_val;
	u32 bbsm_timer_val;
	u16 timer_info_len;
};

int crrm_spl_init(void);
int crrm_uboot_init(void);
int crrm_uboot_late_init(void);
