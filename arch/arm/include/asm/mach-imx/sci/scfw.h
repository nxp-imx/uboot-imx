/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

/*==========================================================================*/
/*!
 * @file  scfw.h
 *
 * Header file containing includes to system headers.
 */
/*==========================================================================*/

#ifndef _SC_SCFW_H
#define _SC_SCFW_H

/* Includes */

#include <linux/types.h>

#ifdef __cplusplus
  #define   __I     volatile             /*!< Defines 'read only' permissions                 */
#else
  #define   __I     volatile const       /*!< Defines 'read only' permissions                 */
#endif
#define     __O     volatile             /*!< Defines 'write only' permissions                */
#define     __IO    volatile             /*!< Defines 'read / write' permissions              */

/*!
 * This type is used to declare a handle for an IPC communication
 * channel. Its meaning is specific to the IPC implementation.
 */
typedef uint64_t sc_ipc_t;

/*!
 * This type is used to declare an ID for an IPC communication
 * channel. For the reference IPC implementation, this ID
 * selects the base address of the MU used for IPC.
 */
typedef uint64_t sc_ipc_id_t;

#endif /* _SC_SCFW_H */

