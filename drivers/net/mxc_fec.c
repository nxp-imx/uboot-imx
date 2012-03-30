/*
 * (C) Copyright 2000-2004
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2008-2012 Freescale Semiconductor, Inc.
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

#ifdef CONFIG_ARCH_MMU
#include <asm/arch/mmu.h>
#endif

#include <common.h>
#include <malloc.h>

#include <asm/fec.h>

#include <command.h>
#include <net.h>
#include <miiphy.h>
#include <linux/types.h>
#include <asm/io.h>

#undef	ET_DEBUG
#undef	MII_DEBUG

/* Ethernet Transmit and Receive Buffers */
#define DBUF_LENGTH		1520
#define TX_BUF_CNT		2
#define PKT_MAXBUF_SIZE		1518
#define PKT_MINBUF_SIZE		64
#define PKT_MAXBLR_SIZE		1520
#define LAST_PKTBUFSRX		(PKTBUFSRX - 1)
#define BD_ENET_RX_W_E		(BD_ENET_RX_WRAP | BD_ENET_RX_EMPTY)
#define BD_ENET_TX_RDY_LST	(BD_ENET_TX_READY | BD_ENET_TX_LAST)

/* the defins of MII operation */
#define FEC_MII_ST      0x40000000
#define FEC_MII_OP_OFF  28
#define FEC_MII_OP_MASK 0x03
#define FEC_MII_OP_RD   0x02
#define FEC_MII_OP_WR   0x01
#define FEC_MII_PA_OFF  23
#define FEC_MII_PA_MASK 0xFF
#define FEC_MII_RA_OFF  18
#define FEC_MII_RA_MASK 0xFF
#define FEC_MII_TA      0x00020000
#define FEC_MII_DATA_OFF 0
#define FEC_MII_DATA_MASK 0x0000FFFF

#define FEC_MII_FRAME   (FEC_MII_ST | FEC_MII_TA)
#define FEC_MII_OP(x)   (((x) & FEC_MII_OP_MASK) << FEC_MII_OP_OFF)
#define FEC_MII_PA(pa)  (((pa) & FEC_MII_PA_MASK) << FEC_MII_PA_OFF)
#define FEC_MII_RA(ra)  (((ra) & FEC_MII_RA_MASK) << FEC_MII_RA_OFF)
#define FEC_MII_SET_DATA(v) (((v) & FEC_MII_DATA_MASK) << FEC_MII_DATA_OFF)
#define FEC_MII_GET_DATA(v) (((v) >> FEC_MII_DATA_OFF) & FEC_MII_DATA_MASK)
#define FEC_MII_READ(pa, ra) ((FEC_MII_FRAME | FEC_MII_OP(FEC_MII_OP_RD)) |\
					FEC_MII_PA(pa) | FEC_MII_RA(ra))
#define FEC_MII_WRITE(pa, ra, v) (FEC_MII_FRAME | FEC_MII_OP(FEC_MII_OP_WR)|\
			FEC_MII_PA(pa) | FEC_MII_RA(ra) | FEC_MII_SET_DATA(v))

#if defined(CONFIG_MX6Q) || defined(CONFIG_MX6DL)
extern int mx6_rgmii_rework(char *devname, int phy_addr);
#endif

#define PHY_MIPSCR_LINK_UP	(0x1 << 10)
#define PHY_MIPSCR_SPEED_MASK	(0x3 << 14)
#define PHY_MIPSCR_1000M	(0x2 << 14)
#define PHY_MIPSCR_100M		(0x1 << 14)
#define PHY_MIPSCR_FULL_DUPLEX	(0x1 << 13)

#ifndef CONFIG_SYS_CACHELINE_SIZE
#define CONFIG_SYS_CACHELINE_SIZE 32
#endif

#ifndef CONFIG_FEC_TIMEOUT
#define FEC_MII_TIMEOUT		50000
#else
#define FEC_MII_TIMEOUT		CONFIG_FEC_TIMEOUT
#endif

#ifndef CONFIG_FEC_TICKET
#define FEC_MII_TICK         	2
#else
#define FEC_MII_TICK         	CONFIG_FEC_TICKET
#endif

/* define phy ID */
#define PHY_OUID_MASK		0x1FFFFF8
#define PHY_OUID_GET(high_bits, low_bits) \
				((high_bits << 19) | (low_bits << 3))
#define PHY_ATHEROS_OUID_GET(valh, vall) \
				PHY_OUID_GET((valh & 0x3F), vall)
#define PHY_MICREL_OUID_GET(valh, vall) \
				PHY_OUID_GET((valh >> 10), vall)
#define PHY_AR8031_OUID         PHY_OUID_GET(0x34, 0x4D)
#define PHY_AR8033_OUID		PHY_OUID_GET(0x34, 0x4D)
#define PHY_KSZ9021_OUID	PHY_OUID_GET(0x5, 0x22)

/* Get phy ID */
static unsigned int fec_phy_ouid;
#define phy_is_ksz9021() (((fec_phy_ouid & PHY_OUID_MASK) == PHY_KSZ9021_OUID) \
			? 1 : 0)
#define phy_is_ar8031()	(((fec_phy_ouid & PHY_OUID_MASK) == PHY_AR8031_OUID) \
			? 1 : 0)
#define phy_is_ar8033()	(((fec_phy_ouid & PHY_OUID_MASK) == PHY_AR8033_OUID) \
			? 1 : 0)

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_MII_GASKET)
/*
 * RMII mode to be configured via a gasket
 */
#define FEC_MIIGSK_CFGR_FRCONT (1 << 6)
#define FEC_MIIGSK_CFGR_LBMODE (1 << 4)
#define FEC_MIIGSK_CFGR_EMODE (1 << 3)
#define FEC_MIIGSK_CFGR_IF_MODE_MASK (3 << 0)
#define FEC_MIIGSK_CFGR_IF_MODE_MII (0 << 0)
#define FEC_MIIGSK_CFGR_IF_MODE_RMII (1 << 0)

