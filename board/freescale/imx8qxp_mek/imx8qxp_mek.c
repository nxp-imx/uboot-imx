/*
 * Copyright 2017-2018 NXP
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
#include <power-domain.h>
#include "../common/tcpc.h"
#include <cdns3-uboot.h>

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
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

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

#define USDHC1_CD_GPIO	IMX_GPIO_NR(4, 22)

static struct fsl_esdhc_cfg usdhc_cfg[CONFIG_SYS_FSL_USDHC_NUM] = {
	{USDHC1_BASE_ADDR, 0, 8},
	{USDHC2_BASE_ADDR, 0, 4},
};

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
	SC_P_EMMC0_STROBE | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
};

static iomux_cfg_t usdhc1_sd[] = {
	SC_P_USDHC1_CLK | MUX_PAD_CTRL(ESDHC_CLK_PAD_CTRL),
	SC_P_USDHC1_CMD | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_DATA0 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_DATA1 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_DATA2 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_DATA3 | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_WP    | MUX_PAD_CTRL(ESDHC_PAD_CTRL), /* Mux for WP */
	SC_P_USDHC1_CD_B  | MUX_MODE_ALT(4) | MUX_PAD_CTRL(ESDHC_PAD_CTRL), /* Mux for CD,  GPIO4 IO22 */
	SC_P_USDHC1_RESET_B | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
	SC_P_USDHC1_VSELECT | MUX_PAD_CTRL(ESDHC_PAD_CTRL),
};

int board_mmc_init(bd_t *bis)
{
	int i, ret;
	struct power_domain pd;

	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-boot device node)    (Physical Port)
	 * mmc0                    USDHC1
	 * mmc1                    USDHC2
	 */
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			if (!power_domain_lookup_name("conn_sdhc0", &pd))
				power_domain_on(&pd);
			imx8_iomux_setup_multiple_pads(emmc0, ARRAY_SIZE(emmc0));
			init_clk_usdhc(0);
			usdhc_cfg[i].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
			break;
		case 1:
			if (!power_domain_lookup_name("conn_sdhc1", &pd))
				power_domain_on(&pd);
			imx8_iomux_setup_multiple_pads(usdhc1_sd, ARRAY_SIZE(usdhc1_sd));
			init_clk_usdhc(1);
			usdhc_cfg[i].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
			gpio_request(USDHC1_CD_GPIO, "sd1_cd");
			gpio_direction_input(USDHC1_CD_GPIO);
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
	}

	return ret;
}

#endif /* CONFIG_FSL_ESDHC */


#ifdef CONFIG_FEC_MXC
#include <miiphy.h>

static iomux_cfg_t pad_enet1[] = {
	SC_P_SPDIF0_TX | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_SPDIF0_RX | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ESAI0_TX3_RX2 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ESAI0_TX2_RX3 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ESAI0_TX1 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ESAI0_TX0 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_INPUT_PAD_CTRL),
	SC_P_ESAI0_SCKR | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ESAI0_TX4_RX1 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ESAI0_TX5_RX0 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ESAI0_FST  | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ESAI0_SCKT | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),
	SC_P_ESAI0_FSR  | MUX_MODE_ALT(3) | MUX_PAD_CTRL(ENET_NORMAL_PAD_CTRL),

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

	/* The BB_PER_RST_B will reset the ENET1 PHY */
	if (0 == CONFIG_FEC_ENET_DEV) {
		ret = dm_gpio_lookup_name("gpio@1a_4", &desc);
		if (ret)
			return;

		ret = dm_gpio_request(&desc, "enet0_reset");
		if (ret)
			return;

		dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT);
		dm_gpio_set_value(&desc, 0);
		udelay(50);
		dm_gpio_set_value(&desc, 1);
	}

	/* The board has a long delay for this reset to become stable */
	mdelay(200);
}

int board_eth_init(bd_t *bis)
{
	int ret;
	struct power_domain pd;

	printf("[%s] %d\n", __func__, __LINE__);

	if (CONFIG_FEC_ENET_DEV) {
		if (!power_domain_lookup_name("conn_enet1", &pd))
			power_domain_on(&pd);
	} else {
		if (!power_domain_lookup_name("conn_enet0", &pd))
			power_domain_on(&pd);
	}

	setup_iomux_fec();

	ret = fecmxc_initialize_multi(bis, CONFIG_FEC_ENET_DEV,
		CONFIG_FEC_MXC_PHYADDR, IMX_FEC_BASE);
	if (ret)
		printf("FEC1 MXC: %s:failed\n", __func__);

	return ret;
}

