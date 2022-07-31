/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2015-2019, Arm Limited and Contributors. All rights reserved.
 * Copyright (C) 2019-2020, Linaro Limited
 */
#ifndef _SCMI_PROTOCOLS_H
#define _SCMI_PROTOCOLS_H

#include <linux/bitops.h>
#include <asm/types.h>

/*
 * Subset the SCMI protocols definition
 * based on SCMI specification v2.0 (DEN0056B)
 * https://developer.arm.com/docs/den0056/b
 */

enum scmi_std_protocol {
	SCMI_PROTOCOL_ID_BASE = 0x10,
	SCMI_PROTOCOL_ID_POWER_DOMAIN = 0x11,
	SCMI_PROTOCOL_ID_SYSTEM = 0x12,
	SCMI_PROTOCOL_ID_PERF = 0x13,
	SCMI_PROTOCOL_ID_CLOCK = 0x14,
	SCMI_PROTOCOL_ID_SENSOR = 0x15,
	SCMI_PROTOCOL_ID_RESET_DOMAIN = 0x16,
	SCMI_PROTOCOL_ID_VOLTAGE_DOMAIN = 0x17,
};

enum scmi_status_code {
	SCMI_SUCCESS =  0,
	SCMI_NOT_SUPPORTED = -1,
	SCMI_INVALID_PARAMETERS = -2,
	SCMI_DENIED = -3,
	SCMI_NOT_FOUND = -4,
	SCMI_OUT_OF_RANGE = -5,
	SCMI_BUSY = -6,
	SCMI_COMMS_ERROR = -7,
	SCMI_GENERIC_ERROR = -8,
	SCMI_HARDWARE_ERROR = -9,
	SCMI_PROTOCOL_ERROR = -10,
};

enum scmi_common_message_id {
	SCMI_PROTOCOL_VERSION = 0x000,
	SCMI_PROTOCOL_ATTRIBUTES = 0x001,
	SCMI_PROTOCOL_MESSAGE_ATTRIBUTES = 0x002
};

/*
 * SCMI Clock Protocol
 */

enum scmi_clock_message_id {
	SCMI_CLOCK_RATE_SET = 0x5,
	SCMI_CLOCK_RATE_GET = 0x6,
	SCMI_CLOCK_CONFIG_SET = 0x7,
};

#define SCMI_CLK_RATE_ASYNC_NOTIFY	BIT(0)
#define SCMI_CLK_RATE_ASYNC_NORESP	(BIT(0) | BIT(1))
#define SCMI_CLK_RATE_ROUND_DOWN	0
#define SCMI_CLK_RATE_ROUND_UP		BIT(2)
#define SCMI_CLK_RATE_ROUND_CLOSEST	BIT(3)

/**
 * struct scmi_clk_state_in - Message payload for CLOCK_CONFIG_SET command
 * @clock_id:	SCMI clock ID
 * @attributes:	Attributes of the targets clock state
 */
struct scmi_clk_state_in {
	u32 clock_id;
	u32 attributes;
};

/**
 * struct scmi_clk_state_out - Response payload for CLOCK_CONFIG_SET command
 * @status:	SCMI command status
 */
struct scmi_clk_state_out {
	s32 status;
};

/**
 * struct scmi_clk_state_in - Message payload for CLOCK_RATE_GET command
 * @clock_id:	SCMI clock ID
 * @attributes:	Attributes of the targets clock state
 */
struct scmi_clk_rate_get_in {
	u32 clock_id;
};

/**
 * struct scmi_clk_rate_get_out - Response payload for CLOCK_RATE_GET command
 * @status:	SCMI command status
 * @rate_lsb:	32bit LSB of the clock rate in Hertz
 * @rate_msb:	32bit MSB of the clock rate in Hertz
 */
struct scmi_clk_rate_get_out {
	s32 status;
	u32 rate_lsb;
	u32 rate_msb;
};

/**
 * struct scmi_clk_state_in - Message payload for CLOCK_RATE_SET command
 * @flags:	Flags for the clock rate set request
 * @clock_id:	SCMI clock ID
 * @rate_lsb:	32bit LSB of the clock rate in Hertz
 * @rate_msb:	32bit MSB of the clock rate in Hertz
 */
struct scmi_clk_rate_set_in {
	u32 flags;
	u32 clock_id;
	u32 rate_lsb;
	u32 rate_msb;
};

/**
 * struct scmi_clk_rate_set_out - Response payload for CLOCK_RATE_SET command
 * @status:	SCMI command status
 */
