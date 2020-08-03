/*
 * Copyright 2017,2019 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <i2c.h>
#include <time.h>
#include "tcpc.h"

#ifdef DEBUG
#define tcpc_debug_log(port, fmt, args...) tcpc_log(port, fmt, ##args)
#else
#define tcpc_debug_log(port, fmt, args...)
#endif

static int tcpc_log(struct tcpc_port *port, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vscnprintf(port->log_p, port->log_size, fmt, args);
	va_end(args);

	port->log_size -= i;
	port->log_p += i;

	return i;
}

int tcpc_set_cc_to_source(struct tcpc_port *port)
{
	uint8_t valb;
	int err;

	if (port == NULL)
		return -EINVAL;

	valb = (TCPC_ROLE_CTRL_CC_RP << TCPC_ROLE_CTRL_CC1_SHIFT) |
			(TCPC_ROLE_CTRL_CC_RP << TCPC_ROLE_CTRL_CC2_SHIFT) |
			(TCPC_ROLE_CTRL_RP_VAL_DEF <<
			 TCPC_ROLE_CTRL_RP_VAL_SHIFT) | TCPC_ROLE_CTRL_DRP;

	err = dm_i2c_write(port->i2c_dev, TCPC_ROLE_CTRL, &valb, 1);
	if (err)
		tcpc_log(port, "%s dm_i2c_write failed, err %d\n", __func__, err);
	return err;
}

int tcpc_set_cc_to_sink(struct tcpc_port *port)
{
	uint8_t valb;
	int err;

	if (port == NULL)
		return -EINVAL;

	valb = (TCPC_ROLE_CTRL_CC_RD << TCPC_ROLE_CTRL_CC1_SHIFT) |
			(TCPC_ROLE_CTRL_CC_RD << TCPC_ROLE_CTRL_CC2_SHIFT) | TCPC_ROLE_CTRL_DRP;

	err = dm_i2c_write(port->i2c_dev, TCPC_ROLE_CTRL, &valb, 1);
	if (err)
		tcpc_log(port, "%s dm_i2c_write failed, err %d\n", __func__, err);
	return err;
}


int tcpc_set_plug_orientation(struct tcpc_port *port, enum typec_cc_polarity polarity)
{
	uint8_t valb;
	int err;

	if (port == NULL)
		return -EINVAL;

	err = dm_i2c_read(port->i2c_dev, TCPC_TCPC_CTRL, &valb, 1);
	if (err) {
		tcpc_log(port, "%s dm_i2c_read failed, err %d\n", __func__, err);
		return -EIO;
	}

	if (polarity == TYPEC_POLARITY_CC2)
		valb |= TCPC_TCPC_CTRL_ORIENTATION;
	else
		valb &= ~TCPC_TCPC_CTRL_ORIENTATION;

	err = dm_i2c_write(port->i2c_dev, TCPC_TCPC_CTRL, &valb, 1);
	if (err) {
		tcpc_log(port, "%s dm_i2c_write failed, err %d\n", __func__, err);
		return -EIO;
	}

	return 0;
}

int tcpc_get_cc_status(struct tcpc_port *port, enum typec_cc_polarity *polarity, enum typec_cc_state *state)
{

	uint8_t valb_cc, cc2, cc1;
	int err;

	if (port == NULL || polarity == NULL || state == NULL)
		return -EINVAL;

	err = dm_i2c_read(port->i2c_dev, TCPC_CC_STATUS, (uint8_t *)&valb_cc, 1);
	if (err) {
		tcpc_log(port, "%s dm_i2c_read failed, err %d\n", __func__, err);
		return -EIO;
	}

	tcpc_debug_log(port, "cc status 0x%x\n", valb_cc);

	cc2 = (valb_cc >> TCPC_CC_STATUS_CC2_SHIFT) & TCPC_CC_STATUS_CC2_MASK;
	cc1 = (valb_cc >> TCPC_CC_STATUS_CC1_SHIFT) & TCPC_CC_STATUS_CC1_MASK;

	if (valb_cc & TCPC_CC_STATUS_LOOK4CONN)
		return -EFAULT;

	*state = TYPEC_STATE_OPEN;

	if (valb_cc & TCPC_CC_STATUS_TERM) {
		if (cc2) {
			*polarity = TYPEC_POLARITY_CC2;

			switch (cc2) {
			case 0x1:
				*state = TYPEC_STATE_SNK_DEFAULT;
				tcpc_log(port, "SNK.Default on CC2\n");
				break;
			case 0x2:
				*state = TYPEC_STATE_SNK_POWER15;
				tcpc_log(port, "SNK.Power1.5 on CC2\n");
				break;
			case 0x3:
				*state = TYPEC_STATE_SNK_POWER30;
				tcpc_log(port, "SNK.Power3.0 on CC2\n");
				break;
			}
		} else if (cc1) {
			*polarity = TYPEC_POLARITY_CC1;

			switch (cc1) {
			case 0x1:
				*state = TYPEC_STATE_SNK_DEFAULT;
				tcpc_log(port, "SNK.Default on CC1\n");
				break;
			case 0x2:
				*state = TYPEC_STATE_SNK_POWER15;
				tcpc_log(port, "SNK.Power1.5 on CC1\n");
				break;
			case 0x3:
				*state = TYPEC_STATE_SNK_POWER30;
				tcpc_log(port, "SNK.Power3.0 on CC1\n");
				break;
			}
		} else {
			*state = TYPEC_STATE_OPEN;
			return -EPERM;
		}

	} else {
		if (cc2) {
			*polarity = TYPEC_POLARITY_CC2;

			switch (cc2) {
			case 0x1:
				if (cc1 == 0x1) {
					*state = TYPEC_STATE_SRC_BOTH_RA;
					tcpc_log(port, "SRC.Ra on both CC1 and CC2\n");
				} else if (cc1 == 0x2) {
					*state = TYPEC_STATE_SRC_RD_RA;
					tcpc_log(port, "SRC.Ra on CC2, SRC.Rd on CC1\n");
				} else if (cc1 == 0x0) {
					tcpc_log(port, "SRC.Ra only on CC2\n");
					return -EFAULT;
				} else
					return -EFAULT;
				break;
			case 0x2:
				if (cc1 == 0x1) {
					*state = TYPEC_STATE_SRC_RD_RA;
					tcpc_log(port, "SRC.Ra on CC1, SRC.Rd on CC2\n");
				} else if (cc1 == 0x0) {
					*state = TYPEC_STATE_SRC_RD;
					tcpc_log(port, "SRC.Rd on CC2\n");
				} else
					return -EFAULT;
				break;
			case 0x3:
				*state = TYPEC_STATE_SRC_RESERVED;
				return -EFAULT;
			}
		} else if (cc1) {
			*polarity = TYPEC_POLARITY_CC1;

			switch (cc1) {
			case 0x1:
				tcpc_log(port, "SRC.Ra only on CC1\n");
				return -EFAULT;
			case 0x2:
				*state = TYPEC_STATE_SRC_RD;
				tcpc_log(port, "SRC.Rd on CC1\n");
				break;
			case 0x3:
				*state = TYPEC_STATE_SRC_RESERVED;
				return -EFAULT;
			}
		} else {
			*state = TYPEC_STATE_OPEN;
			return -EPERM;
		}
	}

	return 0;
}

int tcpc_clear_alert(struct tcpc_port *port, uint16_t clear_mask)
{
	int err;

	if (port == NULL)
		return -EINVAL;

	err = dm_i2c_write(port->i2c_dev, TCPC_ALERT, (const uint8_t *)&clear_mask, 2);
	if (err) {
		tcpc_log(port, "%s dm_i2c_write failed, err %d\n", __func__, err);
		return -EIO;
	}

	return 0;
}

int tcpc_send_command(struct tcpc_port *port, uint8_t command)
{
	int err;

	if (port == NULL)
		return -EINVAL;

	err = dm_i2c_write(port->i2c_dev, TCPC_COMMAND, (const uint8_t *)&command, 1);
	if (err) {
		tcpc_log(port, "%s dm_i2c_write failed, err %d\n", __func__, err);
		return -EIO;
	}

	return 0;
}

int tcpc_polling_reg(struct tcpc_port *port, uint8_t reg,
	uint8_t reg_width, uint16_t mask, uint16_t value, ulong timeout_ms)
{
	uint16_t val = 0;
	int err;
	ulong start;

	if (port == NULL)
		return -EINVAL;

	tcpc_debug_log(port, "%s reg 0x%x, mask 0x%x, value 0x%x\n", __func__, reg, mask, value);

	/* TCPC registers is 8 bits or 16 bits */
	if (reg_width != 1 && reg_width != 2)
		return -EINVAL;

	start = get_timer(0);	/* Get current timestamp */
	do {
		err = dm_i2c_read(port->i2c_dev, reg, (uint8_t *)&val, reg_width);
		if (err)
			return -EIO;

		if ((val & mask) == value)
			return 0;
	} while (get_timer(0) < (start + timeout_ms));

	return -ETIME;
}

