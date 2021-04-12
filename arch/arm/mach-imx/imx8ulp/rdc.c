// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021 NXP
 */

#include <common.h>
#include <asm/io.h>
#include <asm/types.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/mu_hal.h>
#include <asm/arch/s400_api.h>
#include <asm/arch/rdc.h>

struct mbc_mem_dom {
	u32 mem_glbcfg[4];
	u32 nse_blk_index;
	u32 nse_blk_set;
	u32 nse_blk_clr;
	u32 nsr_blk_clr_all;
	u32 memn_glbac[8];
	/* The upper only existed in the beginning of each MBC */
	u32 mem0_blk_cfg_w[64];
	u32 mem0_blk_nse_w[16];
	u32 mem1_blk_cfg_w[8];
	u32 mem1_blk_nse_w[2];
	u32 mem2_blk_cfg_w[8];
	u32 mem2_blk_nse_w[2];
	u32 mem3_blk_cfg_w[8];
	u32 mem3_blk_nse_w[2];/*0x1F0, 0x1F4 */
	u32 reserved[2];
};

struct trdc {
	u8 res0[0x1000];
	struct mbc_mem_dom mem_dom[4][8];
};

int release_rdc(enum rdc_type type)
{
	ulong s_mu_base = 0x27020000UL;
	struct imx8ulp_s400_msg msg;
	int ret;
	u32 rdc_id = (type == RDC_XRDC) ? 0x78 : 0x74;

	msg.version = AHAB_VERSION;
	msg.tag = AHAB_CMD_TAG;
	msg.size = 2;
	msg.command = AHAB_RELEASE_RDC_REQ_CID;
	msg.data[0] = (rdc_id << 8) | 0x2; /* A35 XRDC */

	mu_hal_init(s_mu_base);
	mu_hal_sendmsg(s_mu_base, 0, *((u32 *)&msg));
	mu_hal_sendmsg(s_mu_base, 1, msg.data[0]);

	ret = mu_hal_receivemsg(s_mu_base, 0, (u32 *)&msg);
	if (!ret) {
		ret = mu_hal_receivemsg(s_mu_base, 1, &msg.data[0]);
		if (!ret) {
			if ((msg.data[0] & 0xff) == 0xd6)
				return 0;
		}

		return -EIO;
	}

	return ret;
}

void xrdc_mrc_region_set_access(int mrc_index, u32 addr, u32 access)
{
	ulong xrdc_base = 0x292f0000, off;
	u32 mrgd[5];
	u8 mrcfg, j, region_num;
	u8 dsel;

	mrcfg = readb(xrdc_base + 0x140 + mrc_index);
	region_num = mrcfg & 0x1f;

	for (j = 0; j < region_num; j++) {
		off = 0x2000 + mrc_index * 0x200 + j * 0x20;

		mrgd[0] = readl(xrdc_base + off);
		mrgd[1] = readl(xrdc_base + off + 4);
		mrgd[2] = readl(xrdc_base + off + 8);
		mrgd[3] = readl(xrdc_base + off + 0xc);
		mrgd[4] = readl(xrdc_base + off + 0x10);

		debug("MRC [%u][%u]\n", mrc_index, j);
		debug("0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
		      mrgd[0], mrgd[1], mrgd[2], mrgd[3], mrgd[4]);

		/* hit */
		if (addr >= mrgd[0] && addr <= mrgd[1]) {
			/* find domain 7 DSEL */
			dsel = (mrgd[2] >> 21) & 0x7;
			if (dsel == 1) {
				mrgd[4] &= ~0xFFF;
				mrgd[4] |= (access & 0xFFF);
			} else if (dsel == 2) {
				mrgd[4] &= ~0xFFF0000;
				mrgd[4] |= ((access & 0xFFF) << 16);
			}

			/* not handle other cases, since S400 only set ACCESS1 and 2 */
			writel(mrgd[4], xrdc_base + off + 0x10);
			return;
		}
	}
}

int trdc_mbc_set_access(u32 mbc_x, u32 dom_x, u32 mem_x, u32 blk_x, bool sec_access)
{
	struct trdc *trdc_base = (struct trdc *)0x28031000U;
	struct mbc_mem_dom *mbc_dom;
	u32 *cfg_w, *nse_w;
	u32 index, offset, val;

	mbc_dom = &trdc_base->mem_dom[mbc_x][dom_x];

	switch (mem_x) {
	case 0:
		cfg_w = &mbc_dom->mem0_blk_cfg_w[blk_x / 8];
		nse_w = &mbc_dom->mem0_blk_nse_w[blk_x / 32];
		break;
	case 1:
		cfg_w = &mbc_dom->mem1_blk_cfg_w[blk_x / 8];
		nse_w = &mbc_dom->mem1_blk_nse_w[blk_x / 32];
		break;
	case 2:
		cfg_w = &mbc_dom->mem2_blk_cfg_w[blk_x / 8];
		nse_w = &mbc_dom->mem2_blk_nse_w[blk_x / 32];
		break;
	case 3:
		cfg_w = &mbc_dom->mem3_blk_cfg_w[blk_x / 8];
		nse_w = &mbc_dom->mem3_blk_nse_w[blk_x / 32];
		break;
	default:
		return -EINVAL;
	};

	index = blk_x % 8;
	offset = index * 4;

	val = readl((void __iomem *)cfg_w);

	val &= ~(0xFU << offset);

	/* MBC0-3
	 *  Global 0, 0x7777 secure pri/user read/write/execute, S400 has already set it.
	 *  So select MBC0_MEMN_GLBAC0
	 */
	if (sec_access) {
		val |= (0x0 << offset);
		writel(val, (void __iomem *)cfg_w);
	} else {
		val |= (0x8 << offset); /* nse bit set */
		writel(val, (void __iomem *)cfg_w);
	}

	return 0;
}
