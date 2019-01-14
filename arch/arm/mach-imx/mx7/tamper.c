/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/arch/snvs.h>

void enable_active_tamper(unsigned int tx, unsigned int rx)
{
	int val;

	printf("start active tamper test on %d -> %d\n", tx, rx);

	/****************************
	 *   Configuring CAAM and SNVS  *
	 ****************************/

	/* Initialize power glitch detector register */
	val = 0x41736166;
	writel(val, SNVS_LPPGDR);

	/* W1C PGD */
	val = readl(SNVS_LPSR) & 0x00000008;
	writel(val, SNVS_LPSR);

	/* Programming ZMK via SW */
	writel(0x11110000, SNVS_LPZMKR0);
	writel(0x22220000, SNVS_LPZMKR1);
	writel(0x33330000, SNVS_LPZMKR2);
	writel(0x44440000, SNVS_LPZMKR3);
	writel(0x55550000, SNVS_LPZMKR4);
	writel(0x66660000, SNVS_LPZMKR5);
	writel(0x77770000, SNVS_LPZMKR6);
	writel(0x88880000, SNVS_LPZMKR7);

	val = readl(SNVS_LPMKCR) | 0xa;
	writel(val, SNVS_LPMKCR);
	val = readl(SNVS_HPCOMR) | 0x1000;
	writel(val, SNVS_HPCOMR);

	val = readl(SNVS_LPMKCR) | 0x10;
	writel(val, SNVS_LPMKCR);

	val = readl(SNVS_HPSVSR);

	/* LP Security Violation is a non-fatal Violation */
	val = 0x40000000;
	writel(val, SNVS_HPSVCR);

	/* Enable SRTC invalidation in case of security violation */
	val = readl(SNVS_LPCR);
	val |= 0x11;
	writel(val, SNVS_LPCR);

	/*********************************
	 *   Configuring active tamper tx output  *
	 *********************************/

	/* Configure LFSR polynomial and seed for active tamper tx */
	val = AT5_POLYSEED;
	writel(val, SNVS_LPAT1CR + (tx - 5) * 4);

	/* Enable active tamper tx external pad */
	val = readl(SNVS_LPATCTLR) | (1 << (tx - 5 + 16));
	writel(val, SNVS_LPATCTLR);

	/* Enable active tamper tx clk 16hz */
	val = readl(SNVS_LPATCLKR);
	val &= ~(3 << (tx - 5) * 4);
	writel(val, SNVS_LPATCLKR);

	/* Enable active tamper tx LFSR */
	val = readl(SNVS_LPATCTLR) | (1 << (tx - 5));
	writel(val, SNVS_LPATCTLR);

	/* Enable glitch filter for external tamper rx */
	if (rx < 2) {
		val = readl(SNVS_LPTGFCR);
		if (rx == 0)
			val |= 0x800000;
		else if (rx == 1)
			val |= 0x80000000;
		writel(val, SNVS_LPTGFCR);
	} else if (rx < 6) {
		val = readl(SNVS_LPTGF1CR);
		val |= 1 << ((rx - 1) * 8 - 1);
		writel(val, SNVS_LPTGF1CR);
	} else {
		val = readl(SNVS_LPTGF2CR);
		val |= 1 << ((rx - 5) * 8 - 1);
		writel(val, SNVS_LPTGF2CR);
	}

	/* Route active tamper tx to external tamper rx */
	if (rx < 8) {
		val = readl(SNVS_LPATRC1R);
		val &= ~(0xf << (rx * 4));
		val |= ((tx - 4) << (rx * 4));
		writel(val, SNVS_LPATRC1R);
	} else {
		val = readl(SNVS_LPATRC2R);
		val &= ~(0xf << ((rx - 8) * 4));
		val |= ((tx - 4) << ((rx - 8) * 4));
		writel(val, SNVS_LPATRC2R);
	}

	/* Enable external tamper rx */
	if (rx < 2) {
		val = readl(SNVS_LPTDCR);
		if (rx == 0)
			val |= 0x200;
		else if (rx == 1)
			val |= 0x400;
		writel(val, SNVS_LPTDCR);
	} else {
		val = readl(SNVS_LPTDC2R);
		val |= 1 << (rx - 2);
		writel(val, SNVS_LPTDC2R);
	}
}

