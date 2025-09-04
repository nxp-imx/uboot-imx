// SPDX-License-Identifier: GPL-2.0+
/**
 * Copyright 2024 NXP
 */
#include <asm/arch/ddr.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/sys_proto.h>

#include <command.h>
#include <stdlib.h>
#include <errno.h>
#include <imx_container.h>
#include <log.h>
#include <mmc.h>
#include <nand.h>
#include <spl.h>

#define QB_STATE_LOAD_SIZE    0x10000          /** 64k */

#define MMC_DEV		0
#define QSPI_DEV	1
#define NAND_DEV	2
#define QSPI_NOR_DEV	3
#define ROM_API_DEV	4
#define RAM_DEV	5

extern rom_passover_t rom_passover_data;

#define IMG_FLAGS_IMG_TYPE_SHIFT  (0u)
#define IMG_FLAGS_IMG_TYPE_MASK   (0xfU)
#define IMG_FLAGS_IMG_TYPE(x)     (((x) & IMG_FLAGS_IMG_TYPE_MASK) >> \
                                   IMG_FLAGS_IMG_TYPE_SHIFT)

#define IMG_FLAGS_CORE_ID_SHIFT   (4u)
#define IMG_FLAGS_CORE_ID_MASK    (0xf0U)
#define IMG_FLAGS_CORE_ID(x)      (((x) & IMG_FLAGS_CORE_ID_MASK) >> \
                                   IMG_FLAGS_CORE_ID_SHIFT)

#define IMG_TYPE_DDR_TDATA_DUMMY  (0x0Du)   /* dummy DDR training data image */

/** Polynomial: 0xEDB88320 */
static u32 const p_table[] =
{
	0x00000000,0x1db71064,0x3b6e20c8,0x26d930ac,
	0X76dc4190,0X6b6b51f4,0X4db26158,0X5005713c,
	0xedb88320,0xf00f9344,0xd6d6a3e8,0xcb61b38c,
	0x9b64c2b0,0x86d3d2d4,0xa00ae278,0xbdbdf21c,
};

/**
 * Implement half-byte CRC algorithm
 */
static u32 qb_crc32(const void* addr, u32 len)
{
	u32 crc = ~0x00, idx, i, val;
	const u8 *chr = (const u8*)addr;

	for (i = 0; i < len; i++, chr++)
	{
		val = (u32)(*chr);

		idx = (crc ^ (val >> 0)) & 0x0F;
		crc = p_table[idx] ^ (crc >> 4);
		idx = (crc ^ (val >> 4)) & 0x0F;
		crc = p_table[idx] ^ (crc >> 4);
	}

	return ~crc;
}

static bool qb_check(void)
{
	struct ddrphy_qb_state *qb_state;
	u32 i, size, crc;

	/**
	 * Ensure MAC is not empty, the reason is that
	 * the data is invalidated after first save run
	 */
	qb_state = (struct ddrphy_qb_state *)CONFIG_SAVED_QB_STATE_BASE;

	if (is_imx95_a0()) {
		/** For iMX95 A0/1 check the CRC32 value */
		size = sizeof(struct ddrphy_qb_state) - MAC_LENGTH * sizeof(u32);
		crc = qb_crc32(&qb_state->TrainedVREFCA_A0, size);

		return (crc == qb_state->mac[0]);
	} else {
		for (i = 0; i < MAC_LENGTH; i++) {
			if (qb_state->mac[i] == 0)
				return false;
		}
	}

	return true;
}

static int do_qb_check(struct cmd_tbl *cmdtp, int flag,
		       int argc, char * const argv[])
{
	return qb_check() ? CMD_RET_SUCCESS : CMD_RET_FAILURE;
}

#if IS_ENABLED(CONFIG_SCMI_FIRMWARE)
static int scmi_get_boot_device_offset(unsigned long *img_off)
{
	int ret;
	rom_passover_t rom_data = {0};
	rom_passover_t *rdata = &rom_data;

	ret = scmi_get_rom_data(rdata);
	if (ret != 0) {
		printf("SCMI: fail to get ROM passover data %d\n", ret);
		return ret;
	}

	*img_off = rom_data.img_ofs;

	return 0;
}

static int scmi_get_boot_stage(u8 *stage)
{
	int ret;
	rom_passover_t rom_data = {0};
	rom_passover_t *rdata = &rom_data;

	ret = scmi_get_rom_data(rdata);
	if (ret != 0) {
		printf("SCMI: fail to get ROM passover data %d\n", ret);
		return ret;
	}

	*stage = rom_data.boot_stage;

	return 0;
}
#endif

