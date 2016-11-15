/*
 * Copyright 2016 Freescale Semiconductors, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <imx_lpi2c.h>
#include <i2c.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/clock.h>
#include <asm/errno.h>

DECLARE_GLOBAL_DATA_PTR;
#define LPI2C_FIFO_SIZE 4
#define LPI2C_TIMEOUT_MS 100

#ifndef CONFIG_SYS_IMX_I2C1_SPEED
#define CONFIG_SYS_IMX_I2C1_SPEED 100000
#endif
#ifndef CONFIG_SYS_IMX_I2C2_SPEED
#define CONFIG_SYS_IMX_I2C2_SPEED 100000
#endif
#ifndef CONFIG_SYS_IMX_I2C3_SPEED
#define CONFIG_SYS_IMX_I2C3_SPEED 100000
#endif
#ifndef CONFIG_SYS_IMX_I2C4_SPEED
#define CONFIG_SYS_IMX_I2C4_SPEED 100000
#endif
#ifndef CONFIG_SYS_IMX_I2C5_SPEED
#define CONFIG_SYS_IMX_I2C5_SPEED 100000
#endif
#ifndef CONFIG_SYS_IMX_I2C6_SPEED
#define CONFIG_SYS_IMX_I2C6_SPEED 100000
#endif
#ifndef CONFIG_SYS_IMX_I2C7_SPEED
#define CONFIG_SYS_IMX_I2C7_SPEED 100000
#endif
#ifndef CONFIG_SYS_IMX_I2C8_SPEED
#define CONFIG_SYS_IMX_I2C8_SPEED 100000
#endif

#ifndef CONFIG_SYS_IMX_I2C1_SLAVE
#define CONFIG_SYS_IMX_I2C1_SLAVE 0
#endif
#ifndef CONFIG_SYS_IMX_I2C2_SLAVE
#define CONFIG_SYS_IMX_I2C2_SLAVE 0
#endif
#ifndef CONFIG_SYS_IMX_I2C3_SLAVE
#define CONFIG_SYS_IMX_I2C3_SLAVE 0
#endif
#ifndef CONFIG_SYS_IMX_I2C4_SLAVE
#define CONFIG_SYS_IMX_I2C4_SLAVE 0
#endif
#ifndef CONFIG_SYS_IMX_I2C5_SLAVE
#define CONFIG_SYS_IMX_I2C5_SLAVE 0
#endif
#ifndef CONFIG_SYS_IMX_I2C6_SLAVE
#define CONFIG_SYS_IMX_I2C6_SLAVE 0
#endif
#ifndef CONFIG_SYS_IMX_I2C7_SLAVE
#define CONFIG_SYS_IMX_I2C7_SLAVE 0
#endif
#ifndef CONFIG_SYS_IMX_I2C8_SLAVE
#define CONFIG_SYS_IMX_I2C8_SLAVE 0
#endif

static struct imx_lpi2c_reg *imx_lpi2c[] = {
	(struct imx_lpi2c_reg *)LPI2C1_BASE_ADDR,
	(struct imx_lpi2c_reg *)LPI2C2_BASE_ADDR,
	(struct imx_lpi2c_reg *)LPI2C3_BASE_ADDR,
	(struct imx_lpi2c_reg *)LPI2C4_BASE_ADDR,
#if defined(CONFIG_MX7ULP)
	(struct imx_lpi2c_reg *)LPI2C5_BASE_ADDR,
	(struct imx_lpi2c_reg *)LPI2C6_BASE_ADDR,
	(struct imx_lpi2c_reg *)LPI2C7_BASE_ADDR,
	(struct imx_lpi2c_reg *)LPI2C8_BASE_ADDR,
#endif
};

/* Weak linked function for overridden by some SoC power function */
int __weak init_i2c_power(unsigned i2c_num)
{
	return 0;
}

