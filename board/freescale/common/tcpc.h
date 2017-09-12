/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __TCPCI_H
#define __TCPCI_H

#include <dm.h>

#define TCPC_TCPC_CTRL			0x19
#define TCPC_TCPC_CTRL_ORIENTATION	BIT(0)

#define TCPC_CC_STATUS			0x1d
#define TCPC_CC_STATUS_TERM		BIT(4)
#define TCPC_CC_STATUS_CC2_SHIFT	2
#define TCPC_CC_STATUS_CC2_MASK		0x3
#define TCPC_CC_STATUS_CC1_SHIFT	0
#define TCPC_CC_STATUS_CC1_MASK		0x3

#define TCPC_ROLE_CTRL			0x1a
#define TCPC_ROLE_CTRL_DRP		BIT(6)
#define TCPC_ROLE_CTRL_RP_VAL_SHIFT	4
#define TCPC_ROLE_CTRL_RP_VAL_MASK	0x3
#define TCPC_ROLE_CTRL_RP_VAL_DEF	0x0
#define TCPC_ROLE_CTRL_RP_VAL_1_5	0x1
#define TCPC_ROLE_CTRL_RP_VAL_3_0	0x2
#define TCPC_ROLE_CTRL_CC2_SHIFT	2
#define TCPC_ROLE_CTRL_CC2_MASK		0x3
#define TCPC_ROLE_CTRL_CC1_SHIFT	0
#define TCPC_ROLE_CTRL_CC1_MASK		0x3
#define TCPC_ROLE_CTRL_CC_RA		0x0
#define TCPC_ROLE_CTRL_CC_RP		0x1
#define TCPC_ROLE_CTRL_CC_RD		0x2
#define TCPC_ROLE_CTRL_CC_OPEN		0x3

#define TCPC_COMMAND			0x23
#define TCPC_CMD_WAKE_I2C		0x11
#define TCPC_CMD_DISABLE_VBUS_DETECT	0x22
#define TCPC_CMD_ENABLE_VBUS_DETECT	0x33
#define TCPC_CMD_DISABLE_SINK_VBUS	0x44
#define TCPC_CMD_SINK_VBUS		0x55
#define TCPC_CMD_DISABLE_SRC_VBUS	0x66
#define TCPC_CMD_SRC_VBUS_DEFAULT	0x77
#define TCPC_CMD_SRC_VBUS_HIGH		0x88
#define TCPC_CMD_LOOK4CONNECTION	0x99
#define TCPC_CMD_RXONEMORE		0xAA
#define TCPC_CMD_I2C_IDLE		0xFF

#define TCPC_ALERT			0x10
#define TCPC_ALERT_VBUS_DISCNCT		BIT(11)
#define TCPC_ALERT_RX_BUF_OVF		BIT(10)
#define TCPC_ALERT_FAULT		BIT(9)
#define TCPC_ALERT_V_ALARM_LO		BIT(8)
#define TCPC_ALERT_V_ALARM_HI		BIT(7)
#define TCPC_ALERT_TX_SUCCESS		BIT(6)
#define TCPC_ALERT_TX_DISCARDED		BIT(5)
#define TCPC_ALERT_TX_FAILED		BIT(4)
#define TCPC_ALERT_RX_HARD_RST		BIT(3)
#define TCPC_ALERT_RX_STATUS		BIT(2)
#define TCPC_ALERT_POWER_STATUS		BIT(1)
#define TCPC_ALERT_CC_STATUS		BIT(0)

#define TCPC_POWER_STATUS		0x1e
#define TCPC_POWER_STATUS_UNINIT	BIT(6)
#define TCPC_POWER_STATUS_VBUS_DET	BIT(3)
#define TCPC_POWER_STATUS_VBUS_PRES	BIT(2)

enum typec_cc_polarity {
	TYPEC_POLARITY_CC1,
	TYPEC_POLARITY_CC2,
};

typedef void (*ss_mux_sel)(enum typec_cc_polarity pol);

int tcpc_set_cc_to_source(struct udevice *i2c_dev);
int tcpc_set_plug_orientation(struct udevice *i2c_dev, enum typec_cc_polarity polarity);
int tcpc_get_cc_polarity(struct udevice *i2c_dev, enum typec_cc_polarity *polarity);
int tcpc_clear_alert(struct udevice *i2c_dev, uint16_t clear_mask);
int tcpc_send_command(struct udevice *i2c_dev, uint8_t command);
int tcpc_polling_reg(struct udevice *i2c_dev, uint8_t reg,
	uint8_t reg_width, uint16_t mask, uint16_t value, ulong timeout_ms);
int tcpc_setup_dfp_mode(struct udevice *i2c_dev, ss_mux_sel ss_sel_func);
int tcpc_disable_vbus(struct udevice *i2c_dev);
int tcpc_init(struct udevice *i2c_dev);

#endif /* __TCPCI_H */
