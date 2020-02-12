/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2017-2019 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <asm/io.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm-generic/gpio.h>
#include <fsl_esdhc.h>
#include <mmc.h>
#include <asm/arch/imx8mq_pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <asm/arch/clock.h>
#include <spl.h>
#include <power/pmic.h>
#include <usb.h>
#include <dwc3-uboot.h>
#include <dm.h>
#include <i2c.h>

DECLARE_GLOBAL_DATA_PTR;

#define QSPI_PAD_CTRL	(PAD_CTL_DSE2 | PAD_CTL_HYS)

#define UART_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL1)

#define WDOG_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_HYS | PAD_CTL_PUE)

static iomux_v3_cfg_t const wdog_pads[] = {
	IMX8MQ_PAD_GPIO1_IO02__WDOG1_WDOG_B | MUX_PAD_CTRL(WDOG_PAD_CTRL),
};

#ifdef CONFIG_FSL_QSPI
static iomux_v3_cfg_t const qspi_pads[] = {
	IMX8MQ_PAD_NAND_ALE__QSPI_A_SCLK | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	IMX8MQ_PAD_NAND_CE0_B__QSPI_A_SS0_B | MUX_PAD_CTRL(QSPI_PAD_CTRL),

	IMX8MQ_PAD_NAND_DATA00__QSPI_A_DATA0 | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	IMX8MQ_PAD_NAND_DATA01__QSPI_A_DATA1 | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	IMX8MQ_PAD_NAND_DATA02__QSPI_A_DATA2 | MUX_PAD_CTRL(QSPI_PAD_CTRL),
	IMX8MQ_PAD_NAND_DATA03__QSPI_A_DATA3 | MUX_PAD_CTRL(QSPI_PAD_CTRL),
};

int board_qspi_init(void)
{
	imx_iomux_v3_setup_multiple_pads(qspi_pads, ARRAY_SIZE(qspi_pads));

	set_clk_qspi();

	return 0;
}
#endif

static iomux_v3_cfg_t const uart_pads[] = {
	IMX8MQ_PAD_UART1_RXD__UART1_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MQ_PAD_UART1_TXD__UART1_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

int board_early_init_f(void)
{
	struct wdog_regs *wdog = (struct wdog_regs *)WDOG1_BASE_ADDR;

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));

	set_wdog_reset(wdog);

	imx_iomux_v3_setup_multiple_pads(uart_pads, ARRAY_SIZE(uart_pads));

	return 0;
}

#ifdef CONFIG_BOARD_POSTCLK_INIT
int board_postclk_init(void)
{
	/* TODO */
	return 0;
}
#endif

/* layout of baseboard id */
#define IMX8MQ_GPIO3_IO24 IMX_GPIO_NR(3, 24)  //board_id[2]:2
#define IMX8MQ_GPIO3_IO22 IMX_GPIO_NR(3, 22)  //board_id[2]:1
#define IMX8MQ_GPIO3_IO19 IMX_GPIO_NR(3, 19)  //board_id[2]:0

/* GPIO port description */
static unsigned long imx8m_gpio_ports[] = {
	[0] = GPIO1_BASE_ADDR,
	[1] = GPIO2_BASE_ADDR,
	[2] = GPIO3_BASE_ADDR,
	[3] = GPIO4_BASE_ADDR,
	[4] = GPIO5_BASE_ADDR,
};

/* use legacy gpio operations before device model is ready. */
static int gpio_direction_input_legacy(unsigned int gpio)
{
	unsigned int port;
	struct gpio_regs *regs;
	u32 l;

	port = gpio/32;

	gpio &= 0x1f;

	regs = (struct gpio_regs *)imx8m_gpio_ports[port];

	l = readl(&regs->gpio_dir);
	/* set direction as input. */
	l &= ~(1 << gpio);
	writel(l, &regs->gpio_dir);

	return 0;
}

static int gpio_get_value_legacy(unsigned gpio)
{
	unsigned int port;
	struct gpio_regs *regs;
	u32 val;

	port = gpio/32;

	gpio &= 0x1f;

	regs = (struct gpio_regs *)imx8m_gpio_ports[port];

	val = (readl(&regs->gpio_dr) >> gpio) & 0x01;

	return val;
}

