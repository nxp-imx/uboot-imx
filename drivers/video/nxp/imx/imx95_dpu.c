// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023-2026 NXP
 *
 */
#include <dm.h>
#include <dm/device_compat.h>
#include <dm/device-internal.h>
#include <env.h>
#include <linux/err.h>
#include <linux/iopoll.h>
#include <malloc.h>
#include <video.h>
#include <display.h>

#include <asm/cache.h>
#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/sys_proto.h>
#include <asm/io.h>
#include <panel.h>
#include <video_bridge.h>
#include <video_link.h>
#include <clk.h>
#include <regmap.h>
#include <syscon.h>

#if defined(CONFIG_ANDROID_SUPPORT) && defined(CONFIG_IMX_TRUSTY_OS)
#include <trusty/trusty_dev.h>
#include "../lib/trusty/ql-tipc/arch/arm/smcall.h"

#define SMC_ENTITY_IMX_LINUX_OPT 54
#define SMC_IMX_ECHO SMC_FASTCALL_NR(SMC_ENTITY_IMX_LINUX_OPT, 0)
#define SMC_IMX_DPU_REG_WRITE SMC_FASTCALL_NR(SMC_ENTITY_IMX_LINUX_OPT, 3)
#define SMC_IMX_DPU_REG_READ SMC_FASTCALL_NR(SMC_ENTITY_IMX_LINUX_OPT, 4)

#ifdef writel
#undef writel
#define writel(v, a) trusty_simple_fast_call32(SMC_IMX_DPU_REG_WRITE, (fdt_addr_t)(a), (v), 0)
#endif
#ifdef clrsetbits_le32
#undef clrsetbits_le32
#define clrsetbits_le32(addr, clear, set) \
        { \
                u32 dpu_reg_value = trusty_simple_fast_call32(SMC_IMX_DPU_REG_READ, (fdt_addr_t)(addr), 0, 0); \
                dpu_reg_value &= ~(clear); \
                dpu_reg_value |= (set); \
                trusty_simple_fast_call32(SMC_IMX_DPU_REG_WRITE, (fdt_addr_t)(addr), (dpu_reg_value), 0); \
        }
#endif

#ifdef readl_poll_timeout
#undef readl_poll_timeout
#define readl_poll_timeout(addr, val, cond, timeout_us) \
({ \
        unsigned long timeout = timer_get_us() + timeout_us; \
        for (;;) { \
                (val) = trusty_simple_fast_call32(SMC_IMX_DPU_REG_READ, (fdt_addr_t)(addr), 0, 0); \
                if (cond) \
                        break; \
                if (timeout_us && time_after(timer_get_us(), timeout)) { \
                        val = trusty_simple_fast_call32(SMC_IMX_DPU_REG_READ, (fdt_addr_t)(addr), 0, 0); \
                        break; \
                } \
        } \
        (cond) ? 0 : -ETIMEDOUT; \
})
#endif

#endif

#define  LINEWIDTH(w)			(((w) - 1) & 0x3fff)
#define  LINECOUNT(h)			((((h) - 1) & 0x3fff) << 16)

#define  FRAMEWIDTH(w)			(((w) - 1) & 0x3fff)
#define  FRAMEHEIGHT(h)			((((h) - 1) & 0x3fff) << 16)

#define  BITSPERPIXEL_MASK		0x3f0000
#define  BITSPERPIXEL(bpp)		(((bpp) & 0x3f) << 16)
#define  STRIDE_MASK			0xffff
#define  STRIDE(n)			(((n) - 1) & 0xffff)

#define  YUVCONVERSIONMODE_MASK		0x60000
#define  YUVCONVERSIONMODE(m)		(((m) & 0x3) << 17)
#define  SOURCEBUFFERENABLE		BIT(31)

#define  R_BITS(n)			(((n) & 0xf) << 24)
#define  G_BITS(n)			(((n) & 0xf) << 16)
#define  B_BITS(n)			(((n) & 0xf) << 8)
#define  A_BITS(n)			((n) & 0xf)

#define  R_SHIFT(n)			(((n) & 0x1f) << 24)
#define  G_SHIFT(n)			(((n) & 0x1f) << 16)
#define  B_SHIFT(n)			(((n) & 0x1f) << 8)
#define  A_SHIFT(n)			((n) & 0x1f)

#define  COMBINERLINEFLUSH_ENABLE	BIT(30)
#define  COMBINERTIMEOUT_ENABLE		BIT(29)
#define  COMBINERTIMEOUT(n)		(((n) & 0xff) << 21)
#define  COMBINERTIMEOUT_MASK		0x1fe00000
#define  SETMAXBURSTLENGTH(n)		((((n) - 1) & 0xff) << 13)
#define  SETMAXBURSTLENGTH_MASK		0x1fe000
#define  SETBURSTLENGTH(n)		(((n) & 0x1f) << 8)
#define  SETBURSTLENGTH_MASK		0x1f00
#define  SETNUMBUFFERS(n)		((n) & 0xff)
#define  SETNUMBUFFERS_MASK		0xff
#define  LINEMODE_MASK			0x80000000
#define  LINEMODE_SHIFT			31
enum dpu95_linemode {
	/*
	 * Mandatory setting for operation in the Display Controller.
	 * Works also for Blit Engine with marginal performance impact.
	 */
	LINEMODE_DISPLAY = 0,
	/* Recommended setting for operation in the Blit Engine. */
	LINEMODE_BLIT = (1 << LINEMODE_SHIFT),
};

#define  CONSTANTALPHA_MASK		0xff
#define  CONSTANTALPHA(n)		((n) & CONSTANTALPHA_MASK)

