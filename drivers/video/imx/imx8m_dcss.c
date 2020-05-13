// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 *
 */
#include <common.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <env.h>
#include <linux/errno.h>
#include <malloc.h>
#include <video.h>
#include <video_fb.h>
#include <display.h>

#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/sys_proto.h>
#include <asm/io.h>
#include <video_bridge.h>
#include <clk.h>
#include <video_link.h>

#ifdef DEBUG
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


struct imx8m_dcss_priv {
	struct udevice *disp_dev;
	struct display_timing timings;

	bool hpol;		/* horizontal pulse polarity	*/
	bool vpol;		/* vertical pulse polarity	*/
	bool enabled;

	fdt_addr_t addr;
};

__weak int imx8m_dcss_clock_init(u32 pixclk)
{
	return 0;
}

__weak int imx8m_dcss_power_init(void)
{
	return 0;
}

static void imx8m_dcss_reset(struct udevice *dev)
{
	struct imx8m_dcss_priv *priv = dev_get_priv(dev);
	u32 temp;

	/* DCSS reset */
	reg32_write(priv->addr + 0x2f000, 0xffffffff);

	/* DCSS clock selection */
	reg32_write(priv->addr + 0x2f010, 0x1);
	temp = reg32_read(priv->addr + 0x2f010);
	debug("%s(): DCSS clock control 0x%08x\n", __func__, temp);
}

