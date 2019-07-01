// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 *
 */
#include <common.h>
#include <malloc.h>
#include <spi.h>
#include <asm/io.h>
#include <linux/sizes.h>
#include <dm.h>
#include <errno.h>
#include <watchdog.h>
#include <clk.h>
#include "fsl_fspi.h"
#include <spi-mem.h>

DECLARE_GLOBAL_DATA_PTR;

#define FSL_FSPI_NAND_SIZE SZ_4G
#define FSL_FSPI_NAND_NUM 1
#define RX_BUFFER_SIZE		0x200
#define TX_BUFFER_SIZE		0x400
#define AHB_BUFFER_SIZE		0x800

#define FLASH_STATUS_WEL	0x02

#define FSPI_NAND_CAS 12

/* SEQID */
enum fspi_lut_id {
	SEQID_RESET = 0,
	SEQID_WREN = 1,
	SEQID_READID = 2,
	SEQID_SET_FEATURE = 3,
	SEQID_GET_FEATURE = 4,
	SEQID_BLK_ERASE	= 5,
	SEQID_PAGE_READ = 6,
	SEQID_READ_FROM_CACHE_NORMAL = 7,
	SEQID_READ_FROM_CACHE_FAST = 8,
	SEQID_READ_FROM_CACHE_X2 = 9,
	SEQID_READ_FROM_CACHE_X4 = 10,
	SEQID_READ_FROM_CACHE_DUALIO = 11,
	SEQID_READ_FROM_CACHE_QUADIO = 12,
	SEQID_PROG_EXEC = 13,
	SEQID_PROG_LOAD = 14,
	SEQID_PROG_LOAD_RANDOM = 15,
	SEQID_PROG_LOAD_X4 = 16,
	SEQID_PROG_LOAD_RANDOM_X4 = 17,
	SEQID_END,
};

/* SPI NAND CMD */
#define SPINAND_CMD_RESET		0xff
#define SPINAND_CMD_WREN	0x06
#define SPINAND_CMD_READID	0x9f
#define SPINAND_CMD_SET_FEATURE	0x1f
#define SPINAND_CMD_GET_FEATURE	0x0f
#define SPINAND_CMD_BLK_ERASE	0xd8
#define SPINAND_CMD_PAGE_READ	0x13
#define SPINAND_CMD_PAGE_READ_FROM_CACHE_NORMAL	0x03
#define SPINAND_CMD_PAGE_READ_FROM_CACHE_FAST	0x0b
#define SPINAND_CMD_PAGE_READ_FROM_CACHE_X2	0x3b
#define SPINAND_CMD_PAGE_READ_FROM_CACHE_X4	0x6b
#define SPINAND_CMD_PAGE_READ_FROM_CACHE_DUALIO	0xbb
#define SPINAND_CMD_PAGE_READ_FROM_CACHE_QUADIO	0xeb
#define SPINAND_CMD_PROG_EXEC	0x10
#define SPINAND_CMD_PROG_LOAD	0x02
#define SPINAND_CMD_PROG_LOAD_RANDOM	0x84
#define SPINAND_CMD_PROG_LOAD_X4	0x32
#define SPINAND_CMD_PROG_LOAD_RANDOM_X4	0x34

/* fsl_fspi_platdata flags */
#define FSPI_FLAG_REGMAP_ENDIAN_BIG	BIT(0)

/* default SCK frequency, unit: HZ */
#define FSL_FSPI_DEFAULT_SCK_FREQ	50000000

/* FSPI max chipselect signals number */
#define FSL_FSPI_MAX_CHIPSELECT_NUM     4

/**
 * struct fsl_fspi_platdata - platform data for NXP FSPI
 *
 * @flags: Flags for FSPI FSPI_FLAG_...
 * @speed_hz: Default SCK frequency
 * @reg_base: Base address of FSPI registers
 * @amba_base: Base address of FSPI memory mapping
 * @amba_total_size: size of FSPI memory mapping
 * @flash_num: Number of active slave devices
 * @num_chipselect: Number of FSPI chipselect signals
 */
struct fsl_fspi_platdata {
	u32 flags;
	u32 speed_hz;
	u32 reg_base;
	u32 amba_base;
	u32 amba_total_size;
	u32 flash_num;
	u32 num_chipselect;
};

/**
 * struct fsl_fspi_priv - private data for NXP FSPI
 *
 * @flags: Flags for FSPI FSPI_FLAG_...
 * @bus_clk: FSPI input clk frequency
 * @speed_hz: Default SCK frequency
 * @cur_seqid: current LUT table sequence id
 * @sf_addr: flash access offset
 * @amba_base: Base address of FSPI memory mapping of every CS
 * @amba_total_size: size of FSPI memory mapping
 * @cur_amba_base: Base address of FSPI memory mapping of current CS
 * @flash_num: Number of active slave devices
 * @num_chipselect: Number of FSPI chipselect signals
 * @regs: Point to FSPI register structure for I/O access
 */
