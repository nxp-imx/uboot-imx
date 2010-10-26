/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/err.h>
#include <linux/list.h>
#include <malloc.h>
#include <common.h>
#include <asm/apbh_dma.h>
#include <asm/io.h>

#ifdef CONFIG_ARCH_MMU
#include <asm/arch/mmu.h>
#endif

#ifndef BM_APBH_CTRL0_APB_BURST_EN
#define BM_APBH_CTRL0_APB_BURST_EN BM_APBH_CTRL0_APB_BURST4_EN
#endif

#if 0
static inline s32 mxs_dma_apbh_reset_block(void *hwreg, int is_enable)
{
	int timeout;

	/* the process of software reset of IP block is done
	   in several steps:

	   - clear SFTRST and wait for block is enabled;
	   - clear clock gating (CLKGATE bit);
	   - set the SFTRST again and wait for block is in reset;
	   - clear SFTRST and wait for reset completion.
	 */
	/* clear SFTRST */
	REG_CLR_ADDR(hwreg, BM_APBH_CTRL0_SFTRST);

	for (timeout = 1000000; timeout > 0; timeout--)
		/* still in SFTRST state ? */
		if ((REG_RD_ADDR(hwreg) & BM_APBH_CTRL0_SFTRST) == 0)
			break;
		if (timeout <= 0) {
			printk(KERN_ERR "%s(%p): timeout when enabling\n",
				__func__, hwreg);
			return -ETIME;
	}

	/* clear CLKGATE */
	REG_CLR_ADDR(hwreg, BM_APBH_CTRL0_CLKGATE);

	if (is_enable) {
		/* now again set SFTRST */
		REG_SET_ADDR(hwreg, BM_APBH_CTRL0_SFTRST);
		for (timeout = 1000000; timeout > 0; timeout--)
			/* poll until CLKGATE set */
			if (REG_RD_ADDR(hwreg) & BM_APBH_CTRL0_CLKGATE)
				break;
		if (timeout <= 0) {
			printk(KERN_ERR "%s(%p): timeout when resetting\n",
				__func__, hwreg);
			return -ETIME;
		}

		REG_CLR_ADDR(hwreg, BM_APBH_CTRL0_SFTRST);
		for (timeout = 1000000; timeout > 0; timeout--)
			/* still in SFTRST state ? */
			if ((REG_RD_ADDR(hwreg) & BM_APBH_CTRL0_SFTRST) == 0)
				break;
		if (timeout <= 0) {
			printk(KERN_ERR "%s(%p): timeout when enabling "
				"after reset\n", __func__, hwreg);
			return -ETIME;
		}

		/* clear CLKGATE */
		REG_CLR_ADDR(hwreg, BM_APBH_CTRL0_CLKGATE);
	}
	for (timeout = 1000000; timeout > 0; timeout--)
		/* still in SFTRST state ? */
		if ((REG_RD_ADDR(hwreg) & BM_APBH_CTRL0_CLKGATE) == 0)
			break;

	if (timeout <= 0) {
		printk(KERN_ERR "%s(%p): timeout when unclockgating\n",
			__func__, hwreg);
		return -ETIME;
	}

	return 0;
}
#endif

