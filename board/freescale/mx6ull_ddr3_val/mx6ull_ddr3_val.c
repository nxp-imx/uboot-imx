/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/arch/clock.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/iomux.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/mx6-pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/gpio.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <asm/io.h>
#include <common.h>
#include <fsl_esdhc_imx.h>
#include <i2c.h>
#include <linux/sizes.h>
#include <linux/fb.h>
#include <miiphy.h>
#include <mmc.h>
#include <mxsfb.h>
#include <netdev.h>
#include <power/pmic.h>
#include <power/pfuze100_pmic.h>
#include "../common/pfuze.h"
#include <usb.h>
#include <usb/ehci-ci.h>
#if defined(CONFIG_MXC_EPDC)
#include <lcd.h>
#include <mxc_epdc_fb.h>
#endif
#include <asm/mach-imx/video.h>

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_22K_UP  | PAD_CTL_SPEED_LOW |		\
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL_WP (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_DOWN | PAD_CTL_SPEED_LOW |		\
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define ENET_PAD_CTRL  (PAD_CTL_PUS_100K_UP | PAD_CTL_PUE |     \
	PAD_CTL_SPEED_HIGH   |                                   \
	PAD_CTL_DSE_48ohm   | PAD_CTL_SRE_FAST)

#define ENET_CLK_PAD_CTRL  (PAD_CTL_SPEED_MED | \
	PAD_CTL_DSE_120ohm   | PAD_CTL_SRE_FAST)

#define ENET_RX_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |          \
	PAD_CTL_SPEED_HIGH   | PAD_CTL_SRE_FAST)

#define I2C_PAD_CTRL    (PAD_CTL_PKE | PAD_CTL_PUE |            \
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |               \
	PAD_CTL_DSE_40ohm | PAD_CTL_HYS |			\
	PAD_CTL_ODE)

#define LCD_PAD_CTRL    (PAD_CTL_HYS | PAD_CTL_PUS_100K_UP | PAD_CTL_PUE | \
	PAD_CTL_PKE | PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm)

#define GPMI_PAD_CTRL0 (PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP)
#define GPMI_PAD_CTRL1 (PAD_CTL_DSE_40ohm | PAD_CTL_SPEED_MED | \
			PAD_CTL_SRE_FAST)
#define GPMI_PAD_CTRL2 (GPMI_PAD_CTRL0 | GPMI_PAD_CTRL1)

#define WEIM_NOR_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE | \
		PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED | \
		PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST)

#define SPI_PAD_CTRL (PAD_CTL_HYS |				\
	PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm | PAD_CTL_SRE_FAST)

#define OTG_ID_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_47K_UP  | PAD_CTL_SPEED_LOW |		\
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define EPDC_PAD_CTRL	0x010b1

#ifdef CONFIG_SYS_I2C
#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)
/* I2C1 for PMIC and EEPROM */
struct i2c_pads_info i2c_pad_info1 = {
	.scl = {
		/* conflict with usb_otg2_pwr */
		.i2c_mode = MX6_PAD_GPIO1_IO02__I2C1_SCL | PC,
		.gpio_mode = MX6_PAD_GPIO1_IO02__GPIO1_IO02 | PC,
		.gp = IMX_GPIO_NR(1, 2),
	},
	.sda = {
		/* conflict with usb_otg2_oc */
		.i2c_mode = MX6_PAD_GPIO1_IO03__I2C1_SDA | PC,
		.gpio_mode = MX6_PAD_GPIO1_IO03__GPIO1_IO03 | PC,
		.gp = IMX_GPIO_NR(1, 3),
	},
};
#endif

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_SIZE;

	return 0;
}

static iomux_v3_cfg_t const uart1_pads[] = {
	MX6_PAD_UART1_TX_DATA__UART1_DCE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX6_PAD_UART1_RX_DATA__UART1_DCE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

#ifdef CONFIG_MX6ULL_DDR3_VAL_EMMC_REWORK
static iomux_v3_cfg_t const usdhc1_emmc_pads[] = {
	MX6_PAD_SD1_CLK__USDHC1_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_CMD__USDHC1_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA0__USDHC1_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA1__USDHC1_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA2__USDHC1_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA3__USDHC1_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	/*
	 * The following 4 pins conflicts with qspi and nand flash.
	 * You can comment out the following 4 pins and change
	 * {USDHC1_BASE_ADDR, 0, 8}  -> {USDHC1_BASE_ADDR, 0, 4}
	 * to make emmc and qspi coexists.
	 */
	MX6_PAD_NAND_READY_B__USDHC1_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_CE0_B__USDHC1_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_CE1_B__USDHC1_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_CLE__USDHC1_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),

	/* Default NO WP for emmc, since we use pull down */
	MX6_PAD_UART1_CTS_B__USDHC1_WP  | MUX_PAD_CTRL(USDHC_PAD_CTRL_WP),
	/* RST_B */
	MX6_PAD_GPIO1_IO09__GPIO1_IO09 | MUX_PAD_CTRL(NO_PAD_CTRL),
};
#else
static iomux_v3_cfg_t const usdhc1_pads[] = {
	MX6_PAD_SD1_CLK__USDHC1_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_CMD__USDHC1_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA0__USDHC1_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA1__USDHC1_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA2__USDHC1_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA3__USDHC1_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_UART1_CTS_B__USDHC1_WP | MUX_PAD_CTRL(USDHC_PAD_CTRL),

	/* VSELECT */
	MX6_PAD_GPIO1_IO05__GPIO1_IO05 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	/* CD */
	MX6_PAD_UART1_RTS_B__GPIO1_IO19 | MUX_PAD_CTRL(NO_PAD_CTRL),
	/* RST_B */
	MX6_PAD_GPIO1_IO09__GPIO1_IO09 | MUX_PAD_CTRL(NO_PAD_CTRL),
};
#endif

