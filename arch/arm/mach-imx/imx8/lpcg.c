/*
 * Copyright 2017-2019 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <asm/arch/lpcg.h>

#define LPCG_CLOCK_MASK         0x3U
#define LPCG_CLOCK_OFF          0x0U
#define LPCG_CLOCK_ON           0x2U
#define LPCG_CLOCK_AUTO         0x3U
#define LPCG_CLOCK_STOP         0x8U

#define LPCG_ALL_CLOCK_OFF      0x00000000U
#define LPCG_ALL_CLOCK_ON       0x22222222U
#define LPCG_ALL_CLOCK_AUTO     0x33333333U
#define LPCG_ALL_CLOCK_STOP     0x88888888U

static inline void lpcg_write(u32 lpcgVal, ulong lpcg_addr)
{
	/*
	* Write twice with 4x DSC clock cycles (40x IPS clock cycles) interval
	* to work around LPCG issue
	*/
	writel(lpcgVal, lpcg_addr);
	udelay(10); /* 10us is enough. Worst case is 40x IPS cycle (200Mhz) */
	writel(lpcgVal, lpcg_addr);
	udelay(10);
}

void lpcg_clock_off(u32 lpcg_addr, u8 clk)
{
	u32 lpcgVal;

	/* Read from LPCG */
	lpcgVal = readl((ulong)lpcg_addr);

	/* Modify */
	lpcgVal &= ~((u32)(LPCG_CLOCK_MASK) << (clk * 4U));
	lpcgVal |= ((u32)(LPCG_CLOCK_OFF) << (clk * 4U));

	/* Write to LPCG */
	lpcg_write(lpcgVal, (ulong)lpcg_addr);
}

void lpcg_clock_on(u32 lpcg_addr, u8 clk)
{
	u32 lpcgVal;

	/* Read from LPCG */
	lpcgVal = readl((ulong)lpcg_addr);

	/* Modify */
	lpcgVal &= ~((u32)(LPCG_CLOCK_MASK) << (clk * 4U));
	lpcgVal |= ((u32)(LPCG_CLOCK_ON) << (clk * 4U));

	/* Write to LPCG */
	lpcg_write(lpcgVal, (ulong)lpcg_addr);
}

bool lpcg_is_clock_on(u32 lpcg_addr, u8 clk)
{
	u32 lpcgVal;

	/* Read from LPCG */
	lpcgVal = readl((ulong)lpcg_addr);
	lpcgVal = (lpcgVal >> (clk * 4U)) & (u32)(LPCG_CLOCK_MASK);

	if (lpcgVal == LPCG_CLOCK_ON)
		return true;

	return false;
}

void lpcg_clock_autogate(u32 lpcg_addr, u8 clk)
{
	u32 lpcgVal;

	/* Read from LPCG */
	lpcgVal = readl((ulong)lpcg_addr);

	/* Modify */
	lpcgVal &= ~((u32)(LPCG_CLOCK_MASK) << (clk * 4U));
	lpcgVal |= ((u32)(LPCG_CLOCK_AUTO) << (clk * 4U));

	/* Write to LPCG */
	lpcg_write(lpcgVal, (ulong)lpcg_addr);
}

void lpcg_all_clock_off(u32 lpcg_addr)
{
	/* Write to LPCG */
	lpcg_write(LPCG_ALL_CLOCK_OFF, (ulong)lpcg_addr);
}

void lpcg_all_clock_on(u32 lpcg_addr)
{
	/* Write to LPCG */
	lpcg_write(LPCG_ALL_CLOCK_ON, (ulong)lpcg_addr);

	/* Wait for clocks to start */
	while ((readl((ulong)lpcg_addr) & LPCG_ALL_CLOCK_STOP) != 0U)
	{
	}
}

void lpcg_all_clock_autogate(u32 lpcg_addr)
{
	/* Write to LPCG */
	lpcg_write(LPCG_ALL_CLOCK_AUTO, (ulong)lpcg_addr);
}
