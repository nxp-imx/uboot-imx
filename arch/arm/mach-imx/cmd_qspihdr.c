// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2021 NXP
 */
#include <common.h>
#include <command.h>
#include <dm.h>
#include <mapmem.h>
#include <asm/io.h>
#include <spi.h>
#include <spi_flash.h>
#include <dm/device-internal.h>

static struct spi_flash *flash;

#define QSPI_HDR_TAG		0xc0ffee01 /* c0ffee01 */
#define QSPI_HDR_TAG_OFF	0x1fc
#define FSPI_HDR_TAG		0x42464346/* FCFB, bigendian */
#define FSPI_HDR_TAG_OFF	0x0

#define HDR_LEN			0x200

#ifdef CONFIG_MX7
#define QSPI_HDR_OFF	0x0
#define QSPI_DATA_OFF	0x400
#else
#define QSPI_HDR_OFF	0x400
#define QSPI_DATA_OFF	0x1000
#endif

#ifdef CONFIG_IMX8MM
#define FSPI_HDR_OFF	0x0
#define FSPI_DATA_OFF	0x1000
#else
#define FSPI_HDR_OFF	0x400
#define FSPI_DATA_OFF	0x1000
#endif

#define FLAG_VERBOSE		1

struct qspi_config_parameter {
	u32 dqs_loopback;			/* Sets DQS LoopBack Mode to enable Dummy Pad MCR[24] */
	u32 hold_delay;				/* No needed on ULT1 */
	u32 hsphs;				/* Half Speed Phase Shift */
	u32 hsdly;				/* Half Speed Delay Selection */
	u32 device_quad_mode_en;		/* Write Command to Device */
	u32 device_cmd;				/* Cmd to xfer to device */
	u32 write_cmd_ipcr;			/* IPCR value of Write Cmd */
	u32 write_enable_ipcr;			/* IPCR value of Write enable */
	u32 cs_hold_time;			/* CS hold time in terms of serial clock.(for example 1 serial clock cyle) */
	u32 cs_setup_time;			/* CS setup time in terms of serial clock.(for example 1 serial clock cyle) */
	u32 sflash_A1_size;			/* interms of Bytes */
	u32 sflash_A2_size;			/* interms of Bytes */
	u32 sflash_B1_size;			/* interms of Bytes */
	u32 sflash_B2_size;			/* interms of Bytes */
	u32 sclk_freq;				/* 0 - 18MHz, 1 - 49MHz, 2 - 55MHz, 3 - 60MHz, 4 - 66Mhz, 5 - 76MHz, 6 - 99MHz (only for SDR Mode) */
	u16 busy_bit_offset;			/* Flash device busy bit offset in status register */
	u16 busy_bit_polarity;			/* Polarity of busy bit, 0 means the busy bit is 1 while busy and vice versa. */
	u32 sflash_type;			/* 1 - Single, 2 - Dual, 4 - Quad */
	u32 sflash_port;			/* 0 - Only Port-A, 1 - Both PortA and PortB */
	u32 ddr_mode_enable;			/* Enable DDR mode if set to TRUE */
	u32 dqs_enable;				/* Enable DQS mode if set to TRUE. Bit 0 represents DQS_EN, bit 1 represents DQS_LAT_EN */
	u32 parallel_mode_enable;		/* Enable Individual or parrallel mode. */
	u32 portA_cs1;				/* Enable Port A CS1 */
	u32 portB_cs1;				/* Enable Port B CS1 */
	u32 fsphs;				/* Full Speed Phase Selection */
	u32 fsdly;				/* Full Speed Phase Selection */
	u32 ddrsmp;				/* Select the sampling point for incoming data when serial flash is in DDR mode. */
	u32 command_seq[64];			/* Set of seq to perform optimum read on SFLASH as as per vendor SFLASH */
	u32 read_status_ipcr;			/* IPCR value of Read Status Reg */
	u32 enable_dqs_phase;			/* Enable DQS phase */
	u32 config_cmds_en;			/* Enable config commands */
	u32 config_cmds[4];			/* config commands, used to configure nor flash */
	u32 config_cmds_args[4];		/* config commands argu */
	u32 dqs_pad_setting_override;		/* DQS pin pad setting override */
	u32 sclk_pad_setting_override;		/* SCLK pin pad setting override */
	u32 data_pad_setting_override;		/* DATA pins pad setting override */
	u32 cs_pad_setting_override;		/* CS pins pad setting override */
	u32 dqs_loopback_internal;		/* 0: dqs loopback from pad, 1: dqs loopback internally */
	u32 dqs_phase_sel;			/* dqs phase sel */
	u32 dqs_fa_delay_chain_sel;		/* dqs fa delay chain selection */
	u32 dqs_fb_delay_chain_sel;		/* dqs fb delay chain selection */
	u32 sclk_fa_delay_chain_sel;		/* sclk fa delay chain selection */
	u32 sclk_fb_delay_chain_sel;		/* sclk fb delay chain selection */
	u32 misc_clock_enable;			/* Misc clock enable, bit 0 means differential clock enable, bit 1 means CK2 clock enable. */
	u32 reserve[15];			/* Reserved area, the total size of configuration structure should be 512 bytes */
	u32 tag;				/* QSPI configuration TAG, should be 0xc0ffee01 */
};