static int mxs_dma_apbh_enable(struct mxs_dma_chan *pchan, unsigned int chan)
{
	unsigned int sem;
	struct mxs_dma_device *pdev = pchan->dma;
	struct mxs_dma_desc *pdesc;

	pdesc = list_first_entry(&pchan->active, struct mxs_dma_desc, node);
	if (pdesc == NULL)
		return -EFAULT;

	sem = readl(pdev->base + HW_APBH_CHn_SEMA(chan));
	sem = (sem & BM_APBH_CHn_SEMA_PHORE) >> BP_APBH_CHn_SEMA_PHORE;
	if (pchan->flags & MXS_DMA_FLAGS_BUSY) {
		if (pdesc->cmd.cmd.bits.chain == 0)
			return 0;
		if (sem < 2) {
			if (!sem)
				return 0;
			pdesc = list_entry(pdesc->node.next,
					   struct mxs_dma_desc, node);
#ifdef CONFIG_ARCH_MMU
			writel(iomem_to_phys(mxs_dma_cmd_address(pdesc)),
				     pdev->base + HW_APBH_CHn_NXTCMDAR(chan));
#else
			writel(mxs_dma_cmd_address(pdesc),
				     pdev->base + HW_APBH_CHn_NXTCMDAR(chan));
#endif
		}
		sem = pchan->pending_num;
		pchan->pending_num = 0;
		writel(BF_APBH_CHn_SEMA_INCREMENT_SEMA(sem),
			     pdev->base + HW_APBH_CHn_SEMA(chan));
		pchan->active_num += sem;
		return 0;
	}

	pchan->active_num += pchan->pending_num;
	pchan->pending_num = 0;
#ifdef CONFIG_ARCH_MMU
	writel(iomem_to_phys(mxs_dma_cmd_address(pdesc)),
	     pdev->base + HW_APBH_CHn_NXTCMDAR(chan));
#else
	writel(mxs_dma_cmd_address(pdesc),
	     pdev->base + HW_APBH_CHn_NXTCMDAR(chan));
#endif
	writel(pchan->active_num, pdev->base + HW_APBH_CHn_SEMA(chan));
	REG_CLR(pdev->base, HW_APBH_CTRL0, 1 << chan);
	return 0;
}

static void mxs_dma_apbh_disable(struct mxs_dma_chan *pchan, unsigned int chan)
{
	struct mxs_dma_device *pdev = pchan->dma;

	REG_SET(pdev->base, HW_APBH_CTRL0,
		1 << (chan + BP_APBH_CTRL0_CLKGATE_CHANNEL));
}

static void mxs_dma_apbh_reset(struct mxs_dma_device *pdev, unsigned int chan)
{
	REG_SET(pdev->base, HW_APBH_CHANNEL_CTRL,
		1 << (chan + BP_APBH_CHANNEL_CTRL_RESET_CHANNEL));
}

static void mxs_dma_apbh_freeze(struct mxs_dma_device *pdev, unsigned int chan)
{
	REG_SET(pdev->base, HW_APBH_CHANNEL_CTRL, 1 << chan);
}

static void
mxs_dma_apbh_unfreeze(struct mxs_dma_device *pdev, unsigned int chan)
{
	REG_CLR(pdev->base, HW_APBH_CHANNEL_CTRL, 1 << chan);
}

static void mxs_dma_apbh_info(struct mxs_dma_device *pdev,
		unsigned int chan, struct mxs_dma_info *info)
{
	unsigned int reg;

	reg = REG_RD(pdev->base, HW_APBH_CTRL2);
	info->status = reg >> chan;
	info->buf_addr = readl(pdev->base + HW_APBH_CHn_BAR(chan));
}

static int
mxs_dma_apbh_read_semaphore(struct mxs_dma_device *pdev, unsigned int chan)
{
	unsigned int reg;

	reg = readl(pdev->base + HW_APBH_CHn_SEMA(chan));
	return (reg & BM_APBH_CHn_SEMA_PHORE) >> BP_APBH_CHn_SEMA_PHORE;
}

static void
mxs_dma_apbh_enable_irq(struct mxs_dma_device *pdev,
			unsigned int chan, int enable)
{
	if (enable)
		REG_SET(pdev->base, HW_APBH_CTRL1, 1 << (chan + 16));
	else
		REG_CLR(pdev->base, HW_APBH_CTRL1, 1 << (chan + 16));

}

static int
mxs_dma_apbh_irq_is_pending(struct mxs_dma_device *pdev, unsigned int chan)
{
	unsigned int reg;

	reg = REG_RD(pdev->base, HW_APBH_CTRL1);
	reg |= REG_RD(pdev->base, HW_APBH_CTRL2);

	return reg & (1 << chan);
}

static void mxs_dma_apbh_ack_irq(struct mxs_dma_device *pdev,
				unsigned int chan)
{
	REG_CLR(pdev->base, HW_APBH_CTRL1, 1 << chan);
	REG_CLR(pdev->base, HW_APBH_CTRL2, 1 << chan);
}

