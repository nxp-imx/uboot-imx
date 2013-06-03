/*
 * Copyright (C) 2012-2013 Freescale Semiconductor, Inc.
 *
 * Author: Fabio Estevam <fabio.estevam@freescale.com>
 * Author: Jason Liu <r64343@freescale.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/iomux.h>
#include <asm/arch/mx6-pins.h>
#include <asm/arch/crm_regs.h>
#include <asm/errno.h>
#include <asm/gpio.h>
#include <asm/imx-common/iomux-v3.h>
#include <asm/imx-common/boot_mode.h>
#ifdef CONFIG_I2C_MXC
#include <i2c.h>
#include <asm/imx-common/mxc_i2c.h>
#endif
#include <mmc.h>
#include <fsl_esdhc.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/arch/sys_proto.h>

DECLARE_GLOBAL_DATA_PTR;

#define I2C_EXP_RST IMX_GPIO_NR(1, 15)
#define I2C3_STEER  IMX_GPIO_NR(5, 4)
#define UART_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |            \
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |               \
	PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |            \
	PAD_CTL_PUS_47K_UP  | PAD_CTL_SPEED_LOW |               \
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define ENET_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED   |		\
	PAD_CTL_DSE_40ohm   | PAD_CTL_HYS)

#define SPI_PAD_CTRL (PAD_CTL_HYS |				\
	PAD_CTL_PUS_100K_DOWN | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm     | PAD_CTL_SRE_FAST)

#define WEIM_NOR_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST)


#define GPMI_PAD_CTRL0 (PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP)
#define GPMI_PAD_CTRL1 (PAD_CTL_DSE_40ohm | PAD_CTL_SPEED_MED | \
			PAD_CTL_SRE_FAST)
#define GPMI_PAD_CTRL2 (GPMI_PAD_CTRL0 | GPMI_PAD_CTRL1)

#define I2C_PAD_CTRL	(PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm | PAD_CTL_HYS |			\
	PAD_CTL_ODE | PAD_CTL_SRE_FAST)

#ifdef CONFIG_I2C_MXC
#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)
/* I2C2 pfuze */
struct i2c_pads_info i2c_pad_info1 = {
	/* don't support Sabreauto REVA */
	.scl = {
		.i2c_mode = MX6_PAD_EIM_EB2__I2C2_SCL | PC,
		.gpio_mode = MX6_PAD_EIM_EB2__GPIO_2_30 | PC,
		.gp = IMX_GPIO_NR(2, 30)
	},
	.sda = {
		.i2c_mode = MX6_PAD_KEY_ROW3__I2C2_SDA | PC,
		.gpio_mode = MX6_PAD_KEY_ROW3__GPIO_4_13 | PC,
		.gp = IMX_GPIO_NR(4, 13)
	}
};
#endif

int dram_init(void)
{
	gd->ram_size = ((ulong)CONFIG_DDR_MB * 1024 * 1024);

	return 0;
}

iomux_v3_cfg_t const uart4_pads[] = {
	MX6_PAD_KEY_COL0__UART4_TXD | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX6_PAD_KEY_ROW0__UART4_RXD | MUX_PAD_CTRL(UART_PAD_CTRL),
};

iomux_v3_cfg_t const enet_pads[] = {
	MX6_PAD_KEY_COL1__ENET_MDIO		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_KEY_COL2__ENET_MDC		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TXC__ENET_RGMII_TXC	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TD0__ENET_RGMII_TD0	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TD1__ENET_RGMII_TD1	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TD2__ENET_RGMII_TD2	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TD3__ENET_RGMII_TD3	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TX_CTL__RGMII_TX_CTL	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET_REF_CLK__ENET_TX_CLK	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RXC__ENET_RGMII_RXC	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RD0__ENET_RGMII_RD0	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RD1__ENET_RGMII_RD1	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RD2__ENET_RGMII_RD2	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RD3__ENET_RGMII_RD3	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RX_CTL__RGMII_RX_CTL	| MUX_PAD_CTRL(ENET_PAD_CTRL),
};

