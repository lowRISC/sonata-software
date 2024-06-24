// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include "lcd.hh"
#include <utility>

template<typename T>
using Cap = CHERI::Capability<T>;

using namespace sonata::lcd;

/**
 * Helper.  Returns a pointer to the SPI device.
 */
[[nodiscard, gnu::always_inline]] static Cap<volatile SonataSpi> spi()
{
	return MMIO_CAPABILITY(SonataSpi, spi1);
}

/**
 * Helper.  Returns a pointer to the GPIO device.
 */
[[nodiscard, gnu::always_inline]] static Cap<volatile SonataGPIO> gpio()
{
	return MMIO_CAPABILITY(SonataGPIO, gpio);
}

static constexpr uint32_t LcdCsPin  = 0;
static constexpr uint32_t LcdRstPin = 1;
static constexpr uint32_t LcdDcPin  = 2;
static constexpr uint32_t LcdBlPin  = 3;

static inline void set_gpio_output_bit(uint32_t bit, bool value)
{
	uint32_t output = gpio()->output;
	output &= ~(1 << bit);
	output |= value << bit;
	gpio()->output = output;
}

namespace sonata::lcd::internal
{
	void __cheri_libcall lcd_init(LCD_Interface *lcdIntf, St7735Context *ctx)
	{
		// Set the initial state of the LCD control pins.
		set_gpio_output_bit(LcdDcPin, false);
		set_gpio_output_bit(LcdBlPin, true);
		set_gpio_output_bit(LcdCsPin, false);

		// Initialise SPI driver.
		spi()->init(false, false, true, false);

		// Reset LCD.
		set_gpio_output_bit(LcdRstPin, false);
		thread_millisecond_wait(150);
		set_gpio_output_bit(LcdRstPin, true);

		// Initialise LCD driverr.
		lcdIntf->handle = nullptr;
		lcdIntf->spi_write =
		  [](void *handle, uint8_t *data, size_t len) -> uint32_t {
			spi()->blocking_write(data, len);
			return len;
		};
		lcdIntf->gpio_write =
		  [](void *handle, bool csHigh, bool dcHigh) -> uint32_t {
			set_gpio_output_bit(LcdCsPin, csHigh);
			set_gpio_output_bit(LcdDcPin, dcHigh);
			return 0;
		};
		lcdIntf->timer_delay = [](uint32_t ms) { thread_millisecond_wait(ms); };
		lcd_st7735_init(ctx, lcdIntf);

		// Set the LCD orentiation.
		lcd_st7735_set_orientation(ctx, LCD_Rotate180);

		lcd_st7735_clean(ctx);
	}
	void __cheri_libcall lcd_destroy(LCD_Interface *lcdIntf, St7735Context *ctx)
	{
		lcd_st7735_clean(ctx);
		// Hold LCD in reset.
		set_gpio_output_bit(LcdRstPin, false);
		// Turn off backlight.
		set_gpio_output_bit(LcdBlPin, false);
	}
} // namespace sonata::lcd::internal

void __cheri_libcall SonataLcd::clean()
{
	// Clean the display with a white rectangle.
	lcd_st7735_clean(&ctx);
}

void __cheri_libcall SonataLcd::clean(Color color)
{
	// Clean the display with a rectangle of the given colour
	size_t w, h;
	lcd_st7735_get_resolution(&ctx, &h, &w);
	lcd_st7735_fill_rectangle(
	  &ctx,
	  {.origin = {.x = 0, .y = 0}, .width = w, .height = h},
	  static_cast<uint32_t>(color));
}

void __cheri_libcall SonataLcd::draw_image_rgb565(Rect           rect,
                                                  const uint8_t *data)
{
	internal::lcd_st7735_draw_rgb565(
	  &ctx,
	  {{rect.left, rect.top}, rect.right - rect.left, rect.bottom - rect.top},
	  data);
}

void __cheri_libcall SonataLcd::draw_str(Point       point,
                                         const char *str,
                                         Color       background,
                                         Color       foreground)
{
	lcd_st7735_set_font(&ctx, &internal::m3x6_16ptFont);
	lcd_st7735_set_font_colors(&ctx,
	                           static_cast<uint32_t>(background),
	                           static_cast<uint32_t>(foreground));
	lcd_st7735_puts(&ctx, {point.x, point.y}, str);
}

void __cheri_libcall SonataLcd::draw_pixel(Point point, Color color)
{
	lcd_st7735_draw_pixel(
	  &ctx, {point.x, point.y}, static_cast<uint32_t>(color));
}

void __cheri_libcall SonataLcd::draw_line(Point a, Point b, Color color)
{
	if (a.y == b.y)
	{
		uint32_t x1 = std::min(a.x, b.x);
		uint32_t x2 = std::max(a.x, b.x);
		lcd_st7735_draw_horizontal_line(
		  &ctx, {{x1, a.y}, x2 - x1}, static_cast<uint32_t>(color));
	}
	else if (a.x == b.x)
	{
		uint32_t y1 = std::min(a.y, b.y);
		uint32_t y2 = std::max(a.y, b.y);
		lcd_st7735_draw_vertical_line(
		  &ctx, {{a.x, y1}, y2 - y1}, static_cast<uint32_t>(color));
	}
	else
	{
		// We currently only support horizontal and vertical lines.
		panic();
	}
}

void __cheri_libcall SonataLcd::draw_image_bgr(Rect rect, const uint8_t *data)
{
	lcd_st7735_draw_bgr(
	  &ctx,
	  {{rect.left, rect.top}, rect.right - rect.left, rect.bottom - rect.top},
	  data);
}

void __cheri_libcall SonataLcd::fill_rect(Rect rect, Color color)
{
	lcd_st7735_fill_rectangle(
	  &ctx,
	  {{rect.left, rect.top}, rect.right - rect.left, rect.bottom - rect.top},
	  static_cast<uint32_t>(color));
}
