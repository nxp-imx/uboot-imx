/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __MACH_MX53_IOMUX_H__
#define __MACH_MX53_IOMUX_H__

#include <common.h>
#include <asm/io.h>
#include <asm/arch/mx53.h>
#include <asm/arch/mx53_pins.h>

/*!
 * @file mach-mx53/iomux.h
 *
 * @brief I/O Muxing control definitions and functions
 *
 * @ingroup GPIO_MX53
 */

typedef unsigned int iomux_pin_name_t;

/*!
 * various IOMUX output functions
 */
typedef enum iomux_config {
	IOMUX_CONFIG_ALT0,	/*!< used as alternate function 0 */
	IOMUX_CONFIG_ALT1,	/*!< used as alternate function 1 */
	IOMUX_CONFIG_ALT2,	/*!< used as alternate function 2 */
	IOMUX_CONFIG_ALT3,	/*!< used as alternate function 3 */
	IOMUX_CONFIG_ALT4,	/*!< used as alternate function 4 */
	IOMUX_CONFIG_ALT5,	/*!< used as alternate function 5 */
	IOMUX_CONFIG_ALT6,	/*!< used as alternate function 6 */
	IOMUX_CONFIG_ALT7,	/*!< used as alternate function 7 */
	IOMUX_CONFIG_GPIO,	/*!< added to help user use GPIO mode */
	IOMUX_CONFIG_SION = 0x1 << 4,	/*!< used as LOOPBACK:MUX SION bit */
} iomux_pin_cfg_t;

/*!
 * various IOMUX pad functions
 */
typedef enum iomux_pad_config {
	PAD_CTL_SRE_SLOW = 0x0 << 0,
	PAD_CTL_SRE_FAST = 0x1 << 0,
	PAD_CTL_DRV_LOW = 0x0 << 1,
	PAD_CTL_DRV_MEDIUM = 0x1 << 1,
	PAD_CTL_DRV_HIGH = 0x2 << 1,
	PAD_CTL_DRV_MAX = 0x3 << 1,
	PAD_CTL_ODE_OPENDRAIN_NONE = 0x0 << 3,
	PAD_CTL_ODE_OPENDRAIN_ENABLE = 0x1 << 3,
	PAD_CTL_360K_PD	= 0x0 << 4,
	PAD_CTL_75K_PU = 0x1 << 4,
	PAD_CTL_100K_PU = 0x2 << 4,
	PAD_CTL_22K_PU = 0x3 << 4,
	PAD_CTL_PUE_KEEPER = 0x0 << 6,
	PAD_CTL_PUE_PULL = 0x1 << 6,
	PAD_CTL_PKE_NONE = 0x0 << 7,
	PAD_CTL_PKE_ENABLE = 0x1 << 7,
	PAD_CTL_HYS_NONE = 0x0 << 8,
	PAD_CTL_HYS_ENABLE = 0x1 << 8,
	PAD_CTL_DDR_INPUT_CMOS = 0x0 << 9,
	PAD_CTL_DDR_INPUT_DDR = 0x1 << 9,
	PAD_CTL_DRV_VOT_LOW = 0x0 << 13,
	PAD_CTL_DRV_VOT_HIGH = 0x1 << 13,
} iomux_pad_config_t;

/*!
 * various IOMUX input select register index
 */
