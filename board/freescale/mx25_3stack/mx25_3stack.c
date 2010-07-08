/*
 * (c) Copyright 2009-2010 Freescale Semiconductor
 *
 * (c) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
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
#include <asm/arch/mx25.h>
#include <asm/arch/mx25-regs.h>
#include <asm/arch/mx25_pins.h>
#include <asm/arch/iomux.h>
#include <asm/arch/gpio.h>
#include <imx_spi.h>

#ifdef CONFIG_LCD
#include <mx2fb.h>
#include <lcd.h>
#endif

#ifdef CONFIG_GET_FEC_MAC_ADDR_FROM_IIM
#include <asm/imx_iim.h>
#endif

#ifdef CONFIG_CMD_MMC
#include <mmc.h>
#include <fsl_esdhc.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

static u32 system_rev;
#ifdef CONFIG_LCD
char lcd_cmap[256];
#endif

u32 get_board_rev(void)
{
	return system_rev;
}

static inline void setup_soc_rev(void)
{
	int reg;
	reg = __REG(IIM_BASE + IIM_SREV);
	if (!reg) {
		reg = __REG(ROMPATCH_REV);
		reg <<= 4;
	} else
		reg += CHIP_REV_1_0;
	system_rev = 0x25000 + (reg & 0xFF);
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

#ifdef CONFIG_CMD_MMC

struct fsl_esdhc_cfg esdhc_cfg[2] = {
	{MMC_SDHC1_BASE, 1, 1},
	{MMC_SDHC2_BASE, 1, 1},
};

int esdhc_gpio_init(bd_t *bis)
{
	s32 status = 0;
	u32 index = 0;
	u32 val = 0;

	for (index = 0; index < CONFIG_SYS_FSL_ESDHC_NUM;
		++index) {
		switch (index) {
		case 0:
			/* Pins */
			writel(0x10, IOMUXC_BASE + 0x190); /* SD1_CMD */
			writel(0x10, IOMUXC_BASE + 0x194); /* SD1_CLK */
			writel(0x00, IOMUXC_BASE + 0x198); /* SD1_DATA0 */
			writel(0x00, IOMUXC_BASE + 0x19c); /* SD1_DATA1 */
			writel(0x00, IOMUXC_BASE + 0x1a0); /* SD1_DATA2 */
			writel(0x00, IOMUXC_BASE + 0x1a4); /* SD1_DATA3 */
			writel(0x06, IOMUXC_BASE + 0x094); /* D12 (SD1_DATA4) */
			writel(0x06, IOMUXC_BASE + 0x090); /* D13 (SD1_DATA5) */
			writel(0x06, IOMUXC_BASE + 0x08c); /* D14 (SD1_DATA6) */
			writel(0x06, IOMUXC_BASE + 0x088); /* D15 (SD1_DATA7) */
			writel(0x05, IOMUXC_BASE + 0x010); /* A14 (SD1_WP) */
			writel(0x05, IOMUXC_BASE + 0x014); /* A15 (SD1_DET) */

			/* Pads */
			writel(0xD1, IOMUXC_BASE + 0x388); /* SD1_CMD */
			writel(0xD1, IOMUXC_BASE + 0x38c); /* SD1_CLK */
			writel(0xD1, IOMUXC_BASE + 0x390); /* SD1_DATA0 */
			writel(0xD1, IOMUXC_BASE + 0x394); /* SD1_DATA1 */
			writel(0xD1, IOMUXC_BASE + 0x398); /* SD1_DATA2 */
			writel(0xD1, IOMUXC_BASE + 0x39c); /* SD1_DATA3 */
			writel(0xD1, IOMUXC_BASE + 0x28c); /* D12 (SD1_DATA4) */
			writel(0xD1, IOMUXC_BASE + 0x288); /* D13 (SD1_DATA5) */
			writel(0xD1, IOMUXC_BASE + 0x284); /* D14 (SD1_DATA6) */
			writel(0xD1, IOMUXC_BASE + 0x280); /* D15 (SD1_DATA7) */
			writel(0xD1, IOMUXC_BASE + 0x230); /* A14 (SD1_WP) */
			writel(0xD1, IOMUXC_BASE + 0x234); /* A15 (SD1_DET) */

			/*
			 * Set write protect and card detect gpio as inputs
			 * A14 (SD1_WP) and A15 (SD1_DET)
			 */
			val = ~(3 << 0) & readl(GPIO1_BASE + GPIO_GDIR);
			writel(val, GPIO1_BASE + GPIO_GDIR);
			break;
		case 1:
			/* Pins */
			writel(0x16, IOMUXC_BASE + 0x0e8); /* LD8 (SD1_CMD) */
			writel(0x16, IOMUXC_BASE + 0x0ec); /* LD9 (SD1_CLK) */
			writel(0x06, IOMUXC_BASE + 0x0f0); /* LD10 (SD1_DATA0)*/
			writel(0x06, IOMUXC_BASE + 0x0f4); /* LD11 (SD1_DATA1)*/
			writel(0x06, IOMUXC_BASE + 0x0f8); /* LD12 (SD1_DATA2)*/
			writel(0x06, IOMUXC_BASE + 0x0fc); /* LD13 (SD1_DATA3)*/
			/* CSI_D2 (SD1_DATA4) */
			writel(0x02, IOMUXC_BASE + 0x120);
			/* CSI_D3 (SD1_DATA5) */
			writel(0x02, IOMUXC_BASE + 0x124);
			/* CSI_D4 (SD1_DATA6) */
			writel(0x02, IOMUXC_BASE + 0x128);
			/* CSI_D5 (SD1_DATA7) */
			writel(0x02, IOMUXC_BASE + 0x12c);

			/* Pads */
			writel(0xD1, IOMUXC_BASE + 0x2e0); /* LD8 (SD1_CMD) */
			writel(0xD1, IOMUXC_BASE + 0x2e4); /* LD9 (SD1_CLK) */
			writel(0xD1, IOMUXC_BASE + 0x2e8); /* LD10 (SD1_DATA0)*/
			writel(0xD1, IOMUXC_BASE + 0x2ec); /* LD11 (SD1_DATA1)*/
			writel(0xD1, IOMUXC_BASE + 0x2f0); /* LD12 (SD1_DATA2)*/
			writel(0xD1, IOMUXC_BASE + 0x2f4); /* LD13 (SD1_DATA3)*/
			/* CSI_D2 (SD1_DATA4) */
			writel(0xD1, IOMUXC_BASE + 0x318);
			/* CSI_D3 (SD1_DATA5) */
			writel(0xD1, IOMUXC_BASE + 0x31c);
			/* CSI_D4 (SD1_DATA6) */
			writel(0xD1, IOMUXC_BASE + 0x320);
			/* CSI_D5 (SD1_DATA7) */
			writel(0xD1, IOMUXC_BASE + 0x324);
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
	return 0;
}

