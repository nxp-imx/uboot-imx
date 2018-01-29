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
#include <i2c.h>
#include <asm/arch/sys_proto.h>

#include <video_fb.h>
#include <asm/arch/video_common.h>
#include <power-domain.h>
#include <imx8_hdmi.h>

DECLARE_GLOBAL_DATA_PTR;

/* test congiurations */
#undef IMXDCSS_LOAD_HDMI_FIRMWARE
#undef IMXDCSS_SET_PIXEL_CLOCK

static struct video_mode_settings gmode;
static uint32_t gpixfmt;
GraphicDevice panel;
struct video_mode_settings *imx8m_get_gmode(void)
{
	return &gmode;
}

void imx8m_show_gmode(void)
{
	printf("gmode =\n"
	       "pixelclock = %u\n"
	       "xres       = %u\n"
	       "yres       = %u\n"
	       "hfp        = %u\n"
	       "hbp        = %u\n"
	       "vfp        = %u\n"
	       "vbp        = %u\n"
	       "hsync      = %u\n"
	       "vsync      = %u\n"
	       "hpol       = %u\n"
	       "vpol       = %u\n",
	       gmode.pixelclock,
	       gmode.xres,
	       gmode.yres,
	       gmode.hfp,
	       gmode.hbp,
	       gmode.vfp,
	       gmode.vbp, gmode.hsync, gmode.vsync, gmode.hpol, gmode.vpol);
}

GraphicDevice *imx8m_get_gd(void)
{
	return &panel;
}

#define REG_BASE_ADDR 0x32e00000UL

/*#define DEBUGREG*/
#ifdef DEBUGREG
#define reg32_write(addr, val) \
do { \
	debug("%s():%d 0x%08x -> 0x%08x\n", __func__, __LINE__, \
	(unsigned int)addr, (unsigned int)val); \
	__raw_writel(val, addr); \
} while (0)
#else
#define reg32_write(addr, val) __raw_writel(val, addr)
#endif

#define reg32_read(addr) __raw_readl(addr)

#define reg32setbit(addr, bitpos) \
	reg32_write((addr), (reg32_read((addr)) | (1<<(bitpos))))
#define reg32clearbit(addr, bitpos) \
	reg32_write((addr), (reg32_read((addr)) & ~(1<<(bitpos))))

#define reg32_read_tst(addr, val, mask) \
do { \
	u32 temp = reg32_read((addr)); \
	if ((temp & (mask)) == ((val) & (mask))) \
		debug("%s():%d 0x%08x -> 0x%08x\n", \
			__func__, __LINE__, addr, val); \
	else  \
		debug("%s():%d 0x%08x -> 0x%08x instead of 0x%08x\n", \
			__func__, __LINE__, addr, temp, val); \
} while (0)

#define COLOR_LIST_SIZE 8
static u32 color_list_argb32[COLOR_LIST_SIZE] = {
	0xFFFFFFFF,		/* white */
	0xFFFF0000,		/* red */
	0xFF00FF00,		/* green */
	0xFF0000FF,		/* blue */
	0xFFFFFF00,		/* yellow */
	0xFF00FFFF,		/* cyan */
	0xFFFF00FF,		/* magenta */
	0xFFC1C2C3,		/* silver */
};				/*AARRGGBB */

static unsigned int get_color_index(unsigned short px, unsigned short py,
				    unsigned short width, unsigned short height,
				    unsigned short bar_size)
{
	const int mw = 5;	/* margin width */
	if ((py >= 0 && py < mw) || (py >= height - mw && py < height) ||
	    (px >= 0 && px < mw) || (px >= width - mw && px < width)) {
		return 1;
	}

	return py / bar_size;
}

