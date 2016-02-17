// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2016,2020 NXP
 * Heinz Wrobel <Heinz.Wrobel@nxp.com>
 *
 * Basic GIC support to permit dealing with interrupt handlers in ARMv8.
 * This code is currently only tested on S32V234, but should be generic
 * enough to be placed outside a CPU specific directory at some point.
 * We ignore SGI/PPI because that is done in gic_64.S.
 *
 * Some of this code is taken from sources developed by <Jay.Tu@nxp.com>
 */

#include <common.h>
#include <linux/compiler.h>
#include <asm/io.h>
#include <asm/gicsupport.h>
#include <asm/gic.h>
#include <asm/proc-armv/system.h>

#define GICD_INVALID_IRQ		(-1)

#define GICD_ICENABLERn_CLEAR_MASK	(0xffffffff)

#define HWIRQ_BIT(irq)			BIT((irq) & 0x1f)
#define HWIRQ_BANK(irq)			(((irq) >> 5) * sizeof(u32))

#define HWIRQ_BANK_2BITS(irq)		(((irq) >> 4) * sizeof(u32))

#define GICD_INT_DEF_TARGET		(0x01)
#define GICD_INT_DEF_TARGET_X4		((GICD_INT_DEF_TARGET << 24) |\
					(GICD_INT_DEF_TARGET << 16) |\
					(GICD_INT_DEF_TARGET << 8) |\
					GICD_INT_DEF_TARGET)

#define GICD_INT_DEF_PRI		(0xa0)
#define GICD_INT_DEF_PRI_X4		((GICD_INT_DEF_PRI << 24) |\
					(GICD_INT_DEF_PRI << 16) |\
					(GICD_INT_DEF_PRI << 8) |\
					GICD_INT_DEF_PRI)

#define GICD_TYPER_SPIS(typer)		((((typer) & 0x1f) + 1) * 32)
#define GICD_CTLR_ENABLE_G1A		BIT(1)
#define GICD_CTLR_ENABLE_G1		BIT(0)
#define GICR_PROPBASER_IDBITS_MASK	(0x1f)
#define GICC_PMR_PRIORITY		(0xff)

#define GICD_ICFGR_MASK			(0x3)
#define GICD_ICFGR_IRQ_SHIFT(irq)	(((irq) & 0x0f) * 2)
#define GICD_ICFGR_IRQ_MASK(irq)	(GICD_ICFGR_MASK << \
					GICD_ICFGR_IRQ_SHIFT(irq))
#define GICD_ICFGR_IRQ_TYPE(type, irq)	((type & GICD_ICFGR_MASK) << \
					GICD_ICFGR_IRQ_SHIFT(irq))

#define GIC_SUPPORTED_HANDLERS		(16)

#define GIC_GROUP_IRQ			(1022)
#define GIC_SPURIOUS_IRQ		(1023)

/* A performance implementation would use an indexed table.
 * U-Boot has a need for a small footprint, so we do a simple search
 * and only support a few handlers concurrently.
 */
static struct
{
	int irq;
	void (*handler)(struct pt_regs *pt_regs, unsigned int esr);
	const char *name;
	int type;
	u32 count;
} inthandlers[GIC_SUPPORTED_HANDLERS];

static u32 count_spurious;
static bool pre_init_done, full_init_done;

static void interrupt_handler_init(void)
{
	/* We need to decouple this init from the generic
	 * interrupt_init() function because interrupt handlers
	 * may be preregistered by code before the GIC init has
	 * happened! PCIe support is a good example for this.
	 */
	int i;

	if (!pre_init_done) {
		pre_init_done = 1;
		/* Make sure that we do not have any active handlers */
		for (i = 0; i < ARRAY_SIZE(inthandlers); i++) {
			inthandlers[i].irq = GICD_INVALID_IRQ;
			inthandlers[i].count = 0;
		}
		count_spurious = 0;
	}
}

/* Function to support handlers. Benign to call before GIC init */
static void gic_unmask_irq(unsigned int irq)
{
	u32 mask;

	if (full_init_done) {
		mask = HWIRQ_BIT(irq);
		writel(mask,
		       GICD_BASE + GICD_ISENABLERn + HWIRQ_BANK(irq));
	}
}

/* Function to support handlers. Benign to call before GIC init */
static void gic_mask_irq(unsigned int irq)
{
	u32 mask = HWIRQ_BIT(irq);

	writel(mask, GICD_BASE + GICD_ICENABLERn + HWIRQ_BANK(irq));
}

/* Function to support handlers. Benign to call before GIC init */
static void gic_set_type(unsigned int irq, int type)
{
	u32 icfgr;

	if (full_init_done) {
		icfgr = readl(GICD_BASE + GICD_ICFGR +
				HWIRQ_BANK_2BITS(irq));
		icfgr &= ~GICD_ICFGR_IRQ_MASK(irq);
		icfgr |= GICD_ICFGR_IRQ_TYPE(type, irq);
		writel(icfgr,
		       GICD_BASE + GICD_ICFGR + HWIRQ_BANK_2BITS(irq));
	}
}

