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

#include <trusty/sysdeps.h>

#include <common.h>
#include <linux/string.h>
#include <malloc.h>

extern int trusty_encode_page_info(struct ns_mem_page_info *page_info,
                                   void *vaddr);

void trusty_lock(struct trusty_dev *dev)
{
}
void trusty_unlock(struct trusty_dev *dev)
{
}

void trusty_local_irq_disable(unsigned long *state)
{
    disable_interrupts();
}

void trusty_local_irq_restore(unsigned long *state)
{
    enable_interrupts();
}

void trusty_idle(struct trusty_dev *dev)
{
    wfi();
}

void trusty_abort(void)
{
    do_reset(NULL, 0, 0, NULL);
    __builtin_unreachable();
}

void trusty_printf(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
}

void *trusty_memcpy(void *dest, const void *src, size_t n)
{
    return memcpy(dest, src, n);
}

void *trusty_memset(void *dest, const int c, size_t n)
{
    return memset(dest, c, n);
}

char *trusty_strcpy(char *dest, const char *src)
{
    return strcpy(dest, src);
}

size_t trusty_strlen(const char *str)
{
    return strlen(str);
}

void *trusty_calloc(size_t n, size_t size)
{
    return calloc(n, size);
}

void trusty_free(void *addr)
{
    if (addr)
        free(addr);
}

void *trusty_alloc_pages(unsigned count)
{
    return memalign(PAGE_SIZE, count * PAGE_SIZE);
}

void trusty_free_pages(void *va, unsigned count)
{
    if (va)
        free(va);
}