#define FEC_MIIGSK_ENR_READY (1 << 2)
#define FEC_MIIGSK_ENR_EN (1 << 1)

static inline void fec_localhw_setup(volatile fec_t *fecp)
{
	/*
	 * Set up the MII gasket for RMII mode
	 */
	printf("FEC: enable RMII gasket\n");

	/* disable the gasket and wait */
	fecp->fec_miigsk_enr = 0;
	while (fecp->fec_miigsk_enr & FEC_MIIGSK_ENR_READY)
		udelay(1);

	/* configure the gasket for RMII, 50 MHz, no loopback, no echo */
	fecp->fec_miigsk_cfgr = FEC_MIIGSK_CFGR_IF_MODE_RMII;

	/* re-enable the gasket */
	fecp->fec_miigsk_enr = FEC_MIIGSK_ENR_EN;

	while (!(fecp->fec_miigsk_enr & FEC_MIIGSK_ENR_READY))
		udelay(1);
}
#else
static inline void fec_localhw_setup(volatile fec_t *fecp)
{
}
#endif

#if defined(CONFIG_CMD_NET) && defined(CONFIG_NET_MULTI)

struct fec_info_s fec_info[] = {
	{
	 0,			/* index */
	 CONFIG_FEC0_IOBASE,	/* io base */
	 CONFIG_FEC0_PHY_ADDR,	/* phy_addr */
	 0,			/* duplex and speed */
	 0,			/* phy name */
	 0,			/* phyname init */
	 0,			/* RX BD */
	 0,			/* TX BD */
	 0,			/* rx Index */
	 0,			/* tx Index */
	 0,			/* tx buffer */
#ifdef CONFIG_ARCH_MMU
	 { 0 },			/* rx buffer */
#endif
	 0,			/* initialized flag */
	 },
};

static int mxc_fec_mii_read(char *devname, unsigned char addr,
			    unsigned char reg, unsigned short *value);
static int mxc_fec_mii_write(char *devname, unsigned char addr,
			     unsigned char reg, unsigned short value);
int fec_send(struct eth_device *dev, volatile void *packet, int length);
int fec_recv(struct eth_device *dev);
int fec_init(struct eth_device *dev, bd_t *bd);
void fec_halt(struct eth_device *dev);
void fec_reset(struct eth_device *dev);

static void mxc_fec_mii_init(volatile fec_t *fecp)
{
	u32 clk;
	clk = mxc_get_clock(MXC_IPG_CLK);

	fecp->mscr = (fecp->mscr & (~0x7E)) | (((clk + 499999) / 5000000) << 1);
}

static void mxc_fec_phy_powerup(char *devname, int phy_addr)
{
	unsigned short value;
	mxc_fec_mii_read(devname, phy_addr, PHY_BMCR, &value);
	if (value & PHY_BMCR_POWD)
		mxc_fec_mii_write(devname, phy_addr, PHY_BMCR, (value & ~PHY_BMCR_POWD));
}

static void mxc_get_phy_ouid(char *devname, int phy_addr)
{
	unsigned short value1, value2;

	mxc_fec_mii_read(devname, phy_addr, PHY_PHYIDR1, &value1);
	mxc_fec_mii_read(devname, phy_addr, PHY_PHYIDR2, &value2);

	fec_phy_ouid = PHY_ATHEROS_OUID_GET(value2, value1);
	if (phy_is_ar8031())
		return;
	else if (phy_is_ar8033())
		return;

	fec_phy_ouid = PHY_MICREL_OUID_GET(value2, value1);
	if (phy_is_ksz9021())
		return;

	fec_phy_ouid = 0xFFFFFFFF;
}

static inline int __fec_mii_read(volatile fec_t *fecp, unsigned char addr,
				 unsigned char reg, unsigned short *value)
{
	int waiting = FEC_MII_TIMEOUT;
	if (fecp->eir & FEC_EIR_MII)
		fecp->eir = FEC_EIR_MII;

	fecp->mmfr = FEC_MII_READ(addr, reg);
	while (1) {
		if (fecp->eir & FEC_EIR_MII) {
			fecp->eir = FEC_EIR_MII;
			break;
		}
		if ((waiting--) <= 0)
			return -1;
		udelay(FEC_MII_TICK);
	}
	*value = FEC_MII_GET_DATA(fecp->mmfr);
	return 0;
}

static inline int __fec_mii_write(volatile fec_t *fecp, unsigned char addr,
				  unsigned char reg, unsigned short value)
{
	int waiting = FEC_MII_TIMEOUT;
	if (fecp->eir & FEC_EIR_MII)
		fecp->eir = FEC_EIR_MII;

	fecp->mmfr = FEC_MII_WRITE(addr, reg, value);
	while (1) {
		if (fecp->eir & FEC_EIR_MII) {
			fecp->eir = FEC_EIR_MII;
			break;
		}
		if ((waiting--) <= 0)
			return -1;
		udelay(FEC_MII_TICK);
	}
	return 0;
}

static int mxc_fec_mii_read(char *devname, unsigned char addr,
			    unsigned char reg, unsigned short *value)
{
	struct eth_device *dev = eth_get_dev_by_name(devname);
	struct fec_info_s *info;
	volatile fec_t *fecp;

	if (!dev)
		return -1;
	info = dev->priv;
	fecp = (fec_t *) (info->iobase);
	return __fec_mii_read(fecp, addr, reg, value);
}

static int mxc_fec_mii_write(char *devname, unsigned char addr,
			     unsigned char reg, unsigned short value)
{
	struct eth_device *dev = eth_get_dev_by_name(devname);
	struct fec_info_s *info;
	volatile fec_t *fecp;
	if (!dev)
		return -1;
	info = dev->priv;
	fecp = (fec_t *) (info->iobase);
	return __fec_mii_write(fecp, addr, reg, value);
}

