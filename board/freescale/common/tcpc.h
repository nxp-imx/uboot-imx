/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __TCPCI_H
#define __TCPCI_H

#include <dm.h>

#define TCPC_VENDOR_ID			0x0
#define TCPC_PRODUCT_ID			0x2

#define TCPC_ALERT					0x10
#define TCPC_ALERT_VBUS_DISCNCT		BIT(11)
#define TCPC_ALERT_RX_BUF_OVF		BIT(10)
#define TCPC_ALERT_FAULT			BIT(9)
#define TCPC_ALERT_V_ALARM_LO		BIT(8)
#define TCPC_ALERT_V_ALARM_HI		BIT(7)
#define TCPC_ALERT_TX_SUCCESS		BIT(6)
#define TCPC_ALERT_TX_DISCARDED		BIT(5)
#define TCPC_ALERT_TX_FAILED		BIT(4)
#define TCPC_ALERT_RX_HARD_RST		BIT(3)
#define TCPC_ALERT_RX_STATUS		BIT(2)
#define TCPC_ALERT_POWER_STATUS		BIT(1)
#define TCPC_ALERT_CC_STATUS		BIT(0)

#define TCPC_TCPC_CTRL				0x19
#define TCPC_TCPC_CTRL_BIST_MODE	BIT(1)
#define TCPC_TCPC_CTRL_ORIENTATION	BIT(0)

#define TCPC_ROLE_CTRL				0x1a
#define TCPC_ROLE_CTRL_DRP			BIT(6)
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

#define TCPC_POWER_CTRL						0x1c
#define TCPC_POWER_CTRL_EN_VCONN			BIT(0)
#define TCPC_POWER_CTRL_VCONN_POWER			BIT(1)
#define TCPC_POWER_CTRL_FORCE_DISCH			BIT(2)
#define TCPC_POWER_CTRL_EN_BLEED_CH			BIT(3)
#define TCPC_POWER_CTRL_AUTO_DISCH_DISCO	BIT(4)
#define TCPC_POWER_CTRL_DIS_V_ALARMS		BIT(5)
#define TCPC_POWER_CTRL_VBUS_V_MONITOR		BIT(6)

#define TCPC_CC_STATUS					0x1d
#define TCPC_CC_STATUS_LOOK4CONN		BIT(5)
#define TCPC_CC_STATUS_TERM				BIT(4)
#define TCPC_CC_STATUS_CC2_SHIFT		2
#define TCPC_CC_STATUS_CC2_MASK			0x3
#define TCPC_CC_STATUS_CC1_SHIFT		0
#define TCPC_CC_STATUS_CC1_MASK			0x3

#define TCPC_POWER_STATUS				0x1e
#define TCPC_POWER_STATUS_UNINIT		BIT(6)
#define TCPC_POWER_STATUS_VBUS_DET		BIT(3)
#define TCPC_POWER_STATUS_VBUS_PRES		BIT(2)
#define TCPC_POWER_STATUS_SINKING_VBUS	BIT(0)

#define TCPC_FAULT_STATUS               0x1f

#define TCPC_COMMAND					0x23
#define TCPC_CMD_WAKE_I2C				0x11
#define TCPC_CMD_DISABLE_VBUS_DETECT	0x22
#define TCPC_CMD_ENABLE_VBUS_DETECT		0x33
#define TCPC_CMD_DISABLE_SINK_VBUS		0x44
#define TCPC_CMD_SINK_VBUS				0x55
#define TCPC_CMD_DISABLE_SRC_VBUS		0x66
#define TCPC_CMD_SRC_VBUS_DEFAULT		0x77
#define TCPC_CMD_SRC_VBUS_HIGH			0x88
#define TCPC_CMD_LOOK4CONNECTION		0x99
#define TCPC_CMD_RXONEMORE				0xAA
#define TCPC_CMD_I2C_IDLE				0xFF

#define TCPC_DEV_CAP_1					0x24
#define TCPC_DEV_CAP_2					0x26
#define TCPC_STD_INPUT_CAP				0x28
#define TCPC_STD_OUTPUT_CAP				0x29

#define TCPC_MSG_HDR_INFO				0x2e
#define TCPC_MSG_HDR_INFO_DATA_ROLE		BIT(3)
#define TCPC_MSG_HDR_INFO_PWR_ROLE		BIT(0)
#define TCPC_MSG_HDR_INFO_REV_SHIFT		1
#define TCPC_MSG_HDR_INFO_REV_MASK		0x3

