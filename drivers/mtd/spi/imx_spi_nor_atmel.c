/*
 * (C) Copyright 2008-2010 Freescale Semiconductor, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

#define WRITE_ENABLE(a)			 spi_nor_cmd_1byte(a, WREN)
#define ENABLE_WRITE_STATUS(a)	 spi_nor_cmd_1byte(a, EWSR)

struct imx_spi_flash_params {
	u8		idcode1;
	u32		block_size;
	u32		block_count;
	u32		device_size;
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
		.idcode1		= 0x27,
		.block_size		= SZ_64K,
		.block_count		= 64,
		.device_size		= SZ_64K * 64,
		.name			= "AT45DB321D - 4MB",
	},
};

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
	g_tx_buf[1] = STAT_READ;

	if (spi_xfer(flash->spi, 2 << 3, g_tx_buf, g_rx_buf,
			SPI_XFER_BEGIN | SPI_XFER_END) != 0) {
		printf("Error: %s(): %d\n", __func__, __LINE__);
		return 0;
	}
	return g_rx_buf[0];
}

#if 0
/*!
 * Erase a block_size data from block_addr offset in the flash
 */
static int spi_nor_erase_page(struct spi_flash *flash,
				void *page_addr)
{
	u32 addr = (u32)page_addr;

	if ((addr & 512) != 0) {
		printf("Error - page_addr is not "
				"512 Bytes aligned: %p\n",
				page_addr);
		return -1;
	}

	/* now do the block erase */
	if (spi_xfer(flash->spi, 4 << 3, g_tx_buf, g_rx_buf,
			SPI_XFER_BEGIN | SPI_XFER_END) != 0) {
		return -1;
	}

	while (spi_nor_status(flash) & RDSR_BUSY)
		;

	return 0;
}
#endif

static int spi_nor_flash_read(struct spi_flash *flash, u32 offset,
		size_t len, void *buf)
{
	struct imx_spi_flash *imx_sf = to_imx_spi_flash(flash);
	u32 *cmd = (u32 *)g_tx_buf;
	u32 max_rx_sz = (MAX_SPI_BYTES) - 4;
	u8 *d_buf = (u8 *)buf;
	u8 *s_buf;
	s32 s32remain_size = len;
	int i;

	if (!(flash->spi))
		return -1;

	printf("Reading SPI NOR flash 0x%x [0x%x bytes] -> ram 0x%p\n",
		offset, len, buf);
	debug("%s(from flash=0x%08x to ram=%p len=0x%x)\n",
		__func__,
		offset, buf, len);

	if (len == 0)
		return 0;

	*cmd = (READ << 24) | ((u32)offset & 0x00FFFFFF);

	for (; s32remain_size > 0;
			s32remain_size -= max_rx_sz, *cmd += max_rx_sz) {
		debug("Addr:0x%p=>Offset:0x%08x, %d bytes transferred\n",
				d_buf,
				(*cmd & 0x00FFFFFF),
				(len - s32remain_size));
		debug("%d%% completed\n", ((len - s32remain_size) * 100 / len));

		if (s32remain_size < max_rx_sz) {
			debug("100%% completed\n");

			if (spi_xfer(flash->spi, (s32remain_size + 4) << 3,
				g_tx_buf, g_rx_buf,
				SPI_XFER_BEGIN | SPI_XFER_END) != 0) {
				printf("Error: %s(%d): failed\n",
					__FILE__, __LINE__);
				return -1;
			}
			/* throw away 4 bytes (5th received bytes is real) */
			s_buf = g_rx_buf + 4;

			/* now adjust the endianness */
			for (i = s32remain_size; i >= 0; i -= 4, s_buf += 4) {
				if (i < 4) {
					if (i == 1) {
						*d_buf = s_buf[0];
					} else if (i == 2) {
						*d_buf++ = s_buf[1];
						*d_buf++ = s_buf[0];
					} else if (i == 3) {
						*d_buf++ = s_buf[2];
						*d_buf++ = s_buf[1];
						*d_buf++ = s_buf[0];
					}
					printf("SUCCESS\n\n");
					return 0;
				}
				/* copy 4 bytes */
				*d_buf++ = s_buf[3];
				*d_buf++ = s_buf[2];
				*d_buf++ = s_buf[1];
				*d_buf++ = s_buf[0];
			}
		}

		/* now grab max_rx_sz data (+4 is
		*needed due to 4-throw away bytes */
		if (spi_xfer(flash->spi, (max_rx_sz + 4) << 3,
			g_tx_buf, g_rx_buf,
			SPI_XFER_BEGIN | SPI_XFER_END) != 0) {
			printf("Error: %s(%d): failed\n", __FILE__, __LINE__);
			return -1;
		}
		/* throw away 4 bytes (5th received bytes is real) */
		s_buf = g_rx_buf + 4;
		/* now adjust the endianness */
		for (i = 0; i < max_rx_sz; i += 4, s_buf += 4) {
			*d_buf++ = s_buf[3];
			*d_buf++ = s_buf[2];
			*d_buf++ = s_buf[1];
			*d_buf++ = s_buf[0];
		}

		if ((s32remain_size % imx_sf->params->block_size) == 0)
			printf(".");
	}
	printf("SUCCESS\n\n");

	return -1;
}