void tcpc_print_log(struct tcpc_port *port)
{
	if (port == NULL)
		return;

	if (port->log_print == port->log_p) /*nothing to output*/
		return;

	printf("%s", port->log_print);

	port->log_print = port->log_p;
}

int tcpc_setup_dfp_mode(struct tcpc_port *port)
{
	enum typec_cc_polarity pol;
	enum typec_cc_state state;
	int ret;

	if ((port == NULL) || (port->i2c_dev == NULL))
		return -EINVAL;

	if (tcpc_pd_sink_check_charging(port)) {
		tcpc_log(port, "%s: Can't apply DFP mode when PD is charging\n",
			__func__);
		return -EPERM;
	}

	tcpc_set_cc_to_source(port);

	ret = tcpc_send_command(port, TCPC_CMD_LOOK4CONNECTION);
	if (ret)
		return ret;

	/* At least wait tCcStatusDelay + tTCPCFilter + tCcTCPCSampleRate (max) = 200us + 500us + ?ms
	 * PTN5110 datasheet does not contain the sample rate value, according other productions,
	 * the sample rate is at ms level, about 2 ms -10ms. So wait 100ms should be enough.
	 */
	mdelay(100);

	ret = tcpc_polling_reg(port, TCPC_ALERT, 2, TCPC_ALERT_CC_STATUS, TCPC_ALERT_CC_STATUS, 100);
	if (ret) {
		tcpc_log(port, "%s: Polling ALERT register, TCPC_ALERT_CC_STATUS bit failed, ret = %d\n",
			__func__, ret);
		return ret;
	}

	ret = tcpc_get_cc_status(port, &pol, &state);
	tcpc_clear_alert(port, TCPC_ALERT_CC_STATUS);

	if (!ret) {
		/* If presenting as Rd/audio mode/open, return */
		if (state != TYPEC_STATE_SRC_RD_RA && state != TYPEC_STATE_SRC_RD)
			return -EPERM;

		if (pol == TYPEC_POLARITY_CC1)
			tcpc_debug_log(port, "polarity cc1\n");
		else
			tcpc_debug_log(port, "polarity cc2\n");

		if (port->ss_sel_func)
			port->ss_sel_func(pol);

		ret = tcpc_set_plug_orientation(port, pol);
		if (ret)
			return ret;

		/* Enable source vbus default voltage */
		ret = tcpc_send_command(port, TCPC_CMD_SRC_VBUS_DEFAULT);
		if (ret)
			return ret;

		/* The max vbus on time is 200ms, we add margin 100ms */
		mdelay(300);

	}

	return 0;
}