static void setup_iomux_enet(void)
{
	imx_iomux_v3_setup_multiple_pads(enet_pads, ARRAY_SIZE(enet_pads));
}

iomux_v3_cfg_t const usdhc3_pads[] = {
	MX6_PAD_SD3_CLK__USDHC3_CLK	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_CMD__USDHC3_CMD	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT0__USDHC3_DAT0	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT1__USDHC3_DAT1	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT2__USDHC3_DAT2	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT3__USDHC3_DAT3	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT4__USDHC3_DAT4	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT5__USDHC3_DAT5	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT6__USDHC3_DAT6	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT7__USDHC3_DAT7	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_GPIO_18__USDHC3_VSELECT | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NANDF_CS2__GPIO_6_15   | MUX_PAD_CTRL(NO_PAD_CTRL),
};

#ifdef CONFIG_I2C_MXC
static int setup_pmic_voltages(void)
{
	unsigned char value, rev_id = 0 ;
	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
	if (!i2c_probe(0x8)) {
		if (i2c_read(0x8, 0, 1, &value, 1)) {
			printf("Read device ID error!\n");
			return -1;
		}
		if (i2c_read(0x8, 3, 1, &rev_id, 1)) {
			printf("Read Rev ID error!\n");
			return -1;
		}
		printf("Found PFUZE100! deviceid=%x,revid=%x\n", value, rev_id);
		/* set SW1AB staby volatage 0.975V*/
		if (i2c_read(0x8, 0x21, 1, &value, 1)) {
			printf("Read SW1ABSTBY error!\n");
			return -1;
		}
		value &= ~0x3f;
		value |= 0x1b;
		if (i2c_write(0x8, 0x21, 1, &value, 1)) {
			printf("Set SW1ABSTBY error!\n");
			return -1;
		}
		/* set SW1AB/VDDARM step ramp up time from 16us to 4us/25mV */
		if (i2c_read(0x8, 0x24, 1, &value, 1)) {
			printf("Read SW1ABSTBY error!\n");
			return -1;
		}
		value &= ~0xc0;
		value |= 0x40;
		if (i2c_write(0x8, 0x24, 1, &value, 1)) {
			printf("Set SW1ABSTBY error!\n");
			return -1;
		}

		/* set SW1C staby volatage 0.975V*/
		if (i2c_read(0x8, 0x2f, 1, &value, 1)) {
			printf("Read SW1CSTBY error!\n");
			return -1;
		}
		value &= ~0x3f;
		value |= 0x1b;
		if (i2c_write(0x8, 0x2f, 1, &value, 1)) {
			printf("Set SW1CSTBY error!\n");
			return -1;
		}

		/* set SW1C/VDDSOC step ramp up time to from 16us to 4us/25mV */
		if (i2c_read(0x8, 0x32, 1, &value, 1)) {
			printf("Read SW1ABSTBY error!\n");
			return -1;
		}
		value &= ~0xc0;
		value |= 0x40;
		if (i2c_write(0x8, 0x32, 1, &value, 1)) {
			printf("Set SW1ABSTBY error!\n");
			return -1;
		}
	}

	return 0;
}
#endif

static void setup_iomux_uart(void)
{
	imx_iomux_v3_setup_multiple_pads(uart4_pads, ARRAY_SIZE(uart4_pads));
}

#ifdef CONFIG_FSL_ESDHC
struct fsl_esdhc_cfg usdhc_cfg[1] = {
	{USDHC3_BASE_ADDR},
};

int board_mmc_getcd(struct mmc *mmc)
{
	gpio_direction_input(IMX_GPIO_NR(6, 15));
	return !gpio_get_value(IMX_GPIO_NR(6, 15));
}

