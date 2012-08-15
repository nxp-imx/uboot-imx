/*
 * Freescale GPMI NFC NAND Flash Driver
 *
 * Copyright (C) 2008 Embedded Alley Solutions, Inc.
 *
 * Copyright (C) 2010-2012 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/mtd/mtd.h>
#include "gpmi_nfc_gpmi.h"
#include "gpmi_nfc_bch.h"
#include "nand_device_info.h"
#include <linux/mtd/nand.h>
#include <linux/types.h>
#include <asm/apbh_dma.h>
#include <asm/io.h>
#include <malloc.h>
#include <common.h>

#ifdef CONFIG_ARCH_MMU
#include <asm/arch/mmu.h>
#endif

/* add our owner bbt descriptor */
static uint8_t scan_ff_pattern[] = { 0xff };
static struct nand_bbt_descr gpmi_bbt_descr = {
	.options	= 0,
	.offs		= 0,
	.len		= 1,
	.pattern	= scan_ff_pattern
};

/*  We will use all the (page + OOB). */
static struct nand_ecclayout gpmi_hw_ecclayout = {
	.eccbytes = 0,
	.eccpos = { 0, },
	.oobfree = { {.offset = 0, .length = 0} }
};

/**
 * gpmi_nfc_cmd_ctrl - MTD Interface cmd_ctrl()
 *
 * This is the function that we install in the cmd_ctrl function pointer of the
 * owning struct nand_chip. The only functions in the reference implementation
 * that use these functions pointers are cmdfunc and select_chip.
 *
 * In this driver, we implement our own select_chip, so this function will only
 * be called by the reference implementation's cmdfunc. For this reason, we can
 * ignore the chip enable bit and concentrate only on sending bytes to the
 * NAND Flash.
 *
 * @mtd:   The owning MTD.
 * @data:  The value to push onto the data signals.
 * @ctrl:  The values to push onto the control signals.
 */
static void gpmi_nfc_cmd_ctrl(struct mtd_info *mtd, int data, unsigned int ctrl)
{
	struct nand_chip      *chip = mtd->priv;
	struct gpmi_nfc_info  *gpmi_info = chip->priv;
	struct nfc_hal        *nfc  =  gpmi_info->nfc;
	int error;
	static u8 *cmd_queue;
	static u32 cmd_Q_len;
#if defined(CONFIG_MTD_DEBUG)
	unsigned int          i;
	char                  display[GPMI_NFC_COMMAND_BUFFER_SIZE * 5];
#endif
	MTDDEBUG(MTD_DEBUG_LEVEL2, "%s =>\n", __func__);

	MTDDEBUG(MTD_DEBUG_LEVEL1, "%s: cmd data: 0x%08x\n", __func__, data);

	if (!cmd_queue) {
#ifdef CONFIG_ARCH_MMU
		cmd_queue =
		(u8 *)ioremap_nocache((u32)iomem_to_phys((ulong)memalign(MXS_DMA_ALIGNMENT,
		GPMI_NFC_COMMAND_BUFFER_SIZE)),
		MXS_DMA_ALIGNMENT);
#else
		cmd_queue =
		memalign(MXS_DMA_ALIGNMENT, GPMI_NFC_COMMAND_BUFFER_SIZE);
#endif
		if (!cmd_queue) {
			printf("%s: failed to allocate command "
				"queuebuffer\n",
				__func__);
			return;
		}
		memset(cmd_queue, 0, GPMI_NFC_COMMAND_BUFFER_SIZE);
		cmd_Q_len = 0;
	}

	/*
	 * Every operation begins with a command byte and a series of zero or
	 * more address bytes. These are distinguished by either the Address
	 * Latch Enable (ALE) or Command Latch Enable (CLE) signals being
	 * asserted. When MTD is ready to execute the command, it will
	 * deasert both latch enables.
	 *
	 * Rather than run a separate DMA operation for every single byte, we
	 * queue them up and run a single DMA operation for the entire series
	 * of command and data bytes.
	 */

	if ((ctrl & (NAND_ALE | NAND_CLE))) {
		if (data != NAND_CMD_NONE)
			cmd_queue[cmd_Q_len++] = data;
		return;
	}

	/*
	 * If control arrives here, MTD has deasserted both the ALE and CLE,
	 * which means it's ready to run an operation. Check if we have any
	 * bytes to send.
	 */

	if (!cmd_Q_len)
		return;

#if defined(CONFIG_MTD_DEBUG)
	display[0] = 0;
	for (i = 0; i < cmd_Q_len; i++)
		sprintf(display + strlen(display),
			" 0x%02x", cmd_queue[i] & 0xff);
	MTDDEBUG(MTD_DEBUG_LEVEL1, "%s: command: %s\n", __func__, display);
#endif

#ifdef CONFIG_ARCH_MMU
	error = nfc->send_command(mtd, gpmi_info->cur_chip,
		(dma_addr_t)iomem_to_phys((u32)cmd_queue), cmd_Q_len);
#else
	error = nfc->send_command(mtd, gpmi_info->cur_chip,
		(dma_addr_t)cmd_queue, cmd_Q_len);
#endif

	if (error)
		printf("Command execute failed!\n");

	/* Reset. */
	cmd_Q_len = 0;


	MTDDEBUG(MTD_DEBUG_LEVEL2, "<= %s\n", __func__);
}

