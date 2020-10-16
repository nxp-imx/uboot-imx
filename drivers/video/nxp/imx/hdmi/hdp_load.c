/*
 * Copyright 2017-2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>

#include "API_General.h"
#include "scfw_utils.h"

DECLARE_GLOBAL_DATA_PTR;

#define ON  1
#define OFF 0

static void display_set_power(int onoff)
{
	SC_PM_SET_RESOURCE_POWER_MODE(-1, SC_R_DC_0, onoff);
	SC_PM_SET_RESOURCE_POWER_MODE(-1, SC_R_HDMI, onoff);
}

static void display_set_clocks(void)
{
	const sc_pm_clock_rate_t pll = 800000000;
	const sc_pm_clock_rate_t hdmi_core_clock = pll / 4; /* 200 Mhz */
	const sc_pm_clock_rate_t hdmi_bus_clock = pll / 8;  /* 100 Mhz */

	SC_PM_SET_RESOURCE_POWER_MODE(-1,
				      SC_R_HDMI_PLL_0, SC_PM_PW_MODE_OFF);
	SC_PM_SET_CLOCK_RATE(-1,
			     SC_R_HDMI_PLL_0, SC_PM_CLK_PLL, pll);
	SC_PM_SET_RESOURCE_POWER_MODE(-1,
				      SC_R_HDMI_PLL_0, SC_PM_PW_MODE_ON);

	/* HDMI DI Bus Clock  */
	SC_PM_SET_CLOCK_RATE(-1,
			     SC_R_HDMI, SC_PM_CLK_MISC4, hdmi_bus_clock);
	/* HDMI DI Core Clock  */
	SC_PM_SET_CLOCK_RATE(-1,
			     SC_R_HDMI, SC_PM_CLK_MISC2, hdmi_core_clock);
}

static void display_enable_clocks(int enable)
{
	SC_PM_CLOCK_ENABLE(-1, SC_R_HDMI_PLL_0, SC_PM_CLK_PLL, enable);
	SC_PM_CLOCK_ENABLE(-1, SC_R_HDMI, SC_PM_CLK_MISC2, enable);
	SC_PM_CLOCK_ENABLE(-1, SC_R_HDMI, SC_PM_CLK_MISC4, enable);
	if (enable == OFF)
		SC_PM_SET_RESOURCE_POWER_MODE(-1,
				SC_R_HDMI_PLL_0, SC_PM_PW_MODE_OFF);
}

int do_hdp(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	if (argc < 2)
		return 0;

	if (strncmp(argv[1], "tracescfw", 9) == 0) {
		g_debug_scfw = 1;
		printf("Enabled SCFW API tracing\n");
	} else if (strncmp(argv[1], "load", 4) == 0) {
		unsigned long address = 0;
		unsigned long offset  = 0x2000;
		const int iram_size   = 0x10000;
		const int dram_size   = 0x8000;
		const char *s;

		if (argc > 2) {
			address = simple_strtoul(argv[2], NULL, 0);
			if (argc > 3)
				offset = simple_strtoul(argv[3], NULL, 0);
		} else {
			printf("Missing address\n");
		}

		printf("Loading hdp firmware from 0x%016lx offset 0x%016lx\n",
		       address, offset);
		display_set_power(SC_PM_PW_MODE_ON);
		display_set_clocks();
		display_enable_clocks(ON);
		cdn_api_loadfirmware((unsigned char *)(address + offset),
				     iram_size,
				     (unsigned char *)(address + offset +
						       iram_size),
				     dram_size);

		s = env_get("hdp_authenticate_fw");
		if (s && !strcmp(s, "yes"))
			SC_MISC_AUTH(-1, SC_SECO_AUTH_HDMI_TX_FW, 0);

		display_enable_clocks(OFF);
		printf("Loading hdp firmware Complete\n");

		/* do not turn off hdmi power or firmware load will be lost */
	} else {
		printf("test error argc %d\n", argc);
	}

	return 0;
}

/***************************************************/
U_BOOT_CMD(
	hdp,  CONFIG_SYS_MAXARGS, 1, do_hdp,
	"load hdmi firmware ",
	"[<command>] ...\n"
	"hdpload [address] [<offset>]\n"
	"        address - address where the binary image starts\n"
	"        <offset> - IRAM offset in the binary image (8192 default)\n"
	"\n"
	"        if \"hdp_authenticate_fw\" is set to \"yes\", the seco\n"
	"        will authenticate the firmware and load HDCP keys.\n"
	"\n"
	"tracescfw - Trace SCFW API calls for video commands\n"
	);
