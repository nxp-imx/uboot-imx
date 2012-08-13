/*
 * Copyright (C) 2012 Freescale Semiconductor, Inc.
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
#include <asm/arch/regs-anadig.h>
#include <asm/errno.h>
#ifdef CONFIG_MXC_FEC
#include <miiphy.h>
#endif

#if defined(CONFIG_VIDEO_MX5)
#include <linux/list.h>
#include <linux/fb.h>
#include <linux/mxcfb.h>
#include <ipu.h>
#endif
#if defined(CONFIG_VIDEO_MX5) || defined(CONFIG_MXC_EPDC)
#include <lcd.h>
#endif

#include "../../../drivers/video/mxc_epdc_fb.h"

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

#ifdef CONFIG_ANDROID_RECOVERY
#include <recovery.h>
#endif
DECLARE_GLOBAL_DATA_PTR;

static enum boot_device boot_dev;

#define GPIO_VOL_DN_KEY IMX_GPIO_NR(1, 5)
#define USB_OTG_PWR IMX_GPIO_NR(3, 22)
#define USB_H1_POWER IMX_GPIO_NR(1, 29)

#ifdef CONFIG_VIDEO_MX5
extern unsigned char fsl_bmp_reversed_600x400[];
extern int fsl_bmp_reversed_600x400_size;
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

#ifdef CONFIG_DWC_AHSATA

#define ANATOP_PLL_LOCK                 0x80000000
#define ANATOP_PLL_ENABLE_MASK          0x00002000
#define ANATOP_PLL_BYPASS_MASK          0x00010000
#define ANATOP_PLL_LOCK                 0x80000000
#define ANATOP_PLL_PWDN_MASK            0x00001000
#define ANATOP_PLL_HOLD_RING_OFF_MASK   0x00000800
#define ANATOP_SATA_CLK_ENABLE_MASK     0x00100000

int setup_sata(void)
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
#endif

int dram_init(void)
{
	/*
	 * Switch PL301_FAST2 to DDR Dual-channel mapping
	 * however this block the boot up, temperory redraw
	 */
	/*
	 * u32 reg = 1;
	 * writel(reg, GPV0_BASE_ADDR);
	 */

	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;

	return 0;
}

static void setup_uart(void)
{
#if defined CONFIG_MX6Q
	/* UART1 TXD */
	mxc_iomux_v3_setup_pad(MX6Q_PAD_CSI0_DAT10__UART1_TXD);

	/* UART1 RXD */
	mxc_iomux_v3_setup_pad(MX6Q_PAD_CSI0_DAT11__UART1_RXD);
#elif defined CONFIG_MX6DL
	/* UART1 TXD */
	mxc_iomux_v3_setup_pad(MX6DL_PAD_CSI0_DAT10__UART1_TXD);

	/* UART1 RXD */
	mxc_iomux_v3_setup_pad(MX6DL_PAD_CSI0_DAT11__UART1_RXD);
#endif
}

#ifdef CONFIG_VIDEO_MX5
void setup_lvds_poweron(void)
{
	int reg;
	/* AUX_5V_EN: GPIO(6, 10) */
#ifdef CONFIG_MX6DL
	mxc_iomux_v3_setup_pad(MX6DL_PAD_NANDF_RB0__GPIO_6_10);
#else
	mxc_iomux_v3_setup_pad(MX6Q_PAD_NANDF_RB0__GPIO_6_10);
#endif

	reg = readl(GPIO6_BASE_ADDR + GPIO_GDIR);
	reg |= (1 << 10);
	writel(reg, GPIO6_BASE_ADDR + GPIO_GDIR);

	reg = readl(GPIO6_BASE_ADDR + GPIO_DR);
	reg |= (1 << 10);
	writel(reg, GPIO6_BASE_ADDR + GPIO_DR);
}
#endif

#ifdef CONFIG_I2C_MXC
#define I2C1_SDA_GPIO5_26_BIT_MASK  (1 << 26)
#define I2C1_SCL_GPIO5_27_BIT_MASK  (1 << 27)
#define I2C2_SCL_GPIO4_12_BIT_MASK  (1 << 12)
#define I2C2_SDA_GPIO4_13_BIT_MASK  (1 << 13)
#define I2C3_SCL_GPIO1_3_BIT_MASK   (1 << 3)
#define I2C3_SDA_GPIO1_6_BIT_MASK   (1 << 6)


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
		/* GPIO_6 for I2C3_SDA */
		mxc_iomux_v3_setup_pad(MX6Q_PAD_GPIO_6__I2C3_SDA);