void enable_passive_tamper(unsigned int rx, unsigned int high)
{
	int val;

	printf("start passive tamper test on pin %d\n", rx);

	/****************************
	 *   Configuring CAAM and SNVS  *
	 ****************************/

	/* Initialize power glitch detector register */
	val = 0x41736166;
	writel(val, SNVS_LPPGDR);

	/* W1C PGD */
	val = readl(SNVS_LPSR) & 0x00000008;
	writel(val, SNVS_LPSR);

	/* Programming ZMK via SW */
	writel(0x11111111, SNVS_LPZMKR0);
	writel(0x22222222, SNVS_LPZMKR1);
	writel(0x33333333, SNVS_LPZMKR2);
	writel(0x44444444, SNVS_LPZMKR3);
	writel(0x55555555, SNVS_LPZMKR4);
	writel(0x66666666, SNVS_LPZMKR5);
	writel(0x77777777, SNVS_LPZMKR6);
	writel(0x88888888, SNVS_LPZMKR7);

	val = readl(SNVS_LPMKCR) | 0xa;
	writel(val, SNVS_LPMKCR);
	val = readl(SNVS_HPCOMR) | 0x1000;
	writel(val, SNVS_HPCOMR);

	val = readl(SNVS_LPMKCR) | 0x10;
	writel(val, SNVS_LPMKCR);

	/* LP Security Violation is a non-fatal Violation */
	val = 0x40000000;
	writel(val, SNVS_HPSVCR);

	/* Enable SRTC invalidation in case of security violation */
	val = readl(SNVS_LPCR);
	val |= 0x11;
	writel(val, SNVS_LPCR);

	/*********************************
	 *   Configuring passive tamper rx          *
	 *********************************/

	/* Enable glitch filter for external tamper rx */
	if (rx < 2) {
		val = readl(SNVS_LPTGFCR);
		if (rx == 0)
			val |= 0x800000;
		else if (rx == 1)
			val |= 0x80000000;
		writel(val, SNVS_LPTGFCR);
	} else if (rx < 6) {
		val = readl(SNVS_LPTGF1CR);
		val |= 1 << ((rx - 1) * 8 - 1);
		writel(val, SNVS_LPTGF1CR);
	} else {
		val = readl(SNVS_LPTGF2CR);
		val |= 1 << ((rx - 5) * 8 - 1);
		writel(val, SNVS_LPTGF2CR);
	}

	if (high == 1) {
		/* Set external tampering rx polarity to high and enable tamper */
		if (rx < 2) {
			val = readl(SNVS_LPTDCR);
			if (rx == 0)
				val |= 0x800;
			else if (rx == 1)
				val |= 0x1000;
			writel(val, SNVS_LPTDCR);
		} else {
			val = readl(SNVS_LPTDC2R);
			val |= 1 << (rx - 2 + 16);
			writel(val, SNVS_LPTDC2R);
		}
	}
	/* Enable external tamper rx */
	if (rx < 2) {
		val = readl(SNVS_LPTDCR);
		if (rx == 0)
			val |= 0x200;
		else if (rx == 1)
			val |= 0x400;
		writel(val, SNVS_LPTDCR);
	} else {
		val = readl(SNVS_LPTDC2R);
		val |= 1 << (rx - 2);
		writel(val, SNVS_LPTDC2R);
	}
}

void stop_tamper(int rx)
{
	int val;

	/* stop tamper */
	if (rx < 2) {
		val = readl(SNVS_LPTDCR);
		if (rx == 0)
			val &= ~0x200;
		else if (rx == 1)
			val &= ~0x400;
		writel(val, SNVS_LPTDCR);
	} else {
		val = readl(SNVS_LPTDC2R);
		val &= ~(1 << (rx - 2));
		writel(val, SNVS_LPTDC2R);
	}

	/* clear tamper status */
	if (rx < 2) {
		val = readl(SNVS_LPSR);
		val |= 1 << (rx + 9);
		writel(val, SNVS_LPSR);
	} else if (rx < 10) {
		val = readl(SNVS_LPTDSR);
		val |= 1 << (rx - 2);
		writel(val, SNVS_LPTDSR);
	}
}

