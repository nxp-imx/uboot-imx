/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <asm/io.h>
#include <linux/err.h>
#include <linux/bug.h>
#include <linux/delay.h>
#include <linux/log2.h>
#include <asm/unaligned.h>
#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/sys_proto.h>
#include <div64.h>
#include <video_bridge.h>
#include <panel.h>
#include <dsi_host.h>
#include <asm/arch/gpio.h>
#include <dm/device-internal.h>

#define MIPI_FIFO_TIMEOUT		250000 /* 250ms */

#define DRIVER_NAME "sec_mipi_dsim"

/* dsim registers */
#define DSIM_VERSION			0x00
#define DSIM_STATUS			0x04
#define DSIM_RGB_STATUS			0x08
#define DSIM_SWRST			0x0c
#define DSIM_CLKCTRL			0x10
#define DSIM_TIMEOUT			0x14
#define DSIM_CONFIG			0x18
#define DSIM_ESCMODE			0x1c
#define DSIM_MDRESOL			0x20
#define DSIM_MVPORCH			0x24
#define DSIM_MHPORCH			0x28
#define DSIM_MSYNC			0x2c
#define DSIM_SDRESOL			0x30
#define DSIM_INTSRC			0x34
#define DSIM_INTMSK			0x38

/* packet */
#define DSIM_PKTHDR			0x3c
#define DSIM_PAYLOAD			0x40
#define DSIM_RXFIFO			0x44
#define DSIM_FIFOTHLD			0x48
#define DSIM_FIFOCTRL			0x4c
#define DSIM_MEMACCHR		0x50
#define DSIM_MULTI_PKT			0x78

/* pll control */
#define DSIM_PLLCTRL_1G			0x90
#define DSIM_PLLCTRL			0x94
#define DSIM_PLLCTRL1			0x98
#define DSIM_PLLCTRL2			0x9c
#define DSIM_PLLTMR			0xa0

/* dphy */
#define DSIM_PHYTIMING			0xb4
#define DSIM_PHYTIMING1			0xb8
#define DSIM_PHYTIMING2			0xbc

/* reg bit manipulation */
#define REG_MASK(e, s) (((1 << ((e) - (s) + 1)) - 1) << (s))
#define REG_PUT(x, e, s) (((x) << (s)) & REG_MASK(e, s))
#define REG_GET(x, e, s) (((x) & REG_MASK(e, s)) >> (s))

/* register bit fields */
#define STATUS_PLLSTABLE		BIT(31)
#define STATUS_SWRSTRLS			BIT(20)
#define STATUS_TXREADYHSCLK		BIT(10)
#define STATUS_ULPSCLK			BIT(9)
#define STATUS_STOPSTATECLK		BIT(8)
#define STATUS_GET_ULPSDAT(x)		REG_GET(x,  7,  4)
#define STATUS_GET_STOPSTATEDAT(x)	REG_GET(x,  3,  0)

#define RGB_STATUS_CMDMODE_INSEL	BIT(31)
#define RGB_STATUS_GET_RGBSTATE(x)	REG_GET(x, 12,  0)

#define CLKCTRL_TXREQUESTHSCLK		BIT(31)
#define CLKCTRL_DPHY_SEL_1G		BIT(29)
#define CLKCTRL_DPHY_SEL_1P5G		(0x0 << 29)
#define CLKCTRL_ESCCLKEN		BIT(28)
#define CLKCTRL_PLLBYPASS		BIT(29)
#define CLKCTRL_BYTECLKSRC_DPHY_PLL	REG_PUT(0, 26, 25)
#define CLKCTRL_BYTECLKEN		BIT(24)
#define CLKCTRL_SET_LANEESCCLKEN(x)	REG_PUT(x, 23, 19)
#define CLKCTRL_SET_ESCPRESCALER(x)	REG_PUT(x, 15,  0)

#define TIMEOUT_SET_BTAOUT(x)		REG_PUT(x, 23, 16)
#define TIMEOUT_SET_LPDRTOUT(x)		REG_PUT(x, 15,  0)

#define CONFIG_NON_CONTINOUS_CLOCK_LANE	BIT(31)
#define CONFIG_CLKLANE_STOP_START	BIT(30)
#define CONFIG_MFLUSH_VS		BIT(29)
#define CONFIG_EOT_R03			BIT(28)
#define CONFIG_SYNCINFORM		BIT(27)
#define CONFIG_BURSTMODE		BIT(26)
#define CONFIG_VIDEOMODE		BIT(25)
#define CONFIG_AUTOMODE			BIT(24)
#define CONFIG_HSEDISABLEMODE		BIT(23)
#define CONFIG_HFPDISABLEMODE		BIT(22)
#define CONFIG_HBPDISABLEMODE		BIT(21)
#define CONFIG_HSADISABLEMODE		BIT(20)
#define CONFIG_SET_MAINVC(x)		REG_PUT(x, 19, 18)
#define CONFIG_SET_SUBVC(x)		REG_PUT(x, 17, 16)
#define CONFIG_SET_MAINPIXFORMAT(x)	REG_PUT(x, 14, 12)
#define CONFIG_SET_SUBPIXFORMAT(x)	REG_PUT(x, 10,  8)
#define CONFIG_SET_NUMOFDATLANE(x)	REG_PUT(x,  6,  5)
#define CONFIG_SET_LANEEN(x)		REG_PUT(x,  4,  0)

#define ESCMODE_SET_STOPSTATE_CNT(X)	REG_PUT(x, 31, 21)
#define ESCMODE_FORCESTOPSTATE		BIT(20)
#define ESCMODE_FORCEBTA		BIT(16)
#define ESCMODE_CMDLPDT			BIT(7)
#define ESCMODE_TXLPDT			BIT(6)
#define ESCMODE_TXTRIGGERRST		BIT(5)

#define MDRESOL_MAINSTANDBY		BIT(31)
#define MDRESOL_SET_MAINVRESOL(x)	REG_PUT(x, 27, 16)
#define MDRESOL_SET_MAINHRESOL(x)	REG_PUT(x, 11,  0)

#define MVPORCH_SET_CMDALLOW(x)		REG_PUT(x, 31, 28)
#define MVPORCH_SET_STABLEVFP(x)	REG_PUT(x, 26, 16)
#define MVPORCH_SET_MAINVBP(x)		REG_PUT(x, 10,  0)

#define MHPORCH_SET_MAINHFP(x)		REG_PUT(x, 31, 16)
#define MHPORCH_SET_MAINHBP(x)		REG_PUT(x, 15,  0)

#define MSYNC_SET_MAINVSA(x)		REG_PUT(x, 31, 22)
#define MSYNC_SET_MAINHSA(x)		REG_PUT(x, 15,  0)

#define INTSRC_PLLSTABLE		BIT(31)
#define INTSRC_SWRSTRELEASE		BIT(30)
#define INTSRC_SFRPLFIFOEMPTY		BIT(29)
#define INTSRC_SFRPHFIFOEMPTY		BIT(28)
#define INTSRC_FRAMEDONE		BIT(24)
#define INTSRC_LPDRTOUT			BIT(21)
#define INTSRC_TATOUT			BIT(20)
#define INTSRC_RXDATDONE		BIT(18)
#define INTSRC_MASK			(INTSRC_PLLSTABLE	|	\
					 INTSRC_SWRSTRELEASE	|	\
					 INTSRC_SFRPLFIFOEMPTY	|	\
					 INTSRC_SFRPHFIFOEMPTY	|	\
					 INTSRC_FRAMEDONE	|	\
					 INTSRC_LPDRTOUT	|	\
					 INTSRC_TATOUT		|	\
					 INTSRC_RXDATDONE)

#define INTMSK_MSKPLLSTABLE		BIT(31)
#define INTMSK_MSKSWRELEASE		BIT(30)
#define INTMSK_MSKSFRPLFIFOEMPTY	BIT(29)
#define INTMSK_MSKSFRPHFIFOEMPTY	BIT(28)
#define INTMSK_MSKFRAMEDONE		BIT(24)
#define INTMSK_MSKLPDRTOUT		BIT(21)
#define INTMSK_MSKTATOUT		BIT(20)
#define INTMSK_MSKRXDATDONE		BIT(18)

#define PKTHDR_SET_DATA1(x)		REG_PUT(x, 23, 16)
#define PKTHDR_GET_DATA1(x)		REG_GET(x, 23, 16)
#define PKTHDR_SET_DATA0(x)		REG_PUT(x, 15,  8)
#define PKTHDR_GET_DATA0(x)		REG_GET(x, 15,  8)
#define PKTHDR_GET_WC(x)		REG_GET(x, 23,  8)
#define PKTHDR_SET_DI(x)		REG_PUT(x,  7,  0)
#define PKTHDR_GET_DI(x)		REG_GET(x,  7,  0)
#define PKTHDR_SET_DT(x)		REG_PUT(x,  5,  0)
#define PKTHDR_GET_DT(x)		REG_GET(x,  5,  0)
#define PKTHDR_SET_VC(x)		REG_PUT(x,  7,  6)
#define PKTHDR_GET_VC(x)		REG_GET(x,  7,  6)