int tcpc_setup_ufp_mode(struct tcpc_port *port)
{
	enum typec_cc_polarity pol;
	enum typec_cc_state state;
	int ret;

	if ((port == NULL) || (port->i2c_dev == NULL))
		return -EINVAL;

	/* Check if the PD charge is working. If not, need to configure CC role for UFP */
	if (!tcpc_pd_sink_check_charging(port)) {

		/* Disable the source vbus once it is enabled by DFP mode */
		tcpc_disable_src_vbus(port);

		tcpc_set_cc_to_sink(port);

		ret = tcpc_send_command(port, TCPC_CMD_LOOK4CONNECTION);
		if (ret)
			return ret;

		/* At least wait tCcStatusDelay + tTCPCFilter + tCcTCPCSampleRate (max) = 200us + 500us + ?ms
		 * PTN5110 datasheet does not contain the sample rate value, according other productions,
		 * the sample rate is at ms level, about 2 ms -10ms. So wait 100ms should be enough.
		 */
		mdelay(100);

		ret = tcpc_polling_reg(port, TCPC_ALERT, 2, TCPC_ALERT_CC_STATUS, TCPC_ALERT_CC_STATUS, 100);
		if (ret) {
			tcpc_log(port, "%s: Polling ALERT register, TCPC_ALERT_CC_STATUS bit failed, ret = %d\n",
				__func__, ret);
			return ret;
		}

		ret = tcpc_get_cc_status(port, &pol, &state);
		tcpc_clear_alert(port, TCPC_ALERT_CC_STATUS);

	} else {
		ret = tcpc_get_cc_status(port, &pol, &state);
	}

	if (!ret) {
		/* If presenting not as sink, then return */
		if (state != TYPEC_STATE_SNK_DEFAULT && state != TYPEC_STATE_SNK_POWER15 &&
			state != TYPEC_STATE_SNK_POWER30)
			return -EPERM;

		if (pol == TYPEC_POLARITY_CC1)
			tcpc_debug_log(port, "polarity cc1\n");
		else
			tcpc_debug_log(port, "polarity cc2\n");

		if (port->ss_sel_func)
			port->ss_sel_func(pol);

		ret = tcpc_set_plug_orientation(port, pol);
		if (ret)
			return ret;
	}

	return 0;
}