static int imx_lpci2c_check_clear_error(struct i2c_adapter *adap)
{
	struct imx_lpi2c_reg *regs = imx_lpi2c[adap->hwadapnr];
	lpi2c_status_t result = LPI2C_SUCESS;
	u32 val, status;

	status = readl(&regs->msr);
	/* errors to check for */
	status &= LPI2C_MSR_NDF_MASK | LPI2C_MSR_ALF_MASK |
		LPI2C_MSR_FEF_MASK | LPI2C_MSR_PLTF_MASK;

	if (status) {
		if (status & LPI2C_MSR_PLTF_MASK)
			result = LPI2C_PIN_LOW_TIMEOUT_ERR;
		else if (status & LPI2C_MSR_ALF_MASK)
			result = LPI2C_ARB_LOST_ERR;
		else if (status & LPI2C_MSR_NDF_MASK)
			result = LPI2C_NAK_ERR;
		else if (status & LPI2C_MSR_FEF_MASK)
			result = LPI2C_FIFO_ERR;

		/* clear status flags */
		writel(0x7f00, &regs->msr);
		/* reset fifos */
		val = readl(&regs->mcr);
		val |= LPI2C_MCR_RRF_MASK | LPI2C_MCR_RTF_MASK;
		writel(val, &regs->mcr);
	}

	return result;
}

static int imx_lpci2c_check_busy_bus(struct i2c_adapter *adap)
{
	struct imx_lpi2c_reg *regs = imx_lpi2c[adap->hwadapnr];
	lpi2c_status_t result = LPI2C_SUCESS;
	u32 status;

	status = readl(&regs->msr);

	if ((status & LPI2C_MSR_BBF_MASK) && !(status & LPI2C_MSR_MBF_MASK))
		result = LPI2C_BUSY;

	return result;
}

static int bus_i2c_wait_for_tx_ready(struct i2c_adapter *adap)
{
	struct imx_lpi2c_reg *regs = imx_lpi2c[adap->hwadapnr];
	lpi2c_status_t result = LPI2C_SUCESS;
	u32 txcount = 0;
	ulong start_time = get_timer(0);

	do {
		txcount = LPI2C_MFSR_TXCOUNT(readl(&regs->mfsr));
		txcount = LPI2C_FIFO_SIZE - txcount;
		result = imx_lpci2c_check_clear_error(adap);
		if (result) {
			debug("i2c: wait for tx ready: result 0x%x\n", result);
			return result;
		}
		if (get_timer(start_time) > LPI2C_TIMEOUT_MS) {
			debug("i2c: wait for tx ready: timeout\n");
			return -1;
		}
	} while (!txcount);

	return result;
}

static int bus_i2c_start(struct i2c_adapter *adap, u8 addr, u8 dir)
{
	struct imx_lpi2c_reg *regs = imx_lpi2c[adap->hwadapnr];
	lpi2c_status_t result = LPI2C_SUCESS;
	u32 val;

	result = imx_lpci2c_check_busy_bus(adap);
	if (result) {
		debug("i2c: start check busy bus: 0x%x\n", result);
		return result;
	}
	/* clear all status flags */
	writel(0x7f00, &regs->msr);
	/* turn off auto-stop condition */
	val = readl(&regs->mcfgr1) & ~LPI2C_MCFGR1_AUTOSTOP_MASK;
	writel(val, &regs->mcfgr1);
	/* wait tx fifo ready */
	result = bus_i2c_wait_for_tx_ready(adap);
	if (result) {
		debug("i2c: start wait for tx ready: 0x%x\n", result);
		return result;
	}
	/* issue start command */
	val = LPI2C_MTDR_CMD(0x4) | (addr << 0x1) | dir;
	writel(val, &regs->mtdr);

	return result;
}

static int bus_i2c_stop(struct i2c_adapter *adap)
{
	struct imx_lpi2c_reg *regs = imx_lpi2c[adap->hwadapnr];
	lpi2c_status_t result = LPI2C_SUCESS;
	u32 status;

	result = bus_i2c_wait_for_tx_ready(adap);
	if (result) {
		debug("i2c: stop wait for tx ready: 0x%x\n", result);
		return result;
	}

	/* send stop command */
	writel(LPI2C_MTDR_CMD(0x2), &regs->mtdr);

	while (result == LPI2C_SUCESS) {
		status = readl(&regs->msr);
		result = imx_lpci2c_check_clear_error(adap);
		/* stop detect flag */
		if (status & LPI2C_MSR_SDF_MASK) {
			/* clear stop flag */
			status &= LPI2C_MSR_SDF_MASK;
			writel(status, &regs->msr);
			break;
		}
	}

	return result;
}

