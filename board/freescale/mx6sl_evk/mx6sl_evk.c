/*
 * Copyright (C) 2010-2012 Freescale Semiconductor, Inc.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/mx6.h>
#include <asm/arch/mx6_pins.h>
#include <asm/arch/mx6sl_pins.h>
#if defined(CONFIG_SECURE_BOOT)
#include <asm/arch/mx6_secure.h>
#endif
#include <asm/arch/iomux-v3.h>
#include <asm/arch/regs-anadig.h>
#include <asm/errno.h>
#include <imx_wdog.h>
#ifdef CONFIG_MXC_FEC
#include <miiphy.h>
#endif

#if defined(CONFIG_MXC_EPDC)
#include <lcd.h>
#endif

#ifdef CONFIG_IMX_ECSPI
#include <imx_spi.h>
#endif

#ifdef CONFIG_CMD_MMC
#include <mmc.h>
#include <fsl_esdhc.h>
#endif

#ifdef CONFIG_MXC_GPIO
#include <asm/gpio.h>
#include <asm/arch/gpio.h>
#endif

#ifdef CONFIG_ANDROID_RECOVERY
#include <recovery.h>
#endif

#if CONFIG_I2C_MXC
#include <i2c.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

static u32 system_rev;
static enum boot_device boot_dev;

#define USB_OTG_PWR IMX_GPIO_NR(4, 0)
#define USB_H1_PWR IMX_GPIO_NR(4, 2)

static inline void setup_boot_device(void)
{
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
	uint bt_mem_ctl = (soc_sbmr & 0x000000FF) >> 4;
	uint bt_mem_type = (soc_sbmr & 0x00000008) >> 3;

	switch (bt_mem_ctl) {
	case 0x0:
		if (bt_mem_type)
			boot_dev = ONE_NAND_BOOT;
		else
			boot_dev = WEIM_NOR_BOOT;
		break;
	case 0x3:
		if (bt_mem_type)
			boot_dev = I2C_BOOT;
		else
			boot_dev = SPI_NOR_BOOT;
		break;
	case 0x4:
	case 0x5:
		boot_dev = SD_BOOT;
		break;
	case 0x6:
	case 0x7:
		boot_dev = MMC_BOOT;
		break;
		break;
	default:
		boot_dev = UNKNOWN_BOOT;
		break;
	}
}

enum boot_device get_boot_device(void)
{
	return boot_dev;
}

u32 get_board_rev(void)
{

	system_rev = 0x60000 | BOARD_REV_3; /* means revB: EVK */

	return system_rev;
}

int dram_init(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;

	return 0;
}

static void setup_uart(void)
{
	/* UART1 TXD */
	mxc_iomux_v3_setup_pad(MX6SL_PAD_UART1_TXD__UART1_TXD);

	/* UART1 RXD */
	mxc_iomux_v3_setup_pad(MX6SL_PAD_UART1_RXD__UART1_RXD);
}

#ifdef CONFIG_NET_MULTI
int board_eth_init(bd_t *bis)
{
	int rc = -ENODEV;

	return rc;
}
#endif

#ifdef CONFIG_CMD_MMC

/* On this board, only SD3 can support 1.8V signalling
 * that is required for UHS-I mode of operation.
 * Last element in struct is used to indicate 1.8V support.
 */
struct fsl_esdhc_cfg usdhc_cfg[3] = {
	{USDHC1_BASE_ADDR, 1, 1, 1, 1},
	{USDHC2_BASE_ADDR, 1, 1, 1, 1},
	{USDHC3_BASE_ADDR, 1, 1, 1, 1},
};

#ifdef CONFIG_DYNAMIC_MMC_DEVNO
int get_mmc_env_devno(void)
{
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);

	if (SD_BOOT == boot_dev || MMC_BOOT == boot_dev) {
		/* BOOT_CFG2[3] and BOOT_CFG2[4] */
		return (soc_sbmr & 0x00001800) >> 11;
	} else
		return -1;

}
#endif

iomux_v3_cfg_t usdhc1_pads[] = {
	/* 8 bit SD */
	MX6SL_PAD_SD1_CLK__USDHC1_CLK,
	MX6SL_PAD_SD1_CMD__USDHC1_CMD,
	MX6SL_PAD_SD1_DAT0__USDHC1_DAT0,
	MX6SL_PAD_SD1_DAT1__USDHC1_DAT1,
	MX6SL_PAD_SD1_DAT2__USDHC1_DAT2,
	MX6SL_PAD_SD1_DAT3__USDHC1_DAT3,
	MX6SL_PAD_SD1_DAT4__USDHC1_DAT4,
	MX6SL_PAD_SD1_DAT5__USDHC1_DAT5,
	MX6SL_PAD_SD1_DAT6__USDHC1_DAT6,
	MX6SL_PAD_SD1_DAT7__USDHC1_DAT7,
};

