// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023 NXP
 */

#include <command.h>
#include <dm.h>
#include <mapmem.h>
#include <asm/io.h>
#include <mtd.h>

#define FIRMWARE_MAX_NUM		8
#define SERIAL_NAND_BAD_BLOCK_MAX_NUM	256U
#define FSPI_CFG_BLK_TAG		0x42464346/* FCFB, bigendian */
#define FSPI_CFG_BLK_VERSION		0x56010000/* Version 1.0 */
#define FSPI_CFG_BLK_SIZE		512

#define FSPI_NAND_FCB_FINGERPRINT	0x4E464342	/* 'NFCB' */
#define FSPI_NAND_FCB_VERSION		0x00000001	/* Version 1.0 */
#define FSPI_NAND_DBBT_FINGERPRINT	0x44424254	/* 'DBBT' */
#define FSPI_NAND_DBBT_VERSION		0x00000001	/* Version 1.0 */

#define FSPI_LUT_OPERAND0_MASK (0xFFU)
#define FSPI_LUT_OPERAND0_SHIFT (0U)
#define FSPI_LUT_OPERAND0(x) \
	(((u32)(((u32)(x)) << FSPI_LUT_OPERAND0_SHIFT)) & FSPI_LUT_OPERAND0_MASK)
#define FSPI_LUT_NUM_PADS0_MASK (0x300U)
#define FSPI_LUT_NUM_PADS0_SHIFT (8U)
#define FSPI_LUT_NUM_PADS0(x) \
	(((u32)(((u32)(x)) << FSPI_LUT_NUM_PADS0_SHIFT)) & FSPI_LUT_NUM_PADS0_MASK)
#define FSPI_LUT_OPCODE0_MASK (0xFC00U)
#define FSPI_LUT_OPCODE0_SHIFT (10U)
#define FSPI_LUT_OPCODE0(x) (((u32)(((u32)(x)) << FSPI_LUT_OPCODE0_SHIFT)) & FSPI_LUT_OPCODE0_MASK)
#define FSPI_LUT_OPERAND1_MASK (0xFF0000U)
#define FSPI_LUT_OPERAND1_SHIFT (16U)
#define FSPI_LUT_OPERAND1(x) \
	(((u32)(((u32)(x)) << FSPI_LUT_OPERAND1_SHIFT)) & FSPI_LUT_OPERAND1_MASK)
#define FSPI_LUT_NUM_PADS1_MASK (0x3000000U)
#define FSPI_LUT_NUM_PADS1_SHIFT (24U)
#define FSPI_LUT_NUM_PADS1(x) \
	(((u32)(((u32)(x)) << FSPI_LUT_NUM_PADS1_SHIFT)) & FSPI_LUT_NUM_PADS1_MASK)
#define FSPI_LUT_OPCODE1_MASK (0xFC000000U)
#define FSPI_LUT_OPCODE1_SHIFT (26U)
#define FSPI_LUT_OPCODE1(x) (((u32)(((u32)(x)) << FSPI_LUT_OPCODE1_SHIFT)) & FSPI_LUT_OPCODE1_MASK)

#define FSPI_LUT_SEQ(cmd0, pad0, op0, cmd1, pad1, op1) \
	(FSPI_LUT_OPERAND0(op0) | FSPI_LUT_NUM_PADS0(pad0) | FSPI_LUT_OPCODE0(cmd0) | \
	 FSPI_LUT_OPERAND1(op1) | FSPI_LUT_NUM_PADS1(pad1) | FSPI_LUT_OPCODE1(cmd1))

#define CMD_SDR         0x01U
#define CMD_DDR         0x21U
#define RADDR_SDR       0x02U
#define RADDR_DDR       0x22U
#define CADDR_SDR       0x03U
#define CADDR_DDR       0x23U
#define MODE1_SDR       0x04U
#define MODE1_DDR       0x24U
#define MODE2_SDR       0x05U
#define MODE2_DDR       0x25U
#define MODE4_SDR       0x06U
#define MODE4_DDR       0x26U
#define MODE8_SDR       0x07U
#define MODE8_DDR       0x27U
#define WRITE_SDR       0x08U
#define WRITE_DDR       0x28U
#define READ_SDR        0x09U
#define READ_DDR        0x29U
#define LEARN_SDR       0x0AU
#define LEARN_DDR       0x2AU
#define DATSZ_SDR       0x0BU
#define DATSZ_DDR       0x2BU
#define DUMMY_SDR       0x0CU
#define DUMMY_DDR       0x2CU
#define DUMMY_RWDS_SDR  0x0DU
#define DUMMY_RWDS_DDR  0x2DU
#define JMP_ON_CS       0x1FU
#define STOP            0U