/**
 * gpmi_nfc_dev_ready() - MTD Interface dev_ready()
 *
 * @mtd:   A pointer to the owning MTD.
 */
static int gpmi_nfc_dev_ready(struct mtd_info *mtd)
{
	struct nand_chip      *chip = mtd->priv;
	struct gpmi_nfc_info  *gpmi_info = chip->priv;
	struct nfc_hal *nfc = gpmi_info->nfc;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	return (nfc->is_ready(mtd, gpmi_info->cur_chip)) ? 1 : 0;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);
}

/**
 * gpmi_nfc_select_chip() - MTD Interface select_chip()
 *
 * @mtd:   A pointer to the owning MTD.
 * @chip:  The chip number to select, or -1 to select no chip.
 */
static void gpmi_nfc_select_chip(struct mtd_info *mtd, int chip)
{
	struct nand_chip	*nand = mtd->priv;
	struct gpmi_nfc_info	*gpmi_info = nand->priv;
	struct nfc_hal *nfc = gpmi_info->nfc;

	MTDDEBUG(MTD_DEBUG_LEVEL2, "%s =>\n", __func__);

	nfc->begin(mtd);

	gpmi_info->cur_chip = chip;

	MTDDEBUG(MTD_DEBUG_LEVEL2, "<= %s\n", __func__);
}

/**
 * gpmi_nfc_read_buf() - MTD Interface read_buf().
 *
 * @mtd:  A pointer to the owning MTD.
 * @buf:  The destination buffer.
 * @len:  The number of bytes to read.
 */
static void gpmi_nfc_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct nand_chip	*chip = mtd->priv;
	struct gpmi_nfc_info	*gpmi_info = chip->priv;
	struct nfc_hal		*nfc = gpmi_info->nfc;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	if (len > NAND_MAX_PAGESIZE)
		printf("[%s] Inadequate DMA buffer\n", __func__);

	if (!buf)
		printf("[%s] Buffer pointer is NULL\n", __func__);

	/* Ask the NFC. */
#ifdef CONFIG_ARCH_MMU
	nfc->read_data(mtd, gpmi_info->cur_chip,
			(dma_addr_t)iomem_to_phys((u32)gpmi_info->data_buf),
			len);
#else
	nfc->read_data(mtd, gpmi_info->cur_chip,
			(dma_addr_t)gpmi_info->data_buf, len);
#endif

	memcpy(buf, gpmi_info->data_buf, len);

	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);
}

/**
 * gpmi_nfc_write_buf() - MTD Interface write_buf().
 *
 * @mtd:  A pointer to the owning MTD.
 * @buf:  The source buffer.
 * @len:  The number of bytes to read.
 */
static void gpmi_nfc_write_buf(struct mtd_info *mtd,
				const uint8_t *buf, int len)
{
	struct nand_chip	*chip = mtd->priv;
	struct gpmi_nfc_info	*gpmi_info = chip->priv;
	struct nfc_hal		*nfc = gpmi_info->nfc;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	if (len > NAND_MAX_PAGESIZE)
		printf("[%s] Inadequate DMA buffer\n", __func__);

	if (!buf)
		printf("[%s] Buffer pointer is NULL\n", __func__);

	memcpy(gpmi_info->data_buf, buf, len);

	/* Ask the NFC. */
#ifdef CONFIG_ARCH_MMU
	nfc->send_data(mtd, gpmi_info->cur_chip,
			(dma_addr_t)iomem_to_phys((u32)gpmi_info->data_buf),
			len);
#else
	nfc->send_data(mtd, gpmi_info->cur_chip,
			(dma_addr_t)gpmi_info->data_buf, len);
#endif

	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);
}

/**
 * gpmi_nfc_read_byte() - MTD Interface read_byte().
 *
 * @mtd:  A pointer to the owning MTD.
 */
static uint8_t gpmi_nfc_read_byte(struct mtd_info *mtd)
{
	u8 byte;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	gpmi_nfc_read_buf(mtd, (u8 *)&byte, 1);

	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);

	return byte;
}

#ifdef CONFIG_GPMI_NFC_SWAP_BLOCK_MARK
/**
 * gpmi_nfc_block_mark_swapping() - Handles block mark swapping.
 *
 * Note that, when this function is called, it doesn't know whether it's
 * swapping the block mark, or swapping it *back* -- but it doesn't matter
 * because the the operation is the same.
 *
 * @this:       Per-device data.
 * @payload:    A pointer to the payload buffer.
 * @auxiliary:  A pointer to the auxiliary buffer.
 */