iomux_v3_cfg_t usdhc2_pads[] = {
	/* boot SD */
	MX6SL_PAD_SD2_CLK__USDHC2_CLK,
	MX6SL_PAD_SD2_CMD__USDHC2_CMD,
	MX6SL_PAD_SD2_DAT0__USDHC2_DAT0,
	MX6SL_PAD_SD2_DAT1__USDHC2_DAT1,
	MX6SL_PAD_SD2_DAT2__USDHC2_DAT2,
	MX6SL_PAD_SD2_DAT3__USDHC2_DAT3,
};

iomux_v3_cfg_t usdhc3_pads[] = {
	MX6SL_PAD_SD3_CLK__USDHC3_CLK,
	MX6SL_PAD_SD3_CMD__USDHC3_CMD,
	MX6SL_PAD_SD3_DAT0__USDHC3_DAT0,
	MX6SL_PAD_SD3_DAT1__USDHC3_DAT1,
	MX6SL_PAD_SD3_DAT2__USDHC3_DAT2,
	MX6SL_PAD_SD3_DAT3__USDHC3_DAT3,
};

int usdhc_gpio_init(bd_t *bis)
{
	s32 status = 0;
	u32 index = 0;

	for (index = 0; index < CONFIG_SYS_FSL_USDHC_NUM; ++index) {
		switch (index) {
		case 0:
			mxc_iomux_v3_setup_multiple_pads(usdhc1_pads,
							ARRAY_SIZE(usdhc1_pads));
			break;
		case 1:
			mxc_iomux_v3_setup_multiple_pads(usdhc2_pads,
							ARRAY_SIZE(usdhc2_pads));
			break;
		case 2:
			mxc_iomux_v3_setup_multiple_pads(usdhc3_pads,
							ARRAY_SIZE(usdhc3_pads));
			break;
		default:
			printf("Warning: you configured more USDHC controllers"
			       "(%d) then supported by the board (%d)\n",
			       index + 1, CONFIG_SYS_FSL_USDHC_NUM);
			return status;
		}
		status |= fsl_esdhc_initialize(bis, &usdhc_cfg[index]);
	}

	return status;
}

int board_mmc_init(bd_t *bis)
{
	if (!usdhc_gpio_init(bis))
		return 0;
	else
		return -1;
}

#ifdef CONFIG_MXC_EPDC
#ifdef CONFIG_SPLASH_SCREEN
int setup_splash_img()
{
#ifdef CONFIG_SPLASH_IS_IN_MMC
	int mmc_dev = get_mmc_env_devno();
	ulong offset = CONFIG_SPLASH_IMG_OFFSET;
	ulong size = CONFIG_SPLASH_IMG_SIZE;
	ulong addr = 0;
	char *s = NULL;
	struct mmc *mmc = find_mmc_device(mmc_dev);
	uint blk_start, blk_cnt, n;

	s = getenv("splashimage");

	if (NULL == s) {
		puts("env splashimage not found!\n");
		return -1;
	}
	addr = simple_strtoul(s, NULL, 16);

	if (!mmc) {
		printf("MMC Device %d not found\n", mmc_dev);
		return -1;
	}

	if (mmc_init(mmc)) {
		puts("MMC init failed\n");
		return -1;
	}

	blk_start = ALIGN(offset, mmc->read_bl_len) / mmc->read_bl_len;
	blk_cnt = ALIGN(size, mmc->read_bl_len) / mmc->read_bl_len;
	n = mmc->block_dev.block_read(mmc_dev, blk_start,
				      blk_cnt, (u_char *) addr);
	flush_cache((ulong) addr, blk_cnt * mmc->read_bl_len);

	return (n == blk_cnt) ? 0 : -1;
#endif
}
#endif

vidinfo_t panel_info = {
	.vl_refresh = 85,
	.vl_col = 800,
	.vl_row = 600,
	.vl_pixclock = 26666667,
	.vl_left_margin = 8,
	.vl_right_margin = 100,
	.vl_upper_margin = 4,
	.vl_lower_margin = 8,
	.vl_hsync = 4,
	.vl_vsync = 1,
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
	.gdclk_hp_offs = 419,
	.gdsp_offs = 20,
	.gdoe_offs = 0,
	.gdclk_offs = 5,
	.num_ce = 1,
};