#define  ALPHASRCENABLE			BIT(8)
#define  ALPHACONSTENABLE		BIT(9)
#define  ALPHAMASKENABLE		BIT(10)
#define  ALPHATRANSENABLE		BIT(11)
#define  ALPHA_ENABLE_MASK		(ALPHASRCENABLE | ALPHACONSTENABLE | \
					 ALPHAMASKENABLE | ALPHATRANSENABLE)
#define  RGBALPHASRCENABLE		BIT(12)
#define  RGBALPHACONSTENABLE		BIT(13)
#define  RGBALPHAMASKENABLE		BIT(14)
#define  RGBALPHATRANSENABLE		BIT(15)
#define  RGB_ENABLE_MASK		(RGBALPHASRCENABLE |	\
					 RGBALPHACONSTENABLE |	\
					 RGBALPHAMASKENABLE |	\
					 RGBALPHATRANSENABLE)
#define  PREMULCONSTRGB			BIT(16)


/* register in blk-ctrl */
#define  VIDEO_PLANE(n)			BIT(8 + 2 * (n))
#define  INT_PLANE			BIT(6)
#define  FRAC_PLANE(n)			BIT(2 * (n))

#define CTRL_MODE_MASK          BIT(0)
enum dpu95_lb_mode {
	LB_NEUTRAL, /* Output is same as primary input. */
	LB_BLEND,
};

#define  PRIM_A_BLD_FUNC__ZERO			(0x0 << 8)
#define  PRIM_A_BLD_FUNC__ONE			(0x1 << 8)
#define  SEC_A_BLD_FUNC__ONE			(0x1 << 12)
#define  PRIM_C_BLD_FUNC__ZERO			0x0
#define  PRIM_C_BLD_FUNC__ONE			0x1
#define  SEC_C_BLD_FUNC__ONE			(0x1 << 4)

#define PIXENGCFG_LAYERBLEND_DYNAMIC			0x8
#define  PIXENGCFG_DYNAMIC_PRIM_SEL_MASK	0x3f
#define  PIXENGCFG_DYNAMIC_SEC_SEL_SHIFT	8
#define  PIXENGCFG_DYNAMIC_SEC_SEL_MASK		0x3f00

#define CLKEN_MASK_SHIFT        24
#define CLKEN_MASK          (0x3 << 24)
#define CLKEN(n)            ((n) << CLKEN_MASK_SHIFT)
enum dpu95_pec_clken {
	CLKEN_DISABLE = 0x0,
	CLKEN_AUTOMATIC = 0x1,
	CLKEN_FULL = 0x3,
};

#define POSITION				0x14
#define  XPOS(x)				((x) & 0xffff)
#define  YPOS(y)				(((y) & 0xffff) << 16)

#define PIXENGCFG_EXTDST_STATIC		0x8
#define  POWERDOWN			BIT(4)
#define  SYNC_MODE			BIT(8)
#define  AUTO				BIT(8)
#define  SINGLE				0
#define  DIV_MASK			0xff0000
#define  DIV(n)				(((n) & 0xff) << 16)
#define  DIV_RESET			0x80

#define PIXENGCFG_EXTDST_DYNAMIC	0xc

#define PIXENGCFG_TRIGGER		0x14
#define  SYNC_TRIGGER			BIT(0)
#define  TRIGGER_SEQUENCE_COMPLETE	BIT(4)

#define STATICCONTROL			0x8
#define  KICK_MODE			BIT(8)
#define  EXTERNAL			BIT(8)
#define  SOFTWARE			0
#define  PERFCOUNTMODE			BIT(12)

#define MODECONTROL		0x10
enum dpu95_db_modecontrol {
	DB_MODE_PRIMARY,
	DB_MODE_SECONDARY,
	DB_MODE_BLEND,
	DB_MODE_SIDEBYSIDE,
};


/*frame gen*/
#define HTCFG1				0xc
#define  HTOTAL(n)			((((n) - 1) & 0x3fff) << 16)
#define  HACT(n)			((n) & 0x3fff)

#define HTCFG2				0x10
#define  HSEN				BIT(31)
#define  HSBP(n)			((((n) - 1) & 0x3fff) << 16)
#define  HSYNC(n)			(((n) - 1) & 0x3fff)

#define VTCFG1				0x14
#define  VTOTAL(n)			((((n) - 1) & 0x3fff) << 16)
#define  VACT(n)			((n) & 0x3fff)

#define VTCFG2				0x18
#define  VSEN				BIT(31)
#define  VSBP(n)			((((n) - 1) & 0x3fff) << 16)
#define  VSYNC(n)			(((n) - 1) & 0x3fff)

#define INTCONFIG(n)			(0x1c + 4 * (n))
#define  EN				BIT(31)
#define  ROW(n)				(((n) & 0x3fff) << 16)
#define  COL(n)				((n) & 0x3fff)

#define PKICKCONFIG			0x2c

#define SKICKCONFIG			0x30
#define  SKICKTRIG			BIT(30)

#define SECSTATCONFIG			0x34
#define FGSRCR(n)			(0x38 + ((n) - 1) * 0x4)
#define FGKSDR				0x74

#define PACFG				0x78
#define  STARTX(n)			(((n) + 1) & 0x3fff)
#define  STARTY(n)			(((((n) + 1) & 0x3fff)) << 16)

#define SACFG				0x7c

#define FGINCTRL			0x80
#define FGINCTRLPANIC			0x84
#define  FGDM_MASK			0x7
#define  ENPRIMALPHA			BIT(3)
#define  ENSECALPHA			BIT(4)

