/*
 * Freescale QuadSPI driver.
 *
 * Copyright (C) 2014-2015 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <common.h>
#include <malloc.h>
#include <spi.h>

#include <asm/io.h>

#define QUADSPI_AHBMAP_BANK_MAXSIZE SZ_64M

/* The registers */
#define QUADSPI_MCR			0x00
#define QUADSPI_MCR_RESERVED_SHIFT	16
#define QUADSPI_MCR_RESERVED_MASK	(0xF << QUADSPI_MCR_RESERVED_SHIFT)
#define QUADSPI_MCR_MDIS_SHIFT		14
#define QUADSPI_MCR_MDIS_MASK		(1 << QUADSPI_MCR_MDIS_SHIFT)
#define QUADSPI_MCR_CLR_TXF_SHIFT	11
#define QUADSPI_MCR_CLR_TXF_MASK	(1 << QUADSPI_MCR_CLR_TXF_SHIFT)
#define QUADSPI_MCR_CLR_RXF_SHIFT	10
#define QUADSPI_MCR_CLR_RXF_MASK	(1 << QUADSPI_MCR_CLR_RXF_SHIFT)
#define QUADSPI_MCR_DDR_EN_SHIFT	7
#define QUADSPI_MCR_DDR_EN_MASK		(1 << QUADSPI_MCR_DDR_EN_SHIFT)
#define QUADSPI_MCR_END_CFG_SHIFT   2
#define QUADSPI_MCR_END_CFG_MASK    (3 << QUADSPI_MCR_END_CFG_SHIFT)
#define QUADSPI_MCR_SWRSTHD_SHIFT	1
#define QUADSPI_MCR_SWRSTHD_MASK	(1 << QUADSPI_MCR_SWRSTHD_SHIFT)
#define QUADSPI_MCR_SWRSTSD_SHIFT	0
#define QUADSPI_MCR_SWRSTSD_MASK	(1 << QUADSPI_MCR_SWRSTSD_SHIFT)

#define QUADSPI_IPCR			0x08
#define QUADSPI_IPCR_SEQID_SHIFT	24
#define QUADSPI_IPCR_SEQID_MASK		(0xF << QUADSPI_IPCR_SEQID_SHIFT)

#define QUADSPI_BUF0CR			0x10
#define QUADSPI_BUF1CR			0x14
#define QUADSPI_BUF2CR			0x18
#define QUADSPI_BUFXCR_INVALID_MSTRID	0xe

#define QUADSPI_BUF3CR			0x1c
#define QUADSPI_BUF3CR_ALLMST_SHIFT	31
#define QUADSPI_BUF3CR_ALLMST_MASK  (1 << QUADSPI_BUF3CR_ALLMST_SHIFT)
#define QUADSPI_BUF3CR_ADATSZ_SHIFT	8
#define QUADSPI_BUF3CR_ADATSZ_MASK	(0xFF << QUADSPI_BUF3CR_ADATSZ_SHIFT)

#define QUADSPI_BFGENCR			0x20
#define QUADSPI_BFGENCR_PAR_EN_SHIFT	16
#define QUADSPI_BFGENCR_PAR_EN_MASK	(1 << (QUADSPI_BFGENCR_PAR_EN_SHIFT))
#define QUADSPI_BFGENCR_SEQID_SHIFT	12
#define QUADSPI_BFGENCR_SEQID_MASK	(0xF << QUADSPI_BFGENCR_SEQID_SHIFT)

#define QUADSPI_BUF0IND			0x30
#define QUADSPI_BUF1IND			0x34
#define QUADSPI_BUF2IND			0x38
#define QUADSPI_SFAR			0x100

#define QUADSPI_SMPR			0x108
#define QUADSPI_SMPR_DDRSMP_SHIFT	16
#define QUADSPI_SMPR_DDRSMP_MASK	(7 << QUADSPI_SMPR_DDRSMP_SHIFT)
#define QUADSPI_SMPR_FSDLY_SHIFT	6
#define QUADSPI_SMPR_FSDLY_MASK		(1 << QUADSPI_SMPR_FSDLY_SHIFT)
#define QUADSPI_SMPR_FSPHS_SHIFT	5
#define QUADSPI_SMPR_FSPHS_MASK		(1 << QUADSPI_SMPR_FSPHS_SHIFT)
#define QUADSPI_SMPR_HSENA_SHIFT	0
#define QUADSPI_SMPR_HSENA_MASK		(1 << QUADSPI_SMPR_HSENA_SHIFT)