#define FSPI_1PAD    0U
#define FSPI_2PAD    1U
#define FSPI_4PAD    2U
#define FSPI_8PAD    3U

#define NAND_LUT_IDX_READCACHE		0
#define NAND_LUT_IDX_READSTATUS		1
#define NAND_LUT_IDX_WRITEENABLE	3
#define NAND_LUT_IDX_READCACHE_ODD	4
#define NAND_LUT_IDX_ERASEBLOCK		5
#define NAND_LUT_IDX_RESET		6
#define NAND_LUT_IDX_PROGRAMLOAD	9
#define NAND_LUT_IDX_PROGRAMLOAD_ODD	10
#define NAND_LUT_IDX_READPAGE		11
#define NAND_LUT_IDX_READECCSTATUS	13
#define NAND_LUT_IDX_PROGRAMEXEC	14
#define NAND_LUT_IDX_READID		15

struct lut_seq {
	u8 seqNum;
	u8 seqId;
	u16 reserved;
};

struct fspi_mem_config {
	u32 tag;
	u32 version;
	u16 reserved;
	u8  reserved0[2];
	u8  readSampleClkSrc;
	u8  dataHoldTime;
	u8  dataSetupTime;
	u8  columnAddressWidth;
	u8  deviceModeCfgEnable;
	u8  reserved1[3];
	struct lut_seq deviceModeSeq;
	u32 deviceModeArg;
	u8  configCmdEnable;
	u8  reserved2[3];
	struct lut_seq configCmdSeqs[4];
	u32 configCmdArgs[4];
	u32 controllerMiscOption;
	u8  deviceType;
	u8  sflashPadType;
	u8  serialClkFreq;
	u8  lutCustomSeqEnable;
	u32 reserved3[2];
	u32 sflashA1Size;
	u32 sflashA2Size;
	u32 sflashB1Size;
	u32 sflashB2Size;
	u32 csPadSettingOverride;
	u32 sclkPadSettingOverride;
	u32 dataPadSettingOverride;
	u32 dqsPadSettingOverride;
	u32 timeoutInMs;
	u32 commandInterval;
	u16 dataValidTime[2];
	u16 busyOffset;
	u16 busyBitPolarity;
	u32 lookupTable[64];
	struct lut_seq lutCustomSeq[12];
	u32 reserved4[4];
};

struct fspi_nand_config {
	struct fspi_mem_config mem_config;
	u32 page_data_size;
	u32 page_total_size;
	u32 page_per_block;
	u8 bypass_read_status;
	u8 bypass_ecc_read;
	u8 has_multi_plane;
	u8 skip_odd_blocks;
	u8 ecc_check_custom;
	u8 ip_cmd_ser_clk_freq;
	u16 read_page_time_us;
	u32 ecc_status_mask;
	u32 ecc_failure_mask;
	u32 block_per_device;
	u32 reserved[8];
};

struct firmware_info {
	u32 start_page;
	u32 pages_in_firmware;
};

struct fspi_nand_fcb {
	u32 crc_checksum;
	u32 fingerprint;
	u32 version;
	u32 DBBT_search_start_page;
	u16 search_stride;
	u16 search_count;
	u32 firmware_copies;
	u32 reserved0[10];
	struct firmware_info firmware_table[FIRMWARE_MAX_NUM];
	u32 reserved1[32];
	struct fspi_nand_config config;
	u32 reserved2[64];
};

struct fspi_nand_dbbt {
	u32 crc_checksum;
	u32 fingerprint;
	u32 version;
	u32 reserved0;
	u32 bad_block_number;
	u32 reserved1[3];
	u32 bad_block_table[SERIAL_NAND_BAD_BLOCK_MAX_NUM];
};