#define FGCCR				0x88
#define  CCALPHA(a)			(((a) & 0x3) << 30)
#define  CCRED(r)			(((r) & 0x3ff) << 20)
#define  CCGREEN(g)			(((g) & 0x3ff) << 10)
#define  CCBLUE(b)			((b) & 0x3ff)

#define FGENABLE			0x8c
#define  FGEN				BIT(0)

enum dpu95_fg_dm {
	FG_DM_BLACK,
	/* Constant Color Background is shown. */
	FG_DM_CONSTCOL,
	FG_DM_PRIM,
	FG_DM_SEC,
	FG_DM_PRIM_ON_TOP,
	FG_DM_SEC_ON_TOP,
	/* White color background with test pattern is shown. */
	FG_DM_TEST,
};

#define FGCHSTAT			0x9c
#define  SECSYNCSTAT			BIT(24)
#define  SFIFOEMPTY			BIT(16)
#define  PRIMSYNCSTAT			BIT(8)
#define  PFIFOEMPTY			BIT(0)

#define FGCHSTATCLR			0xa0
#define  CLRSECSTAT			BIT(16)
#define  CLRPRIMSTAT			BIT(0)

#define FRAMEDIMENSIONS		0xc
#define  WIDTH(w)		(((w) - 1) & 0x3fff)
#define  HEIGHT(h)		((((h) - 1) & 0x3fff) << 16)

#define CONSTANTCOLOR		0x10
#define  RED(r)			(((r) & 0xff) << 24)
#define  GREEN(g)		(((g) & 0xff) << 16)
#define  BLUE(b)		(((b) & 0xff) << 8)
#define  ALPHA(a)		((a) & 0xff)

#define DISPLAY_STATIC			0x1c
#define  PIPELINE_SYNC			BIT(0)

#define EXTDST0_STATIC			0x20
#define  MASTER				BIT(15)

enum dpu95_link_id {
	DPU95_LINK_ID_NONE		= 0x00,
	DPU95_LINK_ID_STORE9		= 0x01,
	DPU95_LINK_ID_EXTDST0		= 0x02,
	DPU95_LINK_ID_EXTDST4		= 0x03,
	DPU95_LINK_ID_EXTDST1		= 0x04,
	DPU95_LINK_ID_EXTDST5		= 0x05,
	DPU95_LINK_ID_FETCHECO9		= 0x07,
	DPU95_LINK_ID_HSCALER9		= 0x08,
	DPU95_LINK_ID_FILTER9		= 0x0a,
	DPU95_LINK_ID_CONSTFRAME0	= 0x0c,
	DPU95_LINK_ID_CONSTFRAME4	= 0x0d,
	DPU95_LINK_ID_CONSTFRAME1	= 0x10,
	DPU95_LINK_ID_CONSTFRAME5	= 0x11,
	DPU95_LINK_ID_LAYERBLEND1	= 0x14,
	DPU95_LINK_ID_LAYERBLEND2	= 0x15,
	DPU95_LINK_ID_LAYERBLEND3	= 0x16,
	DPU95_LINK_ID_LAYERBLEND4	= 0x17,
	DPU95_LINK_ID_LAYERBLEND5	= 0x18,
	DPU95_LINK_ID_LAYERBLEND6	= 0x19,
	DPU95_LINK_ID_FETCHLAYER0	= 0x1a,
	DPU95_LINK_ID_FETCHLAYER1	= 0x1b,
	DPU95_LINK_ID_FETCHYUV3		= 0x1c,
	DPU95_LINK_ID_FETCHYUV0		= 0x1d,
	DPU95_LINK_ID_FETCHECO0		= 0x1e,
	DPU95_LINK_ID_FETCHYUV1		= 0x1f,
	DPU95_LINK_ID_FETCHECO1		= 0x20,
	DPU95_LINK_ID_FETCHYUV2		= 0x21,
	DPU95_LINK_ID_FETCHECO2		= 0x22,
	DPU95_LINK_ID_MATRIX4		= 0x23,
	DPU95_LINK_ID_HSCALER4		= 0x24,
};

/* registers in blk-ctrl */
#define  DSIP_CLK_SEL(n)		(0x3 << (2 * (n)))
#define  CCM				0x0
#define  DSI_PLL(n)			(0x1 << (2 * (n)))
#define  LVDS_PLL_7(n)			(0x2 << (2 * (n)))

struct imx95_dpu_priv {
	/*struct udevice *bridge;*/
	struct udevice *disp_dev;

	u32 gpixfmt;
	u32 disp_id;

	bool enc_is_dsi;

	void __iomem *regs;
	void __iomem *regs_fetchlayer;
	void __iomem *regs_layerblend;
	void __iomem *regs_extdst;
	void __iomem *regs_domainblend;
	void __iomem *regs_framegen;
	void __iomem *regs_constframe;
	struct regmap *regmap;


	struct clk clk_axi;
	struct clk clk_apb;
	struct clk clk_ocram;
	struct clk clk_pix;
	struct clk clk_ldb;
	struct clk clk_ldbvco;

	struct display_timing timings;
	struct imx95_dpu_devdata *devdata;
};

struct dpu95_unit_id_off {
	u32 id;
	u32 offset;
	enum dpu95_link_id link_id;
};

struct imx95_dpu_devdata {
	unsigned int clock_ctrl;
	unsigned int shden;
	unsigned int plane_association;
	struct dpu95_unit_id_off fetchlayer[2];
	struct dpu95_unit_id_off domainblend[2];
	struct dpu95_unit_id_off framegen[2];
};

static void dpu95_fl_set_linemode(struct imx95_dpu_priv *priv,
				  enum dpu95_linemode mode)
{
	clrsetbits_le32(priv->regs_fetchlayer + 0x10, LINEMODE_MASK, mode);
}