typedef enum iomux_input_select {
	MUX_IN_AUDMUX_P4_INPUT_DA_AMX_SELECT_I = 0,
	MUX_IN_AUDMUX_P4_INPUT_DB_AMX_SELECT_I,
	MUX_IN_AUDMUX_P4_INPUT_RXCLK_AMX_SELECT_INPUT,
	MUX_IN_AUDMUX_P4_INPUT_RXFS_AMX_SELECT_INPUT,
	MUX_IN_AUDMUX_P4_INPUT_TXCLK_AMX_SELECT_INPUT,
	MUX_IN_AUDMUX_P4_INPUT_TXFS_AMX_SELECT_INPUT,
	MUX_IN_AUDMUX_P5_INPUT_DA_AMX_SELECT_I,
	MUX_IN_AUDMUX_P5_INPUT_DB_AMX_SELECT_I,
	MUX_IN_AUDMUX_P5_INPUT_RXCLK_AMX_SELECT_INPUT,
	MUX_IN_AUDMUX_P5_INPUT_RXFS_AMX_SELECT_INPUT,
	MUX_IN_AUDMUX_P5_INPUT_TXCLK_AMX_SELECT_INPUT,
	MUX_IN_AUDMUX_P5_INPUT_TXFS_AMX_SELECT_INPUT,
	MUX_IN_CAN1_IPP_IND_CANRX_SELECT_INPUT,		/*0x760*/
	MUX_IN_CAN2_IPP_IND_CANRX_SELECT_INPUT,
	MUX_IN_CCM_IPP_ASRC_EXT_SELECT_INPUT,
	MUX_IN_CCM_IPP_DI1_CLK_SELECT_INPUT,
	MUX_IN_CCM_PLL1_BYPASS_CLK_SELECT_INPUT,
	MUX_IN_CCM_PLL2_BYPASS_CLK_SELECT_INPUT,
	MUX_IN_CCM_PLL3_BYPASS_CLK_SELECT_INPUT,
	MUX_IN_CCM_PLL4_BYPASS_CLK_SELECT_INPUT,
	MUX_IN_CSPI_IPP_CSPI_CLK_IN_SELECT_INPUT,	/*0x780*/
	MUX_IN_CSPI_IPP_IND_MISO_SELECT_INPUT,
	MUX_IN_CSPI_IPP_IND_MOSI_SELECT_INPUT,
	MUX_IN_CSPI_IPP_IND_SS_B_1_SELECT_INPUT,
	MUX_IN_CSPI_IPP_IND_SS_B_2_SELECT_INPUT,
	MUX_IN_CSPI_IPP_IND_SS_B_3_SELECT_INPUT,
	MUX_IN_CSPI_IPP_IND_SS_B_4_SELECT_INPUT,
	MUX_IN_ECSPI1_IPP_CSPI_CLK_IN_SELECT_INPUT,
	MUX_IN_ECSPI1_IPP_IND_MISO_SELECT_INPUT,
	MUX_IN_ECSPI1_IPP_IND_MOSI_SELECT_INPUT,
	MUX_IN_ECSPI1_IPP_IND_SS_B_1_SELECT_INPUT,
	MUX_IN_ECSPI1_IPP_IND_SS_B_2_SELECT_INPUT,
	MUX_IN_ECSPI1_IPP_IND_SS_B_3_SELECT_INPUT,	/*0x7B0*/
	MUX_IN_ECSPI1_IPP_IND_SS_B_4_SELECT_INPUT,
	MUX_IN_ECSPI2_IPP_CSPI_CLK_IN_SELECT_INPUT,
	MUX_IN_ECSPI2_IPP_IND_MISO_SELECT_INPUT,
	MUX_IN_ECSPI2_IPP_IND_MOSI_SELECT_INPUT,
	MUX_IN_ECSPI2_IPP_IND_SS_B_1_SELECT_INPUT,
	MUX_IN_ECSPI2_IPP_IND_SS_B_2_SELECT_INPUT,
	MUX_IN_ESAI1_IPP_IND_FSR_SELECT_INPUT,
	MUX_IN_ESAI1_IPP_IND_FST_SELECT_INPUT,
	MUX_IN_ESAI1_IPP_IND_HCKR_SELECT_INPUT,
	MUX_IN_ESAI1_IPP_IND_HCKT_SELECT_INPUT,
	MUX_IN_ESAI1_IPP_IND_SCKR_SELECT_INPUT,
	MUX_IN_ESAI1_IPP_IND_SCKT_SELECT_INPUT,		/*0x7E0*/
	MUX_IN_ESAI1_IPP_IND_SDO0_SELECT_INPUT,
	MUX_IN_ESAI1_IPP_IND_SDO1_SELECT_INPUT,
	MUX_IN_ESAI1_IPP_IND_SDO2_SDI3_SELECT_INPUT,
	MUX_IN_ESAI1_IPP_IND_SDO3_SDI2_SELECT_INPUT,
	MUX_IN_ESAI1_IPP_IND_SDO4_SDI1_SELECT_INPUT,
	MUX_IN_ESAI1_IPP_IND_SDO5_SDI0_SELECT_INPUT,
	MUX_IN_ESDHC1_IPP_WP_ON_SELECT_INPUT,
	MUX_IN_FEC_FEC_COL_SELECT_INPUT,	/*0x800*/
	MUX_IN_FEC_FEC_MDI_SELECT_INPUT,
	MUX_IN_FEC_FEC_RX_CLK_SELECT_INPUT,
	MUX_IN_FIRI_IPP_IND_RXD_SELECT_INPUT,
	MUX_IN_GPC_PMIC_RDY_SELECT_INPUT,
	MUX_IN_I2C1_IPP_SCL_IN_SELECT_INPUT,
	MUX_IN_I2C1_IPP_SDA_IN_SELECT_INPUT,
	MUX_IN_I2C2_IPP_SCL_IN_SELECT_INPUT,
	MUX_IN_I2C2_IPP_SDA_IN_SELECT_INPUT,
	MUX_IN_I2C3_IPP_SCL_IN_SELECT_INPUT,
	MUX_IN_I2C3_IPP_SDA_IN_SELECT_INPUT,
	MUX_IN_IPU_IPP_DI_0_IND_DISPB_SD_D_SELECT_INPUT,
	MUX_IN_IPU_IPP_DI_1_IND_DISPB_SD_D_SELECT_INPUT,
	MUX_IN_IPU_IPP_IND_SENS1_DATA_EN_SELECT_INPUT,
	MUX_IN_IPU_IPP_IND_SENS1_HSYNC_SELECT_INPUT,
	MUX_IN_IPU_IPP_IND_SENS1_VSYNC_SELECT_INPUT,
	MUX_IN_KPP_IPP_IND_COL_5_SELECT_INPUT,	/*0x840*/
	MUX_IN_KPP_IPP_IND_COL_6_SELECT_INPUT,
	MUX_IN_KPP_IPP_IND_COL_7_SELECT_INPUT,
	MUX_IN_KPP_IPP_IND_ROW_5_SELECT_INPUT,
	MUX_IN_KPP_IPP_IND_ROW_6_SELECT_INPUT,
	MUX_IN_KPP_IPP_IND_ROW_7_SELECT_INPUT,
	MUX_IN_MLB_MLBCLK_IN_SELECT_INPUT,
	MUX_IN_MLB_MLBDAT_IN_SELECT_INPUT,
	MUX_IN_MLB_MLBSIG_IN_SELECT_INPUT,
	MUX_IN_OWIRE_BATTERY_LINE_IN_SELECT_INPUT,
	MUX_IN_SDMA_EVENTS_14_SELECT_INPUT,
	MUX_IN_SDMA_EVENTS_15_SELECT_INPUT,
	MUX_IN_SPDIF_SPDIF_IN1_SELECT_INPUT,	/*0x870*/
	MUX_IN_UART1_IPP_UART_RTS_B_SELECT_INPUT,
	MUX_IN_UART1_IPP_UART_RXD_MUX_SELECT_INPUT,
	MUX_IN_UART2_IPP_UART_RTS_B_SELECT_INPUT,
	MUX_IN_UART2_IPP_UART_RXD_MUX_SELECT_INPUT,
	MUX_IN_UART3_IPP_UART_RTS_B_SELECT_INPUT,
	MUX_IN_UART3_IPP_UART_RXD_MUX_SELECT_INPUT,
	MUX_IN_UART4_IPP_UART_RTS_B_SELECT_INPUT,
	MUX_IN_UART4_IPP_UART_RXD_MUX_SELECT_INPUT,
	MUX_IN_UART5_IPP_UART_RTS_B_SELECT_INPUT,
	MUX_IN_UART5_IPP_UART_RXD_MUX_SELECT_INPUT,
	MUX_IN_USBOH3_IPP_IND_OTG_OC_SELECT_INPUT,
	MUX_IN_USBOH3_IPP_IND_UH1_OC_SELECT_INPUT,
	MUX_IN_USBOH3_IPP_IND_UH2_OC_SELECT_INPUT,
} iomux_input_select_t;