#elif defined CONFIG_MX6DL
		/* GPIO_3 for I2C3_SCL */
		mxc_iomux_v3_setup_pad(MX6DL_PAD_GPIO_3__I2C3_SCL);
		/* GPIO_6 for I2C3_SDA */
		mxc_iomux_v3_setup_pad(MX6DL_PAD_GPIO_6__I2C3_SDA);
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
static void mx6q_i2c_gpio_scl_direction(int bus, int output)
{
	u32 reg;

	switch (bus) {
	case 1:
#if defined CONFIG_MX6Q
		mxc_iomux_v3_setup_pad(MX6Q_PAD_CSI0_DAT9__GPIO_5_27);
#elif defined CONFIG_MX6DL
		mxc_iomux_v3_setup_pad(MX6DL_PAD_CSI0_DAT9__GPIO_5_27);
#endif
		reg = readl(GPIO5_BASE_ADDR + GPIO_GDIR);
		if (output)
			reg |= I2C1_SCL_GPIO5_27_BIT_MASK;
		else
			reg &= ~I2C1_SCL_GPIO5_27_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + GPIO_GDIR);
		break;
	case 2:
#if defined CONFIG_MX6Q
		mxc_iomux_v3_setup_pad(MX6Q_PAD_KEY_COL3__GPIO_4_12);
#elif defined CONFIG_MX6DL
		mxc_iomux_v3_setup_pad(MX6DL_PAD_KEY_COL3__GPIO_4_12);
#endif
		reg = readl(GPIO4_BASE_ADDR + GPIO_GDIR);
		if (output)
			reg |= I2C2_SCL_GPIO4_12_BIT_MASK;
		else
			reg &= ~I2C2_SCL_GPIO4_12_BIT_MASK;
		writel(reg, GPIO4_BASE_ADDR + GPIO_GDIR);
		break;
	case 3:
#if defined CONFIG_MX6Q
		mxc_iomux_v3_setup_pad(MX6Q_PAD_GPIO_3__GPIO_1_3);
#elif defined CONFIG_MX6DL
		mxc_iomux_v3_setup_pad(MX6DL_PAD_GPIO_3__GPIO_1_3);
#endif
		reg = readl(GPIO1_BASE_ADDR + GPIO_GDIR);
		if (output)
			reg |= I2C3_SCL_GPIO1_3_BIT_MASK;
		else
			reg &= I2C3_SCL_GPIO1_3_BIT_MASK;
		writel(reg, GPIO1_BASE_ADDR + GPIO_GDIR);
		break;
	}
}

/* set 1 to output, sent 0 to input */
static void mx6q_i2c_gpio_sda_direction(int bus, int output)
{
	u32 reg;

	switch (bus) {
	case 1:
#if defined CONFIG_MX6Q
		mxc_iomux_v3_setup_pad(MX6Q_PAD_CSI0_DAT8__GPIO_5_26);
#elif defined CONFIG_MX6DL
		mxc_iomux_v3_setup_pad(MX6DL_PAD_CSI0_DAT8__GPIO_5_26);
#endif
		reg = readl(GPIO5_BASE_ADDR + GPIO_GDIR);
		if (output)
			reg |= I2C1_SDA_GPIO5_26_BIT_MASK;
		else
			reg &= ~I2C1_SDA_GPIO5_26_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + GPIO_GDIR);
		break;
	case 2:
#if defined CONFIG_MX6Q
		mxc_iomux_v3_setup_pad(MX6Q_PAD_KEY_ROW3__GPIO_4_13);
#elif defined CONFIG_MX6DL
		mxc_iomux_v3_setup_pad(MX6DL_PAD_KEY_ROW3__GPIO_4_13);
#endif
		reg = readl(GPIO4_BASE_ADDR + GPIO_GDIR);
		if (output)
			reg |= I2C2_SDA_GPIO4_13_BIT_MASK;
		else
			reg &= ~I2C2_SDA_GPIO4_13_BIT_MASK;
		writel(reg, GPIO4_BASE_ADDR + GPIO_GDIR);
	case 3:
#if defined CONFIG_MX6Q
		mxc_iomux_v3_setup_pad(MX6Q_PAD_GPIO_6__GPIO_1_6);
#elif defined CONFIG_MX6DL
		mxc_iomux_v3_setup_pad(MX6DL_PAD_GPIO_6__GPIO_1_6);
#endif
		reg = readl(GPIO1_BASE_ADDR + GPIO_GDIR);
		if (output)
			reg |= I2C3_SDA_GPIO1_6_BIT_MASK;
		else
			reg &= ~I2C3_SDA_GPIO1_6_BIT_MASK;
		writel(reg, GPIO1_BASE_ADDR + GPIO_GDIR);
	default:
		break;
	}
}

