// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2017 NXP
 *
 * Contains firmware in octet string format for SJA1105.
 */

#include <common.h>
#include <errno.h>
#include <sja1105_cfg.h>

extern struct sja1105_cfgs_s *sja1105_cfgs;

int sja1105_get_cfg(u32 devid, u32 cs, u32 *bin_len, u8 **cfg_bin)
{
	int i = 0;

	while (sja1105_cfgs[i].cfg_bin) {
		if (sja1105_cfgs[i].devid == devid &&
		    sja1105_cfgs[i].cs == cs) {
			*bin_len = sja1105_cfgs[i].bin_len;
			*cfg_bin = sja1105_cfgs[i].cfg_bin;
			return 0;
		}
		i++;
	}

	*bin_len = 0;
	*cfg_bin = NULL;

	printf("No matching device ID found for devid %X, cs %d.\n", devid, cs);

	return -EINVAL;
}
