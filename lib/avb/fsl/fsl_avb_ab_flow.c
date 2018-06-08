/*
 * Copyright 2018 NXP
 */

#include <common.h>
#include <fsl_avb.h>
#include <mmc.h>
#include <spl.h>
#include <part.h>
#include <image.h>
#include "utils.h"

#if defined(CONFIG_DUAL_BOOTLOADER) || !defined(CONFIG_SPL_BUILD)
static const char* slot_suffixes[2] = {"_a", "_b"};

/* This is a copy of slot_set_unbootable() form
 * external/avb/libavb_ab/avb_ab_flow.c.
 */
void fsl_slot_set_unbootable(AvbABSlotData* slot) {
	slot->priority = 0;
	slot->tries_remaining = 0;
	slot->successful_boot = 0;
}

/* Ensure all unbootable and/or illegal states are marked as the
 * canonical 'unbootable' state, e.g. priority=0, tries_remaining=0,
 * and successful_boot=0. This is a copy of slot_normalize from
 * external/avb/libavb_ab/avb_ab_flow.c.
 */
void fsl_slot_normalize(AvbABSlotData* slot) {
	if (slot->priority > 0) {
		if ((slot->tries_remaining == 0) && (!slot->successful_boot)) {
			/* We've exhausted all tries -> unbootable. */
			fsl_slot_set_unbootable(slot);
		}
		if ((slot->tries_remaining > 0) && (slot->successful_boot)) {
			/* Illegal state - avb_ab_mark_slot_successful() will clear
			* tries_remaining when setting successful_boot.
			*/
			fsl_slot_set_unbootable(slot);
		}
	} else {
		fsl_slot_set_unbootable(slot);
	}
}

/* This is a copy of slot_is_bootable() from
 * externel/avb/libavb_ab/avb_ab_flow.c.
 */
bool fsl_slot_is_bootable(AvbABSlotData* slot) {
	return (slot->priority > 0) &&
		(slot->successful_boot || (slot->tries_remaining > 0));
}
#endif /* CONFIG_DUAL_BOOTLOADER || !CONFIG_SPL_BUILD */

#if defined(CONFIG_DUAL_BOOTLOADER) && defined(CONFIG_SPL_BUILD)

#define FSL_AB_METADATA_MISC_PARTITION_OFFSET 2048
#define PARTITION_NAME_LEN 13
#define PARTITION_MISC "misc"
#define PARTITION_BOOTLOADER "bootloader"

/* Pre-declaration of h_spl_load_read(), see detail implementation in
 * common/spl/spl_mmc.c.
 */
ulong h_spl_load_read(struct spl_load_info *load, ulong sector,
		      ulong count, void *buf);

void fsl_avb_ab_data_update_crc_and_byteswap(const AvbABData* src,
					     AvbABData* dest) {
	memcpy(dest, src, sizeof(AvbABData));
	dest->crc32 = cpu_to_be32(
			avb_crc32((const uint8_t*)dest,
				  sizeof(AvbABData) - sizeof(uint32_t)));
}

void fsl_avb_ab_data_init(AvbABData* data) {
	memset(data, '\0', sizeof(AvbABData));
	memcpy(data->magic, AVB_AB_MAGIC, AVB_AB_MAGIC_LEN);
	data->version_major = AVB_AB_MAJOR_VERSION;
	data->version_minor = AVB_AB_MINOR_VERSION;
	data->slots[0].priority = AVB_AB_MAX_PRIORITY;
	data->slots[0].tries_remaining = AVB_AB_MAX_TRIES_REMAINING;
	data->slots[0].successful_boot = 0;
	data->slots[1].priority = AVB_AB_MAX_PRIORITY - 1;
	data->slots[1].tries_remaining = AVB_AB_MAX_TRIES_REMAINING;
	data->slots[1].successful_boot = 0;
}

bool fsl_avb_ab_data_verify_and_byteswap(const AvbABData* src,
					 AvbABData* dest) {
	/* Ensure magic is correct. */
	if (memcmp(src->magic, AVB_AB_MAGIC, AVB_AB_MAGIC_LEN) != 0) {
		printf("Magic is incorrect.\n");
		return false;
	}

	memcpy(dest, src, sizeof(AvbABData));
	dest->crc32 = be32_to_cpu(dest->crc32);

	/* Ensure we don't attempt to access any fields if the major version
	* is not supported.
	*/
	if (dest->version_major > AVB_AB_MAJOR_VERSION) {
		printf("No support for given major version.\n");
		return false;
	}

	/* Fail if CRC32 doesn't match. */
	if (dest->crc32 !=
		avb_crc32((const uint8_t*)dest, sizeof(AvbABData) - sizeof(uint32_t))) {
		printf("CRC32 does not match.\n");
		return false;
	}

	return true;
}