static int bus_i2c_send(struct i2c_adapter *adap, u8 *txbuf, int len)
{
	struct imx_lpi2c_reg *regs = imx_lpi2c[adap->hwadapnr];
	lpi2c_status_t result = LPI2C_SUCESS;

	/* empty tx */
	if (!len)
		return result;

	while (len--) {
		result = bus_i2c_wait_for_tx_ready(adap);
		if (result) {
			debug("i2c: send wait fot tx ready: %d\n", result);
			return result;
		}
		writel(*txbuf++, &regs->mtdr);
	}

	return result;
}

static int bus_i2c_receive(struct i2c_adapter *adap, u8 *rxbuf, int len)
{
	struct imx_lpi2c_reg *regs = imx_lpi2c[adap->hwadapnr];
	lpi2c_status_t result = LPI2C_SUCESS;
	u32 val;
	ulong start_time = get_timer(0);

	/* empty read */
	if (!len)
		return result;

	result = bus_i2c_wait_for_tx_ready(adap);
	if (result) {
		debug("i2c: receive wait fot tx ready: %d\n", result);
		return result;
	}

	/* clear all status flags */
	writel(0x7f00, &regs->msr);
	/* send receive command */
	val = LPI2C_MTDR_CMD(0x1) | LPI2C_MTDR_DATA(len - 1);
	writel(val, &regs->mtdr);

	while (len--) {
		do {
			result = imx_lpci2c_check_clear_error(adap);
			if (result) {
				debug("i2c: receive check clear error: %d\n", result);
				return result;
			}
			if (get_timer(start_time) > LPI2C_TIMEOUT_MS) {
				debug("i2c: receive mrdr: timeout\n");
				return -1;
			}
			val = readl(&regs->mrdr);
		} while (val & LPI2C_MRDR_RXEMPTY_MASK);
		*rxbuf++ = LPI2C_MRDR_DATA(val);
	}

	return result;
}

static int bus_i2c_read(struct i2c_adapter *adap, u8 chip, u32 addr,
		int alen, u8 *buf, int len)
{
	lpi2c_status_t result = LPI2C_SUCESS;

	result = bus_i2c_start(adap, chip, 0);
	if (result)
		return result;
	result = bus_i2c_send(adap, (u8 *)&addr, 1);
	if (result)
		return result;
	result = bus_i2c_start(adap, chip, 1);
	if (result)
		return result;
	result = bus_i2c_receive(adap, buf, len);
	if (result)
		return result;
	result = bus_i2c_stop(adap);
	if (result)
		return result;

	return result;
}

static int bus_i2c_write(struct i2c_adapter *adap, u8 chip, u32 addr,
		int alen, u8 *buf, int len)
{
	lpi2c_status_t result = LPI2C_SUCESS;

	result = bus_i2c_start(adap, chip, 0);
	if (result)
		return result;
	result = bus_i2c_send(adap, (u8 *)&addr, 1);
	if (result)
		return result;
	result = bus_i2c_send(adap, buf, len);
	if (result)
		return result;
	result = bus_i2c_stop(adap);
	if (result)
		return result;

	return result;
}