static unsigned long get_boot_device_offset(void *dev, int dev_type)
{
	unsigned long offset = 0;
	struct mmc *mmc;

#if IS_ENABLED(CONFIG_SCMI_FIRMWARE)
	int ret;
	ret = scmi_get_boot_device_offset(&offset);
	if (!ret)
		return offset;
	/* fall back to boot from primary set if get rom passover failed */
#endif

	switch (dev_type) {
	case ROM_API_DEV:
		offset = (unsigned long)dev;
		break;
	case MMC_DEV:
		mmc = (struct mmc *)dev;

		if (IS_SD(mmc) || mmc->part_config == MMCPART_NOAVAILABLE) {
			offset = CONTAINER_HDR_MMCSD_OFFSET;
		} else {
			u8 part = EXT_CSD_EXTRACT_BOOT_PART(mmc->part_config);
			if (part == 1 || part == 2) {
				offset = CONTAINER_HDR_EMMC_OFFSET;
			} else {
				offset = CONTAINER_HDR_MMCSD_OFFSET;
			}
		}
		break;
	case QSPI_DEV:
		offset = CONTAINER_HDR_QSPI_OFFSET;
		break;
	case NAND_DEV:
		offset = CONTAINER_HDR_NAND_OFFSET;
		break;
	case QSPI_NOR_DEV:
		offset = CONTAINER_HDR_QSPI_OFFSET + 0x08000000;
		break;
	case RAM_DEV:
		offset = (unsigned long)dev + CONTAINER_HDR_MMCSD_OFFSET;
		break;
	}

	return offset;
}

static int parse_container(void *addr, u32 *qb_data_off)
{
	struct container_hdr *phdr;
	struct boot_img_t *img_entry;
	u8 i = 0;
	u32 img_type, img_end;

	phdr = (struct container_hdr *)addr;
	if (phdr->tag != 0x87 || (phdr->version != 0x0 && phdr->version != 0x2)) {
		return -1;
	}

	img_entry = (struct boot_img_t *)(addr + sizeof(struct container_hdr));
	for (i = 0; i < phdr->num_images; i++) {
		img_type = IMG_FLAGS_IMG_TYPE(img_entry->hab_flags);
		if (img_type == IMG_TYPE_DDR_TDATA_DUMMY && img_entry->size == 0) {
			/** Image entry pointing to DDR Training Data */
			(*qb_data_off) = img_entry->offset;
			return 0;
		}

		img_end = img_entry->offset + img_entry->size;
		if (i + 1 < phdr->num_images) {
			img_entry++;
			if (img_end + QB_STATE_LOAD_SIZE == img_entry->offset) {
				/** hole detected */
				(*qb_data_off) = img_end;
				return 0;
			}
		}
	}

	return -1;
}

static int get_dev_qbdata_offset(void *dev, int dev_type, unsigned long offset, u32 *qbdata_offset)
{
	u16 ctnr_hdr_align = container_hdr_alignment();
	void *buf = (void *)env_get_hex("loadaddr", 0);;
	int ret = 0;
	char cmd[128];
	unsigned long count = 0;
	struct mmc *mmc;

	if (!buf) {
		printf("loadaddr env is not defined, please set it\n");
		return -ENOMEM;
	}

	switch (dev_type) {
	case MMC_DEV:
		mmc = (struct mmc *)dev;

		count = blk_dread(mmc_get_blk_desc(mmc),
				  offset / mmc->read_bl_len,
				  ctnr_hdr_align / mmc->read_bl_len,
				  buf);
		if (count == 0) {
			printf("Read container image from MMC/SD failed\n");
			return -EIO;
		}
		break;
	case QSPI_DEV:
		sprintf(cmd, "sf read 0x%x 0x%lx 0x%x", (unsigned int)(uintptr_t)buf,
			offset, ctnr_hdr_align);
		/** Read data */
		ret = run_command(cmd, 0);
		if (ret) {
			printf("Read container header from QSPI failed\n");
			return -EIO;
		}
		break;
	case QSPI_NOR_DEV:
	case RAM_DEV:
		memcpy(buf, (const void *)offset, ctnr_hdr_align);
		break;
	}

	ret = parse_container(buf, qbdata_offset);

	return ret;
}

static int get_qbdata_offset(void *dev, int dev_type, u32 *qbdata_offset)
{
	u32 offset = get_boot_device_offset(dev, dev_type);
	u16 ctnr_hdr_align = container_hdr_alignment();
	u32 contOffset;
	int ret, i;

	for (i = 0; i < 3; i++)
	{
		contOffset = offset + i * ctnr_hdr_align;
		ret = get_dev_qbdata_offset(dev, dev_type, contOffset, qbdata_offset);
		if (ret == 0)
		{
			(*qbdata_offset) += contOffset;
			break;
		}
	}

	return ret;
}

