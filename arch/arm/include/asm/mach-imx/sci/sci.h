/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

/*==========================================================================*/
/*!
 * @file sci.h
 *
 * Header file containing the public System Controller Interface (SCI)
 * definitions.
 *
 *
 * @{
 */
/*==========================================================================*/

#ifndef _SC_SCI_H
#define _SC_SCI_H

/* Defines */

/* Includes */

#include <asm/mach-imx/sci/types.h>
#include <asm/mach-imx/sci/ipc.h>
#include <asm/mach-imx/sci/svc/misc/api.h>
#include <asm/mach-imx/sci/svc/pad/api.h>
#include <asm/mach-imx/sci/svc/pm/api.h>
#include <asm/mach-imx/sci/svc/rm/api.h>
#include <asm/mach-imx/sci/svc/timer/api.h>

#define SC_IPC_AP_CH0       	(MU_BASE_ADDR(0))
#define SC_IPC_AP_CH1       	(MU_BASE_ADDR(1))
#define SC_IPC_AP_CH2       	(MU_BASE_ADDR(2))
#define SC_IPC_AP_CH3       	(MU_BASE_ADDR(3))
#define SC_IPC_AP_CH4       	(MU_BASE_ADDR(4))

#ifndef SC_IPC_CH
#define SC_IPC_CH		SC_IPC_AP_CH1
#endif

/* Types */

/* Functions */

#endif /* _SC_SCI_H */

/**@}*/

