/*
 * Copyright 2004-2010 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file mxc_keyb.c
 *
 * @brief Driver for the Freescale Semiconductor MXC keypad port.
 *
 * The keypad driver is designed as a standard Input driver which interacts
 * with low level keypad port hardware. Upon opening, the Keypad driver
 * initializes the keypad port. When the keypad interrupt happens the driver
 * calles keypad polling timer and scans the keypad matrix for key
 * press/release. If all key press/release happened it comes out of timer and
 * waits for key press interrupt. The scancode for key press and release events
 * are passed to Input subsytem.
 *
 * @ingroup keypad
 */

#include <asm/io.h>
#include <common.h>
#include <asm/errno.h>
#include <linux/types.h>
#include <asm/mxc_key_defs.h>
#include <malloc.h>

/*
 *  * Module header file
 *   */
#include <mxc_keyb.h>

/*!
 * Comment KPP_DEBUG to disable debug messages
 */

#undef	KPP_DEBUG

#ifdef	KPP_DEBUG
#define	KPP_PRINTF(fmt, args...)	printf(fmt , ##args)

static void mxc_kpp_dump_regs()
{
	unsigned short t1, t2, t3;

	t1 = __raw_readw(KPCR);
	t2 = __raw_readw(KPSR);
	t3 = __raw_readw(KDDR);
	/*
	KPP_PRINTF("KPCR=0x%04x, KPSR=0x%04x, KDDR=0x%04x\n",
		t1, t2, t3);
		*/
}
#else
#define KPP_PRINTF(fmt, args...)
#endif

static u16 mxc_key_mapping[] = CONFIG_MXC_KEYMAPPING;

/*!
 * This structure holds the keypad private data structure.
 */
static struct keypad_priv kpp_dev;

/*! Indicates if the key pad device is enabled. */

/*! This static variable indicates whether a key event is pressed/released. */
static unsigned short KPress;

/*! cur_rcmap and prev_rcmap array is used to detect key press and release. */
static unsigned short *cur_rcmap;	/* max 64 bits (8x8 matrix) */
static unsigned short *prev_rcmap;

/*!
 * Debounce polling period(10ms) in system ticks.
 */
static unsigned short KScanRate = (10 * CONFIG_SYS_HZ) / 1000;

/*!
 * These arrays are used to store press and release scancodes.
 */
static short **press_scancode;
static short **release_scancode;

static const unsigned short *mxckpd_keycodes;
static unsigned short mxckpd_keycodes_size;

/*!
 * These functions are used to configure and the GPIO pins for keypad to
 * activate and deactivate it.
 */
extern void setup_mxc_kpd(void);

/*!
 * This function is called to scan the keypad matrix to find out the key press
 * and key release events. Make scancode and break scancode are generated for
 * key press and key release events.
 *
 * The following scanning sequence are done for
 * keypad row and column scanning,
 * -# Write 1's to KPDR[15:8], setting column data to 1's
 * -# Configure columns as totem pole outputs(for quick discharging of keypad
 * capacitance)
 * -# Configure columns as open-drain
 * -# Write a single column to 0, others to 1.
 * -# Sample row inputs and save data. Multiple key presses can be detected on
 * a single column.
 * -# Repeat steps the above steps for remaining columns.
 * -# Return all columns to 0 in preparation for standby mode.
 * -# Clear KPKD and KPKR status bit(s) by writing to a 1,
 *    Set the KPKR synchronizer chain by writing "1" to KRSS register,
 *    Clear the KPKD synchronizer chain by writing "1" to KDSC register
 *
 * @result    Number of key pressed/released.
 */
