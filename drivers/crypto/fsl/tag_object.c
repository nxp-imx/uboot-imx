// SPDX-License-Identifier: (GPL-2.0-or-later OR BSD-3-Clause)
/*
 * Copyright 2021 NXP
 *
 * Based on Tag object in drivers/crypto/caam in Linux
 */

#include <common.h>
#include "tag_object.h"

/**
 * init_tag_object_header - Initialize the tag object header by setting up
 *			the TAG_OBJECT_MAGIC number, tag object version,
 *			a valid type and the object's length
 * @header:		The header configuration to initialize
 * @version:		The tag object version
 * @type:		The tag object type
 * @red_key_len:	The red key length
 * @obj_len:		The object (actual data) length
 */
void init_tag_object_header(struct header_conf *header, u32 version,
			    u32 type, size_t red_key_len, size_t obj_len)
{
	header->_magic_number = TAG_OBJECT_MAGIC;
	header->version = version;
	header->type = type;
	header->red_key_len = red_key_len;
	header->obj_len = obj_len;
}

/**
 * set_tag_object_header_conf - Set tag object header configuration
 * @header:			The tag object header configuration to set
 * @buffer:			The buffer needed to be tagged
 * @buf_size:			The buffer size
 * @tag_obj_size:		The tagged object size
 *
 * Return:			'0' on success, error code otherwise
 */
int set_tag_object_header_conf(const struct header_conf *header,
			       void *buffer, size_t buf_size, u32 *tag_obj_size)
{
	/* Retrieve the tag object */
	struct tagged_object *tag_obj = (struct tagged_object *)buffer;
	/*
	 * Requested size for the tagged object is the buffer size
	 * and the header configuration size (TAG_OVERHEAD_SIZE)
	 */
	size_t req_size = buf_size + TAG_OVERHEAD_SIZE;

	/*
	 * Check if the configuration can be set,
	 * based on the size of the tagged object
	 */
	if (*tag_obj_size < req_size)
		return -EINVAL;

	/*
	 * Buffers might overlap, use memmove to
	 * copy the buffer into the tagged object
	 */
	memmove(&tag_obj->object, buffer, buf_size);
	/* Copy the tag object header configuration into the tagged object */
	memcpy(&tag_obj->header, header, TAG_OVERHEAD_SIZE);
	/* Set tagged object size */
	*tag_obj_size = req_size;

	return 0;
}

/**
 * tag_black_obj      - Tag a black object (blob/key) with a tag object header.
 *
 * @black_obj         : contains black key/blob,
 *                      obtained from CAAM, that needs to be tagged
 * @black_obj_len     : size of black object (blob/key)
 * @key_len           : size of plain key
 * @black_max_len     : The maximum size of the black object (blob/key)
 *
 * Return             : '0' on success, error code otherwise
 */
int tag_black_obj(u8 *black_obj, size_t black_obj_len, size_t key_len, size_t black_max_len)
{
	struct header_conf tag;
	u32 type = 1;				/*ECB encrypted black key*/
	int ret;
	u32 size_tagged = black_max_len;

	if (!black_obj)
		return -EINVAL;

	/* Prepare and set the tag */
	init_tag_object_header(&tag, 0, type, key_len, black_obj_len);
	ret = set_tag_object_header_conf(&tag, black_obj, black_obj_len, &size_tagged);

	return ret;
}