/* set 1 to high 0 to low */
static void mx6q_i2c_gpio_scl_set_level(int bus, int high)
{
	u32 reg;

	switch (bus) {
	case 1:
		reg = readl(GPIO5_BASE_ADDR + GPIO_DR);
		if (high)
			reg |= I2C1_SCL_GPIO5_27_BIT_MASK;
		else
			reg &= ~I2C1_SCL_GPIO5_27_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + GPIO_DR);
		break;
	case 2:
		reg = readl(GPIO4_BASE_ADDR + GPIO_DR);
		if (high)
			reg |= I2C2_SCL_GPIO4_12_BIT_MASK;
		else
			reg &= ~I2C2_SCL_GPIO4_12_BIT_MASK;
		writel(reg, GPIO4_BASE_ADDR + GPIO_DR);
		break;
	case 3:
		reg = readl(GPIO1_BASE_ADDR + GPIO_DR);
		if (high)
			reg |= I2C3_SCL_GPIO1_3_BIT_MASK;
		else
			reg &= ~I2C3_SCL_GPIO1_3_BIT_MASK;
		writel(reg, GPIO1_BASE_ADDR + GPIO_DR);
		break;
	}
}

/* set 1 to high 0 to low */
static void mx6q_i2c_gpio_sda_set_level(int bus, int high)
{
	u32 reg;

	switch (bus) {
	case 1:
		reg = readl(GPIO5_BASE_ADDR + GPIO_DR);
		if (high)
			reg |= I2C1_SDA_GPIO5_26_BIT_MASK;
		else
			reg &= ~I2C1_SDA_GPIO5_26_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + GPIO_DR);
		break;
	case 2:
		reg = readl(GPIO4_BASE_ADDR + GPIO_DR);
		if (high)
			reg |= I2C2_SDA_GPIO4_13_BIT_MASK;
		else
			reg &= ~I2C2_SDA_GPIO4_13_BIT_MASK;
		writel(reg, GPIO4_BASE_ADDR + GPIO_DR);
		break;
	case 3:
		reg = readl(GPIO1_BASE_ADDR + GPIO_DR);
		if (high)
			reg |= I2C3_SDA_GPIO1_6_BIT_MASK;
		else
			reg &= ~I2C3_SDA_GPIO1_6_BIT_MASK;
		writel(reg, GPIO1_BASE_ADDR + GPIO_DR);
		break;
	}
}