struct fspi_nand {
	struct fspi_nand_fcb fcb;
	struct fspi_nand_dbbt dbbt;
	struct fspi_nand_dbbt dbbt_mirror;
	struct mtd_info *mtd;
};

#define FSPI_NAND_WRITE_MAX_RETRIES	3

static struct mtd_info *get_mtd_by_name(const char *name)
{
	struct mtd_info *mtd;

	mtd_probe_devices();

	mtd = get_mtd_device_nm(name);
	if (IS_ERR_OR_NULL(mtd))
		printf("MTD device %s not found, ret %ld\n", name,
		       PTR_ERR(mtd));

	return mtd;
}

static void fspinand_prep_dft_config(struct fspi_nand *f)
{
	struct fspi_nand_config *config = &f->fcb.config;
	struct mtd_info *mtd = f->mtd;

	config->mem_config.tag = FSPI_CFG_BLK_TAG;
	config->mem_config.version = FSPI_CFG_BLK_VERSION;
	config->mem_config.sflashA1Size = mtd->size;
	config->mem_config.serialClkFreq = 1;
	config->mem_config.sflashPadType = 1;
	config->mem_config.dataHoldTime = 3;
	config->mem_config.dataSetupTime = 3;
	config->mem_config.columnAddressWidth = mtd->writesize_shift + 1;
	config->mem_config.deviceType = 2; //spi-nand device
	config->mem_config.commandInterval = 100;

	config->page_data_size = mtd->writesize;
	config->page_total_size = 1 << config->mem_config.columnAddressWidth;
	config->page_per_block = mtd->erasesize / mtd->writesize;
	config->has_multi_plane = 0;
	config->block_per_device = mtd_div_by_eb(mtd->size, mtd);

	/* Read Status */
	config->mem_config.lookupTable[4 * NAND_LUT_IDX_READSTATUS + 0] =
		FSPI_LUT_SEQ(CMD_SDR, FSPI_1PAD, 0x0F, CMD_SDR, FSPI_1PAD, 0xC0);
	config->mem_config.lookupTable[4 * NAND_LUT_IDX_READSTATUS + 1] =
		FSPI_LUT_SEQ(READ_SDR, FSPI_1PAD, 0x01, STOP, FSPI_1PAD, 0x00);

	/* Read Page */
	config->mem_config.lookupTable[4 * NAND_LUT_IDX_READPAGE + 0] =
		FSPI_LUT_SEQ(CMD_SDR, FSPI_1PAD, 0x13, RADDR_SDR, FSPI_1PAD, 0x18);

	/* Read Cache */
	config->mem_config.lookupTable[4 * NAND_LUT_IDX_READCACHE + 0] =
		FSPI_LUT_SEQ(CMD_SDR, FSPI_1PAD, 0x03, CADDR_SDR, FSPI_1PAD, 0x10);
	config->mem_config.lookupTable[4 * NAND_LUT_IDX_READCACHE + 1] =
		FSPI_LUT_SEQ(DUMMY_SDR, FSPI_1PAD, 0x08, READ_SDR, FSPI_1PAD, 0x80);

	/* Read Cache Odd */
	config->mem_config.lookupTable[4 * NAND_LUT_IDX_READCACHE_ODD + 0] =
		FSPI_LUT_SEQ(CMD_SDR, FSPI_1PAD, 0x03, CADDR_SDR, FSPI_1PAD, 0x10);
	config->mem_config.lookupTable[4 * NAND_LUT_IDX_READCACHE_ODD + 1] =
		FSPI_LUT_SEQ(DUMMY_SDR, FSPI_1PAD, 0x08, READ_SDR, FSPI_1PAD, 0x80);

	/* Write Enable */
	config->mem_config.lookupTable[4 * NAND_LUT_IDX_WRITEENABLE + 0] =
		FSPI_LUT_SEQ(CMD_SDR, FSPI_1PAD, 0x06, STOP, FSPI_1PAD, 0x00);

	/* Page Program Load */
	config->mem_config.lookupTable[4 * NAND_LUT_IDX_PROGRAMLOAD + 0] =
		FSPI_LUT_SEQ(CMD_SDR, FSPI_1PAD, 0x02, CADDR_SDR, FSPI_1PAD, 0x10);
	config->mem_config.lookupTable[4 * NAND_LUT_IDX_PROGRAMLOAD + 1] =
		FSPI_LUT_SEQ(WRITE_SDR, FSPI_1PAD, 0x40, STOP, FSPI_1PAD, 0);

	/* Page Program Load Odd */
	config->mem_config.lookupTable[4 * NAND_LUT_IDX_PROGRAMLOAD_ODD + 0] =
		FSPI_LUT_SEQ(CMD_SDR, FSPI_1PAD, 0x02, CADDR_SDR, FSPI_1PAD, 0x10);
	config->mem_config.lookupTable[4 * NAND_LUT_IDX_PROGRAMLOAD_ODD + 1] =
		FSPI_LUT_SEQ(WRITE_SDR, FSPI_1PAD, 0x40, STOP, FSPI_1PAD, 0);

	/* Page Program Execute */
	config->mem_config.lookupTable[4 * NAND_LUT_IDX_PROGRAMEXEC + 0] =
		FSPI_LUT_SEQ(CMD_SDR, FSPI_1PAD, 0x10, RADDR_SDR, FSPI_1PAD, 0x18);

	/* Erase Block */
	config->mem_config.lookupTable[4 * NAND_LUT_IDX_ERASEBLOCK + 0] =
		FSPI_LUT_SEQ(CMD_SDR, FSPI_1PAD, 0xD8, RADDR_SDR, FSPI_1PAD, 0x18);

	/* Read Ecc Status */
	config->mem_config.lookupTable[4 * NAND_LUT_IDX_READECCSTATUS + 0] =
		FSPI_LUT_SEQ(CMD_SDR, FSPI_1PAD, 0x0F, CMD_SDR, FSPI_1PAD, 0xC0);
	config->mem_config.lookupTable[4 * NAND_LUT_IDX_READECCSTATUS + 1] =
		FSPI_LUT_SEQ(READ_SDR, FSPI_1PAD, 0x01, STOP, FSPI_1PAD, 0x00);
};

