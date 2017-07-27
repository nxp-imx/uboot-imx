/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <netdev.h>
#include <fsl_ifc.h>
#include <fdt_support.h>
#include <libfdt.h>
#include <environment.h>
#include <fsl_esdhc.h>
#include <i2c.h>
#include "pca953x.h"

#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/clock.h>
#include <asm/imx-common/sci/sci.h>
#include <asm/arch/imx8-pins.h>
#include <dm.h>
#include <imx8_hsio.h>
#include <usb.h>
#include <asm/arch/iomux.h>
#include <asm/arch/sys_proto.h>
#include <asm/imx-common/video.h>
#include <asm/arch/video_common.h>

DECLARE_GLOBAL_DATA_PTR;

#define ESDHC_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define ESDHC_CLK_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))


#define ENET_INPUT_PAD_CTRL	((SC_PAD_CONFIG_OD_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_18V_10MA << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define ENET_NORMAL_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_18V_10MA << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define FSPI_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define GPIO_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define I2C_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_LOW << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define UART_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_LOW << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PD << PADRING_PULL_SHIFT))

static iomux_cfg_t uart0_pads[] = {
	SC_P_UART0_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	SC_P_UART0_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static void setup_iomux_uart(void)
{
	imx8_iomux_setup_multiple_pads(uart0_pads, ARRAY_SIZE(uart0_pads));
}

int board_early_init_f(void)
{
	sc_ipc_t ipcHndl = 0;
	sc_err_t sciErr = 0;

	ipcHndl = gd->arch.ipc_channel_handle;

	/* Power up the GPT */
	sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_GPT_0, SC_PM_PW_MODE_ON);
	if (sciErr != SC_ERR_NONE)
		return 0;

	/* Set GPT clock root to 24 MHz */
	sc_pm_clock_rate_t gpt_rate = 24000000;
	sciErr = sc_pm_set_clock_rate(ipcHndl, SC_R_GPT_0, SC_PM_CLK_PER, &gpt_rate);
	if (sciErr != SC_ERR_NONE)
		return 0;

	/* Enable GPT clock root */
	sciErr = sc_pm_clock_enable(ipcHndl, SC_R_GPT_0, SC_PM_CLK_PER, true, false);
	if (sciErr != SC_ERR_NONE)
		return 0;

	/* Power up UART0 */
	sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_UART_0, SC_PM_PW_MODE_ON);
	if (sciErr != SC_ERR_NONE)
		return 0;

	/* Set UART0 clock root to 80 MHz */
	sc_pm_clock_rate_t rate = 80000000;
	sciErr = sc_pm_set_clock_rate(ipcHndl, SC_R_UART_0, 2, &rate);
	if (sciErr != SC_ERR_NONE)
		return 0;

	/* Enable UART0 clock root */
	sciErr = sc_pm_clock_enable(ipcHndl, SC_R_UART_0, 2, true, false);
	if (sciErr != SC_ERR_NONE)
		return 0;

	setup_iomux_uart();

	return 0;
}

#ifdef CONFIG_FSL_ESDHC

#define USDHC1_CD_GPIO	IMX_GPIO_NR(5, 22)
#define USDHC2_CD_GPIO	IMX_GPIO_NR(4, 12)

static struct fsl_esdhc_cfg usdhc_cfg[CONFIG_SYS_FSL_USDHC_NUM] = {
#ifdef CONFIG_TARGET_IMX8QM_LPDDR4_ARM2
	{USDHC1_BASE_ADDR, 0, 8},
#endif
	{USDHC2_BASE_ADDR, 0, 4},
	{USDHC3_BASE_ADDR, 0, 4},
};

#ifdef CONFIG_TARGET_IMX8QM_LPDDR4_ARM2
static iomux_cfg_t emmc0[] = {
	SC_P_EMMC0_CLK | MUX_PAD_CTRL(ESDHC_CLK_PAD_CTRL),
	SC_P_EMMC0_CMD | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA0 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA1 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA2 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA3 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA4 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA5 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA6 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_DATA7 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_RESET_B | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_EMMC0_STROBE | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
};
#endif

static iomux_cfg_t usdhc1_sd[] = {
	SC_P_USDHC1_CLK | MUX_PAD_CTRL(ESDHC_CLK_PAD_CTRL),
	SC_P_USDHC1_CMD | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_DATA0 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_DATA1 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_DATA2 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_DATA3 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_DATA6 | MUX_MODE_ALT(2) | MUX_PAD_CTRL(ESDHC_PAD_CTRL), /* Mux for WP */
	SC_P_USDHC1_DATA7 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ESDHC_PAD_CTRL), /* Mux for CD,  GPIO5 IO22 */
	SC_P_USDHC1_RESET_B | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_VSELECT | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
};