void imx8m_create_color_bar(void *start_address,
			   struct video_mode_settings *vms)
{
	/*struct video_mode_settings *vms = &vm_settings[g_vm]; */
	uint32_t color_bar_size = vms->yres / COLOR_LIST_SIZE;
	int i, j;
	u32 *pointer;
	int color_index = 0;
	pointer = (u32 *)start_address;
	uint32_t *color_map = &color_list_argb32[0];
	debug("%s(), %d: start_address %p\n",
	      __func__, __LINE__, start_address);
	debug("%s(), %d: pointer %p\n", __func__, __LINE__, pointer);
	debug("%s(), %d x %d\n", __func__, vms->xres, vms->yres);
	for (i = 0; i < vms->yres; i++) {
		for (j = 0; j < vms->xres; j++) {
			color_index = get_color_index(j, i, vms->xres,
						      vms->yres,
						      color_bar_size);
			*pointer = color_map[color_index];
			pointer++;
		}
	}
	invalidate_dcache_all();
}

static void imx8m_set_clocks(int apb_clk, int b_clk, int hdmi_core_clk,
		      int p_clk, int rtr_clk)
{
	if (b_clk == 800) {
		/* b_clk: bus_clk_root(4) sel 2nd input source and
		   pre_div to 0; output should be 800M */
		reg32_write(CCM_BUS_CLK_ROOT_GEN_TAGET_CLR(4),
			    (0x7 << 24) | (0x7 << 16));
		reg32_write(CCM_BUS_CLK_ROOT_GEN_TAGET_SET(4), (0x2 << 24));
	} else {
		printf("b_clk does not match a supported frequency");
	}
	if (rtr_clk == 400) {
		/* rtr_clk: bus_clk_root(6) sel 1st input source
		   and pre_div to 1; output should be 400M */
		reg32_write(CCM_BUS_CLK_ROOT_GEN_TAGET_CLR(6),
			    (0x7 << 24) | (0x7 << 16));
		reg32_write(CCM_BUS_CLK_ROOT_GEN_TAGET_SET(6),
			    (0x1 << 24) | (0x1 << 16));
	} else {
		debug("rtr_clk does not match a supported frequency");
	}

#ifdef IMXDCSS_LOAD_HDMI_FIRMWARE
	/* If ROM is loading HDMI firmware then this clock should not be set */
	if (hdmi_core_clk == 200) {
		/* hdmi_core_clk: ip_clk_root(69) sel 1st input source and
		   pre_div to 0 */
		reg32_write(CCM_IP_CLK_ROOT_GEN_TAGET_CLR(69),
			    (0x7 << 24) | (0x7 << 16));
		reg32_write(CCM_IP_CLK_ROOT_GEN_TAGET_SET(69), (0x1 << 24));
		g_hdmi_core_clock = 200000000;
	} else {
		debug("hdmi_core_clk does not match a supported frequency");
	}
#endif

#ifdef IMXDCSS_SET_PIXEL_CLOCK
	/* This would be needed for MIPI-DSI DCSS display */
	if (p_clk == 27) {
		/* p_clk: ip_clk_root(9) sel 1st input source and
		   pre_div to 1; post_div to 5, output 100M */
		reg32_write(CCM_IP_CLK_ROOT_GEN_TAGET_CLR(9),
			    (0x7 << 24) | (0x7 << 16));
		reg32_write(CCM_IP_CLK_ROOT_GEN_TAGET_SET(9),
			    (0x1 << 24) | (29 << 16));
	} else if (p_clk == 100) {
		/* p_clk: ip_clk_root(9) sel 1st input source and
		   pre_div to 1; post_div to 5, output 100M */
		reg32_write(CCM_IP_CLK_ROOT_GEN_TAGET_CLR(9),
			    (0x7 << 24) | (0x7 << 16));
		reg32_write(CCM_IP_CLK_ROOT_GEN_TAGET_SET(9),
			    (0x1 << 24) | (0x5 << 16));
	} else if (p_clk == 120) {
		/* p_clk: ip_clk_root(9) sel 1st input source and
		   pre_div to 1; post_div to 4, output 120M */
		reg32_write(CCM_IP_CLK_ROOT_GEN_TAGET_CLR(9),
			    (0x7 << 24) | (0x7 << 16));
		reg32_write(CCM_IP_CLK_ROOT_GEN_TAGET_SET(9),
			    (0x1 << 24) | (0x4 << 16));
	} else if (p_clk == 200) {
		/* I added this to speed up the pixel clock and
		   get frames out faster. may need to adjust this.
		 */
		reg32_write(CCM_IP_CLK_ROOT_GEN_TAGET_CLR(9),
			    (0x7 << 24) | (0x7 << 16));
		reg32_write(CCM_IP_CLK_ROOT_GEN_TAGET_SET(9),
			    (0x4 << 24) | (0x3 << 16)); /*for emu use 800 / 4 */
	} else if (p_clk == 400) {
		/* I added this to speed up the pixel clock and
		   get frames out faster. may need to adjust this.
		 */
		reg32_write(CCM_IP_CLK_ROOT_GEN_TAGET_CLR(9),
			    (0x7 << 24) | (0x7 << 16));
		reg32_write(CCM_IP_CLK_ROOT_GEN_TAGET_SET(9),
			    (0x4 << 24) | (0x1 << 16)); /*for emu use 800 / 2 */
	} else if (p_clk == 40) {	/* Do not reprogram, will get 40MHz */
	} else {
		debug("p_clk does not match a supported frequency");
	}
#endif
}

