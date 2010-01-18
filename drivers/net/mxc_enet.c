/*
 * (C) Copyright 2000-2004
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2008-2010 Freescale Semiconductor, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <malloc.h>
#include <net.h>
#include <miiphy.h>
#include <asm/arch/mx28.h>
#include <asm/arch/regs-enet.h>
#include <asm/arch/regs-clkctrl.h>

/*
 * Debug message switch
 */
#undef	MXC_ENET_DEBUG

/*
 * Buffer descriptor control/status used by Ethernet receive.
 */
#define BD_ENET_RX_EMPTY	((ushort)0x8000)
#define BD_ENET_RX_RO1		((ushort)0x4000)
#define BD_ENET_RX_WRAP		((ushort)0x2000)
#define BD_ENET_RX_INTR		((ushort)0x1000)
#define BD_ENET_RX_RO2		BD_ENET_RX_INTR
#define BD_ENET_RX_LAST		((ushort)0x0800)
#define BD_ENET_RX_FIRST	((ushort)0x0400)
#define BD_ENET_RX_MISS		((ushort)0x0100)
#define BD_ENET_RX_BC		((ushort)0x0080)
#define BD_ENET_RX_MC		((ushort)0x0040)
#define BD_ENET_RX_LG		((ushort)0x0020)
#define BD_ENET_RX_NO		((ushort)0x0010)
#define BD_ENET_RX_SH		((ushort)0x0008)
#define BD_ENET_RX_CR		((ushort)0x0004)
#define BD_ENET_RX_OV		((ushort)0x0002)
#define BD_ENET_RX_CL		((ushort)0x0001)
#define BD_ENET_RX_TR		BD_ENET_RX_CL
#define BD_ENET_RX_STATS	((ushort)0x013f)	/* All status bits */

/*
 * Buffer descriptor control/status used by Ethernet transmit.
 */
#define BD_ENET_TX_READY	((ushort)0x8000)
#define BD_ENET_TX_PAD		((ushort)0x4000)
#define BD_ENET_TX_TO1		BD_ENET_TX_PAD
#define BD_ENET_TX_WRAP		((ushort)0x2000)
#define BD_ENET_TX_INTR		((ushort)0x1000)
#define BD_ENET_TX_TO2		BD_ENET_TX_INTR_
#define BD_ENET_TX_LAST		((ushort)0x0800)
#define BD_ENET_TX_TC		((ushort)0x0400)
#define BD_ENET_TX_DEF		((ushort)0x0200)
#define BD_ENET_TX_ABC		BD_ENET_TX_DEF
#define BD_ENET_TX_HB		((ushort)0x0100)
#define BD_ENET_TX_LC		((ushort)0x0080)
#define BD_ENET_TX_RL		((ushort)0x0040)
#define BD_ENET_TX_RCMASK	((ushort)0x003c)
#define BD_ENET_TX_UN		((ushort)0x0002)
#define BD_ENET_TX_CSL		((ushort)0x0001)
#define BD_ENET_TX_STATS	((ushort)0x03ff)	/* All status bits */

/*
 * Buffer descriptors
 */
typedef struct cpm_buf_desc {
	ushort cbd_datlen;	/* Data length in buffer */
	ushort cbd_sc;		/* Status and Control */
	uint cbd_bufaddr;	/* Buffer address in host memory */
} cbd_t;

/* ENET private information */
struct enet_info_s {
	int index;
	u32 iobase;
	int phy_addr;
	int dup_spd;
	char *phy_name;
	int phyname_init;
	cbd_t *rxbd;		/* Rx BD */
	cbd_t *txbd;		/* Tx BD */
	uint rxIdx;
	uint txIdx;
	char *rxbuf;
	char *txbuf;
	int initialized;
	struct enet_info_s *next;
};

