// SPDX-License-Identifier:     GPL-2.0+
/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 */

#ifndef BCB_H
#define BCB_H
#include <linux/types.h>
#include <linux/stat.h>
#include <android_bootloader_message.h>

#define FASTBOOT_BCB_CMD "bootonce-bootloader"
#ifdef CONFIG_ANDROID_RECOVERY
#define RECOVERY_BCB_CMD "boot-recovery"
#define RECOVERY_FASTBOOT_ARG "recovery\n--fastboot"
#endif

/* bcb struct is defined in include/android_bootloader_message.h */

/* start from bootloader_message_ab.slot_suffix[BOOTCTRL_IDX] */
#define BOOTCTRL_IDX                            0
#define MISC_COMMAND_IDX                        0
#define BOOTCTRL_OFFSET         \
	(u32)(&(((struct bootloader_message_ab *)0)->slot_suffix[BOOTCTRL_IDX]))
#define MISC_COMMAND \
	(u32)(uintptr_t)(&(((struct bootloader_message *)0)->command[MISC_COMMAND_IDX]))

#ifdef CONFIG_ANDROID_RECOVERY
#define RECOVERY_OPTIONS\
	(u32)(uintptr_t)(&(((struct bootloader_message *)0)->recovery[0]))
#endif
int bcb_rw_block(bool bread, char **ppblock,
		uint *pblksize, char *pblock_write, uint offset, uint size);

int bcb_write_command(char *bcb_command);
int bcb_read_command(char *command);

#ifdef CONFIG_ANDROID_RECOVERY
int bcb_write_recovery_opt(char *opts);
#endif
#endif
