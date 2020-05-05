/*
 * Copyright (C) 2014-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/mx7-pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/gpio.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/io.h>
#include <linux/sizes.h>
#include <common.h>
#include <fsl_esdhc_imx.h>
#include <mmc.h>
#include <miiphy.h>
#include <netdev.h>
#include <power/pmic.h>
#include <power/pfuze3000_pmic.h>
#include "../common/pfuze.h"
#ifdef CONFIG_SYS_I2C_MXC
#include <i2c.h>
#include <asm/mach-imx/mxc_i2c.h>
#endif
#include <asm/arch/crm_regs.h>

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL  (PAD_CTL_DSE_3P3V_49OHM | \
	PAD_CTL_PUS_PU100KOHM | PAD_CTL_HYS)

#define USDHC_PAD_CTRL (PAD_CTL_DSE_3P3V_32OHM | PAD_CTL_SRE_SLOW | \
	PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PUS_PU47KOHM)

#define ENET_PAD_CTRL  (PAD_CTL_PUS_PU100KOHM | PAD_CTL_DSE_3P3V_98OHM)
#define ENET_PAD_CTRL_MII  (PAD_CTL_PUS_PU100KOHM | PAD_CTL_DSE_3P3V_98OHM)

#define ENET_RX_PAD_CTRL  (PAD_CTL_PUS_PU100KOHM | PAD_CTL_DSE_3P3V_98OHM)

#define I2C_PAD_CTRL    (PAD_CTL_DSE_3P3V_32OHM | PAD_CTL_SRE_SLOW | \
	PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PUS_PU100KOHM)

#define LCD_PAD_CTRL    (PAD_CTL_HYS | PAD_CTL_PUS_PU100KOHM | \
	PAD_CTL_DSE_3P3V_49OHM)

#define QSPI_PAD_CTRL	\
		(PAD_CTL_DSE_3P3V_49OHM | PAD_CTL_PUE | PAD_CTL_PUS_PU47KOHM)

#define SPI_PAD_CTRL (PAD_CTL_DSE_3P3V_49OHM | PAD_CTL_SRE_SLOW | PAD_CTL_HYS)

#define NAND_PAD_CTRL (PAD_CTL_DSE_3P3V_49OHM | PAD_CTL_SRE_SLOW | PAD_CTL_HYS)

#define NAND_PAD_READY0_CTRL (PAD_CTL_DSE_3P3V_49OHM | PAD_CTL_PUS_PU5KOHM)

#define WEIM_NOR_PAD_CTRL (PAD_CTL_DSE_3P3V_49OHM | PAD_CTL_SRE_SLOW | \
	PAD_CTL_PUS_PU100KOHM)


#ifdef CONFIG_SYS_I2C
#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)
/* I2C1 for PMIC */
struct i2c_pads_info i2c_pad_info1 = {
	.scl = {
		.i2c_mode = MX7D_PAD_I2C1_SCL__I2C1_SCL | PC,
		.gpio_mode = MX7D_PAD_I2C1_SCL__GPIO4_IO8 | PC,
		.gp = IMX_GPIO_NR(4, 8),
	},
	.sda = {
		.i2c_mode = MX7D_PAD_I2C1_SDA__I2C1_SDA | PC,
		.gpio_mode = MX7D_PAD_I2C1_SDA__GPIO4_IO9 | PC,
		.gp = IMX_GPIO_NR(4, 9),
	},
};

/* I2C2 */
struct i2c_pads_info i2c_pad_info2 = {
	.scl = {
		.i2c_mode = MX7D_PAD_I2C2_SCL__I2C2_SCL | PC,
		.gpio_mode = MX7D_PAD_I2C2_SCL__GPIO4_IO10 | PC,
		.gp = IMX_GPIO_NR(4, 10),
	},
	.sda = {
		.i2c_mode = MX7D_PAD_I2C2_SDA__I2C2_SDA | PC,
		.gpio_mode = MX7D_PAD_I2C2_SDA__GPIO4_IO11 | PC,
		.gp = IMX_GPIO_NR(4, 11),
	},
};
#endif

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_SIZE;

	return 0;
}

