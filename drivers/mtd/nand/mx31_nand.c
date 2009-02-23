/*
 * Copyright 2004-2009 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <common.h>
#include <nand.h>
#include <asm-arm/arch/mx31-regs.h>

/*
 * Define delays in microsec for NAND device operations
 */
#define TROP_US_DELAY   2000

/*
 * Macros to get byte and bit positions of ECC
 */
#define COLPOS(x) ((x) >> 4)
#define BITPOS(x) ((x) & 0xf)

/* Define single bit Error positions in Main & Spare area */
#define MAIN_SINGLEBIT_ERROR 0x4
#define SPARE_SINGLEBIT_ERROR 0x1

struct nand_info {
	int oob;
	int read_status;
	int largepage;
	u16 col;
};

static struct nand_info nandinfo;
static int ecc_disabled;

/*
 * OOB placement block for use with hardware ecc generation
 */
static struct nand_ecclayout nand_hw_eccoob_8 = {
	.eccbytes = 5,
	.eccpos = {6, 7, 8, 9, 10},
	.oobfree = {
		    {0, 5},
		    {11, 5}
		    }
};

static struct nand_ecclayout nand_hw_eccoob_2k = {
	.eccbytes = 20,
	.eccpos = {6, 7, 8, 9, 10, 22, 23, 24, 25, 26,
		   38, 39, 40, 41, 42, 54, 55, 56, 57, 58},
	.oobfree = {
		    {0, 5},
		    {11, 10},
		    {27, 10},
		    {43, 10},
		    {59, 5}
		    }
};

/* Define some generic bad / good block scan pattern which are used
 * while scanning a device for factory marked good / bad blocks. */
static uint8_t scan_ff_pattern[] = { 0xff, 0xff };

static struct nand_bbt_descr smallpage_memorybased = {
	.options = NAND_BBT_SCAN2NDPAGE,
	.offs = 5,
	.len = 1,
	.pattern = scan_ff_pattern
};

static struct nand_bbt_descr largepage_memorybased = {
	.options = 0,
	.offs = 0,
	.len = 2,
	.pattern = scan_ff_pattern
};

/* Generic flash bbt decriptors */
static uint8_t bbt_pattern[] = { 'B', 'b', 't', '0' };
static uint8_t mirror_pattern[] = { '1', 't', 'b', 'B' };

static struct nand_bbt_descr bbt_main_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
	    | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 0,
	.len = 4,
	.veroffs = 4,
	.maxblocks = 4,
	.pattern = bbt_pattern
};

static struct nand_bbt_descr bbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
	    | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 0,
	.len = 4,
	.veroffs = 4,
	.maxblocks = 4,
	.pattern = mirror_pattern
};

/**
 * memcpy variant that copies 32 bit words. This is needed since the
 * NFC only allows 32 bit accesses. Added for U-boot.
 */
static void *memcpy_32(void *dest, const void *src, size_t n)
{
	u32 *dst_32 = (u32 *) dest;
	const u32 *src_32 = (u32 *) src;

	while (n > 0) {
		*dst_32++ = *src_32++;
		n -= 4;
	}

	return dest;
}

/**
 * This function polls the NANDFC to wait for the basic operation to
 * complete by checking the INT bit of config2 register.
 *
 * @param       max_retries    number of retry attempts (separated by 1 us)
 */
static void wait_op_done(int max_retries)
{
	while (max_retries-- > 0) {
		if (NFC_CONFIG2 & NFC_INT) {
			NFC_CONFIG2 &= ~NFC_INT;
			break;
		}
		udelay(1);
	}
	if (max_retries <= 0)
		MTDDEBUG(MTD_DEBUG_LEVEL0, "wait: INT not set\n");
}

/**
 * This function issues the specified command to the NAND device and
 * waits for completion.
 *
 * @param       cmd     command for NAND Flash
 */
static void send_cmd(u16 cmd)
{
	MTDDEBUG(MTD_DEBUG_LEVEL3, "send_cmd(0x%x)\n", cmd);

	NFC_FLASH_CMD = (u16) cmd;
	NFC_CONFIG2 = NFC_CMD;

	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY);
}

