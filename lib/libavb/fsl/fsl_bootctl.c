/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <stdlib.h>
#include <linux/string.h>

#include "fsl_avb.h"

/* as libavb's bootctl doesn't have the get_var support
 * we add the getvar support on our side ...*/
#define SLOT_NUM 2
static char *slot_suffix[SLOT_NUM] = {"_a", "_b"};

static int strcmp_l1(const char *s1, const char *s2) {
	if (!s1 || !s2)
		return -1;
	return strncmp(s1, s2, strlen(s1));
}

static bool slot_is_bootable(AvbABSlotData* slot) {
  return slot->priority > 0 &&
         (slot->successful_boot || (slot->tries_remaining > 0));
}

int slotidx_from_suffix(char *suffix) {
	int slot = -1;

	if (!strcmp(suffix, "_a") ||
			!strcmp(suffix, "a"))
		slot = 0;
	else if (!strcmp(suffix, "_b") ||
			!strcmp(suffix, "b"))
		slot = 1;

	return slot;
}

bool is_slotvar_avb(char *cmd) {

	assert(cmd != NULL);
	if (!strcmp_l1("has-slot:", cmd) ||
		!strcmp_l1("slot-successful:", cmd) ||
		!strcmp_l1("slot-count", cmd) ||
		!strcmp_l1("slot-suffixes", cmd) ||
		!strcmp_l1("current-slot", cmd) ||
		!strcmp_l1("slot-unbootable:", cmd) ||
		!strcmp_l1("slot-retry-count:", cmd))
		return true;
	return false;
}

static char *get_curr_slot(AvbABData *ab_data) {
	if (slot_is_bootable(&ab_data->slots[0]) &&
		slot_is_bootable(&ab_data->slots[1])) {
		if (ab_data->slots[1].priority > ab_data->slots[0].priority)
			return slot_suffix[1];
		else
			return slot_suffix[0];
	} else if (slot_is_bootable(&ab_data->slots[0]))
		return slot_suffix[0];
	else if (slot_is_bootable(&ab_data->slots[1]))
		return slot_suffix[1];
	else
		return "no valid slot";
}

int get_slotvar_avb(AvbOps *ops, char *cmd, char *buffer, size_t size) {

	AvbABData ab_data;
	AvbABSlotData *slot_data;
	int slot;

	assert(ops != NULL && cmd != NULL && buffer != NULL);

	char *str = cmd;
	if (!strcmp_l1("has-slot:", cmd)) {
		str += strlen("has-slot:");
		if (!strcmp(str, "system") || !strcmp(str, "boot"))
			strlcpy(buffer, "yes", size);
		else
			strlcpy(buffer, "no", size);
		return 0;

	} else if (!strcmp_l1("slot-suffixes", cmd)) {
		strlcpy(buffer, "_a,_b", size);
		return 0 ;

	} else if (!strcmp_l1("slot-count", cmd)) {
		strlcpy(buffer, "2", size);
		return 0 ;
	}

	/* load ab meta */
	if (ops->read_ab_metadata == NULL ||
			ops->read_ab_metadata(ops, &ab_data) != AVB_IO_RESULT_OK) {
		strlcpy(buffer, "ab data read error", size);
		return -1 ;
	}

	if (!strcmp_l1("current-slot", cmd)) {
		strlcpy(buffer, get_curr_slot(&ab_data), size);

	} else if (!strcmp_l1("slot-successful:", cmd)) {
		str += strlen("slot-successful:");
		slot = slotidx_from_suffix(str);
		if (slot < 0) {
			strlcpy(buffer, "no such slot", size);
			return -1;
		} else {
			slot_data = &ab_data.slots[slot];
			bool succ = (slot_data->successful_boot != 0);
			strlcpy(buffer, succ ? "yes" : "no", size);
		}

	} else if (!strcmp_l1("slot-unbootable:", cmd)) {
		str += strlen("slot-unbootable:");
		slot = slotidx_from_suffix(str);
		if (slot < 0) {
			strlcpy(buffer, "no such slot", size);
			return -1;
		} else {
			slot_data = &ab_data.slots[slot];
			bool bootable = slot_is_bootable(slot_data);
			strlcpy(buffer, bootable ? "no" : "yes", size);
		}

	} else if (!strcmp_l1("slot-retry-count:", cmd)) {
		str += strlen("slot-retry-count:");
		slot = slotidx_from_suffix(str);
		if (slot < 0) {
			strlcpy(buffer, "no such slot", size);
			return -1;
		}
		else {
			slot_data = &ab_data.slots[slot];
			char var[7];
			sprintf(var, "%d",
				slot_data->tries_remaining);
			strlcpy(buffer, var, size);
		}

	} else {
		strlcpy(buffer, "no such slot command", size);
		return -1;
	}

	return 0;
}