/* Writes A/B metadata to disk only if it has changed.
 */
int fsl_save_metadata_if_changed_dual_uboot(struct blk_desc *dev_desc,
					    AvbABData* ab_data,
					    AvbABData* ab_data_orig) {
	AvbABData serialized;
	size_t num_bytes;
	disk_partition_t info;

	/* Save metadata if changed. */
	if (memcmp(ab_data, ab_data_orig, sizeof(AvbABData)) != 0) {
		/* Get misc partition info */
		if (part_get_info_by_name(dev_desc, PARTITION_MISC, &info)) {
			printf("Can't get partition info of partition: misc\n");
			return -1;
		}

		/* Writing A/B metadata to disk. */
		fsl_avb_ab_data_update_crc_and_byteswap(ab_data, &serialized);
		if (write_to_partition_in_bytes(dev_desc, &info,
				FSL_AB_METADATA_MISC_PARTITION_OFFSET,
				sizeof(AvbABData),
				(void *)&serialized, &num_bytes) ||
				(num_bytes != sizeof(AvbABData))) {
			printf("Error--write metadata fail!\n");
			return -1;
		}
	}
	return 0;
}

/* Load metadate from misc partition.
 */
int fsl_load_metadata_dual_uboot(struct blk_desc *dev_desc,
				 AvbABData* ab_data,
				 AvbABData* ab_data_orig) {
	disk_partition_t info;
	AvbABData serialized;
	size_t num_bytes;

	if (part_get_info_by_name(dev_desc, PARTITION_MISC, &info)) {
		printf("Can't get partition info of partition: misc\n");
		return -1;
	} else {
		read_from_partition_in_bytes(
		    dev_desc, &info, FSL_AB_METADATA_MISC_PARTITION_OFFSET,
		    sizeof(AvbABData),
				(void *)ab_data, &num_bytes );
		if (num_bytes != sizeof(AvbABData)) {
			printf("Error--read metadata fail!\n");
			return -1;
		} else {
			if (!fsl_avb_ab_data_verify_and_byteswap(ab_data, &serialized)) {
				printf("Error validating A/B metadata from disk.\n");
				printf("Resetting and writing new A/B metadata to disk.\n");
				fsl_avb_ab_data_init(ab_data);
				fsl_avb_ab_data_update_crc_and_byteswap(ab_data, &serialized);
				num_bytes = 0;
				if (write_to_partition_in_bytes(
					dev_desc, &info,
					FSL_AB_METADATA_MISC_PARTITION_OFFSET,
					sizeof(AvbABData),
					(void *)&serialized, &num_bytes) ||
					(num_bytes != sizeof(AvbABData))) {
					printf("Error--write metadata fail!\n");
					return -1;
				} else
					return 0;
			} else {
				memcpy(ab_data_orig, ab_data, sizeof(AvbABData));
				/* Ensure data is normalized, e.g. illegal states will be marked as
				 * unbootable and all unbootable states are represented with
				 * (priority=0, tries_remaining=0, successful_boot=0).
				 */
				fsl_slot_normalize(&ab_data->slots[0]);
				fsl_slot_normalize(&ab_data->slots[1]);
				return 0;
			}
		}
	}
}

int mmc_load_image_raw_sector_dual_uboot(
		struct spl_image_info *spl_image, struct mmc *mmc)
{
	unsigned long count;
	disk_partition_t info;
	int ret = 0, n = 0;
	char partition_name[PARTITION_NAME_LEN];
	struct blk_desc *dev_desc;
	struct image_header *header;
	AvbABData ab_data, ab_data_orig;
	size_t slot_index_to_boot, target_slot;

	/* Check if gpt is valid */
	dev_desc = mmc_get_blk_desc(mmc);
	if (dev_desc) {
		if (part_get_info(dev_desc, 1, &info)) {
			printf("GPT is invalid, please flash correct GPT!\n");
			ret = -EIO;
			goto end;
		}
	} else {
		printf("Get block desc fail!\n");
		ret = -EIO;
		goto end;
	}

