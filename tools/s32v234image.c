// SPDX-License-Identifier: GPL-2.0+
/* Copyright 2019-2020 NXP */

#include <image.h>
#include <generated/autoconf.h>
#include <config.h>
#include "imagetool.h"
#include "s32v234image.h"
#include <asm/arch/mc_me_regs.h>
#include <asm/arch/mc_cgm_regs.h>

#define S32V234_AUTO_OFFSET ((size_t)(-1))

#ifdef CONFIG_FLASH_BOOT
#  define S32V234_COMMAND_SEQ_FILL_OFF 20
#endif

#ifdef CONFIG_FLASH_BOOT
#  define S32V234_QSPI_PARAMS_OFFSET	0x200U
#  define S32V234_QSPI_PARAMS_SIZE	0x200
#endif

#define S32V234_IVT_OFFSET	0x1000U

#ifdef CONFIG_FLASH_BOOT
#  define S32V234_HEADER_SIZE	0x2000U
#else
#  define S32V234_HEADER_SIZE	0x1000U
#endif
#define S32V234_INITLOAD_SIZE	0x2000U

static struct program_image image_layout = {
#ifdef CONFIG_FLASH_BOOT
	.qspi_params = {
		.offset = S32V234_QSPI_PARAMS_OFFSET,
		.size = S32V234_QSPI_PARAMS_SIZE,
	},
#endif
	.ivt = {
#ifdef CONFIG_FLASH_BOOT
		.offset = S32V234_IVT_OFFSET,
#else
		/* The offset is actually 0x1000, but we do not
		 * want to integrate it in the generated image.
		 * This allows writing the image at 0x1000 on
		 * sdcard/qspi, which avoids overwriting the
		 * partition table.
		 */
		.offset = 0x0,
#endif
		.size = sizeof(struct ivt),
	},
	.boot_data = {
		.offset = S32V234_AUTO_OFFSET,
		.alignment = 0x8U,
		.size = sizeof(struct boot_data),
	},
	.dcd = {
		.offset = S32V234_AUTO_OFFSET,
		.alignment = 0x8U,
		.size = DCD_MAXIMUM_SIZE,
	},
};

static uint32_t dcd_data[] = {
	DCD_HEADER,
	DCD_WRITE_HEADER(4, PARAMS_BYTES(4)),
	DCD_ADDR(FXOSC_CTL), DCD_MASK(FXOSC_CTL_FASTBOOT_VALUE),

	DCD_ADDR(MC_ME_DRUN_MC),
	DCD_MASK(DRUN_MC_RESETVAL | MC_ME_RUNMODE_MC_XOSCON |
		 MC_ME_RUNMODE_MC_SYSCLK(SYSCLK_FXOSC)),

	DCD_ADDR(MC_ME_MCTL),
	DCD_MASK(MC_ME_MCTL_KEY | MC_ME_MCTL_DRUN),
	DCD_ADDR(MC_ME_MCTL),
	DCD_MASK(MC_ME_MCTL_INVERTEDKEY | MC_ME_MCTL_DRUN),
};

static struct ivt *get_ivt(struct program_image *image)
{
	return (struct ivt *)image->ivt.data;
}

static uint8_t *get_dcd(struct program_image *image)
{
	return image->dcd.data;
}

static struct boot_data *get_boot_data(struct program_image *image)
{
	return (struct boot_data *)image->boot_data.data;
}

static void s32v234_print_header(const void *header)
{
}

#ifdef CONFIG_FLASH_BOOT
static struct qspi_params *get_qspi_params(struct program_image *image)
{
	return (struct qspi_params *)image->qspi_params.data;
}

static void s32v234_set_qspi_params(struct qspi_params *qspi_params)
{
	memcpy(qspi_params, &s32v234_qspi_params, sizeof(*qspi_params));
}
#endif

static void set_data_pointers(struct program_image *layout, void *header)
{
	uint8_t *data = (uint8_t *)header;

	layout->ivt.data = data + layout->ivt.offset;
#ifdef CONFIG_FLASH_BOOT
	layout->qspi_params.data = data + layout->qspi_params.offset;
#endif
	layout->boot_data.data = data + layout->boot_data.offset;
	layout->dcd.data = data + layout->dcd.offset;
}

static void s32v234_set_header(void *header, struct stat *sbuf, int unused,
			       struct image_tool_params *tool_params)
{
	uint8_t *dcd;
	struct ivt *ivt;
	struct boot_data *boot_data;

	set_data_pointers(&image_layout, header);

	dcd = get_dcd(&image_layout);
	if (sizeof(dcd_data) > DCD_MAXIMUM_SIZE) {
		fprintf(stderr, "DCD exceeds the maximum size\n");
		exit(EXIT_FAILURE);
	}
	memcpy(dcd, &dcd_data[0], sizeof(dcd_data));
	*(uint16_t *)(dcd + DCD_HEADER_LENGTH_OFFSET) =
						cpu_to_be16(sizeof(dcd_data));