int tcpc_disable_src_vbus(struct tcpc_port *port)
{
	int ret;

	if (port == NULL)
		return -EINVAL;

	/* Disable VBUS*/
	ret = tcpc_send_command(port, TCPC_CMD_DISABLE_SRC_VBUS);
	if (ret)
		return ret;

	/* The max vbus off time is 0.5ms, we add margin 0.5 ms */
	mdelay(1);

	return 0;
}

int tcpc_disable_sink_vbus(struct tcpc_port *port)
{
	int ret;

	if (port == NULL)
		return -EINVAL;

	/* Disable SINK VBUS*/
	ret = tcpc_send_command(port, TCPC_CMD_DISABLE_SINK_VBUS);
	if (ret)
		return ret;

	/* The max vbus off time is 0.5ms, we add margin 0.5 ms */
	mdelay(1);

	return 0;
}


static int tcpc_pd_receive_message(struct tcpc_port *port, struct pd_message *msg)
{
	int ret;
	uint8_t cnt;
	uint16_t val;

	if (port == NULL)
		return -EINVAL;

	/* Generally the max tSenderResponse is 30ms, max tTypeCSendSourceCap is 200ms, we set the timeout to 500ms */
	ret = tcpc_polling_reg(port, TCPC_ALERT, 2, TCPC_ALERT_RX_STATUS, TCPC_ALERT_RX_STATUS, 500);
	if (ret) {
		tcpc_log(port, "%s: Polling ALERT register, TCPC_ALERT_RX_STATUS bit failed, ret = %d\n",
			__func__, ret);
		return ret;
	}

	cnt = 0;
	ret = dm_i2c_read(port->i2c_dev, TCPC_RX_BYTE_CNT, (uint8_t *)&cnt, 1);
	if (ret)
		return -EIO;

	if (cnt > 0) {
		ret = dm_i2c_read(port->i2c_dev, TCPC_RX_BUF_FRAME_TYPE, (uint8_t *)msg, cnt);
		if (ret)
			return -EIO;

		/* Clear RX status alert bit */
		val = TCPC_ALERT_RX_STATUS;
		ret = dm_i2c_write(port->i2c_dev, TCPC_ALERT, (const uint8_t *)&val, 2);
		if (ret)
			return -EIO;
	}

	return cnt;
}

static int tcpc_pd_transmit_message(struct tcpc_port *port, struct pd_message *msg_p, uint8_t bytes)
{
	int ret;
	uint8_t valb;
	uint16_t val = 0;

	if (port == NULL)
		return -EINVAL;

	if (msg_p == NULL || bytes <= 0)
		return -EINVAL;

	ret = dm_i2c_write(port->i2c_dev, TCPC_TX_BYTE_CNT, (const uint8_t *)&bytes, 1);
	if (ret)
		return -EIO;

	ret = dm_i2c_write(port->i2c_dev, TCPC_TX_HDR, (const uint8_t *)&(msg_p->header), bytes);
	if (ret)
		return -EIO;

	valb = (3 << TCPC_TRANSMIT_RETRY_SHIFT) | (TCPC_TX_SOP << TCPC_TRANSMIT_TYPE_SHIFT);
	ret = dm_i2c_write(port->i2c_dev, TCPC_TRANSMIT, (const uint8_t *)&valb, 1);
	if (ret)
		return -EIO;

	/* Max tReceive is 1.1ms, we set to 5ms timeout */
	ret = tcpc_polling_reg(port, TCPC_ALERT, 2, TCPC_ALERT_TX_SUCCESS, TCPC_ALERT_TX_SUCCESS, 5);
	if (ret) {
		if (ret == -ETIME) {
			ret = dm_i2c_read(port->i2c_dev, TCPC_ALERT, (uint8_t *)&val, 2);
			if (ret)
				return -EIO;

			if (val & TCPC_ALERT_TX_FAILED)
				tcpc_log(port, "%s: PD TX FAILED, ALERT = 0x%x\n", __func__, val);

			if (val & TCPC_ALERT_TX_DISCARDED)
				tcpc_log(port, "%s: PD TX DISCARDED, ALERT = 0x%x\n", __func__, val);

		} else {
			tcpc_log(port, "%s: Polling ALERT register, TCPC_ALERT_TX_SUCCESS bit failed, ret = %d\n",
				__func__, ret);
		}
	} else {
		port->tx_msg_id = (port->tx_msg_id + 1) & PD_HEADER_ID_MASK;
	}

	/* Clear ALERT status */
	val &= (TCPC_ALERT_TX_FAILED | TCPC_ALERT_TX_DISCARDED | TCPC_ALERT_TX_SUCCESS);
	ret = dm_i2c_write(port->i2c_dev, TCPC_ALERT, (const uint8_t *)&val, 2);
	if (ret)
		return -EIO;

	return ret;
}

