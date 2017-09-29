/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * AVB switch driver module for SJA1105
 * Copyright 2017,2020 NXP
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
/*
 *
 * \file  sja1105_ll.h
 *
 * \author Philippe guasch
 *
 * \date 2016-02-22
 *
 * \brief SJA1105 Low level register definitions that could be part of the HAL
 *
 */

#ifndef _SJA1105_LL_H__
#define _SJA1105_LL_H__

#define WIDTH2MASK(_w_) ((1 << (_w_)) - 1)

/*
 * The configuration must be loaded into SJA1105 starting from 0x20000
 * The configuration must be split in 64 words block transfers
 */

#define SJA1105_CONFIG_START_ADDRESS 0x20000UL
#define SJA1105_CONFIG_WORDS_PER_BLOCK 1

#define CMD_RWOP_SHIFT 31
#define CMD_RD_OP 0
#define CMD_WR_OP 1
#define CMD_REG_ADDR_SHIFT 4
#define CMD_REG_ADDR_WIDTH 21
#define CMD_REG_RD_CNT_SHIFT 25
#define CMD_REG_RD_CNT_WIDTH 6

#define CMD_ENCODE_RWOP(_write_)   ((_write_) << CMD_RWOP_SHIFT)
#define CMD_ENCODE_ADDR(_addr_) \
	(((_addr_) & WIDTH2MASK(CMD_REG_ADDR_WIDTH)) << CMD_REG_ADDR_SHIFT)
#define CMD_ENCODE_WRD_CNT(_cnt_) \
	(((_cnt_) & WIDTH2MASK(CMD_REG_RD_CNT_WIDTH)) << CMD_REG_RD_CNT_SHIFT)

#define SJA1105_REG_RESET_CTRL 0x100440UL
#define SJA1105_BIT_RESET_CTRL_COLDRESET BIT(2)
#define SJA1105_BIT_RESET_CTRL_WARMRESET BIT(3)

#define SJA1105_REG_PLL_0_S    0x100007UL
#define SJA1105_REG_PLL_1_S    0x100009UL
#define SJA1105_REG_PLL_LOCKED_BIT 0

#define SJA1105_REG_PLL_1_C    0x10000aUL
#define SJA1105_REG_PLL_CLK_SRC_SHIFT 24
#define SJA1105_REG_PLL_CLK_SRC_WIDTH 5
#define SJA1105_REG_PLL_MSEL_SHIFT 16
#define SJA1105_REG_PLL_MSEL_WIDTH 8
#define SJA1105_REG_PLL_AUTOBLOCK_SHIFT 11
#define SJA1105_REG_PLL_AUTOBLOCK_WIDTH 1
#define SJA1105_REG_PLL_PSEL_SHIFT 8
#define SJA1105_REG_PLL_PSEL_WIDTH 2
#define SJA1105_REG_PLL_DIRECT_SHIFT 7
#define SJA1105_REG_PLL_DIRECT_WIDTH 1
#define SJA1105_REG_PLL_FBSEL_SHIFT 6
#define SJA1105_REG_PLL_FBSEL_WIDTH 1
#define SJA1105_REG_PLL_BYPASS_SHIFT 1
#define SJA1105_REG_PLL_BYPASS_WIDTH 1
#define SJA1105_REG_PLL_PD_SHIFT 0
#define SJA1105_REG_PLL_PD_WIDTH 1

#define ENCODE_REG_PLL_CLK_SRC(_val_)   \
	(((_val_) & WIDTH2MASK(SJA1105_REG_PLL_CLK_SRC_WIDTH)) << \
	 SJA1105_REG_PLL_CLK_SRC_SHIFT)
#define ENCODE_REG_PLL_MSEL(_val_)       \
	(((_val_) & WIDTH2MASK(SJA1105_REG_PLL_MSEL_WIDTH)) << \
	 SJA1105_REG_PLL_MSEL_SHIFT)
#define ENCODE_REG_PLL_AUTOBLOCK(_val_)  \
	(((_val_) & WIDTH2MASK(SJA1105_REG_PLL_AUTOBLOCK_WIDTH)) << \
	 SJA1105_REG_PLL_AUTOBLOCK_SHIFT)
#define ENCODE_REG_PLL_PSEL(_val_)       \
	(((_val_) & WIDTH2MASK(SJA1105_REG_PLL_PSEL_WIDTH)) << \
	 SJA1105_REG_PLL_PSEL_SHIFT)