/* Register read/write struct */
typedef struct enet {
	u32 resv0;		/* 0x0000 */
	u32 eir;		/* 0x0004 */
	u32 eimr;		/* 0x0008 */
	u32 resv1;		/* 0x000c */
	u32 rdar;		/* 0x0010 */
	u32 tdar;		/* 0x0014 */
	u32 resv2[3];		/* 0x0018 */
	u32 ecr;		/* 0x0024 */
	u32 resv3[6];		/* 0x0028 */
	u32 mmfr;		/* 0x0040 */
	u32 mscr;		/* 0x0044 */
	u32 resv4[7];		/* 0x0048 */
	u32 mibc;		/* 0x0064 */
	u32 resv5[7];		/* 0x0068 */
	u32 rcr;		/* 0x0084 */
	u32 resv6[15];		/* 0x0088 */
	u32 tcr;		/* 0x00c4 */
	u32 resv7[7];		/* 0x00c8 */
	u32 palr;		/* 0x00e4 */
	u32 paur;		/* 0x00e8 */
	u32 opd;		/* 0x00ec */
	u32 resv8[10];		/* 0x00f0 */
	u32 iaur;		/* 0x0118 */
	u32 ialr;		/* 0x011c */
	u32 gaur;		/* 0x0120 */
	u32 galr;		/* 0x0124 */
	u32 resv9[7];		/* 0x0128 */
	u32 tfwr;		/* 0x0144 */
	u32 resv10;		/* 0x0148 */
	u32 frbr;		/* 0x014c */
	u32 frsr;		/* 0x0150 */
	u32 resv11[11];		/* 0x0154 */
	u32 erdsr;		/* 0x0180 */
	u32 etdsr;		/* 0x0184 */
	u32 emrbr;		/* 0x0188 */
	/* Unused registers  ... */
} enet_t;

/*
 * Ethernet Transmit and Receive Buffers
 */
#define DBUF_LENGTH			1520
#define RX_BUF_CNT			(PKTBUFSRX)
#define TX_BUF_CNT			(RX_BUF_CNT)
#define PKT_MINBUF_SIZE			64
#define PKT_MAXBLR_SIZE			1520
#define LAST_RX_BUF			(RX_BUF_CNT - 1)
#define BD_ENET_RX_W_E			(BD_ENET_RX_WRAP | BD_ENET_RX_EMPTY)
#define BD_ENET_TX_RDY_LST		(BD_ENET_TX_READY | BD_ENET_TX_LAST)

/*
 * MII definitions
 */
#define ENET_MII_ST			0x40000000
#define ENET_MII_OP_OFF			28
#define ENET_MII_OP_MASK		0x03
#define ENET_MII_OP_RD			0x02
#define ENET_MII_OP_WR			0x01
#define ENET_MII_PA_OFF			23
#define ENET_MII_PA_MASK		0xFF
#define ENET_MII_RA_OFF			18
#define ENET_MII_RA_MASK		0xFF
#define ENET_MII_TA			0x00020000
#define ENET_MII_DATA_OFF		0
#define ENET_MII_DATA_MASK		0x0000FFFF

#define ENET_MII_FRAME	\
			(ENET_MII_ST | ENET_MII_TA)

#define ENET_MII_OP(x)	\
			(((x) & ENET_MII_OP_MASK) << ENET_MII_OP_OFF)

#define ENET_MII_PA(pa)	\
			(((pa) & ENET_MII_PA_MASK) << ENET_MII_PA_OFF)

#define ENET_MII_RA(ra)	\
			(((ra) & ENET_MII_RA_MASK) << ENET_MII_RA_OFF)

#define ENET_MII_SET_DATA(v)	\
			(((v) & ENET_MII_DATA_MASK) << ENET_MII_DATA_OFF)

#define ENET_MII_GET_DATA(v) \
			(((v) >> ENET_MII_DATA_OFF) & ENET_MII_DATA_MASK)

#define ENET_MII_READ(pa, ra)	\
		((ENET_MII_FRAME | ENET_MII_OP(ENET_MII_OP_RD)) | \
		ENET_MII_PA(pa) | ENET_MII_RA(ra))

#define ENET_MII_WRITE(pa, ra, v) \
		(ENET_MII_FRAME | ENET_MII_OP(ENET_MII_OP_WR) | \
		ENET_MII_PA(pa) | ENET_MII_RA(ra) | ENET_MII_SET_DATA(v))

