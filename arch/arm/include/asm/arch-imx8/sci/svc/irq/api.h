/* SPDX-License-Identifier:     GPL-2.0+ */
/*
 * Copyright 2018-2019 NXP
 */

#ifndef SC_IRQ_API_H
#define SC_IRQ_API_H

/* Defines */

#define SC_IRQ_NUM_GROUP        7U   /* Number of groups */

/* Defines for sc_irq_group_t */
#define SC_IRQ_GROUP_TEMP       0U   /* Temp interrupts */
#define SC_IRQ_GROUP_WDOG       1U   /* Watchdog interrupts */
#define SC_IRQ_GROUP_RTC        2U   /* RTC interrupts */
#define SC_IRQ_GROUP_WAKE       3U   /* Wakeup interrupts */
#define SC_IRQ_GROUP_SYSCTR     4U   /* System counter interrupts */
#define SC_IRQ_GROUP_REBOOTED   5U   /* Partition reboot complete */
#define SC_IRQ_GROUP_REBOOT     6U   /* Partition reboot starting */

/* Defines for sc_irq_temp_t */
#define SC_IRQ_TEMP_HIGH         (1UL << 0U)    /* Temp alarm interrupt */
#define SC_IRQ_TEMP_CPU0_HIGH    (1UL << 1U)    /* CPU0 temp alarm interrupt */
#define SC_IRQ_TEMP_CPU1_HIGH    (1UL << 2U)    /* CPU1 temp alarm interrupt */
#define SC_IRQ_TEMP_GPU0_HIGH    (1UL << 3U)    /* GPU0 temp alarm interrupt */
#define SC_IRQ_TEMP_GPU1_HIGH    (1UL << 4U)    /* GPU1 temp alarm interrupt */
#define SC_IRQ_TEMP_DRC0_HIGH    (1UL << 5U)    /* DRC0 temp alarm interrupt */
#define SC_IRQ_TEMP_DRC1_HIGH    (1UL << 6U)    /* DRC1 temp alarm interrupt */
#define SC_IRQ_TEMP_VPU_HIGH     (1UL << 7U)    /* DRC1 temp alarm interrupt */
#define SC_IRQ_TEMP_PMIC0_HIGH   (1UL << 8U)    /* PMIC0 temp alarm interrupt */
#define SC_IRQ_TEMP_PMIC1_HIGH   (1UL << 9U)    /* PMIC1 temp alarm interrupt */
#define SC_IRQ_TEMP_LOW          (1UL << 10U)   /* Temp alarm interrupt */
#define SC_IRQ_TEMP_CPU0_LOW     (1UL << 11U)   /* CPU0 temp alarm interrupt */
#define SC_IRQ_TEMP_CPU1_LOW     (1UL << 12U)   /* CPU1 temp alarm interrupt */
#define SC_IRQ_TEMP_GPU0_LOW     (1UL << 13U)   /* GPU0 temp alarm interrupt */
#define SC_IRQ_TEMP_GPU1_LOW     (1UL << 14U)   /* GPU1 temp alarm interrupt */
#define SC_IRQ_TEMP_DRC0_LOW     (1UL << 15U)   /* DRC0 temp alarm interrupt */
#define SC_IRQ_TEMP_DRC1_LOW     (1UL << 16U)   /* DRC1 temp alarm interrupt */
#define SC_IRQ_TEMP_VPU_LOW      (1UL << 17U)   /* DRC1 temp alarm interrupt */
#define SC_IRQ_TEMP_PMIC0_LOW    (1UL << 18U)   /* PMIC0 temp alarm interrupt */
#define SC_IRQ_TEMP_PMIC1_LOW    (1UL << 19U)   /* PMIC1 temp alarm interrupt */
#define SC_IRQ_TEMP_PMIC2_HIGH   (1UL << 20U)   /* PMIC2 temp alarm interrupt */
#define SC_IRQ_TEMP_PMIC2_LOW    (1UL << 21U)   /* PMIC2 temp alarm interrupt */

/* Defines for sc_irq_wdog_t */
#define SC_IRQ_WDOG              (1U << 0U)    /* Watchdog interrupt */

/* Defines for sc_irq_rtc_t */
#define SC_IRQ_RTC               (1U << 0U)    /* RTC interrupt */

/* Defines for sc_irq_wake_t */
#define SC_IRQ_BUTTON            (1U << 0U)    /* Button interrupt */
#define SC_IRQ_PAD               (1U << 1U)    /* Pad wakeup */
#define SC_IRQ_USR1              (1U << 2U)    /* User defined 1 */
#define SC_IRQ_USR2              (1U << 3U)    /* User defined 2 */
#define SC_IRQ_BC_PAD            (1U << 4U)    /* Pad wakeup (broadcast to all partitions) */
#define SC_IRQ_SW_WAKE           (1U << 5U)    /* Software requested wake */
#define SC_IRQ_SECVIO            (1U << 6U)    /* Security violation */

/* Defines for sc_irq_sysctr_t */
#define SC_IRQ_SYSCTR            (1U << 0U)    /* SYSCTR interrupt */

/* Types */

/*
 * This type is used to declare an interrupt group.
 */
typedef u8 sc_irq_group_t;

/*
 * This type is used to declare a bit mask of temp interrupts.
 */
typedef u8 sc_irq_temp_t;

/*
 * This type is used to declare a bit mask of watchdog interrupts.
 */
typedef u8 sc_irq_wdog_t;

/*
 * This type is used to declare a bit mask of RTC interrupts.
 */
typedef u8 sc_irq_rtc_t;

/*
 * This type is used to declare a bit mask of wakeup interrupts.
 */
typedef u8 sc_irq_wake_t;

#endif /* SC_IRQ_API_H */