static int mx6q_i2c_gpio_check_sda(int bus)
{
	u32 reg;
	int result = 0;

	switch (bus) {
	case 1:
		reg = readl(GPIO5_BASE_ADDR + GPIO_PSR);
		result = !!(reg & I2C1_SDA_GPIO5_26_BIT_MASK);
		break;
	case 2:
		reg = readl(GPIO4_BASE_ADDR + GPIO_PSR);
		result = !!(reg & I2C2_SDA_GPIO4_13_BIT_MASK);
		break;
	case 3:
		reg = readl(GPIO1_BASE_ADDR + GPIO_PSR);
		result = !!(reg & I2C3_SDA_GPIO1_6_BIT_MASK);
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

	for (bus = 1; bus <= 3; bus++) {
		mx6q_i2c_gpio_sda_direction(bus, 0);

		if (mx6q_i2c_gpio_check_sda(bus) == 0) {
			printf("i2c: I2C%d SDA is low, start i2c recovery...\n", bus);
			mx6q_i2c_gpio_scl_direction(bus, 1);
			mx6q_i2c_gpio_scl_set_level(bus, 1);
			__udelay(10000);

			for (i = 0; i < 9; i++) {
				mx6q_i2c_gpio_scl_set_level(bus, 1);
				__udelay(5);
				mx6q_i2c_gpio_scl_set_level(bus, 0);
				__udelay(5);
			}

			/* 9th clock here, the slave should already
			   release the SDA, we can set SDA as high to
			   a NAK.*/
			mx6q_i2c_gpio_sda_direction(bus, 1);
			mx6q_i2c_gpio_sda_set_level(bus, 1);
			__udelay(1); /* Pull up SDA first */
			mx6q_i2c_gpio_scl_set_level(bus, 1);
			__udelay(5); /* plus pervious 1 us */
			mx6q_i2c_gpio_scl_set_level(bus, 0);
			__udelay(5);
			mx6q_i2c_gpio_sda_set_level(bus, 0);
			__udelay(5);
			mx6q_i2c_gpio_scl_set_level(bus, 1);
			__udelay(5);
			/* Here: SCL is high, and SDA from low to high, it's a
			 * stop condition */
			mx6q_i2c_gpio_sda_set_level(bus, 1);
			__udelay(5);

			mx6q_i2c_gpio_sda_direction(bus, 0);
			if (mx6q_i2c_gpio_check_sda(bus) == 1)
				printf("I2C%d Recovery success\n", bus);
			else {
				printf("I2C%d Recovery failed, I2C1 SDA still low!!!\n", bus);
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
		case 3:
			setup_i2c(I2C3_BASE_ADDR);
			break;
		}
	}

	return result;
}


void setup_pmic_voltages(void)
{
	unsigned char value = 0 ;
	#if CONFIG_MX6_INTER_LDO_BYPASS
	unsigned int val = 0;
	#endif
	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
	if (!i2c_probe(0x8)) {
		if (i2c_read(0x8, 0, 1, &value, 1))
			printf("%s:i2c_read:error\n", __func__);
		printf("Found PFUZE100! device id=%x\n", value);
		if (value == 0x10) {
			/*workaround ER1 of pfuze1.0: set all buck regulators
			in PWM mode except SW1C*/
			value = 0x6;
			i2c_write(0x8, 0x23, 1, &value, 1);/*SW1AB*/

			value = 0x6;
			i2c_write(0x8, 0x38, 1, &value, 1);/*SW2*/

			value = 0x6;
			i2c_write(0x8, 0x3f, 1, &value, 1);/*SW3A*/

			value = 0x6;
			i2c_write(0x8, 0x46, 1, &value, 1);/*SW3B*/

			value = 0x6;
			i2c_write(0x8, 0x4d, 1, &value, 1);/*SW4*/
		}
		/*VDDCORE/VDDSOC default 1.375V is not enough, considering
		pfuze tolerance and IR drop and ripple, need increase
		to 1.425V for SabreSD*/
		value = 0x2d;
		i2c_write(0x8, 0x20, 1, &value, 1);

		value = 0x2d;
		i2c_write(0x8, 0x2e, 1, &value, 1);

		#if CONFIG_MX6_INTER_LDO_BYPASS
		/*VDDCORE 1.1V@800Mhz: SW1AB*/
		value = 0x20;
		i2c_write(0x8, 0x20, 1, &value, 1);

		/*VDDSOC 1.2V : SW1C*/
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

		/*clear PowerDown Enable bit of WDOG1_WMCR*/
		writew(0, WDOG1_BASE_ADDR + 0x08);
		printf("hw_anadig_reg_core=%x\n",
			REG_RD(ANATOP_BASE_ADDR, HW_ANADIG_REG_CORE));
		#endif

	}
}
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

#if defined CONFIG_MX6Q
		/* SCLK */
		mxc_iomux_v3_setup_pad(MX6Q_PAD_KEY_COL0__ECSPI1_SCLK);

		/* MISO */
		mxc_iomux_v3_setup_pad(MX6Q_PAD_KEY_COL1__ECSPI1_MISO);

		/* MOSI */
		mxc_iomux_v3_setup_pad(MX6Q_PAD_KEY_ROW0__ECSPI1_MOSI);

		mxc_iomux_v3_setup_pad(MX6Q_PAD_KEY_ROW1__ECSPI1_SS0);
#elif defined CONFIG_MX6DL
		/* SCLK */
		mxc_iomux_v3_setup_pad(MX6DL_PAD_KEY_COL0__ECSPI1_SCLK);

		/* MISO */
		mxc_iomux_v3_setup_pad(MX6DL_PAD_KEY_COL1__ECSPI1_MISO);

		/* MOSI */
		mxc_iomux_v3_setup_pad(MX6DL_PAD_KEY_ROW0__ECSPI1_MOSI);

		mxc_iomux_v3_setup_pad(MX6DL_PAD_KEY_ROW1__ECSPI1_SS0);
#endif
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
#if 0
#ifdef CONFIG_NAND_GPMI

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

int setup_gpmi_nand(void)
{
	unsigned int reg;

	/* config gpmi nand iomux */
	mxc_iomux_v3_setup_multiple_pads(nfc_pads,
			ARRAY_SIZE(nfc_pads));


	/* config gpmi and bch clock to 11Mhz*/
	reg = readl(CCM_BASE_ADDR + CLKCTL_CS2CDR);
	reg &= 0xF800FFFF;
	reg |= 0x01E40000;
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
	{USDHC3_BASE_ADDR, 1, 1, 1, 0},
	{USDHC4_BASE_ADDR, 1, 1, 1, 0},
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

int usdhc_gpio_init(bd_t *bis)
{
	s32 status = 0;
	u32 index = 0;

	for (index = 0; index < CONFIG_SYS_FSL_USDHC_NUM;
		++index) {
		switch (index) {
		case 0:
			mxc_iomux_v3_setup_multiple_pads(usdhc1_pads,
				sizeof(usdhc1_pads) /
				sizeof(usdhc1_pads[0]));
			break;
		case 1:
			mxc_iomux_v3_setup_multiple_pads(usdhc2_pads,
				sizeof(usdhc2_pads) /
				sizeof(usdhc2_pads[0]));
			break;
		case 2:
			mxc_iomux_v3_setup_multiple_pads(usdhc3_pads,
				sizeof(usdhc3_pads) /
				sizeof(usdhc3_pads[0]));
			break;
		case 3:
			mxc_iomux_v3_setup_multiple_pads(usdhc4_pads,
				sizeof(usdhc4_pads) /
				sizeof(usdhc4_pads[0]));
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
		printf("MMC Device %d not found\n",
			mmc_dev);
		return -1;
	}

	if (mmc_init(mmc)) {
		puts("MMC init failed\n");
		return  -1;
	}

	blk_start = ALIGN(offset, mmc->read_bl_len) / mmc->read_bl_len;
	blk_cnt   = ALIGN(size, mmc->read_bl_len) / mmc->read_bl_len;
	n = mmc->block_dev.block_read(mmc_dev, blk_start,
					blk_cnt, (u_char *)addr);
	flush_cache((ulong)addr, blk_cnt * mmc->read_bl_len);

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
	cmap:0,
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

	/* EIM_A17 - GPIO2[21] for PWR_GOOD status */
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_A17__GPIO_2_21);

	/* EIM_D17 - GPIO3[17] for VCOM control */
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_D17__GPIO_3_17);

	/* Set as output */
	reg = readl(GPIO3_BASE_ADDR + GPIO_GDIR);
	reg |= (1 << 17);
	writel(reg, GPIO3_BASE_ADDR + GPIO_GDIR);

	/* EIM_D20 - GPIO3[20] for EPD PMIC WAKEUP */
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_D20__GPIO_3_20);
	/* Set as output */
	reg = readl(GPIO3_BASE_ADDR + GPIO_GDIR);
	reg |= (1 << 20);
	writel(reg, GPIO3_BASE_ADDR + GPIO_GDIR);

	/* EIM_A18 - GPIO2[20] for EPD PWR CTL0 */
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_A18__GPIO_2_20);
	/* Set as output */
	reg = readl(GPIO2_BASE_ADDR + GPIO_GDIR);
	reg |= (1 << 20);
	writel(reg, GPIO2_BASE_ADDR + GPIO_GDIR);
}

