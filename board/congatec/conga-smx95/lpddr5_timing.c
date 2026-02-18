/*
 * LPDDR5 Timing Configuration
 * Conga-SMX95
 *
 * This file provides the DDR timing configuration
 * structure for i.MX95 LPDDR5 initialization.
 *
 * NOTE:
 * Actual timing values must be generated using
 * NXP DDR Tool for LPDDR5 and validated on hardware.
 */

#include <common.h>
#include <asm/arch/ddr.h>

/* Placeholder timing configuration structure */
struct dram_timing_info conga_smx95_lpddr5_timing = {
    .ddrc_cfg = NULL,
    .ddrc_cfg_num = 0,

    .ddrphy_cfg = NULL,
    .ddrphy_cfg_num = 0,

    .fsp_msg = NULL,
    .fsp_msg_num = 0,

    .ddrphy_trained_csr = NULL,
    .ddrphy_trained_csr_num = 0,

    .ddrphy_pie = NULL,
    .ddrphy_pie_num = 0,

    .fsp_table = NULL,
};