static int bus_i2c_set_bus_speed(struct i2c_adapter *adap, int speed)
{
	struct imx_lpi2c_reg *regs = imx_lpi2c[adap->hwadapnr];
	u32 val;
	u32 preescale = 0, best_pre = 0, clkhi = 0;
	u32 best_clkhi = 0, abs_error = 0, rate;
	u32 error = 0xffffffff;
	u32 clock_rate;
	bool mode;
	int i;

	clock_rate = imx_get_i2cclk(adap->hwadapnr);
	if (!clock_rate)
		return -EPERM;

	mode = (readl(&regs->mcr) & LPI2C_MCR_MEN_MASK) >> LPI2C_MCR_MEN_SHIFT;
	/* disable master mode */
	val = readl(&regs->mcr) & ~LPI2C_MCR_MEN_MASK;
	writel(val | LPI2C_MCR_MEN(0), &regs->mcr);

	for (preescale = 1; (preescale <= 128) &&
		(error != 0); preescale = 2 * preescale) {
		for (clkhi = 1; clkhi < 32; clkhi++) {
			if (clkhi == 1)
				rate = (clock_rate / preescale) / (1 + 3 + 2 + 2 / preescale);
			else
				rate = (clock_rate / preescale / (3 * clkhi + 2 + 2 / preescale));

			abs_error = speed > rate ? speed - rate : rate - speed;

			if (abs_error < error) {
				best_pre = preescale;
				best_clkhi = clkhi;
				error = abs_error;
				if (abs_error == 0)
					break;
			}
		}
	}

	/* Standard, fast, fast mode plus and ultra-fast transfers. */
	val = LPI2C_MCCR0_CLKHI(best_clkhi);
	if (best_clkhi < 2)
		val |= LPI2C_MCCR0_CLKLO(3) | LPI2C_MCCR0_SETHOLD(2) | LPI2C_MCCR0_DATAVD(1);
	else
		val |= LPI2C_MCCR0_CLKLO(2 * best_clkhi) | LPI2C_MCCR0_SETHOLD(best_clkhi) |
			LPI2C_MCCR0_DATAVD(best_clkhi / 2);
	writel(val, &regs->mccr0);

	for (i = 0; i < 8; i++) {
		if (best_pre == (1 << i)) {
			best_pre = i;
			break;
		}
	}

	val = readl(&regs->mcfgr1) & ~LPI2C_MCFGR1_PRESCALE_MASK;
	writel(val | LPI2C_MCFGR1_PRESCALE(best_pre), &regs->mcfgr1);

	if (mode) {
		val = readl(&regs->mcr) & ~LPI2C_MCR_MEN_MASK;
		writel(val | LPI2C_MCR_MEN(1), &regs->mcr);
	}

	return 0;
}

static int bus_i2c_setup(struct i2c_adapter *adap, int speed)
{
	int ret;

	/* power up i2c resource */
	ret = init_i2c_power(adap->hwadapnr);
	if (ret) {
		debug("init_i2c_power err = %d\n", ret);
		return ret;
	}

	/* enable i2c clock */
	ret = enable_i2c_clk(1, adap->hwadapnr);
	if (ret) {
		debug("init_i2c_power err = %d\n", ret);
		return ret;
	}

	return 0;
}

static int bus_i2c_init(struct i2c_adapter *adap, int speed)
{
	struct imx_lpi2c_reg *regs = imx_lpi2c[adap->hwadapnr];
	u32 val;
	int ret;

	/* reset peripheral */
	writel(LPI2C_MCR_RST_MASK, &regs->mcr);
	writel(0x0, &regs->mcr);
	/* Disable Dozen mode */
	writel(LPI2C_MCR_DBGEN(0) | LPI2C_MCR_DOZEN(1), &regs->mcr);
	/* host request disable, active high, external pin */
	val = readl(&regs->mcfgr0);
	val &= (~(LPI2C_MCFGR0_HREN_MASK | LPI2C_MCFGR0_HRPOL_MASK |
				LPI2C_MCFGR0_HRSEL_MASK));
	val |= LPI2C_MCFGR0_HRPOL(0x1);
	writel(val, &regs->mcfgr0);
	/* pincfg and ignore ack */
	val = readl(&regs->mcfgr1);
	val &= ~(LPI2C_MCFGR1_PINCFG_MASK | LPI2C_MCFGR1_IGNACK_MASK);
	val |= LPI2C_MCFGR1_PINCFG(0x0); /* 2 pin open drain */
	val |= LPI2C_MCFGR1_IGNACK(0x0); /* ignore nack */
	writel(val, &regs->mcfgr1);

	ret = bus_i2c_set_bus_speed(adap, speed);

	/* enable lpi2c in master mode */
	val = readl(&regs->mcr) & ~LPI2C_MCR_MEN_MASK;
	writel(val | LPI2C_MCR_MEN(1), &regs->mcr);

	debug("i2c : controller bus %d, speed %d:\n", adap->hwadapnr, speed);

	return ret;
}