static int imx8m_power_init(uint32_t clock_control)
{
	u32 temp;
	/*struct video_mode_settings *vms = &vm_settings[g_vm]; */

	debug("\nenabling display clock...\n");
	clock_enable(CCGR_DISPLAY, 1);

	reg32_write(0x303A00EC, 0x0000ffff);	/*PGC_CPU_MAPPING */
	reg32setbit(0x303A00F8, 10);	/*PU_PGC_SW_PUP_REQ : disp was 10 */
#ifdef LOAD_HDMI_FIRMWARE
	reg32setbit(0x303A00F8, 9);	/*PU_PGC_SW_PUP_REQ : hdmi was 9 */
#endif
	imx8m_set_clocks(133, 800, 200, 27, 400);

	/* DCSS reset */
	reg32_write(0x32e2f000, 0xffffffff);

	/* DCSS clock selection */
	reg32_write(0x32e2f010, clock_control);
	temp = reg32_read(0x32e2f010);
	debug("%s(): DCSS clock control 0x%08x\n", __func__, temp);

	/* take DCSS out of reset - not needed OFB */
	/*__raw_writel(0xffffffff, 0x32e2f004); */

	return 0;
}

static void imx8m_display_init(u64 buffer, int encoding,
			       struct video_mode_settings *vms)
{
	/*struct video_mode_settings *vms = &vm_settings[g_vm]; */

	debug("entering %s() ...\n", __func__);
	debug("%s() buffer ...\n", __func__);

	/* DTRC-CHAN2/3 */
	reg32_write(REG_BASE_ADDR + 0x160c8, 0x00000002);
	reg32_write(REG_BASE_ADDR + 0x170c8, 0x00000002);

	/* CHAN1_DPR */
	reg32_write(REG_BASE_ADDR + 0x180c0, (unsigned int)buffer);
	reg32_write(REG_BASE_ADDR + 0x18090, 0x00000002);
	reg32_write(REG_BASE_ADDR + 0x180a0, vms->xres);
	reg32_write(REG_BASE_ADDR + 0x180b0, vms->yres);
	reg32_write(REG_BASE_ADDR + 0x18110,
		    (unsigned int)buffer + vms->xres * vms->yres);
	reg32_write(REG_BASE_ADDR + 0x180f0, 0x00000280);
	reg32_write(REG_BASE_ADDR + 0x18100, 0x000000f0);
	reg32_write(REG_BASE_ADDR + 0x18070, ((vms->xres * 4) << 16));
	reg32_write(REG_BASE_ADDR + 0x18050, 0x000e4203);
	reg32_write(REG_BASE_ADDR + 0x18050, 0x000e4203);
	reg32_write(REG_BASE_ADDR + 0x18200, 0x00000038);
	reg32_write(REG_BASE_ADDR + 0x18000, 0x00000004);
	reg32_write(REG_BASE_ADDR + 0x18000, 0x00000005);

