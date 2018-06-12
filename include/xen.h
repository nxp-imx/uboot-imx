/******************************************************************************
 * xen.h
 *
 * Guest OS interface to Xen.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Copyright (c) 2004, K A Fraser
 */

#ifndef __XEN_PUBLIC_XEN_H__
#define __XEN_PUBLIC_XEN_H__

//#include <asm/xen/interface.h>

/*
 * XEN "SYSTEM CALLS" (a.k.a. HYPERCALLS).
 */

/*
 * x86_32: EAX = vector; EBX, ECX, EDX, ESI, EDI = args 1, 2, 3, 4, 5.
 *         EAX = return value
 *         (argument registers may be clobbered on return)
 * x86_64: RAX = vector; RDI, RSI, RDX, R10, R8, R9 = args 1, 2, 3, 4, 5, 6.
 *         RAX = return value
 *         (argument registers not clobbered on return; RCX, R11 are)
 */
#define __HYPERVISOR_set_trap_table        0
#define __HYPERVISOR_mmu_update            1
#define __HYPERVISOR_set_gdt               2
#define __HYPERVISOR_stack_switch          3
#define __HYPERVISOR_set_callbacks         4
#define __HYPERVISOR_fpu_taskswitch        5
#define __HYPERVISOR_sched_op_compat       6
#define __HYPERVISOR_platform_op           7
#define __HYPERVISOR_set_debugreg          8
#define __HYPERVISOR_get_debugreg          9
#define __HYPERVISOR_update_descriptor    10
#define __HYPERVISOR_memory_op            12
#define __HYPERVISOR_multicall            13
#define __HYPERVISOR_update_va_mapping    14
#define __HYPERVISOR_set_timer_op         15
#define __HYPERVISOR_event_channel_op_compat 16
#define __HYPERVISOR_xen_version          17
#define __HYPERVISOR_console_io           18
#define __HYPERVISOR_physdev_op_compat    19
#define __HYPERVISOR_grant_table_op       20
#define __HYPERVISOR_vm_assist            21
#define __HYPERVISOR_update_va_mapping_otherdomain 22
#define __HYPERVISOR_iret                 23 /* x86 only */
#define __HYPERVISOR_vcpu_op              24
#define __HYPERVISOR_set_segment_base     25 /* x86/64 only */
#define __HYPERVISOR_mmuext_op            26
#define __HYPERVISOR_xsm_op               27
#define __HYPERVISOR_nmi_op               28
#define __HYPERVISOR_sched_op             29
#define __HYPERVISOR_callback_op          30
#define __HYPERVISOR_xenoprof_op          31
#define __HYPERVISOR_event_channel_op     32
#define __HYPERVISOR_physdev_op           33
#define __HYPERVISOR_hvm_op               34
#define __HYPERVISOR_sysctl               35
#define __HYPERVISOR_domctl               36
#define __HYPERVISOR_kexec_op             37
#define __HYPERVISOR_tmem_op              38
#define __HYPERVISOR_xc_reserved_op       39 /* reserved for XenClient */
#define __HYPERVISOR_xenpmu_op            40

/* Architecture-specific hypercall definitions. */
#define __HYPERVISOR_arch_0               48
#define __HYPERVISOR_arch_1               49
#define __HYPERVISOR_arch_2               50
#define __HYPERVISOR_arch_3               51
#define __HYPERVISOR_arch_4               52
#define __HYPERVISOR_arch_5               53
#define __HYPERVISOR_arch_6               54
#define __HYPERVISOR_arch_7               55

/*
 * VIRTUAL INTERRUPTS
 *
 * Virtual interrupts that a guest OS may receive from Xen.
 * In the side comments, 'V.' denotes a per-VCPU VIRQ while 'G.' denotes a
 * global VIRQ. The former can be bound once per VCPU and cannot be re-bound.
 * The latter can be allocated only once per guest: they must initially be
 * allocated to VCPU0 but can subsequently be re-bound.
 */