static void setup_epdc_power()
{
	unsigned int reg;

	/* Setup epdc voltage */

	/* EPDC_PWRSTAT - GPIO2[13] for PWR_GOOD status */
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_PWRSTAT__GPIO_2_13);

	/* EPDC_VCOM0 - GPIO2[3] for VCOM control */
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_VCOM0__GPIO_2_3);

	/* Set as output */
	reg = readl(GPIO2_BASE_ADDR + GPIO_GDIR);
	reg |= (1 << 3);
	writel(reg, GPIO2_BASE_ADDR + GPIO_GDIR);

	/* EPDC_PWRWAKEUP - GPIO2[14] for EPD PMIC WAKEUP */
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_PWRWAKEUP__GPIO_2_14);
	/* Set as output */
	reg = readl(GPIO2_BASE_ADDR + GPIO_GDIR);
	reg |= (1 << 14);
	writel(reg, GPIO2_BASE_ADDR + GPIO_GDIR);

	/* EPDC_PWRCTRL0 - GPIO2[7] for EPD PWR CTL0 */
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_PWRCTRL0__GPIO_2_7);
	/* Set as output */
	reg = readl(GPIO2_BASE_ADDR + GPIO_GDIR);
	reg |= (1 << 7);
	writel(reg, GPIO2_BASE_ADDR + GPIO_GDIR);
}

void epdc_power_on()
{
	unsigned int reg;

	/* Set EPD_PWR_CTL0 to high - enable EINK_VDD (3.15) */
	reg = readl(GPIO2_BASE_ADDR + GPIO_DR);
	reg |= (1 << 7);
	writel(reg, GPIO2_BASE_ADDR + GPIO_DR);

	/* Set PMIC Wakeup to high - enable Display power */
	reg = readl(GPIO2_BASE_ADDR + GPIO_DR);
	reg |= (1 << 14);
	writel(reg, GPIO2_BASE_ADDR + GPIO_DR);

	/* Wait for PWRGOOD == 1 */
	while (1) {
		reg = readl(GPIO2_BASE_ADDR + GPIO_DR);
		if (!(reg & (1 << 13)))
			break;

		udelay(100);
	}

	/* Enable VCOM */
	reg = readl(GPIO2_BASE_ADDR + GPIO_DR);
	reg |= (1 << 3);
	writel(reg, GPIO2_BASE_ADDR + GPIO_DR);

	reg = readl(GPIO2_BASE_ADDR + GPIO_DR);

	udelay(500);
}

void epdc_power_off()
{
	unsigned int reg;
	/* Set PMIC Wakeup to low - disable Display power */
	reg = readl(GPIO2_BASE_ADDR + GPIO_DR);
	reg &= ~(1 << 14);
	writel(reg, GPIO2_BASE_ADDR + GPIO_DR);

	/* Disable VCOM */
	reg = readl(GPIO2_BASE_ADDR + GPIO_DR);
	reg &= ~(1 << 3);
	writel(reg, GPIO2_BASE_ADDR + GPIO_DR);

	/* Set EPD_PWR_CTL0 to low - disable EINK_VDD (3.15) */
	reg = readl(GPIO2_BASE_ADDR + GPIO_DR);
	reg &= ~(1 << 7);
	writel(reg, GPIO2_BASE_ADDR + GPIO_DR);
}

int setup_waveform_file()
{
#ifdef CONFIG_WAVEFORM_FILE_IN_MMC
	int mmc_dev = get_mmc_env_devno();
	ulong offset = CONFIG_WAVEFORM_FILE_OFFSET;
	ulong size = CONFIG_WAVEFORM_FILE_SIZE;
	ulong addr = CONFIG_WAVEFORM_BUF_ADDR;
	char *s = NULL;
	struct mmc *mmc = find_mmc_device(mmc_dev);
	uint blk_start, blk_cnt, n;

	if (!mmc) {
		printf("MMC Device %d not found\n", mmc_dev);
		return -1;
	}

	if (mmc_init(mmc)) {
		puts("MMC init failed\n");
		return -1;
	}

	blk_start = ALIGN(offset, mmc->read_bl_len) / mmc->read_bl_len;
	blk_cnt = ALIGN(size, mmc->read_bl_len) / mmc->read_bl_len;
	n = mmc->block_dev.block_read(mmc_dev, blk_start,
				      blk_cnt, (u_char *) addr);
	flush_cache((ulong) addr, blk_cnt * mmc->read_bl_len);

	return (n == blk_cnt) ? 0 : -1;
#else
	return -1;
#endif
}