struct fsl_fspi_priv {
	u32 flags;
	u32 bus_clk;
	u32 speed_hz;
	u32 cur_seqid;
	u32 sf_addr;
	u32 amba_base[FSL_FSPI_MAX_CHIPSELECT_NUM];
	u32 amba_total_size;
	u32 cur_amba_base;
	u32 flash_num;
	u32 num_chipselect;
	struct fsl_fspi_regs *regs;
};

struct fspi_cmd_func_pair {
	u8 cmd;
	int (*fspi_op_func)(struct fsl_fspi_priv *priv, u32 seqid, const struct spi_mem_op *op);
};

static u32 fspi_read32(u32 flags, u32 *addr)
{
	return flags & FSPI_FLAG_REGMAP_ENDIAN_BIG ?
		in_be32(addr) : in_le32(addr);
}

static void fspi_write32(u32 flags, u32 *addr, u32 val)
{
	flags & FSPI_FLAG_REGMAP_ENDIAN_BIG ?
		out_be32(addr, val) : out_le32(addr, val);
}

static void fspi_nand_set_lut(struct fsl_fspi_priv *priv)
{
	struct fsl_fspi_regs *regs = priv->regs;
	u32 lut_base;

	/* Unlock the LUT */
	fspi_write32(priv->flags, &regs->lutkey, FLEXSPI_LUTKEY_VALUE);
	fspi_write32(priv->flags, &regs->lutcr, FLEXSPI_LCKER_UNLOCK);

	/* RESET */
	lut_base = SEQID_RESET * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(SPINAND_CMD_RESET) | PAD0(LUT_PAD1) |
		     INSTR0(LUT_CMD));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Write Enable */
	lut_base = SEQID_WREN * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(SPINAND_CMD_WREN) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Read ID*/
	lut_base = SEQID_READID * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(SPINAND_CMD_READID) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(4) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_READ));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Set Feature */
	lut_base = SEQID_SET_FEATURE * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(SPINAND_CMD_SET_FEATURE) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(0xB0) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_CMD));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], OPRND0(1) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_WRITE));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Get Feature */
	lut_base = SEQID_GET_FEATURE * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(SPINAND_CMD_GET_FEATURE) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(0xC0) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_CMD));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], OPRND0(1) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_READ));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Erase a block */
	lut_base = SEQID_BLK_ERASE * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(SPINAND_CMD_BLK_ERASE) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(ADDR24BIT) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Page read */
	lut_base = SEQID_PAGE_READ * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(SPINAND_CMD_PAGE_READ) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD)  | OPRND1(ADDR24BIT) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Read from cache normal */
	lut_base = SEQID_READ_FROM_CACHE_NORMAL * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(SPINAND_CMD_PAGE_READ_FROM_CACHE_NORMAL) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD)  | OPRND1(0) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_MODE4));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], OPRND0(ADDR12BIT) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CADDR_SDR) | OPRND1(8) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_DUMMY));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], OPRND0(0) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_READ));
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Read from cache fast */
	lut_base = SEQID_READ_FROM_CACHE_FAST * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(SPINAND_CMD_PAGE_READ_FROM_CACHE_FAST) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD)  | OPRND1(0) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_MODE4));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], OPRND0(ADDR12BIT) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CADDR_SDR) | OPRND1(8) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_DUMMY));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], OPRND0(0) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_READ));
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Read from cache x2 */
	lut_base = SEQID_READ_FROM_CACHE_X2 * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(SPINAND_CMD_PAGE_READ_FROM_CACHE_X2) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD)  | OPRND1(0) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_MODE4));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], OPRND0(ADDR12BIT) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CADDR_SDR) | OPRND1(8) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_DUMMY));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], OPRND0(0) |
		     PAD0(LUT_PAD2) | INSTR0(LUT_READ));
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Read from cache x4 */
	lut_base = SEQID_READ_FROM_CACHE_X4 * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(SPINAND_CMD_PAGE_READ_FROM_CACHE_X4) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD)  | OPRND1(0) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_MODE4));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], OPRND0(ADDR12BIT) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CADDR_SDR) | OPRND1(8) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_DUMMY));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], OPRND0(0) |
		     PAD0(LUT_PAD4) | INSTR0(LUT_READ));
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Read from cache dual IO */
	lut_base = SEQID_READ_FROM_CACHE_DUALIO * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(SPINAND_CMD_PAGE_READ_FROM_CACHE_DUALIO) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(0) |
		     PAD1(LUT_PAD2) | INSTR1(LUT_MODE4));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], OPRND0(ADDR12BIT) |
		     PAD0(LUT_PAD2) | INSTR0(LUT_CADDR_SDR) | OPRND1(4) |
		     PAD1(LUT_PAD2) | INSTR1(LUT_DUMMY));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2],  OPRND0(0) |
		     PAD0(LUT_PAD2) | INSTR0(LUT_READ));
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Read from cache Quad IO */
	lut_base = SEQID_READ_FROM_CACHE_QUADIO * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(SPINAND_CMD_PAGE_READ_FROM_CACHE_QUADIO) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(0) |
		     PAD1(LUT_PAD4) | INSTR1(LUT_MODE4));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], OPRND0(ADDR12BIT) |
		     PAD0(LUT_PAD4) | INSTR0(LUT_CADDR_SDR) | OPRND1(4) |
		     PAD1(LUT_PAD4) | INSTR1(LUT_DUMMY));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], OPRND0(0) |
		     PAD0(LUT_PAD4) | INSTR0(LUT_READ));
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Program execute */
	lut_base = SEQID_PROG_EXEC * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(SPINAND_CMD_PROG_EXEC) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD)  | OPRND1(ADDR24BIT) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Program load */
	lut_base = SEQID_PROG_LOAD * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(SPINAND_CMD_PROG_LOAD) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD)  | OPRND1(0) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_MODE4));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], OPRND0(ADDR12BIT) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CADDR_SDR) | OPRND1(0) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_WRITE));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Program load random */
	lut_base = SEQID_PROG_LOAD_RANDOM * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(SPINAND_CMD_PROG_LOAD_RANDOM) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD)  | OPRND1(0) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_MODE4));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1],  OPRND0(ADDR12BIT) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CADDR_SDR) | OPRND1(0) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_WRITE));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Program load x4 */
	lut_base = SEQID_PROG_LOAD_X4 * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(SPINAND_CMD_PROG_LOAD_X4) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD)  | OPRND1(0) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_MODE4));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1],  OPRND0(ADDR12BIT) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CADDR_SDR) | OPRND1(0) |
		     PAD1(LUT_PAD4) | INSTR1(LUT_WRITE));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Program load random x4 */
	lut_base = SEQID_PROG_LOAD_RANDOM_X4 * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(SPINAND_CMD_PROG_LOAD_RANDOM_X4) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD)  | OPRND1(0) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_MODE4));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], OPRND0(ADDR12BIT) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CADDR_SDR) | OPRND1(0) |
		     PAD1(LUT_PAD4) | INSTR1(LUT_WRITE));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Lock the LUT */
	fspi_write32(priv->flags, &regs->lutkey, FLEXSPI_LUTKEY_VALUE);
	fspi_write32(priv->flags, &regs->lutcr, FLEXSPI_LCKER_LOCK);
}

