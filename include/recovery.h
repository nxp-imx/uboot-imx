// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2010-2016 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2017 NXP
 */

#ifndef __RECOVERY_H_
#define __RECOVERY_H_

struct reco_envs {
	char *cmd;
	char *args;
};

void board_recovery_setup(void);

#endif