static void setup_epdc()
{
	unsigned int reg;

	/* epdc iomux settings */
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_D0__EPDC_SDDO_0);
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_D1__EPDC_SDDO_1);
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_D2__EPDC_SDDO_2);
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_D3__EPDC_SDDO_3);
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_D4__EPDC_SDDO_4);
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_D5__EPDC_SDDO_5);
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_D6__EPDC_SDDO_6);
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_D7__EPDC_SDDO_7);
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_GDCLK__EPDC_GDCLK);
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_GDSP__EPDC_GDSP);
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_GDOE__EPDC_GDOE);
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_GDRL__EPDC_GDRL);
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_SDCLK__EPDC_SDCLK);
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_SDOE__EPDC_SDOE);
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_SDLE__EPDC_SDLE);
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_SDSHR__EPDC_SDSHR);
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_BDR0__EPDC_BDR_0);
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_SDCE0__EPDC_SDCE_0);
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_SDCE1__EPDC_SDCE_1);
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_SDCE2__EPDC_SDCE_2);

	/*** epdc Maxim PMIC settings ***/

	/* EPDC PWRSTAT - GPIO2[13] for PWR_GOOD status */
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_PWRSTAT__GPIO_2_13);

	/* EPDC VCOM0 - GPIO2[3] for VCOM control */
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_VCOM0__GPIO_2_3);

	/* UART4 TXD - GPIO2[14] for EPD PMIC WAKEUP */
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_PWRWAKEUP__GPIO_2_14);

	/* EIM_A18 - GPIO2[7] for EPD PWR CTL0 */
	mxc_iomux_v3_setup_pad(MX6SL_PAD_EPDC_PWRCTRL0__GPIO_2_7);

	/*** Set pixel clock rates for EPDC ***/

	/* EPDC AXI clk from PFD_400M, set to 396/2 = 198MHz */
	reg = readl(CCM_BASE_ADDR + CLKCTL_CHSCCDR);
	reg &= ~0x3F000;
	reg |= (0x4 << 15) | (1 << 12);
	writel(reg, CCM_BASE_ADDR + CLKCTL_CHSCCDR);

	/* EPDC AXI clk enable */
	reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR3);
	reg |= 0x0030;
	writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR3);

	/* EPDC PIX clk from PFD_540M, set to 540/4/5 = 27MHz */
	reg = readl(CCM_BASE_ADDR + CLKCTL_CSCDR2);
	reg &= ~0x03F000;
	reg |= (0x5 << 15) | (4 << 12);
	writel(reg, CCM_BASE_ADDR + CLKCTL_CSCDR2);

	reg = readl(CCM_BASE_ADDR + CLKCTL_CBCMR);
	reg &= ~0x03800000;
	reg |= (0x3 << 23);
	writel(reg, CCM_BASE_ADDR + CLKCTL_CBCMR);

	/* EPDC PIX clk enable */
	reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR3);
	reg |= 0x0C00;
	writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR3);

	panel_info.epdc_data.working_buf_addr = CONFIG_WORKING_BUF_ADDR;
	panel_info.epdc_data.waveform_buf_addr = CONFIG_WAVEFORM_BUF_ADDR;

	panel_info.epdc_data.wv_modes.mode_init = 0;
	panel_info.epdc_data.wv_modes.mode_du = 1;
	panel_info.epdc_data.wv_modes.mode_gc4 = 3;
	panel_info.epdc_data.wv_modes.mode_gc8 = 2;
	panel_info.epdc_data.wv_modes.mode_gc16 = 2;
	panel_info.epdc_data.wv_modes.mode_gc32 = 2;

	panel_info.epdc_data.epdc_timings = panel_timings;

	setup_epdc_power();

	/* Assign fb_base */
	gd->fb_base = CONFIG_FB_BASE;
}
#endif

/* For DDR mode operation, provide target delay parameter for each SD port.
 * Use cfg->esdhc_base to distinguish the SD port #. The delay for each port
 * is dependent on signal layout for that particular port.  If the following
 * CONFIG is not defined, then the default target delay value will be used.
 */
#ifdef CONFIG_GET_DDR_TARGET_DELAY
u32 get_ddr_delay(struct fsl_esdhc_cfg *cfg)
{
	/* No delay required on EVK board SD ports */
	return 0;
}
#endif
#endif

#ifdef CONFIG_IMX_ECSPI
s32 spi_get_cfg(struct imx_spi_dev_t *dev)
{
	switch (dev->slave.cs) {
	case 0:
		/* SPI-NOR */
		dev->base = ECSPI1_BASE_ADDR;
		dev->freq = 25000000;
		dev->ss_pol = IMX_SPI_ACTIVE_LOW;
		dev->ss = 0;
		dev->fifo_sz = 64 * 4;
		dev->us_delay = 0;
		break;
	default:
		printf("Invalid Bus ID!\n");
		break;
	}

	return 0;
}