static iomux_cfg_t usdhc2_sd[] = {
	SC_P_USDHC2_CLK | MUX_PAD_CTRL(ESDHC_CLK_PAD_CTRL),
	SC_P_USDHC2_CMD | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC2_DATA0 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC2_DATA1 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC2_DATA2 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC2_DATA3 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC2_RESET_B | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC2_WP | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC2_CD_B | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ESDHC_PAD_CTRL),	/* Mux to GPIO4 IO12 */
};

int board_mmc_init(bd_t *bis)
{
	int i, ret;

	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-boot device node)    (Physical Port)
	 * mmc0                    USDHC1
	 * mmc1                    USDHC2
	 * mmc2                    USDHC3
	 */
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
#ifdef CONFIG_TARGET_IMX8QM_LPDDR4_ARM2
		case 0:
			imx8_iomux_setup_multiple_pads(emmc0, ARRAY_SIZE(emmc0));
			init_clk_usdhc(0);
			usdhc_cfg[i].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
			break;
		case 1:
#else
		case 0:
#endif
			imx8_iomux_setup_multiple_pads(usdhc1_sd, ARRAY_SIZE(usdhc1_sd));
			init_clk_usdhc(1);
			usdhc_cfg[i].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
			gpio_request(USDHC1_CD_GPIO, "sd1_cd");
			gpio_direction_input(USDHC1_CD_GPIO);
			break;
#ifdef CONFIG_TARGET_IMX8QM_LPDDR4_ARM2
		case 2:
#else
		case 1:
#endif
			imx8_iomux_setup_multiple_pads(usdhc2_sd, ARRAY_SIZE(usdhc2_sd));
			init_clk_usdhc(2);
			usdhc_cfg[i].sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
			gpio_request(USDHC2_CD_GPIO, "sd2_cd");
			gpio_direction_input(USDHC2_CD_GPIO);
			break;
		default:
			printf("Warning: you configured more USDHC controllers"
				"(%d) than supported by the board\n", i + 1);
			return 0;
		}
		ret = fsl_esdhc_initialize(bis, &usdhc_cfg[i]);
		if (ret) {
			printf("Warning: failed to initialize mmc dev %d\n", i);
			return ret;
		}
	}

	return 0;
}

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;

	switch (cfg->esdhc_base) {
	case USDHC1_BASE_ADDR:
		ret = 1; /* eMMC */
		break;
	case USDHC2_BASE_ADDR:
		ret = !gpio_get_value(USDHC1_CD_GPIO);
		break;
	case USDHC3_BASE_ADDR:
		ret = !gpio_get_value(USDHC2_CD_GPIO);
		break;
	}

	return ret;
}

#endif /* CONFIG_FSL_ESDHC */


#ifdef CONFIG_FEC_MXC
#include <miiphy.h>

static iomux_cfg_t pad_enet1[] = {
	SC_P_ENET1_RGMII_RX_CTL | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET1_RGMII_RXD0 | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET1_RGMII_RXD1 | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET1_RGMII_RXD2 | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET1_RGMII_RXD3 | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET1_RGMII_RXC | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET1_RGMII_TX_CTL | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET1_RGMII_TXD0 | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET1_RGMII_TXD1 | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET1_RGMII_TXD2 | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET1_RGMII_TXD3 | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET1_RGMII_TXC | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),

	/* Shared MDIO */
	SC_P_ENET0_MDC | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_MDIO | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
};

static iomux_cfg_t pad_enet0[] = {
	SC_P_ENET0_RGMII_RX_CTL | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET0_RGMII_RXD0 | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET0_RGMII_RXD1 | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET0_RGMII_RXD2 | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET0_RGMII_RXD3 | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET0_RGMII_RXC | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ENET0_RGMII_TX_CTL | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_RGMII_TXD0 | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_RGMII_TXD1 | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_RGMII_TXD2 | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_RGMII_TXD3 | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_RGMII_TXC | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),

	/* Shared MDIO */
	SC_P_ENET0_MDC | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ENET0_MDIO | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
};

