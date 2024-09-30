// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 NXP
 *
 * Author: Alice Guo <alice.guo@nxp.com>
 */

#include <asm/arch/ccm_regs.h>
#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/sys_proto.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <command.h>
#include <errno.h>
#ifdef CONFIG_CLK_SCMI
#include <dt-bindings/clock/fsl,imx95-clock.h>
#include <dm/uclass.h>
#include <dm/uclass-internal.h>
#include <linux/clk-provider.h>
#include <scmi_agent.h>
#include <scmi_protocols.h>
#include <dm/device.h>
#include <dm/device-internal.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

u32 get_arm_core_clk(void)
{
	u32 val;

	/* TODO: */
	val = imx_clk_scmi_get_rate(IMX95_CLK_SEL_A55C0);
	if (val)
		return val;
	return imx_clk_scmi_get_rate(IMX95_CLK_A55);
}

void set_arm_core_max_clk(void)
{
	int ret;
	u32 arm_domain_id = 8;

	struct scmi_perf_in in = {
		.domain_id = arm_domain_id,
		.perf_level = 3,
	};
	struct scmi_perf_out out;
	struct scmi_msg msg = SCMI_MSG_IN(SCMI_PROTOCOL_ID_PERF, SCMI_PERF_LEVEL_SET, in, out);

	ret = devm_scmi_process_msg(gd->arch.scmi_dev, &msg);
	if (ret)
		printf("%s: %d\n", __func__, ret);
}

void enable_usboh3_clk(unsigned char enable)
{

}

int clock_init_early(void)
{
	return 0;
}

/* Set bus and A55 core clock per voltage mode */
int clock_init_late(void)
{
	set_arm_core_max_clk();

	return 0;
}

u32 get_lpuart_clk(void)
{
	return imx_clk_scmi_get_rate(IMX95_CLK_LPUART1);
}

void init_uart_clk(u32 index)
{
	u32 clock_id;
	switch (index) {
	case 0:
		clock_id = IMX95_CLK_LPUART1;
		break;
	case 1:
		clock_id = IMX95_CLK_LPUART2;
		break;
	case 2:
		clock_id = IMX95_CLK_LPUART3;
		break;
	default:
		return;
	}

	/* 24MHz */
	imx_clk_scmi_enable(clock_id, false);
	imx_clk_scmi_set_parent(clock_id, IMX95_CLK_24M);
	imx_clk_scmi_set_rate(clock_id, 24000000);
	imx_clk_scmi_enable(clock_id, true);

}

/* I2C check */
u32 imx_get_i2cclk(u32 i2c_num)
{
	if (i2c_num > 7)
		return -EINVAL;
	switch (i2c_num) {
	case 0:
		return imx_clk_scmi_get_rate(IMX95_CLK_LPI2C1);
	case 1:
		return imx_clk_scmi_get_rate(IMX95_CLK_LPI2C2);
	case 2:
		return imx_clk_scmi_get_rate(IMX95_CLK_LPI2C3);
	case 3:
		return imx_clk_scmi_get_rate(IMX95_CLK_LPI2C4);
	case 4:
		return imx_clk_scmi_get_rate(IMX95_CLK_LPI2C5);
	case 5:
		return imx_clk_scmi_get_rate(IMX95_CLK_LPI2C6);
	case 6:
		return imx_clk_scmi_get_rate(IMX95_CLK_LPI2C7);
	case 7:
		return imx_clk_scmi_get_rate(IMX95_CLK_LPI2C8);
	default:
		return 0;
	}

	return 0;
}

