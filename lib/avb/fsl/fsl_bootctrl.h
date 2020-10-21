/*
 * Copyright 2020 NXP
 *
 */

#ifndef __FSL_BOOTCTRL_H__
#define __FSL_BOOTCTRL_H__

#include "android_bootloader_message.h"

typedef enum {
	AVB_AB_FLOW_RESULT_OK,
	AVB_AB_FLOW_RESULT_OK_WITH_VERIFICATION_ERROR,
	AVB_AB_FLOW_RESULT_ERROR_OOM,
	AVB_AB_FLOW_RESULT_ERROR_IO,
	AVB_AB_FLOW_RESULT_ERROR_NO_BOOTABLE_SLOTS,
	AVB_AB_FLOW_RESULT_ERROR_INVALID_ARGUMENT
} AvbABFlowResult;

/* High-level operations/functions/methods for A/B that are platform
 * dependent.
 */
struct AvbABOps;
typedef struct AvbABOps AvbABOps;

struct AvbABOps {
	/* Operations from libavb. */
	AvbOps* ops;

	/* Reads A/B metadata from persistent storage. Returned data is
	 * properly byteswapped. Returns AVB_IO_RESULT_OK on success, error
	 * code otherwise.
	 *
	 * If the data read is invalid (e.g. wrong magic or CRC checksum
	 * failure), the metadata shoule be reset using fsl_avb_ab_data_init()
	 * and then written to persistent storage.
	 *
	 * Implementations will typically want to use fsl_avb_ab_data_read()
	 * here to use the 'misc' partition for persistent storage.
	 */
	AvbIOResult (*read_ab_metadata)(AvbABOps* ab_ops, struct bootloader_control* data);

	/* Writes A/B metadata to persistent storage. This will byteswap and
	 * update the CRC as needed. Returns AVB_IO_RESULT_OK on success,
	 * error code otherwise.
	 *
	 * Implementations will typically want to use fsl_avb_ab_data_write()
	 * here to use the 'misc' partition for persistent storage.
	 */
	AvbIOResult (*write_ab_metadata)(AvbABOps* ab_ops,
					 const struct bootloader_control* data);
};

/* Copies |src| to |dest|, byte-swapping fields in the
 * process. Returns false if the data is invalid (e.g. wrong magic,
 * wrong CRC32 etc.), true otherwise.
 */
bool fsl_avb_ab_data_verify_and_byteswap(const struct bootloader_control* src,
					 struct bootloader_control* dest);

/* Copies |src| to |dest|, byte-swapping fields in the process. Also
 * updates the |crc32| field in |dest|.
 */
void fsl_avb_ab_data_update_crc_and_byteswap(const struct bootloader_control* src,
					     struct bootloader_control* dest);

/* Initializes |data| such that it has two slots and both slots have
 * maximum tries remaining. The CRC is not set.
 */
void fsl_avb_ab_data_init(struct bootloader_control* data);

/* Reads A/B metadata from the 'misc' partition using |ops|. Returned
 * data is properly byteswapped. Returns AVB_IO_RESULT_OK on
 * success, error code otherwise.
 *
 * If the data read from disk is invalid (e.g. wrong magic or CRC
 * checksum failure), the metadata will be reset using
 * fsl_avb_ab_data_init() and then written to disk.
 */
AvbIOResult fsl_avb_ab_data_read(AvbABOps* ab_ops, struct bootloader_control* data);

/* Writes A/B metadata to the 'misc' partition using |ops|. This will
 * byteswap and update the CRC as needed. Returns AVB_IO_RESULT_OK on
 * success, error code otherwise.
 */
AvbIOResult fsl_avb_ab_data_write(AvbABOps* ab_ops, const struct bootloader_control* data);

/* True if the given slot is active, false otherwise.
 * */
bool fsl_slot_is_bootable(struct slot_metadata* slot);

/* Mark one slot as active. */
AvbIOResult fsl_avb_ab_mark_slot_active(AvbABOps* ab_ops,
					unsigned int slot_number);

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
int get_curr_slot(struct bootloader_control* ab_data);

/* Get current bootable slot without passing the "bootloader_control" struct.
 * return 0 for the first slot
 * return 1 for the second slot
 * return -1 for not supported slot
 * */
int current_slot(void);

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

/* read a/b metadata to get curr slot
 * return slot suffix '_a'/'_b' or NULL */
char *select_slot(AvbABOps *ab_ops);

int get_slotvar_avb(AvbABOps *ab_ops, char *cmd, char *buffer, size_t size);

#endif /* __FSL_BOOTCTRL_H__ */
