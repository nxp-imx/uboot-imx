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
#include <asm/errno.h>
#include <asm/arch/mx51.h>
#include <asm/arch/mx51_pins.h>
#include <asm/arch/iomux.h>
#include <i2c.h>
#include <mxc_keyb.h>
#include <asm/arch/keypad.h>
#include "board-mx51_3stack.h"
#include <netdev.h>

DECLARE_GLOBAL_DATA_PTR;

static u32 system_rev;
u32	mx51_io_base_addr;

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
int setup_ata()
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

#ifdef BOARD_LATE_INIT
int board_late_init(void)
{
#if defined(CONFIG_FSL_ANDROID) && defined(CONFIG_MXC_KPD)
	struct kpp_key_info key_info = {0, 0};
	int switch_delay = CONFIG_ANDROID_BOOTMOD_DELAY;
	int state = 0, boot_mode_switch = 0;
#endif

#if defined(CONFIG_FSL_ANDROID) && defined(CONFIG_MXC_KPD)
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
						state = 2;
					} else {
						state = 0;
					}
					break;
				case 1:
					/* Home is already pressed, try to detect Power */
					if (TEST_POWER_KEY_DEPRESS(key_info.val,
						    key_info.evt)) {
						boot_mode_switch = 1;
					} else {
					    if (TEST_HOME_KEY_DEPRESS(key_info.val,
							key_info.evt))
						state = 2;
					    else
						state = 0;
					}
					break;
				case 2:
					/* Power is already pressed, try to detect Home */
					if (TEST_HOME_KEY_DEPRESS(key_info.val,
						    key_info.evt)) {
						boot_mode_switch = 1;
					} else {
						if (TEST_POWER_KEY_DEPRESS(key_info.val,
							    key_info.evt))
							state = 1;
						else
							state = 0;
					}
					break;
				default:
					break;
				}

				if (1 == boot_mode_switch) {
					printf("Boot mode switched to recovery mode!\n");
					/* Set env to recovery mode */
					setenv("bootargs_android", CONFIG_ANDROID_RECOVERY_BOOTARGS);
					setenv("bootcmd_android", CONFIG_ANDROID_RECOVERY_BOOTCMD);
					setenv("bootcmd", "run bootcmd_android");
					break;
				}
			}
		}
		for (i = 0; i < 100; ++i)
			udelay(10000);
	}
#endif

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
	return rc;
}
#endif

#ifdef CONFIG_CMD_MMC

u32 *imx_esdhc_base_addr;

int esdhc_gpio_init(void)
{
	u32 interface_esdhc = 0;
	s32 status = 0;

	interface_esdhc = (readl(SRC_BASE_ADDR + 0x4) & (0x00180000)) >> 19;

	switch (interface_esdhc) {
	case 0:

		imx_esdhc_base_addr = (u32 *)MMC_SDHC1_BASE_ADDR;

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
int setup_mxc_kpd()
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
