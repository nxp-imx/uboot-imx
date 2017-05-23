/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <linux/errno.h>
#include <asm/arch/clock.h>
#include <asm/imx-common/sci/sci.h>
#include <asm/arch/imx8-pins.h>
#include <asm/arch/i2c.h>
#include <asm/arch/sys_proto.h>

DECLARE_GLOBAL_DATA_PTR;

u32 get_lpuart_clk(void)
{
	return mxc_get_clock(MXC_UART_CLK);
}

static u32 get_arm_main_clk(void)
{
	sc_err_t err;
	sc_pm_clock_rate_t clkrate;

	if (is_imx8qm())
		err = sc_pm_get_clock_rate((sc_ipc_t)gd->arch.ipc_channel_handle,
				SC_R_A53, SC_PM_CLK_CPU, &clkrate);
	else if (is_imx8qxp())
		err = sc_pm_get_clock_rate((sc_ipc_t)gd->arch.ipc_channel_handle,
				SC_R_A35, SC_PM_CLK_CPU, &clkrate);
	else
		err = SC_ERR_UNAVAILABLE;

	if (err != SC_ERR_NONE) {
		printf("sc get ARM clk failed! err=%d\n", err);
		return 0;
	}
	return clkrate;
}

unsigned int mxc_get_clock(enum mxc_clock clk)
{
	sc_err_t err;
	sc_pm_clock_rate_t clkrate;

	switch (clk) {
	case MXC_UART_CLK:
		err = sc_pm_get_clock_rate((sc_ipc_t)gd->arch.ipc_channel_handle,
				SC_R_UART_0, 2, &clkrate);
		if (err != SC_ERR_NONE) {
			printf("sc get UART clk failed! err=%d\n", err);
			return 0;
		}
		return clkrate;
	case MXC_ESDHC_CLK:
		err = sc_pm_get_clock_rate((sc_ipc_t)gd->arch.ipc_channel_handle,
				SC_R_SDHC_0, 2, &clkrate);
		if (err != SC_ERR_NONE) {
			printf("sc get uSDHC1 clk failed! err=%d\n", err);
			return 0;
		}
		return clkrate;
	case MXC_ESDHC2_CLK:
		err = sc_pm_get_clock_rate((sc_ipc_t)gd->arch.ipc_channel_handle,
				SC_R_SDHC_1, 2, &clkrate);
		if (err != SC_ERR_NONE) {
			printf("sc get uSDHC2 clk failed! err=%d\n", err);
			return 0;
		}
		return clkrate;
	case MXC_ESDHC3_CLK:
		err = sc_pm_get_clock_rate((sc_ipc_t)gd->arch.ipc_channel_handle,
				SC_R_SDHC_2, 2, &clkrate);
		if (err != SC_ERR_NONE) {
			printf("sc get uSDHC3 clk failed! err=%d\n", err);
			return 0;
		}
		return clkrate;
	case MXC_FEC_CLK:
		err = sc_pm_get_clock_rate((sc_ipc_t)gd->arch.ipc_channel_handle,
				SC_R_ENET_0, 2, &clkrate);
		if (err != SC_ERR_NONE) {
			printf("sc get ENET clk failed! err=%d\n", err);
			return 0;
		}
		return clkrate;
	case MXC_ARM_CLK:
		return get_arm_main_clk();
	default:
		printf("Unsupported mxc_clock %d\n", clk);
		break;
	}

	return 0;
}

u32 imx_get_fecclk(void)
{
	return mxc_get_clock(MXC_FEC_CLK);
}

int enable_i2c_clk(unsigned char enable, unsigned i2c_num)
{
	sc_ipc_t ipc;
	sc_err_t err;

	if (i2c_num >= ARRAY_SIZE(imx_i2c_desc))
		return -EINVAL;

	ipc = gd->arch.ipc_channel_handle;

	if (enable)
		err = sc_pm_clock_enable(ipc,
			imx_i2c_desc[i2c_num].rsrc, 2, true, false);
	else
		err = sc_pm_clock_enable(ipc,
			imx_i2c_desc[i2c_num].rsrc, 2, false, false);

	if (err != SC_ERR_NONE) {
		printf("i2c clock error %d\n", err);
		return -EPERM;
	}

	return 0;
}

