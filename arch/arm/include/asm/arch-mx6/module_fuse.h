/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 */

/*
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MODULE_FUSE_H__
#define __MODULE_FUSE_H__

enum fuse_module_type{
	MX6_MODULE_TSC,
	MX6_MODULE_ADC1,
	MX6_MODULE_ADC2,
	MX6_MODULE_SIM1,
	MX6_MODULE_SIM2,
	MX6_MODULE_FLEXCAN1,
	MX6_MODULE_FLEXCAN2,
	MX6_MODULE_SPDIF,
	MX6_MODULE_EIM,
	MX6_MODULE_SD1,
	MX6_MODULE_SD2,
	MX6_MODULE_SD3,
	MX6_MODULE_SD4,
	MX6_MODULE_QSPI1,
	MX6_MODULE_QSPI2,
	MX6_MODULE_GPMI,
	MX6_MODULE_APBHDMA,
	MX6_MODULE_LCDIF,
	MX6_MODULE_PXP,
	MX6_MODULE_CSI,
	MX6_MODULE_ENET1,
	MX6_MODULE_ENET2,
	MX6_MODULE_CAAM,
	MX6_MODULE_USB_OTG1,
	MX6_MODULE_USB_OTG2,
	MX6_MODULE_SAI2,
	MX6_MODULE_SAI3,
	MX6_MODULE_BEE,
	MX6_MODULE_UART1,
	MX6_MODULE_UART2,
	MX6_MODULE_UART3,
	MX6_MODULE_UART4,
	MX6_MODULE_UART5,
	MX6_MODULE_UART6,
	MX6_MODULE_UART7,
	MX6_MODULE_UART8,
	MX6_MODULE_PWM5,
	MX6_MODULE_PWM6,
	MX6_MODULE_PWM7,
	MX6_MODULE_PWM8,
	MX6_MODULE_ECSPI1,
	MX6_MODULE_ECSPI2,
	MX6_MODULE_ECSPI3,
	MX6_MODULE_ECSPI4,
	MX6_MODULE_ECSPI5,
	MX6_MODULE_I2C1,
	MX6_MODULE_I2C2,
	MX6_MODULE_I2C3,
	MX6_MODULE_I2C4,
	MX6_MODULE_GPT1,
	MX6_MODULE_GPT2,
	MX6_MODULE_EPIT1,
	MX6_MODULE_EPIT2,
};

#if !defined(CONFIG_MODULE_FUSE)
static inline u32 check_module_fused(enum fuse_module_type module)
{
	return 0;
};

static inline u32 mx6_esdhc_fused(u32 base_addr)
{
	return 0;
};

static inline u32 mx6_ecspi_fused(u32 base_addr)
{
	return 0;
};
static inline u32 mx6_uart_fused(u32 base_addr)
{
	return 0;
};
static inline u32 mx6_usb_fused(u32 base_addr)
{
	return 0;
};
static inline u32 mx6_qspi_fused(u32 base_addr)
{
	return 0;
};
static inline u32 mx6_i2c_fused(u32 base_addr)
{
	return 0;
};
static inline u32 mx6_enet_fused(u32 base_addr)
{
	return 0;
};

#else
u32 check_module_fused(enum fuse_module_type module);
u32 mx6_esdhc_fused(u32 base_addr);
u32 mx6_ecspi_fused(u32 base_addr);
u32 mx6_uart_fused(u32 base_addr);
u32 mx6_usb_fused(u32 base_addr);
u32 mx6_qspi_fused(u32 base_addr);
u32 mx6_i2c_fused(u32 base_addr);
u32 mx6_enet_fused(u32 base_addr);
#endif

#ifdef DEBUG
void print_fuse_status();
void simulate_fuse();
#endif

#endif /* __MODULE_FUSE_H__ */