	/* SCALER */
	reg32_write(REG_BASE_ADDR + 0x1c008, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c00c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c010, 0x00000002);
	reg32_write(REG_BASE_ADDR + 0x1c014, 0x00000002);
	reg32_write(REG_BASE_ADDR + 0x1c018,
		    ((vms->yres - 1) << 16 | (vms->xres - 1)));
	reg32_write(REG_BASE_ADDR + 0x1c01c,
		    ((vms->yres - 1) << 16 | (vms->xres - 1)));
	reg32_write(REG_BASE_ADDR + 0x1c020,
		    ((vms->yres - 1) << 16 | (vms->xres - 1)));
	reg32_write(REG_BASE_ADDR + 0x1c024,
		    ((vms->yres - 1) << 16 | (vms->xres - 1)));
	reg32_write(REG_BASE_ADDR + 0x1c028, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c02c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c030, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c034, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c038, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c03c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c040, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c044, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c048, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c04c, 0x00002000);
	reg32_write(REG_BASE_ADDR + 0x1c050, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c054, 0x00002000);
	reg32_write(REG_BASE_ADDR + 0x1c058, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c05c, 0x00002000);
	reg32_write(REG_BASE_ADDR + 0x1c060, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c064, 0x00002000);
	reg32_write(REG_BASE_ADDR + 0x1c080, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c0c0, 0x00040000);
	reg32_write(REG_BASE_ADDR + 0x1c100, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c084, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c0c4, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c104, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c088, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c0c8, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c108, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c08c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c0cc, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c10c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c090, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c0d0, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c110, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c094, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c0d4, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c114, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c098, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c0d8, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c118, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c09c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c0dc, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c11c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c0a0, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c0e0, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c120, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c0a4, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c0e4, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c124, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c0a8, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c0e8, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c128, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c0ac, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c0ec, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c12c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c0b0, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c0f0, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c130, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c0b4, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c0f4, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c134, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c0b8, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c0f8, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c138, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c0bc, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c0fc, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c13c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c140, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c180, 0x00040000);
	reg32_write(REG_BASE_ADDR + 0x1c1c0, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c144, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c184, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c1c4, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c148, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c188, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c1c8, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c14c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c18c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c1cc, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c150, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c190, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c1d0, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c154, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c194, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c1d4, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c158, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c198, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c1d8, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c15c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c19c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c1dc, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c160, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c1a0, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c1e0, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c164, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c1a4, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c1e4, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c168, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c1a8, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c1e8, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c16c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c1ac, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c1ec, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c170, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c1b0, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c1f0, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c174, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c1b4, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c1f4, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c178, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c1b8, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c1f8, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c17c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c1bc, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c1fc, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c300, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c340, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c380, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c304, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c344, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c384, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c308, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c348, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c388, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c30c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c34c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c38c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c310, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c350, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c390, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c314, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c354, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c394, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c318, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c358, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c398, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c31c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c35c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c39c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c320, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c360, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c3a0, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c324, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c364, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c3a4, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c328, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c368, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c3a8, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c32c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c36c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c3ac, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c330, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c370, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c3b0, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c334, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c374, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c3b4, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c338, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c378, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c3b8, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c33c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c37c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c3bc, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c200, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c240, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c280, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c204, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c244, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c284, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c208, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c248, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c288, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c20c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c24c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c28c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c210, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c250, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c290, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c214, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c254, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c294, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c218, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c258, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c298, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c21c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c25c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c29c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c220, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c260, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c2a0, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c224, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c264, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c2a4, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c228, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c268, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c2a8, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c22c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c26c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c2ac, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c230, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c270, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c2b0, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c234, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c274, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c2b4, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c238, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c278, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c2b8, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c23c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c27c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c2bc, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c2bc, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x1c000, 0x00000011);

	/* SUBSAM */
	reg32_write(REG_BASE_ADDR + 0x1b070, 0x21612161);
	reg32_write(REG_BASE_ADDR + 0x1b080, 0x03ff0000);
	reg32_write(REG_BASE_ADDR + 0x1b090, 0x03ff0000);

	reg32_write(REG_BASE_ADDR + 0x1b010,
		    (((vms->vfp + vms->vbp + vms->vsync + vms->yres -
		       1) << 16) | (vms->hfp + vms->hbp + vms->hsync +
				    vms->xres - 1)));
	reg32_write(REG_BASE_ADDR + 0x1b020,
		    (((vms->hsync - 1) << 16) | vms->hpol << 31 | (vms->hfp +
								   vms->hbp +
								   vms->hsync +
								   vms->xres -
								   1)));
	reg32_write(REG_BASE_ADDR + 0x1b030,
		    (((vms->vfp + vms->vsync -
		       1) << 16) | vms->vpol << 31 | (vms->vfp - 1)));

