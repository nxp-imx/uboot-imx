/*
 * Copyright 2004-2010 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file mxc_nd2.h
 *
 * @brief This file contains the NAND Flash Controller register information.
 *
 *
 * @ingroup NAND_MTD
 */

#ifndef __MXC_NAND_H__
#define __MXC_NAND_H__

#include <asm/arch/mx25-regs.h>

#define IS_2K_PAGE_NAND		((mtd->writesize / info->num_of_intlv) \
						== NAND_PAGESIZE_2KB)
#define IS_4K_PAGE_NAND		((mtd->writesize / info->num_of_intlv) \
					== NAND_PAGESIZE_4KB)
#define IS_LARGE_PAGE_NAND	((mtd->writesize / info->num_of_intlv) > 512)

#define GET_NAND_OOB_SIZE       (mtd->oobsize / info->num_of_intlv)
#define GET_NAND_PAGE_SIZE      (mtd->writesize / info->num_of_intlv)

/*
 * main area for bad block marker is in the last data section
 * the spare area for swapped bad block marker is the second
 * byte of last spare section
 */
#define NAND_SECTIONS        (GET_NAND_PAGE_SIZE >> 9)
#define NAND_OOB_PER_SECTION (((GET_NAND_OOB_SIZE / NAND_SECTIONS) >> 1) << 1)
#define NAND_CHUNKS          (GET_NAND_PAGE_SIZE / (512 + NAND_OOB_PER_SECTION))

#define BAD_BLK_MARKER_MAIN_OFFS \
	(GET_NAND_PAGE_SIZE - NAND_CHUNKS * NAND_OOB_PER_SECTION)

#define BAD_BLK_MARKER_SP_OFFS (NAND_CHUNKS * SPARE_LEN)

#define BAD_BLK_MARKER_OOB_OFFS (NAND_CHUNKS * NAND_OOB_PER_SECTION)

#define BAD_BLK_MARKER_MAIN  \
	((u32)MAIN_AREA0 + BAD_BLK_MARKER_MAIN_OFFS)

#define BAD_BLK_MARKER_SP  \
	((u32)SPARE_AREA0 + BAD_BLK_MARKER_SP_OFFS)

#define NAND_PAGESIZE_2KB	2048
#define NAND_PAGESIZE_4KB	4096
#define NAND_MAX_PAGESIZE	4096

/*
 * Addresses for NFC registers
 */
#define NFC_REG_BASE		(NFC_BASE_ADDR + 0x1000)
#define NFC_BUF_ADDR		(NFC_REG_BASE + 0xE04)
#define NFC_FLASH_ADDR		(NFC_REG_BASE + 0xE06)
#define NFC_FLASH_CMD		(NFC_REG_BASE + 0xE08)
#define NFC_CONFIG		(NFC_REG_BASE + 0xE0A)
#define NFC_ECC_STATUS_RESULT	(NFC_REG_BASE + 0xE0C)
#define NFC_SPAS		(NFC_REG_BASE + 0xE10)
#define NFC_WRPROT		(NFC_REG_BASE + 0xE12)
#define NFC_UNLOCKSTART_BLKADDR	(NFC_REG_BASE + 0xE20)
#define NFC_UNLOCKEND_BLKADDR	(NFC_REG_BASE + 0xE22)
#define NFC_CONFIG1		(NFC_REG_BASE + 0xE1A)
#define NFC_CONFIG2		(NFC_REG_BASE + 0xE1C)

/*!
 * Addresses for NFC RAM BUFFER Main area 0
 */
#define MAIN_AREA0	(u16 *)(NFC_BASE_ADDR + 0x000)
#define MAIN_AREA1	(u16 *)(NFC_BASE_ADDR + 0x200)

/*!
 * Addresses for NFC SPARE BUFFER Spare area 0
 */
#define SPARE_AREA0	(u16 *)(NFC_BASE_ADDR + 0x1000)
#define SPARE_LEN	64
#define SPARE_COUNT	8
#define SPARE_SIZE	(SPARE_LEN * SPARE_COUNT)


#define SPAS_SHIFT	(0)
#define SPAS_MASK	(0xFF00)
#define IS_4BIT_ECC	\
	((raw_read(REG_NFC_ECC_MODE) & NFC_ECC_MODE_4) >> 0)

#define NFC_SET_SPAS(v)			\
	raw_write(((raw_read(REG_NFC_SPAS) & SPAS_MASK) | \
	((v<<SPAS_SHIFT))), \
	REG_NFC_SPAS)

#define NFC_SET_ECC_MODE(v) \
do { \
	if ((v) == NFC_SPAS_218)  { \
		raw_write((raw_read(REG_NFC_ECC_MODE) & \
		NFC_ECC_MODE_8), \
		REG_NFC_ECC_MODE); \
	} else { \
		raw_write((raw_read(REG_NFC_ECC_MODE) | \
		NFC_ECC_MODE_4), \
		REG_NFC_ECC_MODE); \
	} \
} while (0)