int enable_i2c_clk(unsigned char enable, u32 i2c_num)
{
	u32 clock_id;
	if (i2c_num > 7)
		return -EINVAL;

	switch (i2c_num) {
	case 0:
		clock_id = IMX95_CLK_LPI2C1;
		break;
	case 1:
		clock_id = IMX95_CLK_LPI2C2;
		break;
	case 2:
		clock_id = IMX95_CLK_LPI2C3;
		break;
	case 3:
		clock_id = IMX95_CLK_LPI2C4;
		break;
	case 4:
		clock_id = IMX95_CLK_LPI2C5;
		break;
	case 5:
		clock_id = IMX95_CLK_LPI2C6;
		break;
	case 6:
		clock_id = IMX95_CLK_LPI2C7;
		break;
	case 7:
		clock_id = IMX95_CLK_LPI2C8;
		break;
	default:
		return 0;
	}

	/* 24MHz */
	imx_clk_scmi_enable(clock_id, false);
	imx_clk_scmi_set_parent(clock_id, IMX95_CLK_24M);
	imx_clk_scmi_set_rate(clock_id, 24000000);
	imx_clk_scmi_enable(clock_id, true);

	return 0;
}

/* dfs_clkout[1]: 800.00MHz */
void init_clk_usdhc(u32 usdhc_id)
{
	u32 clock_id;

	switch (usdhc_id) {
	case 0:
		clock_id = IMX95_CLK_USDHC1;
		break;
	case 1:
		clock_id = IMX95_CLK_USDHC2;
		break;
	case 2:
		clock_id = IMX95_CLK_USDHC3;
		break;
	default:
		return;
	};

	/* 400MHz */
	imx_clk_scmi_enable(clock_id, false);
	imx_clk_scmi_set_parent(clock_id, IMX95_CLK_SYSPLL1_PFD1);
	imx_clk_scmi_set_rate(clock_id, 400000000);
	imx_clk_scmi_enable(clock_id, true);
}

int set_clk_netc(enum enet_freq type)
{
	ulong rate;

	switch (type) {
	case ENET_125MHZ:
		rate = MHZ(250); /* 250Mhz */
		break;
	case ENET_50MHZ:
		rate = MHZ(100); /* 100Mhz */
		break;
	case ENET_25MHZ:
		rate = MHZ(50); /* 50Mhz */
		break;
	default:
		return -EINVAL;
	}

	/* disable the clock first */
	imx_clk_scmi_enable(IMX95_CLK_ENETREF, false);
	imx_clk_scmi_set_parent(IMX95_CLK_ENETREF, IMX95_CLK_SYSPLL1_PFD0);
	imx_clk_scmi_set_rate(IMX95_CLK_ENETREF, rate);
	imx_clk_scmi_enable(IMX95_CLK_ENETREF, true);

	return 0;
}


#ifdef CONFIG_SPL_BUILD
void dram_pll_init(ulong pll_val)
{
	/* Try to configure the DDR PLL. */
	u64 ddr_rate = pll_val;
	/*vco_range 2.5G - 5G */
	u64 vco_rate = ddr_rate * DIV_ROUND_UP(MHZ(3000), ddr_rate);
	u64 v_rate, rate;
	v_rate = imx_clk_scmi_set_rate(IMX95_CLK_DRAMPLL_VCO, vco_rate);
	rate = imx_clk_scmi_set_rate(IMX95_CLK_DRAMPLL, ddr_rate);

	debug("%s vco:%llu rate:%llu\n", __func__, v_rate, rate);
}