#define QUADSPI_RBSR			0x10c
#define QUADSPI_RBSR_RDBFL_SHIFT	8
#define QUADSPI_RBSR_RDBFL_MASK		(0x3F << QUADSPI_RBSR_RDBFL_SHIFT)

#define QUADSPI_RBCT			0x110
#define QUADSPI_RBCT_WMRK_MASK		0x1F
#define QUADSPI_RBCT_RXBRD_SHIFT	8
#define QUADSPI_RBCT_RXBRD_USEIPS	(0x1 << QUADSPI_RBCT_RXBRD_SHIFT)

#define QUADSPI_TBSR			0x150
#define QUADSPI_TBDR			0x154
#define QUADSPI_SR			0x15c
#define QUADSPI_SR_IP_ACC_SHIFT     1
#define QUADSPI_SR_IP_ACC_MASK      (0x1 << QUADSPI_SR_IP_ACC_SHIFT)
#define QUADSPI_SR_AHB_ACC_SHIFT    2
#define QUADSPI_SR_AHB_ACC_MASK     (0x1 << QUADSPI_SR_AHB_ACC_SHIFT)


#define QUADSPI_FR			0x160
#define QUADSPI_FR_TFF_MASK		0x1

#define QUADSPI_SFA1AD			0x180
#define QUADSPI_SFA2AD			0x184
#define QUADSPI_SFB1AD			0x188
#define QUADSPI_SFB2AD			0x18c
#define QUADSPI_RBDR			0x200

#define QUADSPI_LUTKEY			0x300
#define QUADSPI_LUTKEY_VALUE		0x5AF05AF0

#define QUADSPI_LCKCR			0x304
#define QUADSPI_LCKER_LOCK		0x1
#define QUADSPI_LCKER_UNLOCK		0x2

#define QUADSPI_RSER			0x164
#define QUADSPI_RSER_TFIE		(0x1 << 0)

#define QUADSPI_LUT_BASE		0x310

/*
 * The definition of the LUT register shows below:
 *
 *  ---------------------------------------------------
 *  | INSTR1 | PAD1 | OPRND1 | INSTR0 | PAD0 | OPRND0 |
 *  ---------------------------------------------------
 */
#define OPRND0_SHIFT		0
#define PAD0_SHIFT		8
#define INSTR0_SHIFT		10
#define OPRND1_SHIFT		16

/* Instruction set for the LUT register. */
#define LUT_STOP		0
#define LUT_CMD			1
#define LUT_ADDR		2
#define LUT_DUMMY		3
#define LUT_MODE		4
#define LUT_MODE2		5
#define LUT_MODE4		6
#define LUT_READ		7
#define LUT_WRITE		8
#define LUT_JMP_ON_CS		9
#define LUT_ADDR_DDR		10
#define LUT_MODE_DDR		11
#define LUT_MODE2_DDR		12
#define LUT_MODE4_DDR		13
#define LUT_READ_DDR		14
#define LUT_WRITE_DDR		15
#define LUT_DATA_LEARN		16

/*
 * The PAD definitions for LUT register.
 *
 * The pad stands for the lines number of IO[0:3].
 * For example, the Quad read need four IO lines, so you should
 * set LUT_PAD4 which means we use four IO lines.
 */
#define LUT_PAD1		0
#define LUT_PAD2		1
#define LUT_PAD4		2

/* Oprands for the LUT register. */
#define ADDR24BIT		0x18
#define ADDR32BIT		0x20