static void dpu95_fl_set_numbuffers(struct imx95_dpu_priv *priv,
				    unsigned int num)
{
	clrsetbits_le32(priv->regs_fetchlayer + 0x10, SETNUMBUFFERS_MASK,
			    SETNUMBUFFERS(num));
}

static void dpu95_fl_set_burstlength(struct imx95_dpu_priv *priv, unsigned int burst_length)
{
	clrsetbits_le32(priv->regs_fetchlayer + 0x10, SETBURSTLENGTH_MASK,
			    SETBURSTLENGTH(burst_length));
}

static void dpu95_fl_set_maxburstlength(struct imx95_dpu_priv *priv, unsigned int burst_length)
{
	clrsetbits_le32(priv->regs_fetchlayer + 0x10, SETMAXBURSTLENGTH_MASK,
			    SETMAXBURSTLENGTH(burst_length));
}

static void dpu95_fl_set_baseaddress(struct imx95_dpu_priv *priv,
				     dma_addr_t baddr)
{
	writel(lower_32_bits(baddr), priv->regs_fetchlayer + 0x18);
	writel(upper_32_bits(baddr), priv->regs_fetchlayer + 0x1c);
}

static void dpu95_fl_set_pixel_blend_mode(struct imx95_dpu_priv *priv,
					  u8 alpha)
{
	clrsetbits_le32(priv->regs_fetchlayer + 0x48, PREMULCONSTRGB | ALPHA_ENABLE_MASK | RGB_ENABLE_MASK,
			    ALPHACONSTENABLE);	/* enable constant alpha */

	clrsetbits_le32(priv->regs_fetchlayer + 0x44, CONSTANTALPHA_MASK,
			    CONSTANTALPHA(alpha));
}

static void dpu95_fl_set_src_buf_dimensions(struct imx95_dpu_priv *priv,
				     unsigned int w, unsigned int h)
{
	writel(LINEWIDTH(w) | LINECOUNT(h), priv->regs_fetchlayer + 0x2c);
}

static void dpu95_fl_set_framedimensions(struct imx95_dpu_priv *priv,
					unsigned int w, unsigned int h)
{
	writel(FRAMEWIDTH(w) | FRAMEHEIGHT(h), priv->regs_fetchlayer + 0x1d8);
}

static void dpu95_fl_set_src_bpp(struct imx95_dpu_priv *priv, unsigned int bpp)
{
	clrsetbits_le32(priv->regs_fetchlayer + 0x28, BITSPERPIXEL_MASK, BITSPERPIXEL(bpp));
}

static void dpu95_fl_set_src_stride(struct imx95_dpu_priv *priv,
				    unsigned int stride)
{
	clrsetbits_le32(priv->regs_fetchlayer + 0x28, STRIDE_MASK, STRIDE(stride));
}

static void dpu95_fl_enable_src_buf(struct imx95_dpu_priv *priv, bool enable)
{
	clrsetbits_le32(priv->regs_fetchlayer + 0x48, SOURCEBUFFERENABLE,
			    enable ? SOURCEBUFFERENABLE : 0);
}

static void dpu95_fl_set_fmt(struct imx95_dpu_priv *priv, enum video_log2_bpp bpix, enum video_format format)
{
	dpu95_fl_set_src_bpp(priv, VNBITS(bpix));

	clrsetbits_le32(priv->regs_fetchlayer + 0x48, YUVCONVERSIONMODE_MASK,
			    YUVCONVERSIONMODE(0));

	/* Fixed to X8R8G8B8 */
	if (format == VIDEO_X8R8G8B8) {
		writel((R_BITS(8) | G_BITS(8) | B_BITS(8) | A_BITS(0)), priv->regs_fetchlayer + 0x30);
		writel((R_SHIFT(16) | G_SHIFT(8) | B_SHIFT(0) | A_SHIFT(0)), priv->regs_fetchlayer + 0x34);
	}
}

static void dpu95_fl_set_stream_id(struct imx95_dpu_priv *priv, unsigned int stream_id)
{
	int ret;

	ret = regmap_update_bits(priv->regmap, priv->devdata->plane_association,
				 FRAC_PLANE(0), stream_id ? FRAC_PLANE(0) : 0);
	if (ret < 0)
		printf("failed to set association Frac plane 0 bit: %d\n", ret);
}

static void dpu95_lb_mode(struct imx95_dpu_priv *priv, enum dpu95_lb_mode mode)
{
	clrsetbits_le32(priv->regs_layerblend + 0xc, CTRL_MODE_MASK, mode);
}

static void dpu95_lb_blendcontrol(struct imx95_dpu_priv *priv)
{
	writel(SEC_C_BLD_FUNC__ONE | SEC_A_BLD_FUNC__ONE, priv->regs_layerblend + 0x10);
}

static void dpu95_lb_pec_dynamic_prim_sel(struct imx95_dpu_priv *priv, u32 prim)
{
	clrsetbits_le32(priv->regs_layerblend + 0x1000 + 0x8, PIXENGCFG_DYNAMIC_PRIM_SEL_MASK, prim);
}

static void dpu95_lb_pec_dynamic_sec_sel(struct imx95_dpu_priv *priv, u32 sec)
{
	clrsetbits_le32(priv->regs_layerblend + 0x1000 + 0x8, PIXENGCFG_DYNAMIC_SEC_SEL_MASK, sec << PIXENGCFG_DYNAMIC_SEC_SEL_SHIFT);
}

static void dpu95_lb_pec_dynamic_clken(struct imx95_dpu_priv *priv, enum dpu95_pec_clken clken)
{
	clrsetbits_le32(priv->regs_layerblend + 0x1000 + 0x8, CLKEN_MASK, CLKEN(clken));
}

