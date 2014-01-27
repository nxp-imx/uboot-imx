/*
 * Copyright (C) 2010-2014 Freescale Semiconductor, Inc.
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
#include <asm/arch/mx6dl_pins.h>
#include <asm/arch/iomux-v3.h>
#include <asm/errno.h>
#include <miiphy.h>
#if defined(CONFIG_VIDEO_MX5)
#include <linux/list.h>
#include <linux/fb.h>
#include <linux/mxcfb.h>
#include <ipu.h>
#include <lcd.h>
#endif

#ifdef CONFIG_IMX_ECSPI
#include <imx_spi.h>
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

#ifdef CONFIG_CMD_IMXOTP
#include <imx_otp.h>
#endif

#ifdef CONFIG_MXC_GPIO
#include <asm/gpio.h>
#include <asm/arch/gpio.h>
#endif

#ifdef CONFIG_DWC_AHSATA
#include <ahci.h>
#endif

#define I2C_EXP_RST IMX_GPIO_NR(1, 15)
#define I2C3_STEER  IMX_GPIO_NR(5, 4)

DECLARE_GLOBAL_DATA_PTR;

static enum boot_device boot_dev;
extern int sata_curr_device;

#ifdef CONFIG_VIDEO_MX5
extern unsigned char fsl_bmp_600x400[];
extern int fsl_bmp_600x400_size;
extern int g_ipu_hw_rev;

#if defined(CONFIG_BMP_8BPP)
unsigned short colormap[256];
#elif defined(CONFIG_BMP_16BPP)
unsigned short colormap[65536];
#else
unsigned short colormap[16777216];
#endif

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
			boot_dev = SATA_BOOT;
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
	return fsl_system_rev;
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
	X_ARM_MMU_SECTION(0x000, 0x000, 0x001,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* ROM, 1M */
	X_ARM_MMU_SECTION(0x001, 0x001, 0x008,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* 8M */
	X_ARM_MMU_SECTION(0x009, 0x009, 0x001,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* IRAM */
	X_ARM_MMU_SECTION(0x00A, 0x00A, 0x0F6,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* 246M */
	/* 2 GB memory starting at 0x10000000, only map 1.875 GB */
	X_ARM_MMU_SECTION(0x100, 0x100, 0x780,
			ARM_CACHEABLE, ARM_BUFFERABLE,
			ARM_ACCESS_PERM_RW_RW);
	/* uncached alias of the same 1.875 GB memory */
	X_ARM_MMU_SECTION(0x100, 0x880, 0x780,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW);

	/* Enable MMU */
	MMU_ON();
}
#endif


#define ANATOP_PLL_LOCK                 0x80000000
#define ANATOP_PLL_ENABLE_MASK          0x00002000
#define ANATOP_PLL_BYPASS_MASK          0x00010000
#define ANATOP_PLL_PWDN_MASK            0x00001000
#define ANATOP_PLL_HOLD_RING_OFF_MASK   0x00000800
#define ANATOP_SATA_CLK_ENABLE_MASK     0x00100000

#ifdef CONFIG_DWC_AHSATA
/* Staggered Spin-up */
#define	HOST_CAP_SSS			(1 << 27)
/* host version register*/
#define	HOST_VERSIONR			0xfc
#define PORT_SATA_SR			0x128

int sata_initialize(void)
{
	u32 reg = 0;
	u32 iterations = 1000000;

	if (sata_curr_device == -1) {
		/* Reset HBA */
		writel(HOST_RESET, SATA_ARB_BASE_ADDR + HOST_CTL);

		reg = 0;
		while (readl(SATA_ARB_BASE_ADDR + HOST_VERSIONR) == 0) {
			reg++;
			if (reg > 1000000)
				break;
		}

		reg = readl(SATA_ARB_BASE_ADDR + HOST_CAP);
		if (!(reg & HOST_CAP_SSS)) {
			reg |= HOST_CAP_SSS;
			writel(reg, SATA_ARB_BASE_ADDR + HOST_CAP);
		}

		reg = readl(SATA_ARB_BASE_ADDR + HOST_PORTS_IMPL);
		if (!(reg & 0x1))
			writel((reg | 0x1),
					SATA_ARB_BASE_ADDR + HOST_PORTS_IMPL);

		/* Release resources when there is no device on the port */
		do {
			reg = readl(SATA_ARB_BASE_ADDR + PORT_SATA_SR) & 0xF;
			if ((reg & 0xF) == 0)
				iterations--;
			else
				break;

		} while (iterations > 0);
	}

	return __sata_initialize();
}
#endif

static int setup_sata(void)
{
	u32 reg = 0;
	s32 timeout = 100000;

	/* Enable sata clock */
	reg = readl(CCM_BASE_ADDR + 0x7c); /* CCGR5 */
	reg |= 0x30;
	writel(reg, CCM_BASE_ADDR + 0x7c);

	/* Enable PLLs */
	reg = readl(ANATOP_BASE_ADDR + 0xe0); /* ENET PLL */
	reg &= ~ANATOP_PLL_PWDN_MASK;
	writel(reg, ANATOP_BASE_ADDR + 0xe0);
	reg |= ANATOP_PLL_ENABLE_MASK;
	while (timeout--) {
		if (readl(ANATOP_BASE_ADDR + 0xe0) & ANATOP_PLL_LOCK)
			break;
	}
	if (timeout <= 0)
		return -1;
	reg &= ~ANATOP_PLL_BYPASS_MASK;
	writel(reg, ANATOP_BASE_ADDR + 0xe0);
	reg |= ANATOP_SATA_CLK_ENABLE_MASK;
	writel(reg, ANATOP_BASE_ADDR + 0xe0);

	/* Enable sata phy */
	reg = readl(IOMUXC_BASE_ADDR + 0x34); /* GPR13 */

	reg &= ~0x07ffffff;
	/*
	 * rx_eq_val_0 = 5 [26:24]
	 * los_lvl = 0x12 [23:19]
	 * rx_dpll_mode_0 = 0x3 [18:16]
	 * mpll_ss_en = 0x0 [14]
	 * tx_atten_0 = 0x4 [13:11]
	 * tx_boost_0 = 0x0 [10:7]
	 * tx_lvl = 0x11 [6:2]
	 * mpll_ck_off_b = 0x1 [1]
	 * tx_edgerate_0 = 0x0 [0]
	 * */
	reg |= 0x59124c6;
	writel(reg, IOMUXC_BASE_ADDR + 0x34);

	return 0;
}

int dram_init(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
	return 0;
}

static void setup_uart(void)
{
#if defined CONFIG_MX6Q
	/* UART4 TXD */
	mxc_iomux_v3_setup_pad(MX6Q_PAD_KEY_COL0__UART4_TXD);

	/* UART4 RXD */
	mxc_iomux_v3_setup_pad(MX6Q_PAD_KEY_ROW0__UART4_RXD);
#elif defined CONFIG_MX6DL
	/* UART4 TXD */
	mxc_iomux_v3_setup_pad(MX6DL_PAD_KEY_COL0__UART4_TXD);

	/* UART4 RXD */
	mxc_iomux_v3_setup_pad(MX6DL_PAD_KEY_ROW0__UART4_RXD);
#endif
}

#ifdef CONFIG_I2C_MXC
static void setup_i2c(unsigned int module_base)
{
	unsigned int reg;

	switch (module_base) {
	case I2C1_BASE_ADDR:
#if defined CONFIG_MX6Q
		/* i2c1 SDA */
		mxc_iomux_v3_setup_pad(MX6Q_PAD_CSI0_DAT8__I2C1_SDA);

		/* i2c1 SCL */
		mxc_iomux_v3_setup_pad(MX6Q_PAD_CSI0_DAT9__I2C1_SCL);
#elif defined CONFIG_MX6DL
		/* i2c1 SDA */
		mxc_iomux_v3_setup_pad(MX6DL_PAD_CSI0_DAT8__I2C1_SDA);
		/* i2c1 SCL */
		mxc_iomux_v3_setup_pad(MX6DL_PAD_CSI0_DAT9__I2C1_SCL);
#endif

		/* Enable i2c clock */
		reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR2);
		reg |= 0xC0;
		writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR2);

		break;
	case I2C2_BASE_ADDR:
#if defined CONFIG_MX6Q
		/* i2c2 SDA */
		mxc_iomux_v3_setup_pad(MX6Q_PAD_KEY_ROW3__I2C2_SDA);

		/* i2c2 SCL */
		mxc_iomux_v3_setup_pad(MX6Q_PAD_KEY_COL3__I2C2_SCL);
#elif defined CONFIG_MX6DL
		/* i2c2 SDA */
		mxc_iomux_v3_setup_pad(MX6DL_PAD_KEY_ROW3__I2C2_SDA);

		/* i2c2 SCL */
		mxc_iomux_v3_setup_pad(MX6DL_PAD_KEY_COL3__I2C2_SCL);
#endif

		/* Enable i2c clock */
		reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR2);
		reg |= 0x300;
		writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR2);

		break;
	case I2C3_BASE_ADDR:
#if defined CONFIG_MX6Q
		/* GPIO_3 for I2C3_SCL */
		mxc_iomux_v3_setup_pad(MX6Q_PAD_GPIO_3__I2C3_SCL);

		if (mx6_board_is_reva()) /* GPIO_16 for I2C3_SDA */
			mxc_iomux_v3_setup_pad(MX6Q_PAD_GPIO_16__I2C3_SDA);
		else
			mxc_iomux_v3_setup_pad(MX6Q_PAD_EIM_D18__I2C3_SDA);
#elif defined CONFIG_MX6DL
		/* GPIO_3 for I2C3_SCL */
		mxc_iomux_v3_setup_pad(MX6DL_PAD_GPIO_3__I2C3_SCL);

		if (mx6_board_is_reva()) /* GPIO_16 for I2C3_SDA */
			mxc_iomux_v3_setup_pad(MX6DL_PAD_GPIO_16__I2C3_SDA);
		else
			mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_D18__I2C3_SDA);
#endif
		/* Enable i2c clock */
		reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR2);
		reg |= 0xC00;
		writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR2);

		break;
	default:
		printf("Invalid I2C base: 0x%x\n", module_base);
		break;
	}
}

void setup_lvds_poweron(void)
{
	uchar value;
	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);

#if defined CONFIG_MX6Q
	i2c_read(0x30, 3, 1, &value, 1);
	value &= ~0x2;
	i2c_write(0x30, 3, 1, &value, 1);

	i2c_read(0x30, 1, 1, &value, 1);
	value |= 0x2;
	i2c_write(0x30, 1, 1, &value, 1);
#endif
}
#endif

s32 spi_get_cfg(struct imx_spi_dev_t *dev)
{
#ifdef CONFIG_IMX_ECSPI
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
	case 1:
		/* SPI-NOR */
		dev->base = ECSPI1_BASE_ADDR;
		dev->freq = 25000000;
		dev->ss_pol = IMX_SPI_ACTIVE_LOW;
		dev->ss = 1;
		dev->fifo_sz = 64 * 4;
		dev->us_delay = 0;
		break;
	default:
		printf("Invalid Bus ID!\n");
	}
#endif
	return 0;
}

void spi_io_init(struct imx_spi_dev_t *dev)
{
#ifdef CONFIG_IMX_ECSPI
	u32 reg;

	switch (dev->base) {
	case ECSPI1_BASE_ADDR:
		/* Enable clock */
		reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR1);
		reg |= 0x3;
		writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR1);