int get_imx8m_baseboard_id(void)
{
	int  i = 0, value = 0;
	int baseboard_id;
	int pin[3];

	/* initialize the pin array */
	pin[0] = IMX8MQ_GPIO3_IO19;
	pin[1] = IMX8MQ_GPIO3_IO22;
	pin[2] = IMX8MQ_GPIO3_IO24;

	/* Set gpio direction as input and get the input value */
	baseboard_id = 0;
	for (i = 0; i < 3; i++) {
		gpio_direction_input_legacy(pin[i]);
		if ((value = gpio_get_value_legacy(pin[i])) < 0) {
			printf("Error! Read gpio port: %d failed!\n", pin[i]);
			return -1;
		} else
			baseboard_id |= ((value & 0x01) << i);
	}

	return baseboard_id;
}
#ifdef CONFIG_IMX_TRUSTY_OS
int get_tee_load(ulong *load)
{
	int board_id;

	board_id = get_imx8m_baseboard_id();
	/* load TEE to the last 32M of DDR */
	if ((board_id == AIY_MICRON_1G) ||
			(board_id == AIY_HYNIX_1G)) {
		/* for 1G DDR board */
		*load = (ulong)TEE_LOAD_ADDR_1G;
#ifndef CONFIG_AIY_LPDDR4_3G
	} else if (board_id == AIY_KINGSTON_2G){
		/* for 2G DDR board */
		*load = (ulong)TEE_LOAD_ADDR_2G;
#else
	} else if (board_id == AIY_MICRON_3G){
		/* for 3G DDR board */
		*load = (ulong)TEE_LOAD_ADDR_3G;
#endif
	} else {
		printf("ddr size not support!\n");
		return -1;
	}

	return 0;
}
#endif

int board_phys_sdram_size(phys_size_t *size)
{
	int baseboard_id;

	if (!size) {
		printf("Invalid parameter!\n");
		return -EINVAL;
	}

	/* different boards have different DDR type, distinguish the DDR
	 * type by board id.
	 */
	baseboard_id = get_imx8m_baseboard_id();
	if ((baseboard_id == AIY_MICRON_1G) ||
			(baseboard_id == AIY_HYNIX_1G)) {
		/* 1G DDR size */
		*size = 0x40000000;
#ifndef CONFIG_AIY_LPDDR4_3G
	} else if (baseboard_id == AIY_KINGSTON_2G) {
		/* 2G DDR size */
		*size = 0x80000000;
#else
	} else if (baseboard_id == AIY_MICRON_3G) {
		/* 3G DDR size */
		*size = 0xc0000000;
#endif
	} else {
		printf("ddr size not support!\n");
		return -EPERM;
	}

	return 0;
}


#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, bd_t *bd)
{
	return 0;
}
#endif

#ifdef CONFIG_FEC_MXC
#define FEC_RST_PAD IMX_GPIO_NR(1, 9)
static iomux_v3_cfg_t const fec1_rst_pads[] = {
	IMX8MQ_PAD_GPIO1_IO09__GPIO1_IO9 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static void setup_iomux_fec(void)
{
	imx_iomux_v3_setup_multiple_pads(fec1_rst_pads,
					 ARRAY_SIZE(fec1_rst_pads));

	gpio_request(IMX_GPIO_NR(1, 9), "fec1_rst");
	gpio_direction_output(IMX_GPIO_NR(1, 9), 0);
	udelay(500);
	gpio_direction_output(IMX_GPIO_NR(1, 9), 1);
}

static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *const iomuxc_gpr_regs
		= (struct iomuxc_gpr_base_regs *) IOMUXC_GPR_BASE_ADDR;

	setup_iomux_fec();

	/* Use 125M anatop REF_CLK1 for ENET1, not from external */
	clrsetbits_le32(&iomuxc_gpr_regs->gpr[1],
			IOMUXC_GPR_GPR1_GPR_ENET1_TX_CLK_SEL_SHIFT, 0);
	return set_clk_enet(ENET_125MHZ);
}


int board_phy_config(struct phy_device *phydev)
{
	/* enable rgmii rxc skew and phy mode select to RGMII copper */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x1f);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x8);

	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x100);

	if (phydev->drv->config)
		phydev->drv->config(phydev);
	return 0;
}
#endif

#ifdef CONFIG_USB_DWC3

#define USB_PHY_CTRL0			0xF0040
#define USB_PHY_CTRL0_REF_SSP_EN	BIT(2)

#define USB_PHY_CTRL1			0xF0044
#define USB_PHY_CTRL1_RESET		BIT(0)
#define USB_PHY_CTRL1_COMMONONN		BIT(1)
#define USB_PHY_CTRL1_ATERESET		BIT(3)
#define USB_PHY_CTRL1_VDATSRCENB0	BIT(19)
#define USB_PHY_CTRL1_VDATDETENB0	BIT(20)

#define USB_PHY_CTRL2			0xF0048
#define USB_PHY_CTRL2_TXENABLEN0	BIT(8)

#define PTN5150A_BUS				2
#define PTN5150A_ADDR				0x3D
#define PTN5150A_CONTROL			0x2
#define PTN5150A_CONTROL_PORT_MASK	0x6