static int fspi_nand_op_cmd(struct fsl_fspi_priv *priv, u32 seqid, const struct spi_mem_op *op)
{
	struct fsl_fspi_regs *regs = priv->regs;
	u32 addr = priv->cur_amba_base;

	debug("%s seqid=%u, addr_nbytes = %u, addr_val = %llx\n",
	      __func__, seqid, op->addr.nbytes, op->addr.val);

	if (op->addr.nbytes != 0)
		addr = (op->addr.val << FSPI_NAND_CAS) + addr;

	/* invalid the TXFIFO first */
	fspi_write32(priv->flags, &regs->iptxfcr, FLEXSPI_IPTXFCR_CLR_MASK);

	fspi_write32(priv->flags, &regs->ipcr0, addr);

	fspi_write32(priv->flags, &regs->ipcr1,
		     (seqid << FLEXSPI_IPCR1_SEQID_SHIFT) | 0);

	/* Trigger the command */
	fspi_write32(priv->flags, &regs->ipcmd, 1);

	/* Wait for command done */
	while (!(fspi_read32(priv->flags, &regs->intr)
		 & FLEXSPI_INTR_IPCMDDONE_MASK))
		;

	fspi_write32(priv->flags, &regs->intr, FLEXSPI_INTR_IPCMDDONE_MASK);

	return 0;
}

static void fspi_nand_set_oprnd1(struct fsl_fspi_priv *priv,  u32 seqid, u8 oprnd)
{
	struct fsl_fspi_regs *regs = priv->regs;
	u32 lut_base, val;

	debug("set oprnd1 %u\n", oprnd);

	/* Unlock the LUT */
	fspi_write32(priv->flags, &regs->lutkey, FLEXSPI_LUTKEY_VALUE);
	fspi_write32(priv->flags, &regs->lutcr, FLEXSPI_LCKER_UNLOCK);

	lut_base = seqid * 4;
	val = fspi_read32(priv->flags, &regs->lut[lut_base]);
	val &= ~(OPRND1(0xff));
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     val  | OPRND1(oprnd));

	/* Lock the LUT */
	fspi_write32(priv->flags, &regs->lutkey, FLEXSPI_LUTKEY_VALUE);
	fspi_write32(priv->flags, &regs->lutcr, FLEXSPI_LCKER_LOCK);
}