/**
 * This function sends an address (or partial address) to the
 * NAND device.  The address is used to select the source/destination for
 * a NAND command.
 *
 * @param       addr    address to be written to NFC.
 * @param       islast  1 if this is the last address cycle for command
 */
static void send_addr(u16 addr)
{
	MTDDEBUG(MTD_DEBUG_LEVEL3, "send_addr(0x%x %d)\n", addr);

	NFC_FLASH_ADDR = addr;
	NFC_CONFIG2 = NFC_ADDR;

	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY);
}

/**
 * This function requests the NANDFC to initate the transfer
 * of data currently in the NANDFC RAM buffer to the NAND device.
 *
 * @param	buf_id	      Specify Internal RAM Buffer number (0-3)
 * @param       oob    set to 1 if only the spare area is transferred
 */
static void send_prog_page(u8 buf_id)
{
	MTDDEBUG(MTD_DEBUG_LEVEL3, "send_prog_page (%d)\n", nandinfo.oob);

	/* NANDFC buffer 0 is used for page read/write */

	NFC_BUF_ADDR = buf_id;

	/* Configure spare or page+spare access */
	if (!nandinfo.largepage) {
		if (nandinfo.oob)
			NFC_CONFIG1 |= NFC_SP_EN;
		else
			NFC_CONFIG1 &= ~NFC_SP_EN;
	}
	NFC_CONFIG2 = NFC_INPUT;

	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY);
}

/**
 * This function will correct the single bit ECC error
 *
 * @param  buf_id	Specify Internal RAM Buffer number (0-3)
 * @param  eccpos 	Ecc byte and bit position
 * @param  oob  	set to 1 if only spare area needs correction
 */
static void mxc_nd_correct_error(u8 buf_id, u16 eccpos, int oob)
{
	u16 col;
	u8 pos;
	u16 *buf;

	/* Get col & bit position of error
	   these macros works for both 8 & 16 bits */
	col = COLPOS(eccpos);	/* Get half-word position */
	pos = BITPOS(eccpos);	/* Get bit position */

	MTDDEBUG(MTD_DEBUG_LEVEL3,
	      "mxc_nd_correct_error (col=%d pos=%d)\n", col, pos);

	/* Set the pointer for main / spare area */
	if (!oob)
		buf = (u16 *)(MAIN_AREA0 + col + (256 * buf_id));
	else
		buf = (u16 *)(SPARE_AREA0 + col + (8 * buf_id));

	/* Fix the data */
	*buf ^= 1 << pos;
}

/**
 * This function will maintains state of single bit Error
 * in Main & spare  area
 *
 * @param buf_id	Specify Internal RAM Buffer number (0-3)
 * @param spare  	set to 1 if only spare area needs correction
 */
static void mxc_nd_correct_ecc(u8 buf_id, int spare)
{
	u16 value, ecc_status;

	/* Read the ECC result */
	ecc_status = NFC_ECC_STATUS_RESULT;
	MTDDEBUG(MTD_DEBUG_LEVEL3,
	      "mxc_nd_correct_ecc (Ecc status=%x)\n", ecc_status);

	if (((ecc_status & 0xC) == MAIN_SINGLEBIT_ERROR)
	    || ((ecc_status & 0x3) == SPARE_SINGLEBIT_ERROR)) {
		if (ecc_disabled) {
			if ((ecc_status & 0xC) == MAIN_SINGLEBIT_ERROR) {
				value = NFC_RSLTMAIN_AREA;
				/* Correct single bit error in Mainarea
				   NFC will not correct the error in
				   current page */
				mxc_nd_correct_error(buf_id, value, 0);
			}
			if ((ecc_status & 0x3) == SPARE_SINGLEBIT_ERROR) {
				value = NFC_RSLTSPARE_AREA;
				/* Correct single bit error in Mainarea
				   NFC will not correct the error in
				   current page */
				mxc_nd_correct_error(buf_id, value, 1);
			}

		} else {
			/* Disable ECC  */
			NFC_CONFIG1 &= ~NFC_ECC_EN;
			ecc_disabled = 1;
		}
	} else if (ecc_status == 0) {
		if (ecc_disabled) {
			/* Enable ECC */
			NFC_CONFIG1 |= NFC_ECC_EN;
			ecc_disabled = 0;
		}
	}			/* else 2-bit Error. Do nothing */
}

