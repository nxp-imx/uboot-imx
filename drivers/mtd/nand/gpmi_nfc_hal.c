/*
 * Freescale GPMI NFC NAND Flash Driver
 *
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
 * Copyright (C) 2008 Embedded Alley Solutions, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/mtd/mtd.h>
#include "gpmi_nfc_gpmi.h"
#include "gpmi_nfc_bch.h"
#include <linux/mtd/nand.h>
#include <linux/types.h>
#include <asm/apbh_dma.h>
#include <asm/io.h>
#include <common.h>

#ifdef CONFIG_ARCH_MMU
#include <asm/arch/mmu.h>
#endif

#define MIN_PROP_DELAY_IN_NS	(5)
#define MAX_PROP_DELAY_IN_NS	(9)

#define NFC_DMA_DESCRIPTOR_COUNT	(4)

static struct mxs_dma_desc *dma_desc[NFC_DMA_DESCRIPTOR_COUNT];

static struct gpmi_nfc_timing  safe_timing = {
	.m_u8DataSetup		= 80,
	.m_u8DataHold		= 60,
	.m_u8AddressSetup		= 25,
	.m_u8HalfPeriods		= 0,
	.m_u8SampleDelay		= 6,
	.m_u8NandTimingState	= 0,
	.m_u8tREA		= -1,
	.m_u8tRLOH		= -1,
	.m_u8tRHOH		= -1,
};

/**
 * init() - Initializes the NFC hardware.
 *
 * @this:  Per-device data.
 */
static int init(void)
{
	int error = 0, i;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	/* Initialize DMA. */
	MTDDEBUG(MTD_DEBUG_LEVEL1, "dma_desc: 0x%08x, ",
		(unsigned int)dma_desc);
	for (i = 0; i < NFC_DMA_DESCRIPTOR_COUNT; ++i) {
		dma_desc[i] = mxs_dma_alloc_desc();

		if (NULL == dma_desc[i]) {
			for (i -= 1; i >= 0; --i)
				mxs_dma_free_desc(dma_desc[i]);
			error = -ENOMEM;
		}
		MTDDEBUG(MTD_DEBUG_LEVEL1,
			"dma_desc[%d]: 0x%08x, ",
			i, (unsigned int)dma_desc[i]);
	}
	MTDDEBUG(MTD_DEBUG_LEVEL1, "\n");

	if (error)
		return error;

	mxs_dma_init();

	/* Reset the GPMI block. */
	gpmi_nfc_reset_block((void *)(CONFIG_GPMI_REG_BASE + HW_GPMI_CTRL0), 1);

	/* Choose NAND mode. */
	REG_CLR(CONFIG_GPMI_REG_BASE, HW_GPMI_CTRL1,
		BM_GPMI_CTRL1_GPMI_MODE);

	/* Set the IRQ polarity. */
	REG_SET(CONFIG_GPMI_REG_BASE, HW_GPMI_CTRL1,
		BM_GPMI_CTRL1_ATA_IRQRDY_POLARITY);

	/* Disable write protection. */
	REG_SET(CONFIG_GPMI_REG_BASE, HW_GPMI_CTRL1,
		BM_GPMI_CTRL1_DEV_RESET);

	/* Select BCH ECC. */
	REG_SET(CONFIG_GPMI_REG_BASE, HW_GPMI_CTRL1,
		BM_GPMI_CTRL1_BCH_MODE);

	memcpy(&gpmi_nfc_hal.timing, &safe_timing,
		sizeof(struct gpmi_nfc_timing));

	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);
	return 0;
}

/**
 * set_geometry() - Configures the NFC geometry.
 *
 * @this:  Per-device data.
 */
static int set_geometry(struct mtd_info *mtd)
{
	u32 block_count;
	u32 block_size;
	u32 metadata_size;
	u32 ecc_strength;
	u32 page_size;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	/* Translate the abstract choices into register fields. */
	block_count = GPMI_NFC_ECC_CHUNK_CNT(mtd->writesize) - 1;
#if defined(CONFIG_GPMI_NFC_V2)
	block_size = GPMI_NFC_CHUNK_DATA_CHUNK_SIZE >> 2;
#else
	block_size = GPMI_NFC_CHUNK_DATA_CHUNK_SIZE;
#endif
	metadata_size = GPMI_NFC_METADATA_SIZE;

	ecc_strength =
		gpmi_nfc_get_ecc_strength(mtd->writesize, mtd->oobsize) >> 1;

	page_size    = mtd->writesize + mtd->oobsize;

	/*
	 * Reset the BCH block. Notice that we pass in true for the just_enable
	 * flag. This is because the soft reset for the version 0 BCH block
	 * doesn't work and the version 1 BCH block is similar enough that we
	 * suspect the same (though this has not been officially tested). If you
	 * try to soft reset a version 0 BCH block, it becomes unusable until
	 * the next hard reset.
	 */

#if defined(CONFIG_GPMI_NFC_V2)
	gpmi_nfc_reset_block((void *)CONFIG_BCH_REG_BASE + HW_BCH_CTRL, 0);
#else
	gpmi_nfc_reset_block((void *)CONFIG_BCH_REG_BASE + HW_BCH_CTRL, 1);
#endif

	/* Configure layout 0. */
	writel(BF_BCH_FLASH0LAYOUT0_NBLOCKS(block_count)     |
		BF_BCH_FLASH0LAYOUT0_META_SIZE(metadata_size) |
		BF_BCH_FLASH0LAYOUT0_ECC0(ecc_strength)       |
		BF_BCH_FLASH0LAYOUT0_DATA0_SIZE(block_size),
		CONFIG_BCH_REG_BASE + HW_BCH_FLASH0LAYOUT0);

	writel(BF_BCH_FLASH0LAYOUT1_PAGE_SIZE(page_size)   |
		BF_BCH_FLASH0LAYOUT1_ECCN(ecc_strength)     |
		BF_BCH_FLASH0LAYOUT1_DATAN_SIZE(block_size),
		CONFIG_BCH_REG_BASE + HW_BCH_FLASH0LAYOUT1);

	/* Set *all* chip selects to use layout 0. */
	writel(0, CONFIG_BCH_REG_BASE + HW_BCH_LAYOUTSELECT);

	/* Enable interrupts. */
	REG_SET(CONFIG_BCH_REG_BASE, HW_BCH_CTRL,
		BM_BCH_CTRL_COMPLETE_IRQ_EN);

	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);
	return 0;
}

/**
 * ns_to_cycles - Converts time in nanoseconds to cycles.
 *
 * @ntime:   The time, in nanoseconds.
 * @period:  The cycle period, in nanoseconds.
 * @min:     The minimum allowable number of cycles.
 */
static u32 ns_to_cycles(u32 time, u32 period, u32 min)
{
	u32 k;

	/*
	 * Compute the minimum number of cycles that entirely contain the
	 * given time.
	 */
	k = (time + period - 1) / period;

	return max(k, min);
}

static int calculte_hw_timing(struct mtd_info *mtd,
				struct gpmi_nfc_timing *nfc_timing,
				struct gpmi_nfc_timing *hw_timing)
{
	struct nand_chip *chip = mtd->priv;
	struct gpmi_nfc_info *gpmi_info = chip->priv;
	struct nfc_hal         *nfc =  gpmi_info->nfc;