static void dpu95_lb_position(struct imx95_dpu_priv *priv, int x, int y)
{
	writel(XPOS(x) | YPOS(y), priv->regs_layerblend + POSITION);
}

static void dpu95_ed_pec_src_sel(struct imx95_dpu_priv *priv, u32 src)
{
	writel(src, priv->regs_extdst + 0x1000 + PIXENGCFG_EXTDST_DYNAMIC);
}

static void dpu95_ed_pec_poweron(struct imx95_dpu_priv *priv)
{
	clrsetbits_le32(priv->regs_extdst + 0x1000 + PIXENGCFG_EXTDST_STATIC, POWERDOWN, 0);
}

static void dpu95_ed_pec_sync_mode_single(struct imx95_dpu_priv *priv)
{
	clrsetbits_le32(priv->regs_extdst + 0x1000 + PIXENGCFG_EXTDST_STATIC, SYNC_MODE, SINGLE);
}

static void dpu95_ed_pec_enable_shden(struct imx95_dpu_priv *priv, bool enable)
{
	clrsetbits_le32(priv->regs_extdst + 0x1000 + PIXENGCFG_EXTDST_STATIC,
			priv->devdata->shden, enable ? priv->devdata->shden : 0);
}

static void dpu95_ed_pec_sync_trigger(struct imx95_dpu_priv *priv)
{
	writel(SYNC_TRIGGER, priv->regs_extdst + 0x1000 + PIXENGCFG_TRIGGER);
}

static void dpu95_ed_kick_mode_external(struct imx95_dpu_priv *priv)
{
	clrsetbits_le32(priv->regs_extdst + STATICCONTROL, KICK_MODE, EXTERNAL);
}

static void dpu95_db_modecontrol(struct imx95_dpu_priv *priv,
			  enum dpu95_db_modecontrol m)
{
	writel(m, priv->regs_domainblend + MODECONTROL);
}

static void dpu95_fg_cfg_videomode(struct imx95_dpu_priv *priv,
			    bool enc_is_dsi)
{
	u32 hact, htotal, hsync, hsbp;
	u32 vact, vtotal, vsync, vsbp;
	u32 vsync_bp, vsync_fp;
	u32 kick_row, kick_col;
	int ret;

	hact = priv->timings.hactive.typ;
	hsync = priv->timings.hsync_len.typ;
	hsbp = priv->timings.hback_porch.typ + hsync;
	htotal = hact + hsbp + priv->timings.hfront_porch.typ;

	vsync_bp = priv->timings.vback_porch.typ;
	vsync_fp = priv->timings.vfront_porch.typ;
	if (enc_is_dsi) {
		/* reduce one pixel from vfp and add one to vbp */
		vsync_fp -= 1;
		vsync_bp += 1;
	}

	vact = priv->timings.vactive.typ;
	vsync = priv->timings.vsync_len.typ;
	vsbp = vsync_bp + vsync;
	vtotal = vact + vsbp + vsync_fp;

	/* video mode */
	writel(HACT(hact)   | HTOTAL(htotal), priv->regs_framegen + HTCFG1);
	writel(HSYNC(hsync) | HSBP(hsbp) | HSEN, priv->regs_framegen + HTCFG2);
	writel(VACT(vact)   | VTOTAL(vtotal), priv->regs_framegen + VTCFG1);
	writel(VSYNC(vsync) | VSBP(vsbp) | VSEN, priv->regs_framegen + VTCFG2);

	kick_col = hact + 1;
	kick_row = vact;

	/* pkickconfig */
	writel(COL(kick_col) | ROW(kick_row) | EN, priv->regs_framegen + PKICKCONFIG);

	/* primary area position configuration */
	writel(STARTX(0) | STARTY(0), priv->regs_framegen + PACFG);

	/* alpha */
	clrsetbits_le32(priv->regs_framegen + FGINCTRL, ENPRIMALPHA | ENSECALPHA, 0);
	clrsetbits_le32(priv->regs_framegen + FGINCTRLPANIC, ENPRIMALPHA | ENSECALPHA, 0);

	/* constant color is green(used in panic mode)  */
	writel(CCGREEN(0x3ff), priv->regs_framegen + FGCCR);

	if (enc_is_dsi)
		clk_set_rate(&priv->clk_pix, priv->timings.pixelclock.typ);

	ret = regmap_update_bits(priv->regmap, priv->devdata->clock_ctrl,
				 DSIP_CLK_SEL(priv->disp_id), enc_is_dsi ? CCM : LVDS_PLL_7(priv->disp_id));
	if (ret < 0)
		printf("FrameGen0 failed to set DSIP_CLK_SEL: %d\n", ret);
}

static void dpu95_fg_enable(struct imx95_dpu_priv *priv, bool enable)
{
	writel(enable ? FGEN : 0, priv->regs_framegen + FGENABLE);
}

static void dpu95_fg_displaymode(struct imx95_dpu_priv *priv, enum dpu95_fg_dm mode)
{
	clrsetbits_le32(priv->regs_framegen + FGINCTRL, FGDM_MASK, mode);
}

static void dpu95_fg_enable_clock(struct imx95_dpu_priv *priv, bool enc_is_dsi, bool enable)
{
	if (enable) {
		if (enc_is_dsi) {
			clk_enable(&priv->clk_pix);
		} else {
			clk_enable(&priv->clk_ldbvco);
			clk_enable(&priv->clk_ldb);
		}
	} else {
		if (enc_is_dsi) {
			clk_disable(&priv->clk_pix);
		} else {
			clk_disable(&priv->clk_ldb);
			clk_disable(&priv->clk_ldbvco);
		}
	}
}

