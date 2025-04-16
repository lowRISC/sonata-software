// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "lcd.h"

#include <gpio.h>
#include <lucida_console_10pt.h>
#include <lucida_console_12pt.h>
#include <m3x6_16pt.h>
#include <sonata_system.h>
#include <spi.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <timer.h>

// Define our own GPIO_OUT as the version from `sonata-system` uses void
// pointer arithmetic, which clang-tidy forbids.
#define GPIO_OUT_LCD GPIO_FROM_BASE_ADDR((GPIO_BASE + GPIO_OUT_REG))

static void timer_delay(uint32_t ms)
{
	uint32_t timeout = get_elapsed_time() + ms;
	while (get_elapsed_time() < timeout)
	{
		__asm__ volatile("wfi");
	}
}

static uint32_t spi_write(void *handle, uint8_t *data, size_t len)
{
	spi_tx((spi_t *)handle, data, len);
	spi_wait_idle((spi_t *)handle);
	return len;
}

static uint32_t gpio_write(void *handle, bool cs, bool dc)
{
	set_output_bit(GPIO_OUT_LCD, LcdDcPin, dc);
	set_output_bit(GPIO_OUT_LCD, LcdCsPin, cs);
	return 0;
}

int lcd_init(spi_t *spi, St7735Context *lcd, LCD_Interface *interface)
{
	// Set the initial state of the LCD control pins
	set_output_bit(GPIO_OUT_LCD, LcdDcPin, 0x0);
	set_output_bit(GPIO_OUT_LCD, LcdBlPin, 0x1);
	set_output_bit(GPIO_OUT_LCD, LcdCsPin, 0x0);

	// Reset the LCD
	set_output_bit(GPIO_OUT_LCD, LcdRstPin, 0x0);
	timer_delay(150);
	set_output_bit(GPIO_OUT_LCD, LcdRstPin, 0x1);

	// Init LCD Driver, and set the SPI driver
	interface->handle      = spi;         // SPI handle.
	interface->spi_write   = spi_write;   // SPI write callback.
	interface->gpio_write  = gpio_write;  // GPIO write callback.
	interface->timer_delay = timer_delay; // Timer delay callback.
	lcd_st7735_init(lcd, interface);

	// Set the LCD orientation
	lcd_st7735_set_orientation(lcd, LCD_Rotate180);

	// Setup text font bitmaps to be used and the colors.
	lcd_st7735_set_font(lcd, &m3x6_16ptFont);
	lcd_st7735_set_font_colors(lcd, BGRColorWhite, BGRColorBlack);

	// Clean display with a white rectangle.
	lcd_st7735_clean(lcd);

	return 0;
}

void lcd_draw_str(St7735Context *lcd,
                  uint32_t       x,
                  uint32_t       y,
                  LcdFont        font,
                  const char    *format,
                  uint32_t       backgroundColour,
                  uint32_t       textColour,
                  ...)
{
	// Format the provided string
	char    buffer[1024];
	va_list args;
	va_start(args, textColour);
	vsnprintf(buffer, 1024, format, args);
	va_end(args);

	Font stringFont;
	switch (font)
	{
		case LcdFontLucidaConsole_10pt:
			stringFont = lucidaConsole_10ptFont;
			break;
		case LcdFontLucidaConsole_12pt:
			stringFont = lucidaConsole_12ptFont;
			break;
		default:
			stringFont = m3x6_16ptFont;
	}
	lcd_st7735_set_font(lcd, &stringFont);
	lcd_st7735_set_font_colors(lcd, backgroundColour, textColour);
	lcd_st7735_puts(lcd, (LCD_Point){x, y}, buffer);
}

void lcd_clean(St7735Context *lcd, uint32_t color)
{
	size_t w, h;
	lcd_st7735_get_resolution(lcd, &h, &w);
	LCD_rectangle rect = {(LCD_Point){0, 0}, w, h};
	lcd_st7735_fill_rectangle(lcd, rect, color);
}

void lcd_fill_rect(St7735Context *lcd,
                   uint32_t       x,
                   uint32_t       y,
                   uint32_t       w,
                   uint32_t       h,
                   uint32_t       color)
{
	LCD_rectangle rect = {(LCD_Point){x, y}, w, h};
	lcd_st7735_fill_rectangle(lcd, rect, color);
}

void lcd_draw_img(St7735Context *lcd,
                  uint32_t       x,
                  uint32_t       y,
                  uint32_t       w,
                  uint32_t       h,
                  const uint8_t *data)
{
	LCD_rectangle rect = {(LCD_Point){x, y}, w, h};
	lcd_st7735_draw_rgb565(lcd, rect, data);
}
