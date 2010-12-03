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

#include <common.h>
#include <malloc.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <linux/mtd/nand.h>
#include <asm-arm/arch/mxc_nand.h>
#include "nand_device_info.h"

struct nand_info {
	int status_req;
	int large_page;
	int auto_mode;
	u16 col_addr;
	u8 num_of_intlv;
	int page_mask;
	int hw_ecc;
	u8 *data_buf;
	u8 *oob_buf;
};

/*
 * Define delays in microsec for NAND device operations
 */
#define TROP_US_DELAY   2000

/*
 * OOB placement block for use with hardware ecc generation
 */
static struct nand_ecclayout nand_hw_eccoob_512 = {
	.eccbytes = 9,
	.eccpos = {7, 8, 9, 10, 11, 12, 13, 14, 15},
	.oobfree = {{0, 4} }
};

static struct nand_ecclayout nand_hw_eccoob_2k = {
	.eccbytes = 9,
	.eccpos = {7, 8, 9, 10, 11, 12, 13, 14, 15},
	.oobfree = {{2, 4} }
};

static struct nand_ecclayout nand_hw_eccoob_4k = {
	.eccbytes = 9,
	.eccpos = {7, 8, 9, 10, 11, 12, 13, 14, 15},
	.oobfree = {{2, 4} }
};


static void mxc_nand_bi_swap(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	struct nand_info *info = this->priv;
	u16 ma, sa, nma, nsa;

	if (!IS_LARGE_PAGE_NAND)
		return;

	ma = __raw_readw(BAD_BLK_MARKER_MAIN);
	sa = __raw_readw(BAD_BLK_MARKER_SP);

	nma = (ma & 0xFF00) | (sa >> 8);
	nsa = (sa & 0x00FF) | (ma << 8);

	__raw_writew(nma, BAD_BLK_MARKER_MAIN);
	__raw_writew(nsa, BAD_BLK_MARKER_SP);

}

/*!
 * @defgroup NAND_MTD NAND Flash MTD Driver for MXC processors
 */

/*!
 * @file mxc_nd2.c
 *
 * @brief This file contains the hardware specific layer for NAND Flash on
 * MXC processor
 *
 * @ingroup NAND_MTD
 */

/*!
 * Half word access.Added for U-boot.
 */
static void *nfc_memcpy(void *dest, const void *src, size_t n)
{
	u16 *dst_16 = (u16 *) dest;
	const u16 *src_16 = (u16 *) src;

	while (n > 0) {
		*dst_16++ = *src_16++;
		n -= 2;
	}

	return dest;
}

/*
 * Functions to transfer data to/from spare erea.
 */
static void
copy_spare(struct mtd_info *mtd, void *pbuf, void *pspare, int len, int bfrom)
{
	u16 i, j;
	u16 m = mtd->oobsize;
	u16 n = mtd->writesize >> 9;
	u8 *d = (u8 *) pbuf;
	u8 *s = (u8 *) pspare;
	u16 t = SPARE_LEN;
	struct nand_chip *this = mtd->priv;
	struct nand_info *info = this->priv;

	m /= info->num_of_intlv;
	n /= info->num_of_intlv;

	j = (m / n >> 1) << 1;

	if (bfrom) {
		for (i = 0; i < n - 1; i++)
			nfc_memcpy(&d[i * j], &s[i * t], j);

		/* the last section */
		nfc_memcpy(&d[i * j], &s[i * t], len - i * j);
	} else {
		for (i = 0; i < n - 1; i++)
			nfc_memcpy(&s[i * t], &d[i * j], j);

		/* the last section */
		nfc_memcpy(&s[i * t], &d[i * j], len - i * j);
	}
}

/*!
 * This function polls the NFC to wait for the basic operation to complete by
 * checking the INT bit of config2 register.
 *
 * @param       maxRetries     number of retry attempts (separated by 1 us)
 * @param       useirq         True if IRQ should be used rather than polling
 */
static void wait_op_done(int max_retries)
{

	while (max_retries-- > 0) {
		if (raw_read(REG_NFC_OPS_STAT) & NFC_OPS_STAT) {
			WRITE_NFC_IP_REG((raw_read(REG_NFC_OPS_STAT) &
					~NFC_OPS_STAT),
					REG_NFC_OPS_STAT);
			break;
		}
		udelay(1);
	}
	if (max_retries <= 0)
		MTDDEBUG(MTD_DEBUG_LEVEL0, "wait: INT not set\n");
}

/*!
 * This function sends an address (or partial address) to the
 * NAND device.  The address is used to select the source/destination for
 * a NAND command.
 *
 * @param       addr    address to be written to NFC.
 * @param       useirq  True if IRQ should be used rather than polling
 */
static void send_addr(u16 addr)
{
	MTDDEBUG(MTD_DEBUG_LEVEL3, "send_addr(0x%x)\n", addr);

	/* fill address */
	raw_write((addr << NFC_FLASH_ADDR_SHIFT), REG_NFC_FLASH_ADDR);

	/* clear status */
	ACK_OPS;

	/* send out address */
	raw_write(NFC_ADDR, REG_NFC_OPS);

	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY);
}

