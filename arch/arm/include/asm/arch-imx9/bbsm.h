/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2025 NXP
 */

#ifndef __IMX_BBSM_H__
#define __IMX_BBSM_H__

/* BBSM Registers offsets */
#define BBSM_REG_OFFSET_EXT_TAMPER_ACTIVITY (0x18)
#define BBSM_REG_OFFSET_EXT_TAMPER_CONGIF0  (0x100)
#define BBSM_REG_OFFSET_EXT_TAMPER_CONGIF1  (0x104)
/* BBSM Registers ZMK offsets, i = 0 to 7 */
#define BBSM_REG_OFFSET_ZMK(i)              (0x200 + (i) * 4)

/*
 * Prepare config for External Tamper(0).
 * Config for External Tamper(1) can be prepared through similar steps.
 *
 * Passive External Tamper(0) alert set to be Alert Level 2.
 * Note: In case of Alert Level 4, ZMK register bits will be zeroized.
 */
#define EXT_TAMPER_CONFIG_PASSIVE_ACTIVE_LOW (0x1)
#define EXT_TAMPER_CONFIG_ALERT_LEVEL_1      (0x3)
#define EXT_TAMPER_CONFIG_ALERT_LEVEL_2      (0x5)
#define EXT_TAMPER_CONFIG_ALERT_LEVEL_3      (0x6)
#define EXT_TAMPER_CONFIG_ALERT_LEVEL_4      (0x9)
#define EXT_TAMPER_CONFIG_GLITCH_FILTER_EN   (0x1)
#define EXT_TAMPER_CONFIG0 (\
		EXT_TAMPER_CONFIG_PASSIVE_ACTIVE_LOW << 0 | \
		EXT_TAMPER_CONFIG_ALERT_LEVEL_2 << 8 | \
		EXT_TAMPER_CONFIG_GLITCH_FILTER_EN << 12)

/*
 * Enable BBSM Tamper Detection:
 *  - Set ELE policies to apply in case of BBSM events.
 *  - Configure External Passive Tamper detection.
 */
int bbsm_tamper_detect_enable(void);
#endif