	/* Load AB metadata from misc partition */
	if (fsl_load_metadata_dual_uboot(dev_desc, &ab_data,
					 &ab_data_orig)) {
		ret = -1;
		goto end;
	}

	slot_index_to_boot = 2;  // Means not 0 or 1
	target_slot =
	    (ab_data.slots[1].priority > ab_data.slots[0].priority) ? 1 : 0;

	for (n = 0; n < 2; n++) {
		if (!fsl_slot_is_bootable(&ab_data.slots[target_slot])) {
			target_slot = (target_slot == 1 ? 0 : 1);
			continue;
		}
		/* Choose slot to load. */
		snprintf(partition_name, PARTITION_NAME_LEN,
			 PARTITION_BOOTLOADER"%s",
			 slot_suffixes[target_slot]);

		/* Read part info from gpt */
		if (part_get_info_by_name(dev_desc, partition_name, &info)) {
			printf("Can't get partition info of partition bootloader%s\n",
			       slot_suffixes[target_slot]);
		} else {
			header = (struct image_header *)(CONFIG_SYS_TEXT_BASE -
				 sizeof(struct image_header));

			/* read image header to find the image size & load address */
			count = blk_dread(dev_desc, info.start, 1, header);
			if (count == 0) {
				ret = -EIO;
				goto end;
			}

			if (IS_ENABLED(CONFIG_SPL_LOAD_FIT) &&
				       image_get_magic(header) == FDT_MAGIC) {
				struct spl_load_info load;

				debug("Found FIT\n");
				load.dev = mmc;
				load.priv = NULL;
				load.filename = NULL;
				load.bl_len = mmc->read_bl_len;
				load.read = h_spl_load_read;
				ret = spl_load_simple_fit(spl_image, &load,
							  info.start, header);
			} else {
				ret = -1;
			}
		}

		/* Set current slot to unbootable if load/verify fail. */
		if (ret != 0) {
			printf("Load or verify bootloader%s fail, setting unbootable..\n",
			       slot_suffixes[target_slot]);
			fsl_slot_set_unbootable(&ab_data.slots[target_slot]);
			/* Switch to another slot. */
			target_slot = (target_slot == 1 ? 0 : 1);
		} else {
			slot_index_to_boot = target_slot;
			n = 2;
		}
	}

	if (slot_index_to_boot == 2) {
		/* No bootable slots! */
		printf("No bootable slots found.\n");
		ret = -1;
		goto end;
	} else if (!ab_data.slots[slot_index_to_boot].successful_boot &&
		   (ab_data.slots[slot_index_to_boot].tries_remaining > 0)) {
		ab_data.slots[slot_index_to_boot].tries_remaining -= 1;
	}
	printf("Booting from bootloader%s...\n", slot_suffixes[slot_index_to_boot]);

end:
	/* Save metadata if changed. */
	if (fsl_save_metadata_if_changed_dual_uboot(dev_desc, &ab_data, &ab_data_orig)) {
		ret = -1;
	}

	if (ret)
		return -1;
	else
		return 0;
}

/* For normal build */
#elif !defined(CONFIG_SPL_BUILD)

/* Writes A/B metadata to disk only if it has been changed.
 */
static AvbIOResult fsl_save_metadata_if_changed(AvbABOps* ab_ops,
						AvbABData* ab_data,
						AvbABData* ab_data_orig) {
	if (avb_safe_memcmp(ab_data, ab_data_orig, sizeof(AvbABData)) != 0) {
		avb_debug("Writing A/B metadata to disk.\n");
		return ab_ops->write_ab_metadata(ab_ops, ab_data);
	}
	return AVB_IO_RESULT_OK;
}

/* Helper function to load metadata - returns AVB_IO_RESULT_OK on
 * success, error code otherwise. This is a copy of load_metadata()
 * from /lib/avb/libavb_ab/avb_ab_flow.c.
 */
