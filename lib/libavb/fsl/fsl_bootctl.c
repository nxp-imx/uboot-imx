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

static unsigned int slotidx_from_suffix(char *suffix) {
	unsigned int slot = ~0;

	if (!strcmp(suffix, "_a"))
		slot = 0;
	else if (!strcmp(suffix, "_b"))
		slot = 1;

	return slot;
}

bool is_slotvar_avb(char *cmd) {

	assert(cmd != NULL);
	if (!strcmp_l1("has-slot:", cmd) ||
		!strcmp_l1("slot-successful:", cmd) ||
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

void get_slotvar_avb(AvbOps *ops, char *cmd, char *response, size_t chars_left) {

	AvbABData ab_data;
	AvbABSlotData *slot_data;
	unsigned int slot;

	assert(ops != NULL && cmd != NULL && response != NULL);

	if (!strcmp_l1("has-slot:", cmd)) {
		char *ptnname = NULL;
		ptnname = strchr(cmd, ':') + 1;
		if (!strcmp(ptnname, "system") || !strcmp(ptnname, "boot"))
			strncat(response, "yes", chars_left);
		else
			strncat(response, "no", chars_left);
		return;

	} else if (!strcmp_l1("slot-suffixes", cmd)) {
		strncat(response, "_a,_b", chars_left);
		return;
	}

	/* load ab meta */
	if (ops->read_ab_metadata == NULL ||
			ops->read_ab_metadata(ops, &ab_data) != AVB_IO_RESULT_OK) {
		strncat(response, "ab data read error", chars_left);
		return;
	}

	if (!strcmp_l1("current-slot", cmd)) {
		strncat(response, get_curr_slot(&ab_data), chars_left);

	} else if (!strcmp_l1("slot-successful:", cmd)) {
		char *suffix = strchr(cmd, ':') + 1;
		slot = slotidx_from_suffix(suffix);
		if (slot >= SLOT_NUM) {
			strncat(response, "no such slot", chars_left);
		} else {
			slot_data = &ab_data.slots[slot];
			bool succ = (slot_data->successful_boot != 0);
			strncat(response, succ ? "yes" : "no", chars_left);
		}

	} else if (!strcmp_l1("slot-unbootable:", cmd)) {
		char *suffix = strchr(cmd, ':') + 1;
		slot = slotidx_from_suffix(suffix);
		if (slot >= SLOT_NUM) {
			strncat(response, "no such slot", chars_left);
		} else {
			slot_data = &ab_data.slots[slot];
			bool bootable = slot_is_bootable(slot_data);
			strncat(response, bootable ? "no" : "yes", chars_left);
		}

	} else if (!strcmp_l1("slot-retry-count:", cmd)) {
		char *suffix = strchr(cmd, ':') + 1;
		slot = slotidx_from_suffix(suffix);
		if (slot >= SLOT_NUM)
			strncat(response, "no such slot", chars_left);
		else {
			slot_data = &ab_data.slots[slot];
			sprintf(response, "OKAY%d",
				slot_data->tries_remaining);
		}

	} else
		strncat(response, "no such slot command", chars_left);

	return;
}
