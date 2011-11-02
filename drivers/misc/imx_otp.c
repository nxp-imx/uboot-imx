/*
 * Copyright (C) 2011 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <linux/types.h>
#include <asm/io.h>
#include <common.h>
#include <asm-arm/arch/regs-ocotp.h>
#include <imx_otp.h>

#define HW_OCOTP_CUSTn(n)	(0x00000400 + (n) * 0x10)
#define BF(value, field)	(((value) << BP_##field) & BM_##field)
#define DEF_RELAX 20

#ifdef CONFIG_IMX_OTP_DEBUG
#define log(a, ...) printf("[%s,%3d]:"a"\n", __func__, __LINE__, ## __VA_ARGS__)
#else
#define log(a, ...)
#endif

static int otp_wait_busy(u32 flags)
{
	int count;
	u32 c;

	for (count = 10000; count >= 0; count--) {
		c = readl(IMX_OTP_BASE + HW_OCOTP_CTRL);
		if (!(c & (BM_OCOTP_CTRL_BUSY | BM_OCOTP_CTRL_ERROR | flags)))
			break;
	}

	if (count < 0) {
		printf("ERROR: otp_wait_busy timeout. 0x%X\n", c);
		/* clear ERROR bit, busy bit will be cleared by controller */
		writel(BM_OCOTP_CTRL_ERROR, IMX_OTP_BASE + HW_OCOTP_CTRL_CLR);
		return -1;
	}

	log("wait busy successful.");
	return 0;
}

static int set_otp_timing(void)
{
	u32 clk_rate = 0;
	u32 relax, strobe_read, strobe_prog;
	u32 timing = 0;

	/* get clock */
	clk_rate = mxc_get_clock(MXC_IPG_CLK);
	if (clk_rate == -1) {
		printf("ERROR: mxc_get_clock failed\n");
		return -1;
	}

	log("clk_rate: %d.", clk_rate);

	relax = clk_rate / (1000000000 / DEF_RELAX) - 1;
	strobe_prog = clk_rate / (1000000000 / 10000) + 2 * (DEF_RELAX + 1) - 1;
	strobe_read = clk_rate / (1000000000 / 40) + 2 * (DEF_RELAX + 1) - 1;

	timing = BF(relax, OCOTP_TIMING_RELAX);
	timing |= BF(strobe_read, OCOTP_TIMING_STROBE_READ);
	timing |= BF(strobe_prog, OCOTP_TIMING_STROBE_PROG);
	log("timing: 0x%X", timing);

	writel(timing, IMX_OTP_BASE + HW_OCOTP_TIMING);

	return 0;
}

static int otp_read_prep(void)
{
	return  (!set_otp_timing()) ? otp_wait_busy(0) : -1;
}

static int otp_read_post(void)
{
	return 0;
}

static int otp_blow_prep(void)
{
	return  (!set_otp_timing()) ? otp_wait_busy(0) : -1;
}

static int otp_blow_post(void)
{
	printf("Reloading shadow registers...\n");
	/* reload all the shadow registers */
	writel(BM_OCOTP_CTRL_RELOAD_SHADOWS,
			IMX_OTP_BASE + HW_OCOTP_CTRL_SET);
	udelay(1);

	return otp_wait_busy(BM_OCOTP_CTRL_RELOAD_SHADOWS);
}

static int fuse_read_addr(u32 addr, u32 *pdata)
{
	u32 ctrl_reg = 0;

#ifdef CONFIG_IMX_OTP_READ_SHADOW_REG
	*pdata = readl(IMX_OTP_BASE + HW_OCOTP_CUSTn(addr));
	printf("Shadow register data: 0x%X\n", *pdata);
#endif

	ctrl_reg = readl(IMX_OTP_BASE + HW_OCOTP_CTRL);
	ctrl_reg &= ~BM_OCOTP_CTRL_ADDR;
	ctrl_reg &= ~BM_OCOTP_CTRL_WR_UNLOCK;
	ctrl_reg |= BF(addr, OCOTP_CTRL_ADDR);
	writel(ctrl_reg, IMX_OTP_BASE + HW_OCOTP_CTRL);

	writel(BM_OCOTP_READ_CTRL_READ_FUSE, IMX_OTP_BASE + HW_OCOTP_READ_CTRL);
	if (otp_wait_busy(0))
		return -1;

	*pdata = readl(IMX_OTP_BASE + HW_OCOTP_READ_FUSE_DATA);
	*pdata = BF_OCOTP_READ_FUSE_DATA_DATA(*pdata);
	return 0;
}

