/*
 * Copyright 2017-2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <malloc.h>
#include <spi.h>
#include <asm/io.h>
#include <linux/sizes.h>
#include <dm.h>
#include <errno.h>
#include <watchdog.h>
#include "fsl_fspi.h"
#include <asm/imx-common/sci/sci.h>

DECLARE_GLOBAL_DATA_PTR;

#define RX_BUFFER_SIZE		0x200
#define TX_BUFFER_SIZE		0x400
#define AHB_BUFFER_SIZE		0x800

#ifdef CONFIG_SPI_FLASH_4BYTES_ADDR
#define OFFSET_BITS_MASK	GENMASK(31, 0)
#else
#define OFFSET_BITS_MASK	GENMASK(23, 0)
#endif

#define FLASH_STATUS_WEL	0x02

/* SEQID */
#define SEQID_READ		0
#define SEQID_WREN		1
#define SEQID_FAST_READ		2
#define SEQID_RDSR		3
#define SEQID_SE		4
#define SEQID_CHIP_ERASE	5
#define SEQID_PP		6
#define SEQID_RDID		7
#define SEQID_BE_4K		8
#ifdef CONFIG_SPI_FLASH_BAR
#define SEQID_BRRD		9
#define SEQID_BRWR		10
#define SEQID_RDEAR		11
#define SEQID_WREAR		12
#endif
#define SEQID_RDEVCR		13
#define SEQID_WREVCR		14
#define SEQID_QUAD_OUTPUT	15
#define SEQID_RDFSR		16
#define SEQID_EN4B		17


/* FSPI CMD */
#define FSPI_CMD_PP		0x02	/* Page program (up to 256 bytes) */
#define FSPI_CMD_RDSR		0x05	/* Read status register */
#define FSPI_CMD_WREN		0x06	/* Write enable */
#define FSPI_CMD_FAST_READ	0x0b	/* Read data bytes (high frequency) */
#define FSPI_CMD_READ		0x03	/* Read data bytes */
#define FSPI_CMD_BE_4K		0x20    /* 4K erase */
#define FSPI_CMD_CHIP_ERASE	0xc7	/* Erase whole flash chip */
#define FSPI_CMD_SE		0xd8	/* Sector erase (usually 64KiB) */
#define FSPI_CMD_RDID		0x9f	/* Read JEDEC ID */

/* Used for Micron, winbond and Macronix flashes */
#define	FSPI_CMD_WREAR		0xc5	/* EAR register write */
#define	FSPI_CMD_RDEAR		0xc8	/* EAR reigster read */

/* Used for Spansion flashes only. */
#define	FSPI_CMD_BRRD		0x16	/* Bank register read */
#define	FSPI_CMD_BRWR		0x17	/* Bank register write */

/* 4-byte address FSPI CMD - used on Spansion and some Macronix flashes */
#define FSPI_CMD_FAST_READ_4B	0x0c    /* Read data bytes (high frequency) */
#define FSPI_CMD_PP_4B		0x12    /* Page program (up to 256 bytes) */
#define FSPI_CMD_SE_4B		0xdc    /* Sector erase (usually 64KiB) */
#define FSPI_CMD_BE_4K_4B	0x21    /* 4K erase */

#define FSPI_CMD_RD_EVCR	0x65    /* Read EVCR register */
#define FSPI_CMD_WR_EVCR	0x61    /* Write EVCR register */

#define FSPI_CMD_EN4B		0xB7

/* 1-1-4 READ CMD */
#define FSPI_CMD_QUAD_OUTPUT		0x6b
#define FSPI_CMD_DDR_QUAD_OUTPUT	0x6d

/* read flag status register */
#define FSPI_CMD_RDFSR		0x70

/* fsl_fspi_platdata flags */
#define FSPI_FLAG_REGMAP_ENDIAN_BIG	BIT(0)

/* default SCK frequency, unit: HZ */
#define FSL_FSPI_DEFAULT_SCK_FREQ	50000000

/* FSPI max chipselect signals number */
#define FSL_FSPI_MAX_CHIPSELECT_NUM     4

#ifdef CONFIG_DM_SPI
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
#endif

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

#ifndef CONFIG_DM_SPI
struct fsl_fspi {
	struct spi_slave slave;
	struct fsl_fspi_priv priv;
};
#endif

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

/* FSPI support swapping the flash read/write data
 * in hardware
 */
static inline u32 fspi_endian_xchg(u32 data)
{
	return data;
}

static void fspi_set_lut(struct fsl_fspi_priv *priv)
{
	struct fsl_fspi_regs *regs = priv->regs;
	u32 lut_base;

	/* Unlock the LUT */
	fspi_write32(priv->flags, &regs->lutkey, FLEXSPI_LUTKEY_VALUE);
	fspi_write32(priv->flags, &regs->lutcr, FLEXSPI_LCKER_UNLOCK);

	/* READ */
	lut_base = SEQID_READ * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(FSPI_CMD_READ) | PAD0(LUT_PAD1) |
		     INSTR0(LUT_CMD) | OPRND1(ADDR32BIT) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1],
		     OPRND0(0) | PAD0(LUT_PAD1) |
		     INSTR0(LUT_READ));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Write Enable */
	lut_base = SEQID_WREN * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(FSPI_CMD_WREN) |
		PAD0(LUT_PAD1) | INSTR0(LUT_CMD));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Fast Read */
	lut_base = SEQID_FAST_READ * 4;
#ifdef CONFIG_SPI_FLASH_BAR
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(FSPI_CMD_FAST_READ) | PAD0(LUT_PAD1) |
		     INSTR0(LUT_CMD) | OPRND1(ADDR24BIT) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