	u8  improved_timing_is_available;
	u32 clock_frequency_in_hz;
	u32 clock_period_in_ns;
	u8  dll_use_half_periods;
	u32 dll_delay_shift;
	u32 max_sample_delay_in_ns;
	u32 address_setup_in_cycles;
	u32 data_setup_in_ns;
	u32 data_setup_in_cycles;
	u32 data_hold_in_cycles;
	s32 ideal_sample_delay_in_ns;
	u32 sample_delay_factor;
	s32 tEYE;
	u32 min_prop_delay_in_ns = MIN_PROP_DELAY_IN_NS;
	u32 max_prop_delay_in_ns = MAX_PROP_DELAY_IN_NS;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	/*
	 * If there are multiple chips, we need to relax the timings to allow
	 * for signal distortion due to higher capacitance.
	 */
	if (chip->numchips > 2) {
		nfc_timing->m_u8DataSetup    += 10;
		nfc_timing->m_u8DataHold     += 10;
		nfc_timing->m_u8AddressSetup += 10;
	} else {
		nfc_timing->m_u8DataSetup    += 5;
		nfc_timing->m_u8DataHold     += 5;
		nfc_timing->m_u8AddressSetup += 5;
	}

	/* Check if improved timing information is available. */
	improved_timing_is_available =
		(nfc_timing->m_u8tREA  >= 0) &&
		(nfc_timing->m_u8tRLOH >= 0) &&
		(nfc_timing->m_u8tRHOH >= 0) ;

	/* Inspect the clock. */
	clock_frequency_in_hz = mxc_get_clock(MXC_GPMI_CLK);
	clock_period_in_ns    = 1000000000 / clock_frequency_in_hz;

	/*
	 * The NFC quantizes setup and hold parameters in terms of clock cycles.
	 * Here, we quantize the setup and hold timing parameters to the
	 * next-highest clock period to make sure we apply at least the
	 * specified times.
	 *
	 * For data setup and data hold, the hardware interprets a value of zero
	 * as the largest possible delay. This is not what's intended by a zero
	 * in the input parameter, so we impose a minimum of one cycle.
	 */
	data_setup_in_cycles    = ns_to_cycles(nfc_timing->m_u8DataSetup,
						clock_period_in_ns, 1);
	data_hold_in_cycles     = ns_to_cycles(nfc_timing->m_u8DataHold,
						clock_period_in_ns, 1);
	address_setup_in_cycles = ns_to_cycles(nfc_timing->m_u8AddressSetup,
						clock_period_in_ns, 0);

	/*
	 * The clock's period affects the sample delay in a number of ways:
	 *
	 * (1) The NFC HAL tells us the maximum clock period the sample delay
	 *     DLL can tolerate. If the clock period is greater than half that
	 *     maximum, we must configure the DLL to be driven by half periods.
	 *
	 * (2) We need to convert from an ideal sample delay, in ns, to a
	 *     "sample delay factor," which the NFC uses. This factor depends on
	 *     whether we're driving the DLL with full or half periods.
	 *     Paraphrasing the reference manual:
	 *
	 *         AD = SDF x 0.125 x RP
	 *
	 * where:
	 *
	 *     AD   is the applied delay, in ns.
	 *     SDF  is the sample delay factor, which is dimensionless.
	 *     RP   is the reference period, in ns, which is a full clock period
	 *          if the DLL is being driven by full periods, or half that if
	 *          the DLL is being driven by half periods.
	 *
	 * Let's re-arrange this in a way that's more useful to us:
	 *
	 *                        8
	 *         SDF  =  AD x ----
	 *                       RP
	 *
	 * The reference period is either the clock period or half that, so this
	 * is:
	 *
	 *                        8       AD x DDF
	 *         SDF  =  AD x -----  =  --------
	 *                      f x P        P
	 *
	 * where:
	 *
	 *       f  is 1 or 1/2, depending on how we're driving the DLL.
	 *       P  is the clock period.
	 *     DDF  is the DLL Delay Factor, a dimensionless value that
	 *          incorporates all the constants in the conversion.
	 *
	 * DDF will be either 8 or 16, both of which are powers of two. We can
	 * reduce the cost of this conversion by using bit shifts instead of
	 * multiplication or division. Thus:
	 *
	 *                 AD << DDS
	 *         SDF  =  ---------
	 *                     P
	 *
	 *     or
	 *
	 *         AD  =  (SDF >> DDS) x P
	 *
	 * where:
	 *
	 *     DDS  is the DLL Delay Shift, the logarithm to base 2 of the DDF.
	 */
	if (clock_period_in_ns > (nfc->max_dll_clock_period_in_ns >> 1)) {
		dll_use_half_periods = 0;
		dll_delay_shift      = 3 + 1;
	} else {
		dll_use_half_periods = 1;
		dll_delay_shift      = 3;
	}

	/*
	 * Compute the maximum sample delay the NFC allows, under current
	 * conditions. If the clock is running too slowly, no sample delay is
	 * possible.
	 */
	if (clock_period_in_ns > nfc->max_dll_clock_period_in_ns)
		max_sample_delay_in_ns = 0;

	else {

		/*
		 * Compute the delay implied by the largest sample delay factor
		 * the NFC allows.
		 */

		max_sample_delay_in_ns =
			(nfc->max_sample_delay_factor * clock_period_in_ns) >>
								dll_delay_shift;

		/*
		 * Check if the implied sample delay larger than the NFC
		 * actually allows.
		 */

		if (max_sample_delay_in_ns > nfc->max_dll_delay_in_ns)
			max_sample_delay_in_ns = nfc->max_dll_delay_in_ns;

	}

	/*
	 * Check if improved timing information is available. If not, we have to
	 * use a less-sophisticated algorithm.
	 */

	if (!improved_timing_is_available) {

		/*
		 * Fold the read setup time required by the NFC into the ideal
		 * sample delay.
		 */

		ideal_sample_delay_in_ns = nfc_timing->m_u8SampleDelay +
						nfc->internal_data_setup_in_ns;

		/*
		 * The ideal sample delay may be greater than the maximum
		 * allowed by the NFC. If so, we can trade off sample delay time
		 * for more data setup time.
		 *
		 * In each iteration of the following loop, we add a cycle to
		 * the data setup time and subtract a corresponding amount from
		 * the sample delay until we've satisified the constraints or
		 * can't do any better.
		 */

		while ((ideal_sample_delay_in_ns > max_sample_delay_in_ns) &&
			(data_setup_in_cycles < nfc->max_data_setup_cycles)) {

			data_setup_in_cycles++;
			ideal_sample_delay_in_ns -= clock_period_in_ns;

			if (ideal_sample_delay_in_ns < 0)
				ideal_sample_delay_in_ns = 0;
		}

		/*
		 * Compute the sample delay factor that corresponds most closely
		 * to the ideal sample delay. If the result is too large for the
		 * NFC, use the maximum value.
		 *
		 * Notice that we use the ns_to_cycles function to compute the
		 * sample delay factor. We do this because the form of the
		 * computation is the same as that for calculating cycles.
		 */
		sample_delay_factor =
			ns_to_cycles(
				ideal_sample_delay_in_ns << dll_delay_shift,
							clock_period_in_ns, 0);

		if (sample_delay_factor > nfc->max_sample_delay_factor)
			sample_delay_factor = nfc->max_sample_delay_factor;

		/* Skip to the part where we return our results. */
		goto rtn_rslt;
	}

	/*
	 * If control arrives here, we have more detailed timing information,
	 * so we can use a better algorithm.
	 */

	/*
	 * Fold the read setup time required by the NFC into the maximum
	 * propagation delay.
	 */
	max_prop_delay_in_ns += nfc->internal_data_setup_in_ns;

	/*
	 * Earlier, we computed the number of clock cycles required to satisfy
	 * the data setup time. Now, we need to know the actual nanoseconds.
	 */
	data_setup_in_ns = clock_period_in_ns * data_setup_in_cycles;

	/*
	 * Compute tEYE, the width of the data eye when reading from the NAND
	 * Flash. The eye width is fundamentally determined by the data setup
	 * time, perturbed by propagation delays and some characteristics of the
	 * NAND Flash device.
	 *
	 * start of the eye = max_prop_delay + tREA
	 * end of the eye   = min_prop_delay + tRHOH + data_setup
	 */

	tEYE = (int)min_prop_delay_in_ns + (int)nfc_timing->m_u8tRHOH+
							(int)data_setup_in_ns;

