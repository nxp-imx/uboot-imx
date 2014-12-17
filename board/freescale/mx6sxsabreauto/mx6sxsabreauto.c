/*
 * Copyright (C) 2014 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/arch/clock.h>
#include <asm/arch/iomux.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/mx6-pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/gpio.h>
#include <asm/imx-common/iomux-v3.h>
#include <asm/imx-common/boot_mode.h>
#include <asm/io.h>
#include <linux/sizes.h>
#include <common.h>
#include <fsl_esdhc.h>
#include <mmc.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/arch/crm_regs.h>
#ifdef CONFIG_SYS_I2C_MXC
#include <i2c.h>
#include <asm/imx-common/mxc_i2c.h>
#endif
#ifdef CONFIG_MXC_RDC
#include <asm/imx-common/rdc-sema.h>
#include <asm/arch/imx-rdc.h>
#endif

#ifdef CONFIG_VIDEO_MXS
#include <linux/fb.h>
#include <mxsfb.h>
#endif

#ifdef CONFIG_MAX7310_IOEXP
#include <gpio_exp.h>
#endif

#ifdef CONFIG_FASTBOOT
#include <fastboot.h>
#ifdef CONFIG_ANDROID_RECOVERY
#include <recovery.h>
#endif
#endif /*CONFIG_FASTBOOT*/

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_22K_UP  | PAD_CTL_SPEED_LOW |		\
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
	PAD_CTL_ODE | PAD_CTL_SRE_FAST)

#define LCD_PAD_CTRL    (PAD_CTL_HYS | PAD_CTL_PUS_100K_UP | PAD_CTL_PUE | \
	PAD_CTL_PKE | PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm)

#define BUTTON_PAD_CTRL    (PAD_CTL_PKE | PAD_CTL_PUE | \
	PAD_CTL_PUS_22K_UP | PAD_CTL_DSE_40ohm)

#define GPMI_PAD_CTRL0 (PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP)
#define GPMI_PAD_CTRL1 (PAD_CTL_DSE_40ohm | PAD_CTL_SPEED_MED | \
			PAD_CTL_SRE_FAST)
#define GPMI_PAD_CTRL2 (GPMI_PAD_CTRL0 | GPMI_PAD_CTRL1)


#ifdef CONFIG_SYS_I2C_MXC
#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)
/* I2C2 for PMIC */
struct i2c_pads_info i2c_pad_info2 = {
	.scl = {
		.i2c_mode = MX6SX_PAD_GPIO1_IO02__I2C2_SCL | PC,
		.gpio_mode = MX6SX_PAD_GPIO1_IO02__GPIO1_IO_2 | PC,
		.gp = IMX_GPIO_NR(1, 2),
	},
	.sda = {
		.i2c_mode = MX6SX_PAD_GPIO1_IO03__I2C2_SDA | PC,
		.gpio_mode = MX6SX_PAD_GPIO1_IO03__GPIO1_IO_3 | PC,
		.gp = IMX_GPIO_NR(1, 3),
	},
};

/* I2C3 */
struct i2c_pads_info i2c_pad_info3 = {
	.scl = {
		.i2c_mode = MX6SX_PAD_KEY_COL4__I2C3_SCL | PC,
		.gpio_mode = MX6SX_PAD_KEY_COL4__GPIO2_IO_14 | PC,
		.gp = IMX_GPIO_NR(2, 14),
	},
	.sda = {
		.i2c_mode = MX6SX_PAD_KEY_ROW4__I2C3_SDA | PC,
		.gpio_mode = MX6SX_PAD_KEY_ROW4__GPIO2_IO_19 | PC,
		.gp = IMX_GPIO_NR(2, 19),
	},
};
#endif

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_SIZE;

	return 0;
}