#ifdef CONFIG_DISCOVER_PHY
static inline int __fec_mii_info(volatile fec_t *fecp, unsigned char addr)
{
	unsigned int id = 0;
	unsigned short val;
	if (__fec_mii_read(fecp, addr, PHY_PHYIDR2, &val) != 0)
		return -1;
	id = val;
	if (id == 0xFFFF)
		return -1;

	if (__fec_mii_read(fecp, addr, PHY_PHYIDR1, &val) != 0)
		return -1;

	if (val == 0xFFFF)
		return -1;

	id |= val << 16;

	printf("PHY indentify @ 0x%x = 0x%08x\n", addr, id);
	return 0;
}

static int mxc_fec_mii_discover_phy(struct eth_device *dev)
{
	unsigned int addr;
	struct fec_info_s *info = dev->priv;
	volatile fec_t *fecp = (fec_t *) (info->iobase);
	for (addr = 0; addr < 0x20; addr++) {
		if (!__fec_mii_info(fecp, addr))
			return addr;
	}
	return -1;
}
#endif

static inline u16 getFecPhyStatus(volatile fec_t *fecp, unsigned char addr)
{
	unsigned short val;
	if (__fec_mii_read(fecp, addr, PHY_BMSR, &val) != 0)
		return 0;
	return val;
}

static void setFecDuplexSpeed(volatile fec_t *fecp, unsigned char addr,
			      int dup_spd)
{
	unsigned short val = 0;
	int ret, cnt;

#ifdef CONFIG_MX28
	/* Dummy read with delay to get phy start working */
	do {
		__fec_mii_read(fecp, CONFIG_FEC0_PHY_ADDR, PHY_PHYIDR1, &val);
		udelay(10000);
#ifdef MII_DEBUG
		printf("Dummy read on phy\n");
#endif
	} while (val == 0 || val == 0xffff);
#endif

	ret = __fec_mii_read(fecp, addr, PHY_BMCR, &val);
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

	ret |= __fec_mii_write(fecp, addr, PHY_BMCR, val);

	if (!ret && (val & PHY_BMCR_AUTON)) {
		ret = 0;
		while (ret++ < 100) {
			if (__fec_mii_read(fecp, addr, PHY_BMSR, &val))
				break;
			if (!(val & PHY_BMSR_AUTN_ABLE))
				break;
			if (val & PHY_BMSR_AUTN_COMP)
				break;
		};
	}

	/*wait the cable link*/
	for (cnt = 0; cnt < 200; cnt++) {
		val = 0;
		ret = __fec_mii_read(fecp, addr, PHY_BMSR, &val);
		if (val & PHY_BMSR_LS)
			break;
		udelay(100*1000);
	}

	if (ret) {
		/* set default mode to 100M full duplex */
		dup_spd = _100BASET | (FULL << 16);
	} else {
		if (val & PHY_BMSR_LS) {
			printf("FEC: Link is Up %x\n", val);
		} else {
			printf("FEC: Link is down %x\n", val);
		}

		/* for Atheros PHY */
		if (phy_is_ar8031() || phy_is_ar8033()) {
			ret = __fec_mii_read(fecp, addr, PHY_MIPSCR, &val);
			if (ret)
				dup_spd = _100BASET | (FULL << 16);
			else {
				if ((val & PHY_MIPSCR_SPEED_MASK) == PHY_MIPSCR_1000M)
					dup_spd = _1000BASET;
				else if ((val & PHY_MIPSCR_SPEED_MASK) == PHY_MIPSCR_100M)
					dup_spd = _100BASET;
				else
					dup_spd = _10BASET;

				if (val & PHY_MIPSCR_FULL_DUPLEX)
					dup_spd |= (FULL << 16);
				else
					dup_spd |= (HALF << 16);
			}
		} else if (phy_is_ksz9021()) { /* for Micrel phy */
			ret = __fec_mii_read(fecp, addr, 0x1f, &val);
			if (ret)
				dup_spd = _100BASET | (FULL << 16);
			else {
				if (val & (1 << 6))
					dup_spd = _1000BASET;
				else if (val & (1 << 5))
					dup_spd = _100BASET;
				else
					dup_spd = _10BASET;
				if (val & (1 << 3))
					dup_spd |= (FULL << 16);
				else
					dup_spd |= (HALF << 16);
			}
		} else { /* for SMSC and other unknow phys */
			ret = __fec_mii_read(fecp, addr, PHY_BMSR, &val);
			if (ret)
				dup_spd = _100BASET | (FULL << 16);
			else {
				if (val & (PHY_BMSR_100TXF | PHY_BMSR_100TXH | PHY_BMSR_100T4))
					dup_spd = _100BASET;
				else
					dup_spd = _10BASET;
				if (val & (PHY_BMSR_100TXF | PHY_BMSR_10TF))
					dup_spd |= (FULL << 16);
				else
					dup_spd |= (HALF << 16);
			}
		}
	}

	if ((dup_spd >> 16) == FULL) {
		/* Set maximum frame length */
#ifdef CONFIG_RMII
		fecp->rcr = FEC_RCR_MAX_FL(PKT_MAXBUF_SIZE) | FEC_RCR_RMII_MODE;
#else
		fecp->rcr = FEC_RCR_MAX_FL(PKT_MAXBUF_SIZE) | FEC_RCR_MII_MODE;
#endif
		fecp->tcr = FEC_TCR_FDEN;
	} else {
		/* Half duplex mode */
#ifdef CONFIG_RMII
		fecp->rcr = FEC_RCR_MAX_FL(PKT_MAXBUF_SIZE) |
			FEC_RCR_RMII_MODE | FEC_RCR_DRT;
#else
		fecp->rcr = FEC_RCR_MAX_FL(PKT_MAXBUF_SIZE) |
			FEC_RCR_MII_MODE | FEC_RCR_DRT;
#endif
		fecp->tcr &= ~FEC_TCR_FDEN;
	}

#if defined(CONFIG_MX6Q) || defined(CONFIG_MX6DL)
	if ((dup_spd & 0xFFFF) == _1000BASET) {
		fecp->ecr |= (0x1 << 5);
	} else if ((dup_spd & 0xFFFF) == _100BASET) {
		fecp->ecr &= ~(0x1 << 5);
	} else {
		fecp->ecr &= ~(0x1 << 5);
		fecp->rcr |= (0x1 << 9);
	}
#endif

#ifdef ET_DEBUG
	if ((dup_spd & 0xFFFF) == _1000BASET)
		printf("1000Mbps\n");
	else if ((dup_spd & 0xFFFF) == _100BASET)
		printf("100Mbps\n");
	else
		printf("10Mbps\n");
#endif
}