#define FIFOCTRL_FULLRX			BIT(25)
#define FIFOCTRL_EMPTYRX		BIT(24)
#define FIFOCTRL_FULLHSFR		BIT(23)
#define FIFOCTRL_EMPTYHSFR		BIT(22)
#define FIFOCTRL_FULLLSFR		BIT(21)
#define FIFOCTRL_EMPTYLSFR		BIT(20)
#define FIFOCTRL_FULLHMAIN		BIT(11)
#define FIFOCTRL_EMPTYHMAIN		BIT(10)
#define FIFOCTRL_FULLLMAIN		BIT(9)
#define FIFOCTRL_EMPTYLMAIN		BIT(8)
#define FIFOCTRL_NINITRX		BIT(4)
#define FIFOCTRL_NINITSFR		BIT(3)
#define FIFOCTRL_NINITI80		BIT(2)
#define FIFOCTRL_NINITSUB		BIT(1)
#define FIFOCTRL_NINITMAIN		BIT(0)

#define PLLCTRL_DPDNSWAP_CLK		BIT(25)
#define PLLCTRL_DPDNSWAP_DAT		BIT(24)
#define PLLCTRL_PLLEN			BIT(23)
#define PLLCTRL_SET_PMS(x)		REG_PUT(x, 19,  1)
#define PLLCTRL_SET_P(x)		REG_PUT(x, 18, 13)
#define PLLCTRL_SET_M(x)		REG_PUT(x, 12,  3)
#define PLLCTRL_SET_S(x)		REG_PUT(x,  2,  0)

#define PHYTIMING_SET_M_TLPXCTL(x)	REG_PUT(x, 15,  8)
#define PHYTIMING_SET_M_THSEXITCTL(x)	REG_PUT(x,  7,  0)

#define PHYTIMING1_SET_M_TCLKPRPRCTL(x)	 REG_PUT(x, 31, 24)
#define PHYTIMING1_SET_M_TCLKZEROCTL(x)	 REG_PUT(x, 23, 16)
#define PHYTIMING1_SET_M_TCLKPOSTCTL(x)	 REG_PUT(x, 15,  8)
#define PHYTIMING1_SET_M_TCLKTRAILCTL(x) REG_PUT(x,  7,  0)

#define PHYTIMING2_SET_M_THSPRPRCTL(x)	REG_PUT(x, 23, 16)
#define PHYTIMING2_SET_M_THSZEROCTL(x)	REG_PUT(x, 15,  8)
#define PHYTIMING2_SET_M_THSTRAILCTL(x)	REG_PUT(x,  7,  0)

#define dsim_read(dsim, reg)		readl(dsim->base + reg)
#define dsim_write(dsim, val, reg)	writel(val, dsim->base + reg)

#define MAX_MAIN_HRESOL		2047
#define MAX_MAIN_VRESOL		2047
#define MAX_SUB_HRESOL		1024
#define MAX_SUB_VRESOL		1024

/* in KHZ */
#define MAX_ESC_CLK_FREQ	20000

/* dsim all irqs index */
#define PLLSTABLE		1
#define SWRSTRELEASE		2
#define SFRPLFIFOEMPTY		3
#define SFRPHFIFOEMPTY		4
#define SYNCOVERRIDE		5
#define BUSTURNOVER		6
#define FRAMEDONE		7
#define LPDRTOUT		8
#define TATOUT			9
#define RXDATDONE		10
#define RXTE			11
#define RXACK			12
#define ERRRXECC		13
#define ERRRXCRC		14
#define ERRESC3			15
#define ERRESC2			16
#define ERRESC1			17
#define ERRESC0			18
#define ERRSYNC3		19
#define ERRSYNC2		20
#define ERRSYNC1		21
#define ERRSYNC0		22
#define ERRCONTROL3		23
#define ERRCONTROL2		24
#define ERRCONTROL1		25
#define ERRCONTROL0		26

/* Dispmix Control & GPR Registers */
#define DISPLAY_MIX_SFT_RSTN_CSR		0x00
#ifdef CONFIG_IMX8MN
#define MIPI_DSI_I_PRESETn_SFT_EN		BIT(0) | BIT(1)
#else
   #define MIPI_DSI_I_PRESETn_SFT_EN		BIT(5)
#endif
#define DISPLAY_MIX_CLK_EN_CSR			0x04

#ifdef CONFIG_IMX8MN
#define MIPI_DSI_PCLK_SFT_EN		 BIT(0)
#define MIPI_DSI_CLKREF_SFT_EN		 BIT(1)
#else
   #define MIPI_DSI_PCLK_SFT_EN			BIT(8)
   #define MIPI_DSI_CLKREF_SFT_EN		BIT(9)
#endif
#define GPR_MIPI_RESET_DIV			0x08
   /* Clock & Data lanes reset: Active Low */
   #define GPR_MIPI_S_RESETN			BIT(16)
   #define GPR_MIPI_M_RESETN			BIT(17)

#define	PS2KHZ(ps)	(1000000000UL / (ps))

#define MIPI_HFP_PKT_OVERHEAD	6
#define MIPI_HBP_PKT_OVERHEAD	6
#define MIPI_HSA_PKT_OVERHEAD	6


/* DSIM PLL configuration from spec:
 *
 * Fout(DDR) = (M * Fin) / (P * 2^S), so Fout / Fin = M / (P * 2^S)
 * Fin_pll   = Fin / P     (6 ~ 12 MHz)
 * S: [2:0], M: [12:3], P: [18:13], so
 * TODO: 'S' is in [0 ~ 3], 'M' is in, 'P' is in [1 ~ 33]
 *
 */

struct sec_mipi_dsim {
	void __iomem *base;

	/* kHz clocks */
	uint64_t pix_clk;
	uint64_t bit_clk;

	unsigned int lanes;
	unsigned int channel;			/* virtual channel */
	enum mipi_dsi_pixel_format format;
	unsigned long mode_flags;
	unsigned int pms;
	unsigned int p;
	unsigned int m;
	unsigned int s;

	 struct mipi_dsi_device *device;
	 uint32_t max_data_lanes;
	 uint64_t max_data_rate;

	 struct mipi_dsi_host dsi_host;

	 struct display_timing timings;
};

struct sec_mipi_dsim_range {
	uint32_t min;
	uint32_t max;
};

/* DPHY timings structure */
struct sec_mipi_dsim_dphy_timing {
	uint32_t bit_clk;	/* MHz */

	uint32_t clk_prepare;
	uint32_t clk_zero;
	uint32_t clk_post;
	uint32_t clk_trail;

	uint32_t hs_prepare;
	uint32_t hs_zero;
	uint32_t hs_trail;

	uint32_t lpx;
	uint32_t hs_exit;
};

#define DSIM_DPHY_TIMING(bclk, cpre, czero, cpost, ctrail,	\
			 hpre, hzero, htrail, lp, hexit)	\
	.bit_clk	= bclk,					\
	.clk_prepare	= cpre,					\
	.clk_zero	= czero,				\
	.clk_post	= cpost,				\
	.clk_trail	= ctrail,				\
	.hs_prepare	= hpre,					\
	.hs_zero	= hzero,				\
	.hs_trail	= htrail,				\
	.lpx		= lp,					\
	.hs_exit	= hexit

