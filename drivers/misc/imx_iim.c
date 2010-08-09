/*
 * (C) Copyright 2008-2010 Freescale Semiconductor, Inc.
 * Terry Lv
 *
 * Copyright 2007, Freescale Semiconductor, Inc
 * Andy Fleming
 *
 * Based vaguely on the pxa mmc code:
 * (C) Copyright 2003
 * Kyle Harris, Nexus Technologies, Inc. kharris@nexus-tech.net
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <linux/types.h>
#include <asm/io.h>
#include <asm/imx_iim.h>
#include <common.h>
#include <net.h>

static const struct iim_regs *imx_iim =
		(struct iim_regs *)IMX_IIM_BASE;

/*
static void quick_itoa(u32 num, char *a)
{
	int i, j, k;
	for (i = 0; i <= 7; i++) {
		j = (num >> (4 * i)) & 0xf;
		k = (j < 10) ? '0' : ('a' - 0xa);
		a[i] = j + k;
	}
}
*/

/* slen - streng length, e.g.: 23 -> slen=2; abcd -> slen=4 */
/* only convert hex value as string input. so "12" is 0x12. */
static u32 quick_atoi(char *a, u32 slen)
{
	u32 i, num = 0, digit;

	for (i = 0; i < slen; i++) {
		if (a[i] >= '0' && a[i] <= '9') {
			digit = a[i] - '0';
		} else if (a[i] >= 'a' && a[i] <= 'f') {
			digit = a[i] - 'a' + 10;
		} else if (a[i] >= 'A' && a[i] <= 'F') {
			digit = a[i] - 'A' + 10;
		} else {
			printf("ERROR: %c\n", a[i]);
			return -1;
		}
		num = (num * 16) + digit;
	}

    return num;
}

static void fuse_op_start(void)
{
	/* Do not generate interrupt */
	writel(0, &(imx_iim->statm));
	/* clear the status bits and error bits */
	writel(0x3, &(imx_iim->stat));
	writel(0xfe, &(imx_iim->err));
}

/*
 * The action should be either:
 *          POLL_FUSE_PRGD
 * or:
 *          POLL_FUSE_SNSD
 */
static s32 poll_fuse_op_done(s32 action)
{
	u32 status, error;

	if (action != POLL_FUSE_PRGD && action != POLL_FUSE_SNSD) {
		printf("%s(%d) invalid operation\n", __func__, action);
		return -1;
	}

	/* Poll busy bit till it is NOT set */
	while ((readl(&(imx_iim->stat)) & IIM_STAT_BUSY) != 0)
		;

	/* Test for successful write */
	status = readl(&(imx_iim->stat));
	error = readl(&(imx_iim->err));

	if ((status & action) != 0 && \
			(error & (action >> IIM_ERR_SHIFT)) == 0) {
		if (error) {
			printf("Even though the operation"
				"seems successful...\n");
			printf("There are some error(s) "
				"at addr=0x%x: 0x%x\n",
				(u32)&(imx_iim->err), error);
		}
		return 0;
	}
	printf("%s(%d) failed\n", __func__, action);
	printf("status address=0x%x, value=0x%x\n",
		(u32)&(imx_iim->stat), status);
	printf("There are some error(s) at addr=0x%x: 0x%x\n",
		(u32)&(imx_iim->err), error);
	return -1;
}

static u32 sense_fuse(s32 bank, s32 row, s32 bit)
{
	s32 addr, addr_l, addr_h, reg_addr;

	fuse_op_start();

	addr = ((bank << 11) | (row << 3) | (bit & 0x7));
	/* Set IIM Program Upper Address */
	addr_h = (addr >> 8) & 0x000000FF;
	/* Set IIM Program Lower Address */
	addr_l = (addr & 0x000000FF);

#ifdef IIM_FUSE_DEBUG
	printf("%s: addr_h=0x%x, addr_l=0x%x\n",
			__func__, addr_h, addr_l);
#endif
	writel(addr_h, &(imx_iim->ua));
	writel(addr_l, &(imx_iim->la));

	/* Start sensing */
	writel(0x8, &(imx_iim->fctl));
	if (poll_fuse_op_done(POLL_FUSE_SNSD) != 0) {
		printf("%s(bank: %d, row: %d, bit: %d failed\n",
			__func__, bank, row, bit);
	}
	reg_addr = (s32)&(imx_iim->sdat);

	return readl(reg_addr);
}

int iim_read(int bank, char row)
{
	u32 fuse_val;
	s32 err = 0;

	printf("Read fuse at bank:%d row:%d\n", bank, row);
	fuse_val = sense_fuse(bank, row, 0);
	printf("fuses at (bank:%d, row:%d) = 0x%x\n", bank, row, fuse_val);

	return err;
}

