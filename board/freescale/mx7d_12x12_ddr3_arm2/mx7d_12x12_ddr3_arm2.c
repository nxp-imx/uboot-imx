/*
 * Copyright (C) 2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/mx7-pins.h>
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
#include <power/pmic.h>
#include <power/pfuze300_pmic.h>
#include "../common/pfuze.h"
#ifdef CONFIG_SYS_I2C_MXC
#include <i2c.h>
#include <asm/imx-common/mxc_i2c.h>
#endif
#include <asm/arch/crm_regs.h>

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL  (PAD_CTL_DSE_3P3V_49OHM | \
	PAD_CTL_PUS_PU100KOHM | PAD_CTL_HYS)
#define USDHC_PAD_CTRL (PAD_CTL_DSE_3P3V_32OHM | PAD_CTL_SRE_SLOW | \
	PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PUS_PU47KOHM)
#define I2C_PAD_CTRL    (PAD_CTL_DSE_3P3V_32OHM | PAD_CTL_SRE_SLOW | \
	PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PUS_PU100KOHM)
#define SPI_PAD_CTRL (PAD_CTL_DSE_3P3V_49OHM | PAD_CTL_SRE_SLOW | PAD_CTL_HYS)

#ifdef CONFIG_SYS_I2C_MXC
#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)
/* I2C1 for PMIC */
struct i2c_pads_info i2c_pad_info1 = {
	.scl = {
		.i2c_mode = MX7D_PAD_GPIO1_IO04__I2C1_SCL | PC,
		.gpio_mode = MX7D_PAD_GPIO1_IO04__GPIO1_IO4 | PC,
		.gp = IMX_GPIO_NR(1, 4),
	},
	.sda = {
		.i2c_mode = MX7D_PAD_GPIO1_IO05__I2C1_SDA | PC,
		.gpio_mode = MX7D_PAD_GPIO1_IO05__GPIO1_IO5 | PC,
		.gp = IMX_GPIO_NR(1, 5),
	},
};

