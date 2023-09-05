// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2018 - 2023 NXP
 *
 */

#include <common.h>
#include <cpu_func.h>
#include <hang.h>
#include <image.h>
#include <init.h>
#include <log.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/arch/ddr.h>
#include <asm/arch/imx8mq_pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/clock.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <fsl_esdhc_imx.h>
#include <fsl_sec.h>
#include <mmc.h>
#include <linux/delay.h>
#include <power/pmic.h>
#include <power/pfuze100_pmic.h>
#include <spl.h>
#include "../common/pfuze.h"
#include <asm/arch/imx8mq_sec_def.h>
#include <asm/arch/imx8m_csu.h>
#include <asm/arch/imx8m_rdc.h>

DECLARE_GLOBAL_DATA_PTR;

extern struct dram_timing_info dram_timing_b0;
extern struct dram_timing_info dram_timing_4g;

static void spl_dram_init(void)
{
	/* Check PCA6416A IO EXP on 4GB WEVK only */
#if IS_ENABLED(CONFIG_IMX8MQ_4GB_DDR_TIMING)
	I2C_SET_BUS(2);
	if (!i2c_probe(0x20)) {
		ddr_init(&dram_timing_4g);
		return;
	} else {
		panic("Fail to detect WEVK board but 4GB DDR is enabled\n");
	}
#else
	/* ddr init */
	if (soc_rev() >= CHIP_REV_2_1)
		ddr_init(&dram_timing);
	else
		ddr_init(&dram_timing_b0);
#endif
}

#define I2C_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_HYS | PAD_CTL_PUE)
#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)
static struct i2c_pads_info i2c_pad_info1 = {
	.scl = {
		.i2c_mode = IMX8MQ_PAD_I2C1_SCL__I2C1_SCL | PC,
		.gpio_mode = IMX8MQ_PAD_I2C1_SCL__GPIO5_IO14 | PC,
		.gp = IMX_GPIO_NR(5, 14),
	},
	.sda = {
		.i2c_mode = IMX8MQ_PAD_I2C1_SDA__I2C1_SDA | PC,
		.gpio_mode = IMX8MQ_PAD_I2C1_SDA__GPIO5_IO15 | PC,
		.gp = IMX_GPIO_NR(5, 15),
	},
};

static struct i2c_pads_info i2c_pad_info3 = {
	.scl = {
		.i2c_mode = IMX8MQ_PAD_I2C3_SCL__I2C3_SCL | PC,
		.gpio_mode = IMX8MQ_PAD_I2C3_SCL__GPIO5_IO18| PC,
		.gp = IMX_GPIO_NR(5, 18),
	},
	.sda = {
		.i2c_mode = IMX8MQ_PAD_I2C3_SDA__I2C3_SDA | PC,
		.gpio_mode = IMX8MQ_PAD_I2C3_SDA__GPIO5_IO19 | PC,
		.gp = IMX_GPIO_NR(5, 19),
	},
};

#define USDHC2_CD_GPIO	IMX_GPIO_NR(2, 12)
#define USDHC1_PWR_GPIO IMX_GPIO_NR(2, 10)
#define USDHC2_PWR_GPIO IMX_GPIO_NR(2, 19)

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;

	switch (cfg->esdhc_base) {
	case USDHC1_BASE_ADDR:
		ret = 1;
		break;
	case USDHC2_BASE_ADDR:
		ret = !gpio_get_value(USDHC2_CD_GPIO);
		return ret;
	}

	return 1;
}

#define USDHC_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_HYS | PAD_CTL_PUE | \
			 PAD_CTL_FSEL2)
#define USDHC_GPIO_PAD_CTRL (PAD_CTL_PUE | PAD_CTL_DSE1)

