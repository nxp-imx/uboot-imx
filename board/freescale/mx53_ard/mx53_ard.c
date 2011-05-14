/*
 * Copyright (C) 2007, Guennadi Liakhovetski <lg@denx.de>
 *
 * (C) Copyright 2009-2011 Freescale Semiconductor, Inc.
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
#if defined(CONFIG_VIDEO_MX5)
#include <asm/imx_pwm.h>
#include <linux/list.h>
#include <linux/fb.h>
#include <linux/mxcfb.h>
#include <ipu.h>
#include <lcd.h>
#endif
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

#ifdef CONFIG_VIDEO_MX5
extern unsigned char fsl_bmp_600x400[];
extern int fsl_bmp_600x400_size;

#if defined(CONFIG_BMP_8BPP)
unsigned short colormap[256];
#elif defined(CONFIG_BMP_16BPP)
unsigned short colormap[65536];
#else
unsigned short colormap[16777216];
#endif

struct pwm_device pwm0 = {
	.pwm_id = 0,
	.pwmo_invert = 1,
};

struct pwm_device pwm1 = {
	.pwm_id = 1,
	.pwmo_invert = 1,
};

static int di = 1;

extern int ipuv3_fb_init(struct fb_videomode *mode, int di,
			int interface_pix_fmt,
			ipu_di_clk_parent_t di_clk_parent,
			int di_clk_val);

static struct fb_videomode lvds_xga = {
	 "XGA", 60, 1024, 768, 15385, 220, 40, 21, 7, 60, 10,
	 FB_SYNC_EXT,
	 FB_VMODE_NONINTERLACED,
	 0,
};

vidinfo_t panel_info;
#endif

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

u32 get_board_rev_from_fuse(void)
{
	u32 board_rev = readl(IIM_BASE_ADDR + 0x878);

	return board_rev;
}

u32 get_board_id_from_fuse(void)
{
	u32 board_id = readl(IIM_BASE_ADDR + 0x87c);

	return board_id;
}

u32 get_board_rev(void)
{
	return system_rev;
}

static inline void setup_soc_rev(void)
{
	int reg;
	u32 board_rev = get_board_rev_from_fuse();

	/* Si rev is obtained from ROM */
	reg = __REG(ROM_SI_REV);

	switch (reg) {
	case 0x10:
		system_rev = 0x53000 | CHIP_REV_1_0;
		break;
	case 0x20:
		system_rev = 0x53000 | CHIP_REV_2_0;
		break;
	case 0x21:
		system_rev = 0x53000 | CHIP_REV_2_1;
		break;
	default:
		system_rev = 0x53000 | CHIP_REV_UNKNOWN;
	}

	switch (board_rev) {
	case 0x01:
		system_rev |= BOARD_REV_1;
		break;
	case 0x02:
	default:
		system_rev |= BOARD_REV_2;
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

#if defined(CONFIG_MX53_ARD_DDR3)
	gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
	gd->bd->bi_dram[1].size = PHYS_SDRAM_2_SIZE;
#endif
	return 0;
}

static void setup_uart(void)
{
	/* UART1 TXD */
	mxc_request_iomux(MX53_PIN_ATA_DIOW, IOMUX_CONFIG_ALT3);
	mxc_iomux_set_pad(MX53_PIN_ATA_DIOW, 0x1E4);

	/* UART1 RXD */
	mxc_request_iomux(MX53_PIN_ATA_DMACK, IOMUX_CONFIG_ALT3);
	mxc_iomux_set_input(MUX_IN_UART1_IPP_UART_RXD_MUX_SELECT_INPUT, 0x3);
	mxc_iomux_set_pad(MX53_PIN_ATA_DMACK, 0x1E4);
}

#ifdef CONFIG_I2C_MXC
static void setup_i2c(unsigned int module_base)
{
	switch (module_base) {
	case I2C1_BASE_ADDR:
		/* No device is connected via I2C1 on ARD */
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

		mxc_request_iomux(MX53_PIN_EIM_EB2,
				IOMUX_CONFIG_ALT5 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C2_IPP_SCL_IN_SELECT_INPUT,
				INPUT_CTL_PATH1);
		mxc_iomux_set_pad(MX53_PIN_EIM_EB2,
				PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);
		break;
	case I2C3_BASE_ADDR:
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
				INPUT_CTL_PATH2);
		mxc_iomux_set_pad(MX53_PIN_GPIO_16,
				PAD_CTL_PUE_PULL | PAD_CTL_PKE_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_360K_PD |
				PAD_CTL_HYS_ENABLE);
		break;
	default:
		printf("Invalid I2C base: 0x%x\n", module_base);
		break;
	}
}