static void setup_iomux_fec(void)
{
	if (0 == CONFIG_FEC_ENET_DEV)
		imx8_iomux_setup_multiple_pads(pad_enet0, ARRAY_SIZE(pad_enet0));
	else
		imx8_iomux_setup_multiple_pads(pad_enet1, ARRAY_SIZE(pad_enet1));
}

static void enet_device_phy_reset(void)
{
	struct gpio_desc desc;
	int ret;

	if (0 == CONFIG_FEC_ENET_DEV) {
		ret = dm_gpio_lookup_name("gpio@18_1", &desc);
		if (ret)
			return;

		ret = dm_gpio_request(&desc, "enet0_reset");
		if (ret)
			return;
	} else {
		ret = dm_gpio_lookup_name("gpio@18_4", &desc);
		if (ret)
			return;

		ret = dm_gpio_request(&desc, "enet1_reset");
		if (ret)
			return;
	}

	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT);
	dm_gpio_set_value(&desc, 0);
	udelay(50);
	dm_gpio_set_value(&desc, 1);

	/* The board has a long delay for this reset to become stable */
	mdelay(200);
}

int board_eth_init(bd_t *bis)
{
	int ret;

	printf("[%s] %d\n", __func__, __LINE__);

	setup_iomux_fec();

	ret = fecmxc_initialize_multi(bis, CONFIG_FEC_ENET_DEV,
		CONFIG_FEC_MXC_PHYADDR, IMX_FEC_BASE);
	if (ret)
		printf("FEC1 MXC: %s:failed\n", __func__);

	return ret;
}

int board_phy_config(struct phy_device *phydev)
{
#ifdef CONFIG_FEC_ENABLE_MAX7322
	uint8_t value;

	/* This is needed to drive the pads to 1.8V instead of 1.5V */
	i2c_set_bus_num(CONFIG_MAX7322_I2C_BUS);

	if (!i2c_probe(CONFIG_MAX7322_I2C_ADDR)) {
		/* Write 0x1 to enable O0 output, this device has no addr */
		/* hence addr length is 0 */
		value = 0x1;
		if (i2c_write(CONFIG_MAX7322_I2C_ADDR, 0, 0, &value, 1))
			printf("MAX7322 write failed\n");
	} else {
		printf("MAX7322 Not found\n");
	}
	mdelay(1);
#endif

	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x1f);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x8);

	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x100);

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}

static int setup_fec(int ind)
{
	sc_err_t err;
	sc_ipc_t ipc;
	sc_pm_clock_rate_t rate = 24000000;
	sc_rsrc_t enet[2] = {SC_R_ENET_0, SC_R_ENET_1};

	if (ind > 1)
		return -EINVAL;

	ipc = gd->arch.ipc_channel_handle;
	/* Power on ENET, notify if SCI error occurred */
	err = sc_pm_set_resource_power_mode(ipc, enet[ind], SC_PM_PW_MODE_ON);
	if (err != SC_ERR_NONE) {
		printf("\nSC_R_ENET_0 Power up failed! (error = %d)\n", err);
		return -EIO;
	}

	/* Disable SC_R_ENET_0 clock root */
	err = sc_pm_clock_enable(ipc, enet[ind], 0, false, false);
	err |= sc_pm_clock_enable(ipc, enet[ind], 2, false, false);
	err |= sc_pm_clock_enable(ipc, enet[ind], 4, false, false);
	if (err != SC_ERR_NONE) {
		printf("\nSC_R_ENET_0 set clock disable failed! (error = %d)\n", err);
		return -EIO;
	}

	/* Set SC_R_ENET_0 clock root to 125 MHz */
	rate = 125000000;

	/* div = 8 clk_source = PLL_1 ss_slice #7 in verfication codes */
	err = sc_pm_set_clock_rate(ipc, enet[ind], 2, &rate);
	if (err != SC_ERR_NONE) {
		printf("\nSC_R_ENET_0 set clock ref clock 125M failed! (error = %d)\n", err);
		return -EIO;
	}

	/* Enable SC_R_ENET_0 clock root */
	err = sc_pm_clock_enable(ipc, enet[ind], 0, true, true);
	err |= sc_pm_clock_enable(ipc, enet[ind], 2, true, true);
	err |= sc_pm_clock_enable(ipc, enet[ind], 4, true, true);
	if (err != SC_ERR_NONE) {
		printf("\nSC_R_ENET_0 set clock enable failed! (error = %d)\n", err);
		return -EIO;
	}

	/* Reset ENET PHY */
	enet_device_phy_reset();

	return 0;
}
#endif