static struct mxs_dma_device mxs_dma_apbh = {
	.name = "mxs-dma-apbh",
};

static int mxs_dma_apbh_probe(void)
{
	int i = 1000000;
	u32 base = CONFIG_MXS_DMA_REG_BASE;

	mxs_dma_apbh.base = (void *)base;

	/*
	mxs_dma_apbh_reset_block((void *)(base + HW_APBH_CTRL0), 1);
	*/
	REG_CLR(base, HW_APBH_CTRL0,
		BM_APBH_CTRL0_SFTRST);
	for (; i > 0; --i) {
		if (!(REG_RD(base, HW_APBH_CTRL0) &
		      BM_APBH_CTRL0_SFTRST))
			break;
		udelay(2);
	}
	if (i <= 0)
		return -ETIME;
	REG_CLR(base, HW_APBH_CTRL0, BM_APBH_CTRL0_CLKGATE);

#ifdef CONFIG_APBH_DMA_BURST8
	REG_SET(base, HW_APBH_CTRL0,
		BM_APBH_CTRL0_AHB_BURST8_EN);
#else
	REG_CLR(base, HW_APBH_CTRL0,
		BM_APBH_CTRL0_AHB_BURST8_EN);
#endif

#ifdef CONFIG_APBH_DMA_BURST
	REG_SET(base, HW_APBH_CTRL0,
		BM_APBH_CTRL0_APB_BURST_EN);
#else
	REG_CLR(base, HW_APBH_CTRL0,
		BM_APBH_CTRL0_APB_BURST_EN);
#endif

	mxs_dma_apbh.chan_base = MXS_DMA_CHANNEL_AHB_APBH;
	mxs_dma_apbh.chan_num = MXS_MAX_DMA_CHANNELS;

	return mxs_dma_device_register(&mxs_dma_apbh);
}

/* DMA engine */

/*
 * The list of DMA drivers that manage various DMA channels. A DMA device
 * driver registers to manage DMA channels by calling mxs_dma_device_register().
 */
static LIST_HEAD(mxs_dma_devices);

/*
 * The array of struct mxs_dma_chan that represent every DMA channel in the
 * system. The index of the structure in the array indicates the specific DMA
 * hardware it represents (see mach-mx28/include/mach/dma.h).
 */

static struct mxs_dma_chan mxs_dma_channels[MXS_MAX_DMA_CHANNELS];

int mxs_dma_request(int channel)
{
	int ret = 0;
	struct mxs_dma_chan *pchan;

	if ((channel < 0) || (channel >= MXS_MAX_DMA_CHANNELS))
		return -EINVAL;

	pchan = mxs_dma_channels + channel;
	if ((pchan->flags & MXS_DMA_FLAGS_VALID) != MXS_DMA_FLAGS_VALID) {
		ret = -ENODEV;
		goto out;
	}
	if (pchan->flags & MXS_DMA_FLAGS_ALLOCATED) {
		ret = -EBUSY;
		goto out;
	}
	pchan->flags |= MXS_DMA_FLAGS_ALLOCATED;
	pchan->active_num = 0;
	pchan->pending_num = 0;
	INIT_LIST_HEAD(&pchan->active);
	INIT_LIST_HEAD(&pchan->done);
out:
	return ret;
}

void mxs_dma_release(int channel)
{
	struct mxs_dma_chan *pchan;

	if ((channel < 0) || (channel >= MXS_MAX_DMA_CHANNELS))
		return;

	pchan = mxs_dma_channels + channel;

	if (!(pchan->flags & MXS_DMA_FLAGS_ALLOCATED))
		return;

	if (pchan->flags & MXS_DMA_FLAGS_BUSY)
		return;

	pchan->dev = 0;
	pchan->active_num = 0;
	pchan->pending_num = 0;
	pchan->flags &= ~MXS_DMA_FLAGS_ALLOCATED;
}