	tEYE -= (int)max_prop_delay_in_ns + (int)nfc_timing->m_u8tREA;

	/*
	 * The eye must be open. If it's not, we can try to open it by
	 * increasing its main forcer, the data setup time.
	 *
	 * In each iteration of the following loop, we increase the data setup
	 * time by a single clock cycle. We do this until either the eye is
	 * open or we run into NFC limits.
	 */
	while ((tEYE <= 0) &&
			(data_setup_in_cycles < nfc->max_data_setup_cycles)) {
		/* Give a cycle to data setup. */
		data_setup_in_cycles++;
		/* Synchronize the data setup time with the cycles. */
		data_setup_in_ns += clock_period_in_ns;
		/* Adjust tEYE accordingly. */
		tEYE += clock_period_in_ns;
	}

	/*
	 * When control arrives here, the eye is open. The ideal time to sample
	 * the data is in the center of the eye:
	 *
	 *     end of the eye + start of the eye
	 *     ---------------------------------  -  data_setup
	 *                    2
	 *
	 * After some algebra, this simplifies to the code immediately below.
	 */

	ideal_sample_delay_in_ns =
		((int)max_prop_delay_in_ns +
			(int)nfc_timing->m_u8tREA+
				(int)min_prop_delay_in_ns +
					(int)nfc_timing->m_u8tRHOH-
						(int)data_setup_in_ns) >> 1;

	/*
	 * The following figure illustrates some aspects of a NAND Flash read:
	 *
	 *
	 *           __                   _____________________________________
	 * RDN         \_________________/
	 *
	 *                                         <---- tEYE ----->
	 *                                        /-----------------\
	 * Read Data ----------------------------<                   >---------
	 *                                        \-----------------/
	 *             ^                 ^                 ^              ^
	 *             |                 |                 |              |
	 *             |<--Data Setup -->|<--Delay Time -->|              |
	 *             |                 |                 |              |
	 *             |                 |                                |
	 *             |                 |<--   Quantized Delay Time   -->|
	 *             |                 |                                |
	 *
	 *
	 * We have some issues we must now address:
	 *
	 * (1) The *ideal* sample delay time must not be negative. If it is, we
	 *     jam it to zero.
	 *
	 * (2) The *ideal* sample delay time must not be greater than that
	 *     allowed by the NFC. If it is, we can increase the data setup
	 *     time, which will reduce the delay between the end of the data
	 *     setup and the center of the eye. It will also make the eye
	 *     larger, which might help with the next issue...
	 *
	 * (3) The *quantized* sample delay time must not fall either before the
	 *     eye opens or after it closes (the latter is the problem
	 *     illustrated in the above figure).
	 */

	/* Jam a negative ideal sample delay to zero. */
	if (ideal_sample_delay_in_ns < 0)
		ideal_sample_delay_in_ns = 0;

	/*
	 * Extend the data setup as needed to reduce the ideal sample delay
	 * below the maximum permitted by the NFC.
	 */
	while ((ideal_sample_delay_in_ns > max_sample_delay_in_ns) &&
			(data_setup_in_cycles < nfc->max_data_setup_cycles)) {

		/* Give a cycle to data setup. */
		data_setup_in_cycles++;
		/* Synchronize the data setup time with the cycles. */
		data_setup_in_ns += clock_period_in_ns;
		/* Adjust tEYE accordingly. */
		tEYE += clock_period_in_ns;

		/*
		 * Decrease the ideal sample delay by one half cycle, to keep it
		 * in the middle of the eye.
		 */
		ideal_sample_delay_in_ns -= (clock_period_in_ns >> 1);

		/* Jam a negative ideal sample delay to zero. */
		if (ideal_sample_delay_in_ns < 0)
			ideal_sample_delay_in_ns = 0;

	}

	/*
	 * Compute the sample delay factor that corresponds to the ideal sample
	 * delay. If the result is too large, then use the maximum allowed
	 * value.
	 *
	 * Notice that we use the ns_to_cycles function to compute the sample
	 * delay factor. We do this because the form of the computation is the
	 * same as that for calculating cycles.
	 */
	sample_delay_factor =
		ns_to_cycles(ideal_sample_delay_in_ns << dll_delay_shift,
							clock_period_in_ns, 0);

	if (sample_delay_factor > nfc->max_sample_delay_factor)
		sample_delay_factor = nfc->max_sample_delay_factor;

	/*
	 * These macros conveniently encapsulate a computation we'll use to
	 * continuously evaluate whether or not the data sample delay is inside
	 * the eye.
	 */
	#define IDEAL_DELAY  ((int)ideal_sample_delay_in_ns)

	#define QUANTIZED_DELAY  \
		((int) ((sample_delay_factor * clock_period_in_ns) >> \
							dll_delay_shift))

	#define DELAY_ERROR  (abs(QUANTIZED_DELAY - IDEAL_DELAY))

	#define SAMPLE_IS_NOT_WITHIN_THE_EYE  (DELAY_ERROR > (tEYE >> 1))

	/*
	 * While the quantized sample time falls outside the eye, reduce the
	 * sample delay or extend the data setup to move the sampling point back
	 * toward the eye. Do not allow the number of data setup cycles to
	 * exceed the maximum allowed by the NFC.
	 */
	while (SAMPLE_IS_NOT_WITHIN_THE_EYE &&
		(data_setup_in_cycles < nfc->max_data_setup_cycles)) {

		/*
		 * If control arrives here, the quantized sample delay falls
		 * outside the eye. Check if it's before the eye opens, or after
		 * the eye closes.
		 */

		if (QUANTIZED_DELAY > IDEAL_DELAY) {
			/*
			 * If control arrives here, the quantized sample delay
			 * falls after the eye closes. Decrease the quantized
			 * delay time and then go back to re-evaluate.
			 */
			if (sample_delay_factor != 0)
				sample_delay_factor--;

			continue;

		}

		/*
		 * If control arrives here, the quantized sample delay falls
		 * before the eye opens. Shift the sample point by increasing
		 * data setup time. This will also make the eye larger.
		 */

		/* Give a cycle to data setup. */
		data_setup_in_cycles++;
		/* Synchronize the data setup time with the cycles. */
		data_setup_in_ns += clock_period_in_ns;
		/* Adjust tEYE accordingly. */
		tEYE += clock_period_in_ns;

		/*
		 * Decrease the ideal sample delay by one half cycle, to keep it
		 * in the middle of the eye.
		 */
		ideal_sample_delay_in_ns -= (clock_period_in_ns >> 1);

		/* ...and one less period for the delay time. */
		ideal_sample_delay_in_ns -= clock_period_in_ns;

		/* Jam a negative ideal sample delay to zero. */
		if (ideal_sample_delay_in_ns < 0)
			ideal_sample_delay_in_ns = 0;

		/*
		 * We have a new ideal sample delay, so re-compute the quantized
		 * delay.
		 */

		sample_delay_factor =
			ns_to_cycles(
				ideal_sample_delay_in_ns << dll_delay_shift,
							clock_period_in_ns, 0);

		if (sample_delay_factor > nfc->max_sample_delay_factor)
			sample_delay_factor = nfc->max_sample_delay_factor;

	}

	/* Control arrives here when we're ready to return our results. */

rtn_rslt:
	hw_timing->m_u8DataSetup	= data_setup_in_cycles;
	hw_timing->m_u8DataHold		= data_hold_in_cycles;
	hw_timing->m_u8AddressSetup	= address_setup_in_cycles;
	hw_timing->m_u8HalfPeriods	= dll_use_half_periods;
	hw_timing->m_u8SampleDelay	= sample_delay_factor;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);
	return 0;
}

/**
 * set_timing() - Configures the NFC timing.
 *
 * @this:    Per-device data.
 * @timing:  The timing of interest.
 */