static int fspi_nand_op_read_reg(struct fsl_fspi_priv *priv, u32 seqid, const struct spi_mem_op *op)
{
	struct fsl_fspi_regs *regs = priv->regs;
	u32 data, size, len = op->data.nbytes;
	int i;
	u8 *rxbuf = op->data.buf.in;

	debug("%s seqid=%u, data_nbytes = %u, data_buf_in = %lx\n",
	      __func__, seqid, op->data.nbytes, (ulong)rxbuf);

	if (op->addr.nbytes == 1) {
		fspi_nand_set_oprnd1(priv, seqid, (u8)op->addr.val);
	} else if (op->addr.nbytes != 0) {
		printf("Error: %s seqid=%u, reg addr size is %u\n",
		       __func__, seqid, op->addr.nbytes);
		return -EINVAL;
	}

	/* invalid the RXFIFO first */
	fspi_write32(priv->flags, &regs->iprxfcr, FLEXSPI_IPRXFCR_CLR_MASK);

	fspi_write32(priv->flags, &regs->ipcr0, priv->cur_amba_base);

	fspi_write32(priv->flags, &regs->ipcr1,
		     (seqid << FLEXSPI_IPCR1_SEQID_SHIFT) | len);
	/* Trigger the command */
	fspi_write32(priv->flags, &regs->ipcmd, 1);

	/* Wait for command done */
	while (!(fspi_read32(priv->flags, &regs->intr)
		 & FLEXSPI_INTR_IPCMDDONE_MASK))
		;

	i = 0;
	while ((32 >= len) && (len > 0)) {
		data = fspi_read32(priv->flags, &regs->rfdr[i]);

		debug("rfdr 0x%x\n", data);
		size = (len < 4) ? len : 4;
		memcpy(rxbuf, &data, size);
		len -= size;
		rxbuf += size;
		i++;
	}
	fspi_write32(priv->flags, &regs->intr, FLEXSPI_INTR_IPRXWA_MASK);

	fspi_write32(priv->flags, &regs->iprxfcr, FLEXSPI_IPRXFCR_CLR_MASK);
	fspi_write32(priv->flags, &regs->intr, FLEXSPI_INTR_IPCMDDONE_MASK);

	return 0;
}

static int fspi_nand_op_write_reg(struct fsl_fspi_priv *priv, u32 seqid, const struct spi_mem_op *op)
{
	struct fsl_fspi_regs *regs = priv->regs;
	u32 data, size, len = op->data.nbytes;
	int i;
	u8 *txbuf = (u8 *)(op->data.buf.out);

	debug("%s seqid=%u, data_nbytes = %u, data_buf_out = %lx\n",
	      __func__, seqid, op->data.nbytes, (ulong)txbuf);

	if (op->addr.nbytes != 1) {
		printf("Error: fspi_nand_%s seqid=%u, reg addr size is %u\n",
		       __func__, seqid, op->addr.nbytes);
		return -EINVAL;
	}

	fspi_nand_set_oprnd1(priv, seqid, (u8)op->addr.val);

	/* invalid the TXFIFO first */
	fspi_write32(priv->flags, &regs->iprxfcr, FLEXSPI_IPTXFCR_CLR_MASK);

	fspi_write32(priv->flags, &regs->ipcr0, priv->cur_amba_base);

	i = 0;
	while ((32 >= len) && (len > 0)) {
		data = 0;
		size = (len < 4) ? len : 4;
		memcpy(&data, txbuf, size);
		 fspi_write32(priv->flags, &regs->tfdr[i], data);
		len -= size;
		txbuf += size;
		i++;
	}
	fspi_write32(priv->flags, &regs->intr, FLEXSPI_INTR_IPTXWE_MASK);

	fspi_write32(priv->flags, &regs->ipcr1,
		     (seqid << FLEXSPI_IPCR1_SEQID_SHIFT) | len);

	/* Trigger the command */
	fspi_write32(priv->flags, &regs->ipcmd, 1);

	/* Wait for command done */
	while (!(fspi_read32(priv->flags, &regs->intr)
		& FLEXSPI_INTR_IPCMDDONE_MASK))
		;

	/* invalid the TXFIFO first */
	fspi_write32(priv->flags, &regs->iptxfcr,
		     FLEXSPI_IPTXFCR_CLR_MASK);
	fspi_write32(priv->flags, &regs->intr,
		     FLEXSPI_INTR_IPCMDDONE_MASK);

	return 0;
}