#define ENCODE_REG_PLL_DIRECT(_val_)     \
	(((_val_) & WIDTH2MASK(SJA1105_REG_PLL_DIRECT_WIDTH)) << \
	 SJA1105_REG_PLL_DIRECT_SHIFT)
#define ENCODE_REG_PLL_FBSEL(_val_)      \
	(((_val_) & WIDTH2MASK(SJA1105_REG_PLL_FBSEL_WIDTH)) << \
	 SJA1105_REG_PLL_FBSEL_SHIFT)
#define ENCODE_REG_PLL_BYPASS(_val_)     \
	(((_val_) & WIDTH2MASK(SJA1105_REG_PLL_BYPASS_WIDTH)) << \
	 SJA1105_REG_PLL_BYPASS_SHIFT)
#define ENCODE_REG_PLL_PD(_val_)         \
	(((_val_) & WIDTH2MASK(SJA1105_REG_PLL_PD_WIDTH)) << \
	 SJA1105_REG_PLL_PD_SHIFT)

/* 0x10000b thru 0x10000f */
#define SJA1105_REG_IDIV_C(_port_)     (0x10000bUL + (_port_))

#define SJA1105_REG_IDIV_CLKSRC_SHIFT 24
#define SJA1105_REG_IDIV_CLKSRC_WIDTH 5
#define SJA1105_REG_IDIV_AUTOBLOCK_SHIFT 11
#define SJA1105_REG_IDIV_AUTOBLOCK_WIDTH 1
#define SJA1105_REG_IDIV_IDIV_SHIFT 2
#define SJA1105_REG_IDIV_IDIV_WIDTH 4
#define SJA1105_REG_IDIV_PD_SHIFT 0
#define SJA1105_REG_IDIV_PD_WIDTH 1
#define ENCODE_REG_IDIV_CLK_SRC(_val_)    \
	(((_val_) & WIDTH2MASK(SJA1105_REG_IDIV_CLKSRC_WIDTH)) << \
	 SJA1105_REG_IDIV_CLKSRC_SHIFT)
#define ENCODE_REG_IDIV_AUTOBLOCK(_val_)  \
	(((_val_) & WIDTH2MASK(SJA1105_REG_IDIV_AUTOBLOCK_WIDTH)) << \
	 SJA1105_REG_IDIV_AUTOBLOCK_SHIFT)
#define ENCODE_REG_IDIV_IDIV(_val_)       \
	(((_val_) & WIDTH2MASK(SJA1105_REG_IDIV_IDIV_WIDTH)) << \
	 SJA1105_REG_IDIV_IDIV_SHIFT)
#define ENCODE_REG_IDIV_PD(_val_)         \
	(((_val_) & WIDTH2MASK(SJA1105_REG_PLL_PD_WIDTH)) << \
	 SJA1105_REG_IDIV_PD_SHIFT)

#define SJA1105_REG_MII_TX_CLK(_port_)     (0x100013UL + ((_port_) * 7))
#define SJA1105_REG_MII_RX_CLK(_port_)     (0x100014UL + ((_port_) * 7))
#define SJA1105_REG_RMII_REF_CLK(_port_)   (0x100015UL + ((_port_) * 7))
#define SJA1105_REG_RGMII_TX_CLK(_port_)   (0x100016UL + ((_port_) * 7))
#define SJA1105_REG_EXT_TX_CLK(_port_)     (0x100018UL + ((_port_) * 7))
#define SJA1105_REG_EXT_RX_CLK(_port_)     (0x100019UL + ((_port_) * 7))

#define SJA1105_REG_MII_CTRL_CLKSRC_SHIFT 24
#define SJA1105_REG_MII_CTRL_CLKSRC_WIDTH 5
#define SJA1105_REG_MII_CTRL_AUTOBLOCK_SHIFT 11
#define SJA1105_REG_MII_CTRL_AUTOBLOCK_WIDTH 1
#define SJA1105_REG_MII_CTRL_PD_SHIFT 0
#define SJA1105_REG_MII_CTRL_PD_WIDTH 1
#define ENCODE_REG_MII_CTRL_CLKSRC(_val_)     \
	(((_val_) & WIDTH2MASK(SJA1105_REG_MII_CTRL_CLKSRC_WIDTH)) << \
	 SJA1105_REG_MII_CTRL_CLKSRC_SHIFT)