/**
 * This function requests the NANDFC to initated the transfer
 * of data from the NAND device into in the NANDFC ram buffer.
 *
 * @param  	buf_id		Specify Internal RAM Buffer number (0-3)
 * @param       oob    	set 1 if only the spare area is
 * transferred
 */
static void send_read_page(u8 buf_id)
{
	MTDDEBUG(MTD_DEBUG_LEVEL3, "send_read_page (%d)\n", nandinfo.oob);

	/* NANDFC buffer 0 is used for page read/write */
	NFC_BUF_ADDR = buf_id;

	/* Configure spare or page+spare access */
	if (!nandinfo.largepage) {
		if (nandinfo.oob)
			NFC_CONFIG1 |= NFC_SP_EN;
		else
			NFC_CONFIG1 &= ~NFC_SP_EN;
	}

	NFC_CONFIG2 = NFC_OUTPUT;

	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY);

	/* If there are single bit errors in
	   two consecutive page reads then
	   the error is not  corrected by the
	   NFC for the second page.
	   Correct single bit error in driver */

	mxc_nd_correct_ecc(buf_id, nandinfo.oob);
}

/**
 * This function requests the NANDFC to perform a read of the
 * NAND device ID.
 */
static void send_read_id(void)
{
	/* NANDFC buffer 0 is used for device ID output */
	NFC_BUF_ADDR = 0x0;

	/* Read ID into main buffer */
	NFC_CONFIG1 &= ~NFC_SP_EN;
	NFC_CONFIG2 = NFC_ID;

	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY);
}

/**
 * This function requests the NANDFC to perform a read of the
 * NAND device status and returns the current status.
 *
 * @return  device status
 */
static u16 get_dev_status(void)
{
	volatile u16 *mainbuf = MAIN_AREA1;
	u32 store;
	u16 ret;
	/* Issue status request to NAND device */

	/* store the main area1 first word, later do recovery */
	store = *((u32 *) mainbuf);
	/*
	 * NANDFC buffer 1 is used for device status to prevent
	 * corruption of read/write buffer on status requests.
	 */
	NFC_BUF_ADDR = 1;

	/* Read status into main buffer */
	NFC_CONFIG1 &= ~NFC_SP_EN;
	NFC_CONFIG2 = NFC_STATUS;

	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY);

	/* Status is placed in first word of main buffer */
	/* get status, then recovery area 1 data */
	ret = mainbuf[0];
	*((u32 *) mainbuf) = store;

	return ret;
}

static void mxc_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	/*
	 * If HW ECC is enabled, we turn it on during init.  There is
	 * no need to enable again here.
	 */
}

static int mxc_nand_correct_data(struct mtd_info *mtd, unsigned char *dat,
				 unsigned char *read_ecc, unsigned char *calc_ecc)
{
	/*
	 * 1-Bit errors are automatically corrected in HW.  No need for
	 * additional correction.  2-Bit errors cannot be corrected by
	 * HW ECC, so we need to return failure
	 */
	u16 ecc_status = NFC_ECC_STATUS_RESULT;

	if (((ecc_status & 0x3) == 2) || ((ecc_status >> 2) == 2)) {
		MTDDEBUG(MTD_DEBUG_LEVEL0,
		      "MXC_NAND: HWECC uncorrectable 2-bit ECC error\n");
		return -1;
	}

	return 0;
}

static int mxc_nand_calculate_ecc(struct mtd_info *mtd, const unsigned char *dat,
				  unsigned char *ecc_code)
{
	/*
	 * Just return success.  HW ECC does not read/write the NFC spare
	 * buffer.  Only the FLASH spare area contains the calcuated ECC.
	 */
	return 0;
}

