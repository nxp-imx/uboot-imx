// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2018 NXP
 */

#include <common.h>
#include <linux/errno.h>
#include <asm/arch/clock.h>

DECLARE_GLOBAL_DATA_PTR;

u32 mxc_get_clock(enum mxc_clock clk)
{
	switch (clk) {
	default:
		printf("Unsupported mxc_clock %d\n", clk);
		break;
	}

	return 0;
}

void enable_usboh3_clk(unsigned char enable)
{
	LPCG_AllClockOn(USB_2_LPCG);
	return;
}

void init_clk_usb3(int index)
{
	sc_err_t err;

	err = sc_pm_clock_enable(-1, SC_R_USB_2, SC_PM_CLK_MISC, true, false);
	if (err != SC_ERR_NONE)
		printf("USB3 set clock failed!, line=%d (error = %d)\n",
			__LINE__, err);

	err = sc_pm_clock_enable(-1, SC_R_USB_2, SC_PM_CLK_MST_BUS, true, false);
	if (err != SC_ERR_NONE)
		printf("USB3 set clock failed!, line=%d (error = %d)\n",
			__LINE__, err);

	err = sc_pm_clock_enable(-1, SC_R_USB_2, SC_PM_CLK_PER, true, false);
	if (err != SC_ERR_NONE)
		printf("USB3 set clock failed!, line=%d (error = %d)\n",
			__LINE__, err);

	LPCG_AllClockOn(USB_3_LPCG);
	return;
}

int cdns3_enable_clks(int index)
{
	init_clk_usb3(index);
	return 0;
}

int cdns3_disable_clks(int index)
{
	sc_err_t err;

	LPCG_AllClockOff(USB_3_LPCG);

	err = sc_pm_clock_enable(-1, SC_R_USB_2, SC_PM_CLK_MISC, false, false);
	if (err != SC_ERR_NONE)
		printf("USB3 disable clock failed!, line=%d (error = %d)\n",
			__LINE__, err);

	err = sc_pm_clock_enable(-1, SC_R_USB_2, SC_PM_CLK_MST_BUS, false, false);
	if (err != SC_ERR_NONE)
		printf("USB3 disable clock failed!, line=%d (error = %d)\n",
			__LINE__, err);

	err = sc_pm_clock_enable(-1, SC_R_USB_2, SC_PM_CLK_PER, false, false);
	if (err != SC_ERR_NONE)
		printf("USB3 disable clock failed!, line=%d (error = %d)\n",
			__LINE__, err);

	return 0;
}
