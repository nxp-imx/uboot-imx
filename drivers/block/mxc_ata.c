/*
 * (C) Copyright 2008-2009 Freescale Semiconductor, Inc.
 *
 * See file CREDITS for	list of people who contributed to this
 * project.
 *
 * This	program	is free	software; you can redistribute it and/or
 * modify it under the terms of	the GNU General Public License as
 * published by	the Free Software Foundation; either version 2 of
 * the License,	or (at your option) any later version.
 *
 * This	program	is distributed in the hope that	it will	be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59	Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include "mxc_ata.h"

#include <pata.h>
#include <libata.h>
#include <common.h>
#include <malloc.h>
#include <asm/io.h>
#include <linux/types.h>

extern block_dev_desc_t pata_dev_desc[CONFIG_SYS_ATA_MAX_DEVICE];
extern void setup_ata(void);

struct mxc_ata_time_regs {
	u8 time_off, time_on, time_1, time_2w;
	u8 time_2r, time_ax, time_pio_rdx, time_4;
	u8 time_9, time_m, time_jn, time_d;
	u8 time_k, time_ack, time_env, time_rpx;
	u8 time_zah, time_mlix, time_dvh, time_dzfs;
	u8 time_dvs, time_cvh, time_ss, time_cyc;
};

/*
 * This structure contains the timing parameters for
 * ATA bus timing in the 5 PIO modes.  The timings
 * are in nanoseconds, and are converted to clock
 * cycles before being stored in the ATA controller
 * timing registers.
 */
static struct {
	s16 t0, t1, t2_8, t2_16, t2i, t4, t9, tA;
} pio_specs[] = {
	[0] = {
	.t0 = 600, .t1 = 70, .t2_8 = 290, .t2_16 = 165, .t2i = 40, .t4 =
		30, .t9 = 20, .tA = 50,},
	[1] = {
	.t0 = 383, .t1 = 50, .t2_8 = 290, .t2_16 = 125, .t2i = 0, .t4 =
		20, .t9 = 15, .tA = 50,},
	[2] = {
	.t0 = 240, .t1 = 30, .t2_8 = 290, .t2_16 = 100, .t2i = 0, .t4 =
		15, .t9 = 10, .tA = 50,},
	[3] = {
	.t0 = 180, .t1 = 30, .t2_8 = 80, .t2_16 = 80, .t2i = 0, .t4 =
		10, .t9 = 10, .tA = 50,},
	[4] = {
	.t0 = 120, .t1 = 25, .t2_8 = 70, .t2_16 = 70, .t2i = 0, .t4 =
		10, .t9 = 10, .tA = 50,},
	};

#define NR_PIO_SPECS (sizeof(pio_specs) / sizeof(pio_specs[0]))

static inline void mdelay(u32 msec)
{
	u32 i;
	for (i = 0; i < msec; i++)
		udelay(1000);
}

static inline void sdelay(u32 sec)
{
	u32 i;
	for (i = 0; i < sec; i++)
		mdelay(1000);
}

void dprint_buffer(u8 *buf, s32 len)
{
	s32 i, j;

	i = 0;
	j = 0;
	printf("\n\r");

	for (i = 0; i < len; i++) {
		printf("%02x ", *buf++);
		j++;
		if (j == 16) {
			printf("\n\r");
			j = 0;
		}
	}
	printf("\n\r");
}


static void update_timing_config(struct mxc_ata_time_regs *tp)
{
	u32 *lp = (u32 *)tp;
	u32 *ctlp = (u32 *) ATA_BASE_ADDR;
	s32 i;

	for (i = 0; i < 5; ++i) {
		writel(*lp, ctlp);
		lp++;
		ctlp++;
	}
}

static void set_ata_bus_timing(u8 xfer_mode)
{
	s32 speed = xfer_mode;
	struct mxc_ata_time_regs tr = { 0 };
	s32 T = 1 * 1000 * 1000 * 1000 / mxc_get_clock(MXC_IPG_CLK);

	if (speed >= NR_PIO_SPECS)
		return;

	tr.time_off = 3;
	tr.time_on = 3;

	tr.time_1 = (pio_specs[speed].t1 + T) / T;
	tr.time_2w = (pio_specs[speed].t2_8 + T) / T;

	tr.time_2r = (pio_specs[speed].t2_8 + T) / T;
	tr.time_ax = (pio_specs[speed].tA + T) / T + 2;
	tr.time_pio_rdx = 1;
	tr.time_4 = (pio_specs[speed].t4 + T) / T;

	tr.time_9 = (pio_specs[speed].t9 + T) / T;

	update_timing_config(&tr);
}

static u8 ata_sff_busy_wait(u32 bits, u32 max, u32 delay)
{
	u8 status;
	u32 iterations = 1;

	if (max != 0)
		iterations = max;

	do {
		udelay(delay);
		status = readb(CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_DCDR);
		if (max != 0)
			iterations--;
	} while (status != 0xff && (status & bits) && (iterations > 0));

	if (iterations == 0) {
		printf("ata_sff_busy_wait timeout status = %x\n", status);
		return 0xff;
	}

	return status;
}

