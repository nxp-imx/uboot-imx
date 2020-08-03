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

#include <trusty/trusty_dev.h>
#include <trusty/util.h>

/* 48-bit physical address bits 47:12 */

#define NS_PTE_PHYSADDR_SHIFT      12
#define NS_PTE_PHYSADDR(pte)       ((pte) & 0xFFFFFFFFF000ULL)

/* Access permissions bits 7:6
 *      EL0     EL1
 * 00   None    RW
 * 01   RW      RW
 * 10   None    RO
 * 11   RO      RO
 */
#define NS_PTE_AP_SHIFT                    6
#define NS_PTE_AP_MASK                     (0x3 << NS_PTE_AP_SHIFT)

/* Memory type and cache attributes bits 55:48 */
#define NS_PTE_MAIR_SHIFT                  48
#define NS_PTE_MAIR_MASK                   (0x00FFULL << NS_PTE_MAIR_SHIFT)

#define NS_PTE_MAIR_INNER_SHIFT            48
#define NS_PTE_MAIR_INNER_MASK             (0x000FULL << NS_PTE_MAIR_INNER_SHIFT)

#define NS_PTE_MAIR_OUTER_SHIFT            52
#define NS_PTE_MAIR_OUTER_MASK             (0x000FULL << NS_PTE_MAIR_OUTER_SHIFT)

/* Normal memory */
#define NS_MAIR_NORMAL_CACHED_WB_RWA       0xFF /* inner and outer write back read/write allocate */
#define NS_MAIR_NORMAL_CACHED_WT_RA        0xAA /* inner and outer write through read allocate */
#define NS_MAIR_NORMAL_CACHED_WB_RA        0xEE /* inner and outer write back, read allocate */
#define NS_MAIR_NORMAL_UNCACHED            0x44 /* uncached */

/* Device memory */
#define NS_MAIR_DEVICE_STRONGLY_ORDERED    0x00 /* nGnRnE (strongly ordered) */
#define NS_MAIR_DEVICE                     0x04 /* nGnRE  (device) */
#define NS_MAIR_DEVICE_GRE                 0x0C /* GRE */

/* shareable attributes bits 9:8 */
#define NS_PTE_SHAREABLE_SHIFT             8

#define NS_NON_SHAREABLE                   0x0
#define NS_OUTER_SHAREABLE                 0x2
#define NS_INNER_SHAREABLE                 0x3

typedef uintptr_t addr_t;
typedef uintptr_t vaddr_t;
typedef uintptr_t paddr_t;

#if NS_ARCH_ARM64

#define  PAR_F  (0x1 <<  0)

/*
 * ARM64
 */

/* Note: this will crash if called from user space */
static void arm64_write_ATS1ExW(uint64_t vaddr)
{
    uint64_t _current_el;

    __asm__ volatile("mrs %0, CurrentEL" : "=r" (_current_el));

    _current_el = (_current_el >> 2) & 0x3;
    switch (_current_el) {
    case 0x1:
        __asm__ volatile("at S1E1W, %0" :: "r" (vaddr));
        break;
    case 0x2:
        __asm__ volatile("at S1E2W, %0" :: "r" (vaddr));
        break;
    case 0x3:
    default:
        trusty_fatal("Unsupported execution state: EL%lu\n", _current_el );
        break;
    }

    __asm__ volatile("isb" ::: "memory");
}

static uint64_t arm64_read_par64(void)
{
    uint64_t _val;
    __asm__ volatile("mrs %0, par_el1" : "=r" (_val));
    return _val;
}


static uint64_t va2par(vaddr_t va)
{
    uint64_t par;
    unsigned long irq_state;

    trusty_local_irq_disable(&irq_state);
    arm64_write_ATS1ExW(va);
    par = arm64_read_par64();
    trusty_local_irq_restore(&irq_state);

    return par;
}

static uint64_t par2attr(uint64_t par)
{
    uint64_t attr;

    /* set phys address */
    attr = NS_PTE_PHYSADDR(par);

    /* cache attributes */
    attr |= ((par >> 56) & 0xFF) << NS_PTE_MAIR_SHIFT;

    /* shareable attributes */
    attr |= ((par >> 7) & 0x03) << NS_PTE_SHAREABLE_SHIFT;

    /* the memory is writable and accessible so leave AP field 0 */
    attr |= 0x0 << NS_PTE_AP_SHIFT;

    return attr;
}

#else

