/*
 * Copyright (C) 2007, Guennadi Liakhovetski <lg@denx.de>
 *
 * (C) Copyright 2009 Freescale Semiconductor, Inc.
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
#include <asm/arch/mx51.h>
#include <asm/arch/mx51_pins.h>
#include <asm/arch/iomux.h>
#include <asm/errno.h>
#include <i2c.h>
#include <mxc_keyb.h>
#include <asm/arch/keypad.h>
#include "board-imx51.h"
#include <asm/arch/imx_spi.h>
#include <asm/arch/imx_spi_pmic.h>

#include <asm/errno.h>

#ifdef CONFIG_CMD_MMC
#include <mmc.h>
#include <fsl_esdhc.h>
#endif

#ifdef CONFIG_FSL_ANDROID
#include <part.h>
#include <ext2fs.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <ubi_uboot.h>
#include <jffs2/load_kernel.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

static u32 system_rev;
static enum boot_device boot_dev;
u32	mx51_io_base_addr;

static inline void setup_boot_device(void)
{
	uint *fis_addr = (uint *)IRAM_BASE_ADDR;

	switch (*fis_addr) {
	case NAND_FLASH_BOOT:
		boot_dev = NAND_BOOT;
		break;
	case SPI_NOR_FLASH_BOOT:
		boot_dev = SPI_NOR_BOOT;
		break;
	case MMC_FLASH_BOOT:
		boot_dev = MMC_BOOT;
		break;
	default:
		{
			uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
			uint bt_mem_ctl = soc_sbmr & 0x00000003;
			uint bt_mem_type = (soc_sbmr & 0x000000C0) >> 7;

			switch (bt_mem_ctl) {
			case 0x3:
				if (bt_mem_type == 0)
					boot_dev = MMC_BOOT;
				else if (bt_mem_type == 3)
					boot_dev = SPI_NOR_BOOT;
				else
					boot_dev = UNKNOWN_BOOT;
				break;
			case 0x1:
				boot_dev = NAND_BOOT;
				break;
			default:
				boot_dev = UNKNOWN_BOOT;
			}
		}
		break;
	}
}

enum boot_device get_boot_device(void)
{
	return boot_dev;
}

u32 get_board_rev(void)
{
	return system_rev;
}

static inline void setup_soc_rev(void)
{
	int reg;
	reg = __REG(ROM_SI_REV);
	switch (reg) {
	case 0x02:
		system_rev = 0x51000 | CHIP_REV_1_1;
		break;
	case 0x10:
		if ((__REG(GPIO1_BASE_ADDR + 0x0) & (0x1 << 22)) == 0) {
			system_rev = 0x51000 | CHIP_REV_2_5;
		} else {
			system_rev = 0x51000 | CHIP_REV_2_0;
		}
		break;
	case 0x20:
		system_rev = 0x51000 | CHIP_REV_3_0;
		break;
	default:
		system_rev = 0x51000 | CHIP_REV_1_0;
	}
}

static inline void set_board_rev(int rev)
{
	system_rev |= (rev & 0xF) << 8;
}

inline int is_soc_rev(int rev)
{
	return (system_rev & 0xFF) - rev;
}

int dram_init(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
	return 0;
}

static void setup_uart(void)
{
	unsigned int pad = PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE |
			 PAD_CTL_PUE_PULL | PAD_CTL_DRV_HIGH;
	mxc_request_iomux(MX51_PIN_UART1_RXD, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_UART1_RXD, pad | PAD_CTL_SRE_FAST);
	mxc_request_iomux(MX51_PIN_UART1_TXD, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_UART1_TXD, pad | PAD_CTL_SRE_FAST);
	mxc_request_iomux(MX51_PIN_UART1_RTS, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_UART1_RTS, pad);
	mxc_request_iomux(MX51_PIN_UART1_CTS, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_UART1_CTS, pad);
	/* enable GPIO1_9 for CLK0 and GPIO1_8 for CLK02 */
	writel(0x00000004, 0x73fa83e8);
	writel(0x00000004, 0x73fa83ec);
}

