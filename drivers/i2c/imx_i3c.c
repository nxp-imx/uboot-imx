// SPDX-License-Identifier: GPL-2.0+
/*
 * I3C controller driver.
 *
 * Copyright 2021 NXP
 * Author: Clark Wang (xiaoning.wang@nxp.com)
 */

#include <common.h>
#include <clk.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <fdtdec.h>
#include <i2c.h>
#include "imx_i3c.h"

#define I3C_NACK_TOUT_MS	1
#define I3C_TIMEOUT_MS		100

struct imx_i3c_bus {
	int index;
	ulong base;
	ulong driver_data;
	int speed;
	struct gpio_desc switch_gpio;
	struct i2c_pads_info *pads_info;
	struct udevice *bus;
	struct clk per_clk;
	struct clk ipg_clk;
};

static int bus_i3c_init(struct udevice *bus);

/* Weak linked function for overridden by some SoC power function */
int __weak init_i3c_power(unsigned i2c_num)
{
	return 0;
}

int __weak enable_i3c_clk(unsigned char enable, unsigned int i3c_num)
{
	return 0;
}

int __weak imx_get_i3cclk(u32 i3c_num)
{
	return 0;
}

int __weak board_imx_i3c_bind(struct udevice *dev)
{
	return 0;
}

i3c_master_state_t bus_i3c_masterstate(const struct imx_i3c_reg *regs)
{
	u32 masterState = (readl(&regs->mstatus) & I3C_MSTATUS_STATE_MASK) >> I3C_MSTATUS_STATE_SHIFT;
	i3c_master_state_t returnCode;

	switch (masterState) {
	case (u32)I3C_MASTERSTATE_IDLE:
		returnCode = I3C_MASTERSTATE_IDLE;
		break;
	case (u32)I3C_MASTERSTATE_SLVREQ:
		returnCode = I3C_MASTERSTATE_SLVREQ;
		break;
	case (u32)I3C_MASTERSTATE_MSGSDR:
		returnCode = I3C_MASTERSTATE_MSGSDR;
		break;
	case (u32)I3C_MASTERSTATE_NORMACT:
		returnCode = I3C_MASTERSTATE_NORMACT;
		break;
	case (u32)I3C_MASTERSTATE_DDR:
		returnCode = I3C_MASTERSTATE_DDR;
		break;
	case (u32)I3C_MASTERSTATE_DAA:
		returnCode = I3C_MASTERSTATE_DAA;
		break;
	case (u32)I3C_MASTERSTATE_IBIACK:
		returnCode = I3C_MASTERSTATE_IBIACK;
		break;
	case (u32)I3C_MASTERSTATE_IBIRCV:
		returnCode = I3C_MASTERSTATE_IBIRCV;
		break;
	default:
		returnCode = I3C_MASTERSTATE_IDLE;
		break;
	}

	return returnCode;
}

static int imx_i3c_check_busy_bus(const struct imx_i3c_reg *regs)
{
	u32 status;

	status = bus_i3c_masterstate(regs);
	if ((status != I3C_MASTERSTATE_IDLE) && (status != I3C_MASTERSTATE_NORMACT))
		return status;

	return I3C_SUCESS;
}