/*
 * Default memory configuration for SPI NAND.
 *
 * The key purpose is to enable on-die ECC by setting the ECC_EN bit in the
 * device status/configuration register via two SET FEATURE commands run by
 * the FlexSPI ROM before any read or program operation:
 *
 *   configCmd[0]: SET FEATURE 0xA0 = 0x00  -- clear all block-lock bits
 *   configCmd[1]: SET FEATURE 0xB0 = 0x10  -- set ECC_EN (bit 4)
 *
 * Most SPI NAND vendors follow the same JEDEC SET FEATURE opcode (0x1F) and
 * use the same register layout, so this config works for the majority of
 * devices.  If your chip uses a different register address, a different bit
 * position for ECC_EN, or requires additional initialization steps, this
 * default config will not be compatible -- use a customized header image
 * instead (see FSPI_NAND_HDR_SLOTS layout).
 */
static void fspinand_prep_dft_mem_config(struct fspi_nand *f)
{
	struct fspi_nand_config *config = &f->fcb.config;

	config->mem_config.configCmdEnable = true;

	config->mem_config.configCmdSeqs[0].seqId = 2;
	config->mem_config.configCmdSeqs[0].seqNum = 1;
	config->mem_config.lookupTable[4 * 2 + 0] = 0x04A0041F;
	config->mem_config.lookupTable[4 * 2 + 1] = 0x00002001;
	config->mem_config.configCmdArgs[0] = 0;

	config->mem_config.configCmdSeqs[1].seqId = 6;
	config->mem_config.configCmdSeqs[1].seqNum = 1;
	config->mem_config.lookupTable[4 * 6 + 0] = 0x04B0041F;
	config->mem_config.lookupTable[4 * 6 + 1] = 0x00002001;
	config->mem_config.configCmdArgs[1] = 0x10;

	config->mem_config.sflashPadType = 1;
};