	reg32_write(REG_BASE_ADDR + 0x1b040,
		    ((1 << 31) | ((vms->vsync + vms->vfp + vms->vbp) << 16) |
		     (vms->hsync + vms->hbp - 1)));
	reg32_write(REG_BASE_ADDR + 0x1b050,
		    (((vms->vsync + vms->vfp + vms->vbp + vms->yres -
		       1) << 16) | (vms->hsync + vms->hbp + vms->xres - 1)));

	/* subsample mode 0 none, 1 422, 2 420 */
	switch (encoding) {
	case 4:
		reg32_write(REG_BASE_ADDR + 0x1b060, 0x00000001);
		break;

	case 8:
		reg32_write(REG_BASE_ADDR + 0x1b060, 0x00000002);
		break;

	case 2:
	case 1:
	default:
		reg32_write(REG_BASE_ADDR + 0x1b060, 0x0000000);
	}

	reg32_write(REG_BASE_ADDR + 0x1b000, 0x00000001);
#if 0
	/* not needed for splash setup */
	/* HDR10 Chan3 LUT */
	reg32_write(REG_BASE_ADDR + 0x03874, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x03080, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x03000, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x03800, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x07874, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x07080, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x07000, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x07800, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x0b874, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x0b080, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x0b000, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x0b800, 0x00000000);

	/* HDR10 Tables and Registers */
	/*reg32_write(REG_BASE_ADDR+0x0f074, 0x00000003); */
	/*reg32_write(REG_BASE_ADDR+0x0f000, 0x00000004); */

	reg32_write(REG_BASE_ADDR + 0x0f004, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x0f008, 0x00000001);
	reg32_write(REG_BASE_ADDR + 0x0f00c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x0f010, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x0f014, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x0f018, 0x00000001);
	reg32_write(REG_BASE_ADDR + 0x0f01c, 0x00000001);
	reg32_write(REG_BASE_ADDR + 0x0f020, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x0f024, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x0f028, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x0f02c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x0f030, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x0f034, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x0f038, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x0f03c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x0f040, 0xffffffff);
	reg32_write(REG_BASE_ADDR + 0x0f044, 0xffffffff);
	reg32_write(REG_BASE_ADDR + 0x0f048, 0xffffffff);
	reg32_write(REG_BASE_ADDR + 0x0f04c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x0f050, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x0f054, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x0f058, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x0f05c, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x0f060, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x0f064, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x0f068, 0xffffffff);
	reg32_write(REG_BASE_ADDR + 0x0f06c, 0xffffffff);
	reg32_write(REG_BASE_ADDR + 0x0f070, 0xffffffff);
	reg32_write(REG_BASE_ADDR + 0x0f074, 0x00000000);
	reg32_write(REG_BASE_ADDR + 0x0f000, 0x00000003);
#endif
	/* DTG */
	/*reg32_write(REG_BASE_ADDR + 0x20000, 0xff000484); */
	/* disable local alpha */
	reg32_write(REG_BASE_ADDR + 0x20000, 0xff005084);
	reg32_write(REG_BASE_ADDR + 0x20004,
		    (((vms->vfp + vms->vbp + vms->vsync + vms->yres -
		       1) << 16) | (vms->hfp + vms->hbp + vms->hsync +
				    vms->xres - 1)));
	reg32_write(REG_BASE_ADDR + 0x20008,
		    (((vms->vsync + vms->vfp + vms->vbp -
		       1) << 16) | (vms->hsync + vms->hbp - 1)));
	reg32_write(REG_BASE_ADDR + 0x2000c,
		    (((vms->vsync + vms->vfp + vms->vbp + vms->yres -
		       1) << 16) | (vms->hsync + vms->hbp + vms->xres - 1)));
	reg32_write(REG_BASE_ADDR + 0x20010,
		    (((vms->vsync + vms->vfp + vms->vbp -
		       1) << 16) | (vms->hsync + vms->hbp - 1)));
	reg32_write(REG_BASE_ADDR + 0x20014,
		    (((vms->vsync + vms->vfp + vms->vbp + vms->yres -
		       1) << 16) | (vms->hsync + vms->hbp + vms->xres - 1)));
	reg32_write(REG_BASE_ADDR + 0x20028, 0x000b000a);

