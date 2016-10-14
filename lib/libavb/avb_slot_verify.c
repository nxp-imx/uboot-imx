/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "avb_slot_verify.h"
#include "avb_chain_partition_descriptor.h"
#include "avb_footer.h"
#include "avb_hash_descriptor.h"
#include "avb_kernel_cmdline_descriptor.h"
#include "avb_sha.h"
#include "avb_util.h"
#include "avb_vbmeta_image.h"

/* Maximum allow length (in bytes) of a partition name, including
 * ab_suffix.
 */
#define PART_NAME_MAX_SIZE 32

/* Maximum number of partitions that can be loaded with avb_slot_verify(). */
#define MAX_NUMBER_OF_LOADED_PARTITIONS 32

/* Maximum size of a vbmeta image - 64 KiB. */
#define VBMETA_MAX_SIZE (64 * 1024)

static AvbSlotVerifyResult load_and_verify_hash_partition(
    AvbOps* ops, const char* const* requested_partitions, const char* ab_suffix,
    const AvbDescriptor* descriptor, AvbSlotVerifyData* slot_data) {
  AvbHashDescriptor hash_desc;
  const uint8_t* desc_partition_name;
  const uint8_t* desc_salt;
  const uint8_t* desc_digest;
  char part_name[PART_NAME_MAX_SIZE];
  AvbSlotVerifyResult ret;
  AvbIOResult io_ret;
  uint8_t* image_buf = NULL;
  size_t part_num_read;
  uint8_t* digest;
  size_t digest_len;
  const char* found;

  if (!avb_hash_descriptor_validate_and_byteswap(
          (const AvbHashDescriptor*)descriptor, &hash_desc)) {
    ret = AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA;
    goto out;
  }

  desc_partition_name =
      ((const uint8_t*)descriptor) + sizeof(AvbHashDescriptor);
  desc_salt = desc_partition_name + hash_desc.partition_name_len;
  desc_digest = desc_salt + hash_desc.salt_len;

  if (!avb_validate_utf8(desc_partition_name, hash_desc.partition_name_len)) {
    avb_error("Partition name is not valid UTF-8.\n");
    ret = AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA;
    goto out;
  }

  if (!avb_str_concat(
          part_name, sizeof part_name, (const char*)desc_partition_name,
          hash_desc.partition_name_len, ab_suffix, avb_strlen(ab_suffix))) {
    avb_error("Partition name and suffix does not fit.\n");
    ret = AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA;
    goto out;
  }

  image_buf = avb_malloc(hash_desc.image_size);
  if (image_buf == NULL) {
    ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
    goto out;
  }

  io_ret =
      ops->read_from_partition(ops, part_name, 0 /* offset */,
                               hash_desc.image_size, image_buf, &part_num_read);
  if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
    ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
    goto out;
  } else if (io_ret != AVB_IO_RESULT_OK) {
    avb_errorv(part_name, ": Error loading data from partition.\n", NULL);
    ret = AVB_SLOT_VERIFY_RESULT_ERROR_IO;
    goto out;
  }
  if (part_num_read != hash_desc.image_size) {
    avb_errorv(part_name, ": Read fewer than requested bytes.\n", NULL);
    ret = AVB_SLOT_VERIFY_RESULT_ERROR_IO;
    goto out;
  }

  if (avb_strcmp((const char*)hash_desc.hash_algorithm, "sha256") == 0) {
    AvbSHA256Ctx sha256_ctx;
    avb_sha256_init(&sha256_ctx);
    avb_sha256_update(&sha256_ctx, desc_salt, hash_desc.salt_len);
    avb_sha256_update(&sha256_ctx, image_buf, hash_desc.image_size);
    digest = avb_sha256_final(&sha256_ctx);
    digest_len = AVB_SHA256_DIGEST_SIZE;
  } else if (avb_strcmp((const char*)hash_desc.hash_algorithm, "sha512") == 0) {
    AvbSHA512Ctx sha512_ctx;
    avb_sha512_init(&sha512_ctx);
    avb_sha512_update(&sha512_ctx, desc_salt, hash_desc.salt_len);
    avb_sha512_update(&sha512_ctx, image_buf, hash_desc.image_size);
    digest = avb_sha512_final(&sha512_ctx);
    digest_len = AVB_SHA512_DIGEST_SIZE;
  } else {
    avb_errorv(part_name, ": Unsupported hash algorithm.\n", NULL);
    ret = AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA;
    goto out;
  }

  if (digest_len != hash_desc.digest_len) {
    avb_errorv(part_name, ": Digest in descriptor not of expected size.\n",
               NULL);
    ret = AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA;
    goto out;
  }

  if (avb_safe_memcmp(digest, desc_digest, digest_len) != 0) {
    avb_errorv(part_name,
               ": Hash of data does not match digest in descriptor.\n", NULL);
    ret = AVB_SLOT_VERIFY_RESULT_ERROR_VERIFICATION;
    goto out;
  }

  ret = AVB_SLOT_VERIFY_RESULT_OK;

  /* If this is the requested partition, copy to slot_data. */
  found =
      avb_strv_find_str(requested_partitions, (const char*)desc_partition_name,
                        hash_desc.partition_name_len);
  if (found != NULL) {
    AvbPartitionData* loaded_partition;
    if (slot_data->num_loaded_partitions == MAX_NUMBER_OF_LOADED_PARTITIONS) {
      avb_errorv(part_name, ": Too many loaded partitions.\n", NULL);
      ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
      goto out;
    }
    loaded_partition =
        &slot_data->loaded_partitions[slot_data->num_loaded_partitions++];
    loaded_partition->partition_name = avb_strdup(found);
    loaded_partition->data_size = hash_desc.image_size;
    loaded_partition->data = image_buf;
    image_buf = NULL;
  }