static void mxc_do_addr_cycle_auto(struct mtd_info *mtd, int column,
							int page_addr)
{
#ifdef CONFIG_MXC_NFC_SP_AUTO
	if (page_addr != -1 && column != -1) {
		u32 mask = 0xFFFF;
		/* the column address */
		raw_write(column & mask, NFC_FLASH_ADDR0);
		raw_write((raw_read(NFC_FLASH_ADDR0) |
			   ((page_addr & mask) << 16)), NFC_FLASH_ADDR0);
		/* the row address */
		raw_write(((raw_read(NFC_FLASH_ADDR8) & (mask << 16)) |
			   ((page_addr & (mask << 16)) >> 16)),
			  NFC_FLASH_ADDR8);
	} else if (page_addr != -1) {
		raw_write(page_addr, NFC_FLASH_ADDR0);
	}

	MTDDEBUG(MTD_DEBUG_LEVEL3,
	      "AutoMode:the ADDR REGS value is (0x%x, 0x%x)\n",
	      raw_read(NFC_FLASH_ADDR0), raw_read(NFC_FLASH_ADDR8));
#endif
}

static void mxc_do_addr_cycle_atomic(struct mtd_info *mtd, int column,
							int page_addr)
{
	struct nand_chip *this = mtd->priv;
	struct nand_info *info = this->priv;

	u32 page_mask = info->page_mask;

	if (column != -1) {
		send_addr(column & 0xFF);
		if (IS_2K_PAGE_NAND) {
			/* another col addr cycle for 2k page */
			send_addr((column >> 8) & 0xF);
		} else if (IS_4K_PAGE_NAND) {
			/* another col addr cycle for 4k page */
			send_addr((column >> 8) & 0x1F);
		}
	}
	if (page_addr != -1) {
		do {
			send_addr(page_addr & 0xff);
			page_mask >>= 8;
			page_addr >>= 8;
		} while (page_mask != 0);
	}
}

/*
 * Function to perform the address cycles.
 */
static void mxc_nand_addr_cycle(struct mtd_info *mtd, int column, int page_addr)
{
	struct nand_chip *this = mtd->priv;
	struct nand_info *info = this->priv;

	if (info->auto_mode)
		mxc_do_addr_cycle_auto(mtd, column, page_addr);
	else
		mxc_do_addr_cycle_atomic(mtd, column, page_addr);
}

static void send_cmd_atomic(struct mtd_info *mtd, u16 cmd)
{
	/* fill command */
	raw_write(cmd, REG_NFC_FLASH_CMD);

	/* clear status */
	ACK_OPS;

	/* send out command */
	raw_write(NFC_CMD, REG_NFC_OPS);

	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY);
}

/*
 * Function to record the ECC corrected/uncorrected errors resulted
 * after a page read. This NFC detects and corrects upto to 4 symbols
 * of 9-bits each.
 */
static int mxc_nand_ecc_status(struct mtd_info *mtd)
{
	u32 ecc_stat, err;
	int no_subpages = 1;
	int ret = 0;
	struct nand_chip *this = mtd->priv;
	struct nand_info *info = this->priv;
	u8 ecc_bit_mask = 0xf;

	no_subpages = mtd->writesize >> 9;

	no_subpages /= info->num_of_intlv;

	ecc_stat = GET_NFC_ECC_STATUS();
	do {
		err = ecc_stat & ecc_bit_mask;
		if (err == ecc_bit_mask) {
			printk(KERN_WARNING "UnCorrectable RS-ECC Error\n");
			return -1;
		} else {
			ret += err;
		}
		ecc_stat >>= 4;
	} while (--no_subpages);

	MTDDEBUG(MTD_DEBUG_LEVEL3, "Correctable ECC Error(%d)\n", ret);

	return ret;
}

/*!
 * This function handle the interleave related work
 * @param	mtd	mtd info
 * @param	cmd	command
 */