/* descending order based on 'bit_clk' value */
static const struct sec_mipi_dsim_dphy_timing dphy_timing[] = {
	{ DSIM_DPHY_TIMING(2100, 19, 91, 22, 19, 20, 35, 22, 15, 26), },
	{ DSIM_DPHY_TIMING(2090, 19, 91, 22, 19, 19, 35, 22, 15, 26), },
	{ DSIM_DPHY_TIMING(2080, 19, 91, 21, 18, 19, 35, 22, 15, 26), },
	{ DSIM_DPHY_TIMING(2070, 18, 90, 21, 18, 19, 35, 22, 15, 25), },
	{ DSIM_DPHY_TIMING(2060, 18, 90, 21, 18, 19, 34, 22, 15, 25), },
	{ DSIM_DPHY_TIMING(2050, 18, 89, 21, 18, 19, 34, 22, 15, 25), },
	{ DSIM_DPHY_TIMING(2040, 18, 89, 21, 18, 19, 34, 21, 15, 25), },
	{ DSIM_DPHY_TIMING(2030, 18, 88, 21, 18, 19, 34, 21, 15, 25), },
	{ DSIM_DPHY_TIMING(2020, 18, 88, 21, 18, 19, 34, 21, 15, 25), },
	{ DSIM_DPHY_TIMING(2010, 18, 87, 21, 18, 19, 34, 21, 15, 25), },
	{ DSIM_DPHY_TIMING(2000, 18, 87, 21, 18, 19, 33, 21, 15, 25), },
	{ DSIM_DPHY_TIMING(1990, 18, 87, 21, 18, 18, 33, 21, 14, 24), },
	{ DSIM_DPHY_TIMING(1980, 18, 86, 21, 18, 18, 33, 21, 14, 24), },
	{ DSIM_DPHY_TIMING(1970, 17, 86, 21, 17, 18, 33, 21, 14, 24), },
	{ DSIM_DPHY_TIMING(1960, 17, 85, 21, 17, 18, 33, 21, 14, 24), },
	{ DSIM_DPHY_TIMING(1950, 17, 85, 21, 17, 18, 32, 21, 14, 24), },
	{ DSIM_DPHY_TIMING(1940, 17, 84, 20, 17, 18, 32, 21, 14, 24), },
	{ DSIM_DPHY_TIMING(1930, 17, 84, 20, 17, 18, 32, 20, 14, 24), },
	{ DSIM_DPHY_TIMING(1920, 17, 84, 20, 17, 18, 32, 20, 14, 24), },
	{ DSIM_DPHY_TIMING(1910, 17, 83, 20, 17, 18, 32, 20, 14, 23), },
	{ DSIM_DPHY_TIMING(1900, 17, 83, 20, 17, 18, 32, 20, 14, 23), },
	{ DSIM_DPHY_TIMING(1890, 17, 82, 20, 17, 18, 31, 20, 14, 23), },
	{ DSIM_DPHY_TIMING(1880, 17, 82, 20, 17, 17, 31, 20, 14, 23), },
	{ DSIM_DPHY_TIMING(1870, 17, 81, 20, 17, 17, 31, 20, 14, 23), },
	{ DSIM_DPHY_TIMING(1860, 16, 81, 20, 17, 17, 31, 20, 13, 23), },
	{ DSIM_DPHY_TIMING(1850, 16, 80, 20, 16, 17, 31, 20, 13, 23), },
	{ DSIM_DPHY_TIMING(1840, 16, 80, 20, 16, 17, 30, 20, 13, 23), },
	{ DSIM_DPHY_TIMING(1830, 16, 80, 20, 16, 17, 30, 20, 13, 22), },
	{ DSIM_DPHY_TIMING(1820, 16, 79, 20, 16, 17, 30, 19, 13, 22), },
	{ DSIM_DPHY_TIMING(1810, 16, 79, 19, 16, 17, 30, 19, 13, 22), },
	{ DSIM_DPHY_TIMING(1800, 16, 78, 19, 16, 17, 30, 19, 13, 22), },
	{ DSIM_DPHY_TIMING(1790, 16, 78, 19, 16, 17, 30, 19, 13, 22), },
	{ DSIM_DPHY_TIMING(1780, 16, 77, 19, 16, 16, 29, 19, 13, 22), },
	{ DSIM_DPHY_TIMING(1770, 16, 77, 19, 16, 16, 29, 19, 13, 22), },
	{ DSIM_DPHY_TIMING(1760, 16, 77, 19, 16, 16, 29, 19, 13, 22), },
	{ DSIM_DPHY_TIMING(1750, 15, 76, 19, 16, 16, 29, 19, 13, 21), },
	{ DSIM_DPHY_TIMING(1740, 15, 76, 19, 15, 16, 29, 19, 13, 21), },
	{ DSIM_DPHY_TIMING(1730, 15, 75, 19, 15, 16, 28, 19, 12, 21), },
	{ DSIM_DPHY_TIMING(1720, 15, 75, 19, 15, 16, 28, 19, 12, 21), },
	{ DSIM_DPHY_TIMING(1710, 15, 74, 19, 15, 16, 28, 18, 12, 21), },
	{ DSIM_DPHY_TIMING(1700, 15, 74, 19, 15, 16, 28, 18, 12, 21), },
	{ DSIM_DPHY_TIMING(1690, 15, 73, 19, 15, 16, 28, 18, 12, 21), },
	{ DSIM_DPHY_TIMING(1680, 15, 73, 18, 15, 16, 28, 18, 12, 21), },
	{ DSIM_DPHY_TIMING(1670, 15, 73, 18, 15, 15, 27, 18, 12, 20), },
	{ DSIM_DPHY_TIMING(1660, 15, 72, 18, 15, 15, 27, 18, 12, 20), },
	{ DSIM_DPHY_TIMING(1650, 14, 72, 18, 15, 15, 27, 18, 12, 20), },
	{ DSIM_DPHY_TIMING(1640, 14, 71, 18, 15, 15, 27, 18, 12, 20), },
	{ DSIM_DPHY_TIMING(1630, 14, 71, 18, 15, 15, 27, 18, 12, 20), },
	{ DSIM_DPHY_TIMING(1620, 14, 70, 18, 14, 15, 26, 18, 12, 20), },
	{ DSIM_DPHY_TIMING(1610, 14, 70, 18, 14, 15, 26, 17, 12, 20), },
	{ DSIM_DPHY_TIMING(1600, 14, 70, 18, 14, 15, 26, 17, 12, 20), },
	{ DSIM_DPHY_TIMING(1590, 14, 69, 18, 14, 15, 26, 17, 11, 19), },
	{ DSIM_DPHY_TIMING(1580, 14, 69, 18, 14, 15, 26, 17, 11, 19), },
	{ DSIM_DPHY_TIMING(1570, 14, 68, 18, 14, 15, 26, 17, 11, 19), },
	{ DSIM_DPHY_TIMING(1560, 14, 68, 18, 14, 14, 25, 17, 11, 19), },
	{ DSIM_DPHY_TIMING(1550, 14, 67, 18, 14, 14, 25, 17, 11, 19), },
	{ DSIM_DPHY_TIMING(1540, 13, 67, 17, 14, 14, 25, 17, 11, 19), },
	{ DSIM_DPHY_TIMING(1530, 13, 66, 17, 14, 14, 25, 17, 11, 19), },
	{ DSIM_DPHY_TIMING(1520, 13, 66, 17, 14, 14, 25, 17, 11, 19), },
	{ DSIM_DPHY_TIMING(1510, 13, 66, 17, 13, 14, 24, 17, 11, 18), },
	{ DSIM_DPHY_TIMING(1500, 13, 65, 17, 13, 14, 24, 16, 11, 18), },
	{ DSIM_DPHY_TIMING(1490, 13, 65, 17, 13, 14, 24, 16, 11, 18), },
	{ DSIM_DPHY_TIMING(1480, 13, 64, 17, 13, 14, 24, 16, 11, 18), },
	{ DSIM_DPHY_TIMING(1470, 13, 64, 17, 13, 14, 24, 16, 11, 18), },
	{ DSIM_DPHY_TIMING(1460, 13, 63, 17, 13, 13, 24, 16, 10, 18), },
	{ DSIM_DPHY_TIMING(1450, 13, 63, 17, 13, 13, 23, 16, 10, 18), },
	{ DSIM_DPHY_TIMING(1440, 13, 63, 17, 13, 13, 23, 16, 10, 18), },
	{ DSIM_DPHY_TIMING(1430, 12, 62, 17, 13, 13, 23, 16, 10, 17), },
	{ DSIM_DPHY_TIMING(1420, 12, 62, 17, 13, 13, 23, 16, 10, 17), },
	{ DSIM_DPHY_TIMING(1410, 12, 61, 16, 13, 13, 23, 16, 10, 17), },
	{ DSIM_DPHY_TIMING(1400, 12, 61, 16, 13, 13, 23, 16, 10, 17), },
	{ DSIM_DPHY_TIMING(1390, 12, 60, 16, 12, 13, 22, 15, 10, 17), },
	{ DSIM_DPHY_TIMING(1380, 12, 60, 16, 12, 13, 22, 15, 10, 17), },
	{ DSIM_DPHY_TIMING(1370, 12, 59, 16, 12, 13, 22, 15, 10, 17), },
	{ DSIM_DPHY_TIMING(1360, 12, 59, 16, 12, 13, 22, 15, 10, 17), },
	{ DSIM_DPHY_TIMING(1350, 12, 59, 16, 12, 12, 22, 15, 10, 16), },
	{ DSIM_DPHY_TIMING(1340, 12, 58, 16, 12, 12, 21, 15, 10, 16), },
	{ DSIM_DPHY_TIMING(1330, 11, 58, 16, 12, 12, 21, 15,  9, 16), },
	{ DSIM_DPHY_TIMING(1320, 11, 57, 16, 12, 12, 21, 15,  9, 16), },
	{ DSIM_DPHY_TIMING(1310, 11, 57, 16, 12, 12, 21, 15,  9, 16), },
	{ DSIM_DPHY_TIMING(1300, 11, 56, 16, 12, 12, 21, 15,  9, 16), },
	{ DSIM_DPHY_TIMING(1290, 11, 56, 16, 12, 12, 21, 15,  9, 16), },
	{ DSIM_DPHY_TIMING(1280, 11, 56, 15, 11, 12, 20, 14,  9, 16), },
	{ DSIM_DPHY_TIMING(1270, 11, 55, 15, 11, 12, 20, 14,  9, 15), },
	{ DSIM_DPHY_TIMING(1260, 11, 55, 15, 11, 12, 20, 14,  9, 15), },
	{ DSIM_DPHY_TIMING(1250, 11, 54, 15, 11, 11, 20, 14,  9, 15), },
	{ DSIM_DPHY_TIMING(1240, 11, 54, 15, 11, 11, 20, 14,  9, 15), },
	{ DSIM_DPHY_TIMING(1230, 11, 53, 15, 11, 11, 19, 14,  9, 15), },
	{ DSIM_DPHY_TIMING(1220, 10, 53, 15, 11, 11, 19, 14,  9, 15), },
	{ DSIM_DPHY_TIMING(1210, 10, 52, 15, 11, 11, 19, 14,  9, 15), },
	{ DSIM_DPHY_TIMING(1200, 10, 52, 15, 11, 11, 19, 14,  9, 15), },
	{ DSIM_DPHY_TIMING(1190, 10, 52, 15, 11, 11, 19, 14,  8, 14), },
	{ DSIM_DPHY_TIMING(1180, 10, 51, 15, 11, 11, 19, 13,  8, 14), },
	{ DSIM_DPHY_TIMING(1170, 10, 51, 15, 10, 11, 18, 13,  8, 14), },
	{ DSIM_DPHY_TIMING(1160, 10, 50, 15, 10, 11, 18, 13,  8, 14), },
	{ DSIM_DPHY_TIMING(1150, 10, 50, 15, 10, 11, 18, 13,  8, 14), },
	{ DSIM_DPHY_TIMING(1140, 10, 49, 14, 10, 10, 18, 13,  8, 14), },
	{ DSIM_DPHY_TIMING(1130, 10, 49, 14, 10, 10, 18, 13,  8, 14), },
	{ DSIM_DPHY_TIMING(1120, 10, 49, 14, 10, 10, 17, 13,  8, 14), },
	{ DSIM_DPHY_TIMING(1110,  9, 48, 14, 10, 10, 17, 13,  8, 13), },
	{ DSIM_DPHY_TIMING(1100,  9, 48, 14, 10, 10, 17, 13,  8, 13), },
	{ DSIM_DPHY_TIMING(1090,  9, 47, 14, 10, 10, 17, 13,  8, 13), },
	{ DSIM_DPHY_TIMING(1080,  9, 47, 14, 10, 10, 17, 13,  8, 13), },
	{ DSIM_DPHY_TIMING(1070,  9, 46, 14, 10, 10, 17, 12,  8, 13), },
	{ DSIM_DPHY_TIMING(1060,  9, 46, 14, 10, 10, 16, 12,  7, 13), },
	{ DSIM_DPHY_TIMING(1050,  9, 45, 14,  9, 10, 16, 12,  7, 13), },
	{ DSIM_DPHY_TIMING(1040,  9, 45, 14,  9, 10, 16, 12,  7, 13), },
	{ DSIM_DPHY_TIMING(1030,  9, 45, 14,  9,  9, 16, 12,  7, 12), },
	{ DSIM_DPHY_TIMING(1020,  9, 44, 14,  9,  9, 16, 12,  7, 12), },
	{ DSIM_DPHY_TIMING(1010,  8, 44, 13,  9,  9, 15, 12,  7, 12), },
	{ DSIM_DPHY_TIMING(1000,  8, 43, 13,  9,  9, 15, 12,  7, 12), },
	{ DSIM_DPHY_TIMING( 990,  8, 43, 13,  9,  9, 15, 12,  7, 12), },
	{ DSIM_DPHY_TIMING( 980,  8, 42, 13,  9,  9, 15, 12,  7, 12), },
	{ DSIM_DPHY_TIMING( 970,  8, 42, 13,  9,  9, 15, 12,  7, 12), },
	{ DSIM_DPHY_TIMING( 960,  8, 42, 13,  9,  9, 15, 11,  7, 12), },
	{ DSIM_DPHY_TIMING( 950,  8, 41, 13,  9,  9, 14, 11,  7, 11), },
	{ DSIM_DPHY_TIMING( 940,  8, 41, 13,  8,  9, 14, 11,  7, 11), },
	{ DSIM_DPHY_TIMING( 930,  8, 40, 13,  8,  8, 14, 11,  6, 11), },
	{ DSIM_DPHY_TIMING( 920,  8, 40, 13,  8,  8, 14, 11,  6, 11), },
	{ DSIM_DPHY_TIMING( 910,  8, 39, 13,  8,  8, 14, 11,  6, 11), },
	{ DSIM_DPHY_TIMING( 900,  7, 39, 13,  8,  8, 13, 11,  6, 11), },
	{ DSIM_DPHY_TIMING( 890,  7, 38, 13,  8,  8, 13, 11,  6, 11), },
	{ DSIM_DPHY_TIMING( 880,  7, 38, 12,  8,  8, 13, 11,  6, 11), },
	{ DSIM_DPHY_TIMING( 870,  7, 38, 12,  8,  8, 13, 11,  6, 10), },
	{ DSIM_DPHY_TIMING( 860,  7, 37, 12,  8,  8, 13, 11,  6, 10), },
	{ DSIM_DPHY_TIMING( 850,  7, 37, 12,  8,  8, 13, 10,  6, 10), },
	{ DSIM_DPHY_TIMING( 840,  7, 36, 12,  8,  8, 12, 10,  6, 10), },
	{ DSIM_DPHY_TIMING( 830,  7, 36, 12,  8,  8, 12, 10,  6, 10), },
	{ DSIM_DPHY_TIMING( 820,  7, 35, 12,  7,  7, 12, 10,  6, 10), },
	{ DSIM_DPHY_TIMING( 810,  7, 35, 12,  7,  7, 12, 10,  6, 10), },
	{ DSIM_DPHY_TIMING( 800,  7, 35, 12,  7,  7, 12, 10,  6, 10), },
	{ DSIM_DPHY_TIMING( 790,  6, 34, 12,  7,  7, 11, 10,  5,  9), },
	{ DSIM_DPHY_TIMING( 780,  6, 34, 12,  7,  7, 11, 10,  5,  9), },
	{ DSIM_DPHY_TIMING( 770,  6, 33, 12,  7,  7, 11, 10,  5,  9), },
	{ DSIM_DPHY_TIMING( 760,  6, 33, 12,  7,  7, 11, 10,  5,  9), },
	{ DSIM_DPHY_TIMING( 750,  6, 32, 12,  7,  7, 11,  9,  5,  9), },
	{ DSIM_DPHY_TIMING( 740,  6, 32, 11,  7,  7, 11,  9,  5,  9), },
	{ DSIM_DPHY_TIMING( 730,  6, 31, 11,  7,  7, 10,  9,  5,  9), },
	{ DSIM_DPHY_TIMING( 720,  6, 31, 11,  7,  6, 10,  9,  5,  9), },
	{ DSIM_DPHY_TIMING( 710,  6, 31, 11,  6,  6, 10,  9,  5,  8), },
	{ DSIM_DPHY_TIMING( 700,  6, 30, 11,  6,  6, 10,  9,  5,  8), },
	{ DSIM_DPHY_TIMING( 690,  5, 30, 11,  6,  6, 10,  9,  5,  8), },
	{ DSIM_DPHY_TIMING( 680,  5, 29, 11,  6,  6,  9,  9,  5,  8), },
	{ DSIM_DPHY_TIMING( 670,  5, 29, 11,  6,  6,  9,  9,  5,  8), },
	{ DSIM_DPHY_TIMING( 660,  5, 28, 11,  6,  6,  9,  9,  4,  8), },
	{ DSIM_DPHY_TIMING( 650,  5, 28, 11,  6,  6,  9,  9,  4,  8), },
	{ DSIM_DPHY_TIMING( 640,  5, 28, 11,  6,  6,  9,  8,  4,  8), },
	{ DSIM_DPHY_TIMING( 630,  5, 27, 11,  6,  6,  9,  8,  4,  7), },
	{ DSIM_DPHY_TIMING( 620,  5, 27, 11,  6,  6,  8,  8,  4,  7), },
	{ DSIM_DPHY_TIMING( 610,  5, 26, 10,  6,  5,  8,  8,  4,  7), },
	{ DSIM_DPHY_TIMING( 600,  5, 26, 10,  6,  5,  8,  8,  4,  7), },
	{ DSIM_DPHY_TIMING( 590,  5, 25, 10,  5,  5,  8,  8,  4,  7), },
	{ DSIM_DPHY_TIMING( 580,  4, 25, 10,  5,  5,  8,  8,  4,  7), },
	{ DSIM_DPHY_TIMING( 570,  4, 24, 10,  5,  5,  7,  8,  4,  7), },
	{ DSIM_DPHY_TIMING( 560,  4, 24, 10,  5,  5,  7,  8,  4,  7), },
	{ DSIM_DPHY_TIMING( 550,  4, 24, 10,  5,  5,  7,  8,  4,  6), },
	{ DSIM_DPHY_TIMING( 540,  4, 23, 10,  5,  5,  7,  8,  4,  6), },
	{ DSIM_DPHY_TIMING( 530,  4, 23, 10,  5,  5,  7,  7,  3,  6), },
	{ DSIM_DPHY_TIMING( 520,  4, 22, 10,  5,  5,  7,  7,  3,  6), },
	{ DSIM_DPHY_TIMING( 510,  4, 22, 10,  5,  5,  6,  7,  3,  6), },
	{ DSIM_DPHY_TIMING( 500,  4, 21, 10,  5,  4,  6,  7,  3,  6), },
	{ DSIM_DPHY_TIMING( 490,  4, 21, 10,  5,  4,  6,  7,  3,  6), },
	{ DSIM_DPHY_TIMING( 480,  4, 21,  9,  4,  4,  6,  7,  3,  6), },
	{ DSIM_DPHY_TIMING( 470,  3, 20,  9,  4,  4,  6,  7,  3,  5), },
	{ DSIM_DPHY_TIMING( 460,  3, 20,  9,  4,  4,  5,  7,  3,  5), },
	{ DSIM_DPHY_TIMING( 450,  3, 19,  9,  4,  4,  5,  7,  3,  5), },
	{ DSIM_DPHY_TIMING( 440,  3, 19,  9,  4,  4,  5,  7,  3,  5), },
	{ DSIM_DPHY_TIMING( 430,  3, 18,  9,  4,  4,  5,  7,  3,  5), },
	{ DSIM_DPHY_TIMING( 420,  3, 18,  9,  4,  4,  5,  6,  3,  5), },
	{ DSIM_DPHY_TIMING( 410,  3, 17,  9,  4,  4,  5,  6,  3,  5), },
	{ DSIM_DPHY_TIMING( 400,  3, 17,  9,  4,  3,  4,  6,  3,  5), },
	{ DSIM_DPHY_TIMING( 390,  3, 17,  9,  4,  3,  4,  6,  2,  4), },
	{ DSIM_DPHY_TIMING( 380,  3, 16,  9,  4,  3,  4,  6,  2,  4), },
	{ DSIM_DPHY_TIMING( 370,  2, 16,  9,  3,  3,  4,  6,  2,  4), },
	{ DSIM_DPHY_TIMING( 360,  2, 15,  9,  3,  3,  4,  6,  2,  4), },
	{ DSIM_DPHY_TIMING( 350,  2, 15,  9,  3,  3,  3,  6,  2,  4), },
	{ DSIM_DPHY_TIMING( 340,  2, 14,  8,  3,  3,  3,  6,  2,  4), },
	{ DSIM_DPHY_TIMING( 330,  2, 14,  8,  3,  3,  3,  6,  2,  4), },
	{ DSIM_DPHY_TIMING( 320,  2, 14,  8,  3,  3,  3,  5,  2,  4), },
	{ DSIM_DPHY_TIMING( 310,  2, 13,  8,  3,  3,  3,  5,  2,  3), },
	{ DSIM_DPHY_TIMING( 300,  2, 13,  8,  3,  3,  3,  5,  2,  3), },
	{ DSIM_DPHY_TIMING( 290,  2, 12,  8,  3,  2,  2,  5,  2,  3), },
	{ DSIM_DPHY_TIMING( 280,  2, 12,  8,  3,  2,  2,  5,  2,  3), },
	{ DSIM_DPHY_TIMING( 270,  2, 11,  8,  3,  2,  2,  5,  2,  3), },
	{ DSIM_DPHY_TIMING( 260,  1, 11,  8,  3,  2,  2,  5,  1,  3), },
	{ DSIM_DPHY_TIMING( 250,  1, 10,  8,  2,  2,  2,  5,  1,  3), },
	{ DSIM_DPHY_TIMING( 240,  1,  9,  8,  2,  2,  1,  5,  1,  3), },
	{ DSIM_DPHY_TIMING( 230,  1,  8,  8,  2,  2,  1,  5,  1,  2), },
	{ DSIM_DPHY_TIMING( 220,  1,  8,  8,  2,  2,  1,  5,  1,  2), },
	{ DSIM_DPHY_TIMING( 210,  1,  7,  7,  2,  2,  1,  4,  1,  2), },
	{ DSIM_DPHY_TIMING( 200,  1,  7,  7,  2,  2,  1,  4,  1,  2), },
	{ DSIM_DPHY_TIMING( 190,  1,  7,  7,  2,  1,  1,  4,  1,  2), },
	{ DSIM_DPHY_TIMING( 180,  1,  6,  7,  2,  1,  0,  4,  1,  2), },
	{ DSIM_DPHY_TIMING( 170,  1,  6,  7,  2,  1,  0,  4,  1,  2), },
	{ DSIM_DPHY_TIMING( 160,  1,  6,  7,  2,  1,  0,  4,  1,  2), },
	{ DSIM_DPHY_TIMING( 150,  0,  5,  7,  2,  1,  0,  4,  1,  1), },
	{ DSIM_DPHY_TIMING( 140,  0,  5,  7,  1,  1,  0,  4,  1,  1), },
	{ DSIM_DPHY_TIMING( 130,  0,  4,  7,  1,  1,  0,  4,  0,  1), },
	{ DSIM_DPHY_TIMING( 120,  0,  4,  7,  1,  1,  0,  4,  0,  1), },
	{ DSIM_DPHY_TIMING( 110,  0,  3,  7,  1,  0,  0,  4,  0,  1), },
	{ DSIM_DPHY_TIMING( 100,  0,  3,  7,  1,  0,  0,  3,  0,  1), },
	{ DSIM_DPHY_TIMING(  90,  0,  2,  7,  1,  0,  0,  3,  0,  1), },
	{ DSIM_DPHY_TIMING(  80,  0,  2,  6,  1,  0,  0,  3,  0,  1), },
};