	/* disable local alpha */
	reg32_write(REG_BASE_ADDR + 0x20000, 0xff005184);

	debug("leaving %s() ...\n", __func__);
}

void imx8m_display_shutdown(void)
{
	/* stop the DCSS modules in use */
	/* dtg */
	reg32_write(REG_BASE_ADDR + 0x20000, 0);
	/* scaler */
	reg32_write(REG_BASE_ADDR + 0x1c000, 0);
	reg32_write(REG_BASE_ADDR + 0x1c400, 0);
	reg32_write(REG_BASE_ADDR + 0x1c800, 0);
	/* dpr */
	reg32_write(REG_BASE_ADDR + 0x18000, 0);
	reg32_write(REG_BASE_ADDR + 0x19000, 0);
	reg32_write(REG_BASE_ADDR + 0x1a000, 0);
	/* sub-sampler*/
	reg32_write(REG_BASE_ADDR + 0x1b000, 0);
#if 0
	/* reset the DCSS */
	reg32_write(0x32e2f000, 0xffffe8);
	udelay(100);
	reg32_write(0x32e2f000, 0xffffff);
#endif

}
void *video_hw_init(void)
{
	void *fb;
	int encoding = 1;

	debug("%s()\n", __func__);

	imx8m_power_init(0x1);

	panel.winSizeX = gmode.xres;
	panel.winSizeY = gmode.yres;
	panel.plnSizeX = gmode.xres;
	panel.plnSizeY = gmode.yres;
	panel.gdfBytesPP = 4;
	panel.gdfIndex = GDF_32BIT_X888RGB;
	panel.memSize = gmode.xres * gmode.yres * panel.gdfBytesPP;

	/* Allocate framebuffer */
	fb = memalign(0x1000, roundup(panel.memSize, 0x1000));
	debug("%s(): fb %p\n", __func__, fb);
	if (!fb) {
		printf("%s, %s(): Error allocating framebuffer!\n",
		       __FILE__, __func__);
		return NULL;
	}

	imx8m_create_color_bar((void *)((uint64_t) fb), &gmode);

	imx8_hdmi_enable(encoding, &gmode); /* may change gmode */

	/* start dccs */
	imx8m_display_init((uint64_t) fb, encoding, &gmode);

	panel.frameAdrs = (ulong) fb;
	debug("IMXDCSS display started ...\n");

	return &panel;
}

void imx8m_fb_disable(void)
{
	debug("%s()\n", __func__);
	if (panel.frameAdrs) {
#ifdef CONFIG_VIDEO_IMX8_HDMI
		imx8_hdmi_disable();
#endif
		imx8m_display_shutdown();
	}

}

int imx8m_fb_init(struct fb_videomode const *mode,
		    uint8_t disp, uint32_t pixfmt)
{
	debug("entering %s()\n", __func__);

	if (disp > 1) {
		debug("Invalid disp parameter %d for imxdcss_fb_init()\n",
		      disp);
		return -EINVAL;
	}

	memset(&gmode, 0, sizeof(struct video_mode_settings));
	gmode.pixelclock = PS2KHZ(mode->pixclock) * 1000;
	gmode.xres = mode->xres;
	gmode.hbp = mode->left_margin;
	gmode.hfp = mode->right_margin;

	gmode.yres = mode->yres;
	gmode.vbp = mode->upper_margin;
	gmode.vfp = mode->lower_margin;

	gmode.hsync = mode->hsync_len;
	gmode.vsync = mode->vsync_len;
	gmode.hpol = (mode->flag & FB_SYNC_HOR_HIGH_ACT) ? 1 : 0;
	gmode.vpol = (mode->flag & FB_SYNC_VERT_HIGH_ACT) ? 1 : 0;
	gpixfmt = pixfmt;

	debug("leaving %s()\n", __func__);

	return 0;
}