#else
	if (FSL_FSPI_FLASH_SIZE  <= SZ_16M)
		fspi_write32(priv->flags, &regs->lut[lut_base],
			     OPRND0(FSPI_CMD_FAST_READ) | PAD0(LUT_PAD1) |
			     INSTR0(LUT_CMD) | OPRND1(ADDR24BIT) |
			     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
	else
		fspi_write32(priv->flags, &regs->lut[lut_base],
			     OPRND0(FSPI_CMD_FAST_READ_4B) |
			     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) |
			     OPRND1(ADDR32BIT) | PAD1(LUT_PAD1) |
			     INSTR1(LUT_ADDR));
#endif
	fspi_write32(priv->flags, &regs->lut[lut_base + 1],
		     OPRND0(8) | PAD0(LUT_PAD1) | INSTR0(LUT_DUMMY) |
		     OPRND1(0) | PAD1(LUT_PAD1) |
		     INSTR1(LUT_READ));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Read Status */
	lut_base = SEQID_RDSR * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(FSPI_CMD_RDSR) |
		PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(1) |
		PAD1(LUT_PAD1) | INSTR1(LUT_READ));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Erase a sector */
	lut_base = SEQID_SE * 4;
#ifdef CONFIG_SPI_FLASH_BAR
	fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(FSPI_CMD_SE) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(ADDR24BIT) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
#else
	if (FSL_FSPI_FLASH_SIZE  <= SZ_16M)
		fspi_write32(priv->flags, &regs->lut[lut_base],
			     OPRND0(FSPI_CMD_SE) | PAD0(LUT_PAD1) |
			     INSTR0(LUT_CMD) | OPRND1(ADDR24BIT) |
			     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
	else
		fspi_write32(priv->flags, &regs->lut[lut_base],
			     OPRND0(FSPI_CMD_SE_4B) | PAD0(LUT_PAD1) |
			     INSTR0(LUT_CMD) | OPRND1(ADDR32BIT) |
			     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
#endif
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Erase the whole chip */
	lut_base = SEQID_CHIP_ERASE * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(FSPI_CMD_CHIP_ERASE) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Page Program */
	lut_base = SEQID_PP * 4;
#ifdef CONFIG_SPI_FLASH_BAR
	fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(FSPI_CMD_PP) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(ADDR24BIT) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
#else
	if (FSL_FSPI_FLASH_SIZE  <= SZ_16M)
		fspi_write32(priv->flags, &regs->lut[lut_base],
			     OPRND0(FSPI_CMD_PP) | PAD0(LUT_PAD1) |
			     INSTR0(LUT_CMD) | OPRND1(ADDR24BIT) |
			     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
	else
		fspi_write32(priv->flags, &regs->lut[lut_base],
			     OPRND0(FSPI_CMD_PP_4B) | PAD0(LUT_PAD1) |
			     INSTR0(LUT_CMD) | OPRND1(ADDR32BIT) |
			     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
#endif
	fspi_write32(priv->flags, &regs->lut[lut_base + 1],
		     OPRND0(0) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_WRITE));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* READ ID */
	lut_base = SEQID_RDID * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(FSPI_CMD_RDID) |
		PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(8) |
		PAD1(LUT_PAD1) | INSTR1(LUT_READ));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* SUB SECTOR 4K ERASE */
	lut_base = SEQID_BE_4K * 4;
#ifdef CONFIG_SPI_FLASH_BAR
	fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(FSPI_CMD_BE_4K) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(ADDR24BIT) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
#else
	if (FSL_FSPI_FLASH_SIZE  <= SZ_16M)
		fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(FSPI_CMD_BE_4K) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(ADDR24BIT) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
	else
		fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(FSPI_CMD_BE_4K_4B) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(ADDR32BIT) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR));
#endif
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

#ifdef CONFIG_SPI_FLASH_BAR
	/*
	 * BRRD BRWR RDEAR WREAR are all supported, because it is hard to
	 * dynamically check whether to set BRRD BRWR or RDEAR WREAR during
	 * initialization.
	 */
	lut_base = SEQID_BRRD * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(FSPI_CMD_BRRD) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(1) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_READ));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	lut_base = SEQID_BRWR * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(FSPI_CMD_BRWR) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(1) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_WRITE));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	lut_base = SEQID_RDEAR * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(FSPI_CMD_RDEAR) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(1) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_READ));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	lut_base = SEQID_WREAR * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(FSPI_CMD_WREAR) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(1) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_WRITE));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);
#endif
	lut_base = SEQID_RDEVCR * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(FSPI_CMD_RD_EVCR) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	lut_base = SEQID_WREVCR * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(FSPI_CMD_WR_EVCR) |
		     PAD0(LUT_PAD1) | INSTR0(LUT_CMD));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

