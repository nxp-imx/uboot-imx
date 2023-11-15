// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 NXP
 *
 * Peng Fan <peng.fan@nxp.com>
 */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/ccm_regs.h>
#include <asm/mach-imx/sys_proto.h>
#include <asm/global_data.h>
#include <dm/uclass.h>
#include <dm/uclass-internal.h>
#include <dm/device.h>
#include <dm/device-internal.h>
#include <dt-bindings/clock/fsl,imx95-clock.h>
#include <linux/clk-provider.h>
#include <scmi_agent.h>
#include <scmi_protocols.h>

DECLARE_GLOBAL_DATA_PTR;


int imx_clk_scmi_enable(u32 clock_id, bool enable)
{
	struct scmi_clk_state_in in = {
		.clock_id = clock_id,
		.attributes = (enable) ? 1 : 0,
	};
	struct scmi_clk_state_out out;
	struct scmi_msg msg = SCMI_MSG_IN(SCMI_PROTOCOL_ID_CLOCK,
					  SCMI_CLOCK_CONFIG_SET,
					  in, out);
	int ret;

	ret = devm_scmi_process_msg(gd->arch.scmi_dev, gd->arch.scmi_channel, &msg);
	if (ret)
		return ret;

	return scmi_to_linux_errno(out.status);
}

ulong imx_clk_scmi_set_rate(u32 clock_id, ulong rate)
{
	struct scmi_clk_rate_set_in in = {
		.clock_id = clock_id,
		.flags = SCMI_CLK_RATE_ROUND_CLOSEST,
		.rate_lsb = (u32)rate,
		.rate_msb = (u32)((u64)rate >> 32),
	};
	struct scmi_clk_rate_set_out out;
	struct scmi_msg msg = SCMI_MSG_IN(SCMI_PROTOCOL_ID_CLOCK,
					  SCMI_CLOCK_RATE_SET,
					  in, out);
	int ret;

	ret = devm_scmi_process_msg(gd->arch.scmi_dev, gd->arch.scmi_channel, &msg);
	if (ret < 0)
		return ret;

	ret = scmi_to_linux_errno(out.status);
	if (ret < 0)
		return ret;

	struct scmi_clk_rate_get_in in_rate = {
		.clock_id = clock_id,
	};
	struct scmi_clk_rate_get_out out_rate;

	msg = SCMI_MSG_IN(SCMI_PROTOCOL_ID_CLOCK, SCMI_CLOCK_RATE_GET, in_rate, out_rate);

	ret = devm_scmi_process_msg(gd->arch.scmi_dev, gd->arch.scmi_channel, &msg);
	if (ret < 0)
		return ret;

	ret = scmi_to_linux_errno(out_rate.status);
	if (ret < 0)
		return ret;

	return (ulong)(((u64)out_rate.rate_msb << 32) | out_rate.rate_lsb);
}

ulong imx_clk_scmi_get_rate(u32 clock_id)
{
	struct scmi_clk_rate_get_in in = {
		.clock_id = clock_id,
	};
	struct scmi_clk_rate_get_out out;
	struct scmi_msg msg = SCMI_MSG_IN(SCMI_PROTOCOL_ID_CLOCK,
					  SCMI_CLOCK_RATE_GET,
					  in, out);
	int ret;

	ret = devm_scmi_process_msg(gd->arch.scmi_dev, gd->arch.scmi_channel, &msg);
	if (ret < 0)
		return ret;

	ret = scmi_to_linux_errno(out.status);
	if (ret < 0)
		return ret;

	return (ulong)(((u64)out.rate_msb << 32) | out.rate_lsb);
}

int imx_clk_scmi_set_parent(u32 clock_id, u32 parent_id)
{
	struct scmi_clk_parent_set_in in = {
		.clock_id = clock_id,
		.parent_clk = parent_id,
	};
	struct scmi_clk_parent_set_out out;
	struct scmi_msg msg = SCMI_MSG_IN(SCMI_PROTOCOL_ID_CLOCK,
					  SCMI_CLOCK_PARENT_SET,
					  in, out);
	int ret;

	ret = devm_scmi_process_msg(gd->arch.scmi_dev, gd->arch.scmi_channel, &msg);
	if (ret < 0)
		return ret;

	ret = scmi_to_linux_errno(out.status);
	if (ret < 0 && ret != -EACCES)
		printf("%s: %d, clock_id %u\n", __func__, ret, clock_id);

	return ret;
}