static iomux_v3_cfg_t const usdhc1_pads[] = {
	IMX8MQ_PAD_SD1_CLK__USDHC1_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MQ_PAD_SD1_CMD__USDHC1_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MQ_PAD_SD1_DATA0__USDHC1_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MQ_PAD_SD1_DATA1__USDHC1_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MQ_PAD_SD1_DATA2__USDHC1_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MQ_PAD_SD1_DATA3__USDHC1_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MQ_PAD_SD1_DATA4__USDHC1_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MQ_PAD_SD1_DATA5__USDHC1_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MQ_PAD_SD1_DATA6__USDHC1_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MQ_PAD_SD1_DATA7__USDHC1_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MQ_PAD_SD1_RESET_B__GPIO2_IO10 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static iomux_v3_cfg_t const usdhc2_pads[] = {
	IMX8MQ_PAD_SD2_CLK__USDHC2_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL), /* 0xd6 */
	IMX8MQ_PAD_SD2_CMD__USDHC2_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL), /* 0xd6 */
	IMX8MQ_PAD_SD2_DATA0__USDHC2_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL), /* 0xd6 */
	IMX8MQ_PAD_SD2_DATA1__USDHC2_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL), /* 0xd6 */
	IMX8MQ_PAD_SD2_DATA2__USDHC2_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL), /* 0x16 */
	IMX8MQ_PAD_SD2_DATA3__USDHC2_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL), /* 0xd6 */
	IMX8MQ_PAD_SD2_CD_B__GPIO2_IO12 | MUX_PAD_CTRL(USDHC_GPIO_PAD_CTRL),
	IMX8MQ_PAD_SD2_RESET_B__GPIO2_IO19 | MUX_PAD_CTRL(USDHC_GPIO_PAD_CTRL),
};

static struct fsl_esdhc_cfg usdhc_cfg[2] = {
	{USDHC1_BASE_ADDR, 0, 8},
	{USDHC2_BASE_ADDR, 0, 4},
};

int board_mmc_init(struct bd_info *bis)
{
	int i, ret;
	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-Boot device node)    (Physical Port)
	 * mmc0                    USDHC1
	 * mmc1                    USDHC2
	 */
	for (i = 0; i < CFG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			init_clk_usdhc(0);
			usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
			imx_iomux_v3_setup_multiple_pads(usdhc1_pads,
							 ARRAY_SIZE(usdhc1_pads));
			gpio_request(USDHC1_PWR_GPIO, "usdhc1_reset");
			gpio_direction_output(USDHC1_PWR_GPIO, 0);
			udelay(500);
			gpio_direction_output(USDHC1_PWR_GPIO, 1);
			break;
		case 1:
			init_clk_usdhc(1);
			usdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
			imx_iomux_v3_setup_multiple_pads(usdhc2_pads,
							 ARRAY_SIZE(usdhc2_pads));
			gpio_request(USDHC2_PWR_GPIO, "usdhc2_reset");
			gpio_direction_output(USDHC2_PWR_GPIO, 0);
			udelay(500);
			gpio_direction_output(USDHC2_PWR_GPIO, 1);
			break;
		default:
			printf("Warning: you configured more USDHC controllers(%d) than supported by the board\n", i + 1);
			return -EINVAL;
		}

		ret = fsl_esdhc_initialize(bis, &usdhc_cfg[i]);
		if (ret)
			return ret;
	}

	return 0;
}

#if CONFIG_IS_ENABLED(POWER_LEGACY)
#define I2C_PMIC	0
int power_init_board(void)
{
	struct pmic *p;
	int ret;
	unsigned int reg;

	ret = power_pfuze100_init(I2C_PMIC);
	if (ret)
		return -ENODEV;

	p = pmic_get("PFUZE100");
	ret = pmic_probe(p);
	if (ret)
		return -ENODEV;

	pmic_reg_read(p, PFUZE100_DEVICEID, &reg);
	printf("PMIC:  PFUZE100 ID=0x%02x\n", reg);

	pmic_reg_read(p, PFUZE100_SW3AVOL, &reg);
	if ((reg & 0x3f) != 0x18) {
		reg &= ~0x3f;
		reg |= 0x18;
		pmic_reg_write(p, PFUZE100_SW3AVOL, reg);
	}

	ret = pfuze_mode_init(p, APS_PFM);
	if (ret < 0)
		return ret;

	/* set SW3A standby mode to off */
	pmic_reg_read(p, PFUZE100_SW3AMODE, &reg);
	reg &= ~0xf;
	reg |= APS_OFF;
	pmic_reg_write(p, PFUZE100_SW3AMODE, reg);

	return 0;
}
#endif