#if defined CONFIG_MX6Q
		/* SCLK */
		mxc_iomux_v3_setup_pad(MX6Q_PAD_EIM_D16__ECSPI1_SCLK);

		/* MISO */
		mxc_iomux_v3_setup_pad(MX6Q_PAD_EIM_D17__ECSPI1_MISO);

		/* MOSI */
		mxc_iomux_v3_setup_pad(MX6Q_PAD_EIM_D18__ECSPI1_MOSI);

		if (dev->ss == 0)
			mxc_iomux_v3_setup_pad(MX6Q_PAD_EIM_EB2__ECSPI1_SS0);
		else if (dev->ss == 1)
			mxc_iomux_v3_setup_pad(MX6Q_PAD_EIM_D19__ECSPI1_SS1);
#elif defined CONFIG_MX6DL
		/* SCLK */
		mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_D16__ECSPI1_SCLK);

		/* MISO */
		mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_D17__ECSPI1_MISO);

		/* MOSI */
		mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_D18__ECSPI1_MOSI);

		if (dev->ss == 0)
			mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_EB2__ECSPI1_SS0);
		else if (dev->ss == 1)
			mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_D19__ECSPI1_SS1);
#endif
		break;
	case ECSPI2_BASE_ADDR:
	case ECSPI3_BASE_ADDR:
		/* ecspi2-3 fall through */
		break;
	default:
		break;
	}
#endif
}

#ifdef CONFIG_NAND_GPMI
#if defined CONFIG_MX6Q
iomux_v3_cfg_t nfc_pads[] = {
	MX6Q_PAD_NANDF_CLE__RAWNAND_CLE,
	MX6Q_PAD_NANDF_ALE__RAWNAND_ALE,
	MX6Q_PAD_NANDF_WP_B__RAWNAND_RESETN,
	MX6Q_PAD_NANDF_RB0__RAWNAND_READY0,
	MX6Q_PAD_NANDF_CS0__RAWNAND_CE0N,
	MX6Q_PAD_NANDF_CS1__RAWNAND_CE1N,
	MX6Q_PAD_NANDF_CS2__RAWNAND_CE2N,
	MX6Q_PAD_NANDF_CS3__RAWNAND_CE3N,
	MX6Q_PAD_SD4_CMD__RAWNAND_RDN,
	MX6Q_PAD_SD4_CLK__RAWNAND_WRN,
	MX6Q_PAD_NANDF_D0__RAWNAND_D0,
	MX6Q_PAD_NANDF_D1__RAWNAND_D1,
	MX6Q_PAD_NANDF_D2__RAWNAND_D2,
	MX6Q_PAD_NANDF_D3__RAWNAND_D3,
	MX6Q_PAD_NANDF_D4__RAWNAND_D4,
	MX6Q_PAD_NANDF_D5__RAWNAND_D5,
	MX6Q_PAD_NANDF_D6__RAWNAND_D6,
	MX6Q_PAD_NANDF_D7__RAWNAND_D7,
	MX6Q_PAD_SD4_DAT0__RAWNAND_DQS,
};
#elif defined CONFIG_MX6DL
iomux_v3_cfg_t nfc_pads[] = {
	MX6DL_PAD_NANDF_CLE__RAWNAND_CLE,
	MX6DL_PAD_NANDF_ALE__RAWNAND_ALE,
	MX6DL_PAD_NANDF_WP_B__RAWNAND_RESETN,
	MX6DL_PAD_NANDF_RB0__RAWNAND_READY0,
	MX6DL_PAD_NANDF_CS0__RAWNAND_CE0N,
	MX6DL_PAD_NANDF_CS1__RAWNAND_CE1N,
	MX6DL_PAD_NANDF_CS2__RAWNAND_CE2N,
	MX6DL_PAD_NANDF_CS3__RAWNAND_CE3N,
	MX6DL_PAD_SD4_CMD__RAWNAND_RDN,
	MX6DL_PAD_SD4_CLK__RAWNAND_WRN,
	MX6DL_PAD_NANDF_D0__RAWNAND_D0,
	MX6DL_PAD_NANDF_D1__RAWNAND_D1,
	MX6DL_PAD_NANDF_D2__RAWNAND_D2,
	MX6DL_PAD_NANDF_D3__RAWNAND_D3,
	MX6DL_PAD_NANDF_D4__RAWNAND_D4,
	MX6DL_PAD_NANDF_D5__RAWNAND_D5,
	MX6DL_PAD_NANDF_D6__RAWNAND_D6,
	MX6DL_PAD_NANDF_D7__RAWNAND_D7,
	MX6DL_PAD_SD4_DAT0__RAWNAND_DQS,
};
#endif

int setup_gpmi_nand(void)
{
	unsigned int reg;

	/* config gpmi nand iomux */
	mxc_iomux_v3_setup_multiple_pads(nfc_pads,
			ARRAY_SIZE(nfc_pads));

	/* config gpmi and bch clock to 20Mhz, from pll2 400M pfd*/
	reg = readl(CCM_BASE_ADDR + CLKCTL_CS2CDR);
	reg &= 0xF800FFFF;
	reg |= 0x02630000;
	writel(reg, CCM_BASE_ADDR + CLKCTL_CS2CDR);

	/* enable gpmi and bch clock gating */
	reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR4);
	reg |= 0xFF003000;
	writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR4);

	/* enable apbh clock gating */
	reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR0);
	reg |= 0x0030;
	writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR0);

}
#endif

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
struct fsl_esdhc_cfg usdhc_cfg[4] = {
	{USDHC1_BASE_ADDR, 1, 1, 1, 0},
	{USDHC2_BASE_ADDR, 1, 1, 1, 0},
	{USDHC3_BASE_ADDR, 1, 1, 1, 1},
	{USDHC4_BASE_ADDR, 1, 1, 1, 0},
};

#ifdef CONFIG_CMD_WEIMNOR