	ivt = get_ivt(&image_layout);
	ivt->tag = IVT_TAG;
	ivt->length = cpu_to_be16(sizeof(struct ivt));
	ivt->version = IVT_VERSION;
	ivt->entry = CONFIG_SYS_TEXT_BASE;
	ivt->self = ivt->entry - S32V234_INITLOAD_SIZE + S32V234_IVT_OFFSET;
	ivt->dcd_pointer = ivt->self + image_layout.dcd.offset -
				image_layout.ivt.offset;
	ivt->boot_data_pointer = ivt->self + image_layout.boot_data.offset -
				image_layout.ivt.offset;

	boot_data = get_boot_data(&image_layout);
	boot_data->start = ivt->entry - S32V234_INITLOAD_SIZE;
	boot_data->length = ROUND(sbuf->st_size + S32V234_INITLOAD_SIZE,
				  0x1000);
#ifdef CONFIG_FLASH_BOOT
	s32v234_set_qspi_params(get_qspi_params(&image_layout));
#endif
}

static int s32v234_check_image_type(uint8_t type)
{
	if (type == IH_TYPE_S32V234IMAGE)
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;
}

static int image_parts_comp(const void *p1, const void *p2)
{
	const struct image_comp **part1 = (typeof(part1))p1;
	const struct image_comp **part2 = (typeof(part2))p2;

	if ((*part2)->offset > (*part1)->offset)
		return -1;

	if ((*part2)->offset < (*part1)->offset)
		return 1;

	return 0;
}

static void check_overlap(struct image_comp *comp1,
			  struct image_comp *comp2)
{
	size_t end1 = comp1->offset + comp1->size;
	size_t end2 = comp2->offset + comp2->size;

	if (end1 > comp2->offset && end2 > comp1->offset) {
		fprintf(stderr, "Detected overlap between 0x%zx@0x%zx and "
				"0x%zx@0x%zx\n",
				comp1->size, comp1->offset,
				comp2->size, comp2->offset);
		exit(EXIT_FAILURE);
	}
}

static void s32v234_compute_dyn_offsets(struct image_comp **parts,
					size_t n_parts)
{
	size_t i;
	size_t align_mask;
	size_t rem;

	for (i = 0U; i < n_parts; i++) {
		if (parts[i]->offset == S32V234_AUTO_OFFSET) {
			if (i == 0) {
				parts[i]->offset = 0U;
				continue;
			}

			parts[i]->offset = parts[i - 1]->offset +
			    parts[i - 1]->size;
		}

		/* Apply alignment constraints */
		if (parts[i]->alignment != 0U) {
			align_mask = parts[i]->alignment - 1U;
			rem = parts[i]->offset & align_mask;
			if (rem != 0U) {
				parts[i]->offset -= rem;
				parts[i]->offset += parts[i]->alignment;
			}
		}

		if (i != 0)
			check_overlap(parts[i - 1], parts[i]);
	}
}

static int s32v234_build_layout(struct program_image *program_image,
				size_t *header_size, void **image)
{
	uint8_t *image_layout;
	struct image_comp *parts[] = {&program_image->ivt,
		&program_image->boot_data,
		&program_image->dcd,
#ifdef CONFIG_FLASH_BOOT
		&program_image->qspi_params,
#endif
	};
	size_t last_comp = ARRAY_SIZE(parts) - 1;

	program_image->dcd.size = sizeof(dcd_data);

	qsort(&parts[0], ARRAY_SIZE(parts), sizeof(parts[0]), image_parts_comp);

	/* Compute auto-offsets */
	s32v234_compute_dyn_offsets(parts, ARRAY_SIZE(parts));

	*header_size = S32V234_HEADER_SIZE;
	if (parts[last_comp]->offset + parts[last_comp]->size > *header_size) {
		perror("S32V234 Header is too large");
		exit(EXIT_FAILURE);
	}

	image_layout = calloc(*header_size, sizeof(*image_layout));
	if (!image_layout) {
		perror("Call to calloc() failed");
		return -ENOMEM;
	}

	*image = image_layout;
	return 0;
}

static int s32v234_vrec_header(struct image_tool_params *tool_params,
			       struct image_type_params *type_params)
{
	size_t header_size;
	void *image = NULL;

	s32v234_build_layout(&image_layout, &header_size, &image);
	type_params->header_size = header_size;
	type_params->hdr = image;

	return 0;
}

U_BOOT_IMAGE_TYPE(
	s32v2image,
	"NXP S32V234 Boot Image",
	0,
	NULL,
	NULL,
	NULL,
	s32v234_print_header,
	s32v234_set_header,
	NULL,
	s32v234_check_image_type,
	NULL,
	s32v234_vrec_header
);