#define ENET_MII_TIMEOUT		50000
#define ENET_MII_TICK			2
#define ENET_MII_PHYADDR		0x0

/*
 * Misc definitions
 */
#ifndef CONFIG_SYS_CACHELINE_SIZE
#define CONFIG_SYS_CACHELINE_SIZE	32
#endif

#define	ENET_RESET_DELAY		100
#define ENET_MAX_TIMEOUT		50000
#define ENET_TIMEOUT_TICKET		2

#define __swap_32(x)	((((unsigned long)x) << 24) | \
			((0x0000FF00UL & ((unsigned long)x)) << 8) | \
			((0x00FF0000UL & ((unsigned long)x)) >> 8) | \
			(((unsigned long)x) >> 24))

/*
 * Functions
 */
extern void enet_board_init(void);

#if defined(CONFIG_CMD_NET) && defined(CONFIG_NET_MULTI)
static struct enet_info_s enet_info[] = {
	{
		0,			/* index */
		REGS_ENET_BASE,		/* io base */
#ifdef CONFIG_DISCOVER_PHY
		-1,			/* discover phy_addr  */
#else
		ENET_MII_PHYADDR,	/* phy_addr for MAC0 */
#endif
		0,			/* duplex and speed */
		0,			/* phy name */
		0,			/* phyname init */
		0,			/* RX BD */
		0,			/* TX BD */
		0,			/* rx Index */
		0,			/* tx Index */
		0,			/* tx buffer */
		0			/* initialized flag */
	}
};

static inline int __enet_mii_read(volatile enet_t *enetp, unsigned char addr,
				 unsigned char reg, unsigned short *value)
{
	int waiting = ENET_MII_TIMEOUT;
	if (enetp->eir & BM_ENET_MAC0_EIR_MII)
		enetp->eir |= BM_ENET_MAC0_EIR_MII;

	enetp->mmfr = ENET_MII_READ(addr, reg);
	while (1) {
		if (enetp->eir & BM_ENET_MAC0_EIR_MII) {
			enetp->eir |= BM_ENET_MAC0_EIR_MII;
			break;
		}
		if ((waiting--) <= 0)
			return -1;
		udelay(ENET_MII_TICK);
	}
	*value = ENET_MII_GET_DATA(enetp->mmfr);
	return 0;
}

static inline int __enet_mii_write(volatile enet_t *enetp, unsigned char addr,
				  unsigned char reg, unsigned short value)
{
	int waiting = ENET_MII_TIMEOUT;
	if (enetp->eir & BM_ENET_MAC0_EIR_MII)
		enetp->eir |= BM_ENET_MAC0_EIR_MII;

	enetp->mmfr = ENET_MII_WRITE(addr, reg, value);
	while (1) {
		if (enetp->eir & BM_ENET_MAC0_EIR_MII) {
			enetp->eir |= BM_ENET_MAC0_EIR_MII;
			break;
		}
		if ((waiting--) <= 0)
			return -1;
		udelay(ENET_MII_TICK);
	}
	return 0;
}

static int mxc_enet_mii_read(char *devname, unsigned char addr,
			    unsigned char reg, unsigned short *value)
{
	struct eth_device *dev = eth_get_dev_by_name(devname);
	struct enet_info_s *info;
	volatile enet_t *enetp;

	if (!dev)
		return -1;
	info = dev->priv;
	enetp = (enet_t *) (info->iobase);
	return __enet_mii_read(enetp, addr, reg, value);
}

static int mxc_enet_mii_write(char *devname, unsigned char addr,
			     unsigned char reg, unsigned short value)
{
	struct eth_device *dev = eth_get_dev_by_name(devname);
	struct enet_info_s *info;
	volatile enet_t *enetp;
	if (!dev)
		return -1;
	info = dev->priv;
	enetp = (enet_t *) (info->iobase);
	return __enet_mii_write(enetp, addr, reg, value);
}