void setup_pmic_voltages(void)
{
  int value;
  /* Increase VDDGP as 1.1v for 800MHZ
   * Since VDDGP voltage is caculated via resistors, the actual
   * voltage is only about 1.1v
   */
   if (is_soc_rev(CHIP_REV_2_0) == 0) {
    i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
    /* Update B2DTV1 */
    /* VCC = (1+R1/R2)*(0.3625 + B1DTV1*0.0125)
     * = (1 + 100/180)*(0.3625 + 27*0.0125)
     * =  1.1083V
     */
    value |= 0x1C;
    i2c_write(0x34, 0x23, 1, &value, 1);
    /* Update VCCR */
    i2c_read(0x34, 0x20, 1, &value, 1);
    value = (value & 0xfc) | 0x1;
    i2c_write(0x34, 0x20, 1, &value, 1);
    /* Update SCR1 */
    i2c_read(0x34, 0x12, 1, &value, 1);
    value = (value & 0xfe) | 0x1;
    i2c_write(0x34, 0x12, 1, &value, 1);
    /* Update OVEN */
    i2c_read(0x34, 0x10, 1, &value, 1);
    value = (value & 0xfe) | 0x1;
    i2c_write(0x34, 0x10, 1, &value, 1);
    udelay(10000);
   }
}
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
		printf("Invalid Bus ID!\n");
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

#ifdef CONFIG_CMD_MMC

struct fsl_esdhc_cfg esdhc_cfg[2] = {
	{MMC_SDHC1_BASE_ADDR, 1, 1},
	{MMC_SDHC2_BASE_ADDR, 1, 1},
};

#ifdef CONFIG_DYNAMIC_MMC_DEVNO
int get_mmc_env_devno()
{
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
	return (soc_sbmr & 0x00300000) ? 1 : 0;
}
#endif

#ifdef CONFIG_EMMC_DDR_PORT_DETECT
int detect_mmc_emmc_ddr_port(struct fsl_esdhc_cfg *cfg)
{
	return (MMC_SDHC3_BASE_ADDR == cfg->esdhc_base) ? 1 : 0;
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

#ifdef CONFIG_LCD
void enable_pwm1_pad(void)
{
	mxc_request_iomux(MX53_PIN_DISP0_DAT9, IOMUX_CONFIG_ALT2);
}

void disable_pwm1_pad(void)
{
	unsigned int reg;

	mxc_request_iomux(MX53_PIN_DISP0_DAT9, IOMUX_CONFIG_ALT1);
	reg = readl(GPIO4_BASE_ADDR + 0x4);
	reg |= 0x40000000;
	writel(reg, GPIO4_BASE_ADDR + 0x4);
	reg = readl(GPIO4_BASE_ADDR + 0x0);
	reg |= 0x40000000;
	writel(reg, GPIO4_BASE_ADDR + 0x0);
}

void enable_pwm1_clk(void)
{
	unsigned int reg;

	reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR2);
	reg |= 0x30000;
	writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR2);
}

void disable_pwm1_clk(void)
{
	unsigned int reg;

	reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR2);
	reg &= ~0x30000;
	writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR2);
}

void enable_pwm0_pad(void)
{
	mxc_request_iomux(MX53_PIN_DISP0_DAT8, IOMUX_CONFIG_ALT2);
}

void disable_pwm0_pad(void)
{
	unsigned int reg;

	mxc_request_iomux(MX53_PIN_DISP0_DAT8, IOMUX_CONFIG_ALT1);
	reg = readl(GPIO4_BASE_ADDR + 0x4);
	reg |= 0x40000000;
	writel(reg, GPIO4_BASE_ADDR + 0x4);
	reg = readl(GPIO4_BASE_ADDR + 0x0);
	reg |= 0x20000000;
	writel(reg, GPIO4_BASE_ADDR + 0x0);
}

void enable_pwm0_clk(void)
{
	unsigned int reg;

	reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR2);
	reg |= 0x3000;
	writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR2);
}