int board_phy_config(struct phy_device *phydev)
{
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x1f);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x8);

	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x00);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x82ee);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x100);

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}

static int setup_fec(int ind)
{
	/* Reset ENET PHY */
	enet_device_phy_reset();

	return 0;
}
#endif

#ifdef CONFIG_MXC_GPIO
#define IOEXP_RESET IMX_GPIO_NR(1, 1)

static iomux_cfg_t board_gpios[] = {
	SC_P_SPI2_SDO | MUX_MODE_ALT(4) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
	SC_P_ENET0_REFCLK_125M_25M | MUX_MODE_ALT(4) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
};

static void board_gpio_init(void)
{
	int ret;
	struct gpio_desc desc;

	ret = dm_gpio_lookup_name("gpio@1a_3", &desc);
	if (ret)
		return;

	ret = dm_gpio_request(&desc, "bb_per_rst_b");
	if (ret)
		return;

	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT);
	dm_gpio_set_value(&desc, 0);
	udelay(50);
	dm_gpio_set_value(&desc, 1);

	imx8_iomux_setup_multiple_pads(board_gpios, ARRAY_SIZE(board_gpios));

	/* enable i2c port expander assert reset line */
	gpio_request(IOEXP_RESET, "ioexp_rst");
	gpio_direction_output(IOEXP_RESET, 1);
}
#endif