static void send_cmd_interleave(struct mtd_info *mtd, u16 cmd)
{
#ifdef CONFIG_MXC_NFC_SP_AUTO

	struct nand_chip *this = mtd->priv;
	struct nand_info *info = this->priv;
	u32 addr_low = raw_read(NFC_FLASH_ADDR0);
	u32 addr_high = raw_read(NFC_FLASH_ADDR8);
	u32 page_addr = addr_low >> 16 | addr_high << 16;
	u32 i, j = info->num_of_intlv;
	u8 *dbuf = info->data_buf;
	u8 *obuf = info->oob_buf;
	u32 dlen = mtd->writesize / j;
	u32 olen = mtd->oobsize / j;
	u32 ncs = 0;

	/* adjust the addr value
	 * since ADD_OP mode is 01
	 */
	if (cmd == NAND_CMD_ERASE2)
		page_addr = addr_low;

	ncs = NFC_GET_NFC_ACTIVE_CS();

	if (j > 1) {
		page_addr *= j;
	} else {
		page_addr *= this->numchips;
		page_addr += ncs;
	}

	switch (cmd) {
	case NAND_CMD_PAGEPROG:
		for (i = 0; i < j; i++) {
			/* reset addr cycle */
			mxc_nand_addr_cycle(mtd, 0, page_addr++);

			/* data transfer */
			nfc_memcpy(MAIN_AREA0, dbuf, dlen);
			copy_spare(mtd, obuf, SPARE_AREA0, olen, 0);
			mxc_nand_bi_swap(mtd);

			/* update the value */
			dbuf += dlen;
			obuf += olen;

			NFC_SET_RBA(0);
			raw_write(0, REG_NFC_OPS_STAT);
			raw_write(NFC_AUTO_PROG, REG_NFC_OPS);

			/* wait auto_prog_done bit set */
			while (!(raw_read(REG_NFC_OPS_STAT) & NFC_OP_DONE))
				;
		}

		wait_op_done(TROP_US_DELAY);
		while (!(raw_read(REG_NFC_OPS_STAT) & NFC_RB));

		break;
	case NAND_CMD_READSTART:
		for (i = 0; i < j; i++) {
			/* reset addr cycle */
			mxc_nand_addr_cycle(mtd, 0, page_addr++);

			NFC_SET_RBA(0);
			raw_write(0, REG_NFC_OPS_STAT);
			raw_write(NFC_AUTO_READ, REG_NFC_OPS);
			wait_op_done(TROP_US_DELAY);

			/* check ecc error */
			mxc_nand_ecc_status(mtd);

			/* data transfer */
			mxc_nand_bi_swap(mtd);
			nfc_memcpy(dbuf, MAIN_AREA0, dlen);
			copy_spare(mtd, obuf, SPARE_AREA0, olen, 1);

			/* update the value */
			dbuf += dlen;
			obuf += olen;
		}
		break;
	case NAND_CMD_ERASE2:
		for (i = 0; i < j; i++) {
			mxc_nand_addr_cycle(mtd, -1, page_addr++);
			raw_write(0, REG_NFC_OPS_STAT);
			raw_write(NFC_AUTO_ERASE, REG_NFC_OPS);
			wait_op_done(TROP_US_DELAY);
		}
		break;
	case NAND_CMD_RESET:
		for (i = 0; i < j; i++) {
			if (j > 1)
				NFC_SET_NFC_ACTIVE_CS(i);
			send_cmd_atomic(mtd, cmd);
		}
		break;
	default:
		break;
	}
#endif
}

static void send_cmd_auto(struct mtd_info *mtd, u16 cmd)
{
#ifdef CONFIG_MXC_NFC_SP_AUTO
	switch (cmd) {
	case NAND_CMD_READ0:
	case NAND_CMD_READOOB:
		raw_write(NAND_CMD_READ0, REG_NFC_FLASH_CMD);
		break;
	case NAND_CMD_SEQIN:
	case NAND_CMD_ERASE1:
		raw_write(cmd, REG_NFC_FLASH_CMD);
		break;
	case NAND_CMD_PAGEPROG:
	case NAND_CMD_ERASE2:
	case NAND_CMD_READSTART:
		raw_write(raw_read(REG_NFC_FLASH_CMD) | cmd << NFC_CMD_1_SHIFT,
			  REG_NFC_FLASH_CMD);
		send_cmd_interleave(mtd, cmd);
		break;
	case NAND_CMD_READID:
		send_cmd_atomic(mtd, cmd);
		send_addr(0);
		break;
	case NAND_CMD_RESET:
		send_cmd_interleave(mtd, cmd);
		break;
	case NAND_CMD_STATUS:
		send_cmd_atomic(mtd, cmd);
		break;
	default:
		break;
	}
#endif
}

/*!
 * This function issues the specified command to the NAND device and
 * waits for completion.
 *
 * @param       cmd     command for NAND Flash
 * @param       useirq  True if IRQ should be used rather than polling
 */
static void send_cmd(struct mtd_info *mtd, u16 cmd)
{

	struct nand_chip *this = mtd->priv;
	struct nand_info *info = this->priv;

	if (info->auto_mode)
		send_cmd_auto(mtd, cmd);
	else
		send_cmd_atomic(mtd, cmd);

	MTDDEBUG(MTD_DEBUG_LEVEL3, "send_cmd(0x%x)\n", cmd);
}

/*!
 * This function requests the NFC to initate the transfer
 * of data currently in the NFC RAM buffer to the NAND device.
 *
 * @param	buf_id	      Specify Internal RAM Buffer number
 */
