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

#if !defined(AVB_INSIDE_LIBAVB_H) && !defined(AVB_COMPILATION)
#error "Never include this file directly, include libavb.h instead."
#endif

#ifndef AVB_SLOT_VERIFY_H_
#define AVB_SLOT_VERIFY_H_

#include "avb_ops.h"
#include "avb_vbmeta_image.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Return codes used in avb_slot_verify(), see that function for
 * documentation for each field.
 *
 * Use avb_slot_verify_result_to_string() to get a textual
 * representation usable for error/debug output.
 */
typedef enum {
  AVB_SLOT_VERIFY_RESULT_OK,
  AVB_SLOT_VERIFY_RESULT_ERROR_OOM,
  AVB_SLOT_VERIFY_RESULT_ERROR_IO,
  AVB_SLOT_VERIFY_RESULT_ERROR_VERIFICATION,
  AVB_SLOT_VERIFY_RESULT_ERROR_ROLLBACK_INDEX,
  AVB_SLOT_VERIFY_RESULT_ERROR_PUBLIC_KEY_REJECTED,
  AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA,
  AVB_SLOT_VERIFY_RESULT_ERROR_UNSUPPORTED_VERSION
} AvbSlotVerifyResult;

/* Get a textual representation of |result|. */
const char* avb_slot_verify_result_to_string(AvbSlotVerifyResult result);

/* Maximum number of rollback index locations supported. */
#define AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS 32

/* AvbPartitionData contains data loaded from partitions when using
 * avb_slot_verify(). The |partition_name| field contains the name of
 * the partition (without A/B suffix), |data| points to the loaded
 * data which is |data_size| bytes long.
 *
 * Note that this is strictly less than the partition size - it's only
 * the image stored there, not the entire partition nor any of the
 * metadata.
 */
typedef struct {
  char* partition_name;
  uint8_t* data;
  size_t data_size;
} AvbPartitionData;

/* AvbVBMetaData contains a vbmeta struct loaded from a partition when
 * using avb_slot_verify(). The |partition_name| field contains the
 * name of the partition (without A/B suffix), |vbmeta_data| points to
 * the loaded data which is |vbmeta_size| bytes long.
 *
 * The |verify_result| field contains the result of
 * avb_vbmeta_image_verify() on the data. This is guaranteed to be
 * AVB_VBMETA_VERIFY_RESULT_OK for all vbmeta images if
 * avb_slot_verify() returns AVB_SLOT_VERIFY_RESULT_OK.
 *
 * You can use avb_descriptor_get_all(), avb_descriptor_foreach(), and
 * avb_vbmeta_image_header_to_host_byte_order() with this data.
 */
typedef struct {
  char* partition_name;
  uint8_t* vbmeta_data;
  size_t vbmeta_size;
  AvbVBMetaVerifyResult verify_result;
} AvbVBMetaData;

/* AvbSlotVerifyData contains data needed to boot a particular slot
 * and is returned by avb_slot_verify() if partitions in a slot are
 * successfully verified.
 *
 * All data pointed to by this struct - including data in each item in
 * the |partitions| array - will be freed when the
 * avb_slot_verify_data_free() function is called.
 *
 * The |ab_suffix| field is the copy of the of |ab_suffix| field
 * passed to avb_slot_verify(). It is the A/B suffix of the slot.
 *
 * The VBMeta images that were checked are available in the
 * |vbmeta_images| field. The field |num_vbmeta_images| contains the
 * number of elements in this array. The first element -
 * vbmeta_images[0] - is guaranteed to be from the partition with the
 * top-level vbmeta struct. This is usually the "vbmeta" partition in
 * the requested slot but if there is no "vbmeta" partition it can
 * also be the "boot" partition.
 *
 * The partitions loaded and verified from from the slot are
 * accessible in the |loaded_partitions| array. The field
 * |num_loaded_partitions| contains the number of elements in this
 * array. The order of partitions in this array may not necessarily be
 * the same order as in the passed-in |requested_partitions| array.
 *
 * Rollback indexes for the verified slot are stored in the
 * |rollback_indexes| field. Note that avb_slot_verify() will NEVER
 * modify stored_rollback_index[n] locations e.g. it will never use
 * the write_rollback_index() AvbOps operation. Instead it is the job
 * of the caller of avb_slot_verify() to do this based on e.g. A/B
 * policy and other factors. See libavb_ab/avb_ab_flow.c for an
 * example of how to do this.
 *
 * The |cmdline| field is a NUL-terminated string in UTF-8 resulting
 * from concatenating all |AvbKernelCmdlineDescriptor| and then
 * performing proper substitution of the variables
 * $(ANDROID_SYSTEM_PARTUUID), $(ANDROID_BOOT_PARTUUID), and
 * $(ANDROID_VBMETA_PARTUUID) using the
 * get_unique_guid_for_partition() operation in |AvbOps|.
 *
 * Additionally, the |cmdline| field will have the following kernel
 * command-line options set:
 *
 *   androidboot.vbmeta.device_state: set to "locked" or "unlocked"
 *   depending on the result of the result of AvbOps's
 *   read_is_unlocked() function.
 *
 *   androidboot.vbmeta.{hash_alg, size, digest}: Will be set to
 *   the digest of all images in |vbmeta_images|.
 *
 *   androidboot.vbmeta.device: This is set to the value
 *   PARTUUID=$(ANDROID_VBMETA_PARTUUID) before substitution so it
 *   will end up pointing to the vbmeta partition for the verified
 *   slot. If there is no vbmeta partition it will point to the boot
 *   partition of the verified slot.
 *
 *   androidboot.vbmeta.avb_version: This is set to the decimal value
 *   of AVB_VERSION_MAJOR followed by a dot followed by the decimal
 *   value of AVB_VERSION_MINOR, for example "1.0" or "1.4". This
 *   version number represents the vbmeta file format version
 *   supported by libavb copy used in the boot loader. This is not
 *   necessarily the same version number of the on-disk metadata for
 *   the slot that was verified.
 *
 * Note that androidboot.slot_suffix is not set in |cmdline| - you
 * will have to pass this command-line option yourself.
 *
 * This struct may grow in the future without it being considered an
 * ABI break.
 */