/* I2C2 */
struct i2c_pads_info i2c_pad_info2 = {
	.scl = {
		.i2c_mode = MX7D_PAD_GPIO1_IO06__I2C2_SCL | PC,
		.gpio_mode = MX7D_PAD_GPIO1_IO06__GPIO1_IO6 | PC,
		.gp = IMX_GPIO_NR(1, 6),
	},
	.sda = {
		.i2c_mode = MX7D_PAD_GPIO1_IO07__I2C2_SDA | PC,
		.gpio_mode = MX7D_PAD_GPIO1_IO07__GPIO1_IO7 | PC,
		.gp = IMX_GPIO_NR(1, 7),
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

static iomux_v3_cfg_t const usdhc2_emmc_pads[] = {
	MX7D_PAD_SD2_CLK__SD2_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD2_CMD__SD2_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD2_DATA0__SD2_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD2_DATA1__SD2_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD2_DATA2__SD2_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD2_DATA3__SD2_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_ECSPI1_SCLK__SD2_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_ECSPI1_MOSI__SD2_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_ECSPI1_MISO__SD2_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_ECSPI1_SS0__SD2_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),

	MX7D_PAD_SD2_RESET_B__GPIO5_IO11 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

static iomux_v3_cfg_t const usdhc3_pads[] = {
	MX7D_PAD_SD3_CLK__SD3_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_CMD__SD3_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA0__SD3_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA1__SD3_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA2__SD3_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA3__SD3_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),

	MX7D_PAD_GPIO1_IO14__GPIO1_IO14  | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_GPIO1_IO15__GPIO1_IO15  | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

static iomux_v3_cfg_t const wdog_pads[] = {
	MX7D_PAD_GPIO1_IO00__WDOG1_WDOG_B | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static void setup_iomux_uart(void)
{
	imx_iomux_v3_setup_multiple_pads(uart1_pads, ARRAY_SIZE(uart1_pads));
}

#ifdef CONFIG_FSL_ESDHC

#define USDHC3_CD_GPIO	IMX_GPIO_NR(1, 14)
#define USDHC2_PWR_GPIO	IMX_GPIO_NR(5, 11)

static struct fsl_esdhc_cfg usdhc_cfg[2] = {
	{USDHC2_BASE_ADDR},
	{USDHC3_BASE_ADDR, 0, 4},
};

int mmc_get_env_devno(void)
{
	struct bootrom_sw_info **p =
		(struct bootrom_sw_info **)ROM_SW_INFO_ADDR;

	u8 boot_type = (*p)->boot_dev_type;
	u8 dev_no = (*p)->boot_dev_instance;

	/* If not boot from sd/mmc, use default value */
	if ((boot_type != BOOT_TYPE_SD) && (boot_type != BOOT_TYPE_MMC))
		return CONFIG_SYS_MMC_ENV_DEV;

	return dev_no - 1;
}

int mmc_map_to_kernel_blk(int dev_no)
{
	return dev_no + 1;
}

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;

	switch (cfg->esdhc_base) {
	case USDHC2_BASE_ADDR:
		ret = 1; /* uSDHC2 emmc always present */
		break;
	case USDHC3_BASE_ADDR:
		ret = !gpio_get_value(USDHC3_CD_GPIO);
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
	 * mmc0                    USDHC2 (eMMC)
	 * mmc1                    USDHC3
	 */
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			imx_iomux_v3_setup_multiple_pads(
				usdhc2_emmc_pads, ARRAY_SIZE(usdhc2_emmc_pads));
			gpio_direction_output(USDHC2_PWR_GPIO, 1);
			usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
			break;
		case 1:
			imx_iomux_v3_setup_multiple_pads(
				usdhc3_pads, ARRAY_SIZE(usdhc3_pads));
			gpio_direction_input(USDHC3_CD_GPIO);
			usdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
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

#ifdef CONFIG_SYS_USE_SPINOR
iomux_v3_cfg_t const ecspi1_pads[] = {
	MX7D_PAD_SD1_RESET_B__ECSPI4_SCLK | MUX_PAD_CTRL(SPI_PAD_CTRL),
	MX7D_PAD_SD1_WP__ECSPI4_MOSI | MUX_PAD_CTRL(SPI_PAD_CTRL),
	MX7D_PAD_SD1_CD_B__ECSPI4_MISO | MUX_PAD_CTRL(SPI_PAD_CTRL),
	/* Chip selects CS0:CS3 */
	MX7D_PAD_SD1_CLK__GPIO5_IO3 | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX7D_PAD_SD1_CMD__GPIO5_IO4 | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX7D_PAD_SD1_DATA0__GPIO5_IO5 | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX7D_PAD_SD1_DATA1__GPIO5_IO6 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

void setup_spinor(void)
{
	imx_iomux_v3_setup_multiple_pads(ecspi1_pads,
					 ARRAY_SIZE(ecspi1_pads));
	gpio_direction_output(IMX_GPIO_NR(5, 3), 0);
}

int board_spi_cs_gpio(unsigned bus, unsigned cs)
{
	return (bus == 3 && cs == 0) ? (IMX_GPIO_NR(5, 3)) : -1;
}
#endif

int board_early_init_f(void)
{
	setup_iomux_uart();

#ifdef CONFIG_SYS_I2C_MXC
	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info1);
	setup_i2c(1, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info2);
#endif

	return 0;
}

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

#ifdef CONFIG_SYS_USE_SPINOR
	setup_spinor();
#endif

	return 0;
}

#ifdef CONFIG_CMD_BMODE
static const struct boot_mode board_boot_modes[] = {
	/* 4 bit bus width */
	{"emmc", MAKE_CFGVAL(0x10, 0x22, 0x00, 0x00)},
	{"sd3", MAKE_CFGVAL(0x10, 0x1a, 0x00, 0x00)},
	{NULL,   0},
};
#endif

#ifdef CONFIG_POWER
#define I2C_PMIC	0
int power_init_board(void)
{
	struct pmic *p;
	int ret;
	unsigned int reg, rev_id;

	ret = power_pfuze300_init(I2C_PMIC);
	if (ret)
		return ret;

	p = pmic_get("PFUZE300");
	ret = pmic_probe(p);
	if (ret)
		return ret;

	pmic_reg_read(p, PFUZE300_DEVICEID, &reg);
	pmic_reg_read(p, PFUZE300_REVID, &rev_id);
	printf("PMIC: PFUZE300 DEV_ID=0x%x REV_ID=0x%x\n", reg, rev_id);

	/* disable Low Power Mode during standby mode */
	pmic_reg_read(p, PFUZE300_LDOGCTL, &reg);
	reg |= 0x1;
	pmic_reg_write(p, PFUZE300_LDOGCTL, reg);

	/* SW1A/1B mode set to APS/APS */
	reg = 0x8;
	pmic_reg_write(p, PFUZE300_SW1AMODE, reg);
	pmic_reg_write(p, PFUZE300_SW1BMODE, reg);

	/* SW1A/1B standby voltage set to 1.025V */
	reg = 0xd;
	pmic_reg_write(p, PFUZE300_SW1ASTBY, reg);
	pmic_reg_write(p, PFUZE300_SW1BSTBY, reg);

	/* decrease SW1B normal voltage to 0.975V */
	pmic_reg_read(p, PFUZE300_SW1BVOLT, &reg);
	reg &= ~0x1f;
	reg |= PFUZE300_SW1AB_SETP(975);
	pmic_reg_write(p, PFUZE300_SW1BVOLT, reg);

	return 0;
}
#endif

int board_late_init(void)
{
#ifdef CONFIG_CMD_BMODE
	add_board_boot_modes(board_boot_modes);
#endif

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_init();
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
	puts("Board: MX7D 12x12 DDR3 ARM2\n");

	return 0;
}

#ifdef CONFIG_USB_EHCI_MX7
iomux_v3_cfg_t const usb_otg1_pads[] = {
	MX7D_PAD_UART3_TX_DATA__USB_OTG1_PWR | MUX_PAD_CTRL(NO_PAD_CTRL),
};

iomux_v3_cfg_t const usb_otg2_pads[] = {
	MX7D_PAD_UART3_CTS_B__USB_OTG2_PWR | MUX_PAD_CTRL(NO_PAD_CTRL),
};

int board_ehci_hcd_init(int port)
{
	switch (port) {
	case 0:
		imx_iomux_v3_setup_multiple_pads(usb_otg1_pads,
			ARRAY_SIZE(usb_otg1_pads));
		break;
	case 1:
		imx_iomux_v3_setup_multiple_pads(usb_otg2_pads,
			ARRAY_SIZE(usb_otg2_pads));
		break;
	default:
		printf("MXC USB port %d not yet supported\n", port);
		return 1;
	}
	return 0;
}
#endif