#ifdef CONFIG_FSL_FSPI
void board_fspi_init(void)
{
	sc_err_t sciErr = 0;
	sc_pm_clock_rate_t rate;
	sc_ipc_t ipcHndl = gd->arch.ipc_channel_handle;

	/* Power up the FSPI0*/
	sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_FSPI_0, SC_PM_PW_MODE_ON);
	if (sciErr != SC_ERR_NONE) {
		puts("FSPI0 power up failed\n");
		return;
	}

	/* Set FSPI0 clock root to 29 MHz */
	rate = 29000000;
	sciErr = sc_pm_set_clock_rate(ipcHndl, SC_R_FSPI_0, SC_PM_CLK_PER, &rate);
	if (sciErr != SC_ERR_NONE) {
		puts("FSPI0 setrate failed\n");
		return;
	}

	/* Enable FSPI0 clock root */
	sciErr = sc_pm_clock_enable(ipcHndl, SC_R_FSPI_0, SC_PM_CLK_PER, true, false);
	if (sciErr != SC_ERR_NONE) {
		puts("FSPI0 enable clock failed\n");
		return;
	}
}
#endif

#ifdef CONFIG_MXC_GPIO

#define DEBUG_LED   IMX_GPIO_NR(2, 15)
#define IOEXP_RESET IMX_GPIO_NR(1, 12)

static iomux_cfg_t board_gpios[] = {
	SC_P_LVDS1_I2C0_SCL | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
	SC_P_SPDIF0_TX | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
};

static void setup_gpio(void)
{
	sc_err_t err;
	sc_ipc_t ipc;
	u32 i;

	ipc = gd->arch.ipc_channel_handle;

	/* Power up all GPIOs */
	for (i = 0; i < 8; i++) {
		err = sc_pm_set_resource_power_mode(ipc, SC_R_GPIO_0 + i, SC_PM_PW_MODE_ON);
		if (err != SC_ERR_NONE)
			printf("gpio_%u powerup error\n", i);
	}
}

static void board_gpio_init(void)
{
	setup_gpio();

	imx8_iomux_setup_multiple_pads(board_gpios, ARRAY_SIZE(board_gpios));

	gpio_request(DEBUG_LED, "debug_led");
	gpio_direction_output(DEBUG_LED, 1);

	/* enable i2c port expander assert reset line */
	gpio_request(IOEXP_RESET, "ioexp_rst");
	gpio_direction_output(IOEXP_RESET, 1);
}
#endif

int checkboard(void)
{
#ifdef CONFIG_TARGET_IMX8QM_LPDDR4_ARM2
	puts("Board: iMX8QM LPDDR4 ARM2\n");
#else
	puts("Board: iMX8QM DDR4 ARM2\n");
#endif

	print_bootinfo();

	/* Note:  After reloc, ipcHndl will no longer be valid.  If handle
	 *        returned by sc_ipc_open matches SC_IPC_CH, use this
	 *        macro (valid after reloc) for subsequent SCI calls.
	 */
	if (gd->arch.ipc_channel_handle != SC_IPC_CH) {
		printf("\nSCI error! Invalid handle\n");
	}

#ifdef SCI_FORCE_ABORT
	sc_rpc_msg_t abort_msg;

	puts("Send abort request\n");
	RPC_SIZE(&abort_msg) = 1;
	RPC_SVC(&abort_msg) = SC_RPC_SVC_ABORT;
	sc_ipc_write(SC_IPC_CH, &abort_msg);

	/* Close IPC channel */
	sc_ipc_close(SC_IPC_CH);
#endif /* SCI_FORCE_ABORT */

	return 0;
}

#ifdef CONFIG_FSL_HSIO

#define PCIE_PAD_CTRL	((SC_PAD_CONFIG_OD_IN << PADRING_CONFIG_SHIFT))
static iomux_cfg_t board_pcie_pins[] = {
	SC_P_PCIE_CTRL0_CLKREQ_B | MUX_MODE_ALT(0) | MUX_PAD_CTRL(PCIE_PAD_CTRL),
	SC_P_PCIE_CTRL0_WAKE_B | MUX_MODE_ALT(0) | MUX_PAD_CTRL(PCIE_PAD_CTRL),
	SC_P_PCIE_CTRL0_PERST_B | MUX_MODE_ALT(0) | MUX_PAD_CTRL(PCIE_PAD_CTRL),
};

