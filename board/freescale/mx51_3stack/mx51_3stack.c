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
#include <asm/errno.h>
#include <asm/arch/mx51.h>
#include <asm/arch/mx51_pins.h>
#include <asm/arch/iomux.h>
#include <i2c.h>
#include "board-mx51_3stack.h"
#include <netdev.h>

#ifdef CONFIG_CMD_MMC
#include <mmc.h>
#include <fsl_esdhc.h>
#endif

#ifdef CONFIG_ARCH_MMU
#include <asm/mmu.h>
#include <asm/arch/mmu.h>
#endif

#ifdef CONFIG_ANDROID_RECOVERY
#include "../common/recovery.h"
#include <mxc_keyb.h>
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
			uint bt_mem_type = (soc_sbmr & 0x00000180) >> 7;

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
#ifdef CONFIG_ARCH_MMU
	reg = __REG(0x20000000 + ROM_SI_REV); /* Virtual address */
#else
	reg = __REG(ROM_SI_REV); /* Virtual address */
#endif

	switch (reg) {
	case 0x02:
		system_rev = 0x51000 | CHIP_REV_1_1;
		break;
	case 0x10:
		system_rev = 0x51000 | CHIP_REV_2_0;
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

#ifdef CONFIG_ARCH_MMU
void board_mmu_init(void)
{
	unsigned long ttb_base = PHYS_SDRAM_1 + 0x4000;
	unsigned long i;

	/*
	* Set the TTB register
	*/
	asm volatile ("mcr p15, 0, %0, c2, c0, 0" : : "r"(ttb_base) /*:*/);

	/*
	* Set the Domain Access Control Register
	*/
	i = ARM_ACCESS_DACR_DEFAULT;
	asm volatile ("mcr p15, 0, %0, c3, c0, 0" : : "r"(i) /*:*/);

	/*
	* First clear all TT entries - ie Set them to Faulting
	*/
	memset((void *)ttb_base, 0, ARM_FIRST_LEVEL_PAGE_TABLE_SIZE);
	/* Actual   Virtual  Size   Attributes          Function */
	/* Base     Base     MB     cached? buffered?  access permissions */
	/* xxx00000 xxx00000 */
	X_ARM_MMU_SECTION(0x000, 0x200, 0x1,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* ROM */
	X_ARM_MMU_SECTION(0x1FF, 0x1FF, 0x001,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* IRAM */
	X_ARM_MMU_SECTION(0x300, 0x300, 0x100,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* GPU */
	X_ARM_MMU_SECTION(0x400, 0x400, 0x200,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* IPUv3D */
	X_ARM_MMU_SECTION(0x600, 0x600, 0x300,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* periperals */
	X_ARM_MMU_SECTION(0x900, 0x000, 0x080,
			ARM_CACHEABLE, ARM_BUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* SDRAM */
	X_ARM_MMU_SECTION(0x900, 0x900, 0x080,
			ARM_CACHEABLE, ARM_BUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* SDRAM */
	X_ARM_MMU_SECTION(0x900, 0x980, 0x080,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* SDRAM 0:128M*/
	X_ARM_MMU_SECTION(0xA00, 0xA00, 0x100,
			ARM_CACHEABLE, ARM_BUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* SDRAM */
	X_ARM_MMU_SECTION(0xB80, 0xB80, 0x10,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CS1 EIM control*/
	X_ARM_MMU_SECTION(0xCC0, 0xCC0, 0x040,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CS4/5/NAND Flash buffer */

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

#if defined(CONFIG_MXC_ATA)
int setup_ata(void)
{
	u32 pad;

	pad = (PAD_CTL_DRV_HIGH | PAD_CTL_DRV_VOT_HIGH);

	/* Need to disable nand iomux first */
	mxc_request_iomux(MX51_PIN_NANDF_ALE, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_ALE, pad);

	mxc_request_iomux(MX51_PIN_NANDF_CS2, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_CS2, pad);

	mxc_request_iomux(MX51_PIN_NANDF_CS3, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_CS3, pad);

	mxc_request_iomux(MX51_PIN_NANDF_CS4, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_CS4, pad);

	mxc_request_iomux(MX51_PIN_NANDF_CS5, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_CS5, pad);

	mxc_request_iomux(MX51_PIN_NANDF_CS6, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_CS6, pad);

	mxc_request_iomux(MX51_PIN_NANDF_RE_B, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_RE_B, pad);

	mxc_request_iomux(MX51_PIN_NANDF_WE_B, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_WE_B, pad);

	mxc_request_iomux(MX51_PIN_NANDF_CLE, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_CLE, pad);

	mxc_request_iomux(MX51_PIN_NANDF_RB0, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_RB0, pad);

	mxc_request_iomux(MX51_PIN_NANDF_WP_B, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_WP_B, pad);

	/* TO 2.0 */
	mxc_request_iomux(MX51_PIN_GPIO_NAND, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_GPIO_NAND, pad);

	/* TO 1.0 */
	mxc_request_iomux(MX51_PIN_NANDF_RB5, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_RB5, pad);

	mxc_request_iomux(MX51_PIN_NANDF_RB1, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_RB1, pad);

	mxc_request_iomux(MX51_PIN_NANDF_D0, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_D0, pad);

	mxc_request_iomux(MX51_PIN_NANDF_D1, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_D1, pad);

	mxc_request_iomux(MX51_PIN_NANDF_D2, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_D2, pad);

	mxc_request_iomux(MX51_PIN_NANDF_D3, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_D3, pad);

	mxc_request_iomux(MX51_PIN_NANDF_D4, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_D4, pad);

	mxc_request_iomux(MX51_PIN_NANDF_D5, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_D5, pad);

	mxc_request_iomux(MX51_PIN_NANDF_D6, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_D6, pad);

	mxc_request_iomux(MX51_PIN_NANDF_D7, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_D7, pad);

	mxc_request_iomux(MX51_PIN_NANDF_D8, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_D8, pad);

	mxc_request_iomux(MX51_PIN_NANDF_D9, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_D9, pad);

	mxc_request_iomux(MX51_PIN_NANDF_D10, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_D10, pad);

	mxc_request_iomux(MX51_PIN_NANDF_D11, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_D11, pad);

	mxc_request_iomux(MX51_PIN_NANDF_D12, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_D12, pad);

	mxc_request_iomux(MX51_PIN_NANDF_D13, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_D13, pad);

	mxc_request_iomux(MX51_PIN_NANDF_D14, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_D14, pad);

	mxc_request_iomux(MX51_PIN_NANDF_D15, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX51_PIN_NANDF_D15, pad);

	return 0;
}
#endif

#ifdef CONFIG_I2C_MXC
static setup_i2c(unsigned int module_base)
{
	unsigned int reg;

	switch (module_base) {
	case I2C1_BASE_ADDR:
		reg = IOMUXC_BASE_ADDR + 0x210; /* i2c SDA */
		writel(0x11, reg);
		reg = IOMUXC_BASE_ADDR + 0x600;
		writel(0x1ad, reg);
		reg = IOMUXC_BASE_ADDR + 0x9B4;
		writel(0x1, reg);

		reg = IOMUXC_BASE_ADDR + 0x224; /* i2c SCL */
		writel(0x11, reg);
		reg = IOMUXC_BASE_ADDR + 0x614;
		writel(0x1ad, reg);
		reg = IOMUXC_BASE_ADDR + 0x9B0;
		writel(0x1, reg);
		break;
	case I2C2_BASE_ADDR:
		/* Workaround for Atlas Lite */
		writel(0x0, IOMUXC_BASE_ADDR + 0x3CC); /* i2c SCL */
		writel(0x0, IOMUXC_BASE_ADDR + 0x3D0); /* i2c SDA */
		reg = readl(GPIO1_BASE_ADDR + 0x0);
		reg |= 0xC;  /* write a 1 on the SCL and SDA lines */
		writel(reg, GPIO1_BASE_ADDR + 0x0);
		reg = readl(GPIO1_BASE_ADDR + 0x4);
		reg |= 0xC;  /* configure GPIO lines as output */
		writel(reg, GPIO1_BASE_ADDR + 0x4);
		reg = readl(GPIO1_BASE_ADDR + 0x0);
		reg &= ~0x4 ; /* set SCL low for a few milliseconds */
		writel(reg, GPIO1_BASE_ADDR + 0x0);
		udelay(20000);
		reg |= 0x4;
		writel(reg, GPIO1_BASE_ADDR + 0x0);
		udelay(10);
		reg = readl(GPIO1_BASE_ADDR + 0x4);
		reg &= ~0xC;  /* configure GPIO lines back as input */
		writel(reg, GPIO1_BASE_ADDR + 0x4);

		writel(0x12, IOMUXC_BASE_ADDR + 0x3CC);  /* i2c SCL */
		writel(0x3, IOMUXC_BASE_ADDR + 0x9B8);
		writel(0x1ed, IOMUXC_BASE_ADDR + 0x7D4);

		writel(0x12, IOMUXC_BASE_ADDR + 0x3D0); /* i2c SDA */
		writel(0x3, IOMUXC_BASE_ADDR + 0x9BC);
		writel(0x1ed, IOMUXC_BASE_ADDR + 0x7D8);
		break;
	default:
		printf("Invalid I2C base: 0x%x\n", module_base);
		break;
	}
}

#define REV_ATLAS_LITE_1_0	   0x8
#define REV_ATLAS_LITE_1_1	   0x9
#define REV_ATLAS_LITE_2_0	   0x10
#define REV_ATLAS_LITE_2_1	   0x11

void setup_core_voltages(void)
{
	unsigned char buf[4] = { 0 };

	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);

	if (is_soc_rev(CHIP_REV_2_0) <= 0) {
		/* Set core voltage to 1.1V */
		if (i2c_read(0x8, 24, 1, buf, 3)) {
			puts("setup_core_voltages: read PMIC@0x08:0x18 fail\n");
			return;
		}
		buf[2] = (buf[2] & (~0x1F)) | 0x14;
		if (i2c_write(0x8, 24, 1, buf, 3)) {
			puts("setup_core_voltages: write PMIC@0x08:0x18 fail\n");
			return;
		}

		/* Setup VCC (SW2) to 1.25 */
		if (i2c_read(0x8, 25, 1, buf, 3)) {
			puts("setup_core_voltages: read PMIC@0x08:0x19 fail\n");
			return;
		}
		buf[2] = (buf[2] & (~0x1F)) | 0x1A;
		if (i2c_write(0x8, 25, 1, buf, 3)) {
			puts("setup_core_voltages: write PMIC@0x08:0x19 fail\n");
			return;
		}

		/* Setup 1V2_DIG1 (SW3) to 1.25 */
		if (i2c_read(0x8, 26, 1, buf, 3)) {
			puts("setup_core_voltages: read PMIC@0x08:0x1A fail\n");
			return;
		}
		buf[2] = (buf[2] & (~0x1F)) | 0x1A;
		if (i2c_write(0x8, 26, 1, buf, 3)) {
			puts("setup_core_voltages: write PMIC@0x08:0x1A fail\n");
			return;
		}

		udelay(50);

		/* Raise the core frequency to 800MHz */
		writel(0x0, CCM_BASE_ADDR + CLKCTL_CACRR);
	} else {
		/* TO 3.0 */
		/* Setup VCC (SW2) to 1.225 */
		if (i2c_read(0x8, 25, 1, buf, 3)) {
			puts("setup_core_voltages: read PMIC@0x08:0x19 fail\n");
			return;
		}
		buf[2] = (buf[2] & (~0x1F)) | 0x19;
		if (i2c_write(0x8, 25, 1, buf, 3)) {
			puts("setup_core_voltages: write PMIC@0x08:0x19 fail\n");
			return;
		}

		/* Setup 1V2_DIG1 (SW3) to 1.2 */
		if (i2c_read(0x8, 26, 1, buf, 3)) {
			puts("setup_core_voltages: read PMIC@0x08:0x1A fail\n");
			return;
		}
		buf[2] = (buf[2] & (~0x1F)) | 0x18;
		if (i2c_write(0x8, 26, 1, buf, 3)) {
			puts("setup_core_voltages: write PMIC@0x08:0x1A fail\n");
			return;
		}
	}

	if (i2c_read(0x8, 7, 1, buf, 3)) {
		puts("setup_core_voltages: read PMIC@0x08:0x07 fail\n");
		return;
	}

	if (((buf[2] & 0x1F) < REV_ATLAS_LITE_2_0) || (((buf[1] >> 1) & 0x3) == 0)) {
		/* Set switchers in PWM mode for Atlas 2.0 and lower */
		/* Setup the switcher mode for SW1 & SW2*/
		if (i2c_read(0x8, 28, 1, buf, 3)) {
			puts("setup_core_voltages: read PMIC@0x08:0x1C fail\n");
			return;
		}
		buf[2] = (buf[2] & (~0xF)) | 0x5;
		buf[1] = (buf[1] & (~0x3C)) | 0x14;
		if (i2c_write(0x8, 28, 1, buf, 3)) {
			puts("setup_core_voltages: write PMIC@0x08:0x1C fail\n");
			return;
		}

		/* Setup the switcher mode for SW3 & SW4*/
		if (i2c_read(0x8, 29, 1, buf, 3)) {
			puts("setup_core_voltages: read PMIC@0x08:0x1D fail\n");
			return;
		}
		buf[2] = (buf[2] & (~0xF)) | 0x5;
		buf[1] = (buf[1] & (~0xF)) | 0x5;
		if (i2c_write(0x8, 29, 1, buf, 3)) {
			puts("setup_core_voltages: write PMIC@0x08:0x1D fail\n");
			return;
		}
	} else {
		/* Set switchers in Auto in NORMAL mode & STANDBY mode for Atlas 2.0a */
		/* Setup the switcher mode for SW1 & SW2*/
		if (i2c_read(0x8, 28, 1, buf, 3)) {
			puts("setup_core_voltages: read PMIC@0x08:0x1C fail\n");
			return;
		}
		buf[2] = (buf[2] & (~0xF)) | 0x8;
		buf[1] = (buf[1] & (~0x3C)) | 0x20;
		if (i2c_write(0x8, 28, 1, buf, 3)) {
			puts("setup_core_voltages: write PMIC@0x08:0x1C fail\n");
			return;
		}

		/* Setup the switcher mode for SW3 & SW4*/
		if (i2c_read(0x8, 29, 1, buf, 3)) {
			puts("setup_core_voltages: read PMIC@0x08:0x1D fail\n");
			return;
		}
		buf[2] = (buf[2] & (~0xF)) | 0x8;
		buf[1] = (buf[1] & (~0xF)) | 0x8;
		if (i2c_write(0x8, 29, 1, buf, 3)) {
			puts("setup_core_voltages: write PMIC@0x08:0x1D fail\n");
			return;
		}
	}
}

#endif

int board_init(void)
{
	setup_boot_device();
	setup_soc_rev();

	gd->bd->bi_arch_number = MACH_TYPE_MX51_3DS;	/* board id for linux */
	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	setup_uart();
	setup_nfc();
	setup_expio();
#ifdef CONFIG_I2C_MXC
	setup_i2c(I2C2_BASE_ADDR);
	setup_core_voltages();
#endif
	return 0;
}

#ifdef CONFIG_ANDROID_RECOVERY
struct reco_envs supported_reco_envs[BOOT_DEV_NUM] = {
	{
	 .cmd = CONFIG_ANDROID_RECOVERY_BOOTCMD_NAND,
	 .args = CONFIG_ANDROID_RECOVERY_BOOTARGS_NAND,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC,
	 .args = CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC,
	 },
};

static int check_mmc_recovery_cmd_file(int dev_num, int part_num, char *path)
{
	block_dev_desc_t *dev_desc = NULL;
	struct mmc *mmc = find_mmc_device(dev_num);
	disk_partition_t info;
	ulong part_length = 0;
	int filelen = 0;

	memset(&info, 0, sizeof(disk_partition_t));

	dev_desc = get_dev("mmc", dev_num);

	if (NULL == dev_desc) {
		printf("** Block device MMC %d not supported\n",
				dev_num);
		return 0;
	}

	mmc_init(mmc);

	if (get_partition_info(dev_desc,
			part_num,
			&info)) {
		printf("** Bad partition %d **\n",
			part_num);
		return 0;
	}

	part_length = ext2fs_set_blk_dev(dev_desc,
						part_num);
	if (part_length == 0) {
		printf("** Bad partition - mmc 0:%d **\n",
			part_num);
		ext2fs_close();
		return 0;
	}

	if (!ext2fs_mount(part_length)) {
		printf("** Bad ext2 partition or disk - mmc 0:%d **\n",
			part_num);
		ext2fs_close();
		return 0;
	}

	filelen = ext2fs_open(path);

	ext2fs_close();

	return (filelen > 0) ? 1 : 0;
}

extern int ubifs_init(void);
extern int ubifs_mount(char *vol_name);
extern int ubifs_load(char *filename, u32 addr, u32 size);

static int check_nand_recovery_cmd_file(char *mtd_part_name,
				char *ubi_part_name,
				char *path)
{
	struct mtd_device *dev_desc = NULL;
	struct part_info *part = NULL;
	struct mtd_partition mtd_part;
	struct mtd_info *mtd_info = NULL;
	char mtd_dev[16] = { 0 };
	char mtd_buffer[80] = { 0 };
	u8 pnum = 0,
	   read_test = 0;
	int err = 0,
		filelen = 0;

	memset(&mtd_part, 0, sizeof(struct mtd_partition));

	/* ========== ubi and mtd operations ========== */
	if (mtdparts_init() != 0) {
		printf("Error initializing mtdparts!\n");
		return 0;
	}

	if (find_dev_and_part(mtd_part_name, &dev_desc, &pnum, &part)) {
		printf("Partition %s not found!\n", mtd_part_name);
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
	add_mtd_partitions(mtd_info, &mtd_part, 1);

	err = ubi_mtd_param_parse(mtd_buffer, NULL);
	if (err) {
		del_mtd_partitions(mtd_info);
		return 0;
	}

	err = ubi_init();
	if (err) {
		del_mtd_partitions(mtd_info);
		return 0;
	}

	/* ========== ubifs operations ========== */
	/* Init ubifs */
	ubifs_init();

	if (ubifs_mount(ubi_part_name)) {
		printf("Mount ubifs volume %s fail!\n",
				ubi_part_name);
		return 0;
	}

	/* Try to read one byte for a read test. */
	if (ubifs_load(path, (u32)&read_test, 1)) {
		/* File not found */
		filelen = 0;
	} else
		filelen = 1;

	return filelen;
}

int check_recovery_cmd_file(void)
{
	int if_exist = 0;
	char *env = NULL;

	switch (get_boot_device()) {
	case MMC_BOOT:
		if_exist = check_mmc_recovery_cmd_file(0,
				CONFIG_ANDROID_CACHE_PARTITION_MMC,
				CONFIG_ANDROID_RECOVERY_CMD_FILE);
		break;
	case NAND_BOOT:
		env = getenv("mtdparts");
		if (!env)
			setenv("mtdparts", MTDPARTS_DEFAULT);

		env = getenv("mtdids");
		if (!env)
			setenv("mtdids", MTDIDS_DEFAULT);

		env = getenv("partition");
		if (!env)
			setenv("partition", MTD_ACTIVE_PART);

		/*
		if_exist = check_nand_recovery_cmd_file(CONFIG_ANDROID_UBIFS_PARTITION_NM,
						CONFIG_ANDROID_CACHE_PARTITION_NAND,
						CONFIG_ANDROID_RECOVERY_CMD_FILE);
		*/
		break;
	case SPI_NOR_BOOT:
		return 0;
		break;
	case UNKNOWN_BOOT:
	default:
		return 0;
		break;
	}

	return if_exist;
}
#endif

#ifdef BOARD_LATE_INIT
int board_late_init(void)
{
	return 0;
}
#endif

int checkboard(void)
{
	printf("Board: MX51 3STACK ");

	if (system_rev & CHIP_REV_2_0) {
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

#if defined(CONFIG_SMC911X)
extern int smc911x_initialize(u8 dev_num, int base_addr);
#endif

#ifdef CONFIG_NET_MULTI
int board_eth_init(bd_t *bis)
{
	int rc = -ENODEV;
#if defined(CONFIG_SMC911X)
	rc = smc911x_initialize(0, CONFIG_SMC911X_BASE);
#endif

	cpu_eth_init(bis);

	return rc;
}
#endif

#ifdef CONFIG_CMD_MMC

struct fsl_esdhc_cfg esdhc_cfg[2] = {
	{MMC_SDHC1_BASE_ADDR, 1, 1},
};

#ifdef CONFIG_DYNAMIC_MMC_DEVNO
int get_mmc_env_devno()
{
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
	return (soc_sbmr & 0x00180000) ? 1 : 0;
}
#endif

int esdhc_gpio_init(bd_t *bis)
{
	u32 index = 0;
	s32 status = 0;

	for (index = 0; index < CONFIG_SYS_FSL_ESDHC_NUM;
		++index) {
		switch (index) {
		case 0:
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
			mxc_iomux_set_pad(MX51_PIN_SD1_CMD,
					PAD_CTL_DRV_MAX | PAD_CTL_DRV_VOT_HIGH |
					PAD_CTL_HYS_ENABLE | PAD_CTL_47K_PU |
					PAD_CTL_PUE_PULL |
					PAD_CTL_PKE_ENABLE | PAD_CTL_SRE_FAST);
			mxc_iomux_set_pad(MX51_PIN_SD1_CLK,
					PAD_CTL_DRV_MAX | PAD_CTL_DRV_VOT_HIGH |
					PAD_CTL_HYS_NONE | PAD_CTL_47K_PU |
					PAD_CTL_PUE_PULL |
					PAD_CTL_PKE_ENABLE | PAD_CTL_SRE_FAST);
			mxc_iomux_set_pad(MX51_PIN_SD1_DATA0,
					PAD_CTL_DRV_MAX | PAD_CTL_DRV_VOT_HIGH |
					PAD_CTL_HYS_ENABLE | PAD_CTL_47K_PU |
					PAD_CTL_PUE_PULL |
					PAD_CTL_PKE_ENABLE | PAD_CTL_SRE_FAST);
			mxc_iomux_set_pad(MX51_PIN_SD1_DATA1,
					PAD_CTL_DRV_MAX | PAD_CTL_DRV_VOT_HIGH |
					PAD_CTL_HYS_ENABLE | PAD_CTL_47K_PU |
					PAD_CTL_PUE_PULL |
					PAD_CTL_PKE_ENABLE | PAD_CTL_SRE_FAST);
			mxc_iomux_set_pad(MX51_PIN_SD1_DATA2,
					PAD_CTL_DRV_MAX | PAD_CTL_DRV_VOT_HIGH |
					PAD_CTL_HYS_ENABLE | PAD_CTL_47K_PU |
					PAD_CTL_PUE_PULL |
					PAD_CTL_PKE_ENABLE | PAD_CTL_SRE_FAST);
			mxc_iomux_set_pad(MX51_PIN_SD1_DATA3,
					PAD_CTL_DRV_MAX | PAD_CTL_DRV_VOT_HIGH |
					PAD_CTL_HYS_ENABLE | PAD_CTL_100K_PD |
					PAD_CTL_PUE_PULL |
					PAD_CTL_PKE_ENABLE | PAD_CTL_SRE_FAST);
			break;
		case 1:
			status = 1;
			break;
		case 2:
			status = 1;
			break;
		case 3:
			status = 1;
			break;
		default:
			status = 1;
			break;
		}
		status |= fsl_esdhc_initialize(bis, &esdhc_cfg[index]);
	}

	return status;
}

int board_mmc_init(bd_t *bis)
{
	if (!esdhc_gpio_init(bis))
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