#if !defined(CONFIG_NAND_MXS) && !defined(CONFIG_MX6ULL_DDR3_VAL_QSPIB_REWORK)
static iomux_v3_cfg_t const usdhc2_pads[] = {
	/* usdhc2_clk, nand_re_b, qspi1b_clk */
	MX6_PAD_NAND_RE_B__USDHC2_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	/* usdhc2_cmd, nand_we_b, qspi1b_cs0_b */
	MX6_PAD_NAND_WE_B__USDHC2_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	/* usdhc2_data0, nand_data0, qspi1b_cs1_b */
	MX6_PAD_NAND_DATA00__USDHC2_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	/* usdhc2_data1, nand_data1 */
	MX6_PAD_NAND_DATA01__USDHC2_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	/* usdhc2_data2, nand_data2, qspi1b_dat0 */
	MX6_PAD_NAND_DATA02__USDHC2_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	/* usdhc2_data3, nand_data3, qspi1b_dat1 */
	MX6_PAD_NAND_DATA03__USDHC2_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),

	/*
	 * VSELECT
	 * Conflicts with WDOG1, so default disabled.
	 * MX6_PAD_GPIO1_IO08__USDHC2_VSELECT | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	 */
	/*
	 * CD
	 * Share with sdhc1
	 * MX6_PAD_CSI_MCLK__GPIO4_IO17 | MUX_PAD_CTRL(NO_PAD_CTRL),
	 */
	/*
	 * RST_B
	 * Pin conflicts with NAND ALE, if want to test nand,
	 * Connect R169(B), disconnect R169(A).
	 */
	MX6_PAD_NAND_ALE__GPIO4_IO10 | MUX_PAD_CTRL(NO_PAD_CTRL),
};
#endif

#ifdef CONFIG_NAND_MXS
static iomux_v3_cfg_t const nand_pads[] = {
	MX6_PAD_NAND_DATA00__RAWNAND_DATA00 | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_DATA01__RAWNAND_DATA01 | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_DATA02__RAWNAND_DATA02 | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_DATA03__RAWNAND_DATA03 | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_DATA04__RAWNAND_DATA04 | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_DATA05__RAWNAND_DATA05 | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_DATA06__RAWNAND_DATA06 | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_DATA07__RAWNAND_DATA07 | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_CLE__RAWNAND_CLE | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_ALE__RAWNAND_ALE | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_CE0_B__RAWNAND_CE0_B | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_CE1_B__RAWNAND_CE1_B | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_CSI_MCLK__RAWNAND_CE2_B   | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_CSI_PIXCLK__RAWNAND_CE3_B | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_RE_B__RAWNAND_RE_B | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_WE_B__RAWNAND_WE_B | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_WP_B__RAWNAND_WP_B | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_READY_B__RAWNAND_READY_B | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_DQS__RAWNAND_DQS | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
};

static void setup_gpmi_nand(void)
{
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;

	/* config gpmi nand iomux */
	SETUP_IOMUX_PADS(nand_pads);

	setup_gpmi_io_clk((MXC_CCM_CS2CDR_ENFC_CLK_PODF(0) |
			MXC_CCM_CS2CDR_ENFC_CLK_PRED(3) |
			MXC_CCM_CS2CDR_ENFC_CLK_SEL(3)));

	/* enable apbh clock gating */
	setbits_le32(&mxc_ccm->CCGR0, MXC_CCM_CCGR0_APBHDMA_MASK);
}
#endif

