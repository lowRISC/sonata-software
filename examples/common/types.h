// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#pragma once
enum LcdConstantes
{
	// Pin out mapping.
	LcdCsPin = 0,
	LcdDcPin,
	LcdRstPin,
	LcdMosiPin,
	LcdSclkPin,
	// Spi clock rate.
	LcdSpiSpeedHz = 5 * 100 * 1000,
};

enum RGBColors
{
	RGBColorBlack      = 0x000000,
	RGBColorWhite      = 0xFFFFFF,
	RGBColorBlue       = 0x0000FF,
	RGBColorGreen      = 0x00FF00,
	RGBColorRed        = 0xFF0000,
	RGBColorLightGrey  = 0xAAAAAA,
	RGBColorGrey       = 0xCCCCCC,
	RGBColorDarkGrey   = 0xA0A0A0,
	RGBColorDarkerGrey = 0x808080,
};

// Fonts available for LCD rendering
typedef enum
{
	M3x6_16pt,          // NOLINT
	M5x7_16pt,          // NOLINT
	LucidaConsole_10pt, // NOLINT
	LucidaConsole_12pt, // NOLINT
} LcdFont;