#define TCPC_RX_DETECT					0x2f
#define TCPC_RX_DETECT_HARD_RESET		BIT(5)
#define TCPC_RX_DETECT_SOP				BIT(0)

#define TCPC_RX_BYTE_CNT				0x30
#define TCPC_RX_BUF_FRAME_TYPE			0x31
#define TCPC_RX_HDR						0x32
#define TCPC_RX_DATA					0x34 /* through 0x4f */

#define TCPC_TRANSMIT					0x50
#define TCPC_TRANSMIT_RETRY_SHIFT		4
#define TCPC_TRANSMIT_RETRY_MASK		0x3
#define TCPC_TRANSMIT_TYPE_SHIFT		0
#define TCPC_TRANSMIT_TYPE_MASK			0x7

#define TCPC_TX_BYTE_CNT				0x51
#define TCPC_TX_HDR						0x52
#define TCPC_TX_DATA					0x54 /* through 0x6f */

#define TCPC_VBUS_VOLTAGE					0x70
#define TCPC_VBUS_VOL_MASK					0x3ff
#define TCPC_VBUS_VOL_SCALE_FACTOR_MASK		0xc00
#define TCPC_VBUS_VOL_SCALE_FACTOR_SHIFT	10
#define TCPC_VBUS_VOL_MV_UNIT				25

#define TCPC_VBUS_SINK_DISCONNECT_THRESH	0x72
#define TCPC_VBUS_STOP_DISCHARGE_THRESH		0x74
#define TCPC_VBUS_VOLTAGE_ALARM_HI_CFG		0x76
#define TCPC_VBUS_VOLTAGE_ALARM_LO_CFG		0x78

enum typec_role {
	TYPEC_SINK,
	TYPEC_SOURCE,
	TYPEC_ROLE_UNKNOWN,
};

enum typec_data_role {
	TYPEC_DEVICE,
	TYPEC_HOST,
};

enum typec_cc_polarity {
	TYPEC_POLARITY_CC1,
	TYPEC_POLARITY_CC2,
};

enum typec_cc_state {
	TYPEC_STATE_OPEN,
	TYPEC_STATE_SRC_BOTH_RA,
	TYPEC_STATE_SRC_RD_RA,
	TYPEC_STATE_SRC_RD,
	TYPEC_STATE_SRC_RESERVED,
	TYPEC_STATE_SNK_DEFAULT,
	TYPEC_STATE_SNK_POWER15,
	TYPEC_STATE_SNK_POWER30,
};


/* USB PD Messages */
enum pd_ctrl_msg_type {
	/* 0 Reserved */
	PD_CTRL_GOOD_CRC = 1,
	PD_CTRL_GOTO_MIN = 2,
	PD_CTRL_ACCEPT = 3,
	PD_CTRL_REJECT = 4,
	PD_CTRL_PING = 5,
	PD_CTRL_PS_RDY = 6,
	PD_CTRL_GET_SOURCE_CAP = 7,
	PD_CTRL_GET_SINK_CAP = 8,
	PD_CTRL_DR_SWAP = 9,
	PD_CTRL_PR_SWAP = 10,
	PD_CTRL_VCONN_SWAP = 11,
	PD_CTRL_WAIT = 12,
	PD_CTRL_SOFT_RESET = 13,
	/* 14-15 Reserved */
};

enum pd_data_msg_type {
	/* 0 Reserved */
	PD_DATA_SOURCE_CAP = 1,
	PD_DATA_REQUEST = 2,
	PD_DATA_BIST = 3,
	PD_DATA_SINK_CAP = 4,
	/* 5-14 Reserved */
	PD_DATA_VENDOR_DEF = 15,
};

enum tcpc_transmit_type {
	TCPC_TX_SOP = 0,
	TCPC_TX_SOP_PRIME = 1,
	TCPC_TX_SOP_PRIME_PRIME = 2,
	TCPC_TX_SOP_DEBUG_PRIME = 3,
	TCPC_TX_SOP_DEBUG_PRIME_PRIME = 4,
	TCPC_TX_HARD_RESET = 5,
	TCPC_TX_CABLE_RESET = 6,
	TCPC_TX_BIST_MODE_2 = 7
};

enum pd_sink_state{
	UNATTACH = 0,
	ATTACHED,
	WAIT_SOURCE_CAP,
	WAIT_SOURCE_ACCEPT,
	WAIT_SOURCE_READY,
	SINK_READY,
};


