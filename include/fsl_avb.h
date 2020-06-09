/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __FSL_AVB_H__
#define __FSL_AVB_H__

#include "../lib/avb/libavb_atx/libavb_atx.h"
#include "../lib/avb/fsl/fsl_bootctrl.h"
/* Reads |num_bytes| from offset |offset| from partition with name
 * |partition| (NUL-terminated UTF-8 string). If |offset| is
 * negative, its absolute value should be interpreted as the number
 * of bytes from the end of the partition.
 *
 * This function returns AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION if
 * there is no partition with the given name,
 * AVB_IO_RESULT_ERROR_RANGE_OUTSIDE_PARTITION if the requested
 * |offset| is outside the partition, and AVB_IO_RESULT_ERROR_IO if
 * there was an I/O error from the underlying I/O subsystem.  If the
 * operation succeeds as requested AVB_IO_RESULT_OK is returned and
 * the data is available in |buffer|.
 *
 * The only time partial I/O may occur is if reading beyond the end
 * of the partition. In this case the value returned in
 * |out_num_read| may be smaller than |num_bytes|.
 */
AvbIOResult fsl_read_from_partition(AvbOps* ops, const char* partition,
                                    int64_t offset, size_t num_bytes,
                                    void* buffer, size_t* out_num_read);

/* multi block read version
 * */
AvbIOResult fsl_read_from_partition_multi(AvbOps* ops, const char* partition,
                                          int64_t offset, size_t num_bytes,
                                          void* buffer, size_t* out_num_read);

/* Writes |num_bytes| from |bffer| at offset |offset| to partition
 * with name |partition| (NUL-terminated UTF-8 string). If |offset|
 * is negative, its absolute value should be interpreted as the
 * number of bytes from the end of the partition.
 *
 * This function returns AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION if
 * there is no partition with the given name,
 * AVB_IO_RESULT_ERROR_RANGE_OUTSIDE_PARTITION if the requested
 * byterange goes outside the partition, and AVB_IO_RESULT_ERROR_IO
 * if there was an I/O error from the underlying I/O subsystem.  If
 * the operation succeeds as requested AVB_IO_RESULT_OK is
 * returned.
 *
 * This function never does any partial I/O, it either transfers all
 * of the requested bytes or returns an error.
 */
AvbIOResult fsl_write_to_partition(AvbOps* ops, const char* partition,
                                   int64_t offset, size_t num_bytes,
                                   const void* buffer);

/* Checks if the given public key used to sign the 'vbmeta'
 * partition is trusted. Boot loaders typically compare this with
 * embedded key material generated with 'avbtool
 * extract_public_key'.
 *
 * If AVB_IO_RESULT_OK is returned then |out_is_trusted| is set -
 * true if trusted or false if untrusted.
 */
AvbIOResult fsl_validate_vbmeta_public_key_rpmb(AvbOps* ops,
                                                const uint8_t* public_key_data,
                                                size_t public_key_length,
                                                const uint8_t* public_key_metadata,
                                                size_t public_key_metadata_length,
                                                bool* out_is_trusted);

/* Gets the rollback index corresponding to the slot given by
 * |rollback_index_slot|. The value is returned in
 * |out_rollback_index|. Returns AVB_IO_RESULT_OK if the rollback
 * index was retrieved, otherwise an error code.
 *
 * A device may have a limited amount of rollback index slots (say,
 * one or four) so may error out if |rollback_index_slot| exceeds
 * this number.
 */
AvbIOResult fsl_read_rollback_index_rpmb(AvbOps* ops, size_t rollback_index_slot,
                                         uint64_t* out_rollback_index);

/* Sets the rollback index corresponding to the slot given by
 * |rollback_index_slot| to |rollback_index|. Returns
 * AVB_IO_RESULT_OK if the rollback index was set, otherwise an
 * error code.
 *
 * A device may have a limited amount of rollback index slots (say,
 * one or four) so may error out if |rollback_index_slot| exceeds
 * this number.
 */
AvbIOResult fsl_write_rollback_index_rpmb(AvbOps* ops, size_t rollback_index_slot,
                                          uint64_t rollback_index);

/* Gets whether the device is unlocked. The value is returned in
 * |out_is_unlocked| (true if unlocked, false otherwise). Returns
 * AVB_IO_RESULT_OK if the state was retrieved, otherwise an error
 * code.
 */