int board_mmc_init(bd_t *bis)
{
	if (!esdhc_gpio_init(bis))
		return 0;
	else
		return -1;
}
#endif

s32 spi_get_cfg(struct imx_spi_dev_t *dev)
{
	switch (dev->slave.cs) {
	case 0:
		/* cpld */
		dev->base = CSPI1_BASE;
		dev->freq = 25000000;
		dev->ss_pol = IMX_SPI_ACTIVE_LOW;
		dev->ss = 0;
		dev->fifo_sz = 32;
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
	case CSPI1_BASE:
		writel(0, IOMUXC_BASE + 0x180);		/* CSPI1 SCLK */
		writel(0x1C0, IOMUXC_BASE + 0x5c4);
		writel(0, IOMUXC_BASE + 0x184);		/* SPI_RDY */
		writel(0x1E0, IOMUXC_BASE + 0x5c8);
		writel(0, IOMUXC_BASE + 0x170);		/* MOSI */
		writel(0x1C0, IOMUXC_BASE + 0x5b4);
		writel(0, IOMUXC_BASE + 0x174);		/* MISO */
		writel(0x1C0, IOMUXC_BASE + 0x5b8);
		writel(0, IOMUXC_BASE + 0x17C);		/* SS1 */
		writel(0x1E0, IOMUXC_BASE + 0x5C0);
		break;
	default:
		break;
	}
}

#ifdef CONFIG_LCD

vidinfo_t panel_info = {
	vl_refresh:60,
	vl_col:640,
	vl_row:480,
	vl_pixclock:39683,
	vl_left_margin:45,
	vl_right_margin:114,
	vl_upper_margin:33,
	vl_lower_margin:11,
	vl_hsync:1,
	vl_vsync:1,
	vl_sync : FB_SYNC_CLK_LAT_FALL,
	vl_mode:0,
	vl_flag:0,
	vl_bpix:4,
	cmap : (void *)lcd_cmap,
};