static int fuse_blow_addr(u32 addr, u32 value)
{
	u32 ctrl_reg = 0;

	log("blowing...");

	/* control register */
	ctrl_reg = readl(IMX_OTP_BASE + HW_OCOTP_CTRL);
	ctrl_reg &= ~BM_OCOTP_CTRL_ADDR;
	ctrl_reg |= BF(addr, OCOTP_CTRL_ADDR);
	ctrl_reg |= BF(BV_OCOTP_CTRL_WR_UNLOCK__KEY, OCOTP_CTRL_WR_UNLOCK);
	writel(ctrl_reg, IMX_OTP_BASE + HW_OCOTP_CTRL);

	writel(BF_OCOTP_DATA_DATA(value), IMX_OTP_BASE + HW_OCOTP_DATA);
	if (otp_wait_busy(0))
		return -1;

	/* write postamble */
	udelay(2000);
	return 0;
}

/*
 * read one u32 to indexed fuse
 */
int imx_otp_read_one_u32(u32 index, u32 *pdata)
{
	u32 ctrl_reg = 0;
	int ret = 0;

	log("index: 0x%X", index);
	if (index > IMX_OTP_ADDR_MAX) {
		printf("ERROR: invalid address.\n");
		ret = -1;
		goto exit_nop;
	}

	if (otp_clk_enable()) {
		ret = -1;
		printf("ERROR: failed to initialize OTP\n");
		goto exit_nop;
	}

	if (otp_read_prep()) {
		ret = -1;
		printf("ERROR: read preparation failed\n");
		goto exit_cleanup;
	}
	if (fuse_read_addr(index, pdata)) {
		ret = -1;
		printf("ERROR: read failed\n");
		goto exit_cleanup;
	}
	if (otp_read_post()) {
		ret = -1;
		printf("ERROR: read post operation failed\n");
		goto exit_cleanup;
	}

	if (*pdata == IMX_OTP_DATA_ERROR_VAL) {
		ctrl_reg = readl(IMX_OTP_BASE + HW_OCOTP_CTRL);
		if (ctrl_reg & BM_OCOTP_CTRL_ERROR) {
			printf("ERROR: read fuse failed\n");
			ret = -1;
			goto exit_cleanup;
		}
	}

exit_cleanup:
	otp_clk_disable();
exit_nop:
	return ret;
}

/*
 * blow one u32 to indexed fuse
 */
int imx_otp_blow_one_u32(u32 index, u32 data, u32 *pfused_value)
{
	u32 ctrl_reg = 0;
	int ret = 0;

	if (otp_clk_enable()) {
		ret = -1;
		goto exit_nop;
	}

	if (otp_blow_prep()) {
		ret = -1;
		printf("ERROR: blow preparation failed\n");
		goto exit_cleanup;
	}
	if (fuse_blow_addr(index, data)) {
		ret = -1;
		printf("ERROR: blow fuse failed\n");
		goto exit_cleanup;
	}
	if (otp_blow_post()) {
		ret = -1;
		printf("ERROR: blow post operation failed\n");
		goto exit_cleanup;
	}

	ctrl_reg = readl(IMX_OTP_BASE + HW_OCOTP_CTRL);
	if (ctrl_reg & BM_OCOTP_CTRL_ERROR) {
		ret = -1;
		goto exit_cleanup;
	}

	if (imx_otp_read_one_u32(index, pfused_value)) {
		ret = -1;
		goto exit_cleanup;
	}

exit_cleanup:
	otp_clk_disable();
exit_nop:
	return ret;
}