static int set_timing(struct mtd_info *mtd,
			const struct gpmi_nfc_timing *timing)
{
	struct nand_chip *chip = mtd->priv;
	struct gpmi_nfc_info *gpmi_info = chip->priv;
	struct nfc_hal *nfc = gpmi_info->nfc;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	/* Accept the new timing. */
	nfc->timing = *timing;

	/* Return success. */
	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);
	return 0;
}

/**
 * get_timing() - Retrieves the NFC hardware timing.
 *
 * @this:                    Per-device data.
 * @clock_frequency_in_hz:   The clock frequency, in Hz, during the current
 *                           I/O transaction. If no I/O transaction is in
 *                           progress, this is the clock frequency during the
 *                           most recent I/O transaction.
 * @hardware_timing:         The hardware timing configuration in effect during
 *                           the current I/O transaction. If no I/O transaction
 *                           is in progress, this is the hardware timing
 *                           configuration during the most recent I/O
 *                           transaction.
 */
static void get_timing(struct mtd_info *mtd,
			unsigned long *clock_frequency_in_hz,
			struct gpmi_nfc_timing *hardware_timing)
{
	struct nand_chip *chip = mtd->priv;
	struct gpmi_nfc_info *gpmi_info = chip->priv;
	struct nfc_hal           *nfc       =  gpmi_info->nfc;
	u32 register_image;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	/* Return the clock frequency. */
	*clock_frequency_in_hz = nfc->clock_frequency_in_hz;

	/* Retrieve the hardware timing. */
	register_image = REG_RD(CONFIG_GPMI_REG_BASE, HW_GPMI_TIMING0);

	hardware_timing->m_u8DataSetup =
		(register_image & BM_GPMI_TIMING0_DATA_SETUP) >>
						BP_GPMI_TIMING0_DATA_SETUP;

	hardware_timing->m_u8DataHold =
		(register_image & BM_GPMI_TIMING0_DATA_HOLD) >>
						BP_GPMI_TIMING0_DATA_HOLD;

	hardware_timing->m_u8AddressSetup =
		(register_image & BM_GPMI_TIMING0_ADDRESS_SETUP) >>
						BP_GPMI_TIMING0_ADDRESS_SETUP;

	register_image = REG_RD(CONFIG_GPMI_REG_BASE, HW_GPMI_CTRL1);

	hardware_timing->m_u8HalfPeriods =
		(register_image & BM_GPMI_CTRL1_HALF_PERIOD) >>
						BP_GPMI_CTRL1_HALF_PERIOD;

	hardware_timing->m_u8SampleDelay =
		(register_image & BM_GPMI_CTRL1_RDN_DELAY) >>
						BP_GPMI_CTRL1_RDN_DELAY;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);
}

/**
 * exit() - Shuts down the NFC hardware.
 *
 * @this:  Per-device data.
 */
static void exit(struct mtd_info *mtd)
{
	int i;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	for (i = 0; i < NFC_DMA_DESCRIPTOR_COUNT; ++i)
		mxs_dma_free_desc(dma_desc[i]);

	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);
}

/**
 * begin() - Begin NFC I/O.
 *
 * @this:  Per-device data.
 */
static void begin(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	struct gpmi_nfc_info *gpmi_info = chip->priv;
	struct nfc_hal         *nfc =  gpmi_info->nfc;
	struct gpmi_nfc_timing hw_timing;
#if defined(CONFIG_GPMI_NFC_V0)
	u32 clock_period_in_ns;
	u32 register_image;
	u32 dll_wait_time_in_us;
#endif

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	/* Get the timing information we need. */
	nfc->clock_frequency_in_hz = mxc_get_clock(MXC_GPMI_CLK);
#if defined(CONFIG_GPMI_NFC_V0)
	clock_period_in_ns = 1000000000 / nfc->clock_frequency_in_hz;
#endif
	calculte_hw_timing(mtd, &(nfc->timing), &hw_timing);

#if defined(CONFIG_GPMI_NFC_V0)
	/* Set up all the simple timing parameters. */
	register_image =
		BF_GPMI_TIMING0_ADDRESS_SETUP(hw_timing.m_u8AddressSetup) |
		BF_GPMI_TIMING0_DATA_HOLD(hw_timing.m_u8DataHold)         |
		BF_GPMI_TIMING0_DATA_SETUP(hw_timing.m_u8DataSetup)       ;
	writel(register_image, CONFIG_GPMI_REG_BASE + HW_GPMI_TIMING0);

	/*
	 * HEY - PAY ATTENTION!
	 *
	 * DLL_ENABLE must be set to zero when setting RDN_DELAY or HALF_PERIOD.
	 */
	REG_CLR(CONFIG_GPMI_REG_BASE, HW_GPMI_CTRL1,
			BM_GPMI_CTRL1_DLL_ENABLE)

	/* Clear out the DLL control fields. */
	REG_CLR(CONFIG_GPMI_REG_BASE, HW_GPMI_CTRL1,
			BM_GPMI_CTRL1_RDN_DELAY);
	REG_CLR(CONFIG_GPMI_REG_BASE, HW_GPMI_CTRL1,
			BM_GPMI_CTRL1_HALF_PERIOD);

	/* If no sample delay is called for, return immediately. */
	if (!hw.sample_delay_factor)
		return;

	/* Configure the HALF_PERIOD flag. */
	if (hw.use_half_periods)
		REG_SET(CONFIG_GPMI_REG_BASE, HW_GPMI_CTRL1,
				BM_GPMI_CTRL1_HALF_PERIOD);

	/* Set the delay factor. */
	REG_SET(CONFIG_GPMI_REG_BASE, HW_GPMI_CTRL1,
			BF_GPMI_CTRL1_RDN_DELAY(hw_timing.sample_delay_factor));

	/* Enable the DLL. */
	REG_SET(CONFIG_GPMI_REG_BASE, HW_GPMI_CTRL1,
			BM_GPMI_CTRL1_DLL_ENABLE);

	/*
	 * After we enable the GPMI DLL, we have to wait 64 clock cycles before
	 * we can use the GPMI.
	 *
	 * Calculate the amount of time we need to wait, in microseconds.
	 */

	dll_wait_time_in_us = (clock_period_in_ns * 64) / 1000;

	if (!dll_wait_time_in_us)
		dll_wait_time_in_us = 1;

	/* Wait for the DLL to settle. */
	udelay(dll_wait_time_in_us);
#endif
	/* Apply the hardware timing. */

	/* Coming soon - the clock handling code isn't ready yet. */
	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);
}

/**
 * end() - End NFC I/O.
 *
 * @this:  Per-device data.
 */
static void end(struct mtd_info *mtd)
{
	/* Disable the clock. */
}

/**
 * clear_bch() - Clears a BCH interrupt.
 *
 * @this:  Per-device data.
 */
static void clear_bch(struct mtd_info *mtd)
{
	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);
	REG_CLR(CONFIG_BCH_REG_BASE, HW_BCH_CTRL,
		BM_BCH_CTRL_COMPLETE_IRQ);
	MTDDEBUG(MTD_DEBUG_LEVEL3, "<=%s\n", __func__);
}

/**
 * is_ready() - Returns the ready/busy status of the given chip.
 *
 * @this:  Per-device data.
 * @chip:  The chip of interest.
 */
static int is_ready(struct mtd_info *mtd, unsigned int target_chip)
{
	u32 mask;
	u32 register_image;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	/* Extract and return the status. */
#if defined(CONFIG_GPMI_NFC_V0)
	mask = BM_GPMI_DEBUG_READY0 << target_chip;

	register_image = REG_RD(CONFIG_GPMI_REG_BASE, HW_GPMI_DEBUG);
#else
	mask = BF_GPMI_STAT_READY_BUSY(1 << 0);

	register_image = REG_RD(CONFIG_GPMI_REG_BASE, HW_GPMI_STAT);
#endif

	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);
	return register_image & mask;
}

