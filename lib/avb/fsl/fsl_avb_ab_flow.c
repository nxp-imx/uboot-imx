/*
 * Copyright 2018 NXP
 */

#include <common.h>
#include <fsl_avb.h>

static const char* slot_suffixes[2] = {"_a", "_b"};

/* This is a copy of slot_set_unbootable() form
 * lib/avb/libavb_ab/avb_ab_flow.c.
 */
static void fsl_slot_set_unbootable(AvbABSlotData* slot) {
	slot->priority = 0;
	slot->tries_remaining = 0;
	slot->successful_boot = 0;
}

/* Ensure all unbootable and/or illegal states are marked as the
 * canonical 'unbootable' state, e.g. priority=0, tries_remaining=0,
 * and successful_boot=0. This is a copy of fsl_slot_normalize from
 * lib/avb/libavb_ab/avb_ab_flow.c.
 */
static void fsl_slot_normalize(AvbABSlotData* slot) {
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

/* Writes A/B metadata to disk only if it has changed - returns
 * AVB_IO_RESULT_OK on success, error code otherwise. This is a
 * copy of save_metadata_if_changed form lib/avb/libavb_ab/avb_ab_flow.c.
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

/* This is a copy of slot_is_bootable() from
 * lib/avb/libavb_ab/avb_ab_flow.c.
 */
static bool fsl_slot_is_bootable(AvbABSlotData* slot) {
	return (slot->priority > 0) &&
		(slot->successful_boot || (slot->tries_remaining > 0));
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
