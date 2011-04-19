/*
 * Copyright (C) 2011 Freescale Semiconductor, Inc.
 *
 * This code is based on drivers/mtd/devices/mxc_m25p80.c in Linux kernel.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <common.h>
#include <spi.h>
#include <spi_flash.h>
#include <asm/errno.h>
#include <linux/types.h>
#include <malloc.h>

#include <imx_spi.h>
#include <imx_spi_nor.h>

static u8 g_tx_buf[256];
static u8 g_rx_buf[256];

#define WRITE_ENABLE(a)		spi_nor_cmd_1byte(a, OPCODE_WREN)
#define	write_enable(a)		WRITE_ENABLE(a)

#define	SPI_FIFOSIZE		24

struct imx_spi_flash_params {
	u32		idcode1;
	u32		block_size;
	u32		block_count;
	u32		device_size;
	u32		page_size;
	const char	*name;
};

struct imx_spi_flash {
	const struct imx_spi_flash_params *params;
	struct spi_flash flash;
};

static inline struct imx_spi_flash *
to_imx_spi_flash(struct spi_flash *flash)
{
	return container_of(flash, struct imx_spi_flash, flash);
}

static const struct imx_spi_flash_params imx_spi_flash_table[] = {
	{
		.idcode1		= 0x20,
		.block_size		= SZ_64K,
		.block_count		= 64,
		.device_size		= SZ_64K * 64,
		.page_size		= 256,
		.name			= "M25P32 - 4MB",
	},
};

static s32 spi_nor_cmd_1byte(struct spi_flash *flash, u8 cmd)
{
	g_tx_buf[0] = cmd;
	g_tx_buf[1] = 0;
	g_tx_buf[2] = 0;
	g_tx_buf[3] = 0;

	if (spi_xfer(flash->spi, (1 << 3), g_tx_buf, g_rx_buf,
			SPI_XFER_BEGIN | SPI_XFER_END) != 0) {
		printf("Error: %s(): %d\n", __func__, __LINE__);
		return -1;
	}
	return 0;
}

static s32 spi_nor_flash_query(struct spi_flash *flash, void* data)
{
	u8 au8Tmp[4] = { 0 };
	u8 *pData = (u8 *)data;

	g_tx_buf[3] = JEDEC_ID;

	if (spi_xfer(flash->spi, (4 << 3), g_tx_buf, au8Tmp,
				SPI_XFER_BEGIN | SPI_XFER_END)) {
		return -1;
	}

	printf("JEDEC ID: 0x%02x:0x%02x:0x%02x\n",
			au8Tmp[2], au8Tmp[1], au8Tmp[0]);

	pData[0] = au8Tmp[2];
	pData[1] = au8Tmp[1];
	pData[2] = au8Tmp[0];

	return 0;
}

static s32 spi_nor_status(struct spi_flash *flash)
{
	g_tx_buf[1] = OPCODE_RDSR;

	if (spi_xfer(flash->spi, 2 << 3, g_tx_buf, g_rx_buf,
			SPI_XFER_BEGIN | SPI_XFER_END) != 0) {
		printf("Error: %s(): %d\n", __func__, __LINE__);
		return 0;
	}
	return g_rx_buf[0];
}

static int wait_till_ready(struct spi_flash *flash)
{
	int sr;
	int times = 10000;

	do {
		sr = spi_nor_status(flash);
		if (sr < 0)
			break;
		else if (!(sr & SR_WIP))
			return 0;

		udelay(1000);

	} while (times--);

	return 1;
}

static int erase_chip(struct spi_flash *flash)
{
	/* Wait until finished previous write command. */
	if (wait_till_ready(flash))
		return 1;

	/* Send write enable, then erase commands. */
	WRITE_ENABLE(flash);

	/* Set up command buffer. */
	g_tx_buf[3] = OPCODE_CHIP_ERASE;

	if (spi_xfer(flash->spi, (4 << 3), g_tx_buf, g_rx_buf,
				SPI_XFER_BEGIN | SPI_XFER_END)) {
		return -1;
	}

	return 0;
}

static int erase_sector(struct spi_flash *flash, u32 offset)
{
	/* Wait until finished previous write command. */
	if (wait_till_ready(flash))
		return 1;

	/* Send write enable, then erase commands. */
	WRITE_ENABLE(flash);

	/* Set up command buffer. */
	g_tx_buf[3] = OPCODE_SE;
	g_tx_buf[2] = offset >> 16;
	g_tx_buf[1] = offset >> 8;
	g_tx_buf[0] = offset;

	if (spi_xfer(flash->spi, (4 << 3), g_tx_buf, g_rx_buf,
				SPI_XFER_BEGIN | SPI_XFER_END)) {
		return -1;
	}

	return 0;
}

