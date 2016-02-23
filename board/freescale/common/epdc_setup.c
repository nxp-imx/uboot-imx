/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Peng Fan <Peng.Fan@freescale.com>
 */
#include <common.h>
#include <lcd.h>
#include <linux/err.h>
#include <linux/types.h>
#include <malloc.h>
#include <mxc_epdc_fb.h>
#include <fs.h>
#include <cpu_func.h>

#define is_digit(c)	((c) >= '0' && (c) <= '9')
__weak int mmc_get_env_devno(void)
{
	return 0;
}
__weak int check_mmc_autodetect(void)
{
	return 0;
}

int board_setup_waveform_file(ulong waveform_buf)
{
	char *fs_argv[5];
	char addr[17];
	ulong file_len, mmc_dev;

	if (!check_mmc_autodetect())
		mmc_dev = env_get_ulong("mmcdev", 10, 0);
	else
		mmc_dev = mmc_get_env_devno();

	sprintf(addr, "%lx", (ulong)CONFIG_SYS_LOAD_ADDR);

	fs_argv[0] = "fatload";
	fs_argv[1] = "mmc";
	fs_argv[2] = simple_itoa(mmc_dev);
	fs_argv[3] = addr;
	fs_argv[4] = env_get("epdc_waveform");

	if (!fs_argv[4])
		fs_argv[4] = "epdc_splash.bin";

	if (do_fat_fsload(NULL, 0, 5, fs_argv)) {
		printf("File %s not found on MMC Device %lu!\n", fs_argv[4], mmc_dev);
		return -1;
	}

	file_len = env_get_hex("filesize", 0);
	if (!file_len)
		return -1;

	memcpy((void *)waveform_buf, (const void *)CONFIG_SYS_LOAD_ADDR, file_len);

	flush_cache(waveform_buf, roundup(file_len, ARCH_DMA_MINALIGN));

	return 0;
}

int board_setup_logo_file(void *display_buf)
{
	int logo_width, logo_height;
	char *fs_argv[5];
	char addr[17];
	int array[3];
	ulong file_len, mmc_dev;
	char *buf, *s;
	int arg = 0, val = 0, pos = 0;
	int i, j, max_check_length;
	int row, col, row_end, col_end;

	if (!display_buf)
		return -EINVAL;

	/* Assume PGM header not exceeds 128 bytes */
	max_check_length = 128;

	if (!check_mmc_autodetect())
		mmc_dev = env_get_ulong("mmcdev", 10, 0);
	else
		mmc_dev = mmc_get_env_devno();

	memset(display_buf, 0xFF, panel_info.vl_col * panel_info.vl_row);

	fs_argv[0] = "fatsize";
	fs_argv[1] = "mmc";
	fs_argv[2] = simple_itoa(mmc_dev);
	fs_argv[3] = env_get("epdc_logo");
	if (!fs_argv[3])
		fs_argv[3] = "epdc_logo.pgm";
	if (do_fat_size(NULL, 0, 4, fs_argv)) {
		debug("File %s not found on MMC Device %lu, use black border\n", fs_argv[3], mmc_dev);
		/* Draw black border around framebuffer*/
		memset(display_buf, 0x0, 24 * panel_info.vl_col);
		for (i = 24; i < (panel_info.vl_row - 24); i++) {
			memset((u8 *)display_buf + i * panel_info.vl_col,
			       0x00, 24);
			memset((u8 *)display_buf + i * panel_info.vl_col
				+ panel_info.vl_col - 24, 0x00, 24);
		}
		memset((u8 *)display_buf +
		       panel_info.vl_col * (panel_info.vl_row - 24),
		       0x00, 24 * panel_info.vl_col);
		return 0;
	}

	file_len = env_get_hex("filesize", 0);
	if (!file_len)
		return -EINVAL;

	buf = memalign(ARCH_DMA_MINALIGN, file_len);
	if (!buf)
		return -ENOMEM;

	sprintf(addr, "%lx", (ulong)CONFIG_SYS_LOAD_ADDR);

	fs_argv[0] = "fatload";
	fs_argv[1] = "mmc";
	fs_argv[2] = simple_itoa(mmc_dev);
	fs_argv[3] = addr;
	fs_argv[4] = env_get("epdc_logo");

	if (!fs_argv[4])
		fs_argv[4] = "epdc_logo.pgm";

	if (do_fat_fsload(NULL, 0, 5, fs_argv)) {
		printf("File %s not found on MMC Device %lu!\n", fs_argv[4], mmc_dev);
		free(buf);
		return -1;
	}

	memcpy((void *)buf, (const void *)CONFIG_SYS_LOAD_ADDR, file_len);

	if (strncmp(buf, "P5", 2)) {
		printf("Wrong format for epdc logo, use PGM-P5 format.\n");
		free(buf);
		return -EINVAL;
	}
	/* Skip P5\n */
	pos += 3;
	arg = 0;
	for (i = 3; i < max_check_length; ) {
		/* skip \n \t and space */
		if ((buf[i] == '\n') || (buf[i] == '\t') || (buf[i] == ' ')) {
			i++;
			continue;
		}
		/* skip comment */
		if (buf[i] == '#') {
			while (buf[i++] != '\n')
				;
			continue;
		}

		/* HEIGTH, WIDTH, MAX PIXEL VLAUE total 3 args */
		if (arg > 2)
			break;
		val = 0;
		while (is_digit(buf[i])) {
			val = val * 10 + buf[i] - '0';
			i++;
		}
		array[arg++] = val;

		i++;
	}

	/* Point to data area */
	pos = i;

	logo_width = array[0];
	logo_height = array[1];

	if ((logo_width > panel_info.vl_col) ||
	    (logo_height > panel_info.vl_row)) {
		printf("Picture: too big\n");
		free(buf);
		return -EINVAL;
	}

	/* m,m means center of screen */
	row = 0;
	col = 0;
	s = env_get("splashpos");
	if (s) {
		if (s[0] == 'm')
			col = (panel_info.vl_col  - logo_width) >> 1;
		else
			col = simple_strtol(s, NULL, 0);
		s = strchr(s + 1, ',');
		if (s != NULL) {
			if (s[1] == 'm')
				row = (panel_info.vl_row  - logo_height) >> 1;
			else
				row = simple_strtol(s + 1, NULL, 0);
		}
	}
	if ((col + logo_width > panel_info.vl_col) ||
	    (row + logo_height > panel_info.vl_row)) {
		printf("Incorrect pos, use (0, 0)\n");
		row = 0;
		col = 0;
	}

	/* Draw picture at the center of screen */
	row_end = row + logo_height;
	col_end = col + logo_width;
	for (i = row; i < row_end; i++) {
		for (j = col; j < col_end; j++) {
			*((u8 *)display_buf + i * (panel_info.vl_col) + j) =
				 buf[pos++];
		}
	}

	free(buf);

	flush_cache((ulong)display_buf, file_len - pos - 1);

	return 0;
}
