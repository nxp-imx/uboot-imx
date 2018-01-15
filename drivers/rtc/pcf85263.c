// SPDX-License-Identifier: GPL-2.0+
/*
 * rtc-pcf85263 Driver for the NXP PCF85263 RTC
 *
 * Copyright 2016 Parkeon
 * (C) Copyright 2017 MicroSys Electronics GmbH
 * Copyright 2020 NXP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "pcf85263.h"

#include <common.h>
#include <command.h>
#include <rtc.h>
#include <i2c.h>

#if defined(CONFIG_CMD_DATE)

static pcf85263_t pcf85263;

static uchar rtc_read(uchar reg)
{
	return i2c_reg_read(CONFIG_SYS_I2C_RTC_ADDR, reg);
}

static void rtc_write(uchar reg, uchar val)
{
	i2c_reg_write(CONFIG_SYS_I2C_RTC_ADDR, reg, val);
}

static inline bool pcf85263_century_half(int year)
{
	return (year % 100) >= 50;
}

static int pcf85263_check_osc_stopped(void)
{
	uchar regval = 0;
	int ret = 0;

	regval = rtc_read(PCF85263_REG_RTC_SC);
	ret = regval & PCF85263_REG_RTC_SC_OS ? 1 : 0;
	if (ret)
		puts("PCF85263: Oscillator stop detected, date/time is not reliable.\n");

	return ret;
}

static int pcf85263_read_ram_byte(pcf85263_t *pcf85263)
{
	uchar regval;

	regval = rtc_read(PCF85263_REG_RAM_BYTE);

	pcf85263->century = regval & PCF85263_STATE_CENTURY_MASK;
	pcf85263->century_half = !!(regval & PCF85263_STATE_UPPER_HALF_CENTURY);

	/* Not valid => not initialised yet */
	if (!pcf85263->century) {
		int year;

		regval = rtc_read(PCF85263_REG_RTC_YR);

		pcf85263->century = 2;
		year = bcd2bin(regval) + 1900 + (pcf85263->century - 1) * 100;
		pcf85263->century_half = pcf85263_century_half(year);

		rtc_write(PCF85263_REG_RAM_BYTE, pcf85263->century);
	}

	return 0;
}

static void pcf85263_update_ram_byte(pcf85263_t *pcf85263)
{
	uchar val = pcf85263->century & PCF85263_STATE_CENTURY_MASK;

	if (pcf85263->century_half)
		val |= PCF85263_STATE_UPPER_HALF_CENTURY;

	rtc_write(PCF85263_REG_RAM_BYTE, val);
}

static void pcf85263_update_century(pcf85263_t *pcf85263, int year)
{
	uchar cur_century_half;

	cur_century_half = pcf85263_century_half(year);

	if (cur_century_half == pcf85263->century_half)
		return;

	if (!cur_century_half) /* Year has wrapped around */
		pcf85263->century++;

	pcf85263->century_half = cur_century_half;

	pcf85263_update_ram_byte(pcf85263);
}

static int pcf85263_bcd12h_to_bin24h(uchar regval)
{
	int hr = bcd2bin(regval & 0x1f);
	uchar pm = regval & PCF85263_HR_PM;

	if (hr == 12)
		return pm ? 12 : 0;

	return pm ? hr + 12 : hr;
}

int rtc_get(struct rtc_time *tmp)
{
	const int first = PCF85263_REG_RTC_SC;
	const int last = PCF85263_REG_RTC_YR;
	const int len = last - first + 1;
	uchar regs[len];
	uchar hr_reg;
	int i;

	for (i = 0; i < len; i++)
		regs[i] = rtc_read(first + i);

	if (regs[PCF85263_REG_RTC_SC - first] & PCF85263_REG_RTC_SC_OS) {
		puts("PCF85263: Oscillator stop detected, date/time is not reliable.\n");
		return -1;
	}

	tmp->tm_sec = bcd2bin(regs[PCF85263_REG_RTC_SC - first] & 0x7f);
	tmp->tm_min = bcd2bin(regs[PCF85263_REG_RTC_MN - first] & 0x7f);

	hr_reg = regs[PCF85263_REG_RTC_HR - first];
	if (pcf85263.mode_12h)
		tmp->tm_hour = pcf85263_bcd12h_to_bin24h(hr_reg);
	else
		tmp->tm_hour = bcd2bin(hr_reg & 0x3f);

	tmp->tm_mday = bcd2bin(regs[PCF85263_REG_RTC_DT - first]);
	tmp->tm_wday = bcd2bin(regs[PCF85263_REG_RTC_DW - first]) & 0x7;
	tmp->tm_mon  = bcd2bin(regs[PCF85263_REG_RTC_MO - first]);
	tmp->tm_year = bcd2bin(regs[PCF85263_REG_RTC_YR - first]) + 1900;

	tmp->tm_wday -= 1;
	if (tmp->tm_wday < 0)
		tmp->tm_wday += 7;

	pcf85263_update_century(&pcf85263, tmp->tm_year);

	tmp->tm_year += (pcf85263.century - 1) * 100;

	return 0;
}