static int fspi_nand_op_read(struct fsl_fspi_priv *priv, u32 seqid, const struct spi_mem_op *op)
{
	struct fsl_fspi_regs *regs = priv->regs;
	int i, size;
	u32 to_or_from;
	u8 *rxbuf;
	u8 panel;

	to_or_from = op->addr.val + priv->cur_amba_base;
	rxbuf = op->data.buf.in;
	panel = op->addr.val >> FSPI_NAND_CAS;

	/* Update LUT to select plane */
	fspi_nand_set_oprnd1(priv, seqid, panel);

	debug("%s seqid=%u, addr_val = 0x%llx, addr_nbytes = %u, data_buf_in = 0x%lx, data_nbytes = 0x%x\n",
	      __func__, seqid, op->addr.val, op->addr.nbytes,
	      (ulong)rxbuf, op->data.nbytes);

	/* invalid the RXFIFO */
	fspi_write32(priv->flags, &regs->iprxfcr, FLEXSPI_IPRXFCR_CLR_MASK);

	fspi_write32(priv->flags, &regs->ipcr0, to_or_from);

	fspi_write32(priv->flags, &regs->ipcr1,
		     (seqid << FLEXSPI_IPCR1_SEQID_SHIFT) |
		     op->data.nbytes);

	/* Trigger the command */
	fspi_write32(priv->flags, &regs->ipcmd, 1);

	size = op->data.nbytes / 8;
	for (i = 0; i < size; ++i) {
		/* Wait for RXFIFO available*/
		while (!(fspi_read32(priv->flags, &regs->intr)
			 & FLEXSPI_INTR_IPRXWA_MASK))
			;

		memcpy(rxbuf, &regs->rfdr, 8);
		rxbuf += 8;

		/* move the FIFO pointer */
		fspi_write32(priv->flags, &regs->intr,
			     FLEXSPI_INTR_IPRXWA_MASK);
	}

	size = op->data.nbytes % 8;

	if (size) {
		/* Wait for data filled*/
		while (!(fspi_read32(priv->flags, &regs->iprxfsts)
			& FLEXSPI_IPRXFSTS_FILL_MASK))
			;
		memcpy(rxbuf, &regs->rfdr, size);
	}

	/* invalid the RXFIFO */
	fspi_write32(priv->flags, &regs->iprxfcr,
		     FLEXSPI_IPRXFCR_CLR_MASK);
	fspi_write32(priv->flags, &regs->intr,
		     FLEXSPI_INTR_IPCMDDONE_MASK);

	return 0;
}

static int fspi_nand_op_write(struct fsl_fspi_priv *priv, u32 seqid, const struct spi_mem_op *op)
{
	struct fsl_fspi_regs *regs = priv->regs;
	int i, size;
	u8 *txbuf;
	u8 panel;
	u32 to_or_from = op->addr.val + priv->cur_amba_base;

	txbuf = (u8 *)(op->data.buf.out);
	panel = op->addr.val >> FSPI_NAND_CAS;

	/* Update LUT to select plane */
	fspi_nand_set_oprnd1(priv, seqid, panel);

	debug("%s seqid=%u, addr_val = 0x%llx, addr_nbytes = %u, data_buf_in = 0x%lx, data_nbytes = 0x%x\n",
	      __func__, seqid, op->addr.val, op->addr.nbytes,
	      (ulong)txbuf, op->data.nbytes);

	/* invalid the TXFIFO first */
	fspi_write32(priv->flags, &regs->iptxfcr, FLEXSPI_IPTXFCR_CLR_MASK);

	fspi_write32(priv->flags, &regs->ipcr0, to_or_from);

	size = op->data.nbytes / 8;
	for (i = 0; i < size; i++) {
		/* Wait for TXFIFO empty*/
		while (!(fspi_read32(priv->flags, &regs->intr)
			 & FLEXSPI_INTR_IPTXWE_MASK))
			;

		memcpy(&regs->tfdr, txbuf, 8);
		txbuf += 8;
		fspi_write32(priv->flags, &regs->intr,
			     FLEXSPI_INTR_IPTXWE_MASK);
	}

	size = op->data.nbytes % 8;
	if (size) {
		/* Wait for TXFIFO empty*/
		while (!(fspi_read32(priv->flags, &regs->intr)
			 & FLEXSPI_INTR_IPTXWE_MASK))
			;

		memcpy(&regs->tfdr, txbuf, size);
		fspi_write32(priv->flags, &regs->intr,
			     FLEXSPI_INTR_IPTXWE_MASK);
	}

	fspi_write32(priv->flags, &regs->ipcr1,
		     (seqid << FLEXSPI_IPCR1_SEQID_SHIFT) | op->data.nbytes);

	/* Trigger the command */
	fspi_write32(priv->flags, &regs->ipcmd, 1);

	/* Wait for command done */
	while (!(fspi_read32(priv->flags, &regs->intr)
		& FLEXSPI_INTR_IPCMDDONE_MASK))
		;

	/* invalid the TXFIFO first */
	fspi_write32(priv->flags, &regs->iptxfcr,
		     FLEXSPI_IPTXFCR_CLR_MASK);
	fspi_write32(priv->flags, &regs->intr,
		     FLEXSPI_INTR_IPCMDDONE_MASK);

	return 0;
}