void setup_nfc(void)
{
	/* Enable NFC IOMUX */
	mxc_request_iomux(MX51_PIN_NANDF_CS0, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_CS1, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_CS2, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_CS3, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_CS4, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_CS5, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_CS6, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_NANDF_CS7, IOMUX_CONFIG_ALT0);
}

static void setup_expio(void)
{
	u32 reg;
	/* CS5 setup */
	mxc_request_iomux(MX51_PIN_EIM_CS5, IOMUX_CONFIG_ALT0);
	writel(0x00410089, WEIM_BASE_ADDR + 0x78 + CSGCR1);
	writel(0x00000002, WEIM_BASE_ADDR + 0x78 + CSGCR2);
	/* RWSC=50, RADVA=2, RADVN=6, OEA=0, OEN=0, RCSA=0, RCSN=0 */
	writel(0x32260000, WEIM_BASE_ADDR + 0x78 + CSRCR1);
	/* APR = 0 */
	writel(0x00000000, WEIM_BASE_ADDR + 0x78 + CSRCR2);
	/* WAL=0, WBED=1, WWSC=50, WADVA=2, WADVN=6, WEA=0, WEN=0,
	 * WCSA=0, WCSN=0
	 */
	writel(0x72080F00, WEIM_BASE_ADDR + 0x78 + CSWCR1);
	if ((readw(CS5_BASE_ADDR + PBC_ID_AAAA) == 0xAAAA) &&
	    (readw(CS5_BASE_ADDR + PBC_ID_5555) == 0x5555)) {
		if (is_soc_rev(CHIP_REV_2_0) < 0) {
			reg = readl(CCM_BASE_ADDR + CLKCTL_CBCDR);
			reg = (reg & (~0x70000)) | 0x30000;
			writel(reg, CCM_BASE_ADDR + CLKCTL_CBCDR);
			/* make sure divider effective */
			while (readl(CCM_BASE_ADDR + CLKCTL_CDHIPR) != 0)
				;
			writel(0x0, CCM_BASE_ADDR + CLKCTL_CCDR);
		}
		mx51_io_base_addr = CS5_BASE_ADDR;
	} else {
		/* CS1 */
		writel(0x00410089, WEIM_BASE_ADDR + 0x18 + CSGCR1);
		writel(0x00000002, WEIM_BASE_ADDR + 0x18 + CSGCR2);
		/*  RWSC=50, RADVA=2, RADVN=6, OEA=0, OEN=0, RCSA=0, RCSN=0 */
		writel(0x32260000, WEIM_BASE_ADDR + 0x18 + CSRCR1);
		/* APR=0 */
		writel(0x00000000, WEIM_BASE_ADDR + 0x18 + CSRCR2);
		/* WAL=0, WBED=1, WWSC=50, WADVA=2, WADVN=6, WEA=0,
		 * WEN=0, WCSA=0, WCSN=0
		 */
		writel(0x72080F00, WEIM_BASE_ADDR + 0x18 + CSWCR1);
		mx51_io_base_addr = CS1_BASE_ADDR;
	}

	/* Reset interrupt status reg */
	writew(0x1F, mx51_io_base_addr + PBC_INT_REST);
	writew(0x00, mx51_io_base_addr + PBC_INT_REST);
	writew(0xFFFF, mx51_io_base_addr + PBC_INT_MASK);

	/* Reset the XUART and Ethernet controllers */
	reg = readw(mx51_io_base_addr + PBC_SW_RESET);
	reg |= 0x9;
	writew(reg, mx51_io_base_addr + PBC_SW_RESET);
	reg &= ~0x9;
	writew(reg, mx51_io_base_addr + PBC_SW_RESET);
}

