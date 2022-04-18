/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2020 NXP.
 */

#ifndef SNVS_SECURITY_SC_CONF_BOARD_H_
#define SNVS_SECURITY_SC_CONF_BOARD_H_

#ifdef CONFIG_TARGET_IMX8QM_MEK
#include "snvs_security_sc_conf_8qm_mek.h"
#elif CONFIG_TARGET_IMX8QXP_MEK
#include "snvs_security_sc_conf_8qxp_mek.h"
#elif CONFIG_TARGET_IMX8DXL_EVK
#include "snvs_security_sc_conf_8dxl_evk.h"
#else

#include "snvs_security_sc_conf.h"

/* Default configuration of the tamper for all boards */
static __maybe_unused struct snvs_security_sc_conf snvs_default_config = {
	.hp = {
		.lock = 0x1f0703ff,
		.secvio_intcfg = 0x8000002f,
		.secvio_ctl = 0xC000007f,
	},
	.lp = {
		.lock = 0x1f0003ff,
		.secvio_ctl = 0x36,
		.tamper_filt_cfg = 0,
		.tamper_det_cfg = 0x76, /* analogic tampers
					 * + rollover tampers
					 */
		.tamper_det_cfg2 = 0,
		.tamper_filt1_cfg = 0,
		.tamper_filt2_cfg = 0,
		.act_tamper1_cfg = 0,
		.act_tamper2_cfg = 0,
		.act_tamper3_cfg = 0,
		.act_tamper4_cfg = 0,
		.act_tamper5_cfg = 0,
		.act_tamper_ctl = 0,
		.act_tamper_clk_ctl = 0,
		.act_tamper_routing_ctl1 = 0,
		.act_tamper_routing_ctl2 = 0,
	}
};

static __maybe_unused struct snvs_dgo_conf snvs_dgo_default_config = {
	.tamper_misc_ctl = 0x80000000, /* Lock the DGO */
};

static struct tamper_pin_cfg tamper_pin_list_default_config[] = {0};

#endif

#endif /* SNVS_SECURITY_SC_CONF_BOARD_H_ */