/*!
 * various IOMUX input functions
 */
typedef enum iomux_input_config {
	INPUT_CTL_PATH0 = 0x0,
	INPUT_CTL_PATH1,
	INPUT_CTL_PATH2,
	INPUT_CTL_PATH3,
	INPUT_CTL_PATH4,
	INPUT_CTL_PATH5,
	INPUT_CTL_PATH6,
	INPUT_CTL_PATH7,
} iomux_input_config_t;

struct mxc_iomux_pin_cfg {
	iomux_pin_name_t pin;
	u8 mux_mode;
	u16 pad_cfg;
	u8 in_select;
	u8 in_mode;
};

/*!
 * Request ownership for an IO pin. This function has to be the first one
 * being called before that pin is used. The caller has to check the
 * return value to make sure it returns 0.
 *
 * @param  pin		a name defined by \b iomux_pin_name_t
 * @param  config	config as defined in \b #iomux_pin_ocfg_t
 *
 * @return		0 if successful; Non-zero otherwise
 */
int mxc_request_iomux(iomux_pin_name_t pin, iomux_pin_cfg_t config);

/*!
 * Release ownership for an IO pin
 *
 * @param  pin		a name defined by \b iomux_pin_name_t
 * @param  config	config as defined in \b #iomux_pin_ocfg_t
 */
void mxc_free_iomux(iomux_pin_name_t pin, iomux_pin_cfg_t config);

/*!
 * This function configures the pad value for a IOMUX pin.
 *
 * @param  pin          a pin number as defined in \b #iomux_pin_name_t
 * @param  config      the ORed value of elements defined in
 *                             \b #iomux_pad_config_t
 */
void mxc_iomux_set_pad(iomux_pin_name_t pin, u32 config);

/*!
 * This function gets the current pad value for a IOMUX pin.
 *
 * @param  pin          a pin number as defined in \b #iomux_pin_name_t
 * @return		current pad value
 */
unsigned int mxc_iomux_get_pad(iomux_pin_name_t pin);

/*!
 * This function configures input path.
 *
 * @param  input        index of input select register as defined in
 *                              \b #iomux_input_select_t
 * @param  config       the binary value of elements defined in \b #iomux_input_config_t
 */
void mxc_iomux_set_input(iomux_input_select_t input, u32 config);

#endif				/*  __MACH_MX53_IOMUX_H__ */