static AvbIOResult fsl_load_metadata(AvbABOps* ab_ops,
				     AvbABData* ab_data,
				     AvbABData* ab_data_orig) {
	AvbIOResult io_ret;

	io_ret = ab_ops->read_ab_metadata(ab_ops, ab_data);
	if (io_ret != AVB_IO_RESULT_OK) {
		avb_error("I/O error while loading A/B metadata.\n");
		return io_ret;
	}
	*ab_data_orig = *ab_data;

	/* Ensure data is normalized, e.g. illegal states will be marked as
	 * unbootable and all unbootable states are represented with
	 * (priority=0, tries_remaining=0, successful_boot=0).
	 */
	fsl_slot_normalize(&ab_data->slots[0]);
	fsl_slot_normalize(&ab_data->slots[1]);
	return AVB_IO_RESULT_OK;
}

#ifdef CONFIG_DUAL_BOOTLOADER
AvbABFlowResult avb_flow_dual_uboot(AvbABOps* ab_ops,
				    const char* const* requested_partitions,
				    AvbSlotVerifyFlags flags,
				    AvbHashtreeErrorMode hashtree_error_mode,
				    AvbSlotVerifyData** out_data) {
	AvbOps* ops = ab_ops->ops;
	AvbSlotVerifyData* slot_data = NULL;
	AvbSlotVerifyData* data = NULL;
	AvbABFlowResult ret;
	AvbABData ab_data, ab_data_orig;
	AvbIOResult io_ret;
	bool saw_and_allowed_verification_error = false;
	AvbSlotVerifyResult verify_result;
	bool set_slot_unbootable = false;
	int target_slot;

	io_ret = fsl_load_metadata(ab_ops, &ab_data, &ab_data_orig);
	if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
		ret = AVB_AB_FLOW_RESULT_ERROR_OOM;
		goto out;
	} else if (io_ret != AVB_IO_RESULT_OK) {
		ret = AVB_AB_FLOW_RESULT_ERROR_IO;
		goto out;
	}

	/* Choose the target slot, it should be the same with the one in SPL. */
	target_slot = get_curr_slot(&ab_data);
	if (target_slot == -1) {
		ret = AVB_AB_FLOW_RESULT_ERROR_NO_BOOTABLE_SLOTS;
		printf("No bootable slot found!\n");
		goto out;
	}

	printf("Verifying slot %s ...\n", slot_suffixes[target_slot]);
	verify_result = avb_slot_verify(ops,
					requested_partitions,
					slot_suffixes[target_slot],
					flags,
					hashtree_error_mode,
					&slot_data);

	switch (verify_result) {
		case AVB_SLOT_VERIFY_RESULT_ERROR_OOM:
			ret = AVB_AB_FLOW_RESULT_ERROR_OOM;
			goto out;

		case AVB_SLOT_VERIFY_RESULT_ERROR_IO:
			ret = AVB_AB_FLOW_RESULT_ERROR_IO;
			goto out;

		case AVB_SLOT_VERIFY_RESULT_OK:
			ret = AVB_AB_FLOW_RESULT_OK;
			break;

		case AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA:
		case AVB_SLOT_VERIFY_RESULT_ERROR_UNSUPPORTED_VERSION:
			/* Even with AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR
			 * these mean game over.
			 */
			set_slot_unbootable = true;
			break;

		case AVB_SLOT_VERIFY_RESULT_ERROR_VERIFICATION:
		case AVB_SLOT_VERIFY_RESULT_ERROR_ROLLBACK_INDEX:
		case AVB_SLOT_VERIFY_RESULT_ERROR_PUBLIC_KEY_REJECTED:
			if (flags & AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR) {
				/* Do nothing since we allow this. */
				avb_debugv("Allowing slot ",
					   slot_suffixes[target_slot],
					   " which verified "
					   "with result ",
					   avb_slot_verify_result_to_string(verify_result),
					   " because "
					   "AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR "
					   "is set.\n",
					   NULL);
				saw_and_allowed_verification_error =
					 true;
			} else {
				set_slot_unbootable = true;
			}
			break;

		case AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_ARGUMENT:
			ret = AVB_AB_FLOW_RESULT_ERROR_INVALID_ARGUMENT;
			goto out;
			/* Do not add a 'default:' case here because
			 * of -Wswitch.
			 */
	}

	if (set_slot_unbootable) {
		avb_errorv("Error verifying slot ",
			   slot_suffixes[target_slot],
			   " with result ",
			   avb_slot_verify_result_to_string(verify_result),
			   " - setting unbootable.\n",
			   NULL);
		fsl_slot_set_unbootable(&ab_data.slots[target_slot]);

		/* Only the slot chosen by SPL will be verified here so we
		 * return AVB_AB_FLOW_RESULT_ERROR_NO_BOOTABLE_SLOTS if the
		 * slot should be set unbootable.
		 */
		ret = AVB_AB_FLOW_RESULT_ERROR_NO_BOOTABLE_SLOTS;
		goto out;
	}

	/* Finally, select this slot. */
	avb_assert(slot_data != NULL);
	data = slot_data;
	slot_data = NULL;
	if (saw_and_allowed_verification_error) {
		avb_assert(
			flags & AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR);
		ret = AVB_AB_FLOW_RESULT_OK_WITH_VERIFICATION_ERROR;
	} else {
		ret = AVB_AB_FLOW_RESULT_OK;
	}

