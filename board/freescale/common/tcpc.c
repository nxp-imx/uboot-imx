/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <i2c.h>
#include <time.h>
#include "tcpc.h"

int tcpc_set_cc_to_source(struct udevice *i2c_dev)
{
	uint8_t valb;
	int err;

	valb = (TCPC_ROLE_CTRL_CC_RP << TCPC_ROLE_CTRL_CC1_SHIFT) |
			(TCPC_ROLE_CTRL_CC_RP << TCPC_ROLE_CTRL_CC2_SHIFT) |
			(TCPC_ROLE_CTRL_RP_VAL_DEF <<
			 TCPC_ROLE_CTRL_RP_VAL_SHIFT);

	err = dm_i2c_write(i2c_dev, TCPC_ROLE_CTRL, &valb, 1);
	if (err)
		printf("%s dm_i2c_write failed, err %d\n", __func__, err);
	return err;
}

int tcpc_set_plug_orientation(struct udevice *i2c_dev, enum typec_cc_polarity polarity)
{
	uint8_t valb;
	int err;

	err = dm_i2c_read(i2c_dev, TCPC_TCPC_CTRL, &valb, 1);
	if (err) {
		printf("%s dm_i2c_read failed, err %d\n", __func__, err);
		return -EIO;
	}

	if (polarity == TYPEC_POLARITY_CC2)
		valb |= TCPC_TCPC_CTRL_ORIENTATION;
	else
		valb &= ~TCPC_TCPC_CTRL_ORIENTATION;

	err = dm_i2c_write(i2c_dev, TCPC_TCPC_CTRL, &valb, 1);
	if (err) {
		printf("%s dm_i2c_write failed, err %d\n", __func__, err);
		return -EIO;
	}

	return 0;
}

int tcpc_get_cc_polarity(struct udevice *i2c_dev, enum typec_cc_polarity *polarity)
{

	uint8_t valb;
	int err;

	err = dm_i2c_read(i2c_dev, TCPC_CC_STATUS, &valb, 1);
	if (err) {
		printf("%s dm_i2c_read failed, err %d\n", __func__, err);
		return -EIO;
	}

	debug("cc status 0x%x\n", valb);

	/* Set to Rp at default */
	if (valb & TCPC_CC_STATUS_TERM)
		return -EPERM;

	if (((valb >> TCPC_CC_STATUS_CC1_SHIFT) & TCPC_CC_STATUS_CC1_MASK) == 0x2)
		*polarity = TYPEC_POLARITY_CC1;
	else if (((valb >> TCPC_CC_STATUS_CC2_SHIFT) & TCPC_CC_STATUS_CC2_MASK) == 0x2)
		*polarity = TYPEC_POLARITY_CC2;
	else
		return -EFAULT;
	return 0;
}

int tcpc_clear_alert(struct udevice *i2c_dev, uint16_t clear_mask)
{
	int err;
	err = dm_i2c_write(i2c_dev, TCPC_ALERT, (const uint8_t *)&clear_mask, 2);
	if (err) {
		printf("%s dm_i2c_write failed, err %d\n", __func__, err);
		return -EIO;
	}

	return 0;
}

int tcpc_send_command(struct udevice *i2c_dev, uint8_t command)
{
	int err;
	err = dm_i2c_write(i2c_dev, TCPC_COMMAND, (const uint8_t *)&command, 1);
	if (err) {
		printf("%s dm_i2c_write failed, err %d\n", __func__, err);
		return -EIO;
	}

	return 0;
}

int tcpc_polling_reg(struct udevice *i2c_dev, uint8_t reg,
	uint8_t reg_width, uint16_t mask, uint16_t value, ulong timeout_ms)
{
	uint16_t val = 0;
	int err;
	ulong start;

	debug("%s reg 0x%x, mask 0x%x, value 0x%x\n", __func__, reg, mask, value);

	/* TCPC registers is 8 bits or 16 bits */
	if (reg_width != 1 && reg_width != 2)
		return -EINVAL;

	start = get_timer(0);	/* Get current timestamp */
	do {
		err = dm_i2c_read(i2c_dev, reg, (uint8_t *)&val, reg_width);
		if (err)
			return -EIO;

		debug("val = 0x%x\n", val);

		if ((val & mask) == value)
			return 0;
	} while (get_timer(0) < (start + timeout_ms));

	return -ETIME;
}

int tcpc_init(struct udevice *i2c_dev)
{
	int ret;

	/* Check the Initialization Status bit in 1s */
	ret = tcpc_polling_reg(i2c_dev, TCPC_POWER_STATUS, 1, TCPC_POWER_STATUS_UNINIT, 0, 1000);
	if (ret) {
		printf("%s: Polling TCPC POWER STATUS Initialization Status bit failed, ret = %d\n",
			__func__, ret);
		return ret;
	}

	tcpc_clear_alert(i2c_dev, 0xffff);

	return 0;
}

int tcpc_setup_dfp_mode(struct udevice *i2c_dev, ss_mux_sel ss_sel_func)
{
	enum typec_cc_polarity pol;
	int ret;

	tcpc_set_cc_to_source(i2c_dev);

	ret = tcpc_send_command(i2c_dev, TCPC_CMD_LOOK4CONNECTION);
	if (ret)
		return ret;

	/* At least wait tCcStatusDelay + tTCPCFilter + tCcTCPCSampleRate (max) = 200us + 500us + ?ms
	 * PTN5110 datasheet does not contain the sample rate value, according other productions,
	 * the sample rate is at ms level, about 2 ms -10ms. So wait 100ms should be enough.
	 */
	mdelay(100);

	ret = tcpc_polling_reg(i2c_dev, TCPC_ALERT, 2, TCPC_ALERT_CC_STATUS, TCPC_ALERT_CC_STATUS, 100);
	if (ret) {
		printf("%s: Polling ALERT register, TCPC_ALERT_CC_STATUS bit failed, ret = %d\n",
			__func__, ret);
		return ret;
	}

	ret = tcpc_get_cc_polarity(i2c_dev, &pol);
	tcpc_clear_alert(i2c_dev, TCPC_ALERT_CC_STATUS);

	if (!ret) {
		if (pol == TYPEC_POLARITY_CC1)
			debug("polarity cc1\n");
		else
			debug("polarity cc2\n");

		if (ss_sel_func)
			ss_sel_func(pol);

		ret = tcpc_set_plug_orientation(i2c_dev, pol);
		if (ret)
			return ret;

		/* Disable sink vbus */
		ret = tcpc_send_command(i2c_dev, TCPC_CMD_DISABLE_SINK_VBUS);
		if (ret)
			return ret;

		/* Enable source vbus default voltage */
		ret = tcpc_send_command(i2c_dev, TCPC_CMD_SRC_VBUS_DEFAULT);
		if (ret)
			return ret;

		/* The max vbus on time is 200ms, we add margin 100ms */
		mdelay(300);

	}

	return 0;
}

int tcpc_disable_vbus(struct udevice *i2c_dev)
{
	int ret;

	/* Disable VBUS*/
	ret = tcpc_send_command(i2c_dev, TCPC_CMD_DISABLE_SINK_VBUS);
	if (ret)
		return ret;

	ret = tcpc_send_command(i2c_dev, TCPC_CMD_DISABLE_SRC_VBUS);
	if (ret)
		return ret;

	/* The max vbus off time is 0.5ms, we add margin 0.5 ms */
	mdelay(1);

	return 0;
}