#ifdef CONFIG_FSPI_QUAD_SUPPORT
	/* QUAD OUTPUT READ */
	lut_base = SEQID_QUAD_OUTPUT * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base],
		     OPRND0(FSPI_CMD_DDR_QUAD_OUTPUT) | PAD0(LUT_PAD1) |
		     INSTR0(LUT_CMD) | OPRND1(ADDR24BIT) |
		     PAD1(LUT_PAD1) | INSTR1(LUT_ADDR_DDR));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1],
		     OPRND0(0xc) | PAD0(LUT_PAD4) |
		     INSTR0(LUT_DUMMY_DDR) | OPRND1(0) |
		     PAD1(LUT_PAD4) | INSTR1(LUT_READ_DDR));
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);
#endif

	/* Read Flag Status */
	lut_base = SEQID_RDFSR * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(FSPI_CMD_RDFSR) |
		PAD0(LUT_PAD1) | INSTR0(LUT_CMD) | OPRND1(1) |
		PAD1(LUT_PAD1) | INSTR1(LUT_READ));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Enter 4 bytes address mode */
	lut_base = SEQID_EN4B * 4;
	fspi_write32(priv->flags, &regs->lut[lut_base], OPRND0(FSPI_CMD_EN4B) |
		PAD0(LUT_PAD1) | INSTR0(LUT_CMD));
	fspi_write32(priv->flags, &regs->lut[lut_base + 1], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 2], 0);
	fspi_write32(priv->flags, &regs->lut[lut_base + 3], 0);

	/* Lock the LUT */
	fspi_write32(priv->flags, &regs->lutkey, FLEXSPI_LUTKEY_VALUE);
	fspi_write32(priv->flags, &regs->lutcr, FLEXSPI_LCKER_LOCK);
}

#if defined(CONFIG_SYS_FSL_FSPI_AHB)
/*
 * If we have changed the content of the flash by writing or erasing,
 * we need to invalidate the AHB buffer. If we do not do so, we may read out
 * the wrong data. The spec tells us reset the AHB domain and Serial Flash
 * domain at the same time.
 */
static inline void fspi_ahb_invalid(struct fsl_fspi_priv *priv)
{
	struct fsl_fspi_regs *regs = priv->regs;
	u32 reg;

	reg = fspi_read32(priv->flags, &regs->mcr0);
	reg |= FLEXSPI_MCR0_SWRST_MASK;
	fspi_write32(priv->flags, &regs->mcr0, reg);

	/*
	 * The minimum delay : 1 AHB + 2 SFCK clocks.
	 * Delay 1 us is enough.
	 */
	while ((fspi_read32(priv->flags, &regs->mcr0) & 1))
		;
}

#define FSPI_AHB_BASE_ADDR 0x08000000
/* Read out the data from the AHB buffer. */
static inline void fspi_ahb_read(struct fsl_fspi_priv *priv, u8 *rxbuf, int len)
{
	/* Read out the data directly from the AHB buffer. */
	memcpy(rxbuf, (u8 *)(0x08000000 + (uintptr_t)priv->sf_addr) , len);

}

/*
 * There are two different ways to read out the data from the flash:
 *  the "IP Command Read" and the "AHB Command Read".
 *
 * The IC guy suggests we use the "AHB Command Read" which is faster
 * then the "IP Command Read". (What's more is that there is a bug in
 * the "IP Command Read" in the Vybrid.)
 *
 * After we set up the registers for the "AHB Command Read", we can use
 * the memcpy to read the data directly. A "missed" access to the buffer
 * causes the controller to clear the buffer, and use the sequence pointed
 * by the QUADSPI_BFGENCR[SEQID] to initiate a read from the flash.
 */
static void fspi_init_ahb_read(struct fsl_fspi_priv *priv)
{
	struct fsl_fspi_regs *regs = priv->regs;
	int i;

	/* AHB configuration for access buffer 0~7 .*/
	for (i = 0; i < 7; i++)
		fspi_write32(priv->flags, &regs->ahbrxbuf0cr0 + i, 0);

	/*
	 * Set ADATSZ with the maximum AHB buffer size to improve the read
	 * performance
	 */
	fspi_write32(priv->flags, &regs->ahbrxbuf7cr0, AHB_BUFFER_SIZE / 8 |
		     FLEXSPI_AHBRXBUF0CR7_PREF_MASK);

	fspi_write32(priv->flags, &regs->ahbcr, FLEXSPI_AHBCR_PREF_EN_MASK);
	/*
	 * Set the default lut sequence for AHB Read.
	 * Parallel mode is disabled.
	 */
#ifdef CONFIG_FSPI_QUAD_SUPPORT
	fspi_write32(priv->flags, &regs->flsha1cr2, SEQID_QUAD_OUTPUT);
#else
	fspi_write32(priv->flags, &regs->flsha1cr2, SEQID_FAST_READ);
#endif

}
#endif

#ifdef CONFIG_SPI_FLASH_BAR
/* Bank register read/write, EAR register read/write */
static void fspi_op_rdbank(struct fsl_fspi_priv *priv, u8 *rxbuf, u32 len)
{
	struct fsl_fspi_regs *regs = priv->regs;
	u32  data, seqid;

	/* invalid the RXFIFO first */
	fspi_write32(priv->flags, &regs->iprxfcr, FLEXSPI_IPRXFCR_CLR_MASK);

	fspi_write32(priv->flags, &regs->ipcr0, priv->cur_amba_base);

	if (priv->cur_seqid == FSPI_CMD_BRRD)
		seqid = SEQID_BRRD;
	else
		seqid = SEQID_RDEAR;

	fspi_write32(priv->flags, &regs->ipcr1,
		     (seqid << FLEXSPI_IPCR1_SEQID_SHIFT) | len);

	/* Trigger the command */
	fspi_write32(priv->flags, &regs->ipcmd, 1);

	/* Wait for command done */
	while (!(fspi_read32(priv->flags, &regs->intr)
		 & FLEXSPI_INTR_IPCMDDONE_MASK))
		;

	while (1) {
		data = fspi_read32(priv->flags, &regs->rfdr[0]);
		memcpy(rxbuf, &data, len);
			break;
	}

	fspi_write32(priv->flags, &regs->intr, FLEXSPI_INTR_IPRXWA_MASK);
	fspi_write32(priv->flags, &regs->iprxfcr, FLEXSPI_IPRXFCR_CLR_MASK);
	fspi_write32(priv->flags, &regs->intr, FLEXSPI_INTR_IPCMDDONE_MASK);

}
#endif