out:
  if (image_buf != NULL) {
    avb_free(image_buf);
  }
  return ret;
}

static AvbSlotVerifyResult load_and_verify_vbmeta(
    AvbOps* ops, const char* const* requested_partitions, const char* ab_suffix,
    int rollback_index_slot, const char* partition_name,
    size_t partition_name_len, const uint8_t* expected_public_key,
    size_t expected_public_key_length, AvbSlotVerifyData* slot_data,
    AvbAlgorithmType* out_algorithm_type) {
  char full_partition_name[PART_NAME_MAX_SIZE];
  AvbSlotVerifyResult ret;
  AvbIOResult io_ret;
  size_t vbmeta_offset;
  size_t vbmeta_size;
  uint8_t* vbmeta_buf = NULL;
  size_t vbmeta_num_read;
  AvbVBMetaVerifyResult vbmeta_ret;
  const uint8_t* pk_data;
  size_t pk_len;
  AvbVBMetaImageHeader vbmeta_header;
  uint64_t stored_rollback_index;
  const AvbDescriptor** descriptors = NULL;
  size_t num_descriptors;
  size_t n;
  int is_main_vbmeta;

  avb_assert(slot_data != NULL);

  is_main_vbmeta = (avb_strcmp(partition_name, "vbmeta") == 0);

  if (!avb_validate_utf8((const uint8_t*)partition_name, partition_name_len)) {
    avb_error("Partition name is not valid UTF-8.\n");
    ret = AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA;
    goto out;
  }

  /* Construct full partition name. */
  if (!avb_str_concat(full_partition_name, sizeof full_partition_name,
                      partition_name, partition_name_len, ab_suffix,
                      avb_strlen(ab_suffix))) {
    avb_error("Partition name and suffix does not fit.\n");
    ret = AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA;
    goto out;
  }

  avb_debugv("Loading vbmeta struct from partition '", full_partition_name,
             "'.\n", NULL);

  /* If we're loading from the main vbmeta partition, the vbmeta
   * struct is in the beginning. Otherwise we have to locate it via a
   * footer.
   */
  if (is_main_vbmeta) {
    vbmeta_offset = 0;
    vbmeta_size = VBMETA_MAX_SIZE;
  } else {
    uint8_t footer_buf[AVB_FOOTER_SIZE];
    size_t footer_num_read;
    AvbFooter footer;

    io_ret =
        ops->read_from_partition(ops, full_partition_name, -AVB_FOOTER_SIZE,
                                 AVB_FOOTER_SIZE, footer_buf, &footer_num_read);
    if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
      ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
      goto out;
    } else if (io_ret != AVB_IO_RESULT_OK) {
      avb_errorv(full_partition_name, ": Error loading footer.\n", NULL);
      ret = AVB_SLOT_VERIFY_RESULT_ERROR_IO;
      goto out;
    }
    avb_assert(footer_num_read == AVB_FOOTER_SIZE);

    if (!avb_footer_validate_and_byteswap((const AvbFooter*)footer_buf,
                                          &footer)) {
      avb_errorv(full_partition_name, ": Error validating footer.\n", NULL);
      ret = AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA;
      goto out;
    }

    /* Basic footer sanity check since the data is untrusted. */
    if (footer.vbmeta_size > VBMETA_MAX_SIZE) {
      avb_errorv(full_partition_name, ": Invalid vbmeta size in footer.\n",
                 NULL);
      ret = AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA;
      goto out;
    }

    vbmeta_offset = footer.vbmeta_offset;
    vbmeta_size = footer.vbmeta_size;
  }

  vbmeta_buf = avb_malloc(vbmeta_size);
  if (vbmeta_buf == NULL) {
    ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
    goto out;
  }

  io_ret = ops->read_from_partition(ops, full_partition_name, vbmeta_offset,
                                    vbmeta_size, vbmeta_buf, &vbmeta_num_read);
  if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
    ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
    goto out;
  } else if (io_ret != AVB_IO_RESULT_OK) {
    avb_errorv(full_partition_name, ": Error loading vbmeta data.\n", NULL);
    ret = AVB_SLOT_VERIFY_RESULT_ERROR_IO;
    goto out;
  }
  avb_assert(vbmeta_num_read <= vbmeta_size);

  /* Check if the image is properly signed and get the public key used
   * to sign the image.
   */
  vbmeta_ret =
      avb_vbmeta_image_verify(vbmeta_buf, vbmeta_num_read, &pk_data, &pk_len);
  if (vbmeta_ret != AVB_VBMETA_VERIFY_RESULT_OK) {
    avb_errorv(full_partition_name, ": Error verifying vbmeta image.\n", NULL);
    ret = AVB_SLOT_VERIFY_RESULT_ERROR_VERIFICATION;
    goto out;
  }

  /* Check if key used to make signature matches what is expected. */
  if (expected_public_key != NULL) {
    avb_assert(!is_main_vbmeta);
    if (expected_public_key_length != pk_len ||
        avb_safe_memcmp(expected_public_key, pk_data, pk_len) != 0) {
      avb_errorv(full_partition_name,
                 ": Public key used to sign data does not match key in chain "
                 "partition descriptor.\n",
                 NULL);
      ret = AVB_SLOT_VERIFY_RESULT_ERROR_PUBLIC_KEY_REJECTED;
      goto out;
    }
  } else {
    bool key_is_trusted = false;

    avb_assert(is_main_vbmeta);
    io_ret =
        ops->validate_vbmeta_public_key(ops, pk_data, pk_len, &key_is_trusted);
    if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
      ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
      goto out;
    } else if (io_ret != AVB_IO_RESULT_OK) {
      avb_errorv(full_partition_name,
                 ": Error while checking public key used to sign data.\n",
                 NULL);
      ret = AVB_SLOT_VERIFY_RESULT_ERROR_IO;
      goto out;
    }
    if (!key_is_trusted) {
      avb_errorv(full_partition_name,
                 ": Public key used to sign data rejected.\n", NULL);
      ret = AVB_SLOT_VERIFY_RESULT_ERROR_PUBLIC_KEY_REJECTED;
      goto out;
    }
  }

  avb_vbmeta_image_header_to_host_byte_order((AvbVBMetaImageHeader*)vbmeta_buf,
                                             &vbmeta_header);

  /* Check rollback index. */
  io_ret = ops->read_rollback_index(ops, rollback_index_slot,
                                    &stored_rollback_index);
  if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
    ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
    goto out;
  } else if (io_ret != AVB_IO_RESULT_OK) {
    avb_errorv(full_partition_name,
               ": Error getting rollback index for slot.\n", NULL);
    ret = AVB_SLOT_VERIFY_RESULT_ERROR_IO;
    goto out;
  }
  if (vbmeta_header.rollback_index < stored_rollback_index) {
    avb_errorv(
        full_partition_name,
        ": Image rollback index is less than the stored rollback index.\n",
        NULL);
    ret = AVB_SLOT_VERIFY_RESULT_ERROR_ROLLBACK_INDEX;
    goto out;
  }

  /* Now go through all descriptors and take the appropriate action:
   *
   * - hash descriptor: Load data from partition, calculate hash, and
   *   checks that it matches what's in the hash descriptor.
   *
   * - hashtree descriptor: Do nothing since verification happens
   *   on-the-fly from within the OS.
   *
   * - chained partition descriptor: Load the footer, load the vbmeta
   *   image, verify vbmeta image (includes rollback checks, hash
   *   checks, bail on chained partitions).
   */
  descriptors =
      avb_descriptor_get_all(vbmeta_buf, vbmeta_num_read, &num_descriptors);
  for (n = 0; n < num_descriptors; n++) {
    AvbDescriptor desc;

    if (!avb_descriptor_validate_and_byteswap(descriptors[n], &desc)) {
      avb_errorv(full_partition_name, ": Descriptor is invalid.\n", NULL);
      ret = AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA;
      goto out;
    }

    switch (desc.tag) {
      case AVB_DESCRIPTOR_TAG_HASH: {
        AvbSlotVerifyResult sub_ret;
        sub_ret = load_and_verify_hash_partition(
            ops, requested_partitions, ab_suffix, descriptors[n], slot_data);
        if (sub_ret != AVB_SLOT_VERIFY_RESULT_OK) {
          ret = sub_ret;
          goto out;
        }
      } break;

      case AVB_DESCRIPTOR_TAG_CHAIN_PARTITION: {
        AvbSlotVerifyResult sub_ret;
        AvbChainPartitionDescriptor chain_desc;
        const uint8_t* chain_partition_name;
        const uint8_t* chain_public_key;

        /* Only allow CHAIN_PARTITION descriptors in the main vbmeta image. */
        if (!is_main_vbmeta) {
          avb_errorv(full_partition_name,
                     ": Encountered chain descriptor not in main image.\n",
                     NULL);
          ret = AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA;
          goto out;
        }

        if (!avb_chain_partition_descriptor_validate_and_byteswap(
                (AvbChainPartitionDescriptor*)descriptors[n], &chain_desc)) {
          avb_errorv(full_partition_name,
                     ": Chain partition descriptor is invalid.\n", NULL);
          ret = AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA;
          goto out;
        }

        chain_partition_name = ((const uint8_t*)descriptors[n]) +
                               sizeof(AvbChainPartitionDescriptor);
        chain_public_key = chain_partition_name + chain_desc.partition_name_len;

        sub_ret = load_and_verify_vbmeta(
            ops, requested_partitions, ab_suffix,
            chain_desc.rollback_index_slot, (const char*)chain_partition_name,
            chain_desc.partition_name_len, chain_public_key,
            chain_desc.public_key_len, slot_data,
            NULL /* out_algorithm_type */);
        if (sub_ret != AVB_SLOT_VERIFY_RESULT_OK) {
          ret = sub_ret;
          goto out;
        }
      } break;

      case AVB_DESCRIPTOR_TAG_KERNEL_CMDLINE: {
        const uint8_t* kernel_cmdline;
        AvbKernelCmdlineDescriptor kernel_cmdline_desc;

        if (!avb_kernel_cmdline_descriptor_validate_and_byteswap(
                (AvbKernelCmdlineDescriptor*)descriptors[n],
                &kernel_cmdline_desc)) {
          avb_errorv(full_partition_name,
                     ": Kernel cmdline descriptor is invalid.\n", NULL);
          ret = AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA;
          goto out;
        }

        kernel_cmdline = ((const uint8_t*)descriptors[n]) +
                         sizeof(AvbKernelCmdlineDescriptor);

        if (!avb_validate_utf8(kernel_cmdline,
                               kernel_cmdline_desc.kernel_cmdline_length)) {
          avb_errorv(full_partition_name,
                     ": Kernel cmdline is not valid UTF-8.\n", NULL);
          ret = AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA;
          goto out;
        }

        if (slot_data->cmdline == NULL) {
          slot_data->cmdline =
              avb_calloc(kernel_cmdline_desc.kernel_cmdline_length + 1);
          if (slot_data->cmdline == NULL) {
            ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
            goto out;
          }
          avb_memcpy(slot_data->cmdline, kernel_cmdline,
                     kernel_cmdline_desc.kernel_cmdline_length);
        } else {
          /* new cmdline is: <existing_cmdline> + ' ' + <newcmdline> + '\0' */
          size_t orig_size = avb_strlen(slot_data->cmdline);
          size_t new_size =
              orig_size + 1 + kernel_cmdline_desc.kernel_cmdline_length + 1;
          char* new_cmdline = avb_calloc(new_size);
          if (new_cmdline == NULL) {
            ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
            goto out;
          }
          avb_memcpy(new_cmdline, slot_data->cmdline, orig_size);
          new_cmdline[orig_size] = ' ';
          avb_memcpy(new_cmdline + orig_size + 1, kernel_cmdline,
                     kernel_cmdline_desc.kernel_cmdline_length);
          avb_free(slot_data->cmdline);
          slot_data->cmdline = new_cmdline;
        }
      } break;

      /* Explicit fall-through */
      case AVB_DESCRIPTOR_TAG_PROPERTY:
      case AVB_DESCRIPTOR_TAG_HASHTREE:
        /* Do nothing. */
        break;
    }
  }

  ret = AVB_SLOT_VERIFY_RESULT_OK;

  /* So far, so good. Copy needed data to user, if requested. */
  if (is_main_vbmeta) {
    if (slot_data->vbmeta_data != NULL) {
      avb_free(slot_data->vbmeta_data);
    }
    /* Note that |vbmeta_buf| is actually |vbmeta_num_read| bytes long
     * and this includes data past the end of the image. Pass the
     * actual size of the vbmeta image. Also, no need to use
     * avb_safe_add() since the header has already been verified.
     */
    slot_data->vbmeta_size = sizeof(AvbVBMetaImageHeader) +
                             vbmeta_header.authentication_data_block_size +
                             vbmeta_header.auxiliary_data_block_size;
    slot_data->vbmeta_data = vbmeta_buf;
    vbmeta_buf = NULL;
  }

  if (rollback_index_slot >= AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_SLOTS) {
    avb_errorv(full_partition_name, ": Invalid rollback_index_slot.\n", NULL);
    ret = AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA;
    goto out;
  }

  slot_data->rollback_indexes[rollback_index_slot] =
      vbmeta_header.rollback_index;

  if (out_algorithm_type != NULL) {
    *out_algorithm_type = (AvbAlgorithmType)vbmeta_header.algorithm_type;
  }

