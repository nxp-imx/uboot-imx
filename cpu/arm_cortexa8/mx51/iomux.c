/*
 * Copyright 2009 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @defgroup GPIO_MX51 Board GPIO and Muxing Setup
 * @ingroup MSL_MX51
 */
/*!
 * @file mach-mx51/iomux.c
 *
 * @brief I/O Muxing control functions
 *
 * @ingroup GPIO_MX51
 */
#include <common.h>
#include <asm/io.h>
#include <asm/arch/mx51.h>
#include <asm/arch/mx51_pins.h>
#include <asm/arch/iomux.h>

/*!
 * IOMUX register (base) addresses
 */
enum iomux_reg_addr {
	IOMUXGPR0 = IOMUXC_BASE_ADDR,
	IOMUXGPR1 = IOMUXC_BASE_ADDR + 0x004,
	IOMUXSW_MUX_CTL = IOMUXC_BASE_ADDR,
	IOMUXSW_MUX_END = IOMUXC_BASE_ADDR + MUX_I_END,
	IOMUXSW_PAD_CTL = IOMUXC_BASE_ADDR + PAD_I_START,
	IOMUXSW_INPUT_CTL = IOMUXC_BASE_ADDR,
};

#define MUX_PIN_NUM_MAX (((MUX_I_END - MUX_I_START) >> 2) + 1)

static inline u32 _get_mux_reg(iomux_pin_name_t pin)
{
	u32 mux_reg = PIN_TO_IOMUX_MUX(pin);

	if (is_soc_rev(CHIP_REV_2_0) < 0) {
		if ((pin == MX51_PIN_NANDF_RB5) ||
			(pin == MX51_PIN_NANDF_RB6) ||
			(pin == MX51_PIN_NANDF_RB7))
			; /* Do nothing */
		else if (mux_reg >= 0x2FC)
			mux_reg += 8;
		else if (mux_reg >= 0x130)
			mux_reg += 0xC;
	}
	mux_reg += IOMUXSW_MUX_CTL;
	return mux_reg;
}

static inline u32 _get_pad_reg(iomux_pin_name_t pin)
{
	u32 pad_reg = PIN_TO_IOMUX_PAD(pin);

	if (is_soc_rev(CHIP_REV_2_0) < 0) {
		if ((pin == MX51_PIN_NANDF_RB5) ||
			(pin == MX51_PIN_NANDF_RB6) ||
			(pin == MX51_PIN_NANDF_RB7))
			; /* Do nothing */
		else if (pad_reg == 0x4D0 - PAD_I_START)
			pad_reg += 0x4C;
		else if (pad_reg == 0x860 - PAD_I_START)
			pad_reg += 0x9C;
		else if (pad_reg >= 0x804 - PAD_I_START)
			pad_reg += 0xB0;
		else if (pad_reg >= 0x7FC - PAD_I_START)
			pad_reg += 0xB4;
		else if (pad_reg >= 0x4E4 - PAD_I_START)
			pad_reg += 0xCC;
		else
			pad_reg += 8;
	}
	pad_reg += IOMUXSW_PAD_CTL;
	return pad_reg;
}

static inline u32 _get_mux_end()
{
	if (is_soc_rev(CHIP_REV_2_0) < 0)
		return IOMUXC_BASE_ADDR + (0x3F8 - 4);
	else
		return IOMUXC_BASE_ADDR + (0x3F0 - 4);
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

	if (is_soc_rev(CHIP_REV_2_0) < 0) {
		if (input == MUX_IN_IPU_IPP_DI_0_IND_DISPB_SD_D_SELECT_INPUT)
			input -= 4;
		else if (input ==
			 MUX_IN_IPU_IPP_DI_1_IND_DISPB_SD_D_SELECT_INPUT)
			input -= 3;
		else if (input >= MUX_IN_KPP_IPP_IND_COL_6_SELECT_INPUT)
			input -= 2;
		else if (input >=
			 MUX_IN_HSC_MIPI_MIX_PAR_SISG_TRIG_SELECT_INPUT)
			input -= 5;
		else if (input >=
			 MUX_IN_HSC_MIPI_MIX_IPP_IND_SENS1_DATA_EN_SELECT_INPUT)
			input -= 3;
		else if (input >= MUX_IN_ECSPI2_IPP_IND_SS_B_3_SELECT_INPUT)
			input -= 2;
		else if (input >= MUX_IN_CCM_PLL1_BYPASS_CLK_SELECT_INPUT)
			input -= 1;

		reg += INPUT_CTL_START_TO1;
	} else {
		reg += INPUT_CTL_START;
	}

	writel(config, reg);
}
