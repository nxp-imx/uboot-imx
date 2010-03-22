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
#define WRITE_DISABLE(a)		 spi_nor_cmd_1byte(a, WRDI)
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
		.idcode1		= 0x25,
		.block_size		= SZ_64K,
		.block_count		= 32,
		.device_size		= SZ_64K * 32,
		.name			= "SST25VF016B - 2MB",
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

static s32 spi_nor_cmd_1byte(struct spi_flash *flash, u8 cmd)
{
	g_tx_buf[0] = cmd;

	if (spi_xfer(flash->spi, (1 << 3), g_tx_buf, g_rx_buf,
			SPI_XFER_BEGIN | SPI_XFER_END) != 0) {
		printf("Error: %s(): %d\n", __func__, __LINE__);
		return -1;
	}
	return 0;
}

static s32 spi_nor_status(struct spi_flash *flash)
{
	g_tx_buf[1] = RDSR;

	if (spi_xfer(flash->spi, 2 << 3, g_tx_buf, g_rx_buf,
			SPI_XFER_BEGIN | SPI_XFER_END) != 0) {
		printf("Error: %s(): %d\n", __func__, __LINE__);
		return 0;
	}
	return g_rx_buf[0];
}

static int spi_nor_program_1byte(struct spi_flash *flash,
		u8 data, void *addr)
{
	u32 addr_val = (u32)addr;

	/* need to do write-enable command */
	if (WRITE_ENABLE(flash) != 0) {
		printf("Error : %d\n", __LINE__);
		return -1;
	}
	g_tx_buf[0] = BYTE_PROG; /* need to skip bytes 1, 2, 3 */
	g_tx_buf[4] = data;
	g_tx_buf[5] = addr_val & 0xFF;
	g_tx_buf[6] = (addr_val >> 8) & 0xFF;
	g_tx_buf[7] = (addr_val >> 16) & 0xFF;

	debug("0x%x: 0x%x\n", *(u32 *)g_tx_buf, *(u32 *)(g_tx_buf + 4));
	debug("addr=0x%x\n", addr_val);

	if (spi_xfer(flash->spi, 5 << 3, g_tx_buf, g_rx_buf,
			SPI_XFER_BEGIN | SPI_XFER_END) != 0) {
		printf("Error: %s(%d): failed\n", __FILE__, __LINE__);
		return -1;
	}

	while (spi_nor_status(flash) & RDSR_BUSY)
		;

	return 0;
}

/*!
 * Write 'val' to flash WRSR (write status register)
 */
static int spi_nor_write_status(struct spi_flash *flash, u8 val)
{
	g_tx_buf[0] = val;
	g_tx_buf[1] = WRSR;

	if (spi_xfer(flash->spi, 2 << 3, g_tx_buf, g_rx_buf,
			SPI_XFER_BEGIN | SPI_XFER_END) != 0) {
		printf("Error: %s(): %d\n", __func__, __LINE__);
		return -1;
	}
	return 0;
}

/*!
 * Erase a block_size data from block_addr offset in the flash
 */