void lcdc_hw_init(void)
{
	/* Set VSTBY_REQ as GPIO3[17] on ALT5 */
	mxc_request_iomux(MX25_PIN_VSTBY_REQ, MUX_CONFIG_ALT5);

	/* Set GPIO3[17] as output */
	writel(0x20000, GPIO3_BASE + 0x04);

	/* Set GPIOE as LCDC_LD[16] on ALT2 */
	mxc_request_iomux(MX25_PIN_GPIO_E, MUX_CONFIG_ALT2);

	/* Set GPIOF as LCDC_LD[17] on ALT2 */
	mxc_request_iomux(MX25_PIN_GPIO_F, MUX_CONFIG_ALT2);

	/* Enable pull up on LCDC_LD[16]	*/
	mxc_iomux_set_pad(MX25_PIN_GPIO_E,
			PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PU);

	/* Enable pull up on LCDC_LD[17]	*/
	mxc_iomux_set_pad(MX25_PIN_GPIO_F,
			PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PU);

	/* Enable Pull/Keeper for pad LSCKL */
	mxc_iomux_set_pad(MX25_PIN_LSCLK,
			PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PUD |
			PAD_CTL_100K_PU | PAD_CTL_SRE_FAST);

	gd->fb_base = CONFIG_FB_BASE;
}

#ifdef CONFIG_SPLASH_SCREEN
int setup_splash_img()
{
#ifdef CONFIG_SPLASH_IS_IN_MMC
	int mmc_dev = CONFIG_SPLASH_IMG_MMC_DEV;
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
#endif

#ifdef CONFIG_GET_FEC_MAC_ADDR_FROM_IIM

int fec_get_mac_addr(unsigned char *mac)
{
	u32 *iim0_mac_base =
		(u32 *)(IIM_BASE + IIM_BANK_AREA_0_OFFSET +
			CONFIG_IIM_MAC_ADDR_OFFSET);
	int i;

	for (i = 0; i < 6; ++i, ++iim0_mac_base)
		mac[i] = readl(iim0_mac_base);

	return 0;
}
#endif

int board_init(void)
{

#ifdef CONFIG_MFG
	/* MFG firmware need reset usb to avoid host crash firstly */
#define USBCMD 0x140
	int val = readl(USB_BASE + USBCMD);
	val &= ~0x1; /*RS bit*/
	writel(val, USB_BASE + USBCMD);
#endif

	setup_soc_rev();

	/* setup pins for UART1 */
	/* UART 1 IOMUX Configs */
	mxc_request_iomux(MX25_PIN_UART1_RXD, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX25_PIN_UART1_TXD, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX25_PIN_UART1_RTS, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX25_PIN_UART1_CTS, MUX_CONFIG_FUNC);
	mxc_iomux_set_pad(MX25_PIN_UART1_RXD,
			PAD_CTL_HYS_SCHMITZ | PAD_CTL_PKE_ENABLE |
			PAD_CTL_PUE_PUD | PAD_CTL_100K_PU);
	mxc_iomux_set_pad(MX25_PIN_UART1_TXD,
			PAD_CTL_PUE_PUD | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX25_PIN_UART1_RTS,
			PAD_CTL_HYS_SCHMITZ | PAD_CTL_PKE_ENABLE |
			PAD_CTL_PUE_PUD | PAD_CTL_100K_PU);
	mxc_iomux_set_pad(MX25_PIN_UART1_CTS,
			PAD_CTL_PUE_PUD | PAD_CTL_100K_PD);

	/* setup pins for FEC */
	mxc_request_iomux(MX25_PIN_FEC_TX_CLK, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX25_PIN_FEC_RX_DV, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX25_PIN_FEC_RDATA0, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX25_PIN_FEC_TDATA0, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX25_PIN_FEC_TX_EN, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX25_PIN_FEC_MDC, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX25_PIN_FEC_MDIO, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX25_PIN_FEC_RDATA1, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX25_PIN_FEC_TDATA1, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX25_PIN_POWER_FAIL, MUX_CONFIG_FUNC); /* PHY INT */

#define FEC_PAD_CTL1 (PAD_CTL_HYS_SCHMITZ | PAD_CTL_PUE_PUD | \
			PAD_CTL_PKE_ENABLE)
#define FEC_PAD_CTL2 (PAD_CTL_PUE_PUD)

	mxc_iomux_set_pad(MX25_PIN_FEC_TX_CLK, FEC_PAD_CTL1);
	mxc_iomux_set_pad(MX25_PIN_FEC_RX_DV, FEC_PAD_CTL1);
	mxc_iomux_set_pad(MX25_PIN_FEC_RDATA0, FEC_PAD_CTL1);
	mxc_iomux_set_pad(MX25_PIN_FEC_TDATA0, FEC_PAD_CTL2);
	mxc_iomux_set_pad(MX25_PIN_FEC_TX_EN, FEC_PAD_CTL2);
	mxc_iomux_set_pad(MX25_PIN_FEC_MDC, FEC_PAD_CTL2);
	mxc_iomux_set_pad(MX25_PIN_FEC_MDIO, FEC_PAD_CTL1 | PAD_CTL_22K_PU);
	mxc_iomux_set_pad(MX25_PIN_FEC_RDATA1, FEC_PAD_CTL1);
	mxc_iomux_set_pad(MX25_PIN_FEC_TDATA1, FEC_PAD_CTL2);
	mxc_iomux_set_pad(MX25_PIN_POWER_FAIL, FEC_PAD_CTL1);

	/*
	 * Set up the FEC_RESET_B and FEC_ENABLE GPIO pins.
	 * Assert FEC_RESET_B, then power up the PHY by asserting
	 * FEC_ENABLE, at the same time lifting FEC_RESET_B.
	 *
	 * FEC_RESET_B: gpio2[3] is ALT 5 mode of pin D12
	 * FEC_ENABLE_B: gpio4[8] is ALT 5 mode of pin A17
	 */
	mxc_request_iomux(MX25_PIN_A17, MUX_CONFIG_ALT5); /* FEC_EN */
	mxc_request_iomux(MX25_PIN_D12, MUX_CONFIG_ALT5); /* FEC_RESET_B */

	mxc_iomux_set_pad(MX25_PIN_A17, PAD_CTL_ODE_OpenDrain);
	mxc_iomux_set_pad(MX25_PIN_D12, 0);

	mxc_set_gpio_direction(MX25_PIN_A17, 0); /* FEC_EN */
	mxc_set_gpio_direction(MX25_PIN_D12, 0); /* FEC_RESET_B */

	/* drop PHY power */
	mxc_set_gpio_dataout(MX25_PIN_A17, 0);	/* FEC_EN */

	/* assert reset */
	mxc_set_gpio_dataout(MX25_PIN_D12, 0);	/* FEC_RESET_B */
	udelay(2);		/* spec says 1us min */

	/* turn on PHY power and lift reset */
	mxc_set_gpio_dataout(MX25_PIN_A17, 1);	/* FEC_EN */
	mxc_set_gpio_dataout(MX25_PIN_D12, 1);	/* FEC_RESET_B */

#define I2C_PAD_CTL (PAD_CTL_HYS_SCHMITZ | PAD_CTL_PKE_ENABLE | \
		PAD_CTL_PUE_PUD | PAD_CTL_100K_PU | PAD_CTL_ODE_OpenDrain)

	mxc_request_iomux(MX25_PIN_I2C1_CLK, MUX_CONFIG_SION);
	mxc_request_iomux(MX25_PIN_I2C1_DAT, MUX_CONFIG_SION);
	mxc_iomux_set_pad(MX25_PIN_I2C1_CLK, 0x1E8);
	mxc_iomux_set_pad(MX25_PIN_I2C1_DAT, 0x1E8);