static void dpu95_fg_primary_clear_channel_status(struct imx95_dpu_priv *priv)
{
	writel(CLRPRIMSTAT, priv->regs_framegen + FGCHSTATCLR);
}

static int dpu95_fg_wait_for_primary_syncup(struct imx95_dpu_priv *priv)
{
	u32 val;
	int ret;

	ret = readl_poll_timeout(priv->regs_framegen + FGCHSTAT, val,
				 val & PRIMSYNCSTAT, 500000);
	if (ret) {
		printf("failed to wait for FrameGen primary syncup\n");
		return -ETIMEDOUT;
	}

	debug("FrameGen primary syncup\n");

	return 0;
}

static void dpu95_cf_framedimensions(struct imx95_dpu_priv *priv, unsigned int w,
			      unsigned int h)
{
	writel(WIDTH(w) | HEIGHT(h), priv->regs_constframe + FRAMEDIMENSIONS);
}

static void dpu95_cf_constantcolor_black(struct imx95_dpu_priv *priv)
{
	writel(0, priv->regs_constframe + CONSTANTCOLOR);
}

static void dpu95_dm_extdst0_master(struct imx95_dpu_priv *priv)
{
	writel(MASTER, priv->regs + 0x2000 + EXTDST0_STATIC);
}

struct dpu95_unit_id_off layerblend[6] = {
	{1, 0x170000, DPU95_LINK_ID_LAYERBLEND1 },
	{2, 0x180000, DPU95_LINK_ID_LAYERBLEND2 },
	{3, 0x190000, DPU95_LINK_ID_LAYERBLEND3 },
	{4, 0x1a0000, DPU95_LINK_ID_LAYERBLEND4 },
	{5, 0x1b0000, DPU95_LINK_ID_LAYERBLEND5 },
	{6, 0x1c0000, DPU95_LINK_ID_LAYERBLEND6 },
};

struct dpu95_unit_id_off extdst[4] = {
	{0, 0x110000, DPU95_LINK_ID_EXTDST0 },
	{4, 0x120000, DPU95_LINK_ID_EXTDST4 },
	{1, 0x150000, DPU95_LINK_ID_EXTDST1 },
	{5, 0x160000, DPU95_LINK_ID_EXTDST5 },
};

struct dpu95_unit_id_off constframe[4] = {
	{0, 0xf0000, DPU95_LINK_ID_CONSTFRAME0 },
	{4, 0x100000, DPU95_LINK_ID_CONSTFRAME4 },
	{1, 0x130000, DPU95_LINK_ID_CONSTFRAME1 },
	{5, 0x140000, DPU95_LINK_ID_CONSTFRAME5 },
};

static u32 find_unit_addr(struct dpu95_unit_id_off *unit, int num, u32 id)
{
	int i;

	for (i = 0; i < num; i++) {
		if (id == unit[i].id)
			return unit[i].offset;
	}

	return 0;
}

static enum dpu95_link_id find_unit_link_id(struct dpu95_unit_id_off *unit, int num, u32 id)
{
	int i;

	for (i = 0; i < num; i++) {
		if (id == unit[i].id)
			return unit[i].link_id;
	}

	return DPU95_LINK_ID_NONE;
}


static int imx95_dpu_path_config(struct udevice *dev, u32 fl_id, u32 lb_id, u32 cf_id, u32 ed_id, u32 db_id, u32 fg_id)
{
	struct imx95_dpu_priv *priv = dev_get_priv(dev);
	u32 off;

	off = find_unit_addr(priv->devdata->fetchlayer, ARRAY_SIZE(priv->devdata->fetchlayer), fl_id);
	if (!off) {
		dev_err(dev, "Invalid fl_id %u\n", fl_id);
		return -EINVAL;
	}
	priv->regs_fetchlayer = priv->regs + off;

	off = find_unit_addr(layerblend, ARRAY_SIZE(layerblend), lb_id);
	if (!off) {
		dev_err(dev, "Invalid lb_id %u\n", lb_id);
		return -EINVAL;
	}
	priv->regs_layerblend = priv->regs + off;

	off = find_unit_addr(extdst, ARRAY_SIZE(extdst), ed_id);
	if (!off) {
		dev_err(dev, "Invalid ed_id %u\n", ed_id);
		return -EINVAL;
	}
	priv->regs_extdst = priv->regs + off;

	off = find_unit_addr(priv->devdata->domainblend, ARRAY_SIZE(priv->devdata->domainblend), db_id);
	if (!off) {
		dev_err(dev, "Invalid db_id %u\n", db_id);
		return -EINVAL;
	}
	priv->regs_domainblend = priv->regs + off;

	off = find_unit_addr(priv->devdata->framegen, ARRAY_SIZE(priv->devdata->framegen), fg_id);
	if (!off) {
		dev_err(dev, "Invalid fg_id %u\n", fg_id);
		return -EINVAL;
	}
	priv->regs_framegen = priv->regs + off;

	off = find_unit_addr(constframe, ARRAY_SIZE(constframe), cf_id);
	if (!off) {
		dev_err(dev, "Invalid cf_id %u\n", cf_id);
		return -EINVAL;
	}
	priv->regs_constframe = priv->regs + off;

	dpu95_ed_pec_enable_shden(priv, false);
	dpu95_ed_pec_poweron(priv);
	dpu95_ed_pec_src_sel(priv, find_unit_link_id(layerblend, ARRAY_SIZE(layerblend),lb_id));
	dpu95_lb_pec_dynamic_sec_sel(priv, find_unit_link_id(priv->devdata->fetchlayer, ARRAY_SIZE(priv->devdata->fetchlayer), fl_id));
	dpu95_lb_pec_dynamic_prim_sel(priv, find_unit_link_id(constframe, ARRAY_SIZE(constframe), cf_id));

	if (ed_id == 0 || ed_id == 1)
		dpu95_db_modecontrol(priv, DB_MODE_PRIMARY);
	else
		dpu95_db_modecontrol(priv, DB_MODE_SECONDARY);

	return 0;
}