#ifdef CONFIG_MX28
static void swap_packet(void *packet, int length)
{
	int i;
	unsigned int *buf = packet;

	for (i = 0; i < (length + 3) / 4; i++, buf++)
		*buf = __swab32(*buf);
}
#endif

extern int fec_get_mac_addr(unsigned char *mac);
static int fec_get_hwaddr(struct eth_device *dev, unsigned char *mac)
{
#ifdef CONFIG_GET_FEC_MAC_ADDR_FROM_IIM
	fec_get_mac_addr(mac);

	return 0;
#else
	return -1;
#endif
}

static int fec_set_hwaddr(struct eth_device *dev)
{
	uchar *mac = dev->enetaddr;
	struct fec_info_s *info = dev->priv;
	volatile fec_t *fecp = (fec_t *)(info->iobase);

	writel(0, &fecp->iaur);
	writel(0, &fecp->ialr);
	writel(0, &fecp->gaur);
	writel(0, &fecp->galr);

	/*
	 * Set physical address
	 */
	writel((mac[0] << 24) + (mac[1] << 16) + (mac[2] << 8) + mac[3],
			&fecp->palr);
	writel((mac[4] << 24) + (mac[5] << 16) + 0x8808, &fecp->paur);

	return 0;
}

int fec_send(struct eth_device *dev, volatile void *packet, int length)
{
	struct fec_info_s *info = dev->priv;
	volatile fec_t *fecp = (fec_t *) (info->iobase);
	int j, rc;
	u16 phyStatus = 0;

	__fec_mii_read(fecp, info->phy_addr, PHY_BMSR, &phyStatus);

	if (!(phyStatus & PHY_BMSR_LS)) {
		printf("FEC: Link is down %x\n", phyStatus);
		return -1;
	}

	/* section 16.9.23.3
	 * Wait for ready
	 */
	j = 0;
	while ((info->txbd[info->txIdx].cbd_sc & BD_ENET_TX_READY) &&
	       (j < FEC_MAX_TIMEOUT)) {
		udelay(FEC_TIMEOUT_TICKET);
		j++;
	}
	if (j >= FEC_MAX_TIMEOUT)
		printf("TX not ready\n");

#ifdef CONFIG_MX28
	swap_packet((void *)packet, length);
#endif

#ifdef CONFIG_ARCH_MMU
	memcpy(ioremap_nocache((ulong)info->txbd[info->txIdx].cbd_bufaddr,
			length),
			(void *)packet, length);
#else
	info->txbd[info->txIdx].cbd_bufaddr = (uint)packet;
#endif
	info->txbd[info->txIdx].cbd_datlen = length;
	info->txbd[info->txIdx].cbd_sc =
	    (info->txbd[info->txIdx].cbd_sc & BD_ENET_TX_WRAP) |
	    BD_ENET_TX_TC | BD_ENET_TX_RDY_LST;

	/* Activate transmit Buffer Descriptor polling */
	fecp->tdar = 0x01000000;	/* Descriptor polling active    */

	/* FEC fix for MCF5275, FEC unable to initial transmit data packet.
	 * A nop will ensure the descriptor polling active completed.
	 */
	__asm__("nop");

#ifdef CONFIG_SYS_UNIFY_CACHE
	icache_invalid();
#endif
	j = 0;

	while ((info->txbd[info->txIdx].cbd_sc & BD_ENET_TX_READY) &&
	       (j < FEC_MAX_TIMEOUT)) {
		udelay(FEC_TIMEOUT_TICKET);
		j++;
	}
	if (j >= FEC_MAX_TIMEOUT)
		printf("TX timeout packet at %p\n", packet);

	fecp->eir &= fecp->eir;

#ifdef ET_DEBUG
	printf("%s[%d] %s: cycles: %d    status: %x  retry cnt: %d\n",
	       __FILE__, __LINE__, __func__, j,
	       info->txbd[info->txIdx].cbd_sc,
	       (info->txbd[info->txIdx].cbd_sc & 0x003C) >> 2);
#endif
	/* return only status bits */
	rc = (info->txbd[info->txIdx].cbd_sc & BD_ENET_TX_STATS);
	info->txIdx = (info->txIdx + 1) % TX_BUF_CNT;

	return rc;
}

int fec_recv(struct eth_device *dev)
{
	struct fec_info_s *info = dev->priv;
	volatile fec_t *fecp = (fec_t *) (info->iobase);
	int length;

	for (;;) {
#ifdef CONFIG_SYS_UNIFY_CACHE
		icache_invalid();
#endif
		/* section 16.9.23.2 */
		if (info->rxbd[info->rxIdx].cbd_sc & BD_ENET_RX_EMPTY) {
			length = -1;
			break;	/* nothing received - leave for() loop */
		}

		length = info->rxbd[info->rxIdx].cbd_datlen;

		if (info->rxbd[info->rxIdx].cbd_sc & 0x003f) {
#ifdef ET_DEBUG
			printf("%s[%d] err: %x\n",
			       __func__, __LINE__,
			       info->rxbd[info->rxIdx].cbd_sc);
#endif
			fecp->eir &= fecp->eir;
		} else {
			length -= 4;
#ifdef CONFIG_MX28
			swap_packet((void *)NetRxPackets[info->rxIdx], length);
#endif
			/* Pass the packet up to the protocol layers. */
#ifdef CONFIG_ARCH_MMU
			memcpy((void *)NetRxPackets[info->rxIdx],
				ioremap_nocache((ulong)info->rxbd[info->rxIdx].cbd_bufaddr, 0),
				length);
#endif
			NetReceive(NetRxPackets[info->rxIdx], length);

			fecp->eir |= FEC_EIR_RXF;
		}

		/* Give the buffer back to the FEC. */
		info->rxbd[info->rxIdx].cbd_datlen = 0;

		/* wrap around buffer index when necessary */
		if (info->rxIdx == LAST_PKTBUFSRX) {
			info->rxbd[PKTBUFSRX - 1].cbd_sc = BD_ENET_RX_W_E;
			info->rxIdx = 0;
		} else {
			info->rxbd[info->rxIdx].cbd_sc = BD_ENET_RX_EMPTY;
			info->rxIdx++;
		}

		/* Try to fill Buffer Descriptors */
		fecp->rdar = 0x01000000; /* Descriptor polling active    */
	}

	return length;
}

