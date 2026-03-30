// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2018-2022 NXP
 */

#include <command.h>
#include <vsprintf.h>

#include <asm/arch/sys_proto.h>
#include <asm/io.h>
#include <env.h>
#include <linux/errno.h>
#include <mmc.h>
#include <stdbool.h>

static int check_mmc_autodetect(void) {
  char *autodetect_str = env_get("mmcautodetect");

  if (autodetect_str && !strcmp(autodetect_str, "yes"))
    return 1;

  return 0;
}

/* This should be defined for each board */
__weak int mmc_map_to_kernel_blk(int dev_no) { return dev_no; }

#if defined(CONFIG_IMX8_ROMAPI)
void board_check_boot_stage(void);
#endif

void board_late_mmc_env_init(void) {
  char cmd[32];
  char mmcblk[32];
  u32 dev_no = mmc_get_env_dev();

  if (check_mmc_autodetect()) {
    env_set_ulong("mmcdev", dev_no);

    /* Set mmcblk env */
    sprintf(mmcblk, "/dev/mmcblk%dp2 rootwait rw",
            mmc_map_to_kernel_blk(dev_no));
    env_set("mmcroot", mmcblk);

    sprintf(cmd, "mmc dev %d", dev_no);
    run_command(cmd, 0);
  }

#if defined(CONFIG_IMX8_ROMAPI) && !defined(CONFIG_SPL_BUILD)
  board_check_boot_stage();
#endif
}

#if defined(CONFIG_IMX8_ROMAPI)
static const char *boot_stage_name(u32 stage) {
  switch (stage) {
  case BT_STAGE_PRIMARY:
    return "primary";
  case BT_STAGE_SECONDARY:
    return "secondary";
  case BT_STAGE_RECOVERY:
    return "recovery";
  case BT_STAGE_USB:
    return "usb";
  default:
    return "unknown";
  }
}

void board_check_boot_stage(void) {
  int ret;
  u32 bstage;

  ret = rom_api_query_boot_infor(QUERY_BT_STAGE, &bstage);
  if (ret != ROM_API_OKAY) {
    printf("Failed to query boot stage (error: 0x%x)\n", ret);
    return;
  }

  printf("Boot Stage: %s (0x%x)\n", boot_stage_name(bstage), bstage);

  env_set("boot_stage", boot_stage_name(bstage));

  if (bstage == BT_STAGE_SECONDARY) {
    /* Set env variable to indicate secondary boot */
    env_set("corrupted_uboot", "yes");

#ifdef CONFIG_FIX_BOOT_PART
    /* Fix eMMC ESD register by manually switching boot partition */
    int dev_no = mmc_get_env_dev();
    struct mmc *mmc = find_mmc_device(dev_no);
    if (mmc) {
      mmc_init(mmc);
      /* Read current boot partition and toggle to the other one */
      u8 current_part = (mmc->part_config >> 3) & 0x7;
      u8 target_part = (current_part == 1) ? 2 : 1;
      printf("Toggling boot partition from %d to %d\n", current_part,
             target_part);
      mmc_set_part_conf(mmc, 0, target_part, 0);
    }
#endif
  }
}
#endif /* CONFIG_IMX8_ROMAPI */