int mxs_dma_enable(int channel)
{
	int ret = 0;
	struct mxs_dma_chan *pchan;
	struct mxs_dma_device *pdma;

	if ((channel < 0) || (channel >= MXS_MAX_DMA_CHANNELS))
		return -EINVAL;

	pchan = mxs_dma_channels + channel;
	if (!(pchan->flags & MXS_DMA_FLAGS_ALLOCATED))
		return -EINVAL;

	pdma = pchan->dma;
	if (pchan->pending_num)
		ret = mxs_dma_apbh_enable(pchan, channel - pdma->chan_base);
	pchan->flags |= MXS_DMA_FLAGS_BUSY;
	return ret;
}

void mxs_dma_disable(int channel)
{
	struct mxs_dma_chan *pchan;
	struct mxs_dma_device *pdma;

	if ((channel < 0) || (channel >= MXS_MAX_DMA_CHANNELS))
		return;

	pchan = mxs_dma_channels + channel;
	if (!(pchan->flags & MXS_DMA_FLAGS_ALLOCATED))
		return;
	if (!(pchan->flags & MXS_DMA_FLAGS_BUSY))
		return;
	pdma = pchan->dma;
	mxs_dma_apbh_disable(pchan, channel - pdma->chan_base);
	pchan->flags &= ~MXS_DMA_FLAGS_BUSY;
	pchan->active_num = 0;
	pchan->pending_num = 0;
	list_splice_init(&pchan->active, &pchan->done);
}

int mxs_dma_get_info(int channel, struct mxs_dma_info *info)
{
	struct mxs_dma_chan *pchan;
	struct mxs_dma_device *pdma;

	if (!info)
		return -EINVAL;
	if ((channel < 0) || (channel >= MXS_MAX_DMA_CHANNELS))
		return -EINVAL;

	pchan = mxs_dma_channels + channel;
	if (!(pchan->flags & MXS_DMA_FLAGS_ALLOCATED))
		return -EFAULT;
	pdma = pchan->dma;
	mxs_dma_apbh_info(pdma, channel - pdma->chan_base, info);

	return 0;
}

int mxs_dma_cooked(int channel, struct list_head *head)
{
	int sem;
	struct mxs_dma_chan *pchan;
	struct list_head *p, *q;
	struct mxs_dma_desc *pdesc;

	if ((channel < 0) || (channel >= MXS_MAX_DMA_CHANNELS))
		return -EINVAL;

	pchan = mxs_dma_channels + channel;
	if (!(pchan->flags & MXS_DMA_FLAGS_ALLOCATED))
		return -EINVAL;

	sem = mxs_dma_read_semaphore(channel);
	if (sem < 0)
		return sem;
	if (sem == pchan->active_num)
		return 0;
	list_for_each_safe(p, q, &pchan->active) {
		if ((pchan->active_num) <= sem)
			break;
		pdesc = list_entry(p, struct mxs_dma_desc, node);
		pdesc->flags &= ~MXS_DMA_DESC_READY;
		if (head)
			list_move_tail(p, head);
		else
			list_move_tail(p, &pchan->done);
		if (pdesc->flags & MXS_DMA_DESC_LAST)
			pchan->active_num--;
	}
	if (sem == 0)
		pchan->flags &= ~MXS_DMA_FLAGS_BUSY;

	return 0;
}

void mxs_dma_reset(int channel)
{
	struct mxs_dma_chan *pchan;
	struct mxs_dma_device *pdma;

	if ((channel < 0) || (channel >= MXS_MAX_DMA_CHANNELS))
		return;

	pchan = mxs_dma_channels + channel;
	if (!(pchan->flags & MXS_DMA_FLAGS_ALLOCATED))
		return;
	pdma = pchan->dma;
	mxs_dma_apbh_reset(pdma, channel - pdma->chan_base);
}

void mxs_dma_freeze(int channel)
{
	struct mxs_dma_chan *pchan;
	struct mxs_dma_device *pdma;
	if ((channel < 0) || (channel >= MXS_MAX_DMA_CHANNELS))
		return;
	pchan = mxs_dma_channels + channel;
	if (!(pchan->flags & MXS_DMA_FLAGS_ALLOCATED))
		return;
	pdma = pchan->dma;
	mxs_dma_apbh_freeze(pdma, channel - pdma->chan_base);
}