struct fspi_config_parameter {
	u32 tag;			/* tag, 0x46434642 ascii 'FCFB' */
	u32 version;			/* 0x00000156 ascii bugfix | minor | major | 'V' */
	u16 reserved;
	u8  reserved0[2];
	u8  readSampleClkSrc;		/* 0 - internal loopback, 1 - loopback from DQS pad, 2 - loopback from SCK pad, 3 - Flash provided DQS */
	u8  dataHoldTime;		/* CS hold time */
	u8  dataSetupTime;		/* CS setup time */
	u8  columnAddressWidth;		/* 3 - for HyperFlash, 0 - other devices */
	u8  deviceModeCfgEnable;	/* device mode configuration enable feature, 0 - disable, 1- enable */
	u8  reserved1[3];
	u32 deviceModeSeq;		/* sequence parameter for device mode configuration */
	u32 deviceModeArg;		/* device mode argument, effective only when deviceModeCfgEnable = 1 */
	u8  configCmdEnable;		/* config command enable feature, 0 - disable, 1 - enable */
	u8  reserved2[3];
	u32 configCmdSeqs[4];		/* sequences for config command, allow 4 separate configuration command sequences */
	u32 configCmdArgs[4];		/* arguments for each separate configuration command sequence */
	u32 controllerMiscOption;
					/*
					 *
					 * +--------+----------------------------------------------------------+
					 * | offset | description                                              |
					 * +--------+----------------------------------------------------------+
					 * |        | differential clock enable                                |
					 * |   0    |                                                          |
					 * |        | 0 - differential clock is not supported                  |
					 * |        | 1 - differential clock is supported                      |
					 * +--------+----------------------------------------------------------+
					 * |        | CK2 enable                                               |
					 * |   1    |                                                          |
					 * |        | must set 0 for this silicon                              |
					 * |        |                                                          |
					 * +--------+----------------------------------------------------------+
					 * |        | parallel mode enable                                     |
					 * |   2    |                                                          |
					 * |        | must set 0 for this silicon                              |
					 * |        |                                                          |
					 * +--------+----------------------------------------------------------+
					 * |        | word addressable enable                                  |
					 * |   3    |                                                          |
					 * |        | 0 - device is not word addressable                       |
					 * |        | 1 - device is word addressable                           |
					 * +--------+----------------------------------------------------------+
					 * |        | safe configuration frequency enable                      |
					 * |   4    |                                                          |
					 * |        | 0 - configure external device using specified frequency  |
					 * |        | 1 - configure external device using 30MHz                |
					 * +--------+----------------------------------------------------------+
					 * |   5    | reserved                                                 |
					 * +--------+----------------------------------------------------------+
					 * |        | ddr mode enable                                          |
					 * |   6    |                                                          |
					 * |        | 0 - external device works using SDR commands             |
					 * |        | 1 - external device works using DDR commands             |
					 * +--------+----------------------------------------------------------+
					 */
	u8  deviceType;			/* 1 - serial NOR */
	u8  sflashPadType;		/* 1 - single pad, 2 - dual pads, 4 - quad pads, 8 - octal pads */
	u8  serialClkFreq;		/* 1 - 20MHz, 2 - 50MHz, 3 - 60MHz, 4 - 80MHz, 5 - 100MHz, 6 - 133MHz, 7 - 166MHz, other values - 20MHz*/
	u8  lutCustomSeqEnable;		/* 0 - use pre-defined LUT sequence index and number, 1 - use LUT sequence parameters provided in this block */
	u32 reserved3[2];
	u32 sflashA1Size;		/* For SPI NOR, need to fill with actual size, in terms of bytes */
	u32 sflashA2Size;		/* same as above */
	u32 sflashB1Size;		/* same as above */
	u32 sflashB2Size;		/* same as above */
	u32 csPadSettingOverride;	/* set to 0 if it is not supported */
	u32 sclkPadSettingOverride;	/* set to 0 if it is not supported */
	u32 dataPadSettingOverride;	/* set to 0 if it is not supported */
	u32 dqsPadSettingOverride;	/* set to 0 if it is not supported */
	u32 timeoutInMs;		/* maximum wait time during dread busy status, not used in ROM */
	u32 commandInterval;		/* interval of CS deselected period, set to 0 */
	u16 dataValidTime[2];		/* time from clock edge to data valid edge */
					/* This field is used when the FlexSPI root clock is less than 100MHz and the read sample */
					/* clock source is device provided DQS signal without CK2 support. */
					/* [31:16] - data valid time for DLLB in terms of 0.1ns */
					/* [15:0]  - data valid time for DLLA in terms of 0.1ns */
	u16 busyOffset;			/* busy bit offset, valid range: 0 - 31 */
	u16 busyBitPolarity;		/* 0 - busy bit is 1 if device is busy, 1 - busy bit is 0 if device is busy */
	u32 lookupTable[64];		/* lookup table */
	u32 lutCustomSeq[12];		/* customized LUT sequence */
	u32 reserved4[4];
	u32 pageSize;			/* page size of serial NOR flash, not used in ROM */
	u32 sectorSize;			/* sector size of serial NOR flash, not used in ROM */
	u32 reserved5[14];
};