static iomux_v3_cfg_t const uart1_pads[] = {
	MX7D_PAD_UART1_TX_DATA__UART1_DCE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX7D_PAD_UART1_RX_DATA__UART1_DCE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const usdhc1_pads[] = {
	MX7D_PAD_SD1_CLK__SD1_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_CMD__SD1_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_DATA0__SD1_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_DATA1__SD1_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_DATA2__SD1_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_DATA3__SD1_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),

	MX7D_PAD_GPIO1_IO08__SD1_VSELECT | MUX_PAD_CTRL(USDHC_PAD_CTRL),

	MX7D_PAD_SD1_CD_B__GPIO5_IO0    | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_RESET_B__GPIO5_IO2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

#ifdef CONFIG_MTD_NOR_FLASH
static iomux_v3_cfg_t const eimnor_pads[] = {
	MX7D_PAD_LCD_DATA00__EIM_DATA0 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_LCD_DATA01__EIM_DATA1 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_LCD_DATA02__EIM_DATA2 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_LCD_DATA03__EIM_DATA3 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_LCD_DATA04__EIM_DATA4 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_LCD_DATA05__EIM_DATA5 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_LCD_DATA06__EIM_DATA6 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_LCD_DATA07__EIM_DATA7 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_LCD_DATA08__EIM_DATA8 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_LCD_DATA09__EIM_DATA9 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_LCD_DATA10__EIM_DATA10 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_LCD_DATA11__EIM_DATA11 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_LCD_DATA12__EIM_DATA12 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_LCD_DATA13__EIM_DATA13 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_LCD_DATA14__EIM_DATA14 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_LCD_DATA15__EIM_DATA15 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),

	MX7D_PAD_EPDC_DATA00__EIM_AD0   | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_DATA01__EIM_AD1   | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_DATA02__EIM_AD2   | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_DATA03__EIM_AD3   | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_DATA04__EIM_AD4   | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_DATA05__EIM_AD5   | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_DATA06__EIM_AD6   | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_DATA07__EIM_AD7   | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_BDR1__EIM_AD8     | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_PWR_COM__EIM_AD9  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_SDCLK__EIM_AD10   | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_SDLE__EIM_AD11    | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_SDOE__EIM_AD12    | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_SDSHR__EIM_AD13   | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_SDCE0__EIM_AD14   | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_SDCE1__EIM_AD15   | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_SDCE2__EIM_ADDR16 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_SDCE3__EIM_ADDR17 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_GDCLK__EIM_ADDR18 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_GDOE__EIM_ADDR19  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_GDRL__EIM_ADDR20  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_GDSP__EIM_ADDR21  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_BDR0__EIM_ADDR22  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_LCD_DATA20__EIM_ADDR23 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_LCD_DATA21__EIM_ADDR24 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_LCD_DATA22__EIM_ADDR25 | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),

	MX7D_PAD_EPDC_DATA08__EIM_OE    | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_DATA09__EIM_RW    | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_DATA10__EIM_CS0_B | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_DATA12__EIM_LBA_B | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
	MX7D_PAD_EPDC_DATA13__EIM_WAIT  | MUX_PAD_CTRL(WEIM_NOR_PAD_CTRL),
};

static void eimnor_cs_setup(void)
{
	writel(0x00000120, WEIM_IPS_BASE_ADDR + 0x090);
	writel(0x00210081, WEIM_IPS_BASE_ADDR + 0x000);
	writel(0x00000001, WEIM_IPS_BASE_ADDR + 0x004);
	writel(0x0e020000, WEIM_IPS_BASE_ADDR + 0x008);
	writel(0x00000000, WEIM_IPS_BASE_ADDR + 0x00c);
	writel(0x0704a040, WEIM_IPS_BASE_ADDR + 0x010);
}

static void setup_eimnor(void)
{
	imx_iomux_v3_setup_multiple_pads(eimnor_pads,
			ARRAY_SIZE(eimnor_pads));

	eimnor_cs_setup();
}
#endif