static void gpmi_nfc_block_mark_swapping(struct gpmi_nfc_info *gpmi_info,
					void *data_buf, void *oob_buf)
{
	u8  *p;
	u8  *a;
	u32 bit;
	u8  mask;
	u8  from_data;
	u8  from_oob;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);
	/*
	 * If control arrives here, we're swapping. Make some convenience
	 * variables.
	 */
	bit = gpmi_info->block_mark_bit_offset;
	p   = ((u8 *)data_buf) + gpmi_info->block_mark_byte_offset;
	a   = oob_buf;


	/*
	 * Get the byte from the data area that overlays the block mark. Since
	 * the ECC engine applies its own view to the bits in the page, the
	 * physical block mark won't (in general) appear on a byte boundary in
	 * the data.
	 */
	from_data = (p[0] >> bit) | (p[1] << (8 - bit));

	/* Get the byte from the OOB. */
	from_oob = a[0];

	/* Swap them. */
	a[0] = from_data;

	mask = (0x1 << bit) - 1;
	p[0] = (p[0] & mask) | (from_oob << bit);

	mask = ~0 << bit;
	p[1] = (p[1] & mask) | (from_oob >> (8 - bit));

	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);
}
#endif

/**
 * gpmi_nfc_ecc_read_page() - MTD Interface ecc.read_page().
 *
 * @mtd:   A pointer to the owning MTD.
 * @nand:  A pointer to the owning NAND Flash MTD.
 * @buf:   A pointer to the destination buffer.
 */
static int gpmi_nfc_ecc_read_page(struct mtd_info *mtd,
				struct nand_chip *nand, uint8_t *buf)
{
	struct gpmi_nfc_info    *gpmi_info = nand->priv;
	struct nfc_hal          *nfc     =  gpmi_info->nfc;
	unsigned int            i;
	unsigned char           *status;
	unsigned int            failed;
	unsigned int            corrected;
	int                     error = 0;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	MTDDEBUG(MTD_DEBUG_LEVEL1, "Buf: 0x%08x, data_buf: 0x%08x, "
		"oob_buf: 0x%08x",
		(u32)buf, (u32)gpmi_info->data_buf, (u32)gpmi_info->oob_buf);
	/* Ask the NFC. */
#ifdef CONFIG_ARCH_MMU
	error = nfc->read_page(mtd, gpmi_info->cur_chip,
			(dma_addr_t)iomem_to_phys((u32)gpmi_info->data_buf),
			(dma_addr_t)iomem_to_phys((u32)gpmi_info->oob_buf));
#else
	error = nfc->read_page(mtd, gpmi_info->cur_chip,
				(dma_addr_t)gpmi_info->data_buf,
				(dma_addr_t)gpmi_info->oob_buf);
#endif
	if (error) {
		printf("[%s] Error in ECC-based read: %d\n",
			__func__, error);
		goto exit;
	}

	/* Handle block mark swapping. */
	gpmi_nfc_block_mark_swapping(gpmi_info, gpmi_info->data_buf,
				gpmi_info->oob_buf);

	/* Loop over status bytes, accumulating ECC status. */
	failed    = 0;
	corrected = 0;

	status = ((u8 *)gpmi_info->oob_buf) +
		gpmi_info->auxiliary_status_offset;

	for (i = 0; i < gpmi_info->ecc_chunk_count; i++, status++) {

		if ((*status == 0x00) || (*status == 0xff))
			continue;

		if (*status == 0xfe) {
			failed++;
			continue;
		}

		corrected += *status;
	}

	/*
	 * Propagate ECC status to the owning MTD only when failed or
	 * corrected times nearly reaches our ECC correction threshold.
	 */
	if (failed || corrected >= (gpmi_info->ecc_strength - 1)) {
		mtd->ecc_stats.failed    += failed;
		mtd->ecc_stats.corrected += corrected;
	}

	/*
	 * It's time to deliver the OOB bytes. See gpmi_nfc_ecc_read_oob() for
	 * details about our policy for delivering the OOB.
	 *
	 * We fill the caller's buffer with set bits, and then copy the block
	 * mark to th caller's buffer. Note that, if block mark swapping was
	 * necessary, it has already been done, so we can rely on the first
	 * byte of the auxiliary buffer to contain the block mark.
	 */
	memset(nand->oob_poi, ~0, mtd->oobsize);

	nand->oob_poi[0] = ((u8 *)gpmi_info->oob_buf)[0];

	MTDDEBUG(MTD_DEBUG_LEVEL1, "nand->oob_poi[0]: 0x%02x\n",
		nand->oob_poi[0]);

	/* Return. */
	memcpy(buf, gpmi_info->data_buf, mtd->writesize);
exit:
	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);
	return error;
}

/**
 * gpmi_nfc_ecc_write_page() - MTD Interface ecc.write_page().
 *
 * @mtd:   A pointer to the owning MTD.
 * @nand:  A pointer to the owning NAND Flash MTD.
 * @buf:   A pointer to the source buffer.
 */