static void get_tamper_status(void)
{
	unsigned int lpsr, lptdsr, hpsr, ssm;

	lpsr = readl(SNVS_LPSR);
	lptdsr = readl(SNVS_LPTDSR);
	hpsr = readl(SNVS_HPSR);
	ssm = (hpsr & 0xf00) >> 8;

	if (lpsr & (1 << 9))
		printf("External Tampering 0 Detected\n");
	if (lpsr & (1 << 10))
		printf("External Tampering 1 Detected\n");
	if (lptdsr & (1 << 0))
		printf("External Tampering 2 Detected\n");
	if (lptdsr & (1 << 1))
		printf("External Tampering 3 Detected\n");
	if (lptdsr & (1 << 2))
		printf("External Tampering 4 Detected\n");
	if (lptdsr & (1 << 3))
		printf("External Tampering 5 Detected\n");
	if (lptdsr & (1 << 4))
		printf("External Tampering 6 Detected\n");
	if (lptdsr & (1 << 5))
		printf("External Tampering 7 Detected\n");
	if (lptdsr & (1 << 6))
		printf("External Tampering 8 Detected\n");
	if (lptdsr & (1 << 7))
		printf("External Tampering 9 Detected\n");
	if (!(lpsr & (3 << 9)) && !(lptdsr & 0xff))
		printf("No External Tampering Detected\n");

	if (hpsr & 0x80000000)
		printf("Zeroizable Master Key is clear\n");
	else
		printf("Zeroizable Master Key is not zero\n");

	if (ssm == 0)
		printf("System Security Monitor State: Init\n");
	else if (ssm == 0x8)
		printf("System Security Monitor State: Init Intermediate\n");
	else if (ssm == 0x9)
		printf("System Security Monitor State: Check\n");
	else if (ssm == 0xb)
		printf("System Security Monitor State: Non-Secure\n");
	else if (ssm == 0xd)
		printf("System Security Monitor State: Trusted\n");
	else if (ssm == 0xf)
		printf("System Security Monitor State: Secure\n");
	else if (ssm == 0x3)
		printf("System Security Monitor State: Soft Fail\n");
	else if (ssm == 0x1)
		printf("System Security Monitor State: Hard Fail\n");
	else
		printf("System Security Monitor State: 0x%x\n", ssm);
}

static void clear_tamper_warning(void)
{
	unsigned int lpsr, lptdsr;

	lpsr = readl(SNVS_LPSR);
	lptdsr = readl(SNVS_LPTDSR);

	writel(lpsr, SNVS_LPSR);
	writel(lptdsr, SNVS_LPTDSR);
}

static int do_tamper(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	const char *op = argc >= 2 ? argv[1] : NULL;
	unsigned int tx, rx, high;

	if (argc < 2)
		return CMD_RET_USAGE;

	if (!strcmp(op, "active")) {
		if (argc < 4)
			return CMD_RET_USAGE;

		tx = simple_strtoul(argv[2], NULL, 16);
		rx = simple_strtoul(argv[3], NULL, 16);
		if ((tx > 9) || (tx < 5))
			return CMD_RET_USAGE;
		if ((rx > 9) || (rx == tx))
			return CMD_RET_USAGE;

		enable_active_tamper(tx, rx);

	} else if (!strcmp(op, "passive")) {
		if (argc < 4)
			return CMD_RET_USAGE;

		rx = simple_strtoul(argv[2], NULL, 16);
		if (rx > 9)
			return CMD_RET_USAGE;

		high = simple_strtoul(argv[3], NULL, 16);
		if (high != 0)
			high = 1;
		enable_passive_tamper(rx, high);

	} else if (!strcmp(op, "status")) {
		get_tamper_status();
	} else if (!strcmp(op, "clear")) {
		clear_tamper_warning();
	} else if (!strcmp(op, "stop")) {
		if (argc < 3)
			return CMD_RET_USAGE;

		rx = simple_strtoul(argv[2], NULL, 16);
		if (rx > 9)
			return CMD_RET_USAGE;
		stop_tamper(rx);
	} else {
		return CMD_RET_USAGE;
	}

	return 0;
}

U_BOOT_CMD(
		imx_tamper, CONFIG_SYS_MAXARGS, 0, do_tamper,
		"imx tamper command for setting for test",
		"active <tx rx>  - tx is active tamper pin from 9 ~ 5, \n"
		"    rx pin is from 9 ~ 0 and should not equal to tx pin\n"
		"passive <rx> <high> - rx is passive tamper pin from 9 ~ 0, \n"
		"    high: 1 - high assert, 0 - low assert\n"
		"status - Get tamper status\n"
		"clear - clear tamper warning\n"
		"stop rx - rx is tamper pin to stop\n"
	  );
