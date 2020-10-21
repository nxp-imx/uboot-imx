// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2020 NXP
 */

#include "android_bootloader_message.h"
#include "../lib/avb/fsl/utils.h"
#include "fb_fsl_virtual_ab.h"
#include "fsl_avb.h"
#include "fb_fsl.h"

static int read_virtual_ab_message(misc_virtual_ab_message *message)
{
	size_t num_bytes;
	int source_slot;

	if (fsl_read_from_partition_multi(NULL, FASTBOOT_PARTITION_MISC,
					SYSTEM_SPACE_SIZE_IN_MISC,
					sizeof(misc_virtual_ab_message),
					(void *)message, &num_bytes) || (num_bytes != sizeof(misc_virtual_ab_message))) {
		printf("Error reading virtual AB message from misc!\n");
		return -1;
	}

	if ((message->magic != MISC_VIRTUAL_AB_MAGIC_HEADER) ||
		(message->version != MISC_VIRTUAL_AB_MESSAGE_VERSION)) {
		printf("Invalid virtual AB status, resetting...\n");
		message->version = MISC_VIRTUAL_AB_MESSAGE_VERSION;
		message->magic = MISC_VIRTUAL_AB_MAGIC_HEADER;
		message->merge_status = VIRTUAL_AB_NONE;

		/* Reset the source slot as the current slot */
		source_slot = current_slot();
		if (source_slot != -1)
			message->source_slot = source_slot;
		else
			return -1;

		if (fsl_write_to_partition(NULL, FASTBOOT_PARTITION_MISC,
						SYSTEM_SPACE_SIZE_IN_MISC,
						sizeof(misc_virtual_ab_message),
						(void *)message)) {
			printf("Error writing virtual AB message to misc!\n");
			return -1;
		}
	}

	return 0;
}

/* Flash or erase shall be prohibited to "misc", "userdata" and "metadata" partitions
 * when the virtual AB status is VIRTUAL_AB_MERGING or VIRTUAL_AB_SNAPSHOTTED.
 * */
bool partition_is_protected_during_merge(char *part)
{
	if ((!strncmp(part, "misc", sizeof("misc")) ||
		!strncmp(part, "userdata", sizeof("userdata")) ||
		!strncmp(part, "metadata", sizeof("metadata"))) &&
		(virtual_ab_update_is_merging() ||
		 (virtual_ab_update_is_snapshoted() && !virtual_ab_slot_match())))
		return true;
	else
		return false;
}

bool virtual_ab_update_is_merging(void)
{
	misc_virtual_ab_message message;
	read_virtual_ab_message(&message);
	if (message.merge_status == VIRTUAL_AB_MERGING)
		return true;
	else
		return false;
}

bool virtual_ab_update_is_snapshoted(void)
{
	misc_virtual_ab_message message;

	read_virtual_ab_message(&message);
	if (message.merge_status == VIRTUAL_AB_SNAPSHOTTED)
		return true;
	else
		return false;
}

bool virtual_ab_slot_match(void)
{
	misc_virtual_ab_message message;
	read_virtual_ab_message(&message);

	if (message.source_slot == current_slot())
		return true;
	else
		return false;
}

int virtual_ab_cancel_update(void)
{
	misc_virtual_ab_message message;

	read_virtual_ab_message(&message);
	message.merge_status = VIRTUAL_AB_CANCELLED;

	if (fsl_write_to_partition(NULL, FASTBOOT_PARTITION_MISC,
					SYSTEM_SPACE_SIZE_IN_MISC,
					sizeof(misc_virtual_ab_message),
					(void *)&message)) {
		printf("Error writing virtual AB message to misc!\n");
		return -1;
	}

	return 0;
}