void epdc_power_on()
{
	unsigned int reg;

	/* Set EPD_PWR_CTL0 to high - enable EINK_VDD (3.15) */
	reg = readl(GPIO2_BASE_ADDR + GPIO_DR);
	reg |= (1 << 20);
	writel(reg, GPIO2_BASE_ADDR + GPIO_DR);

	/* Set PMIC Wakeup to high - enable Display power */
	reg = readl(GPIO3_BASE_ADDR + GPIO_DR);
	reg |= (1 << 20);
	writel(reg, GPIO3_BASE_ADDR + GPIO_DR);

	/* Wait for PWRGOOD == 1 */
	while (1) {
		reg = readl(GPIO2_BASE_ADDR + GPIO_DR);
		if (!(reg & (1 << 21)))
			break;

		udelay(100);
	}

	/* Enable VCOM */
	reg = readl(GPIO3_BASE_ADDR + GPIO_DR);
	reg |= (1 << 17);
	writel(reg, GPIO3_BASE_ADDR + GPIO_DR);

	reg = readl(GPIO3_BASE_ADDR + GPIO_DR);

	udelay(500);
}

void  epdc_power_off()
{
	unsigned int reg;
	/* Set PMIC Wakeup to low - disable Display power */
	reg = readl(GPIO3_BASE_ADDR + GPIO_DR);
	reg &= ~(1 << 20);
	writel(reg, GPIO3_BASE_ADDR + GPIO_DR);

	/* Disable VCOM */
	reg = readl(GPIO3_BASE_ADDR + GPIO_DR);
	reg &= ~(1 << 17);
	writel(reg, GPIO3_BASE_ADDR + GPIO_DR);

	/* Set EPD_PWR_CTL0 to low - disable EINK_VDD (3.15) */
	reg = readl(GPIO2_BASE_ADDR + GPIO_DR);
	reg &= ~(1 << 20);
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
		printf("MMC Device %d not found\n",
			mmc_dev);
		return -1;
	}

	if (mmc_init(mmc)) {
		puts("MMC init failed\n");
		return -1;
	}

	blk_start = ALIGN(offset, mmc->read_bl_len) / mmc->read_bl_len;
	blk_cnt   = ALIGN(size, mmc->read_bl_len) / mmc->read_bl_len;
	n = mmc->block_dev.block_read(mmc_dev, blk_start,
		blk_cnt, (u_char *)addr);
	flush_cache((ulong)addr, blk_cnt * mmc->read_bl_len);

	return (n == blk_cnt) ? 0 : -1;
