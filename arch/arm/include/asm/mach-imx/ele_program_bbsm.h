/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2025 NXP
 */

#ifndef __ELE_PROGRAM_BBSM_H__
#define __ELE_PROGRAM_BBSM_H__

#define ELE_PROGRAM_BBSM_REQ (0xBB)

/* ELE Program BBSM API operation flags */
#define ELE_PROGRAM_BBSM_OP_READ_REG (0x6A)
#define ELE_PROGRAM_BBSM_OP_WRITE_REG (0x71)
#define ELE_PROGRAM_BBSM_OP_SET_BBSM_EVENT_POLICIES (0xCA)
#define ELE_PROGRAM_BBSM_OP_CLEAR_INTERRUPT (0xFE)

/* ELE policies */
#define ELE_POLICY_1_LOG_EVENT (0x1)
#define ELE_POLICY_2_ABORT (0x2)
#define ELE_POLICY_3_TRIGGER_INTERRUPT (0x4)
#define ELE_POLICY_4_DISABLE_HSM (0x8)

/*
 * Below policies being set based on Alert Level:
 *
 * Alert Level 1: Log Event
 * Alert Level 2: Log Event + Trigger Interrupt
 * Alert Level 3: Abort
 * Alert Level 4: Log Event + Disable HSM APIs + Trigger Interrupt
 *
 * Note: In case of Alert Level 4 Tamper, ZMK register bits will get cleared.
 */
#define ELE_ALERT_LEVEL_1_POLICIES \
		ELE_POLICY_1_LOG_EVENT
#define ELE_ALERT_LEVEL_2_POLICIES ( \
		ELE_POLICY_1_LOG_EVENT | \
		ELE_POLICY_3_TRIGGER_INTERRUPT)
#define ELE_ALERT_LEVEL_3_POLICIES \
		ELE_POLICY_2_ABORT
#define ELE_ALERT_LEVEL_4_POLICIES ( \
		ELE_POLICY_1_LOG_EVENT | \
		ELE_POLICY_3_TRIGGER_INTERRUPT | \
		ELE_POLICY_4_DISABLE_HSM)

/* ELE Alert Level offsets for Policy Mask */
#define ELE_ALERT_LEVEL_1_POLICY_OFFSET 0
#define ELE_ALERT_LEVEL_2_POLICY_OFFSET 4
#define ELE_ALERT_LEVEL_3_POLICY_OFFSET 8
#define ELE_ALERT_LEVEL_4_POLICY_OFFSET 12

/* Policy Bitmask of each Alert Level */
#define ELE_ALERT_LEVEL_1_POLICIES_BITMASK \
		(ELE_ALERT_LEVEL_1_POLICIES << ELE_ALERT_LEVEL_1_POLICY_OFFSET)
#define ELE_ALERT_LEVEL_2_POLICIES_BITMASK \
		(ELE_ALERT_LEVEL_2_POLICIES << ELE_ALERT_LEVEL_2_POLICY_OFFSET)
#define ELE_ALERT_LEVEL_3_POLICIES_BITMASK \
		(ELE_ALERT_LEVEL_3_POLICIES << ELE_ALERT_LEVEL_3_POLICY_OFFSET)
#define ELE_ALERT_LEVEL_4_POLICIES_BITMASK \
		(ELE_ALERT_LEVEL_4_POLICIES << ELE_ALERT_LEVEL_4_POLICY_OFFSET)

/* ELE Policies Mask for BBSM Events */
#define ELE_POLICY_MASK_BBSM ( \
		ELE_ALERT_LEVEL_1_POLICIES_BITMASK | \
		ELE_ALERT_LEVEL_2_POLICIES_BITMASK | \
		ELE_ALERT_LEVEL_3_POLICIES_BITMASK | \
		ELE_ALERT_LEVEL_4_POLICIES_BITMASK)

int ele_program_bbsm(u8 operation, u16 policy_mask, u32 reg_offset,
		     u32 reg_value, u32 *response, u32 *resp_reg_value);
#endif
