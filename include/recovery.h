/*
 * Copyright (C) 2010-2014 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __RECOVERY_H_
#define __RECOVERY_H_

struct reco_envs {
	char *cmd;
	char *args;
};

void check_recovery_mode(void);
int recovery_check_and_clean_flag(void);
int check_recovery_cmd_file(void);
void board_recovery_setup(void);

#endif
