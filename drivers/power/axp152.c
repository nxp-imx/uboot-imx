/*
 * (C) Copyright 2012
 * Henrik Nordstrom <henrik@henriknordstrom.net>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <command.h>
#include <i2c.h>
#include <axp_pmic.h>
#include <errno.h>

#define AXP152_I2C_ADDR	0x32

static int pmic_bus_init(void)
{
	return 0;
}

static int pmic_bus_read(u8 reg, u8 *data)
{
	return i2c_read(AXP152_I2C_ADDR, reg, 1, data, 1);
}

static int pmic_bus_write(u8 reg, u8 data)
{
	return i2c_write(AXP152_I2C_ADDR, reg, 1, &data, 1);
}

static u8 axp152_mvolt_to_target(int mvolt, int min, int max, int div)
{
	if (mvolt < min)
		mvolt = min;
	else if (mvolt > max)
		mvolt = max;

	return (mvolt - min) / div;
}

int axp_set_dcdc1(enum axp152_dcdc1_voltages volt)
{
	if (volt < AXP152_DCDC1_1V7 || volt > AXP152_DCDC1_3V5)
		return -EINVAL;

	return pmic_bus_write(AXP152_DCDC1_VOLTAGE, volt);
}

int axp_set_dcdc2(unsigned int mvolt)
{
	int rc;
	u8 current, target;

	target = axp152_mvolt_to_target(mvolt, 700, 2275, 25);

	/* Do we really need to be this gentle? It has built-in voltage slope */
	while ((rc = pmic_bus_read(AXP152_DCDC2_VOLTAGE, &current)) == 0 &&
	       current != target) {
		if (current < target)
			current++;
		else
			current--;
		rc = pmic_bus_write(AXP152_DCDC2_VOLTAGE, current);
		if (rc)
			break;
	}
	return rc;
}

int axp_set_dcdc3(unsigned int mvolt)
{
	u8 target = axp152_mvolt_to_target(mvolt, 700, 3500, 50);

	return pmic_bus_write(AXP152_DCDC3_VOLTAGE, target);
}

int axp_set_dcdc4(unsigned int mvolt)
{
	u8 target = axp152_mvolt_to_target(mvolt, 700, 3500, 25);

	return pmic_bus_write(AXP152_DCDC4_VOLTAGE, target);
}

int axp_set_ldo0(enum axp152_ldo0_volts volt, enum axp152_ldo0_curr_limit curr_limit)
{
	u8 target = curr_limit | (volt << 4) | (1 << 7);

	return pmic_bus_write(AXP152_LDO0_VOLTAGE, target);
}

int axp_disable_ldo0(void)
{
	int ret;
	u8 target;

	ret = pmic_bus_read(AXP152_LDO0_VOLTAGE, &target);
	if (ret)
		return ret;

	target &= ~(1 << 7);

	return pmic_bus_write(AXP152_LDO0_VOLTAGE, target);
}

int axp_set_ldo1(unsigned int mvolt)
{
	u8 target = axp152_mvolt_to_target(mvolt, 700, 3500, 100);

	return pmic_bus_write(AXP152_LDO1_VOLTAGE, target);
}


int axp_set_ldo2(unsigned int mvolt)
{
	u8 target = axp152_mvolt_to_target(mvolt, 700, 3500, 100);

	return pmic_bus_write(AXP152_LDO2_VOLTAGE, target);
}

int axp_set_aldo1(enum axp152_aldo_voltages volt)
{
	u8 val;
	int ret;

	ret = pmic_bus_read(AXP152_ALDO1_ALDO2_VOLTAGE, &val);
	if (ret)
		return ret;

	val |= (volt << 4);
	return pmic_bus_write(AXP152_ALDO1_ALDO2_VOLTAGE, val);
}

int axp_set_aldo2(enum axp152_aldo_voltages volt)
{
	u8 val;
	int ret;

	ret = pmic_bus_read(AXP152_ALDO1_ALDO2_VOLTAGE, &val);
	if (ret)
		return ret;

	val |= volt;
	return pmic_bus_write(AXP152_ALDO1_ALDO2_VOLTAGE, val);
}

int axp_set_power_output(int val)
{
	return pmic_bus_write(AXP152_POWER_CONTROL, val);
}

int axp_init(void)
{
	u8 ver;
	int rc;
	int ret;
	u8 reg;

	rc = pmic_bus_init();
	if (rc)
		return rc;

	rc = pmic_bus_read(AXP152_CHIP_VERSION, &ver);
	if (rc)
		return rc;

	if (ver != 0x05)
		return -EINVAL;

	/* Set the power off sequence to `reverse of power on sequence` */
	ret = pmic_bus_read(AXP152_SHUTDOWN, &reg);
	if (ret)
		return ret;
	reg |= AXP152_POWEROFF_SEQ;
	ret = pmic_bus_write(AXP152_SHUTDOWN, reg);
	if (ret)
		return ret;


	/* Enable the power recovery */
	ret = pmic_bus_read(AXP152_POWER_RECOVERY, &reg);
	if (ret)
		return ret;
	reg |= AXP152_POWER_RECOVERY_EN;
	ret = pmic_bus_write(AXP152_POWER_RECOVERY, reg);
	return ret;

}

int do_poweroff(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	pmic_bus_write(AXP152_SHUTDOWN, AXP152_POWEROFF);

	/* infinite loop during shutdown */
	while (1) {}

	/* not reached */
	return 0;
}