int board_mmc_init(bd_t *bis)
{
	imx_iomux_v3_setup_multiple_pads(usdhc3_pads, ARRAY_SIZE(usdhc3_pads));

	usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
	return fsl_esdhc_initialize(bis, &usdhc_cfg[0]);
}
#endif

#ifdef CONFIG_SYS_USE_SPINOR
iomux_v3_cfg_t const ecspi1_pads[] = {
	MX6_PAD_EIM_D16__ECSPI1_SCLK | MUX_PAD_CTRL(SPI_PAD_CTRL),
	MX6_PAD_EIM_D17__ECSPI1_MISO | MUX_PAD_CTRL(SPI_PAD_CTRL),
	MX6_PAD_EIM_D18__ECSPI1_MOSI | MUX_PAD_CTRL(SPI_PAD_CTRL),
	MX6_PAD_EIM_D19__GPIO_3_19   | MUX_PAD_CTRL(NO_PAD_CTRL),
	/* Steer logic */
	MX6_PAD_EIM_A24__GPIO_5_4    | MUX_PAD_CTRL(NO_PAD_CTRL),
};

void setup_spinor(void)
{
	imx_iomux_v3_setup_multiple_pads(ecspi1_pads,
					 ARRAY_SIZE(ecspi1_pads));
	gpio_direction_output(IMX_GPIO_NR(5, 4), 0);
	gpio_direction_output(IMX_GPIO_NR(3, 19), 0);
}
#endif

#ifdef CONFIG_SYS_USE_EIMNOR
iomux_v3_cfg_t eimnor_pads[] = {
	MX6_PAD_EIM_D16__WEIM_WEIM_D_16      | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_D17__WEIM_WEIM_D_17      | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_D18__WEIM_WEIM_D_18      | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_D19__WEIM_WEIM_D_19      | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_D20__WEIM_WEIM_D_20      | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_D21__WEIM_WEIM_D_21      | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_D22__WEIM_WEIM_D_22      | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_D23__WEIM_WEIM_D_23      | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_D24__WEIM_WEIM_D_24      | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_D25__WEIM_WEIM_D_25      | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_D26__WEIM_WEIM_D_26      | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_D27__WEIM_WEIM_D_27      | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_D28__WEIM_WEIM_D_28      | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_D29__WEIM_WEIM_D_29      | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_D30__WEIM_WEIM_D_30      | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_D31__WEIM_WEIM_D_31      | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_DA0__WEIM_WEIM_DA_A_0    | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_DA1__WEIM_WEIM_DA_A_1    | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_DA2__WEIM_WEIM_DA_A_2    | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_DA3__WEIM_WEIM_DA_A_3    | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_DA4__WEIM_WEIM_DA_A_4    | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_DA5__WEIM_WEIM_DA_A_5    | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_DA6__WEIM_WEIM_DA_A_6    | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_DA7__WEIM_WEIM_DA_A_7    | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_DA8__WEIM_WEIM_DA_A_8    | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_DA9__WEIM_WEIM_DA_A_9    | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_DA10__WEIM_WEIM_DA_A_10  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_DA11__WEIM_WEIM_DA_A_11  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL) ,
	MX6_PAD_EIM_DA12__WEIM_WEIM_DA_A_12  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_DA13__WEIM_WEIM_DA_A_13  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_DA14__WEIM_WEIM_DA_A_14  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_DA15__WEIM_WEIM_DA_A_15  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_A16__WEIM_WEIM_A_16      | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_A17__WEIM_WEIM_A_17      | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_A18__WEIM_WEIM_A_18      | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_A19__WEIM_WEIM_A_19      | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_A20__WEIM_WEIM_A_20      | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_A21__WEIM_WEIM_A_21      | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_A22__WEIM_WEIM_A_22      | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_A23__WEIM_WEIM_A_23      | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX6_PAD_EIM_OE__WEIM_WEIM_OE         | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX6_PAD_EIM_RW__WEIM_WEIM_RW         | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX6_PAD_EIM_CS0__WEIM_WEIM_CS_0      | MUX_PAD_CTRL(NO_PAD_CTRL),
	/* Steer logic */
	MX6_PAD_EIM_A24__GPIO_5_4            | MUX_PAD_CTRL(NO_PAD_CTRL),
};
static void eimnor_cs_setup(void)
{
	writel(0x00000120, WEIM_BASE_ADDR + 0x090);
	writel(0x00020181, WEIM_BASE_ADDR + 0x000);
	writel(0x00000001, WEIM_BASE_ADDR + 0x004);
	writel(0x0a020000, WEIM_BASE_ADDR + 0x008);
	writel(0x0000c000, WEIM_BASE_ADDR + 0x00c);
	writel(0x0804a240, WEIM_BASE_ADDR + 0x010);
}

