/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2017 NXP
 *
 * SJA1105 Driver
 */

#ifndef _SJA1105_H_
#define _SJA1105_H_

/**
 * sja1105_probe() - Probe for SJA1105 switch based on CS and Bus
 *
 * Given a bus number and chip select, this probes the corresponding bus device
 * for a SJA1105 switch. Writes the firmware on to the device.
 *
 * @cs:		Chip select to look for
 * @bus:	SPI bus number
 * @return	0 if found, 0 < on error
 */
int sja1105_probe(u32 cs, u32 bus);

#endif