out:
	io_ret = fsl_save_metadata_if_changed(ab_ops, &ab_data, &ab_data_orig);
	if (io_ret != AVB_IO_RESULT_OK) {
		if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
			ret = AVB_AB_FLOW_RESULT_ERROR_OOM;
		} else {
			ret = AVB_AB_FLOW_RESULT_ERROR_IO;
		}
		if (data != NULL) {
			avb_slot_verify_data_free(data);
			data = NULL;
		}
	}

	if (slot_data != NULL)
		avb_slot_verify_data_free(slot_data);

	if (out_data != NULL) {
		*out_data = data;
	} else {
		if (data != NULL) {
			avb_slot_verify_data_free(data);
		}
	}

	return ret;
}
#else /* CONFIG_DUAL_BOOTLOADER */
/* For legacy i.mx6/7, we won't enable A/B due to the limitation of
 * storage capacity, but we still want to verify boot/recovery with
 * AVB. */
AvbABFlowResult avb_single_flow(AvbABOps* ab_ops,
                            const char* const* requested_partitions,
                            AvbSlotVerifyFlags flags,
                            AvbHashtreeErrorMode hashtree_error_mode,
                            AvbSlotVerifyData** out_data) {
  AvbOps* ops = ab_ops->ops;
  AvbSlotVerifyData* slot_data = NULL;
  AvbSlotVerifyData* data = NULL;
  AvbABFlowResult ret;
  bool saw_and_allowed_verification_error = false;

  /* Validate boot/recovery. */
  AvbSlotVerifyResult verify_result;

  verify_result = avb_slot_verify(ops,
                    requested_partitions,
                    "",
                    flags,
                    hashtree_error_mode,
                    &slot_data);
  switch (verify_result) {
      case AVB_SLOT_VERIFY_RESULT_ERROR_OOM:
        ret = AVB_AB_FLOW_RESULT_ERROR_OOM;
        goto out;

      case AVB_SLOT_VERIFY_RESULT_ERROR_IO:
        ret = AVB_AB_FLOW_RESULT_ERROR_IO;
        goto out;

      case AVB_SLOT_VERIFY_RESULT_OK:
        break;

      case AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA:
      case AVB_SLOT_VERIFY_RESULT_ERROR_UNSUPPORTED_VERSION:
          /* Even with AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR
           * these mean game over.
           */
        ret = AVB_AB_FLOW_RESULT_ERROR_NO_BOOTABLE_SLOTS;
        goto out;

      /* explicit fallthrough. */
      case AVB_SLOT_VERIFY_RESULT_ERROR_VERIFICATION:
      case AVB_SLOT_VERIFY_RESULT_ERROR_ROLLBACK_INDEX:
      case AVB_SLOT_VERIFY_RESULT_ERROR_PUBLIC_KEY_REJECTED:
        if (flags & AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR) {
          /* Do nothing since we allow this. */
          avb_debugv("Allowing slot ",
                     slot_suffixes[n],
                     " which verified "
                     "with result ",
                     avb_slot_verify_result_to_string(verify_result),
                     " because "
                     "AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR "
                     "is set.\n",
                     NULL);
          saw_and_allowed_verification_error = true;
        } else {
            ret = AVB_AB_FLOW_RESULT_ERROR_NO_BOOTABLE_SLOTS;
            goto out;
        }
        break;

      case AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_ARGUMENT:
        ret = AVB_AB_FLOW_RESULT_ERROR_INVALID_ARGUMENT;
        goto out;
        /* Do not add a 'default:' case here because of -Wswitch. */
      }

  avb_assert(slot_data != NULL);
  data = slot_data;
  slot_data = NULL;
  if (saw_and_allowed_verification_error) {
    avb_assert(flags & AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR);
    ret = AVB_AB_FLOW_RESULT_OK_WITH_VERIFICATION_ERROR;
  } else {
    ret = AVB_AB_FLOW_RESULT_OK;
  }

out:
  if (slot_data != NULL) {
    avb_slot_verify_data_free(slot_data);
  }

  if (out_data != NULL) {
    *out_data = data;
  } else {
    if (data != NULL) {
      avb_slot_verify_data_free(data);
    }
  }

  return ret;
}