static iomux_v3_cfg_t const uart1_pads[] = {
	MX6SX_PAD_GPIO1_IO04__UART1_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX6SX_PAD_GPIO1_IO05__UART1_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const usdhc3_pads[] = {
	MX6SX_PAD_SD3_CLK__USDHC3_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6SX_PAD_SD3_CMD__USDHC3_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6SX_PAD_SD3_DATA0__USDHC3_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6SX_PAD_SD3_DATA1__USDHC3_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6SX_PAD_SD3_DATA2__USDHC3_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6SX_PAD_SD3_DATA3__USDHC3_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6SX_PAD_SD3_DATA4__USDHC3_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6SX_PAD_SD3_DATA5__USDHC3_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6SX_PAD_SD3_DATA6__USDHC3_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6SX_PAD_SD3_DATA7__USDHC3_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),

	/* CD pin */
	MX6SX_PAD_USB_H_DATA__GPIO7_IO_10 | MUX_PAD_CTRL(NO_PAD_CTRL),

	/* RST_B, used for power reset cycle */
	MX6SX_PAD_KEY_COL1__GPIO2_IO_11 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static iomux_v3_cfg_t const usdhc4_pads[] = {
	MX6SX_PAD_SD4_CLK__USDHC4_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6SX_PAD_SD4_CMD__USDHC4_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6SX_PAD_SD4_DATA0__USDHC4_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6SX_PAD_SD4_DATA1__USDHC4_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6SX_PAD_SD4_DATA2__USDHC4_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6SX_PAD_SD4_DATA3__USDHC4_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6SX_PAD_SD4_DATA4__USDHC4_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6SX_PAD_SD4_DATA5__USDHC4_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6SX_PAD_SD4_DATA6__USDHC4_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6SX_PAD_SD4_DATA7__USDHC4_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),

	/* CD pin */
	MX6SX_PAD_USB_H_STROBE__GPIO7_IO_11 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

#ifdef CONFIG_FEC_MXC
static iomux_v3_cfg_t const fec1_pads[] = {
	MX6SX_PAD_ENET1_MDC__ENET1_MDC | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6SX_PAD_ENET1_MDIO__ENET1_MDIO | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6SX_PAD_RGMII1_RX_CTL__ENET1_RX_EN | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6SX_PAD_RGMII1_RD0__ENET1_RX_DATA_0 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6SX_PAD_RGMII1_RD1__ENET1_RX_DATA_1 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6SX_PAD_RGMII1_RD2__ENET1_RX_DATA_2 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6SX_PAD_RGMII1_RD3__ENET1_RX_DATA_3 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6SX_PAD_RGMII1_RXC__ENET1_RX_CLK | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6SX_PAD_RGMII1_TX_CTL__ENET1_TX_EN | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6SX_PAD_RGMII1_TD0__ENET1_TX_DATA_0 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6SX_PAD_RGMII1_TD1__ENET1_TX_DATA_1 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6SX_PAD_RGMII1_TD2__ENET1_TX_DATA_2 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6SX_PAD_RGMII1_TD3__ENET1_TX_DATA_3 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6SX_PAD_RGMII1_TXC__ENET1_RGMII_TXC | MUX_PAD_CTRL(ENET_PAD_CTRL),
};

static iomux_v3_cfg_t const fec2_pads[] = {
	MX6SX_PAD_ENET1_MDC__ENET2_MDC | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6SX_PAD_ENET1_MDIO__ENET2_MDIO | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6SX_PAD_RGMII2_RX_CTL__ENET2_RX_EN | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6SX_PAD_RGMII2_RD0__ENET2_RX_DATA_0 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6SX_PAD_RGMII2_RD1__ENET2_RX_DATA_1 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6SX_PAD_RGMII2_RD2__ENET2_RX_DATA_2 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6SX_PAD_RGMII2_RD3__ENET2_RX_DATA_3 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6SX_PAD_RGMII2_RXC__ENET2_RX_CLK | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6SX_PAD_RGMII2_TX_CTL__ENET2_TX_EN | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6SX_PAD_RGMII2_TD0__ENET2_TX_DATA_0 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6SX_PAD_RGMII2_TD1__ENET2_TX_DATA_1 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6SX_PAD_RGMII2_TD2__ENET2_TX_DATA_2 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6SX_PAD_RGMII2_TD3__ENET2_TX_DATA_3 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6SX_PAD_RGMII2_TXC__ENET2_RGMII_TXC | MUX_PAD_CTRL(ENET_PAD_CTRL),
};

static void setup_iomux_fec(int fec_id)
{
	if (0 == fec_id)
		imx_iomux_v3_setup_multiple_pads(fec1_pads, ARRAY_SIZE(fec1_pads));
	else
		imx_iomux_v3_setup_multiple_pads(fec2_pads, ARRAY_SIZE(fec2_pads));
}
#endif

#ifdef CONFIG_MAX7310_IOEXP

#define CPU_PER_RST_B	IOEXP_GPIO_NR(1, 4)
#define LVDS_EN_PIN		IOEXP_GPIO_NR(1, 7)
#define STEER_ENET		IOEXP_GPIO_NR(2, 2)

int setup_max7310(void)
{
	/* Must call this function after i2c has setup */
#ifdef CONFIG_SYS_I2C_MXC
	gpio_exp_setup_port(1, 2, 0x30);
	gpio_exp_setup_port(2, 2, 0x32);

	return 0;
#else
	return -EPERM;
#endif
}
#endif

static void setup_iomux_uart(void)
{
	imx_iomux_v3_setup_multiple_pads(uart1_pads, ARRAY_SIZE(uart1_pads));
}

#ifdef CONFIG_QSPI

#define QSPI_PAD_CTRL1	\
		(PAD_CTL_SRE_FAST | PAD_CTL_SPEED_MED | \
		 PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_47K_UP | PAD_CTL_DSE_60ohm)

static iomux_v3_cfg_t const quadspi_pads[] = {
	MX6SX_PAD_QSPI1A_SS0_B__QSPI1_A_SS0_B   | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6SX_PAD_QSPI1A_SCLK__QSPI1_A_SCLK     | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6SX_PAD_QSPI1A_DATA0__QSPI1_A_DATA_0  | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6SX_PAD_QSPI1A_DATA1__QSPI1_A_DATA_1	| MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6SX_PAD_QSPI1A_DATA2__QSPI1_A_DATA_2  | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6SX_PAD_QSPI1A_DATA3__QSPI1_A_DATA_3  | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6SX_PAD_QSPI1B_SS0_B__QSPI1_B_SS0_B   | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6SX_PAD_QSPI1B_SCLK__QSPI1_B_SCLK     | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6SX_PAD_QSPI1B_DATA0__QSPI1_B_DATA_0  | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6SX_PAD_QSPI1B_DATA1__QSPI1_B_DATA_1  | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6SX_PAD_QSPI1B_DATA2__QSPI1_B_DATA_2  | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
	MX6SX_PAD_QSPI1B_DATA3__QSPI1_B_DATA_3  | MUX_PAD_CTRL(QSPI_PAD_CTRL1),
};

int board_qspi_init(void)
{
	/* Set the iomux */
	imx_iomux_v3_setup_multiple_pads(quadspi_pads, ARRAY_SIZE(quadspi_pads));

	/* Set the clock */
	enable_qspi_clk(0);

	return 0;
}
#endif

#ifdef CONFIG_SYS_USE_NAND
iomux_v3_cfg_t gpmi_pads[] = {
	MX6SX_PAD_NAND_CLE__RAWNAND_CLE		| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6SX_PAD_NAND_ALE__RAWNAND_ALE		| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6SX_PAD_NAND_WP_B__RAWNAND_WP_B	| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6SX_PAD_NAND_READY_B__RAWNAND_READY_B	| MUX_PAD_CTRL(GPMI_PAD_CTRL0),
	MX6SX_PAD_NAND_CE0_B__RAWNAND_CE0_B		| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6SX_PAD_NAND_RE_B__RAWNAND_RE_B		| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6SX_PAD_NAND_WE_B__RAWNAND_WE_B		| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6SX_PAD_NAND_DATA00__RAWNAND_DATA00	| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6SX_PAD_NAND_DATA01__RAWNAND_DATA01	| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6SX_PAD_NAND_DATA02__RAWNAND_DATA02	| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6SX_PAD_NAND_DATA03__RAWNAND_DATA03	| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6SX_PAD_NAND_DATA04__RAWNAND_DATA04	| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6SX_PAD_NAND_DATA05__RAWNAND_DATA05	| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6SX_PAD_NAND_DATA06__RAWNAND_DATA06	| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6SX_PAD_NAND_DATA07__RAWNAND_DATA07	| MUX_PAD_CTRL(GPMI_PAD_CTRL2),
};

static void setup_gpmi_nand(void)
{
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;

	/* config gpmi nand iomux */
	imx_iomux_v3_setup_multiple_pads(gpmi_pads, ARRAY_SIZE(gpmi_pads));

	/* Disable the QSPI2 root clock */
	clrbits_le32(&mxc_ccm->CCGR4, MXC_CCM_CCGR4_QSPI2_ENFC_MASK
				| MXC_CCM_CCGR4_RAWNAND_U_GPMI_BCH_INPUT_GPMI_IO_MASK);

	/* config gpmi and bch clock to 100 MHz */
	clrsetbits_le32(&mxc_ccm->cs2cdr,
			MXC_CCM_CS2CDR_QSPI2_CLK_PODF_MASK |
			MXC_CCM_CS2CDR_QSPI2_CLK_PRED_MASK |
			MXC_CCM_CS2CDR_QSPI2_CLK_SEL_MASK,
			MXC_CCM_CS2CDR_QSPI2_CLK_PODF(0) |
			MXC_CCM_CS2CDR_QSPI2_CLK_PRED(3) |
			MXC_CCM_CS2CDR_QSPI2_CLK_SEL(3));

	/* enable gpmi and bch clock gating */
	setbits_le32(&mxc_ccm->CCGR4,
		     MXC_CCM_CCGR4_RAWNAND_U_BCH_INPUT_APB_MASK |
		     MXC_CCM_CCGR4_RAWNAND_U_GPMI_BCH_INPUT_BCH_MASK |
		     MXC_CCM_CCGR4_RAWNAND_U_GPMI_BCH_INPUT_GPMI_IO_MASK |
		     MXC_CCM_CCGR4_RAWNAND_U_GPMI_INPUT_APB_MASK |
		     MXC_CCM_CCGR4_PL301_MX6QPER1_BCH_MASK |
		     MXC_CCM_CCGR4_QSPI2_ENFC_MASK);

	/* enable apbh clock gating */
	setbits_le32(&mxc_ccm->CCGR0, MXC_CCM_CCGR0_APBHDMA_MASK);
}
#endif


#ifdef CONFIG_FSL_ESDHC
static struct fsl_esdhc_cfg usdhc_cfg[3] = {
	{USDHC3_BASE_ADDR},
	{USDHC4_BASE_ADDR},
};

#define USDHC3_CD_GPIO	IMX_GPIO_NR(7, 10)
#define USDHC3_RST_GPIO	IMX_GPIO_NR(2, 11)
#define USDHC4_CD_GPIO	IMX_GPIO_NR(7, 11)

int mmc_get_env_devno(void)
{
	u32 soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
	int dev_no;
	u32 bootsel;

	bootsel = (soc_sbmr & 0x000000FF) >> 6 ;

	/* If not boot from sd/mmc, use default value */
	if (bootsel != 1)
		return CONFIG_SYS_MMC_ENV_DEV;

	/* BOOT_CFG2[3] and BOOT_CFG2[4] */
	dev_no = (soc_sbmr & 0x00001800) >> 11;

	/* need ubstract 1 to map to the mmc device id
	 * see the comments in board_mmc_init function
	 */

	dev_no -= 2;

	return dev_no;
}

int mmc_map_to_kernel_blk(int dev_no)
{
	return dev_no + 2;
}

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;

	switch (cfg->esdhc_base) {
	case USDHC3_BASE_ADDR:
		ret = !gpio_get_value(USDHC3_CD_GPIO);
		break;
	case USDHC4_BASE_ADDR:
		ret = !gpio_get_value(USDHC4_CD_GPIO);
		break;
	}

	return ret;

}