#if defined CONFIG_MX6Q
iomux_v3_cfg_t nor_pads[] = {
	MX6Q_PAD_EIM_D16__WEIM_WEIM_D_16,
	MX6Q_PAD_EIM_D17__WEIM_WEIM_D_17,
	MX6Q_PAD_EIM_D18__WEIM_WEIM_D_18,
	MX6Q_PAD_EIM_D19__WEIM_WEIM_D_19,
	MX6Q_PAD_EIM_D20__WEIM_WEIM_D_20,
	MX6Q_PAD_EIM_D21__WEIM_WEIM_D_21,
	MX6Q_PAD_EIM_D22__WEIM_WEIM_D_22,
	MX6Q_PAD_EIM_D23__WEIM_WEIM_D_23,
	MX6Q_PAD_EIM_D24__WEIM_WEIM_D_24,
	MX6Q_PAD_EIM_D25__WEIM_WEIM_D_25,
	MX6Q_PAD_EIM_D26__WEIM_WEIM_D_26,
	MX6Q_PAD_EIM_D27__WEIM_WEIM_D_27,
	MX6Q_PAD_EIM_D28__WEIM_WEIM_D_28,
	MX6Q_PAD_EIM_D29__WEIM_WEIM_D_29,
	MX6Q_PAD_EIM_D30__WEIM_WEIM_D_30,
	MX6Q_PAD_EIM_D31__WEIM_WEIM_D_31,
	MX6Q_PAD_EIM_DA0__WEIM_WEIM_DA_A_0,
	MX6Q_PAD_EIM_DA1__WEIM_WEIM_DA_A_1,
	MX6Q_PAD_EIM_DA2__WEIM_WEIM_DA_A_2,
	MX6Q_PAD_EIM_DA3__WEIM_WEIM_DA_A_3,
	MX6Q_PAD_EIM_DA4__WEIM_WEIM_DA_A_4,
	MX6Q_PAD_EIM_DA5__WEIM_WEIM_DA_A_5,
	MX6Q_PAD_EIM_DA6__WEIM_WEIM_DA_A_6,
	MX6Q_PAD_EIM_DA7__WEIM_WEIM_DA_A_7,
	MX6Q_PAD_EIM_DA8__WEIM_WEIM_DA_A_8,
	MX6Q_PAD_EIM_DA9__WEIM_WEIM_DA_A_9,
	MX6Q_PAD_EIM_DA10__WEIM_WEIM_DA_A_10,
	MX6Q_PAD_EIM_DA11__WEIM_WEIM_DA_A_11,
	MX6Q_PAD_EIM_DA12__WEIM_WEIM_DA_A_12,
	MX6Q_PAD_EIM_DA13__WEIM_WEIM_DA_A_13,
	MX6Q_PAD_EIM_DA14__WEIM_WEIM_DA_A_14,
	MX6Q_PAD_EIM_DA15__WEIM_WEIM_DA_A_15,
	MX6Q_PAD_EIM_A16__WEIM_WEIM_A_16,
	MX6Q_PAD_EIM_A17__WEIM_WEIM_A_17,
	MX6Q_PAD_EIM_A18__WEIM_WEIM_A_18,
	MX6Q_PAD_EIM_A19__WEIM_WEIM_A_19,
	MX6Q_PAD_EIM_A20__WEIM_WEIM_A_20,
	MX6Q_PAD_EIM_A21__WEIM_WEIM_A_21,
	MX6Q_PAD_EIM_A22__WEIM_WEIM_A_22,
	MX6Q_PAD_EIM_A23__WEIM_WEIM_A_23,
	MX6Q_PAD_EIM_OE__WEIM_WEIM_OE,
	MX6Q_PAD_EIM_RW__WEIM_WEIM_RW,
	MX6Q_PAD_EIM_CS0__WEIM_WEIM_CS_0
};
#elif defined CONFIG_MX6DL
iomux_v3_cfg_t nor_pads[] = {
	MX6DL_PAD_EIM_D16__WEIM_WEIM_D_16,
	MX6DL_PAD_EIM_D17__WEIM_WEIM_D_17,
	MX6DL_PAD_EIM_D18__WEIM_WEIM_D_18,
	MX6DL_PAD_EIM_D19__WEIM_WEIM_D_19,
	MX6DL_PAD_EIM_D20__WEIM_WEIM_D_20,
	MX6DL_PAD_EIM_D21__WEIM_WEIM_D_21,
	MX6DL_PAD_EIM_D22__WEIM_WEIM_D_22,
	MX6DL_PAD_EIM_D23__WEIM_WEIM_D_23,
	MX6DL_PAD_EIM_D24__WEIM_WEIM_D_24,
	MX6DL_PAD_EIM_D25__WEIM_WEIM_D_25,
	MX6DL_PAD_EIM_D26__WEIM_WEIM_D_26,
	MX6DL_PAD_EIM_D27__WEIM_WEIM_D_27,
	MX6DL_PAD_EIM_D28__WEIM_WEIM_D_28,
	MX6DL_PAD_EIM_D29__WEIM_WEIM_D_29,
	MX6DL_PAD_EIM_D30__WEIM_WEIM_D_30,
	MX6DL_PAD_EIM_D31__WEIM_WEIM_D_31,
	MX6DL_PAD_EIM_DA0__WEIM_WEIM_DA_A_0,
	MX6DL_PAD_EIM_DA1__WEIM_WEIM_DA_A_1,
	MX6DL_PAD_EIM_DA2__WEIM_WEIM_DA_A_2,
	MX6DL_PAD_EIM_DA3__WEIM_WEIM_DA_A_3,
	MX6DL_PAD_EIM_DA4__WEIM_WEIM_DA_A_4,
	MX6DL_PAD_EIM_DA5__WEIM_WEIM_DA_A_5,
	MX6DL_PAD_EIM_DA6__WEIM_WEIM_DA_A_6,
	MX6DL_PAD_EIM_DA7__WEIM_WEIM_DA_A_7,
	MX6DL_PAD_EIM_DA8__WEIM_WEIM_DA_A_8,
	MX6DL_PAD_EIM_DA9__WEIM_WEIM_DA_A_9,
	MX6DL_PAD_EIM_DA10__WEIM_WEIM_DA_A_10,
	MX6DL_PAD_EIM_DA11__WEIM_WEIM_DA_A_11,
	MX6DL_PAD_EIM_DA12__WEIM_WEIM_DA_A_12,
	MX6DL_PAD_EIM_DA13__WEIM_WEIM_DA_A_13,
	MX6DL_PAD_EIM_DA14__WEIM_WEIM_DA_A_14,
	MX6DL_PAD_EIM_DA15__WEIM_WEIM_DA_A_15,
	MX6DL_PAD_EIM_A16__WEIM_WEIM_A_16,
	MX6DL_PAD_EIM_A17__WEIM_WEIM_A_17,
	MX6DL_PAD_EIM_A18__WEIM_WEIM_A_18,
	MX6DL_PAD_EIM_A19__WEIM_WEIM_A_19,
	MX6DL_PAD_EIM_A20__WEIM_WEIM_A_20,
	MX6DL_PAD_EIM_A21__WEIM_WEIM_A_21,
	MX6DL_PAD_EIM_A22__WEIM_WEIM_A_22,
	MX6DL_PAD_EIM_A23__WEIM_WEIM_A_23,
	MX6DL_PAD_EIM_OE__WEIM_WEIM_OE,
	MX6DL_PAD_EIM_RW__WEIM_WEIM_RW,
	MX6DL_PAD_EIM_CS0__WEIM_WEIM_CS_0
};
#endif

