/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __SEC_MIPI_DSIM_H__
#define __SEC_MIPI_DSIM_H__

struct sec_mipi_dsim_plat_data {
	uint32_t version;
	uint32_t max_data_lanes;
	uint64_t max_data_rate;
	ulong reg_base;
	ulong gpr_base;
};

int sec_mipi_dsim_setup(const struct sec_mipi_dsim_plat_data *plat_data);

#endif
