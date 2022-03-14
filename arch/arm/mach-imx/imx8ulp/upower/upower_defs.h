/* SPDX-License-Identifier: BSD-3-Clause */
/* +FHDR------------------------------------------------------------------------
 * Copyright 2019-2021 NXP
 * -----------------------------------------------------------------------------
 * FILE NAME      : upower_defs.h
 * DEPARTMENT     : BSTC - Campinas, Brazil
 * AUTHOR         : Celso Brites
 * AUTHOR'S EMAIL : celso.brites@nxp.com
 * -----------------------------------------------------------------------------
 * RELEASE HISTORY
 * VERSION DATE        AUTHOR                  DESCRIPTION
 *
 * $Log: upower_defs.h.rca $
 *
 *  Revision: 1.66 Tue Apr 27 12:48:48 2021 nxa11511
 *  Adds new pwm function number UPWR_PWM_REGCFG for new service upwr_pwm_reg_config
 *  (same value as UPWR_PWM_DEVMOD, deprecated).
 *  Adds struct upwr_reg_config_t, only a stub for now.
 *  Replaces typedef upwr_pwm_devmode_msg with upwr_pwm_regcfg_msg.
 *  upwr_pwm_msg.devmode replaced with upwr_pwm_msg.regcfg
 *
 *  Revision: 1.60 Fri Oct 23 11:49:56 2020 nxa11511
 *  Deleted the GPL license statements, leaving only BSD, as it is compatible with Linux and good for closed ROM/firmware code.
 *
 *  Revision: 1.59 Wed Sep 30 15:57:35 2020 nxa11511
 *  Now UPWR_DGN_MAX = UPWR_DGN_ALL.
 *  Redefines upwr_dgn_log_t according to dgn_lib.S;
 *  Merge from branch dgn_lib.
 *
 *  Revision: 1.58.1.1 Tue Sep 29 10:07:12 2020 nxa11511
 *  Adds UPWR_DGN_ALL to upwr_dgn_mode_t, which is now also UPWR_DGN_MAX.
 *  In upwr_dgn_log_t, DGN_LOG_EVENTNEW added DGN_LOG_SPARE deleted.
 *
 *  Revision: 1.49 Mon Jun  8 06:46:30 2020 nxa11511
 *  *** empty comment string ***
 *
 *  Revision: 1.44 Tue Apr  7 13:34:01 2020 nxf42682
 *  Put TYPES_LOCAL_H - fixed serious compilation error of version 1.42 and 1.43
 *
 *  Revision: 1.43 Tue Mar 31 12:50:46 2020 nxf42682
 *  Merged version 1.42 with 1.41.1.1
 *
 *  Revision: 1.42 Tue Mar 31 08:06:59 2020 nxa11511
 *  Fixes a compiling error.
 *
 *  Revision: 1.41 Mon Mar 30 23:07:26 2020 nxa10721
 *  Added support for AVD bias
 *
 *  Revision: 1.40 Mon Mar 30 14:29:44 2020 nxa11511
 *  Updates to API spec 20200404:
 *  API functions upwr_power_on and upwr_boot_start deleted.
 *  API functions upwr_xcp_power_on and upwr_xcp_boot_start moved to the Power Management service group;
 *  renamed to upwr_pwm_dom_power_on and upwr_pwm_boot_start
 *
 *  Revision: 1.39 Fri Mar 27 17:17:34 2020 nxa11511
 *  Adds typedef upwr_start_msg.
 *  (sets new typedef upwr_xcp_start_msg;
 *  Adds typedef upwr_resp_msg upwr_shutdown_msg;
 *
 *  Revision: 1.35 Tue Mar 10 06:24:09 2020 nxa11511
 *  Fixes identations to comply with the Linux kernel coding guidelines.
 *
 *  Revision: 1.34 Thu Mar  5 22:08:03 2020 nxa10721
 *  Using the RTD monitor config also for APD
 *
 *  Revision: 1.33 Mon Mar  2 12:16:14 2020 nxa11511
 *  Changes typedef upwr_start_msg to simple 1-word message.
 *
 *  Revision: 1.29 Mon Feb 10 10:34:29 2020 nxa10721
 *  Temporarily turns RTD config pointers as uint32_t for A35 compilation in SoC
 *
 *  Revision: 1.28 Sun Feb  9 16:10:01 2020 nxa10721
 *  Added abs_pwr_mode_t, solving TKT0532383
 *  Define RTD swt and mem configs as a pointer or 32-bit word, according to CPU
 *
 *  Revision: 1.27 Thu Jan 30 07:09:03 2020 nxa11511
 *  typedef upwr_rom_vers_t members major and minor renamed to vmajor and vminor to avoid clashing with a Linux include macro.
 *
 *  Revision: 1.23 Mon Nov 25 10:38:33 2019 nxa10721
 *  Typecastings to reduce warns
 *
 *  Revision: 1.21 Wed Nov 13 21:59:44 2019 nxa10721
 *  Added toutines to handle swt offset
 *
 *  Revision: 1.20 Tue Nov  5 12:46:45 2019 nxa10721
 *  Added APD power mode config structs, using offsets instead of pointers
 *
 *  Revision: 1.19 Thu Oct 24 11:33:48 2019 nxa10721
 *  Remove some g++ warns on strings
 *
 *  Revision: 1.18 Fri Oct 18 06:42:40 2019 nxa10721
 *  Added APD and core power modes
 *
 *  Revision: 1.17 Wed Oct  9 11:35:24 2019 nxa13158
 *  replaced powersys low power mode config by struct config
 *
 *  Revision: 1.16 Tue Sep 24 12:16:30 2019 nxa13158
 *  updated upwr_pmc_mon_rtd_cfg_t struct (removed unecessary union)
 *
 *  Revision: 1.15 Mon Aug 26 14:24:26 2019 nxa13158
 *  reorganized power modes enum to make easy to reuse in tb
 *
 *  Revision: 1.14 Fri Aug 23 17:54:11 2019 nxa11511
 *  Renames UPWR_RESP_NOT_IMPL to UPWR_RESP_UNINSTALLD.
 *
 *  Revision: 1.13 Wed Aug 21 12:59:15 2019 nxa13158
 *  renamed RTD mode to active DMA, moved pmc_bias_mode_t to
 *  pmc_api. Updated mem bias struct config
 *
 *  Revision: 1.12 Wed Aug 21 07:01:47 2019 nxa11511
 *  Several changes in message formats.
 *
 *  Revision: 1.9 Thu Aug 15 17:10:04 2019 nxa13158
 *  removed POR from power modes transitions. Not needed anymore
 *
 *  Revision: 1.8 Thu Aug 15 11:50:08 2019 nxa11511
 *  UPWR_SG_PMODE renamed to UPWR_SG_PMGMT.
 *
 *  Revision: 1.7 Wed Aug 14 10:16:48 2019 nxa13158
 *  Fixed upwr_pmc_bias_cfg_t struct definition
 *
 *  Revision: 1.6 Tue Aug 13 17:52:07 2019 nxa11511
 *  Adds Exception function enum.
 *  Fixes union upwr_pmc_mon_rtd_cfg_t.
 *
 *  Revision: 1.5 Tue Aug 13 15:26:40 2019 nxa13158
 *  added Power Modes configuration structs.
 *
 *  Revision: 1.4 Mon Aug 12 18:18:40 2019 nxa11511
 *  Message structs/unions turned into typedefs.
 *  Adds message formats for the new initialization procedure with the boot start step.
 *
 *  Revision: 1.3 Sat Aug 10 09:06:21 2019 nxa11511
 *  Adds extern "C" if __cplusplus is #defined and UPWR_NAMESPACE is #undefined.
 *
 *  Revision: 1.1 Thu Aug  1 17:14:33 2019 nxa11511
 *  uPower driver API #defines and typedefs shared with the firmware
 *
 * -----------------------------------------------------------------------------
 * KEYWORDS: micro-power uPower driver API
 * -----------------------------------------------------------------------------
 * PURPOSE: uPower driver API #defines and typedefs shared with the firmware
 * -----------------------------------------------------------------------------
 * PARAMETERS:
 * PARAM NAME RANGE:DESCRIPTION:       DEFAULTS:                           UNITS
 * -----------------------------------------------------------------------------
 * REUSE ISSUES: no reuse issues
 * -FHDR--------------------------------------------------------------------- */

