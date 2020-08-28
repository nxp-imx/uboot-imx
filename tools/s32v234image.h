/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright 2019-2020 NXP */

#ifndef S32V234IMAGE_H
#define S32V234IMAGE_H

#include <asm/types.h>
#include <generated/autoconf.h>

#define DCD_HEADER			(0x500000d2)
#define DCD_MAXIMUM_SIZE		(8192)
#define DCD_HEADER_LENGTH_OFFSET	(1)

#define DCD_COMMAND_HEADER(tag, len, params) ((tag) | \
					      (cpu_to_be16((len)) << 8) | \
					      (params) << 24)
#define DCD_WRITE_TAG	(0xcc)
#define DCD_CHECK_TAG	(0xcf)
#define DCD_NOP_TAG	(0xc0)

#define PARAMS_DATA_SET		BIT(4)
#define PARAMS_DATA_MASK	BIT(3)
#define PARAMS_BYTES(x)		((x) & 0x7)

#define DCD_WRITE_HEADER(n, params)	DCD_COMMAND_HEADER(DCD_WRITE_TAG, \
							   4 + (n) * 8, \
							   (params))
#define DCD_CHECK_HEADER(params)	DCD_COMMAND_HEADER(DCD_CHECK_TAG, \
							   16, \
							   (params))
#define DCD_CHECK_HEADER_NO_COUNT(params) \
					DCD_COMMAND_HEADER(DCD_CHECK_TAG, \
							   12, \
							   (params))
#define DCD_NOP_HEADER			DCD_COMMAND_HEADER(DCD_NOP_TAG, 4, 0)

#define DCD_ADDR(x)	cpu_to_be32((x))
#define DCD_MASK(x)	cpu_to_be32((x))
#define DCD_COUNT(x)	cpu_to_be32((x))

#define IVT_TAG				0xd1
#define IVT_VERSION			0x50

struct ivt {
	__u8		tag;
	__u16		length;
	__u8		version;
	__u32		entry;
	__u32		reserved1;
	__u32		dcd_pointer;
	__u32		boot_data_pointer;
	__u32		self;
	__u32		reserved2;
	__u32		self_test;
	__u32		reserved3;
	__u32		reserved4;
} __attribute((packed));

struct boot_data {
	__u32		start;
	__u32		length;
	__u8		reserved2[4];
} __packed;

struct image_comp {
	size_t offset;
	size_t size;
	size_t alignment;
	uint8_t *data;
};

struct program_image {
	struct image_comp ivt;
#ifdef CONFIG_FLASH_BOOT
	struct image_comp qspi_params;
#endif
	struct image_comp boot_data;
	struct image_comp dcd;
	__u8 *header;
};

#ifdef CONFIG_FLASH_BOOT
struct qspi_params {
	__u32 dqs;
	__u8 hold_delay;
	__u8 half_speed_phase_sel;
	__u8 half_speed_delay_sel;
	__u8 reserved1;
	__u32 clock_conf;
	__u32 soc_conf;
	__u32 reserved2;
	__u32 cs_hold;
	__u32 cs_setup;
	__u32 flash_a1_size;
	__u32 flash_a2_size;
	__u32 flash_b1_size;
	__u32 flash_b2_size;
	__u32 clock_freq;
	__u32 reserved3;
	__u8 mode;
	__u8 flash_b_sel;
	__u8 ddr_mode;
	__u8 dss;
	__u8 parallel_mode_en;
	__u8 cs1_port_a;
	__u8 cs1_port_b;
	__u8 full_speed_phase_sel;
	__u8 full_speed_delay_sel;
	__u8 ddr_sampling_point;
	__u8 luts[256];
};

static const struct qspi_params s32v234_qspi_params = {
	.hold_delay = 0x1,
	.flash_a1_size = 0x40000000,
	.clock_freq = 0x3,
	.ddr_mode = 0x1,
	.dss = 0x1,
	.luts = {
		/*Flash specific LUT */
		0xA0, 0x47, 0x18, 0x2B, 0x10, 0x4F, 0x0F, 0x0F, 0x80,
		/* 128 bytes*/
		0x3B, 0x00, 0x03,
		/*STOP - 8pads*/
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	}
};
#endif //CONFIG_FLASH_BOOT

#endif /* S32V234IMAGE_H */