void spi_io_init(struct imx_spi_dev_t *dev)
{
	switch (dev->base) {
	case CSPI1_BASE_ADDR:
		/* 000: Select mux mode: ALT0 mux port: MOSI of instance: ecspi1 */
		mxc_request_iomux(MX51_PIN_CSPI1_MOSI, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_CSPI1_MOSI, 0x105);

		/* 000: Select mux mode: ALT0 mux port: MISO of instance: ecspi1. */
		mxc_request_iomux(MX51_PIN_CSPI1_MISO, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_CSPI1_MISO, 0x105);

		if (dev->ss == 0) {
			/* de-select SS1 of instance: ecspi1. */
			mxc_request_iomux(MX51_PIN_CSPI1_SS1, IOMUX_CONFIG_ALT3);
			mxc_iomux_set_pad(MX51_PIN_CSPI1_SS1, 0x85);
			/* 000: Select mux mode: ALT0 mux port: SS0 of instance: ecspi1. */
			mxc_request_iomux(MX51_PIN_CSPI1_SS0, IOMUX_CONFIG_ALT0);
			mxc_iomux_set_pad(MX51_PIN_CSPI1_SS0, 0x185);
		} else if (dev->ss == 1) {
			/* de-select SS0 of instance: ecspi1. */
			mxc_request_iomux(MX51_PIN_CSPI1_SS0, IOMUX_CONFIG_ALT3);
			mxc_iomux_set_pad(MX51_PIN_CSPI1_SS0, 0x85);
			/* 000: Select mux mode: ALT0 mux port: SS1 of instance: ecspi1. */
			mxc_request_iomux(MX51_PIN_CSPI1_SS1, IOMUX_CONFIG_ALT0);
			mxc_iomux_set_pad(MX51_PIN_CSPI1_SS1, 0x105);
		}

		/* 000: Select mux mode: ALT0 mux port: RDY of instance: ecspi1. */
		mxc_request_iomux(MX51_PIN_CSPI1_RDY, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_CSPI1_RDY, 0x180);

		/* 000: Select mux mode: ALT0 mux port: SCLK of instance: ecspi1. */
		mxc_request_iomux(MX51_PIN_CSPI1_SCLK, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX51_PIN_CSPI1_SCLK, 0x105);
		break;
	case CSPI2_BASE_ADDR:
	default:
		break;
	}
}

