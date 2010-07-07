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
#include <asm/arch/mx50.h>
#include <asm/arch/mx50_pins.h>
#include <asm/arch/iomux.h>
#include <asm/errno.h>

#ifdef CONFIG_IMX_CSPI
#include <imx_spi.h>
#include <asm/arch/imx_spi_pmic.h>
#endif

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

#ifdef CONFIG_CMD_CLOCK
#include <asm/clock.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

static u32 system_rev;
static enum boot_device boot_dev;
u32	mx51_io_base_addr;

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
	system_rev = 0x50000 | CHIP_REV_1_0;
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

	/* UART1 RXD */
	mxc_request_iomux(MX50_PIN_UART1_RXD, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX50_PIN_UART1_RXD, 0x1E4);
	mxc_iomux_set_input(MUX_IN_UART1_IPP_UART_RXD_MUX_SELECT_INPUT, 0x1);

	/* UART1 TXD */
	mxc_request_iomux(MX50_PIN_UART1_TXD, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX50_PIN_UART1_TXD, 0x1E4);
}

#ifdef CONFIG_I2C_MXC
static void setup_i2c(unsigned int module_base)
{
	switch (module_base) {
	case I2C1_BASE_ADDR:
		/* i2c1 SDA */
		mxc_request_iomux(MX50_PIN_I2C1_SDA,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX50_PIN_I2C1_SDA, PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);
		/* i2c1 SCL */
		mxc_request_iomux(MX50_PIN_I2C1_SCL,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX50_PIN_I2C1_SCL, PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);
		break;
	case I2C2_BASE_ADDR:
		/* i2c2 SDA */
		mxc_request_iomux(MX50_PIN_I2C2_SDA,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX50_PIN_I2C2_SDA,
				PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);

		/* i2c2 SCL */
		mxc_request_iomux(MX50_PIN_I2C2_SCL,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX50_PIN_I2C2_SCL,
				PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);
		break;
	case I2C3_BASE_ADDR:
		/* i2c3 SDA */
		mxc_request_iomux(MX50_PIN_I2C3_SDA,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX50_PIN_I2C3_SDA,
				PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);

		/* i2c3 SCL */
		mxc_request_iomux(MX50_PIN_I2C3_SCL,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX50_PIN_I2C3_SCL,
				PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);
		break;
	default:
		printf("Invalid I2C base: 0x%x\n", module_base);
		break;
	}
}

#endif

#ifdef CONFIG_IMX_CSPI
s32 spi_get_cfg(struct imx_spi_dev_t *dev)
{
	switch (dev->slave.cs) {
	case 0:
		/* PMIC */
		dev->base = CSPI3_BASE_ADDR;
		dev->freq = 25000000;
		dev->ss_pol = IMX_SPI_ACTIVE_HIGH;
		dev->ss = 0;
		dev->fifo_sz = 32;
		dev->us_delay = 0;
		break;
	case 1:
		/* SPI-NOR */
		dev->base = CSPI3_BASE_ADDR;
		dev->freq = 25000000;
		dev->ss_pol = IMX_SPI_ACTIVE_LOW;
		dev->ss = 1;
		dev->fifo_sz = 32;
		dev->us_delay = 0;
		break;
	default:
		printf("Invalid Bus ID! \n");
	}

	return 0;
}