#ifndef _UPWR_DEFS_H
#define _UPWR_DEFS_H

#ifndef TYPES_LOCAL_H

#include <stdint.h> /* this include breaks the SoC compile - TBD why? */

#endif /* not production code */

#ifndef UPWR_PMC_SWT_WORDS
#define UPWR_PMC_SWT_WORDS              (1U)
#endif

#ifndef UPWR_PMC_MEM_WORDS
#define UPWR_PMC_MEM_WORDS              (2U)
#endif

/* ****************************************************************************
 * DOWNSTREAM MESSAGES - COMMANDS/FUNCTIONS
 * ****************************************************************************
 */

#define UPWR_SRVGROUP_BITS  (4U)
#define UPWR_FUNCTION_BITS  (4U)
#define UPWR_PWDOMAIN_BITS  (4U)
#define UPWR_HEADER_BITS   \
                      (UPWR_SRVGROUP_BITS + UPWR_FUNCTION_BITS + UPWR_PWDOMAIN_BITS)
#define UPWR_ARG_BITS      (32U - UPWR_HEADER_BITS)
#if   ((UPWR_ARG_BITS & 1U) > 0U)
#error "UPWR_ARG_BITS must be an even number"
#endif
#define UPWR_ARG64_BITS          (64U - UPWR_HEADER_BITS)
#define UPWR_HALF_ARG_BITS       (UPWR_ARG_BITS >> 1U)
#define UPWR_DUAL_OFFSET_BITS    ((UPWR_ARG_BITS + 32U) >> 1U)

