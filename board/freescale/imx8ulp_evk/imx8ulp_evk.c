// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2020 NXP
 */

#include <common.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/arch/imx8ulp-pins.h>
#include <asm/arch/clock.h>
#include <asm/arch/pcc.h>
#include <asm/arch/sys_proto.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/gpio.h>
#include <i2c.h>
#include <power-domain.h>
#include <dt-bindings/power/imx8ulp-power.h>

DECLARE_GLOBAL_DATA_PTR;
#if defined(CONFIG_NXP_FSPI) || defined(CONFIG_FSL_FSPI_NAND)
#define FSPI_PAD_CTRL	(PAD_CTL_PUS_UP | PAD_CTL_DSE)
static iomux_cfg_t const flexspi0_pads[] = {
	IMX8ULP_PAD_PTC5__FLEXSPI0_A_SS0_b | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTC6__FLEXSPI0_A_SCLK | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTC10__FLEXSPI0_A_DATA0 | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTC9__FLEXSPI0_A_DATA1 | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTC8__FLEXSPI0_A_DATA2 | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTC7__FLEXSPI0_A_DATA3 | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTC4__FLEXSPI0_A_DATA4 | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTC3__FLEXSPI0_A_DATA5 | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTC2__FLEXSPI0_A_DATA6 | MUX_PAD_CTRL(FSPI_PAD_CTRL),
	IMX8ULP_PAD_PTC1__FLEXSPI0_A_DATA7 | MUX_PAD_CTRL(FSPI_PAD_CTRL),
};

static void setup_flexspi(void)
{
	init_clk_fspi(0);
}

static void setup_rtd_flexspi0(void)
{
	imx8ulp_iomux_setup_multiple_pads(flexspi0_pads, ARRAY_SIZE(flexspi0_pads));

	/* Set PCC of flexspi0, 192Mhz % 4 = 48Mhz */
	writel(0xD6000003, 0x280300e4);
}

#endif

#if IS_ENABLED(CONFIG_FEC_MXC)
#define ENET_CLK_PAD_CTRL	(PAD_CTL_PUS_UP | PAD_CTL_DSE | PAD_CTL_IBE_ENABLE)
static iomux_cfg_t const enet_clk_pads[] = {
	IMX8ULP_PAD_PTE19__ENET0_REFCLK | MUX_PAD_CTRL(ENET_CLK_PAD_CTRL),
	IMX8ULP_PAD_PTF10__ENET0_1588_CLKIN | MUX_PAD_CTRL(ENET_CLK_PAD_CTRL),
};

static int setup_fec(void)
{
	/*
	 * Since ref clock and timestamp clock are from external,
	 * set the iomux prior the clock enablement
	 */
	imx8ulp_iomux_setup_multiple_pads(enet_clk_pads, ARRAY_SIZE(enet_clk_pads));

	/* Select enet time stamp clock: 001 - External Timestamp Clock */
	cgc1_enet_stamp_sel(1);

	/* enable FEC PCC */
	pcc_clock_enable(4, ENET_PCC4_SLOT, true);
	pcc_reset_peripheral(4, ENET_PCC4_SLOT, false);

	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
	if (phydev->drv->config)
		phydev->drv->config(phydev);
	return 0;
}
#endif

#define I2C_PAD_CTRL	(PAD_CTL_ODE)
static const iomux_cfg_t lpi2c0_pads[] = {
	IMX8ULP_PAD_PTA8__LPI2C0_SCL | MUX_PAD_CTRL(I2C_PAD_CTRL),
	IMX8ULP_PAD_PTA9__LPI2C0_SDA | MUX_PAD_CTRL(I2C_PAD_CTRL),
};

#define TPM_PAD_CTRL	(PAD_CTL_DSE)
static const iomux_cfg_t tpm0_pads[] = {
	IMX8ULP_PAD_PTA3__TPM0_CH2 | MUX_PAD_CTRL(TPM_PAD_CTRL),
};

void mipi_dsi_mux_panel(void)
{
	int ret;
	struct gpio_desc desc;

	/* It is temp solution to directly access i2c, need change to rpmsg later */

	/* enable lpi2c0 clock and iomux */
	imx8ulp_iomux_setup_multiple_pads(lpi2c0_pads, ARRAY_SIZE(lpi2c0_pads));
	writel(0xD2000000, 0x28091060);

	ret = dm_gpio_lookup_name("gpio@20_9", &desc);
	if (ret) {
		printf("%s lookup gpio@20_9 failed ret = %d\n", __func__, ret);
		return;
	}

	ret = dm_gpio_request(&desc, "dsi_mux");
	if (ret) {
		printf("%s request dsi_mux failed ret = %d\n", __func__, ret);
		return;
	}

	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);
}

void mipi_dsi_panel_backlight(void)
{
	/* It is temp solution to directly access pwm, need change to rpmsg later */
	imx8ulp_iomux_setup_multiple_pads(tpm0_pads, ARRAY_SIZE(tpm0_pads));
	writel(0xD4000001, 0x28091054);

	/* Use center-aligned PWM mode, CPWMS=1, MSnB:MSnA = 10, ELSnB:ELSnA = 00 */
	writel(1000, 0x28095018);
	writel(1000, 0x28095034); /* MOD = CV, full duty */
	writel(0x28, 0x28095010);
	writel(0x20, 0x28095030);
}