static void mxc_enet_mii_init(volatile enet_t *enetp)
{
	/* Set RMII mode */
	enetp->rcr |= BM_ENET_MAC0_RCR_RMII_MODE;

	/* The phy requires MDC clock below 2.5 MHz */
	enetp->mscr |= (enetp->mscr & ~BM_ENET_MAC0_MSCR_MII_SPEED) |
			(40 << BP_ENET_MAC0_MSCR_MII_SPEED);
}

#ifdef CONFIG_DISCOVER_PHY
static inline int __enet_mii_info(volatile enet_t *enetp, unsigned char addr)
{
	unsigned int id = 0;
	unsigned short val;

	if (__enet_mii_read(enetp, addr, PHY_PHYIDR2, &val) != 0)
		return -1;
	id = val;
	if (id == 0xffff)
		return -1;

	if (__enet_mii_read(enetp, addr, PHY_PHYIDR1, &val) != 0)
		return -1;

	if (val == 0xffff)
		return -1;

	id |= val << 16;

#ifdef MXC_ENMXC_ENET_DEBUG
	printf("PHY indentify @ 0x%x = 0x%08x\n", addr, id);
#endif
	return 0;
}

static int mxc_enet_mii_discover_phy(struct eth_device *dev)
{
	unsigned short addr, val;
	struct enet_info_s *info = dev->priv;
	volatile enet_t *enetp = (enet_t *) (info->iobase);

	/* Dummy read with delay to get phy start working */
	do {
		__enet_mii_read(enetp, ENET_MII_PHYADDR, PHY_PHYIDR1, &val);
		udelay(10000);
#ifdef MXC_ENET_DEBUG
		printf("Dummy read on phy\n");
#endif
	} while (val == 0 || val == 0xffff);

	/* Read phy ID */
	for (addr = 0; addr < 0x20; addr++) {
		if (!__enet_mii_info(enetp, addr))
			return addr;
	}

	return -1;
}
#endif

static void set_duplex_speed(volatile enet_t *enetp,  unsigned char addr,
			      int dup_spd)
{
	unsigned short val;
	int ret;

	ret = __enet_mii_read(enetp, addr, PHY_BMCR, &val);
	switch (dup_spd >> 16) {
	case HALF:
		val &= (~PHY_BMCR_DPLX);
		break;
	case FULL:
		val |= PHY_BMCR_DPLX;
		break;
	default:
		val |= PHY_BMCR_AUTON | PHY_BMCR_RST_NEG;
	}
	ret |= __enet_mii_write(enetp, addr, PHY_BMCR, val);

	if (!ret && (val & PHY_BMCR_AUTON)) {
		ret = 0;
		while (ret++ < ENET_MII_TIMEOUT) {
			if (__enet_mii_read(enetp, addr, PHY_BMSR, &val))
				break;
			if (!(val & PHY_BMSR_AUTN_ABLE))
				break;
			if (val & PHY_BMSR_AUTN_COMP)
				break;
		}
	}

	if (__enet_mii_read(enetp, addr, PHY_BMSR, &val)) {
		dup_spd = _100BASET | (FULL << 16);
	} else {
		if (val & (PHY_BMSR_100TXF | PHY_BMSR_100TXH | PHY_BMSR_100T4))
			dup_spd = _100BASET;
		else
			dup_spd = _10BASET;
		if (val & (PHY_BMSR_100TXF | PHY_BMSR_10TF))
			dup_spd |= (FULL << 16);
		else
			dup_spd |= (HALF << 16);
	}

	if ((dup_spd >> 16) == FULL) {
		enetp->tcr |= BM_ENET_MAC0_TCR_FEDN;
#ifdef MXC_ENET_DEBUG
		printf("full duplex, ");
#endif
	} else {
		enetp->rcr |= BM_ENET_MAC0_RCR_DRT;
		enetp->tcr &= ~BM_ENET_MAC0_TCR_FEDN;
#ifdef MXC_ENET_DEBUG
		printf("half duplex, ");
#endif
	}
#ifdef MXC_ENET_DEBUG
	if ((dup_spd & 0xffff) == _100BASET)
		printf("100 Mbps\n");
	else
		printf("10 Mbps\n");
#endif
}