void disable_pwm0_clk(void)
{
	unsigned int reg;

	reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR2);
	reg &= ~0x3000;
	writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR2);
}

void lcd_enable(void)
{
	char *s;
	int ret;
	unsigned int reg;

	s = getenv("lvds_num");
	di = simple_strtol(s, NULL, 10);

	/* 200Hz PWM wave, 50% duty */
	if (di == 1) {
		pwm1.enable_pwm_pad = enable_pwm1_pad;
		pwm1.disable_pwm_pad = disable_pwm1_pad;
		pwm1.enable_pwm_clk = enable_pwm1_clk;
		pwm1.disable_pwm_clk = disable_pwm1_clk;
		imx_pwm_config(pwm1, 2500000, 5000000);
		imx_pwm_enable(pwm1);
	} else {
		pwm0.enable_pwm_pad = enable_pwm0_pad;
		pwm0.disable_pwm_pad = disable_pwm0_pad;
		pwm0.enable_pwm_clk = enable_pwm0_clk;
		pwm0.disable_pwm_clk = disable_pwm0_clk;
		imx_pwm_config(pwm0, 2500000, 5000000);
		imx_pwm_enable(pwm0);
	}

	ret = ipuv3_fb_init(&lvds_xga, di, IPU_PIX_FMT_RGB666,
			DI_PCLK_LDB, 65000000);
	if (ret)
		puts("LCD cannot be configured\n");

	reg = readl(CCM_BASE_ADDR + CLKCTL_CSCMR2);
	reg &= ~0xFC000000;
	reg |= 0xB4000F00;
	writel(reg, CCM_BASE_ADDR + CLKCTL_CSCMR2);

	reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR6);
	reg |= 0xF0000000;
	writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR6);

	if (di == 1)
		writel(0x40C, IOMUXC_BASE_ADDR + 0x8);
	else
		writel(0x201, IOMUXC_BASE_ADDR + 0x8);
}
#endif

#ifdef CONFIG_VIDEO_MX5
void panel_info_init(void)
{
	panel_info.vl_bpix = LCD_BPP;
	panel_info.vl_col = lvds_xga.xres;
	panel_info.vl_row = lvds_xga.yres;
	panel_info.cmap = colormap;
}
#endif

#ifdef CONFIG_SPLASH_SCREEN
void setup_splash_image(void)
{
	char *s;
	ulong addr;

	s = getenv("splashimage");

	if (s != NULL) {
		addr = simple_strtoul(s, NULL, 16);

#if defined(CONFIG_ARCH_MMU)
		addr = ioremap_nocache(iomem_to_phys(addr),
				fsl_bmp_600x400_size);
#endif
		memcpy((char *)addr, (char *)fsl_bmp_600x400,
				fsl_bmp_600x400_size);
	}
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
}
#endif

#if defined(CONFIG_DWC_AHSATA)
static void setup_sata_device(void)
{
	/* only RevB board uses internal SATA clock */
#if defined(CONFIG_MX53_ARD_DDR3)
	u32 *tmp_base =
		(u32 *)(IIM_BASE_ADDR + 0x180c);

	/* Set USB_PHY1 clk, fuse bank4 row3 bit2 */
	set_usb_phy1_clk();
	writel((readl(tmp_base) & (~0x7)) | 0x4, tmp_base);
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

	gd->bd->bi_arch_number = MACH_TYPE_MX53_ARD;

	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	setup_uart();

#ifdef CONFIG_MXC_NAND
	setup_nfc();
#endif

#ifdef CONFIG_I2C_MXC
	setup_i2c(CONFIG_SYS_I2C_PORT);
	setup_pmic_voltages();
#endif

	weim_smc911x_iomux();
	weim_cs1_settings();

#if defined(CONFIG_DWC_AHSATA)
	setup_sata_device();
#endif

#ifdef CONFIG_VIDEO_MX5
	panel_info_init();

	gd->fb_base = CONFIG_FB_BASE;
#ifdef CONFIG_ARCH_MMU
	gd->fb_base = ioremap_nocache(iomem_to_phys(gd->fb_base), 0);
#endif
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
	printf("Board: MX53-ARD ");

	switch (get_board_rev_from_fuse()) {
	case 0x2:
		printf("Rev. B\n");
		break;
	case 0x1:
	default:
		printf("Rev. A\n");
		break;

	}

	printf("Boot Reason: [");
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
