// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2020 NXP
 */

typedef enum {
	VIRTUAL_AB_NONE = 0,
	VIRTUAL_AB_UNKNOWN,
	VIRTUAL_AB_SNAPSHOTTED,
	VIRTUAL_AB_MERGING,
	VIRTUAL_AB_CANCELLED,
} Virtual_AB_Status;

bool partition_is_protected_during_merge(char *part);
bool virtual_ab_update_is_merging(void);
bool virtual_ab_update_is_snapshoted(void);
bool virtual_ab_slot_match(void);
int virtual_ab_cancel_update(void);