#ifdef CONFIG_MXC_SPI
#ifndef CONFIG_DM_SPI
static iomux_v3_cfg_t const ecspi1_pads[] = {
	MX6_PAD_CSI_DATA06__ECSPI1_MOSI | MUX_PAD_CTRL(SPI_PAD_CTRL),
	MX6_PAD_CSI_DATA04__ECSPI1_SCLK | MUX_PAD_CTRL(SPI_PAD_CTRL),
	MX6_PAD_CSI_DATA07__ECSPI1_MISO | MUX_PAD_CTRL(SPI_PAD_CTRL),

	/* CS Pin */
	MX6_PAD_CSI_DATA05__GPIO4_IO26 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static void setup_spinor(void)
{
	SETUP_IOMUX_PADS(ecspi1_pads);
	gpio_request(IMX_GPIO_NR(4, 26), "escpi cs");
	gpio_direction_output(IMX_GPIO_NR(4, 26), 0);
}

int board_spi_cs_gpio(unsigned bus, unsigned cs)
{
	return (bus == 0 && cs == 0) ? (IMX_GPIO_NR(4, 26)) : -1;
}
#endif
#endif

#ifdef CONFIG_FEC_MXC
/*
 * pin conflicts for fec1 and fec2, GPIO1_IO06 and GPIO1_IO07 can only
 * be used for ENET1 or ENET2, cannot be used for both.
 */
static iomux_v3_cfg_t const fec1_pads[] = {
	MX6_PAD_GPIO1_IO06__ENET1_MDIO | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_GPIO1_IO07__ENET1_MDC | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_TX_DATA0__ENET1_TDATA00 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_TX_DATA1__ENET1_TDATA01 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_TX_EN__ENET1_TX_EN | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_TX_CLK__ENET1_REF_CLK1 | MUX_PAD_CTRL(ENET_CLK_PAD_CTRL),
	/* Pin conflicts with LCD PWM1 */
	MX6_PAD_ENET1_RX_DATA0__ENET1_RDATA00 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_RX_DATA1__ENET1_RDATA01 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_RX_ER__ENET1_RX_ER | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_RX_EN__ENET1_RX_EN | MUX_PAD_CTRL(ENET_PAD_CTRL),
};

static iomux_v3_cfg_t const fec1_phy_rst[] = {
	/*
	 * ALT5 mode is only valid when TAMPER pin is used for GPIO.
	 * This depends on FUSE settings, TAMPER_PIN_DISABLE[1:0].
	 *
	 * ENET1_RST
	 */
	MX6_PAD_SNVS_TAMPER2__GPIO5_IO02 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static iomux_v3_cfg_t const fec2_pads[] = {
	MX6_PAD_GPIO1_IO06__ENET2_MDIO | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_GPIO1_IO07__ENET2_MDC | MUX_PAD_CTRL(ENET_PAD_CTRL),

	MX6_PAD_ENET2_RX_DATA0__ENET2_RDATA00 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET2_RX_DATA1__ENET2_RDATA01 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_UART3_TX_DATA__ENET2_RDATA02 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_UART3_RX_DATA__ENET2_RDATA03 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET2_RX_EN__ENET2_RX_EN | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET2_RX_ER__ENET2_RX_ER | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_UART3_CTS_B__ENET2_RX_CLK | MUX_PAD_CTRL(ENET_PAD_CTRL),

	MX6_PAD_ENET2_TX_DATA0__ENET2_TDATA00 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET2_TX_DATA1__ENET2_TDATA01 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_UART4_TX_DATA__ENET2_TDATA02 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_UART4_RX_DATA__ENET2_TDATA03 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET2_TX_CLK__ENET2_TX_CLK | MUX_PAD_CTRL(ENET_CLK_PAD_CTRL),
	MX6_PAD_ENET2_TX_EN__ENET2_TX_EN | MUX_PAD_CTRL(ENET_PAD_CTRL),

	MX6_PAD_UART5_RX_DATA__ENET2_COL | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_UART5_TX_DATA__ENET2_CRS | MUX_PAD_CTRL(ENET_PAD_CTRL),
};

static iomux_v3_cfg_t const fec2_phy_rst[] = {
	/*
	 * ENET2_RST
	 *
	 * This depends on FUSE settings, TAMPER_PIN_DISABLE[1:0]
	 */
	MX6_PAD_SNVS_TAMPER4__GPIO5_IO04 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static void setup_iomux_fec(int fec_id)
{
	if (fec_id == 0) {
		SETUP_IOMUX_PADS(fec1_pads);
	} else {
		SETUP_IOMUX_PADS(fec2_pads);
	}
}
#endif

static void setup_iomux_uart(void)
{
	SETUP_IOMUX_PADS(uart1_pads);
}

#ifdef CONFIG_FSL_QSPI

#ifndef CONFIG_DM_SPI
#define QSPI_PAD_CTRL1	\
	(PAD_CTL_SRE_FAST | PAD_CTL_SPEED_MED | \
	 PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_47K_UP | PAD_CTL_DSE_120ohm)

static iomux_v3_cfg_t const quadspi_pads[] = {
	MX6_PAD_NAND_WP_B__QSPI_A_SCLK	| MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6_PAD_NAND_READY_B__QSPI_A_DATA00	| MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6_PAD_NAND_CE0_B__QSPI_A_DATA01	| MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6_PAD_NAND_CE1_B__QSPI_A_DATA02	| MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6_PAD_NAND_CLE__QSPI_A_DATA03	| MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6_PAD_NAND_DQS__QSPI_A_SS0_B	| MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6_PAD_NAND_DATA07__QSPI_A_SS1_B	| MUX_PAD_CTRL(QSPI_PAD_CTRL1),

#ifdef CONFIG_MX6ULL_DDR3_VAL_QSPIB_REWORK
	MX6_PAD_NAND_RE_B__QSPI_B_SCLK	| MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6_PAD_NAND_WE_B__QSPI_B_SS0_B	| MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6_PAD_NAND_DATA00__QSPI_B_SS1_B	| MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6_PAD_NAND_DATA02__QSPI_B_DATA00	| MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6_PAD_NAND_DATA03__QSPI_B_DATA01	| MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6_PAD_NAND_DATA04__QSPI_B_DATA02	| MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6_PAD_NAND_DATA05__QSPI_B_DATA03	| MUX_PAD_CTRL(QSPI_PAD_CTRL1),
#endif
};
#endif

int board_qspi_init(void)
{
#ifndef CONFIG_DM_SPI
	/* Set the iomux */
	SETUP_IOMUX_PADS(quadspi_pads);
#endif
	/* Set the clock */
	enable_qspi_clk(0);

	return 0;
}
#endif

#ifdef CONFIG_FSL_ESDHC
static struct fsl_esdhc_cfg usdhc_cfg[2] = {
#ifdef CONFIG_MX6ULL_DDR3_VAL_EMMC_REWORK
	/* If want to use qspi, should change to 4 bit width */
	{USDHC1_BASE_ADDR, 0, 8},
#else
	{USDHC1_BASE_ADDR, 0, 4},
#endif
#if !defined(CONFIG_NAND_MXS) && !defined(CONFIG_MX6ULL_DDR3_VAL_QSPIB_REWORK)
	{USDHC2_BASE_ADDR, 0, 4},
#endif
};

#define USDHC1_CD_GPIO	IMX_GPIO_NR(1, 19)
#define USDHC1_PWR_GPIO	IMX_GPIO_NR(1, 9)
#define USDHC1_VSELECT IMX_GPIO_NR(1, 5)
#define USDHC2_PWR_GPIO	IMX_GPIO_NR(4, 10)

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;

	switch (cfg->esdhc_base) {
	case USDHC1_BASE_ADDR:
#ifdef CONFIG_MX6ULL_DDR3_VAL_EMMC_REWORK
		ret = 1;
#else
		ret = !gpio_get_value(USDHC1_CD_GPIO);
#endif
		break;
#if !defined(CONFIG_NAND_MXS) && !defined(CONFIG_MX6ULL_DDR3_VAL_QSPIB_REWORK)
	case USDHC2_BASE_ADDR:
		ret = 1;
		break;
#endif
	}

	return ret;
}

int board_mmc_init(bd_t *bis)
{
	int i;

	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-boot device node)    (Physical Port)
	 * mmc0                    USDHC1
	 * mmc1                    USDHC2
	 */
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
#ifdef CONFIG_MX6ULL_DDR3_VAL_EMMC_REWORK
			SETUP_IOMUX_PADS(usdhc1_emmc_pads);
#else
			SETUP_IOMUX_PADS(usdhc1_pads);
			gpio_request(USDHC1_CD_GPIO, "usdhc1 cd");
			gpio_direction_input(USDHC1_CD_GPIO);
#endif
			usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
			/* 3.3V */
			gpio_request(USDHC1_VSELECT, "usdhc1 vsel");
			gpio_request(USDHC1_PWR_GPIO, "usdhc1 pwr");
			gpio_direction_output(USDHC1_VSELECT, 0);
			gpio_direction_output(USDHC1_PWR_GPIO, 1);
			break;
#if !defined(CONFIG_NAND_MXS) && !defined(CONFIG_MX6ULL_DDR3_VAL_QSPIB_REWORK)
		case 1:
			SETUP_IOMUX_PADS(usdhc2_pads);
			gpio_request(USDHC2_PWR_GPIO, "usdhc2 pwr");
			gpio_direction_output(USDHC2_PWR_GPIO, 1);
			usdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
			break;
#endif
		default:
			printf("Warning: you configured more USDHC controllers (%d) than supported by the board\n", i + 1);
			return 0;
			}

			if (fsl_esdhc_initialize(bis, &usdhc_cfg[i]))
				printf("Warning: failed to initialize mmc dev %d\n", i);
	}

	return 0;
}
#endif

