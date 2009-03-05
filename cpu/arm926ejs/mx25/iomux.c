/*
 * Copyright 2008-2009 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @defgroup GPIO_MX25 Board GPIO and Muxing Setup
 * @ingroup MSL_MX25
 */
/*!
 * @file mach-mx25/iomux.c
 *
 * @brief I/O Muxing control functions
 *
 * @ingroup GPIO_MX25
 */

#include <common.h>
#include <asm/arch/mx25.h>
#include <asm/arch/mx25_pins.h>
#include <asm/arch/iomux.h>

/*!
 * IOMUX register (base) addresses
 */
enum iomux_reg_addr {
	IOMUXGPR = IOMUXC_BASE,
	/*!< General purpose */
	IOMUXSW_MUX_CTL = IOMUXC_BASE + 0x008,
	/*!< MUX control */
	IOMUXSW_MUX_END = IOMUXC_BASE + 0x228,
	/*!< last MUX control register */
	IOMUXSW_PAD_CTL = IOMUXC_BASE + 0x22C,
	/*!< Pad control */
	IOMUXSW_PAD_END = IOMUXC_BASE + 0x414,
	/*!< last Pad control register */
	IOMUXSW_INPUT_CTL = IOMUXC_BASE + 0x460,
	/*!< input select register */
	IOMUXSW_INPUT_END = IOMUXC_BASE + 0x580,
	/*!< last input select register */
};

#define MUX_PIN_NUM_MAX		\
		(((IOMUXSW_MUX_END - IOMUXSW_MUX_CTL) >> 2) + 1)
#define MUX_INPUT_NUM_MUX	\
		(((IOMUXSW_INPUT_END - IOMUXSW_INPUT_CTL) >> 2) + 1)

#define PIN_TO_IOMUX_INDEX(pin) (PIN_TO_IOMUX_MUX(pin) >> 2)

#define MUX_USED 0x80

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
	u32 mux_reg = PIN_TO_IOMUX_MUX(pin);

	if (mux_reg != NON_MUX_I) {
		mux_reg += IOMUXGPR;
		__REG(mux_reg) = cfg;
	}

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
	u32 pad_reg = IOMUXGPR + PIN_TO_IOMUX_PAD(pin);

	__REG(pad_reg) = config;
}

/*!
 * This function enables/disables the general purpose function for a particular
 * signal.
 *
 * @param  gp   one signal as defined in \b #iomux_gp_func_t
 * @param  en   \b #true to enable; \b #false to disable
 */
void mxc_iomux_set_gpr(iomux_gp_func_t gp, int en)
{
	u32 l;

	l = __REG(IOMUXGPR);
	if (en)
		l |= gp;
	else
		l &= ~gp;

	__REG(IOMUXGPR) = l;
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

	__REG(reg) = config;
}

