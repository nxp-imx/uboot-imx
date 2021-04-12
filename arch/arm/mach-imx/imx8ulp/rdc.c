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

int release_xrdc(void)
{
	ulong s_mu_base = 0x27020000UL;
	struct imx8ulp_s400_msg msg;
	int ret;

	msg.version = AHAB_VERSION;
	msg.tag = AHAB_CMD_TAG;
	msg.size = 2;
	msg.command = AHAB_RELEASE_RDC_REQ_CID;
	msg.data[0] = (0x78 << 8) | 0x2; /* A35 XRDC */

	mu_hal_init(s_mu_base);
	mu_hal_sendmsg(s_mu_base, 0, *((u32 *)&msg));
	mu_hal_sendmsg(s_mu_base, 1, msg.data[0]);

	ret = mu_hal_receivemsg(s_mu_base, 0, (u32 *)&msg);
	if (!ret) {
		ret = mu_hal_receivemsg(s_mu_base, 1, &msg.data[0]);
		if (!ret)
			return ret;

		if ((msg.data[0] & 0xff) == 0)
			return 0;
		else
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