/* Macros for constructing the LUT register. */
#define LUT0(ins, pad, opr)						\
		(((opr) << OPRND0_SHIFT) | ((LUT_##pad) << PAD0_SHIFT) | \
		((LUT_##ins) << INSTR0_SHIFT))

#define LUT1(ins, pad, opr)	(LUT0(ins, pad, opr) << OPRND1_SHIFT)

/* other macros for LUT register. */
#define QUADSPI_LUT(x)          (QUADSPI_LUT_BASE + (x) * 4)
#define QUADSPI_LUT_NUM		64

/* SEQID -- we can have 16 seqids at most. */
#define SEQID_QUAD_READ		0
#define SEQID_WREN		1
#define SEQID_FAST_READ		2
#define SEQID_RDSR		3
#define SEQID_SE		4
#define SEQID_CHIP_ERASE	5
#define SEQID_PP		6
#define SEQID_RDID		7
#define SEQID_WRSR		8
#define SEQID_RDCR		9
#define SEQID_DDR_QUAD_READ	10
#define SEQID_BE_4K		11
#ifdef CONFIG_SPI_FLASH_BAR
#define SEQID_BRRD		12
#define SEQID_BRWR		13
#define SEQID_RDEAR		14
#define SEQID_WREAR		15
#endif

/* Flash opcodes. */
#define	OPCODE_WREN		0x06	/* Write enable */
#define	OPCODE_RDSR		0x05	/* Read status register */
#define	OPCODE_WRSR		0x01	/* Write status register 1 byte */
#define	OPCODE_NORM_READ	0x03	/* Read data bytes (low frequency) */
#define	OPCODE_FAST_READ	0x0b	/* Read data bytes (high frequency) */
#define	OPCODE_QUAD_READ        0x6b    /* Read data bytes */
#define	OPCODE_DDR_QUAD_READ	0x6d    /* Read data bytes in DDR mode*/
#define	OPCODE_PP		0x02	/* Page program (up to 256 bytes) */
#define	OPCODE_BE_4K		0x20	/* Erase 4KiB block */
#define	OPCODE_BE_4K_PMC	0xd7	/* Erase 4KiB block on PMC chips */
#define	OPCODE_BE_32K		0x52	/* Erase 32KiB block */
#define	OPCODE_CHIP_ERASE	0xc7	/* Erase whole flash chip */
#define	OPCODE_SE		0xd8	/* Sector erase (usually 64KiB) */
#define	OPCODE_RDID		0x9f	/* Read JEDEC ID */
#define	OPCODE_RDCR             0x35    /* Read configuration register */

/* 4-byte address opcodes - used on Spansion and some Macronix flashes. */
#define	OPCODE_NORM_READ_4B	0x13	/* Read data bytes (low frequency) */
#define	OPCODE_FAST_READ_4B	0x0c	/* Read data bytes (high frequency) */
#define	OPCODE_QUAD_READ_4B	0x6c    /* Read data bytes */
#define	OPCODE_PP_4B		0x12	/* Page program (up to 256 bytes) */
#define	OPCODE_SE_4B		0xdc	/* Sector erase (usually 64KiB) */

/* Used for SST flashes only. */
#define	OPCODE_BP		0x02	/* Byte program */
#define	OPCODE_WRDI		0x04	/* Write disable */
#define	OPCODE_AAI_WP		0xad	/* Auto address increment word program */

/* Used for Macronix and Winbond flashes. */
#define	OPCODE_EN4B		0xb7	/* Enter 4-byte mode */
#define	OPCODE_EX4B		0xe9	/* Exit 4-byte mode */

/* Used for Micron, winbond and Macronix flashes */
#define	OPCODE_WREAR		0xc5	/* EAR register write */
#define	OPCODE_RDEAR		0xc8	/* EAR reigster read */

/* Used for Spansion flashes only. */
#define	OPCODE_BRRD		0x16	/* Bank register read */
#define	OPCODE_BRWR		0x17	/* Bank register write */

/* Status Register bits. */
#define	SR_WIP			1	/* Write in progress */
#define	SR_WEL			2	/* Write enable latch */
/* meaning of other SR_* bits may differ between vendors */
#define	SR_BP0			4	/* Block protect 0 */
#define	SR_BP1			8	/* Block protect 1 */
#define	SR_BP2			0x10	/* Block protect 2 */
#define	SR_SRWD			0x80	/* SR write protect */

#define SR_QUAD_EN_MX           0x40    /* Macronix Quad I/O */

/* Configuration Register bits. */
#define CR_QUAD_EN_SPAN		0x2     /* Spansion Quad I/O */

/* Endianess Configuration */
#define BE_64	0x00
#define LE_32	0x01
#define BE_32	0x02
#define LE_64	0x03



enum fsl_qspi_devtype {
	FSL_QUADSPI_VYBRID,
	FSL_QUADSPI_IMX6SX,
};

struct fsl_qspi_devtype_data {
	enum fsl_qspi_devtype devtype;
	int rxfifo;
	int txfifo;
};

struct fsl_qspi {
	struct spi_slave slave;
	uint32_t max_khz;
	uint32_t mode;
	u32 iobase;
	u32 ahb_base; /* Used when read from AHB bus */
	u32 bank_memmap_phy[4];
	struct fsl_qspi_devtype_data *devtype_data;
};

static inline void fsl_qspi_unlock_lut(struct fsl_qspi *q)
{
	writel(QUADSPI_LUTKEY_VALUE, q->iobase + QUADSPI_LUTKEY);
	writel(QUADSPI_LCKER_UNLOCK, q->iobase + QUADSPI_LCKCR);
}

static inline void fsl_qspi_lock_lut(struct fsl_qspi *q)
{
	writel(QUADSPI_LUTKEY_VALUE, q->iobase + QUADSPI_LUTKEY);
	writel(QUADSPI_LCKER_LOCK, q->iobase + QUADSPI_LCKCR);
}

static void fsl_qspi_init_lut(struct fsl_qspi *q)
{
	u32 base = q->iobase;
	int rxfifo = q->devtype_data->rxfifo;
	u32 lut_base;
	u8 cmd, addrlen, dummy;
	int i;

	fsl_qspi_unlock_lut(q);

	/* Clear all the LUT table */
	for (i = 0; i < QUADSPI_LUT_NUM; i++)
		writel(0, base + QUADSPI_LUT_BASE + i * 4);

	/* Quad Read */
	lut_base = SEQID_QUAD_READ * 4;

	/* U-boot SPI flash only support 24bits address*/
	cmd = OPCODE_QUAD_READ;
	addrlen = ADDR24BIT;
	dummy = 8;

	writel(LUT0(CMD, PAD1, cmd) | LUT1(ADDR, PAD1, addrlen),
			base + QUADSPI_LUT(lut_base));
	writel(LUT0(DUMMY, PAD1, dummy) | LUT1(READ, PAD4, rxfifo),
			base + QUADSPI_LUT(lut_base + 1));

	/* Write enable */
	lut_base = SEQID_WREN * 4;
	writel(LUT0(CMD, PAD1, OPCODE_WREN), base + QUADSPI_LUT(lut_base));

	/* Fast Read */
	lut_base = SEQID_FAST_READ * 4;
	cmd = OPCODE_FAST_READ;
	addrlen = ADDR24BIT;
	dummy = 8;

	writel(LUT0(CMD, PAD1, cmd) | LUT1(ADDR, PAD1, addrlen),
			base + QUADSPI_LUT(lut_base));
	writel(LUT0(DUMMY, PAD1, dummy) | LUT1(READ, PAD1, rxfifo),
			base + QUADSPI_LUT(lut_base + 1));

	/* Page Program */
	lut_base = SEQID_PP * 4;
	cmd = OPCODE_PP;
	addrlen = ADDR24BIT;

	writel(LUT0(CMD, PAD1, cmd) | LUT1(ADDR, PAD1, addrlen),
			base + QUADSPI_LUT(lut_base));
	writel(LUT0(WRITE, PAD1, 0), base + QUADSPI_LUT(lut_base + 1));

	/* Read Status */
	lut_base = SEQID_RDSR * 4;
	writel(LUT0(CMD, PAD1, OPCODE_RDSR) | LUT1(READ, PAD1, 0x1),
			base + QUADSPI_LUT(lut_base));

	/* Erase a sector */
	lut_base = SEQID_SE * 4;
	cmd = OPCODE_SE;
	addrlen = ADDR24BIT;

	writel(LUT0(CMD, PAD1, cmd) | LUT1(ADDR, PAD1, addrlen),
			base + QUADSPI_LUT(lut_base));

	/* Erase the whole chip */
	lut_base = SEQID_CHIP_ERASE * 4;
	writel(LUT0(CMD, PAD1, OPCODE_CHIP_ERASE),
			base + QUADSPI_LUT(lut_base));

	/* READ ID */
	lut_base = SEQID_RDID * 4;
	writel(LUT0(CMD, PAD1, OPCODE_RDID) | LUT1(READ, PAD1, 0x8),
			base + QUADSPI_LUT(lut_base));

	/* Write Register */
	lut_base = SEQID_WRSR * 4;
	writel(LUT0(CMD, PAD1, OPCODE_WRSR) | LUT1(WRITE, PAD1, 0x2),
			base + QUADSPI_LUT(lut_base));

	/* Read Configuration Register */
	lut_base = SEQID_RDCR * 4;
	writel(LUT0(CMD, PAD1, OPCODE_RDCR) | LUT1(READ, PAD1, 0x1),
			base + QUADSPI_LUT(lut_base));

	/* DDR QUAD Read */
	lut_base = SEQID_DDR_QUAD_READ * 4;
	cmd = OPCODE_DDR_QUAD_READ;
	addrlen = ADDR24BIT;
	dummy = 6;

	writel(LUT0(CMD, PAD1, cmd) | LUT1(ADDR_DDR, PAD1, addrlen),
			base + QUADSPI_LUT(lut_base));
	writel(LUT0(DUMMY, PAD1, dummy) | LUT1(READ_DDR, PAD4, rxfifo),
			base + QUADSPI_LUT(lut_base + 1));
	writel(LUT0(JMP_ON_CS, PAD1, 0),
			base + QUADSPI_LUT(lut_base + 2));

	/* SUB SECTOR 4K ERASE */
	lut_base = SEQID_BE_4K * 4;
	cmd = OPCODE_BE_4K;
	addrlen = ADDR24BIT;

	writel(LUT0(CMD, PAD1, cmd) | LUT1(ADDR, PAD1, addrlen),
			base + QUADSPI_LUT(lut_base));

#ifdef CONFIG_SPI_FLASH_BAR
	/*
	 * BRRD BRWR RDEAR WREAR are all supported, because it is hard to
	 * dynamically check whether to set BRRD BRWR or RDEAR WREAR.
	 */
	lut_base = SEQID_BRRD * 4;
	cmd = OPCODE_BRRD;
	writel(LUT0(CMD, PAD1, cmd) | LUT1(READ, PAD1, 0x1),
	       base + QUADSPI_LUT(lut_base));

	lut_base = SEQID_BRWR * 4;
	cmd = OPCODE_BRWR;
	writel(LUT0(CMD, PAD1, cmd) | LUT1(WRITE, PAD1, 0x1),
	       base + QUADSPI_LUT(lut_base));

	lut_base = SEQID_RDEAR * 4;
	cmd = OPCODE_RDEAR;
	writel(LUT0(CMD, PAD1, cmd) | LUT1(READ, PAD1, 0x1),
	       base + QUADSPI_LUT(lut_base));

	lut_base = SEQID_WREAR * 4;
	cmd = OPCODE_WREAR;
	writel(LUT0(CMD, PAD1, cmd) | LUT1(WRITE, PAD1, 0x1),
	       base + QUADSPI_LUT(lut_base));
#endif

	fsl_qspi_lock_lut(q);
}

/*Enable DDR Read Mode*/
static void fsl_enable_ddr_mode(struct fsl_qspi *q)
{
	u32 base = q->iobase;
	u32 reg, reg2;

	reg = readl(base + QUADSPI_MCR);
	/* Firstly, disable the module */
	writel(reg | QUADSPI_MCR_MDIS_MASK, base + QUADSPI_MCR);

	/* Set the Sampling Register for DDR */
	reg2 = readl(base + QUADSPI_SMPR);
	reg2 &= ~QUADSPI_SMPR_DDRSMP_MASK;
	reg2 |= (2 << QUADSPI_SMPR_DDRSMP_SHIFT);
	writel(reg2, base + QUADSPI_SMPR);

	/* Enable the module again (enable the DDR too) */
	reg |= QUADSPI_MCR_DDR_EN_MASK;
	reg |= (1 << 29); /* enable bit 29 for imx6sx */

	writel(reg, base + QUADSPI_MCR);

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
static void fsl_qspi_init_abh_read(struct fsl_qspi *q)
{
	u32 base = q->iobase;

	/* Map the SPI NOR to accessiable address, arrage max space for each bank*/
	writel(q->bank_memmap_phy[0] + QUADSPI_AHBMAP_BANK_MAXSIZE,
		base + QUADSPI_SFA1AD);
	writel(q->bank_memmap_phy[1] + QUADSPI_AHBMAP_BANK_MAXSIZE,
		base + QUADSPI_SFA2AD);
	writel(q->bank_memmap_phy[2] + QUADSPI_AHBMAP_BANK_MAXSIZE,
		base + QUADSPI_SFB1AD);
	writel(q->bank_memmap_phy[3] + QUADSPI_AHBMAP_BANK_MAXSIZE,
		base + QUADSPI_SFB2AD);

	/* AHB configuration for access buffer 0/1/2 .*/
	writel(QUADSPI_BUFXCR_INVALID_MSTRID, base + QUADSPI_BUF0CR);
	writel(QUADSPI_BUFXCR_INVALID_MSTRID, base + QUADSPI_BUF1CR);
	writel(QUADSPI_BUFXCR_INVALID_MSTRID, base + QUADSPI_BUF2CR);
	writel(QUADSPI_BUF3CR_ALLMST_MASK | (0x80 << QUADSPI_BUF3CR_ADATSZ_SHIFT),
			base + QUADSPI_BUF3CR);

	/* We only use the buffer3 */
	writel(0, base + QUADSPI_BUF0IND);
	writel(0, base + QUADSPI_BUF1IND);
	writel(0, base + QUADSPI_BUF2IND);

	/* Set the default lut sequence for AHB Read. */
	writel(SEQID_FAST_READ << QUADSPI_BFGENCR_SEQID_SHIFT,
		base + QUADSPI_BFGENCR);

	/*Enable DDR Mode*/
	fsl_enable_ddr_mode(q);
}

static int fsl_qspi_init(struct fsl_qspi *q)
{
	u32 base = q->iobase;
	u32 reg;
	void *ptr;

	ptr = malloc(sizeof(struct fsl_qspi_devtype_data));
	if (!ptr) {
		puts("FSL_QSPI: per-type data not allocated !\n");
		return 1;
	}
	q->devtype_data = ptr;
	q->devtype_data->rxfifo = 128;
	q->devtype_data->txfifo = 512;

	/* init the LUT table */
	fsl_qspi_init_lut(q);

	/* Disable the module */
	writel(QUADSPI_MCR_MDIS_MASK | QUADSPI_MCR_RESERVED_MASK,
			base + QUADSPI_MCR);

	reg = readl(base + QUADSPI_SMPR);
	writel(reg & ~(QUADSPI_SMPR_FSDLY_MASK
			| QUADSPI_SMPR_FSPHS_MASK
			| QUADSPI_SMPR_HSENA_MASK
			| QUADSPI_SMPR_DDRSMP_MASK), base + QUADSPI_SMPR);

	/* Enable the module */
	writel(QUADSPI_MCR_RESERVED_MASK | LE_64 << QUADSPI_MCR_END_CFG_SHIFT,
		base + QUADSPI_MCR);

	/* We do not enable the interrupt */

	/* init for AHB read */
	fsl_qspi_init_abh_read(q);

	/*
	 * High level code use page_size and max_write_size to calculate
	 * the number of bytes that should be programmed once.
	 */
	q->slave.max_write_size = q->devtype_data->txfifo;

	return 0;
}

void spi_init(void)
{
}

struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
		unsigned int max_hz, unsigned int mode)
{
	struct fsl_qspi *q;
	int ret;

	if (bus > 1) {
		puts("FSL_QSPI: Not a valid bus !\n");
		return NULL;
	}

	if (cs > 1) {
		puts("FSL_QSPI: Not a valid cs !\n");
		return NULL;
	}

	q = spi_alloc_slave(struct fsl_qspi, bus, cs);
	if (!q) {
		puts("FSL_QSPI: SPI Slave not allocated !\n");
		return NULL;
	}

	q->iobase = CONFIG_QSPI_BASE;
	q->bank_memmap_phy[0] = CONFIG_QSPI_MEMMAP_BASE;
	q->bank_memmap_phy[1] = q->bank_memmap_phy[0] + QUADSPI_AHBMAP_BANK_MAXSIZE;
	q->bank_memmap_phy[2] = q->bank_memmap_phy[1] + QUADSPI_AHBMAP_BANK_MAXSIZE;
	q->bank_memmap_phy[3] = q->bank_memmap_phy[2] + QUADSPI_AHBMAP_BANK_MAXSIZE;

	/* Init the QuadSPI controller */
	ret = fsl_qspi_init(q);
	if (ret) {
		puts("FSL_QSPI: init failed!\n");
		return NULL;
	}

	return &q->slave;
}

void spi_free_slave(struct spi_slave *slave)
{
	struct fsl_qspi *q;

	q = container_of(slave, struct fsl_qspi, slave);
	free(q->devtype_data);
	free(q);
}

int spi_claim_bus(struct spi_slave *q)
{
	return 0;
}

void spi_release_bus(struct spi_slave *q)
{
}

/* Get the SEQID for the command */
static int fsl_qspi_get_seqid(struct fsl_qspi *q, u8 cmd)
{
	switch (cmd) {
	case OPCODE_QUAD_READ:
	case OPCODE_QUAD_READ_4B:
		return SEQID_QUAD_READ;
	case OPCODE_FAST_READ:
	case OPCODE_FAST_READ_4B:
		return SEQID_FAST_READ;
	case OPCODE_WREN:
		return SEQID_WREN;
	case OPCODE_RDSR:
		return SEQID_RDSR;
	case OPCODE_SE:
		return SEQID_SE;
	case OPCODE_CHIP_ERASE:
		return SEQID_CHIP_ERASE;
	case OPCODE_PP:
	case OPCODE_PP_4B:
		return SEQID_PP;
	case OPCODE_RDID:
		return SEQID_RDID;
	case OPCODE_WRSR:
		return SEQID_WRSR;
	case OPCODE_RDCR:
		return SEQID_RDCR;
	case OPCODE_DDR_QUAD_READ:
		return SEQID_DDR_QUAD_READ;
	case OPCODE_BE_4K:
		return SEQID_BE_4K;
#ifdef CONFIG_SPI_FLASH_BAR
	case OPCODE_BRRD:
		return SEQID_BRRD;
	case OPCODE_BRWR:
		return SEQID_BRWR;
	case OPCODE_RDEAR:
		return SEQID_RDEAR;
	case OPCODE_WREAR:
		return SEQID_WREAR;
#endif
	default:
		break;
	}
	return -1;
}

/* return 1 on success */
static int fsl_qspi_wait_to_complete(struct fsl_qspi *q)
{
	u32 base = q->iobase;
	u32 reg;

	/*printf("QuadSPI: poll the busy bit\n");*/
	while (1) {
		reg = readl(base + QUADSPI_SR);
		if (reg & 1)
			continue;
		else
			return 1;
	}

	return 0;
}

/*
 * If we have changed the content of the flash by writing or erasing,
 * we need to invalidate the AHB buffer. If we do not do so, we may read out
 * the wrong data. The spec tells us reset the AHB domain and Serial Flash
 * domain at the same time.
 */
static inline void fsl_qspi_invalid(struct fsl_qspi *q)
{
    u32 reg;

    reg = readl(q->iobase + QUADSPI_MCR);
    reg |= QUADSPI_MCR_SWRSTHD_MASK | QUADSPI_MCR_SWRSTSD_MASK;
    writel(reg, q->iobase + QUADSPI_MCR);

    /*
     * The minimum delay : 1 AHB + 2 SFCK clocks.
     * Delay 1 us is enough.
     */
    udelay(1);

    reg &= ~(QUADSPI_MCR_SWRSTHD_MASK | QUADSPI_MCR_SWRSTSD_MASK);
    writel(reg, q->iobase + QUADSPI_MCR);
}

static int
fsl_qspi_runcmd(struct fsl_qspi *q, u8 cmd, unsigned int addr, int len)
{
	u32 base = q->iobase;
	int seqid;
	u32 reg, reg2;
	int err;
	int bank_id;

	/* check the SR first, wait previous cmd completed*/
	do {
		reg2 = readl(base + QUADSPI_SR);
		if (reg2 & (QUADSPI_SR_IP_ACC_MASK | QUADSPI_SR_AHB_ACC_MASK)) {
			udelay(1);
			printf("The controller is busy, 0x%x\n", reg2);
			continue;
		}
		break;
	} while (1);

	/* save the reg */
	reg = readl(base + QUADSPI_MCR);

	/* get the bank index */
	bank_id = ((q->slave.bus) << 1) + (q->slave.cs);

	writel(q->bank_memmap_phy[bank_id] + addr, base + QUADSPI_SFAR);
	writel(QUADSPI_RBCT_WMRK_MASK | QUADSPI_RBCT_RXBRD_USEIPS,
			base + QUADSPI_RBCT);
	writel(reg | QUADSPI_MCR_CLR_RXF_MASK, base + QUADSPI_MCR);

	/* trigger the LUT now */
	seqid = fsl_qspi_get_seqid(q, cmd);
	writel((seqid << QUADSPI_IPCR_SEQID_SHIFT) | len, base + QUADSPI_IPCR);

	/* Wait until completed */
	err = fsl_qspi_wait_to_complete(q);
	if (!err)
		err = -1;
	else
		err = 0;

	/* restore the MCR */
	writel(reg, base + QUADSPI_MCR);

	/* After switch BANK, AHB buffer should also be invalid. */
	if ((OPCODE_SE == cmd) || (OPCODE_PP == cmd) ||
	    (OPCODE_BE_4K == cmd) || (OPCODE_WREAR == cmd) ||
	    (OPCODE_BRWR == cmd))
		fsl_qspi_invalid(q);
	return err;
}

/*
 * An IC bug makes us to re-arrange the 32-bit data.
 * The following chips, such as IMX6SLX, have fixed this bug.
 */
static inline u32 fsl_qspi_endian_xchg(struct fsl_qspi *q, u32 a)
{
	return a;
}

/* Read out the data from the AHB buffer. */
static void fsl_qspi_ahb_read(struct fsl_qspi *q,
	unsigned int addr, int len, u8 *rxbuf)
{
	int bank_id;

	/* get the bank index */
	bank_id = ((q->slave.bus) << 1) + (q->slave.cs);

	/* Read out the data directly from the AHB buffer.*/
	memcpy(rxbuf, (u8 *)(q->bank_memmap_phy[bank_id] + addr), len);
}

/* Read out the data from the QUADSPI_RBDR buffer registers. */
static void fsl_qspi_ip_read(struct fsl_qspi *q, int len, u8 *rxbuf)
{
	u32 tmp;
	int i = 0;

	while (len > 0) {
		tmp = readl(q->iobase + QUADSPI_RBDR + i * 4);
		tmp = fsl_qspi_endian_xchg(q, tmp);

		if (len >= 4) {
			memcpy(rxbuf, &tmp, 4);
			rxbuf += 4;
		} else {
			memcpy(rxbuf, &tmp, len);
			break;
		}

		len -= 4;
		i++;
	}
}

/* Write data to the QUADSPI_TBDR buffer registers. */
static void fsl_qspi_write_data(struct fsl_qspi *q, int len, u8* txbuf)
{
	u32 tmp;
	u32 t1, t2;
	int j;

	/* clear the TX FIFO. */
	tmp = readl(q->iobase + QUADSPI_MCR);
	writel(tmp | QUADSPI_MCR_CLR_TXF_MASK, q->iobase + QUADSPI_MCR);

	/* fill the TX data to the FIFO */
	t2 = len % 4;
	t1 = len >> 2; /* 4 Bytes aligned */

	for (j = 0; j < t1; j++) {
		memcpy(&tmp, txbuf, 4);
		tmp = fsl_qspi_endian_xchg(q, tmp);
		writel(tmp, q->iobase + QUADSPI_TBDR);
		txbuf += 4;
	}

	if (t2) {
		tmp = 0;
		memcpy(&tmp, txbuf, t2);
		tmp = fsl_qspi_endian_xchg(q, tmp);
		writel(tmp, q->iobase + QUADSPI_TBDR);
	}

#if defined(CONFIG_MX7D) || defined(CONFIG_MX6UL)
	u32 t3;
	/* iMX7D and MX6UL TXFIFO must be at least 16 bytes*/
	t3 = t1 + ((t2 + 3) >> 2);
	for (; t3 < 4; t3++)
		writel(0, q->iobase + QUADSPI_TBDR);
#endif

}

/* see the spi_flash_read_write() */
int  spi_xfer(struct spi_slave *slave, unsigned int bitlen, const void *dout,
		void *din, unsigned long flags)
{
	struct fsl_qspi *q = container_of(slave, struct fsl_qspi, slave);
	int len = bitlen / 8;
	int ret = 0;
	u8 *buf;
	static u8 opcode;
	static unsigned int addr;

	if (!opcode && (flags & SPI_XFER_BEGIN)) {
		/* spi_xfer for cmd phase */
		buf = (u8 *)dout;
		opcode = buf[0];
		if (len > 1)
			addr = buf[1] << 16 | buf[2] << 8 | buf[3];

		/* if transfer cmd only */
		if (flags & SPI_XFER_END)
			ret = fsl_qspi_runcmd(q, opcode, addr, 0);

	} else if (opcode) {
		/* spi_xfer for data phase */
		if (din) {
			/* read*/
			buf = (u8 *)din;
			if (OPCODE_FAST_READ == opcode) {
				fsl_qspi_ahb_read(q, addr, len, buf);
			} else {
				ret = fsl_qspi_runcmd(q, opcode, addr, len);
				if (!ret)
					fsl_qspi_ip_read(q, len, buf);
			}
		} else if (dout) {
			/* write data, prepare data first */
			buf = (u8 *)dout;
			fsl_qspi_write_data(q, len, buf);
			/* then run page program cmd */
			ret = fsl_qspi_runcmd(q, opcode, addr, len);
		}
	}

	if (ret || (flags & SPI_XFER_END)) {
		opcode = 0;
		addr = 0;
	}

	return ret;
}