static void setup_fec(void)
{
	/*FEC_MDIO*/
	writel(0x3, IOMUXC_BASE_ADDR + 0x0D4);
	writel(0x1FD, IOMUXC_BASE_ADDR + 0x0468);
	writel(0x0, IOMUXC_BASE_ADDR + 0x0954);

	/*FEC_MDC*/
	writel(0x2, IOMUXC_BASE_ADDR + 0x13C);
	writel(0x2004, IOMUXC_BASE_ADDR + 0x0524);

	/* FEC RDATA[3] */
	writel(0x3, IOMUXC_BASE_ADDR + 0x0EC);
	writel(0x180, IOMUXC_BASE_ADDR + 0x0480);
	writel(0x0, IOMUXC_BASE_ADDR + 0x0964);

	/* FEC RDATA[2] */
	writel(0x3, IOMUXC_BASE_ADDR + 0x0E8);
	writel(0x180, IOMUXC_BASE_ADDR + 0x047C);
	writel(0x0, IOMUXC_BASE_ADDR + 0x0960);

	/* FEC RDATA[1] */
	writel(0x3, IOMUXC_BASE_ADDR + 0x0d8);
	writel(0x180, IOMUXC_BASE_ADDR + 0x046C);
	writel(0x0, IOMUXC_BASE_ADDR + 0x095C);

	/* FEC RDATA[0] */
	writel(0x2, IOMUXC_BASE_ADDR + 0x016C);
	writel(0x2180, IOMUXC_BASE_ADDR + 0x0554);
	writel(0x0, IOMUXC_BASE_ADDR + 0x0958);

	/* FEC TDATA[3] */
	writel(0x2, IOMUXC_BASE_ADDR + 0x148);
	writel(0x2004, IOMUXC_BASE_ADDR + 0x0530);

	/* FEC TDATA[2] */
	writel(0x2, IOMUXC_BASE_ADDR + 0x144);
	writel(0x2004, IOMUXC_BASE_ADDR + 0x052C);

	/* FEC TDATA[1] */
	writel(0x2, IOMUXC_BASE_ADDR + 0x140);
	writel(0x2004, IOMUXC_BASE_ADDR + 0x0528);

	/* FEC TDATA[0] */
	writel(0x2, IOMUXC_BASE_ADDR + 0x0170);
	writel(0x2004, IOMUXC_BASE_ADDR + 0x0558);

	/* FEC TX_EN */
	writel(0x1, IOMUXC_BASE_ADDR + 0x014C);
	writel(0x2004, IOMUXC_BASE_ADDR + 0x0534);

	/* FEC TX_ER */
	writel(0x2, IOMUXC_BASE_ADDR + 0x0138);
	writel(0x2004, IOMUXC_BASE_ADDR + 0x0520);

	/* FEC TX_CLK */
	writel(0x1, IOMUXC_BASE_ADDR + 0x0150);
	writel(0x2180, IOMUXC_BASE_ADDR + 0x0538);
	writel(0x0, IOMUXC_BASE_ADDR + 0x0974);

	/* FEC COL */
	writel(0x1, IOMUXC_BASE_ADDR + 0x0124);
	writel(0x2180, IOMUXC_BASE_ADDR + 0x0500);
	writel(0x0, IOMUXC_BASE_ADDR + 0x094c);

	/* FEC RX_CLK */
	writel(0x1, IOMUXC_BASE_ADDR + 0x0128);
	writel(0x2180, IOMUXC_BASE_ADDR + 0x0504);
	writel(0x0, IOMUXC_BASE_ADDR + 0x0968);

	/* FEC CRS */
	writel(0x3, IOMUXC_BASE_ADDR + 0x0f4);
	writel(0x180, IOMUXC_BASE_ADDR + 0x0488);
	writel(0x0, IOMUXC_BASE_ADDR + 0x0950);

	/* FEC RX_ER */
	writel(0x3, IOMUXC_BASE_ADDR + 0x0f0);
	writel(0x180, IOMUXC_BASE_ADDR + 0x0484);
	writel(0x0, IOMUXC_BASE_ADDR + 0x0970);

	/* FEC RX_DV */
	writel(0x2, IOMUXC_BASE_ADDR + 0x164);
	writel(0x2180, IOMUXC_BASE_ADDR + 0x054C);
	writel(0x0, IOMUXC_BASE_ADDR + 0x096C);
}

