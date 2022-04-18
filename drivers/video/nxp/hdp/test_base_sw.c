/******************************************************************************
 *
 * Copyright (C) 2016-2017 Cadence Design Systems, Inc.
 * All rights reserved worldwide.
 *
 * Copyright 2017-2018 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************
 *
 * test_base_sw.c
 *
 ******************************************************************************
 */

#ifndef __UBOOT__
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#else
#include <common.h>
#include <asm/io.h>
#include <linux/delay.h>

#ifdef CONFIG_ARCH_IMX8M
/* mscale */
#define HDMI_BASE     0x32c00000
#define HDMI_PHY_BASE 0x32c80000
#define HDMI_SEC_BASE 0x32e40000
#endif
#ifdef CONFIG_ARCH_IMX8
/* QM */
#define HDMI_BASE 0x56268000
#define HDMI_SEC_BASE 0x56269000
#define HDMI_OFFSET_ADDR 0x56261008
#define HDMI_SEC_OFFSET_ADDR 0x5626100c

#define HDMI_RX_BASE 0x58268000
#define HDMI_RX_SEC_BASE 0x58269000
#define HDMI_RX_OFFSET_ADDR 0x58261004
#define HDMI_RX_SEC_OFFSET_ADDR 0x58261008
#endif

#ifdef CONFIG_ARCH_LS1028A
#define HDMI_BASE	0xf200000
#endif
#endif

#ifdef CONFIG_ARCH_IMX8M
int cdn_apb_read(unsigned int addr, unsigned int *value)
{
	unsigned int temp;
	u64 tmp_addr = addr + HDMI_BASE;

	temp = __raw_readl(tmp_addr);
	*value = temp;
	return 0;
}

int cdn_apb_write(unsigned int addr, unsigned int value)
{
	u64 tmp_addr = addr + HDMI_BASE;

	__raw_writel(value, tmp_addr);
	return 0;
}

int cdn_sapb_read(unsigned int addr, unsigned int *value)
{
	unsigned int temp;
	u64 tmp_addr = addr + HDMI_SEC_BASE;

	temp = __raw_readl(tmp_addr);
	*value = temp;
	return 0;
}

int cdn_sapb_write(unsigned int addr, unsigned int value)
{
	u64 tmp_addr = addr + HDMI_SEC_BASE;

	__raw_writel(value, tmp_addr);
	return 0;
}

void cdn_sleep(uint32_t ms)
{
	mdelay(ms);
}

void cdn_usleep(uint32_t us)
{
	udelay(us);
}
#endif
#ifdef CONFIG_ARCH_IMX8
int cdn_apb_read(unsigned int addr, unsigned int *value)
{
	unsigned int temp;
	u64 tmp_addr = (addr & 0xfff) + HDMI_BASE;

	__raw_writel(addr >> 12, HDMI_OFFSET_ADDR);

	temp = __raw_readl(tmp_addr);
	*value = temp;
	return 0;
}

int cdn_apb_write(unsigned int addr, unsigned int value)
{
	u64 tmp_addr = (addr & 0xfff) + HDMI_BASE;

	__raw_writel(addr >> 12, HDMI_OFFSET_ADDR);
	__raw_writel(value, tmp_addr);

	return 0;
}

int cdn_sapb_read(unsigned int addr, unsigned int *value)
{
	unsigned int temp;
	u64 tmp_addr = (addr & 0xfff) + HDMI_SEC_BASE;

	__raw_writel(addr >> 12, HDMI_SEC_OFFSET_ADDR);

	temp = __raw_readl(tmp_addr);
	*value = temp;
	return 0;
}

int cdn_sapb_write(unsigned int addr, unsigned int value)
{
	u64 tmp_addr = (addr & 0xfff) + HDMI_SEC_BASE;

	__raw_writel(addr >> 12, HDMI_SEC_OFFSET_ADDR);
	__raw_writel(value, tmp_addr);

	return 0;
}

int hdp_rx_apb_read(unsigned int addr, unsigned int *value)
{
	unsigned int temp;
	u64 tmp_addr = (addr & 0xfff) + HDMI_RX_BASE;

	__raw_writel(addr >> 12, HDMI_RX_OFFSET_ADDR);

	temp = __raw_readl(tmp_addr);

	*value = temp;
	return 0;
}

int hdp_rx_apb_write(unsigned int addr, unsigned int value)
{
	u64 tmp_addr = (addr & 0xfff) + HDMI_RX_BASE;

	__raw_writel(addr >> 12, HDMI_RX_OFFSET_ADDR);

	__raw_writel(value, tmp_addr);

	return 0;
}

int hdp_rx_sapb_read(unsigned int addr, unsigned int *value)
{
	unsigned int temp;
	u64 tmp_addr = (addr & 0xfff) + HDMI_RX_SEC_BASE;

	__raw_writel(addr >> 12, HDMI_RX_SEC_OFFSET_ADDR);

	temp = __raw_readl(tmp_addr);
	*value = temp;
	return 0;
}

int hdp_rx_sapb_write(unsigned int addr, unsigned int value)
{
	u64 tmp_addr = (addr & 0xfff) + HDMI_RX_SEC_BASE;

	__raw_writel(addr >> 12, HDMI_RX_SEC_OFFSET_ADDR);
	__raw_writel(value, tmp_addr);

	return 0;
}

void cdn_sleep(uint32_t ms)
{
	mdelay(ms);
}

void cdn_usleep(uint32_t us)
{
	udelay(us);
}
#endif

#ifdef CONFIG_ARCH_LS1028A
int cdn_apb_read(unsigned int addr, unsigned int *value)
{
	unsigned int temp;
	u64 tmp_addr = addr + HDMI_BASE;

	temp = __raw_readl(tmp_addr);
	*value = temp;
	return 0;
}

int cdn_apb_write(unsigned int addr, unsigned int value)
{
	u64 tmp_addr = addr + HDMI_BASE;

	__raw_writel(value, tmp_addr);
	return 0;
}
#endif