void reset_lsm6dsx(uint8_t i2c_bus, uint8_t addr)
{
	struct udevice *bus;
	struct udevice *i2c_dev = NULL;
	int ret;
	struct i2c_msg msg;
	u8 i2c_buf[2] = { 0x12, 0x1 };

	ret = uclass_get_device_by_seq(UCLASS_I2C, i2c_bus, &bus);
	if (ret) {
		printf("%s: Can't find bus\n", __func__);
		return;
	}

	ret = dm_i2c_probe(bus, addr, 0, &i2c_dev);
	if (ret) {
		printf("%s: Can't find device id=0x%x\n",
			__func__, addr);
		return;
	}

	msg.addr = addr;
	msg.flags = 0;
	msg.len = 2;
	msg.buf = i2c_buf;

	ret = dm_i2c_xfer(i2c_dev, &msg, 1);
	if (!ret)
		printf("%s: Reset device 0x%x successfully.\n", __func__, addr);
}

int board_init(void)
{
#if defined(CONFIG_NXP_FSPI) || defined(CONFIG_FSL_FSPI_NAND)
	setup_flexspi();

	if (get_boot_mode() == SINGLE_BOOT) {
		setup_rtd_flexspi0();
	}
#endif

#if defined(CONFIG_FEC_MXC)
	setup_fec();
#endif

	/* When sync with M33 is failed, use local driver to set for video */
	if (!is_m33_handshake_necessary() && IS_ENABLED(CONFIG_VIDEO)) {
		mipi_dsi_mux_panel();
		mipi_dsi_panel_backlight();
	}

	return 0;
}

int board_early_init_f(void)
{
	return 0;
}

int board_late_init(void)
{
	ulong addr;

#if CONFIG_IS_ENABLED(ENV_IS_IN_MMC)
	board_late_mmc_env_init();
#endif

	env_set("sec_boot", "no");
#ifdef CONFIG_AHAB_BOOT
	env_set("sec_boot", "yes");
#endif

#ifdef CONFIG_SYS_I2C_IMX_I3C
	reset_lsm6dsx(8, 0x9);
#endif

	/* clear fdtaddr to avoid obsolete data */
	addr = env_get_hex("fdt_addr_r", 0);
	if (addr)
		memset((void *)addr, 0, 0x400);

	return 0;
}

#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_ANDROID_RECOVERY
#ifdef CONFIG_TARGET_IMX8ULP_EVK
static iomux_cfg_t const recovery_pad[] = {
	IMX8ULP_PAD_PTF7__PTF7 | MUX_PAD_CTRL(PAD_CTL_IBE_ENABLE),
};
#endif
int is_recovery_key_pressing(void)
{
#ifdef CONFIG_TARGET_IMX8ULP_EVK
	int ret;
	struct gpio_desc desc;

	imx8ulp_iomux_setup_multiple_pads(recovery_pad, ARRAY_SIZE(recovery_pad));

	ret = dm_gpio_lookup_name("GPIO3_7", &desc);
	if (ret) {
		printf("%s lookup GPIO3_7 failed ret = %d\n", __func__, ret);
		return 0;
	}

	ret = dm_gpio_request(&desc, "recovery");
	if (ret) {
		printf("%s request recovery pad failed ret = %d\n", __func__, ret);
		return 0;
	}

	dm_gpio_set_dir_flags(&desc, GPIOD_IS_IN);

	ret = dm_gpio_get_value(&desc);
	if (ret < 0) {
                printf("%s error in retrieving GPIO value ret = %d\n", __func__, ret);
                return 0;
        }

	dm_gpio_free(desc.dev, &desc);

	return !ret;
#else
	return 0;
#endif
}
#endif /*CONFIG_ANDROID_RECOVERY*/
#endif /*CONFIG_FSL_FASTBOOT*/


void board_quiesce_devices(void)
{
	/* Disable the power domains may used in u-boot before entering kernel */
#if CONFIG_IS_ENABLED(POWER_DOMAIN)
	struct udevice *scmi_devpd;
	int ret, i;
	struct power_domain pd;
	ulong ids[] = {
		IMX8ULP_PD_FLEXSPI2, IMX8ULP_PD_USB0, IMX8ULP_PD_USDHC0,
		IMX8ULP_PD_USDHC1, IMX8ULP_PD_USDHC2_USB1, IMX8ULP_PD_DCNANO,
		IMX8ULP_PD_MIPI_DSI};

	ret = uclass_get_device(UCLASS_POWER_DOMAIN, 0, &scmi_devpd);
	if (ret) {
		printf("Cannot get scmi devpd: err=%d\n", ret);
		return;
	}

	pd.dev = scmi_devpd;

	for (i = 0; i < ARRAY_SIZE(ids); i++) {
		pd.id = ids[i];
		ret = power_domain_off(&pd);
		if (ret)
			printf("power_domain_off %lu failed: err=%d\n", ids[i], ret);
	}
#endif
}