out:
  if (vbmeta_buf != NULL) {
    avb_free(vbmeta_buf);
  }
  if (descriptors != NULL) {
    avb_free(descriptors);
  }
  return ret;
}

#define NUM_GUIDS 2

/* Substitutes all variables (e.g. $(ANDROID_SYSTEM_PARTUUID)) with
 * values. Returns NULL on OOM, otherwise the cmdline with values
 * replaced.
 */
static char* sub_cmdline(AvbOps* ops, const char* cmdline,
                         const char* ab_suffix) {
  const char* part_name_str[NUM_GUIDS] = {"system", "boot"};
  const char* replace_str[NUM_GUIDS] = {"$(ANDROID_SYSTEM_PARTUUID)",
                                        "$(ANDROID_BOOT_PARTUUID)"};
  char* ret = NULL;
  AvbIOResult io_ret;

  /* Replace unique partition GUIDs */
  for (size_t n = 0; n < NUM_GUIDS; n++) {
    char part_name[PART_NAME_MAX_SIZE];
    char guid_buf[37];
    char* new_ret;

    if (!avb_str_concat(part_name, sizeof part_name, part_name_str[n],
                        avb_strlen(part_name_str[n]), ab_suffix,
                        avb_strlen(ab_suffix))) {
      avb_error("Partition name and suffix does not fit.\n");
      goto fail;
    }

    io_ret = ops->get_unique_guid_for_partition(ops, part_name, guid_buf,
                                                sizeof guid_buf);
    if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
      return NULL;
    } else if (io_ret != AVB_IO_RESULT_OK) {
      avb_error("Error getting unique GUID for partition.\n");
      goto fail;
    }

    if (ret == NULL) {
      new_ret = avb_replace(cmdline, replace_str[n], guid_buf);
    } else {
      new_ret = avb_replace(ret, replace_str[n], guid_buf);
    }
    if (new_ret == NULL) {
      goto fail;
    }
    ret = new_ret;
  }

  return ret;