static void fspi_op_rdevcr(struct fsl_fspi_priv *priv, u8 *rxbuf, u32 len)
{
	struct fsl_fspi_regs *regs = priv->regs;
	u32  data;

	/* invalid the RXFIFO first */
	fspi_write32(priv->flags, &regs->iprxfcr, FLEXSPI_IPRXFCR_CLR_MASK);

	fspi_write32(priv->flags, &regs->ipcr0, priv->cur_amba_base);

	fspi_write32(priv->flags, &regs->ipcr1,
		     (SEQID_RDEVCR << FLEXSPI_IPCR1_SEQID_SHIFT) | len);

	/* Trigger the command */
	fspi_write32(priv->flags, &regs->ipcmd, 1);

	/* Wait for command done */
	while (!(fspi_read32(priv->flags, &regs->intr)
		 & FLEXSPI_INTR_IPCMDDONE_MASK))
		;

	while (1) {
		data = fspi_read32(priv->flags, &regs->rfdr[0]);
		memcpy(rxbuf, &data, len);
			break;
	}

	fspi_write32(priv->flags, &regs->intr, FLEXSPI_INTR_IPRXWA_MASK);
	fspi_write32(priv->flags, &regs->iprxfcr, FLEXSPI_IPRXFCR_CLR_MASK);
	fspi_write32(priv->flags, &regs->intr, FLEXSPI_INTR_IPCMDDONE_MASK);

}

static void fspi_op_wrevcr(struct fsl_fspi_priv *priv, u8 *txbuf, u32 len)
{
	struct fsl_fspi_regs *regs = priv->regs;

	/* invalid the TXFIFO first */
	fspi_write32(priv->flags, &regs->iptxfcr, FLEXSPI_IPTXFCR_CLR_MASK);

	fspi_write32(priv->flags, &regs->ipcr0, priv->cur_amba_base);

	/* Wait for TXFIFO empty*/
	while (!(fspi_read32(priv->flags, &regs->intr) & FLEXSPI_INTR_IPTXWE_MASK))
		;

	/* write the data to TXFIFO */
	memcpy(&regs->tfdr, txbuf, len);

	fspi_write32(priv->flags, &regs->intr, FLEXSPI_INTR_IPTXWE_MASK);
	fspi_write32(priv->flags, &regs->ipcr1,
		     (SEQID_WREVCR << FLEXSPI_IPCR1_SEQID_SHIFT) | len);

	/* Trigger the command */
	fspi_write32(priv->flags, &regs->ipcmd, 1);

	/* Wait for command done */
	while (!(fspi_read32(priv->flags, &regs->intr) & FLEXSPI_INTR_IPCMDDONE_MASK))
		;

	/* invalid the TXFIFO first */
	fspi_write32(priv->flags, &regs->iptxfcr, FLEXSPI_IPTXFCR_CLR_MASK);
	fspi_write32(priv->flags, &regs->intr, FLEXSPI_INTR_IPCMDDONE_MASK);

}

static void fspi_op_rdid(struct fsl_fspi_priv *priv, u32 *rxbuf, u32 len)
{
	struct fsl_fspi_regs *regs = priv->regs;
	u32 data, size;
	int i;

	/* invalid the RXFIFO first */
	fspi_write32(priv->flags, &regs->iprxfcr, FLEXSPI_IPRXFCR_CLR_MASK);

	fspi_write32(priv->flags, &regs->ipcr0, priv->cur_amba_base);

	fspi_write32(priv->flags, &regs->ipcr1,
		     (SEQID_RDID << FLEXSPI_IPCR1_SEQID_SHIFT) | len);
	/* Trigger the command */
	fspi_write32(priv->flags, &regs->ipcmd, 1);

	/* Wait for command done */
	while (!(fspi_read32(priv->flags, &regs->intr)
		 & FLEXSPI_INTR_IPCMDDONE_MASK))
		;

	i = 0;
	while ((RX_BUFFER_SIZE >= len) && (len > 0)) {
		data = fspi_read32(priv->flags, &regs->rfdr[i]);
		size = (len < 4) ? len : 4;
		memcpy(rxbuf, &data, size);
		len -= size;
		rxbuf++;
		i++;
	}
	fspi_write32(priv->flags, &regs->intr, FLEXSPI_INTR_IPRXWA_MASK);

	fspi_write32(priv->flags, &regs->iprxfcr, FLEXSPI_IPRXFCR_CLR_MASK);
	fspi_write32(priv->flags, &regs->intr, FLEXSPI_INTR_IPCMDDONE_MASK);
}