int board_mmc_init(bd_t *bis)
{
	int i;

	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-boot device node)    (Physical Port)
	 * mmc0                    USDHC3
	 * mmc1                    USDHC4
	 */
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			imx_iomux_v3_setup_multiple_pads(
				usdhc3_pads, ARRAY_SIZE(usdhc3_pads));
			gpio_direction_input(USDHC3_CD_GPIO);

			/* Need to set steer to B0 to A*/
			gpio_direction_output(USDHC3_RST_GPIO, 1);
			usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
			break;
		case 1:
			imx_iomux_v3_setup_multiple_pads(
				usdhc4_pads, ARRAY_SIZE(usdhc4_pads));
			gpio_direction_input(USDHC4_CD_GPIO);
			usdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC4_CLK);
			break;
		default:
			printf("Warning: you configured more USDHC controllers"
				"(%d) than supported by the board\n", i + 1);
			return 0;
			}

			if (fsl_esdhc_initialize(bis, &usdhc_cfg[i]))
				printf("Warning: failed to initialize mmc dev %d\n", i);
	}

	return 0;
}

int check_mmc_autodetect(void)
{
	char *autodetect_str = getenv("mmcautodetect");

	if ((autodetect_str != NULL) &&
		(strcmp(autodetect_str, "yes") == 0)) {
		return 1;
	}

	return 0;
}