static void tcpc_log_source_caps(struct tcpc_port *port, struct pd_message *msg, unsigned int capcount)
{
	int i;

	for (i = 0; i < capcount; i++) {
		u32 pdo = msg->payload[i];
		enum pd_pdo_type type = pdo_type(pdo);

		tcpc_log(port, "PDO %d: type %d, ",
			 i, type);

		switch (type) {
		case PDO_TYPE_FIXED:
			tcpc_log(port, "%u mV, %u mA [%s%s%s%s%s%s]\n",
				  pdo_fixed_voltage(pdo),
				  pdo_max_current(pdo),
				  (pdo & PDO_FIXED_DUAL_ROLE) ?
							"R" : "",
				  (pdo & PDO_FIXED_SUSPEND) ?
							"S" : "",
				  (pdo & PDO_FIXED_HIGHER_CAP) ?
							"H" : "",
				  (pdo & PDO_FIXED_USB_COMM) ?
							"U" : "",
				  (pdo & PDO_FIXED_DATA_SWAP) ?
							"D" : "",
				  (pdo & PDO_FIXED_EXTPOWER) ?
							"E" : "");
			break;
		case PDO_TYPE_VAR:
			tcpc_log(port, "%u-%u mV, %u mA\n",
				  pdo_min_voltage(pdo),
				  pdo_max_voltage(pdo),
				  pdo_max_current(pdo));
			break;
		case PDO_TYPE_BATT:
			tcpc_log(port, "%u-%u mV, %u mW\n",
				  pdo_min_voltage(pdo),
				  pdo_max_voltage(pdo),
				  pdo_max_power(pdo));
			break;
		default:
			tcpc_log(port, "undefined\n");
			break;
		}
	}
}

static int tcpc_pd_select_pdo(struct pd_message *msg, uint32_t capcount, uint32_t max_snk_mv, uint32_t max_snk_ma)
{
	unsigned int i, max_mw = 0, max_mv = 0;
	int ret = -EINVAL;

	/*
	 * Select the source PDO providing the most power while staying within
	 * the board's voltage limits. Prefer PDO providing exp
	 */
	for (i = 0; i < capcount; i++) {
		u32 pdo = msg->payload[i];
		enum pd_pdo_type type = pdo_type(pdo);
		unsigned int mv, ma, mw;

		if (type == PDO_TYPE_FIXED)
			mv = pdo_fixed_voltage(pdo);
		else
			mv = pdo_min_voltage(pdo);

		if (type == PDO_TYPE_BATT) {
			mw = pdo_max_power(pdo);
		} else {
			ma = min(pdo_max_current(pdo),
				 max_snk_ma);
			mw = ma * mv / 1000;
		}

		/* Perfer higher voltages if available */
		if ((mw > max_mw || (mw == max_mw && mv > max_mv)) &&
		    mv <= max_snk_mv) {
			ret = i;
			max_mw = mw;
			max_mv = mv;
		}
	}

	return ret;
}

