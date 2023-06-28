// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 NXP
 */

#include <common.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <errno.h>
#include <misc.h>
#include <scmi_agent.h>
#include <scmi_protocols.h>
#include <asm/io.h>
#include <asm/mach-imx/iomux-v3.h>
#include <linux/bitops.h>

#include "pinctrl-imx.h"

#if defined(CONFIG_IMX93)
#define DAISY_OFFSET	0x360
#endif
#if defined(CONFIG_IMX95)
#define DAISY_OFFSET	0x408
#endif

/* SCMI pin control types */
#define PINCTRL_TYPE_MUX        192
#define PINCTRL_TYPE_CONFIG     193
#define PINCTRL_TYPE_DAISY_ID   194
#define PINCTRL_TYPE_DAISY_CFG  195
#define PINCTRL_NUM_CFGS_SHIFT  2

static int imx_pinconf_scmi_set(struct udevice *dev, u32 mux_ofs, u32 mux, u32 config_val,
				u32 input_ofs, u32 input_val)
{
	struct imx_pinctrl_priv *priv = dev_get_priv(dev);
	int ret, num_cfgs = 0;

	/* Call SCMI API to set the pin mux and configuration. */
	struct scmi_pinctrl_config_set_out out;
	struct scmi_pinctrl_config_set_in in = {
		.identifier = mux_ofs / 4,
		.attributes = 0,
	};
	if (mux_ofs != 0) {
		in.configs[num_cfgs].type = PINCTRL_TYPE_MUX;
		in.configs[num_cfgs].val = mux;
		num_cfgs++;
	}
	if (config_val != 0) {
		in.configs[num_cfgs].type = PINCTRL_TYPE_CONFIG;
		in.configs[num_cfgs].val = config_val;
		num_cfgs++;
	}
	if (input_ofs != 0) {
		in.configs[num_cfgs].type = PINCTRL_TYPE_DAISY_ID;
		in.configs[num_cfgs].val = (input_ofs - DAISY_OFFSET) / 4;
		num_cfgs++;
		in.configs[num_cfgs].type = PINCTRL_TYPE_DAISY_CFG;
		in.configs[num_cfgs].val = input_val;
		num_cfgs++;
	}
	/* Update the number of configs sent in this call. */
	in.attributes = num_cfgs << PINCTRL_NUM_CFGS_SHIFT;

	struct scmi_msg msg = SCMI_MSG_IN(SCMI_PROTOCOL_ID_PINCTRL,
					  SCMI_MSG_PINCTRL_CONFIG_SET, in, out);

	ret = devm_scmi_process_msg(dev, priv->channel, &msg);
	if (ret != 0 || out.status != 0)
		dev_err(dev, "Failed to set PAD = %d, daisy = %d, scmi_err = %d, ret = %d\n", mux_ofs / 4, input_ofs / 4, out.status, ret);

	return ret;
}

int imx_pinctrl_scmi_conf_pins(struct udevice *dev, u32 *pin_data, int npins)
{
	int mux_ofs, mux, config_val, input_reg, input_val;
	int i, j = 0;
	int ret;

	/*
	 * Refer to linux documentation for details:
	 * Documentation/devicetree/bindings/pinctrl/fsl,imx-pinctrl.txt
	 */
	for (i = 0; i < npins; i++) {
		mux_ofs = pin_data[j++];
		/* Skip config_reg */
		j++;
		input_reg = pin_data[j++];

		mux = pin_data[j++];
		input_val = pin_data[j++];
		config_val = pin_data[j++];

		if (config_val & IMX_PAD_SION)
			mux |= IOMUXC_CONFIG_SION;

		config_val &= ~IMX_PAD_SION;

		ret = imx_pinconf_scmi_set(dev, mux_ofs, mux, config_val, input_reg, input_val);
		if (ret && ret != -EPERM)
			dev_err(dev, "Set pin %d, mux %d, val %d, error\n",
				mux_ofs, mux, config_val);
	}

	return ret;
}

static struct imx_pinctrl_soc_info imx_pinctrl_scmi_soc_info __section(".data") = {
	.flags = ZERO_OFFSET_VALID | IMX_USE_SCMI,
};

static int imx_scmi_pinctrl_probe(struct udevice *dev)
{
	struct imx_pinctrl_priv *priv = dev_get_priv(dev);
	int ret;

	ret = devm_scmi_of_get_channel(dev, &priv->channel);
	if (ret) {
		dev_err(dev, "get channel: %d\n", ret);
		return ret;
	}

	debug("%s %p %s\n", __func__, priv, dev->name);
	return imx_pinctrl_probe(dev, &imx_pinctrl_scmi_soc_info);
}

U_BOOT_DRIVER(scmi_pinctrl_imx) = {
	.name = "scmi_pinctrl_imx",
	.id = UCLASS_PINCTRL,
	.probe = imx_scmi_pinctrl_probe,
	.remove = imx_pinctrl_remove,
	.priv_auto = sizeof(struct imx_pinctrl_priv),
	.ops = &imx_pinctrl_ops,
	.flags = DM_FLAG_PRE_RELOC,
};