static void power_init(void)
{
	struct spi_slave *slave;
	unsigned int val;
	unsigned int reg;

#define REV_ATLAS_LITE_1_0         0x8
#define REV_ATLAS_LITE_1_1         0x9
#define REV_ATLAS_LITE_2_0         0x10
#define REV_ATLAS_LITE_2_1         0x11

	slave = spi_pmic_probe();

	/* Write needed to Power Gate 2 register */
	val = pmic_reg(slave, 34, 0, 0);
	val &= ~0x10000;
	pmic_reg(slave, 34, val, 1);

	/* Write needed to update Charger 0 */
	pmic_reg(slave, 48, 0x0023807F, 1);

	/* power up the system first */
	pmic_reg(slave, 34, 0x00200000, 1);

	if (is_soc_rev(CHIP_REV_2_0) >= 0) {
		/* Set core voltage to 1.1V */
		val = pmic_reg(slave, 24, 0, 0);
		val = (val & (~0x1F)) | 0x14;
		pmic_reg(slave, 24, val, 1);

		/* Setup VCC (SW2) to 1.25 */
		val = pmic_reg(slave, 25, 0, 0);
		val = (val & (~0x1F)) | 0x1A;
		pmic_reg(slave, 25, val, 1);

		/* Setup 1V2_DIG1 (SW3) to 1.25 */
		val = pmic_reg(slave, 26, 0, 0);
		val = (val & (~0x1F)) | 0x1A;
		pmic_reg(slave, 26, val, 1);
		udelay(50);
		/* Raise the core frequency to 800MHz */
		writel(0x0, CCM_BASE_ADDR + CLKCTL_CACRR);
	} else {
		/* TO 3.0 */
		/* Setup VCC (SW2) to 1.225 */
		val = pmic_reg(slave, 25, 0, 0);
		val = (val & (~0x1F)) | 0x19;
		pmic_reg(slave, 25, val, 1);

		/* Setup 1V2_DIG1 (SW3) to 1.2 */
		val = pmic_reg(slave, 26, 0, 0);
		val = (val & (~0x1F)) | 0x18;
		pmic_reg(slave, 26, val, 1);
	}

	if (((pmic_reg(slave, 7, 0, 0) & 0x1F) < REV_ATLAS_LITE_2_0) ||
		(((pmic_reg(slave, 7, 0, 0) >> 9) & 0x3) == 0)) {
		/* Set switchers in PWM mode for Atlas 2.0 and lower */
		/* Setup the switcher mode for SW1 & SW2*/
		val = pmic_reg(slave, 28, 0, 0);
		val = (val & (~0x3C0F)) | 0x1405;
		pmic_reg(slave, 28, val, 1);

		/* Setup the switcher mode for SW3 & SW4 */
		val = pmic_reg(slave, 29, 0, 0);
		val = (val & (~0xF0F)) | 0x505;
		pmic_reg(slave, 29, val, 1);
	} else {
		/* Set switchers in Auto in NORMAL mode & STANDBY mode for Atlas 2.0a */
		/* Setup the switcher mode for SW1 & SW2*/
		val = pmic_reg(slave, 28, 0, 0);
		val = (val & (~0x3C0F)) | 0x2008;
		pmic_reg(slave, 28, val, 1);

		/* Setup the switcher mode for SW3 & SW4 */
		val = pmic_reg(slave, 29, 0, 0);
		val = (val & (~0xF0F)) | 0x808;
		pmic_reg(slave, 29, val, 1);
	}

	/* Set VDIG to 1.65V, VGEN3 to 1.8V, VCAM to 2.5V */
	val = pmic_reg(slave, 30, 0, 0);
	val &= ~0x34030;
	val |= 0x10020;
	pmic_reg(slave, 30, val, 1);

	/* Set VVIDEO to 2.775V, VAUDIO to 3V, VSD to 3.15V */
	val = pmic_reg(slave, 31, 0, 0);
	val &= ~0x1FC;
	val |= 0x1F4;
	pmic_reg(slave, 31, val, 1);

	/* Configure VGEN3 and VCAM regulators to use external PNP */
	val = 0x208;
	pmic_reg(slave, 33, val, 1);
	udelay(200);

	reg = readl(GPIO2_BASE_ADDR + 0x0);
	reg &= ~0x4000;  /* Lower reset line */
	writel(reg, GPIO2_BASE_ADDR + 0x0);

	reg = readl(GPIO2_BASE_ADDR + 0x4);
	reg |= 0x4000;  /* configure GPIO lines as output */
	writel(reg, GPIO2_BASE_ADDR + 0x4);

	/* Reset the ethernet controller over GPIO */
	writel(0x1, IOMUXC_BASE_ADDR + 0x0AC);

	/* Enable VGEN3, VCAM, VAUDIO, VVIDEO, VSD regulators */
	val = 0x49249;
	pmic_reg(slave, 33, val, 1);

	udelay(500);

	reg = readl(GPIO2_BASE_ADDR + 0x0);
	reg |= 0x4000;
	writel(reg, GPIO2_BASE_ADDR + 0x0);

	spi_pmic_free(slave);
}

#ifdef CONFIG_NET_MULTI

int board_eth_init(bd_t *bis)
{
	int rc = -ENODEV;

	return rc;
}
#endif

#ifdef CONFIG_CMD_MMC

u32 *imx_esdhc_base_addr;