#define ENCODE_REG_MII_CTRL_AUTOBLOCK(_val_)  \
	(((_val_) & WIDTH2MASK(SJA1105_REG_MII_CTRL_AUTOBLOCK_WIDTH)) << \
	 SJA1105_REG_MII_CTRL_AUTOBLOCK_SHIFT)
#define ENCODE_REG_MII_CTRL_PD(_val_)         \
	(((_val_) & WIDTH2MASK(SJA1105_REG_MII_CTRL_PD_WIDTH)) << \
	 SJA1105_REG_MII_CTRL_PD_SHIFT)

/* SJA1105 register address at which DeviceId must be read */
#define SJA1105_REG_DEVICEID                    0x00000000UL

/* SJA1105 register address at which Status must be read */
#define SJA1105_REG_STATUS                      0x00000001UL
#define SJA1105_BIT_STATUS_CONFIG_DONE          BIT(31)
#define SJA1105_BIT_STATUS_CRCCHKL              BIT(30)
#define SJA1105_BIT_STATUS_DEVID_MATCH          BIT(29)
#define SJA1105_BIT_STATUS_CRCCHKG              BIT(28)
#define SJA1105_REG_STATUS_NSLOT_SHIFT		0
#define SJA1105_REG_STATUS_NSLOT_WIDTH		4

#define SJA1105_REG_GENERAL_STATUS1		0x00000003UL
#define SJA1105_REG_GENERAL_STATUS2		0x00000004UL
#define SJA1105_REG_GENERAL_STATUS3		0x00000005UL
#define SJA1105_REG_GENERAL_STATUS4		0x00000006UL
#define SJA1105_REG_GENERAL_STATUS5		0x00000007UL
#define SJA1105_REG_GENERAL_STATUS6		0x00000009UL
#define SJA1105_REG_GENERAL_STATUS7		0x0000000AUL
#define SJA1105_REG_GENERAL_STATUS8		0x0000000BUL
#define SJA1105_REG_GENERAL_STATUS9		0x0000000CUL

#define SJA1105_REG_PORT_STATUS(_port_)		(0x100900UL + (_port_))

#define SJA1105_REG_PORT_MAC_STATUS(_port_)     (0x200UL + ((_port_) * 2))
#define SJA1105_REG_PORT_HIGH_STATUS1(_port_)	(0x400UL + (_port_) * 16)
#define SJA1105_REG_PORT_HIGH_STATUS2(_port_)	(0x600UL + (_port_) * 16)

/* CGU Registers */

#define SJA1105_CGU_IDIV_ADDR			0x10000B
#define SJA1105_CGU_IDIV_PORT(port)		(SJA1105_CGU_IDIV_ADDR + (port))

#define SJA1105_CGU_MII_TX_CLK_ADDR		0x100016
#define SJA1105_CGU_MII_TX_CLK_PORT(port)	(SJA1105_CGU_MII_TX_CLK_ADDR + \
						 (port) * 6)

/* Configuration registers */

#define SJA1105_CFG_PAD_MIIX_TX_ADDR		0x100800
#define SJA1105_CFG_PAD_MIIX_TX_PORT(port)	(SJA1105_CFG_PAD_MIIX_TX_ADDR \
						 + (port) * 2)

#define SJA1105_CFG_PAD_MIIX_ID_ADDR		0x100810
#define SJA1105_CFG_PAD_MIIX_ID_PORT(port)	(SJA1105_CFG_PAD_MIIX_ID_ADDR \
						 + (port))

#define SJA1105_PORT_STATUS_MII_ADDR		0x100900
#define SJA1105_PORT_STATUS_MII_PORT(port)	(SJA1105_PORT_STATUS_MII_ADDR \
						 + (port))

#define SJA1105_CFG_PAD_MIIX_ID_RXC_DELAY(delay)	((delay) << 10)
#define SJA1105_CFG_PAD_MIIX_ID_RXC_BYPASS		BIT(9)
#define SJA1105_CFG_PAD_MIIX_ID_RXC_PD			BIT(8)

#define SJA1105_PORT_STATUS_MII_MODE		(0x00000003)

enum sja1105_mii_mode {
	e_mii_mode_mii = 0,
	e_mii_mode_rmii,
	e_mii_mode_rgmii,
	e_mii_mode_sgmii,
};

#define SJA1105_PORT_NB 5

int sja1105_get_cfg(u32 devid, u32 cs, u32 *bin_len, u8 **cfg_bin);

#endif
