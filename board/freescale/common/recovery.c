/*
 * Freescale Android Recovery mode checking routing
 *
 * Copyright (C) 2010-2013 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */
#include <common.h>
#include <malloc.h>
#include <recovery.h>
#ifdef CONFIG_MXC_KPD
#include <mxc_keyb.h>
#endif

extern int check_recovery_cmd_file(void);
extern enum boot_device get_boot_device(void);

#ifdef CONFIG_MXC_KPD

#define PRESSED_VOL_DOWN	0x01
#define PRESSED_POWER	    0x02
#define RECOVERY_KEY_MASK (PRESSED_VOL_DOWN | PRESSED_POWER)

inline int test_key(int value, struct kpp_key_info *ki)
{
	return (ki->val == value) && (ki->evt == KDepress);
}

int check_key_pressing(void)
{
	struct kpp_key_info *key_info = NULL;
	int state = 0, keys, i;

	int ret = 0;

	mxc_kpp_init();
	/* due to glitch suppression circuit,
	   wait sometime to let all keys scanned. */
	udelay(1000);
	keys = mxc_kpp_getc(&key_info);

	printf("Detecting VOL_DOWN+POWER key for recovery(%d:%d) ...\n",
		keys, keys ? key_info->val : 0);
	if (keys > 1) {
		for (i = 0; i < keys; i++) {
			if (test_key(CONFIG_POWER_KEY, &key_info[i]))
				state |= PRESSED_POWER;
			else if (test_key(CONFIG_VOL_DOWN_KEY, &key_info[i]))
				state |= PRESSED_VOL_DOWN;
		}
	}
	if ((state & RECOVERY_KEY_MASK) == RECOVERY_KEY_MASK)
		ret = 1;
	if (key_info)
		free(key_info);
	return ret;
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
	char *env, *boot_cmd;
	int bootdev = get_boot_device();

	printf("recovery on bootdev: %d\n", bootdev);
	boot_cmd = supported_reco_envs[bootdev].cmd;

	if (boot_cmd == NULL) {
		printf("Unsupported bootup device for recovery: dev: %d\n", bootdev);
		return;
	}

	printf("setup env for recovery..\n");

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