static inline int dphy_timing_default_cmp(const void *key, const void *elt)
{
	const struct sec_mipi_dsim_dphy_timing *_key = key;
	const struct sec_mipi_dsim_dphy_timing *_elt = elt;

	/* find an element whose 'bit_clk' is equal to the
	 * the key's 'bit_clk' value or, the difference
	 * between them is less than 5.
	 */
	if (abs((int)(_elt->bit_clk - _key->bit_clk)) <= 5)
		return 0;

	if (_key->bit_clk < _elt->bit_clk)
		/* search bottom half */
		return 1;
	else
		/* search top half */
		return -1;
}

static const struct sec_mipi_dsim_dphy_timing *dphy_timing_search(int start, int size,
	struct sec_mipi_dsim_dphy_timing *key)
{
	int ret;

	if (size == 0)
		return NULL;

	ret = dphy_timing_default_cmp(key, &dphy_timing[start + (size >> 1)]);
	if (ret == -1)
		return dphy_timing_search(start, size >> 1, key);
	else if (ret == 1)
		return dphy_timing_search(start + (size >> 1) + 1, size - 1 - (size >> 1), key);
	else
		return &dphy_timing[start + (size >> 1)];
}

static int sec_mipi_dsim_wait_for_pkt_done(struct sec_mipi_dsim *dsim, unsigned long timeout)
{
	uint32_t intsrc;

	do {
		intsrc = dsim_read(dsim, DSIM_INTSRC);
		if (intsrc & INTSRC_SFRPLFIFOEMPTY) {
			dsim_write(dsim, INTSRC_SFRPLFIFOEMPTY, DSIM_INTSRC);
			return 0;
		}

		udelay(1);
	} while (--timeout);

	return -ETIMEDOUT;
}

