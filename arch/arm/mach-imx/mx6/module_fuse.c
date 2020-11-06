/*
 * (C) Copyright 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <fdt_support.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/module_fuse.h>

struct fuse_entry_desc {
	enum fuse_module_type module;
	const char *node_path;
	u32 fuse_word_offset;
	u32 fuse_bit_offset;
	u32 status;
};

static struct fuse_entry_desc mx6_fuse_descs[] = {
#if defined(CONFIG_MX6ULL)
	{MX6_MODULE_TSC, "/soc/bus@2000000/tsc@2040000", 0x430, 22},
	{MX6_MODULE_ADC2, "/soc/bus@2100000/adc@219c000", 0x430, 23},
	{MX6_MODULE_EPDC, "/soc/bus@2200000/epdc@228c000", 0x430, 24},
	{MX6_MODULE_ESAI, "/soc/bus@2000000/spba-bus@2000000/esai@2024000", 0x430, 25},
	{MX6_MODULE_FLEXCAN1, "/soc/bus@2000000/can@2090000", 0x430, 26},
	{MX6_MODULE_FLEXCAN2, "/soc/bus@2000000/can@2094000", 0x430, 27},
	{MX6_MODULE_SPDIF, "/soc/bus@2000000/spba-bus@2000000/spdif@2004000", 0x440, 2},
	{MX6_MODULE_EIM, "/soc/bus@2100000/weim@21b8000", 0x440, 3},
	{MX6_MODULE_SD1, "/soc/bus@2100000/usdhc@2190000", 0x440, 4},
	{MX6_MODULE_SD2, "/soc/bus@2100000/usdhc@2194000", 0x440, 5},
	{MX6_MODULE_QSPI1, "/soc/bus@2100000/qspi@21e0000", 0x440, 6},
	{MX6_MODULE_GPMI, "/soc/nand-controller@1806000", 0x440, 7},
	{MX6_MODULE_APBHDMA, "/soc/dma-apbh@1804000", 0x440, 7},
	{MX6_MODULE_LCDIF, "/soc/bus@2100000/lcdif@21c8000", 0x440, 8},
	{MX6_MODULE_PXP, "/soc/bus@2100000/pxp@21cc000", 0x440, 9},
	{MX6_MODULE_CSI, "/soc/bus@2100000/csi@21c4000", 0x440, 10},
	{MX6_MODULE_ADC1, "/soc/bus@2100000/adc@2198000", 0x440, 11},
	{MX6_MODULE_ENET1, "/soc/bus@2100000/ethernet@2188000", 0x440, 12},
	{MX6_MODULE_ENET2, "/soc/bus@2000000/ethernet@20b4000", 0x440, 13},
	{MX6_MODULE_DCP, "/soc/bus@2200000/dcp@2280000", 0x440, 14},
	{MX6_MODULE_USB_OTG2, "/soc/bus@2100000/usb@2184200", 0x440, 15},
	{MX6_MODULE_SAI2, "/soc/bus@2000000/spba-bus@2000000/sai@202c000", 0x440, 24},
	{MX6_MODULE_SAI3, "/soc/bus@2000000/spba-bus@2000000/sai@2030000", 0x440, 24},
	{MX6_MODULE_DCP_CRYPTO, "/soc/bus@2200000/dcp@2280000", 0x440, 25},
	{MX6_MODULE_UART5, "/soc/bus@2100000/serial@21f4000", 0x440, 26},
	{MX6_MODULE_UART6, "/soc/bus@2100000/serial@21fc000", 0x440, 26},
	{MX6_MODULE_UART7, "/soc/bus@2000000/spba-bus@2000000/serial@2018000", 0x440, 26},
	{MX6_MODULE_UART8, "/soc/bus@2200000/serial@2288000", 0x440, 26},
	{MX6_MODULE_PWM5, "/soc/bus@2000000/pwm@20f0000", 0x440, 27},
	{MX6_MODULE_PWM6, "/soc/bus@2000000/pwm@20f4000", 0x440, 27},
	{MX6_MODULE_PWM7, "/soc/bus@2000000/pwm@20f8000", 0x440, 27},
	{MX6_MODULE_PWM8, "/soc/bus@2000000/pwm@20fc000", 0x440, 27},
	{MX6_MODULE_ECSPI3, "/soc/bus@2000000/spba-bus@2000000/ecspi@2010000", 0x440, 28},
	{MX6_MODULE_ECSPI4, "/soc/bus@2000000/spba-bus@2000000/ecspi@2014000", 0x440, 28},
	{MX6_MODULE_I2C3, "/soc/bus@2100000/i2c@21a8000", 0x440, 29},
	{MX6_MODULE_I2C4, "/soc/bus@2100000/i2c@21f8000", 0x440, 29},
	{MX6_MODULE_GPT2, "/soc/bus@2000000/gpt@20e8000", 0x440, 30},
	{MX6_MODULE_EPIT2, "/soc/bus@2000000/epit@20d4000", 0x440, 31},

	{MX6_MODULE_TSC, "/soc/aips-bus@2000000/tsc@2040000", 0x430, 22},
	{MX6_MODULE_ADC2, "/soc/aips-bus@2100000/adc@219c000", 0x430, 23},
	{MX6_MODULE_EPDC, "/soc/aips-bus@2200000/epdc@228c000", 0x430, 24},
	{MX6_MODULE_ESAI, "/soc/aips-bus@2000000/spba-bus@2000000/esai@2024000", 0x430, 25},
	{MX6_MODULE_FLEXCAN1, "/soc/aips-bus@2000000/can@2090000", 0x430, 26},
	{MX6_MODULE_FLEXCAN2, "/soc/aips-bus@2000000/can@2094000", 0x430, 27},
	{MX6_MODULE_SPDIF, "/soc/aips-bus@2000000/spba-bus@2000000/spdif@2004000", 0x440, 2},
	{MX6_MODULE_EIM, "/soc/aips-bus@2100000/weim@21b8000", 0x440, 3},
	{MX6_MODULE_SD1, "/soc/aips-bus@2100000/usdhc@2190000", 0x440, 4},
	{MX6_MODULE_SD2, "/soc/aips-bus@2100000/usdhc@2194000", 0x440, 5},
	{MX6_MODULE_QSPI1, "/soc/aips-bus@2100000/qspi@21e0000", 0x440, 6},
	{MX6_MODULE_GPMI, "/soc/gpmi-nand@1806000", 0x440, 7},
	{MX6_MODULE_APBHDMA, "/soc/dma-apbh@1804000", 0x440, 7},
	{MX6_MODULE_LCDIF, "/soc/aips-bus@2100000/lcdif@21c8000", 0x440, 8},
	{MX6_MODULE_PXP, "/soc/aips-bus@2100000/pxp@21cc000", 0x440, 9},
	{MX6_MODULE_CSI, "/soc/aips-bus@2100000/csi@21c4000", 0x440, 10},
	{MX6_MODULE_ADC1, "/soc/aips-bus@2100000/adc@2198000", 0x440, 11},
	{MX6_MODULE_ENET1, "/soc/aips-bus@2100000/ethernet@2188000", 0x440, 12},
	{MX6_MODULE_ENET2, "/soc/aips-bus@2000000/ethernet@20b4000", 0x440, 13},
	{MX6_MODULE_DCP, "/soc/aips-bus@2200000/dcp@2280000", 0x440, 14},
	{MX6_MODULE_USB_OTG2, "/soc/aips-bus@2100000/usb@2184200", 0x440, 15},
	{MX6_MODULE_SAI2, "/soc/aips-bus@2000000/spba-bus@2000000/sai@202c000", 0x440, 24},
	{MX6_MODULE_SAI3, "/soc/aips-bus@2000000/spba-bus@2000000/sai@2030000", 0x440, 24},
	{MX6_MODULE_DCP_CRYPTO, "/soc/aips-bus@2200000/dcp@2280000", 0x440, 25},
	{MX6_MODULE_UART5, "/soc/aips-bus@2100000/serial@21f4000", 0x440, 26},
	{MX6_MODULE_UART6, "/soc/aips-bus@2100000/serial@21fc000", 0x440, 26},
	{MX6_MODULE_UART7, "/soc/aips-bus@2000000/spba-bus@2000000/serial@2018000", 0x440, 26},
	{MX6_MODULE_UART8, "/soc/aips-bus@2200000/serial@2288000", 0x440, 26},
	{MX6_MODULE_PWM5, "/soc/aips-bus@2000000/pwm@20f0000", 0x440, 27},
	{MX6_MODULE_PWM6, "/soc/aips-bus@2000000/pwm@20f4000", 0x440, 27},
	{MX6_MODULE_PWM7, "/soc/aips-bus@2000000/pwm@20f8000", 0x440, 27},
	{MX6_MODULE_PWM8, "/soc/aips-bus@2000000/pwm@20fc000", 0x440, 27},
	{MX6_MODULE_ECSPI3, "/soc/aips-bus@2000000/spba-bus@2000000/ecspi@2010000", 0x440, 28},
	{MX6_MODULE_ECSPI4, "/soc/aips-bus@2000000/spba-bus@2000000/ecspi@2014000", 0x440, 28},
	{MX6_MODULE_I2C3, "/soc/aips-bus@2100000/i2c@21a8000", 0x440, 29},
	{MX6_MODULE_I2C4, "/soc/aips-bus@2100000/i2c@21f8000", 0x440, 29},
	{MX6_MODULE_GPT2, "/soc/aips-bus@2000000/gpt@20e8000", 0x440, 30},
	{MX6_MODULE_EPIT2, "/soc/aips-bus@2000000/epit@20d4000", 0x440, 31},
	/* Paths for older imx tree: */
	{MX6_MODULE_TSC, "/soc/aips-bus@02000000/tsc@02040000", 0x430, 22},
	{MX6_MODULE_ADC2, "/soc/aips-bus@02100000/adc@0219c000", 0x430, 23},
	{MX6_MODULE_EPDC, "/soc/aips-bus@02200000/epdc@0228c000", 0x430, 24},
	{MX6_MODULE_ESAI, "/soc/aips-bus@02000000/spba-bus@02000000/esai@02024000", 0x430, 25},
	{MX6_MODULE_FLEXCAN1, "/soc/aips-bus@02000000/can@02090000", 0x430, 26},
	{MX6_MODULE_FLEXCAN2, "/soc/aips-bus@02000000/can@02094000", 0x430, 27},
	{MX6_MODULE_SPDIF, "/soc/aips-bus@02000000/spba-bus@02000000/spdif@02004000", 0x440, 2},
	{MX6_MODULE_EIM, "/soc/aips-bus@02100000/weim@021b8000", 0x440, 3},
	{MX6_MODULE_SD1, "/soc/aips-bus@02100000/usdhc@02190000", 0x440, 4},
	{MX6_MODULE_SD2, "/soc/aips-bus@02100000/usdhc@02194000", 0x440, 5},
	{MX6_MODULE_QSPI1, "/soc/aips-bus@02100000/qspi@021e0000", 0x440, 6},
	{MX6_MODULE_GPMI, "/soc/gpmi-nand@01806000", 0x440, 7},
	{MX6_MODULE_APBHDMA, "/soc/dma-apbh@01804000", 0x440, 7},
	{MX6_MODULE_LCDIF, "/soc/aips-bus@02100000/lcdif@021c8000", 0x440, 8},
	{MX6_MODULE_PXP, "/soc/aips-bus@02100000/pxp@021cc000", 0x440, 9},
	{MX6_MODULE_CSI, "/soc/aips-bus@02100000/csi@021c4000", 0x440, 10},
	{MX6_MODULE_ADC1, "/soc/aips-bus@02100000/adc@02198000", 0x440, 11},
	{MX6_MODULE_ENET1, "/soc/aips-bus@02100000/ethernet@02188000", 0x440, 12},
	{MX6_MODULE_ENET2, "/soc/aips-bus@02000000/ethernet@020b4000", 0x440, 13},
	{MX6_MODULE_DCP, "/soc/aips-bus@02200000/dcp@02280000", 0x440, 14},
	{MX6_MODULE_USB_OTG2, "/soc/aips-bus@02100000/usb@02184200", 0x440, 15},
	{MX6_MODULE_SAI2, "/soc/aips-bus@02000000/spba-bus@02000000/sai@0202c000", 0x440, 24},
	{MX6_MODULE_SAI3, "/soc/aips-bus@02000000/spba-bus@02000000/sai@02030000", 0x440, 24},
	{MX6_MODULE_DCP_CRYPTO, "/soc/aips-bus@02200000/dcp@02280000", 0x440, 25},
	{MX6_MODULE_UART5, "/soc/aips-bus@02100000/serial@021f4000", 0x440, 26},
	{MX6_MODULE_UART6, "/soc/aips-bus@02100000/serial@021fc000", 0x440, 26},
	{MX6_MODULE_UART7, "/soc/aips-bus@02000000/spba-bus@02000000/serial@02018000", 0x440, 26},
	{MX6_MODULE_UART8, "/soc/aips-bus@02200000/serial@02288000", 0x440, 26},
	{MX6_MODULE_PWM5, "/soc/aips-bus@02000000/pwm@020f0000", 0x440, 27},
	{MX6_MODULE_PWM6, "/soc/aips-bus@02000000/pwm@020f4000", 0x440, 27},
	{MX6_MODULE_PWM7, "/soc/aips-bus@02000000/pwm@020f8000", 0x440, 27},
	{MX6_MODULE_PWM8, "/soc/aips-bus@02000000/pwm@020fc000", 0x440, 27},
	{MX6_MODULE_ECSPI3, "/soc/aips-bus@02000000/spba-bus@02000000/ecspi@02010000", 0x440, 28},
	{MX6_MODULE_ECSPI4, "/soc/aips-bus@02000000/spba-bus@02000000/ecspi@02014000", 0x440, 28},
	{MX6_MODULE_I2C3, "/soc/aips-bus@02100000/i2c@021a8000", 0x440, 29},
	{MX6_MODULE_I2C4, "/soc/aips-bus@02100000/i2c@021f8000", 0x440, 29},
	{MX6_MODULE_GPT2, "/soc/aips-bus@02000000/gpt@020e8000", 0x440, 30},
	{MX6_MODULE_EPIT2, "/soc/aips-bus@02000000/epit@020d4000", 0x440, 31},