struct header_config {
	union {
		struct qspi_config_parameter qspi_hdr_config;
		struct fspi_config_parameter fspi_hdr_config;
	};
};

#if defined(CONFIG_MX6) || defined(CONFIG_MX7) || defined(CONFIG_ARCH_MX7ULP)
static struct qspi_config_parameter qspi_safe_config = {
	.cs_hold_time		= 3,
	.cs_setup_time		= 3,
	.sflash_A1_size		= 0x4000000,
	.sflash_B1_size		= 0x4000000,
	.sflash_type		= 1,
	.command_seq[0]		= 0x08180403,
	.command_seq[1]		= 0x24001c00,
	.tag			= 0xc0ffee01,
};

static struct header_config *safe_config = (struct header_config *)&qspi_safe_config;
#else
static struct fspi_config_parameter fspi_safe_config = {
	.tag			= 0x42464346,
	.version		= 0x56010000,
	.dataHoldTime		= 0x3,
	.dataSetupTime		= 0x3,
	.deviceType		= 0x1,
	.sflashPadType		= 0x1,
	.serialClkFreq		= 0x2,
	.sflashA1Size		= 0x10000000,
	.lookupTable[0]		= 0x0818040b,
	.lookupTable[1]		= 0x24043008,
};

static struct header_config *safe_config = (struct header_config *)&fspi_safe_config;
#endif

static int qspi_erase_update(struct spi_flash *flash, int off, int len, void *buf)
{
	int size;
	int ret;

	size = ROUND(len, flash->sector_size);
	ret = spi_flash_erase(flash, off, size);
	printf("Erase %#x bytes @ %#x %s\n",
	       size, off, ret ? "ERROR" : "OK");
	if (ret)
		return ret;

	ret = spi_flash_write(flash, off, len, buf);
	printf("Write %#x bytes @ %#x %s\n",
	       len, off, ret ? "ERROR" : "OK");

	return ret;
}