AvbABFlowResult avb_ab_flow_fast(AvbABOps* ab_ops,
				 const char* const* requested_partitions,
				 AvbSlotVerifyFlags flags,
				 AvbHashtreeErrorMode hashtree_error_mode,
				 AvbSlotVerifyData** out_data) {
	AvbOps* ops = ab_ops->ops;
	AvbSlotVerifyData* slot_data[2] = {NULL, NULL};
	AvbSlotVerifyData* data = NULL;
	AvbABFlowResult ret;
	AvbABData ab_data, ab_data_orig;
	size_t slot_index_to_boot, n;
	AvbIOResult io_ret;
	bool saw_and_allowed_verification_error = false;
	size_t target_slot;
	AvbSlotVerifyResult verify_result;
	bool set_slot_unbootable = false;

	io_ret = fsl_load_metadata(ab_ops, &ab_data, &ab_data_orig);
	if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
		ret = AVB_AB_FLOW_RESULT_ERROR_OOM;
		goto out;
	} else if (io_ret != AVB_IO_RESULT_OK) {
		ret = AVB_AB_FLOW_RESULT_ERROR_IO;
		goto out;
	}

	slot_index_to_boot = 2;  // Means not 0 or 1
	target_slot =
	    (ab_data.slots[1].priority > ab_data.slots[0].priority) ? 1 : 0;

	for (n = 0; n < 2; n++) {
		if (!fsl_slot_is_bootable(&ab_data.slots[target_slot])) {
			target_slot = (target_slot == 1 ? 0 : 1);
			continue;
		}
		verify_result = avb_slot_verify(ops,
						requested_partitions,
						slot_suffixes[target_slot],
						flags,
						hashtree_error_mode,
						&slot_data[target_slot]);
		switch (verify_result) {
			case AVB_SLOT_VERIFY_RESULT_ERROR_OOM:
				ret = AVB_AB_FLOW_RESULT_ERROR_OOM;
				goto out;

			case AVB_SLOT_VERIFY_RESULT_ERROR_IO:
				ret = AVB_AB_FLOW_RESULT_ERROR_IO;
				goto out;

			case AVB_SLOT_VERIFY_RESULT_OK:
				slot_index_to_boot = target_slot;
				n = 2;
				break;

			case AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA:
			case AVB_SLOT_VERIFY_RESULT_ERROR_UNSUPPORTED_VERSION:
				/* Even with AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR
				 * these mean game over.
				 */
				set_slot_unbootable = true;
				break;

			/* explicit fallthrough. */
			case AVB_SLOT_VERIFY_RESULT_ERROR_VERIFICATION:
			case AVB_SLOT_VERIFY_RESULT_ERROR_ROLLBACK_INDEX:
			case AVB_SLOT_VERIFY_RESULT_ERROR_PUBLIC_KEY_REJECTED:
				if (flags & AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR) {
					/* Do nothing since we allow this. */
					avb_debugv("Allowing slot ",
						   slot_suffixes[target_slot],
						   " which verified "
						   "with result ",
						   avb_slot_verify_result_to_string(verify_result),
						   " because "
						   "AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR "
						   "is set.\n",
						   NULL);
					saw_and_allowed_verification_error =
						 true;
					slot_index_to_boot = target_slot;
					n = 2;
				} else {
					set_slot_unbootable = true;
				}
				break;

			case AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_ARGUMENT:
				ret = AVB_AB_FLOW_RESULT_ERROR_INVALID_ARGUMENT;
				goto out;
				/* Do not add a 'default:' case here because
				 * of -Wswitch.
				 */
		}

		if (set_slot_unbootable) {
			avb_errorv("Error verifying slot ",
				   slot_suffixes[target_slot],
				   " with result ",
				   avb_slot_verify_result_to_string(verify_result),
				   " - setting unbootable.\n",
				   NULL);
			fsl_slot_set_unbootable(&ab_data.slots[target_slot]);
			set_slot_unbootable = false;
		}
		/* switch to another slot */
		target_slot = (target_slot == 1 ? 0 : 1);
	}

	if (slot_index_to_boot == 2) {
		/* No bootable slots! */
		avb_error("No bootable slots found.\n");
		ret = AVB_AB_FLOW_RESULT_ERROR_NO_BOOTABLE_SLOTS;
		goto out;
	}

	/* Update stored rollback index such that the stored rollback index
	* is the largest value supporting all currently bootable slots. Do
	* this for every rollback index location.
	*/
	for (n = 0; n < AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS; n++) {
		uint64_t rollback_index_value = 0;

		if ((slot_data[0] != NULL) && (slot_data[1] != NULL)) {
			uint64_t a_rollback_index =
				 slot_data[0]->rollback_indexes[n];
			uint64_t b_rollback_index =
				 slot_data[1]->rollback_indexes[n];
			rollback_index_value =
				(a_rollback_index < b_rollback_index ?
					 a_rollback_index : b_rollback_index);
		} else if (slot_data[0] != NULL) {
			rollback_index_value =
				 slot_data[0]->rollback_indexes[n];
		} else if (slot_data[1] != NULL) {
			rollback_index_value =
				 slot_data[1]->rollback_indexes[n];
		}

		if (rollback_index_value != 0) {
			uint64_t current_rollback_index_value;
			io_ret = ops->read_rollback_index(
					ops, n, &current_rollback_index_value);
			if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
				ret = AVB_AB_FLOW_RESULT_ERROR_OOM;
				goto out;
			} else if (io_ret != AVB_IO_RESULT_OK) {
				avb_error("Error getting rollback index for slot.\n");
				ret = AVB_AB_FLOW_RESULT_ERROR_IO;
				goto out;
			}
			if (current_rollback_index_value != rollback_index_value) {
				io_ret = ops->write_rollback_index(
						ops, n, rollback_index_value);
				if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
					ret = AVB_AB_FLOW_RESULT_ERROR_OOM;
					goto out;
				} else if (io_ret != AVB_IO_RESULT_OK) {
					avb_error("Error setting stored rollback index.\n");
					ret = AVB_AB_FLOW_RESULT_ERROR_IO;
					goto out;
				}
			}
		}
	}

	/* Finally, select this slot. */
	avb_assert(slot_data[slot_index_to_boot] != NULL);
	data = slot_data[slot_index_to_boot];
	slot_data[slot_index_to_boot] = NULL;
	if (saw_and_allowed_verification_error) {
		avb_assert(
			flags & AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR);
		ret = AVB_AB_FLOW_RESULT_OK_WITH_VERIFICATION_ERROR;
	} else {
		ret = AVB_AB_FLOW_RESULT_OK;
	}

	/* ... and decrement tries remaining, if applicable. */
	if (!ab_data.slots[slot_index_to_boot].successful_boot &&
	    (ab_data.slots[slot_index_to_boot].tries_remaining > 0)) {
		ab_data.slots[slot_index_to_boot].tries_remaining -= 1;
	}

out:
	io_ret = fsl_save_metadata_if_changed(ab_ops, &ab_data, &ab_data_orig);
	if (io_ret != AVB_IO_RESULT_OK) {
		if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
			ret = AVB_AB_FLOW_RESULT_ERROR_OOM;
		} else {
			ret = AVB_AB_FLOW_RESULT_ERROR_IO;
		}
		if (data != NULL) {
			avb_slot_verify_data_free(data);
			data = NULL;
		}
	}

	for (n = 0; n < 2; n++) {
		if (slot_data[n] != NULL) {
			avb_slot_verify_data_free(slot_data[n]);
		}
	}

	if (out_data != NULL) {
		*out_data = data;
	} else {
		if (data != NULL) {
			avb_slot_verify_data_free(data);
		}
	}

	return ret;
}
#endif /* CONFIG_DUAL_BOOTLOADER */

#endif /* CONFIG_DUAL_BOOTLOADER && CONFIG_SPL_BUILD */