static struct dwc3_device dwc3_device_data = {
#ifdef CONFIG_SPL_BUILD
	.maximum_speed = USB_SPEED_HIGH,
#else
	.maximum_speed = USB_SPEED_SUPER,
#endif
	.base = USB1_BASE_ADDR,
	.dr_mode = USB_DR_MODE_PERIPHERAL,
	.index = 0,
	.power_down_scale = 2,
};

int usb_gadget_handle_interrupts(void)
{
	dwc3_uboot_handle_interrupt(0);
	return 0;
}

static void dwc3_nxp_usb_phy_init(struct dwc3_device *dwc3)
{
	u32 RegData;

	RegData = readl(dwc3->base + USB_PHY_CTRL1);
	RegData &= ~(USB_PHY_CTRL1_VDATSRCENB0 | USB_PHY_CTRL1_VDATDETENB0 |
			USB_PHY_CTRL1_COMMONONN);
	RegData |= USB_PHY_CTRL1_RESET | USB_PHY_CTRL1_ATERESET;
	writel(RegData, dwc3->base + USB_PHY_CTRL1);

	RegData = readl(dwc3->base + USB_PHY_CTRL0);
	RegData |= USB_PHY_CTRL0_REF_SSP_EN;
	writel(RegData, dwc3->base + USB_PHY_CTRL0);

	RegData = readl(dwc3->base + USB_PHY_CTRL2);
	RegData |= USB_PHY_CTRL2_TXENABLEN0;
	writel(RegData, dwc3->base + USB_PHY_CTRL2);

	RegData = readl(dwc3->base + USB_PHY_CTRL1);
	RegData &= ~(USB_PHY_CTRL1_RESET | USB_PHY_CTRL1_ATERESET);
	writel(RegData, dwc3->base + USB_PHY_CTRL1);
}

#ifndef CONFIG_SPL_BUILD
static void configure_cc_switch() {
	uint8_t value;
	int ret;
	struct udevice *bus, *dev;

	ret = uclass_get_device_by_seq(UCLASS_I2C, PTN5150A_BUS, &bus);
	if (ret) {
		printf("%s: No bus %d\n", __func__, PTN5150A_BUS);
		return;
	}

	ret = dm_i2c_probe(bus, PTN5150A_ADDR, 0, &dev);
	if (ret) {
		printf("%s: Can't find device id=0x%x, on bus %d\n",
			__func__, PTN5150A_ADDR, PTN5150A_BUS);
		return;
	}

	if (dm_i2c_read(dev, PTN5150A_CONTROL, &value, 1)) {
		printf("Unable to configure USB switch");
		return;
	}

	value &= ~PTN5150A_CONTROL_PORT_MASK;
	if (dm_i2c_write(dev, PTN5150A_CONTROL, &value, 1)) {
		printf("Unable to configure USB switch");
		return;
	}
	printf("Configured USB Switch for UFP\n");
}
#endif
#endif

#if defined(CONFIG_USB_DWC3) || defined(CONFIG_USB_XHCI_IMX8M)
int board_usb_init(int index, enum usb_init_type init)
{
	imx8m_usb_power(index, true);

#ifndef CONFIG_SPL_BUILD
	/* Explicitly set USB switch to UFP (device) to ensure proper
	*fastboot operation on type-C ports. */
	configure_cc_switch();
#endif

	if (index == 0 && init == USB_INIT_DEVICE) {
		dwc3_nxp_usb_phy_init(&dwc3_device_data);
		return dwc3_uboot_init(&dwc3_device_data);
	}

	return 0;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	if (index == 0 && init == USB_INIT_DEVICE) {
		dwc3_uboot_exit(index);
	}

	imx8m_usb_power(index, false);

	return 0;
}
#endif

int board_init(void)
{
	board_qspi_init();

#ifdef CONFIG_FEC_MXC
	setup_fec();
#endif

#if defined(CONFIG_USB_DWC3) || defined(CONFIG_USB_XHCI_IMX8M)
	init_usb_clk();
#endif
	return 0;
}

int board_mmc_get_env_dev(int devno)
{
	return devno;
}

int board_late_init(void)
{
#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	env_set("board_name", "Phanbell");
	env_set("board_rev", "iMX8MQ");
#endif

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif
	int baseboard_id;
	baseboard_id = get_imx8m_baseboard_id();
	if ((baseboard_id == AIY_MICRON_1G) ||
			(baseboard_id == AIY_HYNIX_1G)) {
		/* 1G DDR size */
		env_set("bootargs_ram_capacity", "cma=296M galcore.contiguousSize=33554432 androidboot.displaymode=720p androidboot.lcd_density=160");
	} else {
		env_set("bootargs_ram_capacity", "cma=384M androidboot.lcd_density=213");
	}

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