#define PD_REV10        0x0
#define PD_REV20        0x1

#define PD_HEADER_CNT_SHIFT     12
#define PD_HEADER_CNT_MASK      0x7
#define PD_HEADER_ID_SHIFT      9
#define PD_HEADER_ID_MASK       0x7
#define PD_HEADER_PWR_ROLE      BIT(8)
#define PD_HEADER_REV_SHIFT     6
#define PD_HEADER_REV_MASK      0x3
#define PD_HEADER_DATA_ROLE     BIT(5)
#define PD_HEADER_TYPE_SHIFT    0
#define PD_HEADER_TYPE_MASK     0xf

#define PD_HEADER(type, pwr, data, id, cnt)                             \
	((((type) & PD_HEADER_TYPE_MASK) << PD_HEADER_TYPE_SHIFT) |     \
	 ((pwr) == TYPEC_SOURCE ? PD_HEADER_PWR_ROLE : 0) |             \
	 ((data) == TYPEC_HOST ? PD_HEADER_DATA_ROLE : 0) |             \
	 (PD_REV20 << PD_HEADER_REV_SHIFT) |                            \
	 (((id) & PD_HEADER_ID_MASK) << PD_HEADER_ID_SHIFT) |           \
	 (((cnt) & PD_HEADER_CNT_MASK) << PD_HEADER_CNT_SHIFT))


static inline unsigned int pd_header_cnt(uint16_t header)
{
	return (header >> PD_HEADER_CNT_SHIFT) & PD_HEADER_CNT_MASK;
}

static inline unsigned int pd_header_cnt_le(__le16 header)
{
	return pd_header_cnt(le16_to_cpu(header));
}

static inline unsigned int pd_header_type(uint16_t header)
{
	return (header >> PD_HEADER_TYPE_SHIFT) & PD_HEADER_TYPE_MASK;
}

static inline unsigned int pd_header_type_le(__le16 header)
{
	return pd_header_type(le16_to_cpu(header));
}

#define PD_MAX_PAYLOAD          7

struct pd_message {
	uint8_t   frametype;
	uint16_t  header;
	uint32_t  payload[PD_MAX_PAYLOAD];
} __packed;

enum pd_pdo_type {
	PDO_TYPE_FIXED = 0,
	PDO_TYPE_BATT = 1,
	PDO_TYPE_VAR = 2,
};


#define PDO_TYPE_SHIFT          30
#define PDO_TYPE_MASK           0x3

#define PDO_TYPE(t)     ((t) << PDO_TYPE_SHIFT)

#define PDO_VOLT_MASK           0x3ff
#define PDO_CURR_MASK           0x3ff
#define PDO_PWR_MASK            0x3ff

#define PDO_FIXED_DUAL_ROLE     BIT(29) /* Power role swap supported */
#define PDO_FIXED_SUSPEND       BIT(28) /* USB Suspend supported (Source) */
#define PDO_FIXED_HIGHER_CAP    BIT(28) /* Requires more than vSafe5V (Sink) */
#define PDO_FIXED_EXTPOWER      BIT(27) /* Externally powered */
#define PDO_FIXED_USB_COMM      BIT(26) /* USB communications capable */
#define PDO_FIXED_DATA_SWAP     BIT(25) /* Data role swap supported */
#define PDO_FIXED_VOLT_SHIFT    10      /* 50mV units */
#define PDO_FIXED_CURR_SHIFT    0       /* 10mA units */

#define PDO_FIXED_VOLT(mv)      ((((mv) / 50) & PDO_VOLT_MASK) << PDO_FIXED_VOLT_SHIFT)
#define PDO_FIXED_CURR(ma)      ((((ma) / 10) & PDO_CURR_MASK) << PDO_FIXED_CURR_SHIFT)

#define PDO_FIXED(mv, ma, flags)                        \
	(PDO_TYPE(PDO_TYPE_FIXED) | (flags) |           \
	 PDO_FIXED_VOLT(mv) | PDO_FIXED_CURR(ma))

#define PDO_BATT_MAX_VOLT_SHIFT 20      /* 50mV units */
#define PDO_BATT_MIN_VOLT_SHIFT 10      /* 50mV units */
#define PDO_BATT_MAX_PWR_SHIFT  0       /* 250mW units */