static void copy_packet(char *pdst, char *psrc, int length)
{
	long *pldst = (long *)pdst;
	long *plsrc = (long *)psrc;

	length /= sizeof(long);
	while (length--) {
		*pldst = __swap_32(*plsrc);
		pldst++;
		plsrc++;
	}
}

static int enet_send(struct eth_device *dev, volatile void *packet, int length)
{
	struct enet_info_s *info = dev->priv;
	volatile enet_t *enetp = (enet_t *) (info->iobase);
	int i;
	u16 phyStatus;

	__enet_mii_read(enetp, info->phy_addr, PHY_BMSR, &phyStatus);

	if (!(phyStatus & PHY_BMSR_LS)) {
		printf("ENET: Link is down %x\n", phyStatus);
		return -1;
	}

	/* Wait for ready */
	i = 0;
	while ((info->txbd[info->txIdx].cbd_sc & BD_ENET_TX_READY) &&
	       (i < ENET_MAX_TIMEOUT)) {
		udelay(ENET_TIMEOUT_TICKET);
		i++;
	}
	if (i >= ENET_MAX_TIMEOUT)
		printf("TX buffer is NOT ready\n");

	/* Manipulate the packet buffer */
	copy_packet((char *)info->txbd[info->txIdx].cbd_bufaddr,
		(char *)packet, length + (4 - length % 4));

	/* Set up transmit Buffer Descriptor */
	info->txbd[info->txIdx].cbd_datlen = length;
	info->txbd[info->txIdx].cbd_sc =
	    (info->txbd[info->txIdx].cbd_sc & BD_ENET_TX_WRAP) |
	    BD_ENET_TX_TC | BD_ENET_TX_RDY_LST;

	/* Activate transmit Buffer Descriptor polling */
	enetp->tdar = 0x01000000;

	/* Move Buffer Descriptor to the next */
	info->txIdx = (info->txIdx + 1) % TX_BUF_CNT;

	return length;
}

static int enet_recv(struct eth_device *dev)
{
	struct enet_info_s *info = dev->priv;
	volatile enet_t *enetp = (enet_t *) (info->iobase);
	int length;

	for (;;) {
		if (info->rxbd[info->rxIdx].cbd_sc & BD_ENET_RX_EMPTY) {
			length = -1;
			break;	/* nothing received - leave for() loop */
		}

		length = info->rxbd[info->rxIdx].cbd_datlen;

		if (info->rxbd[info->rxIdx].cbd_sc & 0x003f) {
#ifdef MXC_ENET_DEBUG
			printf("%s[%d] err: %x\n",
			       __func__, __LINE__,
			       info->rxbd[info->rxIdx].cbd_sc);
#endif
		} else {
			length -= 4;

			/* Manipulate the packet buffer */
			copy_packet((char *)NetRxPackets[info->rxIdx],
				(char *)info->rxbd[info->rxIdx].cbd_bufaddr,
				length + (4 - length % 4));

			/* Pass the packet up to the protocol layers. */
			NetReceive(NetRxPackets[info->rxIdx], length);
		}

		/* Give the buffer back to the ENET */
		info->rxbd[info->rxIdx].cbd_datlen = 0;

		/* Wrap around buffer index when necessary */
		if (info->rxIdx == LAST_RX_BUF) {
			info->rxbd[RX_BUF_CNT - 1].cbd_sc = BD_ENET_RX_W_E;
			info->rxIdx = 0;
		} else {
			info->rxbd[info->rxIdx].cbd_sc = BD_ENET_RX_EMPTY;
			info->rxIdx++;
		}

		/* Try to fill Buffer Descriptors */
		enetp->rdar = 0x01000000;
	}

	return length;
}

static void enet_reset(struct eth_device *dev)
{
	int i;
	struct enet_info_s *info = dev->priv;
	volatile enet_t *enetp = (enet_t *) (info->iobase);

	enetp->ecr |= BM_ENET_MAC0_ECR_RESET;
	for (i = 0; (enetp->ecr & BM_ENET_MAC0_ECR_RESET) && (i < ENET_RESET_DELAY); ++i)
		udelay(1);

	if (i == ENET_RESET_DELAY)
		printf("ENET reset timeout\n");
}