#define GET_ECC_STATUS() \
	__raw_readl(REG_NFC_ECC_STATUS_RESULT);

#define NFC_SET_NFMS(v)	\
do { \
	if (((v) & (1 << NFMS_NF_PG_SZ))) { \
		if (IS_2K_PAGE_NAND) { \
			(NFMS |= 0x00000100); \
			(NFMS &= ~0x00000200); \
			NFC_SET_SPAS(NFC_SPAS_64); \
		} else if (IS_4K_PAGE_NAND) { \
			(NFMS &= ~0x00000100); \
			(NFMS |= 0x00000200); \
			GET_NAND_OOB_SIZE == 128 ? \
			NFC_SET_SPAS(NFC_SPAS_128) : \
			NFC_SET_SPAS(NFC_SPAS_218); \
		} else { \
			printk(KERN_ERR "Err for setting page/oob size"); \
		} \
		NFC_SET_ECC_MODE(GET_NAND_OOB_SIZE >> 1); \
	} \
} while (0)


#define WRITE_NFC_IP_REG(val, reg) \
	raw_write((raw_read(REG_NFC_OPS_STAT) & ~NFC_OPS_STAT), \
	REG_NFC_OPS_STAT)

#define GET_NFC_ECC_STATUS() \
	raw_read(REG_NFC_ECC_STATUS_RESULT);

/*!
 * Set INT to 0, Set 1 to specific operation bit, rest to 0 in LAUNCH_NFC
 * Register for Specific operation
 */
#define NFC_CMD			0x1
#define NFC_ADDR		0x2
#define NFC_INPUT		0x4
#define NFC_OUTPUT		0x8
#define NFC_ID			0x10
#define NFC_STATUS		0x20

/* Bit Definitions */
#define NFC_OPS_STAT			(1 << 15)
#define NFC_SP_EN			(1 << 2)
#define NFC_ECC_EN			(1 << 3)
#define NFC_INT_MSK			(1 << 4)
#define NFC_BIG				(1 << 5)
#define NFC_RST				(1 << 6)
#define NFC_CE				(1 << 7)
#define NFC_ONE_CYCLE       		(1 << 8)
#define NFC_BLS_LOCKED			0
#define NFC_BLS_LOCKED_DEFAULT		1
#define NFC_BLS_UNLCOKED		2
#define NFC_WPC_LOCK_TIGHT		1
#define NFC_WPC_LOCK			(1 << 1)
#define NFC_WPC_UNLOCK			(1 << 2)
#define NFC_FLASH_ADDR_SHIFT 		0
#define NFC_UNLOCK_END_ADDR_SHIFT	0

#define NFC_ECC_MODE_4			(1<<0)
#define NFC_ECC_MODE_8			 (~(1<<0))
#define NFC_SPAS_16			 8
#define NFC_SPAS_64			 32
#define NFC_SPAS_128			 64
#define NFC_SPAS_218			 109

/* NFC Register Mapping */
#define REG_NFC_OPS_STAT		NFC_CONFIG2
#define REG_NFC_INTRRUPT		NFC_CONFIG1
#define REG_NFC_FLASH_ADDR		NFC_FLASH_ADDR
#define REG_NFC_FLASH_CMD		NFC_FLASH_CMD
#define REG_NFC_OPS			NFC_CONFIG2
#define REG_NFC_SET_RBA			NFC_BUF_ADDR
#define REG_NFC_ECC_EN			NFC_CONFIG1
#define REG_NFC_ECC_STATUS_RESULT  	NFC_ECC_STATUS_RESULT
#define REG_NFC_CE			NFC_CONFIG1
#define REG_NFC_SP_EN			NFC_CONFIG1
#define REG_NFC_BLS			NFC_CONFIG
#define REG_NFC_WPC			NFC_WRPROT
#define REG_START_BLKADDR		NFC_UNLOCKSTART_BLKADDR
#define REG_END_BLKADDR			NFC_UNLOCKEND_BLKADDR
#define REG_NFC_RST			NFC_CONFIG1
#define REG_NFC_ECC_MODE		NFC_CONFIG1
#define REG_NFC_SPAS			NFC_SPAS


/* NFC V1/V2 Specific MACRO functions definitions */

#define raw_write(v, a)		__raw_writew(v, a)
#define raw_read(a)		__raw_readw(a)

#define NFC_SET_BLS(val)	val

#define UNLOCK_ADDR(start_addr, end_addr) \
{ \
	raw_write(start_addr, REG_START_BLKADDR); \
	raw_write(end_addr, REG_END_BLKADDR); \
}

#define NFC_SET_NFC_ACTIVE_CS(val)
#define NFC_SET_WPC(val)	val

/* NULL Definitions */
#define ACK_OPS
#define NFC_SET_RBA(val) raw_write(val, REG_NFC_SET_RBA);

#define READ_PAGE()	send_read_page(0)
#define PROG_PAGE()	send_prog_page(0)
#define CHECK_NFC_RB	1

#endif				/* __MXC_NAND_H__ */