static void gic_dist_init(unsigned long base)
{
	unsigned int gic_irqs, i;
	u32 ctlr = readl(base + GICD_CTLR);

	/* We turn off the GIC while we mess with it.
	 * The original init has happened in gic_64.S!
	 */
	writel(ctlr & ~(GICD_CTLR_ENABLE_G1A | GICD_CTLR_ENABLE_G1),
	       base + GICD_CTLR);

	gic_irqs = GICD_TYPER_SPIS(readl(base + GICD_TYPER));

	/*
	 * Set all global interrupts to be level triggered, active low.
	 */
	for (i = 32; i < gic_irqs; i += 16)
		writel(0, base + GICD_ICFGR + i / 4);

	/*
	 * Set all global interrupts to this CPU only.
	 */
	for (i = 32; i < gic_irqs; i += 4)
		writel(GICD_INT_DEF_TARGET_X4, base + GICD_ITARGETSRn + i);

	/*
	 * Set priority on all global interrupts.
	 */

	for (i = 32; i < gic_irqs; i += 4)
		writel(GICD_INT_DEF_PRI_X4, base + GICD_IPRIORITYRn + i);

	/*
	 * Disable all interrupts.  Leave the PPI and SGIs alone
	 * as these enables are banked registers.
	 */
	for (i = 32; i < gic_irqs; i += 32)
		writel(GICD_ICENABLERn_CLEAR_MASK,
		       base + GICD_ICENABLERn + i / 8);

	writel(ctlr, base + GICD_CTLR);
}

static void gic_cpu_clean(unsigned long dist_base, unsigned long cpu_base)
{
	/* Initially we need to make sure that we do not have any
	 * left over requests that could cause a mess during
	 * initialization. This happens during sloppy SMP init in
	 * lowlevel.S and should be FIXED. *sigh*
	 */
	writel(GICD_ICENABLERn_CLEAR_MASK, dist_base + GICD_ICENABLERn);
}

static void gic_cpu_init(unsigned long dist_base, unsigned long cpu_base)
{
	/* Accept just about anything */
	writel(GICC_PMR_PRIORITY, cpu_base + GICC_PMR);

	/* gic_64.S doesn't set the recommended AckCtl value. We do */
	writel(0x1e3, cpu_base + GICC_CTLR);
}

/* Public function to support handlers. Benign to call before GIC init */
int gic_irq_status(unsigned int irq)
{
	u32 mask = HWIRQ_BIT(irq);
	u32 v = readl(GICD_BASE + GICD_ISPENDRn + HWIRQ_BANK(irq));

	return !!(v & mask);
}

int gic_register_handler(int irq,
			 void (*handler)(struct pt_regs *pt_regs,
					 unsigned int esr), int type,
			 const char *name)
{
	int i;

	interrupt_handler_init();

	if (full_init_done)
		gic_mask_irq(irq);

	for (i = 0; i < ARRAY_SIZE(inthandlers); i++) {
		if (inthandlers[i].irq < 0) {
			inthandlers[i].handler = handler;
			inthandlers[i].name = name;
			inthandlers[i].type = type;

			/* Done last to avoid race condition */
			inthandlers[i].irq = irq;

			if (full_init_done) {
				gic_set_type(irq, type);
				gic_unmask_irq(irq);
			}
			break;
		}
	}

	return i >= ARRAY_SIZE(inthandlers) ? 0 : 1;
}

int interrupt_init(void)
{
	int i;

	interrupt_handler_init();

	gic_cpu_clean(GICD_BASE, GICC_BASE);

	gic_dist_init(GICD_BASE);

	/* U-Boot runs with a single CPU only */
	gic_cpu_init(GICD_BASE, GICC_BASE);

	full_init_done = 1;

	/* Now that we have set up the GIC, we need to start up
	 * any preregistered handlers.
	 */
	for (i = 0; i < ARRAY_SIZE(inthandlers); i++) {
		int irq = inthandlers[i].irq;

		if (irq >= 0) {
			gic_set_type(irq, inthandlers[i].type);
			gic_unmask_irq(irq);
		}
	}

	return 0;
}

void enable_interrupts(void)
{
	local_irq_enable();
}

int disable_interrupts(void)
{
	int flags;

	local_irq_save(flags);

	return flags;
}

void do_irq(struct pt_regs *pt_regs, unsigned int esr)
{
	int i, group = 0;
	u32 thisirq = readl(GICC_BASE + GICC_IAR);

	if (thisirq == GIC_GROUP_IRQ) {
		/* Group 1 interrupt! */
		group = 1;
		thisirq = readl(GICC_BASE + GICC_AIAR);
	}

	if (thisirq == GIC_SPURIOUS_IRQ) {
		count_spurious++;
		return;
	}

	for (i = 0; i < ARRAY_SIZE(inthandlers); i++) {
		if (inthandlers[i].irq == thisirq) {
			inthandlers[i].count++;
			inthandlers[i].handler(pt_regs, esr);

			break;
		}
	}

	if (group)
		writel(thisirq, GICC_BASE + GICC_AEOIR);
	else
		writel(thisirq, GICC_BASE + GICC_EOIR);

	if (i >= ARRAY_SIZE(inthandlers)) {
		printf("\"Irq\" handler, esr 0x%08x for GIC irq %d, group %d\n",
		       esr, thisirq, group);
		show_regs(pt_regs);
		panic("Resetting CPU ...\n");
	}
}

#if defined(CONFIG_CMD_IRQ)
int do_irqinfo(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int i;

	printf("GIC support is enabled for GIC @ 0x%08x\n", GICD_BASE);
	printf("Spurious: %d\n", count_spurious);
	for (i = 0; i < ARRAY_SIZE(inthandlers); i++) {
		if (inthandlers[i].irq >= 0) {
			printf("%20s(%d): %d\n", inthandlers[i].name,
			       inthandlers[i].irq,
			       inthandlers[i].count);
		}
	}

	return 0;
}
#endif
