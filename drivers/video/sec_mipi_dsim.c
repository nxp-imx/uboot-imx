/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/sys_proto.h>
#include <div64.h>
#include <sec_mipi_dsim.h>
#include <imx_mipi_dsi_bridge.h>

#define MIPI_FIFO_TIMEOUT		250000 /* 250ms */

#define DRIVER_NAME "imx_sec_mipi_dsim"

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

/* fixed phy ref clk rate */
#define PHY_REF_CLK		27000000

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
   #define MIPI_DSI_I_PRESETn_SFT_EN		BIT(5)
#define DISPLAY_MIX_CLK_EN_CSR			0x04
   #define MIPI_DSI_PCLK_SFT_EN			BIT(8)
   #define MIPI_DSI_CLKREF_SFT_EN		BIT(9)
#define GPR_MIPI_RESET_DIV			0x08
   /* Clock & Data lanes reset: Active Low */
   #define GPR_MIPI_S_RESETN			BIT(16)
   #define GPR_MIPI_M_RESETN			BIT(17)

#define DIV_ROUND_UP_ULL(ll,d) \
	({ unsigned long long _tmp = (ll)+(d)-1; do_div(_tmp, d); _tmp; })

#define	PS2KHZ(ps)	(1000000000UL / (ps))

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
	void __iomem *disp_mix_gpr_base;

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
	struct fb_videomode vmode;

	const struct sec_mipi_dsim_plat_data *pdata;

	struct mipi_dsi_client_dev *dsi_panel_dev;
	struct mipi_dsi_client_driver *dsi_panel_drv;
};

static void disp_mix_dsim_soft_reset_release(struct sec_mipi_dsim *dsim, bool release)
{
	if (release)
		/* release dsi blk reset */
		setbits_le32(dsim->disp_mix_gpr_base + DISPLAY_MIX_SFT_RSTN_CSR, MIPI_DSI_I_PRESETn_SFT_EN);

	else
		clrbits_le32(dsim->disp_mix_gpr_base + DISPLAY_MIX_SFT_RSTN_CSR, MIPI_DSI_I_PRESETn_SFT_EN);
}

static void disp_mix_dsim_clks_enable(struct sec_mipi_dsim *dsim, bool enable)
{
	if (enable)
		setbits_le32(dsim->disp_mix_gpr_base + DISPLAY_MIX_CLK_EN_CSR, MIPI_DSI_PCLK_SFT_EN | MIPI_DSI_CLKREF_SFT_EN);
	else
		clrbits_le32(dsim->disp_mix_gpr_base + DISPLAY_MIX_CLK_EN_CSR, MIPI_DSI_PCLK_SFT_EN | MIPI_DSI_CLKREF_SFT_EN);
}

static void disp_mix_dsim_lanes_reset(struct sec_mipi_dsim *dsim, bool reset)
{
	if (!reset)
		/* release lanes reset */
		setbits_le32(dsim->disp_mix_gpr_base + GPR_MIPI_RESET_DIV, GPR_MIPI_S_RESETN | GPR_MIPI_M_RESETN);
	else
		/* reset lanes */
		clrbits_le32(dsim->disp_mix_gpr_base + GPR_MIPI_RESET_DIV, GPR_MIPI_S_RESETN | GPR_MIPI_M_RESETN);
}

static void sec_mipi_dsim_wr_tx_header(struct sec_mipi_dsim *dsim,
			u8 di, u8 data0, u8 data1)
{
	unsigned int reg;

	reg = (data1 << 16) | (data0 << 8) | ((di & 0x3f) << 0);

	dsim_write(dsim, reg, DSIM_PKTHDR);
}

static void sec_mipi_dsim_wr_tx_data(struct sec_mipi_dsim *dsim,
				unsigned int tx_data)
{
	dsim_write(dsim, tx_data, DSIM_PAYLOAD);
}

