/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */
#ifndef __ASM_ARCH_IMX8_I2C_H__
#define __ASM_ARCH_IMX8_I2C_H__

#include <asm/imx-common/sci/sci.h>

struct imx_i2c_map {
	int index;
	sc_rsrc_t rsrc;
};

static struct imx_i2c_map imx_i2c_desc[] = {
	{0, SC_R_I2C_0},
	{1, SC_R_I2C_1},
	{2, SC_R_I2C_2},
	{3, SC_R_I2C_3},
	{4, SC_R_I2C_4},
	{5, SC_R_LVDS_0_I2C_0}, /* lvds0 i2c0 */
	{6, SC_R_LVDS_0_I2C_0}, /* lvds0 i2c1 */
	{7, SC_R_LVDS_1_I2C_0}, /* lvds1 i2c0 */
	{8, SC_R_LVDS_1_I2C_0}, /* lvds1 i2c1 */
	{9, SC_R_CSI_0_I2C_0},
	{10, SC_R_CSI_1_I2C_0},
	{11, SC_R_HDMI_I2C_0},
	{12, SC_R_HDMI_RX_I2C_0},
	{13, SC_R_MIPI_0_I2C_0},
	{14, SC_R_MIPI_0_I2C_1},
	{15, SC_R_MIPI_1_I2C_0},
	{16, SC_R_MIPI_1_I2C_1},
};
#endif /* __ASM_ARCH_IMX8_I2C_H__ */