void board_late_mmc_init(void)
{
	char cmd[32];
	char mmcblk[32];
	u32 dev_no = mmc_get_env_devno();

	if (!check_mmc_autodetect())
		return;

	setenv_ulong("mmcdev", dev_no);

	/* Set mmcblk env */
	sprintf(mmcblk, "/dev/mmcblk%dp2 rootwait rw",
		mmc_map_to_kernel_blk(dev_no));
	setenv("mmcroot", mmcblk);

	sprintf(cmd, "mmc dev %d", dev_no);
	run_command(cmd, 0);
}

#endif

#ifdef CONFIG_VIDEO_MXS
static iomux_v3_cfg_t const lvds_ctrl_pads[] = {
	/* Use GPIO for Brightness adjustment, duty cycle = period */
	MX6SX_PAD_SD1_DATA1__GPIO6_IO_3 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static iomux_v3_cfg_t const lcd_pads[] = {
	MX6SX_PAD_LCD1_CLK__LCDIF1_CLK | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6SX_PAD_LCD1_ENABLE__LCDIF1_ENABLE | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6SX_PAD_LCD1_HSYNC__LCDIF1_HSYNC | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6SX_PAD_LCD1_VSYNC__LCDIF1_VSYNC | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6SX_PAD_LCD1_DATA00__LCDIF1_DATA_0 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6SX_PAD_LCD1_DATA01__LCDIF1_DATA_1 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6SX_PAD_LCD1_DATA02__LCDIF1_DATA_2 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6SX_PAD_LCD1_DATA03__LCDIF1_DATA_3 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6SX_PAD_LCD1_DATA04__LCDIF1_DATA_4 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6SX_PAD_LCD1_DATA05__LCDIF1_DATA_5 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6SX_PAD_LCD1_DATA06__LCDIF1_DATA_6 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6SX_PAD_LCD1_DATA07__LCDIF1_DATA_7 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6SX_PAD_LCD1_DATA08__LCDIF1_DATA_8 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6SX_PAD_LCD1_DATA09__LCDIF1_DATA_9 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6SX_PAD_LCD1_DATA10__LCDIF1_DATA_10 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6SX_PAD_LCD1_DATA11__LCDIF1_DATA_11 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6SX_PAD_LCD1_DATA12__LCDIF1_DATA_12 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6SX_PAD_LCD1_DATA13__LCDIF1_DATA_13 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6SX_PAD_LCD1_DATA14__LCDIF1_DATA_14 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6SX_PAD_LCD1_DATA15__LCDIF1_DATA_15 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6SX_PAD_LCD1_DATA16__LCDIF1_DATA_16 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6SX_PAD_LCD1_DATA17__LCDIF1_DATA_17 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6SX_PAD_LCD1_RESET__GPIO3_IO_27 | MUX_PAD_CTRL(NO_PAD_CTRL),
};


struct lcd_panel_info_t {
	unsigned int lcdif_base_addr;
	int depth;
	void	(*enable)(struct lcd_panel_info_t const *dev);
	struct fb_videomode mode;
};

void do_enable_lvds(struct lcd_panel_info_t const *dev)
{
	enable_lcdif_clock(dev->lcdif_base_addr);
	enable_lvds(dev->lcdif_base_addr);

	imx_iomux_v3_setup_multiple_pads(lvds_ctrl_pads,
							ARRAY_SIZE(lvds_ctrl_pads));

#ifdef CONFIG_MAX7310_IOEXP
	/* LVDS Enable pin */
	gpio_exp_direction_output(LVDS_EN_PIN , 1);
#endif

	/* Set Brightness to high */
	gpio_direction_output(IMX_GPIO_NR(6, 3) , 1);
}

void do_enable_parallel_lcd(struct lcd_panel_info_t const *dev)
{
	enable_lcdif_clock(dev->lcdif_base_addr);

	imx_iomux_v3_setup_multiple_pads(lcd_pads, ARRAY_SIZE(lcd_pads));

	/* Power up the LCD */
	gpio_direction_output(IMX_GPIO_NR(3, 27) , 1);
}

static struct lcd_panel_info_t const displays[] = {{
	.lcdif_base_addr = LCDIF2_BASE_ADDR,
	.depth = 18,
	.enable	= do_enable_lvds,
	.mode	= {
		.name			= "Hannstar-XGA",
		.xres           = 1024,
		.yres           = 768,
		.pixclock       = 15385,
		.left_margin    = 220,
		.right_margin   = 40,
		.upper_margin   = 21,
		.lower_margin   = 7,
		.hsync_len      = 60,
		.vsync_len      = 10,
		.sync           = 0,
		.vmode          = FB_VMODE_NONINTERLACED
} }, {
	.lcdif_base_addr = LCDIF1_BASE_ADDR,
	.depth = 18,
	.enable	= do_enable_parallel_lcd,
	.mode	= {
		.name			= "Boundary-LCD",
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

int board_video_skip(void)
{
	int i;
	int ret;
	char const *panel = getenv("panel");
	if (!panel) {
		panel = displays[0].mode.name;
		printf("No panel detected: default to %s\n", panel);
		i = 0;
	} else {
		for (i = 0; i < ARRAY_SIZE(displays); i++) {
			if (!strcmp(panel, displays[i].mode.name))
				break;
		}
	}
	if (i < ARRAY_SIZE(displays)) {
		ret = mxs_lcd_panel_setup(displays[i].mode, displays[i].depth,
				    displays[i].lcdif_base_addr);
		if (!ret) {
			if (displays[i].enable)
				displays[i].enable(displays+i);
			printf("Display: %s (%ux%u)\n",
			       displays[i].mode.name,
			       displays[i].mode.xres,
			       displays[i].mode.yres);
		} else
			printf("LCD %s cannot be configured: %d\n",
			       displays[i].mode.name, ret);
	} else {
		printf("unsupported panel %s\n", panel);
		return -EINVAL;
	}

	return 0;
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
		= (struct iomuxc_gpr_base_regs *) IOMUXC_GPR_BASE_ADDR;
	int ret;

	if (0 == fec_id)
		/* Use 125M anatop REF_CLK1 for ENET1, clear gpr1[13], gpr1[17]*/
		clrsetbits_le32(&iomuxc_gpr_regs->gpr[1], IOMUX_GPR1_FEC1_MASK, 0);
	else
		/* Use 125M anatop REF_CLK1 for ENET2, clear gpr1[14], gpr1[18]*/
		clrsetbits_le32(&iomuxc_gpr_regs->gpr[1], IOMUX_GPR1_FEC2_MASK, 0);

	ret = enable_fec_anatop_clock(fec_id, ENET_125MHz);
	if (ret)
		return ret;

	enable_enet_clock();

	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
	/* Enable 1.8V(SEL_1P5_1P8_POS_REG) on Phy control debug reg 0 */
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

#ifdef CONFIG_PFUZE100_PMIC_I2C
#define PFUZE100_DEVICEID	0x0
#define PFUZE100_REVID		0x3
#define PFUZE100_FABID		0x4

#define PFUZE100_SW1ABVOL	0x20
#define PFUZE100_SW1ABSTBY	0x21
#define PFUZE100_SW1ABCONF	0x24
#define PFUZE100_SW1CVOL	0x2e
#define PFUZE100_SW1CSTBY	0x2f
#define PFUZE100_SW1CCONF	0x32
#define PFUZE100_SW1ABC_SETP(x)	((x - 3000) / 250)
#define PFUZE100_VGEN5CTL	0x70

/* set all switches APS in normal and PFM mode in standby */
static int setup_pmic_mode(int chip)
{
	unsigned char offset, i, switch_num, value;

	if (!chip) {
		/* pfuze100 */
		switch_num = 6;
		offset = 0x31;
	} else {
		/* pfuze200 */
		switch_num = 4;
		offset = 0x38;
	}

	value = 0xc;
	if (i2c_write(0x8, 0x23, 1, &value, 1)) {
		printf("Set SW1AB mode error!\n");
		return -1;
	}

	for (i = 0; i < switch_num - 1; i++) {
		if (i2c_write(0x8, offset + i * 7, 1, &value, 1)) {
			printf("Set switch%x mode error!\n", offset);
			return -1;
		}
	}

	return 0;
}

static int setup_pmic_voltages(void)
{
	unsigned char value, rev_id = 0;

	i2c_set_bus_num(CONFIG_PMIC_I2C_BUS);

	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_PMIC_I2C_SLAVE);
	if (!i2c_probe(CONFIG_PMIC_I2C_SLAVE)) {
		if (i2c_read(CONFIG_PMIC_I2C_SLAVE, PFUZE100_DEVICEID, 1, &value, 1)) {
			printf("Read device ID error!\n");
			return -1;
		}
		if (i2c_read(CONFIG_PMIC_I2C_SLAVE, PFUZE100_REVID, 1, &rev_id, 1)) {
			printf("Read Rev ID error!\n");
			return -1;
		}
		printf("Found PFUZE100! deviceid 0x%x, revid 0x%x\n", value, rev_id);

		if (setup_pmic_mode(value & 0xf)) {
			printf("setup pmic mode error!\n");
			return -1;
		}
		/* set SW1AB standby volatage 0.975V */
		if (i2c_read(CONFIG_PMIC_I2C_SLAVE, PFUZE100_SW1ABSTBY, 1, &value, 1)) {
			printf("Read SW1ABSTBY error!\n");
			return -1;
		}
		value &= ~0x3f;
		value |= PFUZE100_SW1ABC_SETP(9750);
		if (i2c_write(CONFIG_PMIC_I2C_SLAVE, PFUZE100_SW1ABSTBY, 1, &value, 1)) {
			printf("Set SW1ABSTBY error!\n");
			return -1;
		}

		/* set SW1AB/VDDARM step ramp up time from 16us to 4us/25mV */
		if (i2c_read(CONFIG_PMIC_I2C_SLAVE, PFUZE100_SW1ABCONF, 1, &value, 1)) {
			printf("Read SW1ABCONFIG error!\n");
			return -1;
		}
		value &= ~0xc0;
		value |= 0x40;
		if (i2c_write(CONFIG_PMIC_I2C_SLAVE, PFUZE100_SW1ABCONF, 1, &value, 1)) {
			printf("Set SW1ABCONFIG error!\n");
			return -1;
		}

		/* set SW1C standby volatage 0.975V */
		if (i2c_read(CONFIG_PMIC_I2C_SLAVE, PFUZE100_SW1CSTBY, 1, &value, 1)) {
			printf("Read SW1CSTBY error!\n");
			return -1;
		}
		value &= ~0x3f;
		value |= PFUZE100_SW1ABC_SETP(9750);
		if (i2c_write(CONFIG_PMIC_I2C_SLAVE, PFUZE100_SW1CSTBY, 1, &value, 1)) {
			printf("Set SW1CSTBY error!\n");
			return -1;
		}

		/* set SW1C/VDDSOC step ramp up time to from 16us to 4us/25mV */
		if (i2c_read(CONFIG_PMIC_I2C_SLAVE, PFUZE100_SW1CCONF, 1, &value, 1)) {
			printf("Read SW1CCONFIG error!\n");
			return -1;
		}
		value &= ~0xc0;
		value |= 0x40;
		if (i2c_write(CONFIG_PMIC_I2C_SLAVE, PFUZE100_SW1CCONF, 1, &value, 1)) {
			printf("Set SW1CCONFIG error!\n");
			return -1;
		}

		/* Enable power of VGEN5 3V3, needed for SD3 */
		if (i2c_read(CONFIG_PMIC_I2C_SLAVE, PFUZE100_VGEN5CTL, 1, &value, 1)) {
			printf("Read VGEN5CTL error!\n");
			return -1;
		}
		value &= ~0x1F;
		value |= 0x1F;
		if (i2c_write(CONFIG_PMIC_I2C_SLAVE, PFUZE100_VGEN5CTL, 1, &value, 1)) {
			printf("Set VGEN5CTL error!\n");
			return -1;
		}
	}

	return 0;
}

#ifdef CONFIG_LDO_BYPASS_CHECK
void ldo_mode_set(int ldo_bypass)
{
	unsigned char value;
	/* switch to ldo_bypass mode */
	if (ldo_bypass) {
		/* decrease VDDARM to 1.15V */
		if (i2c_read(CONFIG_PMIC_I2C_SLAVE, PFUZE100_SW1ABVOL, 1, &value, 1)) {
			printf("Read SW1AB error!\n");
			return;
		}
		value &= ~0x3f;
		value |= PFUZE100_SW1ABC_SETP(11500);
		if (i2c_write(CONFIG_PMIC_I2C_SLAVE, PFUZE100_SW1ABVOL, 1, &value, 1)) {
			printf("Set SW1AB error!\n");
			return;
		}
		/* increase VDDSOC to 1.15V */
		if (i2c_read(CONFIG_PMIC_I2C_SLAVE, PFUZE100_SW1CVOL, 1, &value, 1)) {
			printf("Read SW1C error!\n");
			return;
		}
		value &= ~0x3f;
		value |= PFUZE100_SW1ABC_SETP(11500);
		if (i2c_write(CONFIG_PMIC_I2C_SLAVE, PFUZE100_SW1CVOL, 1, &value, 1)) {
			printf("Set SW1C error!\n");
			return;
		}

		set_anatop_bypass(1);
		printf("switch to ldo_bypass mode!\n");
	}

}
#endif
#endif

#ifdef CONFIG_MXC_RDC
static rdc_peri_cfg_t const shared_resources[] = {
	(RDC_PER_GPIO1 | RDC_DOMAIN(0) | RDC_DOMAIN(1)),
};
#endif

int board_early_init_f(void)
{
#ifdef CONFIG_MXC_RDC
	imx_rdc_setup_peripherals(shared_resources, ARRAY_SIZE(shared_resources));
#endif

#ifdef CONFIG_SYS_AUXCORE_FASTUP
	arch_auxiliary_core_up(0, CONFIG_SYS_AUXCORE_BOOTDATA);
#endif

	setup_iomux_uart();
	return 0;
}

int board_init(void)
{
	/* Address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

#ifdef CONFIG_SYS_I2C_MXC
	setup_i2c(1, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info2);
	setup_i2c(2, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info3);
#endif

#ifdef CONFIG_MAX7310_IOEXP
	setup_max7310();

	/* Reset CPU_PER_RST_B signal for enet phy and PCIE */
	gpio_exp_direction_output(CPU_PER_RST_B, 0);
	udelay(500);
	gpio_exp_direction_output(CPU_PER_RST_B, 1);

	/* Set steering signal to L for selecting B0 */
	gpio_exp_direction_output(STEER_ENET, 0);
#endif

#ifdef	CONFIG_FEC_MXC
	setup_fec(CONFIG_FEC_ENET_DEV);
#endif

#ifdef CONFIG_SYS_USE_NAND
	setup_gpmi_nand();
#endif

#ifdef CONFIG_QSPI
	board_qspi_init();
#endif

	return 0;
}

#ifdef CONFIG_CMD_BMODE
static const struct boot_mode board_boot_modes[] = {
	{"sda", MAKE_CFGVAL(0x42, 0x30, 0x00, 0x00)},
	{"sdb", MAKE_CFGVAL(0x40, 0x38, 0x00, 0x00)},
	{"qspi1", MAKE_CFGVAL(0x10, 0x00, 0x00, 0x00)},
	{"nand", MAKE_CFGVAL(0x82, 0x00, 0x00, 0x00)},
	{NULL,	 0},
};
#endif

int board_late_init(void)
{
#ifdef CONFIG_CMD_BMODE
	add_board_boot_modes(board_boot_modes);
#endif

#ifdef CONFIG_PFUZE100_PMIC_I2C
	int ret = 0;

	ret = setup_pmic_voltages();
	if (ret)
		return -1;
#endif

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_init();
#endif
	/* set WDOG_B to reset whole system */
	set_wdog_reset((struct wdog_regs *)WDOG1_BASE_ADDR);

	return 0;
}

u32 get_board_rev(void)
{
	return get_cpu_rev();
}

int checkboard(void)
{
	puts("Board: MX6SX SABRE AUTO\n");

	return 0;
}

#ifdef CONFIG_USB_EHCI_MX6
iomux_v3_cfg_t const usb_otg1_pads[] = {
	MX6SX_PAD_GPIO1_IO09__USB_OTG1_PWR | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX6SX_PAD_GPIO1_IO10__ANATOP_OTG1_ID | MUX_PAD_CTRL(NO_PAD_CTRL)
};

iomux_v3_cfg_t const usb_host2_pads[] = {
	MX6SX_PAD_GPIO1_IO12__USB_OTG2_PWR | MUX_PAD_CTRL(NO_PAD_CTRL),
};

int board_ehci_hcd_init(int port)
{
	switch (port) {
	case 0:
		imx_iomux_v3_setup_multiple_pads(usb_otg1_pads,
			ARRAY_SIZE(usb_otg1_pads));
		break;
	case 1:
		imx_iomux_v3_setup_multiple_pads(usb_host2_pads,
			ARRAY_SIZE(usb_host2_pads));
		break;
	default:
		printf("MXC USB port %d not yet supported\n", port);
		return 1;
	}
	return 0;
}
#endif

#ifdef CONFIG_FASTBOOT

void board_fastboot_setup(void)
{
	switch (get_boot_device()) {
#if defined(CONFIG_FASTBOOT_STORAGE_MMC)
	case SD3_BOOT:
	case MMC3_BOOT:
		if (!getenv("fastboot_dev"))
			setenv("fastboot_dev", "mmc0");
		if (!getenv("bootcmd"))
			setenv("bootcmd", "booti mmc0");
		break;
	case SD4_BOOT:
	case MMC4_BOOT:
		if (!getenv("fastboot_dev"))
			setenv("fastboot_dev", "mmc1");
		if (!getenv("bootcmd"))
			setenv("bootcmd", "booti mmc1");
		break;
#endif /*CONFIG_FASTBOOT_STORAGE_MMC*/
#if defined(CONFIG_FASTBOOT_STORAGE_NAND)
	case NAND_BOOT:
		if (!getenv("fastboot_dev"))
			setenv("fastboot_dev", "nand");
		if (!getenv("fbparts"))
			setenv("fbparts", ANDROID_FASTBOOT_NAND_PARTS);
		if (!getenv("bootcmd"))
			setenv("bootcmd",
				"nand read ${loadaddr} ${boot_nand_offset} "
				"${boot_nand_size};booti ${loadaddr}");
		break;
#endif /*CONFIG_FASTBOOT_STORAGE_NAND*/

	default:
		printf("unsupported boot devices\n");
		break;
	}
}

#ifdef CONFIG_ANDROID_RECOVERY
int check_recovery_cmd_file(void)
{
	int recovery_mode = 0;

	recovery_mode = recovery_check_and_clean_flag();

	return recovery_mode;
}

void board_recovery_setup(void)
{
	int bootdev = get_boot_device();

	switch (bootdev) {
#if defined(CONFIG_FASTBOOT_STORAGE_MMC)
	case SD3_BOOT:
	case MMC3_BOOT:
		if (!getenv("bootcmd_android_recovery"))
			setenv("bootcmd_android_recovery", "booti mmc0 recovery");
		break;
	case SD4_BOOT:
	case MMC4_BOOT:
		if (!getenv("bootcmd_android_recovery"))
			setenv("bootcmd_android_recovery", "booti mmc1 recovery");
		break;
#endif /*CONFIG_FASTBOOT_STORAGE_MMC*/
#if defined(CONFIG_FASTBOOT_STORAGE_NAND)
	case NAND_BOOT:
		if (!getenv("bootcmd_android_recovery"))
			setenv("bootcmd_android_recovery",
				"nand read ${loadaddr} ${recovery_nand_offset} "
				"${recovery_nand_size};booti ${loadaddr}");
		break;
#endif /*CONFIG_FASTBOOT_STORAGE_NAND*/
	default:
		printf("Unsupported bootup device for recovery: dev: %d\n",
			bootdev);
		return;
	}

	printf("setup env for recovery..\n");
	setenv("bootcmd", "run bootcmd_android_recovery");
}
#endif /*CONFIG_ANDROID_RECOVERY*/

#endif /*CONFIG_FASTBOOT*/

#ifdef CONFIG_IMX_UDC
iomux_v3_cfg_t const otg_udc_pads[] = {
	(MX6SX_PAD_GPIO1_IO10__ANATOP_OTG1_ID | MUX_PAD_CTRL(NO_PAD_CTRL)),
};
void udc_pins_setting(void)
{
	imx_iomux_v3_setup_multiple_pads(otg_udc_pads,
		ARRAY_SIZE(otg_udc_pads));
}

#endif /*CONFIG_IMX_UDC*/