#define  PAR_F     (0x1 <<  0)
#define  PAR_SS    (0x1 <<  1)
#define  PAR_SH    (0x1 <<  7)
#define  PAR_NOS   (0x1 << 10)
#define  PAR_LPAE  (0x1 << 11)

/*
 * ARM32
 */

/* Note: this will crash if called from user space */
static void arm_write_ATS1xW(uint64_t vaddr)
{
    uint32_t _cpsr;

    __asm__ volatile("mrs %0, cpsr" : "=r"(_cpsr));

    if ((_cpsr & 0xF) == 0xa)
        __asm__ volatile("mcr    p15, 4, %0, c7, c8, 1" : : "r"(vaddr));
    else
        __asm__ volatile("mcr    p15, 0, %0, c7, c8, 1" : : "r"(vaddr));
}

static uint64_t arm_read_par64(void)
{
    uint32_t lower, higher;

    __asm__ volatile(
        "mrc    p15, 0, %0, c7, c4, 0   \n"
        "tst    %0, #(1 << 11)      @ LPAE / long desc format\n"
        "moveq  %1, #0          \n"
        "mrrcne p15, 0, %0, %1, c7  \n"
         :"=r"(lower), "=r"(higher) : :
    );

    return ((uint64_t)higher << 32) | lower;
}


static uint8_t ish_to_mair[8] = {
    0x04, /* 0b000 Non cacheble */
    0x00, /* 0b001 Strongly ordered */
    0xF0, /* 0b010 reserved */
    0x04, /* 0b011 device */
    0xF0, /* 0b100 reserved */
    0x0F, /* 0b101 write back - write allocate */
    0x0A, /* 0b110 write through */
    0x0E, /* 0b111 write back - no write allocate */
};

static uint8_t osh_to_mair[4] = {
    0x00, /* 0b00   Non-cacheable */
    0x0F, /* 0b01   Write-back, Write-allocate */
    0x0A, /* 0b10   Write-through, no Write-allocate */
    0x0E, /* 0b11   Write-back, no Write-allocate */
};

static uint64_t par2attr(uint64_t par)
{
    uint64_t attr;

    if (par & PAR_LPAE) {
        /* set phys address */
        attr = NS_PTE_PHYSADDR(par);

        /* cache attributes */
        attr |= ((par >> 56) & 0xFF) << NS_PTE_MAIR_SHIFT;

        /* shareable attributes */
        attr |= ((par >> 7) & 0x03) << NS_PTE_SHAREABLE_SHIFT;

    } else {

        /* set phys address */
        trusty_assert((par & PAR_SS) == 0); /* super section not supported */
        attr = NS_PTE_PHYSADDR(par);

        /* cache attributes */
        uint64_t inner = ((uint64_t)ish_to_mair[(par >> 4) & 0x7]) << NS_PTE_MAIR_INNER_SHIFT;
        uint64_t outer = ((uint64_t)osh_to_mair[(par >> 2) & 0x3]) << NS_PTE_MAIR_OUTER_SHIFT;
        uint64_t cache_attributes = (outer << 4) | inner;

        /* Trusty does not support any kind of device memory, so we will force
         * cache attributes to be NORMAL UNCACHED on the Trusty side.
         */
        if (cache_attributes == NS_MAIR_DEVICE_STRONGLY_ORDERED) {
            attr |= ((uint64_t)NS_MAIR_NORMAL_UNCACHED << NS_PTE_MAIR_SHIFT);
        } else {
            attr |= inner;
            attr |= outer;
        }

        /* shareable attributes */
        if (par & PAR_SH) {
            /* how to handle NOS bit ? */
            attr |= ((uint64_t)NS_INNER_SHAREABLE) << NS_PTE_SHAREABLE_SHIFT;
        } else {
            attr |= ((uint64_t)NS_NON_SHAREABLE) << NS_PTE_SHAREABLE_SHIFT;
        }
    }

    /* the memory is writable and accessible so leave AP field 0 */
    attr |= 0x0 << NS_PTE_AP_SHIFT;

    return attr;
}

static uint64_t va2par(vaddr_t va)
{
    uint64_t par;
    unsigned long irq_state;

    trusty_local_irq_disable(&irq_state);
    arm_write_ATS1xW(va);
    par = arm_read_par64();
    trusty_local_irq_restore(&irq_state);

    return par;
}

#endif /* ARM64 */


int trusty_encode_page_info(struct ns_mem_page_info *inf, void *va)
{
    uint64_t par = va2par((vaddr_t)va);

    if (par & PAR_F) {
        return -1;
    }

    inf->attr = par2attr(par);

    return 0;
}