static void enet_halt(struct eth_device *dev)
{
	struct enet_info_s *info = dev->priv;

	/* Reset ENET controller to stop transmit and receive */
	enet_reset(dev);

	/* Clear buffers */
	info->rxIdx = info->txIdx = 0;
	memset(info->rxbd, 0, RX_BUF_CNT * sizeof(cbd_t));
	memset(info->txbd, 0, TX_BUF_CNT * sizeof(cbd_t));
	memset(info->rxbuf, 0, RX_BUF_CNT *  DBUF_LENGTH);
	memset(info->txbuf, 0, TX_BUF_CNT *  DBUF_LENGTH);
}

static int enet_init(struct eth_device *dev, bd_t *bd)
{
	struct enet_info_s *info = dev->priv;
	volatile enet_t *enetp = (enet_t *) (info->iobase);
	int i;
	u8 *ea = NULL;

	/* Turn on ENET clocks */
	REG_WR(REGS_CLKCTRL_BASE, HW_CLKCTRL_ENET,
		REG_RD(REGS_CLKCTRL_BASE, HW_CLKCTRL_ENET) &
		~(BM_CLKCTRL_ENET_SLEEP | BM_CLKCTRL_ENET_DISABLE));

	/* Set up ENET PLL for 50 MHz */
	REG_SET(REGS_CLKCTRL_BASE, HW_CLKCTRL_PLL2CTRL0,
		BM_CLKCTRL_PLL2CTRL0_POWER);	/* Power on ENET PLL */
	udelay(10);				/* Wait 10 us */
	REG_CLR(REGS_CLKCTRL_BASE, HW_CLKCTRL_PLL2CTRL0,
		BM_CLKCTRL_PLL2CTRL0_CLKGATE);	/* Gate on ENET PLL */
	REG_WR(REGS_CLKCTRL_BASE, HW_CLKCTRL_ENET,
		REG_RD(REGS_CLKCTRL_BASE, HW_CLKCTRL_ENET) |
		BM_CLKCTRL_ENET_CLK_OUT_EN);	/* Enable pad output */

	/* Board level init */
	enet_board_init();

	/* Reset ENET controller */
	enet_reset(dev);

#if defined(CONFIG_CMD_MII) || defined(CONFIG_MII) || \
	defined(CONFIG_DISCOVER_PHY)
	mxc_enet_mii_init(enetp);
#ifdef CONFIG_DISCOVER_PHY
	if (info->phy_addr < 0 || info->phy_addr > 0x1f)
		info->phy_addr = mxc_enet_mii_discover_phy(dev);
#endif
	set_duplex_speed(enetp, (unsigned char)info->phy_addr, info->dup_spd);
#else
#ifndef CONFIG_DISCOVER_PHY
	set_duplex_speed(enetp, (unsigned char)info->phy_addr,
				(ENETDUPLEX << 16) | ENETSPEED);
#endif
#endif
	/* We use strictly polling mode only */
	enetp->eimr = 0;

	/* Clear any pending interrupt */
	enetp->eir = 0xffffffff;

	/* Disable loopback mode */
	enetp->rcr &= ~BM_ENET_MAC0_RCR_LOOP;

	/* Enable RX flow control */
	enetp->rcr |= BM_ENET_MAC0_RCR_FCE;

	/* Set station address and enable it for TX  */
	ea = dev->enetaddr;
	enetp->palr = (ea[0] << 24) | (ea[1] << 16) | (ea[2] << 8) | (ea[3]);
	enetp->paur = (ea[4] << 24) | (ea[5] << 16);
	enetp->tcr |= BM_ENET_MAC0_TCR_TX_ADDR_INS;

	/* Clear unicast address hash table */
	enetp->iaur = 0;
	enetp->ialr = 0;

	/* Clear multicast address hash table */
	enetp->gaur = 0;
	enetp->galr = 0;

	/* Set maximum receive buffer size. */
	enetp->emrbr = PKT_MAXBLR_SIZE;

	/* Setup Buffers and Buffer Desriptors */
	info->rxIdx = 0;
	info->txIdx = 0;

	/*
	 * Setup Receiver Buffer Descriptors
	 * Settings:
	 *     Empty, Wrap
	 */
	for (i = 0; i < RX_BUF_CNT; i++) {
		info->rxbd[i].cbd_sc = BD_ENET_RX_EMPTY;
		info->rxbd[i].cbd_datlen = 0;	/* Reset */
		info->rxbd[i].cbd_bufaddr =
			(uint) (&info->rxbuf[0] + i * DBUF_LENGTH);
	}
	info->rxbd[RX_BUF_CNT - 1].cbd_sc |= BD_ENET_RX_WRAP;

	/*
	 * Setup Transmitter Buffer Descriptors
	 * Settings:
	 *    Last, Tx CRC
	 */
	for (i = 0; i < TX_BUF_CNT; i++) {
		info->txbd[i].cbd_sc = BD_ENET_TX_LAST | BD_ENET_TX_TC;
		info->txbd[i].cbd_datlen = 0;	/* Reset */
		info->txbd[i].cbd_bufaddr =
			(uint) (&info->txbuf[0] + i * DBUF_LENGTH);
	}
	info->txbd[TX_BUF_CNT - 1].cbd_sc |= BD_ENET_TX_WRAP;

	/* Set receive and transmit descriptor base */
	enetp->erdsr = (unsigned int)(&info->rxbd[0]);
	enetp->etdsr = (unsigned int)(&info->txbd[0]);

	/* Now enable the transmit and receive processing */
	enetp->ecr |= BM_ENET_MAC0_ECR_ETHER_EN;

	/* And last, try to fill Rx Buffer Descriptors */
	enetp->rdar = 0x01000000;

	return 0;
}