#define VIRQ_TIMER      0  /* V. Timebase update, and/or requested timeout.  */
#define VIRQ_DEBUG      1  /* V. Request guest to dump debug info.           */
#define VIRQ_CONSOLE    2  /* G. (DOM0) Bytes received on emergency console. */
#define VIRQ_DOM_EXC    3  /* G. (DOM0) Exceptional event for some domain.   */
#define VIRQ_TBUF       4  /* G. (DOM0) Trace buffer has records available.  */
#define VIRQ_DEBUGGER   6  /* G. (DOM0) A domain has paused for debugging.   */
#define VIRQ_XENOPROF   7  /* V. XenOprofile interrupt: new sample available */
#define VIRQ_CON_RING   8  /* G. (DOM0) Bytes received on console            */
#define VIRQ_PCPU_STATE 9  /* G. (DOM0) PCPU state changed                   */
#define VIRQ_MEM_EVENT  10 /* G. (DOM0) A memory event has occured           */
#define VIRQ_XC_RESERVED 11 /* G. Reserved for XenClient                     */
#define VIRQ_ENOMEM     12 /* G. (DOM0) Low on heap memory       */
#define VIRQ_XENPMU     13  /* PMC interrupt                                 */

/* Architecture-specific VIRQ definitions. */
#define VIRQ_ARCH_0    16
#define VIRQ_ARCH_1    17
#define VIRQ_ARCH_2    18
#define VIRQ_ARCH_3    19
#define VIRQ_ARCH_4    20
#define VIRQ_ARCH_5    21
#define VIRQ_ARCH_6    22
#define VIRQ_ARCH_7    23

#define NR_VIRQS       24