#ifdef  __cplusplus
#ifndef UPWR_NAMESPACE /* extern "C" 'cancels' the effect of namespace */
extern "C" {
#endif
#endif

/* message header: header fields common to all downstream messages.
 */

struct upwr_msg_hdr {
	uint32_t domain   :UPWR_PWDOMAIN_BITS;           /* power domain */
	uint32_t srvgrp   :UPWR_SRVGROUP_BITS;          /* service group */
	uint32_t function :UPWR_FUNCTION_BITS;             /* function */
	uint32_t arg      :UPWR_ARG_BITS;     /* function-specific argument */
};

/* generic 1-word downstream message format */

typedef union {
	struct upwr_msg_hdr  hdr;
	uint32_t             word;  /* message first word */
} upwr_down_1w_msg;

/* generic 2-word downstream message format */

typedef struct {
	struct upwr_msg_hdr  hdr;
	uint32_t             word2;  /* message second word */
} upwr_down_2w_msg;

/* message format for functions that receive a pointer/offset */

typedef struct {
	struct upwr_msg_hdr  hdr;
	uint32_t             ptr; /* config struct offset */
} upwr_pointer_msg;

/* message format for functions that receive 2 pointers/offsets */

typedef union {
	struct upwr_msg_hdr  hdr;
	struct {
		uint64_t :UPWR_HEADER_BITS;
		uint64_t ptr0:UPWR_DUAL_OFFSET_BITS;
		uint64_t ptr1:UPWR_DUAL_OFFSET_BITS;
	} ptrs;
} upwr_2pointer_msg;

typedef enum { /* Service Groups in priority order, high to low */
	UPWR_SG_EXCEPT,   /* 0 = exception           */
	UPWR_SG_PWRMGMT , /* 1 = power management    */
	UPWR_SG_DELAYM,   /* 2 = delay   measurement */
	UPWR_SG_VOLTM ,   /* 3 = voltage measurement */
	UPWR_SG_CURRM,    /* 4 = current measurement */
	UPWR_SG_TEMPM,    /* 5 = temperature measurement */
	UPWR_SG_DIAG,     /* 6 = diagnostic  */
	UPWR_SG_COUNT
} upwr_sg_t;

/* *************************************************************************
 * Initialization - downstream
 ***************************************************************************/

typedef upwr_down_1w_msg upwr_start_msg; /* start command message */

typedef upwr_down_1w_msg upwr_power_on_msg;   /* power on   command message */
typedef upwr_down_1w_msg upwr_boot_start_msg; /* boot start command message */

typedef union {
	struct upwr_msg_hdr hdr;
	upwr_power_on_msg   power_on;
	upwr_boot_start_msg boot_start;
	upwr_start_msg      start;
} upwr_startup_down_msg;

/* *************************************************************************
 * Service Group EXCEPTION - downstream
 ***************************************************************************/

typedef enum {             /* Exception Functions */
	UPWR_XCP_INIT,     /*  0 = init msg (not a service request itself) */
	UPWR_XCP_PING = UPWR_XCP_INIT,
			   /*  0 = also ping request, since its response is
				   an init msg */
	UPWR_XCP_START,    /*  1 = service start: upwr_start
                            *      (not a service request itself) */
	UPWR_XCP_SHUTDOWN, /*  2 = service shutdown: upwr_xcp_shutdown */
	UPWR_XCP_CONFIG,   /*  3 = uPower configuration: upwr_xcp_config */
	UPWR_XCP_SW_ALARM, /*  4 = uPower software alarm: upwr_xcp_sw_alarm */
	UPWR_XCP_I2C,      /*  5 = I2C access: upwr_xcp_i2c_access */
	UPWR_XCP_SPARE_6,  /*  6 = spare */
	UPWR_XCP_SET_DDR_RETN,  /*  7 = set/clear ddr retention */
	UPWR_XCP_SET_RTD_APD_LLWU,  /*  8 = set/clear rtd/apd llwu */
	UPWR_XCP_SPARE_8 = UPWR_XCP_SET_RTD_APD_LLWU,  /*  8 = spare */
    UPWR_XCP_SET_RTD_USE_DDR,      /* 9 = M33 core set it is using DDR or not */
	UPWR_XCP_SPARE_9 = UPWR_XCP_SET_RTD_USE_DDR,  /*  9 = spare */
	UPWR_XCP_SPARE_10, /* 10 = spare */
	UPWR_XCP_SPARE_11, /* 11 = spare */
	UPWR_XCP_SPARE_12, /* 12 = spare */
	UPWR_XCP_SPARE_13, /* 13 = spare */
	UPWR_XCP_SPARE_14, /* 14 = spare */
	UPWR_XCP_SPARE_15, /* 15 = spare */
	UPWR_XCP_F_COUNT
} upwr_xcp_f_t;

typedef upwr_down_1w_msg    upwr_xcp_ping_msg;
typedef upwr_down_1w_msg    upwr_xcp_shutdown_msg;
typedef upwr_power_on_msg   upwr_xcp_power_on_msg;
typedef upwr_boot_start_msg upwr_xcp_boot_start_msg;
typedef upwr_start_msg      upwr_xcp_start_msg;
typedef upwr_down_2w_msg    upwr_xcp_config_msg;
typedef upwr_down_1w_msg    upwr_xcp_swalarm_msg;
typedef upwr_down_1w_msg    upwr_xcp_ddr_retn_msg;
typedef upwr_down_1w_msg    upwr_xcp_rtd_use_ddr_msg;
typedef upwr_down_1w_msg    upwr_xcp_rtd_apd_llwu_msg;
typedef upwr_pointer_msg    upwr_xcp_i2c_msg;

typedef struct { /* structure pointed by message upwr_xcp_i2c_msg */
	uint16_t         addr;
	int8_t           data_size;
	uint8_t          subaddr_size;
	uint32_t         subaddr;
	uint32_t         data;
} upwr_i2c_access;

/* Exception all messages */

typedef union {
    struct upwr_msg_hdr       hdr;       /* message header */
	upwr_xcp_ping_msg         ping;      /* ping */
    upwr_xcp_start_msg        start;     /* service start */
	upwr_xcp_shutdown_msg     shutdown;  /* shutdown */
	upwr_xcp_boot_start_msg   bootstart; /* boot start */
	upwr_xcp_config_msg       config;    /* uPower configuration */
	upwr_xcp_swalarm_msg      swalarm;   /* software alarm */
	upwr_xcp_i2c_msg          i2c;       /* I2C access */
	upwr_xcp_ddr_retn_msg     set_ddr_retn;       /* set ddr retention msg */
	upwr_xcp_rtd_use_ddr_msg     set_rtd_use_ddr;       /* set rtd is using ddr msg */
	upwr_xcp_rtd_apd_llwu_msg     set_llwu;       /* set rtd/apd llwu msg */
} upwr_xcp_msg;

typedef struct { /* structure pointed by message upwr_volt_dva_req_id_msg */
	uint32_t         id_word0;
	uint32_t         id_word1;
	uint32_t         mode;
} upwr_dva_id_struct;

/**
 * PMIC voltage accuracy is 12.5 mV, 12500 uV
 */
#define PMIC_VOLTAGE_MIN_STEP 12500U

/* *************************************************************************
 * Service Group POWER MANAGEMENT - downstream
 ***************************************************************************/

typedef enum {            /* Power Management Functions */
	UPWR_PWM_REGCFG,  /* 0 = regulator config: upwr_pwm_reg_config */
	UPWR_PWM_DEVMODE = UPWR_PWM_REGCFG, /* deprecated, for old compile */
	UPWR_PWM_VOLT   , /* 1 = voltage change: upwr_pwm_chng_reg_voltage */
	UPWR_PWM_SWITCH , /* 2 = switch control: upwr_pwm_chng_switch_mem */
	UPWR_PWM_PWR_ON,  /* 3 = switch/RAM/ROM power on: upwr_pwm_power_on  */
	UPWR_PWM_PWR_OFF, /* 4 = switch/RAM/ROM power off: upwr_pwm_power_off */
	UPWR_PWM_RETAIN,  /* 5 = retain memory array: upwr_pwm_mem_retain */
	UPWR_PWM_DOM_BIAS,/* 6 = Domain bias control: upwr_pwm_chng_dom_bias */
	UPWR_PWM_MEM_BIAS,/* 7 = Memory bias control: upwr_pwm_chng_mem_bias */
	UPWR_PWM_PMICCFG, /* 8 = PMIC configuration:  upwr_pwm_pmic_config */
	UPWR_PWM_PMICMOD = UPWR_PWM_PMICCFG, /* deprecated, for old compile */
	UPWR_PWM_PES,      /* 9 so far, no use */
	UPWR_PWM_CONFIG , /* 10= apply power mode defined configuration */
	UPWR_PWM_CFGPTR,  /* 11= configuration pointer */
	UPWR_PWM_DOM_PWRON,/* 12 = domain power on: upwr_pwm_dom_power_on */
	UPWR_PWM_BOOT,     /* 13 = boot start: upwr_pwm_boot_start */
    UPWR_PWM_FREQ,     /* 14 = domain frequency setup */
	UPWR_PWM_PARAM,    /* 15 = power management parameters */
	UPWR_PWM_F_COUNT
} upwr_pwm_f_t;

#define MAX_PMETER_SSEL 7U

typedef enum {            /* Voltage Management Functions */
	UPWR_VTM_CHNG_PMIC_RAIL_VOLT,  /* 0 = change pmic rail voltage */
	UPWR_VTM_GET_PMIC_RAIL_VOLT, /* 1 = get pmic rail voltage */
	UPWR_VTM_PMIC_CONFIG, /* 2 = configure PMIC IC */
    UPWR_VTM_DVA_DUMP_INFO, /* 3 = dump dva information */
    UPWR_VTM_DVA_REQ_ID, /* 4 = dva request ID array */
    UPWR_VTM_DVA_REQ_DOMAIN, /* 5 = dva request domain */
    UPWR_VTM_DVA_REQ_SOC, /* 6 = dva request the whole SOC */
    UPWR_VTM_PMETER_MEAS, /* 7 = pmeter measure */
    UPWR_VTM_VMETER_MEAS, /* 8 = vmeter measure */
    UPWR_VTM_PMIC_COLD_RESET, /* 9 = pmic cold reset */
    UPWR_VTM_SET_DVFS_PMIC_RAIL,     /* 10 = set which domain use which pmic rail, for DVFS use */
    UPWR_VTM_SET_PMIC_MODE,        /* 11 = set pmic mode */
    UPWR_VTM_F_COUNT
} upwr_volt_f_t;

#define VMETER_SEL_RTD 0U
#define VMETER_SEL_LDO 1U
#define VMETER_SEL_APD 2U
#define VMETER_SEL_AVD 3U
#define VMETER_SEL_MAX 3U

/**
 * The total TSEL count is 256
 */
#define MAX_TEMP_TSEL 256U

/**
 * Support 3 temperature sensor, sensor 0, 1, 2
 */
#define MAX_TEMP_SENSOR 2U

typedef enum {          /* Temperature Management Functions */
    UPWR_TEMP_GET_CUR_TEMP,  /* 0 = get current temperature */
    UPWR_TEMP_F_COUNT
} upwr_temp_f_t;

typedef enum {          /* Delay Meter Management Functions */
    UPWR_DMETER_GET_DELAY_MARGIN, /* 0 = get delay margin */
    UPWR_DMETER_SET_DELAY_MARGIN, /* 1 = set delay margin */
    UPWR_PMON_REQ, /* 2 = process monitor service */
    UPWR_DMETER_F_COUNT
} upwr_dmeter_f_t;

typedef upwr_down_1w_msg    upwr_volt_pmeter_meas_msg;

typedef upwr_down_1w_msg    upwr_volt_pmic_set_mode_msg;

typedef upwr_down_1w_msg    upwr_volt_vmeter_meas_msg;

struct upwr_reg_config_t {
	uint32_t reg;   // TODO: real config
};

struct upwr_switch_board_t { /* set of 32 switches */
	uint32_t on;   /* Switch on state,   1 bit per instance */
	uint32_t mask; /* actuation mask, 1 bit per instance */
                       /* (bit = 1 applies on bit) */
};

struct upwr_mem_switches_t { /* set of 32 RAM/ROM switches */
	uint32_t array;   /* RAM/ROM array state,      1 bit per instance */
	uint32_t perif;   /* RAM/ROM peripheral state, 1 bit per instance */
	uint32_t mask;    /* actuation mask,       1 bit per instance */
                          /* (bit = 1 applies on bit) */
};

typedef upwr_down_1w_msg upwr_pwm_dom_pwron_msg;  /* domain power on message */
typedef upwr_down_1w_msg upwr_pwm_boot_start_msg; /* boot start      message */


/* functions with complex arguments use the pointer message formats: */

typedef upwr_pointer_msg upwr_pwm_retain_msg;
typedef upwr_pointer_msg upwr_pwm_pmode_cfg_msg;

#if    ( UPWR_ARG_BITS       < UPWR_DOMBIAS_ARG_BITS)
#if    ((UPWR_ARG_BITS + 32) < UPWR_DOMBIAS_ARG_BITS)
#error "too few message bits for domain bias argument"
#endif
#endif

typedef union {
	struct upwr_msg_hdr           hdr;       /* message header */
	struct {
		upwr_pwm_dom_bias_args  B;
	} args;
} upwr_pwm_dom_bias_msg;

/* upwr_pwm_dom_bias_args
   is an SoC-dependent message, defined in upower_soc_defs.h */

typedef union {
	struct upwr_msg_hdr           hdr;       /* message header */
	struct {
		upwr_pwm_mem_bias_args  B;
	} args;
} upwr_pwm_mem_bias_msg;

/* upwr_pwm_mem_bias_args
   is an SoC-dependent message, defined in upower_soc_defs.h */

typedef upwr_pointer_msg upwr_pwm_pes_seq_msg;

/* upwr_pwm_reg_config-specific message format */

typedef upwr_pointer_msg upwr_pwm_regcfg_msg ;

/* upwr_volt_pmic_volt-specific message format */

typedef union {
	struct upwr_msg_hdr           hdr;       /* message header */
	struct {
		uint32_t rsv:UPWR_HEADER_BITS;
        uint32_t domain: 8U;
        uint32_t rail: 8U;
	} args;
} upwr_volt_dom_pmic_rail_msg;

typedef union {
	struct upwr_msg_hdr hdr;
	struct {
		uint32_t rsv:UPWR_HEADER_BITS;
		uint32_t rail: 4U;  /* pmic rail id  */
		uint32_t volt: 12U; /* voltage value, accurate to mV, support 0~3.3V */
	} args;
} upwr_volt_pmic_set_volt_msg;

typedef union {
	struct upwr_msg_hdr hdr;
	struct {
		uint32_t rsv:UPWR_HEADER_BITS;
		uint32_t rail: 16U;  /* pmic rail id  */
	} args;
} upwr_volt_pmic_get_volt_msg;

typedef union {
	struct upwr_msg_hdr hdr;
	struct {
		uint32_t rsv:UPWR_HEADER_BITS;
		uint32_t domain: 8U;
		uint32_t mode: 8U; /* work mode */
	} args;
} upwr_volt_dva_req_domain_msg;

typedef union {
	struct upwr_msg_hdr hdr;
	struct {
		uint32_t rsv:UPWR_HEADER_BITS;
		uint32_t mode: 16U;  /* work mode  */
	} args;
} upwr_volt_dva_req_soc_msg;

typedef union {
	struct upwr_msg_hdr hdr;
	struct {
		uint32_t rsv:UPWR_HEADER_BITS;
		uint32_t addr_offset: 16U;  /* addr_offset to 0x28330000  */
	} args;
} upwr_volt_dva_dump_info_msg;

typedef upwr_pointer_msg upwr_volt_pmiccfg_msg;
typedef upwr_pointer_msg upwr_volt_dva_req_id_msg;
typedef upwr_down_1w_msg upwr_volt_pmic_cold_reset_msg;

/* upwr_pwm_volt-specific message format */

typedef union {
	struct upwr_msg_hdr hdr;
	struct {
		uint32_t rsv:UPWR_HEADER_BITS;
		uint32_t reg:UPWR_HALF_ARG_BITS;  /* regulator id  */
		uint32_t volt:UPWR_HALF_ARG_BITS; /* voltage value */
	} args;
} upwr_pwm_volt_msg;

/* upwr_pwm_freq_setup-specific message format */

/**
 * This message structure is used for DVFS feature
 * 1. Because user may use different PMIC or different board,
 * the pmic regulator of RTD/APD may change,
 * so, user need to tell uPower the regulator number.
 * The number must be matched with PMIC IC and board.
 * use 4 bits for pmic regulator, support to 16 regulator.
 *
 * use 12 bits for target frequency, accurate to MHz, support to 4096 MHz
 */
typedef union {
	struct upwr_msg_hdr hdr;
	struct {
		uint32_t rsv: UPWR_HEADER_BITS;
		uint32_t rail: 4; /* pmic regulator  */
		uint32_t target_freq: 12; /* target frequency */
	} args;
} upwr_pwm_freq_msg;

typedef upwr_down_2w_msg upwr_pwm_param_msg;

/* upwr_pwm_pmiccfg-specific message format */

typedef upwr_pointer_msg upwr_pwm_pmiccfg_msg;

/* functions that pass a pointer use message format upwr_pointer_msg */

typedef upwr_pointer_msg upwr_pwm_cfgptr_msg;

/* functions that pass 2 pointers use message format upwr_2pointer_msg
 */

typedef upwr_2pointer_msg upwr_pwm_switch_msg;
typedef upwr_2pointer_msg upwr_pwm_pwron_msg;
typedef upwr_2pointer_msg upwr_pwm_pwroff_msg;

/* Power Management all messages */

typedef union {
	struct upwr_msg_hdr     hdr;      /* message header */
	upwr_pwm_param_msg      param;    /* power management parameters */
	upwr_pwm_dom_bias_msg   dom_bias; /* domain bias message */
	upwr_pwm_mem_bias_msg   mem_bias; /* memory bias message */
	upwr_pwm_pes_seq_msg    pes;      /* PE seq. message */
	upwr_pwm_pmode_cfg_msg  pmode;    /* power mode config message */
	upwr_pwm_regcfg_msg     regcfg;   /* regulator config message */
	upwr_pwm_volt_msg       volt;     /* set voltage message */
	upwr_pwm_freq_msg       freq;     /* set frequency message */
	upwr_pwm_switch_msg     switches; /* switch control message */
	upwr_pwm_pwron_msg      pwron;    /* switch/RAM/ROM power on  message */
	upwr_pwm_pwroff_msg     pwroff;   /* switch/RAM/ROM power off message */
	upwr_pwm_retain_msg     retain;   /* memory retain message */
	upwr_pwm_cfgptr_msg     cfgptr;   /* configuration pointer message*/
	upwr_pwm_dom_pwron_msg  dompwron; /* domain power on message */
	upwr_pwm_boot_start_msg boot;     /* boot start      message */
} upwr_pwm_msg;

typedef union {
	struct upwr_msg_hdr     hdr;      /* message header */
	upwr_volt_pmic_set_volt_msg  set_pmic_volt;     /* set pmic voltage message */
	upwr_volt_pmic_get_volt_msg  get_pmic_volt;     /* set pmic voltage message */
	upwr_volt_pmic_set_mode_msg  set_pmic_mode;     /* set pmic mode message */
	upwr_volt_pmiccfg_msg    pmiccfg;  /* PMIC configuration message */
	upwr_volt_dom_pmic_rail_msg   dom_pmic_rail; /* domain bias message */
	upwr_volt_dva_dump_info_msg    dva_dump_info;  /* dump dva info message */
	upwr_volt_dva_req_id_msg    dva_req_id;  /* dump dva request id array message */
	upwr_volt_dva_req_domain_msg    dva_req_domain;  /* dump dva request domain message */
	upwr_volt_dva_req_soc_msg    dva_req_soc;  /* dump dva request whole soc message */
	upwr_volt_pmeter_meas_msg    pmeter_meas_msg;  /* pmeter measure message */
	upwr_volt_vmeter_meas_msg    vmeter_meas_msg;  /* vmeter measure message */
	upwr_volt_pmic_cold_reset_msg    cold_reset_msg;  /* pmic cold reset message */
} upwr_volt_msg;


typedef union {
	struct upwr_msg_hdr hdr;
	struct {
		uint32_t rsv:UPWR_HEADER_BITS;
		uint32_t sensor_id: 16U;  /* temperature sensor id  */
	} args;
} upwr_temp_get_cur_temp_msg;

typedef union {
	struct upwr_msg_hdr hdr;
	struct {
		uint32_t rsv:UPWR_HEADER_BITS;
		uint32_t index: 8U;  /* the delay meter index  */
		uint32_t path: 8U;  /* the critical path number  */
	} args;
} upwr_dmeter_get_delay_margin_msg;

#define MAX_DELAY_MARGIN 63U
#define MAX_DELAY_CRITICAL_PATH 7U
#define MAX_DELAY_METER_NUM 1U

typedef union {
	struct upwr_msg_hdr hdr;
	struct {
		uint32_t rsv:UPWR_HEADER_BITS;
		uint32_t index: 4U;  /* the delay meter index  */
		uint32_t path: 4U;  /* the critical path number  */
		uint32_t dm: 8U;  /* the delay margin value of delay meter  */
	} args;
} upwr_dmeter_set_delay_margin_msg;

#define MAX_PMON_CHAIN_SEL 1U

typedef union {
	struct upwr_msg_hdr hdr;
	struct {
		uint32_t rsv:UPWR_HEADER_BITS;
		uint32_t chain_sel: 16U;  /* the process monitor delay chain sel  */
	} args;
} upwr_pmon_msg;

typedef union {
    struct upwr_msg_hdr hdr;           /* message header */
    upwr_temp_get_cur_temp_msg  get_temp_msg;      /* get current temperature message */
} upwr_temp_msg;

typedef union {
    struct upwr_msg_hdr hdr;           /* message header */
    upwr_dmeter_get_delay_margin_msg  get_margin_msg;      /* get delay margin message */
    upwr_dmeter_set_delay_margin_msg  set_margin_msg;      /* set delay margin message */
    upwr_pmon_msg  pmon_msg;      /* process monitor message */
} upwr_dmeter_msg;

typedef upwr_down_2w_msg upwr_down_max_msg; /* longest downstream msg */

/* upwr_dom_bias_cfg_t and upwr_mem_bias_cfg_t
   are SoC-dependent structs, defined in upower_soc_defs.h */

/* Power and mem switches */
typedef struct {
	volatile struct upwr_switch_board_t  swt_board[UPWR_PMC_SWT_WORDS];
	volatile struct upwr_mem_switches_t  swt_mem  [UPWR_PMC_MEM_WORDS]  ;
} swt_config_t;

/* *************************************************************************
 * Service Group DIAGNOSE - downstream
 ***************************************************************************/

typedef enum {            /* Diagnose Functions */
	UPWR_DGN_MODE,    /* 0 = diagnose mode: upwr_dgn_mode */
	UPWR_DGN_F_COUNT,
    UPWR_DGN_BUFFER_EN,
} upwr_dgn_f_t;

typedef enum {
	UPWR_DGN_ALL2ERR, /* record all until an error occurs,
			     freeze recording on error             */
	UPWR_DGN_ALL2HLT, /* record all until an error occurs,
			     halt core        on error             */
	UPWR_DGN_ALL,     /* trace, warnings, errors, task state recorded */
	UPWR_DGN_MAX = UPWR_DGN_ALL,
	UPWR_DGN_TRACE,   /* trace, warnings, errors recorded      */
	UPWR_DGN_SRVREQ,  /* service request activity recorded     */
	UPWR_DGN_WARN,    /* warnings and errors recorded          */
	UPWR_DGN_ERROR,   /* only errors recorded                  */
	UPWR_DGN_NONE,    /* no diagnostic recorded                */
	UPWR_DGN_COUNT
} upwr_dgn_mode_t;

typedef upwr_down_1w_msg upwr_dgn_mode_msg;

typedef union {
	struct upwr_msg_hdr   hdr;
	upwr_dgn_mode_msg     mode_msg;
} upwr_dgn_msg;

typedef struct {
    struct upwr_msg_hdr   hdr;
    uint32_t              buf_addr;
} upwr_dgn_v2_msg;

/* diagnostics log types in the shared RAM log buffer */

typedef enum {
	DGN_LOG_NONE       =   0x00000000,
	DGN_LOG_INFO       =   0x10000000,
	DGN_LOG_ERROR      =   0x20000000,
	DGN_LOG_ASSERT     =   0x30000000,
	DGN_LOG_EXCEPT     =   0x40000000,
	DGN_LOG_EVENT      =   0x50000000, // old event trace
	DGN_LOG_EVENTNEW   =   0x60000000, // new event trace
	DGN_LOG_SERVICE    =   0x70000000,
	DGN_LOG_TASKDEF    =   0x80000000,
	DGN_LOG_TASKEXE    =   0x90000000,
	DGN_LOG_MUTEX      =   0xA0000000,
	DGN_LOG_SEMAPH     =   0xB0000000,
	DGN_LOG_TIMER      =   0xC0000000,
	DGN_LOG_CALLTRACE  =   0xD0000000,
	DGN_LOG_DATA       =   0xE0000000,
	DGN_LOG_PCTRACE    =   0xF0000000
} upwr_dgn_log_t;

/* ****************************************************************************
 * UPSTREAM MESSAGES - RESPONSES
 * ****************************************************************************
 */

/* generic ok/ko response message */

#define UPWR_RESP_ERR_BITS (4U)
#define UPWR_RESP_HDR_BITS (UPWR_RESP_ERR_BITS+\
                            UPWR_SRVGROUP_BITS+UPWR_FUNCTION_BITS)