AvbIOResult fsl_read_is_device_unlocked(AvbOps* ops, bool* out_is_unlocked);

/* Gets the unique partition GUID for a partition with name in
 * |partition| (NUL-terminated UTF-8 string). The GUID is copied as
 * a string into |guid_buf| of size |guid_buf_size| and will be NUL
 * terminated. The string must be lower-case and properly
 * hyphenated. For example:
 *
 *  527c1c6d-6361-4593-8842-3c78fcd39219
 *
 * Returns AVB_IO_RESULT_OK on success, otherwise an error code.
 */
AvbIOResult fsl_get_unique_guid_for_partition(AvbOps* ops,
                                              const char* partition,
                                              char* guid_buf,
                                              size_t guid_buf_size);

/* Gets the size of a partition with the name in |partition|
 * (NUL-terminated UTF-8 string). Returns the value in
 * |out_size_num_bytes|.
 * Returns AVB_IO_RESULT_OK on success, otherwise an error code.
 */
AvbIOResult fsl_get_size_of_partition(AvbOps* ops,
                                      const char* partition,
                                      uint64_t* out_size_num_bytes);

/* reset rollback_index part in avbkey partition
 * used in the switch from LOCK to UNLOCK
 * return 0 if success, non 0 if fail.
 * */
int rbkidx_erase(void);

/* init the avbkey in rpmb partition, include the header/public key/rollback index
 * for public key/rollback index part, use caam to do encrypt
 * return 0 if success, non 0 if fail.
 * */
int avbkey_init(uint8_t *plainkey, uint32_t keylen);

/* Reads permanent |attributes| data. There are no restrictions on where this
 * data is stored. On success, returns AVB_IO_RESULT_OK and populates
 * |attributes|.
 */
AvbIOResult fsl_read_permanent_attributes(
    AvbAtxOps* atx_ops, AvbAtxPermanentAttributes* attributes);

/* Reads a |hash| of permanent attributes. This hash MUST be retrieved from a
 * permanently read-only location (e.g. fuses) when a device is LOCKED. On
 * success, returned AVB_IO_RESULT_OK and populates |hash|.
 */
AvbIOResult fsl_read_permanent_attributes_hash(AvbAtxOps* atx_ops,
                                               uint8_t hash[AVB_SHA256_DIGEST_SIZE]);

/* Provides the key version of a key used during verification. This may be
 * useful for managing the minimum key version.
 */
void fsl_set_key_version(AvbAtxOps* atx_ops,
                         size_t rollback_index_location,
                         uint64_t key_version);

/* Generates |num_bytes| random bytes and stores them in |output|,
 * which must point to a buffer large enough to store the bytes.
 *
 * Returns AVB_IO_RESULT_OK on success, otherwise an error code.
 */
AvbIOResult fsl_get_random(AvbAtxOps* atx_ops,
				size_t num_bytes,
				uint8_t* output);

/* Program ATX perm_attr into RPMB partition */
int avb_atx_fuse_perm_attr(uint8_t *staged_buffer, uint32_t size);

/* Initialize rpmb key with the staged key */
int fastboot_set_rpmb_key(uint8_t *staged_buf, uint32_t key_size);

/* Initialize rpmb key with random key which is generated by caam rng */
int fastboot_set_rpmb_random_key(void);

/* Generate ATX unlock challenge */
int avb_atx_get_unlock_challenge(struct AvbAtxOps* atx_ops,
				uint8_t *upload_buffer, uint32_t *size);
/* Verify ATX unlock credential */
int avb_atx_verify_unlock_credential(struct AvbAtxOps* atx_ops,
					uint8_t *staged_buffer);
/* Check if the perm-attr have been fused. */
bool perm_attr_are_fused(void);

/* Check if the unlock vboot is already disabled */
bool at_unlock_vboot_is_disabled(void);

/* disable at unlock vboot */
int at_disable_vboot_unlock(void);

/* Set vbmeta public key */
int avb_set_public_key(uint8_t *staged_buffer, uint32_t size);

/* Get manufacture protection  public key */
int fastboot_get_mppubk(uint8_t *staged_buffer, uint32_t *size);

/* Check if hab is closed. */
bool hab_is_enabled(void);

/* Return if device is in spl recovery mode. */
bool is_spl_recovery(void);

#endif /* __FSL_AVB_H__ */