void fspi_nand_module_disable(struct fsl_fspi_priv *priv, u8 disable)
{
	u32 mcr_val;

	mcr_val = fspi_read32(priv->flags, &priv->regs->mcr0);
	if (disable)
		mcr_val |= FLEXSPI_MCR0_MDIS_MASK;
	else
		mcr_val &= ~FLEXSPI_MCR0_MDIS_MASK;
	fspi_write32(priv->flags, &priv->regs->mcr0, mcr_val);
}

__weak void init_clk_fspi(int index)
{
}

static int fsl_fspi_nand_child_pre_probe(struct udevice *dev)
{
	struct spi_slave *slave = dev_get_parent_priv(dev);

	slave->max_write_size = TX_BUFFER_SIZE;
	slave->max_read_size = RX_BUFFER_SIZE;

	return 0;
}

static int fsl_fspi_nand_probe(struct udevice *bus)
{
	u32 total_size;
	struct fsl_fspi_platdata *plat = dev_get_platdata(bus);
	struct fsl_fspi_priv *priv = dev_get_priv(bus);
	struct dm_spi_bus *dm_spi_bus;
	u32 val;

	if (CONFIG_IS_ENABLED(CLK)) {
		/* Assigned clock already set clock */
		struct clk fspi_clk;
		int ret;

		ret = clk_get_by_name(bus, "fspi", &fspi_clk);
		if (ret < 0) {
			printf("Can't get fspi clk: %d\n", ret);
			return ret;
		}

		ret = clk_enable(&fspi_clk);
		if (ret < 0) {
			printf("Can't enable fspi clk: %d\n", ret);
			return ret;
		}
	} else {
		init_clk_fspi(bus->seq);
	}
	dm_spi_bus = bus->uclass_priv;

	dm_spi_bus->max_hz = plat->speed_hz;

	priv->regs = (struct fsl_fspi_regs *)(uintptr_t)plat->reg_base;
	priv->flags = plat->flags;

	priv->speed_hz = plat->speed_hz;
	priv->amba_base[0] = plat->amba_base;
	priv->amba_total_size = plat->amba_total_size;
	priv->flash_num = plat->flash_num;
	priv->num_chipselect = plat->num_chipselect;

	fspi_write32(priv->flags, &priv->regs->mcr0,
		     FLEXSPI_MCR0_SWRST_MASK);
	do {
		udelay(1);
	} while (0x1 & fspi_read32(priv->flags, &priv->regs->mcr0));

	/* Disable the module */
	fspi_nand_module_disable(priv, 1);

	/* Enable the module and set to proper value*/
	fspi_write32(priv->flags, &priv->regs->mcr0,
		     0xFFFF0000);

	/* Reset the DLL register to default value */
	fspi_write32(priv->flags, &priv->regs->dllacr, 0x0100);
	fspi_write32(priv->flags, &priv->regs->dllbcr, 0x0100);

	/* Flash Size in KByte */
	total_size = FSL_FSPI_NAND_SIZE * FSL_FSPI_NAND_NUM >> 10;

	/*
	 * Any read access to non-implemented addresses will provide
	 * undefined results.
	 *
	 * In case single die flash devices, TOP_ADDR_MEMA2 and
	 * TOP_ADDR_MEMB2 should be initialized/programmed to
	 * TOP_ADDR_MEMA1 and TOP_ADDR_MEMB1 respectively - in effect,
	 * setting the size of these devices to 0.  This would ensure
	 * that the complete memory map is assigned to only one flash device.
	 */

	fspi_write32(priv->flags, &priv->regs->flsha1cr0,
		     total_size);
	fspi_write32(priv->flags, &priv->regs->flsha2cr0,
		     0);
	fspi_write32(priv->flags, &priv->regs->flshb1cr0,
		     0);
	fspi_write32(priv->flags, &priv->regs->flshb2cr0,
		     0);

	val = fspi_read32(priv->flags, &priv->regs->flsha1cr1);
	val &= ~FLEXSPI_FLSHXCR1_CAS_MASK;
	val |= FSPI_NAND_CAS << FLEXSPI_FLSHXCR1_CAS_SHIFT;
	fspi_write32(priv->flags, &priv->regs->flsha1cr1,
		     val);
	fspi_nand_module_disable(priv, 0);

	fspi_nand_set_lut(priv);

	return 0;
}