static int mxc_kpp_scan_matrix(void)
{
	unsigned short reg_val;
	int col, row;
	short scancode = 0;
	int keycnt = 0;		/* How many keys are still pressed */

	/*
	 * wmb() linux kernel function which guarantees orderings in write
	 * operations
	 */
	/* wmb(); */

	/* save cur keypad matrix to prev */
	memcpy(prev_rcmap, cur_rcmap, kpp_dev.kpp_rows * sizeof(prev_rcmap[0]));
	memset(cur_rcmap, 0, kpp_dev.kpp_rows * sizeof(cur_rcmap[0]));

	for (col = 0; col < kpp_dev.kpp_cols; col++) {	/* Col */
		/* 2. Write 1.s to KPDR[15:8] setting column data to 1.s */
		reg_val = __raw_readw(KPDR);
		reg_val |= 0xff00;
		__raw_writew(reg_val, KPDR);

		/*
		 * 3. Configure columns as totem pole outputs(for quick
		 * discharging of keypad capacitance)
		 */
		reg_val = __raw_readw(KPCR);
		reg_val &= 0x00ff;
		__raw_writew(reg_val, KPCR);

		udelay(2);

#ifdef KPP_DEBUG
		mxc_kpp_dump_regs();
#endif

		/*
		 * 4. Configure columns as open-drain
		 */
		reg_val = __raw_readw(KPCR);
		reg_val |= ((1 << kpp_dev.kpp_cols) - 1) << 8;
		__raw_writew(reg_val, KPCR);

		/*
		 * 5. Write a single column to 0, others to 1.
		 * 6. Sample row inputs and save data. Multiple key presses
		 * can be detected on a single column.
		 * 7. Repeat steps 2 - 6 for remaining columns.
		 */

		/* Col bit starts at 8th bit in KPDR */
		reg_val = __raw_readw(KPDR);
		reg_val &= ~(1 << (8 + col));
		__raw_writew(reg_val, KPDR);

		/* Delay added to avoid propagating the 0 from column to row
		 * when scanning. */

		udelay(5);

#ifdef KPP_DEBUG
		mxc_kpp_dump_regs();
#endif

		/* Read row input */
		reg_val = __raw_readw(KPDR);
		for (row = 0; row < kpp_dev.kpp_rows; row++) {	/* sample row */
			if (TEST_BIT(reg_val, row) == 0) {
				cur_rcmap[row] = BITSET(cur_rcmap[row], col);
				keycnt++;
			}
		}
	}

	/*
	 * 8. Return all columns to 0 in preparation for standby mode.
	 * 9. Clear KPKD and KPKR status bit(s) by writing to a .1.,
	 * set the KPKR synchronizer chain by writing "1" to KRSS register,
	 * clear the KPKD synchronizer chain by writing "1" to KDSC register
	 */
	reg_val = 0x00;
	__raw_writew(reg_val, KPDR);
	reg_val = __raw_readw(KPDR);
	reg_val = __raw_readw(KPSR);
	reg_val |= KBD_STAT_KPKD | KBD_STAT_KPKR | KBD_STAT_KRSS |
	    KBD_STAT_KDSC;
	__raw_writew(reg_val, KPSR);

#ifdef KPP_DEBUG
	mxc_kpp_dump_regs();
#endif

	/* Check key press status change */

	/*
	 * prev_rcmap array will contain the previous status of the keypad
	 * matrix.  cur_rcmap array will contains the present status of the
	 * keypad matrix. If a bit is set in the array, that (row, col) bit is
	 * pressed, else it is not pressed.
	 *
	 * XORing these two variables will give us the change in bit for
	 * particular row and column.  If a bit is set in XOR output, then that
	 * (row, col) has a change of status from the previous state.  From
	 * the diff variable the key press and key release of row and column
	 * are found out.
	 *
	 * If the key press is determined then scancode for key pressed
	 * can be generated using the following statement:
	 *    scancode = ((row * 8) + col);
	 *
	 * If the key release is determined then scancode for key release
	 * can be generated using the following statement:
	 *    scancode = ((row * 8) + col) + MXC_KEYRELEASE;
	 */
	for (row = 0; row < kpp_dev.kpp_rows; row++) {
		unsigned char diff;

		/*
		 * Calculate the change in the keypad row status
		 */
		diff = prev_rcmap[row] ^ cur_rcmap[row];

		for (col = 0; col < kpp_dev.kpp_cols; col++) {
			if ((diff >> col) & 0x1) {
				/* There is a status change on col */
				if ((prev_rcmap[row] & BITSET(0, col)) == 0) {
					/*
					 * Previous state is 0, so now
					 * a key is pressed
					 */
					scancode =
					    ((row * kpp_dev.kpp_cols) +
					     col);
					KPress = 1;
					kpp_dev.iKeyState = KStateUp;

					KPP_PRINTF("Press   (%d, %d) scan=%d "
						 "Kpress=%d\n",
						 row, col, scancode, KPress);
					press_scancode[row][col] =
					    (short)scancode;
				} else {
					/*
					 * Previous state is not 0, so
					 * now a key is released
					 */
					scancode =
					    (row * kpp_dev.kpp_cols) +
					    col + MXC_KEYRELEASE;
					KPress = 0;
					kpp_dev.iKeyState = KStateDown;

					KPP_PRINTF
					    ("Release (%d, %d) scan=%d Kpress=%d\n",
					     row, col, scancode, KPress);
					release_scancode[row][col] =
					    (short)scancode;
					keycnt++;
				}
			}
		}
	}

	return keycnt;
}