struct scmi_clk_rate_set_out {
	s32 status;
};

/*
 * SCMI Reset Domain Protocol
 */

enum scmi_reset_domain_message_id {
	SCMI_RESET_DOMAIN_ATTRIBUTES = 0x3,
	SCMI_RESET_DOMAIN_RESET = 0x4,
};

#define SCMI_RD_NAME_LEN		16

#define SCMI_RD_ATTRIBUTES_FLAG_ASYNC	BIT(31)
#define SCMI_RD_ATTRIBUTES_FLAG_NOTIF	BIT(30)

#define SCMI_RD_RESET_FLAG_ASYNC	BIT(2)
#define SCMI_RD_RESET_FLAG_ASSERT	BIT(1)
#define SCMI_RD_RESET_FLAG_CYCLE	BIT(0)

/**
 * struct scmi_rd_attr_in - Payload for RESET_DOMAIN_ATTRIBUTES message
 * @domain_id:	SCMI reset domain ID
 */
struct scmi_rd_attr_in {
	u32 domain_id;
};

/**
 * struct scmi_rd_attr_out - Payload for RESET_DOMAIN_ATTRIBUTES response
 * @status:	SCMI command status
 * @attributes:	Retrieved attributes of the reset domain
 * @latency:	Reset cycle max lantency
 * @name:	Reset domain name
 */
struct scmi_rd_attr_out {
	s32 status;
	u32 attributes;
	u32 latency;
	char name[SCMI_RD_NAME_LEN];
};

/**
 * struct scmi_rd_reset_in - Message payload for RESET command
 * @domain_id:		SCMI reset domain ID
 * @flags:		Flags for the reset request
 * @reset_state:	Reset target state
 */
struct scmi_rd_reset_in {
	u32 domain_id;
	u32 flags;
	u32 reset_state;
};

/**
 * struct scmi_rd_reset_out - Response payload for RESET command
 * @status:	SCMI command status
 */
struct scmi_rd_reset_out {
	s32 status;
};

/*
 * SCMI Voltage Domain Protocol
 */

enum scmi_voltage_domain_message_id {
	SCMI_VOLTAGE_DOMAIN_ATTRIBUTES = 0x3,
	SCMI_VOLTAGE_DOMAIN_CONFIG_SET = 0x5,
	SCMI_VOLTAGE_DOMAIN_CONFIG_GET = 0x6,
	SCMI_VOLTAGE_DOMAIN_LEVEL_SET = 0x7,
	SCMI_VOLTAGE_DOMAIN_LEVEL_GET = 0x8,
};

#define SCMI_VOLTD_NAME_LEN		16

#define SCMI_VOLTD_CONFIG_MASK		GENMASK(3, 0)
#define SCMI_VOLTD_CONFIG_OFF		0
#define SCMI_VOLTD_CONFIG_ON		0x7

/**
 * struct scmi_voltd_attr_in - Payload for VOLTAGE_DOMAIN_ATTRIBUTES message
 * @domain_id:	SCMI voltage domain ID
 */
struct scmi_voltd_attr_in {
	u32 domain_id;
};

/**
 * struct scmi_voltd_attr_out - Payload for VOLTAGE_DOMAIN_ATTRIBUTES response
 * @status:	SCMI command status
 * @attributes:	Retrieved attributes of the voltage domain
 * @name:	Voltage domain name
 */
struct scmi_voltd_attr_out {
	s32 status;
	u32 attributes;
	char name[SCMI_VOLTD_NAME_LEN];
};

/**
 * struct scmi_voltd_config_set_in - Message payload for VOLTAGE_CONFIG_SET cmd
 * @domain_id:	SCMI voltage domain ID
 * @config:	Configuration data of the voltage domain
 */
struct scmi_voltd_config_set_in {
	u32 domain_id;
	u32 config;
};

/**
 * struct scmi_voltd_config_set_out - Response for VOLTAGE_CONFIG_SET command
 * @status:	SCMI command status
 */
struct scmi_voltd_config_set_out {
	s32 status;
};

/**
 * struct scmi_voltd_config_get_in - Message payload for VOLTAGE_CONFIG_GET cmd
 * @domain_id:	SCMI voltage domain ID
 */
struct scmi_voltd_config_get_in {
	u32 domain_id;
};

/**
 * struct scmi_voltd_config_get_out - Response for VOLTAGE_CONFIG_GET command
 * @status:	SCMI command status
 * @config:	Configuration data of the voltage domain
 */
struct scmi_voltd_config_get_out {
	s32 status;
	u32 config;
};

