/*
 * (C) Copyright 2012 Henrik Nordstrom <henrik@henriknordstrom.net>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

enum axp152_reg {
	AXP152_CHIP_VERSION = 0x3,
	AXP152_POWER_CONTROL = 0x12,
	AXP152_LDO0_VOLTAGE = 0x15,
	AXP152_DCDC2_VOLTAGE = 0x23,
	AXP152_DCDC1_VOLTAGE = 0x26,
	AXP152_DCDC3_VOLTAGE = 0x27,
	AXP152_DCDC4_VOLTAGE = 0x2B,
	AXP152_LDO1_VOLTAGE = 0x29,
	AXP152_LDO2_VOLTAGE = 0x2A,
	AXP152_ALDO1_ALDO2_VOLTAGE = 0x28,
	AXP152_POWER_RECOVERY = 0x31,
	AXP152_SHUTDOWN = 0x32,
	AXP152_GPIO0 = 0x90,
};

enum axp152_ldo0_volts {
       AXP152_LDO0_5V = 0,
       AXP152_LDO0_3V3 = 1,
       AXP152_LDO0_2V8 = 2,
       AXP152_LDO0_2V5 = 3,
};

enum axp152_ldo0_curr_limit {
       AXP152_LDO0_CURR_NOLMIT = 0,
       AXP152_LDO0_CURR_1500MA = 1,
       AXP152_LDO0_CURR_900MA = 2,
       AXP152_LDO0_CURR_500MA = 3,
};

enum axp152_dcdc1_voltages {
       AXP152_DCDC1_1V7 = 0,
       AXP152_DCDC1_1V8 = 1,
       AXP152_DCDC1_1V9 = 2,
       AXP152_DCDC1_2V0 = 3,
       AXP152_DCDC1_2V1 = 4,
       AXP152_DCDC1_2V4 = 5,
       AXP152_DCDC1_2V5 = 6,
       AXP152_DCDC1_2V6 = 7,
       AXP152_DCDC1_2V7 = 8,
       AXP152_DCDC1_2V8 = 9,
       AXP152_DCDC1_3V0 = 10,
       AXP152_DCDC1_3V1 = 11,
       AXP152_DCDC1_3V2 = 12,
       AXP152_DCDC1_3V3 = 13,
       AXP152_DCDC1_3V4 = 14,
       AXP152_DCDC1_3V5 = 15,
};

enum axp152_aldo_voltages {
       AXP152_ALDO_1V2 = 0,
       AXP152_ALDO_1V3 = 1,
       AXP152_ALDO_1V4 = 2,
       AXP152_ALDO_1V5 = 3,
       AXP152_ALDO_1V6 = 4,
       AXP152_ALDO_1V7 = 5,
       AXP152_ALDO_1V8 = 6,
       AXP152_ALDO_1V9 = 7,
       AXP152_ALDO_2V0 = 8,
       AXP152_ALDO_2V5 = 9,
       AXP152_ALDO_2V7 = 10,
       AXP152_ALDO_2V8 = 11,
       AXP152_ALDO_3V0 = 12,
       AXP152_ALDO_3V1 = 13,
       AXP152_ALDO_3V2 = 14,
       AXP152_ALDO_3V3 = 15,
};

#define AXP152_POWEROUT_DC_DC1 BIT(7)
#define AXP152_POWEROUT_DC_DC2 BIT(6)
#define AXP152_POWEROUT_DC_DC3 BIT(5)
#define AXP152_POWEROUT_DC_DC4 BIT(4)
#define AXP152_POWEROUT_ALDO1  BIT(3)
#define AXP152_POWEROUT_ALDO2  BIT(2)
#define AXP152_POWEROUT_DLDO1  BIT(1)
#define AXP152_POWEROUT_DLDO2  BIT(0)

#define AXP152_POWEROFF			(1 << 7)
#define AXP152_POWEROFF_SEQ            (1 << 2)
#define AXP152_POWER_RECOVERY_EN       (1 << 3)

/* For axp_gpio.c */
#define AXP_GPIO0_CTRL			0x90
#define AXP_GPIO1_CTRL			0x91
#define AXP_GPIO2_CTRL			0x92
#define AXP_GPIO3_CTRL			0x93
#define AXP_GPIO_CTRL_OUTPUT_LOW		0x00 /* Drive pin low */
#define AXP_GPIO_CTRL_OUTPUT_HIGH		0x01 /* Drive pin high */
#define AXP_GPIO_CTRL_INPUT			0x02 /* Input */
#define AXP_GPIO_STATE			0x97
#define AXP_GPIO_STATE_OFFSET			0

int axp_set_dcdc1(enum axp152_dcdc1_voltages volt);
int axp_set_dcdc2(unsigned int mvolt);
int axp_set_dcdc3(unsigned int mvolt);
int axp_set_dcdc4(unsigned int mvolt);
int axp_set_ldo0(enum axp152_ldo0_volts volt, enum axp152_ldo0_curr_limit curr_limit);
int axp_disable_ldo0(void);
int axp_set_ldo1(unsigned int mvolt);
int axp_set_ldo2(unsigned int mvolt);
int axp_set_aldo1(enum axp152_aldo_voltages volt);
int axp_set_aldo2(enum axp152_aldo_voltages volt);
int axp_set_power_output(int val);
int axp_init(void);