static void imx8m_dcss_init(struct udevice *dev)
{
	struct imx8m_dcss_priv *priv = dev_get_priv(dev);
	struct video_uc_platdata *plat = dev_get_uclass_platdata(dev);

	debug("%s() ...\n", __func__);

	/* DTRC-CHAN2/3 */
	reg32_write(priv->addr + 0x160c8, 0x00000002);
	reg32_write(priv->addr + 0x170c8, 0x00000002);

	/* CHAN1_DPR */
	reg32_write(priv->addr + 0x180c0, (unsigned int)plat->base);
	reg32_write(priv->addr + 0x18090, 0x00000002);
	reg32_write(priv->addr + 0x180a0, priv->timings.hactive.typ);
	reg32_write(priv->addr + 0x180b0, priv->timings.vactive.typ);
	reg32_write(priv->addr + 0x18110,
		    (unsigned int)plat->base + priv->timings.hactive.typ * priv->timings.vactive.typ);
	reg32_write(priv->addr + 0x180f0, 0x00000280);
	reg32_write(priv->addr + 0x18100, 0x000000f0);
	reg32_write(priv->addr + 0x18070, ((priv->timings.hactive.typ * 4) << 16));
	reg32_write(priv->addr + 0x18050, 0x000e4203);
	reg32_write(priv->addr + 0x18050, 0x000e4203);
	reg32_write(priv->addr + 0x18200, 0x00000038);
	reg32_write(priv->addr + 0x18000, 0x00000004);
	reg32_write(priv->addr + 0x18000, 0x00000005);

	/* SCALER */
	reg32_write(priv->addr + 0x1c008, 0x00000000);
	reg32_write(priv->addr + 0x1c00c, 0x00000000);
	reg32_write(priv->addr + 0x1c010, 0x00000002);
	reg32_write(priv->addr + 0x1c014, 0x00000002);
	reg32_write(priv->addr + 0x1c018,
		    ((priv->timings.vactive.typ - 1) << 16 | (priv->timings.hactive.typ - 1)));
	reg32_write(priv->addr + 0x1c01c,
		    ((priv->timings.vactive.typ - 1) << 16 | (priv->timings.hactive.typ - 1)));
	reg32_write(priv->addr + 0x1c020,
		    ((priv->timings.vactive.typ - 1) << 16 | (priv->timings.hactive.typ - 1)));
	reg32_write(priv->addr + 0x1c024,
		    ((priv->timings.vactive.typ - 1) << 16 | (priv->timings.hactive.typ - 1)));
	reg32_write(priv->addr + 0x1c028, 0x00000000);
	reg32_write(priv->addr + 0x1c02c, 0x00000000);
	reg32_write(priv->addr + 0x1c030, 0x00000000);
	reg32_write(priv->addr + 0x1c034, 0x00000000);
	reg32_write(priv->addr + 0x1c038, 0x00000000);
	reg32_write(priv->addr + 0x1c03c, 0x00000000);
	reg32_write(priv->addr + 0x1c040, 0x00000000);
	reg32_write(priv->addr + 0x1c044, 0x00000000);
	reg32_write(priv->addr + 0x1c048, 0x00000000);
	reg32_write(priv->addr + 0x1c04c, 0x00002000);
	reg32_write(priv->addr + 0x1c050, 0x00000000);
	reg32_write(priv->addr + 0x1c054, 0x00002000);
	reg32_write(priv->addr + 0x1c058, 0x00000000);
	reg32_write(priv->addr + 0x1c05c, 0x00002000);
	reg32_write(priv->addr + 0x1c060, 0x00000000);
	reg32_write(priv->addr + 0x1c064, 0x00002000);
	reg32_write(priv->addr + 0x1c080, 0x00000000);
	reg32_write(priv->addr + 0x1c0c0, 0x00040000);
	reg32_write(priv->addr + 0x1c100, 0x00000000);
	reg32_write(priv->addr + 0x1c084, 0x00000000);
	reg32_write(priv->addr + 0x1c0c4, 0x00000000);
	reg32_write(priv->addr + 0x1c104, 0x00000000);
	reg32_write(priv->addr + 0x1c088, 0x00000000);
	reg32_write(priv->addr + 0x1c0c8, 0x00000000);
	reg32_write(priv->addr + 0x1c108, 0x00000000);
	reg32_write(priv->addr + 0x1c08c, 0x00000000);
	reg32_write(priv->addr + 0x1c0cc, 0x00000000);
	reg32_write(priv->addr + 0x1c10c, 0x00000000);
	reg32_write(priv->addr + 0x1c090, 0x00000000);
	reg32_write(priv->addr + 0x1c0d0, 0x00000000);
	reg32_write(priv->addr + 0x1c110, 0x00000000);
	reg32_write(priv->addr + 0x1c094, 0x00000000);
	reg32_write(priv->addr + 0x1c0d4, 0x00000000);
	reg32_write(priv->addr + 0x1c114, 0x00000000);
	reg32_write(priv->addr + 0x1c098, 0x00000000);
	reg32_write(priv->addr + 0x1c0d8, 0x00000000);
	reg32_write(priv->addr + 0x1c118, 0x00000000);
	reg32_write(priv->addr + 0x1c09c, 0x00000000);
	reg32_write(priv->addr + 0x1c0dc, 0x00000000);
	reg32_write(priv->addr + 0x1c11c, 0x00000000);
	reg32_write(priv->addr + 0x1c0a0, 0x00000000);
	reg32_write(priv->addr + 0x1c0e0, 0x00000000);
	reg32_write(priv->addr + 0x1c120, 0x00000000);
	reg32_write(priv->addr + 0x1c0a4, 0x00000000);
	reg32_write(priv->addr + 0x1c0e4, 0x00000000);
	reg32_write(priv->addr + 0x1c124, 0x00000000);
	reg32_write(priv->addr + 0x1c0a8, 0x00000000);
	reg32_write(priv->addr + 0x1c0e8, 0x00000000);
	reg32_write(priv->addr + 0x1c128, 0x00000000);
	reg32_write(priv->addr + 0x1c0ac, 0x00000000);
	reg32_write(priv->addr + 0x1c0ec, 0x00000000);
	reg32_write(priv->addr + 0x1c12c, 0x00000000);
	reg32_write(priv->addr + 0x1c0b0, 0x00000000);
	reg32_write(priv->addr + 0x1c0f0, 0x00000000);
	reg32_write(priv->addr + 0x1c130, 0x00000000);
	reg32_write(priv->addr + 0x1c0b4, 0x00000000);
	reg32_write(priv->addr + 0x1c0f4, 0x00000000);
	reg32_write(priv->addr + 0x1c134, 0x00000000);
	reg32_write(priv->addr + 0x1c0b8, 0x00000000);
	reg32_write(priv->addr + 0x1c0f8, 0x00000000);
	reg32_write(priv->addr + 0x1c138, 0x00000000);
	reg32_write(priv->addr + 0x1c0bc, 0x00000000);
	reg32_write(priv->addr + 0x1c0fc, 0x00000000);
	reg32_write(priv->addr + 0x1c13c, 0x00000000);
	reg32_write(priv->addr + 0x1c140, 0x00000000);
	reg32_write(priv->addr + 0x1c180, 0x00040000);
	reg32_write(priv->addr + 0x1c1c0, 0x00000000);
	reg32_write(priv->addr + 0x1c144, 0x00000000);
	reg32_write(priv->addr + 0x1c184, 0x00000000);
	reg32_write(priv->addr + 0x1c1c4, 0x00000000);
	reg32_write(priv->addr + 0x1c148, 0x00000000);
	reg32_write(priv->addr + 0x1c188, 0x00000000);
	reg32_write(priv->addr + 0x1c1c8, 0x00000000);
	reg32_write(priv->addr + 0x1c14c, 0x00000000);
	reg32_write(priv->addr + 0x1c18c, 0x00000000);
	reg32_write(priv->addr + 0x1c1cc, 0x00000000);
	reg32_write(priv->addr + 0x1c150, 0x00000000);
	reg32_write(priv->addr + 0x1c190, 0x00000000);
	reg32_write(priv->addr + 0x1c1d0, 0x00000000);
	reg32_write(priv->addr + 0x1c154, 0x00000000);
	reg32_write(priv->addr + 0x1c194, 0x00000000);
	reg32_write(priv->addr + 0x1c1d4, 0x00000000);
	reg32_write(priv->addr + 0x1c158, 0x00000000);
	reg32_write(priv->addr + 0x1c198, 0x00000000);
	reg32_write(priv->addr + 0x1c1d8, 0x00000000);
	reg32_write(priv->addr + 0x1c15c, 0x00000000);
	reg32_write(priv->addr + 0x1c19c, 0x00000000);
	reg32_write(priv->addr + 0x1c1dc, 0x00000000);
	reg32_write(priv->addr + 0x1c160, 0x00000000);
	reg32_write(priv->addr + 0x1c1a0, 0x00000000);
	reg32_write(priv->addr + 0x1c1e0, 0x00000000);
	reg32_write(priv->addr + 0x1c164, 0x00000000);
	reg32_write(priv->addr + 0x1c1a4, 0x00000000);
	reg32_write(priv->addr + 0x1c1e4, 0x00000000);
	reg32_write(priv->addr + 0x1c168, 0x00000000);
	reg32_write(priv->addr + 0x1c1a8, 0x00000000);
	reg32_write(priv->addr + 0x1c1e8, 0x00000000);
	reg32_write(priv->addr + 0x1c16c, 0x00000000);
	reg32_write(priv->addr + 0x1c1ac, 0x00000000);
	reg32_write(priv->addr + 0x1c1ec, 0x00000000);
	reg32_write(priv->addr + 0x1c170, 0x00000000);
	reg32_write(priv->addr + 0x1c1b0, 0x00000000);
	reg32_write(priv->addr + 0x1c1f0, 0x00000000);
	reg32_write(priv->addr + 0x1c174, 0x00000000);
	reg32_write(priv->addr + 0x1c1b4, 0x00000000);
	reg32_write(priv->addr + 0x1c1f4, 0x00000000);
	reg32_write(priv->addr + 0x1c178, 0x00000000);
	reg32_write(priv->addr + 0x1c1b8, 0x00000000);
	reg32_write(priv->addr + 0x1c1f8, 0x00000000);
	reg32_write(priv->addr + 0x1c17c, 0x00000000);
	reg32_write(priv->addr + 0x1c1bc, 0x00000000);
	reg32_write(priv->addr + 0x1c1fc, 0x00000000);
	reg32_write(priv->addr + 0x1c300, 0x00000000);
	reg32_write(priv->addr + 0x1c340, 0x00000000);
	reg32_write(priv->addr + 0x1c380, 0x00000000);
	reg32_write(priv->addr + 0x1c304, 0x00000000);
	reg32_write(priv->addr + 0x1c344, 0x00000000);
	reg32_write(priv->addr + 0x1c384, 0x00000000);
	reg32_write(priv->addr + 0x1c308, 0x00000000);
	reg32_write(priv->addr + 0x1c348, 0x00000000);
	reg32_write(priv->addr + 0x1c388, 0x00000000);
	reg32_write(priv->addr + 0x1c30c, 0x00000000);
	reg32_write(priv->addr + 0x1c34c, 0x00000000);
	reg32_write(priv->addr + 0x1c38c, 0x00000000);
	reg32_write(priv->addr + 0x1c310, 0x00000000);
	reg32_write(priv->addr + 0x1c350, 0x00000000);
	reg32_write(priv->addr + 0x1c390, 0x00000000);
	reg32_write(priv->addr + 0x1c314, 0x00000000);
	reg32_write(priv->addr + 0x1c354, 0x00000000);
	reg32_write(priv->addr + 0x1c394, 0x00000000);
	reg32_write(priv->addr + 0x1c318, 0x00000000);
	reg32_write(priv->addr + 0x1c358, 0x00000000);
	reg32_write(priv->addr + 0x1c398, 0x00000000);
	reg32_write(priv->addr + 0x1c31c, 0x00000000);
	reg32_write(priv->addr + 0x1c35c, 0x00000000);
	reg32_write(priv->addr + 0x1c39c, 0x00000000);
	reg32_write(priv->addr + 0x1c320, 0x00000000);
	reg32_write(priv->addr + 0x1c360, 0x00000000);
	reg32_write(priv->addr + 0x1c3a0, 0x00000000);
	reg32_write(priv->addr + 0x1c324, 0x00000000);
	reg32_write(priv->addr + 0x1c364, 0x00000000);
	reg32_write(priv->addr + 0x1c3a4, 0x00000000);
	reg32_write(priv->addr + 0x1c328, 0x00000000);
	reg32_write(priv->addr + 0x1c368, 0x00000000);
	reg32_write(priv->addr + 0x1c3a8, 0x00000000);
	reg32_write(priv->addr + 0x1c32c, 0x00000000);
	reg32_write(priv->addr + 0x1c36c, 0x00000000);
	reg32_write(priv->addr + 0x1c3ac, 0x00000000);
	reg32_write(priv->addr + 0x1c330, 0x00000000);
	reg32_write(priv->addr + 0x1c370, 0x00000000);
	reg32_write(priv->addr + 0x1c3b0, 0x00000000);
	reg32_write(priv->addr + 0x1c334, 0x00000000);
	reg32_write(priv->addr + 0x1c374, 0x00000000);
	reg32_write(priv->addr + 0x1c3b4, 0x00000000);
	reg32_write(priv->addr + 0x1c338, 0x00000000);
	reg32_write(priv->addr + 0x1c378, 0x00000000);
	reg32_write(priv->addr + 0x1c3b8, 0x00000000);
	reg32_write(priv->addr + 0x1c33c, 0x00000000);
	reg32_write(priv->addr + 0x1c37c, 0x00000000);
	reg32_write(priv->addr + 0x1c3bc, 0x00000000);
	reg32_write(priv->addr + 0x1c200, 0x00000000);
	reg32_write(priv->addr + 0x1c240, 0x00000000);
	reg32_write(priv->addr + 0x1c280, 0x00000000);
	reg32_write(priv->addr + 0x1c204, 0x00000000);
	reg32_write(priv->addr + 0x1c244, 0x00000000);
	reg32_write(priv->addr + 0x1c284, 0x00000000);
	reg32_write(priv->addr + 0x1c208, 0x00000000);
	reg32_write(priv->addr + 0x1c248, 0x00000000);
	reg32_write(priv->addr + 0x1c288, 0x00000000);
	reg32_write(priv->addr + 0x1c20c, 0x00000000);
	reg32_write(priv->addr + 0x1c24c, 0x00000000);
	reg32_write(priv->addr + 0x1c28c, 0x00000000);
	reg32_write(priv->addr + 0x1c210, 0x00000000);
	reg32_write(priv->addr + 0x1c250, 0x00000000);
	reg32_write(priv->addr + 0x1c290, 0x00000000);
	reg32_write(priv->addr + 0x1c214, 0x00000000);
	reg32_write(priv->addr + 0x1c254, 0x00000000);
	reg32_write(priv->addr + 0x1c294, 0x00000000);
	reg32_write(priv->addr + 0x1c218, 0x00000000);
	reg32_write(priv->addr + 0x1c258, 0x00000000);
	reg32_write(priv->addr + 0x1c298, 0x00000000);
	reg32_write(priv->addr + 0x1c21c, 0x00000000);
	reg32_write(priv->addr + 0x1c25c, 0x00000000);
	reg32_write(priv->addr + 0x1c29c, 0x00000000);
	reg32_write(priv->addr + 0x1c220, 0x00000000);
	reg32_write(priv->addr + 0x1c260, 0x00000000);
	reg32_write(priv->addr + 0x1c2a0, 0x00000000);
	reg32_write(priv->addr + 0x1c224, 0x00000000);
	reg32_write(priv->addr + 0x1c264, 0x00000000);
	reg32_write(priv->addr + 0x1c2a4, 0x00000000);
	reg32_write(priv->addr + 0x1c228, 0x00000000);
	reg32_write(priv->addr + 0x1c268, 0x00000000);
	reg32_write(priv->addr + 0x1c2a8, 0x00000000);
	reg32_write(priv->addr + 0x1c22c, 0x00000000);
	reg32_write(priv->addr + 0x1c26c, 0x00000000);
	reg32_write(priv->addr + 0x1c2ac, 0x00000000);
	reg32_write(priv->addr + 0x1c230, 0x00000000);
	reg32_write(priv->addr + 0x1c270, 0x00000000);
	reg32_write(priv->addr + 0x1c2b0, 0x00000000);
	reg32_write(priv->addr + 0x1c234, 0x00000000);
	reg32_write(priv->addr + 0x1c274, 0x00000000);
	reg32_write(priv->addr + 0x1c2b4, 0x00000000);
	reg32_write(priv->addr + 0x1c238, 0x00000000);
	reg32_write(priv->addr + 0x1c278, 0x00000000);
	reg32_write(priv->addr + 0x1c2b8, 0x00000000);
	reg32_write(priv->addr + 0x1c23c, 0x00000000);
	reg32_write(priv->addr + 0x1c27c, 0x00000000);
	reg32_write(priv->addr + 0x1c2bc, 0x00000000);
	reg32_write(priv->addr + 0x1c2bc, 0x00000000);
	reg32_write(priv->addr + 0x1c000, 0x00000011);

	/* SUBSAM */
	reg32_write(priv->addr + 0x1b070, 0x21612161);
	reg32_write(priv->addr + 0x1b080, 0x03ff0000);
	reg32_write(priv->addr + 0x1b090, 0x03ff0000);

	reg32_write(priv->addr + 0x1b010,
		    (((priv->timings.vfront_porch.typ + priv->timings.vback_porch.typ + priv->timings.vsync_len.typ +
			priv->timings.vactive.typ -1) << 16) |
		       (priv->timings.hfront_porch.typ + priv->timings.hback_porch.typ + priv->timings.hsync_len.typ +
			priv->timings.hactive.typ - 1)));
	reg32_write(priv->addr + 0x1b020,
		    (((priv->timings.hsync_len.typ - 1) << 16) | priv->hpol << 31 | (priv->timings.hfront_porch.typ +
			priv->timings.hback_porch.typ + priv->timings.hsync_len.typ + priv->timings.hactive.typ -1)));
	reg32_write(priv->addr + 0x1b030,
		    (((priv->timings.vfront_porch.typ + priv->timings.vsync_len.typ - 1) << 16) | priv->vpol << 31 | (priv->timings.vfront_porch.typ - 1)));
	reg32_write(priv->addr + 0x1b040,
		    ((1 << 31) | ((priv->timings.vsync_len.typ +priv->timings.vfront_porch.typ + priv->timings.vback_porch.typ) << 16) |
		    (priv->timings.hsync_len.typ + priv->timings.hback_porch.typ - 1)));
	reg32_write(priv->addr + 0x1b050,
		    (((priv->timings.vsync_len.typ + priv->timings.vfront_porch.typ + priv->timings.vback_porch.typ + priv->timings.vactive.typ -1) << 16) |
		    (priv->timings.hsync_len.typ + priv->timings.hback_porch.typ + priv->timings.hactive.typ - 1)));

	/* subsample mode 0 bypass 444, 1 422, 2 420 */
	reg32_write(priv->addr + 0x1b060, 0x0000000);

	reg32_write(priv->addr + 0x1b000, 0x00000001);

	/* DTG */
	/*reg32_write(priv->addr + 0x20000, 0xff000484); */
	/* disable local alpha */
	reg32_write(priv->addr + 0x20000, 0xff005084);
	reg32_write(priv->addr + 0x20004,
		    (((priv->timings.vfront_porch.typ + priv->timings.vback_porch.typ + priv->timings.vsync_len.typ + priv->timings.vactive.typ -
		       1) << 16) | (priv->timings.hfront_porch.typ + priv->timings.hback_porch.typ + priv->timings.hsync_len.typ +
			priv->timings.hactive.typ - 1)));
	reg32_write(priv->addr + 0x20008,
		    (((priv->timings.vsync_len.typ + priv->timings.vfront_porch.typ + priv->timings.vback_porch.typ -
		       1) << 16) | (priv->timings.hsync_len.typ + priv->timings.hback_porch.typ - 1)));
	reg32_write(priv->addr + 0x2000c,
		    (((priv->timings.vsync_len.typ + priv->timings.vfront_porch.typ + priv->timings.vback_porch.typ + priv->timings.vactive.typ -
		       1) << 16) | (priv->timings.hsync_len.typ + priv->timings.hback_porch.typ + priv->timings.hactive.typ - 1)));
	reg32_write(priv->addr + 0x20010,
		    (((priv->timings.vsync_len.typ + priv->timings.vfront_porch.typ + priv->timings.vback_porch.typ -
		       1) << 16) | (priv->timings.hsync_len.typ + priv->timings.hback_porch.typ - 1)));
	reg32_write(priv->addr + 0x20014,
		    (((priv->timings.vsync_len.typ + priv->timings.vfront_porch.typ + priv->timings.vback_porch.typ + priv->timings.vactive.typ -
		       1) << 16) | (priv->timings.hsync_len.typ + priv->timings.hback_porch.typ + priv->timings.hactive.typ - 1)));
	reg32_write(priv->addr + 0x20028, 0x000b000a);

	/* disable local alpha */
	reg32_write(priv->addr + 0x20000, 0xff005184);

	debug("leaving %s() ...\n", __func__);
}