static void send_prog_page(struct mtd_info *mtd, u8 buf_id)
{
	struct nand_chip *this = mtd->priv;
	struct nand_info *info = this->priv;

	if (!info->auto_mode) {
		/* set ram buffer id */
		NFC_SET_RBA(buf_id);

		/* clear status */
		ACK_OPS;

		/* transfer data from NFC ram to nand */
		raw_write(NFC_INPUT, REG_NFC_OPS);

		/* Wait for operation to complete */
		wait_op_done(TROP_US_DELAY);

		MTDDEBUG(MTD_DEBUG_LEVEL3, "%s\n", __func__);
	}
}

/*!
 * This function requests the NFC to initated the transfer
 * of data from the NAND device into in the NFC ram buffer.
 *
 * @param  	buf_id		Specify Internal RAM Buffer number
 */
static void send_read_page(struct mtd_info *mtd, u8 buf_id)
{
	struct nand_chip *this = mtd->priv;
	struct nand_info *info = this->priv;

	if (!info->auto_mode) {

		/* set ram buffer id */
		NFC_SET_RBA(buf_id);

		/* clear status */
		ACK_OPS;

		/* transfer data from nand to NFC ram */
		raw_write(NFC_OUTPUT, REG_NFC_OPS);

		/* Wait for operation to complete */
		wait_op_done(TROP_US_DELAY);

		MTDDEBUG(MTD_DEBUG_LEVEL3, "%s(%d)\n", __func__, buf_id);

	}

}

/*!
 * This function requests the NFC to perform a read of the
 * NAND device ID.
 */
static void send_read_id(void)
{
	/* Set RBA bits for BUFFER0 */
	NFC_SET_RBA(0);

	/* clear status */
	ACK_OPS;

	/* Read ID into main buffer */
	raw_write(NFC_ID, REG_NFC_OPS);

	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY);

}

static u16 mxc_do_status_auto(struct mtd_info *mtd)
{
	u16 status = 0;
#ifdef CONFIG_MXC_NFC_SP_AUTO
	int i = 0;
	u32 mask = 0xFF << 16;
	int cs = NFC_GET_NFC_ACTIVE_CS();
	struct nand_chip *this = mtd->priv;
	struct nand_info *info = this->priv;

	for (; i < info->num_of_intlv; i++) {

		/* set ative cs */
		NFC_SET_NFC_ACTIVE_CS(i);

		/* clear status */
		ACK_OPS;

		/* use atomic mode to read status instead
		 * of using auto mode,auto-mode has issues
		 * and the status is not correct.
		*/
		raw_write(NFC_STATUS, REG_NFC_OPS);

		wait_op_done(TROP_US_DELAY);

		status = (raw_read(NFC_CONFIG1) & mask) >> 16;

		if (status & NAND_STATUS_FAIL)
			break;
	}

	/* Restore active CS */
	NFC_SET_NFC_ACTIVE_CS(cs);
#endif
	return status;
}

static u16 mxc_do_status_atomic(struct mtd_info *mtd)
{
	volatile u16 *mainBuf = MAIN_AREA1;
	u8 val = 1;
	u16 ret;

	/* Set ram buffer id */
	NFC_SET_RBA(val);

	/* clear status */
	ACK_OPS;

	/* Read status into main buffer */
	raw_write(NFC_STATUS, REG_NFC_OPS);

	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY);

	/* Status is placed in first word of main buffer */
	/* get status, then recovery area 1 data */
	ret = *mainBuf;

	return ret;
}

/*!
 * This function requests the NFC to perform a read of the
 * NAND device status and returns the current status.
 *
 * @return  device status
 */
static u16 mxc_nand_get_status(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	struct nand_info *info = this->priv;
	u16 status;

	if (info->auto_mode)
		status = mxc_do_status_auto(mtd);
	else
		status = mxc_do_status_atomic(mtd);

	return status;

}

static void mxc_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	raw_write((raw_read(REG_NFC_ECC_EN) | NFC_ECC_EN), REG_NFC_ECC_EN);
	return;
}

/*
 * Function to correct the detected errors. This NFC corrects all the errors
 * detected. So this function just return 0.
 */
static int mxc_nand_correct_data(struct mtd_info *mtd, u_char *dat,
				 u_char *read_ecc, u_char *calc_ecc)
{
	return 0;
}

/*
 * Function to calculate the ECC for the data to be stored in the Nand device.
 * This NFC has a hardware RS(511,503) ECC engine together with the RS ECC
 * CONTROL blocks are responsible for detection  and correction of up to
 * 8 symbols of 9 bits each in 528 byte page.
 * So this function is just return 0.
 */

static int mxc_nand_calculate_ecc(struct mtd_info *mtd, const u_char *dat,
				  u_char *ecc_code)
{
	return 0;
}

/*!
 * This function id is used to read the data buffer from the NAND Flash. To
 * read the data from NAND Flash first the data output cycle is initiated by
 * the NFC, which copies the data to RAMbuffer. This data of length \b len is
 * then copied to buffer \b buf.
 *
 * @param       mtd     MTD structure for the NAND Flash
 * @param       buf     data to be read from NAND Flash
 * @param       len     number of bytes to be read
 */