int rtc_set(struct rtc_time *tmp)
{
	int i;
	uchar reg;
	int wday = tmp->tm_wday + 1;

	if (wday > 6)
		wday = 0;

	uchar regs[] = {
		bin2bcd(tmp->tm_sec),
		bin2bcd(tmp->tm_min),
		bin2bcd(tmp->tm_hour),                /* 24-hour */
		bin2bcd(tmp->tm_mday),
		bin2bcd(wday),
		bin2bcd(tmp->tm_mon),
		bin2bcd(tmp->tm_year % 100)
	};

	rtc_write(PCF85263_REG_STOPENABLE, PCF85263_REG_STOPENABLE_STOP);
	rtc_write(PCF85263_REG_RESET, PCF85263_REG_RESET_CMD_CPR);

	for (i = 0; i < sizeof(regs); i++)
		rtc_write(PCF85263_REG_RTC_SC + i, regs[i]);

	/* As we have set the time in 24H update the hardware for that */
	if (pcf85263.mode_12h) {
		pcf85263.mode_12h = 0;
		reg = rtc_read(PCF85263_REG_OSC);
		reg &= ~PCF85263_REG_OSC_12H;
		rtc_write(PCF85263_REG_OSC, reg);
	}

	/* Start it again */
	rtc_write(PCF85263_REG_STOPENABLE, 0);

	pcf85263.century = (tmp->tm_year / 100) - 18;
	pcf85263.century_half = pcf85263_century_half(tmp->tm_year);

	pcf85263_update_ram_byte(&pcf85263);

	return 0;
}

void rtc_reset(void)
{
}

void rtc_init(void)
{
	uchar regval;
	uchar propval;

	puts("RTC:   PCF85263\n");

	pcf85263_check_osc_stopped();

	pcf85263_read_ram_byte(&pcf85263);

	/* Determine 12/24H mode */
	regval = rtc_read(PCF85263_REG_OSC);
	pcf85263.mode_12h = !!(regval & PCF85263_REG_OSC_12H);

	/* Set oscilator register */
	regval &= ~PCF85263_REG_OSC_12H; /* keep current 12/24 h setting */

	propval = PCF85263_QUARTZCAP_12p5pF;
	regval |= ((propval << PCF85263_REG_OSC_CL_SHIFT)
			& PCF85263_REG_OSC_CL_MASK);

	propval = PCF85263_QUARTZDRIVE_LOW;
	regval |= ((propval << PCF85263_REG_OSC_OSCD_SHIFT)
			& PCF85263_REG_OSC_OSCD_MASK);

	rtc_write(PCF85263_REG_OSC, regval);

	/* Set function register (RTC mode, 1s tick, clock output static) */
	rtc_write(PCF85263_REG_FUNCTION, PCF85263_REG_FUNCTION_COF_OFF);

	/* Set all interrupts to disabled, level mode */
	rtc_write(PCF85263_REG_INTA_CTL, PCF85263_REG_INTx_CTL_ILP);
	rtc_write(PCF85263_REG_INTB_CTL, PCF85263_REG_INTx_CTL_ILP);

	/* Setup IO pin config register */
	regval = PCF85263_REG_PINIO_CLKDISABLE;
	regval |= (PCF85263_INTAPM_HIGHZ | PCF85263_TSPM_DISABLED);
	rtc_write(PCF85263_REG_PINIO, regval);
}

#endif /* CONFIG_CMD_DATE */