static int imx95_dpu_video_init(struct udevice *dev)
{
	struct imx95_dpu_priv *priv = dev_get_priv(dev);
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);

	dpu95_dm_extdst0_master(priv);

	dpu95_fl_set_linemode(priv, LINEMODE_DISPLAY);
	dpu95_fl_set_numbuffers(priv, 16);
	dpu95_fl_set_burstlength(priv, 16);
	dpu95_fl_set_maxburstlength(priv, 64);
	dpu95_fl_set_src_stride(priv, uc_priv->xsize * VNBYTES(uc_priv->bpix));
	dpu95_fl_set_src_buf_dimensions(priv, uc_priv->xsize, uc_priv->ysize);
	dpu95_fl_set_fmt(priv, uc_priv->bpix, uc_priv->format);
	dpu95_fl_set_pixel_blend_mode(priv, 0xff);
	dpu95_fl_enable_src_buf(priv, true);
	dpu95_fl_set_framedimensions(priv, uc_priv->xsize, uc_priv->ysize);
	dpu95_fl_set_baseaddress(priv, plat->base);
	dpu95_fl_set_stream_id(priv, priv->disp_id);

	dpu95_lb_mode(priv, LB_BLEND);
	dpu95_lb_blendcontrol(priv);
	dpu95_lb_position(priv, 0, 0);
	dpu95_lb_pec_dynamic_clken(priv, CLKEN_AUTOMATIC);

	dpu95_ed_pec_sync_mode_single(priv);
	dpu95_ed_kick_mode_external(priv);

	dpu95_fg_displaymode(priv, FG_DM_PRIM);
	dpu95_fg_cfg_videomode(priv, priv->enc_is_dsi);

	dpu95_cf_constantcolor_black(priv);
	dpu95_cf_framedimensions(priv, priv->timings.hactive.typ, priv->timings.vactive.typ);

	dpu95_fg_enable_clock(priv, priv->enc_is_dsi, true);
	dpu95_ed_pec_sync_trigger(priv);
	dpu95_fg_enable(priv, true);

	dpu95_fg_wait_for_primary_syncup(priv);
	dpu95_fg_primary_clear_channel_status(priv);

	debug("DPU95 display start ...\n");

	return 0;
}

static void imx95_dpu_check_dsi(struct udevice *dev)
{
	struct imx95_dpu_priv *priv = dev_get_priv(dev);
	struct udevice *link_dev;
	if (!priv->disp_dev)
		return;

	link_dev = priv->disp_dev;
	while (link_dev && device_get_uclass_id(link_dev) == UCLASS_VIDEO_BRIDGE)
		link_dev = video_link_get_next_device(link_dev);

	if (!link_dev)
		return;

	if (device_get_uclass_id(link_dev) == UCLASS_DISPLAY)
		priv->enc_is_dsi = false;
	else
		priv->enc_is_dsi = true;

	debug("next_videolink_dev %s, is_dsi %d\n", priv->disp_dev->name, priv->enc_is_dsi);
}

static int imx95_dpu_get_timings_from_display(struct udevice *dev,
					   struct display_timing *timings)
{
	struct imx95_dpu_priv *priv = dev_get_priv(dev);
	int err;
	ofnode endpoint, port_node;

	priv->disp_dev = video_link_get_next_device(dev);
	if (!priv->disp_dev) {
		dev_err(dev, "fail to find next video link device\n");
		return -ENODEV;
	}

	endpoint = video_link_get_ep_to_nextdev(priv->disp_dev);
	if (!ofnode_valid(endpoint)) {
		dev_err(dev, "fail to get endpoint to next bridge\n");
		return -ENODEV;
	}

	port_node = ofnode_get_parent(endpoint);

	debug("next_videolink_dev port_node %s\n", ofnode_get_name(port_node));

	priv->disp_id = (u32)ofnode_get_addr_size_index_notrans(port_node, 0, NULL);

	debug("disp_id %u\n", priv->disp_id);

	err = video_link_get_display_timings(timings);
	if (err)
		return err;

	return 0;
}