static iomux_v3_cfg_t const per_rst_pads[] = {
	MX7D_PAD_GPIO1_IO03__GPIO1_IO3 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static iomux_v3_cfg_t const wdog_pads[] = {
	MX7D_PAD_ENET1_COL__WDOG1_WDOG_ANY | MUX_PAD_CTRL(NO_PAD_CTRL),
};

#ifdef CONFIG_FEC_MXC
static iomux_v3_cfg_t const fec2_pads[] = {
	MX7D_PAD_GPIO1_IO11__ENET1_MDC  | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_GPIO1_IO10__ENET1_MDIO | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_EPDC_SDCE0__ENET2_RGMII_RX_CTL | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_EPDC_SDCLK__ENET2_RGMII_RD0 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_EPDC_SDLE__ENET2_RGMII_RD1  | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_EPDC_SDOE__ENET2_RGMII_RD2  | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_EPDC_SDSHR__ENET2_RGMII_RD3 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_EPDC_SDCE1__ENET2_RGMII_RXC | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_EPDC_GDRL__ENET2_RGMII_TX_CTL | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_EPDC_SDCE2__ENET2_RGMII_TD0 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_EPDC_SDCE3__ENET2_RGMII_TD1 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_EPDC_GDCLK__ENET2_RGMII_TD2 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_EPDC_GDOE__ENET2_RGMII_TD3  | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_EPDC_GDSP__ENET2_RGMII_TXC  | MUX_PAD_CTRL(ENET_PAD_CTRL),
};

static void setup_iomux_fec2(void)
{
	imx_iomux_v3_setup_multiple_pads(fec2_pads, ARRAY_SIZE(fec2_pads));
}
#endif

static void setup_iomux_uart(void)
{
	imx_iomux_v3_setup_multiple_pads(uart1_pads, ARRAY_SIZE(uart1_pads));
}

#ifdef CONFIG_FSL_QSPI
#ifndef CONFIG_DM_SPI
static iomux_v3_cfg_t const quadspi_pads[] = {
	MX7D_PAD_EPDC_DATA00__QSPI_A_DATA0 | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA01__QSPI_A_DATA1 | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA02__QSPI_A_DATA2 | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA03__QSPI_A_DATA3 | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA04__QSPI_A_DQS   | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA05__QSPI_A_SCLK  | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA06__QSPI_A_SS0_B | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA07__QSPI_A_SS1_B | MUX_PAD_CTRL(QSPI_PAD_CTRL),

	MX7D_PAD_EPDC_DATA08__QSPI_B_DATA0 | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA09__QSPI_B_DATA1 | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA10__QSPI_B_DATA2 | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA11__QSPI_B_DATA3 | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA12__QSPI_B_DQS   | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA13__QSPI_B_SCLK  | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA14__QSPI_B_SS0_B | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	MX7D_PAD_EPDC_DATA15__QSPI_B_SS1_B | MUX_PAD_CTRL(QSPI_PAD_CTRL),

};
#endif

int board_qspi_init(void)
{
#ifndef CONFIG_DM_SPI
	/* Set the iomux */
	imx_iomux_v3_setup_multiple_pads(quadspi_pads, ARRAY_SIZE(quadspi_pads));
#endif

	/* Set the clock */
	set_clk_qspi();

	return 0;
}
#endif

#ifdef CONFIG_NAND_MXS
static iomux_v3_cfg_t const gpmi_pads[] = {
	MX7D_PAD_SD3_DATA0__NAND_DATA00 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA1__NAND_DATA01 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA2__NAND_DATA02 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA3__NAND_DATA03 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA4__NAND_DATA04 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA5__NAND_DATA05 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA6__NAND_DATA06 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA7__NAND_DATA07 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_CLK__NAND_CLE	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_CMD__NAND_ALE	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_STROBE__NAND_RE_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_RESET_B__NAND_WE_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_MCLK__NAND_WP_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_RX_BCLK__NAND_CE3_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_RX_SYNC__NAND_CE2_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_RX_DATA__NAND_CE1_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_TX_BCLK__NAND_CE0_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_TX_SYNC__NAND_DQS	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_TX_DATA__NAND_READY_B	| MUX_PAD_CTRL(NAND_PAD_READY0_CTRL),
};

static void setup_gpmi_nand(void)
{
	imx_iomux_v3_setup_multiple_pads(gpmi_pads, ARRAY_SIZE(gpmi_pads));

	/*
	 * NAND_USDHC_BUS_CLK is set in rom
	 */

	set_clk_nand();

	/*
	 * APBH clock root is set in init_esdhc, USDHC3_CLK.
	 * There is no clk gate for APBHDMA.
	 * No touch here.
	 */
}
#endif


#ifdef CONFIG_FSL_ESDHC_IMX

#define USDHC1_CD_GPIO	IMX_GPIO_NR(5, 0)
#define USDHC1_PWR_GPIO	IMX_GPIO_NR(5, 2)


static struct fsl_esdhc_cfg usdhc_cfg[1] = {
	{USDHC1_BASE_ADDR, 0, 4},
};

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;

	switch (cfg->esdhc_base) {
	case USDHC1_BASE_ADDR:
		ret = !gpio_get_value(USDHC1_CD_GPIO);
		break;
	}

	return ret;
}
int board_mmc_init(bd_t *bis)
{
	int i, ret;
	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-boot device node)    (Physical Port)
	 * mmc0                    USDHC1
	 */
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			imx_iomux_v3_setup_multiple_pads(
				usdhc1_pads, ARRAY_SIZE(usdhc1_pads));
			gpio_request(USDHC1_CD_GPIO, "usdhc1_cd");
			gpio_request(USDHC1_PWR_GPIO, "usdhc1_pwr");
			gpio_direction_input(USDHC1_CD_GPIO);
			gpio_direction_output(USDHC1_PWR_GPIO, 0);
			udelay(500);
			gpio_direction_output(USDHC1_PWR_GPIO, 1);
			usdhc_cfg[i].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
			break;
		default:
			printf("Warning: you configured more USDHC controllers"
				"(%d) than supported by the board\n", i + 1);
			return -EINVAL;
			}

			ret = fsl_esdhc_initialize(bis, &usdhc_cfg[i]);
			if (ret) {
				printf("Warning: failed to initialize mmc dev %d\n", i);
				return ret;
			}
	}

	return 0;
}
#endif