/**
 * This function reads byte from the NAND Flash
 *
 * @param       mtd     MTD structure for the NAND Flash
 *
 * @return    data read from the NAND Flash
 */
static unsigned char mxc_nand_read_byte(struct mtd_info *mtd)
{
	unsigned char ret_val = 0;
	u16 col, rd_word;
	volatile u16 *mainbuf = MAIN_AREA0;
	volatile u16 *sparebuf = SPARE_AREA0;

	/* Check for status request */
	if (nandinfo.read_status)
		return get_dev_status() & 0xFF;

	/* Get column for 16-bit access */
	col = nandinfo.col >> 1;

	/* If we are accessing the spare region */
	if (nandinfo.oob)
		rd_word = sparebuf[col];
	else
		rd_word = mainbuf[col];

	/* Pick upper/lower byte of word from RAM buffer */
	if (nandinfo.col & 0x1)
		ret_val = (rd_word >> 8) & 0xFF;
	else
		ret_val = rd_word & 0xFF;

	/* Update saved column address */
	nandinfo.col++;

	return ret_val;
}

/**
  * This function reads word from the NAND Flash
  *
  * @param       mtd     MTD structure for the NAND Flash
  *
  * @return    data read from the NAND Flash
  */
static u16 mxc_nand_read_word(struct mtd_info *mtd)
{
	u16 col;
	u16 rd_word, ret_val;
	volatile u16 *p;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "mxc_nand_read_word(col = %d)\n", nandinfo.col);

	col = nandinfo.col;
	/* Adjust saved column address */
	if (col < mtd->writesize && nandinfo.oob)
		col += mtd->writesize;

	if (col < mtd->writesize)
		p = (MAIN_AREA0) + (col >> 1);
	else
		p = (SPARE_AREA0) + ((col - mtd->writesize) >> 1);

	if (col & 1) {
		rd_word = *p;
		ret_val = (rd_word >> 8) & 0xff;
		rd_word = *(p + 1);
		ret_val |= (rd_word << 8) & 0xff00;

	} else
		ret_val = *p;

	/* Update saved column address */
	nandinfo.col = col + 2;

	return ret_val;
}

/**
 * This function writes data of length \b len to buffer \b buf. The data
 * to be written on NAND Flash is first copied to RAMbuffer. After the
 * Data Input Operation by the NFC, the data is written to NAND Flash.
 *
 * @param       mtd     MTD structure for the NAND Flash
 * @param       buf     data to be written to NAND Flash
 * @param       len     number of bytes to be written
 */
static void mxc_nand_write_buf(struct mtd_info *mtd,
			       const unsigned char *buf, int len)
{
	int n;
	int col;
	int i = 0;

	MTDDEBUG(MTD_DEBUG_LEVEL3,
	      "mxc_nand_write_buf(col = %d, len = %d)\n", nandinfo.col, len);

	col = nandinfo.col;

	/* Adjust saved column address */
	if (col < mtd->writesize && nandinfo.oob)
		col += mtd->writesize;

	n = mtd->writesize + mtd->oobsize - col;
	if (len > mtd->writesize + mtd->oobsize - col)
		MTDDEBUG(MTD_DEBUG_LEVEL1, "Error: too much data.\n");

	n = min(len, n);

	MTDDEBUG(MTD_DEBUG_LEVEL3,
	      "%s:%d: col = %d, n = %d\n", __FUNCTION__, __LINE__, col, n);

	while (n) {
		volatile u32 *p;
		if (col < mtd->writesize)
			p = (volatile u32 *)((ulong) (MAIN_AREA0) + (col & ~3));
		else
			p = (volatile u32 *)((ulong) (SPARE_AREA0) -
					     mtd->writesize + (col & ~3));

		MTDDEBUG(MTD_DEBUG_LEVEL3, "%s:%d: p = %p\n",
		      __FUNCTION__, __LINE__, p);

		if (((col | (int)&buf[i]) & 3) || n < 16) {
			u32 data = 0;

			if (col & 3 || n < 4)
				data = *p;

			switch (col & 3) {
			case 0:
				if (n) {
					data = (data & 0xffffff00) |
					    (buf[i++] << 0);
					n--;
					col++;
				}
			case 1:
				if (n) {
					data = (data & 0xffff00ff) |
					    (buf[i++] << 8);
					n--;
					col++;
				}
			case 2:
				if (n) {
					data = (data & 0xff00ffff) |
					    (buf[i++] << 16);
					n--;
					col++;
				}
			case 3:
				if (n) {
					data = (data & 0x00ffffff) |
					    (buf[i++] << 24);
					n--;
					col++;
				}
			}

			*p = data;
		} else {
			int m = mtd->writesize - col;

			if (col >= mtd->writesize)
				m += mtd->oobsize;

			m = min(n, m) & ~3;

			MTDDEBUG(MTD_DEBUG_LEVEL3,
			      "%s:%d: n = %d, m = %d, i = %d, col = %d\n",
			      __FUNCTION__, __LINE__, n, m, i, col);

			memcpy_32((void *)(p), &buf[i], m);
			col += m;
			i += m;
			n -= m;
		}
	}
	/* Update saved column address */
	nandinfo.col = col;
}