/**
 * send_command() - Sends a command and associated addresses.
 *
 * @this:    Per-device data.
 * @chip:    The chip of interest.
 * @buffer:  The physical address of a buffer that contains the command bytes.
 * @length:  The number of bytes in the buffer.
 */
static int send_command(struct mtd_info *mtd, unsigned chip,
			dma_addr_t buffer, unsigned int length)
{
	struct mxs_dma_desc **d = dma_desc;
	s32 dma_channel;
	s32 error;
	u32 command_mode;
	u32 address;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	MTDDEBUG(MTD_DEBUG_LEVEL2, "Chip: %d DMA Buf: 0x%08x Length: %d\n",
		chip, buffer, length);

	/* Compute the DMA channel. */
	dma_channel = MXS_DMA_CHANNEL_AHB_APBH_GPMI0 + chip;

	/* A DMA descriptor that sends out the command. */

	command_mode = BV_GPMI_CTRL0_COMMAND_MODE__WRITE;
	address      = BV_GPMI_CTRL0_ADDRESS__NAND_CLE;

	MTDDEBUG(MTD_DEBUG_LEVEL2, "1st Command: mode: %d, address: %d, ",
		command_mode, address);

	/* reset the cmd bits fieled */
	(*d)->cmd.cmd.data                   = 0;

	(*d)->cmd.cmd.bits.command           = DMA_READ;
#if defined(CONFIG_GPMI_NFC_V2)
	(*d)->cmd.cmd.bits.chain             = 0;
#else
	(*d)->cmd.cmd.bits.chain             = 1;
#endif
	(*d)->cmd.cmd.bits.irq               = 1;
	(*d)->cmd.cmd.bits.nand_lock         = 0;
	(*d)->cmd.cmd.bits.nand_wait_4_ready = 0;
	(*d)->cmd.cmd.bits.dec_sem           = 1;
	(*d)->cmd.cmd.bits.wait4end          = 1;
#if defined(CONFIG_GPMI_NFC_V2)
	(*d)->cmd.cmd.bits.halt_on_terminate = 1;
#else
	(*d)->cmd.cmd.bits.halt_on_terminate = 0;
#endif
	(*d)->cmd.cmd.bits.terminate_flush   = 0;
	(*d)->cmd.cmd.bits.pio_words         = 3;
	(*d)->cmd.cmd.bits.bytes             = length;

#ifdef CONFIG_ARCH_MMU
	(*d)->cmd.address = iomem_to_phys(buffer);
#else
	(*d)->cmd.address = buffer;
#endif

	(*d)->cmd.pio_words[0] =
		BF_GPMI_CTRL0_COMMAND_MODE(command_mode) |
		BM_GPMI_CTRL0_WORD_LENGTH                |
		BF_GPMI_CTRL0_CS(chip)                   |
		BF_GPMI_CTRL0_ADDRESS(address)           |
		BM_GPMI_CTRL0_ADDRESS_INCREMENT          |
		BF_GPMI_CTRL0_XFER_COUNT(length)         ;

	(*d)->cmd.pio_words[1] = 0;
	(*d)->cmd.pio_words[2] = 0;

	MTDDEBUG(MTD_DEBUG_LEVEL2, "PIO Words[0]: 0x%08x, "
		"PIO Words[1]: 0x%08x, PIO Words[2]: 0x%08x\n",
		(unsigned int)(*d)->cmd.pio_words[0],
		(unsigned int)(*d)->cmd.pio_words[1],
		(unsigned int)(*d)->cmd.pio_words[2]);

	mxs_dma_desc_append(dma_channel, (*d));
	d++;

	/* Go! */
	error = mxs_dma_go(dma_channel);

	if (error)
		printf("[%s] DMA error\n", __func__);

	/* Return success. */
	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);
	return error;
}

/**
 * send_data() - Sends data to the given chip.
 *
 * @this:    Per-device data.
 * @chip:    The chip of interest.
 * @buffer:  The physical address of a buffer that contains the data.
 * @length:  The number of bytes in the buffer.
 */
static int send_data(struct mtd_info *mtd, unsigned chip,
			dma_addr_t buffer, unsigned length)
{
	struct mxs_dma_desc	**d  = dma_desc;
	int			dma_channel;
	int			error = 0;
	u32			command_mode;
	u32			address;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	MTDDEBUG(MTD_DEBUG_LEVEL1, "Chip: %d DMA Buf: 0x%08x Length: %d\n",
		chip, buffer, length);

	/* Compute the DMA channel. */
	dma_channel = MXS_DMA_CHANNEL_AHB_APBH_GPMI0 + chip;

	/* A DMA descriptor that writes a buffer out. */
	command_mode = BV_GPMI_CTRL0_COMMAND_MODE__WRITE;
	address      = BV_GPMI_CTRL0_ADDRESS__NAND_DATA;

	MTDDEBUG(MTD_DEBUG_LEVEL1, "1st Command: mode: %d, address: %d, ",
		command_mode, address);

	(*d)->cmd.cmd.data                   = 0;
	(*d)->cmd.cmd.bits.command           = DMA_READ;
	(*d)->cmd.cmd.bits.chain             = 0;
	(*d)->cmd.cmd.bits.irq               = 1;
	(*d)->cmd.cmd.bits.nand_lock         = 0;
	(*d)->cmd.cmd.bits.nand_wait_4_ready = 0;
	(*d)->cmd.cmd.bits.dec_sem           = 1;
	(*d)->cmd.cmd.bits.wait4end          = 1;
	(*d)->cmd.cmd.bits.halt_on_terminate = 0;
	(*d)->cmd.cmd.bits.terminate_flush   = 0;
	(*d)->cmd.cmd.bits.pio_words         = 4;
	(*d)->cmd.cmd.bits.bytes             = length;

#ifdef CONFIG_ARCH_MMU
	(*d)->cmd.address = iomem_to_phys(buffer);
#else
	(*d)->cmd.address = buffer;
#endif

	(*d)->cmd.pio_words[0] =
		BF_GPMI_CTRL0_COMMAND_MODE(command_mode) |
		BM_GPMI_CTRL0_WORD_LENGTH                |
		BF_GPMI_CTRL0_CS(chip)                   |
		BF_GPMI_CTRL0_ADDRESS(address)           |
		BF_GPMI_CTRL0_XFER_COUNT(length)         ;
	(*d)->cmd.pio_words[1] = 0;
	(*d)->cmd.pio_words[2] = 0;
	(*d)->cmd.pio_words[3] = 0;
	MTDDEBUG(MTD_DEBUG_LEVEL1, "PIO Words[0]: 0x%08x, "
		"PIO Words[1]: 0x%08x, PIO Words[2]: 0x%08x, "
		"PIO Words[3]: 0x%08x\n",
		(unsigned int)(*d)->cmd.pio_words[0],
		(unsigned int)(*d)->cmd.pio_words[1],
		(unsigned int)(*d)->cmd.pio_words[2],
		(unsigned int)(*d)->cmd.pio_words[3]);

	mxs_dma_desc_append(dma_channel, (*d));
	d++;

	/* Go! */
	error = mxs_dma_go(dma_channel);

	if (error)
		printf("[%s] DMA error\n", __func__);

	/* Return success. */
	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);
	return error;

}

/**
 * read_data() - Receives data from the given chip.
 *
 * @this:    Per-device data.
 * @chip:    The chip of interest.
 * @buffer:  The physical address of a buffer that will receive the data.
 * @length:  The number of bytes to read.
 */