static int mxc_kpp_reset(void)
{
	unsigned short reg_val;
	int i;

	/*
	* Stop scanning and wait for interrupt.
	* Enable press interrupt and disable release interrupt.
	*/
	__raw_writew(0x00FF, KPDR);
	reg_val = __raw_readw(KPSR);
	reg_val |= (KBD_STAT_KPKR | KBD_STAT_KPKD);
	reg_val |= KBD_STAT_KRSS | KBD_STAT_KDSC;
	__raw_writew(reg_val, KPSR);
	reg_val |= KBD_STAT_KDIE;
	reg_val &= ~KBD_STAT_KRIE;
	__raw_writew(reg_val, KPSR);

#ifdef KPP_DEBUG
	mxc_kpp_dump_regs();
#endif

	/*
	* No more keys pressed... make sure unwanted key codes are
	* not given upstairs
	*/
	for (i = 0; i < kpp_dev.kpp_rows; i++) {
		memset(press_scancode[i], -1,
			sizeof(press_scancode[0][0]) * kpp_dev.kpp_cols);
		memset(release_scancode[i], -1,
			sizeof(release_scancode[0][0]) *
			kpp_dev.kpp_cols);
	}

	return 0;
}

int mxc_kpp_getc(struct kpp_key_info **key_info)
{
	int col, row;
	int key_cnt;
	unsigned short reg_val;
	short scancode = 0;
	int index = 0;
	struct kpp_key_info *keyi;

	reg_val = __raw_readw(KPSR);

	if (reg_val & KBD_STAT_KPKD) {
		/*
		* Disable key press(KDIE status bit) interrupt
		*/
		reg_val &= ~KBD_STAT_KDIE;
		__raw_writew(reg_val, KPSR);

#ifdef KPP_DEBUG
		mxc_kpp_dump_regs();
#endif

		key_cnt = mxc_kpp_scan_matrix();
	} else {
		return 0;
	}

	if (key_cnt <= 0)
		return 0;

	*key_info = keyi =
		(struct kpp_key_info *)malloc
		(sizeof(struct kpp_key_info) * key_cnt);

	/*
	* This switch case statement is the
	* implementation of state machine of debounc
	* logic for key press/release.
	* The explaination of state machine is as
	* follows:
	*
	* KStateUp State:
	* This is in intial state of the state machine
	* this state it checks for any key presses.
	* The key press can be checked using the
	* variable KPress. If KPress is set, then key
	* press is identified and switches the to
	* KStateFirstDown state for key press to
	* debounce.
	*
	* KStateFirstDown:
	* After debounce delay(10ms), if the KPress is
	* still set then pass scancode generated to
	* input device and change the state to
	* KStateDown, else key press debounce is not
	* satisfied so change the state to KStateUp.
	*
	* KStateDown:
	* In this state it checks for any key release.
	* If KPress variable is cleared, then key
	* release is indicated and so, switch the
	* state to KStateFirstUp else to state
	* KStateDown.
	*
	* KStateFirstUp:
	* After debounce delay(10ms), if the KPress is
	* still reset then pass the key release
	* scancode to input device and change
	* the state to KStateUp else key release is
	* not satisfied so change the state to
	* KStateDown.
	*/

	for (row = 0; row < kpp_dev.kpp_rows; row++) {
		for (col = 0; col < kpp_dev.kpp_cols; col++) {
			if ((press_scancode[row][col] != -1)) {
				/* Still Down, so add scancode */
				scancode =
				    press_scancode[row][col];

				keyi[index].val = mxckpd_keycodes[scancode];
				keyi[index++].evt = KDepress;

				KPP_PRINTF("KStateFirstDown: scan=%d val=%d\n",
					scancode, mxckpd_keycodes[scancode]);
				if (index >= key_cnt)
					goto key_detect;

				kpp_dev.iKeyState = KStateDown;
				press_scancode[row][col] = -1;
			}
		}
	}

	for (row = 0; row < kpp_dev.kpp_rows; row++) {
		for (col = 0; col < kpp_dev.kpp_cols; col++) {
			if ((release_scancode[row][col] != -1)) {
				scancode =
				    release_scancode[row][col];
				scancode =
					scancode - MXC_KEYRELEASE;

				keyi[index].val = mxckpd_keycodes[scancode];
				keyi[index++].evt = KRelease;

				KPP_PRINTF("KStateFirstUp: scan=%d val=%d\n",
					scancode, mxckpd_keycodes[scancode]);
				if (index >= key_cnt)
					goto key_detect;

				kpp_dev.iKeyState = KStateUp;
				release_scancode[row][col] = -1;
			}
		}
	}

key_detect:
	mxc_kpp_reset();
	return key_cnt;
}

/*!
 * This function is called to free the allocated memory for local arrays
 */
static void mxc_kpp_free_allocated(void)
{
	int i;

	if (press_scancode) {
		for (i = 0; i < kpp_dev.kpp_rows; i++) {
			if (press_scancode[i])
				free(press_scancode[i]);
		}
		free(press_scancode);
	}

	if (release_scancode) {
		for (i = 0; i < kpp_dev.kpp_rows; i++) {
			if (release_scancode[i])
				free(release_scancode[i]);
		}
		free(release_scancode);
	}

	if (cur_rcmap)
		free(cur_rcmap);

	if (prev_rcmap)
		free(prev_rcmap);
}

