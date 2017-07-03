/*
 * Copyright (c) 2005-2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef IMXDPUV1_EVENTS_H
#define IMXDPUV1_EVENTS_H

/* Shadow load (Blit Engine) */
#define IMXDPUV1_STORE9_SHDLOAD_IRQ	0U
#define IMXDPUV1_STORE9_SHDLOAD_CMD	0U

/* Frame complete (Blit Engine) */
#define IMXDPUV1_STORE9_FRAMECOMPLETE_IRQ	1U
#define IMXDPUV1_STORE9_FRAMECOMPLETE_CMD	1U

/* Sequence complete (Blit Engine) */
#define IMXDPUV1_STORE9_SEQCOMPLETE_IRQ	2U
#define IMXDPUV1_STORE9_SEQCOMPLETE_CMD	2U

/* Shadow load (Display Controller Content Stream 0) */
#define IMXDPUV1_EXTDST0_SHDLOAD_IRQ	3U
#define IMXDPUV1_EXTDST0_SHDLOAD_CMD	3U

/* Frame complete (Display Controller Content Stream 0) */
#define IMXDPUV1_EXTDST0_FRAMECOMPLETE_IRQ	4U
#define IMXDPUV1_EXTDST0_FRAMECOMPLETE_CMD	4U

/* Sequence complete (Display Controller Content Stream 0) */
#define IMXDPUV1_EXTDST0_SEQCOMPLETE_IRQ	5U
#define IMXDPUV1_EXTDST0_SEQCOMPLETE_CMD	5U

/* Shadow load (Display Controller Safety Stream 0) */
#define IMXDPUV1_EXTDST4_SHDLOAD_IRQ	6U
#define IMXDPUV1_EXTDST4_SHDLOAD_CMD	6U

/* Frame complete (Display Controller Safety Stream 0) */
#define IMXDPUV1_EXTDST4_FRAMECOMPLETE_IRQ	7U
#define IMXDPUV1_EXTDST4_FRAMECOMPLETE_CMD	7U

/* Sequence complete (Display Controller Safety Stream 0) */
#define IMXDPUV1_EXTDST4_SEQCOMPLETE_IRQ	8U
#define IMXDPUV1_EXTDST4_SEQCOMPLETE_CMD	8U

/* Shadow load (Display Controller Content Stream 1) */
#define IMXDPUV1_EXTDST1_SHDLOAD_IRQ	9U
#define IMXDPUV1_EXTDST1_SHDLOAD_CMD	9U

/* Frame complete (Display Controller Content Stream 1) */
#define IMXDPUV1_EXTDST1_FRAMECOMPLETE_IRQ	10U
#define IMXDPUV1_EXTDST1_FRAMECOMPLETE_CMD	10U

/* Sequence complete (Display Controller Content Stream 1) */
#define IMXDPUV1_EXTDST1_SEQCOMPLETE_IRQ	11U
#define IMXDPUV1_EXTDST1_SEQCOMPLETE_CMD	11U

/* Shadow load (Display Controller Safety Stream 1) */
#define IMXDPUV1_EXTDST5_SHDLOAD_IRQ	12U
#define IMXDPUV1_EXTDST5_SHDLOAD_CMD	12U

/* Frame complete (Display Controller Safety Stream 1) */
#define IMXDPUV1_EXTDST5_FRAMECOMPLETE_IRQ	13U
#define IMXDPUV1_EXTDST5_FRAMECOMPLETE_CMD	13U

/* Sequence complete (Display Controller Safety Stream 1) */
#define IMXDPUV1_EXTDST5_SEQCOMPLETE_IRQ	14U
#define IMXDPUV1_EXTDST5_SEQCOMPLETE_CMD	14U

/* Shadow load (Display Controller Display Stream 0) */
#define IMXDPUV1_DISENGCFG_SHDLOAD0_IRQ	15U
#define IMXDPUV1_DISENGCFG_SHDLOAD0_CMD	15U

/* Frame complete (Display Controller Display Stream 0) */
#define IMXDPUV1_DISENGCFG_FRAMECOMPLETE0_IRQ	16U
#define IMXDPUV1_DISENGCFG_FRAMECOMPLETE0_CMD	16U