static void mxc_nand_read_buf(struct mtd_info *mtd, u_char * buf, int len)
{
	struct nand_chip *this = mtd->priv;
	struct nand_info *info = this->priv;
	u16 col = info->col_addr;
	u8 *data_buf = info->data_buf;
	u8 *oob_buf = info->oob_buf;

	if (mtd->writesize) {

		int j = mtd->writesize - col;
		int n = mtd->oobsize + j;

		n = min(n, len);

		if (j > 0) {
			if (n > j) {
				memcpy(buf, &data_buf[col], j);
				memcpy(buf + j, &oob_buf[0], n - j);
			} else {
				memcpy(buf, &data_buf[col], n);
			}
		} else {
			col -= mtd->writesize;
			memcpy(buf, &oob_buf[col], len);
		}

		/* update */
		info->col_addr += n;

	} else {
		/* At flash identify phase,
		 * mtd->writesize has not been
		 * set correctly, it should
		 * be zero.And len will less 2
		 */
		memcpy(buf, &data_buf[col], len);

		/* update */
		info->col_addr += len;
	}

}

/*!
 * This function reads byte from the NAND Flash
 *
 * @param       mtd     MTD structure for the NAND Flash
 *
 * @return    data read from the NAND Flash
 */
static uint8_t mxc_nand_read_byte(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	struct nand_info *info = this->priv;
	uint8_t ret;

	/* Check for status request */
	if (info->status_req)
		return mxc_nand_get_status(mtd) & 0xFF;

	mxc_nand_read_buf(mtd, &ret, 1);

	return ret;
}

/*!
  * This function reads word from the NAND Flash
  *
  * @param     mtd     MTD structure for the NAND Flash
  *
  * @return    data read from the NAND Flash
  */
static u16 mxc_nand_read_word(struct mtd_info *mtd)
{
	u16 ret;

	mxc_nand_read_buf(mtd, (uint8_t *) &ret, sizeof(u16));

	return ret;
}

/*!
 * This function reads byte from the NAND Flash
 *
 * @param     mtd     MTD structure for the NAND Flash
 *
 * @return    data read from the NAND Flash
 */
static u_char mxc_nand_read_byte16(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	struct nand_info *info = this->priv;

	/* Check for status request */
	if (info->status_req)
		return mxc_nand_get_status(mtd) & 0xFF;

	return mxc_nand_read_word(mtd) & 0xFF;
}


/*!
 * This function writes data of length \b len from buffer \b buf to the NAND
 * internal RAM buffer's MAIN area 0.
 *
 * @param       mtd     MTD structure for the NAND Flash
 * @param       buf     data to be written to NAND Flash
 * @param       len     number of bytes to be written
 */
static void mxc_nand_write_buf(struct mtd_info *mtd,
			       const u_char *buf, int len)
{
	struct nand_chip *this = mtd->priv;
	struct nand_info *info = this->priv;
	u16 col = info->col_addr;
	u8 *data_buf = info->data_buf;
	u8 *oob_buf = info->oob_buf;
	int j = mtd->writesize - col;
	int n = mtd->oobsize + j;

	n = min(n, len);

	if (j > 0) {
		if (n > j) {
			memcpy(&data_buf[col], buf, j);
			memcpy(&oob_buf[0], buf + j, n - j);
		} else {
			memcpy(&data_buf[col], buf, n);
		}
	} else {
		col -= mtd->writesize;
		memcpy(&oob_buf[col], buf, len);
	}

	/* update */
	info->col_addr += n;
}

/*!
 * This function is used by the upper layer to verify the data in NAND Flash
 * with the data in the \b buf.
 *
 * @param       mtd     MTD structure for the NAND Flash
 * @param       buf     data to be verified
 * @param       len     length of the data to be verified
 *
 * @return      -EFAULT if error else 0
 *
 */
static int mxc_nand_verify_buf(struct mtd_info *mtd, const u_char *buf,
			       int len)
{
	struct nand_chip *this = mtd->priv;
	struct nand_info *info = this->priv;
	u_char *s = info->data_buf;

	const u_char *p = buf;

	for (; len > 0; len--) {
		if (*p++ != *s++)
			return -1;
	}

	return 0;
}

/*!
 * This function is used by upper layer for select and deselect of the NAND
 * chip
 *
 * @param       mtd     MTD structure for the NAND Flash
 * @param       chip    val indicating select or deselect
 */
static void mxc_nand_select_chip(struct mtd_info *mtd, int chip)
{

	switch (chip) {
	case -1:
		break;

	case 0 ... 7:
		NFC_SET_NFC_ACTIVE_CS(chip);
		break;

	default:
		break;
	}
}

/*!
 * This function is used by the upper layer to write command to NAND Flash for
 * different operations to be carried out on NAND Flash
 *
 * @param       mtd             MTD structure for the NAND Flash
 * @param       command         command for NAND Flash
 * @param       column          column offset for the page read
 * @param       page_addr       page to be read from NAND Flash
 */