#elif defined(CONFIG_MX6UL)
	{MX6_MODULE_TSC, "/soc/bus@2000000/tsc@2040000", 0x430, 22},
	{MX6_MODULE_ADC2, "/soc/bus@2100000/adc@219c000", 0x430, 23},
	{MX6_MODULE_SIM1, "/soc/bus@2100000/sim@218c000", 0x430, 24},
	{MX6_MODULE_SIM2, "/soc/bus@2100000/sim@21b4000", 0x430, 25},
	{MX6_MODULE_FLEXCAN1, "/soc/bus@2000000/can@2090000", 0x430, 26},
	{MX6_MODULE_FLEXCAN2, "/soc/bus@2000000/can@2094000", 0x430, 27},
	{MX6_MODULE_SPDIF, "/soc/bus@2000000/spba-bus@2000000/spdif@2004000", 0x440, 2},
	{MX6_MODULE_EIM, "/soc/bus@2100000/weim@21b8000", 0x440, 3},
	{MX6_MODULE_SD1, "/soc/bus@2100000/usdhc@2190000", 0x440, 4},
	{MX6_MODULE_SD2, "/soc/bus@2100000/usdhc@2194000", 0x440, 5},
	{MX6_MODULE_QSPI1, "/soc/bus@2100000/qspi@21e0000", 0x440, 6},
	{MX6_MODULE_GPMI, "/soc/nand-controller@1806000", 0x440, 7},
	{MX6_MODULE_APBHDMA, "/soc/dma-apbh@1804000", 0x440, 7},
	{MX6_MODULE_LCDIF, "/soc/bus@2100000/lcdif@21c8000", 0x440, 8},
	{MX6_MODULE_PXP, "/soc/bus@2100000/pxp@21cc000", 0x440, 9},
	{MX6_MODULE_CSI, "/soc/bus@2100000/csi@21c4000", 0x440, 10},
	{MX6_MODULE_ADC1, "/soc/bus@2100000/adc@2198000", 0x440, 11},
	{MX6_MODULE_ENET1, "/soc/bus@2100000/ethernet@2188000", 0x440, 12},
	{MX6_MODULE_ENET2, "/soc/bus@2000000/ethernet@20b4000", 0x440, 13},
	{MX6_MODULE_CAAM, "/soc/bus@2100000/caam@2140000", 0x440, 14},
	{MX6_MODULE_USB_OTG2, "/soc/bus@2100000/usb@2184200", 0x440, 15},
	{MX6_MODULE_SAI2, "/soc/bus@2000000/spba-bus@2000000/sai@202c000", 0x440, 24},
	{MX6_MODULE_SAI3, "/soc/bus@2000000/spba-bus@2000000/sai@2030000", 0x440, 24},
	{MX6_MODULE_BEE, "/soc/bus@2000000/bee@2044000", 0x440, 25},
	{MX6_MODULE_UART5, "/soc/bus@2100000/serial@21f4000", 0x440, 26},
	{MX6_MODULE_UART6, "/soc/bus@2100000/serial@21fc000", 0x440, 26},
	{MX6_MODULE_UART7, "/soc/bus@2000000/spba-bus@2000000/serial@2018000", 0x440, 26},
	{MX6_MODULE_UART8, "/soc/bus@2000000/spba-bus@2000000/serial@2024000", 0x440, 26},
	{MX6_MODULE_PWM5, "/soc/bus@2000000/pwm@20f0000", 0x440, 27},
	{MX6_MODULE_PWM6, "/soc/bus@2000000/pwm@20f4000", 0x440, 27},
	{MX6_MODULE_PWM7, "/soc/bus@2000000/pwm@20f8000", 0x440, 27},
	{MX6_MODULE_PWM8, "/soc/bus@2000000/pwm@20fc000", 0x440, 27},
	{MX6_MODULE_ECSPI3, "/soc/bus@2000000/spba-bus@2000000/ecspi@2010000", 0x440, 28},
	{MX6_MODULE_ECSPI4, "/soc/bus@2000000/spba-bus@2000000/ecspi@2014000", 0x440, 28},
	{MX6_MODULE_I2C3, "/soc/bus@2100000/i2c@21a8000", 0x440, 29},
	{MX6_MODULE_I2C4, "/soc/bus@2100000/i2c@21f8000", 0x440, 29},
	{MX6_MODULE_GPT2, "/soc/bus@2000000/gpt@20e8000", 0x440, 30},
	{MX6_MODULE_EPIT2, "/soc/bus@2000000/epit@20d4000", 0x440, 31},

	{MX6_MODULE_TSC, "/soc/aips-bus@2000000/tsc@2040000", 0x430, 22},
	{MX6_MODULE_ADC2, "/soc/aips-bus@2100000/adc@219c000", 0x430, 23},
	{MX6_MODULE_SIM1, "/soc/aips-bus@2100000/sim@218c000", 0x430, 24},
	{MX6_MODULE_SIM2, "/soc/aips-bus@2100000/sim@21b4000", 0x430, 25},
	{MX6_MODULE_FLEXCAN1, "/soc/aips-bus@2000000/can@2090000", 0x430, 26},
	{MX6_MODULE_FLEXCAN2, "/soc/aips-bus@2000000/can@2094000", 0x430, 27},
	{MX6_MODULE_SPDIF, "/soc/aips-bus@2000000/spba-bus@2000000/spdif@2004000", 0x440, 2},
	{MX6_MODULE_EIM, "/soc/aips-bus@2100000/weim@21b8000", 0x440, 3},
	{MX6_MODULE_SD1, "/soc/aips-bus@2100000/usdhc@2190000", 0x440, 4},
	{MX6_MODULE_SD2, "/soc/aips-bus@2100000/usdhc@2194000", 0x440, 5},
	{MX6_MODULE_QSPI1, "/soc/aips-bus@2100000/qspi@21e0000", 0x440, 6},
	{MX6_MODULE_GPMI, "/soc/gpmi-nand@1806000", 0x440, 7},
	{MX6_MODULE_APBHDMA, "/soc/dma-apbh@1804000", 0x440, 7},
	{MX6_MODULE_LCDIF, "/soc/aips-bus@2100000/lcdif@21c8000", 0x440, 8},
	{MX6_MODULE_PXP, "/soc/aips-bus@2100000/pxp@21cc000", 0x440, 9},
	{MX6_MODULE_CSI, "/soc/aips-bus@2100000/csi@21c4000", 0x440, 10},
	{MX6_MODULE_ADC1, "/soc/aips-bus@2100000/adc@2198000", 0x440, 11},
	{MX6_MODULE_ENET1, "/soc/aips-bus@2100000/ethernet@2188000", 0x440, 12},
	{MX6_MODULE_ENET2, "/soc/aips-bus@2000000/ethernet@20b4000", 0x440, 13},
	{MX6_MODULE_CAAM, "/soc/aips-bus@2100000/caam@2140000", 0x440, 14},
	{MX6_MODULE_USB_OTG2, "/soc/aips-bus@2100000/usb@2184200", 0x440, 15},
	{MX6_MODULE_SAI2, "/soc/aips-bus@2000000/spba-bus@2000000/sai@202c000", 0x440, 24},
	{MX6_MODULE_SAI3, "/soc/aips-bus@2000000/spba-bus@2000000/sai@2030000", 0x440, 24},
	{MX6_MODULE_BEE, "/soc/aips-bus@2000000/bee@2044000", 0x440, 25},
	{MX6_MODULE_UART5, "/soc/aips-bus@2100000/serial@21f4000", 0x440, 26},
	{MX6_MODULE_UART6, "/soc/aips-bus@2100000/serial@21fc000", 0x440, 26},
	{MX6_MODULE_UART7, "/soc/aips-bus@2000000/spba-bus@2000000/serial@2018000", 0x440, 26},
	{MX6_MODULE_UART8, "/soc/aips-bus@2000000/spba-bus@2000000/serial@2024000", 0x440, 26},
	{MX6_MODULE_PWM5, "/soc/aips-bus@2000000/pwm@20f0000", 0x440, 27},
	{MX6_MODULE_PWM6, "/soc/aips-bus@2000000/pwm@20f4000", 0x440, 27},
	{MX6_MODULE_PWM7, "/soc/aips-bus@2000000/pwm@20f8000", 0x440, 27},
	{MX6_MODULE_PWM8, "/soc/aips-bus@2000000/pwm@20fc000", 0x440, 27},
	{MX6_MODULE_ECSPI3, "/soc/aips-bus@2000000/spba-bus@2000000/ecspi@2010000", 0x440, 28},
	{MX6_MODULE_ECSPI4, "/soc/aips-bus@2000000/spba-bus@2000000/ecspi@2014000", 0x440, 28},
	{MX6_MODULE_I2C3, "/soc/aips-bus@2100000/i2c@21a8000", 0x440, 29},
	{MX6_MODULE_I2C4, "/soc/aips-bus@2100000/i2c@21f8000", 0x440, 29},
	{MX6_MODULE_GPT2, "/soc/aips-bus@2000000/gpt@20e8000", 0x440, 30},
	{MX6_MODULE_EPIT2, "/soc/aips-bus@2000000/epit@20d4000", 0x440, 31},
	/* Paths for older imx tree: */
	{MX6_MODULE_TSC, "/soc/aips-bus@02000000/tsc@02040000", 0x430, 22},
	{MX6_MODULE_ADC2, "/soc/aips-bus@02100000/adc@0219c000", 0x430, 23},
	{MX6_MODULE_SIM1, "/soc/aips-bus@02100000/sim@0218c000", 0x430, 24},
	{MX6_MODULE_SIM2, "/soc/aips-bus@02100000/sim@021b4000", 0x430, 25},
	{MX6_MODULE_FLEXCAN1, "/soc/aips-bus@02000000/can@02090000", 0x430, 26},
	{MX6_MODULE_FLEXCAN2, "/soc/aips-bus@02000000/can@02094000", 0x430, 27},
	{MX6_MODULE_SPDIF, "/soc/aips-bus@02000000/spba-bus@02000000/spdif@02004000", 0x440, 2},
	{MX6_MODULE_EIM, "/soc/aips-bus@02100000/weim@021b8000", 0x440, 3},
	{MX6_MODULE_SD1, "/soc/aips-bus@02100000/usdhc@02190000", 0x440, 4},
	{MX6_MODULE_SD2, "/soc/aips-bus@02100000/usdhc@02194000", 0x440, 5},
	{MX6_MODULE_QSPI1, "/soc/aips-bus@02100000/qspi@021e0000", 0x440, 6},
	{MX6_MODULE_GPMI, "/soc/gpmi-nand@01806000", 0x440, 7},
	{MX6_MODULE_APBHDMA, "/soc/dma-apbh@01804000", 0x440, 7},
	{MX6_MODULE_LCDIF, "/soc/aips-bus@02100000/lcdif@021c8000", 0x440, 8},
	{MX6_MODULE_PXP, "/soc/aips-bus@02100000/pxp@021cc000", 0x440, 9},
	{MX6_MODULE_CSI, "/soc/aips-bus@02100000/csi@021c4000", 0x440, 10},
	{MX6_MODULE_ADC1, "/soc/aips-bus@02100000/adc@02198000", 0x440, 11},
	{MX6_MODULE_ENET1, "/soc/aips-bus@02100000/ethernet@02188000", 0x440, 12},
	{MX6_MODULE_ENET2, "/soc/aips-bus@02000000/ethernet@020b4000", 0x440, 13},
	{MX6_MODULE_CAAM, "/soc/aips-bus@02100000/caam@2140000", 0x440, 14},
	{MX6_MODULE_USB_OTG2, "/soc/aips-bus@02100000/usb@02184200", 0x440, 15},
	{MX6_MODULE_SAI2, "/soc/aips-bus@02000000/spba-bus@02000000/sai@0202c000", 0x440, 24},
	{MX6_MODULE_SAI3, "/soc/aips-bus@02000000/spba-bus@02000000/sai@02030000", 0x440, 24},
	{MX6_MODULE_BEE, "/soc/aips-bus@02000000/bee@02044000", 0x440, 25},
	{MX6_MODULE_UART5, "/soc/aips-bus@02100000/serial@021f4000", 0x440, 26},
	{MX6_MODULE_UART6, "/soc/aips-bus@02100000/serial@021fc000", 0x440, 26},
	{MX6_MODULE_UART7, "/soc/aips-bus@02000000/spba-bus@02000000/serial@02018000", 0x440, 26},
	{MX6_MODULE_UART8, "/soc/aips-bus@02000000/spba-bus@02000000/serial@02024000", 0x440, 26},
	{MX6_MODULE_PWM5, "/soc/aips-bus@02000000/pwm@020f0000", 0x440, 27},
	{MX6_MODULE_PWM6, "/soc/aips-bus@02000000/pwm@020f4000", 0x440, 27},
	{MX6_MODULE_PWM7, "/soc/aips-bus@02000000/pwm@020f8000", 0x440, 27},
	{MX6_MODULE_PWM8, "/soc/aips-bus@02000000/pwm@020fc000", 0x440, 27},
	{MX6_MODULE_ECSPI3, "/soc/aips-bus@02000000/spba-bus@02000000/ecspi@02010000", 0x440, 28},
	{MX6_MODULE_ECSPI4, "/soc/aips-bus@02000000/spba-bus@02000000/ecspi@02014000", 0x440, 28},
	{MX6_MODULE_I2C3, "/soc/aips-bus@02100000/i2c@021a8000", 0x440, 29},
	{MX6_MODULE_I2C4, "/soc/aips-bus@02100000/i2c@021f8000", 0x440, 29},
	{MX6_MODULE_GPT2, "/soc/aips-bus@02000000/gpt@020e8000", 0x440, 30},
	{MX6_MODULE_EPIT2, "/soc/aips-bus@02000000/epit@020d4000", 0x440, 31},
