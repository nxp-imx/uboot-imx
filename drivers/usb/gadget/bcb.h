/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef BCB_H
#define BCB_H
#include <linux/types.h>
#include <linux/stat.h>

#define FASTBOOT_BCB_CMD "bootonce-bootloader"
#ifdef CONFIG_ANDROID_RECOVERY
#define RECOVERY_BCB_CMD "boot-recovery"
#endif
/* keep same as bootable/recovery/bootloader.h */
struct bootloader_message {
	char command[32];
	char status[32];
	char recovery[768];

	/* The 'recovery' field used to be 1024 bytes. It has only ever
	 been used to store the recovery command line, so 768 bytes
	 should be plenty.  We carve off the last 256 bytes to store the
	 stage string (for multistage packages) and possible future
	 expansion. */
	char stage[32];

	/* The 'reserved' field used to be 224 bytes when it was initially
	 carved off from the 1024-byte recovery field. Bump it up to
	 1184-byte so that the entire bootloader_message struct rounds up
	 to 2048-byte.
	 */
	char reserved[1184];
};

struct bootloader_message_ab {
	struct bootloader_message message;
	char slot_suffix[32];

	/* Round up the entire struct to 4096-byte. */
	char reserved[2016];
};

/* start from bootloader_message_ab.slot_suffix[BOOTCTRL_IDX] */
#define BOOTCTRL_IDX                            0
#define MISC_COMMAND_IDX                        0
#define BOOTCTRL_OFFSET         \
	(u32)(&(((struct bootloader_message_ab *)0)->slot_suffix[BOOTCTRL_IDX]))
#define MISC_COMMAND \
	(u32)(&(((struct bootloader_message *)0)->command[MISC_COMMAND_IDX]))
int bcb_rw_block(bool bread, char **ppblock,
		uint *pblksize, char *pblock_write, uint offset, uint size);

int bcb_write_command(char *bcb_command);
int bcb_read_command(char *command);

#endif
