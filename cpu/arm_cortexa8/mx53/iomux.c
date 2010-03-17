/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @defgroup GPIO_MX53 Board GPIO and Muxing Setup
 * @ingroup MSL_MX53
 */
/*!
 * @file mach-mx53/iomux.c
 *
 * @brief I/O Muxing control functions
 *
 * @ingroup GPIO_MX53
 */
#include <common.h>
#include <asm/io.h>
#include <asm/arch/mx53.h>
#include <asm/arch/mx53_pins.h>
#include <asm/arch/iomux.h>

/*!
 * IOMUX register (base) addresses
 */
enum iomux_reg_addr {
	IOMUXGPR0 = IOMUXC_BASE_ADDR,
	IOMUXGPR1 = IOMUXC_BASE_ADDR + 0x004,
	IOMUXGPR2 = IOMUXC_BASE_ADDR + 0x008,
	IOMUXSW_MUX_CTL = IOMUXC_BASE_ADDR,
	IOMUXSW_MUX_END = IOMUXC_BASE_ADDR + MUX_I_END,
	IOMUXSW_PAD_CTL = IOMUXC_BASE_ADDR + PAD_I_START,
	IOMUXSW_INPUT_CTL = IOMUXC_BASE_ADDR + INPUT_CTL_START,
};

static inline u32 _get_mux_reg(iomux_pin_name_t pin)
{
	u32 mux_reg = PIN_TO_IOMUX_MUX(pin);

	mux_reg += IOMUXSW_MUX_CTL;

	return mux_reg;
}

static inline u32 _get_pad_reg(iomux_pin_name_t pin)
{
	u32 pad_reg = PIN_TO_IOMUX_PAD(pin);

	pad_reg += IOMUXSW_PAD_CTL;

	return pad_reg;
}

static inline u32 _get_mux_end(void)
{
	return IOMUXSW_MUX_END;
}

/*!
 * This function is used to configure a pin through the IOMUX module.
 * FIXED ME: for backward compatible. Will be static function!
 * @param  pin		a pin number as defined in \b #iomux_pin_name_t
 * @param  cfg		an output function as defined in \b #iomux_pin_cfg_t
 *
 * @return 		0 if successful; Non-zero otherwise
 */
static int iomux_config_mux(iomux_pin_name_t pin, iomux_pin_cfg_t cfg)
{
	u32 mux_reg = _get_mux_reg(pin);

	if ((mux_reg > _get_mux_end()) || (mux_reg < IOMUXSW_MUX_CTL))
		return -1;
	if (cfg == IOMUX_CONFIG_GPIO)
		writel(PIN_TO_ALT_GPIO(pin), mux_reg);
	else
		writel(cfg, mux_reg);

	return 0;
}

/*!
 * Request ownership for an IO pin. This function has to be the first one
 * being called before that pin is used. The caller has to check the
 * return value to make sure it returns 0.
 *
 * @param  pin		a name defined by \b iomux_pin_name_t
 * @param  cfg		an input function as defined in \b #iomux_pin_cfg_t
 *
 * @return		0 if successful; Non-zero otherwise
 */
int mxc_request_iomux(iomux_pin_name_t pin, iomux_pin_cfg_t cfg)
{
	int ret = iomux_config_mux(pin, cfg);

	return ret;
}

/*!
 * Release ownership for an IO pin
 *
 * @param  pin		a name defined by \b iomux_pin_name_t
 * @param  cfg		an input function as defined in \b #iomux_pin_cfg_t
 */
void mxc_free_iomux(iomux_pin_name_t pin, iomux_pin_cfg_t cfg)
{
}

/*!
 * This function configures the pad value for a IOMUX pin.
 *
 * @param  pin     a pin number as defined in \b #iomux_pin_name_t
 * @param  config  the ORed value of elements defined in \b #iomux_pad_config_t
 */
void mxc_iomux_set_pad(iomux_pin_name_t pin, u32 config)
{
	u32 pad_reg = _get_pad_reg(pin);

	writel(config, pad_reg);
}

unsigned int mxc_iomux_get_pad(iomux_pin_name_t pin)
{
	u32 pad_reg = _get_pad_reg(pin);

	return readl(pad_reg);
}
/*!
 * This function configures input path.
 *
 * @param input index of input select register as defined in \b
 *  			#iomux_input_select_t
 * @param config the binary value of elements defined in \b
 * 			#iomux_input_config_t
 */
void mxc_iomux_set_input(iomux_input_select_t input, u32 config)
{
	u32 reg = IOMUXSW_INPUT_CTL + (input << 2);

	writel(config, reg);
}