static void ata_sff_exec_command(u16 cmd)
{
	writeb(cmd, CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_DCDR);
	readb(CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_DRIVE_CONTROL);
	udelay(4);
}

static s32 ata_identify(int dev, u16 *id)
{
	int i;
	int CIS[256] = { 0 };

	ata_sff_exec_command(ATA_CMD_ID_ATA);
	if (ata_sff_busy_wait(ATA_BUSY, 5000, 500) == 0xff)
		return -1;

	for (i = 0 ; i < 256; ++i)
		id[i] = (u16)readw(CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_DRIVE_DATA);

	ata_swap_buf_le16(id, ATA_ID_WORDS);

	return 0;
}

static s32 ata_dev_set_feature(int dev, u32 feature)
{
	u8 status;

	writeb(feature, CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_DFTR);
	/* Issue Set feature command */
	ata_sff_exec_command(ATA_CMD_SET_FEATURES);
	status = ata_sff_busy_wait(ATA_BUSY, 5000, 500);

	if (status == 0xff)
		return 1;
	if (status & ATA_ERR)
		return 1;

	return 0;
}

static void write_sector_pio(u32 *addr, s32 num_of_sectors)
{
	int i, j;

	for (i = 0; i < num_of_sectors; i++) {
		for (j = 0; j < ATA_SECTOR_SIZE; j = j + 4) {
			/* Write 4 bytes in each iteration */
			writew((*addr & 0xFFFF),
					ATA_BASE_ADDR + MXC_ATA_DRIVE_DATA) ;
			writew(((*addr >> 16) & 0xFFFF),
					ATA_BASE_ADDR + MXC_ATA_DRIVE_DATA) ;
			addr++;
		}
		ata_sff_busy_wait(ATA_BUSY, 5000, 50);
	}
	readb(CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_DRIVE_CONTROL);
}

static void read_sector_pio(u32 *addr, s32 num_of_sectors)
{
	int i, j;
	u32 data[2];

	for (i = 0; i < num_of_sectors; i++) {
		for (j = 0; j < ATA_SECTOR_SIZE; j = j + 4) {
			/* Read 4 bytes in each iteration */
			data[0] = readw(CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_DRIVE_DATA);
			data[1] = readw(CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_DRIVE_DATA);
			*addr = ((data[1] << 16) & 0xFFFF0000) | \
					(data[0] & 0xFFFF);
			addr++;
		}
		ata_sff_busy_wait(ATA_BUSY, 5000, 10);
	}
	readb(CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_DRIVE_CONTROL);
}

static s32 mxc_pata_reset(int dev)
{
	setup_ata();

	/* Deassert the reset bit to enable the interface */
	writel(MXC_ATA_CTRL_ATA_RST_B, \
			CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_CONTROL);
	writel(MXC_ATA_CTRL_ATA_RST_B | MXC_ATA_CTRL_FIFO_RST_B, \
			CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_CONTROL);
	/* Set initial timing and mode */
	set_ata_bus_timing(PIO_XFER_MODE_4);
	/* set fifo alarm to 20 halfwords, midway */
	writeb(20, CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_FIFO_ALARM);

	/* software reset */
	writeb(ATA_NIEN, \
			CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_DRIVE_CONTROL);
	udelay(20);
	writeb(ATA_NIEN | ATA_SRST, \
			CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_DRIVE_CONTROL);
	udelay(20);
	writeb(ATA_NIEN, \
			CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_DRIVE_CONTROL);

	writeb(0, CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_DDHR);

	if (ata_sff_busy_wait(ATA_BUSY | ATA_DRQ, \
				6000, 1000) == 0xff) {
		puts("Failed to initialize the ATA drive\n");
		return -1;
	}

	return 0;
}

/*
 * PATA interface between low level driver and command layer
 */
int init_pata(int dev)
{
	if (dev < 0 || dev > (CONFIG_SYS_ATA_MAX_DEVICE - 1)) {
		printf("the pata index %d is out of ranges\n\r", dev);
		return -1;
	}

	return mxc_pata_reset(dev);
}

ulong pata_read(int dev, ulong blknr, ulong blkcnt, void *buffer)
{
	u8  lba_addr[4] = { 0 };
	u32 num_of_sectors = 0;
	u8  status = 0;
	u32 total_blks = blkcnt;

	while (blkcnt > 0) {
		lba_addr[0] = blknr & 0xFF;
		lba_addr[1] = (blknr >> 8) & 0xFF;
		lba_addr[2] = (blknr >> 16) & 0xFF;
		/* Enable the LBA bit */
		lba_addr[3] = (1 << 6) | ((blknr >> 24) & 0xF);

		if (blkcnt >= MAX_SECTORS)
			num_of_sectors = 0;
		else
			num_of_sectors = blkcnt;

		ata_sff_busy_wait(ATA_BUSY | ATA_DRQ, 5000, 50);
		writeb(num_of_sectors, CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_DSCR);
		writeb(lba_addr[0], CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_DSNR);
		writeb(lba_addr[1], CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_DCLR);
		writeb(lba_addr[2], CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_DCHR);
		writeb(lba_addr[3], CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_DDHR);

		/* Issue Read command */
		ata_sff_exec_command(ATA_CMD_PIO_READ);
		status = ata_sff_busy_wait(ATA_BUSY, 5000, 50);
		if (status & ATA_ERR) {
			puts("Error while issuing ATA Read command\n");
			return -1;
		}
		if (num_of_sectors == 0) {
			read_sector_pio((u32 *)buffer, MAX_SECTORS);
			blkcnt -= MAX_SECTORS;
			blknr += MAX_SECTORS;
			buffer += (MAX_SECTORS * ATA_SECTOR_SIZE);
		} else {
			read_sector_pio((u32 *)buffer, num_of_sectors);
			blkcnt -= num_of_sectors;
			blknr += num_of_sectors;
			buffer += (num_of_sectors * ATA_SECTOR_SIZE);
		}
	}

	return total_blks;
}