u32 imx_get_i2cclk(unsigned i2c_num)
{
	sc_err_t err;
	sc_ipc_t ipc;
	u32 clock_rate;

	if (i2c_num >= ARRAY_SIZE(imx_i2c_desc))
		return 0;

	ipc = gd->arch.ipc_channel_handle;
	err = sc_pm_get_clock_rate(ipc, imx_i2c_desc[i2c_num].rsrc, 2,
		&clock_rate);
	if (err != SC_ERR_NONE)
		return 0;

	return clock_rate;
}

int set_clk_qspi(void)
{
	u32 err;
	u32 rate = 40000000;
	sc_ipc_t ipc;

	ipc = gd->arch.ipc_channel_handle;

	err = sc_pm_set_clock_rate(ipc, SC_R_QSPI_0, SC_PM_CLK_PER, &rate);
	if (err != SC_ERR_NONE) {
		printf("\nqspi set  clock rate (error = %d)\n", err);
		return -EPERM;
	}

	err = sc_pm_clock_enable(ipc, SC_R_QSPI_0 , SC_PM_CLK_PER, true, false);
	if (err != SC_ERR_NONE) {
		printf("\nqspi enable clock enable (error = %d)\n", err);
		return -EPERM;
	}

	return 0;
}

void enable_usboh3_clk(unsigned char enable)
{
	sc_err_t err;
	sc_ipc_t ipc;
	sc_rsrc_t usbs[] = {SC_R_USB_0, SC_R_USB_1, SC_R_USB_2};

	int i;

	ipc = gd->arch.ipc_channel_handle;
	if (enable) {
		for (i = 0; i < 2; i++) {
			/* The 24Mhz OTG PHY clock is pd linked, so it has been power up when pd is on */
			err = sc_pm_clock_enable(ipc, usbs[i], SC_PM_CLK_PHY, true, false);
			if (err != SC_ERR_NONE)
				printf("\nSC_R_USB_%d enable clock enable failed! (error = %d)\n", i, err);
		}
	} else {
		for (i = 0; i < 2; i++) {
			err = sc_pm_clock_enable(ipc, usbs[i], SC_PM_CLK_PHY, false, false);
			if (err != SC_ERR_NONE)
				printf("\nSC_R_USB_%d enable clock disable failed! (error = %d)\n", i, err);
		}
	}

	return;
}

void init_clk_usdhc(u32 index)
{
#ifdef CONFIG_IMX8QM
	sc_rsrc_t usdhcs[] = {SC_R_SDHC_0, SC_R_SDHC_1, SC_R_SDHC_2};
	u32 instances = 3;
#else
	sc_rsrc_t usdhcs[] = {SC_R_SDHC_0, SC_R_SDHC_1};
	u32 instances = 2;
#endif

	sc_err_t err;
	sc_ipc_t ipc;
	sc_pm_clock_rate_t actual = 200000000;

	ipc = gd->arch.ipc_channel_handle;

	if (index >= instances)
		return;

	/* Power on the usdhc */
	err = sc_pm_set_resource_power_mode(ipc, usdhcs[index],
				SC_PM_PW_MODE_ON);
	if (err != SC_ERR_NONE) {
		printf("SDHC_%d Power on failed! (error = %d)\n", index, err);
		return;
	}

	err = sc_pm_set_clock_rate(ipc, usdhcs[index], 2, &actual);
	if (err != SC_ERR_NONE) {
		printf("SDHC_%d set clock failed! (error = %d)\n", index, err);
		return;
	}

	if (actual != 200000000)
		debug("Actual rate for SDHC_%d is %d\n", index, actual);

	err = sc_pm_clock_enable(ipc, usdhcs[index], SC_PM_CLK_PER, true, false);
	if (err != SC_ERR_NONE) {
		printf("SDHC_%d per clk enable failed!\n", index);
		return;
	}
}