static void imx8m_display_shutdown(struct udevice *dev)
{
	struct imx8m_dcss_priv *priv = dev_get_priv(dev);

	/* stop the DCSS modules in use */
	/* dtg */
	reg32_write(priv->addr + 0x20000, 0);
	/* scaler */
	reg32_write(priv->addr + 0x1c000, 0);
	reg32_write(priv->addr + 0x1c400, 0);
	reg32_write(priv->addr + 0x1c800, 0);
	/* dpr */
	reg32_write(priv->addr + 0x18000, 0);
	reg32_write(priv->addr + 0x19000, 0);
	reg32_write(priv->addr + 0x1a000, 0);
	/* sub-sampler*/
	reg32_write(priv->addr + 0x1b000, 0);
}

static int imx8m_dcss_get_timings_from_display(struct udevice *dev,
					   struct display_timing *timings)
{
	struct imx8m_dcss_priv *priv = dev_get_priv(dev);
	int err;

	priv->disp_dev = video_link_get_next_device(dev);
	if (!priv->disp_dev ||
		device_get_uclass_id(priv->disp_dev) != UCLASS_DISPLAY) {

		printf("fail to find display device\n");
		return -ENODEV;
	}

	debug("disp_dev %s\n", priv->disp_dev->name);

	err = video_link_get_display_timings(timings);
	if (err)
		return err;

