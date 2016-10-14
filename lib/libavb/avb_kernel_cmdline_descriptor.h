/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if !defined(AVB_INSIDE_LIBAVB_H) && !defined(AVB_COMPILATION)
#error "Never include this file directly, include libavb.h instead."
#endif

#ifndef AVB_KERNEL_CMDLINE_DESCRIPTOR_H_
#define AVB_KERNEL_CMDLINE_DESCRIPTOR_H_

#include "avb_descriptor.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A descriptor containing information to be appended to the kernel
 * command-line.
 *
 * Following this struct are |kernel_cmdline_len| bytes with the
 * kernel command-line (UTF-8 encoded).
 */
typedef struct AvbKernelCmdlineDescriptor {
  AvbDescriptor parent_descriptor;
  uint32_t kernel_cmdline_length;
} AVB_ATTR_PACKED AvbKernelCmdlineDescriptor;

/* Copies |src| to |dest| and validates, byte-swapping fields in the
 * process if needed. Returns true if valid, false if invalid.
 *
 * Data following the struct is not validated nor copied.
 */
bool avb_kernel_cmdline_descriptor_validate_and_byteswap(
    const AvbKernelCmdlineDescriptor* src,
    AvbKernelCmdlineDescriptor* dest) AVB_ATTR_WARN_UNUSED_RESULT;

#ifdef __cplusplus
}
#endif

#endif /* AVB_KERNEL_CMDLINE_DESCRIPTOR_H_ */