static void sec_mipi_dsim_long_data_wr(struct sec_mipi_dsim *dsim,
			const unsigned char *data0, unsigned int data_size)
{
	unsigned int data_cnt = 0, payload = 0;

        /* in case that data count is more then 4 */
        for (data_cnt = 0; data_cnt < data_size; data_cnt += 4) {
                /*
                 * after sending 4bytes per one time,
                 * send remainder data less then 4.
                 */
                if ((data_size - data_cnt) < 4) {
                        if ((data_size - data_cnt) == 3) {
                                payload = data0[data_cnt] |
                                    data0[data_cnt + 1] << 8 |
                                        data0[data_cnt + 2] << 16;
                        debug("count = 3 payload = %x, %x %x %x\n",
                                payload, data0[data_cnt],
                                data0[data_cnt + 1],
                                data0[data_cnt + 2]);
                        } else if ((data_size - data_cnt) == 2) {
                                payload = data0[data_cnt] |
                                        data0[data_cnt + 1] << 8;
                        debug("count = 2 payload = %x, %x %x\n", payload,
                                data0[data_cnt],
                                data0[data_cnt + 1]);
                        } else if ((data_size - data_cnt) == 1) {
                                payload = data0[data_cnt];
                        }

                        sec_mipi_dsim_wr_tx_data(dsim, payload);
                /* send 4bytes per one time. */
                } else {
                        payload = data0[data_cnt] |
                                data0[data_cnt + 1] << 8 |
                                data0[data_cnt + 2] << 16 |
                                data0[data_cnt + 3] << 24;

                        debug("count = 4 payload = %x, %x %x %x %x\n",
                                payload, *(u8 *)(data0 + data_cnt),
                                data0[data_cnt + 1],
                                data0[data_cnt + 2],
                                data0[data_cnt + 3]);

			sec_mipi_dsim_wr_tx_data(dsim, payload);
                }
        }
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

static int sec_mipi_dsim_pkt_write(struct sec_mipi_dsim *dsim,
		       u8 data_type, const u8 *buf, int len)
{
	int ret = 0;
	const unsigned char *data = (const unsigned char*)buf;

	if (len == 0)
		/* handle generic short write command */
		sec_mipi_dsim_wr_tx_header(dsim, data_type, data[0], data[1]);
	else {
		/* handle generic long write command */
		sec_mipi_dsim_long_data_wr(dsim, data, len);
		sec_mipi_dsim_wr_tx_header(dsim, data_type, len & 0xff, (len & 0xff00) >> 8);

		ret = sec_mipi_dsim_wait_for_pkt_done(dsim, MIPI_FIFO_TIMEOUT);
		if (ret) {
			printf("wait tx done timeout!\n");
			return -ETIMEDOUT;
		}
	}
	mdelay(10);

	return 0;
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
	uint32_t bpp, hfp_wc, hbp_wc, hsa_wc;
	uint32_t mdresol = 0, mvporch = 0, mhporch = 0, msync = 0;
	struct fb_videomode *vmode = &dsim->vmode;

	mdresol |= MDRESOL_SET_MAINVRESOL(vmode->yres) |
		   MDRESOL_SET_MAINHRESOL(vmode->xres);
	dsim_write(dsim, mdresol, DSIM_MDRESOL);

	mvporch |= MVPORCH_SET_MAINVBP(vmode->upper_margin)    |
		   MVPORCH_SET_STABLEVFP(vmode->lower_margin) |
		   MVPORCH_SET_CMDALLOW(0x0);
	dsim_write(dsim, mvporch, DSIM_MVPORCH);

	bpp = mipi_dsi_pixel_format_to_bpp(dsim->format);

	/* calculate hfp & hbp word counts */
	if (dsim->dsi_panel_drv) {
		/* Panel driver is registered, will work with panel */
		hfp_wc = vmode->right_margin * (bpp >> 3);
		hbp_wc = vmode->left_margin * (bpp >> 3);
	} else {
		hfp_wc = vmode->right_margin * (bpp >> 3) / dsim->lanes - 6;
		hbp_wc = vmode->left_margin * (bpp >> 3) / dsim->lanes - 6;
	}

	mhporch |= MHPORCH_SET_MAINHFP(hfp_wc) |
		   MHPORCH_SET_MAINHBP(hbp_wc);

	dsim_write(dsim, mhporch, DSIM_MHPORCH);

	/* calculate hsa word counts */
	if (dsim->dsi_panel_drv) {
		hsa_wc = vmode->hsync_len * (bpp >> 3);
	} else {
		hsa_wc = vmode->hsync_len * (bpp >> 3) / dsim->lanes - 6;
	}

	msync |= MSYNC_SET_MAINVSA(vmode->vsync_len) |
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

	dsim_write(dsim, config, DSIM_CONFIG);
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

/* For now, dsim only support one device attached */
static int sec_mipi_dsim_bridge_attach(struct mipi_dsi_bridge_driver *bridge_driver,
	struct mipi_dsi_client_dev *dsi_dev)
{
	struct sec_mipi_dsim *dsim_host = (struct sec_mipi_dsim *)bridge_driver->driver_private;

	if (!dsi_dev->lanes || dsi_dev->lanes > dsim_host->pdata->max_data_lanes) {
		printf("invalid data lanes number\n");
		return -EINVAL;
	}

	if (!(dsi_dev->mode_flags & MIPI_DSI_MODE_VIDEO)		||
	    !((dsi_dev->mode_flags & MIPI_DSI_MODE_VIDEO_BURST)	||
	      (dsi_dev->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE))) {
		printf("unsupported dsi mode\n");
		return -EINVAL;
	}

	if (dsi_dev->format != MIPI_DSI_FMT_RGB888 &&
	    dsi_dev->format != MIPI_DSI_FMT_RGB565 &&
	    dsi_dev->format != MIPI_DSI_FMT_RGB666 &&
	    dsi_dev->format != MIPI_DSI_FMT_RGB666_PACKED) {
		printf("unsupported pixel format: %#x\n", dsi_dev->format);
		return -EINVAL;
	}

	if (!dsi_dev->name) {
		printf("panel_device name is NULL.\n");
		return -EFAULT;
	}

	if (dsim_host->dsi_panel_drv) {
		if (strcmp(dsi_dev->name, dsim_host->dsi_panel_drv->name)) {
			printf("The panel device name %s is not for LCD driver %s\n",
			       dsi_dev->name, dsim_host->dsi_panel_drv->name);
			return -EFAULT;
		}
	}

	dsim_host->dsi_panel_dev = dsi_dev;

	dsim_host->lanes	 = dsi_dev->lanes;
	dsim_host->channel	 = dsi_dev->channel;
	dsim_host->format	 = dsi_dev->format;
	dsim_host->mode_flags = dsi_dev->mode_flags;

	return 0;
}

static int sec_mipi_dsim_bridge_enable(struct mipi_dsi_bridge_driver *bridge_driver)
{
	int ret;
	struct sec_mipi_dsim *dsim_host = (struct sec_mipi_dsim *)bridge_driver->driver_private;

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
		return -EPERM;
	}

	/* config dphy timings */
	sec_mipi_dsim_config_dphy(dsim_host);

	/* config esc clock, byte clock and etc */
	sec_mipi_dsim_config_clkctrl(dsim_host);

	/* enable data transfer of dsim */
	sec_mipi_dsim_set_standby(dsim_host, true);

	/* Call panel driver's setup */
	if (dsim_host->dsi_panel_drv && dsim_host->dsi_panel_drv->dsi_client_setup) {
		ret = dsim_host->dsi_panel_drv->dsi_client_setup(dsim_host->dsi_panel_dev);
		if (ret < 0) {
			printf("failed to init mipi lcd.\n");
			return ret;
		}
	}

	return 0;
}

static int sec_mipi_dsim_bridge_disable(struct mipi_dsi_bridge_driver *bridge_driver)
{
	uint32_t intsrc;
	struct sec_mipi_dsim *dsim_host = (struct sec_mipi_dsim *)bridge_driver->driver_private;

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

static int sec_mipi_dsim_bridge_mode_set(struct mipi_dsi_bridge_driver *bridge_driver,
	struct fb_videomode *fbmode)
{
	int bpp;
	uint64_t pix_clk, bit_clk;
	struct sec_mipi_dsim *dsim_host = (struct sec_mipi_dsim *)bridge_driver->driver_private;

	dsim_host->vmode = *fbmode;

	bpp = mipi_dsi_pixel_format_to_bpp(dsim_host->format);
	if (bpp < 0)
		return -EINVAL;

	pix_clk = PS2KHZ(fbmode->pixclock) * 1000;
	bit_clk = DIV_ROUND_UP_ULL(pix_clk * bpp, dsim_host->lanes);

	if (bit_clk > dsim_host->pdata->max_data_rate) {
		printf("request bit clk freq exceeds lane's maximum value\n");
		return -EINVAL;
	}

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

/* Add a LCD panel driver, will search the panel device to bind with them */
int sec_mipi_dsim_bridge_add_client_driver(struct mipi_dsi_bridge_driver *bridge_driver,
	struct mipi_dsi_client_driver *panel_drv)
{
	struct sec_mipi_dsim *dsim_host = (struct sec_mipi_dsim *)bridge_driver->driver_private;

	if (!panel_drv) {
		printf("mipi_dsi_northwest_panel_driver is NULL.\n");
		return -EFAULT;
	}

	if (!panel_drv->name) {
		printf("mipi_dsi_northwest_panel_driver name is NULL.\n");
		return -EFAULT;
	}

	if (dsim_host->dsi_panel_dev) {
		if (strcmp(panel_drv->name, dsim_host->dsi_panel_dev->name)) {
			printf("The panel driver name %s is not for LCD device %s\n",
			       panel_drv->name, dsim_host->dsi_panel_dev->name);
			return -EFAULT;
		}
	}

	dsim_host->dsi_panel_drv = panel_drv;

	return 0;
}

static int sec_mipi_dsim_bridge_pkt_write(struct mipi_dsi_bridge_driver *bridge_driver,
			u8 data_type, const u8 *buf, int len)
{
	struct sec_mipi_dsim *dsim_host = (struct sec_mipi_dsim *)bridge_driver->driver_private;

#ifdef DEBUG
	int i = 0;
	printf("sec_mipi_dsim_bridge_pkt_write, data_type %u, len %d buf: \n", data_type, len);

	if (len == 0)
		len = 2;

	for (i; i < len; i++) {
		printf("0x%.2x ", buf[i]);
	}
	printf("\n");
#endif

	return sec_mipi_dsim_pkt_write(dsim_host, data_type, buf, len);
}

struct mipi_dsi_bridge_driver imx_sec_dsim_driver = {
	.attach    = sec_mipi_dsim_bridge_attach,
	.enable   = sec_mipi_dsim_bridge_enable,
	.disable   = sec_mipi_dsim_bridge_disable,
	.mode_set   = sec_mipi_dsim_bridge_mode_set,
	.pkt_write = sec_mipi_dsim_bridge_pkt_write,
	.add_client_driver = sec_mipi_dsim_bridge_add_client_driver,
	.name = DRIVER_NAME,
};

int sec_mipi_dsim_setup(const struct sec_mipi_dsim_plat_data *plat_data)
{
	struct sec_mipi_dsim *dsim_host;

	if (!plat_data) {
		printf("Invalid platform data \n");
		return -EINVAL;
	}

	dsim_host = (struct sec_mipi_dsim *)malloc(sizeof(struct sec_mipi_dsim));
	if (!dsim_host) {
		printf("failed to allocate sec_mipi_dsim object.\n");
		return -ENOMEM;
	}

	dsim_host->base = (void __iomem *)plat_data->reg_base;
	dsim_host->disp_mix_gpr_base = (void __iomem *)plat_data->gpr_base;
	dsim_host->pdata = plat_data;
	dsim_host->dsi_panel_drv = NULL;
	dsim_host->dsi_panel_dev = NULL;

	/* Pull dsim out of reset */
	disp_mix_dsim_soft_reset_release(dsim_host, true);
	disp_mix_dsim_clks_enable(dsim_host, true);
	disp_mix_dsim_lanes_reset(dsim_host, false);

	imx_sec_dsim_driver.driver_private = dsim_host;
	return imx_mipi_dsi_bridge_register_driver(&imx_sec_dsim_driver);
}