#ifdef CONFIG_FEC_MXC
int board_eth_init(bd_t *bis)
{
	int ret;

	setup_iomux_fec2();

	ret = fecmxc_initialize_multi(bis, 0,
		CONFIG_FEC_MXC_PHYADDR, IMX_FEC_BASE);
	if (ret)
		printf("FEC1 MXC: %s:failed\n", __func__);

	return 0;
}

static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *const iomuxc_gpr_regs
		= (struct iomuxc_gpr_base_regs *) IOMUXC_GPR_BASE_ADDR;
	int ret;

	/* Use 125M anatop REF_CLK for ENET2, clear gpr1[14], gpr1[18]*/
	clrsetbits_le32(&iomuxc_gpr_regs->gpr[1],
		(IOMUXC_GPR_GPR1_GPR_ENET2_TX_CLK_SEL_MASK |
		 IOMUXC_GPR_GPR1_GPR_ENET2_CLK_DIR_MASK), 0);

	ret = set_clk_enet(ENET_125MHZ);
	if (ret)
		return ret;

	return 0;
}


int board_phy_config(struct phy_device *phydev)
{
	/* Enable 1.8V(SEL_1P5_1P8_POS_REG) on
	   Phy control debug reg 0 */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x1f);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x8);

	/* rgmii tx clock delay enable */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x100);

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}
#endif

#ifdef CONFIG_MXC_SPI
#ifndef CONFIG_DM_SPI
iomux_v3_cfg_t const ecspi1_pads[] = {
	MX7D_PAD_UART3_RX_DATA__ECSPI1_MISO | MUX_PAD_CTRL(SPI_PAD_CTRL),
	MX7D_PAD_UART3_TX_DATA__ECSPI1_MOSI | MUX_PAD_CTRL(SPI_PAD_CTRL),
	MX7D_PAD_UART3_RTS_B__ECSPI1_SCLK   | MUX_PAD_CTRL(SPI_PAD_CTRL),

	/* CS0 */
	MX7D_PAD_UART3_CTS_B__GPIO4_IO7   | MUX_PAD_CTRL(NO_PAD_CTRL),
};

void setup_spinor(void)
{
	imx_iomux_v3_setup_multiple_pads(ecspi1_pads,
					 ARRAY_SIZE(ecspi1_pads));
	gpio_direction_output(IMX_GPIO_NR(4, 7), 0);
}

int board_spi_cs_gpio(unsigned bus, unsigned cs)
{
	return (bus == 0 && cs == 0) ? (IMX_GPIO_NR(4, 7)) : -1;
}
#endif
#endif

#ifdef CONFIG_USB_EHCI_MX7
#ifndef CONFIG_DM_USB

iomux_v3_cfg_t const usb_otg1_pads[] = {
	MX7D_PAD_GPIO1_IO05__USB_OTG1_PWR | MUX_PAD_CTRL(NO_PAD_CTRL),
};