#define UPWR_RESP_RET_BITS (32U - UPWR_RESP_HDR_BITS)

typedef enum { /* response error codes */
	UPWR_RESP_OK = 0,     /* no error */
	UPWR_RESP_SG_BUSY,    /* service group is busy */
	UPWR_RESP_SHUTDOWN,   /* services not up or shutting down */
	UPWR_RESP_BAD_REQ,    /* invalid request */
	UPWR_RESP_BAD_STATE,  /* system state doesn't allow perform the request */
	UPWR_RESP_UNINSTALLD, /* service or function not installed */
	UPWR_RESP_UNINSTALLED =
	UPWR_RESP_UNINSTALLD, /* service or function not installed (alias) */
	UPWR_RESP_RESOURCE,   /* resource not available */
	UPWR_RESP_TIMEOUT,    /* service timeout */
	UPWR_RESP_COUNT
} upwr_resp_t;

struct upwr_resp_hdr {
	uint32_t errcode :UPWR_RESP_ERR_BITS;
	uint32_t srvgrp  :UPWR_SRVGROUP_BITS;      /* service group */
	uint32_t function:UPWR_FUNCTION_BITS;
	uint32_t ret     :UPWR_RESP_RET_BITS;      /* return value, if any */
};

/* generic 1-word upstream message format */