static void fspinand_prep_fcb(struct fspi_nand *f, u32 firmware_size)
{
	struct fspi_nand_fcb *fcb = &f->fcb;
	struct mtd_info *mtd = f->mtd;
	u32 i;
	u32 *ptr;

	firmware_size = round_up(firmware_size, mtd->writesize);

	fcb->crc_checksum = 0;
	fcb->fingerprint = FSPI_NAND_FCB_FINGERPRINT;
	fcb->version = FSPI_NAND_FCB_VERSION;
	fcb->DBBT_search_start_page = 0x80;
	fcb->search_stride = 0x40;
	fcb->search_count = 1;
	fcb->firmware_copies = 1;
	fcb->firmware_table[0].start_page = 0x100;
	fcb->firmware_table[0].pages_in_firmware =
		mtd_div_by_ws(firmware_size, mtd);

	ptr = (u32 *)fcb;
	for (i = 0; i < sizeof(struct fspi_nand_fcb) / 4 - 1; i++)
		fcb->crc_checksum += ptr[i];
};

static int fspinand_prep_dbbt(struct fspi_nand *f)
{
	struct mtd_info *mtd = f->mtd;
	struct fspi_nand_dbbt *dbbt = &f->dbbt;
	int ret = 0;
	u32 i;
	u64 off;
	u32 toal_blk_num = mtd_div_by_eb(mtd->size, mtd);
	u32 max_num = min(toal_blk_num, SERIAL_NAND_BAD_BLOCK_MAX_NUM);

	dbbt->crc_checksum = 0;
	dbbt->fingerprint = FSPI_NAND_DBBT_FINGERPRINT;
	dbbt->version = FSPI_NAND_DBBT_VERSION;

	for (i = 0; i < max_num; i++) {
		off = i * (u64)mtd->erasesize;
		if (mtd_block_isbad(mtd, off)) {
			dbbt->bad_block_table[dbbt->bad_block_number] = i;
			dbbt->bad_block_number++;
		}
	}

	memcpy(&f->dbbt_mirror, dbbt, sizeof(struct fspi_nand_dbbt));
	return ret;
};

static bool mtd_is_aligned_with_min_io_size(struct mtd_info *mtd, u64 size)
{
	return !do_div(size, mtd->writesize);
}

static bool mtd_is_aligned_with_block_size(struct mtd_info *mtd, u64 size)
{
	return !do_div(size, mtd->erasesize);
}

static int fspinand_readback_check(struct fspi_nand *f, u64 offset, u64 len,
				   void *data)
{
	struct mtd_info *mtd = f->mtd;
	struct mtd_oob_ops io_op;
	int ret = 0;
	loff_t off, remaining;
	u8 *buf;
	bool need_rewrite = false;

	printf("Checking data at offset 0x%llx, len 0x%llx\n", offset, len);

	buf = kmalloc(mtd->writesize, GFP_KERNEL);

	off = offset;
	remaining = len;

	io_op.mode = MTD_OPS_AUTO_OOB;
	io_op.len = mtd->writesize;
	io_op.ooblen = 0;
	io_op.datbuf = buf;
	io_op.oobbuf = NULL;

	/* Read data back, check if block is good */
	while (remaining > 0) {
		/* Skip the block if it is bad */
		if (mtd_is_aligned_with_block_size(mtd, off) &&
		    mtd_block_isbad(mtd, off)) {
			off += mtd->erasesize;
			continue;
		}

		ret = mtd_read_oob(mtd, off, &io_op);

		if (ret) {
			printf("Fail to read at offset 0x%llx\n", off);
				need_rewrite = true;
		} else {
			if (memcmp(buf, data, io_op.retlen)) {
				printf("Data mismatch at offset 0x%llx\n",
				       off);
				need_rewrite = true;
			}
		}

		data += io_op.retlen;
		off += io_op.retlen;
		remaining -= io_op.retlen;
	}

	kfree(buf);
	return need_rewrite;
};

