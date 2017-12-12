/*
 * Copyright (C) 2014 Gateworks Corporation
 * Author: Tim Harvey <tharvey@gateworks.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <nand.h>
#include <malloc.h>

static struct mtd_info *mtd;
static struct nand_chip nand_chip;

static void mxs_nand_command(struct mtd_info *mtd, unsigned int command,
			     int column, int page_addr)
{
	register struct nand_chip *chip = mtd_to_nand(mtd);
	u32 timeo, time_start;

	/* write out the command to the device */
	chip->cmd_ctrl(mtd, command, NAND_CLE);

	/* Serially input address */
	if (column != -1) {
		chip->cmd_ctrl(mtd, column, NAND_ALE);
		chip->cmd_ctrl(mtd, column >> 8, NAND_ALE);
	}
	if (page_addr != -1) {
		chip->cmd_ctrl(mtd, page_addr, NAND_ALE);
		chip->cmd_ctrl(mtd, page_addr >> 8, NAND_ALE);
		/* One more address cycle for devices > 128MiB */
		if (chip->chipsize > (128 << 20))
			chip->cmd_ctrl(mtd, page_addr >> 16, NAND_ALE);
	}
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, 0);

	if (command == NAND_CMD_READ0) {
		chip->cmd_ctrl(mtd, NAND_CMD_READSTART, NAND_CLE);
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, 0);
	} else if (command == NAND_CMD_RNDOUT) {
		/* No ready / busy check necessary */
		chip->cmd_ctrl(mtd, NAND_CMD_RNDOUTSTART,
			       NAND_NCE | NAND_CLE);
		chip->cmd_ctrl(mtd, NAND_CMD_NONE,
			       NAND_NCE);
	}

	/* wait for nand ready */
	ndelay(100);
	timeo = (CONFIG_SYS_HZ * 20) / 1000;
	time_start = get_timer(0);
	while (get_timer(time_start) < timeo) {
		if (chip->dev_ready(mtd))
			break;
	}
}

static u16 onfi_crc16(u16 crc, u8 const *p, size_t len)
{
	int i;
	while (len--) {
		crc ^= *p++ << 8;
		for (i = 0; i < 8; i++)
			crc = (crc << 1) ^ ((crc & 0x8000) ? 0x8005 : 0);
	}

	return crc;
}

/* Parse the Extended Parameter Page. */
static int nand_flash_detect_ext_param_page(struct mtd_info *mtd,
		struct nand_chip *chip, struct nand_onfi_params *p)
{
	struct onfi_ext_param_page *ep;
	struct onfi_ext_section *s;
	struct onfi_ext_ecc_info *ecc;
	uint8_t *cursor;
	int ret = -EINVAL;
	int len;
	int i;

	len = le16_to_cpu(p->ext_param_page_length) * 16;
	ep = malloc(len);
	if (!ep) {
		printf("can't malloc memory 0x%x\n", len);
		return -ENOMEM;
	}

	/* Send our own NAND_CMD_PARAM. */
	chip->cmdfunc(mtd, NAND_CMD_PARAM, 0, -1);

	/* Use the Change Read Column command to skip the ONFI param pages. */
	chip->cmdfunc(mtd, NAND_CMD_RNDOUT,
			sizeof(*p) * p->num_of_param_pages , -1);

	/* Read out the Extended Parameter Page. */
	chip->read_buf(mtd, (uint8_t *)ep, len);
	if ((onfi_crc16(ONFI_CRC_BASE, ((uint8_t *)ep) + 2, len - 2)
		!= le16_to_cpu(ep->crc))) {
		printf("fail in the CRC.\n");
		goto ext_out;
	}

	/*
	 * Check the signature.
	 * Do not strictly follow the ONFI spec, maybe changed in future.
	 */
	if (strncmp((char *)ep->sig, "EPPS", 4)) {
		printf("The signature is invalid.\n");
		goto ext_out;
	}

	/* find the ECC section. */
	cursor = (uint8_t *)(ep + 1);
	for (i = 0; i < ONFI_EXT_SECTION_MAX; i++) {
		s = ep->sections + i;
		if (s->type == ONFI_SECTION_TYPE_2)
			break;
		cursor += s->length * 16;
	}
	if (i == ONFI_EXT_SECTION_MAX) {
		printf("We can not find the ECC section.\n");
		goto ext_out;
	}