typedef union {
	struct upwr_resp_hdr hdr;
	uint32_t             word;
} upwr_resp_msg;

/* generic 2-word upstream message format */

typedef struct {
	struct upwr_resp_hdr   hdr;
	uint32_t               word2;  /* message second word */
} upwr_up_2w_msg;

typedef upwr_up_2w_msg   upwr_up_max_msg;

/* *************************************************************************
 * Exception/Initialization - upstream
 ***************************************************************************/

#define UPWR_SOC_BITS    (7U)
#define UPWR_VMINOR_BITS (4U)
#define UPWR_VFIXES_BITS (4U)
#define UPWR_VMAJOR_BITS \
           (32U - UPWR_HEADER_BITS - UPWR_SOC_BITS - UPWR_VMINOR_BITS - UPWR_VFIXES_BITS)

typedef struct {
	uint32_t soc_id;
	uint32_t vmajor;
	uint32_t vminor;
	uint32_t vfixes;
} upwr_code_vers_t;

/* message sent by firmware initialization, received by upwr_init */

typedef union {
	struct upwr_resp_hdr hdr;
	struct {
		uint32_t rsv:UPWR_RESP_HDR_BITS;
		uint32_t soc:UPWR_SOC_BITS;        /* SoC identification */
		uint32_t vmajor:UPWR_VMAJOR_BITS;  /* firmware major version */
		uint32_t vminor:UPWR_VMINOR_BITS;  /* firmware minor version */
		uint32_t vfixes:UPWR_VFIXES_BITS;  /* firmware fixes version */
	} args;
} upwr_init_msg;