void spi_io_init(struct imx_spi_dev_t *dev)
{
	u32 reg;

	switch (dev->base) {
	case ECSPI1_BASE_ADDR:
		/* Enable clock */
		reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR1);
		reg |= 0x3;
		writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR1);
		/* SCLK */
		mxc_iomux_v3_setup_pad(MX6SL_PAD_ECSPI1_SCLK__ECSPI1_SCLK);

		/* MISO */
		mxc_iomux_v3_setup_pad(MX6SL_PAD_ECSPI1_MISO__ECSPI1_MISO);

		/* MOSI */
		mxc_iomux_v3_setup_pad(MX6SL_PAD_ECSPI1_MOSI__ECSPI1_MOSI);

		if (dev->ss == 0)
			mxc_iomux_v3_setup_pad
			    (MX6SL_PAD_ECSPI1_SS0__ECSPI1_SS0);
		break;
	case ECSPI2_BASE_ADDR:
	case ECSPI3_BASE_ADDR:
		/* ecspi2-3 fall through */
		break;
	default:
		break;
	}
}
#endif

#ifdef CONFIG_MXC_FEC
iomux_v3_cfg_t enet_pads[] = {
	/* LAN8720A */
	MX6SL_PAD_FEC_MDIO__FEC_MDIO,
	MX6SL_PAD_FEC_MDC__FEC_MDC,
	MX6SL_PAD_FEC_RXD0__FEC_RDATA_0,
	MX6SL_PAD_FEC_RXD1__FEC_RDATA_1,
	MX6SL_PAD_FEC_CRS_DV__FEC_RX_DV,
	MX6SL_PAD_FEC_TXD0__FEC_TDATA_0,
	MX6SL_PAD_FEC_TXD1__FEC_TDATA_1,
	MX6SL_PAD_FEC_TX_EN__FEC_TX_EN,
#ifdef CONFIG_FEC_CLOCK_FROM_ANATOP
	MX6SL_PAD_FEC_REF_CLK__FEC_REF_OUT,	/* clock from anatop */
#else
	MX6SL_PAD_FEC_REF_CLK__GPIO_4_26,	/* clock from OSC */
#endif

	/*
	 * Since FEC_RX_ER is not connected with PHY(LAN8720A), we need
	 * either configure FEC_RX_ER PAD to other mode than FEC_RX_ER,
	 * or configure FEC_RX_ER PAD to FEC_RX_ER but need pull it down,
	 * otherwise, FEC MAC will report CRC error always. We configure
	 * FEC_RX_ER PAD to GPIO mode here.
	 */

	MX6SL_PAD_FEC_RX_ER__GPIO_4_19,
	MX6SL_PAD_FEC_TX_CLK__GPIO_4_21,	/* Phy power enable */
};

void enet_board_init(void)
{
	unsigned int reg;
	mxc_iomux_v3_setup_multiple_pads(enet_pads, ARRAY_SIZE(enet_pads));

	/*set GPIO4_26 input as FEC clock */
	reg = readl(GPIO4_BASE_ADDR + 0x04);
	reg &= ~(1 << 26);
	writel(reg, GPIO4_BASE_ADDR + 0x4);

	/* phy power enable and reset: gpio4_21 */
	/* DR: High Level on: Power ON */
	reg = readl(GPIO4_BASE_ADDR + 0x0);
	reg |= (1 << 21);
	writel(reg, GPIO4_BASE_ADDR + 0x0);

	/* DIR: output */
	reg = readl(GPIO4_BASE_ADDR + 0x4);
	reg |= (1 << 21);
	writel(reg, GPIO4_BASE_ADDR + 0x4);

	/* wait RC ms for hw reset */
	udelay(500);
}

#define ANATOP_PLL_LOCK                 0x80000000
#define ANATOP_PLL_PWDN_MASK            0x00001000
#define ANATOP_PLL_BYPASS_MASK          0x00010000
#define ANATOP_FEC_PLL_ENABLE_MASK      0x00002000

static int setup_fec(void)
{
	u32 reg = 0;
	s32 timeout = 100000;

	/* get enet tx reference clk from internal clock from anatop
	 * GPR1[14] = 0, GPR1[18:17] = 00
	 */
	reg = readl(IOMUXC_BASE_ADDR + 0x4);
	reg &= ~(0x3 << 17);
	reg &= ~(0x1 << 14);
	writel(reg, IOMUXC_BASE_ADDR + 0x4);

#ifdef CONFIG_FEC_CLOCK_FROM_ANATOP
	/* Enable PLLs */
	reg = readl(ANATOP_BASE_ADDR + 0xe0);	/* ENET PLL */
	if ((reg & ANATOP_PLL_PWDN_MASK) || (!(reg & ANATOP_PLL_LOCK))) {
		reg &= ~ANATOP_PLL_PWDN_MASK;
		writel(reg, ANATOP_BASE_ADDR + 0xe0);
		while (timeout--) {
			if (readl(ANATOP_BASE_ADDR + 0xe0) & ANATOP_PLL_LOCK)
				break;
		}
		if (timeout <= 0)
			return -1;
	}

	/* Enable FEC clock */
	reg |= ANATOP_FEC_PLL_ENABLE_MASK;
	reg &= ~ANATOP_PLL_BYPASS_MASK;
	writel(reg, ANATOP_BASE_ADDR + 0xe0);
#endif
	return 0;
}
#endif

