/*
 * Copyright 2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*==========================================================================*/
/*!
 * @file fsl_mu_hal.c
 *
 * Source file containing the HAL API for the MU.
 *
 * @addtogroup MU_HAL
 * @{
 */
/*==========================================================================*/

#include <asm/arch/fsl_mu_hal.h>
#if FSL_FEATURE_SOC_MU_COUNT

/*FUNCTION**********************************************************************
 *
 * Function Name : MU_HAL_TrySendMsg
 * Description   : Try to send message to the other core.
 *
 *END**************************************************************************/
mu_status_t MU_HAL_TrySendMsg(MU_Type *base, uint32_t regIndex, uint32_t msg)
{
	assert(regIndex < MU_TR_COUNT);

	/* TX register is empty. */
	if (MU_HAL_IsTxEmpty(base, regIndex)) {
		MU_WR_TR(base, regIndex, msg);
		return kStatus_MU_Success;
	}

	return kStatus_MU_TxNotEmpty;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : MU_HAL_SendMsg
 * Description   : Wait and send message to the other core.
 *
 *END**************************************************************************/
void MU_HAL_SendMsg(MU_Type *base, uint32_t regIndex, uint32_t msg)
{
	assert(regIndex < MU_TR_COUNT);
	uint32_t mask = MU_SR_TE0_MASK >> regIndex;
	/* Wait TX register to be empty. */
	while (!(MU_RD_SR(base) & mask)) { }
	MU_WR_TR(base, regIndex, msg);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : MU_HAL_TryReceiveMsg
 * Description   : Try to receive message from the other core.
 *
 *END**************************************************************************/
mu_status_t MU_HAL_TryReceiveMsg(MU_Type *base, uint32_t regIndex, uint32_t *msg)
{
	assert(regIndex < MU_RR_COUNT);

	/* RX register is full. */
	if (MU_HAL_IsRxFull(base, regIndex)) {
		*msg = MU_RD_RR(base, regIndex);
		return kStatus_MU_Success;
	}

	return kStatus_MU_RxNotFull;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : MU_HAL_ReceiveMsg
 * Description   : Wait to receive message from the other core.
 *
 *END**************************************************************************/
void MU_HAL_ReceiveMsg(MU_Type *base, uint32_t regIndex, uint32_t *msg)
{
	assert(regIndex < MU_TR_COUNT);
	uint32_t mask = MU_SR_RF0_MASK >> regIndex;

	/* Wait RX register to be full. */
	while (!(MU_RD_SR(base) & mask)) { }
	*msg = MU_RD_RR(base, regIndex);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : MU_HAL_TriggerGeneralInt
 * Description   : Trigger general purpose interrupt to the other core.
 *
 *END**************************************************************************/
mu_status_t MU_HAL_TriggerGeneralInt(MU_Type *base, uint32_t index)
{
	/* Previous interrupt has been accepted. */
	if (MU_HAL_IsGeneralIntAccepted(base, index)) {
		/* All interrupts have been accepted, trigger now. */
		MU_WR_CR_GIRn(base, (1U << (MU_GPn_COUNT - 1U)) >> index);
		return kStatus_MU_Success;
	}

	return kStatus_MU_IntPending;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : MU_HAL_TrySetFlags
 * Description   : Try to set some bits of the 3-bit flag.
 *
 *END**************************************************************************/
mu_status_t MU_HAL_TrySetFlags(MU_Type *base, uint32_t flags)
{
	if (MU_RD_SR_FUP(base))
	{
		return kStatus_MU_FlagPending;
	}

	MU_WR_CR(base, (MU_RD_CR(base)
						& ~(MU_CR_GIRn_MASK |
						MU_CR_NMI_MASK  |
						MU_CR_Fn_MASK))
						|   flags);
	return kStatus_MU_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : MU_HAL_SetFlags
 * Description   : Block to set some bits of the 3-bit flag.
 *
 *END**************************************************************************/
void MU_HAL_SetFlags(MU_Type *base, uint32_t flags)
{
	while (MU_RD_SR_FUP(base)) { }
	MU_WR_CR(base, (MU_RD_CR(base)
						& ~(MU_CR_GIRn_MASK |
						MU_CR_NMI_MASK  |
						MU_CR_Fn_MASK))
						|   flags);
}
#endif

/*******************************************************************************
 * EOF
 ******************************************************************************/

/**@}*/