void mxs_dma_unfreeze(int channel)
{
	struct mxs_dma_chan *pchan;
	struct mxs_dma_device *pdma;

	if ((channel < 0) || (channel >= MXS_MAX_DMA_CHANNELS))
		return;

	pchan = mxs_dma_channels + channel;
	if (!(pchan->flags & MXS_DMA_FLAGS_ALLOCATED))
		return;
	pdma = pchan->dma;
	mxs_dma_apbh_unfreeze(pdma, channel - pdma->chan_base);
}

int mxs_dma_read_semaphore(int channel)
{
	int ret = -EINVAL;
	struct mxs_dma_chan *pchan;
	struct mxs_dma_device *pdma;

	if ((channel < 0) || (channel >= MXS_MAX_DMA_CHANNELS))
		return ret;

	pchan = mxs_dma_channels + channel;
	if (!(pchan->flags & MXS_DMA_FLAGS_ALLOCATED))
		return ret;
	pdma = pchan->dma;
	ret = mxs_dma_apbh_read_semaphore(pdma, channel - pdma->chan_base);

	return ret;
}

void mxs_dma_enable_irq(int channel, int en)
{
	struct mxs_dma_chan *pchan;
	struct mxs_dma_device *pdma;

	if ((channel < 0) || (channel >= MXS_MAX_DMA_CHANNELS))
		return;

	pchan = mxs_dma_channels + channel;
	if (!(pchan->flags & MXS_DMA_FLAGS_ALLOCATED))
		return;
	pdma = pchan->dma;
	mxs_dma_apbh_enable_irq(pdma, channel - pdma->chan_base, en);
}

int mxs_dma_irq_is_pending(int channel)
{
	int ret = 0;
	struct mxs_dma_chan *pchan;
	struct mxs_dma_device *pdma;

	if ((channel < 0) || (channel >= MXS_MAX_DMA_CHANNELS))
		return ret;

	pchan = mxs_dma_channels + channel;
	if (!(pchan->flags & MXS_DMA_FLAGS_ALLOCATED))
		return ret;
	pdma = pchan->dma;
	ret = mxs_dma_apbh_irq_is_pending(pdma, channel - pdma->chan_base);

	return ret;
}

void mxs_dma_ack_irq(int channel)
{
	struct mxs_dma_chan *pchan;
	struct mxs_dma_device *pdma;

	if ((channel < 0) || (channel >= MXS_MAX_DMA_CHANNELS))
		return;

	pchan = mxs_dma_channels + channel;
	if (!(pchan->flags & MXS_DMA_FLAGS_ALLOCATED))
		return;
	pdma = pchan->dma;
	mxs_dma_apbh_ack_irq(pdma, channel - pdma->chan_base);
}

/* mxs dma utility function */
struct mxs_dma_desc *mxs_dma_alloc_desc(void)
{
	struct mxs_dma_desc *pdesc;
#ifdef CONFIG_ARCH_MMU
	u32 address;
#endif

#ifdef CONFIG_ARCH_MMU
	address = (u32)iomem_to_phys((ulong)memalign(MXS_DMA_ALIGNMENT,
				sizeof(struct mxs_dma_desc)));
	if (!address)
		return NULL;
	pdesc = (struct mxs_dma_desc *)ioremap_nocache(address,
		MXS_DMA_ALIGNMENT);
	memset(pdesc, 0, sizeof(*pdesc));
	pdesc->address = address;
#else
	pdesc = (struct mxs_dma_desc *)memalign(MXS_DMA_ALIGNMENT,
				sizeof(struct mxs_dma_desc));
	if (pdesc == NULL)
		return NULL;
	memset(pdesc, 0, sizeof(*pdesc));
	pdesc->address = (dma_addr_t)pdesc;
#endif

	return pdesc;
};

void mxs_dma_free_desc(struct mxs_dma_desc *pdesc)
{
	if (pdesc == NULL)
		return;

	free(pdesc);
}