/*
 * enum neg_errnoval HYPERVISOR_mmu_update(const struct mmu_update reqs[],
 *                                         unsigned count, unsigned *done_out,
 *                                         unsigned foreigndom)
 * @reqs is an array of mmu_update_t structures ((ptr, val) pairs).
 * @count is the length of the above array.
 * @pdone is an output parameter indicating number of completed operations
 * @foreigndom[15:0]: FD, the expected owner of data pages referenced in this
 *                    hypercall invocation. Can be DOMID_SELF.
 * @foreigndom[31:16]: PFD, the expected owner of pagetable pages referenced
 *                     in this hypercall invocation. The value of this field
 *                     (x) encodes the PFD as follows:
 *                     x == 0 => PFD == DOMID_SELF
 *                     x != 0 => PFD == x - 1
 *
 * Sub-commands: ptr[1:0] specifies the appropriate MMU_* command.
 * -------------
 * ptr[1:0] == MMU_NORMAL_PT_UPDATE:
 * Updates an entry in a page table belonging to PFD. If updating an L1 table,
 * and the new table entry is valid/present, the mapped frame must belong to
 * FD. If attempting to map an I/O page then the caller assumes the privilege
 * of the FD.
 * FD == DOMID_IO: Permit /only/ I/O mappings, at the priv level of the caller.
 * FD == DOMID_XEN: Map restricted areas of Xen's heap space.
 * ptr[:2]  -- Machine address of the page-table entry to modify.
 * val      -- Value to write.
 *
 * There also certain implicit requirements when using this hypercall. The
 * pages that make up a pagetable must be mapped read-only in the guest.
 * This prevents uncontrolled guest updates to the pagetable. Xen strictly
 * enforces this, and will disallow any pagetable update which will end up
 * mapping pagetable page RW, and will disallow using any writable page as a
 * pagetable. In practice it means that when constructing a page table for a
 * process, thread, etc, we MUST be very dilligient in following these rules:
 *  1). Start with top-level page (PGD or in Xen language: L4). Fill out
 *      the entries.
 *  2). Keep on going, filling out the upper (PUD or L3), and middle (PMD
 *      or L2).
 *  3). Start filling out the PTE table (L1) with the PTE entries. Once
 *      done, make sure to set each of those entries to RO (so writeable bit
 *      is unset). Once that has been completed, set the PMD (L2) for this
 *      PTE table as RO.
 *  4). When completed with all of the PMD (L2) entries, and all of them have
 *      been set to RO, make sure to set RO the PUD (L3). Do the same
 *      operation on PGD (L4) pagetable entries that have a PUD (L3) entry.
 *  5). Now before you can use those pages (so setting the cr3), you MUST also
 *      pin them so that the hypervisor can verify the entries. This is done
 *      via the HYPERVISOR_mmuext_op(MMUEXT_PIN_L4_TABLE, guest physical frame
 *      number of the PGD (L4)). And this point the HYPERVISOR_mmuext_op(
 *      MMUEXT_NEW_BASEPTR, guest physical frame number of the PGD (L4)) can be
 *      issued.
 * For 32-bit guests, the L4 is not used (as there is less pagetables), so
 * instead use L3.
 * At this point the pagetables can be modified using the MMU_NORMAL_PT_UPDATE
 * hypercall. Also if so desired the OS can also try to write to the PTE
 * and be trapped by the hypervisor (as the PTE entry is RO).
 *
 * To deallocate the pages, the operations are the reverse of the steps
 * mentioned above. The argument is MMUEXT_UNPIN_TABLE for all levels and the
 * pagetable MUST not be in use (meaning that the cr3 is not set to it).
 *
 * ptr[1:0] == MMU_MACHPHYS_UPDATE:
 * Updates an entry in the machine->pseudo-physical mapping table.
 * ptr[:2]  -- Machine address within the frame whose mapping to modify.
 *             The frame must belong to the FD, if one is specified.
 * val      -- Value to write into the mapping entry.
 *
 * ptr[1:0] == MMU_PT_UPDATE_PRESERVE_AD:
 * As MMU_NORMAL_PT_UPDATE above, but A/D bits currently in the PTE are ORed
 * with those in @val.
 *
 * @val is usually the machine frame number along with some attributes.
 * The attributes by default follow the architecture defined bits. Meaning that
 * if this is a X86_64 machine and four page table layout is used, the layout
 * of val is:
 *  - 63 if set means No execute (NX)
 *  - 46-13 the machine frame number
 *  - 12 available for guest
 *  - 11 available for guest
 *  - 10 available for guest
 *  - 9 available for guest
 *  - 8 global
 *  - 7 PAT (PSE is disabled, must use hypercall to make 4MB or 2MB pages)
 *  - 6 dirty
 *  - 5 accessed
 *  - 4 page cached disabled
 *  - 3 page write through
 *  - 2 userspace accessible
 *  - 1 writeable
 *  - 0 present
 *
 *  The one bits that does not fit with the default layout is the PAGE_PSE
 *  also called PAGE_PAT). The MMUEXT_[UN]MARK_SUPER arguments to the
 *  HYPERVISOR_mmuext_op serve as mechanism to set a pagetable to be 4MB
 *  (or 2MB) instead of using the PAGE_PSE bit.
 *
 *  The reason that the PAGE_PSE (bit 7) is not being utilized is due to Xen
 *  using it as the Page Attribute Table (PAT) bit - for details on it please
 *  refer to Intel SDM 10.12. The PAT allows to set the caching attributes of
 *  pages instead of using MTRRs.
 *
 *  The PAT MSR is as follows (it is a 64-bit value, each entry is 8 bits):
 *                    PAT4                 PAT0
 *  +-----+-----+----+----+----+-----+----+----+
 *  | UC  | UC- | WC | WB | UC | UC- | WC | WB |  <= Linux
 *  +-----+-----+----+----+----+-----+----+----+
 *  | UC  | UC- | WT | WB | UC | UC- | WT | WB |  <= BIOS (default when machine boots)
 *  +-----+-----+----+----+----+-----+----+----+
 *  | rsv | rsv | WP | WC | UC | UC- | WT | WB |  <= Xen
 *  +-----+-----+----+----+----+-----+----+----+
 *
 *  The lookup of this index table translates to looking up
 *  Bit 7, Bit 4, and Bit 3 of val entry:
 *
 *  PAT/PSE (bit 7) ... PCD (bit 4) .. PWT (bit 3).
 *
 *  If all bits are off, then we are using PAT0. If bit 3 turned on,
 *  then we are using PAT1, if bit 3 and bit 4, then PAT2..
 *
 *  As you can see, the Linux PAT1 translates to PAT4 under Xen. Which means
 *  that if a guest that follows Linux's PAT setup and would like to set Write
 *  Combined on pages it MUST use PAT4 entry. Meaning that Bit 7 (PAGE_PAT) is
 *  set. For example, under Linux it only uses PAT0, PAT1, and PAT2 for the
 *  caching as:
 *
 *   WB = none (so PAT0)
 *   WC = PWT (bit 3 on)
 *   UC = PWT | PCD (bit 3 and 4 are on).
 *
 * To make it work with Xen, it needs to translate the WC bit as so:
 *
 *  PWT (so bit 3 on) --> PAT (so bit 7 is on) and clear bit 3
 *
 * And to translate back it would:
 *
 * PAT (bit 7 on) --> PWT (bit 3 on) and clear bit 7.
 */