void dram_enable_bypass(ulong clk_val)
{
	u64 rate;
	switch (clk_val) {
	case MHZ(625):
		imx_clk_scmi_set_parent(IMX95_CLK_DRAMALT, IMX95_CLK_SYSPLL1_PFD2);
		rate = imx_clk_scmi_set_rate(IMX95_CLK_DRAMALT, clk_val);
		break;
	case MHZ(400):
		imx_clk_scmi_set_parent(IMX95_CLK_DRAMALT, IMX95_CLK_SYSPLL1_PFD1);
		rate = imx_clk_scmi_set_rate(IMX95_CLK_DRAMALT, clk_val);
		break;
	case MHZ(333):
		imx_clk_scmi_set_parent(IMX95_CLK_DRAMALT, IMX95_CLK_SYSPLL1_PFD0);
		rate = imx_clk_scmi_set_rate(IMX95_CLK_DRAMALT, 333333333);
		break;
	case MHZ(200):
		imx_clk_scmi_set_parent(IMX95_CLK_DRAMALT, IMX95_CLK_SYSPLL1_PFD1);
		rate = imx_clk_scmi_set_rate(IMX95_CLK_DRAMALT, clk_val);
		break;
	case MHZ(100):
		imx_clk_scmi_set_parent(IMX95_CLK_DRAMALT, IMX95_CLK_SYSPLL1_PFD1);
		rate = imx_clk_scmi_set_rate(IMX95_CLK_DRAMALT, clk_val);
		break;
	default:
		printf("No matched freq table %lu\n", clk_val);
		return;
	}

	debug("%s:%llu\n", __func__, rate);

	/* Set DRAM APB to 133Mhz */
	imx_clk_scmi_set_parent(IMX95_CLK_DRAMAPB, IMX95_CLK_SYSPLL1_PFD1_DIV2);
	rate = imx_clk_scmi_set_rate(IMX95_CLK_DRAMAPB, 133333333);

	/* Switch from DRAM clock root from PLL to CCM */
	imx_clk_scmi_set_parent(IMX95_CLK_SEL_DRAM, IMX95_CLK_DRAMALT);
}

void dram_disable_bypass(void)
{
	u64 rate;
	/* Set DRAM APB to 133Mhz */
	imx_clk_scmi_set_parent(IMX95_CLK_DRAMAPB, IMX95_CLK_SYSPLL1_PFD1_DIV2);
	rate = imx_clk_scmi_set_rate(IMX95_CLK_DRAMAPB, 133333333);

	/*Set the DRAM_GPR_SEL to be sourced from DRAM_PLL.*/
	imx_clk_scmi_set_parent(IMX95_CLK_SEL_DRAM, IMX95_CLK_DRAMPLL);
	rate = imx_clk_scmi_get_rate(IMX95_CLK_SEL_DRAM);
	printf("%s:SEL_DRAM: %llu\n", __func__, rate);
}

#endif

unsigned int mxc_get_clock(enum mxc_clock clk)
{
	switch (clk) {
	case MXC_ARM_CLK:
		return get_arm_core_clk();
	case MXC_IPG_CLK:
		return imx_clk_scmi_get_rate(IMX95_CLK_BUSWAKEUP);
	case MXC_CSPI_CLK:
		return imx_clk_scmi_get_rate(IMX95_CLK_LPSPI1);
	case MXC_ESDHC_CLK:
		return imx_clk_scmi_get_rate(IMX95_CLK_USDHC1);
	case MXC_ESDHC2_CLK:
		return imx_clk_scmi_get_rate(IMX95_CLK_USDHC2);
	case MXC_ESDHC3_CLK:
		return imx_clk_scmi_get_rate(IMX95_CLK_USDHC3);
	case MXC_UART_CLK:
		return imx_clk_scmi_get_rate(IMX95_CLK_LPUART1);
	case MXC_FLEXSPI_CLK:
		return imx_clk_scmi_get_rate(IMX95_CLK_FLEXSPI1);
	default:
		return -1;
	};

	return -1;
};

static uint32_t clock_ids[] =
{
	IMX95_CLK_SAI1,
	IMX95_CLK_SAI2,
	IMX95_CLK_SAI3,
	IMX95_CLK_SAI4,
	IMX95_CLK_SAI5,
	IMX95_CLK_SPDIF,
	IMX95_CLK_PDM,
	IMX95_CLK_MQS1,
	IMX95_CLK_MQS2,
};

int board_prep_linux(struct bootm_headers *images)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(clock_ids); i++)
		imx_clk_scmi_enable(clock_ids[i], false);

	return 0;
}