#ifdef CONFIG_LCD
	lcdc_hw_init();
#endif

	gd->bd->bi_arch_number = MACH_TYPE_MX25_3DS;    /* board id for linux */
	gd->bd->bi_boot_params = 0x80000100;    /* address of boot parameters */

	return 0;

#undef FEC_PAD_CTL1
#undef FEC_PAD_CTL2
#undef I2C_PAD_CTL
}

#ifdef BOARD_LATE_INIT
int board_late_init(void)
{
	u8 reg[4];

	/* Turn PMIC On*/
	reg[0] = 0x09;
	i2c_write(0x54, 0x02, 1, reg, 1);

#ifdef CONFIG_IMX_SPI_CPLD
	mxc_cpld_spi_init();
#endif

#ifdef CONFIG_SPLASH_SCREEN
	if (!setup_splash_img())
		printf("Read splash screen failed!\n");
#endif

	return 0;
}
#endif


int checkboard(void)
{
	printf("Board: i.MX25 MAX PDK (3DS)\n");
	return 0;
}

int board_eth_init(bd_t *bis)
{
	int rc = -ENODEV;
#if defined(CONFIG_SMC911X)
	rc = smc911x_initialize(0, CONFIG_SMC911X_BASE);
#endif

	cpu_eth_init(bis);

	return rc;
}

