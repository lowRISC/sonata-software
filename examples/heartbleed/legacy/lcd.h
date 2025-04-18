// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#ifndef _HEARTBLEED_LCD_H_
#define _HEARTBLEED_LCD_H_

#include <lcd_st7735.h>
#include <spi.h>

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

	// Constants.
	enum
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

	enum
	{
		BGRColorBlack = 0x000000,
		BGRColorWhite = 0xFFFFFF,
		BGRColorBlue  = 0xFF0000,
		BGRColorGreen = 0x00FF00,
		BGRColorRed   = 0x0000FF,
	};

	// Fonts available for LCD rendering
	typedef enum LcdFont
	{
		LcdFontM3x6_16pt,          // NOLINT
		LcdFontLucidaConsole_10pt, // NOLINT
		LcdFontLucidaConsole_12pt, // NOLINT
	} LcdFont;

	/**
	 * @brief Initialise the LCD, starting and clearing the screen.
	 *
	 * @param spi contains an initialized SPI handle to use to write to the LCD.
	 * @param lcd is an outparam to store the initialized LCD handle.
	 * @param interface is an outparam storing the initialized LCD interface.
	 */
	int lcd_init(spi_t *spi, St7735Context *lcd, LCD_Interface *interface);

	/**
	 * @brief Formats and draws a string to the LCD display based upon the
	 * provided formatting and display information. The string can use
	 * formatting specifiers as defined by `vsnprintf(3)`.
	 *
	 * @param lcd a handle to the LCD to draw to.
	 * @param x is the X-coordinate on the LCD of the top left of the string.
	 * @param y is the Y-coordinate on the LCD of the top left of the string.
	 * @param font is the font to render the string with.
	 * @param format is the formatting string to write.
	 * @param backgroundColour is the colour to use for the string's background.
	 * @param textColour is the colour to render the text in.
	 * The remaining variable arguments are unsigned integers used to fill in
	 * the formatiting specifiers in the format string.
	 */
	void lcd_draw_str(St7735Context *lcd,
	                  uint32_t       x,
	                  uint32_t       y,
	                  LcdFont        font,
	                  const char    *format,
	                  uint32_t       backgroundColour,
	                  uint32_t       textColour,
	                  ...);

	/**
	 * @brief Clean the LCD display with a given colour.
	 *
	 * @param lcd a handle to the LCD to draw to
	 * @param color is the 32-bit colour to display.
	 */
	void lcd_clean(St7735Context *lcd, uint32_t color);

	/**
	 * @brief Draws a rectangle of a given colour on the LCD.
	 *
	 * @param lcd a handle to the LCD to draw to.
	 * @param x is the X-position of the top-left corner of the rectangle.
	 * @param y is the Y-position of the top-left corner of the rectangle.
	 * @param w is the width of the rectangle.
	 * @param h is the height of the rectangle.
	 * @param color is the 32-bit colour to fill the rectangle with.
	 */
	void lcd_fill_rect(St7735Context *lcd,
	                   uint32_t       x,
	                   uint32_t       y,
	                   uint32_t       w,
	                   uint32_t       h,
	                   uint32_t       color);
	/**
	 * @brief Draw an image to the LCD.
	 *
	 * @param lcd a handle to the LCD to draw to.
	 * @param x is the X-position of the top-left corner of the image on the
	 * LCD.
	 * @param y is the Y-position of the top-left corner of the image on the
	 * LCD.
	 * @param w is the width of the image.
	 * @param h is the height of the image.
	 * @param data is the byte array containing the image data, of size at least
	 * of `w` * `h`, where each value is a RGB565 colour value.
	 */
	void lcd_draw_img(St7735Context *lcd,
	                  uint32_t       x,
	                  uint32_t       y,
	                  uint32_t       w,
	                  uint32_t       h,
	                  const uint8_t *data);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _HEARTBLEED_LCD_H_