static void imx_i2c_init(struct i2c_adapter *adap, int speed, int slaveaddr)
{
	int err;
	err = bus_i2c_setup(adap, speed);
	if (err) {
		debug("bus_i2c_setup err = %d\n", err);
		return;
	}
	bus_i2c_init(adap, speed);
}

static int imx_i2c_probe(struct i2c_adapter *adap, uint8_t chip)
{
	lpi2c_status_t result = LPI2C_SUCESS;

	result = bus_i2c_start(adap, chip, 0);
	if (result) {
		bus_i2c_stop(adap);
		bus_i2c_init(adap, 100000);
		return result;
	}

	result = bus_i2c_stop(adap);
	if (result) {
		bus_i2c_init(adap, 100000);
		return -result;
	}

	return result;
}

static int imx_i2c_read(struct i2c_adapter *adap, uint8_t chip,
		uint addr, int alen, uint8_t *buffer, int len)
{
	return bus_i2c_read(adap, chip, addr, alen, buffer, len);
}

static int imx_i2c_write(struct i2c_adapter *adap, uint8_t chip,
		uint addr, int alen, uint8_t *buffer, int len)
{
	return bus_i2c_write(adap, chip, addr, alen, buffer, len);
}

static u32 imx_i2c_set_bus_speed(struct i2c_adapter *adap, uint speed)
{
	return bus_i2c_set_bus_speed(adap, speed);
}

#define LPI2C_ADAPTER(index, speed, slvaddr) \
U_BOOT_I2C_ADAP_COMPLETE(lpi2c##index, imx_i2c_init, imx_i2c_probe, \
		imx_i2c_read, imx_i2c_write, imx_i2c_set_bus_speed, \
		speed, slvaddr, index)

#ifdef CONFIG_SYS_I2C_IMX_LPI2C1
	LPI2C_ADAPTER(0, CONFIG_SYS_IMX_I2C1_SPEED, CONFIG_SYS_IMX_I2C1_SLAVE)
#endif
#ifdef CONFIG_SYS_I2C_IMX_LPI2C2
	LPI2C_ADAPTER(1, CONFIG_SYS_IMX_I2C2_SPEED, CONFIG_SYS_IMX_I2C2_SLAVE)
#endif
#ifdef CONFIG_SYS_I2C_IMX_LPI2C3
	LPI2C_ADAPTER(2, CONFIG_SYS_IMX_I2C3_SPEED, CONFIG_SYS_IMX_I2C3_SLAVE)
#endif
#ifdef CONFIG_SYS_I2C_IMX_LPI2C4
	LPI2C_ADAPTER(3, CONFIG_SYS_IMX_I2C4_SPEED, CONFIG_SYS_IMX_I2C4_SLAVE)
#endif
#ifdef CONFIG_SYS_I2C_IMX_LPI2C5
	LPI2C_ADAPTER(4, CONFIG_SYS_IMX_I2C5_SPEED, CONFIG_SYS_IMX_I2C5_SLAVE)
#endif
#ifdef CONFIG_SYS_I2C_IMX_LPI2C6
	LPI2C_ADAPTER(5, CONFIG_SYS_IMX_I2C6_SPEED, CONFIG_SYS_IMX_I2C6_SLAVE)
#endif
#ifdef CONFIG_SYS_I2C_IMX_LPI2C7
	LPI2C_ADAPTER(6, CONFIG_SYS_IMX_I2C7_SPEED, CONFIG_SYS_IMX_I2C7_SLAVE)
#endif
#ifdef CONFIG_SYS_I2C_IMX_LPI2C8
	LPI2C_ADAPTER(7, CONFIG_SYS_IMX_I2C8_SPEED, CONFIG_SYS_IMX_I2C8_SLAVE)
#endif