fail:
  if (ret != NULL) {
    avb_free(ret);
  }
  return NULL;
}

static int cmdline_append_option(AvbSlotVerifyData* slot_data, const char* key,
                                 const char* value) {
  size_t offset, key_len, value_len;
  char* new_cmdline;

  key_len = avb_strlen(key);
  value_len = avb_strlen(value);

  offset = 0;
  if (slot_data->cmdline != NULL) {
    offset = avb_strlen(slot_data->cmdline);
    if (offset > 0) {
      offset += 1;
    }
  }

  new_cmdline = avb_calloc(offset + key_len + value_len + 2);
  if (new_cmdline == NULL) {
    return 0;
  }
  if (offset > 0) {
    avb_memcpy(new_cmdline, slot_data->cmdline, offset - 1);
    new_cmdline[offset - 1] = ' ';
  }
  avb_memcpy(new_cmdline + offset, key, key_len);
  new_cmdline[offset + key_len] = '=';
  avb_memcpy(new_cmdline + offset + key_len + 1, value, value_len);
  if (slot_data->cmdline != NULL) {
    avb_free(slot_data->cmdline);
  }
  slot_data->cmdline = new_cmdline;

  return 1;
}

static int cmdline_append_uint64_base10(AvbSlotVerifyData* slot_data,
                                        const char* key, uint64_t value) {
  const int MAX_DIGITS = 32;
  char rev_digits[MAX_DIGITS];
  char digits[MAX_DIGITS];
  size_t n, num_digits;

  for (num_digits = 0; num_digits < MAX_DIGITS - 1;) {
    rev_digits[num_digits++] = (value % 10) + '0';
    value /= 10;
    if (value == 0) {
      break;
    }
  }

  for (n = 0; n < num_digits; n++) {
    digits[n] = rev_digits[num_digits - 1 - n];
  }
  digits[n] = '\0';

  return cmdline_append_option(slot_data, key, digits);
}

