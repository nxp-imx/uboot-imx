/*
 * Copyright 2017-2019 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */
#ifndef __ASM_ARCH_IMX8_I2C_H__
#define __ASM_ARCH_IMX8_I2C_H__

#include <asm/arch/sci/sci.h>
#include <asm/arch/lpcg.h>

struct imx_i2c_map {
	unsigned index;
	sc_rsrc_t rsrc;
	u32 lpcg[4];
};

static struct imx_i2c_map imx_i2c_desc[] = {
	{0, SC_R_I2C_0, {LPI2C_0_LPCG}},
	{1, SC_R_I2C_1, {LPI2C_1_LPCG}},
	{2, SC_R_I2C_2, {LPI2C_2_LPCG}},
	{3, SC_R_I2C_3, {LPI2C_3_LPCG}},
#ifdef CONFIG_IMX8QM
	{4, SC_R_I2C_4, {LPI2C_4_LPCG}},
	{5, SC_R_LVDS_0_I2C_0, {DI_LVDS_0_LPCG + 0x10}}, /* lvds0 i2c0 */
	{6, SC_R_LVDS_0_I2C_1, {DI_LVDS_0_LPCG + 0x14}}, /* lvds0 i2c1 */
	{7, SC_R_LVDS_1_I2C_0, {DI_LVDS_1_LPCG + 0x10}}, /* lvds1 i2c0 */
	{8, SC_R_LVDS_1_I2C_1, {DI_LVDS_1_LPCG + 0x14}}, /* lvds1 i2c1 */
#endif
	{9, SC_R_CSI_0_I2C_0, {MIPI_CSI_0_LPCG + 0x14}},
#ifdef CONFIG_IMX8QM
	{10, SC_R_CSI_1_I2C_0, {MIPI_CSI_1_LPCG + 0x14}},
	{11, SC_R_HDMI_I2C_0, {DI_HDMI_LPCG}},
	{12, SC_R_HDMI_RX_I2C_0, {RX_HDMI_LPCG + 0x10, RX_HDMI_LPCG + 0x14, RX_HDMI_LPCG + 0x18, RX_HDMI_LPCG + 0x1C}},
	{13, SC_R_MIPI_0_I2C_0, {MIPI_DSI_0_LPCG + 0x14, MIPI_DSI_0_LPCG + 0x18, MIPI_DSI_0_LPCG + 0x1c}},
	{14, SC_R_MIPI_0_I2C_1, {MIPI_DSI_0_LPCG + 0x24, MIPI_DSI_0_LPCG + 0x28, MIPI_DSI_0_LPCG + 0x2c}},
	{15, SC_R_MIPI_1_I2C_0, {MIPI_DSI_1_LPCG + 0x14, MIPI_DSI_1_LPCG + 0x18, MIPI_DSI_1_LPCG + 0x1c}},
	{16, SC_R_MIPI_1_I2C_1, {MIPI_DSI_1_LPCG + 0x24, MIPI_DSI_1_LPCG + 0x28, MIPI_DSI_1_LPCG + 0x2c}},
#else
	{13, SC_R_MIPI_0_I2C_0, {DI_MIPI0_LPCG + 0x10}},
	{14, SC_R_MIPI_0_I2C_1, {DI_MIPI0_LPCG + 0x14}},
	{15, SC_R_MIPI_1_I2C_0, {DI_MIPI1_LPCG + 0x10}},
	{16, SC_R_MIPI_1_I2C_1, {DI_MIPI1_LPCG + 0x14}},
#endif
};
#endif /* __ASM_ARCH_IMX8_I2C_H__ */