static int sec_mipi_dsim_wait_for_hdr_done(struct sec_mipi_dsim *dsim, unsigned long timeout)
{
	uint32_t intsrc;

	do {
		intsrc = dsim_read(dsim, DSIM_INTSRC);
		if (intsrc & INTSRC_SFRPHFIFOEMPTY) {
			dsim_write(dsim, INTSRC_SFRPHFIFOEMPTY, DSIM_INTSRC);
			return 0;
		}

		udelay(1);
	} while (--timeout);

	return -ETIMEDOUT;
}


static int sec_mipi_dsim_wait_for_rx_done(struct sec_mipi_dsim *dsim, unsigned long timeout)
{
	uint32_t intsrc;

	do {
		intsrc = dsim_read(dsim, DSIM_INTSRC);
		if (intsrc & INTSRC_RXDATDONE) {
			dsim_write(dsim, INTSRC_RXDATDONE, DSIM_INTSRC);
			return 0;
		}

		udelay(1);
	} while (--timeout);

	return -ETIMEDOUT;
}

static int sec_mipi_dsim_wait_pll_stable(struct sec_mipi_dsim *dsim)
{
	uint32_t status;
	ulong start;

	start = get_timer(0);	/* Get current timestamp */

	do {
		status = dsim_read(dsim, DSIM_STATUS);
		if (status & STATUS_PLLSTABLE)
			return 0;
	} while (get_timer(0) < (start + 100)); /* Wait 100ms */

	return -ETIMEDOUT;
}

static int sec_mipi_dsim_calc_pmsk(struct sec_mipi_dsim *dsim)
{
	uint32_t p, m, s;
	uint32_t best_p = 0, best_m = 0, best_s = 0;
	uint32_t fin, fout;
	uint32_t s_pow_2, raw_s;
	uint64_t mfin, pfvco, pfout, psfout;
	uint32_t delta, best_delta = ~0U;
	struct sec_mipi_dsim_range prange = { .min = 1,       .max = 63,  };
	struct sec_mipi_dsim_range mrange = { .min = 64,      .max = 1023,  };
	struct sec_mipi_dsim_range srange = { .min = 0,       .max = 5,  };
	struct sec_mipi_dsim_range krange = { .min = 0,       .max = 32768,  };
	struct sec_mipi_dsim_range fvco_range  = { .min = 1050000,    .max = 2100000,  };
	struct sec_mipi_dsim_range fpref_range = { .min = 2000,       .max = 30000,  };
	struct sec_mipi_dsim_range pr_new = prange;
	struct sec_mipi_dsim_range sr_new = srange;

	fout = dsim->bit_clk;
	fin  = get_dsi_phy_ref_clk();
	fin = DIV_ROUND_UP_ULL(fin, 1000); /* Change to Khz */
	if (fin == 0) {
		printf("Error: DSI PHY reference clock is disabled\n");
		return -EINVAL;
	}

	/* TODO: ignore 'k' for PMS calculation,
	 * only use 'p', 'm' and 's' to generate
	 * the requested PLL output clock.
	 */
	krange.min = 0;
	krange.max = 0;

	/* narrow 'p' range via 'Fpref' limitation:
	 * Fpref : [2MHz ~ 30MHz] (Fpref = Fin / p)
	 */
	prange.min = max(prange.min, DIV_ROUND_UP(fin, fpref_range.max));
	prange.max = min(prange.max, fin / fpref_range.min);

	/* narrow 'm' range via 'Fvco' limitation:
	 * Fvco: [1050MHz ~ 2100MHz] (Fvco = ((m + k / 65536) * Fin) / p)
	 * So, m = Fvco * p / Fin and Fvco > Fin;
	 */
	pfvco = (uint64_t)fvco_range.min * prange.min;
	mrange.min = max_t(uint32_t, mrange.min,
			    DIV_ROUND_UP_ULL(pfvco, fin));
	pfvco = (uint64_t)fvco_range.max * prange.max;
	mrange.max = min_t(uint32_t, mrange.max,
			    DIV_ROUND_UP_ULL(pfvco, fin));

	debug("p: min = %u, max = %u, "
		     "m: min = %u, max = %u, "
		     "s: min = %u, max = %u\n",
		prange.min, prange.max, mrange.min,
		mrange.max, srange.min, srange.max);

	/* first determine 'm', then can determine 'p', last determine 's' */
	for (m = mrange.min; m <= mrange.max; m++) {
		/* p = m * Fin / Fvco */
		mfin = (uint64_t)m * fin;
		pr_new.min = max_t(uint32_t, prange.min,
				   DIV_ROUND_UP_ULL(mfin, fvco_range.max));
		pr_new.max = min_t(uint32_t, prange.max,
				   (mfin / fvco_range.min));

		if (pr_new.max < pr_new.min || pr_new.min < prange.min)
			continue;

		for (p = pr_new.min; p <= pr_new.max; p++) {
			/* s = order_pow_of_two((m * Fin) / (p * Fout)) */
			pfout = (uint64_t)p * fout;
			raw_s = DIV_ROUND_CLOSEST_ULL(mfin, pfout);

			s_pow_2 = rounddown_pow_of_two(raw_s);
			sr_new.min = max_t(uint32_t, srange.min,
					   order_base_2(s_pow_2));

			s_pow_2 = roundup_pow_of_two(DIV_ROUND_CLOSEST_ULL(mfin, pfout));
			sr_new.max = min_t(uint32_t, srange.max,
					   order_base_2(s_pow_2));

			if (sr_new.max < sr_new.min || sr_new.min < srange.min)
				continue;

			for (s = sr_new.min; s <= sr_new.max; s++) {
				/* fout = m * Fin / (p * 2^s) */
				psfout = pfout * (1 << s);
				delta = abs(psfout - mfin);
				if (delta < best_delta) {
					best_p = p;
					best_m = m;
					best_s = s;
					best_delta = delta;
				}
			}
		}
	}

	if (best_delta == ~0U) {
		return -EINVAL;
	}

	dsim->p = best_p;
	dsim->m = best_m;
	dsim->s = best_s;

	debug("fout = %u, fin = %u, m = %u, "
		     "p = %u, s = %u, best_delta = %u\n",
		fout, fin, dsim->m, dsim->p, dsim->s, best_delta);

	return 0;
}