static void imx8qm_hsio_initialize(void)
{
	sc_err_t err;
	sc_ipc_t ipc;

	ipc = gd->arch.ipc_channel_handle;

	err = sc_pm_set_resource_power_mode(ipc, SC_R_SERDES_0, SC_PM_PW_MODE_ON);
	err |= sc_pm_set_resource_power_mode(ipc, SC_R_SERDES_1, SC_PM_PW_MODE_ON);
	if (err != SC_ERR_NONE)
		printf("\nSC_R_SERDES_0 Power up failed! (error = %d)\n", err);

	err = sc_pm_set_resource_power_mode(ipc, SC_R_SATA_0, SC_PM_PW_MODE_ON);
	if (err != SC_ERR_NONE)
		printf("\nSC_R_SATA_0 Power up failed! (error = %d)\n", err);

	err = sc_pm_set_resource_power_mode(ipc, SC_R_PCIE_A, SC_PM_PW_MODE_ON);
	err |= sc_pm_set_resource_power_mode(ipc, SC_R_PCIE_B, SC_PM_PW_MODE_ON);
	if (err != SC_ERR_NONE)
		printf("\nSC_R_PCIE_A/B Power up failed! (error = %d)\n", err);

	imx8_iomux_setup_multiple_pads(board_pcie_pins, ARRAY_SIZE(board_pcie_pins));

}

void pci_init_board(void)
{
	/* test the 1 lane mode of the PCIe A controller */
	mx8qm_pcie_init();
}
#endif

#ifdef CONFIG_USB_EHCI_MX6
static void setup_otg(void)
{
	/* Enable usb power */
	init_otg_power();
}
#endif

int board_init(void)
{
#ifdef CONFIG_MXC_GPIO
	board_gpio_init();
#endif

#ifdef CONFIG_FEC_MXC
	setup_fec(CONFIG_FEC_ENET_DEV);
#endif

#ifdef CONFIG_FSL_FSPI
	board_fspi_init();
#endif

#ifdef CONFIG_FSL_HSIO
	imx8qm_hsio_initialize();
#ifdef CONFIG_SCSI_AHCI_PLAT
	sata_init();
#endif
#endif

#ifdef CONFIG_USB_EHCI_MX6
	setup_otg();
#endif
	return 0;
}

void board_quiesce_devices()
{
	sc_ipc_t ipcHndl = gd->arch.ipc_channel_handle;

	printf("Disable devices powered up by uboot\n");
#if defined(CONFIG_VIDEO_IMXDPUV1)
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_LVDS_0, SC_PM_PW_MODE_OFF);
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_LVDS_1, SC_PM_PW_MODE_OFF);
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_MIPI_0, SC_PM_PW_MODE_OFF);
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_MIPI_1, SC_PM_PW_MODE_OFF);

	sc_pm_set_resource_power_mode(ipcHndl, SC_R_DC_0, SC_PM_PW_MODE_OFF);
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_DC_1, SC_PM_PW_MODE_OFF);
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_DC_0_PLL_0, SC_PM_PW_MODE_OFF);
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_DC_0_PLL_1, SC_PM_PW_MODE_OFF);
#endif
#ifdef CONFIG_FSL_HSIO
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_PCIE_A, SC_PM_PW_MODE_OFF);
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_PCIE_B, SC_PM_PW_MODE_OFF);
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_SERDES_0, SC_PM_PW_MODE_OFF);
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_SERDES_1, SC_PM_PW_MODE_OFF);
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_SATA_0, SC_PM_PW_MODE_OFF);
#endif
#ifdef CONFIG_MXC_GPIO
	/* Power up all GPIOs */
	int i;
	for (i = 0; i < 8; i++)
		sc_pm_set_resource_power_mode(ipcHndl, SC_R_GPIO_0 + i, SC_PM_PW_MODE_OFF);
#endif
#ifdef CONFIG_FSL_FSPI
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_FSPI_0, SC_PM_PW_MODE_OFF);
#endif
#ifdef CONFIG_FEC_MXC
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_ENET_0, SC_PM_PW_MODE_OFF);
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_ENET_1, SC_PM_PW_MODE_OFF);
#endif
#ifdef CONFIG_FSL_ESDHC
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_SDHC_0, SC_PM_PW_MODE_OFF);
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_SDHC_1, SC_PM_PW_MODE_OFF);
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_SDHC_2, SC_PM_PW_MODE_OFF);
#endif
#ifdef CONFIG_USB_EHCI_MX6
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_USB_0, SC_PM_PW_MODE_OFF);
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_USB_0_PHY, SC_PM_PW_MODE_OFF);
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_USB_2, SC_PM_PW_MODE_OFF);
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_USB_2_PHY, SC_PM_PW_MODE_OFF);
#endif
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_I2C_1, SC_PM_PW_MODE_OFF);
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_LVDS_0_I2C_1, SC_PM_PW_MODE_OFF);
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_LVDS_1_I2C_1, SC_PM_PW_MODE_OFF);
	sc_pm_set_resource_power_mode(ipcHndl, SC_R_GPT_0, SC_PM_PW_MODE_OFF);
}