#ifndef CONFIG_SYS_FSL_FSPI_AHB
/* If not use AHB read, read data from ip interface */
static void fspi_op_read(struct fsl_fspi_priv *priv, u32 *rxbuf, u32 len)
{
	struct fsl_fspi_regs *regs = priv->regs;
	int i, size, rx_size;
	u32 to_or_from;

	to_or_from = priv->sf_addr + priv->cur_amba_base;

	/* invalid the RXFIFO */
	fspi_write32(priv->flags, &regs->iprxfcr, FLEXSPI_IPRXFCR_CLR_MASK);

	while (len > 0) {
		WATCHDOG_RESET();

		fspi_write32(priv->flags, &regs->ipcr0, to_or_from);

		rx_size = (len > RX_BUFFER_SIZE) ?
			RX_BUFFER_SIZE : len;

#ifdef CONFIG_FSPI_QUAD_SUPPORT
		fspi_write32(priv->flags, &regs->ipcr1,
			     (SEQID_QUAD_OUTPUT << FLEXSPI_IPCR1_SEQID_SHIFT) |
			     rx_size);
#else
		fspi_write32(priv->flags, &regs->ipcr1,
			     (SEQID_FAST_READ << FLEXSPI_IPCR1_SEQID_SHIFT) |
			     rx_size);
#endif

		to_or_from += rx_size;
		len -= rx_size;

		/* Trigger the command */
		fspi_write32(priv->flags, &regs->ipcmd, 1);

		size = rx_size / 8;
		for (i = 0; i < size; ++i) {
			/* Wait for RXFIFO available*/
			while (!(fspi_read32(priv->flags, &regs->intr)
				 & FLEXSPI_INTR_IPRXWA_MASK))
				;

			memcpy(rxbuf, &regs->rfdr, 8);
			rxbuf += 2;

			/* move the FIFO pointer */
			fspi_write32(priv->flags, &regs->intr,
				     FLEXSPI_INTR_IPRXWA_MASK);
		}

		size = rx_size % 8;

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
	}

}
#endif

static void fspi_op_write(struct fsl_fspi_priv *priv, u8 *txbuf, u32 len)
{
	struct fsl_fspi_regs *regs = priv->regs;
	u32 seqid;
	int i, size, tx_size;
	u32 to_or_from = 0;

	/* invalid the TXFIFO first */
	fspi_write32(priv->flags, &regs->iptxfcr, FLEXSPI_IPTXFCR_CLR_MASK);

	fspi_write32(priv->flags, &regs->ipcr0, priv->cur_amba_base);

	fspi_write32(priv->flags, &regs->ipcr1,
		     (SEQID_WREN << FLEXSPI_IPCR1_SEQID_SHIFT) | 0);

	/* Trigger the command */
	fspi_write32(priv->flags, &regs->ipcmd, 1);

	/* Wait for command done */
	while (!(fspi_read32(priv->flags, &regs->intr)
		 & FLEXSPI_INTR_IPCMDDONE_MASK))
		;

	fspi_write32(priv->flags, &regs->intr, FLEXSPI_INTR_IPCMDDONE_MASK);

	/* invalid the TXFIFO first */
	fspi_write32(priv->flags, &regs->iptxfcr, FLEXSPI_IPTXFCR_CLR_MASK);

	to_or_from = priv->sf_addr + priv->cur_amba_base;

	while (len > 0) {

		/* Default is page programming */
		seqid = SEQID_PP;
#ifdef CONFIG_SPI_FLASH_BAR
		if (priv->cur_seqid == FSPI_CMD_BRWR)
			seqid = SEQID_BRWR;
		else if (priv->cur_seqid == FSPI_CMD_WREAR)
			seqid = SEQID_WREAR;
#endif


		fspi_write32(priv->flags, &regs->ipcr0, to_or_from);

		tx_size = (len > TX_BUFFER_SIZE) ?
			TX_BUFFER_SIZE : len;

		to_or_from += tx_size;
		len -= tx_size;

		size = tx_size / 8;
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

		size = tx_size % 8;
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
			     (seqid << FLEXSPI_IPCR1_SEQID_SHIFT) | tx_size);


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
	}
}

static void fspi_op_rdsr(struct fsl_fspi_priv *priv, void *rxbuf, u32 len)
{
	struct fsl_fspi_regs *regs = priv->regs;
	u32 data;

	/* invalid the RXFIFO first */
	fspi_write32(priv->flags, &regs->iprxfcr, FLEXSPI_IPRXFCR_CLR_MASK);

	fspi_write32(priv->flags, &regs->ipcr0, priv->cur_amba_base);

	fspi_write32(priv->flags, &regs->ipcr1,
		     (SEQID_RDSR << FLEXSPI_IPCR1_SEQID_SHIFT) | len);
	/* Trigger the command */
	fspi_write32(priv->flags, &regs->ipcmd, 1);

	/* Wait for command done */
	while (!(fspi_read32(priv->flags, &regs->intr)
		 & FLEXSPI_INTR_IPCMDDONE_MASK))
		;

	data = fspi_read32(priv->flags, &regs->rfdr[0]);
	memcpy(rxbuf, &data, len);

	fspi_write32(priv->flags, &regs->intr, FLEXSPI_INTR_IPRXWA_MASK);
	fspi_write32(priv->flags, &regs->iprxfcr, FLEXSPI_IPRXFCR_CLR_MASK);
	fspi_write32(priv->flags, &regs->intr, FLEXSPI_INTR_IPCMDDONE_MASK);
}

static void fspi_op_rdfsr(struct fsl_fspi_priv *priv, void *rxbuf, u32 len)
{
	struct fsl_fspi_regs *regs = priv->regs;
	u32 data;

	/* invalid the RXFIFO first */
	fspi_write32(priv->flags, &regs->iprxfcr, FLEXSPI_IPRXFCR_CLR_MASK);

	fspi_write32(priv->flags, &regs->ipcr0, priv->cur_amba_base);

	fspi_write32(priv->flags, &regs->ipcr1,
		     (SEQID_RDFSR << FLEXSPI_IPCR1_SEQID_SHIFT) | len);
	/* Trigger the command */
	fspi_write32(priv->flags, &regs->ipcmd, 1);

	/* Wait for command done */
	while (!(fspi_read32(priv->flags, &regs->intr)
		 & FLEXSPI_INTR_IPCMDDONE_MASK))
		;

	data = fspi_read32(priv->flags, &regs->rfdr[0]);
	memcpy(rxbuf, &data, len);

	fspi_write32(priv->flags, &regs->intr, FLEXSPI_INTR_IPRXWA_MASK);
	fspi_write32(priv->flags, &regs->iprxfcr, FLEXSPI_IPRXFCR_CLR_MASK);
	fspi_write32(priv->flags, &regs->intr, FLEXSPI_INTR_IPCMDDONE_MASK);
}