static int get_board_boot_device(enum boot_device dev)
{
	switch (dev) {
	case SD1_BOOT:
	case MMC1_BOOT:
		return BOOT_DEVICE_MMC1;
	case SD2_BOOT:
	case MMC2_BOOT:
		return BOOT_DEVICE_MMC2;
	case USB_BOOT:
		return BOOT_DEVICE_BOARD;
	case QSPI_BOOT:
		return BOOT_DEVICE_SPI;
	default:
		return BOOT_DEVICE_NONE;
	}
}

static int mmc_get_device_index(u32 dev)
{
	switch (dev) {
	case BOOT_DEVICE_MMC1:
		return 0;
	case BOOT_DEVICE_MMC2:
	case BOOT_DEVICE_MMC2_2:
		return 1;
	}

	return -ENODEV;
}

static int mmc_find_device(struct mmc **mmcp, int mmc_dev)
{
	int err;

#if CONFIG_IS_ENABLED(DM_MMC)
	err = mmc_init_device(mmc_dev);
#else
	err = mmc_initialize(NULL);
#endif /* DM_MMC */

	if (err)
		return err;

	*mmcp = find_mmc_device(mmc_dev);

	return (*mmcp ? 0 : -ENODEV);
}

static int do_qb_mmc(int dev, bool save)
{
	struct mmc *mmc;
	int ret = 0, mmc_dev;
	u32 offset;
	char blk_cmd[128];
	void *buf;

	mmc_dev = mmc_get_device_index(dev);
	if (mmc_dev < 0)
		return mmc_dev;

	ret = mmc_find_device(&mmc, mmc_dev);
	if (ret)
		return ret;

	if (!mmc->has_init)
		ret = mmc_init(mmc);

	if (ret)
		return ret;

	if (IS_SD(mmc) || mmc->part_config == MMCPART_NOAVAILABLE) {
		sprintf(blk_cmd, "mmc dev %x", mmc_dev);
	} else {
		u8 part = EXT_CSD_EXTRACT_BOOT_PART(mmc->part_config);
		if (part == 1 || part == 2) {
#if IS_ENABLED(CONFIG_SCMI_FIRMWARE)
			u8 stage;
			ret = scmi_get_boot_stage(&stage);
			if (!ret) {
				if (stage == 0x9)
					part = (part == 1) ? 2 : 1;
			}
#endif
			sprintf(blk_cmd, "mmc dev %x %x", mmc_dev, part);
		} else {
			sprintf(blk_cmd, "mmc dev %x", mmc_dev);
		}
	}

	/** Select the device and partition */
	ret = run_command(blk_cmd, 0);
	if (ret)
		return ret;

	ret = get_qbdata_offset(mmc, MMC_DEV, &offset);
	if (ret)
		return ret;

	buf = kzalloc(QB_STATE_LOAD_SIZE, GFP_KERNEL);
	if (!buf) {
		printf("Malloc buffer failed\n");
		return -ENOMEM;
	}

	if (save) {
		memcpy(buf, (const void *)CONFIG_SAVED_QB_STATE_BASE,
		       sizeof(struct ddrphy_qb_state));
	} else {
		/** erase => buff is zero */
	}

	ret = blk_dwrite(mmc_get_blk_desc(mmc), offset / mmc->write_bl_len,
			 QB_STATE_LOAD_SIZE / mmc->write_bl_len, (const void*)buf);
	free(buf);

	return (ret > 0 ? 0 : -1);
}

static int do_qb_spi(int dev, bool save)
{
	int ret = 0;
	u32 offset;
	char cmd[128];
	void *buf;

	/** Probe */
	sprintf(cmd, "sf probe");
	ret = run_command(cmd, 0);
	if (ret)
		return ret;

	ret = get_qbdata_offset(0, QSPI_DEV, &offset);
	if (ret)
		return ret;

	buf = kzalloc(QB_STATE_LOAD_SIZE, GFP_KERNEL);
	if (!buf) {
		printf("Malloc buffer failed\n");
		return -ENOMEM;
	}

	if (save) {
		memcpy(buf, (const void *)CONFIG_SAVED_QB_STATE_BASE,
		       sizeof(struct ddrphy_qb_state));
	} else {
		/** erase => buff is zero */
	}

	/** Save / erase data */
	sprintf(cmd, "sf update 0x%x 0x%x 0x%x", (unsigned int)(uintptr_t)buf,
		offset, QB_STATE_LOAD_SIZE);
	ret = run_command(cmd, 0);

	free(buf);

	return ret;
}