#else
	return -1;
#endif
}

static void setup_epdc()
{
	unsigned int reg;

	/* epdc iomux settings */
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_A16__EPDC_SDDO_0);
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_DA10__EPDC_SDDO_1);
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_DA12__EPDC_SDDO_2);
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_DA11__EPDC_SDDO_3);
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_LBA__EPDC_SDDO_4);
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_EB2__EPDC_SDDO_5);
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_CS0__EPDC_SDDO_6);
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_RW__EPDC_SDDO_7);
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_A21__EPDC_GDCLK);
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_A22__EPDC_GDSP);
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_A23__EPDC_GDOE);
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_A24__EPDC_GDRL);
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_D31__EPDC_SDCLK);
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_D27__EPDC_SDOE);
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_DA1__EPDC_SDLE);
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_EB1__EPDC_SDSHR);
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_DA2__EPDC_BDR_0);
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_DA4__EPDC_SDCE_0);
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_DA5__EPDC_SDCE_1);
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_DA6__EPDC_SDCE_2);

	/*** epdc Maxim PMIC settings ***/

	/* EPDC PWRSTAT - GPIO2[21] for PWR_GOOD status */
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_A17__GPIO_2_21);

	/* EPDC VCOM0 - GPIO3[17] for VCOM control */
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_D17__GPIO_3_17);

	/* UART4 TXD - GPIO3[20] for EPD PMIC WAKEUP */
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_D20__GPIO_3_20);

	/* EIM_A18 - GPIO2[20] for EPD PWR CTL0 */
	mxc_iomux_v3_setup_pad(MX6DL_PAD_EIM_A18__GPIO_2_20);

	/*** Set pixel clock rates for EPDC ***/

	/* EPDC AXI clk (IPU2_CLK) from PFD_400M, set to 396/2 = 198MHz */
	reg = readl(CCM_BASE_ADDR + CLKCTL_CSCDR3);
	reg &= ~0x7C000;
	reg |= (1 << 16) | (1 << 14);
	writel(reg, CCM_BASE_ADDR + CLKCTL_CSCDR3);

	/* EPDC AXI clk enable */
	reg = readl(CCM_BASE_ADDR + CLKCTL_CCGR3);
	reg |= 0x00C0;
	writel(reg, CCM_BASE_ADDR + CLKCTL_CCGR3);

	/* EPDC PIX clk (IPU2_DI1_CLK) from PLL5, set to 650/4/6 = ~27MHz */
	reg = readl(CCM_BASE_ADDR + CLKCTL_CSCDR2);
	reg &= ~0x3FE00;
	reg |= (2 << 15) | (5 << 12);
	writel(reg, CCM_BASE_ADDR + CLKCTL_CSCDR2);

	/* PLL5 enable (defaults to 650) */
	reg = readl(ANATOP_BASE_ADDR + ANATOP_PLL_VIDEO);
	reg &= ~((1 << 16) | (1 << 12));
	reg |= (1 << 13);
	writel(reg, ANATOP_BASE_ADDR + ANATOP_PLL_VIDEO);

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
	/* No delay required on SABRESD board SD ports */
	return 0;
}
#endif