/* Sequence complete (Display Controller Display Stream 0) */
#define IMXDPUV1_DISENGCFG_SEQCOMPLETE0_IRQ	17U
#define IMXDPUV1_DISENGCFG_SEQCOMPLETE0_CMD	17U

/* Programmable interrupt 0 (Display Controller Display Stream 0 FrameGen #0 unit) */
#define IMXDPUV1_FRAMEGEN0_INT0_IRQ	18U
#define IMXDPUV1_FRAMEGEN0_INT0_CMD	18U

/* Programmable interrupt 1 (Display Controller Display Stream 0 FrameGen #0 unit) */
#define IMXDPUV1_FRAMEGEN0_INT1_IRQ	19U
#define IMXDPUV1_FRAMEGEN0_INT1_CMD	19U

/* Programmable interrupt 2 (Display Controller Display Stream 0 FrameGen #0 unit) */
#define IMXDPUV1_FRAMEGEN0_INT2_IRQ	20U
#define IMXDPUV1_FRAMEGEN0_INT2_CMD	20U

/* Programmable interrupt 3 (Display Controller Display Stream 0 FrameGen #0 unit) */
#define IMXDPUV1_FRAMEGEN0_INT3_IRQ	21U
#define IMXDPUV1_FRAMEGEN0_INT3_CMD	21U

/* Shadow load (Display Controller Display Stream 0 Sig #0 unit) */
#define IMXDPUV1_SIG0_SHDLOAD_IRQ	22U
#define IMXDPUV1_SIG0_SHDLOAD_CMD	22U

/* Measurement valid (Display Controller Display Stream 0 Sig #0 unit) */
#define IMXDPUV1_SIG0_VALID_IRQ	23U
#define IMXDPUV1_SIG0_VALID_CMD	23U

/* Error condition (Display Controller Display Stream 0 Sig #0 unit) */
#define IMXDPUV1_SIG0_ERROR_IRQ	24U
#define IMXDPUV1_SIG0_ERROR_CMD	24U

/* Shadow load (Display Controller Display Stream 1) */
#define IMXDPUV1_DISENGCFG_SHDLOAD1_IRQ	25U
#define IMXDPUV1_DISENGCFG_SHDLOAD1_CMD	25U

/* Frame complete (Display Controller Display Stream 1) */
#define IMXDPUV1_DISENGCFG_FRAMECOMPLETE1_IRQ	26U
#define IMXDPUV1_DISENGCFG_FRAMECOMPLETE1_CMD	26U

/* Sequence complete (Display Controller Display Stream 1) */
#define IMXDPUV1_DISENGCFG_SEQCOMPLETE1_IRQ	27U
#define IMXDPUV1_DISENGCFG_SEQCOMPLETE1_CMD	27U

/* Programmable interrupt 0 (Display Controller Display Stream 1 FrameGen #1 unit) */
#define IMXDPUV1_FRAMEGEN1_INT0_IRQ	28U
#define IMXDPUV1_FRAMEGEN1_INT0_CMD	28U

/* Programmable interrupt 1 (Display Controller Display Stream 1 FrameGen #1 unit) */
#define IMXDPUV1_FRAMEGEN1_INT1_IRQ	29U
#define IMXDPUV1_FRAMEGEN1_INT1_CMD	29U

/* Programmable interrupt 2 (Display Controller Display Stream 1 FrameGen #1 unit) */
#define IMXDPUV1_FRAMEGEN1_INT2_IRQ	30U
#define IMXDPUV1_FRAMEGEN1_INT2_CMD	30U

/* Programmable interrupt 3 (Display Controller Display Stream 1 FrameGen #1 unit) */
#define IMXDPUV1_FRAMEGEN1_INT3_IRQ	31U
#define IMXDPUV1_FRAMEGEN1_INT3_CMD	31U

/* Shadow load (Display Controller Display Stream 1 Sig #1 unit) */
#define IMXDPUV1_SIG1_SHDLOAD_IRQ	32U
#define IMXDPUV1_SIG1_SHDLOAD_CMD	32U

/* Measurement valid (Display Controller Display Stream 1 Sig #1 unit) */
#define IMXDPUV1_SIG1_VALID_IRQ	33U
#define IMXDPUV1_SIG1_VALID_CMD	33U