#endif
};

u32 check_module_fused(enum fuse_module_type module)
{
	u32 i, reg;
	for (i = 0; i < ARRAY_SIZE(mx6_fuse_descs); i++) {
		if (mx6_fuse_descs[i].module == module) {
			reg = readl(OCOTP_BASE_ADDR + mx6_fuse_descs[i].fuse_word_offset);
			if (reg & (1 << mx6_fuse_descs[i].fuse_bit_offset))
				return 1; /* disabled */
			else
				return 0; /* enabled */
		}
	}

	return  0; /* Not has a fuse, always enabled */
}

#ifdef DEBUG
void print_fuse_status()
{
	u32 i, reg;

	for (i = 0; i < ARRAY_SIZE(mx6_fuse_descs); i++) {
		reg = readl(OCOTP_BASE_ADDR + mx6_fuse_descs[i].fuse_word_offset);
		if (reg & (1 << mx6_fuse_descs[i].fuse_bit_offset))
			printf("%s, disabled\n", mx6_fuse_descs[i].node_path);
	}
}

void simulate_fuse()
{
	u32 i, reg;

    for (i = 0; i < ARRAY_SIZE(mx6_fuse_descs); i++) {
		if (MX6_MODULE_SD2 == mx6_fuse_descs[i].module)
			continue;

		reg = readl(OCOTP_BASE_ADDR + mx6_fuse_descs[i].fuse_word_offset);
		reg |= (1 << mx6_fuse_descs[i].fuse_bit_offset);
		writel(reg, OCOTP_BASE_ADDR + mx6_fuse_descs[i].fuse_word_offset);
    }
}
#endif