static int fsl_fspi_nand_ofdata_to_platdata(struct udevice *bus)
{
	struct fdt_resource res_regs, res_mem;
	struct fsl_fspi_platdata *plat = bus->platdata;
	const void *blob = gd->fdt_blob;
	int node = ofnode_to_offset(bus->node);
	int ret, flash_num = 0, subnode;

	if (fdtdec_get_bool(blob, node, "big-endian"))
		plat->flags |= FSPI_FLAG_REGMAP_ENDIAN_BIG;

	ret = fdt_get_named_resource(blob, node, "reg", "reg-names",
				     "FlexSPI", &res_regs);
	if (ret) {
		debug("Error: can't get regs base addresses(ret = %d)!\n", ret);
		return -ENOMEM;
	}
	ret = fdt_get_named_resource(blob, node, "reg", "reg-names",
				     "FlexSPI-memory", &res_mem);
	if (ret) {
		debug("Error: can't get AMBA base addresses(ret = %d)!\n", ret);
		return -ENOMEM;
	}

	/* Count flash numbers */
	fdt_for_each_subnode(subnode, blob, node)
		++flash_num;

	if (flash_num == 0) {
		debug("Error: Missing flashes!\n");
		return -ENODEV;
	}

	plat->speed_hz = fdtdec_get_int(blob, node, "spi-max-frequency",
					FSL_FSPI_DEFAULT_SCK_FREQ);
	plat->num_chipselect = fdtdec_get_int(blob, node, "num-cs",
					      FSL_FSPI_MAX_CHIPSELECT_NUM);

	plat->reg_base = res_regs.start;
	plat->amba_base = 0;
	plat->amba_total_size = res_mem.end - res_mem.start + 1;
	plat->flash_num = flash_num;

	debug("%s: regs=<0x%x> <0x%x, 0x%x>, max-frequency=%d, endianness=%s\n",
	      __func__,
	      plat->reg_base,
	      plat->amba_base,
	      plat->amba_total_size,
	      plat->speed_hz,
	      plat->flags & FSPI_FLAG_REGMAP_ENDIAN_BIG ? "be" : "le"
	      );

	return 0;
}

static int fsl_fspi_nand_claim_bus(struct udevice *dev)
{
	struct fsl_fspi_priv *priv;
	struct udevice *bus;
	struct dm_spi_slave_platdata *slave_plat = dev_get_parent_platdata(dev);

	bus = dev->parent;
	priv = dev_get_priv(bus);

	priv->cur_amba_base =
		priv->amba_base[0] + FSL_FSPI_NAND_SIZE * slave_plat->cs;

	return 0;
}

static int fsl_fspi_nand_release_bus(struct udevice *dev)
{
	return 0;
}

static int fsl_fspi_nand_set_speed(struct udevice *bus, uint speed)
{
	/* Nothing to do */
	return 0;
}

static int fsl_fspi_nand_set_mode(struct udevice *bus, uint mode)
{
	/* Nothing to do */
	return 0;
}

struct fspi_cmd_func_pair fspi_supported_cmds[SEQID_END] = {
	{SPINAND_CMD_RESET, &fspi_nand_op_cmd},
	{SPINAND_CMD_WREN, &fspi_nand_op_cmd},
	{SPINAND_CMD_READID, &fspi_nand_op_read_reg},
	{SPINAND_CMD_SET_FEATURE, &fspi_nand_op_write_reg},
	{SPINAND_CMD_GET_FEATURE, &fspi_nand_op_read_reg},
	{SPINAND_CMD_BLK_ERASE, &fspi_nand_op_cmd},
	{SPINAND_CMD_PAGE_READ, &fspi_nand_op_cmd},
	{SPINAND_CMD_PAGE_READ_FROM_CACHE_NORMAL, &fspi_nand_op_read},
	{SPINAND_CMD_PAGE_READ_FROM_CACHE_FAST, &fspi_nand_op_read},
	{SPINAND_CMD_PAGE_READ_FROM_CACHE_X2, &fspi_nand_op_read},
	{SPINAND_CMD_PAGE_READ_FROM_CACHE_X4, &fspi_nand_op_read},
	{SPINAND_CMD_PAGE_READ_FROM_CACHE_DUALIO, &fspi_nand_op_read},
	{SPINAND_CMD_PAGE_READ_FROM_CACHE_QUADIO, &fspi_nand_op_read},
	{SPINAND_CMD_PROG_EXEC, &fspi_nand_op_cmd},
	{SPINAND_CMD_PROG_LOAD, &fspi_nand_op_write},
	{SPINAND_CMD_PROG_LOAD_RANDOM, &fspi_nand_op_write},
	{SPINAND_CMD_PROG_LOAD_X4, &fspi_nand_op_write},
	{SPINAND_CMD_PROG_LOAD_RANDOM_X4, &fspi_nand_op_write},
};

