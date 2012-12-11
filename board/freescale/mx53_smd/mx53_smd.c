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
#include <netdev.h>

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

#ifdef CONFIG_GET_FEC_MAC_ADDR_FROM_IIM
#include <asm/imx_iim.h>
#endif

#ifdef CONFIG_CMD_CLOCK
#include <asm/clock.h>
#endif

#ifdef CONFIG_ANDROID_RECOVERY
#include <recovery.h>
#include <part.h>
#include <ext2fs.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <ubi_uboot.h>
#include <jffs2/load_kernel.h>
#define	GPIO_VOL_DN_KEY	14
#endif

DECLARE_GLOBAL_DATA_PTR;

static u32 system_rev;
static enum boot_device boot_dev;

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

struct pwm_device pwm0 = {
	.pwm_id = 1,
	.pwmo_invert = 0,
};

struct pwm_device pwm1 = {
	.pwm_id = 1,
	.pwmo_invert = 0,
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
	case 0x02:
		system_rev |= BOARD_REV_5;
		break;
	case 0x01:
	case 0x00:
		system_rev |= BOARD_REV_4;
		break;
	default:
		system_rev |= BOARD_REV_UNKNOWN;
	}
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
	X_ARM_MMU_SECTION(0x000, 0x000, 0x010,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* ROM, 16M */
	X_ARM_MMU_SECTION(0x010, 0x010, 0x060,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* Reserved, 96M */
	X_ARM_MMU_SECTION(0x070, 0x070, 0x010,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* IRAM, 16M */
	X_ARM_MMU_SECTION(0x080, 0x080, 0x080,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* Reserved region + TZIC. 1M */
	X_ARM_MMU_SECTION(0x100, 0x100, 0x040,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* SATA */
	X_ARM_MMU_SECTION(0x140, 0x140, 0x040,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* Reserved, 64M */
	X_ARM_MMU_SECTION(0x180, 0x180, 0x080,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* IPUv3M */
	X_ARM_MMU_SECTION(0x200, 0x200, 0x200,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* GPU */
	X_ARM_MMU_SECTION(0x400, 0x400, 0x300,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* periperals */
	X_ARM_MMU_SECTION(0x700, 0x700, 0x200,
			ARM_CACHEABLE, ARM_BUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CSD0 512M */
	X_ARM_MMU_SECTION(0x700, 0x900, 0x200,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CSD0 512M */
	X_ARM_MMU_SECTION(0xB00, 0xB00, 0x200,
			ARM_CACHEABLE, ARM_BUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CSD0 512M */
	X_ARM_MMU_SECTION(0xB00, 0xD00, 0x200,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CSD0 512M */
	X_ARM_MMU_SECTION(0xF00, 0xF00, 0x07F,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CS1 EIM control*/
	X_ARM_MMU_SECTION(0xF7F, 0xF7F, 0x001,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* NAND Flash buffer */
	X_ARM_MMU_SECTION(0xF80, 0xF80, 0x080,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* iRam + GPU3D + Reserved */

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
	gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
	gd->bd->bi_dram[1].size = PHYS_SDRAM_2_SIZE;
	return 0;
}

static void setup_uart(void)
{
	/* UART1 RXD */
	mxc_request_iomux(MX53_PIN_CSI0_D11, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX53_PIN_CSI0_D11, 0x1E4);
	mxc_iomux_set_input(MUX_IN_UART1_IPP_UART_RXD_MUX_SELECT_INPUT, 0x1);

	/* UART1 TXD */
	mxc_request_iomux(MX53_PIN_CSI0_D10, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX53_PIN_CSI0_D10, 0x1E4);
}

#ifdef CONFIG_I2C_MXC
static void setup_i2c(unsigned int module_base)
{
	switch (module_base) {
	case I2C1_BASE_ADDR:
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
		mxc_request_iomux(MX53_PIN_GPIO_6,
				IOMUX_CONFIG_ALT2 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C3_IPP_SDA_IN_SELECT_INPUT,
				INPUT_CTL_PATH1);
		mxc_iomux_set_pad(MX53_PIN_GPIO_6,
				PAD_CTL_PUE_PULL | PAD_CTL_PKE_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_360K_PD |
				PAD_CTL_HYS_ENABLE);
		/* No device is connected via I2C3 in EVK and ARM2 */
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
/* DA9053 I2C SDA stuck low issue: the I2C block in DA9053 may not correctly
 * receive a Power On Reset and device is in unknown state during start-up.
 * The only way to get the chip into known state before any communication
 * with the Chip via I2C is to dummy clock the I2C and bring it in a state
 * where I2C can communicate. Dialog suggested to provide 9 clock on SCL.
 * Dialog don't know the exact reason for the fault and assume it is because
 * some random noise or spurious behaviour.
 * This has to been done in host platform specific I2C driver during
 * start-up when the I2C is being configured at platform level to supply with
 * dummy 9 clock on SCL. Dialog I2C driver has no control to provide dummy 9
 * clock on SCL.
 */
#define I2C1_SDA_GPIO5_26_BIT_MASK  (1 << 26)
#define I2C1_SCL_GPIO5_27_BIT_MASK  (1 << 27)
int da9053_i2c_startup_reset(void)
{
	unsigned int reg, i, retry = 3;

	do {
		/* set I2C1_SDA as GPIO input */
		mxc_request_iomux(MX53_PIN_CSI0_D8, IOMUX_CONFIG_ALT1);
		reg = readl(GPIO5_BASE_ADDR + 0x4);
		reg &= ~I2C1_SDA_GPIO5_26_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + 0x4);

		/* set I2C1_SCL as GPIO output */
		mxc_request_iomux(MX53_PIN_CSI0_D9, IOMUX_CONFIG_ALT1);
		reg = readl(GPIO5_BASE_ADDR + 0x0);
		reg |= I2C1_SCL_GPIO5_27_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + 0x0);

		reg = readl(GPIO5_BASE_ADDR + 0x4);
		reg |= I2C1_SCL_GPIO5_27_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + 0x4);
		__udelay(10000);

		for (i = 0; i < 10; i++) {
			reg = readl(GPIO5_BASE_ADDR + 0x0);
			reg |= I2C1_SCL_GPIO5_27_BIT_MASK;
			writel(reg, GPIO5_BASE_ADDR + 0x0);
			__udelay(4);

			reg = readl(GPIO5_BASE_ADDR + 0x0);
			reg &= ~I2C1_SCL_GPIO5_27_BIT_MASK;
			writel(reg, GPIO5_BASE_ADDR + 0x0);
			__udelay(4);
		}
		reg = readl(GPIO5_BASE_ADDR + 0x0);
		reg |= I2C1_SCL_GPIO5_27_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + 0x0);
		__udelay(20);

		/* I2C1_SDA = output */
		reg = readl(GPIO5_BASE_ADDR + 0x4);
		reg |= I2C1_SDA_GPIO5_26_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + 0x4);

		/* I2C1_SDA = H i2c start and stop bit */
		reg = readl(GPIO5_BASE_ADDR + 0x0);
		reg |= I2C1_SDA_GPIO5_26_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + 0x0);
		__udelay(4);
		/* I2C1_SDA = L */
		reg = readl(GPIO5_BASE_ADDR + 0x0);
		reg &= ~I2C1_SDA_GPIO5_26_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + 0x0);
		__udelay(4);
		/* I2C1_SCL = L */
		reg = readl(GPIO5_BASE_ADDR + 0x0);
		reg &= ~I2C1_SCL_GPIO5_27_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + 0x0);
		__udelay(4);
		/* I2C1_SCL = H */
		reg = readl(GPIO5_BASE_ADDR + 0x0);
		reg |= I2C1_SCL_GPIO5_27_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + 0x0);
		__udelay(4);
		/* I2C1_SDA = H */
		reg = readl(GPIO5_BASE_ADDR + 0x0);
		reg |= I2C1_SDA_GPIO5_26_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + 0x0);
		__udelay(4);

		/* I2C1_SDA = input */
		reg = readl(GPIO5_BASE_ADDR + 0x4);
		reg &= ~I2C1_SDA_GPIO5_26_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + 0x4);

		reg = readl(GPIO5_BASE_ADDR + 0x8);
		if (reg & I2C1_SDA_GPIO5_26_BIT_MASK) {
			break;
		} else
			printf("***I2C1_SDA = low***\n");
	} while (retry--);
	/* restore I2C1_SDA/I2C1_SCL iomux */
	mxc_request_iomux(MX53_PIN_CSI0_D8,
			IOMUX_CONFIG_ALT5 | IOMUX_CONFIG_SION);
	mxc_request_iomux(MX53_PIN_CSI0_D9,
			IOMUX_CONFIG_ALT5 | IOMUX_CONFIG_SION);
	return 0;
}

#define DA9053_DUMMY_WRITE_REG (0xff)
int i2c_read_da9053(uchar chip, uint addr, int alen, uchar *buf, int len)
{
	int ret = 0;
	uchar dummy = 0xff;

	ret = i2c_read(chip, addr, alen, buf, len);
	if (ret == 0) {
		ret = i2c_write(chip, DA9053_DUMMY_WRITE_REG, 1, &dummy, 1);
		if (0 != ret)
			printf("[I2C-Rd-DA9053]dummy write reg fail\n");
		return 0;
	} else {
		printf("[I2C-DA9053]read i2c fail\n");
		return -1;
	}
}

int i2c_write_da9053(uchar chip, uint addr, int alen, uchar *buf, int len)
{
	int ret = 0;
	uchar dummy = 0xff;

	ret = i2c_write(chip, addr, alen, buf, len);
	if (ret == 0) {
		ret = i2c_write(chip, DA9053_DUMMY_WRITE_REG, 1, &dummy, 1);
		if (0 != ret)
			printf("[I2C-Wr-DA9053]dummy write reg fail\n");
		return 0;
	} else {
		printf("[I2C-DA9053]write i2c fail\n");
		return -1;
	}
}

#ifdef CONFIG_I2C_MXC
#define I2C1_SDA_GPIO5_26_BIT_MASK  (1 << 26)
#define I2C1_SCL_GPIO5_27_BIT_MASK  (1 << 27)
#define I2C2_SCL_GPIO4_12_BIT_MASK  (1 << 12)
#define I2C2_SDA_GPIO4_13_BIT_MASK  (1 << 13)
#define I2C3_SCL_GPIO1_3_BIT_MASK   (1 << 3)
#define I2C3_SDA_GPIO1_6_BIT_MASK   (1 << 6)
static void mx53_i2c_gpio_scl_direction(int bus, int output)
{
	u32 reg;

	switch (bus) {
	case 1:
		mxc_request_iomux(MX53_PIN_CSI0_D9, IOMUX_CONFIG_ALT1);
		reg = readl(GPIO5_BASE_ADDR + GPIO_GDIR);
		if (output)
			reg |= I2C1_SCL_GPIO5_27_BIT_MASK;
		else
			reg &= ~I2C1_SCL_GPIO5_27_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + GPIO_GDIR);
		break;
	case 2:
		mxc_request_iomux(MX53_PIN_KEY_COL3, IOMUX_CONFIG_ALT1);
		reg = readl(GPIO4_BASE_ADDR + GPIO_GDIR);
		if (output)
			reg |= I2C2_SCL_GPIO4_12_BIT_MASK;
		else
			reg &= ~I2C2_SCL_GPIO4_12_BIT_MASK;
		writel(reg, GPIO4_BASE_ADDR + GPIO_GDIR);
		break;
	case 3:
		mxc_request_iomux(MX53_PIN_GPIO_3, IOMUX_CONFIG_ALT1);
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
static void mx53_i2c_gpio_sda_direction(int bus, int output)
{
	u32 reg;

	switch (bus) {
	case 1:
		mxc_request_iomux(MX53_PIN_CSI0_D8, IOMUX_CONFIG_ALT1);

		reg = readl(GPIO5_BASE_ADDR + GPIO_GDIR);
		if (output) {
			mxc_iomux_set_pad(MX53_PIN_CSI0_D8,
					  PAD_CTL_ODE_OPENDRAIN_ENABLE |
					  PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU);
			reg |= I2C1_SDA_GPIO5_26_BIT_MASK;
		} else
			reg &= ~I2C1_SDA_GPIO5_26_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + GPIO_GDIR);
		break;
	case 2:
		mxc_request_iomux(MX53_PIN_KEY_ROW3, IOMUX_CONFIG_ALT1);

		mxc_iomux_set_pad(MX53_PIN_KEY_ROW3,
				  PAD_CTL_SRE_FAST |
				  PAD_CTL_ODE_OPENDRAIN_ENABLE |
				  PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				  PAD_CTL_HYS_ENABLE);

		reg = readl(GPIO4_BASE_ADDR + GPIO_GDIR);
		if (output)
			reg |= I2C2_SDA_GPIO4_13_BIT_MASK;
		else
			reg &= ~I2C2_SDA_GPIO4_13_BIT_MASK;
		writel(reg, GPIO4_BASE_ADDR + GPIO_GDIR);
	case 3:
		mxc_request_iomux(MX53_PIN_GPIO_6, IOMUX_CONFIG_ALT1);
		mxc_iomux_set_pad(MX53_PIN_GPIO_6,
				  PAD_CTL_PUE_PULL | PAD_CTL_PKE_ENABLE |
				  PAD_CTL_DRV_HIGH | PAD_CTL_360K_PD |
				  PAD_CTL_HYS_ENABLE);
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
static void mx53_i2c_gpio_scl_set_level(int bus, int high)
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
static void mx53_i2c_gpio_sda_set_level(int bus, int high)
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

static int mx53_i2c_gpio_check_sda(int bus)
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
		mx53_i2c_gpio_sda_direction(bus, 0);

		if (mx53_i2c_gpio_check_sda(bus) == 0) {
			printf("i2c: I2C%d SDA is low, start i2c recovery...\n", bus);
			mx53_i2c_gpio_scl_direction(bus, 1);
			mx53_i2c_gpio_scl_set_level(bus, 1);
			__udelay(10000);

			for (i = 0; i < 9; i++) {
				mx53_i2c_gpio_scl_set_level(bus, 1);
				__udelay(5);
				mx53_i2c_gpio_scl_set_level(bus, 0);
				__udelay(5);
			}

			/* 9th clock here, the slave should already
			   release the SDA, we can set SDA as high to
			   a NAK.*/
			mx53_i2c_gpio_sda_direction(bus, 1);
			mx53_i2c_gpio_sda_set_level(bus, 1);
			__udelay(1); /* Pull up SDA first */
			mx53_i2c_gpio_scl_set_level(bus, 1);
			__udelay(5); /* plus pervious 1 us */
			mx53_i2c_gpio_scl_set_level(bus, 0);
			__udelay(5);
			mx53_i2c_gpio_sda_set_level(bus, 0);
			__udelay(5);
			mx53_i2c_gpio_scl_set_level(bus, 1);
			__udelay(5);
			/* Here: SCL is high, and SDA from low to high, it's a
			 * stop condition */
			mx53_i2c_gpio_sda_set_level(bus, 1);
			__udelay(5);

			mx53_i2c_gpio_sda_direction(bus, 0);
			if (mx53_i2c_gpio_check_sda(bus) == 1)
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
#endif

/* restore VUSB 2V5 active after suspend */
#define BUCKPERI_RESTORE_SW_STEP   (0x55)
/* restore VUSB 2V5 power supply after suspend */
#define SUPPLY_RESTORE_VPERISW_EN  (0x20)
#define DA9052_ID1213_REG		   (35)
#define DA9052_SUPPLY_REG		   (60)
void setup_pmic_voltages(void)
{
	uchar value;
	int retries = 10, ret = -1;

	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
	da9053_i2c_startup_reset();

#ifdef CONFIG_CPU_1_2G
	/* Increase VDDGP as 1.3V for 1.2GHz */
	value = 0x60;
#else
	/* increase VDDGP as 1.25V for 1GHZ */
	value = 0x5e;
#endif
	do {
		if (0 != i2c_write_da9053(0x48, 0x2e, 1, &value, 1)) {
			printf("da9052_i2c_is_connected - i2c write failed.....\n");
		} else {
			printf("da9052_i2c_is_connected - i2c write success....\n");
			ret = 0;
		}
	} while (ret != 0 && retries--);
	i2c_read_da9053(0x48, 60, 1, &value, 1);
	value |= 0x1;
	i2c_write_da9053(0x48, 60, 1, &value, 1);

	/* restore VUSB_2V5 when reset from suspend state */
	value = BUCKPERI_RESTORE_SW_STEP;
	i2c_write_da9053(0x48, DA9052_ID1213_REG, 1, &value, 1);
	i2c_read_da9053(0x48, DA9052_SUPPLY_REG, 1, &value, 1);
	value |= SUPPLY_RESTORE_VPERISW_EN;
	i2c_write_da9053(0x48, DA9052_SUPPLY_REG, 1, &value, 1);
}

#endif

#ifdef CONFIG_IMX_ECSPI
s32 spi_get_cfg(struct imx_spi_dev_t *dev)
{
	switch (dev->slave.cs) {
	case 0:
		/* Zigbee */
		dev->base = CSPI1_BASE_ADDR;
		dev->freq = 25000000;
		dev->ss_pol = IMX_SPI_ACTIVE_HIGH;
		dev->ss = 0;
		dev->fifo_sz = 64 * 4;
		dev->us_delay = 0;
		break;
	case 1:
		/* SPI-NOR */
		dev->base = CSPI1_BASE_ADDR;
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
	switch (dev->base) {
	case CSPI1_BASE_ADDR:
		/* SCLK */
		mxc_request_iomux(MX53_PIN_EIM_D16, IOMUX_CONFIG_ALT4);
		mxc_iomux_set_pad(MX53_PIN_EIM_D16, 0x104);
		mxc_iomux_set_input(MUX_IN_ECSPI1_IPP_CSPI_CLK_IN_SELECT_INPUT,
				0x3);

		/* MISO */
		mxc_request_iomux(MX53_PIN_EIM_D17, IOMUX_CONFIG_ALT4);
		mxc_iomux_set_pad(MX53_PIN_EIM_D17, 0x104);
		mxc_iomux_set_input(MUX_IN_ECSPI1_IPP_IND_MISO_SELECT_INPUT,
				0x3);

		/* MOSI */
		mxc_request_iomux(MX53_PIN_EIM_D18, IOMUX_CONFIG_ALT4);
		mxc_iomux_set_pad(MX53_PIN_EIM_D18, 0x104);
		mxc_iomux_set_input(MUX_IN_ECSPI1_IPP_IND_MOSI_SELECT_INPUT,
				0x3);

		if (dev->ss == 0) {
			mxc_request_iomux(MX53_PIN_EIM_EB2,
						IOMUX_CONFIG_ALT4);
			mxc_iomux_set_pad(MX53_PIN_EIM_EB2, 0x104);
			mxc_iomux_set_input(
				MUX_IN_ECSPI1_IPP_IND_SS_B_1_SELECT_INPUT,
				0x3);
		} else if (dev->ss == 1) {
			mxc_request_iomux(MX53_PIN_EIM_D19, IOMUX_CONFIG_ALT4);
			mxc_iomux_set_pad(MX53_PIN_EIM_D19, 0x104);
			mxc_iomux_set_input(
				MUX_IN_ECSPI1_IPP_IND_SS_B_2_SELECT_INPUT,
				0x2);
		}
		break;
	case CSPI2_BASE_ADDR:
	case CSPI3_BASE_ADDR:
		/* ecspi2-3 fall through */
		break;
	default:
		break;
	}
}
#endif


#if defined(CONFIG_DWC_AHSATA)
static void setup_sata_device(void)
{
	u32 *tmp_base =
		(u32 *)(IIM_BASE_ADDR + 0x180c);
	u32 reg;
	mxc_request_iomux(MX53_PIN_EIM_DA3, IOMUX_CONFIG_ALT1);

	/* GPIO_3_3 */
	reg = readl(GPIO3_BASE_ADDR);
	reg |= (0x1 << 3);
	writel(reg, GPIO3_BASE_ADDR);

	reg = readl(GPIO3_BASE_ADDR + 0x4);
	reg |= (0x1 << 3);
	writel(reg, GPIO3_BASE_ADDR + 0x4);

	udelay(1000);

	/* Set USB_PHY1 clk, fuse bank4 row3 bit2 */
	set_usb_phy1_clk();
	writel((readl(tmp_base) & (~0x7)) | 0x4, tmp_base);
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


#ifdef CONFIG_NET_MULTI
int board_eth_init(bd_t *bis)
{
	int rc = -ENODEV;

	return rc;
}
#endif

#ifdef CONFIG_CMD_MMC

struct fsl_esdhc_cfg esdhc_cfg[2] = {
	{MMC_SDHC1_BASE_ADDR, 1, 1},
	{MMC_SDHC3_BASE_ADDR, 1, 1},
};

#ifdef CONFIG_DYNAMIC_MMC_DEVNO
int get_mmc_env_devno(void)
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

			break;
		default:
			printf("Warning: you configured more ESDHC controller"
				"(%d) as supported by the board(2)\n",
				CONFIG_SYS_FSL_ESDHC_NUM);
			return status;
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
	g_ipu_hw_rev = IPUV3_HW_REV_IPUV3M;

	/* 20KHz PWM wave, 50% duty */
	if (di == 1) {
		imx_pwm_config(pwm1, 25000, 50000);
		imx_pwm_enable(pwm1);
	} else {
		imx_pwm_config(pwm0, 25000, 50000);
		imx_pwm_enable(pwm0);
	}
	mxc_request_iomux(MX53_PIN_GPIO_1, IOMUX_CONFIG_ALT4);

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
				fsl_bmp_reversed_600x400_size);
#endif
		memcpy((char *)addr, (char *)fsl_bmp_reversed_600x400,
				fsl_bmp_reversed_600x400_size);
	}
}
#endif

int board_init(void)
{
	unsigned int val;
#ifdef CONFIG_MFG
/* MFG firmware need reset usb to avoid host crash firstly */
#define USBCMD 0x140
	val = readl(OTG_BASE_ADDR + USBCMD);
	val &= ~0x1; /*RS bit*/
	writel(val, OTG_BASE_ADDR + USBCMD);
#endif

	/* Workaround: To make watchdog timeout work in mx53
	 * SMD, force warm reset as cold reset
	 */
	val = readl(SRC_BASE_ADDR);
	val &= 0xFFFFFFFE;
	writel(val, SRC_BASE_ADDR);

	setup_boot_device();
	setup_soc_rev();

	gd->bd->bi_arch_number = MACH_TYPE_MX53_SMD;	/* board id for linux */

	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	setup_uart();

#ifdef CONFIG_MXC_FEC
	setup_fec();
#endif

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

int check_recovery_cmd_file(void)
{
	int button_pressed = 0;
	int recovery_mode = 0;
	u32 reg;

	/* Enter Recovery Mode if Recovery Switch is set. */
	recovery_mode = check_and_clean_recovery_flag();

	/* Check Recovery Combo Button press or not. */
	mxc_request_iomux(MX53_PIN_ATA_DATA14, IOMUX_CONFIG_ALT1);
	/* Set GPIO direction to INPUT */
	reg = readl(GPIO2_BASE_ADDR + GPIO_GDIR);
	reg &= ~(1 << GPIO_VOL_DN_KEY);
	writel(reg, GPIO2_BASE_ADDR + GPIO_GDIR);
	/* Read GPIO status */
	reg = readl(GPIO2_BASE_ADDR + GPIO_PSR);
	if (!(reg & (1 << GPIO_VOL_DN_KEY))) { /* Vol - is low assert */
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
	/* first recovery I2C bus in case other device are reset
	 * during transcation */
	i2c_bus_recovery();
	/* Increase VDDGP voltage */
	setup_pmic_voltages();
#ifdef CONFIG_CPU_1_2G
	/* Switch to 1.2GHz */
	clk_config(CONFIG_REF_CLK_FREQ, 1200, CPU_CLK);
#else
	/* Switch to 1GHZ */
	clk_config(CONFIG_REF_CLK_FREQ, 1000, CPU_CLK);
#endif
#endif
	return 0;
}

int checkboard(void)
{
	printf("Board: MX53-SMD ");

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
