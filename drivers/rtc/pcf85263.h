/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * rtc-pcf85263 Driver for the NXP PCF85263 RTC
 *
 * Copyright 2016 Parkeon
 * (C) Copyright 2017 MicroSys Electronics GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef PCF85263_H
#define PCF85263_H

#include <common.h>

#define PCF85263_REG_RTC_SC             0x01    /* Seconds */
#define PCF85263_REG_RTC_SC_OS          BIT(7)  /* Oscilator stopped flag */
#define PCF85263_REG_RAM_BYTE           0x2c
#define PCF85263_REG_RTC_MN             0x02    /* Minutes */
#define PCF85263_REG_RTC_HR             0x03    /* Hours */
#define PCF85263_REG_RTC_DT             0x04    /* Day of month 1-31 */
#define PCF85263_REG_RTC_DW             0x05    /* Day of week 0-6 */
#define PCF85263_REG_RTC_MO             0x06    /* Month 1-12 */
#define PCF85263_REG_RTC_YR             0x07    /* Year 0-99 */
#define PCF85263_REG_OSC                0x25
#define PCF85263_REG_OSC_12H            BIT(5)
#define PCF85263_REG_OSC_CL_MASK        (BIT(0) | BIT(1))
#define PCF85263_REG_OSC_CL_SHIFT       0
#define PCF85263_REG_OSC_OSCD_MASK      (BIT(2) | BIT(3))
#define PCF85263_REG_OSC_OSCD_SHIFT     2
#define PCF85263_REG_FUNCTION           0x28
#define PCF85263_REG_FUNCTION_COF_OFF   0x7     /* No clock output */
#define PCF85263_REG_INTA_CTL           0x29
#define PCF85263_REG_INTB_CTL           0x2A
#define PCF85263_REG_INTx_CTL_ILP       BIT(7)  /* 0=pulse, 1=level */
#define PCF85263_REG_PINIO              0x27
#define PCF85263_REG_PINIO_CLKDISABLE   BIT(7)
#define PCF85263_REG_PINIO_INTAPM_SHIFT 0
#define PCF85263_REG_PINIO_TSPM_SHIFT   2
#define PCF85263_INTAPM_HIGHZ           (0x3 << PCF85263_REG_PINIO_INTAPM_SHIFT)
#define PCF85263_TSPM_DISABLED          (0x0 << PCF85263_REG_PINIO_TSPM_SHIFT)
#define PCF85263_REG_STOPENABLE         0x2e
#define PCF85263_REG_STOPENABLE_STOP    BIT(0)
#define PCF85263_REG_RESET              0x2f    /* Reset command */
#define PCF85263_REG_RESET_CMD_CPR      0xa4    /* Clear prescaler */

/* Our data stored in the RAM byte */
#define PCF85263_STATE_CENTURY_MASK              0x7f
#define PCF85263_STATE_UPPER_HALF_CENTURY        BIT(7)

#define PCF85263_QUARTZCAP_12p5pF    2

#define PCF85263_QUARTZDRIVE_NORMAL  0
#define PCF85263_QUARTZDRIVE_LOW     1
#define PCF85263_QUARTZDRIVE_HIGH    2

#define PCF85263_HR_PM               BIT(5)

typedef struct pcf85263_s {
	uchar mode_12h;
	uchar century;
	uchar century_half;
} pcf85263_t;

#endif /* PCF85263_H */