static void setup_eimnor(void)
{
	imx_iomux_v3_setup_multiple_pads(eimnor_pads,
			ARRAY_SIZE(eimnor_pads));

	gpio_direction_output(IMX_GPIO_NR(5, 4), 0);

	eimnor_cs_setup();
}
#endif

#ifdef CONFIG_SYS_USE_NAND
iomux_v3_cfg_t gpmi_pads[] = {
	MX6_PAD_NANDF_CLE__RAWNAND_CLE		| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NANDF_ALE__RAWNAND_ALE		| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NANDF_WP_B__RAWNAND_RESETN	| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NANDF_RB0__RAWNAND_READY0	| MUX_PAD_CTRL(GPMI_PAD_CTRL0),
	MX6_PAD_NANDF_CS0__RAWNAND_CE0N		| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_SD4_CMD__RAWNAND_RDN		| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_SD4_CLK__RAWNAND_WRN		| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NANDF_D0__RAWNAND_D0		| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NANDF_D1__RAWNAND_D1		| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NANDF_D2__RAWNAND_D2		| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NANDF_D3__RAWNAND_D3		| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NANDF_D4__RAWNAND_D4		| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NANDF_D5__RAWNAND_D5		| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NANDF_D6__RAWNAND_D6		| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NANDF_D7__RAWNAND_D7		| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_SD4_DAT0__RAWNAND_DQS		| MUX_PAD_CTRL(GPMI_PAD_CTRL1),
};

static void setup_gpmi_nand(void)
{
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;

	/* config gpmi nand iomux */
	imx_iomux_v3_setup_multiple_pads(gpmi_pads, ARRAY_SIZE(gpmi_pads));

	/* gate ENFC_CLK_ROOT clock first,before clk source switch */
	clrbits_le32(&mxc_ccm->CCGR2, MXC_CCM_CCGR2_IOMUX_IPT_CLK_IO_MASK);

	/* config gpmi and bch clock to 100 MHz */
	clrsetbits_le32(&mxc_ccm->cs2cdr,
			MXC_CCM_CS2CDR_ENFC_CLK_PODF_MASK |
			MXC_CCM_CS2CDR_ENFC_CLK_PRED_MASK |
			MXC_CCM_CS2CDR_ENFC_CLK_SEL_MASK,
			MXC_CCM_CS2CDR_ENFC_CLK_PODF(0) |
			MXC_CCM_CS2CDR_ENFC_CLK_PRED(3) |
			MXC_CCM_CS2CDR_ENFC_CLK_SEL(3));

	/* enable ENFC_CLK_ROOT clock */
	setbits_le32(&mxc_ccm->CCGR2, MXC_CCM_CCGR2_IOMUX_IPT_CLK_IO_MASK);

	/* enable gpmi and bch clock gating */
	setbits_le32(&mxc_ccm->CCGR4,
		     MXC_CCM_CCGR4_RAWNAND_U_BCH_INPUT_APB_MASK |
		     MXC_CCM_CCGR4_RAWNAND_U_GPMI_BCH_INPUT_BCH_MASK |
		     MXC_CCM_CCGR4_RAWNAND_U_GPMI_BCH_INPUT_GPMI_IO_MASK |
		     MXC_CCM_CCGR4_RAWNAND_U_GPMI_INPUT_APB_MASK |
		     MXC_CCM_CCGR4_PL301_MX6QPER1_BCH_OFFSET);

	/* enable apbh clock gating */
	setbits_le32(&mxc_ccm->CCGR0, MXC_CCM_CCGR0_APBHDMA_MASK);
}
#endif