static int bus_i3c_check_clear_error(struct imx_i3c_reg *regs)
{
	i3c_status_t result = I3C_SUCESS;
	u32 val, status;

	status = readl(&regs->merrwarn);
	/* errors to check for */
	status &= I3C_MERRWARN_NACK_MASK | I3C_MERRWARN_WRABT_MASK |
		I3C_MERRWARN_TERM_MASK | I3C_MERRWARN_HPAR_MASK | I3C_MERRWARN_HCRC_MASK |
		I3C_MERRWARN_OREAD_MASK | I3C_MERRWARN_OWRITE_MASK | I3C_MERRWARN_MSGERR_MASK |
		I3C_MERRWARN_INVREQ_MASK | I3C_MERRWARN_TIMEOUT_MASK;

	if (status) {
		/* Select the correct error code. Ordered by severity, with bus issues first. */
		if (0UL != (status & (u32)I3C_MERRWARN_TIMEOUT_MASK))
			result = I3C_TIMEOUT;
		else if (0UL != (status & (u32)I3C_MERRWARN_NACK_MASK))
			result = I3C_NACK;
		else if (0UL != (status & (u32)I3C_MERRWARN_WRABT_MASK))
			result = I3C_WRITE_ABORT_ERR;
		else if (0UL != (status & (u32)I3C_MERRWARN_TERM_MASK))
			result = I3C_TERM;
		else if (0UL != (status & (u32)I3C_MERRWARN_HPAR_MASK))
			result = I3C_HDR_PARITY_ERR;
		else if (0UL != (status & (u32)I3C_MERRWARN_HCRC_MASK))
			result = I3C_CRC_ERR;
		else if (0UL != (status & (u32)I3C_MERRWARN_MSGERR_MASK))
			result = I3C_MSG_ERR;
		else if (0UL != (status & (u32)I3C_MERRWARN_OREAD_MASK))
			result = I3C_READFIFO_ERR;
		else if (0UL != (status & (u32)I3C_MERRWARN_OWRITE_MASK))
			result = I3C_WRITEFIFO_ERR;
		else if (0UL != (status & (u32)I3C_MERRWARN_INVREQ_MASK))
			result = I3C_INVALID_REQ;

		/* clear status flags */
		writel(status, &regs->merrwarn);

		/* reset fifos */
		val = readl(&regs->mdatactrl);
		val |= I3C_MDATACTRL_FLUSHTB_MASK | I3C_MDATACTRL_FLUSHFB_MASK;
		writel(val, &regs->mdatactrl);
	}

	return result;
}

static int bus_i3c_wait_for_tx_ready(struct imx_i3c_reg *regs, int bytecount)
{
	i3c_status_t result = I3C_SUCESS;
	u32 txcount = 0;
	ulong start_time = get_timer(0);
	size_t txFifoSize =
	2UL << ((readl(&regs->scapabilities) & I3C_SCAPABILITIES_FIFOTX_MASK)
	>> I3C_SCAPABILITIES_FIFOTX_SHIFT);

	do {
		txcount = (readl(&regs->mdatactrl) & I3C_MDATACTRL_TXCOUNT_MASK)
			  >> I3C_MDATACTRL_TXCOUNT_SHIFT;
		txcount = txFifoSize - txcount;

		result = bus_i3c_check_clear_error(regs);
		if (result) {
			debug("i3c: wait for tx ready err: result 0x%x\n", result);
			return result;
		}
		if (get_timer(start_time) > I3C_TIMEOUT_MS) {
			debug("i3c: wait for tx ready err: timeout\n");
			return I3C_TIMEOUT;
		}
	} while (txcount < bytecount);

	return 0;
}

static int bus_i3c_send(struct udevice *bus, u8 *txbuf, int len)
{
	struct imx_i3c_bus *i3c_bus = dev_get_priv(bus);
	struct imx_i3c_reg *regs = (struct imx_i3c_reg *)(i3c_bus->base);
	i3c_status_t result = I3C_SUCESS;
	bool enableWord = false;
	u8 byteCounts = 1;
	ulong start_time;

	/* empty tx */
	if (!len)
		return result;

	/* Send data buffer */
	while (0UL != len) {
		/* Wait until there is room in the fifo. This also checks for errors. */
		result = bus_i3c_wait_for_tx_ready(regs, byteCounts);
		if (I3C_SUCESS != result) {
			return result;
		}

		/* Write byte into I3C master data register. */
		if (len > byteCounts) {
			if (enableWord)
				writel((uint32_t)txbuf[1] << 8UL | (uint32_t)txbuf[0], &regs->mwdatah);
			else
				writel(*txbuf, &regs->mwdatab);
		} else {
			if (enableWord)
				writel((uint32_t)txbuf[1] << 8UL | (uint32_t)txbuf[0], &regs->mwdatahe);
			else
				writel(*txbuf, &regs->mwdatabe);
		}

		txbuf = txbuf + byteCounts;
		len  = len - byteCounts;
	}

	start_time = get_timer(0);
	while (!(readl(&regs->mstatus) & I3C_MSTATUS_COMPLETE_MASK)) {
		if (get_timer(start_time) > I3C_TIMEOUT_MS) {
			dev_err(bus, "i3c: xfer: timeout\n");
			return -1;
		}
	}
	writel(I3C_MSTATUS_COMPLETE_MASK, &regs->mstatus);

	return 0;
}