static int fspinand_prog_data(struct fspi_nand *f, void *data,
			      u64 offset, u64 len)
{
	struct mtd_info *mtd = f->mtd;
	int ret = 0;
	struct erase_info erase_op;
	struct mtd_oob_ops io_op;
	u64 erase_len, prog_len;
	u64 off, remaining;
	u64 chk_off;
	int retries = FSPI_NAND_WRITE_MAX_RETRIES;

	printf("Programming data at offset 0x%llx, len 0x%llx\n", offset, len);

	if (!mtd_is_aligned_with_block_size(mtd, len))
		erase_len = round_up(len, mtd->erasesize);
	else
		erase_len = len;

	erase_op.mtd = mtd;
	erase_op.addr = offset;
	erase_op.len = mtd->erasesize;

	while (erase_len) {
		ret = mtd_block_isbad(mtd, erase_op.addr);
		if (ret < 0) {
			printf("Failed to get bad block at 0x%08llx\n",
			       erase_op.addr);
			ret = CMD_RET_FAILURE;
			goto error;
		}

		if (ret > 0) {
			printf("Skipping bad block at 0x%08llx\n",
			       erase_op.addr);
			ret = 0;
			erase_op.addr += mtd->erasesize;
			continue;
		}

		printf("Erasing 0x%08llx ... 0x%08llx\n",
		       erase_op.addr, erase_op.addr + erase_op.len - 1);
		ret = mtd_erase(mtd, &erase_op);
		if (ret && ret != -EIO)
			break;

		erase_len -= mtd->erasesize;
		erase_op.addr += mtd->erasesize;
	}

	if (ret && ret != -EIO) {
		ret = CMD_RET_FAILURE;
		goto error;
	}

WRITE_RETRY:
	if (!mtd_is_aligned_with_min_io_size(mtd, len))
		prog_len = round_up(len, mtd->writesize);
	else
		prog_len = len;

	io_op.mode = MTD_OPS_AUTO_OOB;
	io_op.len = mtd->writesize;
	io_op.ooblen = 0;
	io_op.datbuf = data;
	io_op.oobbuf = NULL;

	remaining = prog_len;
	/* Search for the first good block after the given offset */
	off = offset;
	while (mtd_block_isbad(mtd, off))
		off += mtd->erasesize;
	chk_off = off;

	/* Loop over the pages to do the actual read/write */
	while (remaining) {
		/* Skip the block if it is bad */
		if (mtd_is_aligned_with_block_size(mtd, off) &&
		    mtd_block_isbad(mtd, off)) {
			off += mtd->erasesize;
			continue;
		}

		ret = mtd_write_oob(mtd, off, &io_op);

		debug("Writing 0x%08llx ... 0x%08llx (%d pages)\n",
		      off, off + io_op.retlen - 1,
		      mtd_div_by_ws(io_op.retlen, mtd));

		if (ret) {
			printf("Failure to writing at offset 0x%llx\n",
			       off);
			break;
		}

		off += io_op.retlen;
		remaining -= io_op.retlen;
		io_op.datbuf += io_op.retlen;
	}

	/* Read data back and check */
	ret = fspinand_readback_check(f, chk_off, prog_len, data);
	if (ret && retries--)
		goto WRITE_RETRY;

	if (ret)
		ret = CMD_RET_FAILURE;
	else
		ret = CMD_RET_SUCCESS;
error:
	return ret;
}

static int fspinand_prog_fcb(struct fspi_nand *f)
{
	struct fspi_nand_fcb *fcb = &f->fcb;
	struct mtd_info *mtd = f->mtd;
	int ret = 0;

	ret = fspinand_prog_data(f, fcb, 0, sizeof(struct fspi_nand_fcb));
	if (ret) {
		printf("fspinand prog FCB0 fail\n");
		return CMD_RET_FAILURE;
	} else {
		printf("fspinand prog FCB0 success\n");
	}

	ret = fspinand_prog_data(f, fcb, (u64)fcb->search_stride * mtd->writesize,
				 sizeof(struct fspi_nand_fcb));
	if (ret) {
		printf("fspinand prog FCB1 fail\n");
		return CMD_RET_FAILURE;
	} else {
		printf("fspinand prog FCB1 success\n");
	}

	return CMD_RET_SUCCESS;
}