#endif

#ifndef CONFIG_MXC_EPDC
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

#if defined CONFIG_MX6Q
	mxc_iomux_v3_setup_pad(MX6Q_PAD_SD1_DAT3__GPIO_1_21);
#elif defined CONFIG_MX6DL
	mxc_iomux_v3_setup_pad(MX6DL_PAD_SD1_DAT3__GPIO_1_21);
#endif

	reg = readl(GPIO1_BASE_ADDR + GPIO_GDIR);
	reg |= (1 << 21);
	writel(reg, GPIO1_BASE_ADDR + GPIO_GDIR);

	reg = readl(GPIO1_BASE_ADDR + GPIO_DR);
	reg |= (1 << 21);
	writel(reg, GPIO1_BASE_ADDR + GPIO_DR);

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

	/*
	 * LVDS0 mux to IPU1 DI0.
	 * LVDS1 mux to IPU1 DI1.
	 */
	reg = readl(IOMUXC_BASE_ADDR + 0xC);
	reg &= ~(0x000003C0);
	reg |= 0x00000100;
	writel(reg, IOMUXC_BASE_ADDR + 0xC);

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
				fsl_bmp_reversed_600x400_size);
#endif
		memcpy((char *)addr, (char *)fsl_bmp_reversed_600x400,
				fsl_bmp_reversed_600x400_size);
	}
}
#endif
#endif /* !CONFIG_MXC_EPDC */

int board_init(void)
{
/* need set Power Supply Glitch to 0x41736166
*and need clear Power supply Glitch Detect bit
* when POR or reboot or power on Otherwise system
*could not be power off anymore*/
	u32 reg;
	writel(0x41736166, SNVS_BASE_ADDR + 0x64);/*set LPPGDR*/
	udelay(10);
	reg = readl(SNVS_BASE_ADDR + 0x4c);
	reg |= (1 << 3);
	writel(reg, SNVS_BASE_ADDR + 0x4c);/*clear LPSR*/

	mxc_iomux_v3_init((void *)IOMUXC_BASE_ADDR);
	setup_boot_device();
	fsl_set_system_rev();

	/* board id for linux */
	gd->bd->bi_arch_number = MACH_TYPE_MX6Q_SABRESD;

	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	setup_uart();

#ifdef CONFIG_DWC_AHSATA
	setup_sata();
#endif

#ifdef CONFIG_VIDEO_MX5
	/* Enable lvds power */
	setup_lvds_poweron();

	panel_info_init();

	gd->fb_base = CONFIG_FB_BASE;
#ifdef CONFIG_ARCH_MMU
	gd->fb_base = ioremap_nocache(iomem_to_phys(gd->fb_base), 0);
#endif
#endif

#ifdef CONFIG_NAND_GPMI
	setup_gpmi_nand();
#endif

#ifdef CONFIG_MXC_EPDC
	setup_epdc();
#endif
	return 0;
}


