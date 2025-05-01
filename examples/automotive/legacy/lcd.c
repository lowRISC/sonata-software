// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "lcd.h"

#include "../../../third_party/display_drivers/src/core/lucida_console_10pt.h"
#include "../../../third_party/display_drivers/src/core/m3x6_16pt.h"
#include "../../../third_party/display_drivers/src/st7735/lcd_st7735.h"
#include "../../../third_party/sonata-system/sw/legacy/common/gpio.h"
#include "../../../third_party/sonata-system/sw/legacy/common/sonata_system.h"
#include "../../../third_party/sonata-system/sw/legacy/common/spi.h"
#include "../../../third_party/sonata-system/sw/legacy/common/timer.h"

// Define our own GPIO_OUT as the version from `sonata-system` uses void
// pointer arithmetic, which clang-tidy forbids.
#define GPIO_OUT_LCD GPIO_FROM_BASE_ADDR((GPIO_BASE + GPIO_OUT_REG))

static void timer_delay(void *handle, uint32_t ms)
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
	timer_delay(NULL, 150);
	set_output_bit(GPIO_OUT_LCD, LcdRstPin, 0x1);

	// Init LCD Driver, and set the SPI driver
	interface->handle      = spi;         // SPI handle.
	interface->spi_write   = spi_write;   // SPI write callback.
	interface->gpio_write  = gpio_write;  // GPIO write callback.
	interface->timer_delay = timer_delay; // Timer delay callback.
	lcd_st7735_init(lcd, interface);
	lcd_st7735_startup(lcd);

	// Set the LCD orientation
	lcd_st7735_set_orientation(lcd, LCD_Rotate180);

	// Setup text font bitmaps to be used and the colors.
	lcd_st7735_set_font(lcd, &m3x6_16ptFont);
	lcd_st7735_set_font_colors(lcd, BGRColorWhite, BGRColorBlack);

	// Clean display with a white rectangle.
	lcd_st7735_clean(lcd);

	return 0;
}