#ifdef ET_DEBUG
void dbgFecRegs(struct eth_device *dev)
{
	struct fec_info_s *info = dev->priv;
	volatile fec_t *fecp = (fec_t *) (info->iobase);

	printf("=====\n");
	printf("ievent       %x - %x\n", (int)&fecp->eir, fecp->eir);
	printf("imask        %x - %x\n", (int)&fecp->eimr, fecp->eimr);
	printf("r_des_active %x - %x\n", (int)&fecp->rdar, fecp->rdar);
	printf("x_des_active %x - %x\n", (int)&fecp->tdar, fecp->tdar);
	printf("ecntrl       %x - %x\n", (int)&fecp->ecr, fecp->ecr);
	printf("mii_mframe   %x - %x\n", (int)&fecp->mmfr, fecp->mmfr);
	printf("mii_speed    %x - %x\n", (int)&fecp->mscr, fecp->mscr);
	printf("mii_ctrlstat %x - %x\n", (int)&fecp->mibc, fecp->mibc);
	printf("r_cntrl      %x - %x\n", (int)&fecp->rcr, fecp->rcr);
	printf("x_cntrl      %x - %x\n", (int)&fecp->tcr, fecp->tcr);
	printf("padr_l       %x - %x\n", (int)&fecp->palr, fecp->palr);
	printf("padr_u       %x - %x\n", (int)&fecp->paur, fecp->paur);
	printf("op_pause     %x - %x\n", (int)&fecp->opd, fecp->opd);
	printf("iadr_u       %x - %x\n", (int)&fecp->iaur, fecp->iaur);
	printf("iadr_l       %x - %x\n", (int)&fecp->ialr, fecp->ialr);
	printf("gadr_u       %x - %x\n", (int)&fecp->gaur, fecp->gaur);
	printf("gadr_l       %x - %x\n", (int)&fecp->galr, fecp->galr);
	printf("x_wmrk       %x - %x\n", (int)&fecp->tfwr, fecp->tfwr);
	printf("r_bound      %x - %x\n", (int)&fecp->frbr, fecp->frbr);
	printf("r_fstart     %x - %x\n", (int)&fecp->frsr, fecp->frsr);
	printf("r_drng       %x - %x\n", (int)&fecp->erdsr, fecp->erdsr);
	printf("x_drng       %x - %x\n", (int)&fecp->etdsr, fecp->etdsr);
	printf("r_bufsz      %x - %x\n", (int)&fecp->emrbr, fecp->emrbr);

	printf("\n\n\n");
}
#endif

