/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2016 Cadence Design Systems - http://www.cadence.com
 * Copyright 2019 NXP
 */

#ifndef __REG_USBSS_DEV_ADDR_MAP_H__
#define __REG_USBSS_DEV_ADDR_MAP_H__

#include "dev-regs-macro.h"

struct usbss_dev_register_block_type {
	u32 usb_conf;                     /*        0x0 - 0x4        */
	u32 usb_sts;                      /*        0x4 - 0x8        */
	u32 usb_cmd;                      /*        0x8 - 0xc        */
	u32 usb_iptn;                     /*        0xc - 0x10       */
	u32 usb_lpm;                      /*       0x10 - 0x14       */
	u32 usb_ien;                      /*       0x14 - 0x18       */
	u32 usb_ists;                     /*       0x18 - 0x1c       */
	u32 ep_sel;                       /*       0x1c - 0x20       */
	u32 ep_traddr;                    /*       0x20 - 0x24       */
	u32 ep_cfg;                       /*       0x24 - 0x28       */
	u32 ep_cmd;                       /*       0x28 - 0x2c       */
	u32 ep_sts;                       /*       0x2c - 0x30       */
	u32 ep_sts_sid;                   /*       0x30 - 0x34       */
	u32 ep_sts_en;                    /*       0x34 - 0x38       */
	u32 drbl;                         /*       0x38 - 0x3c       */
	u32 ep_ien;                       /*       0x3c - 0x40       */
	u32 ep_ists;                      /*       0x40 - 0x44       */
	u32 usb_pwr;                      /*       0x44 - 0x48       */
	u32 usb_conf2;                    /*       0x48 - 0x4c       */
	u32 usb_cap1;                     /*       0x4c - 0x50       */
	u32 usb_cap2;                     /*       0x50 - 0x54       */
	u32 usb_cap3;                     /*       0x54 - 0x58       */
	u32 usb_cap4;                     /*       0x58 - 0x5c       */
	u32 usb_cap5;                     /*       0x5c - 0x60       */
	u32 PAD2_73;                     /*       0x60 - 0x64       */
	u32 usb_cpkt1;                    /*       0x64 - 0x68       */
	u32 usb_cpkt2;                    /*       0x68 - 0x6c       */
	u32 usb_cpkt3;                    /*       0x6c - 0x70       */
	char pad__0[0x90];                     /*       0x70 - 0x100      */
	u32 PAD2_78;                     /*      0x100 - 0x104      */
	u32 dbg_link1;                    /*      0x104 - 0x108      */
	u32 PAD2_80;                    /*      0x108 - 0x10c      */
	u32 PAD2_81;                     /*      0x10c - 0x110      */
	u32 PAD2_82;                     /*      0x110 - 0x114      */
	u32 PAD2_83;                     /*      0x114 - 0x118      */
	u32 PAD2_84;                     /*      0x118 - 0x11c      */
	u32 PAD2_85;                     /*      0x11c - 0x120      */
	u32 PAD2_86;                     /*      0x120 - 0x124      */
	u32 PAD2_87;                    /*      0x124 - 0x128      */
	u32 PAD2_88;                    /*      0x128 - 0x12c      */
	u32 PAD2_89;                    /*      0x12c - 0x130      */
	u32 PAD2_90;                    /*      0x130 - 0x134      */
	u32 PAD2_91;                    /*      0x134 - 0x138      */
	u32 PAD2_92;                    /*      0x138 - 0x13c      */
	u32 PAD2_93;                    /*      0x13c - 0x140      */
	u32 PAD2_94;                    /*      0x140 - 0x144      */
	u32 PAD2_95;                    /*      0x144 - 0x148      */
	u32 PAD2_96;                    /*      0x148 - 0x14c      */
	u32 PAD2_97;                    /*      0x14c - 0x150      */
	u32 PAD2_98;                    /*      0x150 - 0x154      */
	u32 PAD2_99;                    /*      0x154 - 0x158      */
	u32 PAD2_100;                    /*      0x158 - 0x15c      */
	u32 PAD2_101;                    /*      0x15c - 0x160      */
	u32 PAD2_102;                    /*      0x160 - 0x164      */
	u32 PAD2_103;                    /*      0x164 - 0x168      */
	u32 PAD2_104;                    /*      0x168 - 0x16c      */
	u32 PAD2_105;                    /*      0x16c - 0x170      */
	u32 PAD2_106;                    /*      0x170 - 0x174      */
	u32 PAD2_107;                    /*      0x174 - 0x178      */
	u32 PAD2_108;                    /*      0x178 - 0x17c      */
	u32 PAD2_109;                    /*      0x17c - 0x180      */
	u32 PAD2_110;                    /*      0x180 - 0x184      */
	u32 PAD2_111;                    /*      0x184 - 0x188      */
	u32 PAD2_112;                    /*      0x188 - 0x18c      */
	char pad__1[0x20];                     /*      0x18c - 0x1ac      */
	u32 PAD2_114;                    /*      0x1ac - 0x1b0      */
	u32 PAD2_115;                    /*      0x1b0 - 0x1b4      */
	u32 PAD2_116;                    /*      0x1b4 - 0x1b8      */
	u32 PAD2_117;                    /*      0x1b8 - 0x1bc      */
	u32 PAD2_118;                    /*      0x1bc - 0x1c0      */
	u32 PAD2_119;                    /*      0x1c0 - 0x1c4      */
	u32 PAD2_120;                    /*      0x1c4 - 0x1c8      */
	u32 PAD2_121;                    /*      0x1c8 - 0x1cc      */
	u32 PAD2_122;                    /*      0x1cc - 0x1d0      */
	u32 PAD2_123;                    /*      0x1d0 - 0x1d4      */
	u32 PAD2_124;                    /*      0x1d4 - 0x1d8      */
	u32 PAD2_125;                    /*      0x1d8 - 0x1dc      */
	u32 PAD2_126;                    /*      0x1dc - 0x1e0      */
	u32 PAD2_127;                    /*      0x1e0 - 0x1e4      */
	u32 PAD2_128;                    /*      0x1e4 - 0x1e8      */
	u32 PAD2_129;                    /*      0x1e8 - 0x1ec      */
	u32 PAD2_130;                    /*      0x1ec - 0x1f0      */
	u32 PAD2_131;                    /*      0x1f0 - 0x1f4      */
	u32 PAD2_132;                    /*      0x1f4 - 0x1f8      */
	u32 PAD2_133;                    /*      0x1f8 - 0x1fc      */
	u32 PAD2_134;                    /*      0x1fc - 0x200      */
	u32 PAD2_135;                    /*      0x200 - 0x204      */
	u32 PAD2_136;                    /*      0x204 - 0x208      */
	u32 PAD2_137;                    /*      0x208 - 0x20c      */
	u32 PAD2_138;                    /*      0x20c - 0x210      */
	u32 PAD2_139;                    /*      0x210 - 0x214      */
	u32 PAD2_140;                    /*      0x214 - 0x218      */
	u32 PAD2_141;                    /*      0x218 - 0x21c      */
	u32 PAD2_142;                    /*      0x21c - 0x220      */
	u32 PAD2_143;                    /*      0x220 - 0x224      */
	u32 PAD2_144;                    /*      0x224 - 0x228      */
	char pad__2[0xd8];                     /*      0x228 - 0x300      */
	u32 dma_axi_ctrl;                 /*      0x300 - 0x304      */
	u32 PAD2_147;                   /*      0x304 - 0x308      */
	u32 PAD2_148;                  /*      0x308 - 0x30c      */
	u32 PAD2_149;                /*      0x30c - 0x310      */
	u32 PAD2_150;                /*      0x310 - 0x314      */
};

#endif /* __REG_USBSS_DEV_ADDR_MAP_H__ */