int mxc_enet_initialize(bd_t *bis)
{
	struct eth_device *dev;
	int i;

	for (i = 0; i < sizeof(enet_info) / sizeof(enet_info[0]); i++) {
		dev = (struct eth_device *) memalign(CONFIG_SYS_CACHELINE_SIZE,
			 sizeof(*dev));
		if (dev == NULL)
			hang();

		memset(dev, 0, sizeof(*dev));

		/* Regiester device */
		sprintf(dev->name, "ENET%d", enet_info[i].index);
		dev->priv = &enet_info[i];
		dev->init = enet_init;
		dev->halt = enet_halt;
		dev->send = enet_send;
		dev->recv = enet_recv;
		eth_register(dev);
#if defined(CONFIG_MII) || defined(CONFIG_CMD_MII)
		miiphy_register(dev->name, mxc_enet_mii_read, mxc_enet_mii_write);
#endif
		/* Setup Receive and Transmit buffer descriptor */
		enet_info[i].rxbd =
			(cbd_t *) memalign(CONFIG_SYS_CACHELINE_SIZE,
				RX_BUF_CNT * sizeof(cbd_t));
		enet_info[i].rxbuf =
			(char *) memalign(CONFIG_SYS_CACHELINE_SIZE,
				RX_BUF_CNT * DBUF_LENGTH);
		enet_info[i].txbd =
			(cbd_t *) memalign(CONFIG_SYS_CACHELINE_SIZE,
				TX_BUF_CNT * sizeof(cbd_t));
		enet_info[i].txbuf =
			(char *) memalign(CONFIG_SYS_CACHELINE_SIZE,
				TX_BUF_CNT * DBUF_LENGTH);
		enet_info[i].phy_name =
			(char *)memalign(CONFIG_SYS_CACHELINE_SIZE, 32);
#ifdef MXC_ENET_DEBUG
		printf("%s: rxbd %x txbd %x ->%x\n", dev->name,
		       (int)enet_info[i].rxbd, (int)enet_info[i].txbd,
		       (int)enet_info[i].txbuf);
#endif
	}

	return 1;
}

#endif	/* defined(CONFIG_CMD_NET) && defined(CONFIG_NET_MULTI) */