static int fsl_fspi_nand_get_lut_index(const struct spi_mem_op *op)
{
	int i;

	for (i = 0; i < SEQID_END; i++) {
		if (fspi_supported_cmds[i].cmd == op->cmd.opcode)
			break;
	}

	return i;
}

bool fsl_fspi_nand_supports_op(struct spi_slave *slave, const struct spi_mem_op *op)
{
	int i;

	if (!op || !slave)
		return false;

	i = fsl_fspi_nand_get_lut_index(op);
	if (i == SEQID_END) {
		printf("fsl_fspi_nand: fail to find cmd %u from lut\n", op->cmd.opcode);
		return false;
	}

	debug("fsl_fspi_nand: find seqid %d for cmd %u from lut\n", i, op->cmd.opcode);

	return true;
}

int fsl_fspi_nand_exec_op(struct spi_slave *slave, const struct spi_mem_op *op)
{
	int i;
	struct fsl_fspi_priv *priv;
	struct udevice *bus;

	bus = slave->dev->parent;
	priv = dev_get_priv(bus);

	if (!op || !slave)
		return -EINVAL;

	i = fsl_fspi_nand_get_lut_index(op);
	if (i == SEQID_END) {
		printf("fsl_fspi_nand: fail to find cmd %u from lut\n", op->cmd.opcode);
		return -EPERM;
	}

	return fspi_supported_cmds[i].fspi_op_func(priv, i, op);
}

int fsl_fspi_nand_adjust_op_size(struct spi_slave *slave, struct spi_mem_op *op)
{
	switch (op->cmd.opcode) {
	case SPINAND_CMD_PAGE_READ_FROM_CACHE_NORMAL:
	case SPINAND_CMD_PAGE_READ_FROM_CACHE_FAST:
	case SPINAND_CMD_PAGE_READ_FROM_CACHE_X2:
	case SPINAND_CMD_PAGE_READ_FROM_CACHE_X4:
	case SPINAND_CMD_PAGE_READ_FROM_CACHE_DUALIO:
	case SPINAND_CMD_PAGE_READ_FROM_CACHE_QUADIO:
		if (op->data.nbytes > RX_BUFFER_SIZE)
			op->data.nbytes = RX_BUFFER_SIZE;
		break;
	case SPINAND_CMD_PROG_LOAD:
	case SPINAND_CMD_PROG_LOAD_RANDOM:
	case SPINAND_CMD_PROG_LOAD_X4:
	case SPINAND_CMD_PROG_LOAD_RANDOM_X4:
		if (op->data.nbytes > TX_BUFFER_SIZE)
			op->data.nbytes = TX_BUFFER_SIZE;
		break;
	}

	return 0;
}

static struct spi_controller_mem_ops fspi_nand_mem_ops = {
	.adjust_op_size = fsl_fspi_nand_adjust_op_size,
	.supports_op = fsl_fspi_nand_supports_op,
	.exec_op = fsl_fspi_nand_exec_op,
};

static const struct dm_spi_ops fsl_fspi_nand_ops = {
	.claim_bus	= fsl_fspi_nand_claim_bus,
	.release_bus	= fsl_fspi_nand_release_bus,
	.set_speed	= fsl_fspi_nand_set_speed,
	.set_mode	= fsl_fspi_nand_set_mode,
	.mem_ops	= &fspi_nand_mem_ops,
};

static const struct udevice_id fsl_fspi_nand_ids[] = {
	{ .compatible = "fsl,imx8-fspi-nand" },
	{ }
};

U_BOOT_DRIVER(fsl_fspi_nand) = {
	.name	= "fsl_fspi_nand",
	.id	= UCLASS_SPI,
	.of_match = fsl_fspi_nand_ids,
	.ops	= &fsl_fspi_nand_ops,
	.ofdata_to_platdata = fsl_fspi_nand_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct fsl_fspi_platdata),
	.priv_auto_alloc_size = sizeof(struct fsl_fspi_priv),
	.probe	= fsl_fspi_nand_probe,
	.child_pre_probe = fsl_fspi_nand_child_pre_probe,
};