static void gpmi_nfc_ecc_write_page(struct mtd_info *mtd,
				struct nand_chip *nand, const uint8_t *buf)
{
	struct gpmi_nfc_info *gpmi_info = nand->priv;
	struct nfc_hal       *nfc       =  gpmi_info->nfc;
	int                     error;
	u8 *data_buf = gpmi_info->data_buf;
	u8 *oob_buf  = gpmi_info->oob_buf;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	MTDDEBUG(MTD_DEBUG_LEVEL1, "Buf: 0x%08x, data_buf: 0x%08x, "
		"oob_buf: 0x%08x\n", (u32)buf, (u32)data_buf, (u32)oob_buf);

	memcpy(data_buf, buf, mtd->writesize);
	memcpy(oob_buf, nand->oob_poi, mtd->oobsize);

	MTDDEBUG(MTD_DEBUG_LEVEL1, "oob_buf[0]: 0x%02x\n",
		oob_buf[0]);

#ifdef CONFIG_GPMI_NFC_SWAP_BLOCK_MARK
	/* Handle block mark swapping. */
	gpmi_nfc_block_mark_swapping(gpmi_info,
			(void *)data_buf,
			(void *)oob_buf);
#endif
	/* Ask the NFC. */
#ifdef CONFIG_ARCH_MMU
	error = nfc->send_page(mtd, gpmi_info->cur_chip,
				(dma_addr_t)iomem_to_phys((u32)data_buf),
				(dma_addr_t)iomem_to_phys((u32)oob_buf));
#else
	error = nfc->send_page(mtd, gpmi_info->cur_chip,
				(dma_addr_t)data_buf,
				(dma_addr_t)oob_buf);
#endif

	if (error)
		printf("[%s] Error in ECC-based write: %d\n",
			__func__, error);

	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);
}

/**
 * gpmi_nfc_hook_read_oob() - Hooked MTD Interface read_oob().
 *
 * This function is a veneer that replaces the function originally installed by
 * the NAND Flash MTD code. See the description of the raw_oob_mode field in
 * struct mil for more information about this.
 *
 * @mtd:   A pointer to the MTD.
 * @from:  The starting address to read.
 * @ops:   Describes the operation.
 */
static int gpmi_nfc_hook_read_oob(struct mtd_info *mtd,
				loff_t from, struct mtd_oob_ops *ops)
{
	register struct nand_chip  *chip = mtd->priv;
	struct gpmi_nfc_info       *gpmi_info = chip->priv;
	int                        ret;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	gpmi_info->m_u8RawOOBMode = ops->mode == MTD_OOB_RAW;
	ret = gpmi_info->hooked_read_oob(mtd, from, ops);
	gpmi_info->m_u8RawOOBMode = 0;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);
	return ret;
}

/**
 * gpmi_nfc_hook_write_oob() - Hooked MTD Interface write_oob().
 *
 * This function is a veneer that replaces the function originally installed by
 * the NAND Flash MTD code. See the description of the raw_oob_mode field in
 * struct mil for more information about this.
 *
 * @mtd:   A pointer to the MTD.
 * @to:    The starting address to write.
 * @ops:   Describes the operation.
 */
static int gpmi_nfc_hook_write_oob(struct mtd_info *mtd,
					loff_t to, struct mtd_oob_ops *ops)
{
	register struct nand_chip  *chip = mtd->priv;
	struct gpmi_nfc_info       *gpmi_info = chip->priv;
	int                        ret;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	gpmi_info->m_u8RawOOBMode = ops->mode == MTD_OOB_RAW;
	ret = gpmi_info->hooked_write_oob(mtd, to, ops);
	gpmi_info->m_u8RawOOBMode = false;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);

	return ret;
}

/**
 * gpmi_nfc_hook_block_markbad() - Hooked MTD Interface block_markbad().
 *
 * This function is a veneer that replaces the function originally installed by
 * the NAND Flash MTD code. See the description of the marking_a_bad_block field
 * in struct mil for more information about this.
 *
 * @mtd:  A pointer to the MTD.
 * @ofs:  Byte address of the block to mark.
 */
static int gpmi_nfc_hook_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	register struct nand_chip  *chip = mtd->priv;
	struct gpmi_nfc_info       *gpmi_info = chip->priv;
	int                        ret;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	gpmi_info->m_u8MarkingBadBlock = 1;
	ret = gpmi_info->hooked_block_markbad(mtd, ofs);
	gpmi_info->m_u8MarkingBadBlock = 0;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);

	return ret;
}

