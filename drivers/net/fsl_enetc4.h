// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 NXP
 */

#ifndef _FSL_ENETC4_H
#define _FSL_ENETC4_H

#include <linux/bitops.h>
#include <phy.h>
#define enetc_dbg(dev, fmt, args...)	debug("%s:" fmt, dev->name, ##args)

#define PCI_DEVICE_ID_ENETC4    0xE101
#define PCI_DEVICE_ID_EMDIO     0xEE00
#define PCI_DEVICE_ID_ENETC_ETH	0xE100
#define PCI_DEVICE_ID_ENETC_MDIO	0xEE01

struct enetc_tx_bd {
	__le64 addr;
	__le16 buf_len;
	__le16 frm_len;
	__le16 err_csum;
	__le16 flags;
};

union enetc_rx_bd {
	/* SW provided BD format */
	struct {
		__le64 addr;
		u8 reserved[8];
	} w;

	/* ENETC returned BD format */
	struct {
		__le16 inet_csum;
		__le16 parse_summary;
		__le32 rss_hash;
		__le16 buf_len;
		__le16 vlan_opt;
		union {
			struct {
				__le16 flags;
				__le16 error;
			};
			__le32 lstatus;
		};
	} r;
};

struct bd_ring {
	void *cons_idx;
	void *prod_idx;
	/* next BD index to use */
	int next_prod_idx;
	int next_cons_idx;
	int bd_count;
	int bd_num_in_cl; /* bd number in one cache line */
};

struct enetc_priv {
        struct enetc_tx_bd *enetc_txbd;
        union enetc_rx_bd *enetc_rxbd;

        void *regs_base;
        void *port_regs;

	struct bd_ring tx_bdr;
	struct bd_ring rx_bdr;

	int uclass_id;
	struct mii_dev imdio;
	struct phy_device *phy;
};

#define enetc_read_reg(x)       readl((x))
#define enetc_write_reg(x, val) writel((val), (x))
#define enetc_read(priv, off)   enetc_read_reg((priv)->regs_base + (off))
#define enetc_write(priv, off, v) \
                                enetc_write_reg((priv)->regs_base + (off), v)

#define enetc_port_regs(priv, off) ((priv)->port_regs + (off))
#define enetc_read_port(priv, off) \
			enetc_read_reg(enetc_port_regs((priv), (off)))
#define enetc_write_port(priv, off, v) \
			enetc_write_reg(enetc_port_regs((priv), (off)), v)
#define enetc_bdr_read(priv, t, n, off) \
		       enetc_read(priv, ENETC_BDR(t, n, off))
#define enetc_bdr_write(priv, t, n, off, val) \
			enetc_write(priv, ENETC_BDR(t, n, off), val)

#define ENETC_BD_CNT            CONFIG_SYS_RX_ETH_BUFFER
#define ENETC_BD_ALIGN          128
#define ENETC_POLL_TRIES	32000
#define ENETC_TXBD_FLAGS_F	BIT(15)
#define ENETC_RXBD_STATUS_R(status)	(((status) >> 30) & 0x1)
#define ENETC_RXBD_STATUS_F(status)	(((status) >> 31) & 0x1)
#define ENETC_RXBD_STATUS_ERRORS(status)	(((status) >> 16) & 0xff)
#define ENETC_RXBD_STATUS(flags)	((flags) << 16)

/* IERB registers */
#define IERB_BASE               0x4cde0000
#define IERB_E0BCAR(n)          (IERB_BASE + 0x3030 + (n) * 0x100)

