/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/imx-common/sci/sci.h>
#include "pinctrl-imx.h"

#define PADRING_IFMUX_EN_SHIFT		31
#define PADRING_IFMUX_EN_MASK		(1 << 31)
#define PADRING_GP_EN_SHIFT			30
#define PADRING_GP_EN_MASK			(1 << 30)
#define PADRING_IFMUX_SHIFT			27
#define PADRING_IFMUX_MASK			(0x7 << 27)


static int imx_pinconf_scu_set(struct imx_pinctrl_soc_info *info,
	unsigned int pin_id, unsigned int mux, unsigned int val)
{
	sc_err_t err = SC_ERR_NONE;
	sc_ipc_t ipcHndl = (sc_ipc_t)info->base;

	/*
	 * Mux should be done in pmx set, but we do not have a good api
	 * to handle that in scfw, so config it in pad conf func
	 */

	if (ipcHndl == 0) {
		printf("IPC handle not initialized!\n");
		return -EIO;
	}

	val |= PADRING_IFMUX_EN_MASK;
	val |= PADRING_GP_EN_MASK;
	val |= (mux << PADRING_IFMUX_SHIFT) & PADRING_IFMUX_MASK;

	err = sc_pad_set(ipcHndl, pin_id, val);

	if (err != SC_ERR_NONE)
		return -EIO;

	return 0;
}


int imx_pinctrl_scu_process_pins(struct imx_pinctrl_soc_info *info, u32 *pin_data, int npins)
{
	int pin_id, mux, config_val;
	int i, j = 0;
	int ret;

	/*
	 * Refer to linux documentation for details:
	 * Documentation/devicetree/bindings/pinctrl/fsl,imx-pinctrl.txt
	 */
	for (i = 0; i < npins; i++) {
		pin_id = pin_data[j++];
		mux = pin_data[j++];
		config_val = pin_data[j++];

		ret = imx_pinconf_scu_set(info, pin_id, mux, config_val);
		if (ret)
			printf("Set pin %d, mux %d, val %d, error\n", pin_id, mux, config_val);
	}

	return 0;
}
