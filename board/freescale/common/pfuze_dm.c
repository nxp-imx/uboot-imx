/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <power/pmic.h>
#include <power/pfuze100_pmic.h>

int pfuze_mode_init(struct udevice *dev, u32 mode)
{
	unsigned char offset, i, switch_num;
	u32 id;
	int ret;

	id = pmic_reg_read(dev, PFUZE100_DEVICEID);
	id = id & 0xf;

	if (id == 0) {
		switch_num = 6;
		offset = PFUZE100_SW1CMODE;
	} else if (id == 1) {
		switch_num = 4;
		offset = PFUZE100_SW2MODE;
	} else {
		printf("Not supported, id=%d\n", id);
		return -EINVAL;
	}

	ret = pmic_reg_write(dev, PFUZE100_SW1ABMODE, mode);
	if (ret < 0) {
		printf("Set SW1AB mode error!\n");
		return ret;
	}

	for (i = 0; i < switch_num - 1; i++) {
		ret = pmic_reg_write(dev, offset + i * SWITCH_SIZE, mode);
		if (ret < 0) {
			printf("Set switch 0x%x mode error!\n",
			       offset + i * SWITCH_SIZE);
			return ret;
		}
	}

	return ret;
}

struct udevice *pfuze_common_init(void)
{
	struct udevice *dev;
	int ret;
	unsigned int reg, dev_id, rev_id;

	ret = pmic_get("pfuze100", &dev);
	if (ret == -ENODEV)
		return NULL;

	dev_id = pmic_reg_read(dev, PFUZE100_DEVICEID);
	rev_id = pmic_reg_read(dev, PFUZE100_REVID);
	printf("PMIC: PFUZE100! DEV_ID=0x%x REV_ID=0x%x\n", dev_id, rev_id);

	/* Set SW1AB stanby volage to 0.975V */
	reg = pmic_reg_read(dev, PFUZE100_SW1ABSTBY);
	reg &= ~SW1x_STBY_MASK;
	reg |= SW1x_0_975V;
	pmic_reg_write(dev, PFUZE100_SW1ABSTBY, reg);

	/* Set SW1AB/VDDARM step ramp up time from 16us to 4us/25mV */
	reg = pmic_reg_read(dev, PFUZE100_SW1ABCONF);
	reg &= ~SW1xCONF_DVSSPEED_MASK;
	reg |= SW1xCONF_DVSSPEED_4US;
	pmic_reg_write(dev, PFUZE100_SW1ABCONF, reg);

	/* Set SW1C standby voltage to 0.975V */
	reg = pmic_reg_read(dev, PFUZE100_SW1CSTBY);
	reg &= ~SW1x_STBY_MASK;
	reg |= SW1x_0_975V;
	pmic_reg_write(dev, PFUZE100_SW1CSTBY, reg);

	/* Set SW1C/VDDSOC step ramp up time from 16us to 4us/25mV */
	reg = pmic_reg_read(dev, PFUZE100_SW1CCONF);
	reg &= ~SW1xCONF_DVSSPEED_MASK;
	reg |= SW1xCONF_DVSSPEED_4US;
	pmic_reg_write(dev, PFUZE100_SW1CCONF, reg);

	return dev;
}