static int bus_i3c_receive(struct udevice *bus, u8 *rxbuf, int len)
{
	struct imx_i3c_bus *i3c_bus = dev_get_priv(bus);
	struct imx_i3c_reg *regs = (struct imx_i3c_reg *)(i3c_bus->base);
	i3c_status_t result = I3C_SUCESS;
	u32 val;
	ulong start_time = get_timer(0);

	/* empty read */
	if (!len)
		return result;

	result = bus_i3c_wait_for_tx_ready(regs, 1);
	if (result) {
		dev_err(bus, "i3c: receive wait for tx ready: %d\n", result);
		return result;
	}

	/* clear all status flags */
	val = readl(&regs->mstatus);
	writel(val, &regs->mstatus);

	while (len--) {
		do {
			result = bus_i3c_check_clear_error(regs);
			if (result) {
				dev_err(bus, "i3c: receive check clear error: %d\n",
				      result);
				return result;
			}
			if (get_timer(start_time) > I3C_TIMEOUT_MS) {
				dev_err(bus, "i3c: receive mrdr: timeout\n");
				return -1;
			}
			val = readl(&regs->mdatactrl);
		} while (val & I3C_MDATACTRL_RXEMPTY_MASK);
		val = readl(&regs->mrdatab);
		*rxbuf++ = I3C_MRDATAB_VALUE(val);
	}

	start_time = get_timer(0);
	while (!(readl(&regs->mstatus) & I3C_MSTATUS_COMPLETE_MASK)) {
		if (get_timer(start_time) > I3C_TIMEOUT_MS) {
			dev_err(bus, "i3c: xfer: timeout\n");
			return -1;
		}
	}
	writel(I3C_MSTATUS_COMPLETE_MASK, &regs->mstatus);

	return 0;
}

static int bus_i3c_start(struct udevice *bus, u8 addr, u8 dir, u32 rxSize)
{
	struct imx_i3c_bus *i3c_bus = dev_get_priv(bus);
	struct imx_i3c_reg *regs = (struct imx_i3c_reg *)(i3c_bus->base);
	i3c_status_t result;
	ulong start_time = get_timer(0);
	u32 val;

	result = imx_i3c_check_busy_bus(regs);
	if (result) {
		/* Try to init the bus then check the bus busy again */
		bus_i3c_init(bus);
		result = imx_i3c_check_busy_bus(regs);
		if (result) {
			dev_err(bus, "i3c: Error check busy bus: 0x%x\n", result);
			return result;
		}
	}

	/* clear all status flags */
	val = readl(&regs->mstatus);
	writel(val, &regs->mstatus);

	/* wait tx fifo ready */
	result = bus_i3c_wait_for_tx_ready(regs, 1);
	if (result) {
		dev_err(bus, "i3c: start wait for tx ready: 0x%x\n", result);
		return result;
	}

	/* Issue start command. */
	val = readl(&regs->mctrl);
	val &= ~(I3C_MCTRL_TYPE_MASK | I3C_MCTRL_REQUEST_MASK |
		 I3C_MCTRL_DIR_MASK | I3C_MCTRL_ADDR_MASK |
		 I3C_MCTRL_RDTERM_MASK);
	val |= I3C_MCTRL_TYPE(0) | I3C_MCTRL_REQUEST(1) | I3C_MCTRL_DIR(dir) |
	       I3C_MCTRL_ADDR(addr) | I3C_MCTRL_RDTERM(rxSize);
	writel(val, &regs->mctrl);

	while (!(readl(&regs->mstatus) & I3C_MSTATUS_MCTRLDONE_MASK)) {
		if (get_timer(start_time) > I3C_TIMEOUT_MS) {
			dev_err(bus, "i3c: start: timeout\n");
			return -1;
		}
	}

	return 0;
}