/* Error condition (Display Controller Display Stream 1 Sig #1 unit) */
#define IMXDPUV1_SIG1_ERROR_IRQ	34U
#define IMXDPUV1_SIG1_ERROR_CMD	34U

/* Reserved Do not use */
#define IMXDPUV1_RESERVED35_IRQ	35U
#define IMXDPUV1_RESERVED35_CMD	35U

/* Error condition (Command Sequencer) */
#define IMXDPUV1_CMDSEQ_ERROR_IRQ	36U
#define IMXDPUV1_CMDSEQ_ERROR_CMD	36U

/* Software interrupt 0 (Common Control) */
#define IMXDPUV1_COMCTRL_SW0_IRQ	37U
#define IMXDPUV1_COMCTRL_SW0_CMD	37U

/* Software interrupt 1 (Common Control) */
#define IMXDPUV1_COMCTRL_SW1_IRQ	38U
#define IMXDPUV1_COMCTRL_SW1_CMD	38U

/* Software interrupt 2 (Common Control) */
#define IMXDPUV1_COMCTRL_SW2_IRQ	39U
#define IMXDPUV1_COMCTRL_SW2_CMD	39U

/* Software interrupt 3 (Common Control) */
#define IMXDPUV1_COMCTRL_SW3_IRQ	40U
#define IMXDPUV1_COMCTRL_SW3_CMD	40U

/* Synchronization status activated (Display Controller Safety stream 0) */
#define IMXDPUV1_FRAMEGEN0_PRIMSYNC_ON_IRQ	41U
#define IMXDPUV1_FRAMEGEN0_PRIMSYNC_ON_CMD	41U

/* Synchronization status deactivated (Display Controller Safety stream 0) */
#define IMXDPUV1_FRAMEGEN0_PRIMSYNC_OFF_IRQ	42U
#define IMXDPUV1_FRAMEGEN0_PRIMSYNC_OFF_CMD	42U

/* Synchronization status activated (Display Controller Content stream 0) */
#define IMXDPUV1_FRAMEGEN0_SECSYNC_ON_IRQ	43U
#define IMXDPUV1_FRAMEGEN0_SECSYNC_ON_CMD	43U

/* Synchronization status deactivated (Display Controller Content stream 0) */
#define IMXDPUV1_FRAMEGEN0_SECSYNC_OFF_IRQ	44U
#define IMXDPUV1_FRAMEGEN0_SECSYNC_OFF_CMD	44U

/* Synchronization status activated (Display Controller Safety stream 1) */
#define IMXDPUV1_FRAMEGEN1_PRIMSYNC_ON_IRQ	45U
#define IMXDPUV1_FRAMEGEN1_PRIMSYNC_ON_CMD	45U

/* Synchronization status deactivated (Display Controller Safety stream 1) */
#define IMXDPUV1_FRAMEGEN1_PRIMSYNC_OFF_IRQ	46U
#define IMXDPUV1_FRAMEGEN1_PRIMSYNC_OFF_CMD	46U

/* Synchronization status activated (Display Controller Content stream 1) */
#define IMXDPUV1_FRAMEGEN1_SECSYNC_ON_IRQ	47U
#define IMXDPUV1_FRAMEGEN1_SECSYNC_ON_CMD	47U

/* Synchronization status deactivated (Display Controller Content stream 1) */
#define IMXDPUV1_FRAMEGEN1_SECSYNC_OFF_IRQ	48U
#define IMXDPUV1_FRAMEGEN1_SECSYNC_OFF_CMD	48U

/* Synchronization status (Display Controller Safety stream 0) */
#define IMXDPUV1_FRAMEGEN0_PRIMSYNC_CMD	49U
#define IMXDPUV1_FRAMEGEN0_PRIMSYNC_STS	0U

/* Synchronization status (Display Controller Content stream 0) */
#define IMXDPUV1_FRAMEGEN0_SECSYNC_CMD	50U
#define IMXDPUV1_FRAMEGEN0_SECSYNC_STS	1U

