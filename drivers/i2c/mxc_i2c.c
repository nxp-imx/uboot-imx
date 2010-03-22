/*
 * i2c driver for Freescale mx31
 *
 * (c) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 *
 * (C) Copyright 2008-2010 Freescale Semiconductor, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

#include <common.h>

#if defined(CONFIG_HARD_I2C)

#define IADR	0x00
#define IFDR	0x04
#define I2CR	0x08
#define I2SR	0x0c
#define I2DR	0x10

#define I2CR_IEN	(1 << 7)
#define I2CR_IIEN	(1 << 6)
#define I2CR_MSTA	(1 << 5)
#define I2CR_MTX	(1 << 4)
#define I2CR_TX_NO_AK	(1 << 3)
#define I2CR_RSTA	(1 << 2)

#define I2SR_ICF	(1 << 7)
#define I2SR_IBB	(1 << 5)
#define I2SR_IIF	(1 << 1)
#define I2SR_RX_NO_AK	(1 << 0)

#ifdef CONFIG_SYS_I2C_PORT
# define I2C_BASE	CONFIG_SYS_I2C_PORT
#else
# error "define CONFIG_SYS_I2C_PORT(I2C base address) to use the I2C driver"
#endif

#define I2C_MAX_TIMEOUT		100000
#define I2C_TIMEOUT_TICKET	1

#undef DEBUG

#ifdef DEBUG
#define DPRINTF(args...)  printf(args)
#else
#define DPRINTF(args...)
#endif

static u16 div[] = { 30, 32, 36, 42, 48, 52, 60, 72, 80, 88, 104, 128, 144,
	160, 192, 240, 288, 320, 384, 480, 576, 640, 768, 960,
	1152, 1280, 1536, 1920, 2304, 2560, 3072, 3840
};

static inline void i2c_reset(void)
{
	__REG16(I2C_BASE + I2CR) = 0;	/* Reset module */
	__REG16(I2C_BASE + I2SR) = 0;
	__REG16(I2C_BASE + I2CR) = I2CR_IEN;
}

void i2c_init(int speed, int unused)
{
	int freq;
	int i;

#ifdef CONFIG_MX31
	freq = mx31_get_ipg_clk();
#else
	freq = mxc_get_clock(MXC_IPG_PERCLK);
#endif
	for (i = 0; i < 0x1f; i++)
		if (freq / div[i] <= speed)
			break;

	DPRINTF("%s: root clock: %d, speed: %d div: %x\n",
		__func__, freq, speed, i);

	__REG16(I2C_BASE + IFDR) = i;
	i2c_reset();
}

static int wait_idle(void)
{
	int timeout = I2C_MAX_TIMEOUT;

	while ((__REG16(I2C_BASE + I2SR) & I2SR_IBB) && --timeout) {
		__REG16(I2C_BASE + I2SR) = 0;
		udelay(I2C_TIMEOUT_TICKET);
	}
	DPRINTF("%s:%x\n", __func__, __REG16(I2C_BASE + I2SR));
	return timeout ? timeout : (!(__REG16(I2C_BASE + I2SR) & I2SR_IBB));
}

static int wait_busy(void)
{
	int timeout = I2C_MAX_TIMEOUT;

	while ((!(__REG16(I2C_BASE + I2SR) & I2SR_IBB) && (--timeout))) {
		__REG16(I2C_BASE + I2SR) = 0;
		udelay(I2C_TIMEOUT_TICKET);
	}
	return timeout ? timeout : (__REG16(I2C_BASE + I2SR) & I2SR_IBB);
}

static int wait_complete(void)
{
	int timeout = I2C_MAX_TIMEOUT;

	while ((!(__REG16(I2C_BASE + I2SR) & I2SR_ICF)) && (--timeout)) {
		__REG16(I2C_BASE + I2SR) = 0;
		udelay(I2C_TIMEOUT_TICKET);
	}
	DPRINTF("%s:%x\n", __func__, __REG16(I2C_BASE + I2SR));
	{
		int i;
		for (i = 0; i < 200; i++)
			udelay(10);

	}
	__REG16(I2C_BASE + I2SR) = 0;	/* clear interrupt */

	return timeout;
}

static int tx_byte(u8 byte)
{
	__REG16(I2C_BASE + I2DR) = byte;

	if (!wait_complete() || __REG16(I2C_BASE + I2SR) & I2SR_RX_NO_AK) {
		DPRINTF("%s:%x <= %x\n", __func__, __REG16(I2C_BASE + I2SR),
			byte);
		return -1;
	}
	DPRINTF("%s:%x\n", __func__, byte);
	return 0;
}

static int rx_byte(u32 *pdata, int last)
{
	if (!wait_complete())
		return -1;

	if (last)
		__REG16(I2C_BASE + I2CR) = I2CR_IEN;

	*pdata = __REG16(I2C_BASE + I2DR);
	DPRINTF("%s:%x\n", __func__, *pdata);
	return 0;
}