static int cmdline_append_hex(AvbSlotVerifyData* slot_data, const char* key,
                              const uint8_t* data, size_t data_len) {
  char hex_digits[17] = "0123456789abcdef";
  char* hex_data;
  int ret;
  size_t n;

  hex_data = avb_malloc(data_len * 2 + 1);
  if (hex_data == NULL) return 0;

  for (n = 0; n < data_len; n++) {
    hex_data[n * 2] = hex_digits[data[n] >> 4];
    hex_data[n * 2 + 1] = hex_digits[data[n] & 0x0f];
  }
  hex_data[n * 2] = '\0';

  ret = cmdline_append_option(slot_data, key, hex_data);
  avb_free(hex_data);
  return ret;
}

AvbSlotVerifyResult avb_slot_verify(AvbOps* ops,
                                    const char* const* requested_partitions,
                                    const char* ab_suffix,
                                    AvbSlotVerifyData** out_data) {
  AvbSlotVerifyResult ret;
  AvbSlotVerifyData* slot_data = NULL;
  AvbAlgorithmType algorithm_type = AVB_ALGORITHM_TYPE_NONE;
  AvbIOResult io_ret;

  if (out_data != NULL) {
    *out_data = NULL;
  }

  slot_data = avb_calloc(sizeof(AvbSlotVerifyData));
  if (slot_data == NULL) {
    ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
    goto fail;
  }
  slot_data->loaded_partitions =
      avb_calloc(sizeof(AvbPartitionData) * MAX_NUMBER_OF_LOADED_PARTITIONS);
  if (slot_data->loaded_partitions == NULL) {
    ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
    goto fail;
  }

  ret = load_and_verify_vbmeta(
      ops, requested_partitions, ab_suffix, 0 /* rollback_index_slot */,
      "vbmeta", avb_strlen("vbmeta"), NULL /* expected_public_key */,
      0 /* expected_public_key_length */, slot_data, &algorithm_type);

  /* If things check out, mangle the kernel command-line as needed. */
  if (ret == AVB_SLOT_VERIFY_RESULT_OK) {
    /* Fill in |ab_suffix| field. */
    slot_data->ab_suffix = avb_strdup(ab_suffix);
    if (slot_data->ab_suffix == NULL) {
      ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
      goto fail;
    }

    /* Substitute $(ANDROID_SYSTEM_PARTUUID) and friends. */
    if (slot_data->cmdline != NULL) {
      char* new_cmdline = sub_cmdline(ops, slot_data->cmdline, ab_suffix);
      if (new_cmdline == NULL) {
        ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
        goto fail;
      }
      avb_free(slot_data->cmdline);
      slot_data->cmdline = new_cmdline;
    }

    /* Add androidboot.slot_suffix, if applicable. */
    if (avb_strlen(ab_suffix) > 0) {
      if (!cmdline_append_option(slot_data, "androidboot.slot_suffix",
                                 ab_suffix)) {
        ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
        goto fail;
      }
    }

    /* Set androidboot.avb.device_state to "locked" or "unlocked". */
    bool is_device_unlocked;
    io_ret = ops->read_is_device_unlocked(ops, &is_device_unlocked);
    if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
      ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
      goto fail;
    } else if (io_ret != AVB_IO_RESULT_OK) {
      avb_error("Error getting device state.\n");
      ret = AVB_SLOT_VERIFY_RESULT_ERROR_IO;
      goto fail;
    }
    if (!cmdline_append_option(slot_data, "androidboot.vbmeta.device_state",
                               is_device_unlocked ? "unlocked" : "locked")) {
      ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
      goto fail;
    }

    /* Set androidboot.vbmeta.{hash_alg, size, digest} - use same hash
     * function as is used to sign vbmeta.
     */
    switch (algorithm_type) {
      /* Explicit fallthrough. */
      case AVB_ALGORITHM_TYPE_NONE:
      case AVB_ALGORITHM_TYPE_SHA256_RSA2048:
      case AVB_ALGORITHM_TYPE_SHA256_RSA4096:
      case AVB_ALGORITHM_TYPE_SHA256_RSA8192: {
        AvbSHA256Ctx ctx;
        avb_sha256_init(&ctx);
        avb_sha256_update(&ctx, slot_data->vbmeta_data, slot_data->vbmeta_size);
        if (!cmdline_append_option(slot_data, "androidboot.vbmeta.hash_alg",
                                   "sha256") ||
            !cmdline_append_uint64_base10(slot_data, "androidboot.vbmeta.size",
                                          slot_data->vbmeta_size) ||
            !cmdline_append_hex(slot_data, "androidboot.vbmeta.digest",
                                avb_sha256_final(&ctx),
                                AVB_SHA256_DIGEST_SIZE)) {
          ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
          goto fail;
        }
      } break;
      /* Explicit fallthrough. */
      case AVB_ALGORITHM_TYPE_SHA512_RSA2048:
      case AVB_ALGORITHM_TYPE_SHA512_RSA4096:
      case AVB_ALGORITHM_TYPE_SHA512_RSA8192: {
        AvbSHA512Ctx ctx;
        avb_sha512_init(&ctx);
        avb_sha512_update(&ctx, slot_data->vbmeta_data, slot_data->vbmeta_size);
        if (!cmdline_append_option(slot_data, "androidboot.vbmeta.hash_alg",
                                   "sha512") ||
            !cmdline_append_uint64_base10(slot_data, "androidboot.vbmeta.size",
                                          slot_data->vbmeta_size) ||
            !cmdline_append_hex(slot_data, "androidboot.vbmeta.digest",
                                avb_sha512_final(&ctx),
                                AVB_SHA512_DIGEST_SIZE)) {
          ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
          goto fail;
        }
      } break;
      case _AVB_ALGORITHM_NUM_TYPES:
        avb_assert_not_reached();
        break;
    }

    if (out_data != NULL) {
      *out_data = slot_data;
    } else {
      avb_slot_verify_data_free(slot_data);
    }
  }

  return ret;

