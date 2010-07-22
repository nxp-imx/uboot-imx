/*
 *
 * (C) Copyright 2009-2010 Freescale Semiconductor, Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __PINCTRL_H
#define __PINCTRL_H

#define PIN_BITS		(5)
#define PINS_PER_BANK		(1 << PIN_BITS)
#define PINID_2_BANK(id)	((id) >> PIN_BITS)
#define PINID_2_PIN(id)		((id) & (PINS_PER_BANK - 1))
#define PINID_ENCODE(bank, pin)	(((bank) << PIN_BITS) + (pin))

/*
 * Each pin may be routed up to four different HW interfaces
 * including GPIO
 */
enum pin_fun {
	PIN_FUN1 = 0,
	PIN_FUN2,
	PIN_FUN3,
	PIN_GPIO
};

/*
 * Each pin may have different output drive strength in range from
 * 4mA to 20mA. The most common case is 4, 8 and 12 mA strengths.
 */
enum pad_strength {
	PAD_4MA = 0,
	PAD_8MA,
	PAD_12MA,
	PAD_RESV
};

/*
 * Each pin can be programmed for 1.8V or 3.3V
 */
enum pad_voltage {
	PAD_1V8 = 0,
	PAD_3V3
};

/*
 * Structure to define a group of pins and their parameters
 */
struct pin_desc {
	u32 id;
	enum pin_fun fun;
	enum pad_strength strength;
	enum pad_voltage voltage;
	u32 pullup:1;
};

struct pin_group {
	struct pin_desc *pins;
	int nr_pins;
};

extern void pin_gpio_direction(u32 id, u32 output);
extern u32 pin_gpio_get(u32 id);
extern void pin_gpio_set(u32 id, u32 val);
extern void pin_set_type(u32 id, enum pin_fun cfg);
extern void pin_set_strength(u32 id, enum pad_strength strength);
extern void pin_set_voltage(u32 id, enum pad_voltage volt);
extern void pin_set_pullup(u32 id, u32 pullup);
extern void pin_set_group(struct pin_group *pin_group);

/*
 * Definitions of all i.MX28 pins
 */
/* Bank 0 */
#define PINID_GPMI_D00		PINID_ENCODE(0, 0)
#define PINID_GPMI_D01		PINID_ENCODE(0, 1)
#define PINID_GPMI_D02		PINID_ENCODE(0, 2)
#define PINID_GPMI_D03		PINID_ENCODE(0, 3)
#define PINID_GPMI_D04		PINID_ENCODE(0, 4)
#define PINID_GPMI_D05		PINID_ENCODE(0, 5)
#define PINID_GPMI_D06		PINID_ENCODE(0, 6)
#define PINID_GPMI_D07		PINID_ENCODE(0, 7)
#define PINID_GPMI_CE0N		PINID_ENCODE(0, 16)
#define PINID_GPMI_CE1N		PINID_ENCODE(0, 17)
#define PINID_GPMI_CE2N		PINID_ENCODE(0, 18)
#define PINID_GPMI_CE3N		PINID_ENCODE(0, 19)
#define PINID_GPMI_RDY0		PINID_ENCODE(0, 20)
#define PINID_GPMI_RDY1		PINID_ENCODE(0, 21)
#define PINID_GPMI_RDY2		PINID_ENCODE(0, 22)
#define PINID_GPMI_RDY3		PINID_ENCODE(0, 23)
#define PINID_GPMI_RDN		PINID_ENCODE(0, 24)
#define PINID_GPMI_WRN		PINID_ENCODE(0, 25)
#define PINID_GPMI_ALE		PINID_ENCODE(0, 26)
#define PINID_GPMI_CLE		PINID_ENCODE(0, 27)
#define PINID_GPMI_RESETN	PINID_ENCODE(0, 28)