int mx6_rgmii_rework(struct phy_device *phydev)
{
	unsigned short val;

	/* To enable AR8031 ouput a 125MHz clk from CLK_25M */
	phy_write(phydev, MDIO_DEVAD_NONE, 0xd, 0x7);
	phy_write(phydev, MDIO_DEVAD_NONE, 0xe, 0x8016);
	phy_write(phydev, MDIO_DEVAD_NONE, 0xd, 0x4007);

	val = phy_read(phydev, MDIO_DEVAD_NONE, 0xe);
	val &= 0xffe3;
	val |= 0x18;
	phy_write(phydev, MDIO_DEVAD_NONE, 0xe, val);

	/* introduce tx clock delay */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x5);
	val = phy_read(phydev, MDIO_DEVAD_NONE, 0x1e);
	val |= 0x0100;
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, val);

	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
	mx6_rgmii_rework(phydev);

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}

int board_eth_init(bd_t *bis)
{
	int ret;

	setup_iomux_enet();

	ret = cpu_eth_init(bis);
	if (ret)
		printf("FEC MXC: %s:failed\n", __func__);

	return 0;
}

#define BOARD_REV_B  0x200
#define BOARD_REV_A  0x100

static int mx6sabre_rev(void)
{
	/*
	 * Get Board ID information from OCOTP_GP1[15:8]
	 * i.MX6Q ARD RevA: 0x01
	 * i.MX6Q ARD RevB: 0x02
	 */
	struct ocotp_regs *ocotp = (struct ocotp_regs *)OCOTP_BASE_ADDR;
	int reg = readl(&ocotp->gp1);
	int ret;

	switch (reg >> 8 & 0x0F) {
	case 0x02:
		ret = BOARD_REV_B;
		break;
	case 0x01:
	default:
		ret = BOARD_REV_A;
		break;
	}

	return ret;
}

u32 get_board_rev(void)
{
	int rev = mx6sabre_rev();

	return (get_cpu_rev() & ~(0xF << 8)) | rev;
}

int board_early_init_f(void)
{
	setup_iomux_uart();

#ifdef CONFIG_SYS_USE_SPINOR
	setup_spinor();
#endif

#ifdef CONFIG_SYS_USE_EIMNOR
	setup_eimnor();
#endif

#ifdef CONFIG_SYS_USE_NAND
	setup_gpmi_nand();
#endif
	return 0;
}

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

	return 0;
}

#ifdef CONFIG_CMD_BMODE
static const struct boot_mode board_boot_modes[] = {
	/* 4 bit bus width */
	{"mmc0", MAKE_CFGVAL(0x40, 0x30, 0x00, 0x00)},
	{NULL,   0},
};
#endif

int board_late_init(void)
{
	int ret;
#ifdef CONFIG_CMD_BMODE
	add_board_boot_modes(board_boot_modes);
#endif

#ifdef CONFIG_I2C_MXC
	setup_i2c(1, CONFIG_SYS_I2C_SPEED,
			CONFIG_SYS_I2C_SLAVE, &i2c_pad_info1);
	ret = setup_pmic_voltages();
	if (ret)
		return -1;
#endif

	return 0;
}

int checkboard(void)
{
	int rev = mx6sabre_rev();
	char *revname;

	switch (rev) {
	case BOARD_REV_B:
		revname = "B";
		break;
	case BOARD_REV_A:
	default:
		revname = "A";
		break;
	}

	printf("Board: MX6Q/SDL-Sabreauto rev%s\n", revname);

	return 0;
}