static int sec_mipi_dsim_config_pll(struct sec_mipi_dsim *dsim)
{
	int ret;
	uint32_t pllctrl = 0, status, data_lanes_en, stop;

	dsim_write(dsim, 0x8000, DSIM_PLLTMR);

	/* TODO: config dp/dn swap if requires */

	pllctrl |= PLLCTRL_SET_PMS(dsim->pms) | PLLCTRL_PLLEN;
	dsim_write(dsim, pllctrl, DSIM_PLLCTRL);

	ret = sec_mipi_dsim_wait_pll_stable(dsim);
	if (ret) {
		printf("wait for pll stable time out\n");
		return ret;
	}

	/* wait for clk & data lanes to go to stop state */
	mdelay(1);

	data_lanes_en = (0x1 << dsim->lanes) - 1;
	status = dsim_read(dsim, DSIM_STATUS);
	if (!(status & STATUS_STOPSTATECLK)) {
		printf("clock is not in stop state\n");
		return -EBUSY;
	}

	stop = STATUS_GET_STOPSTATEDAT(status);
	if ((stop & data_lanes_en) != data_lanes_en) {
		printf("one or more data lanes is not in stop state\n");
		return -EBUSY;
	}

	return 0;
}

static void sec_mipi_dsim_set_main_mode(struct sec_mipi_dsim *dsim)
{
	uint32_t bpp, hfp_wc, hbp_wc, hsa_wc, wc;
	uint32_t mdresol = 0, mvporch = 0, mhporch = 0, msync = 0;
	struct display_timing *timings = &dsim->timings;

	mdresol |= MDRESOL_SET_MAINVRESOL(timings->vactive.typ) |
		   MDRESOL_SET_MAINHRESOL(timings->hactive.typ);
	dsim_write(dsim, mdresol, DSIM_MDRESOL);

	mvporch |= MVPORCH_SET_MAINVBP(timings->vback_porch.typ)    |
		   MVPORCH_SET_STABLEVFP(timings->vfront_porch.typ) |
		   MVPORCH_SET_CMDALLOW(0x0);
	dsim_write(dsim, mvporch, DSIM_MVPORCH);

	bpp = mipi_dsi_pixel_format_to_bpp(dsim->format);


	wc = DIV_ROUND_UP(timings->hfront_porch.typ* (bpp >> 3),
		dsim->lanes);
	hfp_wc = wc > MIPI_HFP_PKT_OVERHEAD ?
		wc - MIPI_HFP_PKT_OVERHEAD : timings->hfront_porch.typ;
	wc = DIV_ROUND_UP(timings->hback_porch.typ * (bpp >> 3),
		dsim->lanes);
	hbp_wc = wc > MIPI_HBP_PKT_OVERHEAD ?
		wc - MIPI_HBP_PKT_OVERHEAD : timings->hback_porch.typ;

	mhporch |= MHPORCH_SET_MAINHFP(hfp_wc) |
		   MHPORCH_SET_MAINHBP(hbp_wc);

	dsim_write(dsim, mhporch, DSIM_MHPORCH);

	wc = DIV_ROUND_UP(timings->hsync_len.typ * (bpp >> 3),
		dsim->lanes);
	hsa_wc = wc > MIPI_HSA_PKT_OVERHEAD ?
		wc - MIPI_HSA_PKT_OVERHEAD : timings->hsync_len.typ;

	msync |= MSYNC_SET_MAINVSA(timings->vsync_len.typ) |
		 MSYNC_SET_MAINHSA(hsa_wc);

	debug("hfp_wc %u hbp_wc %u hsa_wc %u\n", hfp_wc, hbp_wc, hsa_wc);

	dsim_write(dsim, msync, DSIM_MSYNC);
}

static void sec_mipi_dsim_config_dpi(struct sec_mipi_dsim *dsim)
{
	uint32_t config = 0, rgb_status = 0, data_lanes_en;

	if (dsim->mode_flags & MIPI_DSI_MODE_VIDEO)
		rgb_status &= ~RGB_STATUS_CMDMODE_INSEL;
	else
		rgb_status |= RGB_STATUS_CMDMODE_INSEL;

	dsim_write(dsim, rgb_status, DSIM_RGB_STATUS);

	if (dsim->mode_flags & MIPI_DSI_CLOCK_NON_CONTINUOUS)
		config |= CONFIG_CLKLANE_STOP_START;

	if (dsim->mode_flags & MIPI_DSI_MODE_VSYNC_FLUSH)
		config |= CONFIG_MFLUSH_VS;

	/* disable EoT packets in HS mode */
	if (dsim->mode_flags & MIPI_DSI_MODE_EOT_PACKET)
		config |= CONFIG_EOT_R03;

	if (dsim->mode_flags & MIPI_DSI_MODE_VIDEO) {
		config |= CONFIG_VIDEOMODE;

		if (dsim->mode_flags & MIPI_DSI_MODE_VIDEO_BURST)
			config |= CONFIG_BURSTMODE;

		else if (dsim->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE)
			config |= CONFIG_SYNCINFORM;

		if (dsim->mode_flags & MIPI_DSI_MODE_VIDEO_AUTO_VERT)
			config |= CONFIG_AUTOMODE;

		if (dsim->mode_flags & MIPI_DSI_MODE_VIDEO_HSE)
			config |= CONFIG_HSEDISABLEMODE;

		if (dsim->mode_flags & MIPI_DSI_MODE_VIDEO_HFP)
			config |= CONFIG_HFPDISABLEMODE;

		if (dsim->mode_flags & MIPI_DSI_MODE_VIDEO_HBP)
			config |= CONFIG_HBPDISABLEMODE;

		if (dsim->mode_flags & MIPI_DSI_MODE_VIDEO_HSA)
			config |= CONFIG_HSADISABLEMODE;
	}

	config |= CONFIG_SET_MAINVC(dsim->channel);

	if (dsim->mode_flags & MIPI_DSI_MODE_VIDEO) {
		switch (dsim->format) {
		case MIPI_DSI_FMT_RGB565:
			config |= CONFIG_SET_MAINPIXFORMAT(0x4);
			break;
		case MIPI_DSI_FMT_RGB666_PACKED:
			config |= CONFIG_SET_MAINPIXFORMAT(0x5);
			break;
		case MIPI_DSI_FMT_RGB666:
			config |= CONFIG_SET_MAINPIXFORMAT(0x6);
			break;
		case MIPI_DSI_FMT_RGB888:
			config |= CONFIG_SET_MAINPIXFORMAT(0x7);
			break;
		default:
			config |= CONFIG_SET_MAINPIXFORMAT(0x7);
			break;
		}
	}

	/* config data lanes number and enable lanes */
	data_lanes_en = (0x1 << dsim->lanes) - 1;
	config |= CONFIG_SET_NUMOFDATLANE(dsim->lanes - 1);
	config |= CONFIG_SET_LANEEN(0x1 | data_lanes_en << 1);

	debug("DSIM config 0x%x\n", config);

	dsim_write(dsim, config, DSIM_CONFIG);
}

static void sec_mipi_dsim_config_cmd_lpm(struct sec_mipi_dsim *dsim,
					 bool enable)
{
	uint32_t escmode;

	escmode = dsim_read(dsim, DSIM_ESCMODE);

	if (enable)
		escmode |= ESCMODE_CMDLPDT;
	else
		escmode &= ~ESCMODE_CMDLPDT;

	dsim_write(dsim, escmode, DSIM_ESCMODE);
}