static void weim_norflash_cs_setup(void)
{
    writel(0x00000120, WEIM_BASE_ADDR + 0x090);
    writel(0x00020181, WEIM_BASE_ADDR + 0x000);
    writel(0x00000001, WEIM_BASE_ADDR + 0x004);
    writel(0x0a020000, WEIM_BASE_ADDR + 0x008);
    writel(0x0000c000, WEIM_BASE_ADDR + 0x00c);
    writel(0x0804a240, WEIM_BASE_ADDR + 0x010);
}

static void setup_nor(void)
{

	mxc_iomux_v3_setup_multiple_pads(nor_pads,
			ARRAY_SIZE(nor_pads));

	weim_norflash_cs_setup();
}
#endif
#if defined CONFIG_MX6Q
iomux_v3_cfg_t usdhc1_pads[] = {
	MX6Q_PAD_SD1_CLK__USDHC1_CLK,
	MX6Q_PAD_SD1_CMD__USDHC1_CMD,
	MX6Q_PAD_SD1_DAT0__USDHC1_DAT0,
	MX6Q_PAD_SD1_DAT1__USDHC1_DAT1,
	MX6Q_PAD_SD1_DAT2__USDHC1_DAT2,
	MX6Q_PAD_SD1_DAT3__USDHC1_DAT3,
};

iomux_v3_cfg_t usdhc2_pads[] = {
	MX6Q_PAD_SD2_CLK__USDHC2_CLK,
	MX6Q_PAD_SD2_CMD__USDHC2_CMD,
	MX6Q_PAD_SD2_DAT0__USDHC2_DAT0,
	MX6Q_PAD_SD2_DAT1__USDHC2_DAT1,
	MX6Q_PAD_SD2_DAT2__USDHC2_DAT2,
	MX6Q_PAD_SD2_DAT3__USDHC2_DAT3,
};

iomux_v3_cfg_t usdhc3_pads[] = {
	MX6Q_PAD_SD3_CLK__USDHC3_CLK,
	MX6Q_PAD_SD3_CMD__USDHC3_CMD,
	MX6Q_PAD_SD3_DAT0__USDHC3_DAT0,
	MX6Q_PAD_SD3_DAT1__USDHC3_DAT1,
	MX6Q_PAD_SD3_DAT2__USDHC3_DAT2,
	MX6Q_PAD_SD3_DAT3__USDHC3_DAT3,
	MX6Q_PAD_SD3_DAT4__USDHC3_DAT4,
	MX6Q_PAD_SD3_DAT5__USDHC3_DAT5,
	MX6Q_PAD_SD3_DAT6__USDHC3_DAT6,
	MX6Q_PAD_SD3_DAT7__USDHC3_DAT7,
	MX6Q_PAD_GPIO_18__USDHC3_VSELECT,
};

iomux_v3_cfg_t usdhc4_pads[] = {
	MX6Q_PAD_SD4_CLK__USDHC4_CLK,
	MX6Q_PAD_SD4_CMD__USDHC4_CMD,
	MX6Q_PAD_SD4_DAT0__USDHC4_DAT0,
	MX6Q_PAD_SD4_DAT1__USDHC4_DAT1,
	MX6Q_PAD_SD4_DAT2__USDHC4_DAT2,
	MX6Q_PAD_SD4_DAT3__USDHC4_DAT3,
	MX6Q_PAD_SD4_DAT4__USDHC4_DAT4,
	MX6Q_PAD_SD4_DAT5__USDHC4_DAT5,
	MX6Q_PAD_SD4_DAT6__USDHC4_DAT6,
	MX6Q_PAD_SD4_DAT7__USDHC4_DAT7,
};
#elif defined CONFIG_MX6DL
iomux_v3_cfg_t usdhc1_pads[] = {
	MX6DL_PAD_SD1_CLK__USDHC1_CLK,
	MX6DL_PAD_SD1_CMD__USDHC1_CMD,
	MX6DL_PAD_SD1_DAT0__USDHC1_DAT0,
	MX6DL_PAD_SD1_DAT1__USDHC1_DAT1,
	MX6DL_PAD_SD1_DAT2__USDHC1_DAT2,
	MX6DL_PAD_SD1_DAT3__USDHC1_DAT3,
};