#ifdef CONFIG_OF_SYSTEM_SETUP
int ft_system_setup(void *blob, bd_t *bd)
{
	u32 i, reg;
	const char *status = "disabled";
	int rc;

	for (i = 0; i < ARRAY_SIZE(mx6_fuse_descs); i++) {
		reg = readl(OCOTP_BASE_ADDR + mx6_fuse_descs[i].fuse_word_offset);
		if (reg & (1 << mx6_fuse_descs[i].fuse_bit_offset)) {

			int nodeoff = fdt_path_offset(blob, mx6_fuse_descs[i].node_path);
			if (nodeoff < 0)
				continue; /* Not found, skip it */
add_status:
			rc = fdt_setprop(blob, nodeoff, "status", status, strlen(status) + 1);
			if (rc) {
				if (rc == -FDT_ERR_NOSPACE) {
					rc = fdt_increase_size(blob, 512);
					if (!rc)
						goto add_status;
				}
				printf("Unable to update property %s:%s, err=%s\n",
					mx6_fuse_descs[i].node_path, "status", fdt_strerror(rc));
			} else {
				printf("Modify %s:%s disabled\n",
					mx6_fuse_descs[i].node_path, "status");
			}
		}
	}

	printf("ft_system_setup for mx6\n");

	return 0;
}
#endif