int mxs_dma_desc_append(int channel, struct mxs_dma_desc *pdesc)
{
	int ret = 0;
	struct mxs_dma_chan *pchan;
	struct mxs_dma_desc *last;
	struct mxs_dma_device *pdma;

	if ((channel < 0) || (channel >= MXS_MAX_DMA_CHANNELS))
		return -EINVAL;

	pchan = mxs_dma_channels + channel;
	if (!(pchan->flags & MXS_DMA_FLAGS_ALLOCATED))
		return -EINVAL;
	pdma = pchan->dma;
#ifdef CONFIG_ARCH_MMU
	pdesc->cmd.next = iomem_to_phys(mxs_dma_cmd_address(pdesc));
#else
	pdesc->cmd.next = mxs_dma_cmd_address(pdesc);
#endif
	pdesc->flags |= MXS_DMA_DESC_FIRST | MXS_DMA_DESC_LAST;
	if (!list_empty(&pchan->active)) {

		last = list_entry(pchan->active.prev,
				  struct mxs_dma_desc, node);

		pdesc->flags &= ~MXS_DMA_DESC_FIRST;
		last->flags &= ~MXS_DMA_DESC_LAST;

#ifdef CONFIG_ARCH_MMU
		last->cmd.next = iomem_to_phys(mxs_dma_cmd_address(pdesc));
#else
		last->cmd.next = mxs_dma_cmd_address(pdesc);
#endif
		last->cmd.cmd.bits.chain = 1;
	}
	pdesc->flags |= MXS_DMA_DESC_READY;
	if (pdesc->flags & MXS_DMA_DESC_FIRST)
		pchan->pending_num++;
	list_add_tail(&pdesc->node, &pchan->active);

	return ret;
}

int mxs_dma_desc_add_list(int channel, struct list_head *head)
{
	int ret = 0, size = 0;
	struct mxs_dma_chan *pchan;
	struct mxs_dma_device *pdma;
	struct list_head *p;
	struct mxs_dma_desc *prev = NULL, *pcur;
	if ((channel < 0) || (channel >= MXS_MAX_DMA_CHANNELS))
		return -EINVAL;
	pchan = mxs_dma_channels + channel;
	if (!(pchan->flags & MXS_DMA_FLAGS_ALLOCATED))
		return -EINVAL;

	if (list_empty(head))
		return 0;

	pdma = pchan->dma;
	list_for_each(p, head) {
		pcur = list_entry(p, struct mxs_dma_desc, node);
		if (!(pcur->cmd.cmd.bits.dec_sem || pcur->cmd.cmd.bits.chain))
			return -EINVAL;
		if (prev)
#ifdef CONFIG_ARCH_MMU
			prev->cmd.next =
				iomem_to_phys(mxs_dma_cmd_address(pcur));
#else
			prev->cmd.next = mxs_dma_cmd_address(pcur);
#endif
		else
			pcur->flags |= MXS_DMA_DESC_FIRST;
		pcur->flags |= MXS_DMA_DESC_READY;
		prev = pcur;
		size++;
	}
	pcur = list_first_entry(head, struct mxs_dma_desc, node);
#ifdef CONFIG_ARCH_MMU
	prev->cmd.next = iomem_to_phys(mxs_dma_cmd_address(pcur));
#else
	prev->cmd.next = mxs_dma_cmd_address(pcur);
#endif
	prev->flags |= MXS_DMA_DESC_LAST;

	if (!list_empty(&pchan->active)) {
		pcur = list_entry(pchan->active.next,
				  struct mxs_dma_desc, node);
		if (pcur->cmd.cmd.bits.dec_sem != prev->cmd.cmd.bits.dec_sem) {
			ret = -EFAULT;
			goto out ;
		}
#ifdef CONFIG_ARCH_MMU
		prev->cmd.next = iomem_to_phys(mxs_dma_cmd_address(pcur));
#else
		prev->cmd.next = mxs_dma_cmd_address(pcur);
#endif
		prev = list_entry(pchan->active.prev,
				  struct mxs_dma_desc, node);
		pcur = list_first_entry(head, struct mxs_dma_desc, node);
		pcur->flags &= ~MXS_DMA_DESC_FIRST;
		prev->flags &= ~MXS_DMA_DESC_LAST;
#ifdef CONFIG_ARCH_MMU
		prev->cmd.next = iomem_to_phys(mxs_dma_cmd_address(pcur));
#else
		prev->cmd.next = mxs_dma_cmd_address(pcur);
#endif
	}
	list_splice(head, &pchan->active);
	pchan->pending_num += size;
	if (!(pcur->cmd.cmd.bits.dec_sem) && (pcur->flags & MXS_DMA_DESC_FIRST))
		pchan->pending_num += 1;
	else
		pchan->pending_num += size;

out:
	return ret;
}