#ifdef CONFIG_I2C_MXC
#define I2C1_SCL_GPIO3_12_BIT_MASK  (1 << 12)
#define I2C1_SDA_GPIO3_13_BIT_MASK  (1 << 13)
#define I2C2_SCL_GPIO3_14_BIT_MASK  (1 << 14)
#define I2C2_SDA_GPIO3_15_BIT_MASK  (1 << 15)

static void setup_i2c(unsigned int module_base)
{
	unsigned int reg;

	switch (module_base) {
	case I2C1_BASE_ADDR:
		/* i2c1 SDA */
		mxc_iomux_v3_setup_pad(MX6SL_PAD_I2C1_SDA__I2C1_SDA);
		/* i2c1 SCL */
		mxc_iomux_v3_setup_pad(MX6SL_PAD_I2C1_SCL__I2C1_SCL);

		/* Enable i2c clock */
		reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR2);
		reg |= 0xC0;
		writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR2);

		break;
	case I2C2_BASE_ADDR:
		/* i2c2 SDA */
		mxc_iomux_v3_setup_pad(MX6SL_PAD_I2C2_SDA__I2C2_SDA);

		/* i2c2 SCL */
		mxc_iomux_v3_setup_pad(MX6SL_PAD_I2C2_SCL__I2C2_SCL);

		/* Enable i2c clock */
		reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR2);
		reg |= 0x300;
		writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR2);

		break;
	default:
		printf("Invalid I2C base: 0x%x\n", module_base);
		break;
	}
}

/* Note: udelay() is not accurate for i2c timing */
static void __udelay(int time)
{
	int i, j;

	for (i = 0; i < time; i++) {
		for (j = 0; j < 200; j++) {
			asm("nop");
			asm("nop");
		}
	}
}

static void mx6sl_i2c_gpio_scl_direction(int bus, int output)
{
	u32 reg;

	switch (bus) {
	case 1:
		mxc_iomux_v3_setup_pad(MX6SL_PAD_I2C1_SCL__GPIO_3_12);
		reg = readl(GPIO3_BASE_ADDR + GPIO_GDIR);
		if (output)
			reg |= I2C1_SCL_GPIO3_12_BIT_MASK;
		else
			reg &= ~I2C1_SCL_GPIO3_12_BIT_MASK;
		writel(reg, GPIO3_BASE_ADDR + GPIO_GDIR);
		break;
	case 2:
		mxc_iomux_v3_setup_pad(MX6SL_PAD_I2C2_SCL__GPIO_3_14);
		reg = readl(GPIO3_BASE_ADDR + GPIO_GDIR);
		if (output)
			reg |= I2C2_SCL_GPIO3_14_BIT_MASK;
		else
			reg &= ~I2C2_SCL_GPIO3_14_BIT_MASK;
		writel(reg, GPIO3_BASE_ADDR + GPIO_GDIR);
		break;
	}
}

/* set 1 to output, sent 0 to input */
static void mx6sl_i2c_gpio_sda_direction(int bus, int output)
{
	u32 reg;

	switch (bus) {
	case 1:
		mxc_iomux_v3_setup_pad(MX6SL_PAD_I2C1_SDA__GPIO_3_13);
		reg = readl(GPIO3_BASE_ADDR + GPIO_GDIR);
		if (output)
			reg |= I2C1_SDA_GPIO3_13_BIT_MASK;
		else
			reg &= ~I2C1_SDA_GPIO3_13_BIT_MASK;
		writel(reg, GPIO3_BASE_ADDR + GPIO_GDIR);
		break;
	case 2:
		mxc_iomux_v3_setup_pad(MX6SL_PAD_I2C2_SDA__GPIO_3_15);
		reg = readl(GPIO3_BASE_ADDR + GPIO_GDIR);
		if (output)
			reg |= I2C2_SDA_GPIO3_15_BIT_MASK;
		else
			reg &= ~I2C2_SDA_GPIO3_15_BIT_MASK;
		writel(reg, GPIO3_BASE_ADDR + GPIO_GDIR);
		break;
	}
}

/* set 1 to high 0 to low */
static void mx6sl_i2c_gpio_scl_set_level(int bus, int high)
{
	u32 reg;

	switch (bus) {
	case 1:
		reg = readl(GPIO3_BASE_ADDR + GPIO_DR);
		if (high)
			reg |= I2C1_SCL_GPIO3_12_BIT_MASK;
		else
			reg &= ~I2C1_SCL_GPIO3_12_BIT_MASK;
		writel(reg, GPIO3_BASE_ADDR + GPIO_DR);
		break;
	case 2:
		reg = readl(GPIO3_BASE_ADDR + GPIO_DR);
		if (high)
			reg |= I2C2_SCL_GPIO3_14_BIT_MASK;
		else
			reg &= ~I2C2_SCL_GPIO3_14_BIT_MASK;
		writel(reg, GPIO3_BASE_ADDR + GPIO_DR);
		break;
	}
}