int esdhc_gpio_init(void)
{
	u32 interface_esdhc = 0;
	s32 status = 0;
	u32 pad = 0;
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);

	interface_esdhc = (soc_sbmr & (0x00180000)) >> 19;

	switch (interface_esdhc) {
	case 0:
		imx_esdhc_base_addr = (u32 *)MMC_SDHC1_BASE_ADDR;

		pad = PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE | PAD_CTL_DRV_HIGH |
		                PAD_CTL_47K_PU | PAD_CTL_SRE_FAST;

		mxc_request_iomux(MX51_PIN_SD1_CMD,
			  IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_SD1_CLK,
			  IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);

		mxc_request_iomux(MX51_PIN_SD1_DATA0,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_SD1_DATA1,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_SD1_DATA2,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_SD1_DATA3,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX51_PIN_SD1_CMD, pad);
		mxc_iomux_set_pad(MX51_PIN_SD1_CLK, pad);
		mxc_iomux_set_pad(MX51_PIN_SD1_DATA0, pad);
		mxc_iomux_set_pad(MX51_PIN_SD1_DATA1, pad);
		mxc_iomux_set_pad(MX51_PIN_SD1_DATA2, pad);
		mxc_iomux_set_pad(MX51_PIN_SD1_DATA3, pad);
		break;
	case 1:
		imx_esdhc_base_addr = (u32 *)MMC_SDHC2_BASE_ADDR;

		pad = PAD_CTL_DRV_MAX | PAD_CTL_22K_PU | PAD_CTL_SRE_FAST;

		mxc_request_iomux(MX51_PIN_SD2_CMD,
			  IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_SD2_CLK,
			  IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);

		mxc_request_iomux(MX51_PIN_SD2_DATA0,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_SD2_DATA1,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_SD2_DATA2,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_request_iomux(MX51_PIN_SD2_DATA3,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX51_PIN_SD2_CMD, pad);
		mxc_iomux_set_pad(MX51_PIN_SD2_CLK, pad);
		mxc_iomux_set_pad(MX51_PIN_SD2_DATA0, pad);
		mxc_iomux_set_pad(MX51_PIN_SD2_DATA1, pad);
		mxc_iomux_set_pad(MX51_PIN_SD2_DATA2, pad);
		mxc_iomux_set_pad(MX51_PIN_SD2_DATA3, pad);
		break;
	case 2:
		status = -1;
		break;
	case 3:
		status = -1;
		break;
	default:
		status = -1;
		break;
	}

	return status;
}

int board_mmc_init(void)
{
	if (!esdhc_gpio_init())
		return fsl_esdhc_mmc_init(gd->bd);
	else
		return -1;
}

#endif

#if defined(CONFIG_MXC_KPD)
int setup_mxc_kpd(void)
{
	mxc_request_iomux(MX51_PIN_KEY_COL0, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_COL1, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_COL2, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_COL3, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_COL4, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_COL5, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_ROW0, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_ROW1, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_ROW2, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX51_PIN_KEY_ROW3, IOMUX_CONFIG_ALT0);

	return 0;
}
#endif

int board_init(void)
{
	setup_boot_device();
	setup_soc_rev();

	gd->bd->bi_arch_number = MACH_TYPE_MX51_BABBAGE;	/* board id for linux */
	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	setup_uart();
	setup_nfc();
	setup_expio();
	setup_fec();
	return 0;
}