#ifdef CONFIG_VIDEO_MXS
static iomux_v3_cfg_t const lcd_pads[] = {
	MX6_PAD_LCD_CLK__LCDIF_CLK | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_ENABLE__LCDIF_ENABLE | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_HSYNC__LCDIF_HSYNC | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_VSYNC__LCDIF_VSYNC | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA00__LCDIF_DATA00 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA01__LCDIF_DATA01 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA02__LCDIF_DATA02 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA03__LCDIF_DATA03 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA04__LCDIF_DATA04 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA05__LCDIF_DATA05 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA06__LCDIF_DATA06 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA07__LCDIF_DATA07 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA08__LCDIF_DATA08 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA09__LCDIF_DATA09 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA10__LCDIF_DATA10 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA11__LCDIF_DATA11 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA12__LCDIF_DATA12 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA13__LCDIF_DATA13 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA14__LCDIF_DATA14 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA15__LCDIF_DATA15 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA16__LCDIF_DATA16 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA17__LCDIF_DATA17 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA18__LCDIF_DATA18 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA19__LCDIF_DATA19 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA20__LCDIF_DATA20 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA21__LCDIF_DATA21 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA22__LCDIF_DATA22 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA23__LCDIF_DATA23 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_RESET__GPIO3_IO04 | MUX_PAD_CTRL(NO_PAD_CTRL),

	/*
	 * PWM1, pin conflicts with ENET1_RX_DATA0
	 * Use GPIO for Brightness adjustment, duty cycle = period.
	 */
	/* MX6_PAD_ENET1_RX_DATA0__GPIO2_IO00 | MUX_PAD_CTRL(NO_PAD_CTRL),*/
};

struct lcd_panel_info_t {
	unsigned int lcdif_base_addr;
	int depth;
	void (*enable)(struct lcd_panel_info_t const *dev);
	struct fb_videomode mode;
};

void do_enable_parallel_lcd(struct display_info_t const *dev)
{
	enable_lcdif_clock(dev->bus, 1);

	SETUP_IOMUX_PADS(lcd_pads);

	/* Power up the LCD */
	gpio_request(IMX_GPIO_NR(3, 4), "lcd power");
	gpio_direction_output(IMX_GPIO_NR(3, 4) , 1);

	/* Set Brightness to high */
	/* gpio_direction_output(IMX_GPIO_NR(2, 0) , 1); */
}

struct display_info_t const displays[] = {{
	.bus = MX6ULL_LCDIF1_BASE_ADDR,
	.addr = 0,
	.pixfmt = 24,
	.detect = NULL,
	.enable	= do_enable_parallel_lcd,
	.mode	= {
		.name		= "MCIMX28LCD",
		.xres           = 800,
		.yres           = 480,
		.pixclock       = 29850,
		.left_margin    = 89,
		.right_margin   = 164,
		.upper_margin   = 23,
		.lower_margin   = 10,
		.hsync_len      = 10,
		.vsync_len      = 10,
		.sync           = 0,
		.vmode          = FB_VMODE_NONINTERLACED
} } };
size_t display_count = ARRAY_SIZE(displays);
#endif

