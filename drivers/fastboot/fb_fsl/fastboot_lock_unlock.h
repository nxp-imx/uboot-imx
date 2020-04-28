/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FASTBOOT_LOCK_UNLOCK_H
#define FASTBOOT_LOCK_UNLOCK_H

#define ALIGN_BYTES 64 /*armv7 cache line need 64 bytes aligned */

//#define FASTBOOT_LOCK_DEBUG
#ifdef CONFIG_FSL_CAAM_KB
#define FASTBOOT_ENCRYPT_LOCK
#endif

#ifdef FASTBOOT_LOCK_DEBUG
#define FB_DEBUG(format, ...) printf(format, ##__VA_ARGS__)
#else
#define FB_DEBUG(format, ...)
#endif

typedef enum {
	FASTBOOT_UNLOCK,
	FASTBOOT_LOCK,
	FASTBOOT_LOCK_ERROR,
	FASTBOOT_LOCK_NUM
}FbLockState;

typedef enum {
	FASTBOOT_UL_DISABLE,
	FASTBOOT_UL_ENABLE,
	FASTBOOT_UL_ERROR,
	FASTBOOT_UL_NUM
}FbLockEnableResult;

FbLockState fastboot_get_lock_stat(void);

int fastboot_set_lock_stat(FbLockState lock);

int fastboot_wipe_data_partition(void);
void fastboot_wipe_all(void);

FbLockEnableResult fastboot_lock_enable(void);
void set_fastboot_lock_disable(void);

int display_lock(FbLockState lock, int verify);

int display_unlock_warning(void);

bool valid_tos(void);
#endif