#ifdef BOARD_LATE_INIT
#if defined(CONFIG_FSL_ANDROID) && defined(CONFIG_MXC_KPD)
inline int waiting_for_func_key_pressing(void)
{
	struct kpp_key_info key_info = {0, 0};
	int switch_delay = CONFIG_ANDROID_BOOTMOD_DELAY;
	int state = 0, boot_mode_switch = 0;

	mxc_kpp_init();

	puts("Press home + power to enter recovery mode ...\n");

	while ((switch_delay > 0) && (!boot_mode_switch)) {
		int i;

		--switch_delay;
		/* delay 100 * 10ms */
		for (i = 0; !boot_mode_switch && i < 100; ++i) {
			/* A state machine to scan home + power key */
			/* Check for home + power */
			if (mxc_kpp_getc(&key_info)) {
				switch (state) {
				case 0:
					/* First press */
					if (TEST_HOME_KEY_DEPRESS(key_info.val, key_info.evt)) {
						/* Press Home */
						state = 1;
					} else if (TEST_POWER_KEY_DEPRESS(key_info.val, key_info.evt)) {
						/* Press Power */
						state = 2;
					} else {
						state = 0;
					}
					break;
				case 1:
					/* Home is already pressed, try to detect Power */
					if (TEST_POWER_KEY_DEPRESS(key_info.val,
						    key_info.evt)) {
						/* Switch */
						boot_mode_switch = 1;
					} else {
					    if (TEST_HOME_KEY_DEPRESS(key_info.val,
							key_info.evt)) {
						/* Not switch */
						state = 2;
					    } else
						state = 0;
					}
					break;
				case 2:
					/* Power is already pressed, try to detect Home */
					if (TEST_HOME_KEY_DEPRESS(key_info.val,
						    key_info.evt)) {
						/* Switch */
						boot_mode_switch = 1;
					} else {
						if (TEST_POWER_KEY_DEPRESS(key_info.val,
							    key_info.evt)) {
							/* Not switch */
							state = 1;
						} else
							state = 0;
					}
					break;
				default:
					break;
				}

				if (1 == boot_mode_switch)
					return 1;
			}
		}
		for (i = 0; i < 100; ++i)
			udelay(10000);
	}

	return 0;
}

inline int switch_to_recovery_mode(void)
{
	printf("Boot mode switched to recovery mode!\n");

	switch (get_boot_device()) {
	case MMC_BOOT:
		/* Set env to recovery mode */
		setenv("bootargs_android", \
			CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC);
		setenv("bootcmd_android", \
			CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC);
		setenv("bootcmd", "run bootcmd_android");
		break;
	case NAND_BOOT:
		setenv("bootargs_android", \
			CONFIG_ANDROID_RECOVERY_BOOTARGS_NAND);
		setenv("bootcmd_android", \
			CONFIG_ANDROID_RECOVERY_BOOTCMD_NAND);
		setenv("bootcmd", "run bootcmd_android");
		break;
	case SPI_NOR_BOOT:
		printf("Recovery mode not supported in SPI NOR boot\n");
		return -1;
		break;
	case UNKNOWN_BOOT:
	default:
		printf("Unknown boot device!\n");
		return -1;
		break;
	}

	return 0;
}