	/* get the info we want. */
	ecc = (struct onfi_ext_ecc_info *)cursor;

	if (!ecc->codeword_size) {
		printf("Invalid codeword size\n");
		goto ext_out;
	}

	chip->ecc_strength_ds = ecc->ecc_bits;
	chip->ecc_step_ds = 1 << ecc->codeword_size;
	ret = 0;

ext_out:
	free(ep);
	return ret;
}


static int mxs_flash_ident(struct mtd_info *mtd)
{
	register struct nand_chip *chip = mtd_to_nand(mtd);
	int i, val;
	u8 mfg_id, dev_id;
	u8 id_data[8];
	struct nand_onfi_params *p = &chip->onfi_params;

	/* Reset the chip */
	chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);

	/* Send the command for reading device ID */
	chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);

	/* Read manufacturer and device IDs */
	mfg_id = chip->read_byte(mtd);
	dev_id = chip->read_byte(mtd);

	/* Try again to make sure */
	chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);
	for (i = 0; i < 8; i++)
		id_data[i] = chip->read_byte(mtd);
	if (id_data[0] != mfg_id || id_data[1] != dev_id) {
		printf("second ID read did not match");
		return -1;
	}
	debug("0x%02x:0x%02x ", mfg_id, dev_id);

	/* read ONFI */
	chip->onfi_version = 0;
	chip->cmdfunc(mtd, NAND_CMD_READID, 0x20, -1);
	if (chip->read_byte(mtd) != 'O' || chip->read_byte(mtd) != 'N' ||
	    chip->read_byte(mtd) != 'F' || chip->read_byte(mtd) != 'I') {
		return -2;
	}

	/* we have ONFI, probe it */
	chip->cmdfunc(mtd, NAND_CMD_PARAM, 0, -1);
	chip->read_buf(mtd, (uint8_t *)p, sizeof(*p));
	mtd->name = p->model;
	mtd->writesize = le32_to_cpu(p->byte_per_page);
	mtd->erasesize = le32_to_cpu(p->pages_per_block) * mtd->writesize;
	mtd->oobsize = le16_to_cpu(p->spare_bytes_per_page);
	chip->chipsize = le32_to_cpu(p->blocks_per_lun);
	chip->chipsize *= (uint64_t)mtd->erasesize * p->lun_count;
	/* Calculate the address shift from the page size */
	chip->page_shift = ffs(mtd->writesize) - 1;
	chip->phys_erase_shift = ffs(mtd->erasesize) - 1;
	/* Convert chipsize to number of pages per chip -1 */
	chip->pagemask = (chip->chipsize >> chip->page_shift) - 1;
	chip->badblockbits = 8;

	/* Check version */
	val = le16_to_cpu(p->revision);
	if (val & (1 << 5))
		chip->onfi_version = 23;
	else if (val & (1 << 4))
		chip->onfi_version = 22;
	else if (val & (1 << 3))
		chip->onfi_version = 21;
	else if (val & (1 << 2))
		chip->onfi_version = 20;
	else if (val & (1 << 1))
		chip->onfi_version = 10;

	if (!chip->onfi_version) {
		printf("unsupported ONFI version: %d\n", val);
		return 0;
	}

	if (p->ecc_bits != 0xff) {
		chip->ecc_strength_ds = p->ecc_bits;
		chip->ecc_step_ds = 512;
	} else if (chip->onfi_version >= 21 &&
		(onfi_feature(chip) & ONFI_FEATURE_EXT_PARAM_PAGE)) {

		if (nand_flash_detect_ext_param_page(mtd, chip, p))
			printf("Failed to detect ONFI extended param page\n");
	} else {
		printf("Could not retrieve ONFI ECC requirements\n");
	}

	debug("ecc_strength_ds %u, ecc_step_ds %u\n", chip->ecc_strength_ds, chip->ecc_step_ds);
	debug("erasesize=%d (>>%d)\n", mtd->erasesize, chip->phys_erase_shift);
	debug("writesize=%d (>>%d)\n", mtd->writesize, chip->page_shift);
	debug("oobsize=%d\n", mtd->oobsize);
	debug("chipsize=%lld\n", chip->chipsize);

	return 0;
}

