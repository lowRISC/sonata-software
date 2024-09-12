// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#ifndef LCD_H
#define LCD_H

#include "../../../third_party/display_drivers/st7735/lcd_st7735.h"
#include "../../../third_party/sonata-system/sw/legacy/common/spi.h"

// Constants.
enum
{
	// Pin out mapping.
	LcdCsPin = 0,
	LcdRstPin,
	LcdDcPin,
	LcdBlPin,
	LcdMosiPin,
	LcdSclkPin,
	// Spi clock rate.
	LcdSpiSpeedHz = 5 * 100 * 1000,
};

enum
{
	BGRColorBlack = 0x000000,
	BGRColorBlue  = 0xFF0000,
	BGRColorGreen = 0x00FF00,
	BGRColorRed   = 0x0000FF,
	BGRColorWhite = 0xFFFFFF,
};

int lcd_init(spi_t *spi, St7735Context *lcd, LCD_Interface *interface);

#endif // LCD_H