/**
 * gpmi_nfc_ecc_read_oob() - MTD Interface ecc.read_oob().
 *
 * There are several places in this driver where we have to handle the OOB and
 * block marks. This is the function where things are the most complicated, so
 * this is where we try to explain it all. All the other places refer back to
 * here.
 *
 * These are the rules, in order of decreasing importance:
 *
 * 1) Nothing the caller does can be allowed to imperil the block mark, so all
 *    write operations take measures to protect it.
 *
 * 2) In read operations, the first byte of the OOB we return must reflect the
 *    true state of the block mark, no matter where that block mark appears in
 *    the physical page.
 *
 * 3) ECC-based read operations return an OOB full of set bits (since we never
 *    allow ECC-based writes to the OOB, it doesn't matter what ECC-based reads
 *    return).
 *
 * 4) "Raw" read operations return a direct view of the physical bytes in the
 *    page, using the conventional definition of which bytes are data and which
 *    are OOB. This gives the caller a way to see the actual, physical bytes
 *    in the page, without the distortions applied by our ECC engine.
 *
 *
 * What we do for this specific read operation depends on two questions:
 *
 * 1) Are we doing a "raw" read, or an ECC-based read?
 *
 * 2) Are we using block mark swapping or transcription?
 *
 * There are four cases, illustrated by the following Karnaugh map:
 *
 *                    |           Raw           |         ECC-based       |
 *       -------------+-------------------------+-------------------------+
 *                    | Read the conventional   |                         |
 *                    | OOB at the end of the   |                         |
 *       Swapping     | page and return it. It  |                         |
 *                    | contains exactly what   |                         |
 *                    | we want.                | Read the block mark and |
 *       -------------+-------------------------+ return it in a buffer   |
 *                    | Read the conventional   | full of set bits.       |
 *                    | OOB at the end of the   |                         |
 *                    | page and also the block |                         |
 *       Transcribing | mark in the metadata.   |                         |
 *                    | Copy the block mark     |                         |
 *                    | into the first byte of  |                         |
 *                    | the OOB.                |                         |
 *       -------------+-------------------------+-------------------------+
 *
 * Note that we break rule #4 in the Transcribing/Raw case because we're not
 * giving an accurate view of the actual, physical bytes in the page (we're
 * overwriting the block mark). That's OK because it's more important to follow
 * rule #2.
 *
 * It turns out that knowing whether we want an "ECC-based" or "raw" read is not
 * easy. When reading a page, for example, the NAND Flash MTD code calls our
 * ecc.read_page or ecc.read_page_raw function. Thus, the fact that MTD wants an
 * ECC-based or raw view of the page is implicit in which function it calls
 * (there is a similar pair of ECC-based/raw functions for writing).
 *
 * Since MTD assumes the OOB is not covered by ECC, there is no pair of
 * ECC-based/raw functions for reading or or writing the OOB. The fact that the
 * caller wants an ECC-based or raw view of the page is not propagated down to
 * this driver.
 *
 * Since our OOB *is* covered by ECC, we need this information. So, we hook the
 * ecc.read_oob and ecc.write_oob function pointers in the owning
 * struct mtd_info with our own functions. These hook functions set the
 * raw_oob_mode field so that, when control finally arrives here, we'll know
 * what to do.
 *
 * @mtd:     A pointer to the owning MTD.
 * @nand:    A pointer to the owning NAND Flash MTD.
 * @page:    The page number to read.
 * @sndcmd:  Indicates this function should send a command to the chip before
 *           reading the out-of-band bytes. This is only false for small page
 *           chips that support auto-increment.
 */
static int gpmi_nfc_ecc_read_oob(struct mtd_info *mtd, struct nand_chip *nand,
				int page, int sndcmd)
{
	struct gpmi_nfc_info      *gpmi_info = nand->priv;
	int block_mark_column;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	/*
	if (sndcmd) {
		nand->cmdfunc(mtd, NAND_CMD_READ0, 0x00, page);
		sndcmd = 0;
	}
	*/

	/*
	 * First, fill in the OOB buffer. If we're doing a raw read, we need to
	 * get the bytes from the physical page. If we're not doing a raw read,
	 * we need to fill the buffer with set bits.
	 */
	if (gpmi_info->m_u8RawOOBMode) {
		/*
		 * If control arrives here, we're doing a "raw" read. Send the
		 * command to read the conventional OOB.
		 */
		nand->cmdfunc(mtd, NAND_CMD_READ0,
				mtd->writesize, page);

		/* Read out the conventional OOB. */
		nand->read_buf(mtd, nand->oob_poi, mtd->oobsize);
	} else {
		/*
		 * If control arrives here, we're not doing a "raw" read. Fill
		 * the OOB buffer with set bits.
		 */
		memset(nand->oob_poi, ~0, mtd->oobsize);
	}

	/*
	 * Now, we want to make sure the block mark is correct. In the
	 * Swapping/Raw case, we already have it. Otherwise, we need to
	 * explicitly read it.
	 */
#ifdef CONFIG_GPMI_NFC_SWAP_BLOCK_MARK
	if (!gpmi_info->m_u8RawOOBMode) {
		/* First, figure out where the block mark is. */
		block_mark_column = mtd->writesize;
#else
	{
		/* First, figure out where the block mark is. */
		block_mark_column = 0;
#endif
		/* Send the command to read the block mark. */
		nand->cmdfunc(mtd, NAND_CMD_READ0, block_mark_column, page);

		/* Read the block mark into the first byte of the OOB buffer. */
		nand->oob_poi[0] = nand->read_byte(mtd);
	}

	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);
	return 0;

}

/**
 * gpmi_nfc_ecc_write_oob() - MTD Interface ecc.write_oob().
 *
 * @mtd:   A pointer to the owning MTD.
 * @nand:  A pointer to the owning NAND Flash MTD.
 * @page:  The page number to write.
 */