static int fspinand_prog_dbbt(struct fspi_nand *f)
{
	struct fspi_nand_fcb *fcb = &f->fcb;
	struct mtd_info *mtd = f->mtd;
	struct fspi_nand_dbbt *dbbt = &f->dbbt;
	int ret = 0;
	u64 off;

	off = (u64)fcb->DBBT_search_start_page * mtd->writesize;
	ret = fspinand_prog_data(f, dbbt, off, sizeof(struct fspi_nand_dbbt));
	if (ret) {
		printf("fspinand prog DBBT0 fail\n");
		return CMD_RET_FAILURE;
	} else {
		printf("fspinand prog DBBT0 success\n");
	}

	off += (u64)fcb->search_stride * mtd->writesize;
	ret = fspinand_prog_data(f, dbbt, off, sizeof(struct fspi_nand_dbbt));
	if (ret) {
		printf("fspinand prog DBBT1 fail\n");
		return CMD_RET_FAILURE;
	} else {
		printf("fspinand prog DBBT1 success\n");
	}

	return CMD_RET_SUCCESS;
}

static int fspinand_prog_firmware(struct fspi_nand *f, void *firmware)
{
	struct fspi_nand_fcb *fcb = &f->fcb;
	struct mtd_info *mtd = f->mtd;
	int ret = 0;
	u64 off;
	u64 size;

	off = (u64)fcb->firmware_table[0].start_page * mtd->writesize;
	size = (u64)fcb->firmware_table[0].pages_in_firmware * mtd->writesize;
	ret = fspinand_prog_data(f, firmware, off, size);
	if (ret) {
		printf("fspinand prog FIRMWARE0 fail\n");
		return CMD_RET_FAILURE;
	} else {
		printf("fspinand prog FIRMWARE0 success\n");
	}

	off = 4 * 1024 * 1024;
	ret = fspinand_prog_data(f, firmware, off, size);
	if (ret) {
		printf("fspinand prog FIRMWARE1 fail\n");
		return CMD_RET_FAILURE;
	} else {
		printf("fspinand prog FIRMWARE1 success\n");
	}

	return CMD_RET_SUCCESS;
}

/*
 * Image-with-header memory layout
 * --------------------------------
 * When a boot image is packaged with a fspi_nand_config header the file
 * reserves space for 4 header slots before the actual firmware binary.
 * Only slot 0 carries the active config; slots 1-3 are reserved/padding.
 *
 *   offset 0x000 (   0 B): fspi_nand_config slot 0 -- active FCFB config (512 B)
 *   offset 0x200 ( 512 B): fspi_nand_config slot 1 -- reserved (512 B)
 *   offset 0x400 (1024 B): fspi_nand_config slot 2 -- reserved (512 B)
 *   offset 0x600 (1536 B): fspi_nand_config slot 3 -- reserved (512 B)
 *   offset 0x800 (2048 B): firmware (boot image)
 *
 * Total header area = FSPI_NAND_HDR_SLOTS * sizeof(struct fspi_nand_config)
 *                   = 4 * 512 = 2048 bytes
 */
#define FSPI_NAND_HDR_SLOTS		4
#define FSPI_NAND_HDR_AREA_SIZE		(FSPI_NAND_HDR_SLOTS * sizeof(struct fspi_nand_config))

/*
 * fspinand_image_has_config - check if the image at addr starts with a
 * fspi_nand_config header by looking for the FCFB tag at offset 0.
 *
 * Returns 0 when the tag is found (condition 2), 1 when not found
 * (condition 1, plain boot image), or negative on mapping error.
 */
static int fspinand_image_has_config(unsigned long addr, int verbose)
{
	u32 *tag;
	int ret;

	tag = map_physmem(addr, sizeof(u32), MAP_WRBACK);
	if (!tag) {
		printf("Failed to map physical memory\n");
		return -1;
	}

	if (*tag == FSPI_CFG_BLK_TAG) {
		if (verbose)
			printf("Found fspi_nand_config header in image\n");
		ret = 0;
	} else {
		if (verbose)
			printf("No fspi_nand_config header in image\n");
		ret = 1;
	}

	unmap_physmem(tag, sizeof(u32));
	return ret;
}