static int tcpc_pd_build_request(struct tcpc_port *port,
										struct pd_message *msg,
										uint32_t capcount,
										uint32_t max_snk_mv,
										uint32_t max_snk_ma,
										uint32_t max_snk_mw,
										uint32_t operating_snk_mw,
										uint32_t *rdo)
{
	unsigned int mv, ma, mw, flags;
	unsigned int max_ma, max_mw;
	enum pd_pdo_type type;
	int index;
	u32 pdo;

	index = tcpc_pd_select_pdo(msg, capcount, max_snk_mv, max_snk_ma);
	if (index < 0)
		return -EINVAL;

	pdo = msg->payload[index];
	type = pdo_type(pdo);

	if (type == PDO_TYPE_FIXED)
		mv = pdo_fixed_voltage(pdo);
	else
		mv = pdo_min_voltage(pdo);

	/* Select maximum available current within the board's power limit */
	if (type == PDO_TYPE_BATT) {
		mw = pdo_max_power(pdo);
		ma = 1000 * min(mw, max_snk_mw) / mv;
	} else {
		ma = min(pdo_max_current(pdo),
			 1000 * max_snk_mw / mv);
	}
	ma = min(ma, max_snk_ma);

	/* XXX: Any other flags need to be set? */
	flags = 0;

	/* Set mismatch bit if offered power is less than operating power */
	mw = ma * mv / 1000;
	max_ma = ma;
	max_mw = mw;
	if (mw < operating_snk_mw) {
		flags |= RDO_CAP_MISMATCH;
		max_mw = operating_snk_mw;
		max_ma = max_mw * 1000 / mv;
	}

	if (type == PDO_TYPE_BATT) {
		*rdo = RDO_BATT(index + 1, mw, max_mw, flags);

		tcpc_log(port, "Requesting PDO %d: %u mV, %u mW%s\n",
			 index, mv, mw,
			 flags & RDO_CAP_MISMATCH ? " [mismatch]" : "");
	} else {
		*rdo = RDO_FIXED(index + 1, ma, max_ma, flags);

		tcpc_log(port, "Requesting PDO %d: %u mV, %u mA%s\n",
			 index, mv, ma,
			 flags & RDO_CAP_MISMATCH ? " [mismatch]" : "");
	}

	return 0;
}

static void tcpc_pd_sink_process(struct tcpc_port *port)
{
	int ret;
	uint8_t msgtype;
	uint32_t objcnt;
	struct pd_message msg;
	enum pd_sink_state pd_state = WAIT_SOURCE_CAP;

	while (tcpc_pd_receive_message(port, &msg) > 0) {

		msgtype = pd_header_type(msg.header);
		objcnt = pd_header_cnt_le(msg.header);

		tcpc_debug_log(port, "get msg, type %d, cnt %d\n", msgtype, objcnt);

		switch (pd_state) {
		case WAIT_SOURCE_CAP:
		case SINK_READY:
			if (msgtype != PD_DATA_SOURCE_CAP)
				continue;

			uint32_t rdo = 0;

			tcpc_log_source_caps(port, &msg, objcnt);

			tcpc_pd_build_request(port, &msg, objcnt,
				port->cfg.max_snk_mv, port->cfg.max_snk_ma,
				port->cfg.max_snk_mw, port->cfg.op_snk_mv,
				&rdo);

			memset(&msg, 0, sizeof(msg));
			msg.header = PD_HEADER(PD_DATA_REQUEST, 0, 0, port->tx_msg_id, 1);  /* power sink, data device, id 0, len 1 */
			msg.payload[0] = rdo;

			ret = tcpc_pd_transmit_message(port, &msg, 6);
			if (ret)
				tcpc_log(port, "send request failed\n");
			else
				pd_state = WAIT_SOURCE_ACCEPT;

			break;
		case WAIT_SOURCE_ACCEPT:
			if (objcnt > 0) /* Should be ctrl message */
				continue;

			if (msgtype == PD_CTRL_ACCEPT) {
				pd_state = WAIT_SOURCE_READY;
				tcpc_log(port, "Source accept request\n");
			} else if (msgtype == PD_CTRL_REJECT) {
				tcpc_log(port, "Source reject request\n");
				return;
			}

			break;
		case WAIT_SOURCE_READY:
			if (objcnt > 0) /* Should be ctrl message */
				continue;

			if (msgtype == PD_CTRL_PS_RDY) {
				tcpc_log(port, "PD source ready!\n");
				pd_state = SINK_READY;
			}

			break;
		default:
			tcpc_log(port, "unexpect status: %u\n", pd_state);
			break;
		}
	}
}