#define MMU_NORMAL_PT_UPDATE      0 /* checked '*ptr = val'. ptr is MA.       */
#define MMU_MACHPHYS_UPDATE       1 /* ptr = MA of frame to modify entry for  */
#define MMU_PT_UPDATE_PRESERVE_AD 2 /* atomically: *ptr = val | (*ptr&(A|D)) */

/*
 * MMU EXTENDED OPERATIONS
 *
 * enum neg_errnoval HYPERVISOR_mmuext_op(mmuext_op_t uops[],
 *                                        unsigned int count,
 *                                        unsigned int *pdone,
 *                                        unsigned int foreigndom)
 */
/* HYPERVISOR_mmuext_op() accepts a list of mmuext_op structures.
 * A foreigndom (FD) can be specified (or DOMID_SELF for none).
 * Where the FD has some effect, it is described below.
 *
 * cmd: MMUEXT_(UN)PIN_*_TABLE
 * mfn: Machine frame number to be (un)pinned as a p.t. page.
 *      The frame must belong to the FD, if one is specified.
 *
 * cmd: MMUEXT_NEW_BASEPTR
 * mfn: Machine frame number of new page-table base to install in MMU.
 *
 * cmd: MMUEXT_NEW_USER_BASEPTR [x86/64 only]
 * mfn: Machine frame number of new page-table base to install in MMU
 *      when in user space.
 *
 * cmd: MMUEXT_TLB_FLUSH_LOCAL
 * No additional arguments. Flushes local TLB.
 *
 * cmd: MMUEXT_INVLPG_LOCAL
 * linear_addr: Linear address to be flushed from the local TLB.
 *
 * cmd: MMUEXT_TLB_FLUSH_MULTI
 * vcpumask: Pointer to bitmap of VCPUs to be flushed.
 *
 * cmd: MMUEXT_INVLPG_MULTI
 * linear_addr: Linear address to be flushed.
 * vcpumask: Pointer to bitmap of VCPUs to be flushed.
 *
 * cmd: MMUEXT_TLB_FLUSH_ALL
 * No additional arguments. Flushes all VCPUs' TLBs.
 *
 * cmd: MMUEXT_INVLPG_ALL
 * linear_addr: Linear address to be flushed from all VCPUs' TLBs.
 *
 * cmd: MMUEXT_FLUSH_CACHE
 * No additional arguments. Writes back and flushes cache contents.
 *
 * cmd: MMUEXT_FLUSH_CACHE_GLOBAL
 * No additional arguments. Writes back and flushes cache contents
 * on all CPUs in the system.
 *
 * cmd: MMUEXT_SET_LDT
 * linear_addr: Linear address of LDT base (NB. must be page-aligned).
 * nr_ents: Number of entries in LDT.
 *
 * cmd: MMUEXT_CLEAR_PAGE
 * mfn: Machine frame number to be cleared.
 *
 * cmd: MMUEXT_COPY_PAGE
 * mfn: Machine frame number of the destination page.
 * src_mfn: Machine frame number of the source page.
 *
 * cmd: MMUEXT_[UN]MARK_SUPER
 * mfn: Machine frame number of head of superpage to be [un]marked.
 */