#ifdef CONFIG_MXC_EPDC
static iomux_v3_cfg_t const epdc_enable_pads[] = {
	MX6_PAD_ENET2_RX_DATA0__EPDC_SDDO08	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_ENET2_RX_DATA1__EPDC_SDDO09	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_ENET2_RX_EN__EPDC_SDDO10	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_ENET2_TX_DATA0__EPDC_SDDO11	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_ENET2_TX_DATA1__EPDC_SDDO12	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_ENET2_TX_EN__EPDC_SDDO13	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_ENET2_TX_CLK__EPDC_SDDO14	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_ENET2_RX_ER__EPDC_SDDO15	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_LCD_CLK__EPDC_SDCLK		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_LCD_ENABLE__EPDC_SDLE		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_LCD_HSYNC__EPDC_SDOE		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_LCD_VSYNC__EPDC_SDCE0		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_LCD_DATA00__EPDC_SDDO00	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_LCD_DATA01__EPDC_SDDO01	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_LCD_DATA02__EPDC_SDDO02	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_LCD_DATA03__EPDC_SDDO03	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_LCD_DATA04__EPDC_SDDO04	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_LCD_DATA05__EPDC_SDDO05	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_LCD_DATA06__EPDC_SDDO06	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_LCD_DATA07__EPDC_SDDO07	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_LCD_DATA14__EPDC_SDSHR	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_LCD_DATA15__EPDC_GDRL		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_LCD_DATA16__EPDC_GDCLK	| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_LCD_DATA17__EPDC_GDSP		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
	MX6_PAD_LCD_RESET__EPDC_GDOE		| MUX_PAD_CTRL(EPDC_PAD_CTRL),
};

static iomux_v3_cfg_t const epdc_disable_pads[] = {
	MX6_PAD_ENET2_RX_DATA0__GPIO2_IO08,
	MX6_PAD_ENET2_RX_DATA1__GPIO2_IO09,
	MX6_PAD_ENET2_RX_EN__GPIO2_IO10,
	MX6_PAD_ENET2_TX_DATA0__GPIO2_IO11,
	MX6_PAD_ENET2_TX_DATA1__GPIO2_IO12,
	MX6_PAD_ENET2_TX_EN__GPIO2_IO13,
	MX6_PAD_ENET2_TX_CLK__GPIO2_IO14,
	MX6_PAD_ENET2_RX_ER__GPIO2_IO15,
	MX6_PAD_LCD_CLK__GPIO3_IO00,
	MX6_PAD_LCD_ENABLE__GPIO3_IO01,
	MX6_PAD_LCD_HSYNC__GPIO3_IO02,
	MX6_PAD_LCD_VSYNC__GPIO3_IO03,
	MX6_PAD_LCD_DATA00__GPIO3_IO05,
	MX6_PAD_LCD_DATA01__GPIO3_IO06,
	MX6_PAD_LCD_DATA02__GPIO3_IO07,
	MX6_PAD_LCD_DATA03__GPIO3_IO08,
	MX6_PAD_LCD_DATA04__GPIO3_IO09,
	MX6_PAD_LCD_DATA05__GPIO3_IO10,
	MX6_PAD_LCD_DATA06__GPIO3_IO11,
	MX6_PAD_LCD_DATA07__GPIO3_IO12,
	MX6_PAD_LCD_DATA14__GPIO3_IO19,
	MX6_PAD_LCD_DATA15__GPIO3_IO20,
	MX6_PAD_LCD_DATA16__GPIO3_IO21,
	MX6_PAD_LCD_DATA17__GPIO3_IO22,
	MX6_PAD_LCD_RESET__GPIO3_IO04,
};

vidinfo_t panel_info = {
	.vl_refresh = 85,
	.vl_col = 1024,
	.vl_row = 758,
	.vl_pixclock = 40000000,
	.vl_left_margin = 12,
	.vl_right_margin = 76,
	.vl_upper_margin = 4,
	.vl_lower_margin = 5,
	.vl_hsync = 12,
	.vl_vsync = 2,
	.vl_sync = 0,
	.vl_mode = 0,
	.vl_flag = 0,
	.vl_bpix = 3,
	.cmap = 0,
};

struct epdc_timing_params panel_timings = {
	.vscan_holdoff = 4,
	.sdoed_width = 10,
	.sdoed_delay = 20,
	.sdoez_width = 10,
	.sdoez_delay = 20,
	.gdclk_hp_offs = 524,
	.gdsp_offs = 327,
	.gdoe_offs = 0,
	.gdclk_offs = 19,
	.num_ce = 1,
};

static iomux_v3_cfg_t const epdc_pwr_ctrl_pads[] = {
	IOMUX_PADS(PAD_LCD_DATA11__GPIO3_IO16	| MUX_PAD_CTRL(EPDC_PAD_CTRL)),
	IOMUX_PADS(PAD_LCD_DATA19__GPIO3_IO24	| MUX_PAD_CTRL(EPDC_PAD_CTRL)),
	IOMUX_PADS(PAD_LCD_DATA09__GPIO3_IO14	| MUX_PAD_CTRL(EPDC_PAD_CTRL)),
	IOMUX_PADS(PAD_LCD_DATA12__GPIO3_IO17	| MUX_PAD_CTRL(EPDC_PAD_CTRL)),
};

static void setup_epdc_power(void)
{
	SETUP_IOMUX_PADS(epdc_pwr_ctrl_pads);

	/* Setup epdc voltage */

	/* EPDC_PWRSTAT - GPIO3[16] for PWR_GOOD status */
	gpio_request(IMX_GPIO_NR(3, 16), "EPDC_PWRSTAT");
	gpio_direction_input(IMX_GPIO_NR(3, 16));

	/* EPDC_VCOM0 - GPIO3[24] for VCOM control */
	/* Set as output */
	gpio_request(IMX_GPIO_NR(3, 24), "EPDC_VCOM0");
	gpio_direction_output(IMX_GPIO_NR(3, 24), 1);

	/* EPDC_PWRWAKEUP - GPIO3[14] for EPD PMIC WAKEUP */
	/* Set as output */
	gpio_request(IMX_GPIO_NR(3, 14), "EPDC_PWRWAKEUP");
	gpio_direction_output(IMX_GPIO_NR(3, 14), 1);

	/* EPDC_PWRCTRL0 - GPIO3[17] for EPD PWR CTL0 */
	/* Set as output */
	gpio_request(IMX_GPIO_NR(3, 17), "EPDC_PWRCTRL0");
	gpio_direction_output(IMX_GPIO_NR(3, 17), 1);
}