static void mxc_nand_command(struct mtd_info *mtd, unsigned command,
			     int column, int page_addr)
{
	struct nand_chip *this = mtd->priv;
	struct nand_info *info = this->priv;

	MTDDEBUG(MTD_DEBUG_LEVEL3,
	      "mxc_nand_command (cmd = 0x%x, col = 0x%x, page = 0x%x)\n",
	      command, column, page_addr);
	/*
	 * Reset command state information
	 */
	info->status_req = 0;

	/*
	 * Command pre-processing step
	 */
	switch (command) {
	case NAND_CMD_STATUS:
		info->col_addr = 0;
		info->status_req = 1;
		break;

	case NAND_CMD_READ0:
		info->col_addr = column;
		break;

	case NAND_CMD_READOOB:
		info->col_addr = column;
		command = NAND_CMD_READ0;
		break;

	case NAND_CMD_SEQIN:
		if (column != 0) {

			/* FIXME: before send SEQIN command for
			 * partial write,We need read one page out.
			 * FSL NFC does not support partial write
			 * It alway send out 512+ecc+512+ecc ...
			 * for large page nand flash. But for small
			 * page nand flash, it did support SPARE
			 * ONLY operation. But to make driver
			 * simple. We take the same as large page,read
			 * whole page out and update. As for MLC nand
			 * NOP(num of operation) = 1. Partial written
			 * on one programed page is not allowed! We
			 * can't limit it on the driver, it need the
			 * upper layer applicaiton take care it
			 */

			mxc_nand_command(mtd, NAND_CMD_READ0, 0, page_addr);
		}

		info->col_addr = column;
		break;

	case NAND_CMD_PAGEPROG:
		if (!info->auto_mode) {
			nfc_memcpy(MAIN_AREA0, info->data_buf, mtd->writesize);
			copy_spare(mtd, info->oob_buf, SPARE_AREA0,
						mtd->oobsize, 0);
			mxc_nand_bi_swap(mtd);
		}

		send_prog_page(mtd, 0);
		break;

	case NAND_CMD_ERASE1:
	case NAND_CMD_ERASE2:
		break;
	}

	/*
	 * Write out the command to the device.
	 */
	send_cmd(mtd, command);

	mxc_nand_addr_cycle(mtd, column, page_addr);

	/*
	 * Command post-processing step
	 */
	switch (command) {

	case NAND_CMD_READOOB:
	case NAND_CMD_READ0:
		if (info->large_page)
			/* send read confirm command */
			send_cmd(mtd, NAND_CMD_READSTART);

		send_read_page(mtd, 0);

		if (!info->auto_mode) {
			mxc_nand_ecc_status(mtd);
			mxc_nand_bi_swap(mtd);
			nfc_memcpy(info->data_buf, MAIN_AREA0, mtd->writesize);
			copy_spare(mtd, info->oob_buf, SPARE_AREA0,
						mtd->oobsize, 1);
		}
		break;

	case NAND_CMD_READID:
		send_read_id();
		info->col_addr = column;
		nfc_memcpy(info->data_buf, MAIN_AREA0, 2048);
		break;
	}
}

static int mxc_nand_read_oob(struct mtd_info *mtd,
			     struct nand_chip *chip, int page, int sndcmd)
{
	struct nand_chip *this = mtd->priv;
	struct nand_info *info = this->priv;

	if (sndcmd) {

		chip->cmdfunc(mtd, NAND_CMD_READ0, 0x00, page);
		sndcmd = 0;
	}

	memcpy(chip->oob_poi, info->oob_buf, mtd->oobsize);

	return sndcmd;
}

static int mxc_nand_read_page(struct mtd_info *mtd, struct nand_chip *chip,
			      uint8_t *buf)
{
	struct nand_chip *this = mtd->priv;
	struct nand_info *info = this->priv;

#ifndef CONFIG_MXC_NFC_SP_AUTO
	mxc_nand_ecc_status(mtd);
#endif

	memcpy(buf, info->data_buf, mtd->writesize);
	memcpy(chip->oob_poi, info->oob_buf, mtd->oobsize);

	return 0;
}

static void mxc_nand_write_page(struct mtd_info *mtd, struct nand_chip *chip,
				const uint8_t *buf)
{
	struct nand_chip *this = mtd->priv;
	struct nand_info *info = this->priv;

	memcpy(info->data_buf, buf, mtd->writesize);
	memcpy(info->oob_buf, chip->oob_poi, mtd->oobsize);

}

/**
 * mxc_nand_prog_page - [REPLACEABLE] write one page
 * @mtd:        MTD device structure
 * @chip:       NAND chip descriptor
 * @buf:        the data to write
 * @page:       page number to write
 * @cached:     cached programming
 * @raw:        use _raw version of write_page
 */