iomux_v3_cfg_t usdhc2_pads[] = {
	MX6DL_PAD_SD2_CLK__USDHC2_CLK,
	MX6DL_PAD_SD2_CMD__USDHC2_CMD,
	MX6DL_PAD_SD2_DAT0__USDHC2_DAT0,
	MX6DL_PAD_SD2_DAT1__USDHC2_DAT1,
	MX6DL_PAD_SD2_DAT2__USDHC2_DAT2,
	MX6DL_PAD_SD2_DAT3__USDHC2_DAT3,
};

iomux_v3_cfg_t usdhc3_pads[] = {
	MX6DL_PAD_SD3_CLK__USDHC3_CLK,
	MX6DL_PAD_SD3_CMD__USDHC3_CMD,
	MX6DL_PAD_SD3_DAT0__USDHC3_DAT0,
	MX6DL_PAD_SD3_DAT1__USDHC3_DAT1,
	MX6DL_PAD_SD3_DAT2__USDHC3_DAT2,
	MX6DL_PAD_SD3_DAT3__USDHC3_DAT3,
	MX6DL_PAD_SD3_DAT4__USDHC3_DAT4,
	MX6DL_PAD_SD3_DAT5__USDHC3_DAT5,
	MX6DL_PAD_SD3_DAT6__USDHC3_DAT6,
	MX6DL_PAD_SD3_DAT7__USDHC3_DAT7,
	MX6DL_PAD_GPIO_18__USDHC3_VSELECT,
};

iomux_v3_cfg_t usdhc4_pads[] = {
	MX6DL_PAD_SD4_CLK__USDHC4_CLK,
	MX6DL_PAD_SD4_CMD__USDHC4_CMD,
	MX6DL_PAD_SD4_DAT0__USDHC4_DAT0,
	MX6DL_PAD_SD4_DAT1__USDHC4_DAT1,
	MX6DL_PAD_SD4_DAT2__USDHC4_DAT2,
	MX6DL_PAD_SD4_DAT3__USDHC4_DAT3,
	MX6DL_PAD_SD4_DAT4__USDHC4_DAT4,
	MX6DL_PAD_SD4_DAT5__USDHC4_DAT5,
	MX6DL_PAD_SD4_DAT6__USDHC4_DAT6,
	MX6DL_PAD_SD4_DAT7__USDHC4_DAT7,
};
#endif

#define USDHC_PAD_CTRL_DEFAULT (PAD_CTL_PKE | PAD_CTL_PUE |		\
		PAD_CTL_PUS_47K_UP  | PAD_CTL_SPEED_LOW |		\
		PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL_100MHZ (PAD_CTL_PKE | PAD_CTL_PUE |	\
		PAD_CTL_PUS_47K_UP  | PAD_CTL_SPEED_MED |		\
		PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL_200MHZ (PAD_CTL_PKE | PAD_CTL_PUE |	\
		PAD_CTL_PUS_47K_UP  | PAD_CTL_SPEED_HIGH |		\
		PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST   | PAD_CTL_HYS)

int usdhc_gpio_init(bd_t *bis)
{
	s32 status = 0;
	u32 index = 0;

	for (index = 0; index < CONFIG_SYS_FSL_USDHC_NUM;
		++index) {
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
		case 3:
			mxc_iomux_v3_setup_multiple_pads(usdhc4_pads,
						ARRAY_SIZE(usdhc4_pads));
			break;
		default:
			printf("Warning: you configured more USDHC controllers"
				"(%d) then supported by the board (%d)\n",
				index+1, CONFIG_SYS_FSL_USDHC_NUM);
			return status;
		}
		status |= fsl_esdhc_initialize(bis, &usdhc_cfg[index]);
	}

	return status;
}

static void usdhc_switch_pad(iomux_v3_cfg_t *pad_list, unsigned count,
	iomux_v3_cfg_t *new_pad_list, iomux_v3_cfg_t pad_val)
{
	u32 i;

	for (i = 0; i < count; i++) {
		new_pad_list[i] = pad_list[i] & (~MUX_PAD_CTRL_MASK);
		new_pad_list[i] |= MUX_PAD_CTRL(pad_val);
	}
}

int board_mmc_io_switch(u32 index, u32 clock)
{
	iomux_v3_cfg_t new_pads[14];
	u32 count;
	iomux_v3_cfg_t pad_ctrl = USDHC_PAD_CTRL_DEFAULT;

	if (clock >= 200000000)
		pad_ctrl = USDHC_PAD_CTRL_200MHZ;
	else if (clock == 100000000)
		pad_ctrl = USDHC_PAD_CTRL_100MHZ;

	switch (index) {
	case 0:
		count = sizeof(usdhc1_pads) / sizeof(usdhc1_pads[0]);
		usdhc_switch_pad(usdhc1_pads, count, new_pads, pad_ctrl);
		break;
	case 1:
		count = sizeof(usdhc2_pads) / sizeof(usdhc2_pads[0]);
		usdhc_switch_pad(usdhc2_pads, count, new_pads, pad_ctrl);
		break;
	case 2:
		count = sizeof(usdhc3_pads) / sizeof(usdhc3_pads[0]);
		usdhc_switch_pad(usdhc3_pads, count, new_pads, pad_ctrl);
		break;
	case 3:
		count = sizeof(usdhc4_pads) / sizeof(usdhc4_pads[0]);
		usdhc_switch_pad(usdhc4_pads, count, new_pads, pad_ctrl);
		break;
	default:
		printf("Warning: you configured more USDHC controllers"
			"(%d) then supported by the board (%d)\n",
			index+1, CONFIG_SYS_FSL_USDHC_NUM);
		return -1;
	}

	mxc_iomux_v3_setup_multiple_pads(new_pads, count);

	return 0;
}

int board_mmc_init(bd_t *bis)
{
	if (!usdhc_gpio_init(bis))
		return 0;
	else
		return -1;
}