static int read_data(struct mtd_info *mtd, unsigned chip,
			dma_addr_t buffer, unsigned int length)
{
	struct mxs_dma_desc  **d        = dma_desc;
	int                  dma_channel;
	int                  error = 0;
	uint32_t             command_mode;
	uint32_t             address;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	MTDDEBUG(MTD_DEBUG_LEVEL1, "Chip: %d DMA Buf: 0x%08x Length: %d\n",
		chip, buffer, length);

	/* Compute the DMA channel. */
	dma_channel = MXS_DMA_CHANNEL_AHB_APBH_GPMI0 + chip;

	/* A DMA descriptor that reads the data. */
	command_mode = BV_GPMI_CTRL0_COMMAND_MODE__READ;
	address      = BV_GPMI_CTRL0_ADDRESS__NAND_DATA;

	MTDDEBUG(MTD_DEBUG_LEVEL1, "1st Command: mode: %d, address: %d, ",
		command_mode, address);

	(*d)->cmd.cmd.data                   = 0;
	(*d)->cmd.cmd.bits.command           = DMA_WRITE;
#if !defined(CONFIG_GPMI_NFC_V0)
	(*d)->cmd.cmd.bits.chain             = 0;
	(*d)->cmd.cmd.bits.irq               = 1;
#else
	(*d)->cmd.cmd.bits.chain             = 1;
	(*d)->cmd.cmd.bits.irq               = 0;
#endif
	(*d)->cmd.cmd.bits.nand_lock         = 0;
	(*d)->cmd.cmd.bits.nand_wait_4_ready = 0;
	(*d)->cmd.cmd.bits.dec_sem           = 1;
	(*d)->cmd.cmd.bits.wait4end          = 1;
#if defined(CONFIG_GPMI_NFC_V2)
	(*d)->cmd.cmd.bits.halt_on_terminate = 1;
#else
	(*d)->cmd.cmd.bits.halt_on_terminate = 0;
#endif
	(*d)->cmd.cmd.bits.terminate_flush   = 0;
	(*d)->cmd.cmd.bits.pio_words         = 1;
	(*d)->cmd.cmd.bits.bytes             = length;

#ifdef CONFIG_ARCH_MMU
	(*d)->cmd.address = iomem_to_phys(buffer);
#else
	(*d)->cmd.address = buffer;
#endif

	(*d)->cmd.pio_words[0] =
		BF_GPMI_CTRL0_COMMAND_MODE(command_mode) |
		BM_GPMI_CTRL0_WORD_LENGTH                |
		BF_GPMI_CTRL0_CS(chip)                   |
		BF_GPMI_CTRL0_ADDRESS(address)           |
		BF_GPMI_CTRL0_XFER_COUNT(length)         ;

	MTDDEBUG(MTD_DEBUG_LEVEL1, "PIO Words[0]: 0x%08x\n",
		(unsigned int)(*d)->cmd.pio_words[0]);

	mxs_dma_desc_append(dma_channel, (*d));
	d++;

#if !defined(CONFIG_GPMI_NFC_V2)
	/*
	 * A DMA descriptor that waits for the command to end and the chip to
	 * become ready.
	 *
	 * I think we actually should *not* be waiting for the chip to become
	 * ready because, after all, we don't care. I think the original code
	 * did that and no one has re-thought it yet.
	 */

	command_mode = BV_GPMI_CTRL0_COMMAND_MODE__WAIT_FOR_READY;
	address      = BV_GPMI_CTRL0_ADDRESS__NAND_DATA;

	MTDDEBUG(MTD_DEBUG_LEVEL2, "2nd Command: mode: %d, address: %d\n",
		command_mode, address);

	(*d)->cmd.cmd.data                   = 0;
	(*d)->cmd.cmd.bits.command           = NO_DMA_XFER;
	(*d)->cmd.cmd.bits.chain             = 0;
	(*d)->cmd.cmd.bits.irq               = 1;
	(*d)->cmd.cmd.bits.nand_lock         = 0;
	(*d)->cmd.cmd.bits.nand_wait_4_ready = 1;
	(*d)->cmd.cmd.bits.dec_sem           = 1;
	(*d)->cmd.cmd.bits.wait4end          = 1;
	(*d)->cmd.cmd.bits.halt_on_terminate = 0;
	(*d)->cmd.cmd.bits.terminate_flush   = 0;
	(*d)->cmd.cmd.bits.pio_words         = 4;
	(*d)->cmd.cmd.bits.bytes             = 0;

	(*d)->cmd.address = 0;

	(*d)->cmd.pio_words[0] =
		BF_GPMI_CTRL0_COMMAND_MODE(command_mode) |
		BM_GPMI_CTRL0_WORD_LENGTH                |
		BF_GPMI_CTRL0_CS(chip)                   |
		BF_GPMI_CTRL0_ADDRESS(address)           |
		BF_GPMI_CTRL0_XFER_COUNT(0)              ;
	(*d)->cmd.pio_words[1] = 0;
	(*d)->cmd.pio_words[2] = 0;
	(*d)->cmd.pio_words[3] = 0;
	MTDDEBUG(MTD_DEBUG_LEVEL2, "PIO Words[0]: 0x%08x, "
		"PIO Words[1]: 0x%08x, "
		"PIO Words[2]: 0x%08x, "
		"PIO Words[3]: 0x%08x\n",
		(unsigned int)(*d)->cmd.pio_words[0],
		(unsigned int)(*d)->cmd.pio_words[1],
		(unsigned int)(*d)->cmd.pio_words[2],
		(unsigned int)(*d)->cmd.pio_words[3]);

	mxs_dma_desc_append(dma_channel, (*d));
	d++;
#endif
	/* Go! */
	error = mxs_dma_go(dma_channel);

	if (error)
		printf("[%s] DMA error\n", __func__);

#ifdef CONFIG_MTD_DEBUG
	{
		int i;
		dma_addr_t *tmp_buf_ptr = (dma_addr_t *)buffer;

		printf("Buffer:");
		for (i = 0; i < length; ++i)
			printf("0x%08x ", tmp_buf_ptr[i]);
		printf("\n");
	}
#endif

	/* Return success. */
	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);
	return error;

}

int wait_for_bch_completion(u32 timeout)
{
	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	while ((!(REG_RD(CONFIG_BCH_REG_BASE, HW_BCH_CTRL) & 0x1)) &&
			--timeout)
		;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);

	return (timeout > 0) ? 0 : 1;
}

/**
 * send_page() - Sends a page, using ECC.
 *
 * @this:       Per-device data.
 * @chip:       The chip of interest.
 * @payload:    The physical address of the payload buffer.
 * @auxiliary:  The physical address of the auxiliary buffer.
 */