ulong pata_write(int dev, ulong blknr, ulong blkcnt, const void *buffer)
{
	s32 lba_addr[4] = { 0 },
		num_of_sectors = 0;
	u8  status = 0;
	u32 total_blks = blkcnt;

	while (blkcnt > 0) {
		lba_addr[0] = blknr & 0xFF;
		lba_addr[1] = (blknr >> 8) & 0xFF;
		lba_addr[2] = (blknr >> 16) & 0xFF;
		/* Enable the LBA bit */
		lba_addr[3] = (1 << 6) | ((blknr >> 24) & 0xF);

		if (blkcnt >= MAX_SECTORS)
			num_of_sectors = 0;
		else
			num_of_sectors = blkcnt;

		ata_sff_busy_wait(ATA_BUSY | ATA_DRQ, 5000, 50);
		writeb(num_of_sectors, CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_DSCR);
		writeb(lba_addr[0], CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_DSNR);
		writeb(lba_addr[1], CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_DCLR);
		writeb(lba_addr[2], CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_DCHR);
		writeb(lba_addr[3], CONFIG_SYS_ATA_BASE_ADDR + MXC_ATA_DDHR);

		/* Issue Write command */
		ata_sff_exec_command(ATA_CMD_PIO_WRITE);
		ata_sff_busy_wait(ATA_BUSY, 5000, 50);
		if (status & ATA_ERR) {
			puts("Error while issuing ATA Write command\n");
			return -1;
		}
		if (num_of_sectors == 0) {
			write_sector_pio((u32 *)buffer, MAX_SECTORS);
			blkcnt -= MAX_SECTORS;
			blknr += MAX_SECTORS;
			buffer += (MAX_SECTORS * ATA_SECTOR_SIZE);
		} else {
			write_sector_pio((u32 *)buffer, num_of_sectors);
			blkcnt -= num_of_sectors;
			blknr += num_of_sectors;
			buffer += (num_of_sectors * ATA_SECTOR_SIZE);
		}
	}

	return total_blks;
}

int scan_pata(int dev)
{
	u8 serial[ATA_ID_SERNO_LEN + 1];
	u8 firmware[ATA_ID_FW_REV_LEN + 1];
	u8 product[ATA_ID_PROD_LEN + 1];
	u16 *id;
	u64 n_sectors;

	id = (u16 *)malloc(ATA_ID_WORDS * 2);
	if (!id) {
		printf("id malloc failed\n\r");
		return -1;
	}

retry:
	/* Identify device to get information */
	ata_identify(dev, id);

	if (ata_id_is_ata(id)) {
		if ((id[2] == 0x37c8 || id[2] == 0x738c)) {
			s32 err_mask = 0;

			err_mask = ata_dev_set_feature(dev, SETFEATURES_SPINUP);
			if (err_mask && id[2] != 0x738c) {
				puts("ATA SPINUP Failed \n");
				return -1;
			}
			if (id[2] == 0x37c8)
				goto retry;
		}
	} else {
		puts("ATA IDENTIFY DEVICE command Failed \n");
	}

	/* Serial number */
	ata_id_c_string(id, serial, ATA_ID_SERNO, sizeof(serial));
	memcpy(pata_dev_desc[dev].product, serial, sizeof(serial));

	/* Firmware version */
	ata_id_c_string(id, firmware, ATA_ID_FW_REV, sizeof(firmware));
	memcpy(pata_dev_desc[dev].revision, firmware, sizeof(firmware));

	/* Product model */
	ata_id_c_string(id, product, ATA_ID_PROD, sizeof(product));
	memcpy(pata_dev_desc[dev].vendor, product, sizeof(product));

	/* Totoal sectors */
	n_sectors = ata_id_n_sectors(id);
	pata_dev_desc[dev].lba = (u32)n_sectors;

	/* Check if support LBA48 */
	if (ata_id_has_lba48(id)) {
		pata_dev_desc[dev].lba48 = 1;
		debug("Device support LBA48\n\r");
	}

#ifdef DEBUG
	fsl_pata_identify(dev, id);
	ata_dump_id(id);
#endif
	free((void *)id);
	return 0;
}