#define ENETC_PORT_REGS_OFF     0x10000
/* ENETC base registers */
#define ENETC_PMR               0x10
#define ENETC_PMR_SI0_EN        BIT(16)
#define ENETC_PSICFGR(n)                (0x2010 + (n) * 0x80) /* 0x2010, 0x2090, 0x2110 */
#define ENETC_PSICFGR_SET_TXBDR(val)    ((val) & 0x7f)
#define ENETC_PSICFGR_SET_RXBDR(val)    (((val) & 0x7f) << 16)
#define ENETC_RX_BDR_CNT	1
#define ENETC_TX_BDR_CNT	1
#define ENETC_RX_BDR_ID		0
#define ENETC_TX_BDR_ID		0
/* Port registers */
#define ENETC_PCAPR0            0x4000
#define ENETC_PCAPRO_MDIO       0x0 /* is not defined in PCAPR of ENETCv4 */
#define ENETC_PIOCAPR           0x4008
#define ENETC_PCS_PROT          GENMASK(15, 0)
#define ENETC_POR		0x4100
/* Ethernet MAC port registers */
#define ENETC_PM_CC             0x5008
#define ENETC_PM_CC_RX_TX_EN    0x8003 /* TXP(default 1) | RX_EN | TX_EN */
#define ENETC_PM_MAXFRM         0x5014
#define ENETC_RX_MAXFRM_SIZE    PKTSIZE_ALIGN
#define ENETC_PM_IMDIO_BASE     0x5030
#define ENETC_PM_IF_MODE	0x5300
#define ENETC_PM_IF_MODE_AN_ENA      BIT(15) /* AN Enable */
#define ENETC_PM_IFM_SSP_MASK	GENMASK(14, 13)
#define ENETC_PM_IFM_SSP_1000	(2 << 13)
#define ENETC_PM_IFM_SSP_100	(0 << 13)
#define ENETC_PM_IFM_SSP_10	(1 << 13)
#define ENETC_PM_IFM_FULL_DPX	BIT(6)
#define ENETC_PM_IF_IFMODE_MASK	GENMASK(2, 0)
/* ENETC Station Interface registers */
#define ENETC_SIMR              0x000
#define ENETC_SIMR_EN           BIT(31)
#define ENETC_SICAR0            0x40
#define ENETC_SICAR_WR_CFG      0x6767
#define ENETC_SICAR_RD_CFG      0x2b2b0000
enum enetc_bdr_type {TX, RX};
#define ENETC_BDR(type, n, off) (0x8000 + (type) * 0x100 + (n) * 0x200 + (off))
#define ENETC_BDR_IDX_MASK	0xffff
#define ENETC_TBMR              0x0
#define ENETC_TBMR_EN           BIT(31)
#define ENETC_TBBAR0            0x10
#define ENETC_TBBAR1            0x14
#define ENETC_TBPIR		0x18
#define ENETC_TBCIR		0x1c
#define ENETC_TBLENR            0x20
#define ENETC_RBMR              0x0
#define ENETC_RBMR_EN           BIT(31)
#define ENETC_RBBSR             0x8
#define ENETC_RBCIR             0xc
#define ENETC_RBBAR0            0x10
#define ENETC_RBBAR1            0x14
#define ENETC_RBPIR             0x18
#define ENETC_RBLENR            0x20

/* NETC EMDIO base function registers */
#define ENETC_EMDIO_BASE	0x4cce0000
#define ENETC_MDIO_BASE		0x1c00 /* EMDIO_BASE: 0x4cce0000*/
#define ENETC_MDIO_CFG          0x0
#define ENETC_EMDIO_CFG_BSY     BIT(0)
#define ENETC_EMDIO_CFG_RD_ER	BIT(1)
#define ENETC_EMDIO_CFG_C22     0x00809508 /* BIT(6): ENC45 */
#define ENETC_EMDIO_CFG_C45     0x00809548
#define ENETC_MDIO_CTL		0x4
#define ENETC_MDIO_CTL_READ	BIT(15)
#define ENETC_MDIO_DATA         0x8
#define ENETC_MDIO_STAT         0xc /* EMDIO_ADDR */

#define ENETC_MDIO_READ_ERR	0xffff

struct enetc_mdio_priv {
	void *regs_base;
};

int enetc_mdio_read_priv(struct enetc_mdio_priv *priv, int addr, int devad,
			 int reg);
int enetc_mdio_write_priv(struct enetc_mdio_priv *priv, int addr, int devad,
			  int reg, u16 val);

void fdt_fixup_enetc_mac(void *blob);
void enetc4_netcmix_blk_ctrl_cfg(void);
int enetc4_ierb_cfg(void);
int enetc4_ierb_cfg_is_valid(void);

#define ENETC_PCS_PHY_ADDR      0
/* LS1028ARM SerDes Module */
#define ENETC_PCS_CR                    0x00
#define ENETC_PCS_CR_RESET_AN           0x1200
#define ENETC_PCS_CR_DEF_VAL            0x0140
#define ENETC_PCS_CR_RST		BIT(15)
#define ENETC_PCS_DEV_ABILITY           0x4
#define ENETC_PCS_DEV_ABILITY_SGMII     0x4001
#define ENETC_PCS_DEV_ABILITY_SXGMII    0x5001
#define ENETC_PCS_LINK_TIMER1           0x12
#define ENETC_PCS_LINK_TIMER1_VAL       0x06a0
#define ENETC_PCS_LINK_TIMER2           0x13
#define ENETC_PCS_LINK_TIMER2_VAL       0x0003
#define ENETC_PCS_IF_MODE               0x14
#define ENETC_PCS_IF_MODE_SGMII         BIT(0)
#define ENETC_PCS_IF_MODE_SGMII_AN      BIT(1)
#define ENETC_PCS_IF_MODE_SPEED_1G      BIT(3)
/* MDIO_SXGMII_USXGMII_REPL */
#define ENETC_PCS_DEVAD_REPL            0x1f

#endif