void spi_io_init(struct imx_spi_dev_t *dev)
{
	switch (dev->base) {
	case CSPI3_BASE_ADDR:
		mxc_request_iomux(MX50_PIN_CSPI_MOSI, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX50_PIN_CSPI_MOSI, 0x4);

		mxc_request_iomux(MX50_PIN_CSPI_MISO, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX50_PIN_CSPI_MISO, 0x4);

		if (dev->ss == 0) {
			/* de-select SS1 of instance: cspi */
			mxc_request_iomux(MX50_PIN_ECSPI1_MOSI,
						IOMUX_CONFIG_ALT1);

			mxc_request_iomux(MX50_PIN_CSPI_SS0, IOMUX_CONFIG_ALT0);
			mxc_iomux_set_pad(MX50_PIN_CSPI_SS0, 0xE4);
		} else if (dev->ss == 1) {
			/* de-select SS0 of instance: cspi */
			mxc_request_iomux(MX50_PIN_CSPI_SS0, IOMUX_CONFIG_ALT1);

			mxc_request_iomux(MX50_PIN_ECSPI1_MOSI,
						IOMUX_CONFIG_ALT2);
			mxc_iomux_set_pad(MX50_PIN_ECSPI1_MOSI, 0xE4);
			mxc_iomux_set_input(
			MUX_IN_CSPI_IPP_IND_SS1_B_SELECT_INPUT, 0x1);
		}

		mxc_request_iomux(MX50_PIN_CSPI_SCLK, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX50_PIN_CSPI_SCLK, 0x4);
		break;
	case CSPI2_BASE_ADDR:
	case CSPI1_BASE_ADDR:
		/* ecspi1-2 fall through */
		break;
	default:
		break;
	}
}
#endif

#ifdef CONFIG_MXC_FEC
static void setup_fec(void)
{
	volatile unsigned int reg;

	/*FEC_MDIO*/
	mxc_request_iomux(MX50_PIN_SSI_RXC, IOMUX_CONFIG_ALT6);
	mxc_iomux_set_pad(MX50_PIN_SSI_RXC, 0xC);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_MDI_SELECT_INPUT, 0x1);

	/*FEC_MDC*/
	mxc_request_iomux(MX50_PIN_SSI_RXFS, IOMUX_CONFIG_ALT6);
	mxc_iomux_set_pad(MX50_PIN_SSI_RXFS, 0x004);

	/* FEC RXD1 */
	mxc_request_iomux(MX50_PIN_DISP_D3, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_DISP_D3, 0x0);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_RDATA_1_SELECT_INPUT, 0x0);

	/* FEC RXD0 */
	mxc_request_iomux(MX50_PIN_DISP_D4, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_DISP_D4, 0x0);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_RDATA_0_SELECT_INPUT, 0x0);

	 /* FEC TXD1 */
	mxc_request_iomux(MX50_PIN_DISP_D6, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_DISP_D6, 0x004);

	/* FEC TXD0 */
	mxc_request_iomux(MX50_PIN_DISP_D7, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_DISP_D7, 0x004);

	/* FEC TX_EN */
	mxc_request_iomux(MX50_PIN_DISP_D5, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_DISP_D5, 0x004);

	/* FEC TX_CLK */
	mxc_request_iomux(MX50_PIN_DISP_D0, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_DISP_D0, 0x0);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_TX_CLK_SELECT_INPUT, 0x0);

	/* FEC RX_ER */
	mxc_request_iomux(MX50_PIN_DISP_D1, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_DISP_D1, 0x0);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_RX_ER_SELECT_INPUT, 0);

	/* FEC CRS */
	mxc_request_iomux(MX50_PIN_DISP_D2, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_DISP_D2, 0x0);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_RX_DV_SELECT_INPUT, 0);

	/* phy reset: gpio4-6 */
	mxc_request_iomux(MX50_PIN_KEY_COL3, IOMUX_CONFIG_ALT1);

	reg = readl(GPIO4_BASE_ADDR + 0x0);
	reg &= ~0x40;
	writel(reg, GPIO4_BASE_ADDR + 0x0);

	reg = readl(GPIO4_BASE_ADDR + 0x4);
	reg |= 0x40;
	writel(reg, GPIO4_BASE_ADDR + 0x4);

	udelay(500);

	reg = readl(GPIO4_BASE_ADDR + 0x0);
	reg |= 0x40;
	writel(reg, GPIO4_BASE_ADDR + 0x0);
}
#endif

#ifdef CONFIG_CMD_MMC

struct fsl_esdhc_cfg esdhc_cfg[3] = {
	{MMC_SDHC1_BASE_ADDR, 1, 1},
	{MMC_SDHC2_BASE_ADDR, 1, 1},
	{MMC_SDHC3_BASE_ADDR, 1, 1},
};