static int spi_nor_flash_read(struct spi_flash *flash, u32 from,
		size_t len, void *buf)
{
	struct imx_spi_flash *imx_sf = to_imx_spi_flash(flash);
	int rx_len = 0, count = 0, i = 0;
	int addr, cmd_len;
	u8 txer[SPI_FIFOSIZE] = { 0 };
	u8 *s = txer;
	u8 *d = buf;

	debug("%s: %s 0x%08x, len %zd\n", __func__, "from", (u32)from, len);

	/* sanity checks */
	if (!len)
		return 0;

	if (from + len > imx_sf->params->device_size)
		return -EINVAL;

	/* Wait till previous write/erase is done. */
	if (wait_till_ready(flash)) {
		/* REVISIT status return?? */
		return 1;
	}

	cmd_len = 4;

	addr = from;

	while (len > 0) {

		rx_len = len > (SPI_FIFOSIZE - cmd_len) ?
		    SPI_FIFOSIZE - cmd_len : len;

		/* Set up the write data buffer. */
		txer[3] = OPCODE_NORM_READ;
		txer[2] = addr >> 16;
		txer[1] = addr >> 8;
		txer[0] = addr;

		if (spi_xfer(flash->spi, (roundup(rx_len, 4) + cmd_len) << 3,
			txer, txer,
			SPI_XFER_BEGIN | SPI_XFER_END) != 0) {
			printf("Error: %s(%d): failed\n",
				__FILE__, __LINE__);
			return -1;
		}

		s = txer + cmd_len;

		for (i = rx_len; i >= 0; i -= 4, s += 4) {
			if (i < 4) {
				if (i == 1) {
					*d = s[3];
				} else if (i == 2) {
					*d++ = s[3];
					*d++ = s[2];
				} else if (i == 3) {
					*d++ = s[3];
					*d++ = s[2];
					*d++ = s[1];
				}

				break;
			}

			*d++ = s[3];
			*d++ = s[2];
			*d++ = s[1];
			*d++ = s[0];
		}

		/* updaate */
		len -= rx_len;
		addr += rx_len;
		count += rx_len;

		debug("%s: left:0x%x, from:0x%08x, to:0x%p, done: 0x%x\n",
		      __func__, len, (u32) addr, d, count);
	}

	return 0;
}

static int _fsl_spi_write(struct spi_flash *flash, const void *buf, int len, int addr)
{
	u8 txer[SPI_FIFOSIZE] = { 0 };
	u8 *d = txer;
	u8 *s = (u8 *) buf;
	int delta = 0, l = 0, i = 0, count = 0;

	count = len;
	delta = count % 4;
	if (delta)
		count -= delta;

	while (count) {
		d = txer;
		l = count > (SPI_FIFOSIZE - 4) ?
		    SPI_FIFOSIZE - 4 : count;

		d[3] = OPCODE_PP;
		d[2] = addr >> 16;
		d[1] = addr >> 8;
		d[0] = addr;

		for (i = 0, d += 4; i < l / 4; i++, d += 4) {
			d[3] = *s++;
			d[2] = *s++;
			d[1] = *s++;
			d[0] = *s++;
		}

		debug("WRITEBUF: (%x) %x %x %x\n",
		      txer[3], txer[2], txer[1], txer[0]);

		wait_till_ready(flash);

		write_enable(flash);

		if (spi_xfer(flash->spi, (l + 4) << 3,
				txer, txer,
				SPI_XFER_BEGIN | SPI_XFER_END) != 0) {
			printf("Error: %s(%d): failed\n",
				__FILE__, __LINE__);
			return -1;
		}

		/* update */
		count -= l;
		addr += l;
	}

	if (delta) {
		d = txer;
		/* to keep uninterested bytes untouched */
		for (i = 4; i < 8; i++)
			d[i] = 0xff;

		d[3] = OPCODE_PP;
		d[2] = (addr >> 16) & 0xff;
		d[1] = (addr >> 8) & 0xff;
		d[0] = (addr) & 0xff;

		switch (delta) {
		case 1:
			d[7] = *s++;
			break;
		case 2:
			d[7] = *s++;
			d[6] = *s++;
			break;
		case 3:
			d[7] = *s++;
			d[6] = *s++;
			d[5] = *s++;
			break;
		default:
			break;
		}

		debug("WRITEBUF: (%x) %x %x %x\n",
		      txer[3], txer[2], txer[1], txer[0]);

		wait_till_ready(flash);

		write_enable(flash);

		if (spi_xfer(flash->spi, (4 + 4) << 3,
				txer, txer,
				SPI_XFER_BEGIN | SPI_XFER_END) != 0) {
			printf("Error: %s(%d): failed\n",
				__FILE__, __LINE__);
			return -1;
		}
	}

	return len;
}