bool tcpc_pd_sink_check_charging(struct tcpc_port *port)
{
	uint8_t valb;
	int err;
	enum typec_cc_polarity pol;
	enum typec_cc_state state;

	if (port == NULL)
		return false;

	/* Check the CC status, must be sink */
	err = tcpc_get_cc_status(port, &pol, &state);
	if (err || (state != TYPEC_STATE_SNK_POWER15
		&& state != TYPEC_STATE_SNK_POWER30
		&& state != TYPEC_STATE_SNK_DEFAULT)) {
		tcpc_debug_log(port, "TCPC wrong state for PD charging, err = %d, CC = 0x%x\n",
			err, state);
		return false;
	}

	/* Check the VBUS PRES and SINK VBUS for dead battery */
	err = dm_i2c_read(port->i2c_dev, TCPC_POWER_STATUS, &valb, 1);
	if (err) {
		tcpc_debug_log(port, "%s dm_i2c_read failed, err %d\n", __func__, err);
		return false;
	}

	if (!(valb & TCPC_POWER_STATUS_VBUS_PRES)) {
		tcpc_debug_log(port, "VBUS NOT PRES \n");
		return false;
	}

	if (!(valb & TCPC_POWER_STATUS_SINKING_VBUS)) {
		tcpc_debug_log(port, "SINK VBUS is not enabled for dead battery\n");
		return false;
	}

	return true;
}

static int tcpc_pd_sink_disable(struct tcpc_port *port)
{
	uint8_t valb;
	int err;

	if (port == NULL)
		return -EINVAL;

	port->pd_state = UNATTACH;

	/* Check the VBUS PRES and SINK VBUS for dead battery */
	err = dm_i2c_read(port->i2c_dev, TCPC_POWER_STATUS, &valb, 1);
	if (err) {
		tcpc_log(port, "%s dm_i2c_read failed, err %d\n", __func__, err);
		return -EIO;
	}

	if ((valb & TCPC_POWER_STATUS_VBUS_PRES) && (valb & TCPC_POWER_STATUS_SINKING_VBUS)) {
		dm_i2c_read(port->i2c_dev, TCPC_POWER_CTRL, (uint8_t *)&valb, 1);
		valb &= ~TCPC_POWER_CTRL_AUTO_DISCH_DISCO; /* disable AutoDischargeDisconnect */
		dm_i2c_write(port->i2c_dev, TCPC_POWER_CTRL, (const uint8_t *)&valb, 1);

		tcpc_disable_sink_vbus(port);
	}

	if (port->cfg.switch_setup_func)
		port->cfg.switch_setup_func(port);

	return 0;
}

static int tcpc_pd_sink_init(struct tcpc_port *port)
{
	uint8_t valb;
	uint16_t val;
	int err;
	enum typec_cc_polarity pol;
	enum typec_cc_state state;

	if (port == NULL)
		return -EINVAL;

	port->pd_state = UNATTACH;

	/* Check the VBUS PRES and SINK VBUS for dead battery */
	err = dm_i2c_read(port->i2c_dev, TCPC_POWER_STATUS, &valb, 1);
	if (err) {
		tcpc_log(port, "%s dm_i2c_read failed, err %d\n", __func__, err);
		return -EIO;
	}

	if (!(valb & TCPC_POWER_STATUS_VBUS_PRES)) {
		tcpc_debug_log(port, "VBUS NOT PRES \n");
		return -EPERM;
	}

	if (!(valb & TCPC_POWER_STATUS_SINKING_VBUS)) {
		tcpc_debug_log(port, "SINK VBUS is not enabled for dead battery\n");
		return -EPERM;
	}

	err = dm_i2c_read(port->i2c_dev, TCPC_ALERT, (uint8_t *)&val, 2);
	if (err) {
		tcpc_log(port, "%s dm_i2c_read failed, err %d\n", __func__, err);
		return -EIO;
	}

	if (!(val & TCPC_ALERT_CC_STATUS)) {
		tcpc_debug_log(port, "CC STATUS not detected for dead battery\n");
		return -EPERM;
	}

	err = tcpc_get_cc_status(port, &pol, &state);
	if (err || (state != TYPEC_STATE_SNK_POWER15
		&& state != TYPEC_STATE_SNK_POWER30
		&& state != TYPEC_STATE_SNK_DEFAULT)) {
		tcpc_log(port, "TCPC wrong state for dead battery, err = %d, CC = 0x%x\n",
			err, state);
		return -EPERM;
	} else {
		err = tcpc_set_plug_orientation(port, pol);
		if (err) {
			tcpc_log(port, "TCPC set plug orientation failed, err = %d\n", err);
			return err;
		}
		port->pd_state = ATTACHED;
	}

	dm_i2c_read(port->i2c_dev, TCPC_POWER_CTRL, (uint8_t *)&valb, 1);
	valb &= ~TCPC_POWER_CTRL_AUTO_DISCH_DISCO; /* disable AutoDischargeDisconnect */
	dm_i2c_write(port->i2c_dev, TCPC_POWER_CTRL, (const uint8_t *)&valb, 1);

	if (port->cfg.switch_setup_func)
		port->cfg.switch_setup_func(port);

	/* As sink role */
	valb = 0x00;
	err = dm_i2c_write(port->i2c_dev, TCPC_MSG_HDR_INFO, (const uint8_t *)&valb, 1);
	if (err) {
		tcpc_log(port, "%s dm_i2c_read failed, err %d\n", __func__, err);
		return -EIO;
	}

	/* Enable rx */
	valb = TCPC_RX_DETECT_SOP | TCPC_RX_DETECT_HARD_RESET;
	err = dm_i2c_write(port->i2c_dev, TCPC_RX_DETECT, (const uint8_t *)&valb, 1);
	if (err) {
		tcpc_log(port, "%s dm_i2c_read failed, err %d\n", __func__, err);
		return -EIO;
	}

	tcpc_pd_sink_process(port);

	return 0;
}