static int do_qspihdr_check(int argc, char * const argv[], int flag)
{
	u32 buf;
	unsigned long addr;
	char *endp;
	void *tmp;
	int ret;

#if defined(CONFIG_MX6) || defined(CONFIG_MX7) || defined(CONFIG_ARCH_MX7ULP)
	int off = QSPI_HDR_OFF + QSPI_HDR_TAG_OFF;
	int tag = QSPI_HDR_TAG;
#else
	int off = FSPI_HDR_OFF + FSPI_HDR_TAG_OFF;
	int tag = FSPI_HDR_TAG;
#endif

	if (argc == 3) {
		/* check data in memory */
		addr = simple_strtoul(argv[2], &endp, 16);

		tmp = map_physmem(addr + off, 4, MAP_WRBACK);
		if (!tmp) {
			printf("Failed to map physical memory\n");
			return 1;
		}

		if (*(u32 *)tmp == tag) {
			if (flag & FLAG_VERBOSE)
				printf("Found boot config header in memory\n");
			unmap_physmem(tmp, 4);
			return 0;
		} else {
			if (flag & FLAG_VERBOSE)
				printf("NO boot config header in memory\n");
			unmap_physmem(tmp, 4);
			return 1;
		}
	} else {
		ret = spi_flash_read(flash, off, 4, &buf);
		if (ret) {
			printf("flash read failed, ret: %d\n", ret);
			return -1;
		}

		if (buf == tag) {
			if (flag & FLAG_VERBOSE)
				printf("Found boot config header in Q(F)SPI\n");
			return 0;
		} else {
			if (flag & FLAG_VERBOSE)
				printf("NO boot config header in Q(F)SPI\n");
			return 1;
		}
	}
}

