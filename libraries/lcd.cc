// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include "lcd.hh"
#include <utility>

template<typename T>
using Cap = CHERI::Capability<T>;

using namespace sonata::lcd;
using LcdPwm = SonataPulseWidthModulation::LcdBacklight;
using LcdSpi = SonataSpi::Lcd;

/**
 * Helper. Returns a pointer to the SPI device.
 */
[[nodiscard, gnu::always_inline]] static Cap<volatile LcdSpi> spi()
{
	return MMIO_CAPABILITY(LcdSpi, spi_lcd);
}

/**
 * Helper. Returns a pointer to the LCD's backlight PWM device.
 */
[[nodiscard, gnu::always_inline]] static Cap<volatile LcdPwm> pwm_bl()
{
	return MMIO_CAPABILITY(LcdPwm, pwm_lcd);
}

static constexpr uint8_t LcdCsPin  = 0;
static constexpr uint8_t LcdDcPin  = 1;
static constexpr uint8_t LcdRstPin = 2;

/**
 * Sets the value of a specific Chip Select of SPI1. These Chip Select
 * lines are used to control the LCD's Chip Select, Reset, DC, and
 * Backlight, in place of e.g. GPIO pins.
 */
void set_chip_select(uint8_t chipSelect, bool value)
{
	const uint32_t CsBit = (1u << chipSelect);
	spi()->chipSelects =
	  value ? (spi()->chipSelects | CsBit) : (spi()->chipSelects & ~CsBit);
}
namespace sonata::lcd::internal
{
	void __cheri_libcall lcd_init(LCD_Interface  *lcdIntf,
	                              St7735Context  *ctx,
	                              LCD_Orientation rot)
	{
		// Set the initial state of the LCD control pins.
		set_chip_select(LcdDcPin, false);
		pwm_bl()->output_set(/*period=*/1, /*duty_cycle=*/255);
		set_chip_select(LcdCsPin, false);

		// Initialise SPI driver.
		spi()->init(false, false, true, false);

		// Reset LCD.
		set_chip_select(LcdRstPin, false);
		thread_millisecond_wait(150);
		set_chip_select(LcdRstPin, true);

		// Initialise LCD driverr.
		lcdIntf->handle = nullptr;
		lcdIntf->spi_write =
		  [](void *handle, uint8_t *data, size_t len) -> uint32_t {
			spi()->blocking_write(data, len);
			return len;
		};
		lcdIntf->gpio_write =
		  [](void *handle, bool csHigh, bool dcHigh) -> uint32_t {
			set_chip_select(LcdCsPin, csHigh);
			set_chip_select(LcdDcPin, dcHigh);
			return 0;
		};
		lcdIntf->timer_delay = [](void *handle, uint32_t ms) {
			thread_millisecond_wait(ms);
		};
		lcd_st7735_init(ctx, lcdIntf);

		lcd_st7735_startup(ctx);

		// Set the LCD orentiation.
		lcd_st7735_set_orientation(ctx, rot);

		lcd_st7735_clean(ctx);
	}
	void __cheri_libcall lcd_destroy(LCD_Interface *lcdIntf, St7735Context *ctx)
	{
		lcd_st7735_clean(ctx);
		// Hold LCD in reset.
		set_chip_select(LcdRstPin, false);
		// Turn off backlight.
		pwm_bl()->output_set(/*period=*/0, /*duty_cycle=*/0);
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
	SonataLcd::draw_str(point, str, background, foreground, Font::M3x6_16pt);
}

void __cheri_libcall SonataLcd::draw_str(Point       point,
                                         const char *str,
                                         Color       background,
                                         Color       foreground,
                                         Font        font)
{
	const internal::Font *internalFont;
	switch (font)
	{
		case Font::LucidaConsole_10pt:
			internalFont = &internal::lucidaConsole_10ptFont;
			break;
		case Font::LucidaConsole_12pt:
			internalFont = &internal::lucidaConsole_12ptFont;
			break;
		default:
			internalFont = &internal::m3x6_16ptFont;
	}
	lcd_st7735_set_font(&ctx, internalFont);
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