static void sec_mipi_dsim_config_dphy(struct sec_mipi_dsim *dsim)
{
	uint32_t phytiming = 0, phytiming1 = 0, phytiming2 = 0, timeout = 0;
	struct sec_mipi_dsim_dphy_timing key = { 0 };
	const struct sec_mipi_dsim_dphy_timing *match = NULL;

	key.bit_clk = DIV_ROUND_CLOSEST_ULL(dsim->bit_clk, 1000);

	match = dphy_timing_search(0, ARRAY_SIZE(dphy_timing), &key);
	if (!match) {
		printf("Fail to find DPHY timing for %uMhz\n", key.bit_clk);
		return;
	}

	phytiming  |= PHYTIMING_SET_M_TLPXCTL(match->lpx)	|
		      PHYTIMING_SET_M_THSEXITCTL(match->hs_exit);
	dsim_write(dsim, phytiming, DSIM_PHYTIMING);

	phytiming1 |= PHYTIMING1_SET_M_TCLKPRPRCTL(match->clk_prepare)	|
		      PHYTIMING1_SET_M_TCLKZEROCTL(match->clk_zero)	|
		      PHYTIMING1_SET_M_TCLKPOSTCTL(match->clk_post)	|
		      PHYTIMING1_SET_M_TCLKTRAILCTL(match->clk_trail);
	dsim_write(dsim, phytiming1, DSIM_PHYTIMING1);

	phytiming2 |= PHYTIMING2_SET_M_THSPRPRCTL(match->hs_prepare)	|
		      PHYTIMING2_SET_M_THSZEROCTL(match->hs_zero)	|
		      PHYTIMING2_SET_M_THSTRAILCTL(match->hs_trail);
	dsim_write(dsim, phytiming2, DSIM_PHYTIMING2);

	timeout |= TIMEOUT_SET_BTAOUT(0xff)	|
		   TIMEOUT_SET_LPDRTOUT(0xff);
	dsim_write(dsim, timeout, DSIM_TIMEOUT);
}

static void sec_mipi_dsim_write_pl_to_sfr_fifo(struct sec_mipi_dsim *dsim,
					       const void *payload,
					       size_t length)
{
	uint32_t pl_data;

	if (!length)
		return;

	while (length >= 4) {
		pl_data = get_unaligned_le32(payload);
		dsim_write(dsim, pl_data, DSIM_PAYLOAD);
		payload += 4;
		length -= 4;
	}

	pl_data = 0;
	switch (length) {
	case 3:
		pl_data |= ((u8 *)payload)[2] << 16;
	case 2:
		pl_data |= ((u8 *)payload)[1] << 8;
	case 1:
		pl_data |= ((u8 *)payload)[0];
		dsim_write(dsim, pl_data, DSIM_PAYLOAD);
		break;
	}
}

static void sec_mipi_dsim_write_ph_to_sfr_fifo(struct sec_mipi_dsim *dsim,
					       void *header,
					       bool use_lpm)
{
	uint32_t pkthdr;

	pkthdr = PKTHDR_SET_DATA1(((u8 *)header)[2])	| /* WC MSB  */
		 PKTHDR_SET_DATA0(((u8 *)header)[1])	| /* WC LSB  */
		 PKTHDR_SET_DI(((u8 *)header)[0]);	  /* Data ID */

	dsim_write(dsim, pkthdr, DSIM_PKTHDR);
}

static int sec_mipi_dsim_read_pl_from_sfr_fifo(struct sec_mipi_dsim *dsim,
					       void *payload,
					       size_t length)
{
	uint8_t data_type;
	uint16_t word_count = 0;
	uint32_t fifoctrl, ph, pl;

	fifoctrl = dsim_read(dsim, DSIM_FIFOCTRL);

	if (WARN_ON(fifoctrl & FIFOCTRL_EMPTYRX))
		return -EINVAL;

	ph = dsim_read(dsim, DSIM_RXFIFO);
	data_type = PKTHDR_GET_DT(ph);
	switch (data_type) {
	case MIPI_DSI_RX_ACKNOWLEDGE_AND_ERROR_REPORT:
		dev_err(dsim->device->dev, "peripheral report error: (0-7)%x, (8-15)%x\n",
			PKTHDR_GET_DATA0(ph), PKTHDR_GET_DATA1(ph));
		return -EPROTO;
	case MIPI_DSI_RX_DCS_SHORT_READ_RESPONSE_2BYTE:
	case MIPI_DSI_RX_GENERIC_SHORT_READ_RESPONSE_2BYTE:
		if (!WARN_ON(length < 2)) {
			((u8 *)payload)[1] = PKTHDR_GET_DATA1(ph);
			word_count++;
		}
		/* fall through */
	case MIPI_DSI_RX_DCS_SHORT_READ_RESPONSE_1BYTE:
	case MIPI_DSI_RX_GENERIC_SHORT_READ_RESPONSE_1BYTE:
		((u8 *)payload)[0] = PKTHDR_GET_DATA0(ph);
		word_count++;
		length = word_count;
		break;
	case MIPI_DSI_RX_DCS_LONG_READ_RESPONSE:
	case MIPI_DSI_RX_GENERIC_LONG_READ_RESPONSE:
		word_count = PKTHDR_GET_WC(ph);
		if (word_count > length) {
			dev_err(dsim->device->dev, "invalid receive buffer length\n");
			return -EINVAL;
		}

		length = word_count;

		while (word_count >= 4) {
			pl = dsim_read(dsim, DSIM_RXFIFO);
			((u8 *)payload)[0] = pl & 0xff;
			((u8 *)payload)[1] = (pl >> 8)  & 0xff;
			((u8 *)payload)[2] = (pl >> 16) & 0xff;
			((u8 *)payload)[3] = (pl >> 24) & 0xff;
			payload += 4;
			word_count -= 4;
		}

		if (word_count > 0) {
			pl = dsim_read(dsim, DSIM_RXFIFO);

			switch (word_count) {
			case 3:
				((u8 *)payload)[2] = (pl >> 16) & 0xff;
			case 2:
				((u8 *)payload)[1] = (pl >> 8) & 0xff;
			case 1:
				((u8 *)payload)[0] = pl & 0xff;
				break;
			}
		}

		break;
	default:
		return -EINVAL;
	}

	return length;
}

static void sec_mipi_dsim_init_fifo_pointers(struct sec_mipi_dsim *dsim)
{
	uint32_t fifoctrl, fifo_ptrs;

	fifoctrl = dsim_read(dsim, DSIM_FIFOCTRL);

	fifo_ptrs = FIFOCTRL_NINITRX	|
		    FIFOCTRL_NINITSFR	|
		    FIFOCTRL_NINITI80	|
		    FIFOCTRL_NINITSUB	|
		    FIFOCTRL_NINITMAIN;

	fifoctrl &= ~fifo_ptrs;
	dsim_write(dsim, fifoctrl, DSIM_FIFOCTRL);
	udelay(500);

	fifoctrl |= fifo_ptrs;
	dsim_write(dsim, fifoctrl, DSIM_FIFOCTRL);
	udelay(500);
}


static void sec_mipi_dsim_config_clkctrl(struct sec_mipi_dsim *dsim)
{
	uint32_t clkctrl = 0, data_lanes_en;
	uint64_t byte_clk, esc_prescaler;

	clkctrl |= CLKCTRL_TXREQUESTHSCLK;

	/* using 1.5Gbps PHY */
	clkctrl |= CLKCTRL_DPHY_SEL_1P5G;

	clkctrl |= CLKCTRL_ESCCLKEN;

	clkctrl &= ~CLKCTRL_PLLBYPASS;

	clkctrl |= CLKCTRL_BYTECLKSRC_DPHY_PLL;

	clkctrl |= CLKCTRL_BYTECLKEN;

	data_lanes_en = (0x1 << dsim->lanes) - 1;
	clkctrl |= CLKCTRL_SET_LANEESCCLKEN(0x1 | data_lanes_en << 1);

	/* calculate esc prescaler from byte clock:
	 * EscClk = ByteClk / EscPrescaler;
	 */
	byte_clk = dsim->bit_clk >> 3;
	esc_prescaler = DIV_ROUND_UP_ULL(byte_clk, MAX_ESC_CLK_FREQ);

	clkctrl |= CLKCTRL_SET_ESCPRESCALER(esc_prescaler);

	debug("DSIM clkctrl 0x%x\n", clkctrl);

	dsim_write(dsim, clkctrl, DSIM_CLKCTRL);
}

static void sec_mipi_dsim_set_standby(struct sec_mipi_dsim *dsim,
				      bool standby)
{
	uint32_t mdresol = 0;

	mdresol = dsim_read(dsim, DSIM_MDRESOL);

	if (standby)
		mdresol |= MDRESOL_MAINSTANDBY;
	else
		mdresol &= ~MDRESOL_MAINSTANDBY;

	dsim_write(dsim, mdresol, DSIM_MDRESOL);
}

static void sec_mipi_dsim_disable_clkctrl(struct sec_mipi_dsim *dsim)
{
	uint32_t clkctrl;

	clkctrl = dsim_read(dsim, DSIM_CLKCTRL);

	clkctrl &= ~CLKCTRL_TXREQUESTHSCLK;

	clkctrl &= ~CLKCTRL_ESCCLKEN;

	clkctrl &= ~CLKCTRL_BYTECLKEN;

	dsim_write(dsim, clkctrl, DSIM_CLKCTRL);
}

static void sec_mipi_dsim_disable_pll(struct sec_mipi_dsim *dsim)
{
	uint32_t pllctrl;

	pllctrl  = dsim_read(dsim, DSIM_PLLCTRL);

	pllctrl &= ~PLLCTRL_PLLEN;

	dsim_write(dsim, pllctrl, DSIM_PLLCTRL);
}