#ifndef ET_DEBUG
void dbgFecRegs(struct eth_device *dev)
{
	struct fec_info_s *info = dev->priv;
	volatile fec_t *fecp = (fec_t *) (info->iobase);

	printf("=====\n");
	printf("ievent       %x - %x\n", (int)&fecp->eir, fecp->eir);
	printf("imask        %x - %x\n", (int)&fecp->eimr, fecp->eimr);
	printf("r_des_active %x - %x\n", (int)&fecp->rdar, fecp->rdar);
	printf("x_des_active %x - %x\n", (int)&fecp->tdar, fecp->tdar);
	printf("ecntrl       %x - %x\n", (int)&fecp->ecr, fecp->ecr);
	printf("mii_mframe   %x - %x\n", (int)&fecp->mmfr, fecp->mmfr);
	printf("mii_speed    %x - %x\n", (int)&fecp->mscr, fecp->mscr);
	printf("mii_ctrlstat %x - %x\n", (int)&fecp->mibc, fecp->mibc);
	printf("r_cntrl      %x - %x\n", (int)&fecp->rcr, fecp->rcr);
	printf("x_cntrl      %x - %x\n", (int)&fecp->tcr, fecp->tcr);
	printf("padr_l       %x - %x\n", (int)&fecp->palr, fecp->palr);
	printf("padr_u       %x - %x\n", (int)&fecp->paur, fecp->paur);
	printf("op_pause     %x - %x\n", (int)&fecp->opd, fecp->opd);
	printf("iadr_u       %x - %x\n", (int)&fecp->iaur, fecp->iaur);
	printf("iadr_l       %x - %x\n", (int)&fecp->ialr, fecp->ialr);
	printf("gadr_u       %x - %x\n", (int)&fecp->gaur, fecp->gaur);
	printf("gadr_l       %x - %x\n", (int)&fecp->galr, fecp->galr);
	printf("x_wmrk       %x - %x\n", (int)&fecp->tfwr, fecp->tfwr);
	printf("r_bound      %x - %x\n", (int)&fecp->frbr, fecp->frbr);
	printf("r_fstart     %x - %x\n", (int)&fecp->frsr, fecp->frsr);
	printf("r_drng       %x - %x\n", (int)&fecp->erdsr, fecp->erdsr);
	printf("x_drng       %x - %x\n", (int)&fecp->etdsr, fecp->etdsr);
	printf("r_bufsz      %x - %x\n", (int)&fecp->emrbr, fecp->emrbr);
#if 0
	printf("\n");
	printf("rmon_t_drop        %x - %x\n", (int)&fecp->rmon_t_drop,
	       fecp->rmon_t_drop);
	printf("rmon_t_packets     %x - %x\n", (int)&fecp->rmon_t_packets,
	       fecp->rmon_t_packets);
	printf("rmon_t_bc_pkt      %x - %x\n", (int)&fecp->rmon_t_bc_pkt,
	       fecp->rmon_t_bc_pkt);
	printf("rmon_t_mc_pkt      %x - %x\n", (int)&fecp->rmon_t_mc_pkt,
	       fecp->rmon_t_mc_pkt);
	printf("rmon_t_crc_align   %x - %x\n", (int)&fecp->rmon_t_crc_align,
	       fecp->rmon_t_crc_align);
	printf("rmon_t_undersize   %x - %x\n", (int)&fecp->rmon_t_undersize,
	       fecp->rmon_t_undersize);
	printf("rmon_t_oversize    %x - %x\n", (int)&fecp->rmon_t_oversize,
	       fecp->rmon_t_oversize);
	printf("rmon_t_frag        %x - %x\n", (int)&fecp->rmon_t_frag,
	       fecp->rmon_t_frag);
	printf("rmon_t_jab         %x - %x\n", (int)&fecp->rmon_t_jab,
	       fecp->rmon_t_jab);
	printf("rmon_t_col         %x - %x\n", (int)&fecp->rmon_t_col,
	       fecp->rmon_t_col);
	printf("rmon_t_p64         %x - %x\n", (int)&fecp->rmon_t_p64,
	       fecp->rmon_t_p64);
	printf("rmon_t_p65to127    %x - %x\n", (int)&fecp->rmon_t_p65to127,
	       fecp->rmon_t_p65to127);
	printf("rmon_t_p128to255   %x - %x\n", (int)&fecp->rmon_t_p128to255,
	       fecp->rmon_t_p128to255);
	printf("rmon_t_p256to511   %x - %x\n", (int)&fecp->rmon_t_p256to511,
	       fecp->rmon_t_p256to511);
	printf("rmon_t_p512to1023  %x - %x\n", (int)&fecp->rmon_t_p512to1023,
	       fecp->rmon_t_p512to1023);
	printf("rmon_t_p1024to2047 %x - %x\n", (int)&fecp->rmon_t_p1024to2047,
	       fecp->rmon_t_p1024to2047);
	printf("rmon_t_p_gte2048   %x - %x\n", (int)&fecp->rmon_t_p_gte2048,
	       fecp->rmon_t_p_gte2048);
	printf("rmon_t_octets      %x - %x\n", (int)&fecp->rmon_t_octets,
	       fecp->rmon_t_octets);

	printf("\n");
	printf("ieee_t_drop      %x - %x\n", (int)&fecp->ieee_t_drop,
	       fecp->ieee_t_drop);
	printf("ieee_t_frame_ok  %x - %x\n", (int)&fecp->ieee_t_frame_ok,
	       fecp->ieee_t_frame_ok);
	printf("ieee_t_1col      %x - %x\n", (int)&fecp->ieee_t_1col,
	       fecp->ieee_t_1col);
	printf("ieee_t_mcol      %x - %x\n", (int)&fecp->ieee_t_mcol,
	       fecp->ieee_t_mcol);
	printf("ieee_t_def       %x - %x\n", (int)&fecp->ieee_t_def,
	       fecp->ieee_t_def);
	printf("ieee_t_lcol      %x - %x\n", (int)&fecp->ieee_t_lcol,
	       fecp->ieee_t_lcol);
	printf("ieee_t_excol     %x - %x\n", (int)&fecp->ieee_t_excol,
	       fecp->ieee_t_excol);
	printf("ieee_t_macerr    %x - %x\n", (int)&fecp->ieee_t_macerr,
	       fecp->ieee_t_macerr);
	printf("ieee_t_cserr     %x - %x\n", (int)&fecp->ieee_t_cserr,
	       fecp->ieee_t_cserr);
	printf("ieee_t_sqe       %x - %x\n", (int)&fecp->ieee_t_sqe,
	       fecp->ieee_t_sqe);
	printf("ieee_t_fdxfc     %x - %x\n", (int)&fecp->ieee_t_fdxfc,
	       fecp->ieee_t_fdxfc);
	printf("ieee_t_octets_ok %x - %x\n", (int)&fecp->ieee_t_octets_ok,
	       fecp->ieee_t_octets_ok);
	printf("\n");
	printf("rmon_r_drop        %x - %x\n", (int)&fecp->rmon_r_drop,
	       fecp->rmon_r_drop);
	printf("rmon_r_packets     %x - %x\n", (int)&fecp->rmon_r_packets,
	       fecp->rmon_r_packets);
	printf("rmon_r_bc_pkt      %x - %x\n", (int)&fecp->rmon_r_bc_pkt,
	       fecp->rmon_r_bc_pkt);
	printf("rmon_r_mc_pkt      %x - %x\n", (int)&fecp->rmon_r_mc_pkt,
	       fecp->rmon_r_mc_pkt);
	printf("rmon_r_crc_align   %x - %x\n", (int)&fecp->rmon_r_crc_align,
	       fecp->rmon_r_crc_align);
	printf("rmon_r_undersize   %x - %x\n", (int)&fecp->rmon_r_undersize,
	       fecp->rmon_r_undersize);
	printf("rmon_r_oversize    %x - %x\n", (int)&fecp->rmon_r_oversize,
	       fecp->rmon_r_oversize);
	printf("rmon_r_frag        %x - %x\n", (int)&fecp->rmon_r_frag,
	       fecp->rmon_r_frag);
	printf("rmon_r_jab         %x - %x\n", (int)&fecp->rmon_r_jab,
	       fecp->rmon_r_jab);
	printf("rmon_r_p64         %x - %x\n", (int)&fecp->rmon_r_p64,
	       fecp->rmon_r_p64);
	printf("rmon_r_p65to127    %x - %x\n", (int)&fecp->rmon_r_p65to127,
	       fecp->rmon_r_p65to127);
	printf("rmon_r_p128to255   %x - %x\n", (int)&fecp->rmon_r_p128to255,
	       fecp->rmon_r_p128to255);
	printf("rmon_r_p256to511   %x - %x\n", (int)&fecp->rmon_r_p256to511,
	       fecp->rmon_r_p256to511);
	printf("rmon_r_p512to1023  %x - %x\n", (int)&fecp->rmon_r_p512to1023,
	       fecp->rmon_r_p512to1023);
	printf("rmon_r_p1024to2047 %x - %x\n", (int)&fecp->rmon_r_p1024to2047,
	       fecp->rmon_r_p1024to2047);
	printf("rmon_r_p_gte2048   %x - %x\n", (int)&fecp->rmon_r_p_gte2048,
	       fecp->rmon_r_p_gte2048);
	printf("rmon_r_octets      %x - %x\n", (int)&fecp->rmon_r_octets,
	       fecp->rmon_r_octets);

	printf("\n");
	printf("ieee_r_drop      %x - %x\n", (int)&fecp->ieee_r_drop,
	       fecp->ieee_r_drop);
	printf("ieee_r_frame_ok  %x - %x\n", (int)&fecp->ieee_r_frame_ok,
	       fecp->ieee_r_frame_ok);
	printf("ieee_r_crc       %x - %x\n", (int)&fecp->ieee_r_crc,
	       fecp->ieee_r_crc);
	printf("ieee_r_align     %x - %x\n", (int)&fecp->ieee_r_align,
	       fecp->ieee_r_align);
	printf("ieee_r_macerr    %x - %x\n", (int)&fecp->ieee_r_macerr,
	       fecp->ieee_r_macerr);
	printf("ieee_r_fdxfc     %x - %x\n", (int)&fecp->ieee_r_fdxfc,
	       fecp->ieee_r_fdxfc);
	printf("ieee_r_octets_ok %x - %x\n", (int)&fecp->ieee_r_octets_ok,
	       fecp->ieee_r_octets_ok);
#endif
	printf("\n\n\n");
}
#endif