static void fspi_op_erase(struct fsl_fspi_priv *priv)
{
	struct fsl_fspi_regs *regs = priv->regs;
	u32 to_or_from = 0;

	to_or_from = priv->sf_addr + priv->cur_amba_base;

	fspi_write32(priv->flags, &regs->ipcr0, to_or_from);

	fspi_write32(priv->flags, &regs->ipcr1,
		     (SEQID_WREN << FLEXSPI_IPCR1_SEQID_SHIFT) | 0);
	/* Trigger the command */
	fspi_write32(priv->flags, &regs->ipcmd, 1);

	while (!(fspi_read32(priv->flags, &regs->intr)
		 & FLEXSPI_INTR_IPCMDDONE_MASK))
		;

	fspi_write32(priv->flags, &regs->intr, FLEXSPI_INTR_IPCMDDONE_MASK);

	if (priv->cur_seqid == FSPI_CMD_SE || priv->cur_seqid == FSPI_CMD_SE_4B) {
		fspi_write32(priv->flags, &regs->ipcr1,
			     (SEQID_SE << FLEXSPI_IPCR1_SEQID_SHIFT) | 0);
	} else if (priv->cur_seqid == FSPI_CMD_BE_4K || priv->cur_seqid == FSPI_CMD_BE_4K_4B) {
		fspi_write32(priv->flags, &regs->ipcr1,
			     (SEQID_BE_4K << FLEXSPI_IPCR1_SEQID_SHIFT) | 0);
	}
	/* Trigger the command */
	fspi_write32(priv->flags, &regs->ipcmd, 1);

	while (!(fspi_read32(priv->flags, &regs->intr)
		 & FLEXSPI_INTR_IPCMDDONE_MASK))
		;

	fspi_write32(priv->flags, &regs->intr, FLEXSPI_INTR_IPCMDDONE_MASK);
}

static void fspi_op_enter_4bytes(struct fsl_fspi_priv *priv)
{
	struct fsl_fspi_regs *regs = priv->regs;

	/* invalid the TXFIFO first */
	fspi_write32(priv->flags, &regs->iptxfcr, FLEXSPI_IPTXFCR_CLR_MASK);

	fspi_write32(priv->flags, &regs->ipcr0, priv->cur_amba_base);

	fspi_write32(priv->flags, &regs->ipcr1,
		     (SEQID_EN4B << FLEXSPI_IPCR1_SEQID_SHIFT) | 0);

	/* Trigger the command */
	fspi_write32(priv->flags, &regs->ipcmd, 1);

	/* Wait for command done */
	while (!(fspi_read32(priv->flags, &regs->intr)
		 & FLEXSPI_INTR_IPCMDDONE_MASK))
		;

	fspi_write32(priv->flags, &regs->intr, FLEXSPI_INTR_IPCMDDONE_MASK);


}

int fspi_xfer(struct fsl_fspi_priv *priv, unsigned int bitlen,
		const void *dout, void *din, unsigned long flags)
{
	u32 bytes = DIV_ROUND_UP(bitlen, 8);
	static u32 wr_sfaddr;
	u32 txbuf;

	if (dout) {
		if (flags & SPI_XFER_BEGIN) {
			priv->cur_seqid = *(u8 *)dout;
#ifdef CONFIG_SPI_FLASH_4BYTES_ADDR
			if (FSL_FSPI_FLASH_SIZE  > SZ_16M)
				dout = (u8 *)dout + 1;
#endif
			memcpy(&txbuf, dout, 4);
		}

		if (flags == SPI_XFER_END) {
			if (priv->cur_seqid == FSPI_CMD_WR_EVCR) {
				fspi_op_wrevcr(priv, (u8 *)dout, bytes);
				return 0;
			}
			priv->sf_addr = wr_sfaddr;
			fspi_op_write(priv, (u8 *)dout, bytes);
			return 0;
		}

		if (priv->cur_seqid == FSPI_CMD_QUAD_OUTPUT ||
		    priv->cur_seqid == FSPI_CMD_FAST_READ ||
		    priv->cur_seqid == FSPI_CMD_FAST_READ_4B) {
			priv->sf_addr = swab32(txbuf) & OFFSET_BITS_MASK;
		} else if ((priv->cur_seqid == FSPI_CMD_SE) ||
			   (priv->cur_seqid == FSPI_CMD_BE_4K) ||
			   (priv->cur_seqid == FSPI_CMD_SE_4B) ||
			   (priv->cur_seqid == FSPI_CMD_BE_4K_4B)) {
			priv->sf_addr = swab32(txbuf) & OFFSET_BITS_MASK;
			fspi_op_erase(priv);
		} else if (priv->cur_seqid == FSPI_CMD_PP ||
			priv->cur_seqid == FSPI_CMD_PP_4B) {
			wr_sfaddr = swab32(txbuf) & OFFSET_BITS_MASK;
		} else if (priv->cur_seqid == FSPI_CMD_WR_EVCR) {
			wr_sfaddr = 0;
		} else if ((priv->cur_seqid == FSPI_CMD_BRWR) ||
			 (priv->cur_seqid == FSPI_CMD_WREAR)) {
#ifdef CONFIG_SPI_FLASH_BAR
			wr_sfaddr = 0;
#endif
		} else if (priv->cur_seqid == FSPI_CMD_EN4B) {
			fspi_op_enter_4bytes(priv);
		}
	}

	if (din) {
		if (priv->cur_seqid == FSPI_CMD_QUAD_OUTPUT ||
		    priv->cur_seqid == FSPI_CMD_FAST_READ ||
		    priv->cur_seqid == FSPI_CMD_FAST_READ_4B) {
#ifdef CONFIG_SYS_FSL_FSPI_AHB
			fspi_ahb_read(priv, din, bytes);
#else
			fspi_op_read(priv, din, bytes);
#endif
		} else if (priv->cur_seqid == FSPI_CMD_RDID)
			fspi_op_rdid(priv, din, bytes);
		else if (priv->cur_seqid == FSPI_CMD_RDSR)
			fspi_op_rdsr(priv, din, bytes);
		else if (priv->cur_seqid == FSPI_CMD_RDFSR)
			fspi_op_rdfsr(priv, din, bytes);
		else if (priv->cur_seqid == FSPI_CMD_RD_EVCR)
			fspi_op_rdevcr(priv, din, bytes);
#ifdef CONFIG_SPI_FLASH_BAR
		else if ((priv->cur_seqid == FSPI_CMD_BRRD) ||
			 (priv->cur_seqid == FSPI_CMD_RDEAR)) {
			priv->sf_addr = 0;
			fspi_op_rdbank(priv, din, bytes);
		}
#endif
	}

#ifdef CONFIG_SYS_FSL_FSPI_AHB
	if ((priv->cur_seqid == FSPI_CMD_SE) ||
		(priv->cur_seqid == FSPI_CMD_SE_4B) ||
	    (priv->cur_seqid == FSPI_CMD_PP) ||
	    (priv->cur_seqid == FSPI_CMD_PP_4B) ||
	    (priv->cur_seqid == FSPI_CMD_BE_4K) ||
	    (priv->cur_seqid == FSPI_CMD_BE_4K_4B) ||
	    (priv->cur_seqid == FSPI_CMD_WREAR) ||
	    (priv->cur_seqid == FSPI_CMD_BRWR))
		fspi_ahb_invalid(priv);
#endif

	return 0;
}