int checkboard(void)
{
	puts("Board: iMX8QXP MEK\n");

	print_bootinfo();

	/* Note:  After reloc, ipcHndl will no longer be valid.  If handle
	 *        returned by sc_ipc_open matches SC_IPC_CH, use this
	 *        macro (valid after reloc) for subsequent SCI calls.
	 */
	if (gd->arch.ipc_channel_handle != SC_IPC_CH)
		printf("\nSCI error! Invalid handle\n");

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

static void imx8qxp_hsio_initialize(void)
{
	struct power_domain pd;
	int ret;

	if (!power_domain_lookup_name("hsio_pcie1", &pd)) {
		ret = power_domain_on(&pd);
		if (ret)
			printf("hsio_pcie1 Power up failed! (error = %d)\n", ret);
	}

	if (!power_domain_lookup_name("hsio_gpio", &pd)) {
		ret = power_domain_on(&pd);
		if (ret)
			 printf("hsio_gpio Power up failed! (error = %d)\n", ret);
	}

	imx8_iomux_setup_multiple_pads(board_pcie_pins, ARRAY_SIZE(board_pcie_pins));
}

void pci_init_board(void)
{
	imx8qxp_hsio_initialize();

	/* test the 1 lane mode of the PCIe A controller */
	mx8qxp_pcie_init();
}

#endif

#ifdef CONFIG_USB_XHCI_IMX8

#define USB_TYPEC_SEL IMX_GPIO_NR(5, 9)
static iomux_cfg_t ss_mux_gpio[] = {
	SC_P_ENET0_REFCLK_125M_25M | MUX_MODE_ALT(4) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
};

struct tcpc_port port;
struct tcpc_port_config port_config = {
	.i2c_bus = 1,
	.addr = 0x50,
	.port_type = TYPEC_PORT_DFP,
};

void ss_mux_select(enum typec_cc_polarity pol)
{
	if (pol == TYPEC_POLARITY_CC1)
		gpio_direction_output(USB_TYPEC_SEL, 0);
	else
		gpio_direction_output(USB_TYPEC_SEL, 1);
}

static void setup_typec(void)
{
	int ret;
	struct gpio_desc typec_en_desc;

	imx8_iomux_setup_multiple_pads(ss_mux_gpio, ARRAY_SIZE(ss_mux_gpio));
	gpio_request(USB_TYPEC_SEL, "typec_sel");

	ret = dm_gpio_lookup_name("gpio@1a_7", &typec_en_desc);
	if (ret) {
		printf("%s lookup gpio@1a_7 failed ret = %d\n", __func__, ret);
		return;
	}

	ret = dm_gpio_request(&typec_en_desc, "typec_en");
	if (ret) {
		printf("%s request typec_en failed ret = %d\n", __func__, ret);
		return;
	}

	/* Enable SS MUX */
	dm_gpio_set_dir_flags(&typec_en_desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);

	tcpc_init(&port, port_config, &ss_mux_select);
}

static struct cdns3_device cdns3_device_data = {
	.none_core_base = 0x5B110000,
	.xhci_base = 0x5B130000,
	.dev_base = 0x5B140000,
	.phy_base = 0x5B160000,
	.otg_base = 0x5B120000,
	.dr_mode = USB_DR_MODE_PERIPHERAL,
	.index = 1,
};

int usb_gadget_handle_interrupts(void)
{
	cdns3_uboot_handle_interrupt(1);
	return 0;
}

int board_usb_init(int index, enum usb_init_type init)
{
	int ret = 0;

	if (index == 1) {
		if (init == USB_INIT_HOST) {
			ret = tcpc_setup_dfp_mode(&port);
		} else {
			struct power_domain pd;
			int ret;

			/* Power on usb */
			if (!power_domain_lookup_name("conn_usb2", &pd)) {
				ret = power_domain_on(&pd);
				if (ret)
					printf("conn_usb2 Power up failed! (error = %d)\n", ret);
			}

			if (!power_domain_lookup_name("conn_usb2_phy", &pd)) {
				ret = power_domain_on(&pd);
				if (ret)
					printf("conn_usb2_phy Power up failed! (error = %d)\n", ret);
			}

			ret = tcpc_setup_ufp_mode(&port);
			printf("%d setufp mode %d\n", index, ret);

			ret = cdns3_uboot_init(&cdns3_device_data);
			printf("%d cdns3_uboot_initmode %d\n", index, ret);
		}
	}

	return ret;

}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	int ret = 0;

	if (index == 1) {
		if (init == USB_INIT_HOST) {
			ret = tcpc_disable_src_vbus(&port);
		} else {
			struct power_domain pd;
			int ret;

			cdns3_uboot_exit(1);

			/* Power off usb */
			if (!power_domain_lookup_name("conn_usb2", &pd)) {
				ret = power_domain_off(&pd);
				if (ret)
					printf("conn_usb2 Power up failed! (error = %d)\n", ret);
			}

			if (!power_domain_lookup_name("conn_usb2_phy", &pd)) {
				ret = power_domain_off(&pd);
				if (ret)
					printf("conn_usb2_phy Power up failed! (error = %d)\n", ret);
			}
		}
	}

	return ret;
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

#ifdef CONFIG_USB_XHCI_IMX8
	setup_typec();
#endif

	return 0;
}

void board_quiesce_devices()
{
	const char *power_on_devices[] = {
		"dma_lpuart0",

		/* HIFI DSP boot */
		"audio_sai0",
		"audio_ocram",
	};

	power_off_pd_devices(power_on_devices, ARRAY_SIZE(power_on_devices));
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
	return devno;
}

int board_late_init(void)
{
#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	setenv("board_name", "MEK");
	setenv("board_rev", "iMX8QXP");
#endif

	setenv("sec_boot", "no");
#ifdef CONFIG_AHAB_BOOT
	setenv("sec_boot", "yes");
#endif

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

#if defined(CONFIG_VIDEO_IMXDPUV1)
static void enable_lvds(struct display_info_t const *dev)
{
	struct gpio_desc desc;
	int ret;

	/* MIPI_DSI0_EN on IOEXP 0x1a port 6, MIPI_DSI1_EN on IOEXP 0x1d port 7 */
	ret = dm_gpio_lookup_name("gpio@1a_6", &desc);
	if (ret)
		return;

	ret = dm_gpio_request(&desc, "lvds0_en");
	if (ret)
		return;

	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);

	display_controller_setup((PS2KHZ(dev->mode.pixclock) * 1000));
	lvds_soc_setup(dev->bus, (PS2KHZ(dev->mode.pixclock) * 1000));
	lvds_configure(dev->bus);
	lvds2hdmi_setup(13);
}

struct display_info_t const displays[] = {{
	.bus	= 0, /* LVDS0 */
	.addr	= 0, /* LVDS0 */
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