int tcpc_init(struct tcpc_port *port, struct tcpc_port_config config, ss_mux_sel ss_sel_func)
{
	int ret;
	uint8_t valb;
	uint16_t vid, pid;
	struct udevice *bus;
	struct udevice *i2c_dev = NULL;

	memset(port, 0, sizeof(struct tcpc_port));

	if (port == NULL)
		return -EINVAL;

	port->cfg = config;
	port->tx_msg_id = 0;
	port->ss_sel_func = ss_sel_func;
	port->log_p = (char *)&(port->logbuffer);
	port->log_size = TCPC_LOG_BUFFER_SIZE;
	port->log_print = port->log_p;
	memset(&(port->logbuffer), 0, TCPC_LOG_BUFFER_SIZE);

	ret = uclass_get_device_by_seq(UCLASS_I2C, port->cfg.i2c_bus, &bus);
	if (ret) {
		printf("%s: Can't find bus\n", __func__);
		return -EINVAL;
	}

	ret = dm_i2c_probe(bus, port->cfg.addr, 0, &i2c_dev);
	if (ret) {
		printf("%s: Can't find device id=0x%x\n",
			__func__, config.addr);
		return -ENODEV;
	}

	port->i2c_dev = i2c_dev;

	/* Check the Initialization Status bit in 1s */
	ret = tcpc_polling_reg(port, TCPC_POWER_STATUS, 1, TCPC_POWER_STATUS_UNINIT, 0, 1000);
	if (ret) {
		tcpc_log(port, "%s: Polling TCPC POWER STATUS Initialization Status bit failed, ret = %d\n",
			__func__, ret);
		return ret;
	}

	dm_i2c_read(port->i2c_dev, TCPC_POWER_STATUS, &valb, 1);
	tcpc_debug_log(port, "POWER STATUS: 0x%x\n", valb);

	/* Clear AllRegistersResetToDefault */
	valb = 0x80;
	ret = dm_i2c_write(port->i2c_dev, TCPC_FAULT_STATUS, (const uint8_t *)&valb, 1);
	if (ret) {
		tcpc_log(port, "%s dm_i2c_read failed, err %d\n", __func__, ret);
		return -EIO;
	}

	/* Read Vendor ID and Product ID */
	ret = dm_i2c_read(port->i2c_dev, TCPC_VENDOR_ID, (uint8_t *)&vid, 2);
	if (ret) {
		tcpc_log(port, "%s dm_i2c_read failed, err %d\n", __func__, ret);
		return -EIO;
	}

	ret = dm_i2c_read(port->i2c_dev, TCPC_PRODUCT_ID, (uint8_t *)&pid, 2);
	if (ret) {
		tcpc_log(port, "%s dm_i2c_read failed, err %d\n", __func__, ret);
		return -EIO;
	}

	tcpc_log(port, "TCPC:  Vendor ID [0x%x], Product ID [0x%x], Addr [I2C%u 0x%x]\n",
		vid, pid, port->cfg.i2c_bus, port->cfg.addr);

	if (!port->cfg.disable_pd) {
		if  (port->cfg.port_type == TYPEC_PORT_UFP
			|| port->cfg.port_type == TYPEC_PORT_DRP)
			tcpc_pd_sink_init(port);
	} else {
		tcpc_pd_sink_disable(port);
	}

	tcpc_clear_alert(port, 0xffff);

	tcpc_print_log(port);

	return 0;
}