#define PDO_BATT_MIN_VOLT(mv) ((((mv) / 50) & PDO_VOLT_MASK) << PDO_BATT_MIN_VOLT_SHIFT)
#define PDO_BATT_MAX_VOLT(mv) ((((mv) / 50) & PDO_VOLT_MASK) << PDO_BATT_MAX_VOLT_SHIFT)
#define PDO_BATT_MAX_POWER(mw) ((((mw) / 250) & PDO_PWR_MASK) << PDO_BATT_MAX_PWR_SHIFT)

#define PDO_BATT(min_mv, max_mv, max_mw)                        \
	(PDO_TYPE(PDO_TYPE_BATT) | PDO_BATT_MIN_VOLT(min_mv) |  \
	 PDO_BATT_MAX_VOLT(max_mv) | PDO_BATT_MAX_POWER(max_mw))

#define PDO_VAR_MAX_VOLT_SHIFT  20      /* 50mV units */
#define PDO_VAR_MIN_VOLT_SHIFT  10      /* 50mV units */
#define PDO_VAR_MAX_CURR_SHIFT  0       /* 10mA units */

#define PDO_VAR_MIN_VOLT(mv) ((((mv) / 50) & PDO_VOLT_MASK) << PDO_VAR_MIN_VOLT_SHIFT)
#define PDO_VAR_MAX_VOLT(mv) ((((mv) / 50) & PDO_VOLT_MASK) << PDO_VAR_MAX_VOLT_SHIFT)
#define PDO_VAR_MAX_CURR(ma) ((((ma) / 10) & PDO_CURR_MASK) << PDO_VAR_MAX_CURR_SHIFT)

#define PDO_VAR(min_mv, max_mv, max_ma)                         \
	(PDO_TYPE(PDO_TYPE_VAR) | PDO_VAR_MIN_VOLT(min_mv) |    \
	 PDO_VAR_MAX_VOLT(max_mv) | PDO_VAR_MAX_CURR(max_ma))

static inline enum pd_pdo_type pdo_type(uint32_t pdo)
{
	return (pdo >> PDO_TYPE_SHIFT) & PDO_TYPE_MASK;
}

static inline unsigned int pdo_fixed_voltage(uint32_t pdo)
{
	return ((pdo >> PDO_FIXED_VOLT_SHIFT) & PDO_VOLT_MASK) * 50;
}

static inline unsigned int pdo_min_voltage(uint32_t pdo)
{
	return ((pdo >> PDO_VAR_MIN_VOLT_SHIFT) & PDO_VOLT_MASK) * 50;
}

static inline unsigned int pdo_max_voltage(uint32_t pdo)
{
	return ((pdo >> PDO_VAR_MAX_VOLT_SHIFT) & PDO_VOLT_MASK) * 50;
}

static inline unsigned int pdo_max_current(uint32_t pdo)
{
	return ((pdo >> PDO_VAR_MAX_CURR_SHIFT) & PDO_CURR_MASK) * 10;
}

static inline unsigned int pdo_max_power(uint32_t pdo)
{
	return ((pdo >> PDO_BATT_MAX_PWR_SHIFT) & PDO_PWR_MASK) * 250;
}

/* RDO: Request Data Object */
#define RDO_OBJ_POS_SHIFT       28
#define RDO_OBJ_POS_MASK        0x7
#define RDO_GIVE_BACK           BIT(27) /* Supports reduced operating current */
#define RDO_CAP_MISMATCH        BIT(26) /* Not satisfied by source caps */
#define RDO_USB_COMM            BIT(25) /* USB communications capable */
#define RDO_NO_SUSPEND          BIT(24) /* USB Suspend not supported */

#define RDO_PWR_MASK                    0x3ff
#define RDO_CURR_MASK                   0x3ff

#define RDO_FIXED_OP_CURR_SHIFT         10
#define RDO_FIXED_MAX_CURR_SHIFT        0

#define RDO_OBJ(idx) (((idx) & RDO_OBJ_POS_MASK) << RDO_OBJ_POS_SHIFT)

#define PDO_FIXED_OP_CURR(ma) ((((ma) / 10) & RDO_CURR_MASK) << RDO_FIXED_OP_CURR_SHIFT)
#define PDO_FIXED_MAX_CURR(ma) ((((ma) / 10) & RDO_CURR_MASK) << RDO_FIXED_MAX_CURR_SHIFT)

#define RDO_FIXED(idx, op_ma, max_ma, flags)                    \
	(RDO_OBJ(idx) | (flags) |                               \
	 PDO_FIXED_OP_CURR(op_ma) | PDO_FIXED_MAX_CURR(max_ma))