u32 mx6_esdhc_fused(u32 base_addr)
{
	switch (base_addr) {
	case USDHC1_BASE_ADDR:
		return check_module_fused(MX6_MODULE_SD1);
	case USDHC2_BASE_ADDR:
		return check_module_fused(MX6_MODULE_SD2);
#ifdef USDHC3_BASE_ADDR
	case USDHC3_BASE_ADDR:
		return check_module_fused(MX6_MODULE_SD3);
#endif
#ifdef USDHC4_BASE_ADDR
	case USDHC4_BASE_ADDR:
		return check_module_fused(MX6_MODULE_SD4);
#endif
	default:
		return 0;
	}
}

u32 mx6_ecspi_fused(u32 base_addr)
{
	switch (base_addr) {
	case ECSPI1_BASE_ADDR:
		return check_module_fused(MX6_MODULE_ECSPI1);
	case ECSPI2_BASE_ADDR:
		return check_module_fused(MX6_MODULE_ECSPI2);
	case ECSPI3_BASE_ADDR:
		return check_module_fused(MX6_MODULE_ECSPI3);
	case ECSPI4_BASE_ADDR:
		return check_module_fused(MX6_MODULE_ECSPI4);
#ifdef ECSPI5_BASE_ADDR
	case ECSPI5_BASE_ADDR:
		return check_module_fused(MX6_MODULE_ECSPI5);
#endif
	default:
		return 0;
	}
}