/* Synchronization status (Display Controller Safety stream 1) */
#define IMXDPUV1_FRAMEGEN1_PRIMSYNC_CMD	51U
#define IMXDPUV1_FRAMEGEN1_PRIMSYNC_STS	2U

/* Synchronization status (Display Controller Content stream 1) */
#define IMXDPUV1_FRAMEGEN1_SECSYNC_CMD	52U
#define IMXDPUV1_FRAMEGEN1_SECSYNC_STS	3U

/* Shadow load request (Display Controller Pixel Engine configuration Store #9 synchronizer) */
#define IMXDPUV1_PIXENGCFG_STORE9_SHDLDREQ_CMD	53U

/* Shadow load request (Display Controller Pixel Engine configuration ExtDst #0 synchronizer) */
#define IMXDPUV1_PIXENGCFG_EXTDST0_SHDLDREQ_CMD	54U

/* Shadow load request (Display Controller Pixel Engine configuration ExtDst #4 synchronizer) */
#define IMXDPUV1_PIXENGCFG_EXTDST4_SHDLDREQ_CMD	55U

/* Shadow load request (Display Controller Pixel Engine configuration ExtDst #1 synchronizer) */
#define IMXDPUV1_PIXENGCFG_EXTDST1_SHDLDREQ_CMD	56U

/* Shadow load request (Display Controller Pixel Engine configuration ExtDst #5 synchronizer) */
#define IMXDPUV1_PIXENGCFG_EXTDST5_SHDLDREQ_CMD	57U

/* Shadow load request (Blit Engine FetchDecode #9 tree) */
#define IMXDPUV1_PIXENGCFG_FETCHDECODE9_SHDLDREQ_CMD	58U

/* Shadow load request (Blit Engine FetchWarp #9 tree) */
#define IMXDPUV1_PIXENGCFG_FETCHWARP9_SHDLDREQ_CMD	59U

/* Shadow load request (Blit Engine FetchEco #9 tree) */
#define IMXDPUV1_PIXENGCFG_FETCHECO9_SHDLDREQ_CMD	60U

/* Shadow load request (Display Controller ConstFrame #0 tree) */
#define IMXDPUV1_PIXENGCFG_CONSTFRAME0_SHDLDREQ_CMD	61U

/* Shadow load request (Display Controller ConstFrame #4 tree) */
#define IMXDPUV1_PIXENGCFG_CONSTFRAME4_SHDLDREQ_CMD	62U

/* Shadow load request (Display Controller ConstFrame #1 tree) */
#define IMXDPUV1_PIXENGCFG_CONSTFRAME1_SHDLDREQ_CMD	63U

/* Shadow load request (Display Controller ConstFrame #5 tree) */
#define IMXDPUV1_PIXENGCFG_CONSTFRAME5_SHDLDREQ_CMD	64U

/* Shadow load request (Display Controller FetchWarp #2 tree) */
#define IMXDPUV1_PIXENGCFG_FETCHWARP2_SHDLDREQ_CMD	65U

/* Shadow load request (Display Controller FetchEco #2 tree) */
#define IMXDPUV1_PIXENGCFG_FETCHECO2_SHDLDREQ_CMD	66U

/* Shadow load request (Display Controller FetchDecode #0 tree) */
#define IMXDPUV1_PIXENGCFG_FETCHDECODE0_SHDLDREQ_CMD	67U

/* Shadow load request (Display Controller FetchEco #0 tree) */
#define IMXDPUV1_PIXENGCFG_FETCHECO0_SHDLDREQ_CMD	68U

/* Shadow load request (Display Controller FetchDecode #1 tree) */
#define IMXDPUV1_PIXENGCFG_FETCHDECODE1_SHDLDREQ_CMD	69U

/* Shadow load request (Display Controller FetchEco #1 tree) */
#define IMXDPUV1_PIXENGCFG_FETCHECO1_SHDLDREQ_CMD	70U

/* Shadow load request (Display Controller FetchLayer #0 tree) */
#define IMXDPUV1_PIXENGCFG_FETCHLAYER0_SHDLDREQ_CMD	71U

/* Shadow load request (Blit Engine FetchWarp #9 unit Layer 0) */
#define IMXDPUV1_FETCHWARP9_SHDLDREQ0_CMD	72U

