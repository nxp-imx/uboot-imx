// SPDX-License-Identifier: GPL-2.0+
/*
 * The NXP PCF2131 RTC uboot driver.
 * Copyright 2022 NXP
 * Date & Time support for PCF2131 RTC
 */

/*      #define DEBUG   */

#include <common.h>
#include <command.h>
#include <dm.h>
#include <i2c.h>
#include <log.h>
#include <rtc.h>

#define PCF2131_REG_CTRL1               0x00
#define PCF2131_BIT_CTRL1_STOP          BIT(5)
#define PCF2131_BIT_CTRL1_100TH_S_DIS   BIT(4)
#define PCF2131_REG_CTRL2               0x01
#define PCF2131_REG_CTRL3               0x02
#define PCF2131_REG_SR_RESET            0x05
#define PCF2131_SR_VAL_Clr_Pres         0xa4
#define PCF2131_REG_SC                  0x07
#define PCF2131_REG_MN                  0x08
#define PCF2131_REG_HR                  0x09
#define PCF2131_REG_DM                  0x0a
#define PCF2131_REG_DW                  0x0b
#define PCF2131_REG_MO                  0x0c
#define PCF2131_REG_YR                  0x0d

static int pcf2131_rtc_read(struct udevice *dev, uint offset, u8 *buffer, uint len)
{
	struct dm_i2c_chip *chip = dev_get_parent_plat(dev);
	struct i2c_msg msg;
	int ret;

	/* Set the address of the start register to be read */
	ret = dm_i2c_write(dev, offset, NULL, 0);
	if (ret < 0)
		return ret;

	/* Read register's data */
	msg.addr = chip->chip_addr;
	msg.flags |= I2C_M_RD;
	msg.len = len;
	msg.buf = buffer;

	return dm_i2c_xfer(dev, &msg, 1);
}

static int pcf2131_rtc_lock(struct udevice *dev)
{
	int ret = 0;
	uchar buf[6] = { PCF2131_REG_CTRL1 };

	ret = pcf2131_rtc_read(dev, PCF2131_REG_CTRL1, buf, sizeof(buf));
	if (ret < 0)
		return ret;

	buf[PCF2131_REG_CTRL1] |= PCF2131_BIT_CTRL1_STOP;
	ret = dm_i2c_write(dev, PCF2131_REG_CTRL1, &buf[PCF2131_REG_CTRL1], 1);
	if (ret < 0)
		return ret;

	buf[PCF2131_REG_SR_RESET] = PCF2131_SR_VAL_Clr_Pres;
	ret = dm_i2c_write(dev, PCF2131_REG_SR_RESET, &buf[PCF2131_REG_SR_RESET], 1);
	return ret;
}

static int pcf2131_rtc_unlock(struct udevice *dev)
{
	int ret = 0;
	uchar buf[6] = { PCF2131_REG_CTRL1 };

	ret = pcf2131_rtc_read(dev, PCF2131_REG_CTRL1, buf, sizeof(buf));
	if (ret < 0)
		return ret;

	buf[PCF2131_REG_CTRL1] &= ~PCF2131_BIT_CTRL1_STOP;
	ret = dm_i2c_write(dev, PCF2131_REG_CTRL1, &buf[PCF2131_REG_CTRL1], 1);
	return ret;
}

static int pcf2131_rtc_write(struct udevice *dev, uint offset,
			     const u8 *buffer, uint len)
{
	int ret = 0;

	ret = pcf2131_rtc_lock(dev);
	if (ret < 0)
		return ret;

	ret = dm_i2c_write(dev, offset, buffer, len);
	if (ret < 0)
		return ret;

	ret = pcf2131_rtc_unlock(dev);
	return ret;
}

static int pcf2131_rtc_set(struct udevice *dev, const struct rtc_time *tm)
{
	uchar buf[7] = {0};
	int i = 0, ret;

	/* hours, minutes and seconds */
	buf[i++] = bin2bcd(tm->tm_sec);
	buf[i++] = bin2bcd(tm->tm_min);
	buf[i++] = bin2bcd(tm->tm_hour);
	buf[i++] = bin2bcd(tm->tm_mday);
	buf[i++] = tm->tm_wday & 0x07;

	/* month, 1 - 12 */
	buf[i++] = bin2bcd(tm->tm_mon);

	/* year */
	buf[i++] = bin2bcd(tm->tm_year % 100);

	ret = pcf2131_rtc_lock(dev);
	if (ret < 0)
		return ret;

	/* write register's data */
	ret = dm_i2c_write(dev, PCF2131_REG_SC, buf, i);
	if (ret < 0)
		return ret;

	ret = pcf2131_rtc_unlock(dev);
	return ret;
}

static int pcf2131_rtc_get(struct udevice *dev, struct rtc_time *tm)
{
	int ret = 0;
	uchar buf[16] = { PCF2131_REG_CTRL1 };

	ret = pcf2131_rtc_read(dev, PCF2131_REG_CTRL1, buf, sizeof(buf));
	if (ret < 0)
		return ret;

	if (buf[PCF2131_REG_CTRL3] & 0x04)
		puts("### Warning: RTC Low Voltage - date/time not reliable\n");

	tm->tm_sec  = bcd2bin(buf[PCF2131_REG_SC] & 0x7F);
	tm->tm_min  = bcd2bin(buf[PCF2131_REG_MN] & 0x7F);
	tm->tm_hour = bcd2bin(buf[PCF2131_REG_HR] & 0x3F);
	tm->tm_mday = bcd2bin(buf[PCF2131_REG_DM] & 0x3F);
	tm->tm_mon  = bcd2bin(buf[PCF2131_REG_MO] & 0x1F);
	tm->tm_year = bcd2bin(buf[PCF2131_REG_YR]) + 1900;
	if (tm->tm_year < 1970)
		tm->tm_year += 100;	/* assume we are in 1970...2069 */
	tm->tm_wday = buf[PCF2131_REG_DW] & 0x07;
	tm->tm_yday = 0;
	tm->tm_isdst = 0;

	debug("Get DATE: %4d-%02d-%02d (wday=%d)  TIME: %2d:%02d:%02d\n",
	      tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_wday,
	      tm->tm_hour, tm->tm_min, tm->tm_sec);

	return ret;
}

static int pcf2131_rtc_reset(struct udevice *dev)
{
	/*Doing nothing here*/

	return 0;
}

static const struct rtc_ops pcf2131_rtc_ops = {
	.get = pcf2131_rtc_get,
	.set = pcf2131_rtc_set,
	.reset = pcf2131_rtc_reset,
	.read = pcf2131_rtc_read,
	.write = pcf2131_rtc_write,
};

static const struct udevice_id pcf2131_rtc_ids[] = {
	{ .compatible = "nxp,pcf2131" },
	{ }
};

U_BOOT_DRIVER(rtc_pcf2131) = {
	.name	= "rtc-pcf2131",
	.id	= UCLASS_RTC,
	.of_match = pcf2131_rtc_ids,
	.ops	= &pcf2131_rtc_ops,
};