static void fec_mii_phy_init(struct eth_device *dev)
{
	struct fec_info_s *info = dev->priv;
	volatile fec_t *fecp = (fec_t *) (info->iobase);

	fec_reset(dev);
	fec_localhw_setup(fecp);
#if defined (CONFIG_CMD_MII) || defined (CONFIG_MII) || \
	defined (CONFIG_DISCOVER_PHY)
	mxc_fec_mii_init(fecp);
	mxc_fec_phy_powerup(dev->name, info->phy_addr);
#endif

}

int fec_init(struct eth_device *dev, bd_t *bd)
{
	struct fec_info_s *info = dev->priv;
	volatile fec_t *fecp = (fec_t *) (info->iobase);
	int i;
	u8 *ea = NULL;

	fec_mii_phy_init(dev);

#if defined(CONFIG_CMD_MII) || defined(CONFIG_MII) || \
	defined(CONFIG_DISCOVER_PHY)
#ifdef CONFIG_DISCOVER_PHY
	if (info->phy_addr < 0 || info->phy_addr > 0x1F)
		info->phy_addr = mxc_fec_mii_discover_phy(dev);
#endif
#if defined(CONFIG_MX6Q) || defined(CONFIG_MX6DL)
	mx6_rgmii_rework(dev->name, info->phy_addr);
#endif
	mxc_get_phy_ouid(dev->name, info->phy_addr);
	setFecDuplexSpeed(fecp, (uchar)info->phy_addr, info->dup_spd);
#else
#ifndef CONFIG_DISCOVER_PHY
	setFecDuplexSpeed(fecp, (uchar)info->phy_addr,
				(FECDUPLEX << 16) | FECSPEED);
#endif                         /* ifndef CONFIG_SYS_DISCOVER_PHY */
#endif                         /* CONFIG_CMD_MII || CONFIG_MII */

	/* We use strictly polling mode only */
	fecp->eimr = 0;

	/* Clear any pending interrupt */
	fecp->eir = 0xffffffff;

	/* Set station address   */
	ea = dev->enetaddr;
	fecp->palr = (ea[0] << 24) | (ea[1] << 16) | (ea[2] << 8) | (ea[3]);
	fecp->paur = (ea[4] << 24) | (ea[5] << 16);

	/* Clear unicast address hash table */
	fecp->iaur = 0;
	fecp->ialr = 0;

	/* Clear multicast address hash table */
	fecp->gaur = 0;
	fecp->galr = 0;

	/* Set maximum receive buffer size. */
	fecp->emrbr = PKT_MAXBLR_SIZE;

#if defined(CONFIG_MX6Q) || defined(CONFIG_MX6DL)
	fecp->rcr &= ~(0x100);
	fecp->rcr |= 0x44;
#endif
	/*
	 * Setup Buffers and Buffer Desriptors
	 */
	info->rxIdx = 0;
	info->txIdx = 0;

	/*
	 * Setup Receiver Buffer Descriptors (13.14.24.18)
	 * Settings:
	 *     Empty, Wrap
	 */
	for (i = 0; i < PKTBUFSRX; i++) {
		info->rxbd[i].cbd_sc = BD_ENET_RX_EMPTY;
		info->rxbd[i].cbd_datlen = 0;	/* Reset */
#ifdef CONFIG_ARCH_MMU
		info->rxbd[i].cbd_bufaddr =
			(uint)iomem_to_phys((ulong)info->rxbuf[i]);
#else
		info->rxbd[i].cbd_bufaddr = (uint)NetRxPackets[i];
#endif
	}
	info->rxbd[PKTBUFSRX - 1].cbd_sc |= BD_ENET_RX_WRAP;

	/*
	 * Setup Ethernet Transmitter Buffer Descriptors (13.14.24.19)
	 * Settings:
	 *    Last, Tx CRC
	 */
	for (i = 0; i < TX_BUF_CNT; i++) {
		info->txbd[i].cbd_sc = BD_ENET_TX_LAST | BD_ENET_TX_TC;
		info->txbd[i].cbd_datlen = 0;	/* Reset */
#ifdef CONFIG_ARCH_MMU
		info->txbd[i].cbd_bufaddr =
			(uint)iomem_to_phys((ulong)&info->txbuf[0]);
#else
		info->txbd[i].cbd_bufaddr = (uint)&info->txbuf[0];
#endif
	}
	info->txbd[TX_BUF_CNT - 1].cbd_sc |= BD_ENET_TX_WRAP;

	/* Set receive and transmit descriptor base */
#ifdef CONFIG_ARCH_MMU
	fecp->erdsr = (uint)iomem_to_phys((ulong)&info->rxbd[0]);
	fecp->etdsr = (uint)iomem_to_phys((ulong)&info->txbd[0]);
#else
	fecp->erdsr = (uint)(&info->rxbd[0]);
	fecp->etdsr = (uint)(&info->txbd[0]);
#endif

#if defined(CONFIG_MX6Q) || defined(CONFIG_MX6DL)
	/* Enable Swap to support little-endian device */
	fecp->ecr |= FEC_ECR_DBSWP;

	/* set ENET tx at store and forward mode */
	fecp->tfwr |= (0x1 << 8);
#endif

	/* Now enable the transmit and receive processing */
	fecp->ecr |= FEC_ECR_ETHER_EN;

	/* And last, try to fill Rx Buffer Descriptors */
	fecp->rdar = 0x01000000;	/* Descriptor polling active    */

	return 1;
}