void fspi_module_disable(struct fsl_fspi_priv *priv, u8 disable)
{
	u32 mcr_val;

	mcr_val = fspi_read32(priv->flags, &priv->regs->mcr0);
	if (disable)
		mcr_val |= FLEXSPI_MCR0_MDIS_MASK;
	else
		mcr_val &= ~FLEXSPI_MCR0_MDIS_MASK;
	fspi_write32(priv->flags, &priv->regs->mcr0, mcr_val);
}

void fspi_cfg_smpr(struct fsl_fspi_priv *priv, u32 clear_bits, u32 set_bits)
{
	return;
#if 0
	u32 smpr_val;

	smpr_val = fspi_read32(priv->flags, &priv->regs->smpr);
	smpr_val &= ~clear_bits;
	smpr_val |= set_bits;
	fspi_write32(priv->flags, &priv->regs->smpr, smpr_val);
#endif
}

__weak void init_clk_fspi(int index)
{
}

#ifndef CONFIG_DM_SPI
static unsigned long spi_bases[] = {
	FSPI0_BASE_ADDR,
};

static unsigned long amba_bases[] = {
	FSPI0_AMBA_BASE,
};

static inline struct fsl_fspi *to_fspi_spi(struct spi_slave *slave)
{
	return container_of(slave, struct fsl_fspi, slave);
}

struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
		unsigned int max_hz, unsigned int mode)
{
	struct fsl_fspi *fspi;
	struct fsl_fspi_regs *regs;
	u32 total_size;

	if (bus >= ARRAY_SIZE(spi_bases))
		return NULL;

	if (cs >= FSL_FSPI_FLASH_NUM)
		return NULL;

	fspi = spi_alloc_slave(struct fsl_fspi, bus, cs);
	if (!fspi)
		return NULL;

#ifdef CONFIG_SYS_FSL_FSPI_BE
	fspi->priv.flags |= FSPI_FLAG_REGMAP_ENDIAN_BIG;
#endif

	init_clk_fspi(bus);

	regs = (struct fsl_fspi_regs *)spi_bases[bus];
	fspi->priv.regs = regs;
	/*
	 * According cs, use different amba_base to choose the
	 * corresponding flash devices.
	 *
	 * If not, only one flash device is used even if passing
	 * different cs using `sf probe`
	 */
	fspi->priv.cur_amba_base = amba_bases[bus] + cs * FSL_FSPI_FLASH_SIZE;

	fspi->slave.max_write_size = TX_BUFFER_SIZE;

#ifdef CONFIG_FSPI_QUAD_SUPPORT
	fspi->slave.mode |= SPI_RX_QUAD;
#endif

	fspi_write32(fspi->priv.flags, &regs->mcr0,
		     FLEXSPI_MCR0_SWRST_MASK);
	do {
		udelay(1);
	} while (0x1 & fspi_read32(fspi->priv.flags, &regs->mcr0));

	/* Disable the module */
	fspi_module_disable(&fspi->priv, 1);

	/* Enable the module and set to proper value*/
	fspi_write32(fspi->priv.flags, &regs->mcr0,
		     0xFFFF0000);

	total_size = FSL_FSPI_FLASH_SIZE * FSL_FSPI_FLASH_NUM >> 10;
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
	fspi_write32(fspi->priv.flags, &regs->flsha1cr0,
		     total_size);
	fspi_write32(fspi->priv.flags, &regs->flsha2cr0,
		     0);
	fspi_write32(fspi->priv.flags, &regs->flshb1cr0,
		     0);
	fspi_write32(fspi->priv.flags, &regs->flshb2cr0,
		     0);

	fspi_set_lut(&fspi->priv);

#ifdef CONFIG_SYS_FSL_FSPI_AHB
	fspi_init_ahb_read(&fspi->priv);
#endif

	fspi_module_disable(&fspi->priv, 0);

	return &fspi->slave;
}