static int gpmi_nfc_ecc_write_oob(struct mtd_info *mtd,
				struct nand_chip *nand, int page)
{
	struct gpmi_nfc_info      *gpmi_info = nand->priv;
	uint8_t                   block_mark = 0;
	int                       block_mark_column;
	int                       status;
	int                       error = 0;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);
	/*
	 * There are fundamental incompatibilities between the i.MX GPMI NFC and
	 * the NAND Flash MTD model that make it essentially impossible to write
	 * the out-of-band bytes.
	 *
	 * We permit *ONE* exception. If the *intent* of writing the OOB is to
	 * mark a block bad, we can do that.
	 */

	if (gpmi_info->m_u8MarkingBadBlock) {
		printf("This driver doesn't support writing the OOB\n");
		error = -EIO;
		goto exit;
	}

	/*
	 * If control arrives here, we're marking a block bad. First, figure out
	 * where the block mark is.
	 *
	 * If we're using swapping, the block mark is in the conventional
	 * location. Otherwise, we're using transcription, and the block mark
	 * appears in the first byte of the page.
	 */
#ifdef CONFIG_GPMI_NFC_SWAP_BLOCK_MARK
	block_mark_column = mtd->writesize;
#else
	block_mark_column = 0;
#endif

	/* Write the block mark. */
	nand->cmdfunc(mtd, NAND_CMD_SEQIN, block_mark_column, page);
	nand->write_buf(mtd, &block_mark, 1);
	nand->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);

	status = nand->waitfunc(mtd, nand);

	/* Check if it worked. */
	if (status & NAND_STATUS_FAIL)
		error = -EIO;

	/* Return. */
exit:
	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);

	return error;
}

/**
 * gpmi_nfc_block_bad - Claims all blocks are good.
 *
 * In principle, this function is *only* called when the NAND Flash MTD system
 * isn't allowed to keep an in-memory bad block table, so it is forced to ask
 * the driver for bad block information.
 *
 * In fact, we permit the NAND Flash MTD system to have an in-memory BBT, so
 * this function is *only* called when we take it away.
 *
 * We take away the in-memory BBT when the user sets the "ignorebad" parameter,
 * which indicates that all blocks should be reported good.
 *
 * Thus, this function is only called when we want *all* blocks to look good,
 * so it *always* return success.
 *
 * @mtd:      Ignored.
 * @ofs:      Ignored.
 * @getchip:  Ignored.
 */
static int gpmi_nfc_block_bad(struct mtd_info *mtd, loff_t ofs, int getchip)
{
	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);
	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);
	return 0;
}

#ifndef CONFIG_GPMI_NFC_SWAP_BLOCK_MARK
/**
 * gpmi_nfc_pre_bbt_scan() - Prepare for the BBT scan.
 *
 * @this:  Per-device data.
 */
static int gpmi_nfc_pre_bbt_scan(struct gpmi_nfc_info *this)
{
	/* Not implemented yet */
	/* Return success. */
	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);
	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);
	return 0;
}
#endif

#define round_down(x, y) ((x) & ~(y - 1))

/*
 *  Calculate the ECC strength by hand:
 *	E : The ECC strength.
 *	G : the length of Galois Field.
 *	N : The chunk count of per page.
 *	O : the oobsize of the NAND chip.
 *	M : the metasize of per page.
 *
 *	The formula is :
 *		E * G * N
 *	      ------------ <= (O - M)
 *                  8
 *
 *      So, we get E by:
 *                    (O - M) * 8
 *              E <= -------------
 *                       G * N
 */
static inline int get_ecc_strength(struct gpmi_nfc_info *geo,
					struct mtd_info *mtd)
{
	int ecc_strength;

	ecc_strength = ((mtd->oobsize - geo->metadata_size) * 8)
			/ (geo->gf_len * geo->ecc_chunk_count);

	/* We need the minor even number. */
	return round_down(ecc_strength, 2);
}