typedef struct {
  char* ab_suffix;
  AvbVBMetaData* vbmeta_images;
  size_t num_vbmeta_images;
  AvbPartitionData* loaded_partitions;
  size_t num_loaded_partitions;
  char* cmdline;
  uint64_t rollback_indexes[AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS];
} AvbSlotVerifyData;

/* Frees a |AvbSlotVerifyData| including all data it points to. */
void avb_slot_verify_data_free(AvbSlotVerifyData* data);

/* Performs a full verification of the slot identified by |ab_suffix|
 * and load the contents of the partitions whose name is in the
 * NULL-terminated string array |requested_partitions| (each partition
 * must use hash verification). If not using A/B, pass an empty string
 * (e.g. "", not NULL) for |ab_suffix|.
 *
 * Typically the |requested_partitions| array only contains a single
 * item for the boot partition, 'boot'.
 *
 * Verification includes loading data from the 'vbmeta', all hash
 * partitions, and possibly other partitions (with |ab_suffix|
 * appended), inspecting rollback indexes, and checking if the public
 * key used to sign the data is acceptable. The functions in |ops|
 * will be used to do this.
 *
 * If |out_data| is not NULL, it will be set to a newly allocated
 * |AvbSlotVerifyData| struct containing all the data needed to
 * actually boot the slot. This data structure should be freed with
 * avb_slot_verify_data_free() when you are done with it. See below
 * for when this is returned.
 *
 * If |allow_verification_error| is false this function will bail out
 * as soon as an error is encountered and |out_data| is set only if
 * AVB_SLOT_VERIFY_RESULT_OK is returned.
 *
 * Otherwise if |allow_verification_error| is true the function will
 * continue verification efforts and |out_data| is also set if
 * AVB_SLOT_VERIFY_RESULT_ERROR_PUBLIC_KEY_REJECTED,
 * AVB_SLOT_VERIFY_RESULT_ERROR_VERIFICATION, or
 * AVB_SLOT_VERIFY_RESULT_ERROR_ROLLBACK_INDEX is returned. It is
 * undefined which error is returned if more than one distinct error
 * is encountered. It is guaranteed that AVB_SLOT_VERIFY_RESULT_OK is
 * returned if, and only if, there are no errors. This mode is needed
 * to boot valid but unverified slots when the device is unlocked.
 *
 * Also note that |out_data| is never set if
 * AVB_SLOT_VERIFY_RESULT_ERROR_OOM, AVB_SLOT_VERIFY_RESULT_ERROR_IO,
 * or AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA is returned.
 *
 * AVB_SLOT_VERIFY_RESULT_OK is returned if everything is verified
 * correctly and all public keys are accepted.
 *
 * AVB_SLOT_VERIFY_RESULT_ERROR_PUBLIC_KEY_REJECTED is returned if
 * everything is verified correctly out but one or more public keys
 * are not accepted. This includes the case where integrity data is
 * not signed.
 *
 * AVB_SLOT_VERIFY_RESULT_ERROR_OOM is returned if unable to
 * allocate memory.
 *
 * AVB_SLOT_VERIFY_RESULT_ERROR_IO is returned if an I/O error
 * occurred while trying to load data or get a rollback index.
 *
 * AVB_SLOT_VERIFY_RESULT_ERROR_VERIFICATION is returned if the data
 * did not verify, e.g. the digest didn't match or signature checks
 * failed.
 *
 * AVB_SLOT_VERIFY_RESULT_ERROR_ROLLBACK_INDEX is returned if a
 * rollback index was less than its stored value.
 *
 * AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA is returned if some
 * of the metadata is invalid or inconsistent.
 *
 * AVB_SLOT_VERIFY_RESULT_ERROR_UNSUPPORTED_VERSION is returned if
 * some of the metadata requires a newer version of libavb than what
 * is in use.
 */
AvbSlotVerifyResult avb_slot_verify(AvbOps* ops,
                                    const char* const* requested_partitions,
                                    const char* ab_suffix,
                                    bool allow_verification_error,
                                    AvbSlotVerifyData** out_data);

#ifdef __cplusplus
}
#endif

#endif /* AVB_SLOT_VERIFY_H_ */
