/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __FSL_AVB_H__
#define __FSL_AVB_H__

#include "../lib/avb/libavb_ab/libavb_ab.h"
#include "../lib/avb/libavb_atx/libavb_atx.h"
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

/* Reads A/B metadata from persistent storage. Returned data is
 * properly byteswapped. Returns AVB_IO_RESULT_OK on success, error
 * code otherwise.
 *
 * If the data read is invalid (e.g. wrong magic or CRC checksum
 * failure), the metadata shoule be reset using avb_ab_data_init()
 * and then written to persistent storage.
 *
 * Implementations will typically want to use avb_ab_data_read()
 * here to use the 'misc' partition for persistent storage.
 */
AvbIOResult fsl_read_ab_metadata(AvbABOps* ab_ops, struct AvbABData* data);

/* Writes A/B metadata to persistent storage. This will byteswap and
 * update the CRC as needed. Returns AVB_IO_RESULT_OK on success,
 * error code otherwise.
 *
 * Implementations will typically want to use avb_ab_data_write()
 * here to use the 'misc' partition for persistent storage.
 */
AvbIOResult fsl_write_ab_metadata(AvbABOps* ab_ops, const struct AvbABData* data);

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
/* check if the fastboot getvar cmd is for query [avb] bootctl's slot var
 * cmd is the fastboot getvar's cmd in
 * return true if it is a bootctl related cmd, false if it's not.
 * */
bool is_slotvar_avb(char *cmd);

/* Get current bootable slot with higher priority.
 * return 0 for the first slot
 * return 1 for the second slot
 * return -1 for not supported slot
 * */
int get_curr_slot(AvbABData *ab_data);

/* return 0 for the first slot
 * return 1 for the second slot
 * return -1 for not supported slot
 * */
int slotidx_from_suffix(char *suffix);

/* return fastboot's getvar cmd response
 * cmd is the fastboot getvar's cmd in
 * if return 0, buffer is bootctl's slot var out
 * if return -1, buffer is error string
 * */
int get_slotvar_avb(AvbABOps *ab_ops, char *cmd, char *buffer, size_t size);

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

/* read a/b metadata to get curr slot
 * return slot suffix '_a'/'_b' or NULL */
char *select_slot(AvbABOps *ab_ops);

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

/* This is the fast version of avb_ab_flow(), this function will
 * not check another slot if one slot can pass the verify (or verify
 * fail is acceptable).
 */
AvbABFlowResult avb_ab_flow_fast(AvbABOps* ab_ops,
                                 const char* const* requested_partitions,
                                 AvbSlotVerifyFlags flags,
                                 AvbHashtreeErrorMode hashtree_error_mode,
                                 AvbSlotVerifyData** out_data);

/* This is for legacy i.mx6/7 which don't enable A/B but want to
 * verify boot/recovery with AVB */
AvbABFlowResult avb_single_flow(AvbABOps* ab_ops,
                                 const char* const* requested_partitions,
                                 AvbSlotVerifyFlags flags,
                                 AvbHashtreeErrorMode hashtree_error_mode,
                                 AvbSlotVerifyData** out_data);

/* Avb verify flow for dual bootloader, only the slot chosen by SPL will
 * be verified.
 */
AvbABFlowResult avb_flow_dual_uboot(AvbABOps* ab_ops,
                                    const char* const* requested_partitions,
                                    AvbSlotVerifyFlags flags,
                                    AvbHashtreeErrorMode hashtree_error_mode,
                                    AvbSlotVerifyData** out_data);

/* Program ATX perm_attr into RPMB partition */
int avb_atx_fuse_perm_attr(uint8_t *staged_buffer, uint32_t size);

#endif /* __FSL_AVB_H__ */