	if (timings->flags & DISPLAY_FLAGS_HSYNC_HIGH)
		priv->hpol = true;

	if (timings->flags & DISPLAY_FLAGS_VSYNC_HIGH)
		priv->vpol = true;

	return 0;
}

static int imx8m_dcss_probe(struct udevice *dev)
{
	struct video_uc_platdata *plat = dev_get_uclass_platdata(dev);
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	struct imx8m_dcss_priv *priv = dev_get_priv(dev);

	u32 fb_start, fb_end;
	int ret;

	debug("%s() plat: base 0x%lx, size 0x%x\n",
	       __func__, plat->base, plat->size);

	priv->addr = dev_read_addr(dev);
	if (priv->addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	ret = imx8m_dcss_get_timings_from_display(dev, &priv->timings);
	if (ret)
		return ret;

	debug("pixelclock %u, hlen %u, vlen %u\n",
		priv->timings.pixelclock.typ, priv->timings.hactive.typ, priv->timings.vactive.typ);

	imx8m_dcss_power_init();

	imx8m_dcss_clock_init(priv->timings.pixelclock.typ);

	imx8m_dcss_reset(dev);

	if (display_enable(priv->disp_dev, 32, NULL) == 0) {
		imx8m_dcss_init(dev);
		priv->enabled = true;
	}

	uc_priv->bpix = VIDEO_BPP32;
	uc_priv->xsize = priv->timings.hactive.typ;
	uc_priv->ysize = priv->timings.vactive.typ;

	/* Enable dcache for the frame buffer */
	fb_start = plat->base & ~(MMU_SECTION_SIZE - 1);
	fb_end = plat->base + plat->size;
	fb_end = ALIGN(fb_end, 1 << MMU_SECTION_SHIFT);
	mmu_set_region_dcache_behaviour(fb_start, fb_end - fb_start,
					DCACHE_WRITEBACK);
	video_set_flush_dcache(dev, true);

	return ret;
}

static int imx8m_dcss_bind(struct udevice *dev)
{
	struct video_uc_platdata *plat = dev_get_uclass_platdata(dev);

	debug("%s\n", __func__);

	/* Max size supported by LCDIF, because in bind, we can't probe panel */
	plat->size = 1920 * 1080 *4;

	return 0;
}

static int imx8m_dcss_remove(struct udevice *dev)
{
	struct imx8m_dcss_priv *priv = dev_get_priv(dev);

	debug("%s\n", __func__);

	if (priv->enabled) {
		device_remove(priv->disp_dev, DM_REMOVE_NORMAL);
		imx8m_display_shutdown(dev);
	}

	return 0;
}

static const struct udevice_id imx8m_dcss_ids[] = {
	{ .compatible = "nxp,imx8mq-dcss" },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(imx8m_dcss) = {
	.name	= "imx8m_dcss",
	.id	= UCLASS_VIDEO,
	.of_match = imx8m_dcss_ids,
	.bind	= imx8m_dcss_bind,
	.probe	= imx8m_dcss_probe,
	.remove = imx8m_dcss_remove,
	.flags	= DM_FLAG_PRE_RELOC,
	.priv_auto_alloc_size	= sizeof(struct imx8m_dcss_priv),
};