static int bus_i3c_stop(struct udevice *bus)
{
	struct imx_i3c_bus *i3c_bus = dev_get_priv(bus);
	struct imx_i3c_reg *regs = (struct imx_i3c_reg *)(i3c_bus->base);
	i3c_status_t result;
	u32 status, val;
	ulong start_time;

	result = bus_i3c_wait_for_tx_ready(regs, 1);
	if (result) {
		dev_err(bus, "i3c: stop wait for tx ready: 0x%x\n", result);
		return result;
	}

	/* send stop command */
	val = readl(&regs->mctrl);
	val &= ~(I3C_MCTRL_REQUEST_MASK | I3C_MCTRL_DIR_MASK |
		 I3C_MCTRL_RDTERM_MASK);
	val |= I3C_MCTRL_REQUEST(0x2);
	writel(val, &regs->mctrl);

	start_time = get_timer(0);
	while (1) {
		status = bus_i3c_masterstate(regs);
		result = bus_i3c_check_clear_error(regs);
		/* idle detect flag */
		if (status == I3C_MASTERSTATE_IDLE) {
			break;
		}

		if (get_timer(start_time) > I3C_NACK_TOUT_MS) {
			dev_err(bus, "stop timeout\n");
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static int bus_i3c_read(struct udevice *bus, u32 chip, u8 *buf, int len)
{
	i3c_status_t result;

	result = bus_i3c_start(bus, chip, 1, len);
	if (result)
		return result;

	result = bus_i3c_receive(bus, &buf[0], len);
	if (result)
		return result;

	return 0;
}

static int bus_i3c_write(struct udevice *bus, u32 chip, u8 *buf, int len)
{
	i3c_status_t result;

	result = bus_i3c_start(bus, chip, 0, 0);
	if (result)
		return result;

	result = bus_i3c_send(bus, &buf[0], len);
	if (result)
		return result;

	return 0;
}

static int bus_i3c_config(struct udevice *bus, struct i2c_msg *msg)
{
	struct imx_i3c_bus *i3c_bus = dev_get_priv(bus);
	struct imx_i3c_reg *regs = (struct imx_i3c_reg *)(i3c_bus->base);
	i3c_status_t result;
	u32 val;

	result = imx_i3c_check_busy_bus(regs);
	if (result)
		return result;

	val = readl(&regs->mctrl);
	val &= ~I3C_MCTRL_IBIRESP_MASK;
	writel(val, &regs->mctrl);

	val = readl(&regs->mdatactrl);
	val |= I3C_MDATACTRL_FLUSHTB_MASK | I3C_MDATACTRL_FLUSHFB_MASK;
	writel(val, &regs->mdatactrl);

	return 0;
}

static int bus_i3c_set_bus_speed(struct udevice *bus)
{
	struct imx_i3c_bus *i3c_bus = dev_get_priv(bus);
	struct imx_i3c_reg *regs = (struct imx_i3c_reg *)(i3c_bus->base);

	u32 div, freq;
	u32 ppBaud, odBaud;
	u32 i3cPPBaud_HZ    = 12500000U;
	/* max is 1.025*i3cPPBaud_HZ */
	u32 i3cPPBaudMax_HZ = i3cPPBaud_HZ / 40U + i3cPPBaud_HZ;
	u32 i3cODBaud_HZ    = 2000000U;
	/* max is 1.1*i3cODBaud_HZ */
	u32 i3cODBaudMax_HZ = i3cODBaud_HZ / 10U + i3cODBaud_HZ;
	u32 sourceClock_Hz, val;
	u8 mode = 0;

	regs = (struct imx_i3c_reg *)devfdt_get_addr(bus);
	if (IS_ENABLED(CONFIG_CLK)) {
		sourceClock_Hz = clk_get_rate(&i3c_bus->per_clk);
		if (sourceClock_Hz <= 0) {
			dev_err(bus, "Failed to get i3c clk: %d\n", sourceClock_Hz);
			return sourceClock_Hz;
		}
	} else {
		sourceClock_Hz = imx_get_i3cclk(dev_seq(bus));
		if (!sourceClock_Hz)
			return -EPERM;
	}

	val = readl(&regs->mconfig);
	mode = (val & I3C_MCONFIG_MSTENA_MASK) >> I3C_MCONFIG_MSTENA_SHIFT;
	/* disable master mode */
	val = val & ~I3C_MCONFIG_MSTENA_MASK;
	writel(val | I3C_MCONFIG_MSTENA(0), &regs->mconfig);
	/* Find out the div to generate target freq */
	freq = sourceClock_Hz / 2;
	/* ppFreq = FCLK / 2 / (PPBAUD + 1)), 0 <= PPBAUD <= 15 */
	div = freq / i3cPPBaud_HZ;
	div = div == 0 ? 1 : div;
	if (freq / div > i3cPPBaudMax_HZ)
		div++;

	if (div > (I3C_MCONFIG_PPBAUD_MASK >> I3C_MCONFIG_PPBAUD_SHIFT) + 1)
		return -EPERM;
	ppBaud = div - 1;
	freq /= div;
	/* odFreq = ppFreq / (ODBAUD + 1), 1 <= ODBAUD <= 255 */
	div = freq / i3cODBaud_HZ;
	div = (div < 2) ? 2 : div;
	if (freq / div > i3cODBaudMax_HZ)
		div++;
	odBaud = div - 1;
	freq /= div;

	val = readl(&regs->mconfig) & ~I3C_MCONFIG_MSTENA_MASK;
	val &= ~(I3C_MCONFIG_PPBAUD_MASK | I3C_MCONFIG_PPLOW_MASK |
		 I3C_MCONFIG_ODBAUD_MASK | I3C_MCONFIG_I2CBAUD_MASK);
	val |= I3C_MCONFIG_PPBAUD(ppBaud) | I3C_MCONFIG_ODBAUD(odBaud) |
	       I3C_MCONFIG_I2CBAUD(0);
	writel(val | I3C_MCONFIG_MSTENA(mode), &regs->mconfig);

	debug("ppBaud=%d, odBaud=%d.\n", ppBaud, odBaud);
	while (readl(&regs->mstatus) & I3C_MSTATUS_SLVSTART_MASK) {
		val = readl(&regs->mstatus) | I3C_MSTATUS_SLVSTART_MASK;
		writel(val, &regs->mstatus);
	}
	return 0;
}

static void bus_i3c_reset(struct udevice *bus)
{
	struct imx_i3c_bus *i3c_bus = dev_get_priv(bus);
	struct imx_i3c_reg *regs = (struct imx_i3c_reg *)(i3c_bus->base);

	writel(readl(&regs->mstatus), &regs->mstatus);
	writel(readl(&regs->merrwarn), &regs->merrwarn);

	/* set watermark */
	writel(I3C_MDATACTRL_FLUSHTB_MASK | I3C_MDATACTRL_FLUSHFB_MASK,
	       &regs->mdatactrl);

	/* reset peripheral */
	writel(0, &regs->mconfig);

	return;
}

static int bus_i3c_init(struct udevice *bus)
{
	struct imx_i3c_bus *i3c_bus = dev_get_priv(bus);
	struct imx_i3c_reg *regs = (struct imx_i3c_reg *)(i3c_bus->base);
	int ret;

	/* reset peripheral */
	writel(0, &regs->mconfig);

	/* set mconfig */
	writel(I3C_MCONFIG_MSTENA(1) | I3C_MCONFIG_DISTO(0) |
		I3C_MCONFIG_HKEEP(0) | I3C_MCONFIG_ODSTOP(1) |
		I3C_MCONFIG_ODHPP(1), &regs->mconfig);

	/* set watermark */
	writel(I3C_MDATACTRL_FLUSHTB_MASK | I3C_MDATACTRL_FLUSHFB_MASK,
	       &regs->mdatactrl);

	ret = bus_i3c_set_bus_speed(bus);
	if (ret)
		return ret;

	dev_dbg(bus, "i3c : controller bus %d, ret:%d:\n", dev_seq(bus), ret);

	return 0;
}

static int imx_i3c_probe_chip(struct udevice *bus, u32 chip,
				u32 chip_flags)
{
	return 0;
}

static int imx_i3c_xfer(struct udevice *bus, struct i2c_msg *msg, int nmsgs)
{
	int ret = 0, ret_stop;

	bus_i3c_init(bus);
	bus_i3c_config(bus, msg);
	if (ret)
		return ret;

	for (; nmsgs > 0; nmsgs--, msg++) {
		if (msg->flags & I2C_M_RD)
			ret = bus_i3c_read(bus, msg->addr, msg->buf, msg->len);
		else {
			ret = bus_i3c_write(bus, msg->addr, msg->buf,
					    msg->len);
			if (ret)
				break;
		}
	}

	if (ret)
		dev_dbg(bus, "%s: error sending\n", __func__);

	ret_stop = bus_i3c_stop(bus);
	if (ret_stop)
		dev_err(bus, "%s: stop bus error\n", __func__);

	ret |= ret_stop;

	bus_i3c_reset(bus);

	return ret;
}

static int imx_i3c_set_bus_speed(struct udevice *bus, unsigned int speed)
{
	return bus_i3c_set_bus_speed(bus);
}

static int imx_i3c_probe(struct udevice *bus)
{
	struct imx_i3c_bus *i3c_bus = dev_get_priv(bus);
	fdt_addr_t addr;
	int ret;

	i3c_bus->driver_data = dev_get_driver_data(bus);

	addr = dev_read_addr(bus);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	i3c_bus->base = addr;
	i3c_bus->index = dev_seq(bus);
	i3c_bus->bus = bus;

	/* power up i3c resource */
	ret = init_i3c_power(dev_seq(bus));
	if (ret) {
		dev_err(bus, "Init i3c power err = %d\n", ret);
		return ret;
	}

	/* enable switch */
	ret = gpio_request_by_name(bus, "switch-gpio", 0, &i3c_bus->switch_gpio,
				   GPIOD_IS_OUT);
	if (ret)
		dev_dbg(bus, "No switch-gpio property\n");
	dm_gpio_set_value(&i3c_bus->switch_gpio, 1);

	if (IS_ENABLED(CONFIG_CLK)) {
		ret = clk_get_by_name(bus, "per", &i3c_bus->per_clk);
		if (ret) {
			dev_err(bus, "Failed to get per clk\n");
			return ret;
		}
		ret = clk_enable(&i3c_bus->per_clk);
		if (ret) {
			dev_err(bus, "Failed to enable per clk\n");
			return ret;
		}

		ret = clk_get_by_name(bus, "ipg", &i3c_bus->ipg_clk);
		if (ret) {
			dev_err(bus, "Failed to get ipg clk\n");
			return ret;
		}
		ret = clk_enable(&i3c_bus->ipg_clk);
		if (ret) {
			dev_err(bus, "Failed to enable ipg clk\n");
			return ret;
		}
	} else {
		/* To i.MX8ULP, only i3c2 can be handled by A core */
		ret = enable_i3c_clk(1, dev_seq(bus));
		if (ret < 0)
			return ret;
	}

	ret = bus_i3c_init(bus);
	if (ret < 0)
		return ret;

	dev_dbg(bus, "i3c : controller bus %d at 0x%lx\n", dev_seq(bus),
		i3c_bus->base);

	return 0;
}

static int imx_i3c_bind(struct udevice *dev)
{
	dev_dbg(dev, "imx_i3c_bind, %s, seq %d\n", dev->name, dev_seq(dev));

	return board_imx_i3c_bind(dev);
}

static const struct dm_i2c_ops imx_i3c_ops = {
	.xfer		= imx_i3c_xfer,
	.probe_chip	= imx_i3c_probe_chip,
	.set_bus_speed	= imx_i3c_set_bus_speed,
};

static const struct udevice_id imx_i3c_ids[] = {
	{ .compatible = "fsl,imx8ulp-i3c", },
	{}
};

U_BOOT_DRIVER(imx_i3c) = {
	.name = "imx_i3c",
	.id = UCLASS_I2C,
	.of_match = imx_i3c_ids,
	.bind = imx_i3c_bind,
	.probe = imx_i3c_probe,
	.priv_auto = sizeof(struct imx_i3c_bus),
	.ops = &imx_i3c_ops,
};
