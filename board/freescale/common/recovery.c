/*
 * Freescale Android Recovery mode checking routing

 * Copyright (C) 2010 Freescale Semiconductor, Inc.
 *
 * This software may be used and distributed according to the
 * terms of the GNU Public License, Version 2, incorporated
 * herein by reference.
 *
 */
#include <common.h>
#include <malloc.h>
#include "recovery.h"
#ifdef CONFIG_MXC_KPD
#include <mxc_keyb.h>
#endif

extern int check_recovery_cmd_file(void);
extern enum boot_device get_boot_device(void);

#ifdef CONFIG_MXC_KPD

#define PRESSED_HOME	0x01
#define PRESSED_POWER	0x02
#define RECOVERY_KEY_MASK (PRESSED_HOME | PRESSED_POWER)

inline int test_key(int value, struct kpp_key_info *ki)
{
	return (ki->val == value) && (ki->evt == KDepress);
}

int check_key_pressing(void)
{
	struct kpp_key_info *key_info;
	int state = 0, keys, i;

	mxc_kpp_init();

	puts("Detecting HOME+POWER key for recovery ...\n");

	/* Check for home + power */
	keys = mxc_kpp_getc(&key_info);
	if (keys < 2)
		return 0;

	for (i = 0; i < keys; i++) {
		if (test_key(CONFIG_POWER_KEY, &key_info[i]))
			state |= PRESSED_HOME;
		else if (test_key(CONFIG_HOME_KEY, &key_info[i]))
			state |= PRESSED_POWER;
	}

	free(key_info);

	if ((state & RECOVERY_KEY_MASK) == RECOVERY_KEY_MASK)
		return 1;

	return 0;
}
#else
/* If not using mxc keypad, currently we will detect power key on board */
int check_key_pressing(void)
{
	return 0;
}
#endif

extern struct reco_envs supported_reco_envs[];

void setup_recovery_env(void)
{
	char *env, *boot_args, *boot_cmd;
	int bootdev = get_boot_device();

	boot_cmd = supported_reco_envs[bootdev].cmd;
	boot_args = supported_reco_envs[bootdev].args;

	if (boot_cmd == NULL) {
		printf("Unsupported bootup device for recovery\n");
		return;
	}

	printf("setup env for recovery..\n");

	env = getenv("bootargs_android_recovery");
	/* Set env to recovery mode */
	/* Only set recovery env when these env not exist, give user a
	 * chance to change their recovery env */
	if (!env)
		setenv("bootargs_android_recovery", boot_args);

	env = getenv("bootcmd_android_recovery");
	if (!env)
		setenv("bootcmd_android_recovery", boot_cmd);
	setenv("bootcmd", "run bootcmd_android_recovery");
}

/* export to lib_arm/board.c */
void check_recovery_mode(void)
{
	if (check_key_pressing())
		setup_recovery_env();
	else if (check_recovery_cmd_file()) {
		puts("Recovery command file founded!\n");
		setup_recovery_env();
	}
}