/* For DDR mode operation, provide target delay parameter for each SD port.
 * Use cfg->esdhc_base to distinguish the SD port #. The delay for each port
 * is dependent on signal layout for that particular port.  If the following
 * CONFIG is not defined, then the default target delay value will be used.
 */
#ifdef CONFIG_GET_DDR_TARGET_DELAY
u32 get_ddr_delay(struct fsl_esdhc_cfg *cfg)
{
	/* No delay required on SABRE Auto board SD ports */
	return 0;
}
#endif

#endif

#ifdef CONFIG_LCD
void lcd_enable(void)
{
	char *s;
	int ret;
	unsigned int reg;

	s = getenv("lvds_num");
	di = simple_strtol(s, NULL, 10);

	/*
	* hw_rev 2: IPUV3DEX
	* hw_rev 3: IPUV3M
	* hw_rev 4: IPUV3H
	*/
	g_ipu_hw_rev = IPUV3_HW_REV_IPUV3H;

	/* Enable IPU clock */
	if (di == 1) {
		reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR3);
		reg |= 0xC033;
		writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR3);
	} else {
		reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR3);
		reg |= 0x300F;
		writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR3);
	}

	ret = ipuv3_fb_init(&lvds_xga, di, IPU_PIX_FMT_RGB666,
			DI_PCLK_LDB, 65000000);
	if (ret)
		puts("LCD cannot be configured\n");

	reg = readl(ANATOP_BASE_ADDR + 0xF0);
	reg &= ~0x00003F00;
	reg |= 0x00001300;
	writel(reg, ANATOP_BASE_ADDR + 0xF4);

	reg = readl(CCM_BASE_ADDR + CLKCTL_CS2CDR);
	reg &= ~0x00007E00;
	reg |= 0x00003600;
	writel(reg, CCM_BASE_ADDR + CLKCTL_CS2CDR);

	reg = readl(CCM_BASE_ADDR + CLKCTL_CSCMR2);
	reg |= 0x00000C00;
	writel(reg, CCM_BASE_ADDR + CLKCTL_CSCMR2);

	reg = 0x0002A953;
	writel(reg, CCM_BASE_ADDR + CLKCTL_CHSCCDR);

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

#if defined CONFIG_MX6Q
iomux_v3_cfg_t gpio_pads[] = {
	MX6Q_PAD_EIM_A24__GPIO_5_4,
	MX6Q_PAD_SD2_DAT0__GPIO_1_15,
};
#elif defined CONFIG_MX6DL
iomux_v3_cfg_t gpio_pads[] = {
	MX6DL_PAD_EIM_A24__GPIO_5_4,
	MX6DL_PAD_SD2_DAT0__GPIO_1_15,
};
#endif

int board_init(void)
{
	mxc_iomux_v3_init((void *)IOMUXC_BASE_ADDR);
	setup_boot_device();
	fsl_set_system_rev();

	/* board id for linux */
	gd->bd->bi_arch_number = MACH_TYPE_MX6Q_SABREAUTO;

	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	setup_uart();

#ifdef CONFIG_I2C_MXC
	if (!mx6_board_is_reva()) {
		mxc_iomux_v3_setup_multiple_pads(gpio_pads,
			ARRAY_SIZE(gpio_pads));
		gpio_direction_output(I2C_EXP_RST, 1);
		gpio_direction_output(I2C3_STEER, 1);
	}
	setup_i2c(CONFIG_SYS_I2C_PORT);
	/* Enable lvds power */
	setup_lvds_poweron();
	/* restore path for weim/spi nor */
	if (!mx6_board_is_reva())
		gpio_direction_output(I2C3_STEER, 0);
#endif
#ifdef CONFIG_CMD_WEIMNOR
	setup_nor();
#endif
	if (cpu_is_mx6q())
		setup_sata();

#ifdef CONFIG_VIDEO_MX5
	panel_info_init();

	gd->fb_base = CONFIG_FB_BASE;
#ifdef CONFIG_ARCH_MMU
	gd->fb_base = ioremap_nocache(iomem_to_phys(gd->fb_base), 0);
#endif
#endif

#ifdef CONFIG_NAND_GPMI
	setup_gpmi_nand();
#endif
	return 0;
}

int board_late_init(void)
{
	return 0;
}

/*
 * The following function is used for debug purpose
 */
static int phy_read(char *devname, unsigned char addr, unsigned char reg,
		    unsigned short *pdata)
{
	int ret = miiphy_read(devname, addr, reg, pdata);
	if (ret)
		printf("Error reading from %s PHY addr=%02x reg=%02x\n",
		       devname, addr, reg);
	return ret;
}

static int phy_write(char *devname, unsigned char addr, unsigned char reg,
		     unsigned short value)
{
	int ret = miiphy_write(devname, addr, reg, value);
	if (ret)
		printf("Error writing to %s PHY addr=%02x reg=%02x\n", devname,
		       addr, reg);
	return ret;
}

int mx6_rgmii_rework(char *devname, int phy_addr)
{
	unsigned short val;

	if (mx6_board_is_reva()) {
		/* Phy: KSZ9021 */
		phy_write(devname, phy_addr, 0x9, 0x1c00);

		/* min rx data delay */
		phy_write(devname, phy_addr, 0x0b, 0x8105);
		phy_write(devname, phy_addr, 0x0c, 0x0000);

		/* max rx/tx clock delay, min rx/tx control delay */
		phy_write(devname, phy_addr, 0x0b, 0x8104);
		phy_write(devname, phy_addr, 0x0c, 0xf0f0);
		phy_write(devname, phy_addr, 0x0b, 0x104);
	} else {
		/* To enable AR8031 ouput a 125MHz clk from CLK_25M */
		phy_write(devname, phy_addr, 0xd, 0x7);
		phy_write(devname, phy_addr, 0xe, 0x8016);
		phy_write(devname, phy_addr, 0xd, 0x4007);
		phy_read(devname, phy_addr, 0xe, &val);

		val &= 0xffe3;
		val |= 0x18;
		phy_write(devname, phy_addr, 0xe, val);

		/* introduce tx clock delay */
		phy_write(devname, phy_addr, 0x1d, 0x5);
		phy_read(devname, phy_addr, 0x1e, &val);
		val |= 0x0100;
		phy_write(devname, phy_addr, 0x1e, val);
	}

	return 0;
}

