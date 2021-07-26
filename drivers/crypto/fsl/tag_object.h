/* SPDX-License-Identifier: (GPL-2.0-or-later OR BSD-3-Clause) */
/*
 * Copyright 2021 NXP
 *
 */

#ifndef _TAG_OBJECT_H_
#define _TAG_OBJECT_H_

#include <linux/compiler.h>
#include "type.h"

/**
 * Magic number to identify the tag object structure
 * 0x54 = 'T'
 * 0x61 = 'a'
 * 0x67 = 'g'
 * 0x4f = 'O'
 */
#define TAG_OBJECT_MAGIC	0x5461674f
#define TAG_OVERHEAD_SIZE	sizeof(struct header_conf)

/**
 * struct header_conf - Header configuration structure, which represents
 *			the metadata (or simply a header) applied to the
 *			actual data (e.g. black key)
 * @_magic_number     : A magic number to identify the structure
 * @version           : The version of the data contained (e.g. tag object)
 * @type              : The type of data contained (e.g. black key, blob, etc.)
 * @red_key_len       : Length of the red key to be loaded by CAAM (for key
 *                      generation or blob encapsulation)
 * @obj_len           : The total length of the (black/red) object (key/blob),
 *                      after encryption/encapsulation
 */
struct header_conf {
	u32 _magic_number;
	u32 version;
	u32 type;
	u32 red_key_len;
	u32 obj_len;
};

/**
 * struct tagged_object - Tag object structure, which represents the metadata
 *                        (or simply a header) and the actual data
 *                        (e.g. black key) obtained from hardware
 * @tag                 : The configuration of the data (e.g. header)
 * @object              : The actual data (e.g. black key)
 */
struct tagged_object {
	struct header_conf header;
	char object;
};

void init_tag_object_header(struct header_conf *header, u32 version,
			    u32 type, size_t red_key_len, size_t obj_len);

int set_tag_object_header_conf(const struct header_conf *header,
			       void *buffer, size_t obj_size, u32 *to_size);

#endif /* _TAG_OBJECT_H_ */