static int send_page(struct mtd_info *mtd, unsigned chip,
				dma_addr_t payload, dma_addr_t auxiliary)
{
	struct mxs_dma_desc  **d        = dma_desc;
	int                  dma_channel;
	int                  error = 0;
	uint32_t             command_mode;
	uint32_t             address;
	uint32_t             ecc_command;
	uint32_t             buffer_mask;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	MTDDEBUG(MTD_DEBUG_LEVEL1, "Chip: %d DMA Buf payload: 0x%08x "
		"auxiliary: 0x%08x\n",
		chip, payload, auxiliary);
	/* Compute the DMA channel. */
	dma_channel = MXS_DMA_CHANNEL_AHB_APBH_GPMI0 + chip;

	/* A DMA descriptor that does an ECC page read. */
	command_mode = BV_GPMI_CTRL0_COMMAND_MODE__WRITE;
	address      = BV_GPMI_CTRL0_ADDRESS__NAND_DATA;
#if defined(CONFIG_GPMI_NFC_V0)
	ecc_command  = BV_GPMI_ECCCTRL_ECC_CMD__BCH_ENCODE;
#else
	ecc_command  = BV_GPMI_ECCCTRL_ECC_CMD__ENCODE;
#endif
	buffer_mask  = BV_GPMI_ECCCTRL_BUFFER_MASK__BCH_PAGE |
			BV_GPMI_ECCCTRL_BUFFER_MASK__BCH_AUXONLY;

	MTDDEBUG(MTD_DEBUG_LEVEL1, "1st Command: mode: %d, address: %d, "
		"ecc command: %d, buffer_mask: %d",
		command_mode, address, ecc_command, buffer_mask);

	(*d)->cmd.cmd.data                   = 0;
	(*d)->cmd.cmd.bits.command           = NO_DMA_XFER;
	(*d)->cmd.cmd.bits.chain             = 0;
	(*d)->cmd.cmd.bits.irq               = 1;
	(*d)->cmd.cmd.bits.nand_lock         = 0;
	(*d)->cmd.cmd.bits.nand_wait_4_ready = 0;
	(*d)->cmd.cmd.bits.dec_sem           = 1;
	(*d)->cmd.cmd.bits.wait4end          = 1;
	(*d)->cmd.cmd.bits.halt_on_terminate = 0;
	(*d)->cmd.cmd.bits.terminate_flush   = 0;
	(*d)->cmd.cmd.bits.pio_words         = 6;
	(*d)->cmd.cmd.bits.bytes             = 0;

	(*d)->cmd.address = 0;

	(*d)->cmd.pio_words[0] =
		BF_GPMI_CTRL0_COMMAND_MODE(command_mode) |
		BM_GPMI_CTRL0_WORD_LENGTH                |
		BF_GPMI_CTRL0_CS(chip)                   |
		BF_GPMI_CTRL0_ADDRESS(address)           |
		BF_GPMI_CTRL0_XFER_COUNT(0)              ;

	(*d)->cmd.pio_words[1] = 0;

	(*d)->cmd.pio_words[2] =
		BM_GPMI_ECCCTRL_ENABLE_ECC               |
		BF_GPMI_ECCCTRL_ECC_CMD(ecc_command)     |
		BF_GPMI_ECCCTRL_BUFFER_MASK(buffer_mask) ;

	(*d)->cmd.pio_words[3] = (mtd->writesize + mtd->oobsize);
#ifdef CONFIG_ARCH_MMU
	(*d)->cmd.pio_words[4] = iomem_to_phys(payload);
	(*d)->cmd.pio_words[5] = iomem_to_phys(auxiliary);
#else
	(*d)->cmd.pio_words[4] = payload;
	(*d)->cmd.pio_words[5] = auxiliary;
#endif

	MTDDEBUG(MTD_DEBUG_LEVEL1, "PIO Words[0]: 0x%08x, "
		"PIO Words[1]: 0x%08x, "
		"PIO Words[2]: 0x%08x, "
		"PIO Words[3]: 0x%08x, "
		"PIO Words[4]: 0x%08x, "
		"PIO Words[5]: 0x%08x\n",
		(unsigned int)(*d)->cmd.pio_words[0],
		(unsigned int)(*d)->cmd.pio_words[1],
		(unsigned int)(*d)->cmd.pio_words[2],
		(unsigned int)(*d)->cmd.pio_words[3],
		(unsigned int)(*d)->cmd.pio_words[4],
		(unsigned int)(*d)->cmd.pio_words[5]);

	mxs_dma_desc_append(dma_channel, (*d));
	d++;

	/* Go! */
	error = mxs_dma_go(dma_channel);

	if (error)
		printf("[%s] DMA error\n", __func__);

	error = wait_for_bch_completion(10000);

	error = (error) ? -ETIMEDOUT : 0;

	if (error)
		printf("[%s] bch timeout!!!\n", __func__);

	clear_bch(NULL);

	/* Return success. */
	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);
	return error;
}

/**
 * read_page() - Reads a page, using ECC.
 *
 * @this:       Per-device data.
 * @chip:       The chip of interest.
 * @payload:    The physical address of the payload buffer.
 * @auxiliary:  The physical address of the auxiliary buffer.
 */