/**
 * This function id is used to read the data buffer from the NAND Flash. To
 * read the data from NAND Flash first the data output cycle is initiated by
 * the NFC, which copies the data to RAMbuffer. This data of length \b len is
 * then copied to buffer \b buf.
 *
 * @param       mtd     MTD structure for the NAND Flash
 * @param       buf     data to be read from NAND Flash
 * @param       len     number of bytes to be read
 */
static void mxc_nand_read_buf(struct mtd_info *mtd, unsigned char *buf, int len)
{
	int n;
	int col;
	int i = 0;

	MTDDEBUG(MTD_DEBUG_LEVEL3,
	      "mxc_nand_read_buf(col = %d, len = %d)\n", nandinfo.col, len);

	col = nandinfo.col;
	/**
	 * Adjust saved column address
	 * for nand_read_oob will pass col within oobsize
	 */
	if (col < mtd->writesize && nandinfo.oob)
		col += mtd->writesize;

	n = mtd->writesize + mtd->oobsize - col;
	n = min(len, n);

	while (n) {
		volatile u32 *p;

		if (col < mtd->writesize)
			p = (volatile u32 *)((ulong) (MAIN_AREA0) + (col & ~3));
		else
			p = (volatile u32 *)((ulong) (SPARE_AREA0) -
					     mtd->writesize + (col & ~3));

		if (((col | (int)&buf[i]) & 3) || n < 16) {
			u32 data;

			data = *p;
			switch (col & 3) {
			case 0:
				if (n) {
					buf[i++] = (u8) (data);
					n--;
					col++;
				}
			case 1:
				if (n) {
					buf[i++] = (u8) (data >> 8);
					n--;
					col++;
				}
			case 2:
				if (n) {
					buf[i++] = (u8) (data >> 16);
					n--;
					col++;
				}
			case 3:
				if (n) {
					buf[i++] = (u8) (data >> 24);
					n--;
					col++;
				}
			}
		} else {
			int m = mtd->writesize - col;

			if (col >= mtd->writesize)
				m += mtd->oobsize;

			m = min(n, m) & ~3;
			memcpy_32(&buf[i], (void *)(p), m);
			col += m;
			i += m;
			n -= m;
		}
	}
	/* Update saved column address */
	nandinfo.col = col;
}

/**
 * This function is used by the upper layer to verify the data in NAND Flash
 * with the data in the \b buf.
 *
 * @param       mtd     MTD structure for the NAND Flash
 * @param       buf     data to be verified
 * @param       len     length of the data to be verified
 *
 * @return      -EFAULT if error else 0
 */
static int
mxc_nand_verify_buf(struct mtd_info *mtd, const unsigned char *buf, int len)
{
	return -1;		/* Was -EFAULT */
}