void spl_board_init(void)
{
	if (IS_ENABLED(CONFIG_FSL_CAAM)) {
		if (sec_init())
			printf("\nsec_init failed!\n");
	}

	init_usb_clk();

	puts("Normal Boot\n");
}

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	/* Just empty function now - can't decide what to choose */
	debug("%s: %s\n", __func__, name);

	return 0;
}
#endif

#define GPR_PCIE_VREG_BYPASS	BIT(12)
static void enable_pcie_vreg(bool enable)
{
	struct iomuxc_gpr_base_regs *gpr =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	if (!enable) {
		setbits_le32(&gpr->gpr[14], GPR_PCIE_VREG_BYPASS);
		setbits_le32(&gpr->gpr[16], GPR_PCIE_VREG_BYPASS);
	} else {
		clrbits_le32(&gpr->gpr[14], GPR_PCIE_VREG_BYPASS);
		clrbits_le32(&gpr->gpr[16], GPR_PCIE_VREG_BYPASS);
	}
}

void board_init_f(ulong dummy)
{
	int ret;

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	/* PCIE_VPH connects to 3.3v on EVK, enable VREG to generate 1.8V to PHY */
	enable_pcie_vreg(true);

	arch_cpu_init();

	init_uart_clk(0);

	board_early_init_f();

	timer_init();

	preloader_console_init();

	ret = spl_init();
	if (ret) {
		debug("spl_init() failed: %d\n", ret);
		hang();
	}

	enable_tzc380();

	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info1);
	setup_i2c(2, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info3);

	power_init_board();

	/* DDR initialization */
	spl_dram_init();

	board_init_r(NULL, 0);
}

#ifdef CONFIG_ANDROID_SUPPORT
void spl_board_prepare_for_boot(void)
{
	uint32_t val;
	struct imx_csu_cfg csu_cfg[] = {
		/* peripherals csl setting */
		CSU_CSLx(CSU_CSL_OCRAM, CSU_SEC_LEVEL_2, LOCKED),
		CSU_CSLx(CSU_CSL_OCRAM_S, CSU_SEC_LEVEL_2, LOCKED),
		CSU_CSLx(CSU_CSL_RDC, CSU_SEC_LEVEL_3, LOCKED),
		CSU_CSLx(CSU_CSL_TZASC, CSU_SEC_LEVEL_4, LOCKED),

		/* SA setting */
		CSU_SA(CSU_SA_M4, 1, LOCKED),
		CSU_SA(CSU_SA_SDMA1, 1, LOCKED),
		CSU_SA(CSU_SA_LCDIF, 1, LOCKED),
		CSU_SA(CSU_SA_USB, 1, LOCKED),
		CSU_SA(CSU_SA_PCIE_CTRL, 1, LOCKED),
		CSU_SA(CSU_SA_VPU_DECODER, 1, LOCKED),
		CSU_SA(CSU_SA_GPU, 1, LOCKED),
		CSU_SA(CSU_SA_ENET1, 1, LOCKED),
		CSU_SA(CSU_SA_USDHC1, 1, LOCKED),
		CSU_SA(CSU_SA_USDHC2, 1, LOCKED),
		CSU_SA(CSU_SA_DISPLAY_CONTROLLER, 1, LOCKED),
		CSU_SA(CSU_SA_HUGO, 1, LOCKED),
		CSU_SA(CSU_SA_DAP, 1, LOCKED),
		CSU_SA(CSU_SA_SDMA2, 1, LOCKED),
#ifdef CONFIG_IMX_TRUSTY_OS
		CSU_CSLx(CSU_CSL_VPU_SEC, CSU_SEC_LEVEL_5, LOCKED),
#endif
		{0}
	};

	struct imx_rdc_cfg rdc_cfg[] = {
		RDC_MDAn(RDC_MDA_DCSS, DID2),
		{0},
	};

	/* csu config */
	imx_csu_init(csu_cfg);

	/* rdc config */
	imx_rdc_init(rdc_cfg);

	/* config the ocram memory range for secure access */
	setbits_le32(IOMUXC_GPR_BASE_ADDR + 0x2c, 0x421);
	val = readl(IOMUXC_GPR_BASE_ADDR + 0x2c);
	setbits_le32(IOMUXC_GPR_BASE_ADDR + 0x2c, val | 0x3C3F0000);
}
#endif