static void epdc_enable_pins(void)
{
	/* epdc iomux settings */
	SETUP_IOMUX_PADS(epdc_enable_pads);
}

static void epdc_disable_pins(void)
{
	/* Configure MUX settings for EPDC pins to GPIO  and drive to 0 */
	SETUP_IOMUX_PADS(epdc_disable_pads);
}

static void setup_epdc(void)
{
	/* Set pixel clock rates for EPDC in clock.c */

	panel_info.epdc_data.wv_modes.mode_init = 0;
	panel_info.epdc_data.wv_modes.mode_du = 1;
	panel_info.epdc_data.wv_modes.mode_gc4 = 3;
	panel_info.epdc_data.wv_modes.mode_gc8 = 2;
	panel_info.epdc_data.wv_modes.mode_gc16 = 2;
	panel_info.epdc_data.wv_modes.mode_gc32 = 2;

	panel_info.epdc_data.epdc_timings = panel_timings;

	setup_epdc_power();
}

void epdc_power_on(void)
{
	unsigned int reg;
	struct gpio_regs *gpio_regs = (struct gpio_regs *)GPIO3_BASE_ADDR;

	/* Set EPD_PWR_CTL0 to high - enable EINK_VDD (3.15) */
	gpio_set_value(IMX_GPIO_NR(3, 17), 1);
	udelay(1000);

	/* Enable epdc signal pin */
	epdc_enable_pins();

	/* Set PMIC Wakeup to high - enable Display power */
	gpio_set_value(IMX_GPIO_NR(3, 14), 1);

	/* Wait for PWRGOOD == 1 */
	while (1) {
		reg = readl(&gpio_regs->gpio_psr);
		if (!(reg & (1 << 16)))
			break;

		udelay(100);
	}

	/* Enable VCOM */
	gpio_set_value(IMX_GPIO_NR(3, 24), 1);

	udelay(500);
}

void epdc_power_off(void)
{
	/* Set PMIC Wakeup to low - disable Display power */
	gpio_set_value(IMX_GPIO_NR(3, 14), 0);

	/* Disable VCOM */
	gpio_set_value(IMX_GPIO_NR(3, 24), 0);

	epdc_disable_pins();

	/* Set EPD_PWR_CTL0 to low - disable EINK_VDD (3.15) */
	gpio_set_value(IMX_GPIO_NR(3, 17), 0);
}
#endif

#ifdef CONFIG_FEC_MXC
int board_eth_init(bd_t *bis)
{
	int ret;

	setup_iomux_fec(CONFIG_FEC_ENET_DEV);

	ret = fecmxc_initialize_multi(bis, CONFIG_FEC_ENET_DEV,
		CONFIG_FEC_MXC_PHYADDR, IMX_FEC_BASE);
	if (ret)
		printf("FEC%d MXC: %s:failed\n", CONFIG_FEC_ENET_DEV, __func__);

	return 0;
}