/* Bank 1 */
#define PINID_LCD_D00		PINID_ENCODE(1, 0)
#define PINID_LCD_D01		PINID_ENCODE(1, 1)
#define PINID_LCD_D02		PINID_ENCODE(1, 2)
#define PINID_LCD_D03		PINID_ENCODE(1, 3)
#define PINID_LCD_D04		PINID_ENCODE(1, 4)
#define PINID_LCD_D05		PINID_ENCODE(1, 5)
#define PINID_LCD_D06		PINID_ENCODE(1, 6)
#define PINID_LCD_D07		PINID_ENCODE(1, 7)
#define PINID_LCD_D08		PINID_ENCODE(1, 8)
#define PINID_LCD_D09		PINID_ENCODE(1, 9)
#define PINID_LCD_D10		PINID_ENCODE(1, 10)
#define PINID_LCD_D11		PINID_ENCODE(1, 11)
#define PINID_LCD_D12		PINID_ENCODE(1, 12)
#define PINID_LCD_D13		PINID_ENCODE(1, 13)
#define PINID_LCD_D14		PINID_ENCODE(1, 14)
#define PINID_LCD_D15		PINID_ENCODE(1, 15)
#define PINID_LCD_D16		PINID_ENCODE(1, 16)
#define PINID_LCD_D17		PINID_ENCODE(1, 17)
#define PINID_LCD_D18		PINID_ENCODE(1, 18)
#define PINID_LCD_D19		PINID_ENCODE(1, 19)
#define PINID_LCD_D20		PINID_ENCODE(1, 20)
#define PINID_LCD_D21		PINID_ENCODE(1, 21)
#define PINID_LCD_D22		PINID_ENCODE(1, 22)
#define PINID_LCD_D23		PINID_ENCODE(1, 23)
#define PINID_LCD_RD_E		PINID_ENCODE(1, 24)
#define PINID_LCD_WR_RWN	PINID_ENCODE(1, 25)
#define PINID_LCD_RS		PINID_ENCODE(1, 26)
#define PINID_LCD_CS		PINID_ENCODE(1, 27)
#define PINID_LCD_VSYNC		PINID_ENCODE(1, 28)
#define PINID_LCD_HSYNC		PINID_ENCODE(1, 29)
#define PINID_LCD_DOTCK		PINID_ENCODE(1, 30)
#define PINID_LCD_ENABLE	PINID_ENCODE(1, 31)

/* Bank 2 */
#define PINID_SSP0_DATA0	PINID_ENCODE(2, 0)
#define PINID_SSP0_DATA1	PINID_ENCODE(2, 1)
#define PINID_SSP0_DATA2	PINID_ENCODE(2, 2)
#define PINID_SSP0_DATA3	PINID_ENCODE(2, 3)
#define PINID_SSP0_DATA4	PINID_ENCODE(2, 4)
#define PINID_SSP0_DATA5	PINID_ENCODE(2, 5)
#define PINID_SSP0_DATA6	PINID_ENCODE(2, 6)
#define PINID_SSP0_DATA7	PINID_ENCODE(2, 7)
#define PINID_SSP0_CMD		PINID_ENCODE(2, 8)
#define PINID_SSP0_DETECT	PINID_ENCODE(2, 9)
#define PINID_SSP0_SCK		PINID_ENCODE(2, 10)
#define PINID_SSP1_SCK		PINID_ENCODE(2, 12)
#define PINID_SSP1_DATA3	PINID_ENCODE(2, 15)
#define PINID_SSP2_SCK		PINID_ENCODE(2, 16)
#define PINID_SSP2_MOSI		PINID_ENCODE(2, 17)
#define PINID_SSP2_MISO		PINID_ENCODE(2, 18)
#define PINID_SSP2_SS0		PINID_ENCODE(2, 19)
#define PINID_SSP2_SS1		PINID_ENCODE(2, 20)
#define PINID_SSP2_SS2		PINID_ENCODE(2, 21)
#define PINID_SSP3_SCK		PINID_ENCODE(2, 24)
#define PINID_SSP3_MOSI		PINID_ENCODE(2, 25)
#define PINID_SSP3_MISO		PINID_ENCODE(2, 26)
#define PINID_SSP3_SS0		PINID_ENCODE(2, 27)