static int spi_nor_flash_write(struct spi_flash *flash, u32 to,
		size_t len, const void *buf)
{
	struct imx_spi_flash *imx_sf = to_imx_spi_flash(flash);
	u32 page_offset, page_size;

	/* sanity checks */
	if (!len)
		return 0;

	if (to + len > imx_sf->params->device_size)
		return -EINVAL;

	/* Wait until finished previous write command. */
	if (wait_till_ready(flash))
		return 1;

	write_enable(flash);

	page_offset = to & (imx_sf->params->page_size - 1);

	/* do all the bytes fit onto one page? */
	if (page_offset + len <= imx_sf->params->page_size) {
		_fsl_spi_write(flash, buf, len, to);

	} else {
		u32 i;

		/* the size of data remaining on the first page */
		page_size = imx_sf->params->page_size - page_offset;

		_fsl_spi_write(flash, buf, page_size, to);

		/* write everything in flash->page_size chunks */
		for (i = page_size; i < len; i += page_size) {
			page_size = len - i;
			if (page_size > imx_sf->params->page_size)
				page_size = imx_sf->params->page_size;

			wait_till_ready(flash);

			write_enable(flash);

			_fsl_spi_write(flash, buf + i, page_size, to + i);
			if (page_size % imx_sf->params->page_size == 0)
				printf(".");
		}
	}

	printf("SUCCESS\n\n");

	return 0;
}

static int spi_nor_flash_erase(struct spi_flash *flash, u32 offset,
		size_t len)
{
	struct imx_spi_flash *imx_sf = to_imx_spi_flash(flash);

	/* whole-chip erase? */
	/*
	if (len == imx_sf->params->device_size) {
		if (erase_chip(flash))
			return -EIO;
		else
			return 0;
	*/

	/* REVISIT in some cases we could speed up erasing large regions
	 * by using OPCODE_SE instead of OPCODE_BE_4K.  We may have set up
	 * to use "small sector erase", but that's not always optimal.
	 */

	/* "sector"-at-a-time erase */
	len = roundup(len, imx_sf->params->block_size);
	while (len) {
		if (erase_sector(flash, offset))
			return -EIO;

		offset += imx_sf->params->block_size;
		len -= imx_sf->params->block_size;
	}

	return 0;
}

struct spi_flash *spi_flash_probe(unsigned int bus, unsigned int cs, unsigned int max_hz, unsigned int spi_mode)
{
	struct spi_slave *spi = NULL;
	const struct imx_spi_flash_params *params = NULL;
	struct imx_spi_flash *imx_sf = NULL;
	u8  idcode[4] = { 0 };
	u32 i = 0;
	s32 ret = 0;

	if (CONFIG_SPI_FLASH_CS != cs) {
		printf("Invalid cs for SPI NOR.\n");
		return NULL;
	}

	spi = spi_setup_slave(bus, cs, max_hz, spi_mode);

	if (!spi) {
		debug("SF: Failed to set up slave\n");
		return NULL;
	}

	ret = spi_claim_bus(spi);
	if (ret) {
		debug("SF: Failed to claim SPI bus: %d\n", ret);
		goto err_claim_bus;
	}

	imx_sf = (struct imx_spi_flash *)malloc(sizeof(struct imx_spi_flash));

	if (!imx_sf) {
		debug("SF: Failed to allocate memory\n");
		spi_free_slave(spi);
		return NULL;
	}

	imx_sf->flash.spi = spi;

	/* Read the ID codes */
	ret = spi_nor_flash_query(&(imx_sf->flash), idcode);
	if (ret)
		goto err_read_id;

	for (i = 0; i < ARRAY_SIZE(imx_spi_flash_table); ++i) {
		params = &imx_spi_flash_table[i];
		if (params->idcode1 == idcode[1])
			break;
	}

	if (i == ARRAY_SIZE(imx_spi_flash_table)) {
		debug("SF: Unsupported DataFlash ID %02x\n",
				idcode[1]);

		goto err_invalid_dev;
	}

	imx_sf->params = params;

	imx_sf->flash.name = params->name;
	imx_sf->flash.size = params->device_size;

	imx_sf->flash.read  = spi_nor_flash_read;
	imx_sf->flash.write = spi_nor_flash_write;
	imx_sf->flash.erase = spi_nor_flash_erase;

	debug("SF: Detected %s with block size %lu, "
			"block count %lu, total %u bytes\n",
			params->name,
			params->block_size,
			params->block_count,
			params->device_size);

	return &(imx_sf->flash);

err_read_id:
	spi_release_bus(spi);
err_invalid_dev:
	if (imx_sf)
		free(imx_sf);
err_claim_bus:
	if (spi)
		spi_free_slave(spi);
	return NULL;
}

void spi_flash_free(struct spi_flash *flash)
{
	struct imx_spi_flash *imx_sf = NULL;

	if (!flash)
		return;

	imx_sf = to_imx_spi_flash(flash);

	if (flash->spi) {
		spi_free_slave(flash->spi);
		flash->spi = NULL;
	}

	free(imx_sf);
}