static int mxc_nand_prog_page(struct mtd_info *mtd, struct nand_chip *chip,
			const uint8_t *buf, int page, int cached, int raw)
{
	int status;
	int i;

	for (i = 0; i < mtd->writesize; i += 4) {
		if (*(u32 *)(buf + i) != (u32)-1)
			break;
	}

	if (i == mtd->writesize)
		return 0;

		chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0x00, page);

	if (unlikely(raw))
		chip->ecc.write_page_raw(mtd, chip, buf);
	else
		chip->ecc.write_page(mtd, chip, buf);

	/*
	 * Cached progamming disabled for now, Not sure if its worth the
	 * trouble. The speed gain is not very impressive. (2.3->2.6Mib/s)
	 */
	cached = 0;

	if (!cached || !(chip->options & NAND_CACHEPRG)) {

		chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
		status = chip->waitfunc(mtd, chip);
		/*
		 * See if operation failed and additional status checks are
		 * available
		 */
		if ((status & NAND_STATUS_FAIL) && (chip->errstat))
			status = chip->errstat(mtd, chip, FL_WRITING, status,
					       page);

		if (status & NAND_STATUS_FAIL)
			return -EIO;
	} else {
		chip->cmdfunc(mtd, NAND_CMD_CACHEDPROG, -1, -1);
		status = chip->waitfunc(mtd, chip);
	}


	#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
	/* Send command to read back the data */
	chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page);

	if (chip->verify_buf(mtd, buf, mtd->writesize))
		return -EIO;
	#endif
	return 0;
}

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

/* Generic flash bbt decriptors
*/
static uint8_t bbt_pattern[] = { 'B', 'b', 't', '0' };
static uint8_t mirror_pattern[] = { '1', 't', 'b', 'B' };

static struct nand_bbt_descr bbt_main_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
	    | NAND_BBT_2BIT | NAND_BBT_VERSION,
	.offs = 0,
	.len = 4,
	.veroffs = 4,
	.maxblocks = 4,
	.pattern = bbt_pattern
};

static struct nand_bbt_descr bbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
	    | NAND_BBT_2BIT | NAND_BBT_VERSION,
	.offs = 0,
	.len = 4,
	.veroffs = 4,
	.maxblocks = 4,
	.pattern = mirror_pattern
};

static int mxc_nand_scan_bbt(struct mtd_info *mtd)
{
	int i;
	uint8_t id_bytes[NAND_DEVICE_ID_BYTE_COUNT];
	struct nand_chip *this = mtd->priv;
	struct nand_info *info = this->priv;
	struct nand_device_info  *dev_info;

	info->page_mask = this->pagemask;

	if (!IS_LARGE_PAGE_NAND)
		goto skip_it;

	/* Read ID bytes from the first NAND Flash chip. */
	this->select_chip(mtd, 0);

	this->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);

	for (i = 0; i < NAND_DEVICE_ID_BYTE_COUNT; i++)
		id_bytes[i] = this->read_byte(mtd);

	/* Get information about this device, based on the ID bytes. */
	dev_info = nand_device_get_info(id_bytes);

	/* Check if we understand this device. */
	if (!dev_info) {
		printk(KERN_ERR "Unrecognized NAND Flash device.\n");
		return !0;
	}

	nand_device_print_info(dev_info);

	/* Correct mtd setting */
	this->chipsize = dev_info->chip_size_in_bytes;
	mtd->size = dev_info->chip_size_in_bytes * this->numchips;
	mtd->writesize = dev_info->page_total_size_in_bytes & ~0x3ff;
	mtd->oobsize = dev_info->page_total_size_in_bytes & 0x3ff;;
	mtd->erasesize = dev_info->block_size_in_pages * mtd->writesize;

	/* Calculate the address shift from the page size */
	this->page_shift = ffs(mtd->writesize) - 1;
	/* Convert chipsize to number of pages per chip -1. */
	this->pagemask = (this->chipsize >> this->page_shift) - 1;

	this->bbt_erase_shift = this->phys_erase_shift =
		ffs(mtd->erasesize) - 1;
	this->chip_shift = ffs(this->chipsize) - 1;
	this->oob_poi = this->buffers->databuf + mtd->writesize;

skip_it:
	if (IS_2K_PAGE_NAND) {
		NFC_SET_NFMS(1 << NFMS_NF_PG_SZ);
		this->ecc.layout = &nand_hw_eccoob_2k;
		info->large_page = 1;
	} else if (IS_4K_PAGE_NAND) {
		NFC_SET_NFMS(1 << NFMS_NF_PG_SZ);
		this->ecc.layout = &nand_hw_eccoob_4k;
		info->large_page = 1;
	} else {
		this->ecc.layout = &nand_hw_eccoob_512;
		info->large_page = 0;
	}

	/* propagate ecc.layout to mtd_info */
	mtd->ecclayout = this->ecc.layout;

	/* jffs2 not write oob */
	/*mtd->flags &= ~MTD_OOB_WRITEABLE;*/

	/* fix up the offset */
	largepage_memorybased.offs = BAD_BLK_MARKER_OOB_OFFS;

	/* use flash based bbt */
	this->bbt_td = &bbt_main_descr;
	this->bbt_md = &bbt_mirror_descr;

	/* update flash based bbt */
	this->options |= NAND_USE_FLASH_BBT;

	if (!this->badblock_pattern) {
		this->badblock_pattern = (mtd->writesize > 512) ?
		    &largepage_memorybased : &smallpage_memorybased;
	}

	/* Build bad block table */
	return nand_scan_bbt(mtd, this->badblock_pattern);
}

