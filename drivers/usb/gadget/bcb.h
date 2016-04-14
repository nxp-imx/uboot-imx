/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef BCB_H
#define BCB_H
#include <linux/types.h>
#include <linux/stat.h>
/* keep same as bootable/recovery/bootloader.h */
struct bootloader_message {
	char command[32];
	char status[32];
	char recovery[768];

	/* The 'recovery' field used to be 1024 bytes.  It has only ever
	been used to store the recovery command line, so 768 bytes
	should be plenty.  We carve off the last 256 bytes to store the
	stage string (for multistage packages) and possible future
	expansion. */
	char stage[32];
	char slot_suffix[32];
	char reserved[192];
};

/* start from bootloader_message.slot_suffix[BOOTCTRL_IDX] */
#define BOOTCTRL_IDX                            0
#define MISC_COMMAND_IDX                        0
#define BOOTCTRL_OFFSET         \
	(u32)(&(((struct bootloader_message *)0)->slot_suffix[BOOTCTRL_IDX]))
#define MISC_COMMAND \
	(u32)(&(((struct bootloader_message *)0)->command[MISC_COMMAND_IDX]))
int rw_block(bool bread, char **ppblock,
		uint *pblksize, char *pblock_write, uint offset, uint size);

void set_mmc_id(unsigned int id);
#endif