/* Bank 3 */
#define PINID_AUART0_RX		PINID_ENCODE(3, 0)
#define PINID_AUART0_TX		PINID_ENCODE(3, 1)
#define PINID_AUART0_CTS	PINID_ENCODE(3, 2)
#define PINID_AUART0_RTS	PINID_ENCODE(3, 3)
#define PINID_AUART1_RX		PINID_ENCODE(3, 4)
#define PINID_AUART1_TX		PINID_ENCODE(3, 5)
#define PINID_AUART1_CTS	PINID_ENCODE(3, 6)
#define PINID_AUART1_RTS	PINID_ENCODE(3, 7)
#define PINID_AUART2_RX		PINID_ENCODE(3, 8)
#define PINID_AUART2_TX		PINID_ENCODE(3, 9)
#define PINID_AUART2_CTS	PINID_ENCODE(3, 10)
#define PINID_AUART2_RTS	PINID_ENCODE(3, 11)
#define PINID_AUART3_RX		PINID_ENCODE(3, 12)
#define PINID_AUART3_TX		PINID_ENCODE(3, 13)
#define PINID_AUART3_CTS	PINID_ENCODE(3, 14)
#define PINID_AUART3_RTS	PINID_ENCODE(3, 15)
#define PINID_PWM0		PINID_ENCODE(3, 16)
#define PINID_PWM1		PINID_ENCODE(3, 17)
#define PINID_PWM2		PINID_ENCODE(3, 18)
#define PINID_SAIF0_MCLK	PINID_ENCODE(3, 20)
#define PINID_SAIF0_LRCLK	PINID_ENCODE(3, 21)
#define PINID_SAIF0_BITCLK	PINID_ENCODE(3, 22)
#define PINID_SAIF0_SDATA0	PINID_ENCODE(3, 23)
#define PINID_I2C0_SCL		PINID_ENCODE(3, 24)
#define PINID_I2C0_SDA		PINID_ENCODE(3, 25)
#define PINID_SAIF1_SDATA0	PINID_ENCODE(3, 26)
#define PINID_SPDIF		PINID_ENCODE(3, 27)
#define PINID_PWM3		PINID_ENCODE(3, 28)
#define PINID_PWM4		PINID_ENCODE(3, 29)
#define PINID_LCD_RESET		PINID_ENCODE(3, 30)

/* Bank 4 */
#define PINID_ENET0_MDC		PINID_ENCODE(4, 0)
#define PINID_ENET0_MDIO	PINID_ENCODE(4, 1)
#define PINID_ENET0_RX_EN	PINID_ENCODE(4, 2)
#define PINID_ENET0_RXD0	PINID_ENCODE(4, 3)
#define PINID_ENET0_RXD1	PINID_ENCODE(4, 4)
#define PINID_ENET0_TX_CLK	PINID_ENCODE(4, 5)
#define PINID_ENET0_TX_EN	PINID_ENCODE(4, 6)
#define PINID_ENET0_TXD0	PINID_ENCODE(4, 7)
#define PINID_ENET0_TXD1	PINID_ENCODE(4, 8)
#define PINID_ENET0_RXD2	PINID_ENCODE(4, 9)
#define PINID_ENET0_RXD3	PINID_ENCODE(4, 10)
#define PINID_ENET0_TXD2	PINID_ENCODE(4, 11)
#define PINID_ENET0_TXD3	PINID_ENCODE(4, 12)
#define PINID_ENET0_RX_CLK	PINID_ENCODE(4, 13)
#define PINID_ENET0_COL		PINID_ENCODE(4, 14)
#define PINID_ENET0_CRS		PINID_ENCODE(4, 15)
#define PINID_ENET_CLK		PINID_ENCODE(4, 16)
#define PINID_JTAG_RTCK		PINID_ENCODE(4, 20)

#endif
