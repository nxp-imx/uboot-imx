// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2025 NXP
 *
 */

#include <stdio.h>
#include <linux/types.h>
#include <asm/arch-imx9/bbsm.h>
#include <asm/mach-imx/ele_program_bbsm.h>

int bbsm_tamper_detect_enable(void)
{
	u32 reg_value = 0, resp = 0;
	int ret;

	/* Set ELE policies to apply for BBSM events */
	ret = ele_program_bbsm(ELE_PROGRAM_BBSM_OP_SET_BBSM_EVENT_POLICIES,
			       ELE_POLICY_MASK_BBSM,
			       0x0, 0x0, &resp, NULL);
	if (ret) {
		printf("BBSM Set Policies failed %d, resp 0x%x\n", ret, resp);
		return ret;
	}

	/*
	 * Write config for External Tamper(0).
	 * Config for External Tamper(1) can be written through similar steps.
	 */
	ret = ele_program_bbsm(ELE_PROGRAM_BBSM_OP_WRITE_REG, 0x0,
			       BBSM_REG_OFFSET_EXT_TAMPER_CONGIF0,
			       EXT_TAMPER_CONFIG0, &resp, NULL);
	if (ret) {
		printf("BBSM Write Reg (EXT Tamper Config0) failed %d, resp 0x%x\n", ret, resp);
		return ret;
	}

	/*
	 * Read External Tamper Activity Register to check if any security event
	 * has been reported.
	 */
	ret = ele_program_bbsm(ELE_PROGRAM_BBSM_OP_READ_REG, 0x0,
			       BBSM_REG_OFFSET_EXT_TAMPER_ACTIVITY,
			       0x0, &resp, &reg_value);
	if (ret)
		printf("BBSM Read Reg (EXT Tamper Activity) failed %d, resp 0x%x\n", ret, resp);

	if (reg_value)
		printf("BBSM External Tamper security event reported!\n");

	return ret;
}