static inline struct sec_mipi_dsim *host_to_dsi(struct mipi_dsi_host *host)
{
	return container_of(host, struct sec_mipi_dsim, dsi_host);
}

static int sec_mipi_dsim_bridge_clk_set(struct sec_mipi_dsim *dsim_host)
{
	int bpp, ret;
	uint64_t pix_clk, bit_clk;

	bpp = mipi_dsi_pixel_format_to_bpp(dsim_host->format);
	if (bpp < 0)
		return -EINVAL;

	pix_clk = dsim_host->timings.pixelclock.typ;
	bit_clk = DIV_ROUND_UP_ULL(pix_clk * bpp, dsim_host->lanes);

#if 0
	if (bit_clk > dsim_host->max_data_rate) {
		printf("request bit clk freq exceeds lane's maximum value\n");
		return -EINVAL;
	}
#endif

	dsim_host->pix_clk = DIV_ROUND_UP_ULL(pix_clk, 1000);
	dsim_host->bit_clk = DIV_ROUND_UP_ULL(bit_clk, 1000);

	ret = sec_mipi_dsim_calc_pmsk(dsim_host);
	if (ret) {
		printf("failed to get pmsk for: fout = %llu\n",
			dsim_host->bit_clk);
		return -EINVAL;
	}

	dsim_host->pms = PLLCTRL_SET_P(dsim_host->p) |
		    PLLCTRL_SET_M(dsim_host->m) |
		    PLLCTRL_SET_S(dsim_host->s);

	debug("%s: bitclk %llu pixclk %llu pms 0x%x\n", __func__,
		dsim_host->bit_clk, dsim_host->pix_clk, dsim_host->pms);

	return 0;
}

static int sec_mipi_dsim_bridge_prepare(struct sec_mipi_dsim *dsim_host)
{
	int ret;

	/* At this moment, the dsim bridge's preceding encoder has
	 * already been enabled. So the dsim can be configed here
	 */

	/* config main display mode */
	sec_mipi_dsim_set_main_mode(dsim_host);

	/* config dsim dpi */
	sec_mipi_dsim_config_dpi(dsim_host);

	/* config dsim pll */
	ret = sec_mipi_dsim_config_pll(dsim_host);
	if (ret) {
		printf("dsim pll config failed: %d\n", ret);
		return ret;
	}

	/* config dphy timings */
	sec_mipi_dsim_config_dphy(dsim_host);

	sec_mipi_dsim_init_fifo_pointers(dsim_host);

	/* config esc clock, byte clock and etc */
	sec_mipi_dsim_config_clkctrl(dsim_host);

	/* enable data transfer of dsim */
	sec_mipi_dsim_set_standby(dsim_host, true);

	return 0;
}

static int sec_mipi_dsim_host_attach(struct mipi_dsi_host *host,
				   struct mipi_dsi_device *device)
{
	struct sec_mipi_dsim *dsi = host_to_dsi(host);

	if (!device->lanes || device->lanes > dsi->max_data_lanes) {
		printf("invalid data lanes number\n");
		return -EINVAL;
	}

	if (!(device->mode_flags & MIPI_DSI_MODE_VIDEO)		||
	    !((device->mode_flags & MIPI_DSI_MODE_VIDEO_BURST)	||
	      (device->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE))) {
		printf("unsupported dsi mode\n");
		return -EINVAL;
	}

	if (device->format != MIPI_DSI_FMT_RGB888 &&
	    device->format != MIPI_DSI_FMT_RGB565 &&
	    device->format != MIPI_DSI_FMT_RGB666 &&
	    device->format != MIPI_DSI_FMT_RGB666_PACKED) {
		printf("unsupported pixel format: %#x\n", device->format);
		return -EINVAL;
	}

	dsi->lanes	 = device->lanes;
	dsi->channel	 = device->channel;
	dsi->format	 = device->format;
	dsi->mode_flags = device->mode_flags;

	debug("lanes %u, channel %u, format 0x%x, mode_flags 0x%lx\n", dsi->lanes,
		dsi->channel, dsi->format, dsi->mode_flags);

	sec_mipi_dsim_bridge_clk_set(dsi);
	sec_mipi_dsim_bridge_prepare(dsi);

	return 0;
}

static ssize_t sec_mipi_dsi_host_transfer(struct mipi_dsi_host *host,
					 const struct mipi_dsi_msg *msg)
{
	struct sec_mipi_dsim *dsim = host_to_dsi(host);
	int ret, nb_bytes;
	bool use_lpm;
	struct mipi_dsi_packet packet;

#ifdef DEBUG
	int i = 0;
	u8 *p = msg->tx_buf;

	printf("sec_mipi_dsi_host_transfer\n");
	for (i; i < msg->tx_len; i++) {
		printf("0x%.2x ", *(u8 *)p);
		p++;
	}
	printf("\n");
#endif

	ret = mipi_dsi_create_packet(&packet, msg);
	if (ret) {
		dev_err(dsim->device->dev, "failed to create dsi packet: %d\n", ret);
		return ret;
	}

	/* config LPM for CMD TX */
	use_lpm = msg->flags & MIPI_DSI_MSG_USE_LPM ? true : false;
	sec_mipi_dsim_config_cmd_lpm(dsim, use_lpm);

	if (packet.payload_length) {		/* Long Packet case */
		/* write packet payload */
		sec_mipi_dsim_write_pl_to_sfr_fifo(dsim,
						   packet.payload,
						   packet.payload_length);

		/* write packet header */
		sec_mipi_dsim_write_ph_to_sfr_fifo(dsim,
						   packet.header,
						   use_lpm);

		ret = sec_mipi_dsim_wait_for_pkt_done(dsim, MIPI_FIFO_TIMEOUT);
		if (ret) {
			dev_err(dsim->device->dev, "wait tx done timeout!\n");
			return -EBUSY;
		}
	} else {
		/* write packet header */
		sec_mipi_dsim_write_ph_to_sfr_fifo(dsim,
						   packet.header,
						   use_lpm);

		ret = sec_mipi_dsim_wait_for_hdr_done(dsim, MIPI_FIFO_TIMEOUT);
		if (ret) {
			dev_err(dsim->device->dev, "wait pkthdr tx done time out\n");
			return -EBUSY;
		}
	}

	/* read packet payload */
	if (unlikely(msg->rx_buf)) {
		ret = sec_mipi_dsim_wait_for_rx_done(dsim,
						  MIPI_FIFO_TIMEOUT);
		if (ret) {
			dev_err(dsim->device->dev, "wait rx done time out\n");
			return -EBUSY;
		}

		ret = sec_mipi_dsim_read_pl_from_sfr_fifo(dsim,
							  msg->rx_buf,
							  msg->rx_len);
		if (ret < 0)
			return ret;
		nb_bytes = msg->rx_len;
	} else {
		nb_bytes = packet.size;
	}

	return nb_bytes;

}


static const struct mipi_dsi_host_ops sec_mipi_dsim_host_ops = {
	.attach = sec_mipi_dsim_host_attach,
	.transfer = sec_mipi_dsi_host_transfer,
};

static int sec_mipi_dsim_init(struct udevice *dev,
			    struct mipi_dsi_device *device,
			    struct display_timing *timings,
			    unsigned int max_data_lanes,
			    const struct mipi_dsi_phy_ops *phy_ops)
{
	struct sec_mipi_dsim *dsi = dev_get_priv(dev);

	dsi->max_data_lanes = max_data_lanes;
	dsi->device = device;
	dsi->dsi_host.ops = &sec_mipi_dsim_host_ops;
	device->host = &dsi->dsi_host;

	dsi->base = (void *)dev_read_addr(device->dev);
	if ((fdt_addr_t)dsi->base == FDT_ADDR_T_NONE) {
		dev_err(device->dev, "dsi dt register address error\n");
		return -EINVAL;
	}

	dsi->timings = *timings;

	return 0;
}

static int sec_mipi_dsim_enable(struct udevice *dev)
{
	return 0;
}

static int sec_mipi_dsim_disable(struct udevice *dev)
{
	uint32_t intsrc;
	struct sec_mipi_dsim *dsim_host = dev_get_priv(dev);

	/* disable data transfer of dsim */
	sec_mipi_dsim_set_standby(dsim_host, false);

	/* disable esc clock & byte clock */
	sec_mipi_dsim_disable_clkctrl(dsim_host);

	/* disable dsim pll */
	sec_mipi_dsim_disable_pll(dsim_host);

	/* Clear all intsrc */
	intsrc = dsim_read(dsim_host, DSIM_INTSRC);
	dsim_write(dsim_host, intsrc, DSIM_INTSRC);

	return 0;
}

struct dsi_host_ops sec_mipi_dsim_ops = {
	.init = sec_mipi_dsim_init,
	.enable = sec_mipi_dsim_enable,
	.disable = sec_mipi_dsim_disable,
};

static int sec_mipi_dsim_probe(struct udevice *dev)
{
	return 0;
}

static const struct udevice_id sec_mipi_dsim_ids[] = {
	{ .compatible = "samsung,sec-mipi-dsi" },
	{ }
};

U_BOOT_DRIVER(sec_mipi_dsim) = {
	.name			= "sec_mipi_dsim",
	.id			= UCLASS_DSI_HOST,
	.of_match		= sec_mipi_dsim_ids,
	.probe			= sec_mipi_dsim_probe,
	.remove 		= sec_mipi_dsim_disable,
	.ops			= &sec_mipi_dsim_ops,
	.priv_auto	= sizeof(struct sec_mipi_dsim),
};