/**
 * This function is used by upper layer for select and deselect of the NAND
 * chip.
 *
 * @param       mtd     MTD structure for the NAND Flash
 * @param       chip    val indicating select or deselect
 */
static void mxc_nand_select_chip(struct mtd_info *mtd, int chip)
{
}

/**
 * This function is used by the upper layer to write command to NAND Flash
 * for different operations to be carried out on NAND Flash
 *
 * @param       mtd             MTD structure for the NAND Flash
 * @param       command         command for NAND Flash
 * @param       column          column offset for the page read
 * @param       page_addr       page to be read from NAND Flash
 */
static void mxc_nand_command(struct mtd_info *mtd, unsigned command,
			     int column, int page_addr)
{
	MTDDEBUG(MTD_DEBUG_LEVEL3,
	      "mxc_nand_command (cmd = 0x%x, col = 0x%x, page = 0x%x)\n",
	      command, column, page_addr);

	/* Reset command state information */
	nandinfo.read_status = 0;
	nandinfo.oob = 0;

	/* Command pre-processing step */
	switch (command) {

	case NAND_CMD_STATUS:
		nandinfo.col = 0;
		nandinfo.read_status = 1;
		break;

	case NAND_CMD_READ0:
		nandinfo.col = column;
		break;

	case NAND_CMD_READOOB:
		nandinfo.col = column;
		nandinfo.oob = 1;
		if (nandinfo.largepage)
			command = NAND_CMD_READ0;
		break;

	case NAND_CMD_SEQIN:
		if (column >= mtd->writesize) {
			/* write oob routine caller */
			if (nandinfo.largepage) {
				/*
				 * FIXME: before send SEQIN command for
				 * write OOB, we must read one page out.
				 * For 2K nand has no READ1 command to set
				 * current HW pointer to spare area, we must
				 * write the whole page including OOB together.
				 */
				/* call itself to read a page */
				mxc_nand_command(mtd, NAND_CMD_READ0, 0,
						 page_addr);
			}
			nandinfo.col = column - mtd->writesize;
			nandinfo.oob = 1;
			/* Set program pointer to spare region */
			if (!nandinfo.largepage)
				send_cmd(NAND_CMD_READOOB);
		} else {
			nandinfo.oob = 0;
			nandinfo.col = column;
			/* Set program pointer to page start */
			if (!nandinfo.largepage)
				send_cmd(NAND_CMD_READ0);
		}
		break;

	case NAND_CMD_PAGEPROG:
		if (ecc_disabled) {
			/* Enable Ecc for page writes */
			NFC_CONFIG1 |= NFC_ECC_EN;
		}
		send_prog_page(0);

		if (nandinfo.largepage) {
			/* data in 4 areas datas */
			send_prog_page(1);
			send_prog_page(2);
			send_prog_page(3);
		}

		break;

	case NAND_CMD_ERASE1:
		break;
	}

	/*
	 * Write out the command to the device.
	 */
	send_cmd(command);

	/*
	 * Write out column address, if necessary
	 */
	if (column != -1) {
		/*
		 * MXC NANDFC can only perform full page+spare or
		 * spare-only read/write.  When the upper layers
		 * layers perform a read/write buf operation,
		 * we will used the saved column adress to index into
		 * the full page.
		 */
		send_addr(0);
		if (nandinfo.largepage)
			/* another col addr cycle for 2k page */
			send_addr(0);
	}

	/*
	 * Write out page address, if necessary
	 */
	if (page_addr != -1) {
		/* paddr_0 - p_addr_7 */
		send_addr((page_addr & 0xff));

		if (nandinfo.largepage) {
			/* One more address cycle for higher
			 * density devices */

			if (mtd->size >= 0x10000000) {
				/* paddr_8 - paddr_15 */
				send_addr((page_addr >> 8) & 0xff);
				send_addr((page_addr >> 16) & 0xff);
			} else
				/* paddr_8 - paddr_15 */
				send_addr((page_addr >> 8) & 0xff);
		} else {
			/* One more address cycle for higher
			 * density devices */

			if (mtd->size >= 0x4000000) {
				/* paddr_8 - paddr_15 */
				send_addr((page_addr >> 8) & 0xff);
				send_addr((page_addr >> 16) & 0xff);
			} else
				/* paddr_8 - paddr_15 */
				send_addr((page_addr >> 8) & 0xff);
		}
	}

	/*
	 * Command post-processing step
	 */
	switch (command) {

	case NAND_CMD_RESET:
		break;

	case NAND_CMD_READOOB:
	case NAND_CMD_READ0:
		if (nandinfo.largepage) {
			/* send read confirm command */
			send_cmd(NAND_CMD_READSTART);
			/* read for each AREA */
			send_read_page(0);
			send_read_page(1);
			send_read_page(2);
			send_read_page(3);
		} else
			send_read_page(0);
		break;

	case NAND_CMD_READID:
		send_read_id();
		nandinfo.col = column;
		break;

	case NAND_CMD_PAGEPROG:
		if (ecc_disabled) {
			/* Disable Ecc after page writes */
			NFC_CONFIG1 &= ~NFC_ECC_EN;
		}
		break;

	case NAND_CMD_ERASE2:
		break;
	}
}