/**
 * struct scmi_voltd_level_set_in - Message payload for VOLTAGE_LEVEL_SET cmd
 * @domain_id:		SCMI voltage domain ID
 * @flags:		Parameter flags for configuring target level
 * @voltage_level:	Target voltage level in microvolts (uV)
 */
struct scmi_voltd_level_set_in {
	u32 domain_id;
	u32 flags;
	s32 voltage_level;
};

/**
 * struct scmi_voltd_level_set_out - Response for VOLTAGE_LEVEL_SET command
 * @status:	SCMI	command status
 */
struct scmi_voltd_level_set_out {
	s32 status;
};

/**
 * struct scmi_voltd_level_get_in - Message payload for VOLTAGE_LEVEL_GET cmd
 * @domain_id:		SCMI voltage domain ID
 */
struct scmi_voltd_level_get_in {
	u32 domain_id;
};

/**
 * struct scmi_voltd_level_get_out - Response for VOLTAGE_LEVEL_GET command
 * @status:		SCMI command status
 * @voltage_level:	Voltage level in microvolts (uV)
 */
struct scmi_voltd_level_get_out {
	s32 status;
	s32 voltage_level;
};

/*
 * SCMI Power Protocol
 */

enum scmi_power_message_id {
	SCMI_POWER_DOMAIN_ATTRIBUTES = 0x3,
	SCMI_POWER_STATE_SET = 0x4,
	SCMI_POWER_STATE_GET = 0x5,
	SCMI_POWER_STATE_NOTIFY = 0x6,
};


#define SCMI_POWER_STATE_GENERIC_ON		(0 << 30)
#define SCMI_POWER_STATE_GENERIC_OFF	(1 << 30)

struct scmi_power_set_state {
	u32 flags;
	u32 domain;
	u32 state;
};

struct scmi_power_set_state_out {
	s32 status;
};

struct scmi_power_get_state {
	u32 domain;
};

struct scmi_power_get_state_out {
	s32 status;
	u32 state;
};

/*
 * SCMI Sensor protocol
 */

enum scmi_sensor_message_id {
	SCMI_SENSOR_DESCRIPTION_GET = 0x3,
	SCMI_SENSOR_TRIP_POINT_NOTIFY = 0x4,
	SCMI_SENSOR_TRIP_POINT_CONFIG = 0x5,
	SCMI_SENSOR_READING_GET = 0x6,
	SCMI_SENSOR_AXIS_DESCRIPTION_GET = 0x7,
	SCMI_SENSOR_LIST_UPDATE_INTERVALS = 0x8,
	SCMI_SENSOR_CONFIG_GET = 0x9,
	SCMI_SENSOR_CONFIG_SET = 0xA,
	SCMI_SENSOR_CONTINUOUS_UPDATE_NOTIFY = 0xB,
};

struct scmi_protocol_attributes_p2a_sensor {
	int32_t status;
	int16_t num_sensors;
	uint8_t max_reqs;
	uint8_t res;
	uint32_t sensor_reg_low;
	uint32_t sensor_reg_high;
	uint32_t sensor_reg_len;
};

#define SCMI_MAX_STR_SIZE   16

struct scmi_msg_resp_attrs {
	s32 min_range_low;
	s32 min_range_high;
	s32 max_range_low;
	s32 max_range_high;
};

struct scmi_sensor_desc {
	u32 id;
	u32 attr_low;
	u32 attr_high;
	u8 name[SCMI_MAX_STR_SIZE];
	u32 power;
	u32 resolution;
	struct scmi_msg_resp_attrs scalar_attrs;
};

struct scmi_sensor_description_get_a2p {
	uint32_t desc_index;
};

struct scmi_sensor_descrition_get_p2a {
	int32_t status;
	uint32_t num_sensor_flags;
	struct scmi_sensor_desc desc[1];
};

struct scmi_sensor_config_get_a2p {
	uint32_t sensor_id;
};

struct scmi_sensor_config_get_p2a {
	int32_t status;
	uint32_t sensor_config;
};

/*
 * Sensor Reading Get
 */
struct scmi_sensor_reading_get_a2p {
	uint32_t sensor_id;
	uint32_t flags;
};

struct scmi_sensor_val {
	uint32_t value_low;
	uint32_t value_high;
	uint32_t timestap_low;
	uint32_t timestap_high;
};

struct scmi_sensor_reading_get_p2a {
	int32_t status;
	struct scmi_sensor_val val;
};

#endif /* _SCMI_PROTOCOLS_H */