/* set 1 to high 0 to low */
static void mx6sl_i2c_gpio_sda_set_level(int bus, int high)
{
	u32 reg;

	switch (bus) {
	case 1:
		reg = readl(GPIO3_BASE_ADDR + GPIO_DR);
		if (high)
			reg |= I2C1_SDA_GPIO3_13_BIT_MASK;
		else
			reg &= ~I2C1_SDA_GPIO3_13_BIT_MASK;
		writel(reg, GPIO3_BASE_ADDR + GPIO_DR);
		break;
	case 2:
		reg = readl(GPIO3_BASE_ADDR + GPIO_DR);
		if (high)
			reg |= I2C2_SDA_GPIO3_15_BIT_MASK;
		else
			reg &= ~I2C2_SDA_GPIO3_15_BIT_MASK;
		writel(reg, GPIO3_BASE_ADDR + GPIO_DR);
		break;
	}
}

static int mx6sl_i2c_gpio_check_sda(int bus)
{
	u32 reg;
	int result = 0;

	switch (bus) {
	case 1:
		reg = readl(GPIO3_BASE_ADDR + GPIO_PSR);
		result = !!(reg & I2C1_SDA_GPIO3_13_BIT_MASK);
		break;
	case 2:
		reg = readl(GPIO3_BASE_ADDR + GPIO_PSR);
		result = !!(reg & I2C2_SDA_GPIO3_15_BIT_MASK);
		break;
	}

	return result;
}

 /* Random reboot cause i2c SDA low issue:
  * the i2c bus busy because some device pull down the I2C SDA
  * line. This happens when Host is reading some byte from slave, and
  * then host is reset/reboot. Since in this case, device is
  * controlling i2c SDA line, the only thing host can do this give the
  * clock on SCL and sending NAK, and STOP to finish this
  * transaction.
  *
  * How to fix this issue:
  * detect if the SDA was low on bus send 8 dummy clock, and 1
  * clock + NAK, and STOP to finish i2c transaction the pending
  * transfer.
  */
int i2c_bus_recovery(void)
{
	int i, bus, result = 0;

	for (bus = 1; bus <= 2; bus++) {
		mx6sl_i2c_gpio_sda_direction(bus, 0);

		if (mx6sl_i2c_gpio_check_sda(bus) == 0) {
			printf("i2c: I2C%d SDA is low, start i2c recovery...\n",
			       bus);
			mx6sl_i2c_gpio_scl_direction(bus, 1);
			mx6sl_i2c_gpio_scl_set_level(bus, 1);
			__udelay(10000);

			for (i = 0; i < 9; i++) {
				mx6sl_i2c_gpio_scl_set_level(bus, 1);
				__udelay(5);
				mx6sl_i2c_gpio_scl_set_level(bus, 0);
				__udelay(5);
			}

			/* 9th clock here, the slave should already
			   release the SDA, we can set SDA as high to
			   a NAK. */
			mx6sl_i2c_gpio_sda_direction(bus, 1);
			mx6sl_i2c_gpio_sda_set_level(bus, 1);
			__udelay(1);	/* Pull up SDA first */
			mx6sl_i2c_gpio_scl_set_level(bus, 1);
			__udelay(5);	/* plus pervious 1 us */
			mx6sl_i2c_gpio_scl_set_level(bus, 0);
			__udelay(5);
			mx6sl_i2c_gpio_sda_set_level(bus, 0);
			__udelay(5);
			mx6sl_i2c_gpio_scl_set_level(bus, 1);
			__udelay(5);
			/* Here: SCL is high, and SDA from low to high, it's a
			 * stop condition */
			mx6sl_i2c_gpio_sda_set_level(bus, 1);
			__udelay(5);

			mx6sl_i2c_gpio_sda_direction(bus, 0);
			if (mx6sl_i2c_gpio_check_sda(bus) == 1)
				printf("I2C%d Recovery success\n", bus);
			else {
				printf
				    ("I2C%d Recovery failed, I2C1 SDA still low!!!\n",
				     bus);
				result |= 1 << bus;
			}
		}
		/* configure back to i2c */
		switch (bus) {
		case 1:
			setup_i2c(I2C1_BASE_ADDR);
			break;
		case 2:
			setup_i2c(I2C2_BASE_ADDR);
			break;
		}
	}

	return result;
}