static int spi_nor_erase_block(struct spi_flash *flash,
				void *block_addr, u32 block_size)
{
	u32 *cmd = (u32 *)g_tx_buf;
	u32 addr = (u32) block_addr;

	if (block_size != SZ_64K &&
		block_size != SZ_32K &&
		block_size != SZ_4K) {
		printf("Error - block_size is not "
				"4kB, 32kB or 64kB: 0x%x\n",
				block_size);
		return -1;
	}

	if ((addr & (block_size - 1)) != 0) {
		printf("Error - block_addr is not "
				"4kB, 32kB or 64kB aligned: %p\n",
				block_addr);
		return -1;
	}

	if (ENABLE_WRITE_STATUS(flash) != 0 ||
			spi_nor_write_status(flash, 0) != 0) {
		printf("Error: %s: %d\n", __func__, __LINE__);
		return -1;
	}

	/* need to do write-enable command */
	if (WRITE_ENABLE(flash) != 0) {
		printf("Error : %d\n", __LINE__);
		return -1;
	}

	if (block_size == SZ_64K)
		*cmd = (ERASE_64K << 24) | (addr & 0x00FFFFFF);
	else if (block_size == SZ_32K)
		*cmd = (ERASE_32K << 24) | (addr & 0x00FFFFFF);
	else if (block_size == SZ_4K)
		*cmd = (ERASE_4K << 24) | (addr & 0x00FFFFFF);

	/* now do the block erase */
	if (spi_xfer(flash->spi, 4 << 3, g_tx_buf, g_rx_buf,
			SPI_XFER_BEGIN | SPI_XFER_END) != 0) {
		return -1;
	}

	while (spi_nor_status(flash) & RDSR_BUSY)
		;

	return 0;
}

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

	for (; s32remain_size > 0; s32remain_size -= max_rx_sz, *cmd += max_rx_sz) {
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
				printf("Error: %s(%d): failed\n", __FILE__, __LINE__);
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
			g_tx_buf, g_rx_buf, SPI_XFER_BEGIN | SPI_XFER_END) != 0) {
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
	struct imx_spi_flash *imx_sf = to_imx_spi_flash(flash);
	u32 d_addr = offset;
	u8 *s_buf = (u8 *)buf;
	s32 s32remain_size = len;

	if (!(flash->spi))
		return -1;

	if (len == 0)
		return 0;

	printf("Writing SPI NOR flash 0x%x [0x%x bytes] <- ram 0x%p\n",
		offset, len, buf);
	debug("%s(flash addr=0x%08x, ram=%p, len=0x%x)\n",
			__func__, offset, buf, len);

	if (ENABLE_WRITE_STATUS(flash) != 0 ||
			spi_nor_write_status(flash, 0) != 0) {
		printf("Error: %s: %d\n", __func__, __LINE__);
		return -1;
	}

	if ((d_addr & 1) != 0) {
		/* program 1st byte */
		if (spi_nor_program_1byte(flash, s_buf[0],
					(void *)d_addr) != 0) {
			printf("Error: %s(%d)\n", __func__, __LINE__);
			return -1;
		}
		if (--s32remain_size == 0)
			return 0;
		d_addr++;
		s_buf++;
	}

	/* need to do write-enable command */
	if (WRITE_ENABLE(flash) != 0) {
		printf("Error : %d\n", __LINE__);
		return -1;
	}

	/*
	These two bytes write will be copied to txfifo first with
	g_tx_buf[1] being shifted out and followed by g_tx_buf[0].
	The reason for this is we will specify burst len=6. So SPI will
	do this kind of data movement.
	*/
	g_tx_buf[0] = d_addr >> 16;
	g_tx_buf[1] = AAI_PROG;    /* need to skip bytes 1, 2 */
	/* byte shifted order is: 7, 6, 5, 4 */
	g_tx_buf[4] = s_buf[1];
	g_tx_buf[5] = s_buf[0];
	g_tx_buf[6] = d_addr;
	g_tx_buf[7] = d_addr >> 8;
	if (spi_xfer(flash->spi, 6 << 3, g_tx_buf, g_rx_buf,
			SPI_XFER_BEGIN | SPI_XFER_END) != 0) {
		printf("Error: %s(%d): failed\n",
				__FILE__, __LINE__);
		return -1;
	}

	while (spi_nor_status(flash) & RDSR_BUSY)
		;

	for (d_addr += 2, s_buf += 2, s32remain_size -= 2;
		s32remain_size > 1;
		d_addr += 2, s_buf += 2, s32remain_size -= 2) {
		debug("%d%% transferred\n",
			((len - s32remain_size) * 100 / len));
		/* byte shifted order is: 2,1,0 */
		g_tx_buf[2] = AAI_PROG;
		g_tx_buf[1] = s_buf[0];
		g_tx_buf[0] = s_buf[1];

		if (spi_xfer(flash->spi, 3 << 3, g_tx_buf, g_rx_buf,
				SPI_XFER_BEGIN | SPI_XFER_END) != 0) {
			printf("Error: %s(%d): failed\n",
					__FILE__, __LINE__);
			return -1;
		}

		while (spi_nor_status(flash) & RDSR_BUSY)
			;

		if ((s32remain_size % imx_sf->params->block_size) == 0)
			printf(".");
	}
	printf("SUCCESS\n\n");
	debug("100%% transferred\n");

	WRITE_DISABLE(flash);
	while (spi_nor_status(flash) & RDSR_BUSY)
		;

	if (WRITE_ENABLE(flash) != 0) {
		printf("Error : %d\n", __LINE__);
		return -1;
	}

	if (len == 1) {
		/* need to do write-enable command */
		/* only 1 byte left */
		if (spi_nor_program_1byte(flash, s_buf[0],
					(void *)d_addr) != 0) {
			printf("Error: %s(%d)\n",
					__func__, __LINE__);
			return -1;
		}
	}
	return 0;
}

static int spi_nor_flash_erase(struct spi_flash *flash, u32 offset,
		size_t len)
{
	s32 s32remain_size = len;

	if (!(flash->spi))
		return -1;

	printf("Erasing SPI NOR flash 0x%x [0x%x bytes]\n",
		offset, len);

	if ((len % SZ_4K) != 0 || len == 0) {
		printf("Error: size (0x%x) is not integer multiples of 4kB(0x1000)\n",
			len);
		return -1;
	}
	if ((offset & (SZ_4K - 1)) != 0) {
		printf("Error - addr is not 4kB(0x1000) aligned: 0x%08x\n",
			offset);
		return -1;
	}
	for (; s32remain_size > 0; s32remain_size -= SZ_4K, offset += SZ_4K) {
		debug("Erasing 0x%08x, %d%% erased\n",
				offset,
				((len - s32remain_size) * 100 / len));
		if (spi_nor_erase_block(flash,
				(void *)offset, SZ_4K) != 0) {
			printf("Error: spi_nor_flash_erase(): %d\n", __LINE__);
			return -1;
		}
		printf(".");
	}
	printf("SUCCESS\n\n");
	debug("100%% erased\n");
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

