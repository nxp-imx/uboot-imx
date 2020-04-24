/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2020 NXP.
 */

#ifndef SNVS_SECURITY_SC_CONF_H_
#define SNVS_SECURITY_SC_CONF_H_

/*
 * File to list different example of tamper configuration:
 * - default
 * - passive to ground
 * - passive to vcc
 * - active
 *
 * for the different platform supported:
 * - imx8qxp-mek
 * - imx8qm-mek
 * - imx8dxl-evk
 */

#include <asm/arch-imx8/imx8-pins.h>

/* Definition of the structures */

struct snvs_security_sc_conf {
	struct snvs_hp_conf {
		u32 lock;		/* HPLR - HP Lock */
		u32 __cmd;		/* HPCOMR - HP Command */
		u32 __ctl;		/* HPCR - HP Control */
		u32 secvio_intcfg;	/* HPSICR - Security Violation Int
					 * Config
					 */
		u32 secvio_ctl;		/* HPSVCR - Security Violation Control*/
		u32 status;		/* HPSR - HP Status */
		u32 secvio_status;	/* HPSVSR - Security Violation Status */
		u32 __ha_counteriv;	/* High Assurance Counter IV */
		u32 __ha_counter;		/* High Assurance Counter */
		u32 __rtc_msb;		/* Real Time Clock/Counter MSB */
		u32 __rtc_lsb;		/* Real Time Counter LSB */
		u32 __time_alarm_msb;	/* Time Alarm MSB */
		u32 __time_alarm_lsb;	/* Time Alarm LSB */
	} hp;
	struct snvs_lp_conf {
		u32 lock;
		u32 __ctl;
		u32 __mstr_key_ctl;	/* Master Key Control */
		u32 secvio_ctl;		/* Security Violation Control */
		u32 tamper_filt_cfg;	/* Tamper Glitch Filters Configuration*/
		u32 tamper_det_cfg;	/* Tamper Detectors Configuration */
		u32 status;
		u32 __srtc_msb;		/* Secure Real Time Clock/Counter MSB */
		u32 __srtc_lsb;		/* Secure Real Time Clock/Counter LSB */
		u32 __time_alarm;		/* Time Alarm */
		u32 __smc_msb;		/* Secure Monotonic Counter MSB */
		u32 __smc_lsb;		/* Secure Monotonic Counter LSB */
		u32 __pwr_glitch_det;	/* Power Glitch Detector */
		u32 __gen_purpose;
		u8 __zmk[32];		/* Zeroizable Master Key */
		u32 __rsvd0;
		u32 __gen_purposes[4];	/* gp0_30 to gp0_33 */
		u32 tamper_det_cfg2;	/* Tamper Detectors Configuration2 */
		u32 tamper_det_status;	/* Tamper Detectors status */
		u32 tamper_filt1_cfg;	/* Tamper Glitch Filter1 Configuration*/
		u32 tamper_filt2_cfg;	/* Tamper Glitch Filter2 Configuration*/
		u32 __rsvd1[4];
		u32 act_tamper1_cfg;	/* Active Tamper1 Configuration */
		u32 act_tamper2_cfg;	/* Active Tamper2 Configuration */
		u32 act_tamper3_cfg;	/* Active Tamper3 Configuration */
		u32 act_tamper4_cfg;	/* Active Tamper4 Configuration */
		u32 act_tamper5_cfg;	/* Active Tamper5 Configuration */
		u32 __rsvd2[3];
		u32 act_tamper_ctl;	/* Active Tamper Control */
		u32 act_tamper_clk_ctl;	/* Active Tamper Clock Control */
		u32 act_tamper_routing_ctl1;/* Active Tamper Routing Control1 */
		u32 act_tamper_routing_ctl2;/* Active Tamper Routing Control2 */
	} lp;
};

struct snvs_dgo_conf {
	u32 tamper_offset_ctl;
	u32 tamper_pull_ctl;
	u32 tamper_ana_test_ctl;
	u32 tamper_sensor_trim_ctl;
	u32 tamper_misc_ctl;
	u32 tamper_core_volt_mon_ctl;
};

struct tamper_pin_cfg {
	u32 pad;
	u32 mux_conf;
};

#define TAMPER_NOT_DEFINED -1
#define TAMPER_NO_IOMUX TAMPER_NOT_DEFINED

/* There is 10 tampers and the list start at 1 */
enum EXT_TAMPER {
	EXT_TAMPER_ET1 = 0,
	EXT_TAMPER_ET2 = 1,
	EXT_TAMPER_ET3 = 2,
	EXT_TAMPER_ET4 = 3,
	EXT_TAMPER_ET5 = 4,
	EXT_TAMPER_ET6 = 5,
	EXT_TAMPER_ET7 = 6,
	EXT_TAMPER_ET8 = 7,
	EXT_TAMPER_ET9 = 8,
	EXT_TAMPER_ET10 = 9,
};

enum ACT_TAMPER {
	ACT_TAMPER_AT1 = EXT_TAMPER_ET6,
	ACT_TAMPER_AT2 = EXT_TAMPER_ET7,
	ACT_TAMPER_AT3 = EXT_TAMPER_ET8,
	ACT_TAMPER_AT4 = EXT_TAMPER_ET9,
	ACT_TAMPER_AT5 = EXT_TAMPER_ET10,
};

#endif /* SNVS_SECURITY_SC_CONF_H_ */