static int spi_nor_flash_write(struct spi_flash *flash, u32 offset,
		size_t len, const void *buf)
{
	u32 d_addr = offset;
	u8 *s_buf = (u8 *)buf;
	unsigned int final_addr = 0;
	int page_size = 528, trans_bytes = 0, buf_ptr = 0,
		bytes_sent = 0, byte_sent_per_iter = 0;
	int page_no = 0, buf_addr = 0, page_off = 0,
		i = 0, j = 0, k = 0, fifo_size = 32;
	int remain_len = 0;

	if (!(flash->spi))
		return -1;

	if (len == 0)
		return 0;

	printf("Writing SPI NOR flash 0x%x [0x%x bytes] <- ram 0x%p\n",
		offset, len, buf);
	debug("%s(flash addr=0x%08x, ram=%p, len=0x%x)\n",
			__func__, offset, buf, len);

	/* Read the status register to get the Page size */
	if (spi_nor_status(flash) & STAT_PG_SZ) {
		page_size = 512;
	} else {
		puts("Unsupported Page Size of 528 bytes\n");
		g_tx_buf[0] = CONFIG_REG4;
		g_tx_buf[1] = CONFIG_REG3;
		g_tx_buf[2] = CONFIG_REG2;
		g_tx_buf[3] = CONFIG_REG1;

		if (spi_xfer(flash->spi, 4 << 3, g_tx_buf, g_rx_buf,
				SPI_XFER_BEGIN | SPI_XFER_END) != 0) {
			printf("Error: %s(): %d", __func__, __LINE__);
			return -1;
		}

		while (!(spi_nor_status(flash) & STAT_BUSY))
			;

		puts("Reprogrammed the Page Size to 512 bytes\n");
		puts("Please Power Cycle the board for the change to take effect\n");

		return -1;
	}

	/* Due to the way CSPI operates send data less
	   that 4 bytes in a different manner */
	remain_len = len % 4;
	if (remain_len)
		len -= remain_len;

	while (1) {
		page_no = d_addr / page_size;
		/* Get the offset within the page
		if address is not page-aligned */
		page_off = (d_addr % page_size);
		if (page_off) {
			if (page_no == 0)
				buf_addr = d_addr;
			else
				buf_addr = page_off;

			trans_bytes = page_size - buf_addr;
		} else {
			buf_addr = 0;
			trans_bytes = page_size;
		}

		if (len <= 0)
			break;

		if (trans_bytes > len)
			trans_bytes = len;

		bytes_sent = trans_bytes;
		/* Write the data to the SPI-NOR Buffer first */
		while (trans_bytes > 0) {
			final_addr = (buf_addr & 0x3FF);
			g_tx_buf[0] = final_addr;
			g_tx_buf[1] = final_addr >> 8;
			g_tx_buf[2] = final_addr >> 16;
			g_tx_buf[3] = BUF1_WR; /*Opcode */

			/* 4 bytes already used for Opcode & address bytes,
			check to ensure we do not overflow the SPI TX buffer */
			if (trans_bytes > (fifo_size - 4))
				byte_sent_per_iter = fifo_size;
			else
				byte_sent_per_iter = trans_bytes + 4;

			for (i = 4; i < byte_sent_per_iter; i += 4) {
				g_tx_buf[i + 3] = s_buf[buf_ptr++];
				g_tx_buf[i + 2] = s_buf[buf_ptr++];
				g_tx_buf[i + 1] = s_buf[buf_ptr++];
				g_tx_buf[i] = s_buf[buf_ptr++];
			}

			if (spi_xfer(flash->spi, byte_sent_per_iter << 3,
					g_tx_buf, g_rx_buf,
					SPI_XFER_BEGIN | SPI_XFER_END) != 0) {
				printf("Error: %s(%d): failed\n",
					__FILE__, __LINE__);
				return -1;
			}

			while (!(spi_nor_status(flash) & STAT_BUSY))
				;

			/* Deduct 4 bytes as it is used for Opcode & address bytes */
			trans_bytes -= (byte_sent_per_iter - 4);
			/* Update the destination buffer address */
			buf_addr += (byte_sent_per_iter - 4);
		}

		/* Send the command to write data from the SPI-NOR Buffer to Flash memory */
		final_addr = (page_size == 512) ? ((page_no & 0x1FFF) << 9) : \
				((page_no & 0x1FFF) << 10);

		/* Specify the Page address in Flash where the data should be written to */
		g_tx_buf[0] = final_addr;
		g_tx_buf[1] = final_addr >> 8;
		g_tx_buf[2] = final_addr >> 16;
		g_tx_buf[3] = BUF1_TO_MEM; /*Opcode */
		if (spi_xfer(flash->spi, 4 << 3, g_tx_buf, g_rx_buf,
				SPI_XFER_BEGIN | SPI_XFER_END) != 0) {
			printf("Error: %s(%d): failed\n", __FILE__, __LINE__);
			return -1;
		}

		while (!(spi_nor_status(flash) & STAT_BUSY))
			;

		d_addr += bytes_sent;
		len -= bytes_sent;
		if (d_addr % (page_size * 50) == 0)
			puts(".");
	}

	if (remain_len) {
		buf_ptr += remain_len;
		/* Write the remaining data bytes first */
		for (i = 0; i < remain_len; ++i)
			g_tx_buf[i] = s_buf[buf_ptr--];

		/* Write the address bytes next in the same word
		as the data byte from the next byte */
		for (j = i, k = 0; j < 4; j++, k++)
			g_tx_buf[j] = final_addr >> (k * 8);

		/* Write the remaining address bytes in the next word */
		j = 0;
		final_addr = (buf_addr & 0x3FF);

		for (j = 0; k < 3; j++, k++)
			g_tx_buf[j] = final_addr >> (k * 8);

		/* Finally the Opcode to write the data to the buffer */
		g_tx_buf[j] = BUF1_WR; /*Opcode */

		if (spi_xfer(flash->spi, (remain_len + 4) << 3,
			g_tx_buf, g_rx_buf,
			SPI_XFER_BEGIN | SPI_XFER_END) != 0) {
			printf("Error: %s(%d): failed\n", __FILE__, __LINE__);
			return -1;
		}

		while (!(spi_nor_status(flash) & STAT_BUSY))
			;

		if (page_size == 512)
			final_addr = (page_no & 0x1FFF) << 9;
		else
			final_addr = (page_no & 0x1FFF) << 10;

		g_tx_buf[0] = final_addr;
		g_tx_buf[1] = final_addr >> 8;
		g_tx_buf[2] = final_addr >> 16;
		g_tx_buf[3] = BUF1_TO_MEM; /*Opcode */
		if (spi_xfer(flash->spi, 4 << 3, g_tx_buf, g_rx_buf,
			SPI_XFER_BEGIN | SPI_XFER_END) != 0) {
			printf("Error: %s(%d): failed\n", __FILE__, __LINE__);
				return -1;
		}

		while (!(spi_nor_status(flash) & STAT_BUSY))
				;
	}

	printf("SUCCESS\n\n");

	return 0;
}

static int spi_nor_flash_erase(struct spi_flash *flash, u32 offset,
		size_t len)
{
	printf("Erase is built in program.\n");

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