#if defined CONFIG_MX6Q
iomux_v3_cfg_t enet_pads[] = {
	MX6Q_PAD_KEY_COL1__ENET_MDIO,
	MX6Q_PAD_KEY_COL2__ENET_MDC,
	MX6Q_PAD_RGMII_TXC__ENET_RGMII_TXC,
	MX6Q_PAD_RGMII_TD0__ENET_RGMII_TD0,
	MX6Q_PAD_RGMII_TD1__ENET_RGMII_TD1,
	MX6Q_PAD_RGMII_TD2__ENET_RGMII_TD2,
	MX6Q_PAD_RGMII_TD3__ENET_RGMII_TD3,
	MX6Q_PAD_RGMII_TX_CTL__ENET_RGMII_TX_CTL,
	MX6Q_PAD_ENET_REF_CLK__ENET_TX_CLK,
	MX6Q_PAD_RGMII_RXC__ENET_RGMII_RXC,
	MX6Q_PAD_RGMII_RD0__ENET_RGMII_RD0,
	MX6Q_PAD_RGMII_RD1__ENET_RGMII_RD1,
	MX6Q_PAD_RGMII_RD2__ENET_RGMII_RD2,
	MX6Q_PAD_RGMII_RD3__ENET_RGMII_RD3,
	MX6Q_PAD_RGMII_RX_CTL__ENET_RGMII_RX_CTL,
};
#elif defined CONFIG_MX6DL
iomux_v3_cfg_t enet_pads[] = {
	MX6DL_PAD_KEY_COL1__ENET_MDIO,
	MX6DL_PAD_KEY_COL2__ENET_MDC,
	MX6DL_PAD_RGMII_TXC__ENET_RGMII_TXC,
	MX6DL_PAD_RGMII_TD0__ENET_RGMII_TD0,
	MX6DL_PAD_RGMII_TD1__ENET_RGMII_TD1,
	MX6DL_PAD_RGMII_TD2__ENET_RGMII_TD2,
	MX6DL_PAD_RGMII_TD3__ENET_RGMII_TD3,
	MX6DL_PAD_RGMII_TX_CTL__ENET_RGMII_TX_CTL,
	MX6DL_PAD_ENET_REF_CLK__ENET_TX_CLK,
	MX6DL_PAD_RGMII_RXC__ENET_RGMII_RXC,
	MX6DL_PAD_RGMII_RD0__ENET_RGMII_RD0,
	MX6DL_PAD_RGMII_RD1__ENET_RGMII_RD1,
	MX6DL_PAD_RGMII_RD2__ENET_RGMII_RD2,
	MX6DL_PAD_RGMII_RD3__ENET_RGMII_RD3,
	MX6DL_PAD_RGMII_RX_CTL__ENET_RGMII_RX_CTL,
};
#endif

void enet_board_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(enet_pads,
			ARRAY_SIZE(enet_pads));
}

int checkboard(void)
{
	printf("Board: %s-SABREAUTO: %s Board: 0x%x [",
	mx6_chip_name(),
	mx6_board_rev_name(),
	fsl_system_rev);

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

#ifdef CONFIG_IMX_UDC
#define SABREAUTO_MAX7310_1_BASE_ADDR	IMX_GPIO_NR(8, 0)
#define SABREAUTO_MAX7310_2_BASE_ADDR	IMX_GPIO_NR(8, 8)
#define SABREAUTO_MAX7310_3_BASE_ADDR	IMX_GPIO_NR(8, 16)

#define SABREAUTO_IO_EXP_GPIO1(x)	(SABREAUTO_MAX7310_1_BASE_ADDR + (x))
#define SABREAUTO_IO_EXP_GPIO2(x)	(SABREAUTO_MAX7310_2_BASE_ADDR + (x))
#define SABREAUTO_IO_EXP_GPIO3(x)	(SABREAUTO_MAX7310_3_BASE_ADDR + (x))

#define SABREAUTO_USB_HOST1_PWR		SABREAUTO_IO_EXP_GPIO2(7)
#define SABREAUTO_USB_OTG_PWR		SABREAUTO_IO_EXP_GPIO3(1)

void udc_pins_setting(void)
{
	mxc_iomux_v3_setup_pad(MX6X_IOMUX(PAD_ENET_RX_ER__ANATOP_USBOTG_ID));

	/* USB_OTG_PWR = 0 */
	gpio_direction_output(SABREAUTO_USB_OTG_PWR, 0);
	/* USB_H1_POWER = 1 */
	gpio_direction_output(SABREAUTO_USB_HOST1_PWR, 1);

	mxc_iomux_set_gpr_register(1, 13, 1, 0);

}
#endif

#ifdef CONFIG_ANDROID_RECOVERY

#define GPIO_VOL_DN_KEY IMX_GPIO_NR(5, 14)

int check_recovery_cmd_file(void)
{
	int button_pressed = 0;
	int recovery_mode = 0;

	recovery_mode = check_and_clean_recovery_flag();

	/* Check Recovery Combo Button press or not. */
	mxc_iomux_v3_setup_pad(MX6X_IOMUX(PAD_DISP0_DAT20__GPIO_5_14));

	gpio_direction_input(GPIO_VOL_DN_KEY);

	if (gpio_get_value(GPIO_VOL_DN_KEY) == 0) { /* VOL_DN key is low assert */
		button_pressed = 1;
		printf("Recovery key pressed\n");
	}

	return recovery_mode || button_pressed;
}
#endif