static void hdr_dump(void *data)
{
#if defined(CONFIG_MX6) || defined(CONFIG_MX7) || defined(CONFIG_ARCH_MX7ULP)
	struct qspi_config_parameter *hdr =
		(struct qspi_config_parameter *)data;
#else
	struct fspi_config_parameter *hdr =
		(struct fspi_config_parameter *)data;
#endif
	int i;

#define PH(mem, cnt) (						\
{								\
	if (cnt > 1) {						\
		int len = strlen(#mem);				\
		char *sub = strchr(#mem, '[');			\
		if (sub)					\
			*sub = '\0';				\
		for (i = 0; i < cnt; ++i)			\
			printf("  %s[%02d%-*s = %08x\n",	\
			       #mem, i, 25 - len, "]",		\
			       (u32)*(&hdr->mem + i));		\
	} else {						\
		printf("  %-25s = %0*x\n",			\
		       #mem, (int)sizeof(hdr->mem), hdr->mem);	\
		}						\
}								\
)

#if defined(CONFIG_MX6) || defined(CONFIG_MX7) || defined(CONFIG_ARCH_MX7ULP)
	PH(dqs_loopback, 1);
	PH(hold_delay, 1);
	PH(hsphs, 1);
	PH(hsdly, 1);
	PH(device_quad_mode_en, 1);
	PH(write_cmd_ipcr, 1);
	PH(write_enable_ipcr, 1);
	PH(cs_hold_time, 1);
	PH(cs_setup_time, 1);
	PH(sflash_A1_size, 1);
	PH(sflash_A2_size, 1);
	PH(sflash_B1_size, 1);
	PH(sflash_B2_size, 1);
	PH(sclk_freq, 1);
	PH(busy_bit_offset, 1);
	PH(busy_bit_polarity, 1);
	PH(sflash_type, 1);
	PH(sflash_port, 1);
	PH(ddr_mode_enable, 1);
	PH(dqs_enable, 1);
	PH(parallel_mode_enable, 1);
	PH(portA_cs1, 1);
	PH(portB_cs1, 1);
	PH(fsphs, 1);
	PH(fsdly, 1);
	PH(ddrsmp, 1);
	PH(command_seq[0], 64);
	PH(read_status_ipcr, 1);
	PH(enable_dqs_phase, 1);
	PH(config_cmds_en, 1);
	PH(config_cmds[0], 4);
	PH(config_cmds_args[0], 4);
	PH(dqs_pad_setting_override, 1);
	PH(sclk_pad_setting_override, 1);
	PH(data_pad_setting_override, 1);
	PH(cs_pad_setting_override, 1);
	PH(dqs_loopback_internal, 1);
	PH(dqs_phase_sel, 1);
	PH(dqs_fa_delay_chain_sel, 1);
	PH(dqs_fb_delay_chain_sel, 1);
	PH(sclk_fa_delay_chain_sel, 1);
	PH(sclk_fb_delay_chain_sel, 1);
	PH(misc_clock_enable, 1);
	PH(tag, 1);
#else
	PH(tag, 1);
	PH(version, 1);
	PH(readSampleClkSrc, 1);
	PH(dataHoldTime, 1);
	PH(dataSetupTime, 1);
	PH(columnAddressWidth, 1);
	PH(deviceModeCfgEnable, 1);
	PH(deviceModeSeq, 1);
	PH(deviceModeArg, 1);
	PH(configCmdEnable, 1);
	PH(configCmdSeqs[0], 4);
	PH(configCmdArgs[0], 4);
	PH(controllerMiscOption, 1);
	PH(deviceType, 1);
	PH(sflashPadType, 1);
	PH(serialClkFreq, 1);
	PH(lutCustomSeqEnable, 1);
	PH(sflashA1Size, 1);
	PH(sflashA2Size, 1);
	PH(sflashB1Size, 1);
	PH(sflashB2Size, 1);
	PH(csPadSettingOverride, 1);
	PH(sclkPadSettingOverride, 1);
	PH(dataPadSettingOverride, 1);
	PH(dqsPadSettingOverride, 1);
	PH(timeoutInMs, 1);
	PH(commandInterval, 1);
	PH(dataValidTime[0], 2);
	PH(busyOffset, 1);
	PH(busyBitPolarity, 1);
	PH(lookupTable[0], 64);
	PH(lutCustomSeq[0], 12);
	PH(pageSize, 1);
	PH(sectorSize, 1);
#endif
}

static int do_qspihdr_dump(int argc, char * const argv[])
{
	unsigned long addr;
	char *endp;
	void *tmp;
	void *buf;
	int ret;

#if defined(CONFIG_MX6) || defined(CONFIG_MX7) || defined(CONFIG_ARCH_MX7ULP)
	int off = QSPI_HDR_OFF;
#else
	int off = FSPI_HDR_OFF;
#endif

	if (argc == 3) {
		/* check data in memory */
		if (do_qspihdr_check(3, argv, FLAG_VERBOSE)) {
			/* return 0 in any cases */
			return 0;
		}

		addr = simple_strtoul(argv[2], &endp, 16);

		tmp = map_physmem(addr + off, HDR_LEN, MAP_WRBACK);
		if (!tmp) {
			printf("Failed to map physical memory\n");
			return 1;
		}

		hdr_dump(tmp);
		unmap_physmem(tmp, HDR_LEN);
	} else {
		/* check data in Q(F)SPI */
		buf = malloc(HDR_LEN);
		if (!buf) {
			printf("Failed to alloc memory\n");
			/* return 0 in any cases */
			return 0;
		}

		ret = spi_flash_read(flash, off, HDR_LEN, buf);
		if (ret) {
			printf("flash read failed, ret: %d\n", ret);
			return -1;
		}

		hdr_dump(buf);
		free(buf);
	}

	return 0;
}

static int do_qspihdr_init(int argc, char * const argv[])
{
	unsigned long addr, len;
	char *endp;
	int total_len;
	void *tmp;
	void *buf;
	bool hdr_flag = false;
	int ret;

#if defined(CONFIG_MX6) || defined(CONFIG_MX7) || defined(CONFIG_ARCH_MX7ULP)
	int hdr_off = QSPI_HDR_OFF;
	int data_off = QSPI_DATA_OFF;
#else
	int hdr_off = FSPI_HDR_OFF;
	int data_off = FSPI_DATA_OFF;

	safe_config->fspi_hdr_config.pageSize = flash->page_size;
	safe_config->fspi_hdr_config.sectorSize = flash->sector_size;
#endif

	addr = simple_strtoul(argv[2], &endp, 16);
	len = simple_strtoul(argv[3], &endp, 16);

	total_len = data_off + len;
	if (total_len > flash->size) {
		printf("Error: length %lx over flash size (%#x)\n",
		       len, flash->size);
		return 1;
	}

	/* check if header exists in this memory area*/
	if (do_qspihdr_check(3, argv, 0) == 0)
		hdr_flag = true;

	tmp = map_physmem(addr, len, MAP_WRBACK);
	if (!tmp) {
		printf("Failed to map physical memory\n");
		return 1;
	}

	if (hdr_flag)
		goto burn_image;

	buf = malloc(total_len);
	if (!buf) {
		printf("Failed to alloc memory\n");
		unmap_physmem(tmp, total_len);
		return 1;
	}

	memset(buf, 0xff, total_len);
	memcpy(buf + hdr_off, safe_config, HDR_LEN);
	memcpy(buf + data_off, tmp, len);

burn_image:
	if (hdr_flag) {
		ret = qspi_erase_update(flash, 0, len, tmp);
	} else {
		ret = qspi_erase_update(flash, 0, total_len, buf);
		free(buf);
	}

	unmap_physmem(tmp, total_len);
	return ret;
}

static int do_qspihdr_update(int argc, char * const argv[])
{
	int len;
	int size;
	void *buf;
	int ret;

#if defined(CONFIG_MX6) || defined(CONFIG_MX7) || defined(CONFIG_ARCH_MX7ULP)
	int hdr_off = QSPI_HDR_OFF;
#else
	int hdr_off = FSPI_HDR_OFF;
#endif

	len = hdr_off + HDR_LEN;
	size = ROUND(len, flash->sector_size);

	buf = malloc(size);
	if (!buf) {
		printf("Failed to alloc memory\n");
		return 1;
	}

	spi_flash_read(flash, 0, size, buf);
	memcpy(buf + hdr_off, safe_config, HDR_LEN);

	ret = qspi_erase_update(flash, 0, size, buf);
	free(buf);

	return ret;
}

static int do_qspihdr(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	char *cmd;
	unsigned int bus = CONFIG_SF_DEFAULT_BUS;
	unsigned int cs = CONFIG_SF_DEFAULT_CS;
	unsigned int speed = CONFIG_SF_DEFAULT_SPEED;
	unsigned int mode = CONFIG_SF_DEFAULT_MODE;
	int flags = 0;
	int ret;

	if (argc < 2)
		goto usage;

#ifdef CONFIG_DM_SPI_FLASH
	struct udevice *new, *bus_dev;

	ret = spi_find_bus_and_cs(bus, cs, &bus_dev, &new);
	if (!ret)
		device_remove(new, DM_REMOVE_NORMAL);
	flash = NULL;
	ret = spi_flash_probe_bus_cs(bus, cs, speed, mode, &new);
	if (ret) {
		printf("Failed to initialize SPI flash at %u:%u (error %d)\n",
		       bus, cs, ret);
		return 1;
	}
	flash = dev_get_uclass_priv(new);
#endif

	cmd = argv[1];

	if (strcmp(cmd, "check") == 0)
		return do_qspihdr_check(argc, argv, flags | FLAG_VERBOSE);

	if (strcmp(cmd, "dump") == 0)
		return do_qspihdr_dump(argc, argv);

	if (strcmp(cmd, "init") == 0) {
		if (argc < 5)
			goto usage;
		return do_qspihdr_init(argc, argv);
	}

	if (strcmp(cmd, "update") == 0) {
		if (argc < 3)
			goto usage;
		return do_qspihdr_update(argc, argv);
	}

	return 0;
usage:
	return CMD_RET_USAGE;
}

static char qspihdr_help_text[] =
	"check [addr] - check if boot config already exists, 0-yes, 1-no\n"
	"		with addr, it will check data in memory of this addr\n"
	"		without addr, it will check data in Q(F)SPI chip\n"
	"qspihdr dump [addr] - dump the header information, if exists\n"
	"		with addr, it will check data in memory of this addr\n"
	"		without addr, it will check data in Q(F)SPI chip\n"
	"qspihdr init addr len safe - burn data to Q(F)SPI with header\n"
	"		if data contains header, it will be used, otherwise,\n"
	"		safe: most common header, single line, sdr, low freq\n"
	"qspihdr update safe - only update the header in Q(F)SPI\n";

U_BOOT_CMD(qspihdr, 5, 1, do_qspihdr,
	"Q(F)SPI Boot Config sub-system",
	qspihdr_help_text
);