#ifdef CONFIG_ANDROID_RECOVERY

int check_recovery_cmd_file(void)
{
	int button_pressed = 0;
	int recovery_mode = 0;

	recovery_mode = check_and_clean_recovery_flag();

	/* Check Recovery Combo Button press or not. */
	mxc_iomux_v3_setup_pad(MX6X_IOMUX(PAD_GPIO_5__GPIO_1_5));

	gpio_direction_input(GPIO_VOL_DN_KEY);

	if (gpio_get_value(GPIO_VOL_DN_KEY) == 0) { /* VOL_DN key is low assert */
		button_pressed = 1;
		printf("Recovery key pressed\n");
	}

	return recovery_mode || button_pressed;
}
#endif

int board_late_init(void)
{
	#ifdef CONFIG_I2C_MXC
	setup_i2c(CONFIG_SYS_I2C_PORT);
	i2c_bus_recovery();
	setup_pmic_voltages();
	#endif
	return 0;
}

#ifdef CONFIG_MXC_FEC
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

	return 0;
}

#if defined CONFIG_MX6Q
iomux_v3_cfg_t enet_pads[] = {
	MX6Q_PAD_ENET_MDIO__ENET_MDIO,
	MX6Q_PAD_ENET_MDC__ENET_MDC,
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
	MX6Q_PAD_GPIO_0__CCM_CLKO,
	MX6Q_PAD_GPIO_3__CCM_CLKO2,
};
#elif defined CONFIG_MX6DL
iomux_v3_cfg_t enet_pads[] = {
	MX6DL_PAD_ENET_MDIO__ENET_MDIO,
	MX6DL_PAD_ENET_MDC__ENET_MDC,
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
	MX6DL_PAD_GPIO_0__CCM_CLKO,
};
#endif

void enet_board_init(void)
{
	unsigned int reg;
#if defined CONFIG_MX6Q
	iomux_v3_cfg_t enet_reset =
			(_MX6Q_PAD_ENET_CRS_DV__GPIO_1_25 &
			~MUX_PAD_CTRL_MASK)           |
			 MUX_PAD_CTRL(0x88);
#elif defined CONFIG_MX6DL
	iomux_v3_cfg_t enet_reset =
			(MX6DL_PAD_ENET_CRS_DV__GPIO_1_25 &
			~MUX_PAD_CTRL_MASK)           |
			 MUX_PAD_CTRL(0x88);
#endif

	mxc_iomux_v3_setup_multiple_pads(enet_pads,
			ARRAY_SIZE(enet_pads));
	mxc_iomux_v3_setup_pad(enet_reset);

	/* phy reset: gpio1-25 */
	reg = readl(GPIO1_BASE_ADDR + 0x0);
	reg &= ~0x2000000;
	writel(reg, GPIO1_BASE_ADDR + 0x0);

	reg = readl(GPIO1_BASE_ADDR + 0x4);
	reg |= 0x2000000;
	writel(reg, GPIO1_BASE_ADDR + 0x4);

	udelay(500);

	reg = readl(GPIO1_BASE_ADDR + 0x0);
	reg |= 0x2000000;
	writel(reg, GPIO1_BASE_ADDR + 0x0);
}
#endif

int checkboard(void)
{
	printf("Board: %s-SABRESD: %s Board: 0x%x [",
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
	printf(" ]\n");

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

void udc_pins_setting(void)
{
	mxc_iomux_v3_setup_pad(MX6X_IOMUX(PAD_ENET_RX_ER__ANATOP_USBOTG_ID));
	mxc_iomux_v3_setup_pad(MX6X_IOMUX(PAD_EIM_D22__GPIO_3_22));
	mxc_iomux_v3_setup_pad(MX6X_IOMUX(PAD_ENET_TXD1__GPIO_1_29));

	/* USB_OTG_PWR = 0 */
	gpio_direction_output(USB_OTG_PWR, 0);
	/* USB_H1_POWER = 1 */
	gpio_direction_output(USB_H1_POWER, 1);

	mxc_iomux_set_gpr_register(1, 13, 1, 0);

}
#endif