static int mxs_read_page_ecc(struct mtd_info *mtd, void *buf, unsigned int page)
{
	register struct nand_chip *chip = mtd_to_nand(mtd);
	int ret;

	chip->cmdfunc(mtd, NAND_CMD_READ0, 0x0, page);
	ret = nand_chip.ecc.read_page(mtd, chip, buf, 1, page);
	if (ret < 0) {
		printf("read_page failed %d\n", ret);
		return -1;
	}
	return 0;
}

static int is_badblock(struct mtd_info *mtd, loff_t offs, int allowbbt)
{
	register struct nand_chip *chip = mtd_to_nand(mtd);
	unsigned int block = offs >> chip->phys_erase_shift;
	unsigned int page = offs >> chip->page_shift;

	debug("%s offs=0x%08x block:%d page:%d\n", __func__, (int)offs, block,
	      page);
	chip->cmdfunc(mtd, NAND_CMD_READ0, mtd->writesize, page);
	memset(chip->oob_poi, 0, mtd->oobsize);
	chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);

	return chip->oob_poi[0] != 0xff;
}

/* setup mtd and nand structs and init mxs_nand driver */
static int mxs_nand_init(void)
{
	/* return if already initalized */
	if (nand_chip.numchips)
		return 0;

	/* init mxs nand driver */
	board_nand_init(&nand_chip);
	mtd = nand_to_mtd(&nand_chip);
	/* set mtd functions */
	nand_chip.cmdfunc = mxs_nand_command;
	nand_chip.numchips = 1;

	/* identify flash device */
	puts(": ");
	if (mxs_flash_ident(mtd)) {
		printf("Failed to identify\n");
		return -1;
	}

	/* allocate and initialize buffers */
	nand_chip.buffers = memalign(ARCH_DMA_MINALIGN,
				     sizeof(*nand_chip.buffers));
	nand_chip.oob_poi = nand_chip.buffers->databuf + mtd->writesize;
	/* setup flash layout (does not scan as we override that) */
	mtd->size = nand_chip.chipsize;
	nand_chip.scan_bbt(mtd);

	printf("%llu MiB\n", (mtd->size / (1024 * 1024)));
	return 0;
}

int nand_spl_load_image(uint32_t offs, unsigned int size, void *buf)
{
	struct nand_chip *chip;
	unsigned int page;
	unsigned int nand_page_per_block;
	unsigned int sz = 0;
	uint8_t *page_buf = NULL;
	uint32_t page_off;

	if (mxs_nand_init())
		return -ENODEV;

	chip = mtd_to_nand(mtd);

	page_buf = malloc(mtd->writesize);
	if (!page_buf)
		return -ENOMEM;

	page = offs >> chip->page_shift;
	page_off = offs & (mtd->writesize - 1);
	nand_page_per_block = mtd->erasesize / mtd->writesize;

	debug("%s offset:0x%08x len:%d page:%d\n", __func__, offs, size, page);

	while (size) {
		if (mxs_read_page_ecc(mtd, page_buf, page) < 0)
			return -1;

		if (size > (mtd->writesize - page_off))
			sz = (mtd->writesize - page_off);
		else
			sz = size;

		memcpy(buf, page_buf + page_off, sz);

		offs += mtd->writesize;
		page++;
		buf += (mtd->writesize - page_off);
		page_off = 0;
		size -= sz;

		/*
		 * Check if we have crossed a block boundary, and if so
		 * check for bad block.
		 */
		if (!(page % nand_page_per_block)) {
			/*
			 * Yes, new block. See if this block is good. If not,
			 * loop until we find a good block.
			 */
			while (is_badblock(mtd, offs, 1)) {
				page = page + nand_page_per_block;
				/* Check i we've reached the end of flash. */
				if (page >= mtd->size >> chip->page_shift) {
					free(page_buf);
					return -ENOMEM;
				}
			}
		}
	}

	free(page_buf);

	return 0;
}

int nand_default_bbt(struct mtd_info *mtd)
{
	return 0;
}

void nand_init(void)
{
}

void nand_deselect(void)
{
}