static void mxc_nfc_init(void)
{
	/* Disable interrupt */
	raw_write((raw_read(REG_NFC_INTRRUPT) | NFC_INT_MSK), REG_NFC_INTRRUPT);

	/* disable spare enable */
	raw_write(raw_read(REG_NFC_SP_EN) & ~NFC_SP_EN, REG_NFC_SP_EN);

	/* Unlock the internal RAM Buffer */
	raw_write(NFC_SET_BLS(NFC_BLS_UNLCOKED), REG_NFC_BLS);
#ifndef CONFIG_MX53
	/* Blocks to be unlocked */
	UNLOCK_ADDR(0x0, 0xFFFF);

	/* Unlock Block Command for given address range */
	raw_write(NFC_SET_WPC(NFC_WPC_UNLOCK), REG_NFC_WPC);
#endif
	/* Enable hw ecc */
	raw_write((raw_read(REG_NFC_ECC_EN) | NFC_ECC_EN), REG_NFC_ECC_EN);
}

static int mxc_alloc_buf(struct nand_info *info)
{
	int err = 0;

	info->data_buf = kmalloc(NAND_MAX_PAGESIZE, GFP_KERNEL);
	if (!info->data_buf) {
		printk(KERN_ERR "%s: failed to allocate data_buf\n", __func__);
		err = -ENOMEM;
		return err;
	}
	memset(info->data_buf, 0, NAND_MAX_PAGESIZE);

	info->oob_buf = kmalloc(NAND_MAX_OOBSIZE, GFP_KERNEL);
	if (!info->oob_buf) {
		printk(KERN_ERR "%s: failed to allocate oob_buf\n", __func__);
		err = -ENOMEM;
		return err;
	}
	memset(info->oob_buf, 0, NAND_MAX_OOBSIZE);

	return err;
}

/*!
 * This function is called during the driver binding process.
 *
 * @param   pdev  the device structure used to store device specific
 *                information that is used by the suspend, resume and
 *                remove functions
 *
 * @return  The function always returns 0.
 */
int board_nand_init(struct nand_chip *nand)
{
	struct nand_info *info;
	struct nand_chip *this = nand;
	struct mtd_info *mtd; /* dummy for compile */
	int err;

	info = kmalloc(sizeof(struct nand_info), GFP_KERNEL);
	if (!info) {
		printk(KERN_ERR "%s: failed to allocate nand_info\n",
		       __func__);
		err = -ENOMEM;
		return err;
	}
	memset(info, 0, sizeof(struct nand_info));

	if (mxc_alloc_buf(info)) {
		err = -ENOMEM;
		return err;
	}

	info->num_of_intlv = 1;

#ifdef CONFIG_MXC_NFC_SP_AUTO
	info->auto_mode = 1;
#endif

	/* init the nfc */
	mxc_nfc_init();

	this->priv = info;
	this->cmdfunc = mxc_nand_command;
	this->select_chip = mxc_nand_select_chip;
	this->read_byte = mxc_nand_read_byte;
	this->read_word = mxc_nand_read_word;
	this->write_buf = mxc_nand_write_buf;
	this->read_buf = mxc_nand_read_buf;
	this->verify_buf = mxc_nand_verify_buf;
	this->scan_bbt = mxc_nand_scan_bbt;
	this->write_page = mxc_nand_prog_page;
	this->ecc.read_page = mxc_nand_read_page;
	this->ecc.write_page = mxc_nand_write_page;
	this->ecc.read_oob = mxc_nand_read_oob;
	this->ecc.calculate = mxc_nand_calculate_ecc;
	this->ecc.correct = mxc_nand_correct_data;
	this->ecc.hwctl = mxc_nand_enable_hwecc;
	this->ecc.layout = &nand_hw_eccoob_512;
	this->ecc.mode = NAND_ECC_HW;
	this->ecc.bytes = 9;
	this->ecc.size = 512;

#ifdef CONFIG_NAND_FW_16BIT
	if (CONFIG_NAND_FW_16BIT == 1) {
		this->read_byte = mxc_nand_read_byte16;
		this->options |= NAND_BUSWIDTH_16;
		NFC_SET_NFMS(1 << NFMS_NF_DWIDTH);
	} else {
		NFC_SET_NFMS(0);
	}
#else
	NFC_SET_NFMS(0);
#endif

	return 0;
}