void detail_board_ddr_info(void)
{
	puts("\nDDR    ");
}

/*
 * Board specific reset that is system reset.
 */
void reset_cpu(ulong addr)
{
	puts("SCI reboot request");
	sc_pm_reboot(SC_IPC_CH, SC_PM_RESET_TYPE_COLD);
	while (1)
		putc('.');
}

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, bd_t *bd)
{
	return 0;
}
#endif

int board_mmc_get_env_dev(int devno)
{
#ifdef CONFIG_TARGET_IMX8QM_LPDDR4_ARM2
	return devno;
#else
	return devno - 1;
#endif
}

int mmc_map_to_kernel_blk(int dev_no)
{
#ifdef CONFIG_TARGET_IMX8QM_LPDDR4_ARM2
	return dev_no;
#else
	return dev_no + 1;
#endif
}

int board_late_init(void)
{
#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

	return 0;
}

#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_ANDROID_RECOVERY
int is_recovery_key_pressing(void)
{
	return 0; /*TODO*/
}
#endif /*CONFIG_ANDROID_RECOVERY*/
#endif /*CONFIG_FSL_FASTBOOT*/

/* Only Enable USB3 resources currently */
int board_usb_init(int index, enum usb_init_type init)
{
	sc_err_t err;
	sc_ipc_t ipc;
	sc_rsrc_t usbs[2] = {SC_R_USB_2, SC_R_USB_2_PHY};
	int i;

	ipc = gd->arch.ipc_channel_handle;

	/* Power on usb, notify if SCI error occurred */
	for (i = 0; i < 2; i++) {
		err = sc_pm_set_resource_power_mode(ipc, usbs[i],
			SC_PM_PW_MODE_ON);
		if (err != SC_ERR_NONE)
			printf("SC_R_USB_%d Power up failed: %d\n", i, err);
	}

	err = sc_pm_clock_enable(ipc, usbs[0], SC_PM_CLK_MISC, true, false);
	if (err != SC_ERR_NONE)
		printf("USB3 set clock failed!, line=%d (error = %d)\n",
			__LINE__, err);

	err = sc_pm_clock_enable(ipc, usbs[0], SC_PM_CLK_MST_BUS, true, false);
	if (err != SC_ERR_NONE)
		printf("USB3 set clock failed!, line=%d (error = %d)\n",
			__LINE__, err);

	err = sc_pm_clock_enable(ipc, usbs[0], SC_PM_CLK_PER, true, false);
	if (err != SC_ERR_NONE)
		printf("USB3 set clock failed!, line=%d (error = %d)\n",
			__LINE__, err);

	return 0;
}

#if defined(CONFIG_VIDEO_IMXDPUV1)
static void enable_lvds(struct display_info_t const *dev)
{
	display_controller_setup((PS2KHZ(dev->mode.pixclock) * 1000));
	lvds_soc_setup(dev->bus, (PS2KHZ(dev->mode.pixclock) * 1000));
	lvds_configure(dev->bus);
	lvds2hdmi_setup(6);
}

struct display_info_t const displays[] = {{
	.bus	= 0, /* LVDS0 */
	.addr	= 0, /* Unused */
	.pixfmt	= IMXDPUV1_PIX_FMT_BGRA32,
	.detect	= NULL,
	.enable	= enable_lvds,
	.mode	= {
		.name           = "IT6263", /* 720P60 */
		.refresh        = 60,
		.xres           = 1280,
		.yres           = 720,
		.pixclock       = 13468, /* 74250000 */
		.left_margin    = 110,
		.right_margin   = 220,
		.upper_margin   = 5,
		.lower_margin   = 20,
		.hsync_len      = 40,
		.vsync_len      = 5,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} } };
size_t display_count = ARRAY_SIZE(displays);

#endif /* CONFIG_VIDEO_IMXDPUV1 */

