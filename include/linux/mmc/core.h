/*
 *  (C) Copyright 2008-2009 Freescale Semiconductor, Inc.
 *
 *  linux/include/linux/mmc/core.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef MMC_CORE_H
#define MMC_CORE_H

#include <asm/arch/sdhc.h>

struct request;
struct mmc_data;
struct mmc_request;

struct mmc_command {
    esdhc_cmd_t cmd;
	esdhc_resp_t resp;
#define MMC_RSP_PRESENT	(1 << 0)
#define MMC_RSP_136	(1 << 1)      /* 136 bit response */
#define MMC_RSP_CRC	(1 << 2)      /* expect valid crc */
#define MMC_RSP_BUSY	(1 << 3)  /* card may send busy */
#define MMC_RSP_OPCODE	(1 << 4)  /* response contains opcode */

#define MMC_CMD_MASK	(3 << 5)  /* non-SPI command type */
#define MMC_CMD_AC	(0 << 5)
#define MMC_CMD_ADTC	(1 << 5)
#define MMC_CMD_BC	(2 << 5)
#define MMC_CMD_BCR	(3 << 5)

#define MMC_RSP_SPI_S1	(1 << 7)  /* one status byte */
#define MMC_RSP_SPI_S2	(1 << 8)  /* second byte */
#define MMC_RSP_SPI_B4	(1 << 9)  /* four data bytes */
#define MMC_RSP_SPI_BUSY (1 << 10) /* card may send busy */

/*
 * These are the native response types, and correspond to valid bit
 * patterns of the above flags.  One additional valid pattern
 * is all zeros, which means we don't expect a response.
 */
#define MMC_RSP_NONE	(0)
#define MMC_RSP_R1	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R1B (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE|MMC_RSP_BUSY)
#define MMC_RSP_R2	(MMC_RSP_PRESENT|MMC_RSP_136|MMC_RSP_CRC)
#define MMC_RSP_R3	(MMC_RSP_PRESENT)
#define MMC_RSP_R4	(MMC_RSP_PRESENT)
#define MMC_RSP_R5	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R6	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R7	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)

#define mmc_resp_type(cmd) ((cmd)->flags & \
		(MMC_RSP_PRESENT|MMC_RSP_136|MMC_RSP_CRC| \
		 MMC_RSP_BUSY|MMC_RSP_OPCODE))

#define MMC_KEEP_CLK_RUN (1 << 31) /* Keep card clock on after request */

/*
 * These are the SPI response types for MMC, SD, and SDIO cards.
 * Commands return R1, with maybe more info.  Zero is an error type;
 * callers must always provide the appropriate MMC_RSP_SPI_Rx flags.
 */
#define MMC_RSP_SPI_R1  (MMC_RSP_SPI_S1)
#define MMC_RSP_SPI_R1B (MMC_RSP_SPI_S1|MMC_RSP_SPI_BUSY)
#define MMC_RSP_SPI_R2  (MMC_RSP_SPI_S1|MMC_RSP_SPI_S2)
#define MMC_RSP_SPI_R3  (MMC_RSP_SPI_S1|MMC_RSP_SPI_B4)
#define MMC_RSP_SPI_R4  (MMC_RSP_SPI_S1|MMC_RSP_SPI_B4)
#define MMC_RSP_SPI_R5  (MMC_RSP_SPI_S1|MMC_RSP_SPI_S2)
#define MMC_RSP_SPI_R7  (MMC_RSP_SPI_S1|MMC_RSP_SPI_B4)

#define mmc_spi_resp_type(cmd) ((cmd)->flags & \
		(MMC_RSP_SPI_S1|MMC_RSP_SPI_BUSY|MMC_RSP_SPI_S2|MMC_RSP_SPI_B4))

/*
 * These are the command types.
 */
#define mmc_cmd_type(cmd) ((cmd)->flags & MMC_CMD_MASK)

	unsigned int        retries; /* max number of retries */
	unsigned int        error;   /* command error */

/*
 * Standard errno values are used for errors, but some have specific
 * meaning in the MMC layer:
 *
 * ETIMEDOUT    Card took too long to respond
 * EILSEQ       Basic format problem with the received or sent data
 *              (e.g. CRC check failed, incorrect opcode in response
 *              or bad end bit)
 * EINVAL       Request cannot be performed because of restrictions
 *              in hardware and/or the driver
 * ENOMEDIUM    Host can determine that the slot is empty and is
 *              actively failing requests
 */

	struct mmc_data *data;      /* data segment associated with cmd */
	struct mmc_request *mrq;    /* associated request */
};

struct mmc_data {
	unsigned int timeout_ns;   /* data timeout (in ns, max 80ms) */
	unsigned int timeout_clks; /* data timeout (in clocks) */
	unsigned int blksz;        /* data block size */
	unsigned int blocks;       /* number of blocks */
	unsigned int error;        /* data error */
	unsigned int flags;

#define MMC_DATA_WRITE	(1 << 8)
#define MMC_DATA_READ	(1 << 9)
#define MMC_DATA_STREAM	(1 << 10)

	unsigned int       bytes_xfered;

	struct mmc_command *stop;  /* stop command */
	struct mmc_request *mrq;   /* associated request */

	unsigned int       sg_len; /* size of scatter list */
	struct scatterlist *sg;    /* I/O scatter list */
};

struct mmc_request {
	struct mmc_command  *cmd;
	struct mmc_data     *data;
	struct mmc_command  *stop;

	void *done_data;           /* completion data */
	void (*done)(struct mmc_request *); /* completion function */
};

#endif
