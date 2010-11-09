/*
 * Copyright (C) 2007, Guennadi Liakhovetski <lg@denx.de>
 *
 * (C) Copyright 2009-2010 Freescale Semiconductor, Inc.
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
#include <asm/arch/mx53.h>
#include <asm/arch/mx53_pins.h>
#include <asm/arch/iomux.h>
#include <asm/errno.h>
#include <imx_spi.h>
#include <netdev.h>

#if CONFIG_I2C_MXC
#include <i2c.h>
#endif

#ifdef CONFIG_CMD_MMC
#include <mmc.h>
#include <fsl_esdhc.h>
#endif

#ifdef CONFIG_ARCH_MMU
#include <asm/mmu.h>
#include <asm/arch/mmu.h>
#endif

#ifdef CONFIG_GET_FEC_MAC_ADDR_FROM_IIM
#include <asm/imx_iim.h>
#endif

#ifdef CONFIG_CMD_CLOCK
#include <asm/clock.h>
#endif

#ifdef CONFIG_ANDROID_RECOVERY
#include "../common/recovery.h"
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

static inline void setup_boot_device(void)
{
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
	uint bt_mem_ctl = (soc_sbmr & 0x000000FF) >> 4 ;
	uint bt_mem_type = (soc_sbmr & 0x00000008) >> 3;

	switch (bt_mem_ctl) {
	case 0x0:
		if (bt_mem_type)
			boot_dev = ONE_NAND_BOOT;
		else
			boot_dev = WEIM_NOR_BOOT;
		break;
	case 0x2:
		if (bt_mem_type)
			boot_dev = SATA_BOOT;
		else
			boot_dev = PATA_BOOT;
		break;
	case 0x3:
		if (bt_mem_type)
			boot_dev = SPI_NOR_BOOT;
		else
			boot_dev = I2C_BOOT;
		break;
	case 0x4:
	case 0x5:
		boot_dev = SD_BOOT;
		break;
	case 0x6:
	case 0x7:
		boot_dev = MMC_BOOT;
		break;
	case 0x8 ... 0xf:
		boot_dev = NAND_BOOT;
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
	return system_rev;
}

static inline void setup_soc_rev(void)
{
	int reg;

	/* Si rev is obtained from ROM */
	reg = __REG(ROM_SI_REV);

	switch (reg) {
	case 0x10:
		system_rev = 0x53000 | CHIP_REV_1_0;
		break;
	case 0x20:
		system_rev = 0x53000 | CHIP_REV_2_0;
		break;
	default:
		system_rev = 0x53000 | CHIP_REV_2_0;
	}
}

static inline void setup_board_rev(int rev)
{
	system_rev |= (rev & 0xF) << 8;
}

inline int is_soc_rev(int rev)
{
	return (system_rev & 0xFF) - rev;
}