u32 mx6_uart_fused(u32 base_addr)
{
	switch (base_addr) {
	case UART1_BASE:
		return check_module_fused(MX6_MODULE_UART1);
	case UART2_BASE:
		return check_module_fused(MX6_MODULE_UART2);
	case UART3_BASE:
		return check_module_fused(MX6_MODULE_UART3);
	case UART4_BASE:
		return check_module_fused(MX6_MODULE_UART4);
	case UART5_BASE:
		return check_module_fused(MX6_MODULE_UART5);
	case MX6UL_UART6_BASE_ADDR:
	case MX6SX_UART6_BASE_ADDR:
		return check_module_fused(MX6_MODULE_UART6);
#ifdef UART7_IPS_BASE_ADDR
	case UART7_IPS_BASE_ADDR:
		return check_module_fused(MX6_MODULE_UART7);
#endif
#ifdef UART8_IPS_BASE_ADDR
	case UART8_IPS_BASE_ADDR:
		return check_module_fused(MX6_MODULE_UART8);
#endif
	}

	return 0;
}

u32 mx6_usb_fused(u32 base_addr)
{
	int i = (base_addr - USB_BASE_ADDR) / 0x200;
	return check_module_fused(MX6_MODULE_USB_OTG1 + i);
}

u32 mx6_qspi_fused(u32 base_addr)
{
	switch (base_addr) {
#ifdef QSPI1_BASE_ADDR
	case QSPI1_BASE_ADDR:
		return check_module_fused(MX6_MODULE_QSPI1);
#endif

#ifdef QSPI2_BASE_ADDR
	case QSPI2_BASE_ADDR:
		return check_module_fused(MX6_MODULE_QSPI2);
#endif
	default:
		return 0;
	}
}

u32 mx6_i2c_fused(u32 base_addr)
{
	switch (base_addr) {
	case I2C1_BASE_ADDR:
		return check_module_fused(MX6_MODULE_I2C1);
	case I2C2_BASE_ADDR:
		return check_module_fused(MX6_MODULE_I2C2);
	case I2C3_BASE_ADDR:
		return check_module_fused(MX6_MODULE_I2C3);
#ifdef I2C4_BASE_ADDR
	case I2C4_BASE_ADDR:
		return check_module_fused(MX6_MODULE_I2C4);
#endif
	}

	return 0;
}

u32 mx6_enet_fused(u32 base_addr)
{
	switch (base_addr) {
	case ENET_BASE_ADDR:
		return check_module_fused(MX6_MODULE_ENET1);
#ifdef ENET2_BASE_ADDR
	case ENET2_BASE_ADDR:
		return check_module_fused(MX6_MODULE_ENET2);
#endif
	default:
		return 0;
	}
}