int i2c_probe(uchar chip)
{
	int ret;

	__REG16(I2C_BASE + I2CR) = 0;	/* Reset module */
	__REG16(I2C_BASE + I2CR) = I2CR_IEN;
	for (ret = 0; ret < 1000; ret++)
		udelay(1);
	__REG16(I2C_BASE + I2CR) = I2CR_IEN | I2CR_MSTA | I2CR_MTX;
	ret = tx_byte(chip << 1);
	__REG16(I2C_BASE + I2CR) = I2CR_IEN;

	return ret;
}

static int i2c_addr(uchar chip, uint addr, int alen)
{
	int i, retry = 0;
	for (retry = 0; retry < 3; retry++) {
		if (wait_idle())
			break;
		i2c_reset();
		for (i = 0; i < I2C_MAX_TIMEOUT; i++)
			udelay(I2C_TIMEOUT_TICKET);
	}
	if (retry >= 3) {
		printf("%s:bus is busy(%x)\n",
		       __func__, __REG16(I2C_BASE + I2SR));
		return -1;
	}
	__REG16(I2C_BASE + I2CR) = I2CR_IEN | I2CR_MSTA | I2CR_MTX;
	if (!wait_busy()) {
		printf("%s:trigger start fail(%x)\n",
		       __func__, __REG16(I2C_BASE + I2SR));
		return -1;
	}
	if (tx_byte(chip << 1) || (__REG16(I2C_BASE + I2SR) & I2SR_RX_NO_AK)) {
		printf("%s:chip address cycle fail(%x)\n",
		       __func__, __REG16(I2C_BASE + I2SR));
		return -1;
	}
	while (alen--)
		if (tx_byte((addr >> (alen * 8)) & 0xff) ||
		    (__REG16(I2C_BASE + I2SR) & I2SR_RX_NO_AK)) {
			printf("%s:device address cycle fail(%x)\n",
			       __func__, __REG16(I2C_BASE + I2SR));
			return -1;
		}
	return 0;
}

int i2c_read(uchar chip, uint addr, int alen, uchar *buf, int len)
{
	int timeout = I2C_MAX_TIMEOUT;
	uint ret;

	DPRINTF("%s chip: 0x%02x addr: 0x%04x alen: %d len: %d\n",
		__func__, chip, addr, alen, len);

	if (i2c_addr(chip, addr, alen)) {
		printf("i2c_addr failed\n");
		return -1;
	}

	__REG16(I2C_BASE + I2CR) = I2CR_IEN | I2CR_MSTA | I2CR_MTX | I2CR_RSTA;

	if (tx_byte(chip << 1 | 1) ||
	    (__REG16(I2C_BASE + I2SR) & I2SR_RX_NO_AK)) {
		printf("%s:Send 2th chip address fail(%x)\n",
		       __func__, __REG16(I2C_BASE + I2SR));
		return -1;
	}
	__REG16(I2C_BASE + I2CR) = I2CR_IEN | I2CR_MSTA |
	    ((len == 1) ? I2CR_TX_NO_AK : 0);
	DPRINTF("CR=%x\n", __REG16(I2C_BASE + I2CR));
	ret = __REG16(I2C_BASE + I2DR);

	while (len--) {
		if (len == 0)
			__REG16(I2C_BASE + I2CR) = I2CR_IEN | I2CR_MSTA |
			    I2CR_TX_NO_AK;

		if (rx_byte(&ret, len == 0) < 0) {
			printf("Read: rx_byte fail\n");
			return -1;
		}
		*buf++ = ret;
	}

	while (__REG16(I2C_BASE + I2SR) & I2SR_IBB && --timeout) {
		__REG16(I2C_BASE + I2SR) = 0;
		udelay(I2C_TIMEOUT_TICKET);
	}
	if (!timeout) {
		printf("%s:trigger stop fail(%x)\n",
		       __func__, __REG16(I2C_BASE + I2SR));
	}
	return 0;
}

int i2c_write(uchar chip, uint addr, int alen, uchar *buf, int len)
{
	int timeout = I2C_MAX_TIMEOUT;
	DPRINTF("%s chip: 0x%02x addr: 0x%04x alen: %d len: %d\n",
		__func__, chip, addr, alen, len);

	if (i2c_addr(chip, addr, alen))
		return -1;

	while (len--)
		if (tx_byte(*buf++))
			return -1;

	__REG16(I2C_BASE + I2CR) = I2CR_IEN;

	while (__REG16(I2C_BASE + I2SR) & I2SR_IBB && --timeout)
		udelay(I2C_TIMEOUT_TICKET);

	return 0;
}

#endif				/* CONFIG_HARD_I2C */