iomux_v3_cfg_t const usb_otg2_pads[] = {
	MX7D_PAD_GPIO1_IO07__USB_OTG2_PWR | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static void setup_usb(void)
{
	imx_iomux_v3_setup_multiple_pads(usb_otg1_pads, ARRAY_SIZE(usb_otg1_pads));
	imx_iomux_v3_setup_multiple_pads(usb_otg2_pads, ARRAY_SIZE(usb_otg2_pads));
}
#endif
#endif

int board_early_init_f(void)
{
	setup_iomux_uart();

#ifdef CONFIG_SYS_I2C
	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info1);
	setup_i2c(1, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info2);
#endif

#ifdef CONFIG_USB_EHCI_MX7
#ifndef CONFIG_DM_USB
	setup_usb();
#endif
#endif
	return 0;
}

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

	/* Reset peripherals */
	imx_iomux_v3_setup_multiple_pads(per_rst_pads, ARRAY_SIZE(per_rst_pads));

	gpio_request(IMX_GPIO_NR(1, 3), "per rst");
	gpio_direction_output(IMX_GPIO_NR(1, 3) , 0);
	udelay(500);
	gpio_set_value(IMX_GPIO_NR(1, 3), 1);

#ifdef CONFIG_MXC_SPI
#ifndef CONFIG_DM_SPI
	setup_spinor();
#endif
#endif

#ifdef CONFIG_MTD_NOR_FLASH
	setup_eimnor();
#endif

#ifdef CONFIG_NAND_MXS
	setup_gpmi_nand();
#endif

#ifdef CONFIG_FEC_MXC
	setup_fec();
#endif

#ifdef CONFIG_FSL_QSPI
	board_qspi_init();
#endif

	return 0;
}

#ifdef CONFIG_CMD_BMODE
static const struct boot_mode board_boot_modes[] = {
	/* 4 bit bus width */
	{"sd1", MAKE_CFGVAL(0x10, 0x12, 0x00, 0x00)},
	{"qspi", MAKE_CFGVAL(0x00, 0x40, 0x00, 0x00)},
	{NULL,   0},
};
#endif

#ifdef CONFIG_DM_PMIC
int power_init_board(void)
{
	struct udevice *dev;
	int ret, dev_id, rev_id, reg;

	ret = pmic_get("pfuze3000@8", &dev);
	if (ret == -ENODEV)
		return 0;
	if (ret != 0)
		return ret;

	dev_id = pmic_reg_read(dev, PFUZE3000_DEVICEID);
	rev_id = pmic_reg_read(dev, PFUZE3000_REVID);
	printf("PMIC: PFUZE3000 DEV_ID=0x%x REV_ID=0x%x\n", dev_id, rev_id);

	/* disable Low Power Mode during standby mode */
	reg = pmic_reg_read(dev, PFUZE3000_LDOGCTL);
	reg |= 0x1;
	pmic_reg_write(dev, PFUZE3000_LDOGCTL, reg);

	/* SW1A/1B mode set to APS/APS */
	reg = 0x8;
	pmic_reg_write(dev, PFUZE3000_SW1AMODE, reg);
	pmic_reg_write(dev, PFUZE3000_SW1BMODE, reg);

	/* SW1A/1B standby voltage set to 0.975V */
	reg = 0xb;
	pmic_reg_write(dev, PFUZE3000_SW1ASTBY, reg);
	pmic_reg_write(dev, PFUZE3000_SW1BSTBY, reg);

	/* set SW1B normal voltage to 0.975V */
	reg = pmic_reg_read(dev, PFUZE3000_SW1BVOLT);
	reg &= ~0x1f;
	reg |= PFUZE3000_SW1AB_SETP(9750);
	pmic_reg_write(dev, PFUZE3000_SW1BVOLT, reg);

	return 0;
}
#endif

int board_late_init(void)
{
#ifdef CONFIG_CMD_BMODE
	add_board_boot_modes(board_boot_modes);
#endif

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));

	set_wdog_reset((struct wdog_regs *)WDOG1_BASE_ADDR);

	return 0;
}

u32 get_board_rev(void)
{
	return get_cpu_rev();
}

int checkboard(void)
{
#ifdef CONFIG_TARGET_MX7D_19X19_LPDDR2_VAL
	puts("Board: MX7D 19x19 LPDDR2 VAL\n");
#else
	puts("Board: MX7D 19x19 LPDDR3 VAL\n");
#endif
	return 0;
}