static int mxc_nand_scan_bbt(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;

	/* Config before scanning */
	/* Do not rely on NFMS_BIT, set/clear NFMS bit based
	 * on mtd->writesize */
	if (mtd->writesize == 2048)
		NFMS |= 1 << NFMS_BIT;
	else if ((NFMS >> NFMS_BIT) & 0x1)
		NFMS &= ~(1 << NFMS_BIT);

	/* use flash based bbt */
	this->bbt_td = &bbt_main_descr;
	this->bbt_md = &bbt_mirror_descr;

	/* update flash based bbt */
	this->options |= NAND_USE_FLASH_BBT;

	if (!this->badblock_pattern) {
		if (nandinfo.largepage)
			this->badblock_pattern = &smallpage_memorybased;
		else
			this->badblock_pattern = (mtd->writesize > 512) ?
			    &largepage_memorybased : &smallpage_memorybased;
	}
	/* Build bad block table */
	return nand_scan_bbt(mtd, this->badblock_pattern);
}

int board_nand_init(struct nand_chip *nand)
{
	nand->chip_delay = 0;

	nand->cmdfunc = mxc_nand_command;
	nand->select_chip = mxc_nand_select_chip;
	nand->read_byte = mxc_nand_read_byte;
	nand->read_word = mxc_nand_read_word;
	nand->write_buf = mxc_nand_write_buf;
	nand->read_buf = mxc_nand_read_buf;
	nand->verify_buf = mxc_nand_verify_buf;
	nand->scan_bbt = mxc_nand_scan_bbt;
	nand->ecc.calculate = mxc_nand_calculate_ecc;
	nand->ecc.correct = mxc_nand_correct_data;
	nand->ecc.hwctl = mxc_nand_enable_hwecc;
	nand->ecc.mode = NAND_ECC_HW;
	nand->ecc.bytes = 3;
	nand->ecc.size = 512;

	/* Reset NAND */
	NFC_CONFIG1 |= NFC_INT_MSK | NFC_RST | NFC_ECC_EN;

	/* Unlock the internal RAM buffer */
	NFC_CONFIG = 0x2;

	/* Block to be unlocked */
	NFC_UNLOCKSTART_BLKADDR = 0x0;
	NFC_UNLOCKEND_BLKADDR = 0x4000;

	/* Unlock Block Command for given address range */
	NFC_WRPROT = 0x4;

	/* Only 8 bit bus support for now */
	nand->options |= 0;

	if ((NFMS >> NFMS_BIT) & 1) {
		nandinfo.largepage = 1;
		nand->ecc.layout = &nand_hw_eccoob_2k;
	} else {
		nandinfo.largepage = 0;
		nand->ecc.layout = &nand_hw_eccoob_8;
	}

	return 0;
}