/* message sent by firmware when the core platform is powered up */

typedef upwr_resp_msg upwr_power_up_msg;

/* message sent by firmware when the core reset is released for boot */

typedef upwr_resp_msg upwr_boot_up_msg;

/* message sent by firmware when ready for service requests */

#define UPWR_RAM_VMINOR_BITS (7)
#define UPWR_RAM_VFIXES_BITS (6)
#define UPWR_RAM_VMAJOR_BITS (32-UPWR_HEADER_BITS \
                                -UPWR_RAM_VFIXES_BITS-UPWR_RAM_VMINOR_BITS)

typedef union {
	struct upwr_resp_hdr hdr;
	struct {
		uint32_t rsv:UPWR_RESP_HDR_BITS;
		uint32_t vmajor:UPWR_RAM_VMAJOR_BITS; /* RAM fw major version */
		uint32_t vminor:UPWR_RAM_VMINOR_BITS; /* RAM fw minor version */
		uint32_t vfixes:UPWR_RAM_VFIXES_BITS; /* RAM fw fixes version */
	} args;
} upwr_ready_msg;

/* message sent by firmware when shutdown finishes */

typedef upwr_resp_msg upwr_shutdown_msg;

typedef union {
	struct upwr_resp_hdr hdr;
	upwr_init_msg        init;
	upwr_power_up_msg    pwrup;
	upwr_boot_up_msg     booted;
	upwr_ready_msg       ready;
} upwr_startup_up_msg;

/* message sent by firmware for uPower config setting */

typedef upwr_resp_msg upwr_config_resp_msg;

/* message sent by firmware for uPower alarm */

typedef upwr_resp_msg upwr_alarm_resp_msg;

/* *************************************************************************
 * Power Management - upstream
 ***************************************************************************/

typedef upwr_resp_msg upwr_param_resp_msg;

enum work_mode {
    OVER_DRIVE,
    NORMAL_DRIVE,
    LOW_DRIVE
};

#define UTIMER3_MAX_COUNT 0xFFFFU

#ifdef  __cplusplus
#ifndef UPWR_NAMESPACE /* extern "C" 'cancels' the effect of namespace */
} /* extern "C" */
#endif
#endif

#endif /* #ifndef _UPWR_DEFS_H */