void setup_pmic_voltages(void)
{
	unsigned char value = 0;
#if CONFIG_MX6_INTER_LDO_BYPASS
	unsigned int val = 0;
#endif
	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
	if (!i2c_probe(0x8)) {
		if (i2c_read(0x8, 0, 1, &value, 1))
			printf("%s:i2c_read:error\n", __func__);
		printf("Found PFUZE100! device id=%x\n", value);
#if CONFIG_MX6_INTER_LDO_BYPASS
		/*VDDCORE 1.1V@800Mhz: SW1AB */
		value = 0x20;
		i2c_write(0x8, 0x20, 1, &value, 1);

		/*VDDSOC 1.2V : SW1C */
		value = 0x24;
		i2c_write(0x8, 0x2e, 1, &value, 1);

		/* Bypass the VDDSOC from Anatop */
		val = REG_RD(ANATOP_BASE_ADDR, HW_ANADIG_REG_CORE);
		val &= ~BM_ANADIG_REG_CORE_REG2_TRG;
		val |= BF_ANADIG_REG_CORE_REG2_TRG(0x1f);
		REG_WR(ANATOP_BASE_ADDR, HW_ANADIG_REG_CORE, val);

		/* Bypass the VDDCORE from Anatop */
		val = REG_RD(ANATOP_BASE_ADDR, HW_ANADIG_REG_CORE);
		val &= ~BM_ANADIG_REG_CORE_REG0_TRG;
		val |= BF_ANADIG_REG_CORE_REG0_TRG(0x1f);
		REG_WR(ANATOP_BASE_ADDR, HW_ANADIG_REG_CORE, val);

		/* Bypass the VDDPU from Anatop */
		val = REG_RD(ANATOP_BASE_ADDR, HW_ANADIG_REG_CORE);
		val &= ~BM_ANADIG_REG_CORE_REG1_TRG;
		val |= BF_ANADIG_REG_CORE_REG1_TRG(0x1f);
		REG_WR(ANATOP_BASE_ADDR, HW_ANADIG_REG_CORE, val);

		printf("hw_anadig_reg_core=%x\n",
		       REG_RD(ANATOP_BASE_ADDR, HW_ANADIG_REG_CORE));
#endif

	}
}
#endif

int board_init(void)
{
	mxc_iomux_v3_init((void *)IOMUXC_BASE_ADDR);
	setup_boot_device();

	/* board id for linux */
	gd->bd->bi_arch_number = MACH_TYPE_MX6SL_EVK;

	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	wdog_preconfig(WDOG1_BASE_ADDR);

	setup_uart();

#ifdef CONFIG_MXC_FEC
	setup_fec();
#endif

#ifdef CONFIG_MXC_EPDC
	setup_epdc();
#endif
	return 0;
}

int board_late_init(void)
{
#ifdef CONFIG_I2C_MXC
	setup_i2c(CONFIG_SYS_I2C_PORT);
	i2c_bus_recovery();
	setup_pmic_voltages();
#endif
	return 0;
}

int checkboard(void)
{
	printf("Board: MX6SoloLite-EVK:[ ");

	switch (__REG(SRC_BASE_ADDR + 0x8)) {
	case 0x0001:
		printf("POR");
		break;
	case 0x0009:
		printf("RST");
		break;
	case 0x0010:
	case 0x0011:
		printf("WDOG");
		break;
	default:
		printf("unknown");
	}
	printf(" ]\n");

	printf("Boot Device: ");
	switch (get_boot_device()) {
	case WEIM_NOR_BOOT:
		printf("NOR\n");
		break;
	case ONE_NAND_BOOT:
		printf("ONE NAND\n");
		break;
	case I2C_BOOT:
		printf("I2C\n");
		break;
	case SPI_NOR_BOOT:
		printf("SPI NOR\n");
		break;
	case SD_BOOT:
		printf("SD\n");
		break;
	case MMC_BOOT:
		printf("MMC\n");
		break;
	case UNKNOWN_BOOT:
	default:
		printf("UNKNOWN\n");
		break;
	}

#ifdef CONFIG_SECURE_BOOT
	if (check_hab_enable() == 1)
		get_hab_status();
#endif

	return 0;
}

#ifdef CONFIG_ANDROID_RECOVERY
int check_recovery_cmd_file(void)
{
	return check_and_clean_recovery_flag();
}
#endif

#ifdef CONFIG_IMX_UDC
void udc_pins_setting(void)
{
	/* USB_OTG_PWR */
	mxc_iomux_v3_setup_pad(MX6SL_PAD_KEY_COL4__GPIO_4_0);
	mxc_iomux_v3_setup_pad(MX6SL_PAD_KEY_COL5__GPIO_4_2);
	/* USB_OTG_PWR = 0 */
	gpio_direction_output(USB_OTG_PWR, 0);
	/* USB_H1_POWER = 1 */
	gpio_direction_output(USB_H1_PWR, 1);
}
#endif