#ifdef CONFIG_DYNAMIC_MMC_DEVNO
int get_mmc_env_devno()
{
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
	int mmc_devno = 0;

	switch (soc_sbmr & 0x00300000) {
	default:
	case 0x0:
		mmc_devno = 0;
		break;
	case 0x00100000:
		mmc_devno = 1;
		break;
	case 0x00200000:
		mmc_devno = 2;
		break;
	}

	return mmc_devno;
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
			mxc_request_iomux(MX50_PIN_SD1_CMD, IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD1_CLK, IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD1_D0,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD1_D1,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD1_D2,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD1_D3,  IOMUX_CONFIG_ALT0);

			mxc_iomux_set_pad(MX50_PIN_SD1_CMD, 0x1E4);
			mxc_iomux_set_pad(MX50_PIN_SD1_CLK, 0xD4);
			mxc_iomux_set_pad(MX50_PIN_SD1_D0,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD1_D1,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD1_D2,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD1_D3,  0x1D4);

			break;
		case 1:
			mxc_request_iomux(MX50_PIN_SD2_CMD, IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD2_CLK, IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD2_D0,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD2_D1,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD2_D2,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD2_D3,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD2_D4,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD2_D5,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD2_D6,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD2_D7,  IOMUX_CONFIG_ALT0);

			mxc_iomux_set_pad(MX50_PIN_SD2_CMD, 0x14);
			mxc_iomux_set_pad(MX50_PIN_SD2_CLK, 0xD4);
			mxc_iomux_set_pad(MX50_PIN_SD2_D0,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD2_D1,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD2_D2,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD2_D3,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD2_D4,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD2_D5,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD2_D6,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD2_D7,  0x1D4);

			break;
		case 2:
			mxc_request_iomux(MX50_PIN_SD3_CMD, IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD3_CLK, IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD3_D0,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD3_D1,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD3_D2,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD3_D3,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD3_D4,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD3_D5,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD3_D6,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD3_D7,  IOMUX_CONFIG_ALT0);

			mxc_iomux_set_pad(MX50_PIN_SD3_CMD, 0x1E4);
			mxc_iomux_set_pad(MX50_PIN_SD3_CLK, 0xD4);
			mxc_iomux_set_pad(MX50_PIN_SD3_D0,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD3_D1,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD3_D2,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD3_D3,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD3_D4,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD3_D5,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD3_D6,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD3_D7,  0x1D4);

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

#ifdef CONFIG_IMX_CSPI
static void setup_power(void)
{
	struct spi_slave *slave;
	unsigned int val;
	unsigned int reg;

	puts("PMIC Mode: SPI\n");

	/* Enable VGEN1 to enable ethernet */
	slave = spi_pmic_probe();

	val = pmic_reg(slave, 30, 0, 0);
	val |= 0x3;
	pmic_reg(slave, 30, val, 1);

	val = pmic_reg(slave, 32, 0, 0);
	val |= 0x1;
	pmic_reg(slave, 32, val, 1);

	/* Enable VCAM   */
	val = pmic_reg(slave, 33, 0, 0);
	val |= 0x40;
	pmic_reg(slave, 33, val, 1);

	spi_pmic_free(slave);
}

void setup_voltage_cpu(void)
{
	/* Currently VDDGP 1.05v
	 * no one tell me we need increase the core
	 * voltage to let CPU run at 800Mhz, not do it
	 */

	/* Raise the core frequency to 800MHz */
	writel(0x0, CCM_BASE_ADDR + CLKCTL_CACRR);

}
#endif

int board_init(void)
{
	/* boot device */
	setup_boot_device();

	/* soc rev */
	setup_soc_rev();

	/* arch id for linux */
	gd->bd->bi_arch_number = MACH_TYPE_MX50_ARM2;

	/* boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	/* iomux for uart */
	setup_uart();

#ifdef CONFIG_MXC_FEC
	/* iomux for fec */
	setup_fec();
#endif

	return 0;
}

int board_late_init(void)
{
#ifdef CONFIG_IMX_CSPI
	setup_power();
#endif
	return 0;
}

int checkboard(void)
{
	printf("Board: MX50 ARM2 board\n");

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
