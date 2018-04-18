/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*==========================================================================*/
/*!
 * @file  ipc.c
 *
 * Implementation of the IPC functions using MUs (client side).
 */
/*==========================================================================*/

/* Includes */

#include <asm/imx-common/sci/scfw.h>
#include <asm/imx-common/sci/ipc.h>
#include <asm/imx-common/sci/rpc.h>
#include <asm/arch/fsl_mu_hal.h>

/* Local Defines */

/* Local Types */

/* Local Functions */

/* Local Variables */


/*----------------------------------------------------------------------*/
/* RPC command/response                                                 */
/*----------------------------------------------------------------------*/
void sc_call_rpc(sc_ipc_t ipc, sc_rpc_msg_t *msg, sc_bool_t no_resp)
{
    sc_ipc_write(ipc, msg);
    if (!no_resp)
        sc_ipc_read(ipc, msg);
}

/*--------------------------------------------------------------------------*/
/* Open an IPC channel                                                      */
/*--------------------------------------------------------------------------*/
sc_err_t sc_ipc_open(sc_ipc_t *ipc, sc_ipc_id_t id)
{
    MU_Type *base = (MU_Type*) id;
    uint32_t i;

    /* Get MU base associated with IPC channel */
    if ((ipc == NULL) || (base == NULL))
        return SC_ERR_IPC;

    /* Init MU */
    MU_HAL_Init(base);

    /* Enable all RX interrupts */
    for (i = 0; i < MU_RR_COUNT; i++)
    {
        MU_HAL_EnableRxFullInt(base, i);
    }

    /* Return MU address as handle */
    *ipc = (sc_ipc_t) id;

    return SC_ERR_NONE;
}

/*--------------------------------------------------------------------------*/
/* Close an IPC channel                                                     */
/*--------------------------------------------------------------------------*/
void sc_ipc_close(sc_ipc_t ipc)
{
    MU_Type *base = (MU_Type*) ipc;

    if (base != NULL)
        MU_HAL_Init(base);
    }

/*--------------------------------------------------------------------------*/
/* Read message from an IPC channel                                         */
/*--------------------------------------------------------------------------*/
void sc_ipc_read(sc_ipc_t ipc, void *data)
{
    MU_Type *base = (MU_Type*) ipc;
    sc_rpc_msg_t *msg = (sc_rpc_msg_t*) data;
    uint8_t count = 0;

    /* Check parms */
    if ((base == NULL) || (msg == NULL))
        return;

    /* Read first word */
    MU_HAL_ReceiveMsg(base, 0, (uint32_t*) msg);
    count++;

    /* Check size */
    if (msg->size > SC_RPC_MAX_MSG)
    {
        *((uint32_t*) msg) = 0;
        return;
    }

    /* Read remaining words */
    while (count < msg->size)
    {
        MU_HAL_ReceiveMsg(base, count % MU_RR_COUNT,
            &(msg->DATA.u32[count - 1]));

        count++;
    }
}

/*--------------------------------------------------------------------------*/
/* Write a message to an IPC channel                                        */
/*--------------------------------------------------------------------------*/
void sc_ipc_write(sc_ipc_t ipc, void *data)
{
    MU_Type *base = (MU_Type*) ipc;
    sc_rpc_msg_t *msg = (sc_rpc_msg_t*) data;
    uint8_t count = 0;

    /* Check parms */
    if ((base == NULL) || (msg == NULL))
        return;

    /* Check size */
    if (msg->size > SC_RPC_MAX_MSG)
        return;

    /* Write first word */
    MU_HAL_SendMsg(base, 0, *((uint32_t*) msg));
    count++;

    /* Write remaining words */
    while (count < msg->size)
    {
        MU_HAL_SendMsg(base, count % MU_TR_COUNT,
            msg->DATA.u32[count - 1]);

        count++;
    }
}