#define MMUEXT_PIN_L1_TABLE      0
#define MMUEXT_PIN_L2_TABLE      1
#define MMUEXT_PIN_L3_TABLE      2
#define MMUEXT_PIN_L4_TABLE      3
#define MMUEXT_UNPIN_TABLE       4
#define MMUEXT_NEW_BASEPTR       5
#define MMUEXT_TLB_FLUSH_LOCAL   6
#define MMUEXT_INVLPG_LOCAL      7
#define MMUEXT_TLB_FLUSH_MULTI   8
#define MMUEXT_INVLPG_MULTI      9
#define MMUEXT_TLB_FLUSH_ALL    10
#define MMUEXT_INVLPG_ALL       11
#define MMUEXT_FLUSH_CACHE      12
#define MMUEXT_SET_LDT          13
#define MMUEXT_NEW_USER_BASEPTR 15
#define MMUEXT_CLEAR_PAGE       16
#define MMUEXT_COPY_PAGE        17
#define MMUEXT_FLUSH_CACHE_GLOBAL 18
#define MMUEXT_MARK_SUPER       19
#define MMUEXT_UNMARK_SUPER     20


/* These are passed as 'flags' to update_va_mapping. They can be ORed. */
/* When specifying UVMF_MULTI, also OR in a pointer to a CPU bitmap.   */
/* UVMF_LOCAL is merely UVMF_MULTI with a NULL bitmap pointer.         */
#define UVMF_NONE               (0UL<<0) /* No flushing at all.   */
#define UVMF_TLB_FLUSH          (1UL<<0) /* Flush entire TLB(s).  */
#define UVMF_INVLPG             (2UL<<0) /* Flush only one entry. */
#define UVMF_FLUSHTYPE_MASK     (3UL<<0)
#define UVMF_MULTI              (0UL<<2) /* Flush subset of TLBs. */
#define UVMF_LOCAL              (0UL<<2) /* Flush local TLB.      */
#define UVMF_ALL                (1UL<<2) /* Flush all TLBs.       */

/*
 * Commands to HYPERVISOR_console_io().
 */
#define CONSOLEIO_write         0
#define CONSOLEIO_read          1

/*
 * Commands to HYPERVISOR_vm_assist().
 */
#define VMASST_CMD_enable                0
#define VMASST_CMD_disable               1

/* x86/32 guests: simulate full 4GB segment limits. */
#define VMASST_TYPE_4gb_segments         0

/* x86/32 guests: trap (vector 15) whenever above vmassist is used. */
#define VMASST_TYPE_4gb_segments_notify  1

/*
 * x86 guests: support writes to bottom-level PTEs.
 * NB1. Page-directory entries cannot be written.
 * NB2. Guest must continue to remove all writable mappings of PTEs.
 */
#define VMASST_TYPE_writable_pagetables  2

/* x86/PAE guests: support PDPTs above 4GB. */
#define VMASST_TYPE_pae_extended_cr3     3

/*
 * x86 guests: Sane behaviour for virtual iopl
 *  - virtual iopl updated from do_iret() hypercalls.
 *  - virtual iopl reported in bounce frames.
 *  - guest kernels assumed to be level 0 for the purpose of iopl checks.
 */
#define VMASST_TYPE_architectural_iopl   4

/*
 * All guests: activate update indicator in vcpu_runstate_info
 * Enable setting the XEN_RUNSTATE_UPDATE flag in guest memory mapped
 * vcpu_runstate_info during updates of the runstate information.
 */
#define VMASST_TYPE_runstate_update_flag 5

#define MAX_VMASST_TYPE 5

#endif /* __XEN_PUBLIC_XEN_H__ */