int common_nfc_set_geometry(struct gpmi_nfc_info *geo, struct mtd_info *mtd)
{
	unsigned int metadata_size;
	unsigned int status_size;
	unsigned int block_mark_bit_offset;

	/*
	 * The size of the metadata can be changed, though we set it to 10
	 * bytes now. But it can't be too large, because we have to save
	 * enough space for BCH.
	 */
	geo->metadata_size = 10;

	/* The default for the length of Galois Field. */
	geo->gf_len = 13;

	/* The default for chunk size. There is no oobsize greater then 512. */
	geo->ecc_chunk_size = 512;
	while (geo->ecc_chunk_size < mtd->oobsize)
		geo->ecc_chunk_size *= 2; /* keep C >= O */

	geo->ecc_chunk_count = mtd->writesize / geo->ecc_chunk_size;

	/* We use the same ECC strength for all chunks. */
	geo->ecc_strength = get_ecc_strength(geo, mtd);
	if (!geo->ecc_strength) {
		printf("We get a wrong ECC strength.\n");
		return -EINVAL;
	}

	/*
	 * The auxiliary buffer contains the metadata and the ECC status. The
	 * metadata is padded to the nearest 32-bit boundary. The ECC status
	 * contains one byte for every ECC chunk, and is also padded to the
	 * nearest 32-bit boundary.
	 */
	metadata_size = ALIGN(geo->metadata_size, 4);
	status_size   = ALIGN(geo->ecc_chunk_count, 4);

	geo->auxiliary_size = metadata_size + status_size;
	geo->auxiliary_status_offset = metadata_size;

#ifndef CONFIG_GPMI_NFC_SWAP_BLOCK_MARK
	return 0;
#endif

	/*
	 * We need to compute the byte and bit offsets of
	 * the physical block mark within the ECC-based view of the page.
	 *
	 * NAND chip with 2K page shows below:
	 *                                             (Block Mark)
	 *                                                   |      |
	 *                                                   |  D   |
	 *                                                   |<---->|
	 *                                                   V      V
	 *    +---+----------+-+----------+-+----------+-+----------+-+
	 *    | M |   data   |E|   data   |E|   data   |E|   data   |E|
	 *    +---+----------+-+----------+-+----------+-+----------+-+
	 *
	 * The position of block mark moves forward in the ECC-based view
	 * of page, and the delta is:
	 *
	 *                   E * G * (N - 1)
	 *             D = (---------------- + M)
	 *                          8
	 *
	 * With the formula to compute the ECC strength, and the condition
	 *       : C >= O         (C is the ecc chunk size)
	 *
	 * It's easy to deduce to the following result:
	 *
	 *         E * G       (O - M)      C - M         C - M
	 *      ----------- <= ------- <=  --------  <  ---------
	 *           8            N           N          (N - 1)
	 *
	 *  So, we get:
	 *
	 *                   E * G * (N - 1)
	 *             D = (---------------- + M) < C
	 *                          8
	 *
	 *  The above inequality means the position of block mark
	 *  within the ECC-based view of the page is still in the data chunk,
	 *  and it's NOT in the ECC bits of the chunk.
	 *
	 *  Use the following to compute the bit position of the
	 *  physical block mark within the ECC-based view of the page:
	 *          (page_size - D) * 8
	 */
	block_mark_bit_offset = mtd->writesize * 8 -
		(geo->ecc_strength * geo->gf_len * (geo->ecc_chunk_count - 1)
				+ geo->metadata_size * 8);

	geo->block_mark_byte_offset = block_mark_bit_offset / 8;
	geo->block_mark_bit_offset  = block_mark_bit_offset % 8;
	return 0;
}

/**
 * gpmi_nfc_scan_bbt() - MTD Interface scan_bbt().
 *
 * The HIL calls this function once, when it initializes the NAND Flash MTD.
 *
 * Nominally, the purpose of this function is to look for or create the bad
 * block table. In fact, since the HIL calls this function at the very end of
 * the initialization process started by nand_scan(), and the HIL doesn't have a
 * more formal mechanism, everyone "hooks" this function to continue the
 * initialization process.
 *
 * At this point, the physical NAND Flash chips have been identified and
 * counted, so we know the physical geometry. This enables us to make some
 * important configuration decisions.
 *
 * The return value of this function propogates directly back to this driver's
 * call to nand_scan(). Anything other than zero will cause this driver to
 * tear everything down and declare failure.
 *
 * @mtd:  A pointer to the owning MTD.
 */
static int gpmi_nfc_scan_bbt(struct mtd_info *mtd)
{
	struct nand_chip        *nand = mtd->priv;
	struct gpmi_nfc_info    *gpmi_info = nand->priv;
	struct nfc_hal *nfc = gpmi_info->nfc;
	struct gpmi_nfc_timing  timing;
	int                     error;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	/* Set nfc geo */
	common_nfc_set_geometry(gpmi_info, mtd);
	nfc->set_geometry(mtd);

	/*
	 * The "safe" GPMI timing that should succeed
	 * with any NAND Flash device
	 * (although, with less-than-optimal performance).
	 */
	timing.m_u8DataSetup	= 80;
	timing.m_u8DataHold	= 60;
	timing.m_u8AddressSetup	= 25;
	timing.m_u8SampleDelay	=  6;
	timing.m_u8tREA		= -1;
	timing.m_u8tRLOH	= -1;
	timing.m_u8tRHOH	= -1;

	error = nfc->set_timing(mtd, &timing);

	if (error)
		return error;

#ifndef CONFIG_GPMI_NFC_SWAP_BLOCK_MARK
	/* Prepare for the BBT scan. */
	error = gpmi_nfc_pre_bbt_scan(gpmi_info);

	if (error)
		return error;
#endif

	/*
	 * Hook some operations at the MTD level. See the descriptions of the
	 * saved function pointer fields for details about why we hook these.
	 */
	gpmi_info->hooked_read_oob = mtd->read_oob;
	mtd->read_oob              = gpmi_nfc_hook_read_oob;

	gpmi_info->hooked_write_oob = mtd->write_oob;
	mtd->write_oob              = gpmi_nfc_hook_write_oob;

	gpmi_info->hooked_block_markbad = mtd->block_markbad;
	mtd->block_markbad              = gpmi_nfc_hook_block_markbad;

	/* We use the reference implementation for bad block management. */
	error = nand_default_bbt(mtd);
	if (error)
		return error;

	/* Return success. */
	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);

	return 0;

}