#ifdef CONFIG_ARCH_MMU
void board_mmu_init(void)
{
	unsigned long ttb_base = PHYS_SDRAM_1 + 0x4000;
	unsigned long i;

	/*
	* Set the TTB register
	*/
	asm volatile ("mcr  p15,0,%0,c2,c0,0" : : "r"(ttb_base) /*:*/);

	/*
	* Set the Domain Access Control Register
	*/
	i = ARM_ACCESS_DACR_DEFAULT;
	asm volatile ("mcr  p15,0,%0,c3,c0,0" : : "r"(i) /*:*/);

	/*
	* First clear all TT entries - ie Set them to Faulting
	*/
	memset((void *)ttb_base, 0, ARM_FIRST_LEVEL_PAGE_TABLE_SIZE);
	/* Actual   Virtual  Size   Attributes          Function */
	/* Base     Base     MB     cached? buffered?  access permissions */
	/* xxx00000 xxx00000 */
	X_ARM_MMU_SECTION(0x000, 0x000, 0x10,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* ROM, 16M */
	X_ARM_MMU_SECTION(0x070, 0x070, 0x010,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* IRAM */
	X_ARM_MMU_SECTION(0x100, 0x100, 0x040,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* SATA */
	X_ARM_MMU_SECTION(0x180, 0x180, 0x100,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* IPUv3M */
	X_ARM_MMU_SECTION(0x200, 0x200, 0x200,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* GPU */
	X_ARM_MMU_SECTION(0x400, 0x400, 0x300,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* periperals */
	X_ARM_MMU_SECTION(0x700, 0x700, 0x400,
			ARM_CACHEABLE, ARM_BUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CSD0 1G */
	X_ARM_MMU_SECTION(0x700, 0xB00, 0x400,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CSD0 1G */
	X_ARM_MMU_SECTION(0xF00, 0xF00, 0x100,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CS1 EIM control*/
	X_ARM_MMU_SECTION(0xF7F, 0xF7F, 0x040,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* NAND Flash buffer */
	X_ARM_MMU_SECTION(0xF80, 0xF80, 0x001,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* iRam */

	/* Workaround for arm errata #709718 */
	/* Setup PRRR so device is always mapped to non-shared */
	asm volatile ("mrc p15, 0, %0, c10, c2, 0" : "=r"(i) : /*:*/);
	i &= (~(3 << 0x10));
	asm volatile ("mcr p15, 0, %0, c10, c2, 0" : : "r"(i) /*:*/);

	/* Enable MMU */
	MMU_ON();
}
#endif

int dram_init(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
	return 0;
}

static void setup_uart(void)
{
#if defined(CONFIG_MX53_ARD)
	/* UART1 TXD */
	mxc_request_iomux(MX53_PIN_ATA_DIOW, IOMUX_CONFIG_ALT3);
	mxc_iomux_set_pad(MX53_PIN_ATA_DIOW, 0x1E4);

	/* UART1 RXD */
	mxc_request_iomux(MX53_PIN_ATA_DMACK, IOMUX_CONFIG_ALT3);
	mxc_iomux_set_input(MUX_IN_UART1_IPP_UART_RXD_MUX_SELECT_INPUT, 0x3);
	mxc_iomux_set_pad(MX53_PIN_ATA_DMACK, 0x1E4);
#else
	/* MX53 EVK and ARM2 board */
	/* UART1 RXD */
	mxc_request_iomux(MX53_PIN_CSI0_D11, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX53_PIN_CSI0_D11, 0x1E4);
	mxc_iomux_set_input(MUX_IN_UART1_IPP_UART_RXD_MUX_SELECT_INPUT, 0x1);

	/* UART1 TXD */
	mxc_request_iomux(MX53_PIN_CSI0_D10, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX53_PIN_CSI0_D10, 0x1E4);
#endif
}

#ifdef CONFIG_I2C_MXC
static void setup_i2c(unsigned int module_base)
{
	switch (module_base) {
	case I2C1_BASE_ADDR:
#if defined(CONFIG_MX53_ARD)
		/* No device is connected via I2C1 on ARD */
		break;
#else
		/* i2c1 SDA */
		mxc_request_iomux(MX53_PIN_CSI0_D8,
				IOMUX_CONFIG_ALT5 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C1_IPP_SDA_IN_SELECT_INPUT,
				INPUT_CTL_PATH0);
		mxc_iomux_set_pad(MX53_PIN_CSI0_D8, PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);
		/* i2c1 SCL */
		mxc_request_iomux(MX53_PIN_CSI0_D9,
				IOMUX_CONFIG_ALT5 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C1_IPP_SCL_IN_SELECT_INPUT,
				INPUT_CTL_PATH0);
		mxc_iomux_set_pad(MX53_PIN_CSI0_D9, PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);
#endif
		break;
	case I2C2_BASE_ADDR:
		/* i2c2 SDA */
		mxc_request_iomux(MX53_PIN_KEY_ROW3,
				IOMUX_CONFIG_ALT4 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C2_IPP_SDA_IN_SELECT_INPUT,
				INPUT_CTL_PATH0);
		mxc_iomux_set_pad(MX53_PIN_KEY_ROW3,
				PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);

#if	defined(CONFIG_MX53_ARD)
		mxc_request_iomux(MX53_PIN_EIM_EB2,
				IOMUX_CONFIG_ALT5 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C2_IPP_SCL_IN_SELECT_INPUT,
				INPUT_CTL_PATH1);
		mxc_iomux_set_pad(MX53_PIN_EIM_EB2,
				PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);
#else
		/* i2c2 SCL */
		mxc_request_iomux(MX53_PIN_KEY_COL3,
				IOMUX_CONFIG_ALT4 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C2_IPP_SCL_IN_SELECT_INPUT,
				INPUT_CTL_PATH0);
		mxc_iomux_set_pad(MX53_PIN_KEY_COL3,
				PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);

#endif
		break;
	case I2C3_BASE_ADDR:
#if	defined(CONFIG_MX53_ARD)
		/* GPIO_3 for I2C3_SCL */
		mxc_request_iomux(MX53_PIN_GPIO_3,
				IOMUX_CONFIG_ALT2 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C3_IPP_SCL_IN_SELECT_INPUT,
				INPUT_CTL_PATH1);
		mxc_iomux_set_pad(MX53_PIN_GPIO_3,
				PAD_CTL_PUE_PULL | PAD_CTL_PKE_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_360K_PD |
				PAD_CTL_HYS_ENABLE);
		/* GPIO_16 for I2C3_SDA */
		mxc_request_iomux(MX53_PIN_GPIO_16,
				IOMUX_CONFIG_ALT6 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C3_IPP_SDA_IN_SELECT_INPUT,
				INPUT_CTL_PATH1);
		mxc_iomux_set_pad(MX53_PIN_GPIO_16,
				PAD_CTL_PUE_PULL | PAD_CTL_PKE_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_360K_PD |
				PAD_CTL_HYS_ENABLE);
#else
		/* No device is connected via I2C3 in EVK and ARM2 */
#endif
		break;
	default:
		printf("Invalid I2C base: 0x%x\n", module_base);
		break;
	}
}

void setup_core_voltages(void)
{
#if	!defined(CONFIG_MX53_ARD)
	unsigned char buf[4] = { 0 };

	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);

	/* Set core voltage VDDGP to 1.05V for 800MHZ */
	buf[0] = 0x45;
	buf[1] = 0x4a;
	buf[2] = 0x52;
	if (i2c_write(0x8, 24, 1, buf, 3))
		return;

	/* Set DDR voltage VDDA to 1.25V */
	buf[0] = 0;
	buf[1] = 0;
	buf[2] = 0x1a;
	if (i2c_write(0x8, 26, 1, buf, 3))
		return;

	if (is_soc_rev(CHIP_REV_2_0) == 0) {
		/* Set VCC to 1.3V for TO2 */
		buf[0] = 0;
		buf[1] = 0;
		buf[2] = 0x1C;
		if (i2c_write(0x8, 25, 1, buf, 3))
			return;

		/* Set VDDA to 1.3V for TO2 */
		buf[0] = 0;
		buf[1] = 0;
		buf[2] = 0x1C;
		if (i2c_write(0x8, 26, 1, buf, 3))
			return;
	}

	/* need to delay 100 ms to allow power supplies to ramp-up */
	udelay(100000);
#endif
	/* Raise the core frequency to 800MHz */
	writel(0x0, CCM_BASE_ADDR + CLKCTL_CACRR);
}

#ifdef CONFIG_MX53_EVK
static int __read_adc_channel(unsigned int chan)
{
	unsigned char buf[4] = { 0 };

	buf[0] = (0xb0 | ((chan & 0x1) << 3) | ((chan >> 1) & 0x7));

	/* LTC2495 need 410ms delay */
	udelay(410000);

	if (i2c_write(0x14, chan, 0, &buf[0], 1)) {
		printf("%s:i2c_write:error\n", __func__);
		return -1;
	}

	/* LTC2495 need 410ms delay*/
	udelay(410000);

	if (i2c_read(0x14, chan, 0, &buf[0], 3)) {
		printf("%s:i2c_read:error\n", __func__);
		return -1;
	}

	return buf[0] << 16 | buf[1] << 8 | buf[2];
}

static int __lookup_board_id(int adc_val)
{
	int id;

	if (adc_val < 0x3FFFC0)
		id = 0;
	else if (adc_val < 0x461863)
		id = 1;
	else if (adc_val < 0x4C30C4)
		id = 2;
	else if (adc_val < 0x524926)
		id = 3;
	else if (adc_val < 0x586187)
		id = 4;
	else if (adc_val < 0x5E79E9)
		id = 5;
	else if (adc_val < 0x64924A)
		id = 6;
	else if (adc_val < 0x6AAAAC)
		id = 7;
	else if (adc_val < 0x70C30D)
		id = 8;
	else if (adc_val < 0x76DB6F)
		id = 9;
	else if (adc_val < 0x7CF3D0)
		id = 10;
	else if (adc_val < 0x830C32)
		id = 11;
	else if (adc_val < 0x892493)
		id = 12;
	else if (adc_val < 0x8F3CF5)
		id = 13;
	else if (adc_val < 0x955556)
		id = 14;
	else if (adc_val < 0x9B6DB8)
		id = 15;
	else if (adc_val < 0xA18619)
		id = 16;
	else if (adc_val < 0xA79E7B)
		id = 17;
	else if (adc_val < 0xADB6DC)
		id = 18;
	else if (adc_val < 0xB3CF3E)
		id = 19;
	else if (adc_val < 0xB9E79F)
		id = 20;
	else if (adc_val <= 0xC00000)
		id = 21;
		else
		return -1;

	return id;
}

static int __print_board_info(int id0, int id1)
{
	int ret = 0;

	switch (id0) {
	case 21:
		switch (id1) {
		case 15:
			printf("MX53-EVK with DDR2 1GByte RevB\n");

			break;
		case 18:
			printf("MX53-EVK with DDR2 2GByte RevA1\n");

			break;
		case 19:
			printf("MX53-EVK with DDR2 2GByte RevA2\n");
			break;
		default:
			printf("Unkown board id1:%d\n", id1);
			ret = -1;

			break;
		}

		break;
	case 11:
		switch (id1) {
		case 1:
			printf("MX53 1.5V DDR3 x8 CPU Card, Rev. A\n");

			break;
		case 11:
			printf("MX53 1.8V DDR2 x8 CPU Card, Rev. A\n");

			break;
		default:
			printf("Unkown board id1:%d\n", id1);
			ret = -1;

			break;
		}

		break;
	default:
		printf("Unkown board id0:%d\n", id0);

		break;
	}

	return ret;
}

static int _identify_board_fix_up(int id0, int id1)
{
	int ret = 0;

#ifdef CONFIG_CMD_CLOCK
	/* For EVK RevB, set DDR to 400MHz */
	if (id0 == 21 && id1 == 15) {
		ret = clk_config(CONFIG_REF_CLK_FREQ, 400, PERIPH_CLK);
		if (ret < 0)
			return ret;

		ret = clk_config(CONFIG_REF_CLK_FREQ, 400, DDR_CLK);
		if (ret < 0)
			return ret;

		/* set up rev #2 for EVK RevB board */
		setup_board_rev(2);
	}
#endif
	return ret;
}

int identify_board_id(void)
{
	int ret = 0;
	int bd_id0, bd_id1;

#define CPU_CHANNEL_ID0 0xc
#define CPU_CHANNEL_ID1 0xd

	ret = bd_id0 = __read_adc_channel(CPU_CHANNEL_ID0);
	if (ret < 0)
		return ret;

	ret = bd_id1 = __read_adc_channel(CPU_CHANNEL_ID1);
	if (ret < 0)
		return ret;

	ret = bd_id0 = __lookup_board_id(bd_id0);
	if (ret < 0)
		return ret;

	ret = bd_id1 = __lookup_board_id(bd_id1);
	if (ret < 0)
		return ret;

	ret = __print_board_info(bd_id0, bd_id1);
	if (ret < 0)
		return ret;

	ret = _identify_board_fix_up(bd_id0, bd_id1);

	return ret;

}
#endif
#endif

#ifdef CONFIG_IMX_ECSPI
s32 spi_get_cfg(struct imx_spi_dev_t *dev)
{
	switch (dev->slave.cs) {
	case 0:
		/* pmic */
		dev->base = CSPI1_BASE_ADDR;
		dev->freq = 2500000;
		dev->ss_pol = IMX_SPI_ACTIVE_HIGH;
		dev->ss = 0;
		dev->fifo_sz = 64 * 4;
		dev->us_delay = 0;
		break;
	case 1:
		/* spi_nor */
		dev->base = CSPI1_BASE_ADDR;
		dev->freq = 2500000;
		dev->ss_pol = IMX_SPI_ACTIVE_LOW;
		dev->ss = 1;
		dev->fifo_sz = 64 * 4;
		dev->us_delay = 0;
		break;
	default:
		printf("Invalid Bus ID! \n");
		break;
	}

	return 0;
}

void spi_io_init(struct imx_spi_dev_t *dev)
{
	switch (dev->base) {
	case CSPI1_BASE_ADDR:
		/* Select mux mode: ALT4 mux port: MOSI of instance: ecspi1 */
		mxc_request_iomux(MX53_PIN_EIM_D18, IOMUX_CONFIG_ALT4);
		mxc_iomux_set_pad(MX53_PIN_EIM_D18, 0x104);
		mxc_iomux_set_input(
				MUX_IN_ECSPI1_IPP_IND_MOSI_SELECT_INPUT, 0x3);

		/* Select mux mode: ALT4 mux port: MISO of instance: ecspi1. */
		mxc_request_iomux(MX53_PIN_EIM_D17, IOMUX_CONFIG_ALT4);
		mxc_iomux_set_pad(MX53_PIN_EIM_D17, 0x104);
		mxc_iomux_set_input(
				MUX_IN_ECSPI1_IPP_IND_MISO_SELECT_INPUT, 0x3);

		if (dev->ss == 0) {
			/* de-select SS1 of instance: ecspi1. */
			mxc_request_iomux(MX53_PIN_EIM_D19, IOMUX_CONFIG_ALT1);
			mxc_iomux_set_pad(MX53_PIN_EIM_D19, 0x1E4);

			/* mux mode: ALT4 mux port: SS0 of instance: ecspi1. */
			mxc_request_iomux(MX53_PIN_EIM_EB2, IOMUX_CONFIG_ALT4);
			mxc_iomux_set_pad(MX53_PIN_EIM_EB2, 0x104);
			mxc_iomux_set_input(
				MUX_IN_ECSPI1_IPP_IND_SS_B_1_SELECT_INPUT, 0x3);
		} else if (dev->ss == 1) {
			/* de-select SS0 of instance: ecspi1. */
			mxc_request_iomux(MX53_PIN_EIM_EB2, IOMUX_CONFIG_ALT1);
			mxc_iomux_set_pad(MX53_PIN_EIM_EB2, 0x1E4);

			/* mux mode: ALT0 mux port: SS1 of instance: ecspi1. */
			mxc_request_iomux(MX53_PIN_EIM_D19, IOMUX_CONFIG_ALT4);
			mxc_iomux_set_pad(MX53_PIN_EIM_D19, 0x104);
			mxc_iomux_set_input(
				MUX_IN_ECSPI1_IPP_IND_SS_B_2_SELECT_INPUT, 0x2);
		}

		/* Select mux mode: ALT0 mux port: SCLK of instance: ecspi1. */
		mxc_request_iomux(MX53_PIN_EIM_D16, IOMUX_CONFIG_ALT4);
		mxc_iomux_set_pad(MX53_PIN_EIM_D16, 0x104);
		mxc_iomux_set_input(
			MUX_IN_CSPI_IPP_CSPI_CLK_IN_SELECT_INPUT, 0x3);
		break;

	case CSPI2_BASE_ADDR:
	default:

		break;
	}
}
#endif

#ifdef CONFIG_MXC_FEC

#ifdef CONFIG_GET_FEC_MAC_ADDR_FROM_IIM

int fec_get_mac_addr(unsigned char *mac)
{
	u32 *iim1_mac_base =
		(u32 *)(IIM_BASE_ADDR + IIM_BANK_AREA_1_OFFSET +
			CONFIG_IIM_MAC_ADDR_OFFSET);
	int i;

	for (i = 0; i < 6; ++i, ++iim1_mac_base)
		mac[i] = (u8)readl(iim1_mac_base);

	return 0;
}
#endif

static void setup_fec(void)
{
	volatile unsigned int reg;

	/*FEC_MDIO*/
	mxc_request_iomux(MX53_PIN_FEC_MDIO, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_MDIO, 0x1FC);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_MDI_SELECT_INPUT, 0x1);

	/*FEC_MDC*/
	mxc_request_iomux(MX53_PIN_FEC_MDC, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_MDC, 0x004);

	/* FEC RXD1 */
	mxc_request_iomux(MX53_PIN_FEC_RXD1, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_RXD1, 0x180);

	/* FEC RXD0 */
	mxc_request_iomux(MX53_PIN_FEC_RXD0, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_RXD0, 0x180);

	 /* FEC TXD1 */
	mxc_request_iomux(MX53_PIN_FEC_TXD1, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_TXD1, 0x004);

	/* FEC TXD0 */
	mxc_request_iomux(MX53_PIN_FEC_TXD0, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_TXD0, 0x004);

	/* FEC TX_EN */
	mxc_request_iomux(MX53_PIN_FEC_TX_EN, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_TX_EN, 0x004);

	/* FEC TX_CLK */
	mxc_request_iomux(MX53_PIN_FEC_REF_CLK, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_REF_CLK, 0x180);

	/* FEC RX_ER */
	mxc_request_iomux(MX53_PIN_FEC_RX_ER, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_RX_ER, 0x180);

	/* FEC CRS */
	mxc_request_iomux(MX53_PIN_FEC_CRS_DV, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_CRS_DV, 0x180);

	/* phy reset: gpio7-6 */
	mxc_request_iomux(MX53_PIN_ATA_DA_0, IOMUX_CONFIG_ALT1);

	reg = readl(GPIO7_BASE_ADDR + 0x0);
	reg &= ~0x40;
	writel(reg, GPIO7_BASE_ADDR + 0x0);

	reg = readl(GPIO7_BASE_ADDR + 0x4);
	reg |= 0x40;
	writel(reg, GPIO7_BASE_ADDR + 0x4);

	udelay(500);

	reg = readl(GPIO7_BASE_ADDR + 0x0);
	reg |= 0x40;
	writel(reg, GPIO7_BASE_ADDR + 0x0);

}
#endif

#if defined(CONFIG_MXC_KPD)
int setup_mxc_kpd(void)
{
	mxc_request_iomux(MX53_PIN_KEY_COL0, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_KEY_COL1, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_KEY_COL2, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_KEY_COL3, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_KEY_COL4, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_GPIO_19,  IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_KEY_ROW0, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_KEY_ROW1, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_KEY_ROW2, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_KEY_ROW3, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_KEY_ROW4, IOMUX_CONFIG_ALT0);

	return 0;
}
#endif

#ifdef CONFIG_NET_MULTI
int board_eth_init(bd_t *bis)
{
	int rc = -ENODEV;
#if defined(CONFIG_SMC911X)
	rc = smc911x_initialize(0, CONFIG_SMC911X_BASE);
#endif

	return rc;
}
#endif

#if defined(CONFIG_MX53_ARD)
void weim_smc911x_iomux()
{
	unsigned int reg;

	/* ETHERNET_INT_B as GPIO2_31 */
	mxc_request_iomux(MX53_PIN_EIM_EB3,
		IOMUX_CONFIG_ALT1);
	reg = readl(GPIO2_BASE_ADDR + 0x4);
	reg &= ~(0x80000000);
	writel(reg, GPIO2_BASE_ADDR + 0x4);

	/* Data bus */
	mxc_request_iomux(MX53_PIN_EIM_D16,
		IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D16, 0xA4);

	mxc_request_iomux(MX53_PIN_EIM_D17,
		IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D17, 0xA4);

	mxc_request_iomux(MX53_PIN_EIM_D18,
		IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D18, 0xA4);

	mxc_request_iomux(MX53_PIN_EIM_D19,
		IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D19, 0xA4);

	mxc_request_iomux(MX53_PIN_EIM_D20,
		IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D20, 0xA4);

	mxc_request_iomux(MX53_PIN_EIM_D21,
		IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D21, 0xA4);

	mxc_request_iomux(MX53_PIN_EIM_D22,
		IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D22, 0xA4);

	mxc_request_iomux(MX53_PIN_EIM_D23,
		IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D23, 0xA4);

	mxc_request_iomux(MX53_PIN_EIM_D24,
		IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D24, 0xA4);

	mxc_request_iomux(MX53_PIN_EIM_D25,
		IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D25, 0xA4);

	mxc_request_iomux(MX53_PIN_EIM_D26,
		IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D26, 0xA4);

	mxc_request_iomux(MX53_PIN_EIM_D27,
		IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D27, 0xA4);

	mxc_request_iomux(MX53_PIN_EIM_D28,
		IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D28, 0xA4);

	mxc_request_iomux(MX53_PIN_EIM_D29,
		IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D29, 0xA4);

	mxc_request_iomux(MX53_PIN_EIM_D30,
		IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D30, 0xA4);

	mxc_request_iomux(MX53_PIN_EIM_D31,
		IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_D31, 0xA4);

	/* Address lines */
	mxc_request_iomux(MX53_PIN_EIM_DA0,
		IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA0, 0xA4);

	mxc_request_iomux(MX53_PIN_EIM_DA1,
		IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA1, 0xA4);

	mxc_request_iomux(MX53_PIN_EIM_DA2,
		IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA2, 0xA4);

	mxc_request_iomux(MX53_PIN_EIM_DA3,
		IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA3, 0xA4);

	mxc_request_iomux(MX53_PIN_EIM_DA4,
		IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA4, 0xA4);

	mxc_request_iomux(MX53_PIN_EIM_DA5,
		IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA5, 0xA4);

	mxc_request_iomux(MX53_PIN_EIM_DA6,
		IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA6, 0xA4);

	/* other EIM signals for ethernet */
	mxc_request_iomux(MX53_PIN_EIM_OE,
		IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_EIM_RW,
		IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_EIM_CS1,
		IOMUX_CONFIG_ALT0);

}

void weim_cs1_settings()
{
	unsigned int reg;

	writel(0x20001, (WEIM_BASE_ADDR + 0x18));
	writel(0x0, (WEIM_BASE_ADDR + 0x1C));
	writel(0x16000202, (WEIM_BASE_ADDR + 0x20));
	writel(0x00000002, (WEIM_BASE_ADDR + 0x24));
	writel(0x16002082, (WEIM_BASE_ADDR + 0x28));
	writel(0x00000000, (WEIM_BASE_ADDR + 0x2C));
	writel(0x00000000, (WEIM_BASE_ADDR + 0x90));

	/* specify 64 MB on CS1 and CS0 */
	reg = readl(IOMUXC_BASE_ADDR + 0x4);
	reg &= ~0x3F;
	reg |= 0x1B;
	writel(reg, (IOMUXC_BASE_ADDR + 0x4));
}
#endif


#ifdef CONFIG_CMD_MMC

#if defined(CONFIG_MX53_ARD)
struct fsl_esdhc_cfg esdhc_cfg[2] = {
	{MMC_SDHC1_BASE_ADDR, 1, 1},
	{MMC_SDHC2_BASE_ADDR, 1, 1},
};
#else
struct fsl_esdhc_cfg esdhc_cfg[2] = {
	{MMC_SDHC1_BASE_ADDR, 1, 1},
	{MMC_SDHC3_BASE_ADDR, 1, 1},
};
#endif

#ifdef CONFIG_DYNAMIC_MMC_DEVNO
int get_mmc_env_devno()
{
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
	return (soc_sbmr & 0x00300000)  ? 1 : 0;
}
#endif


int esdhc_gpio_init(bd_t *bis)
{
	s32 status = 0;
	u32 index = 0;

	for (index = 0; index < CONFIG_SYS_FSL_ESDHC_NUM;
		++index) {
		switch (index) {
		case 0:
			mxc_request_iomux(MX53_PIN_SD1_CMD, IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX53_PIN_SD1_CLK, IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX53_PIN_SD1_DATA0,
						IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX53_PIN_SD1_DATA1,
						IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX53_PIN_SD1_DATA2,
						IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX53_PIN_SD1_DATA3,
						IOMUX_CONFIG_ALT0);

			mxc_iomux_set_pad(MX53_PIN_SD1_CMD, 0x1E4);
			mxc_iomux_set_pad(MX53_PIN_SD1_CLK, 0xD4);
			mxc_iomux_set_pad(MX53_PIN_SD1_DATA0, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_SD1_DATA1, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_SD1_DATA2, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_SD1_DATA3, 0x1D4);
			break;
		case 1:
#if defined(CONFIG_MX53_ARD)
			mxc_request_iomux(MX53_PIN_SD2_CMD,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
			mxc_request_iomux(MX53_PIN_SD2_CLK,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
			mxc_request_iomux(MX53_PIN_SD2_DATA0,
						IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX53_PIN_SD2_DATA1,
						IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX53_PIN_SD2_DATA2,
						IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX53_PIN_SD2_DATA3,
						IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX53_PIN_ATA_DATA12,
						IOMUX_CONFIG_ALT2);
			mxc_request_iomux(MX53_PIN_ATA_DATA13,
						IOMUX_CONFIG_ALT2);
			mxc_request_iomux(MX53_PIN_ATA_DATA14,
						IOMUX_CONFIG_ALT2);
			mxc_request_iomux(MX53_PIN_ATA_DATA15,
						IOMUX_CONFIG_ALT2);

			mxc_iomux_set_pad(MX53_PIN_SD2_CMD, 0x1E4);
			mxc_iomux_set_pad(MX53_PIN_SD2_CLK, 0xD4);
			mxc_iomux_set_pad(MX53_PIN_SD2_DATA0, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_SD2_DATA1, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_SD2_DATA2, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_SD2_DATA3, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA12, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA13, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA14, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA15, 0x1D4);

#else
			mxc_request_iomux(MX53_PIN_ATA_RESET_B,
						IOMUX_CONFIG_ALT2);
			mxc_request_iomux(MX53_PIN_ATA_IORDY,
						IOMUX_CONFIG_ALT2);
			mxc_request_iomux(MX53_PIN_ATA_DATA8,
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA9,
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA10,
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA11,
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA0,
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA1,
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA2,
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA3,
						IOMUX_CONFIG_ALT4);

			mxc_iomux_set_pad(MX53_PIN_ATA_RESET_B, 0x1E4);
			mxc_iomux_set_pad(MX53_PIN_ATA_IORDY, 0xD4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA8, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA9, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA10, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA11, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA0, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA1, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA2, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA3, 0x1D4);
#endif
			break;
		default:
			printf("Warning: you configured more ESDHC controller"
				"(%d) as supported by the board(2)\n",
				CONFIG_SYS_FSL_ESDHC_NUM);
			return status;
			break;
		}
		status |= fsl_esdhc_initialize(bis, &esdhc_cfg[index]);
	}

	return status;
}

int board_mmc_init(bd_t *bis)
{
	if (!esdhc_gpio_init(bis))
		return 0;
	else
		return -1;
}

#endif

#ifdef CONFIG_MXC_NAND
void setup_nfc(void)
{
	u32 i, reg;
	#define M4IF_GENP_WEIM_MM_MASK          0x00000001
	#define WEIM_GCR2_MUX16_BYP_GRANT_MASK  0x00001000

	reg = __raw_readl(M4IF_BASE_ADDR + 0xc);
	reg &= ~M4IF_GENP_WEIM_MM_MASK;
	__raw_writel(reg, M4IF_BASE_ADDR + 0xc);
	for (i = 0x4; i < 0x94; i += 0x18) {
		reg = __raw_readl(WEIM_BASE_ADDR + i);
		reg &= ~WEIM_GCR2_MUX16_BYP_GRANT_MASK;
		__raw_writel(reg, WEIM_BASE_ADDR + i);
	}

	/* To be compatible with some old NAND flash,
	 * limit NFC clocks as 34MHZ. The user can modify
	 * it according to dedicate NAND flash
	 */
	clk_config(0, 34, NFC_CLK);

#if defined(CONFIG_MX53_ARD)
	mxc_request_iomux(MX53_PIN_NANDF_CS0,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_NANDF_CS0,
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_NANDF_CS1,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_NANDF_CS1,
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_NANDF_RB0,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_NANDF_RB0,
			PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
			PAD_CTL_100K_PU);
	mxc_request_iomux(MX53_PIN_NANDF_CLE,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_NANDF_CLE,
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_NANDF_ALE,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_NANDF_ALE,
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_NANDF_WP_B,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_NANDF_WP_B,
			PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
			PAD_CTL_100K_PU);
	mxc_request_iomux(MX53_PIN_NANDF_RE_B,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_NANDF_RE_B,
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_NANDF_WE_B,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_NANDF_WE_B,
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_EIM_DA0,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA0,
			PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PU |
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_EIM_DA1,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA1,
			PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PU |
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_EIM_DA2,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA2,
			PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PU |
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_EIM_DA3,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA3,
			PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PU |
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_EIM_DA4,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA4,
			PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PU |
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_EIM_DA5,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA5,
			PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PU |
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_EIM_DA6,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA6,
			PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PU |
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_EIM_DA7,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA7,
			PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PU |
			PAD_CTL_DRV_HIGH);
#else
	mxc_request_iomux(MX53_PIN_NANDF_CS0,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_NANDF_CS0,
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_NANDF_CS1,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_NANDF_CS1,
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_NANDF_CS2,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_NANDF_CS2,
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_NANDF_CS3,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_NANDF_CS3,
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_NANDF_RB0,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_NANDF_RB0,
			PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
			PAD_CTL_100K_PU);
	mxc_request_iomux(MX53_PIN_NANDF_CLE,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_NANDF_CLE,
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_NANDF_ALE,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_NANDF_ALE,
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_NANDF_WP_B,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_NANDF_WP_B,
			PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
			PAD_CTL_100K_PU);
	mxc_request_iomux(MX53_PIN_NANDF_RE_B,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_NANDF_RE_B,
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_NANDF_WE_B,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_NANDF_WE_B,
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_EIM_DA0,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA0,
			PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PU |
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_EIM_DA1,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA1,
			PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PU |
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_EIM_DA2,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA2,
			PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PU |
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_EIM_DA3,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA3,
			PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PU |
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_EIM_DA4,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA4,
			PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PU |
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_EIM_DA5,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA5,
			PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PU |
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_EIM_DA6,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA6,
			PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PU |
			PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_EIM_DA7,
			IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA7,
			PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PU |
			PAD_CTL_DRV_HIGH);
#endif
}
#endif

int board_init(void)
{
#ifdef CONFIG_MFG
/* MFG firmware need reset usb to avoid host crash firstly */
#define USBCMD 0x140
	int val = readl(OTG_BASE_ADDR + USBCMD);
	val &= ~0x1; /*RS bit*/
	writel(val, OTG_BASE_ADDR + USBCMD);
#endif
	setup_boot_device();
	setup_soc_rev();
#if defined(CONFIG_MX53_ARM2) || defined(CONFIG_MX53_ARM2_DDR3)
	setup_board_rev(1);
#endif

#if defined(CONFIG_MX53_ARD)
	gd->bd->bi_arch_number = MACH_TYPE_MX53_ARD;
#else
	gd->bd->bi_arch_number = MACH_TYPE_MX53_EVK;	/* board id for linux */
#endif
	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	setup_uart();

#ifdef CONFIG_MXC_NAND
	setup_nfc();
#endif

#ifdef CONFIG_MXC_FEC
	setup_fec();
#endif

#ifdef CONFIG_I2C_MXC
	setup_i2c(CONFIG_SYS_I2C_PORT);
	setup_core_voltages();
#endif

#if defined(CONFIG_MX53_ARD)
	weim_smc911x_iomux();
	weim_cs1_settings();
#endif
	return 0;
}


#ifdef CONFIG_ANDROID_RECOVERY
struct reco_envs supported_reco_envs[BOOT_DEV_NUM] = {
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC,
	 .args = CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC,
	 },
	{
	 .cmd = CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC,
	 .args = CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
};

int check_recovery_cmd_file(void)
{
	disk_partition_t info;
	ulong part_length;
	int filelen;
	char *env;

	/* For test only */
	/* When detecting android_recovery_switch,
	 * enter recovery mode directly */
	env = getenv("android_recovery_switch");
	if (!strcmp(env, "1")) {
		printf("Env recovery detected!\nEnter recovery mode!\n");
		return 1;
	}

	printf("Checking for recovery command file...\n");
	switch (get_boot_device()) {
	case MMC_BOOT:
	case SD_BOOT:
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
				printf("** Bad ext2 partition or "
					"disk - mmc 0:%d **\n",
					CONFIG_ANDROID_CACHE_PARTITION_MMC);
				ext2fs_close();
				return 0;
			}

			filelen = ext2fs_open(CONFIG_ANDROID_RECOVERY_CMD_FILE);

			ext2fs_close();
		}
		break;
	case NAND_BOOT:
		return 0;
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
	return 0;
}

int checkboard(void)
{
	printf("Board: ");

/* On EVK, DDR frequency will be bumped up to 400 MHz in identify_board_id() */
#if !defined(CONFIG_MX53_EVK)
	/* Bump up the DDR frequency to 400 MHz */
	if (clk_config(CONFIG_REF_CLK_FREQ, 400, PERIPH_CLK) >= 0)
		clk_config(CONFIG_REF_CLK_FREQ, 400, DDR_CLK);
#endif

#if defined(CONFIG_MX53_ARD)
	printf("MX53-ARD 1.0 [");
#elif defined(CONFIG_MX53_ARM2) || defined(CONFIG_MX53_ARM2_DDR3)
	printf("Board: MX53 ARMADILLO2 ");
	printf("1.0 [");
#elif defined(CONFIG_MX53_EVK)
#ifdef CONFIG_I2C_MXC
	identify_board_id();

	printf("Boot Reason: [");
#endif
#else
	# error "Unknown board config!"
#endif

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
	case WEIM_NOR_BOOT:
		printf("NOR\n");
		break;
	case ONE_NAND_BOOT:
		printf("ONE NAND\n");
		break;
	case PATA_BOOT:
		printf("PATA\n");
		break;
	case SATA_BOOT:
		printf("SATA\n");
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
	case NAND_BOOT:
		printf("NAND\n");
		break;
	case UNKNOWN_BOOT:
	default:
		printf("UNKNOWN\n");
		break;
	}
	return 0;
}
