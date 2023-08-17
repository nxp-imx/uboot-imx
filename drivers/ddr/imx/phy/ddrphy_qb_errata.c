// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 NXP
 */
#include <asm/arch/ddr.h>

/** Errata CSRs
 * STAR_3141216	0x020021 -> in ddrphy_csr_cfg
 * STAR_3256585	0x02000b -> in ddrphy_csr_cfg
 *		0x12000b -> in ddrphy_csr_cfg
 *		0x22000b -> in ddrphy_csr_cfg
 * STAR_3975199	0x0200c7
 *		0x0200ca
 *		0x1200c7
 *		0x1200ca
 *		0x2200c7
 *		0x2200ca
 * STAR_4101789	0x0200c7 -> covered by STAR_3975199
 *		0x0200c5 -> in ddrphy_csr_cfg
 *		0x1200c7 -> covered by STAR_3975199
 *		0x1200c5 -> in ddrphy_csr_cfg
 *		0x2200c7 -> covered by STAR_3975199
 *		0x2200c5 -> in ddrphy_csr_cfg
 */

/**
 * All from STAR_3975199, the remaining errata registers
 * are covered by either ddrphy_csr_cfg or STAR_3975199
 */
const uint32_t ddrphy_err_cfg[DDRPHY_QB_ERR_SIZE] = {
	0x000200c7,
	0x000200ca,
	0x001200c7,
	0x001200ca,
	0x002200c7,
	0x002200ca,
};