inline int check_recovery_cmd_file(void)
{
	disk_partition_t info;
	ulong part_length;
	int filelen;

	switch (get_boot_device()) {
	case MMC_BOOT:
		{
			block_dev_desc_t *dev_desc = NULL;
			struct mmc *mmc = find_mmc_device(0);

			dev_desc = get_dev("mmc", 0);

			if (NULL == dev_desc) {
				puts("** Block device MMC 0 not supported\n");
				return 0;
			}

			mmc_init(mmc);

			if (get_partition_info(dev_desc,
					CONFIG_ANDROID_CACHE_PARTITION_MMC,
					&info)) {
				printf("** Bad partition %d **\n",
					CONFIG_ANDROID_CACHE_PARTITION_MMC);
				return 0;
			}

			part_length = ext2fs_set_blk_dev(dev_desc,
							CONFIG_ANDROID_CACHE_PARTITION_MMC);
			if (part_length == 0) {
				printf("** Bad partition - mmc 0:%d **\n",
					CONFIG_ANDROID_CACHE_PARTITION_MMC);
				ext2fs_close();
				return 0;
			}

			if (!ext2fs_mount(part_length)) {
				printf("** Bad ext2 partition or disk - mmc 0:%d **\n",
					CONFIG_ANDROID_CACHE_PARTITION_MMC);
				ext2fs_close();
				return 0;
			}

			filelen = ext2fs_open(CONFIG_ANDROID_RECOVERY_CMD_FILE);

			ext2fs_close();
		}
		break;
	case NAND_BOOT:
		{
			#if 0
			struct mtd_device *dev_desc = NULL;
			struct part_info *part = NULL;
			struct mtd_partition mtd_part;
			struct mtd_info *mtd_info;
			char mtd_dev[16] = { 0 };
			char mtd_buffer[80] = { 0 };
			u8 pnum;
			int err;
			u8 read_test;

			/* ========== ubi and mtd operations ========== */
			if (mtdparts_init() != 0) {
				printf("Error initializing mtdparts!\n");
				return 0;
			}

			if (find_dev_and_part("nand", &dev_desc, &pnum, &part)) {
				printf("Partition %s not found!\n", "nand");
				return 0;
			}
			sprintf(mtd_dev, "%s%d",
					MTD_DEV_TYPE(dev_desc->id->type),
					dev_desc->id->num);
			mtd_info = get_mtd_device_nm(mtd_dev);
			if (IS_ERR(mtd_info)) {
				printf("Partition %s not found on device %s!\n",
					"nand", mtd_dev);
				return 0;
			}

			sprintf(mtd_buffer, "mtd=%d", pnum);
			memset(&mtd_part, 0, sizeof(mtd_part));
			mtd_part.name = mtd_buffer;
			mtd_part.size = part->size;
			mtd_part.offset = part->offset;
			add_mtd_partitions(&info, &mtd_part, 1);

			err = ubi_mtd_param_parse(mtd_buffer, NULL);
			if (err) {
				del_mtd_partitions(&info);
				return 0;
			}

			err = ubi_init();
			if (err) {
				del_mtd_partitions(&info);
				return 0;
			}

			/* ========== ubifs operations ========== */
			/* Init ubifs */
			ubifs_init();

			if (ubifs_mount(CONFIG_ANDROID_CACHE_PARTITION_NAND)) {
				printf("Mount ubifs volume %s fail!\n",
						CONFIG_ANDROID_CACHE_PARTITION_NAND);
				return 0;
			}

			/* Try to read one byte for a read test. */
			if (ubifs_load(CONFIG_ANDROID_RECOVERY_CMD_FILE,
						&read_test, 1)) {
				/* File not found */
				filelen = 0;
			} else
				filelen = 1;
		#endif
		}
		break;
	case SPI_NOR_BOOT:
		return 0;
		break;
	case UNKNOWN_BOOT:
	default:
		return 0;
		break;
	}

	return (filelen > 0) ? 1 : 0;

}
#endif

int board_late_init(void)
{
	power_init();

#if defined(CONFIG_FSL_ANDROID) && defined(CONFIG_MXC_KPD)
	if (waiting_for_func_key_pressing())
		switch_to_recovery_mode();
	else {
		if (check_recovery_cmd_file()) {
			puts("Recovery command file founded!\n");
			switch_to_recovery_mode();
		}
	}
#endif

	return 0;
}
#endif

int checkboard(void)
{
	printf("Board: MX51 BABBAGE ");

	if (system_rev & CHIP_REV_3_0) {
		printf("3.0 [");
	} else if (system_rev & CHIP_REV_2_5) {
		printf("2.5 [");
	} else if (system_rev & CHIP_REV_2_0) {
		printf("2.0 [");
	} else if (system_rev & CHIP_REV_1_1) {
		printf("1.1 [");
	} else {
		printf("1.0 [");
	}

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
	printf("]\n");

	printf("Boot Device: ");
	switch (get_boot_device()) {
	case NAND_BOOT:
		printf("NAND\n");
		break;
	case SPI_NOR_BOOT:
		printf("SPI NOR\n");
		break;
	case MMC_BOOT:
		printf("MMC\n");
		break;
	case UNKNOWN_BOOT:
	default:
		printf("UNKNOWN\n");
		break;
	}
	return 0;
}