static int imx95_dpu_probe(struct udevice *dev)
{
	struct imx95_dpu_devdata *devdata;
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	struct imx95_dpu_priv *priv = dev_get_priv(dev);

	u32 fb_start, fb_end;
	int ret;

	devdata = (struct imx95_dpu_devdata *)dev_get_driver_data(dev);
	priv->devdata = devdata;

	debug("%s() plat: base 0x%lx, size 0x%x\n",
	       __func__, plat->base, plat->size);

	priv->regs = (void __iomem *)dev_read_addr(dev);
	if ((fdt_addr_t)priv->regs == FDT_ADDR_T_NONE) {
		dev_err(dev, "Fail to get reg\n");
		return -EINVAL;
	}

	priv->regmap = syscon_regmap_lookup_by_phandle(dev, "nxp,blk-ctrl");
	if (IS_ERR(priv->regmap)) {
		dev_err(dev, "failed to get blk-ctrl regmap\n");
		return IS_ERR(priv->regmap);
	}

	ret = clk_get_by_name(dev, "apb", &priv->clk_apb);
	if (ret) {
		dev_err(dev, "failed to get apb clock\n");
		return ret;
	}

	ret = clk_get_by_name(dev, "axi", &priv->clk_axi);
	if (ret) {
		dev_err(dev, "failed to get axi clock\n");
		return ret;
	}

	ret = clk_get_by_name(dev, "ocram", &priv->clk_ocram);
	if (ret) {
		dev_err(dev, "failed to get ocram clock\n");
		return ret;
	}

	ret = clk_get_by_name(dev, "pix", &priv->clk_pix);
	if (ret) {
		dev_err(dev, "failed to get pix clock\n");
		return ret;
	}

	ret = clk_get_by_name(dev, "ldb", &priv->clk_ldb);
	if (ret) {
		dev_err(dev, "failed to get ldb clock\n");
		return ret;
	}

	ret = clk_get_by_name(dev, "ldb_vco", &priv->clk_ldbvco);
	if (ret) {
		dev_err(dev, "failed to get ldb_vco clock\n");
		return ret;
	}

	clk_prepare_enable(&priv->clk_apb);
	clk_prepare_enable(&priv->clk_axi);
	clk_prepare_enable(&priv->clk_ocram);
	clk_disable(&priv->clk_pix);

	ret = imx95_dpu_get_timings_from_display(dev, &priv->timings);
	if (ret)
		return ret;

	if (device_get_uclass_id(priv->disp_dev) != UCLASS_VIDEO_BRIDGE) {
		dev_err(dev, "get bridge device error\n");
		return -ENODEV;
	}

	ret = video_bridge_attach(priv->disp_dev);
	if (ret) {
		dev_err(dev, "fail to attach bridge\n");
		return ret;
	}

	imx95_dpu_check_dsi(dev);

	ret = video_bridge_set_backlight(priv->disp_dev, 80);
	if (ret) {
		dev_err(dev, "fail to set backlight\n");
		return ret;
	}

	uc_priv->bpix = VIDEO_BPP32;
	uc_priv->format = VIDEO_X8R8G8B8;
	uc_priv->xsize = priv->timings.hactive.typ;
	uc_priv->ysize = priv->timings.vactive.typ;

	/* Enable dcache for the frame buffer */
	fb_start = plat->base & ~(MMU_SECTION_SIZE - 1);
	fb_end = plat->base + plat->size;
	fb_end = ALIGN(fb_end, 1 << MMU_SECTION_SHIFT);
	mmu_set_region_dcache_behaviour(fb_start, fb_end - fb_start,
					DCACHE_WRITEBACK);
	video_set_flush_dcache(dev, true);

#if defined(CONFIG_ANDROID_SUPPORT) && defined(CONFIG_IMX_TRUSTY_OS)
	ret = trusty_simple_fast_call32(SMC_IMX_ECHO, 0, 0, 0);
	if (ret) {
		printf("failed to get response of echo. Trusty may not be active.\n"); \
		return ret;
	}
#endif

	ret = imx95_dpu_path_config(dev, 0, 1, 0, priv->disp_id, priv->disp_id, priv->disp_id);
	if (ret) {
		dev_err(dev, "imx95_dpu_path_config fail %d\n", ret);
		return ret;
	}

	ret = imx95_dpu_video_init(dev);
	if (ret) {
		dev_err(dev, "imx95_dpu_video_init fail %d\n", ret);
		return ret;
	}

	return ret;
}

static int imx95_dpu_bind(struct udevice *dev)
{
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);

	plat->size = 2340 * 1080 *4;

	return 0;
}

static int imx95_dpu_remove(struct udevice *dev)
{
	struct imx95_dpu_priv *priv = dev_get_priv(dev);

	if (priv->disp_dev)
		device_remove(priv->disp_dev, DM_REMOVE_NORMAL);

	dpu95_fg_enable(priv, false);
	dpu95_fg_enable_clock(priv, priv->enc_is_dsi, false);
	dpu95_fl_enable_src_buf(priv, false);

	return 0;
}

static const struct imx95_dpu_devdata imx95_dpu_devdata = {
	.clock_ctrl = 0x00,
	.shden = BIT(8),
	.plane_association = 0x20,
	.fetchlayer = {
		{0, 0x1d0000, DPU95_LINK_ID_FETCHLAYER0 },
		{1, 0x1e0000, DPU95_LINK_ID_FETCHLAYER1 },
	},
	.domainblend = {
		{0, 0x2a0000},
		{1, 0x320000},
	},
	.framegen = {
		{0, 0x2b0000},
		{1, 0x330000},
	},
};

static const struct imx95_dpu_devdata imx952_dpu_devdata = {
	.clock_ctrl = 0x04,
	.shden = BIT(0),
	.plane_association = 0x18,
	.fetchlayer = {
		{0, 0x1c0000, 0x19 },
		{1, 0x1d0000, 0x1a },
	},
	.domainblend = {
		{0, 0x2a0000},
		{1, 0x330000},
	},
	.framegen = {
		{0, 0x2b0000},
		{1, 0x340000},
	},
};

static const struct udevice_id imx95_dpu_ids[] = {
	{ .compatible = "nxp,imx95-dpu", .data = (ulong)&imx95_dpu_devdata, },
	{ .compatible = "nxp,imx952-dpu", .data = (ulong)&imx952_dpu_devdata, },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(imx95_dpu) = {
	.name	= "imx95_dpu",
	.id	= UCLASS_VIDEO,
	.of_match = imx95_dpu_ids,
	.bind	= imx95_dpu_bind,
	.probe	= imx95_dpu_probe,
	.remove = imx95_dpu_remove,
	.flags	= DM_FLAG_PRE_RELOC,
	.priv_auto	= sizeof(struct imx95_dpu_priv),
};