fail:
  if (slot_data != NULL) {
    avb_slot_verify_data_free(slot_data);
  }
  return ret;
}

void avb_slot_verify_data_free(AvbSlotVerifyData* data) {
  if (data->ab_suffix != NULL) {
    avb_free(data->ab_suffix);
  }
  if (data->vbmeta_data != NULL) {
    avb_free(data->vbmeta_data);
  }
  if (data->cmdline != NULL) {
    avb_free(data->cmdline);
  }
  if (data->loaded_partitions != NULL) {
    size_t n;
    for (n = 0; n < data->num_loaded_partitions; n++) {
      AvbPartitionData* loaded_partition = &data->loaded_partitions[n];
      if (loaded_partition->partition_name != NULL) {
        avb_free(loaded_partition->partition_name);
      }
      if (loaded_partition->data != NULL) {
        avb_free(loaded_partition->data);
      }
    }
    avb_free(data->loaded_partitions);
  }
  avb_free(data);
}

const char* avb_slot_verify_result_to_string(AvbSlotVerifyResult result) {
  const char* ret = NULL;

  switch (result) {
    case AVB_SLOT_VERIFY_RESULT_OK:
      ret = "OK";
      break;
    case AVB_SLOT_VERIFY_RESULT_ERROR_OOM:
      ret = "ERROR_OOM";
      break;
    case AVB_SLOT_VERIFY_RESULT_ERROR_IO:
      ret = "ERROR_IO";
      break;
    case AVB_SLOT_VERIFY_RESULT_ERROR_VERIFICATION:
      ret = "ERROR_VERIFICATION";
      break;
    case AVB_SLOT_VERIFY_RESULT_ERROR_ROLLBACK_INDEX:
      ret = "ERROR_ROLLBACK_INDEX";
      break;
    case AVB_SLOT_VERIFY_RESULT_ERROR_PUBLIC_KEY_REJECTED:
      ret = "ERROR_PUBLIC_KEY_REJECTED";
      break;
    case AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA:
      ret = "ERROR_INVALID_METADATA";
      break;
      /* Do not add a 'default:' case here because of -Wswitch. */
  }

  if (ret == NULL) {
    avb_error("Unknown AvbSlotVerifyResult value.\n");
    ret = "(unknown)";
  }

  return ret;
}