int mxs_dma_get_cooked(int channel, struct list_head *head)
{
	struct mxs_dma_chan *pchan;

	if ((channel < 0) || (channel >= MXS_MAX_DMA_CHANNELS))
		return -EINVAL;
	pchan = mxs_dma_channels + channel;
	if (!(pchan->flags & MXS_DMA_FLAGS_ALLOCATED))
		return -EINVAL;

	if (head == NULL)
		return 0;

	list_splice(&pchan->done, head);

	return 0;
}

int mxs_dma_device_register(struct mxs_dma_device *pdev)
{
	int i;
	struct mxs_dma_chan *pchan;

	if (pdev == NULL || !pdev->chan_num)
		return -EINVAL;

	if ((pdev->chan_base >= MXS_MAX_DMA_CHANNELS) ||
	    ((pdev->chan_base + pdev->chan_num) > MXS_MAX_DMA_CHANNELS))
		return -EINVAL;

	pchan = mxs_dma_channels + pdev->chan_base;
	for (i = 0; i < pdev->chan_num; i++, pchan++) {
		pchan->dma = pdev;
		pchan->flags = MXS_DMA_FLAGS_VALID;
	}
	list_add(&pdev->node, &mxs_dma_devices);

	return 0;
}

/* DMA Operation */
int mxs_dma_init(void)
{
	s32 dma_channel = 0, err = 0;

	mxs_dma_apbh_probe();

	for (dma_channel = MXS_DMA_CHANNEL_AHB_APBH_GPMI0;
		dma_channel <= MXS_DMA_CHANNEL_AHB_APBH_GPMI7;
		++dma_channel) {
		err = mxs_dma_request(dma_channel);

		if (err) {
			printf("Can't acquire DMA channel %u\n", dma_channel);

			/* Free all the channels we've already acquired. */
			while (--dma_channel >= 0)
				mxs_dma_release(dma_channel);
			return err;
		}

		mxs_dma_reset(dma_channel);
		mxs_dma_ack_irq(dma_channel);
	}

	return 0;
}

int mxs_dma_wait_complete(u32 uSecTimeout, unsigned int chan)
{
	struct mxs_dma_chan *pchan;
	struct mxs_dma_device *pdma;

	if ((chan < 0) || (chan >= MXS_MAX_DMA_CHANNELS))
		return 1;

	pchan = mxs_dma_channels + chan;
	if (!(pchan->flags & MXS_DMA_FLAGS_ALLOCATED))
		return 1;
	pdma = pchan->dma;

	while ((!(REG_RD(pdma->base, HW_APBH_CTRL1) & (1 << chan))) &&
		--uSecTimeout)
		;

	if (uSecTimeout <= 0) {
		/* Abort dma by resetting channel */
		mxs_dma_apbh_reset(pdma, chan - pdma->chan_base);
		return 1;
	}

	return 0;
}

int mxs_dma_go(int chan)
{
	u32 timeout = 10000;
	int  error;

	LIST_HEAD(tmp_desc_list);

	/* Get ready... */
	mxs_dma_enable_irq(chan, 1);

	/* Go! */
	mxs_dma_enable(chan);

	/* Wait for it to finish. */
	error = (mxs_dma_wait_complete(timeout, chan)) ? -ETIMEDOUT : 0;

	/* Clear out the descriptors we just ran. */
	mxs_dma_cooked(chan, &tmp_desc_list);

	/* Shut the DMA channel down. */
	/* Clear irq */
	mxs_dma_ack_irq(chan);
	mxs_dma_reset(chan);
	mxs_dma_enable_irq(chan, 0);
	mxs_dma_disable(chan);

	/* Return. */
	return error;
}