void spi_free_slave(struct spi_slave *slave)
{
	struct fsl_fspi *fspi = to_fspi_spi(slave);

	free(fspi);
}

int spi_claim_bus(struct spi_slave *slave)
{
	return 0;
}

void spi_release_bus(struct spi_slave *slave)
{
	/* Nothing to do */
}

int spi_xfer(struct spi_slave *slave, unsigned int bitlen,
		const void *dout, void *din, unsigned long flags)
{
	struct fsl_fspi *fspi = to_fspi_spi(slave);

	return fspi_xfer(&fspi->priv, bitlen, dout, din, flags);
}

void spi_init(void)
{
	/* Nothing to do */
}
#else
static int fsl_fspi_child_pre_probe(struct udevice *dev)
{
	struct spi_slave *slave = dev_get_parent_priv(dev);

	slave->max_write_size = TX_BUFFER_SIZE;

#ifdef CONFIG_FSPI_QUAD_SUPPORT
	slave->mode |= SPI_RX_QUAD;
#endif

	return 0;
}

static int fsl_fspi_probe(struct udevice *bus)
{
	u32 total_size;
	struct fsl_fspi_platdata *plat = dev_get_platdata(bus);
	struct fsl_fspi_priv *priv = dev_get_priv(bus);
	struct dm_spi_bus *dm_spi_bus;

	init_clk_fspi(bus->seq);

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
	fspi_module_disable(priv, 1);

	/* Enable the module and set to proper value*/
	fspi_write32(priv->flags, &priv->regs->mcr0,
		     0xFFFF0000);

	/* Reset the DLL register to default value */
	fspi_write32(priv->flags, &priv->regs->dllacr, 0x0100);
	fspi_write32(priv->flags, &priv->regs->dllbcr, 0x0100);

	/* Flash Size in KByte */
	total_size = FSL_FSPI_FLASH_SIZE * FSL_FSPI_FLASH_NUM >> 10;

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

	fspi_set_lut(priv);

#ifdef CONFIG_SYS_FSL_FSPI_AHB
	fspi_init_ahb_read(priv);
#endif

	fspi_module_disable(priv, 0);

	return 0;
}

static int fsl_fspi_ofdata_to_platdata(struct udevice *bus)
{
	struct fdt_resource res_regs, res_mem;
	struct fsl_fspi_platdata *plat = bus->platdata;
	const void *blob = gd->fdt_blob;
	int node = bus->of_offset;
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

	debug("%s: regs=<0x%x> <0x%x, 0x%x>, max-frequency=%d, endianess=%s\n",
	      __func__,
	      plat->reg_base,
	      plat->amba_base,
	      plat->amba_total_size,
	      plat->speed_hz,
	      plat->flags & FSPI_FLAG_REGMAP_ENDIAN_BIG ? "be" : "le"
	      );

	return 0;
}

static int fsl_fspi_xfer(struct udevice *dev, unsigned int bitlen,
		const void *dout, void *din, unsigned long flags)
{
	struct fsl_fspi_priv *priv;
	struct udevice *bus;

	bus = dev->parent;
	priv = dev_get_priv(bus);

	return fspi_xfer(priv, bitlen, dout, din, flags);
}

static int fsl_fspi_claim_bus(struct udevice *dev)
{
	struct fsl_fspi_priv *priv;
	struct udevice *bus;
	struct dm_spi_slave_platdata *slave_plat = dev_get_parent_platdata(dev);

	bus = dev->parent;
	priv = dev_get_priv(bus);

	priv->cur_amba_base =
		priv->amba_base[0] + FSL_FSPI_FLASH_SIZE * slave_plat->cs;

	fspi_module_disable(priv, 0);

	return 0;
}

static int fsl_fspi_release_bus(struct udevice *dev)
{
	struct fsl_fspi_priv *priv;
	struct udevice *bus;

	bus = dev->parent;
	priv = dev_get_priv(bus);

	fspi_module_disable(priv, 1);

	return 0;
}

static int fsl_fspi_set_speed(struct udevice *bus, uint speed)
{
	/* Nothing to do */
	return 0;
}

static int fsl_fspi_set_mode(struct udevice *bus, uint mode)
{
	/* Nothing to do */
	return 0;
}

static const struct dm_spi_ops fsl_fspi_ops = {
	.claim_bus	= fsl_fspi_claim_bus,
	.release_bus	= fsl_fspi_release_bus,
	.xfer		= fsl_fspi_xfer,
	.set_speed	= fsl_fspi_set_speed,
	.set_mode	= fsl_fspi_set_mode,
};

static const struct udevice_id fsl_fspi_ids[] = {
	{ .compatible = "fsl,imx8qm-flexspi" },
	{ }
};

U_BOOT_DRIVER(fsl_fspi) = {
	.name	= "fsl_fspi",
	.id	= UCLASS_SPI,
	.of_match = fsl_fspi_ids,
	.ops	= &fsl_fspi_ops,
	.ofdata_to_platdata = fsl_fspi_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct fsl_fspi_platdata),
	.priv_auto_alloc_size = sizeof(struct fsl_fspi_priv),
	.probe	= fsl_fspi_probe,
	.child_pre_probe = fsl_fspi_child_pre_probe,
};
#endif
