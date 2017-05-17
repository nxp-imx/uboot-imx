/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <asm/arch/sci/sci.h>

struct smmu_sid {
	sc_rsrc_t rsrc;
	sc_rm_sid_t sid;
	char dev_name[32];
};

int imx8_config_smmu_sid(struct smmu_sid *dev_sids, int size);