void fec_reset(struct eth_device *dev)
{
	struct fec_info_s *info = dev->priv;
	volatile fec_t *fecp = (fec_t *)(info->iobase);
	int i;

	fecp->ecr = FEC_ECR_RESET;
	for (i = 0; (fecp->ecr & FEC_ECR_RESET) && (i < FEC_RESET_DELAY); ++i)
		udelay(1);

	if (i == FEC_RESET_DELAY)
		printf("FEC_RESET_DELAY timeout\n");

}

void fec_halt(struct eth_device *dev)
{
	struct fec_info_s *info = dev->priv;

	fec_reset(dev);

	info->rxIdx = info->txIdx = 0;
	memset(info->rxbd, 0, PKTBUFSRX * sizeof(cbd_t));
	memset(info->txbd, 0, TX_BUF_CNT * sizeof(cbd_t));
	memset(info->txbuf, 0, DBUF_LENGTH);
}

int mxc_fec_initialize(bd_t *bis)
{
	struct eth_device *dev;
	int i;
	unsigned char ethaddr[6];

	for (i = 0; i < sizeof(fec_info) / sizeof(fec_info[0]); i++) {
#ifdef CONFIG_ARCH_MMU
		int j;
#endif
		dev =
		    (struct eth_device *)memalign(CONFIG_SYS_CACHELINE_SIZE,
						  sizeof *dev);
		if (dev == NULL)
			hang();

		memset(dev, 0, sizeof(*dev));

		sprintf(dev->name, "FEC%d", fec_info[i].index);
		dev->priv = &fec_info[i];
		dev->init = fec_init;
		dev->halt = fec_halt;
		dev->send = fec_send;
		dev->recv = fec_recv;
		dev->write_hwaddr = fec_set_hwaddr;

		/* setup Receive and Transmit buffer descriptor */
#ifdef CONFIG_ARCH_MMU
		fec_info[i].rxbd =
			(cbd_t *)ioremap_nocache(iomem_to_phys((ulong)memalign(CONFIG_SYS_CACHELINE_SIZE,
						(PKTBUFSRX * sizeof(cbd_t)))),
						CONFIG_SYS_CACHELINE_SIZE);
		fec_info[i].txbd =
			(cbd_t *)ioremap_nocache(iomem_to_phys((ulong)memalign(CONFIG_SYS_CACHELINE_SIZE,
						(TX_BUF_CNT * sizeof(cbd_t)))),
						CONFIG_SYS_CACHELINE_SIZE);
		fec_info[i].txbuf =
			(char *)ioremap_nocache(iomem_to_phys((ulong)memalign(CONFIG_SYS_CACHELINE_SIZE,
						DBUF_LENGTH)),
						CONFIG_SYS_CACHELINE_SIZE);
		for (j = 0; j < PKTBUFSRX; ++j) {
			fec_info[i].rxbuf[j] =
				(char *)ioremap_nocache(iomem_to_phys((ulong)memalign(PKTSIZE_ALIGN,
						PKTSIZE)),
						PKTSIZE_ALIGN);
		}
#else
		fec_info[i].rxbd =
			(cbd_t *)memalign(CONFIG_SYS_CACHELINE_SIZE,
						(PKTBUFSRX * sizeof(cbd_t)));
		fec_info[i].txbd =
		    (cbd_t *)memalign(CONFIG_SYS_CACHELINE_SIZE,
				       (TX_BUF_CNT * sizeof(cbd_t)));
		fec_info[i].txbuf =
		    (char *)memalign(CONFIG_SYS_CACHELINE_SIZE, DBUF_LENGTH);
#endif

#ifdef ET_DEBUG
		printf("%s: rxbd %x txbd %x ->%x\n", dev->name,
		       (int)fec_info[i].rxbd, (int)fec_info[i].txbd,
		       (int)fec_info[i].txbuf);
#endif

		fec_info[i].phy_name =
			(char *)memalign(CONFIG_SYS_CACHELINE_SIZE, 32);

		eth_register(dev);

#if defined(CONFIG_MII) || defined(CONFIG_CMD_MII)
		miiphy_register(dev->name, mxc_fec_mii_read,
						mxc_fec_mii_write);
#endif
		if (fec_get_hwaddr(dev, ethaddr) == 0) {
			printf("got MAC address from IIM: %pM\n", ethaddr);
			memcpy(dev->enetaddr, ethaddr, 6);
			fec_set_hwaddr(dev);
		}
	}

	return 1;
}

#endif				/* CONFIG_CMD_NET, FEC_ENET & NET_MULTI */