static int read_page(struct mtd_info *mtd, unsigned chip,
			dma_addr_t payload, dma_addr_t auxiliary)
{
	struct mxs_dma_desc	**d        = dma_desc;
	s32			dma_channel;
	s32			error = 0;
	u32			command_mode;
	u32			address;
	u32			ecc_command;
	u32			buffer_mask;
	u32			page_size = mtd->writesize + mtd->oobsize;

	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s =>\n", __func__);

	MTDDEBUG(MTD_DEBUG_LEVEL1, "Chip: %d DMA Buf payload: 0x%08x "
		"auxiliary: 0x%08x\n",
		chip, payload, auxiliary);
	/* Compute the DMA channel. */
	dma_channel = MXS_DMA_CHANNEL_AHB_APBH_GPMI0 + chip;

	/* Wait for the chip to report ready. */
	command_mode = BV_GPMI_CTRL0_COMMAND_MODE__WAIT_FOR_READY;
	address      = BV_GPMI_CTRL0_ADDRESS__NAND_DATA;

	MTDDEBUG(MTD_DEBUG_LEVEL1, "1st Command: mode: %d, address: %d",
		command_mode, address);

	(*d)->cmd.cmd.data                   = 0;
	(*d)->cmd.cmd.bits.command           = NO_DMA_XFER;
	(*d)->cmd.cmd.bits.chain             = 1;
	(*d)->cmd.cmd.bits.irq               = 0;
	(*d)->cmd.cmd.bits.nand_lock         = 0;
	(*d)->cmd.cmd.bits.nand_wait_4_ready = 1;
#if !defined(CONFIG_GPMI_NFC_V0)
	(*d)->cmd.cmd.bits.dec_sem           = 0;
#else
	(*d)->cmd.cmd.bits.dec_sem           = 1;
#endif
	(*d)->cmd.cmd.bits.wait4end          = 1;
	(*d)->cmd.cmd.bits.halt_on_terminate = 0;
	(*d)->cmd.cmd.bits.terminate_flush   = 0;
	(*d)->cmd.cmd.bits.pio_words         = 1;
	(*d)->cmd.cmd.bits.bytes             = 0;

	(*d)->cmd.address = 0;

	(*d)->cmd.pio_words[0] =
		BF_GPMI_CTRL0_COMMAND_MODE(command_mode) |
		BM_GPMI_CTRL0_WORD_LENGTH                |
		BF_GPMI_CTRL0_CS(chip)                   |
		BF_GPMI_CTRL0_ADDRESS(address)           |
		BF_GPMI_CTRL0_XFER_COUNT(0)              ;

	MTDDEBUG(MTD_DEBUG_LEVEL1, "PIO Words[0]: 0x%08x\n",
		(unsigned int)(*d)->cmd.pio_words[0]);

	mxs_dma_desc_append(dma_channel, (*d));
	d++;

	/* Enable the BCH block and read. */

	command_mode = BV_GPMI_CTRL0_COMMAND_MODE__READ;
	address      = BV_GPMI_CTRL0_ADDRESS__NAND_DATA;
#if defined(CONFIG_GPMI_NFC_V0)
	ecc_command  = BV_GPMI_ECCCTRL_ECC_CMD__BCH_DECODE;
#else
	ecc_command  = BV_GPMI_ECCCTRL_ECC_CMD__DECODE;
#endif
	buffer_mask  = BV_GPMI_ECCCTRL_BUFFER_MASK__BCH_PAGE |
			BV_GPMI_ECCCTRL_BUFFER_MASK__BCH_AUXONLY;

	MTDDEBUG(MTD_DEBUG_LEVEL1, "2nd Command: mode: %d, address: %d, "
		"ecc command: %d, buffer_mask: %d",
		command_mode, address, ecc_command, buffer_mask);

	(*d)->cmd.cmd.data                   = 0;
	(*d)->cmd.cmd.bits.command           = NO_DMA_XFER;
	(*d)->cmd.cmd.bits.chain             = 1;
	(*d)->cmd.cmd.bits.irq               = 0;
	(*d)->cmd.cmd.bits.nand_lock         = 0;
	(*d)->cmd.cmd.bits.nand_wait_4_ready = 0;
#if !defined(CONFIG_GPMI_NFC_V0)
	(*d)->cmd.cmd.bits.dec_sem           = 0;
#else
	(*d)->cmd.cmd.bits.dec_sem           = 1;
#endif
	(*d)->cmd.cmd.bits.wait4end          = 1;
	(*d)->cmd.cmd.bits.halt_on_terminate = 0;
	(*d)->cmd.cmd.bits.terminate_flush   = 0;
	(*d)->cmd.cmd.bits.pio_words         = 6;
	(*d)->cmd.cmd.bits.bytes             = 0;

	(*d)->cmd.address = 0;

	(*d)->cmd.pio_words[0] =
		BF_GPMI_CTRL0_COMMAND_MODE(command_mode)              |
		BM_GPMI_CTRL0_WORD_LENGTH                             |
		BF_GPMI_CTRL0_CS(chip)                                |
		BF_GPMI_CTRL0_ADDRESS(address)                        |
		BF_GPMI_CTRL0_XFER_COUNT(page_size) ;

	(*d)->cmd.pio_words[1] = 0;
	(*d)->cmd.pio_words[2] =
		BM_GPMI_ECCCTRL_ENABLE_ECC	|
		BF_GPMI_ECCCTRL_ECC_CMD(ecc_command)     |
		BF_GPMI_ECCCTRL_BUFFER_MASK(buffer_mask) ;
	(*d)->cmd.pio_words[3] = page_size;
#ifdef CONFIG_ARCH_MMU
	(*d)->cmd.pio_words[4] = iomem_to_phys(payload);
	(*d)->cmd.pio_words[5] = iomem_to_phys(auxiliary);
#else
	(*d)->cmd.pio_words[4] = payload;
	(*d)->cmd.pio_words[5] = auxiliary;
#endif

	MTDDEBUG(MTD_DEBUG_LEVEL1, "PIO Words[0]: 0x%08x, "
		"PIO Words[1]: 0x%08x, "
		"PIO Words[2]: 0x%08x, "
		"PIO Words[3]: 0x%08x, "
		"PIO Words[4]: 0x%08x, "
		"PIO Words[5]: 0x%08x\n",
		(unsigned int)(*d)->cmd.pio_words[0],
		(unsigned int)(*d)->cmd.pio_words[1],
		(unsigned int)(*d)->cmd.pio_words[2],
		(unsigned int)(*d)->cmd.pio_words[3],
		(unsigned int)(*d)->cmd.pio_words[4],
		(unsigned int)(*d)->cmd.pio_words[5]);

	mxs_dma_desc_append(dma_channel, (*d));
	d++;

	/* Disable the BCH block */
	command_mode = BV_GPMI_CTRL0_COMMAND_MODE__WAIT_FOR_READY;
	address      = BV_GPMI_CTRL0_ADDRESS__NAND_DATA;

	MTDDEBUG(MTD_DEBUG_LEVEL1, "3rd Command: mode: %d, address: %d",
		command_mode, address);

	(*d)->cmd.cmd.data                   = 0;
	(*d)->cmd.cmd.bits.command           = NO_DMA_XFER;
	(*d)->cmd.cmd.bits.chain             = 1;
	(*d)->cmd.cmd.bits.irq               = 0;
	(*d)->cmd.cmd.bits.nand_lock         = 0;
	(*d)->cmd.cmd.bits.nand_wait_4_ready = 1;
#if !defined(CONFIG_GPMI_NFC_V0)
	(*d)->cmd.cmd.bits.dec_sem           = 0;
#else
	(*d)->cmd.cmd.bits.dec_sem           = 1;
#endif
	(*d)->cmd.cmd.bits.wait4end          = 1;
	(*d)->cmd.cmd.bits.halt_on_terminate = 0;
	(*d)->cmd.cmd.bits.terminate_flush   = 0;
	(*d)->cmd.cmd.bits.pio_words         = 3;
	(*d)->cmd.cmd.bits.bytes             = 0;

	(*d)->cmd.address = 0;

	(*d)->cmd.pio_words[0] =
		BF_GPMI_CTRL0_COMMAND_MODE(command_mode)              |
		BM_GPMI_CTRL0_WORD_LENGTH                             |
		BF_GPMI_CTRL0_CS(chip)                                |
		BF_GPMI_CTRL0_ADDRESS(address)                        |
		BF_GPMI_CTRL0_XFER_COUNT(page_size) ;

	(*d)->cmd.pio_words[1] = 0;
	(*d)->cmd.pio_words[2] = 0;

	MTDDEBUG(MTD_DEBUG_LEVEL1, "PIO Words[0]: 0x%08x, "
		"PIO Words[1]: 0x%08x, "
		"PIO Words[2]: 0x%08x\n",
		(unsigned int)(*d)->cmd.pio_words[0],
		(unsigned int)(*d)->cmd.pio_words[1],
		(unsigned int)(*d)->cmd.pio_words[2]);

	mxs_dma_desc_append(dma_channel, (*d));
	d++;

	/* Deassert the NAND lock and interrupt. */

	(*d)->cmd.cmd.data                   = 0;
	(*d)->cmd.cmd.bits.command           = NO_DMA_XFER;
	(*d)->cmd.cmd.bits.chain             = 0;
	(*d)->cmd.cmd.bits.irq               = 1;
	(*d)->cmd.cmd.bits.nand_lock         = 0;
	(*d)->cmd.cmd.bits.nand_wait_4_ready = 0;
	(*d)->cmd.cmd.bits.dec_sem           = 1;
	(*d)->cmd.cmd.bits.wait4end          = 0;
	(*d)->cmd.cmd.bits.halt_on_terminate = 0;
	(*d)->cmd.cmd.bits.terminate_flush   = 0;
	(*d)->cmd.cmd.bits.pio_words         = 0;
	(*d)->cmd.cmd.bits.bytes             = 0;

	(*d)->cmd.address = 0;

	mxs_dma_desc_append(dma_channel, (*d));
	d++;

	/* Go! */
	error = mxs_dma_go(dma_channel);

	if (error)
		printf("[%s] DMA error\n", __func__);

	error = wait_for_bch_completion(10000);

	error = (error) ? -ETIMEDOUT : 0;

	if (error)
		printf("[%s] bch timeout!!!\n", __func__);

	clear_bch(NULL);

	/* Return success. */
	MTDDEBUG(MTD_DEBUG_LEVEL3, "<= %s\n", __func__);
	return error;
}

/* This structure represents the NFC HAL for this version of the hardware. */
struct nfc_hal gpmi_nfc_hal = {
#if defined(CONFIG_GPMI_NFC_V0)
	.version                     = 0,
	.description                 = "4-chip GPMI and BCH",
	.max_chip_count              = 4,
#else
#if defined(CONFIG_GPMI_NFC_V1)
	.version                     = 1,
#else
	.version                     = 2,
#endif
	.description                 = "8-chip GPMI and BCH",
	.max_chip_count              = 8,
#endif
	.max_data_setup_cycles       = (BM_GPMI_TIMING0_DATA_SETUP >>
					BP_GPMI_TIMING0_DATA_SETUP),
	.internal_data_setup_in_ns   = 0,
	.max_sample_delay_factor     = (BM_GPMI_CTRL1_RDN_DELAY >>
					BP_GPMI_CTRL1_RDN_DELAY),
	.max_dll_clock_period_in_ns  = 32,
	.max_dll_delay_in_ns         = 16,
	.init                        = init,
	.set_geometry                = set_geometry,
	.set_timing                  = set_timing,
	.get_timing                  = get_timing,
	.exit                        = exit,
	.begin                       = begin,
	.end                         = end,
	.clear_bch                   = clear_bch,
	.is_ready                    = is_ready,
	.send_command                = send_command,
	.send_data                   = send_data,
	.read_data                   = read_data,
	.send_page                   = send_page,
	.read_page                   = read_page,
};