/* Blow fuses based on the bank, row and bit positions (all 0-based)
*/
static s32 fuse_blow_bit(s32 bank, s32 row, s32 bit)
{
	int addr, addr_l, addr_h, ret = -1;

	fuse_op_start();

	/* Disable IIM Program Protect */
	writel(0xaa, &(imx_iim->preg_p));

	addr = ((bank << 11) | (row << 3) | (bit & 0x7));
	/* Set IIM Program Upper Address */
	addr_h = (addr >> 8) & 0x000000FF;
	/* Set IIM Program Lower Address */
	addr_l = (addr & 0x000000FF);

#ifdef IIM_FUSE_DEBUG
	printf("blowing addr_h=0x%x, addr_l=0x%x\n", addr_h, addr_l);
#endif

	writel(addr_h, &(imx_iim->ua));
	writel(addr_l, &(imx_iim->la));

	/* Start Programming */
	writel(0x31, &(imx_iim->fctl));
	if (poll_fuse_op_done(POLL_FUSE_PRGD) == 0)
		ret = 0;

	/* Enable IIM Program Protect */
	writel(0x0, &(imx_iim->preg_p));

	return ret;
}

static void fuse_blow_row(s32 bank, s32 row, s32 value)
{
	u32 reg, i;

	/* enable fuse blown */
	reg = readl(CCM_BASE_ADDR + 0x64);
	reg |= 0x10;
	writel(reg, CCM_BASE_ADDR + 0x64);

	for (i = 0; i < 8; i++) {
		if (((value >> i) & 0x1) == 0)
			continue;
	if (fuse_blow_bit(bank, row, i) != 0) {
			printf("fuse_blow_bit(bank: %d, row: %d, "
				"bit: %d failed\n",
				bank, row, i);
		}
    }
    reg &= ~0x10;
    writel(reg, CCM_BASE_ADDR + 0x64);
}

int iim_blow(int bank, int row, int val)
{
	u32 fuse_val, err = 0;

	printf("Blowing fuse at bank:%d row:%d value:%d\n",
			bank, row, val);
	fuse_blow_row(bank, row, val);
	fuse_val = sense_fuse(bank, row, 0);
	printf("fuses at (bank:%d, row:%d) = 0x%x\n", bank, row, fuse_val);

	return err;
}

static int iim_read_mac_addr(u8 *data)
{
	s32 bank = CONFIG_IIM_MAC_BANK;
	s32 row  = CONFIG_IIM_MAC_ROW;

	data[0] = sense_fuse(bank, row, 0) ;
	data[1] = sense_fuse(bank, row + 1, 0) ;
	data[2] = sense_fuse(bank, row + 2, 0) ;
	data[3] = sense_fuse(bank, row + 3, 0) ;
	data[4] = sense_fuse(bank, row + 4, 0) ;
	data[5] = sense_fuse(bank, row + 5, 0) ;

	if (!memcmp(data, "\0\0\0\0\0\0", 6))
		return 0;
	else
		return 1;
}

int iim_blow_func(char *func_name, char *func_val)
{
	u32 value, i;
	char *s;
	char val[3];
	s32 err = 0;

	if (0 == strcmp(func_name, "scc")) {
		/* fuse_blow scc
	C3D153EDFD2EA9982226EF5047D3B9A0B9C7138EA87C028401D28C2C2C0B9AA2 */
		printf("Ready to burn SCC fuses\n");
		s = func_val;
		for (i = 0; ; ++i) {
			memcpy(val, s, 2);
			val[2] = '\0';
			value = quick_atoi(val, 2);
			/* printf("fuse_blow_row(2, %d, value=0x%x)\n",
					i, value); */
			fuse_blow_row(2, i, value);

			if ((++s)[0] == '\0') {
				printf("ERROR: Odd string input\n");
				err = -1;
				break;
			}
			if ((++s)[0] == '\0') {
				printf("Successful\n");
				break;
			}
		}
	} else if (0 == strcmp(func_name, "srk")) {
		/* fuse_blow srk
	418bccd09b53bee1ab59e2662b3c7877bc0094caee201052add49be8780dff95 */
		printf("Ready to burn SRK key fuses\n");
		s = func_val;
		for (i = 0; ; ++i) {
			memcpy(val, s, 2);
			val[2] = '\0';
			value = quick_atoi(val, 2);
			if (i == 0) {
				/* 0x41 goes to SRK_HASH[255:248],
				 * bank 1, row 1 */
				fuse_blow_row(1, 1, value);
			} else {
				/* 0x8b in SRK_HASH[247:240] bank 3, row 1 */
				/* 0xcc in SRK_HASH[239:232] bank 3, row 2 */
				/* ... */
				fuse_blow_row(3, i, value);

				if ((++s)[0] == '\0') {
					printf("ERROR: Odd string input\n");
					err = -1;
					break;
				}
				if ((++s)[0] == '\0') {
					printf("Successful\n");
					break;
				}
			}
		}
	} else if (0 == strcmp(func_name, "fecmac")) {
		u8 ea[6] = { 0 };

		if (NULL == func_val) {
			/* Read the Mac address and print it */
			iim_read_mac_addr(ea);

			printf("FEC MAC address: ");
			printf("0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n\n",
				ea[0], ea[1], ea[2], ea[3], ea[4], ea[5]);

			return 0;
		}

		eth_parse_enetaddr(func_val, ea);
		if (!is_valid_ether_addr(ea)) {
			printf("Error: invalid mac address parameter!\n");
			err = -1;
		} else {
			for (i = 0; i < 6; ++i)
				fuse_blow_row(1, i + 9, ea[i]);
		}
	} else {
		printf("This command is not supported\n");
	}

	return err;
}