/*!
 * This function is called during the driver binding process.
 *
 * @param   pdev  the device structure used to store device specific
 *                information that is used by the suspend, resume and remove
 *                functions.
 *
 * @return  The function returns 0 on successful registration. Otherwise returns
 *          specific error code.
 */
int mxc_kpp_init(void)
{
	int i;
	int retval;
	unsigned int reg_val;

	kpp_dev.kpp_cols = CONFIG_MXC_KPD_COLMAX;
	kpp_dev.kpp_rows = CONFIG_MXC_KPD_ROWMAX;

	/* clock and IOMUX configuration for keypad */
	setup_mxc_kpd();

	/* Configure keypad */

	/* Enable number of rows in keypad (KPCR[7:0])
	 * Configure keypad columns as open-drain (KPCR[15:8])
	 *
	 * Configure the rows/cols in KPP
	 * LSB nibble in KPP is for 8 rows
	 * MSB nibble in KPP is for 8 cols
	 */
	reg_val = __raw_readw(KPCR);
	reg_val |= (1  << kpp_dev.kpp_rows) - 1;	/* LSB */
	reg_val |= ((1 << kpp_dev.kpp_cols) - 1) << 8;	/* MSB */
	__raw_writew(reg_val, KPCR);

	/* Write 0's to KPDR[15:8] */
	reg_val = __raw_readw(KPDR);
	reg_val &= 0x00ff;
	__raw_writew(reg_val, KPDR);

	/* Configure columns as output,
	 * rows as input (KDDR[15:0]) */
	reg_val = __raw_readw(KDDR);
	reg_val |= 0xff00;
	reg_val &= 0xff00;
	__raw_writew(reg_val, KDDR);

	/* Clear the KPKD Status Flag
	 * and Synchronizer chain. */
	reg_val = __raw_readw(KPSR);
	reg_val &= ~(KBD_STAT_KPKR | KBD_STAT_KPKD);
	reg_val |= KBD_STAT_KPKD;
	reg_val |= KBD_STAT_KRSS | KBD_STAT_KDSC;
	__raw_writew(reg_val, KPSR);
	/* Set the KDIE control bit, and clear the KRIE
	 * control bit (avoid false release events). */
	reg_val |= KBD_STAT_KDIE;
	reg_val &= ~KBD_STAT_KRIE;
	__raw_writew(reg_val, KPSR);

#ifdef KPP_DEBUG
	mxc_kpp_dump_regs();
#endif

	mxckpd_keycodes = mxc_key_mapping;
	mxckpd_keycodes_size = kpp_dev.kpp_cols * kpp_dev.kpp_rows;

	if ((mxckpd_keycodes == (void *)0)
	    || (mxckpd_keycodes_size == 0)) {
		retval = -ENODEV;
		goto err;
	}

	/* allocate required memory */
	press_scancode   = (short **)malloc(kpp_dev.kpp_rows * sizeof(press_scancode[0]));
	release_scancode = (short **)malloc(kpp_dev.kpp_rows * sizeof(release_scancode[0]));

	if (!press_scancode || !release_scancode) {
		retval = -ENOMEM;
		goto err;
	}

	for (i = 0; i < kpp_dev.kpp_rows; i++) {
		press_scancode[i] = (short *)malloc(kpp_dev.kpp_cols
					    * sizeof(press_scancode[0][0]));
		release_scancode[i] =
		    (short *)malloc(kpp_dev.kpp_cols * sizeof(release_scancode[0][0]));

		if (!press_scancode[i] || !release_scancode[i]) {
			retval = -ENOMEM;
			goto err;
		}
	}

	cur_rcmap =
	    (unsigned short *)malloc(kpp_dev.kpp_rows * sizeof(cur_rcmap[0]));
	prev_rcmap =
	    (unsigned short *)malloc(kpp_dev.kpp_rows * sizeof(prev_rcmap[0]));

	if (!cur_rcmap || !prev_rcmap) {
		retval = -ENOMEM;
		goto err;
	}

	for (i = 0; i < kpp_dev.kpp_rows; i++) {
		memset(press_scancode[i], -1,
		       sizeof(press_scancode[0][0]) * kpp_dev.kpp_cols);
		memset(release_scancode[i], -1,
		       sizeof(release_scancode[0][0]) * kpp_dev.kpp_cols);
	}
	memset(cur_rcmap, 0, kpp_dev.kpp_rows * sizeof(cur_rcmap[0]));
	memset(prev_rcmap, 0, kpp_dev.kpp_rows * sizeof(prev_rcmap[0]));

	return 0;

err:
	mxc_kpp_free_allocated();
	return retval;
}