#define RDO_BATT_OP_PWR_SHIFT           10      /* 250mW units */
#define RDO_BATT_MAX_PWR_SHIFT          0       /* 250mW units */

#define RDO_BATT_OP_PWR(mw) ((((mw) / 250) & RDO_PWR_MASK) << RDO_BATT_OP_PWR_SHIFT)
#define RDO_BATT_MAX_PWR(mw) ((((mw) / 250) & RDO_PWR_MASK) << RDO_BATT_MAX_PWR_SHIFT)

#define RDO_BATT(idx, op_mw, max_mw, flags)                     \
	(RDO_OBJ(idx) | (flags) |                               \
	 RDO_BATT_OP_PWR(op_mw) | RDO_BATT_MAX_PWR(max_mw))

static inline unsigned int rdo_index(u32 rdo)
{
	return (rdo >> RDO_OBJ_POS_SHIFT) & RDO_OBJ_POS_MASK;
}

static inline unsigned int rdo_op_current(u32 rdo)
{
	return ((rdo >> RDO_FIXED_OP_CURR_SHIFT) & RDO_CURR_MASK) * 10;
}

static inline unsigned int rdo_max_current(u32 rdo)
{
	return ((rdo >> RDO_FIXED_MAX_CURR_SHIFT) &
			RDO_CURR_MASK) * 10;
}

static inline unsigned int rdo_op_power(u32 rdo)
{
	return ((rdo >> RDO_BATT_OP_PWR_SHIFT) & RDO_PWR_MASK) * 250;
}

static inline unsigned int rdo_max_power(u32 rdo)
{
	return ((rdo >> RDO_BATT_MAX_PWR_SHIFT) & RDO_PWR_MASK) * 250;
}

#define TCPC_LOG_BUFFER_SIZE 1024

struct tcpc_port;

typedef void (*ss_mux_sel)(enum typec_cc_polarity pol);
typedef int (*ext_pd_switch_setup)(struct tcpc_port *port_p);

enum tcpc_port_type {
	TYPEC_PORT_DFP,
	TYPEC_PORT_UFP,
	TYPEC_PORT_DRP,
};

struct tcpc_port_config {
	uint8_t i2c_bus;
	uint8_t addr;
	enum tcpc_port_type port_type;
	uint32_t max_snk_mv;
	uint32_t max_snk_ma;
	uint32_t max_snk_mw;
	uint32_t op_snk_mv;
	bool disable_pd;
	ext_pd_switch_setup switch_setup_func;
};

struct tcpc_port {
	struct tcpc_port_config cfg;
	struct udevice *i2c_dev;
	ss_mux_sel ss_sel_func;
	enum pd_sink_state pd_state;
	uint32_t tx_msg_id;
	uint32_t log_size;
	char logbuffer[TCPC_LOG_BUFFER_SIZE];
	char *log_p;
	char *log_print;
};

int tcpc_set_cc_to_source(struct tcpc_port *port);
int tcpc_set_cc_to_sink(struct tcpc_port *port);
int tcpc_set_plug_orientation(struct tcpc_port *port, enum typec_cc_polarity polarity);
int tcpc_get_cc_status(struct tcpc_port *port, enum typec_cc_polarity *polarity, enum typec_cc_state *state);
int tcpc_clear_alert(struct tcpc_port *port, uint16_t clear_mask);
int tcpc_send_command(struct tcpc_port *port, uint8_t command);
int tcpc_polling_reg(struct tcpc_port *port, uint8_t reg,
	uint8_t reg_width, uint16_t mask, uint16_t value, ulong timeout_ms);
int tcpc_setup_dfp_mode(struct tcpc_port *port);
int tcpc_setup_ufp_mode(struct tcpc_port *port);
int tcpc_disable_src_vbus(struct tcpc_port *port);
int tcpc_init(struct tcpc_port *port, struct tcpc_port_config config, ss_mux_sel ss_sel_func);
bool tcpc_pd_sink_check_charging(struct tcpc_port *port);
void tcpc_print_log(struct tcpc_port *port);

#ifdef CONFIG_SPL_BUILD
int tcpc_setup_ufp_mode(struct tcpc_port *port)
{
	return 0;
}
int tcpc_setup_dfp_mode(struct tcpc_port *port)
{
	return 0;
}

int tcpc_disable_src_vbus(struct tcpc_port *port)
{
	return 0;
}
#endif
#endif /* __TCPCI_H */