static int gpmi_nfc_alloc_buf(struct gpmi_nfc_info *gpmi_info)
{
	int err = 0;
	u8 *pBuf = NULL;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

#ifdef CONFIG_ARCH_MMU
	pBuf = (u8 *)ioremap_nocache(iomem_to_phys((ulong)memalign(MXS_DMA_ALIGNMENT,
		NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE)),
		MXS_DMA_ALIGNMENT);
#else
	pBuf = (u8 *)memalign(MXS_DMA_ALIGNMENT,
				NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE);
#endif
	if (!pBuf) {
		printf("%s: failed to allocate buffer\n", __func__);
		err = -ENOMEM;
		return err;
	}
	memset(pBuf, 0, NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE);

	gpmi_info->data_buf = pBuf;
	gpmi_info->oob_buf  = pBuf + NAND_MAX_PAGESIZE;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);

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
	struct gpmi_nfc_info *gpmi_info;
	struct nand_chip *chip = nand;
	struct nfc_hal *nfc;
	static struct nand_ecclayout fake_ecc_layout;
	int err;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	gpmi_info = kmalloc(sizeof(struct gpmi_nfc_info), GFP_KERNEL);
	if (!gpmi_info) {
		printf("%s: failed to allocate nand_info\n",
		       __func__);
		err = -ENOMEM;
		return err;
	}
	memset(gpmi_info, 0, sizeof(struct gpmi_nfc_info));

	if (gpmi_nfc_alloc_buf(gpmi_info)) {
		err = -ENOMEM;
		return err;
	}

	/* Initialize the NFC HAL. */
	gpmi_info->nfc = &gpmi_nfc_hal;
	nfc = gpmi_info->nfc;
	err = nfc->init();

	memset(&fake_ecc_layout, 0, sizeof(fake_ecc_layout));

	chip->priv = gpmi_info;

	chip->cmd_ctrl		= gpmi_nfc_cmd_ctrl;
	/*
	 * Chip Control
	 *
	 * We rely on the reference implementations of:
	 *     - cmdfunc
	 *     - waitfunc
	 */
	chip->cmdfunc		= NULL;
	chip->waitfunc		= NULL;
	chip->dev_ready		= gpmi_nfc_dev_ready;
	chip->select_chip	= gpmi_nfc_select_chip;
	chip->block_bad		= gpmi_nfc_block_bad;
	chip->block_markbad	= NULL;
	chip->read_byte		= gpmi_nfc_read_byte;
	/*
	 * Low-level I/O
	 *
	 * We don't support a 16-bit NAND Flash bus, so we don't implement
	 * read_word.
	 *
	 * We rely on the reference implentation of verify_buf.
	 */
	chip->read_word		= NULL;
	chip->write_buf		= gpmi_nfc_write_buf;
	chip->read_buf		= gpmi_nfc_read_buf;
	chip->verify_buf	= NULL;
	/*
	 * High-level I/O
	 *
	 * We rely on the reference implementations of:
	 *     - write_page
	 *     - erase_cmd
	 */
	chip->erase_cmd		= NULL;
	chip->write_page	= NULL;
	chip->scan_bbt		= gpmi_nfc_scan_bbt;
	chip->badblock_pattern	= &gpmi_bbt_descr;
	/*
	 * Error Recovery Functions
	 *
	 * We don't fill in the errstat function pointer because it's optional
	 * and we don't have a need for it.
	 */
	chip->errstat		= NULL;
	/*
	 * ECC-aware I/O
	 *
	 * We rely on the reference implementations of:
	 *     - ecc.read_page_raw
	 *     - ecc.write_page_raw
	 */
	chip->ecc.read_page_raw = NULL;
	chip->ecc.write_page_raw = NULL;
	chip->ecc.read_page	= gpmi_nfc_ecc_read_page;
	chip->ecc.layout	= &gpmi_hw_ecclayout;
	/*
	 * Set up NAND Flash options. Specifically:
	 *
	 *     - Disallow partial page writes.
	 */
	chip->options |= NAND_NO_SUBPAGE_WRITE;
	chip->ecc.read_subpage	= NULL;
	chip->ecc.write_page	= gpmi_nfc_ecc_write_page;
	chip->ecc.read_oob	= gpmi_nfc_ecc_read_oob;
	chip->ecc.write_oob	= gpmi_nfc_ecc_write_oob;
	/*
	 * ECC Control
	 *
	 * None of these functions are necessary for us:
	 *     - ecc.hwctl
	 *     - ecc.calculate
	 *     - ecc.correct
	 */
	chip->ecc.calculate	= NULL;
	chip->ecc.correct	= NULL;
	chip->ecc.hwctl		= NULL;
	chip->ecc.layout	= &fake_ecc_layout;
	chip->ecc.mode		= NAND_ECC_HW;
	chip->ecc.bytes		= 9;
	chip->ecc.size		= 512;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);

	return 0;
}