/* Shadow load request (Blit Engine FetchWarp #9 unit Layer 1) */
#define IMXDPUV1_FETCHWARP9_SHDLDREQ1_CMD	73U

/* Shadow load request (Blit Engine FetchWarp #9 unit Layer 2) */
#define IMXDPUV1_FETCHWARP9_SHDLDREQ2_CMD	74U

/* Shadow load request (Blit Engine FetchWarp #9 unit Layer 3) */
#define IMXDPUV1_FETCHWARP9_SHDLDREQ3_CMD	75U

/* Shadow load request (Blit Engine FetchWarp #9 unit Layer 4) */
#define IMXDPUV1_FETCHWARP9_SHDLDREQ4_CMD	76U

/* Shadow load request (Blit Engine FetchWarp #9 unit Layer 5) */
#define IMXDPUV1_FETCHWARP9_SHDLDREQ5_CMD	77U

/* Shadow load request (Blit Engine FetchWarp #9 unit Layer 6) */
#define IMXDPUV1_FETCHWARP9_SHDLDREQ6_CMD	78U

/* Shadow load request (Blit Engine FetchWarp #9 unit Layer 7) */
#define IMXDPUV1_FETCHWARP9_SHDLDREQ7_CMD	79U

/* Shadow load request (Display Controller FetchWarp #2 unit Layer 0) */
#define IMXDPUV1_FETCHWARP2_SHDLDREQ0_CMD	80U

/* Shadow load request (Display Controller FetchWarp #2 unit Layer 1) */
#define IMXDPUV1_FETCHWARP2_SHDLDREQ1_CMD	81U

/* Shadow load request (Display Controller FetchWarp #2 unit Layer 2) */
#define IMXDPUV1_FETCHWARP2_SHDLDREQ2_CMD	82U

/* Shadow load request (Display Controller FetchWarp #2 unit Layer 3) */
#define IMXDPUV1_FETCHWARP2_SHDLDREQ3_CMD	83U

/* Shadow load request (Display Controller FetchWarp #2 unit Layer 4) */
#define IMXDPUV1_FETCHWARP2_SHDLDREQ4_CMD	84U

/* Shadow load request (Display Controller FetchWarp #2 unit Layer 5) */
#define IMXDPUV1_FETCHWARP2_SHDLDREQ5_CMD	85U

/* Shadow load request (Display Controller FetchWarp #2 unit Layer 6) */
#define IMXDPUV1_FETCHWARP2_SHDLDREQ6_CMD	86U

/* Shadow load request (Display Controller FetchWarp #2 unit Layer 7) */
#define IMXDPUV1_FETCHWARP2_SHDLDREQ7_CMD	87U

/* Shadow load request (Display Controller FetchLayer #0 unit Layer 0) */
#define IMXDPUV1_FETCHLAYER0_SHDLDREQ0_CMD	88U

/* Shadow load request (Display Controller FetchLayer #0 unit Layer 1) */
#define IMXDPUV1_FETCHLAYER0_SHDLDREQ1_CMD	89U

/* Shadow load request (Display Controller FetchLayer #0 unit Layer 2) */
#define IMXDPUV1_FETCHLAYER0_SHDLDREQ2_CMD	90U

/* Shadow load request (Display Controller FetchLayer #0 unit Layer 3) */
#define IMXDPUV1_FETCHLAYER0_SHDLDREQ3_CMD	91U

/* Shadow load request (Display Controller FetchLayer #0 unit Layer 4) */
#define IMXDPUV1_FETCHLAYER0_SHDLDREQ4_CMD	92U

/* Shadow load request (Display Controller FetchLayer #0 unit Layer 5) */
#define IMXDPUV1_FETCHLAYER0_SHDLDREQ5_CMD	93U

/* Shadow load request (Display Controller FetchLayer #0 unit Layer 6) */
#define IMXDPUV1_FETCHLAYER0_SHDLDREQ6_CMD	94U

/* Shadow load request (Display Controller FetchLayer #0 unit Layer 7) */
#define IMXDPUV1_FETCHLAYER0_SHDLDREQ7_CMD	95U


#endif /* IMXDPUV1_EVENTS */