static int setup_fec(int fec_id)
{
	struct iomuxc_gpr_base_regs *const iomuxc_gpr_regs
		= (struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;
	int ret;

	if (0 == fec_id) {
		if (check_module_fused(MX6_MODULE_ENET1))
			return -1;
		/*
		 * Use 50M anatop loopback REF_CLK1 for ENET1,
		 * clear gpr1[13], set gpr1[17]
		 */
		clrsetbits_le32(&iomuxc_gpr_regs->gpr[1], IOMUX_GPR1_FEC1_MASK,
				IOMUX_GPR1_FEC1_CLOCK_MUX1_SEL_MASK);
		ret = enable_fec_anatop_clock(fec_id, ENET_50MHZ);
		if (ret)
			return ret;

		SETUP_IOMUX_PADS(fec1_phy_rst);
		gpio_request(IMX_GPIO_NR(5, 2), "fec1 reset");
		gpio_direction_output(IMX_GPIO_NR(5, 2), 0);
		udelay(50);
		gpio_direction_output(IMX_GPIO_NR(5, 2), 1);

	} else {
		if (check_module_fused(MX6_MODULE_ENET2))
			return -1;

		/* clk from phy, set gpr1[14], clear gpr1[18]*/
		clrsetbits_le32(&iomuxc_gpr_regs->gpr[1], IOMUX_GPR1_FEC2_MASK,
				IOMUX_GPR1_FEC2_CLOCK_MUX2_SEL_MASK);

		SETUP_IOMUX_PADS(fec2_phy_rst);
		gpio_request(IMX_GPIO_NR(5, 4), "fec2 reset");
		gpio_direction_output(IMX_GPIO_NR(5, 4), 0);
		udelay(50);
		gpio_direction_output(IMX_GPIO_NR(5, 4), 1);
	}

	enable_enet_clk(1);

	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
	if (CONFIG_FEC_ENET_DEV == 0) {
		phy_write(phydev, MDIO_DEVAD_NONE, 0x16, 0x202);
		phy_write(phydev, MDIO_DEVAD_NONE, 0x1f, 0x8190);
	} else if (CONFIG_FEC_ENET_DEV == 1) {
		phy_write(phydev, MDIO_DEVAD_NONE, 0x16, 0x201);
		phy_write(phydev, MDIO_DEVAD_NONE, 0x1f, 0x8110);
	}

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}
#endif

#ifdef CONFIG_POWER
#define I2C_PMIC	0
static struct pmic *pfuze;
int power_init_board(void)
{
	int ret;
	u32 rev_id, value;

	ret = power_pfuze100_init(I2C_PMIC);
	if (ret)
		return ret;

	pfuze = pmic_get("PFUZE100");
	if (!pfuze)
		return -ENODEV;

	ret = pmic_probe(pfuze);
	if (ret)
		return ret;

	ret = pfuze_mode_init(pfuze, APS_PFM);
	if (ret < 0)
		return ret;

	pmic_reg_read(pfuze, PFUZE100_DEVICEID, &value);
	pmic_reg_read(pfuze, PFUZE100_REVID, &rev_id);
	printf("PMIC: PFUZE200! DEV_ID=0x%x REV_ID=0x%x\n", value, rev_id);

	/*
	 * Our PFUZE0200 is PMPF0200X0AEP, the Pre-programmed OTP
	 * Configuration is F0.
	 * Default VOLT:
	 * VSNVS_VOLT	|	3.0V
	 * SW1AB	|	1.375V
	 * SW2		|	3.3V
	 * SW3A		|	1.5V
	 * SW3B		|	1.5V
	 * VGEN1	|	1.5V
	 * VGEN2	|	1.5V
	 * VGEN3	|	2.5V
	 * VGEN4	|	1.8V
	 * VGEN5	|	2.8V
	 * VGEN6	|	3.3V
	 *
	 * According to schematic, we need SW3A 1.35V, SW3B 3.3V,
	 * VGEN1 1.2V, VGEN2 1.5V, VGEN3 2.8V, VGEN4 1.8V,
	 * VGEN5 3.3V, VGEN6 3.0V.
	 *
	 * Here we just use the default VOLT, but not configure
	 * them, when needed, configure them to our requested voltage.
	 */

	/* set SW1AB standby volatage 0.975V */
	pmic_reg_read(pfuze, PFUZE100_SW1ABSTBY, &value);
	value &= ~0x3f;
	value |= PFUZE100_SW1ABC_SETP(9750);
	pmic_reg_write(pfuze, PFUZE100_SW1ABSTBY, value);

	/* set SW1AB/VDDARM step ramp up time from 16us to 4us/25mV */
	pmic_reg_read(pfuze, PFUZE100_SW1ABCONF, &value);
	value &= ~0xc0;
	value |= 0x40;
	pmic_reg_write(pfuze, PFUZE100_SW1ABCONF, value);

	/* Enable power of VGEN5 3V3 */
	pmic_reg_read(pfuze, PFUZE100_VGEN5VOL, &value);
	value &= ~0x1F;
	value |= 0x1F;
	pmic_reg_write(pfuze, PFUZE100_VGEN5VOL, value);

	return 0;
}

#ifdef CONFIG_LDO_BYPASS_CHECK
void ldo_mode_set(int ldo_bypass)
{
	unsigned int value;
	int is_400M;
	u32 vddarm;

	struct pmic *p = pfuze;

	if (!p) {
		printf("No PMIC found!\n");
		return;
	}

	/* switch to ldo_bypass mode */
	if (ldo_bypass) {
		prep_anatop_bypass();
		/* decrease VDDARM to 1.275V */
		pmic_reg_read(pfuze, PFUZE100_SW1ABVOL, &value);
		value &= ~0x3f;
		value |= PFUZE100_SW1ABC_SETP(12750);
		pmic_reg_write(pfuze, PFUZE100_SW1ABVOL, value);

		is_400M = set_anatop_bypass(1);
		if (is_400M)
			vddarm = PFUZE100_SW1ABC_SETP(10750);
		else
			vddarm = PFUZE100_SW1ABC_SETP(11750);

		pmic_reg_read(pfuze, PFUZE100_SW1ABVOL, &value);
		value &= ~0x3f;
		value |= vddarm;
		pmic_reg_write(pfuze, PFUZE100_SW1ABVOL, value);

		finish_anatop_bypass();

		printf("switch to ldo_bypass mode!\n");
	}
}
#endif

#elif defined(CONFIG_DM_PMIC_PFUZE100)
int power_init_board(void)
{
	struct udevice *dev;
	int ret;
	unsigned int reg, dev_id, rev_id;

	ret = pmic_get("pfuze100@8", &dev);
	if (ret == -ENODEV)
		return ret;

	ret = pfuze_mode_init(dev, APS_PFM);
	if (ret < 0)
		return ret;

	dev_id = pmic_reg_read(dev, PFUZE100_DEVICEID);
	rev_id = pmic_reg_read(dev, PFUZE100_REVID);
	printf("PMIC: PFUZE200! DEV_ID=0x%x REV_ID=0x%x\n", dev_id, rev_id);

	/*
	 * Our PFUZE0200 is PMPF0200X0AEP, the Pre-programmed OTP
	 * Configuration is F0.
	 * Default VOLT:
	 * VSNVS_VOLT	|	3.0V
	 * SW1AB	|	1.375V
	 * SW2		|	3.3V
	 * SW3A		|	1.5V
	 * SW3B		|	1.5V
	 * VGEN1	|	1.5V
	 * VGEN2	|	1.5V
	 * VGEN3	|	2.5V
	 * VGEN4	|	1.8V
	 * VGEN5	|	2.8V
	 * VGEN6	|	3.3V
	 *
	 * According to schematic, we need SW3A 1.35V, SW3B 3.3V,
	 * VGEN1 1.2V, VGEN2 1.5V, VGEN3 2.8V, VGEN4 1.8V,
	 * VGEN5 3.3V, VGEN6 3.0V.
	 *
	 * Here we just use the default VOLT, but not configure
	 * them, when needed, configure them to our requested voltage.
	 */

	/* Set SW1AB stanby volage to 0.975V */
	reg = pmic_reg_read(dev, PFUZE100_SW1ABSTBY);
	reg &= ~SW1x_STBY_MASK;
	reg |= SW1x_0_975V;
	pmic_reg_write(dev, PFUZE100_SW1ABSTBY, reg);

	/* Set SW1AB/VDDARM step ramp up time from 16us to 4us/25mV */
	reg = pmic_reg_read(dev, PFUZE100_SW1ABCONF);
	reg &= ~SW1xCONF_DVSSPEED_MASK;
	reg |= SW1xCONF_DVSSPEED_4US;
	pmic_reg_write(dev, PFUZE100_SW1ABCONF, reg);

	/* Enable power of VGEN5 3V3 */
	reg = pmic_reg_read(dev, PFUZE100_VGEN5VOL);
	reg &= ~0x1F;
	reg |= 0x1F;
	pmic_reg_write(dev, PFUZE100_VGEN5VOL, reg);

	return 0;
}

#ifdef CONFIG_LDO_BYPASS_CHECK
void ldo_mode_set(int ldo_bypass)
{
	struct udevice *dev;
	int ret;
	int is_400M;
	u32 vddarm;

	ret = pmic_get("pfuze100", &dev);
	if (ret == -ENODEV) {
		printf("No PMIC found!\n");
		return;
	}

	/* switch to ldo_bypass mode */
	if (ldo_bypass) {
		/* decrease VDDARM to 1.275V */
		pmic_clrsetbits(dev, PFUZE100_SW1ABVOL, 0x3f, PFUZE100_SW1ABC_SETP(12750));

		is_400M = set_anatop_bypass(1);
		if (is_400M)
			vddarm = PFUZE100_SW1ABC_SETP(10750);
		else
			vddarm = PFUZE100_SW1ABC_SETP(11750);

		pmic_clrsetbits(dev, PFUZE100_SW1ABVOL, 0x3f, vddarm);

		set_anatop_bypass(1);

		printf("switch to ldo_bypass mode!\n");
	}
}
#endif

#endif

int board_early_init_f(void)
{
	setup_iomux_uart();

	return 0;
}

int board_init(void)
{
	/* Address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

#ifdef CONFIG_SYS_I2C
	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info1);
#endif

#ifdef CONFIG_FEC_MXC
	setup_fec(CONFIG_FEC_ENET_DEV);
#endif

#ifdef CONFIG_MXC_SPI
#ifndef CONFIG_DM_SPI
	setup_spinor();
#endif
#endif

#ifdef CONFIG_NAND_MXS
	setup_gpmi_nand();
#endif

#ifdef CONFIG_FSL_QSPI
	board_qspi_init();
#endif

#ifdef	CONFIG_MXC_EPDC
	enable_epdc_clock();
	setup_epdc();
#endif

	return 0;
}

#ifdef CONFIG_CMD_BMODE
static const struct boot_mode board_boot_modes[] = {
	/* 4 bit bus width */
	{"sd1", MAKE_CFGVAL(0x42, 0x20, 0x00, 0x00)},
	{"qspi1", MAKE_CFGVAL(0x10, 0x00, 0x00, 0x00)},
	{NULL,	 0},
};
#endif

int board_late_init(void)
{
#ifdef CONFIG_CMD_BMODE
	add_board_boot_modes(board_boot_modes);
#endif

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

	return 0;
}

u32 get_board_rev(void)
{
	return get_cpu_rev();
}

int checkboard(void)
{
	puts("Board: MX6ULL 14X14 DDR3 Validation\n");

	return 0;
}

#ifdef CONFIG_USB_EHCI_MX6
#ifndef CONFIG_DM_USB

#define USB_OTHERREGS_OFFSET	0x800
#define UCTRL_PWR_POL		(1 << 9)
iomux_v3_cfg_t const usb_otg1_pads[] = {
	MX6_PAD_GPIO1_IO04__USB_OTG1_PWR | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX6_PAD_GPIO1_IO00__ANATOP_OTG1_ID | MUX_PAD_CTRL(OTG_ID_PAD_CTRL),
};

/*
 * Leave it here, but default configuration only supports 1 port now,
 * because we need sd1 and i2c1
 */
iomux_v3_cfg_t const usb_otg2_pads[] = {
	/* conflict with i2c1_scl */
	MX6_PAD_GPIO1_IO02__USB_OTG2_PWR | MUX_PAD_CTRL(NO_PAD_CTRL),
	/* conflict with sd1_vselect */
	MX6_PAD_GPIO1_IO05__ANATOP_OTG2_ID | MUX_PAD_CTRL(OTG_ID_PAD_CTRL),
};

int board_usb_phy_mode(int port)
{
	return usb_phy_mode(port);
}

int board_ehci_hcd_init(int port)
{
	u32 *usbnc_usb_ctrl;

	if (port > 1)
		return -EINVAL;

	switch (port) {
	case 0:
		SETUP_IOMUX_PADS(usb_otg1_pads);
		break;
	case 1:
		SETUP_IOMUX_PADS(usb_otg2_pads);
		break;
	default:
		printf("MXC USB port %d not yet supported\n", port);
		return 1;
	}

	usbnc_usb_ctrl = (u32 *)(USB_BASE_ADDR + USB_OTHERREGS_OFFSET +
				 port * 4);

	/* Set Power polarity */
	setbits_le32(usbnc_usb_ctrl, UCTRL_PWR_POL);

	return 0;
}
#endif
#endif
