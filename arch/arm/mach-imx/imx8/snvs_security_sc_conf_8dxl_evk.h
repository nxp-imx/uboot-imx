/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2020 NXP.
 */

#ifndef SNVS_SECURITY_SC_CONF_8DXL_EVK_H_
#define SNVS_SECURITY_SC_CONF_8DXL_EVK_H_

#include "snvs_security_sc_conf.h"

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

static __maybe_unused struct snvs_security_sc_conf snvs_passive_vcc_config = {
	.hp = {
		.lock = 0x1f0703ff,
		.secvio_intcfg = 0x8000002f,
		.secvio_ctl = 0xC000007f,
	},
	.lp = {
		.lock = 0x1f0003ff,
		.secvio_ctl = 0x36,
		.tamper_filt_cfg = 0,
		.tamper_det_cfg = 0x076, /* analogic tampers
					  *  + rollover tampers
					  */
		.tamper_det_cfg2 = 0x11, /* ET3 + ET7 will trig on line at GND*/
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

static __maybe_unused struct snvs_security_sc_conf snvs_passive_gnd_config = {
	.hp = {
		.lock = 0x1f0703ff,
		.secvio_intcfg = 0x8000002f,
		.secvio_ctl = 0xC000007f,
	},
	.lp = {
		.lock = 0x1f0003ff,
		.secvio_ctl = 0x36,
		.tamper_filt_cfg = 0,
		.tamper_det_cfg = 0x076, /* analogic tampers
					  *  + rollover tampers
					  */
		.tamper_det_cfg2 = 0x110011, /* ET3 + ET7 will trig on line at
					      * VCC
					      */
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

static __maybe_unused struct snvs_security_sc_conf snvs_active_config = {
	.hp = {
		.lock = 0x1f0703ff,
		.secvio_intcfg = 0x8000002f,
		.secvio_ctl = 0xC000007f,
	},
	.lp = {
		.lock = 0x1f0003ff,
		.secvio_ctl = 0x36,
		.tamper_filt_cfg = 0,
		.tamper_det_cfg = 0x076, /* analogic tampers
					  *  + rollover tampers
					  */
		.tamper_det_cfg2 = 0x10, /* Enable ET7 tamper */
		.tamper_filt1_cfg = 0,
		.tamper_filt2_cfg = 0x80, /* Enable filtering */
		.act_tamper1_cfg = 0x84001111,
		.act_tamper2_cfg = 0,
		.act_tamper3_cfg = 0,
		.act_tamper4_cfg = 0,
		.act_tamper5_cfg = 0,
		.act_tamper_ctl = 0x10001,
		.act_tamper_clk_ctl = 0,
		.act_tamper_routing_ctl1 = 0x1000000, /* Route AT1 to ET 7 */
		.act_tamper_routing_ctl2 = 0,
	}
};

static __maybe_unused struct snvs_dgo_conf snvs_dgo_passive_vcc_config = {
	.tamper_misc_ctl = 0x80000000, /* Lock the DGO */
	.tamper_pull_ctl = 0x00000044, /* Pull down IN4 and OUT0 */
	.tamper_ana_test_ctl = 0x20000000, /* Enable tamper */
};

static __maybe_unused struct snvs_dgo_conf snvs_dgo_passive_gnd_config = {
	.tamper_misc_ctl = 0x80000000, /* Lock the DGO */
	.tamper_pull_ctl = 0x00011044, /* Pull down IN4 and OUT0 */
	.tamper_ana_test_ctl = 0x20000000, /* Enable tamper */
};

static __maybe_unused struct snvs_dgo_conf snvs_dgo_active_config = {
	.tamper_misc_ctl = 0x80000000, /* Lock the DGO */
	.tamper_ana_test_ctl = 0x20000000, /* Enable tamper */
};

static struct tamper_pin_cfg tamper_pin_list_default_config[] = {
	{SC_P_SNVS_TAMPER_IN0, 0}, /* Tamp_In0 */
	{SC_P_SNVS_TAMPER_IN1, 0}, /* Tamp_In1 */
	{SC_P_SNVS_TAMPER_IN2, 0}, /* Tamp_In2 */
	{SC_P_SNVS_TAMPER_IN3, 0}, /* Tamp_In3 */
	{TAMPER_NO_IOMUX, 0}, /* Tamp_In4 */
	{TAMPER_NO_IOMUX, 0}, /* Tamp_Out0 */
	{SC_P_SNVS_TAMPER_OUT1, 0}, /* Tamp_Out1 */
	{SC_P_SNVS_TAMPER_OUT2, 0}, /* Tamp_Out2 */
	{SC_P_SNVS_TAMPER_OUT3, 0}, /* Tamp_Out3 */
	{SC_P_SNVS_TAMPER_OUT4, 0}, /* Tamp_Out4 */
};

static __maybe_unused struct tamper_pin_cfg tamper_pin_list_passive_vcc_config[] = {
};

static __maybe_unused struct tamper_pin_cfg tamper_pin_list_passive_gnd_config[] = {
};

static __maybe_unused struct tamper_pin_cfg tamper_pin_list_active_config[] = {
};

#endif /* SNVS_SECURITY_SC_CONF_8DXL_EVK_H_ */