static int do_fspinand_init(int argc, char *const argv[])
{
	struct fspi_nand fspinand;
	struct fspi_nand *f = &fspinand;
	unsigned long addr;
	u32 firmware_size;
	void *img;

	if (argc < 5)
		return CMD_RET_USAGE;

	printf("fspinand init %s\n", argv[2]);

	memset(f, 0, sizeof(struct fspi_nand));
	f->mtd = get_mtd_by_name(argv[2]);
	if (IS_ERR_OR_NULL(f->mtd)) {
		printf("fspinand init fail\n");
		return CMD_RET_FAILURE;
	}

	addr = simple_strtoul(argv[3], NULL, 16);
	firmware_size = simple_strtoul(argv[4], NULL, 16);

	if (fspinand_image_has_config(addr, 1) == 0) {
		/*
		 * Condition 1: image starts with a fspi_nand_config header.
		 * Extract the config, copy it into the FCB, then point the
		 * firmware pointer past the header.  The real boot image
		 * follows immediately after FSPI_NAND_HDR_AREA.
		 */
		img = map_physmem(addr, sizeof(struct fspi_nand_config),
				  MAP_WRBACK);
		if (!img) {
			printf("fspinand init fail: cannot map image\n");
			return CMD_RET_FAILURE;
		}
		memcpy(&f->fcb.config, img, sizeof(struct fspi_nand_config));
		unmap_physmem(img, sizeof(struct fspi_nand_config));

		/*
		 * Firmware data starts after the full header area (4 slots).
		 * See FSPI_NAND_HDR_AREA_SIZE layout comment above.
		 */
		addr += FSPI_NAND_HDR_AREA_SIZE;
		firmware_size -= FSPI_NAND_HDR_AREA_SIZE;

		fspinand_prep_fcb(f, firmware_size);
	} else {
		/*
		 * Condition 2: plain boot image with no config header.
		 * Generate the default config, mem config and FCB as before.
		 */
		fspinand_prep_dft_config(f);
		fspinand_prep_dft_mem_config(f);
		fspinand_prep_fcb(f, firmware_size);
	}

	fspinand_prep_dbbt(f);
	fspinand_prog_fcb(f);
	fspinand_prog_dbbt(f);
	fspinand_prog_firmware(f, (void *)addr);

	printf("fspinand init succeed\n");

	return 0;
}

static int do_fspinand_mark_bad(int argc, char * const argv[])
{
	struct mtd_info *mtd;
	ulong addr;

	mtd = get_mtd_by_name(argv[2]);
	if (IS_ERR_OR_NULL(mtd)) {
		printf("fspinand init fail\n");
		return CMD_RET_FAILURE;
	}

	argc -= 3;
	argv += 3;

	if (argc <= 0)
		return CMD_RET_FAILURE;

	while (argc > 0) {
		addr = hextoul(*argv, NULL);

		if (mtd_block_markbad(mtd, addr)) {
			printf("block 0x%08lx NOT marked as bad! ERROR\n",
			       addr);
		} else {
			printf("block 0x%08lx successfully marked as bad\n",
			       addr);
		}
		--argc;
		++argv;
	}
	return 0;
}

static int do_fspinand(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	char *cmd;

	cmd = argv[1];

	if (strcmp(cmd, "check") == 0) {
		if (argc < 3)
			goto usage;
		return fspinand_image_has_config(simple_strtoul(argv[2], NULL, 16), 1);
	}

	if (strcmp(cmd, "init") == 0) {
		if (argc < 5)
			goto usage;
		return do_fspinand_init(argc, argv);
	}

	if (strcmp(cmd, "mark_bad") == 0) {
		if (argc < 3)
			goto usage;
		return do_fspinand_mark_bad(argc, argv);
	}

	return 0;
usage:
	return CMD_RET_USAGE;
}

static char fspinand_help_text[] =
	"check addr - check if image at memory addr has a fspi_nand_config header\n"
	"		0 - config header found, 1 - plain boot image\n"
	"fspinand init name addr len - burn data to FSPI NAND with FCB/DBBT\n"
	"		if image starts with fspi_nand_config header (FCFB tag), it\n"
	"		is extracted and used; the boot image follows the header.\n"
	"		if no header, default config is generated (original behavior)\n"
	"fspinand mark_bad name addr - mark the addr located block as bad\n";

U_BOOT_CMD(fspinand, 5, 1, do_fspinand,
	   "FSPI NAND Boot Control Blocks(BCB) sub-system",
	   fspinand_help_text
);
