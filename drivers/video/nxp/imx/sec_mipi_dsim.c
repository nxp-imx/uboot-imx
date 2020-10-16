/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <asm/io.h>
#include <linux/err.h>
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

	/* TODO: add a PHY timing table arranged by the pll Fout */

	phytiming  |= PHYTIMING_SET_M_TLPXCTL(6)	|
		      PHYTIMING_SET_M_THSEXITCTL(11);
	dsim_write(dsim, phytiming, DSIM_PHYTIMING);

	phytiming1 |= PHYTIMING1_SET_M_TCLKPRPRCTL(7)	|
		      PHYTIMING1_SET_M_TCLKZEROCTL(38)	|
		      PHYTIMING1_SET_M_TCLKPOSTCTL(13)	|
		      PHYTIMING1_SET_M_TCLKTRAILCTL(8);
	dsim_write(dsim, phytiming1, DSIM_PHYTIMING1);

	phytiming2 |= PHYTIMING2_SET_M_THSPRPRCTL(8)	|
		      PHYTIMING2_SET_M_THSZEROCTL(13)	|
		      PHYTIMING2_SET_M_THSTRAILCTL(11);
	dsim_write(dsim, phytiming2, DSIM_PHYTIMING2);

	timeout |= TIMEOUT_SET_BTAOUT(0xf)	|
		   TIMEOUT_SET_LPDRTOUT(0xf);
	dsim_write(dsim, 0xf000f, DSIM_TIMEOUT);
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
		dev_err(dsim->dev, "peripheral report error: (0-7)%x, (8-15)%x\n",
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
			dev_err(dsim->dev, "invalid receive buffer length\n");
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
	int bpp;
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

	if (dsim_host->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE) {
		/* TODO: add PMS calculate and check
		 * Only support '1080p@60Hz' for now,
		 * add other modes support later
		 */
		dsim_host->pms = 0x4210;
	}

	debug("%s: bitclk %llu pixclk %llu\n", __func__, dsim_host->bit_clk, dsim_host->pix_clk);

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
		dev_err(dsim->dev, "failed to create dsi packet: %d\n", ret);
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
			dev_err(dsim->dev, "wait tx done timeout!\n");
			return -EBUSY;
		}
	} else {
		/* write packet header */
		sec_mipi_dsim_write_ph_to_sfr_fifo(dsim,
						   packet.header,
						   use_lpm);

		ret = sec_mipi_dsim_wait_for_hdr_done(dsim, MIPI_FIFO_TIMEOUT);
		if (ret) {
			dev_err(dsim->dev, "wait pkthdr tx done time out\n");
			return -EBUSY;
		}
	}

	/* read packet payload */
	if (unlikely(msg->rx_buf)) {
		ret = sec_mipi_dsim_wait_for_rx_done(dsim,
						  MIPI_FIFO_TIMEOUT);
		if (ret) {
			dev_err(dsim->dev, "wait rx done time out\n");
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
	.priv_auto_alloc_size	= sizeof(struct sec_mipi_dsim),
};