static int do_qb_save(struct cmd_tbl *cmdtp, int flag,
		      int argc, char * const argv[])
{
	int ret = CMD_RET_FAILURE;
	long dev = -1;
	enum boot_device boot_dev = UNKNOWN_BOOT;
	int qb_dev = BOOT_DEVICE_NONE;
	char *interface = "";

	if (!qb_check())
		return CMD_RET_FAILURE;

	if (argc >= 2) {
		interface = argv[1];
	} else {
		/** qb save -> use boot device */
		boot_dev = get_boot_device();
		qb_dev = get_board_boot_device(boot_dev);
	}

	if (argc == 3)
		dev = simple_strtol(argv[2], NULL, 10);

	if (!strcmp(interface, "mmc") && dev >= 0
	    && dev <= (BOOT_DEVICE_MMC2_2 - BOOT_DEVICE_MMC1))
		qb_dev = BOOT_DEVICE_MMC1 + dev;

	if (!strcmp(interface, "spi"))
		qb_dev = BOOT_DEVICE_SPI;

	switch (qb_dev) {
	case BOOT_DEVICE_MMC1:
	case BOOT_DEVICE_MMC2:
	case BOOT_DEVICE_MMC2_2:
		ret = do_qb_mmc(qb_dev, true);
		break;
	case BOOT_DEVICE_SPI:
		ret = do_qb_spi(qb_dev, true);
		break;
	default:
		printf("Unsupported quickboot device\n");
		break;
	}

	if (ret)
		return CMD_RET_FAILURE;

	/**
	 * invalidate qb_state mem so that at next boot
	 * the check function will fail and save won't happen
	 */
	memset((void *)CONFIG_SAVED_QB_STATE_BASE, 0, sizeof(struct ddrphy_qb_state));

	return CMD_RET_SUCCESS;
}

static int do_qb_erase(struct cmd_tbl *cmdtp, int flag,
		       int argc, char * const argv[])
{
	int ret = CMD_RET_FAILURE;
	long dev = -1;
	enum boot_device boot_dev = UNKNOWN_BOOT;
	int qb_dev = BOOT_DEVICE_NONE;
	char *interface = "";

	if (argc >= 2) {
		interface = argv[1];
	} else {
		/** qb erase -> use boot device */
		boot_dev = get_boot_device();
		qb_dev = get_board_boot_device(boot_dev);
	}

	if (argc == 3)
		dev = simple_strtol(argv[2], NULL, 10);

	if (!strcmp(interface, "mmc") && dev >= 0
	    && dev <= (BOOT_DEVICE_MMC2_2 - BOOT_DEVICE_MMC1))
		qb_dev = BOOT_DEVICE_MMC1 + dev;

	if (!strcmp(interface, "spi"))
		qb_dev = BOOT_DEVICE_SPI;

	switch (qb_dev) {
	case BOOT_DEVICE_MMC1:
	case BOOT_DEVICE_MMC2:
	case BOOT_DEVICE_MMC2_2:
		ret = do_qb_mmc(qb_dev, false);
		break;
	case BOOT_DEVICE_SPI:
		ret = do_qb_spi(qb_dev, false);
		break;
	default:
		printf("Unsupported quickboot device\n");
		break;
	}

	return ret == 0 ? CMD_RET_SUCCESS : CMD_RET_FAILURE;
}

static struct cmd_tbl cmd_qb[] = {
	U_BOOT_CMD_MKENT(check, 1, 1, do_qb_check, "", ""),
	U_BOOT_CMD_MKENT(save,  3, 1, do_qb_save,  "", ""),
	U_BOOT_CMD_MKENT(erase, 3, 1, do_qb_erase, "", ""),
};

static int do_qbops(struct cmd_tbl *cmdtp, int flag, int argc,
		    char *const argv[])
{
	struct cmd_tbl *cp;

	cp = find_cmd_tbl(argv[1], cmd_qb, ARRAY_SIZE(cmd_qb));

	/* Drop the qb command */
	argc--;
	argv++;

	if (cp == NULL) {
		printf("%s cp is null\n", __func__);
		return CMD_RET_USAGE;
	}

	if (argc > cp->maxargs) {
		printf("%s argc(%d) > cp->maxargs(%d)\n", __func__, argc, cp->maxargs);
		return CMD_RET_USAGE;
	}

	if (flag == CMD_FLAG_REPEAT && !cmd_is_repeatable(cp)) {
		printf("%s %s repeat flag set  but command is not repeatable\n", __func__,
			cp->name);
		return CMD_RET_SUCCESS;
	}

	return cp->cmd(cmdtp, flag, argc, argv);
}

U_BOOT_CMD(
	qb, 4, 1, do_qbops,
	"DDR Quick Boot sub system",
	"check - check if quick boot data is stored in mem by training flow\n"
	"qb save [interface] [dev]  - save quick boot data in NVM location    => trigger quick boot flow\n"
	"qb erase [interface] [dev] - erase quick boot data from NVM location => trigger training flow\n"
);
